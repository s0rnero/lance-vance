@echo off
REM Build reVC (rama miami) para navegador con Emscripten en Windows.
REM Modelo on-demand (como dos.zone): paquete inicial bootseed/ (~130 MB) +
REM resto bajo demanda desde streamed/ (fetch + IDB). Monohilo, Asyncify.
REM Uso: gta_vc_browser\build.bat [--configure-only]
REM Requiere: emsdk activado (emsdk_env.bat), CMake + Ninja en PATH.
REM Salida: gta_vc_browser\web\public\build\reVC.html/.js/.wasm (+boot .data)
setlocal
cd /d "%~dp0.."

where emcc >nul 2>nul
if errorlevel 1 (
  echo ERROR: emcc no encontrado. Activa emsdk: C:\Users\s0rno\emsdk\emsdk_env.bat
  exit /b 1
)

set BUILD_DIR=gta_vc_browser\build\web
set OUT_DIR=%CD%\gta_vc_browser\web\public\build
REM CMake + Emscripten exigen barras normales (las invertidas rompen try_compile)
set MODPATH=%CD%\gta_vc_browser\cmake
set MODPATH=%MODPATH:\=/%
set OUTDIR_FWD=%OUT_DIR:\=/%
set REPO_FWD=%CD:\=/%
REM Shim AL/efx.h (copiado de vendor/openal-soft): Emscripten no lo trae.
set SHIMINC=%CD%\gta_vc_browser\include
set SHIMINC=%SHIMINC:\=/%
set CFLAGS=-I%SHIMINC%
set CXXFLAGS=-I%SHIMINC%
set CPATH=%SHIMINC%
REM ondemand.js (pre-js): manifiesto + fetch + IDB. Ruta con barras normales.
set PREJS=%REPO_FWD%/gta_vc_browser/web/ondemand.js
REM Paquete inicial: solo bootseed/ (lista de arranque). El resto llega por red.
set PRELOAD=--preload-file "%REPO_FWD%/gta_vc_browser/bootseed@/"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

set CONFIG_ONLY=0
for %%a in (%*) do (
  if "%%a"=="--configure-only" set CONFIG_ONLY=1
)

call emcmake cmake -G Ninja -S . -B "%BUILD_DIR%" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DCMAKE_EXECUTABLE_SUFFIX=.html ^
  -DCMAKE_MODULE_PATH="%MODPATH%" ^
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%OUTDIR_FWD%" ^
  -DREVC_AUDIO=OAL ^
  -DREVC_WITH_OPUS=OFF ^
  -DREVC_WITH_LIBSNDFILE=OFF ^
  -DREVC_VENDORED_LIBRW=ON ^
  -DREVC_NO_THREADS=ON ^
  -DLIBRW_PLATFORM=GL3 ^
  -DLIBRW_GL3_GFXLIB=GLFW ^
  -DLIBRW_TOOLS=OFF ^
  -DLIBRW_EXAMPLES=OFF ^
  -DCMAKE_EXE_LINKER_FLAGS="-sUSE_GLFW=3 -sUSE_WEBGL2=1 -sFULL_ES3=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=536870912 -sMAXIMUM_MEMORY=1073741824 -sFORCE_FILESYSTEM=1 -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=1048576 -lidbfs.js --pre-js %PREJS% -Wl,--wrap,fopen -Wl,--wrap,open -Wl,--wrap,stat -Wl,--wrap,access %PRELOAD%"
if errorlevel 1 exit /b 1

echo --- configurado en %BUILD_DIR% ---
if "%CONFIG_ONLY%"=="1" exit /b 0
call emmake ninja -C "%BUILD_DIR%"
endlocal
