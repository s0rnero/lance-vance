#!/usr/bin/env python3
"""Regenera el `.dir` de una imagen suelta (`<img>/` + `<img>.dir`) del port web.

El port sirve las IMG como **carpeta de ficheros sueltos + `.dir`**
(`CdStreamPosix`: `OdDirEntry{start, sectors, name}`; lee
`<base>/<name>` en el byte `(sector - start) * 2048`). Por eso, si se sustituye
un fichero por otro de distinto tamaño hay que recalcular dos cosas:

1. `sectors` de su entrada = ceil(tamaño_real / 2048), o el motor leerá de menos
   (modelo/textura truncados) o de más (relleno/ceros).
2. Los `start` de las siguientes: se reempaquetan consecutivos, que es como los
   sirve un `.img` real y como espera el lector de lecturas agrupadas.

El **orden de las entradas se conserva**: el motor mapea los IDs de streaming a
la entrada por índice, y los nombres son los del `.dir` original.

Uso:
    python tools/repack_dir.py gta_vc_browser/streamed/models/gta3.img
    python tools/repack_dir.py <carpeta> --dry-run
"""
import argparse
import os
import struct
import sys

SECTOR = 2048
ENTRY = 32


def read_dir(path):
    if not os.path.exists(path):
        # variante por si alguien pasa la carpeta con otro nombre
        alt = path[:-4] + ".img.dir" if path.endswith(".dir") else None
        if alt and os.path.exists(alt):
            path = alt
        else:
            return None
    raw = open(path, "rb").read()
    out = []
    for i in range(0, len(raw) - (ENTRY - 1), ENTRY):
        start, sectors = struct.unpack_from("<II", raw, i)
        name = raw[i + 8:i + ENTRY].split(b"\0")[0].decode("latin1")
        out.append([start, sectors, name])
    return out


def main():
    ap = argparse.ArgumentParser(description="Recalcula el .dir de una imagen suelta.")
    ap.add_argument("carpeta", help="p.ej. gta_vc_browser/streamed/models/gta3.img")
    ap.add_argument("--dir", dest="dirpath", help="ruta del .dir (por defecto, al lado)")
    ap.add_argument("--dry-run", action="store_true", help="no escribir, solo informar")
    a = ap.parse_args()

    carpeta = a.carpeta.rstrip("/\\")
    # Convención del port: la IMG es una CARPETA "models/gta3.img/" y el .dir va
    # con el nombre de la imagen cambiando la extensión ("models/gta3.dir").
    # Ver CdStreamPosix: toma base="models/gta3.img", ve que es directorio,
    # y sustituye ".img" por ".dir".
    dirpath = a.dirpath or (os.path.splitext(carpeta)[0] + ".dir")
    entries = read_dir(dirpath)
    if entries is None:
        raise SystemExit("ERROR: no existe %s (sin .dir no sé el orden de las entradas)" % dirpath)

    # Ficheros presentes que no estén en el .dir: se INFORMAN pero no se añaden
    # (el motor los carga por ruta, no por sector; meterlos alteraría el
    # mapeo ID->entrada). Los nombres del .dir pueden ir en otra caja que los
    # sueltos, así que se compara sin distinguir mayúsculas.
    conocidos = set(e[2].lower() for e in entries)
    sueltos = []
    for f in sorted(os.listdir(carpeta)):
        p = os.path.join(carpeta, f)
        if os.path.isfile(p) and f.lower() not in conocidos:
            sueltos.append((f, os.path.getsize(p)))
    if sueltos:
        print("%d ficheros sueltos sin entrada en el .dir (se dejan fuera): %s%s"
              % (len(sueltos), ", ".join(f for f, _ in sueltos[:6]),
                 " ..." if len(sueltos) > 6 else ""))

    crecidos = []
    nuevos = []
    faltan = []
    start = 0
    for e in entries:
        p = os.path.join(carpeta, e[2])
        if not os.path.isfile(p):                  # nombre del .dir en otra caja
            cand = os.path.join(carpeta, e[2].lower())
            if os.path.isfile(cand):
                p = cand
        if os.path.isfile(p):
            size = os.path.getsize(p)
            sec = (size + SECTOR - 1) // SECTOR
            if sec != e[1]:
                (crecidos if sec > e[1] else nuevos).append((e[2], e[1], sec))
            e[0], e[1] = start, sec
        else:
            faltan.append(e[2])
        start += e[1]

    out = bytearray()
    for e in entries:
        out += struct.pack("<II", e[0], e[1])
        nm = e[2].encode("latin1")[:23]
        out += nm + b"\0" * (ENTRY - 8 - len(nm))

    crecieron_MB = sum(n - o for _, o, n in crecidos) * SECTOR / 1048576
    print("entradas: %d   reempaquetadas: %d   total: %.1f MB"
          % (len(entries), len(entries), start * SECTOR / 1048576))
    print("sectores cambiados: %d mayores (%s), %d menores (%s)"
          % (len(crecidos), ", ".join("%s %d->%d" % c for c in crecidos[:6]),
             len(nuevos), ", ".join("%s %d->%d" % c for c in nuevos[:6])))
    if crecidos:
        print("crecimiento neto por entradas mayores: %.1f MB" % crecieron_MB)
    if faltan:
        print("AVISO: %d entradas sin fichero (se conservan tal cual): %s"
              % (len(faltan), ", ".join(faltan[:5])))
    if a.dry_run:
        print("[dry-run] no escribo %s" % dirpath)
        return
    with open(dirpath, "wb") as f:
        f.write(out)
    print("escrito %s (%d bytes)" % (dirpath, len(out)))


if __name__ == "__main__":
    main()
