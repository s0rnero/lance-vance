#!/usr/bin/env python3
"""Ensambla gta_vc_browser/streamed/ con datos sueltos listos para servir.

- Copia assets/ (copia legal del GTA VC PC: sin movies/, mp3/ de usuario,
  DirectX/, mss/, docs).
- Encima gamefiles/ (GXT actualizados, particle/generic, neo/, freeroam...).
  (Esto repara lo que el file_packager descartaba por duplicados.)
- Encima radio_light/*.adf como Audio/*.adf (radios ligeras 22kHz).
- Mantiene gta3.img/cuts.img/sfx.RAW (el motor actual los necesita hasta
  el paso 3) ADEMAS de sus versiones troceadas.

Uso: python gta_vc_browser/tools/build_streamed.py
"""
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VC = os.path.join(ROOT, "assets")
GF = os.path.join(ROOT, "gamefiles")
OUT = os.path.join(ROOT, "streamed")
RADIO = os.path.join(ROOT, "radio_light")

SKIP_DIRS = {"movies", "mp3", "directx", "mss", "readme", "icons"}
SKIP_FILES = {"gta-vc.exe", "mss32.dll", "elamigos.jpg", "grandvice.ico",
              "readme.txt",
              # Generados por split_sfx/fix_sdt_*: NO pisar con vainilla (los
              # rates/tamaños vainilla desafinan todo: 8000Hz a 5000Hz etc.).
              "sfx.SDT"}


def copy_tree(src, dst):
    n = 0
    for root, dirs, files in os.walk(src):
        dirs[:] = [d for d in dirs if d.lower() not in SKIP_DIRS]
        for f in files:
            if f.lower() in SKIP_FILES:
                continue
            s = os.path.join(root, f)
            rel = os.path.relpath(s, src)
            t = os.path.join(dst, rel)
            # Colisión archivo/directorio (p. ej. cuts.img archivo vs dir
            # extraído): el motor on-demand lee SUELTO gta3 y ARCHIVO cuts.
            # No meter archivos dentro de dirs extraídos (quedan inalcanzables
            # y el manifiesto miente).
            if os.path.isdir(t):
                print("salto (es dir extraído): %s" % rel)
                continue
            os.makedirs(os.path.dirname(t), exist_ok=True)
            shutil.copy2(s, t)
            n += 1
    return n


def main():
    n1 = copy_tree(VC, OUT)
    print("vc -> streamed: %d ficheros" % n1)
    if os.path.isdir(GF):
        n2 = copy_tree(GF, OUT)
        print("gamefiles encima: %d ficheros" % n2)
    if os.path.isdir(RADIO):
        dst = os.path.join(OUT, "Audio")
        os.makedirs(dst, exist_ok=True)
        n3 = 0
        for f in os.listdir(RADIO):
            if f.lower().endswith(".adf"):
                shutil.copy2(os.path.join(RADIO, f), os.path.join(dst, f))
                n3 += 1
        print("radios ligeras: %d" % n3)
    total_n, total_s = 0, 0
    for root, dirs, files in os.walk(OUT):
        for f in files:
            total_n += 1
            total_s += os.path.getsize(os.path.join(root, f))
    print("TOTAL streamed: %d ficheros, %.1f MB" % (total_n, total_s / 1048576))


if __name__ == "__main__":
    main()
