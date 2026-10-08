#!/usr/bin/env python3
"""Importa una IMG (par `.img`+`.dir`) dentro de una imagen suelta del port web.

El port sirve las IMG como **carpeta de ficheros sueltos + `.dir`**
(ver `CdStreamPosix`: `OdDirEntry{start, sectors, name}` y `OdDoRead`, que
resuelve sector -> entrada por rango y abre `<base>/<name>`). Por eso importar
una IMG de un mod es:

1. escribir cada entrada como fichero suelto en la carpeta destino;
2. actualizar su entrada del `.dir` (o **añadirla al final** si es nueva);
3. reempaquetar los `start` de forma consecutiva, como un `.img` real.

Detalles que importan:

- **El orden de las entradas existentes NO cambia**: el motor indexa por
  *nombre* (`CStreaming::LoadCdDirectory` busca el modelo por nombre y le
  asocia la posición del `.dir`), así que añadir al final es seguro. Reordenar
  sí movería `start` de todo y no aporta nada.
- Los nombres del `.dir` pueden ir en otra caja que los ficheros sueltos; la
  comparación de nombres es sin distinguir mayúsculas.
- Sin `--append`, las entradas nuevas se informan pero **no** se añaden (útil
  para ver el impacto antes de tocar nada).

Uso:
    python tools/import_img.py <src.img> <carpeta_destino> [--append] [--dry-run]

Ejemplo:
    python tools/import_img.py <mod>/cdimages/objects.img \
        gta_vc_browser/streamed/models/gta3.img --append
"""
import argparse
import os
import struct
import sys

SECTOR = 2048
ENTRY = 32


def read_img(img_path):
    """[(nombre, bytes)] de un par .img/.dir."""
    dir_path = os.path.splitext(img_path)[0] + ".dir"
    if not os.path.exists(dir_path):
        raise SystemExit("ERROR: no existe %s" % dir_path)
    raw = open(dir_path, "rb").read()
    data = open(img_path, "rb").read()
    out = []
    for i in range(0, len(raw) - (ENTRY - 1), ENTRY):
        start, sectors = struct.unpack_from("<II", raw, i)
        name = raw[i + 8:i + ENTRY].split(b"\0")[0].decode("latin1")
        if not name:
            continue
        off, ln = start * SECTOR, sectors * SECTOR
        if off + ln > len(data):
            print("  !! %s fuera del .img (start=%d sectors=%d)" % (name, start, sectors))
            continue
        out.append((name, data[off:off + ln]))
    return out


def read_dir(dir_path):
    """[[start, sectors, name]] de la carpeta destino (vacío si no hay .dir)."""
    if not os.path.exists(dir_path):
        return []
    raw = open(dir_path, "rb").read()
    out = []
    for i in range(0, len(raw) - (ENTRY - 1), ENTRY):
        start, sectors = struct.unpack_from("<II", raw, i)
        name = raw[i + 8:i + ENTRY].split(b"\0")[0].decode("latin1")
        out.append([start, sectors, name])
    return out


def write_dir(dir_path, entries):
    out = bytearray()
    for start, sectors, name in entries:
        out += struct.pack("<II", start, sectors)
        nm = name.encode("latin1")[:23]
        out += nm + b"\0" * (ENTRY - 8 - len(nm))
    with open(dir_path, "wb") as f:
        f.write(out)


def main():
    ap = argparse.ArgumentParser(description="Importa una IMG dentro de una imagen suelta del port.")
    ap.add_argument("src", help="ruta del .img de origen (p.ej. mod/cdimages/objects.img)")
    ap.add_argument("dst", help="carpeta destino (p.ej. streamed/models/gta3.img)")
    ap.add_argument("--dir", dest="dirpath", help="ruta del .dir destino (por defecto, al lado)")
    ap.add_argument("--append", action="store_true", help="añadir al .dir las entradas nuevas")
    ap.add_argument("--dry-run", action="store_true", help="no escribir nada, solo informar")
    a = ap.parse_args()

    folder = a.dst.rstrip("/\\")
    dirpath = a.dirpath or (os.path.splitext(folder)[0] + ".dir")
    entries = read_dir(dirpath)
    index = {e[2].lower(): i for i, e in enumerate(entries)}

    src = read_img(a.src)
    cambiado, nuevo, identico = [], [], []
    for name, content in src:
        key = name.lower()
        path = os.path.join(folder, name)
        old = open(path, "rb").read() if os.path.exists(path) else None
        estado = None
        if old is not None and old == content:
            identico.append(name)
            estado = "igual"
        elif key in index:
            cambiado.append((name, len(old) if old else 0, len(content)))
            estado = "cambia"
        else:
            nuevo.append((name, len(content)))
            estado = "nuevo"
        if a.dry_run:
            continue
        os.makedirs(folder, exist_ok=True)
        with open(path, "wb") as f:
            f.write(content)
        if estado == "nuevo":
            if a.append:
                entries.append([0, 0, name])
                index[key] = len(entries) - 1
        else:
            e = entries[index[key]]
            e[1] = (len(content) + SECTOR - 1) // SECTOR

    if not a.dry_run:
        start = 0
        for e in entries:
            e[0], sec = start, e[1]
            # tamaño real del fichero (puede haber cambiado de forma externa)
            p = os.path.join(folder, e[2])
            if not os.path.isfile(p):
                cand = os.path.join(folder, e[2].lower())
                if os.path.isfile(cand):
                    p = cand
            if os.path.isfile(p):
                e[1] = (os.path.getsize(p) + SECTOR - 1) // SECTOR
            start += e[1]
        write_dir(dirpath, entries)

    print("%s: %d entradas (%d iguales, %d cambiadas, %d nuevas)" % (a.src, len(src), len(identico), len(cambiado), len(nuevo)))
    for n, o, c in cambiado:
        print("   cambia  %-28s %9d -> %9d" % (n, o, c))
    for n, s in nuevo:
        print("   %s  %-28s %9d" % ("añade  " if a.append else "NUEVA* ", n, s))
    if nuevo and not a.append:
        print("   (*) sin --append no se añaden al .dir: el motor no las verá")
    print("destino: %s  (%d entradas en el .dir%s)" % (folder, len(entries), ", no escrito (dry-run)" if a.dry_run else ""))


if __name__ == "__main__":
    main()
