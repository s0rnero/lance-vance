#!/usr/bin/env python3
"""Genera streamed/audio/sound.cache: longitudes (ms) de StreamedNameTable.

Evita que el motor abra+mida ~50 ficheros de audio en cada arranque
(radios ADF de 30-70 MB incluidas). Igual que revcDOS (lo trae en preload).
"""
import os
import re
import struct
import subprocess
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SAMPMAN = os.path.normpath(os.path.join(ROOT, "..", "src", "audio", "sampman.h"))
AUD = os.path.join(ROOT, "streamed", "Audio")
OUT = os.path.join(AUD, "sound.cache")


def table():
    d = open(SAMPMAN, encoding="utf8", errors="replace").read()
    m = re.search(r"static char StreamedNameTable\[\]\[\d+\]\s*=\s*\{(.*?)\};", d, re.S)
    return re.findall(r'"([^"]+)"', m.group(1))


def resolve(name):
    base = name.split("\\")[-1]
    for cand in (os.path.join(AUD, base),
                 os.path.join(AUD, base.lower()),
                 os.path.join(AUD, base.upper())):
        if os.path.exists(cand):
            return cand
    # buscar insensible a mayúsculas
    for f in os.listdir(AUD):
        if f.lower() == base.lower():
            return os.path.join(AUD, f)
    return None


def length_ms(entry):
    name, path = entry
    if not path:
        return 0
    try:
        out = subprocess.check_output(
            ["ffprobe", "-v", "quiet", "-show_entries", "format=duration",
             "-of", "default=noprint_wrappers=1:nokey=1", path],
            text=True, timeout=60).strip()
        return int(float(out) * 1000)
    except Exception:
        return 0


def main():
    tab = table()
    print("%d pistas" % len(tab))
    entries = [(t, resolve(t)) for t in tab]
    missing = sorted({t for t, p in entries if not p})
    print("ausentes (%d): %s" % (len(missing), missing[:10]))
    with ThreadPoolExecutor(max_workers=8) as ex:
        ms = list(ex.map(length_ms, entries))
    print("ceros:", sum(1 for x in ms if x == 0))
    with open(OUT, "wb") as f:
        for x in ms:
            f.write(struct.pack("<I", x))
    print("OK %s (%d bytes)" % (OUT, os.path.getsize(OUT)))


if __name__ == "__main__":
    main()
