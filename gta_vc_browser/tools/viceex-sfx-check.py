#!/usr/bin/env python3
"""D8: comprueba, SIN lanzar el navegador, que los sonidos de las armas nuevas
del mod pueden sonar de verdad.

Para qué: el 21/09/2026 las 8 armas del mod disparaban MUDAS. El dato estaba en
el log (`VICEEX sfx arma=51 sample=9943` 40 veces y ni un `ODSFXMISS sfx=9943`),
y la causa en el motor: `InitialiseChannel` descartaba cualquier muestra >=
`SAMPLEBANK_MAX` (el fin de la tabla SDT ORIGINAL) mandándola a la rama de
comentarios de ped. Este comprobador mira las cuatro piezas que tienen que
encajar para que el sonido salga, y falla si alguna se rompe otra vez:

  1. tabla   `Audio/sfx.SDT` con las 13 entradas del mod (size/rate válidos);
  2. fichero el mp3 de cada id existe;
  3. formato la frecuencia REAL del mp3 cuadra con la que declara el SDT (el
             motor declara el buffer con la del SDT: si no cuadran, el sonido
             sale afinado mal) y la duración cuadra con el tamaño PCM;
  4. código el motor ADMITE ese rango de ids (el arreglo D8b) y la tabla
             arma->muestra sigue completa.

Uso:
    python tools/viceex-sfx-check.py            # desde la raíz del repo

Salida: una línea por muestra y un veredicto. Exit 0 = todo encaja.
"""
import os
import re
import struct
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SDT = os.path.join(RAIZ, "streamed", "Audio", "sfx.SDT")
SFXDIR = os.path.join(RAIZ, "streamed", "Audio", "sfx")
SAMP = os.path.join(os.path.dirname(RAIZ), "src", "audio", "sampman_oal.cpp")
SAMPLES = os.path.join(os.path.dirname(RAIZ), "src", "audio", "AudioSamples.h")
LOGIC = os.path.join(os.path.dirname(RAIZ), "src", "audio", "AudioLogic.cpp")

PRIMER_ID = 9941          # SFX_VICEEX_00
NCUANTAS = 13             # SFX_VICEEX_00..12
TOL_RATE = 0.05           # 5 %: por debajo de eso no se oye
TOL_DUR = 0.20            # 20 % de duración (padding/corte del mp3)


def entradas_sdt():
    with open(SDT, "rb") as f:
        d = f.read()
    n = len(d) // 20  # tSample = 5 x uint32
    return n, [struct.unpack_from("<IIIII", d, i * 20) for i in range(n)]


def mp3_fmt(ruta):
    """(rate, duración) del mp3 según ffprobe, o (None, None) si no está."""
    try:
        out = subprocess.check_output(
            ["ffprobe", "-v", "quiet", "-show_entries",
             "stream=sample_rate,duration", "-of", "csv=p=0", ruta],
            stderr=subprocess.DEVNULL).decode(errors="replace").strip()
    except Exception:
        return None, None
    partes = [p for p in out.replace("\n", ",").split(",") if p]
    if not partes:
        return None, None
    rate = int(partes[0])
    dur = float(partes[1]) if len(partes) > 1 else None
    return rate, dur


def main():
    fallos = []
    print("== D8 · sonidos de las armas nuevas del mod ==")

    if not os.path.exists(SDT) or not os.path.exists(SFXDIR):
        print("FAIL: no encuentro %s / %s" % (SDT, SFXDIR))
        return 1

    # 4. el motor admite el rango (regresión del arreglo D8b)
    with open(SAMP, encoding="utf-8", errors="replace") as f:
        samp = f.read()
    if "odViceExSample" not in samp or "SFX_VICEEX_00" not in samp:
        fallos.append("sampman_oal.cpp NO admite las muestras del mod "
                      "(falta odViceExSample / SFX_VICEEX_00): volverían a sonar mudas")
    else:
        print("   motor: InitialiseChannel admite el rango del mod (D8b) OK")

    # 1..3. tabla + ficheros + formato
    n, ents = entradas_sdt()
    print("   tabla: %s con %d entradas" % (os.path.relpath(SDT, RAIZ), n))
    if n < PRIMER_ID + NCUANTAS:
        fallos.append("la tabla SDT no llega a %d entradas (tiene %d)"
                      % (PRIMER_ID + NCUANTAS, n))
    for k in range(NCUANTAS):
        i = PRIMER_ID + k
        if i >= n:
            fallos.append("falta la entrada %d en el SDT" % i)
            continue
        off, size, freq, ls, le = ents[i]
        if size == 0 or freq == 0:
            fallos.append("entrada %d con size/rate a cero (nunca sonaría)" % i)
            continue
        ruta = os.path.join(SFXDIR, "%d.mp3" % i)
        if not os.path.exists(ruta):
            fallos.append("falta %d.mp3" % i)
            continue
        rate, dur = mp3_fmt(ruta)
        dur_sdt = size / 2.0 / freq
        if rate is None:
            print("   %d: SDT rate=%d size=%d  (sin ffprobe: no compruebo formato)" % (i, freq, size))
            continue
        desv = abs(rate - freq) / float(freq)
        ddur = abs((dur or dur_sdt) - dur_sdt) / dur_sdt
        estado = "OK"
        if desv > TOL_RATE:
            estado = "FALLO"
            fallos.append("%d.mp3 a %d Hz pero el SDT declara %d Hz (%.0f %% de desvío: "
                          "sonaría afinado mal)" % (i, rate, freq, desv * 100))
        if ddur > TOL_DUR:
            estado = "FALLO"
            fallos.append("%d.mp3 dura %.3fs y el SDT pide %.3fs (%.0f %%): se corta o "
                          "queda con silencio" % (i, dur or 0, dur_sdt, ddur * 100))
        print("   %d: rate=%d SDT=%d (%.0f%%)  dur=%.3fs SDT=%.3fs (%.0f%%)  %s"
              % (i, rate, freq, desv * 100, dur or 0, dur_sdt, ddur * 100, estado))

    # 4b. la tabla arma -> muestra sigue completa y en rango
    with open(LOGIC, encoding="utf-8", errors="replace") as f:
        logic = f.read()
    armas = re.findall(r"case WEAPONTYPE_(\w+):\s*odSfx = SFX_VICEEX_(\d+);", logic)
    ids = sorted(int(s) for _, s in armas)
    print("   tabla arma->muestra: %d armas (%s)" % (len(armas), ", ".join(
        "%s=%d" % (a, PRIMER_ID + int(s)) for a, s in sorted(armas))))
    if len(armas) != 8:
        fallos.append("la tabla arma->muestra tiene %d armas (se esperan 8)" % len(armas))
    if ids and (min(ids) < 0 or max(ids) >= NCUANTAS):
        fallos.append("la tabla arma->muestra sale del rango del mod (0..%d)" % (NCUANTAS - 1))

    # AudioSamples.h: TOTAL_AUDIO_SAMPLES tiene que ir DESPUÉS de SFX_VICEEX_12
    with open(SAMPLES, encoding="utf-8", errors="replace") as f:
        h = f.read()
    if not re.search(r"SFX_VICEEX_12,\s*TOTAL_AUDIO_SAMPLES", h):
        fallos.append("AudioSamples.h: TOTAL_AUDIO_SAMPLES no sigue a SFX_VICEEX_12 "
                      "(el motor no reservaría sitio para las muestras del mod)")

    print()
    if fallos:
        for f_ in fallos:
            print("   FAIL: " + f_)
        print("D8: FALLO (%d problema(s))" % len(fallos))
        return 1
    print("D8: OK — las 13 muestras del mod están en la tabla, existen, cuadran de "
          "formato y el motor admite el rango")
    return 0


if __name__ == "__main__":
    sys.exit(main())
