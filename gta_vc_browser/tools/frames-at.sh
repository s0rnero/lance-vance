#!/usr/bin/env bash
# Extrae fotogramas del vídeo de una partida en los instantes que diga el log.
#
# Por qué existe (21/09/2026, 7ª partida): el jugador graba la pantalla y el log
# lleva marcas UTC. Cruzar las dos cosas convierte "yo lo vi raro" en un dato
# comprobable (p.ej. la cámara dentro de la cabeza al agacharse, la barra de
# sirena que no está, el nado con la cámara clavada en la superficie).
#
# Uso:
#   tools/frames-at.sh "Grabación ... .mp4" 2026-09-22T01:24:22Z \
#       2026-09-22T01:24:25.549Z 2026-09-22T01:26:05.552Z ...
#
#   - El 2º argumento es el instante UTC del fotograma 0 del vídeo (el inicio de
#     la grabación). Se saca de la duración del vídeo y de su hora de fin:
#          ffmpeg -i VÍDEO        ->  Duration
#          stat -c %y VÍDEO       ->  hora de fin (mtime)  =>  base = fin - duración
#     Con la grabación del 21/09 (6:00.51, mtime 20:30:22 local = 01:30:22Z) la
#     base fue 2026-09-22T01:24:22Z.
#   - Los siguientes argumentos son marcas UTC del log (cualquier línea vale).
#   - Los PNG salen en gta_vc_browser/tmp/frames/ como t<segundos>.png
#
# Nota: el margen de ±0,5 s es el del propio seek de ffmpeg; para ver "el frame
# exacto" de un evento, pásale dos o tres marcas seguidas.
set -euo pipefail

if [ $# -lt 3 ]; then
	sed -n '2,26p' "$0"
	exit 1
fi

VIDEO="$1"; shift
BASE="$1"; shift
OUT="${OUT_DIR:-gta_vc_browser/tmp/frames}"
mkdir -p "$OUT"

command -v ffmpeg >/dev/null || { echo "FALTA ffmpeg en el PATH"; exit 1; }

for TS in "$@"; do
	# segundos desde la base (python: disponible en este repo para las herramientas)
	OFF=$(python - "$BASE" "$TS" <<'PY'
import sys, datetime
def p(s):
	s = s.strip().replace('Z', '+00:00')
	return datetime.datetime.fromisoformat(s)
d = (p(sys.argv[2]) - p(sys.argv[1])).total_seconds()
print(f"{d:.3f}")
PY
)
	INT=${OFF%.*}
	[ "$INT" -lt 0 ] && { echo "  $TS -> fuera del vídeo (t=$OFF)"; continue; }
	NAME="$OUT/t${INT}.png"
	ffmpeg -hide_banner -loglevel error -ss "$OFF" -i "$VIDEO" -frames:v 1 \
		-vf "scale=920:-1" -y "$NAME"
	echo "  $TS -> t=$OFF  $NAME"
done

echo
echo "Ya se pueden leer los PNG con las herramientas de ficheros (se ven como imagen)."
