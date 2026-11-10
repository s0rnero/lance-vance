#!/usr/bin/env python3
"""Genera web/public/manifest.json: ruta_minusculas -> {p: real, s: bytes}, y __vc__ -> {v, h}."""
import hashlib
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "streamed")
DST = os.path.join(ROOT, "web", "public", "manifest.json")

# sfx.RAW no se sirve: el motor web usa muestras sueltas (sfx/*.mp3).
# Mantenerlo fuera del manifiesto evita descargarlo por accidente (340 MB).
SKIP = {"audio/sfx.raw"}

KEY = "__vc__"
SEP = bytes(1)


def filedigest(path):
    h = hashlib.blake2b(digest_size=16)
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


entries = []
for root, dirs, files in os.walk(SRC):
    dirs[:] = [d for d in dirs if ".hide" not in d]
    for f in files:
        full = os.path.join(root, f)
        rel = os.path.relpath(full, SRC).replace(os.sep, "/")
        if rel.lower() in SKIP:
            continue
        entries.append((rel.lower(), rel, full))
entries.sort()

man = {}
stamp = hashlib.blake2b(digest_size=16)
for low, rel, full in entries:
    h = filedigest(full)
    man[low] = {"p": rel, "s": os.path.getsize(full)}
    stamp.update(low.encode("utf-8"))
    stamp.update(SEP)
    stamp.update(h.encode("ascii"))
    stamp.update(SEP)
man[KEY] = {"v": 1, "h": stamp.hexdigest()}

os.makedirs(os.path.dirname(DST), exist_ok=True)
with open(DST, "w") as o:
    json.dump(man, o)
print("%d entradas sello=%s -> %s" % (len(entries), man[KEY]["h"], DST))
