#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QT_PATH="${QT_PATH:-$HOME/Qt/6.5.3/gcc_64}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build_x86}"

echo "======================================"
echo " Kraftek BMS - x86_64 Native Build"
echo "======================================"
echo "Project      : $PROJECT_DIR"
echo "Qt path      : $QT_PATH"
echo "Build dir    : $BUILD_DIR"
echo "Architecture : $(uname -m)"

if [[ "$(uname -m)" != "x86_64" ]]; then
    echo "ERROR: This script requires x86_64 Linux." >&2
    exit 1
fi

QT6_CONFIG="$QT_PATH/lib/cmake/Qt6/Qt6Config.cmake"

if [[ ! -f "$QT6_CONFIG" ]]; then
    echo "ERROR: Qt6Config.cmake not found: $QT6_CONFIG" >&2
    exit 1
fi

if [[ ! -f "$QT_PATH/lib/libQt6Widgets.so" ]]; then
    echo "ERROR: Qt6 Widgets library not found under $QT_PATH/lib" >&2
    exit 1
fi

export QT_ROOT="$QT_PATH"
export PATH="$QT_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$QT_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

JOBS="${JOBS:-$(nproc)}"

echo
echo "Qt version:"
if [[ -x "$QT_PATH/bin/qmake" ]]; then
    "$QT_PATH/bin/qmake" -query QT_VERSION
else
    echo "qmake not found; using CMake package configuration"
fi

echo
echo "Configuring CMake..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_PATH" \
    -DQt6_DIR="$QT_PATH/lib/cmake/Qt6" \
    -DCMAKE_BUILD_TYPE=Release

echo
echo "Building with $JOBS parallel job(s)..."
cmake --build "$BUILD_DIR" --parallel "$JOBS"

echo
echo "Build complete."
echo "Executable: $BUILD_DIR/kraftek_bms"
