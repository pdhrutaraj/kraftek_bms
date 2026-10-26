
#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Native Qt 6.2.4 installation on Jetson TX2 NX
QT_PATH="${QT_PATH:-$HOME/opt/qt6/6.2.4}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build_tx2nx}"

QT6_CONFIG="$QT_PATH/lib/cmake/Qt6/Qt6Config.cmake"

echo "======================================"
echo " Kraftek BMS - TX2 NX Native Build"
echo "======================================"
echo "Project   : $PROJECT_DIR"
echo "Qt path   : $QT_PATH"
echo "Build dir : $BUILD_DIR"
echo "Architecture: $(uname -m)"

# Verify target architecture
if [[ "$(uname -m)" != "aarch64" ]]; then
    echo "ERROR: This script is intended for the ARM64 Jetson TX2 NX." >&2
    exit 1
fi

# Verify Qt installation
if [[ ! -f "$QT6_CONFIG" ]]; then
    echo "ERROR: Qt6Config.cmake not found: $QT6_CONFIG" >&2
    echo "Check that Qt 6.2.4 is installed at: $QT_PATH" >&2
    find "$HOME/opt/qt6" -name Qt6Config.cmake -print 2>/dev/null || true
    exit 1
fi

# Verify the Qt Widgets library
if [[ ! -f "$QT_PATH/lib/libQt6Widgets.so" ]]; then
    echo "ERROR: Qt6 Widgets library not found under $QT_PATH/lib" >&2
    exit 1
fi

export QT_ROOT="$QT_PATH"
export PATH="$QT_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$QT_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Use a conservative job count for TX2 NX memory
JOBS="${JOBS:-2}"

echo
echo "Qt version:"
"$QT_PATH/bin/qmake" -query QT_VERSION

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

if [[ -x "$BUILD_DIR/kraftek_bms" ]]; then
    echo "Executable: $BUILD_DIR/kraftek_bms"
else
    echo "WARNING: Build completed, but executable not found at:"
    echo "  $BUILD_DIR/kraftek_bms"
    echo "Check the executable name configured in CMakeLists.txt."
fi

