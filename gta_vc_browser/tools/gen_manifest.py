#!/usr/bin/env python3
"""Genera web/public/manifest.json: ruta_minusculas -> {p: real, s: bytes}."""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "streamed")
DST = os.path.join(ROOT, "web", "public", "manifest.json")

# sfx.RAW no se sirve: el motor web usa muestras sueltas (sfx/*.mp3).
# Mantenerlo fuera del manifiesto evita descargarlo por accidente (340 MB).
SKIP = {"audio/sfx.raw"}

man = {}
for root, dirs, files in os.walk(SRC):
    dirs[:] = [d for d in dirs if ".hide" not in d]
    for f in files:
        full = os.path.join(root, f)
        rel = os.path.relpath(full, SRC).replace(os.sep, "/")
        if rel.lower() in SKIP:
            continue
        man[rel.lower()] = {"p": rel, "s": os.path.getsize(full)}

os.makedirs(os.path.dirname(DST), exist_ok=True)
with open(DST, "w") as o:
    json.dump(man, o)
print("%d entradas -> %s" % (len(man), DST))
