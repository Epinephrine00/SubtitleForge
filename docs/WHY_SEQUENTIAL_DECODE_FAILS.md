# 순차 디코딩이 왜 동작하지 않는가

## 기대하는 흐름 (가장 단순한 로직)

1. Seek로 트림 시작 키프레임으로 이동
2. 플러시로 디코더 비움
3. `av_read_frame` → 비디오 패킷만 골라서
4. `avcodec_send_packet` → 디코더에 넣고
5. `avcodec_receive_frame` 반복 → 나온 프레임을 presentationIdx로 맵에 저장
6. needIdx가 맵에 있으면 그걸 쓰고, 없으면 3–5 반복

이렇게만 되면 순차 디코딩은 동작해야 한다.

## 실제로 막히는 지점

로그상으로는:

- **decode_while_enter** 는 찍힘 → while에는 들어감
- **decode_store / receive_frame_ok** 는 한 번도 안 찍힘 → `receive_frame`이 0을 한 번도 안 돌려줌
- **av_read_frame_fail** 도 (첫 outF에서) 안 보이면 → 패킷은 계속 읽고 있음

그래서 **“패킷은 읽고 보내는데, 디코더가 프레임을 한 번도 안 준다”** 상태로 끝난다.

## 가능한 원인 (코드가 그렇게 짜여 있을 수 있는 부분)

1. **send_packet이 0을 안 주고 전부 에러**
   - Seek 직후 첫 패킷이 키프레임이어도, 컨테이너/코덱에 따라 `AVERROR_INVALIDDATA` 등으로 거절될 수 있음
   - 우리는 에러 시 패킷만 스킵하고 break는 안 함 → 다음 패킷을 보내지만, **첫 키프레임을 스킵해 버리면** 이후 P/B만 보내게 되고 전부 실패할 수 있음
   - 즉 “에러 나면 스킵”이 오히려 **키프레임까지 스킵**하게 만든 코드일 수 있음

2. **send_packet은 0인데 receive_frame이 계속 EAGAIN**
   - B-frame 등으로 디코더가 지연 출력하면, 패킷 하나만 넣고 receive를 한 번만 부르면 EAGAIN만 나올 수 있음
   - 그런데 우리는 **receive를 while로 여러 번** 부르므로, 이론상으로는 프레임이 나올 때까지 루프 가능
   - 실제로는 “같은 패킷을 계속 보내는” 쪽에 가까운지(아래 3번)가 중요

3. **EAGAIN 처리 때문에 같은 패킷만 반복**
   - `send_packet`이 EAGAIN이면 패킷을 unref 하지 않고 `pendingPkt = true` 유지
   - 다음 턴에 **새 패킷을 읽지 않고** 같은 패킷을 다시 send
   - 디코더가 “이미 가득 찼다”고 해서 계속 EAGAIN만 주면, **같은 패킷만 반복 전송**하는 루프에 갇힐 수 있음
   - 그런 경우 새 패킷을 읽지 않으므로, receive로 새 프레임이 나올 경로가 없음

4. **키프레임만 받기 때문에 첫 키프레임을 못 만남**
   - `frameByPtsIndex.empty()`일 때 `AV_PKT_FLAG_KEY`가 아닌 패킷은 전부 스킵
   - 파일/컨테이너에 따라 **키프레임 플래그가 안 붙어 있거나**, seek 직후 첫 비디오 패킷이 키프레임이 아닐 수 있음
   - 그러면 “키프레임이 나올 때까지” 계속 스킵하다가 EOF까지 가서, **한 번도 send를 안 하고** 루프가 끝날 수 있음

정리하면, **순차 작업 자체가 어려운 게 아니라**,  
에러 스킵 / EAGAIN 처리 / 키프레임만 받는 조건이 겹치면서  
“한 번도 유효한 프레임을 받지 못하는” 코드 경로로만 도는 상태로 짜여 있을 가능성이 크다.

## 이번에 한 수정

1. **videoexporter.cpp**
   - `send_packet` / `receive_frame` **반환값**을 로그로 남김  
     (`send_packet_ret`, `receive_frame_ret` – outF==0일 때 처음 몇 번만)
   - 다음 실행 시 로그를 보면:
     - send가 전부 에러인지, 0이 나오는지
     - receive가 전부 EAGAIN인지, 0이 나오는지  
     → **실제로 어디서 끊기는지** 확인 가능

2. **videodecoder.cpp (frameAt)**
   - 기존: seek 후 **첫 번째로 디코딩된 프레임 한 장**만 반환  
     → BACKWARD seek이면 항상 같은 키프레임만 나옴
   - 수정: seek 후 **디코딩한 프레임 중 `framePtsSec >= timeSec`인 첫 프레임**을 반환
   - 그래서 `timeSec`만 needIdx에 맞게 주면, **폴백 경로만으로도** needIdx마다 다른 프레임이 나오도록 함

## 다음에 할 일

1. 빌드 후 내보내기 한 번 돌리고 **.cursor/debug.log** 확인
2. `send_packet_ret`의 `sendRet` 값: 전부 음수(에러)인지, 0이 나오는지
3. `receive_frame_ret`의 `recvRet` 값: 전부 EAGAIN(-11)인지, 0이 나오는지
4. 그에 맞춰:
   - send가 전부 에러면 → 키프레임 스킵/에러 스킵 로직 재검토
   - receive가 전부 EAGAIN이면 → EAGAIN일 때 패킷 처리(같은 패킷 반복 전송 여부) 재검토

이렇게 하면 “순차 디코딩이 대체 왜 안 되게 짜여 있었는지”를 로그로 특정해서 고칠 수 있다.
