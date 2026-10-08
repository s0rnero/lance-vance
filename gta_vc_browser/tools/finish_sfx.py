#!/usr/bin/env python3
"""Termina split_sfx: tasas de los mp3 ya hechos + #9067 con padding + SDT."""
import os
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "assets", "Audio", "sfx.RAW")
SDT = os.path.join(ROOT, "assets", "Audio", "sfx.SDT")
OUTD = os.path.join(ROOT, "streamed", "Audio", "sfx")
OUTSDT = os.path.join(ROOT, "streamed", "Audio", "sfx.SDT")


def probe_rate(mp3):
    return subprocess.check_output(["ffprobe", "-v", "quiet",
                                    "-select_streams", "a:0",
                                    "-show_entries", "stream=sample_rate",
                                    "-of", "default=noprint_wrappers=1:nokey=1", mp3],
                                   text=True).strip()


def main():
    with open(SDT, "rb") as f:
        sdt = f.read()
    n = len(sdt) // 20
    entries = [struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20]) for i in range(n)]

    # #9067 (u otros faltantes): pad a 4KB y convertir
    raw = open(RAW, "rb")
    missing = [i for i in range(n)
               if entries[i][1] > 0 and not os.path.exists(os.path.join(OUTD, "%d.mp3" % i))]
    print("faltantes:", missing)
    import tempfile
    for i in missing:
        off, size, freq, ls, le = entries[i]
        raw.seek(off)
        data = raw.read(size) + b"\0" * 4096
        with tempfile.TemporaryDirectory() as td:
            rf, mf = os.path.join(td, "s.raw"), os.path.join(td, "s.mp3")
            open(rf, "wb").write(data)
            subprocess.check_call(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                                   "-f", "s16le", "-ar", str(freq), "-ac", "1",
                                   "-i", rf, "-b:a", "96k", mf],
                                  stdout=subprocess.DEVNULL, timeout=60)
            with open(mf, "rb") as f:
                open(os.path.join(OUTD, "%d.mp3" % i), "wb").write(f.read())

    rates = {}

    def probe(i):
        p = os.path.join(OUTD, "%d.mp3" % i)
        if os.path.exists(p):
            try:
                rates[i] = int(probe_rate(p))
            except Exception:
                pass

    with ThreadPoolExecutor(max_workers=16) as ex:
        list(ex.map(probe, range(n)))
    print("tasas ok:", len(rates))

    with open(OUTSDT, "wb") as f:
        for i, (off, size, freq, ls, le) in enumerate(entries):
            f.write(struct.pack("<IIIIi", off, size, rates.get(i, freq), ls, le))
    print("SDT escrito")


if __name__ == "__main__":
    main()
