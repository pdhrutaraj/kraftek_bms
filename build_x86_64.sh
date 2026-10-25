#!/usr/bin/env bash
rm -rf build_x86
set -Eeuo pipefail
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QT_PATH="${QT_PATH:-$HOME/Qt/6.5.3/gcc_64}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build_x86}"
QT6_CONFIG="$QT_PATH/lib/cmake/Qt6/Qt6Config.cmake"
if [[ ! -f "$QT6_CONFIG" ]]; then
  echo "ERROR: Qt6Config.cmake not found: $QT6_CONFIG" >&2
  echo "Set QT_PATH to the Qt 6 gcc_64 installation root." >&2
  find "$HOME/Qt" -name Qt6Config.cmake -print 2>/dev/null || true
  exit 1
fi
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_PREFIX_PATH="$QT_PATH" \
  -DQt6_DIR="$QT_PATH/lib/cmake/Qt6" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
echo "Build complete: $BUILD_DIR/kraftek_bms"
