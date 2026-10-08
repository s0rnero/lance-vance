#!/usr/bin/env python3
"""Copia a bootseed/ los ficheros de bootseed.list (paquete inicial ~130MB).

La lista adapta preload_files.list estilo dos.zone (vc-assets/local/X -> X)
+ sfx.RAW/SDT (banco de audio del arranque).
"""
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "streamed")
DST = os.path.join(ROOT, "bootseed")
LST = os.path.join(ROOT, "bootseed.list")


# Directorios pequeños que siempre van enteros (textos, HUD, datos, neo).
FULL_DIRS = ["TEXT", "txd", "data", "neo", "skins"]
# Ficheros sueltos siempre incluidos (caché de longitudes de audio: evita
# abrir+medir ~50 pistas en cada arranque).
FULL_FILES = ["Audio/sound.cache"]
# Radios enteras en el arranque: REVERTIDO 18/09 (plan fluides-ram). Las 9
# (274 MB) vivían en MEMFS para siempre y eran el 70% del bootseed; con
# prefetchRadios() (25 s tras el arranque) + caché IDB, cambiar de emisora
# cuesta 1-2 s solo la primera vez. FLASH queda vía bootseed.list (menú).
RADIOS = []


def add_file(rel, counter):
    s = os.path.join(SRC, rel)
    if not os.path.exists(s):
        print("FALTA en streamed: %s" % rel)
        return
    t = os.path.join(DST, rel)
    os.makedirs(os.path.dirname(t), exist_ok=True)
    shutil.copy2(s, t)
    counter[0] += 1
    counter[1] += os.path.getsize(t)


def main():
    shutil.rmtree(DST, ignore_errors=True)
    counter = [0, 0]
    with open(LST) as f:
        for line in f:
            line = line.strip()
            if line:
                add_file(line, counter)
    for d in FULL_DIRS:
        src = os.path.join(SRC, d)
        if not os.path.isdir(src):
            continue
        for root, dirs, files in os.walk(src):
            for fn in files:
                add_file(os.path.relpath(os.path.join(root, fn), SRC), counter)
    for rel in FULL_FILES:
        add_file(rel, counter)
    for st in RADIOS:
        got = False
        for cand in ("Audio/%s.adf" % st, "Audio/%s.ADF" % st):
            s = os.path.join(SRC, cand)
            if os.path.exists(s):
                add_file(cand, counter)
                got = True
                break
        if not got:
            print("FALTA radio: %s" % st)
    # Stamp para el build: bootseed/ entra en reVC.data vía --preload-file y
    # ninja no vigila su contenido (solo la ruta), así que sin este fichero un
    # restage tras compilar no reenlaza y el juego sirve datos viejos. Es
    # LINK_DEPENDS del ejecutable (ver src/CMakeLists.txt).
    with open(os.path.join(ROOT, "bootseed.stamp"), "w") as f:
        f.write("staged=%d bytes=%d\n" % (counter[0], counter[1]))
    print("staged %d -> %.1f MB" % (counter[0], counter[1] / 1048576))


if __name__ == "__main__":
    main()
