#!/bin/bash
# Build INFERNUS for WebAssembly
# Usage: bash tools/build_web.sh

set -e

EMSDK_DIR="/c/Users/Juan Miguel/emsdk"
EMSDK_PYTHON="$EMSDK_DIR/python/3.13.3_64bit/python.exe"
EMCC="$EMSDK_DIR/upstream/emscripten/emcc.py"
EMXX="$EMSDK_DIR/upstream/emscripten/em++.py"
TOOLCHAIN="$EMSDK_DIR/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build_web"

echo "=== INFERNUS Web Build ==="
echo "Project: $PROJECT_DIR"
echo "Build:   $BUILD_DIR"

# Set environment for emscripten
export EMSDK="$EMSDK_DIR"
export EM_CONFIG="$EMSDK_DIR/.emscripten"
export PATH="$EMSDK_DIR/upstream/emscripten:$EMSDK_DIR/node/22.16.0_64bit/bin:$EMSDK_DIR/python/3.13.3_64bit:$PATH"

# Create build directory
mkdir -p "$BUILD_DIR"

# Configure with CMake using Emscripten toolchain
echo ""
echo "--- Configuring CMake ---"
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DPLATFORM=Web \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s TOTAL_MEMORY=67108864 --preload-file $PROJECT_DIR/assets@/assets --shell-file $PROJECT_DIR/tools/shell.html" \
    -G "Unix Makefiles" \
    2>&1

# Build
echo ""
echo "--- Building ---"
cmake --build "$BUILD_DIR" --target INFERNUS -j8 2>&1

# Copy output
echo ""
echo "--- Output ---"
if [ -f "$BUILD_DIR/INFERNUS.html" ]; then
    cp "$BUILD_DIR/INFERNUS.html" "$PROJECT_DIR/build_web/index.html" 2>/dev/null || true
    echo "Build complete!"
    echo "Files in $BUILD_DIR:"
    ls -la "$BUILD_DIR"/INFERNUS.* 2>/dev/null
    echo ""
    echo "To test: cd build_web && python -m http.server 8080"
    echo "Then open: http://localhost:8080/INFERNUS.html"
else
    echo "Build output:"
    ls -la "$BUILD_DIR"/*.html "$BUILD_DIR"/*.js "$BUILD_DIR"/*.wasm "$BUILD_DIR"/*.data 2>/dev/null || echo "No web output files found"
fi
