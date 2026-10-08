#!/usr/bin/env python3
"""Repara la columna freq de streamed/Audio/sfx.SDT con la tasa REAL de cada
mp3 (leída del header del primer frame MPEG, sin ffprobe).

Causa: build_streamed.py copia assets/Audio/sfx.SDT (vainilla, rates crudos
5000/10005/26513...) encima del regenerado por split_sfx.py. El motor
reproducía PCM de 8000 Hz a 5000 Hz etc. (pitch/velocidad mal).
Solo toca la columna freq; offsets/sizes/loops intactos.
"""
import os
import struct
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SDT = os.path.join(ROOT, "streamed", "Audio", "sfx.SDT")
OUTD = os.path.join(ROOT, "streamed", "Audio", "sfx")

SR = [{0: 44100, 1: 48000, 2: 32000},
      {0: 22050, 1: 24000, 2: 16000},
      {0: 11025, 1: 12000, 2: 8000}]


def mp3_rate(path):
    with open(path, "rb") as f:
        head = f.read(64 * 1024)
    off = 0
    if head[:3] == b"ID3":
        n = 0
        for c in head[6:10]:
            n = (n << 7) | (c & 0x7F)
        off = 10 + n
    # buscar sync 0xFFE
    while off + 4 <= len(head):
        if head[off] == 0xFF and (head[off + 1] & 0xE0) == 0xE0:
            b1, b2 = head[off + 1], head[off + 2]
            ver = (b1 >> 3) & 3
            layer = (b1 >> 1) & 3
            sri = (b2 >> 2) & 3
            if ver in (0, 1, 2, 3) and ver != 1 and layer == 1 and sri != 3:
                vi = 0 if ver == 3 else (1 if ver == 2 else 2)
                return SR[vi][sri]
        off += 1
    return None


def main():
    with open(SDT, "rb") as f:
        sdt = bytearray(f.read())
    n = len(sdt) // 20
    paths = [os.path.join(OUTD, "%d.mp3" % i) for i in range(n)]

    def one(i):
        if not os.path.exists(paths[i]):
            return None
        return mp3_rate(paths[i])

    changed, missing, unknown = 0, 0, 0
    with ThreadPoolExecutor(max_workers=32) as ex:
        rates = list(ex.map(one, range(n)))
    for i in range(n):
        off, size, freq, ls, le = struct.unpack("<IIIIi", sdt[i * 20:(i + 1) * 20])
        if size == 0:
            continue
        r = rates[i]
        if r is None:
            if not os.path.exists(paths[i]):
                missing += 1
            else:
                unknown += 1
            continue
        if r != freq:
            struct.pack_into("<I", sdt, i * 20 + 8, r)
            changed += 1
    with open(SDT, "wb") as f:
        f.write(sdt)
    print("OK entradas=%d cambiadas=%d faltantes=%d ilegibles=%d" % (n, changed, missing, unknown))


if __name__ == "__main__":
    main()
