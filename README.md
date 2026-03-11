# ReelsForge — Instagram Reels automation

**Reels Forge** is a vertical (9:16) video tool for Instagram Reels: import video and TXT subtitles, add a title, trim, and export MP4 1080×1920 at 60fps with audio. Built for quick reel workflow with subtitle timing and styling.

> **This project has been vibe-coded.**

---

## Features

- **Import** video (mp4, mkv, avi, mov, webm) or audio
- **Import subtitles** from TXT (split by `\n\n`), one block per timeline marker
- **Global subtitle style** (font, size, color)
- **Video title** (multiline, own font/size/color), shown for full duration
- **Trim** (start/end in seconds)
- **Timeline** with waveform, current/total time, and subtitle markers
- **Preview**: center video (width-fit), blurred semi-transparent fill, black background
- **Export**: MP4 1080×1920 60fps with video + audio; subtitle timing aligned to source FPS

---

## Program structure

```
SubtitleForge/
├── CMakeLists.txt          # CMake project (builds executable SubtitleForge)
├── CMakePresets.json       # Presets (e.g. windows-vcpkg)
├── configure-win.ps1       # Windows configure
├── build-win.ps1           # Windows build
├── build-mac.sh            # macOS build
├── src/
│   ├── main.cpp            # Entry point, QApplication, app name "Reels Forge"
│   ├── app/
│   │   └── mainwindow.cpp/h # Main window, menus, import/export, project load/save
│   ├── model/
│   │   ├── project.cpp/h   # Project data (media path, trim, subtitles, title, style)
│   │   ├── subtitle.cpp/h # Subtitle entry and effect
│   │   ├── subtitlerenderer.cpp/h  # Draw title + subtitles on frame
│   │   └── animationengine.cpp     # Subtitle visibility/animation per frame
│   ├── video/
│   │   ├── videoexporter.cpp/h   # Export: encode video, copy audio, mux MP4
│   │   ├── videodecoder.cpp/h    # frameAt(), duration, frameRate (utility)
│   │   └── previewdecoder.cpp/h  # Preview playback decoding
│   ├── audio/
│   │   └── audioplayback.cpp/h   # Playback and waveform sync
│   └── widgets/
│       ├── timelinewidget.cpp/h # Timeline scrubber and subtitle markers
│       └── previewwidget.cpp/h  # Video + overlay preview
├── docs/
│   ├── EXPORT_DECODE_LOGIC.md
│   └── WHY_SEQUENTIAL_DECODE_FAILS.md
└── testdata/
    └── *.sfproj, *.txt     # Sample project and subtitles
```

---

## Dependencies

- **Qt 6** (Widgets, Multimedia)
- **FFmpeg** (libavformat, libavcodec, libavutil, libswresample, libswscale)
- **CMake** 3.20+
- **C++17** compiler

---

## Build

Project path: `reels-forge/SubtitleForge` (this folder).  
If you copied the project elsewhere, **delete the `build/` directory** and reconfigure so paths match.

### Windows

**Option A – CMake Presets (recommended)**

1. Install [vcpkg](https://vcpkg.io/) and set `VCPKG_ROOT` to the vcpkg root.
2. Install deps: `vcpkg install qt6 ffmpeg`
3. From this folder:
   ```powershell
   cmake --preset windows-vcpkg
   cmake --build build --config Release
   ```
4. Run: `.\build\Release\SubtitleForge.exe` (the app window title is **Reels Forge**).

**Option B – Scripts**

```powershell
.\configure-win.ps1
.\build-win.ps1
```

**Without vcpkg:** set Qt6 and FFmpeg via `CMAKE_PREFIX_PATH` and `FFMPEG_DIR` (or pkg-config on WSL).

### macOS

```bash
brew install qt@6 ffmpeg cmake pkg-config
./build-mac.sh
open build-mac/SubtitleForge.app
```

### Linux

```bash
sudo apt install qt6-base-dev qt6-multimedia-dev libavformat-dev libavcodec-dev libavutil-dev libswresample-dev libswscale-dev
cmake -B build
cmake --build build
./build/SubtitleForge
```

---

## Usage

1. **File → Import Video** to load a source file.
2. **File → Import TXT** to load subtitles (paragraphs separated by blank lines); they are spread evenly over the timeline.
3. Set **Trim** (start/end seconds) and **Subtitle style** (font, size, color).
4. Optionally set **Video title** (multiline, font, size, color); it is shown for the full duration.
5. Drag subtitle markers on the timeline to change when each line appears.
6. **File → Export Video** to render MP4 1080×1920 60fps (video + audio).

---

## Keyboard shortcuts

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
