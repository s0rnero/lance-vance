#!/usr/bin/env bash
# Build reVC (rama miami) para navegador con Emscripten.
# Modelo on-demand (como dos.zone): paquete inicial bootseed/ (~130 MB) +
# resto bajo demanda desde streamed/ (fetch + IDB). Monohilo, Asyncify.
# Uso: ./gta_vc_browser/build.sh [--configure-only]
# Salida: gta_vc_browser/web/public/build/reVC.html/.js/.wasm (+boot .data)
set -euo pipefail
cd "$(dirname "$0")/.."

CONFIG_ONLY=0
for arg in "$@"; do
  case "$arg" in
    --configure-only) CONFIG_ONLY=1 ;;
  esac
done

command -v emcc >/dev/null || { echo "ERROR: emcc no encontrado. Activa emsdk primero."; exit 1; }

if [ ! -f vendor/librw/CMakeLists.txt ]; then
  echo "ERROR: vendor/librw vacio. Ejecuta: git submodule update --init --recursive"
  exit 1
fi

# bootseed/ debe existir (tools/stage_bootseed.py)
if [ ! -d gta_vc_browser/bootseed ]; then
  echo "ERROR: falta gta_vc_browser/bootseed. Ejecuta tools/stage_bootseed.py"
  exit 1
fi

# emcc no entiende rutas estilo MSYS (/c/...) en --pre-js/--preload-file: en
# Git Bash, usa la forma Windows (C:/...). En Linux/macOS `pwd -W` no existe y
# el `||` deja la ruta normal.
REPO_FWD="$(pwd -W 2>/dev/null || pwd)"

BUILD_DIR="gta_vc_browser/build/web"
OUT_DIR="$REPO_FWD/gta_vc_browser/web/public/build"
mkdir -p "$BUILD_DIR" "$OUT_DIR"

export CFLAGS="-I$(pwd)/gta_vc_browser/include"
export CXXFLAGS="-I$(pwd)/gta_vc_browser/include"
export CPATH="$(pwd)/gta_vc_browser/include"

PREJS="$REPO_FWD/gta_vc_browser/web/ondemand.js"
PRELOAD="--preload-file $REPO_FWD/gta_vc_browser/bootseed@/"

emcmake cmake -G Ninja -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXECUTABLE_SUFFIX=.html \
  -DCMAKE_MODULE_PATH="$(pwd)/gta_vc_browser/cmake" \
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$OUT_DIR" \
  -DREVC_AUDIO=OAL \
  -DREVC_WITH_OPUS=OFF \
  -DREVC_WITH_LIBSNDFILE=OFF \
  -DREVC_VENDORED_LIBRW=ON \
  -DREVC_NO_THREADS=ON \
  -DLIBRW_PLATFORM=GL3 \
  -DLIBRW_GL3_GFXLIB=GLFW \
  -DLIBRW_TOOLS=OFF \
  -DLIBRW_EXAMPLES=OFF \
  -DCMAKE_EXE_LINKER_FLAGS="-sUSE_GLFW=3 -sUSE_WEBGL2=1 -sFULL_ES3=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=536870912 -sMAXIMUM_MEMORY=1073741824 -sFORCE_FILESYSTEM=1 -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=1048576 -lidbfs.js --pre-js $PREJS -Wl,--wrap,fopen -Wl,--wrap,open -Wl,--wrap,stat -Wl,--wrap,access $PRELOAD"

echo "--- configurado en $BUILD_DIR ---"
if [ "$CONFIG_ONLY" -eq 1 ]; then exit 0; fi
emmake ninja -C "$BUILD_DIR"
