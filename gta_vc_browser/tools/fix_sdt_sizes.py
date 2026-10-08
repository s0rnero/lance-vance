#!/usr/bin/env python3
"""Completa fix_sdt_rates.py: ajusta size + loop points (ls/le) a la tasa real.

Cada mp3 se decodifica (s16le mono a su rate) y size = bytes decodificados.
ls/le se escalan por newfreq/oldfreq (old = tabla vainilla en vc/).
Solo toca size/ls/le; off y freq intactos (freq ya la puso fix_sdt_rates).
"""
import os
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VSDT = os.path.join(ROOT, "assets", "Audio", "sfx.SDT")
SDT = os.path.join(ROOT, "streamed", "Audio", "sfx.SDT")
OUTD = os.path.join(ROOT, "streamed", "Audio", "sfx")
WORKERS = int(sys.argv[1]) if len(sys.argv) > 1 else 16

FF = "ffmpeg"


def decode_len(path, rate):
    p = subprocess.run(
        [FF, "-hide_banner", "-loglevel", "error", "-y", "-i", path,
         "-f", "s16le", "-ac", "1", "-ar", str(rate), "pipe:1"],
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    return len(p.stdout)


def main():
    vsdt = open(VSDT, "rb").read()
    with open(SDT, "rb") as f:
        sdt = bytearray(f.read())
    n = len(sdt) // 20
    assert len(vsdt) // 20 == n

    jobs = []
    for i in range(n):
        off, size, freq, ls, le = struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20])
        if size == 0:
            continue
        p = os.path.join(OUTD, "%d.mp3" % i)
        if not os.path.exists(p):
            continue
        jobs.append(i)

    def one(i):
        off, size, freq, ls, le = struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20])
        voff, vsize, vfreq, vls, vle = struct.unpack("<IIIIi", vsdt[i * 20:(i + 1) * 20])
        try:
            got = decode_len(os.path.join(OUTD, "%d.mp3" % i), freq)
        except Exception:
            return None
        nls, nle = ls, le
        if vfreq and vfreq != freq and (ls != 0 or le not in (0, -1)):
            k = freq / vfreq
            nls = int(round(ls * k))
            nle = int(round(le * k)) if le > 0 else le
        return (i, got, nls, nle)

    changed = 0
    with ThreadPoolExecutor(max_workers=WORKERS) as ex:
        for r in ex.map(one, jobs):
            if not r:
                continue
            i, got, nls, nle = r
            off, size, freq, ls, le = struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20])
            if got != size or nls != ls or nle != le:
                struct.pack_into("<I", sdt, i * 20 + 4, got)
                struct.pack_into("<Ii", sdt, i * 20 + 12, nls, nle)
                changed += 1
    with open(SDT, "wb") as f:
        f.write(sdt)
    print("OK entradas=%d ajustadas=%d" % (n, changed))


if __name__ == "__main__":
    main()
