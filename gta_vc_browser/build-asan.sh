#!/usr/bin/env bash
# Build de DIAGNOSTICO con AddressSanitizer (plan 68.0, T-B). NO es la build del
# jugador para jugar a diario: caza la escritura fuera de limites / use-after-free
# que corrompe el heap y la nombra con fichero/linea. Objetos en build/asan para
# no pisar la build normal (build/web). Salida al mismo /build servido.
# Uso: ./gta_vc_browser/build-asan.sh   (luego subir VERSION a 68.0-ASAN)
set -euo pipefail
cd "$(dirname "$0")/.."

if ! command -v emcc >/dev/null 2>&1; then
  EMSDK_DIR="${EMSDK:-$HOME/emsdk}"
  if [ -f "$EMSDK_DIR/upstream/emscripten/emcc.py" ]; then
    export PATH="$EMSDK_DIR/upstream/emscripten:$EMSDK_DIR/upstream/bin:$PATH"
  fi
fi
command -v emcc >/dev/null || { echo "ERROR: emcc no encontrado."; exit 1; }

if [ ! -f vendor/librw/CMakeLists.txt ]; then
  echo "ERROR: vendor/librw vacio."; exit 1; fi
if [ ! -d gta_vc_browser/bootseed ]; then
  echo "ERROR: falta bootseed."; exit 1; fi

python gta_vc_browser/tools/check_img_index.py --mirror gta_vc_browser/bootseed/models/gta3.dir || exit 1
python gta_vc_browser/tools/check_img_index.py --img gta_vc_browser/streamed/anim/cuts.img --mirror gta_vc_browser/bootseed/anim/cuts.dir || exit 1

REPO_FWD="$(pwd -W 2>/dev/null || pwd)"
BUILD_DIR="gta_vc_browser/build/asan"
OUT_DIR="$REPO_FWD/gta_vc_browser/web/public/build-asan"
mkdir -p "$BUILD_DIR" "$OUT_DIR"

export CFLAGS="-I$(pwd)/gta_vc_browser/include -fsanitize=address"
export CXXFLAGS="-I$(pwd)/gta_vc_browser/include -fsanitize=address"
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
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sFULL_ES3=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=536870912 -sMAXIMUM_MEMORY=2147483648 -sFORCE_FILESYSTEM=1 -sABORTING_MALLOC=1 -sEXPORTED_FUNCTIONS=_main,_ViceExtRescanUserMusic,_ViceExtRescanSaves,_ViceExtRescanSkins,_ViceExtTouchFrame,_ViceExtTouchSetEnabled,_ViceExtTouchSetAutoaim,_ViceExtTouchContext,_ViceExtTouchMenuTap,_ViceExtTouchSkip,_ViceExtTouchPause,_ViceExtTouchOpenMap,_ViceExtTouchWeapon,_ViceExtTouchRadio,_ViceExtTouchAutoAim,_ViceExtTouchVehicleType,_ViceExtTouchNearVehicleType,_ViceExtTouchMissionVehicle,_ViceExtTouchArmedVehicle -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=2097152 -lidbfs.js --pre-js $PREJS -Wl,--wrap,fopen -Wl,--wrap,open -Wl,--wrap,stat -Wl,--wrap,access $PRELOAD"

echo "--- ASan configurado en $BUILD_DIR ---"
emmake ninja -C "$BUILD_DIR"