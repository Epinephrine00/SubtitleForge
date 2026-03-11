# SubtitleForge

Vertical (9:16) subtitle video generator: import video + TXT subtitles, add title, trim, export MP4 1080×1920 60fps.

> **This Project has Vibe-coded.**

## Features

- Import video (mp4, mkv, avi, mov, webm) or audio
- Import subtitles from TXT (split by `\n\n`), one block per timeline marker
- Global subtitle style (font, size, color)
- Video title (multiline, own font/size/color), shown full duration
- Trim (start/end in seconds)
- Timeline with waveform, current/total time, subtitle markers
- Preview: center video (width-fit) + blurred semi-transparent fill + black background
- Export: MP4 1080×1920 60fps only

## Dependencies

- **Qt 6** (Widgets, Multimedia)
- **FFmpeg** (libavformat, libavcodec, libavutil, libswresample, libswscale)
- **CMake** 3.20+
- **C++17** compiler

## Build

Project path: `reels-forge/SubtitleForge` (this folder).  
If you copied the project from another folder, **delete the `build/` directory** and configure again so paths match this location.

### Windows

**Option A – CMake Presets (recommended)**

1. Install [vcpkg](https://vcpkg.io/) and set `VCPKG_ROOT` to the vcpkg root.
2. Install deps: `vcpkg install qt6 ffmpeg`
3. From this folder:
   ```powershell
   cmake --preset windows-vcpkg
   cmake --build build --config Release
   ```
4. Run: `.\build\Release\SubtitleForge.exe`

**Option B – Scripts**

```powershell
# Configure (uses vcpkg if VCPKG_ROOT is set)
.\configure-win.ps1
# Build
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

## Usage

1. **File > Import Video** to load a source file.
2. **File > Import TXT** to load subtitles (paragraphs separated by blank lines); they are spread evenly over the timeline.
3. Set **Trim** (start/end seconds) and **Subtitle style** (font, size, color).
4. Optionally set **Video title** (multiline, font, size, color); it is shown for the full duration.
5. Drag subtitle markers on the timeline to change when each line appears.
6. **File > Export Video** to render MP4 1080×1920 60fps.

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
