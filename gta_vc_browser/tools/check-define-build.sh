#!/bin/bash
# Comprueba que un fichero del motor compila con un #define EXTRA.
#
# Para qué: los bloques de Vice Extended que nacen APAGADOS en `config.h`
# (#ifdef VICEEXT_*) no pasan por el build normal, así que un error dentro de
# ellos no lo ve nadie hasta que alguien enciende el define. Esto reutiliza la
# línea de compilación real que tiene `ninja` para ese fichero (con sus -I y sus
# -D) y le añade el define, sin tocar `config.h`.
#
# Uso:
#   tools/check-define-build.sh VICEEXT_TURN_SIGNALS vehicles/Automobile.cpp
#   tools/check-define-build.sh VICEEXT_TURN_SIGNALS src/vehicles/Automobile.cpp
#
# Nota: usa `em++` de $EMSDK/upstream/emscripten (o /c/Users/$USER/emsdk...).
set -u
cd "$(git rev-parse --show-toplevel)"

DEFINE="${1:?falta el define (p. ej. VICEEXT_TURN_SIGNALS)}"
SRC="${2:?falta el fichero (p. ej. src/vehicles/Automobile.cpp)}"
BASE=$(basename "$SRC")

EMPP="${EMSDK:-/c/Users/${USER:-$(whoami)}/emsdk}/upstream/emscripten/em++"
[ -x "$EMPP" ] || { echo "no encuentro em++ en $EMPP"; exit 2; }

ninja -C gta_vc_browser/build/web -t commands 2>/dev/null | grep -m1 "$BASE.o" > /tmp/check-define-cmd.txt
[ -s /tmp/check-define-cmd.txt ] || { echo "ninja no tiene una línea de compilación para $BASE"; exit 2; }

# Quita la salida (-o/-MD/-MT/-MF) y los -s de enlazado, que con -fsyntax-only no aplican.
ARGS=$(tr ' ' '\n' < /tmp/check-define-cmd.txt | tail -n +2 | awk '
    drop { drop = 0; next }
    $0 == "-o" || $0 == "-MT" || $0 == "-MF" { drop = 1; next }
    $0 == "-MD" { next }
    /^-s/ { next }
    { printf "%s ", $0 }
')

"$EMPP" "-D$DEFINE" -fsyntax-only $ARGS
ESTADO=$?
if [ $ESTADO -eq 0 ]; then
    echo "OK: $BASE compila con -D$DEFINE"
else
    echo "FALLO: $BASE NO compila con -D$DEFINE (mirar los errores de arriba)"
fi
exit $ESTADO
