#!/usr/bin/env python3
"""Bloque D8 (sección 1): mete los 13 sonidos del banco propio del mod
(`ViceEx.SDT` + `ViceEx.RAW`) en el banco de efectos del port.

Cómo queda el banco (el motor decodifica `audio/sfx/<id>.mp3` a demanda):

    ids 0..9940        banco de siempre
    ids 9941..9953     las 13 muestras del mod (en su orden), que es también
                       el orden de `SFX_VICEEX_00..12` en `AudioSamples.h`

Requisitos que ya cumplen los ficheros existentes (medido): mp3 mono a 96 kbps.
La entrada de la SDT es de 20 bytes: `offset, size, rate, loopStart, loopEnd`;
`size` es el tamaño **PCM** (no el del mp3) y `rate` la frecuencia real.
Los pares con el mismo tamaño y Hz del banco del mod son el mismo sonido en dos
variantes (probablemente izquierda/derecha), así que hay 8 sonidos para 8 armas.

Uso:
    python tools/add_viceex_sfx.py            # añade/actualiza los 13
    python tools/add_viceex_sfx.py --lista    # sólo enseña la tabla

Idempotente: si el banco ya lleva las 13 entradas, las reescribe.
"""
import os
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD = os.path.join(os.path.dirname(ROOT),
                   "vice-extended-october-2025-update_1788102643_908113",
                   "GameFiles", "ViceExtended", "audio")
SDTMOD = os.path.join(MOD, "ViceEx.SDT")
WAVS = os.path.join(ROOT, "tmp", "viceex-samples")
OUTD = os.path.join(ROOT, "streamed", "Audio", "sfx")
OUTSDT = os.path.join(ROOT, "streamed", "Audio", "sfx.SDT")
PRIMER_ID = 9941
N = 13


def entradas_mod():
    d = open(SDTMOD, "rb").read()
    return [struct.unpack("<IIIIi", d[i * 20:(i + 1) * 20]) for i in range(len(d) // 20)]


def lista(e):
    print("muestras del mod (índice, segundos, Hz, bytes PCM, par):")
    for i, (off, size, rate, ls, le) in enumerate(e):
        par = ""
        for j, (o2, s2, r2, _, _) in enumerate(e):
            if j != i and s2 == size and r2 == rate:
                par = " par=%d" % j
                break
        print("  %2d  %.3f s  %6d Hz  %7d B  id=%d%s"
              % (i, size / 2.0 / rate, rate, size, PRIMER_ID + i, par))


def formato_real(mp3):
    """(rate, segundos) del mp3 recién creado, según ffprobe. None si no está.

    Por qué hace falta (fallo medido el 21/09/2026): el banco del mod declara
    frecuencias que NO son válidas para MP3 (36000, 33000, 22000 Hz), así que
    ffmpeg resamplea al más cercano (32000/22050). Si la SDT sigue declarando
    36000 y el mp3 está a 32000, el motor reproduce el buffer un 12 % rápido y
    más agudo. La SDT tiene que llevar la frecuencia REAL del fichero.
    """
    try:
        out = subprocess.check_output(
            ["ffprobe", "-v", "quiet", "-show_entries", "stream=sample_rate,duration",
             "-of", "csv=p=0", mp3], stderr=subprocess.DEVNULL).decode(errors="replace")
    except Exception:
        return None
    p = [x for x in out.replace("\n", ",").split(",") if x]
    if len(p) < 2:
        return None
    return int(p[0]), float(p[1])


def convierte(i, size, rate):
    """Codifica la muestra i y devuelve (rate, size) REALES del mp3, o None."""
    wav = os.path.join(WAVS, "viceex-%02d.wav" % i)
    mp3 = os.path.join(OUTD, "%d.mp3" % (PRIMER_ID + i))
    if not os.path.exists(wav):
        print("  falta %s" % wav)
        return None
    subprocess.check_call(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                           "-i", wav, "-ac", "1", "-b:a", "96k", mp3],
                          stdout=subprocess.DEVNULL, timeout=60)
    fmt = formato_real(mp3)
    if fmt is None:
        print("  (sin ffprobe: dejo rate/size del banco del mod para %d)" % i)
        return rate, size
    r, dur = fmt
    n = int(round(dur * r)) * 2          # PCM mono 16 bits
    if r != rate:
        print("  %d: el banco del mod dice %d Hz, el mp3 es %d Hz -> uso %d Hz"
              % (PRIMER_ID + i, rate, r, r))
    return r, n


def main():
    if not os.path.exists(SDTMOD):
        print("no encuentro el banco del mod:", SDTMOD)
        return 1
    ent = entradas_mod()
    lista(ent)
    if "--lista" in sys.argv:
        return 0

    d = open(OUTSDT, "rb").read()
    viejas = len(d) // 20
    if viejas == PRIMER_ID + N:          # ya estaban: nos quedamos con la base
        d = d[:PRIMER_ID * 20]
        viejas = PRIMER_ID
    if viejas != PRIMER_ID:
        print("¡ojo! el banco tiene %d entradas y esperaba %d. No toco nada."
              % (viejas, PRIMER_ID))
        print("(si has añadido muestras a mano, ajusta PRIMER_ID en esta herramienta)")
        return 1

    off = 0
    for i in range(viejas):              # offset acumulado, como el original
        o, s, r, ls, le = struct.unpack("<IIIIi", d[i * 20:(i + 1) * 20])
        off = max(off, o + s)

    out = bytearray(d)
    for i in range(N):
        _, size0, rate0, ls, le = ent[i]
        real = convierte(i, size0, rate0)
        if real is None:
            return 1
        rate, size = real
        mp3 = os.path.join(OUTD, "%d.mp3" % (PRIMER_ID + i))
        print("  id %d <- viceex-%02d.wav (mod: %d B pcm, %d Hz | real: %d B pcm, %d Hz, mp3 %d B)"
              % (PRIMER_ID + i, i, size0, rate0, size, rate, os.path.getsize(mp3)))
        out += struct.pack("<IIIIi", off, size, rate, 0, -1)
        off += size
    with open(OUTSDT, "wb") as f:
        f.write(bytes(out))
    print("SDT escrito: %d entradas (%d nuevas), ids %d..%d"
          % (len(out) // 20, N, PRIMER_ID, PRIMER_ID + N - 1))
    print("siguiente paso: python tools/gen_manifest.py y recompilar")
    return 0


if __name__ == "__main__":
    sys.exit(main())
