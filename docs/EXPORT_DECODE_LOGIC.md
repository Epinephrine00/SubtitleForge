# 비디오 내보내기 — 디코딩/프레임 매핑 로직 설명

이 문서는 `videoexporter.cpp`에서 **원본 비디오를 디코딩해서 출력 프레임과 매핑하는** 부분의 로직을 단계별로 설명합니다.

---

## 1. 사전 준비 (루프 밖)

### 1.1 프로젝트/출력 상수
- `sourceFps` = 프로젝트 소스 FPS (예: 23.976)
- `outputFps` = 출력 영상 FPS (예: 60)
- `totalFrames` = 트림 구간의 “프로젝트 프레임 수” (trimEnd - trimStart)
- `outputFrameCount` = `totalFrames * outputFps / sourceFps` → 만들 출력 프레임 개수
- `trimStartF` = 트림 시작 프레임 번호 (프로젝트 기준)

### 1.2 소스 파일 열기 & 디코더 초기화
- `avformat_open_input` → 컨테이너 열기
- 비디오 스트림 인덱스 `videoStreamIdx` 찾기
- 스트림에서 **실제 FPS** 읽기 → `decodeFps` (avg_frame_rate / r_frame_rate, 없으면 sourceFps)
- 디코더 생성: `avcodec_alloc_context3`, `avcodec_parameters_to_context`, `avcodec_open2`
- `decFrame`, `readPkt`, `videoSws`(스케일러) 할당

### 1.3 Seek 및 디코더 플러시
- **Seek**: `seekTs = (trimStartF / decodeFps) * AV_TIME_BASE` (마이크로초),  
  `av_seek_frame(videoFmt, -1, seekTs, AVSEEK_FLAG_BACKWARD)`  
  → 트림 시작 시점 **이전** 키프레임으로 감
- 성공 시: `trimStartSec = trimStartF / decodeFps`
- **플러시**: `avcodec_send_packet(videoDec, nullptr)` 후  
  `while (avcodec_receive_frame(...) == 0) { }` 로 디코더 내부 버퍼 비움  
  → seek 이전에 있던 프레임들을 버리는 목적

이후 **출력 프레임 루프**로 들어갑니다.

---

## 2. 출력 프레임 루프 (for outF = 0 .. outputFrameCount-1)

각 `outF`(출력 프레임 인덱스)마다:

### 2.1 “필요한 소스 프레임 인덱스” 계산
- `needIdx` = “이 출력 프레임에 대응하는, 트림 구간 내 **디코드 타임라인 상의 프레임 번호**”
  - `srcFrame = (outF * decodeFps) / outputFps`
  - `needIdx = clamp(srcFrame, 0, totalFrames-1)`
- 자막/폴백용으로 **프로젝트 타임라인** 인덱스도 따로 계산:
  - `srcFrameProject = (outF * sourceFps) / outputFps` (같이 clamp)

즉, **출력 60fps**와 **소스 23.976fps**를 맞추기 위해 “지금 이 출력 프레임은 소스의 몇 번째 프레임에 해당하는가”를 `needIdx`로 씁니다.

### 2.2 순차 디코딩 분기 (useSeq일 때만)

`useSeq && videoFmt && videoDec && ...` 일 때만 아래 **while**을 탑니다.

- **맵**: `frameByPtsIndex` = (프레임 인덱스 → QImage)
  - 키 = `presentationIdx` (디코딩된 프레임의 “트림 구간 내 순번”)
  - 값 = 그 프레임을 RGBA로 스케일한 이미지

목표: **`needIdx`에 해당하는 프레임이 맵에 있을 때까지** 패킷을 읽고 디코딩해서 맵에 채움.

---

## 3. “needIdx가 나올 때까지 디코딩” while 루프

```text
while (frameByPtsIndex.find(needIdx) == frameByPtsIndex.end()) {
```

### 3.1 패킷 읽기
- `pendingPkt == false` 일 때만:
  - `av_read_frame(videoFmt, readPkt)`
  - **반환값 < 0** (EOF/에러) → **break** → while 종료, 맵 비어 있을 수 있음
  - `readPkt->stream_index != videoStreamIdx` → 비디오 패킷이 아니면 unref 하고 **continue** (다시 읽기)

비디오 패킷이면 `pendingPkt = true` 로 두고 아래로 진행.

### 3.2 디코더에 패킷 넣기
- `avcodec_send_packet(videoDec, readPkt)`
  - **0** → 패킷 소비됨 → `pendingPkt = false`, unref
  - **EAGAIN** → 디코더 버퍼 가득 → unref 하지 않고, 아래에서 receive로 프레임 뽑음
  - **그 외 에러** → unref 하고 **break** → while 종료 (맵 비어 있음)

즉, **send_packet이 0/EAGAIN이 아닌 에러를 반환하면 여기서 while가 끝나고**,  
`av_read_frame_fail` / `receive_frame_ok` 로그 없이 바로 lookup(mapSize 0)으로 이어질 수 있음.

### 3.3 디코딩된 프레임 받기
- `while (avcodec_receive_frame(videoDec, decFrame) == 0)`  
  - **0이 아닌 값** (EAGAIN 등)이면 이 안쪽 while는 한 번도 안 돌 수 있음.

한 번이라도 **0**이 나오면:
- `decFrame->width/height <= 0` 이면 스킵 (continue).
- `ptsSec = decFrame->pts * tb` (스트림 time_base 기준 초).
- **세그먼트 시작 PTS**:  
  첫 프레임에서 `segmentStartPtsSec = ptsSec`, `segmentStartSet = true`.
- `presentationIdx = (ptsSec - segmentStartPtsSec) * decodeFps` (반올림).
- `presentationIdx < 0` 이면 스킵.
- 그 외: 스케일해서 `frameByPtsIndex[presentationIdx] = im.copy()` 로 **저장**.
- `frameByPtsIndex.find(needIdx) != end()` 이면 **break** (목표 프레임 찾음).

정리하면:
- **receive_frame이 한 번도 0을 안 주면** → `receive_frame_ok` / `decode_store` 로그가 전혀 안 나옴.
- **send_packet이 0/EAGAIN이 아니면** → 위 receive 단계까지 가지도 못하고 break → 역시 `receive_frame_ok` / `decode_store` 없음.

---

## 4. 맵에서 프레임 선택 (while 빠져 나온 뒤)

- `frameByPtsIndex.find(needIdx)` 로 **정확히 needIdx** 있는지 봄.
  - 있으면 → `videoFrame = it->second`, `keyUsed = needIdx`.
- 없으면:
  - `frameByPtsIndex.upper_bound(needIdx)` 로 “needIdx보다 큰 최소 키” 찾고,
  - 그 이전 키가 있으면 그 프레임을 사용 (`keyUsed = 그 키`).
- 그 다음 **메모리 정리**:
  - `nextNeedIdx = (outF + 1) * decodeFps / outputFps`
  - `frameByPtsIndex` 에서 키가 `nextNeedIdx` 미만인 것들 **전부 erase**.

즉, “다음 출력 프레임에 더 이상 필요 없는” 과거 인덱스는 지워서 맵 크기를 유지합니다.

---

## 5. 폴백 (맵에서 못 찾았을 때)

- `videoFrame`이 비어 있고 `srcPath`가 있으면:
  - `timeSec = project.trimmedToSourceFrame(srcFrameProject) / sourceFps`
  - `videoFrame = VideoDecoder::frameAt(srcPath, timeSec)`  
  → **다른 경로(VideoDecoder)** 로 해당 시간의 프레임을 가져옴.

지금 로그처럼 **mapSize가 항상 0**이면, 매 outF마다 이 폴백만 타게 됩니다.

---

## 6. 이후 공통
- `videoFrame`(디코딩 또는 폴백)으로 자막 렌더 → 인코딩 → 파일에 기록.

---

## 7. 현재 로그로 보이는 현상 정리

- **decode_while_enter** 만 2번 찍히고, **av_read_frame_fail / receive_frame_ok / decode_store** 는 전혀 없음.
- 그 직후 **lookup** 에서 `mapSize == 0`, `foundExact == false`, `keyUsed == -1`.

가능한 원인:

1. **avcodec_send_packet이 0/EAGAIN이 아닌 에러를 반환**  
   - Seek 직후 첫 비디오 패킷이 “키프레임 이전”이거나, 디코더가 받을 수 없는 상태일 수 있음.  
   - 그럴 때 현재 코드는 **break** 해 버려서 while가 끝나고, receive_frame을 한 번도 호출하지 않음.
2. 그 결과 **receive_frame이 0을 한 번도 반환하지 않음** → `receive_frame_ok` / `decode_store` 없음.
3. **항상 폴백** `VideoDecoder::frameAt` 만 사용 → 같은 이미지가 반복되는 현상과 연결될 수 있음.

따라서 **send_packet 에러 시 break 하지 말고, 해당 패킷만 버리고 다음 패킷을 읽어서 계속 시도**하도록 바꾸는 것이 다음 수정 후보입니다.
