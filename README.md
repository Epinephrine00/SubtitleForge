# SubtitleForge

A cross-platform subtitle generator that creates chroma-key-ready subtitle videos from audio/video files.

> **This Project has Vibe-coded.**

## Features

- Load audio (mp3, wav, flac, aac, ogg) or video files (mp4, mkv, avi, mov)
- Audio waveform timeline with subtitle markers
- 5-stage subtitle animation system (Intro, Approach, Focus, Recede, Outro)
- Per-stage control: position, scale, blur, opacity, font size
- Real-time preview on #00FF00 green background
- Export to H.264 MP4 (720p / 1080p / 1440p / 2160p at 24/30/60 FPS)
- Project save/load (.sfproj JSON format)
- Multilingual text support (CJK, Latin, etc.)

## Dependencies

- **Qt 6** (Widgets, Multimedia)
- **FFmpeg** (libavformat, libavcodec, libavutil, libswresample, libswscale)
- **CMake** 3.20+
- **C++17** compiler

## Build

```bash
# macOS (Homebrew) — use build-mac/ so Windows build/ is not overwritten
brew install qt@6 ffmpeg cmake pkg-config
./build-mac.sh
# or manually:
# cmake -B build-mac -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
# cmake --build build-mac
# open build-mac/SubtitleForge.app

# Windows (vcpkg)
vcpkg install qt6 ffmpeg
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Linux
sudo apt install qt6-base-dev qt6-multimedia-dev libavformat-dev libavcodec-dev libavutil-dev libswresample-dev libswscale-dev
cmake -B build
cmake --build build
```

## Usage

1. **File > Import Audio/Video** to load a source file
2. The waveform appears on the timeline at the bottom
3. Press **Space** to play/pause, **Left/Right** arrows to step one frame
4. Type subtitle text in the top-left panel and press **Enter** to stamp it at the current time
5. Click a subtitle marker on the timeline to select and edit its text or effects
6. Configure animation stages in the Effects panel (top-right)
7. **File > Export Video** to render the final MP4

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Play / Pause |
| Left | Previous frame |
| Right | Next frame |
| Ctrl+Left | Back 1 second |
| Ctrl+Right | Forward 1 second |
| Ctrl+S | Save project |
| Ctrl+O | Open project |
| Delete | Remove selected subtitle |
