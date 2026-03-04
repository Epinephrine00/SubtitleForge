#!/usr/bin/env bash
# Build SubtitleForge on macOS (uses build-mac/ so Windows build/ is untouched).
set -e
QT_PREFIX="${CMAKE_PREFIX_PATH:-$(brew --prefix qt@6 2>/dev/null || brew --prefix qt6 2>/dev/null)}"
if [[ -z "$QT_PREFIX" || ! -d "$QT_PREFIX" ]]; then
  echo "Qt not found. Install with: brew install qt@6  (or qt6)"
  exit 1
fi
echo "Using Qt at: $QT_PREFIX"
cmake -B build-mac -DCMAKE_PREFIX_PATH="$QT_PREFIX"
cmake --build build-mac
echo "Done. Run: open build-mac/SubtitleForge.app"
