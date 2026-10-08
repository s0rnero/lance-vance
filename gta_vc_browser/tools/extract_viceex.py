#!/usr/bin/env python3
"""Saca a WAV las muestras del banco de audio propio de Vice Extended (D8).

El mod trae `ViceExtended/audio/ViceEx.SDT` + `ViceEx.RAW`: un segundo banco de
sonido (formato de VC: entradas de 20 bytes `off, size, freq, loopStart,
loopEnd` sobre un RAW concatenado) con **13 muestras** que el juego de serie no
tiene. Nuestro motor no lo carga (lo carga su `ViceEx.exe`, que aquí no corre),
y el formato VC no guarda **nombres**: sin oído no se puede saber qué arma suena
con cada una. Esta herramienta hace la parte mecánica y deja la decisión al
jugador:

  1. extrae las 13 a `tmp/viceex-samples/viceex-NN.wav` (mono s16le, a la
     frecuencia que declara el SDT);
  2. escribe `tmp/viceex-samples/viceex-map.tsv`, la plantilla que el jugador
     rellena escuchando (una fila por muestra, con la duración al lado para
     reconocerlas);
  3. imprime el resumen (duración, Hz, deltas contra la muestra anterior).

Con la plantilla rellena, el paso siguiente es `split_sfx.py` (añadir las
muestras al banco servido con índices nuevos) + engancharlas en `AudioLogic` a
las armas nuevas (beretta, desert eagle, shotgun2, uziold, ak47, m16, steyr,
lanzacohetes).

Uso:
    python tools/extract_viceex.py [--sd <SDT>] [--raw <RAW>] [--out <dir>] [--solo 3,4]

Requisitos: ffmpeg (y ffprobe) en el PATH.
"""
import argparse
import os
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD = os.path.join(os.path.dirname(ROOT), "Grand Theft Auto Vice City",
                   "ViceExtended", "audio")
DEF_SDT = os.path.join(MOD, "ViceEx.SDT")
DEF_RAW = os.path.join(MOD, "ViceEx.RAW")
DEF_OUT = os.path.join(ROOT, "tmp", "viceex-samples")

# A qué suena cada índice en el mod no se sabe: esto sólo ordena la plantilla.
PLANTILLA = """# Plantilla del banco ViceEx (bloque D8).
#
# Una fila por muestra. Escucha los WAV de esta misma carpeta y escribe en la
# última columna para qué es (arma o efecto del mod). Ejemplos de lo que busca:
#   beretta, desert eagle, shotgun2, uziold, ak47, m16, steyr, granada/cohete,
#   recarga, impacto, concha al caer, disparo enemigo...
# Lo que dejes vacío se queda sonando con su equivalente de serie.
#
# Los pares con el MISMO tamaño y Hz son casi seguro la misma arma en dos
# variantes (o los dos disparos de una ráfaga): si es así, escríbelo igual en
# las dos y anota "(par)".
#
# indice	fichero	segundos	Hz	que es (rellenar)
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sd", default=DEF_SDT, help="ViceEx.SDT (por defecto el del mod)")
    ap.add_argument("--raw", default=DEF_RAW, help="ViceEx.RAW (por defecto el del mod)")
    ap.add_argument("--out", default=DEF_OUT, help="carpeta de salida")
    ap.add_argument("--solo", default=None, help="sólo estos índices, separados por comas")
    ap.add_argument("--remuestrear", action="store_true",
                    help="además, una copia a 22050 Hz (lo que suena en el juego)")
    a = ap.parse_args()

    if not os.path.exists(a.sd) or not os.path.exists(a.raw):
        raise SystemExit("ERROR: no encuentro %s / %s" % (a.sd, a.raw))

    sdt = open(a.sd, "rb").read()
    if len(sdt) % 20:
        raise SystemExit("ERROR: %s no tiene el tamaño de un SDT (20 B/entrada): %d"
                         % (a.sd, len(sdt)))
    n = len(sdt) // 20
    tam_raw = os.path.getsize(a.raw)
    print("== %s: %d muestras | %s: %d bytes" % (a.sd, n, a.raw, tam_raw))

    filtro = None
    if a.solo:
        filtro = set(int(v) for v in a.solo.split(","))

    os.makedirs(a.out, exist_ok=True)
    filas, suma = [], 0
    for i in range(n):
        off, size, freq, ls, le = struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20])
        suma += size
        segundos = size / (2.0 * freq) if freq else 0.0
        # El bucle: -1/-1 en el mod significa "sin bucle" (efecto de una vez).
        bucle = "" if le < 0 else " bucle=%d..%d" % (ls, le)
        print("   %2d  %7d B  %8d Hz  %.3f s%s" % (i, size, freq, segundos, bucle))
        if filtro is not None and i not in filtro:
            continue
        if off + size > tam_raw:
            print("      !! fuera del RAW (off+size=%d > %d); se salta" % (off + size, tam_raw))
            continue
        with open(a.raw, "rb") as f:
            f.seek(off)
            data = f.read(size)
        raw_tmp = os.path.join(a.out, "_%02d.raw" % i)
        open(raw_tmp, "wb").write(data)
        wav = os.path.join(a.out, "viceex-%02d.wav" % i)
        subprocess.check_call(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                               "-f", "s16le", "-ar", str(freq), "-ac", "1",
                               "-i", raw_tmp, wav])
        if a.remuestrear:
            subprocess.check_call(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                                   "-i", wav, "-ar", "22050",
                                   os.path.join(a.out, "viceex-%02d-22k.wav" % i)])
        os.remove(raw_tmp)
        filas.append("%2d\tviceex-%02d.wav\t%.3f\t%d\t" % (i, i, segundos, freq))

    if suma != tam_raw:
        print("!! aviso: la suma de tamaños (%d) no cuadra con el RAW (%d)" % (suma, tam_raw))
    else:
        print("== la suma de tamaños cuadra exactamente con el RAW: el banco está completo")

    tsv = os.path.join(a.out, "viceex-map.tsv")
    with open(tsv, "w", encoding="utf-8") as f:
        f.write(PLANTILLA)
        f.write("\n".join(filas) + "\n")
    print("== %d WAV en %s" % (len(filas), a.out))
    print("== plantilla para rellenar de oído: %s" % tsv)
    return 0


if __name__ == "__main__":
    sys.exit(main())
