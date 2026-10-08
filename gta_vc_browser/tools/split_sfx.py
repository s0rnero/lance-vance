#!/usr/bin/env python3
"""Trocea sfx.RAW+sfx.SDT en muestras mp3 individuales (receta revcDOS).

Salida: streamed/Audio/sfx/<i>.mp3 + streamed/Audio/sfx.SDT reescrito
(mismo formato, frecuencias actualizadas a las reales de cada mp3).
Las muestras de tamaño 0 se saltan (el motor las tolera en silencio).
"""
import os
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "assets", "Audio", "sfx.RAW")
SDT = os.path.join(ROOT, "assets", "Audio", "sfx.SDT")
OUTD = os.path.join(ROOT, "streamed", "Audio", "sfx")
OUTSDT = os.path.join(ROOT, "streamed", "Audio", "sfx.SDT")
WORKERS = int(sys.argv[1]) if len(sys.argv) > 1 else 8


def main():
    with open(SDT, "rb") as f:
        sdt = f.read()
    n = len(sdt) // 20
    raw = open(RAW, "rb")
    os.makedirs(OUTD, exist_ok=True)
    entries = []
    for i in range(n):
        off, size, freq, ls, le = struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20])
        entries.append([off, size, freq, ls, le])

    newfreq = {}

    def conv(i):
        off, size, freq, ls, le = entries[i]
        if size == 0:
            return
        raw.seek(off)
        data = raw.read(size)
        with tempfile.TemporaryDirectory() as td:
            rf = os.path.join(td, "s.raw")
            mf = os.path.join(td, "s.mp3")
            open(rf, "wb").write(data)
            subprocess.check_call(["ffmpeg", "-hide_banner", "-loglevel", "error",
                                   "-y", "-f", "s16le", "-ar", str(freq), "-ac", "1",
                                   "-i", rf, "-b:a", "96k", mf],
                                  stdout=subprocess.DEVNULL)
            rate = subprocess.check_output(["ffprobe", "-v", "quiet",
                                            "-select_streams", "a:0",
                                            "-show_entries", "stream=sample_rate",
                                            "-of", "default=noprint_wrappers=1:nokey=1", mf],
                                           text=True).strip()
            with open(mf, "rb") as f:
                mp3 = f.read()
            with open(os.path.join(OUTD, "%d.mp3" % i), "wb") as f:
                f.write(mp3)
            newfreq[i] = int(rate)

    with ThreadPoolExecutor(max_workers=WORKERS) as ex:
        futs = [ex.submit(conv, i) for i in range(n)]
        done = 0
        for f in futs:
            f.result()
            done += 1
            if done % 500 == 0:
                print("%d/%d" % (done, n), flush=True)

    with open(OUTSDT, "wb") as f:
        for i, (off, size, freq, ls, le) in enumerate(entries):
            f.write(struct.pack("<IIIIi", off, size, newfreq.get(i, freq), ls, le))
    print("OK %d muestras (+%d vacias)" % (len(newfreq), n - len(newfreq)))


if __name__ == "__main__":
    main()
