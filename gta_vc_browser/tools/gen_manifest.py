#!/usr/bin/env python3
"""Genera web/public/manifest.json y empaqueta los ficheros pequenos en packs/."""
import hashlib
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "streamed")
DST = os.path.join(ROOT, "web", "public", "manifest.json")
PACKS = os.path.join(ROOT, "web", "public", "packs")

# sfx.RAW no se sirve: el motor web usa muestras sueltas (sfx/*.mp3).
# Mantenerlo fuera del manifiesto evita descargarlo por accidente (340 MB).
SKIP = {"audio/sfx.raw"}

KEY = "__vc__"
SEP = bytes(1)

# Los ficheros de hasta 4 MB entran en paquetes de hasta 8 MB, que es lo que
# convierte 17.489 peticiones en ~130. Los grandes siguen sueltos.
PACK_MAX = 4 * 1048576
PACK_TARGET = 8 * 1048576


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

os.makedirs(PACKS, exist_ok=True)

man = {}
stamp = hashlib.blake2b(digest_size=16)
packs = []
pila = []
buf = bytearray()
empaquetados = 0


def cerrar():
    if not pila:
        return
    data = bytes(buf)
    nombre = hashlib.blake2b(data, digest_size=8).hexdigest() + ".vcp"
    destino = os.path.join(PACKS, nombre)
    if not os.path.exists(destino):
        with open(destino, "wb") as o:
            o.write(data)
    indice = len(packs)
    packs.append({"n": nombre, "s": len(data)})
    for low, off in pila:
        man[low]["pk"] = indice
        man[low]["po"] = off
    global empaquetados
    empaquetados += len(pila)
    pila.clear()
    buf.clear()


for low, rel, full in entries:
    tam = os.path.getsize(full)
    if 0 < tam <= PACK_MAX:
        with open(full, "rb") as f:
            data = f.read()
        h = hashlib.blake2b(data, digest_size=16).hexdigest()
    else:
        data = None
        h = filedigest(full)
    man[low] = {"p": rel, "s": tam, "h": h[:16]}
    stamp.update(low.encode("utf-8"))
    stamp.update(SEP)
    stamp.update(h.encode("ascii"))
    stamp.update(SEP)
    if data is None:
        continue
    if buf and len(buf) + tam > PACK_TARGET:
        cerrar()
    pila.append((low, len(buf)))
    buf += data

cerrar()
man[KEY] = {"v": 1, "h": stamp.hexdigest(), "packs": packs}

os.makedirs(os.path.dirname(DST), exist_ok=True)
with open(DST, "w") as o:
    json.dump(man, o)
print("%d entradas, %d paquetes con %d ficheros, %d sueltos, sello=%s -> %s" % (
    len(entries), len(packs), empaquetados, len(entries) - empaquetados,
    man[KEY]["h"], DST))
