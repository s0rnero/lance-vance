#!/usr/bin/env bash
# Captura de rendimiento del movil por adb, para el plan 82.3 (F0 / F0-bis).
#
# Hardware de referencia: Moto G72 = MediaTek Helio G99 (MT6789) + Mali-G57 MC2,
# pantalla 1080x2400 que corre a 90 Hz por defecto. En Android 13 sin root no se
# puede leer /sys/class/kgsl ni /sys/class/devfreq/*.mali (Permission denied), y
# no hay contador de "GPU busy". Lo que SI se puede:
#   - temperatura de CPU / GPU / SKIN  -> dumpsys thermalservice
#   - memoria de GPU global y por PID  -> dumpsys gpu
#   - FPS y jank reales (present-to-present, todos los layers) -> SurfaceFlinger
#   - refresco activo de la pantalla   -> dumpsys display
#
# Uso:
#   bash gta_vc_browser/tools/adb-measure.sh [etiqueta] [segundos]
#     etiqueta  nombre del caso (por defecto "medicion")
#     segundos  duracion de la captura (por defecto 60)
#
# Requisitos: movil conectado por USB o Wi-Fi (adb connect) con el juego abierto
# en Chrome y la pantalla encendida. Escribe logs/adb-<etiqueta>-<fecha>.txt
#
# Ejemplos:
#   bash gta_vc_browser/tools/adb-measure.sh ciudad-distancia-max 90
#   bash gta_vc_browser/tools/adb-measure.sh ciudad-ventana-960 90
set -u

if [ -n "${ADB:-}" ]; then
  :
elif command -v adb >/dev/null 2>&1; then
  ADB=adb
elif [ -x "$LOCALAPPDATA/Android/platform-tools/adb.exe" ]; then
  ADB="$LOCALAPPDATA/Android/platform-tools/adb.exe"
else
  echo "adb no encontrado: instala platform-tools o exporta ADB=<ruta a adb>" >&2
  exit 1
fi

LABEL="${1:-medicion}"
SECS="${2:-60}"
case "$SECS" in ''|*[!0-9]*) echo "segundos debe ser un entero" >&2; exit 2 ;; esac

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT/logs"
STAMP="$(date +%Y-%m-%d_%H-%M-%S)"
OUT="$ROOT/logs/adb-$LABEL-$STAMP.txt"

if [ "$("$ADB" get-state 2>&1 | tr -d '\r')" != "device" ]; then
  echo "el movil no esta listo para adb. revisa la conexion (USB o adb connect ip:puerto)." >&2
  exit 3
fi

sh_shell() { "$ADB" shell "$1" 2>/dev/null | tr -d '\r'; }

# Ultima lectura de una temperatura del HAL (mName=CPU|GPU|SKIN).
temp_of() {
  sh_shell "dumpsys thermalservice" |
    awk -v want="$1" -F'mValue=|, mType' '$0 ~ ("mName=" want) {v=$2} END{if(v=="")printf "-"; else printf "%.1f", v}'
}

refresh_of() {
  sh_shell "dumpsys display" |
    grep -m1 'mActiveSfDisplayMode=' |
    sed -n 's/.*refreshRate=\([0-9.]*\).*/\1/p'
}

gpu_mem_of() {
  sh_shell "dumpsys gpu" | awk '/Global total/ {print int($3/1048576); exit}'
}

{
  echo "# adb-measure $STAMP label=$LABEL dur=${SECS}s"
  echo "# modelo=$(sh_shell 'getprop ro.product.model') soc=$(sh_shell 'getprop ro.soc.model') android=$(sh_shell 'getprop ro.build.version.release')"
  echo "# gpu_mem_inicio_mb=$(gpu_mem_of) refresco=$(refresh_of)"
  echo "# nota: sin root no hay 'GPU busy'; usar temperatura, gpu_mem y el A/B de pixeles/objetos"
} > "$OUT"

sh_shell "dumpsys SurfaceFlinger --timestats -clear -enable" >/dev/null 2>&1 || true

echo "capturando $SECS s en '$LABEL' (deja el juego en marcha y conduce)..."
ROWS="$(mktemp)"
for _ in $(seq 1 "$SECS"); do
  printf '%s cpu=%s gpu=%s skin=%s\n' \
    "$(date +%H:%M:%S)" "$(temp_of CPU)" "$(temp_of GPU)" "$(temp_of SKIN)" >> "$ROWS"
  sleep 1
done

{
  echo
  echo "## muestras (1 Hz, grados C)"
  cat "$ROWS"
  echo
  echo "## resumen termico"
  awk -F'skin=' '{split($0,a,"gpu="); split(a[2],g," "); if (g[1]=="-") next; n++; s+=g[1]; if(n==1||g[1]<mn)mn=g[1]; if(g[1]>mx)mx=g[1]} END{if(n) printf "gpu_temp n=%d min=%.1f media=%.1f max=%.1f\n", n, mn, s/n, mx; else print "sin temperaturas"}' "$ROWS"
  echo
  echo "## estado final"
  echo "gpu_mem_fin_mb=$(gpu_mem_of) refresco=$(refresh_of)"
  echo
  echo "## SurfaceFlinger --timestats"
  sh_shell "dumpsys SurfaceFlinger --timestats -dump"
  echo
  echo "## resumen de FPS (present-to-present, todos los layers)"
  sh_shell "dumpsys SurfaceFlinger --timestats -dump" |
    awk '/totalP2PTime/ {p=$3} /^totalFrames/ {f=$3} END{if(f>0) printf "frames=%d p2p_medio=%.2fms fps_medio=%.1f\n", f, p/f, 1000/(p/f)}'
} >> "$OUT"
rm -f "$ROWS"

echo "escrito: $OUT"
