#!/usr/bin/env python3
"""Comprueba el índice de una imagen suelta (`<img>/` + `<img>.dir`) del port web.

El motor resuelve cada sector pedido contra el `.dir` que leyó al arrancar y abre
el fichero de la entrada que lo posee. Si el índice no describe los ficheros que
de verdad se abren, dos modelos comparten sector y el motor recibe el DFF de otro:
la cabecera declara más bytes que el búfer y `rw::malloc_h` revienta el proceso
(plan 47.0). Esta puerta falla la build antes de que eso llegue al navegador.

Uso:
    python tools/check_img_index.py
    python tools/check_img_index.py --img gta_vc_browser/streamed/models/gta3.img \\
        --mirror gta_vc_browser/bootseed/models/gta3.dir
"""
import argparse
import hashlib
import json
import os
import struct
import sys

SECTOR = 2048
ENTRY = 32


def read_dir(path):
    raw = open(path, "rb").read()
    out = []
    for i in range(0, len(raw) - (ENTRY - 1), ENTRY):
        start, sectors = struct.unpack_from("<II", raw, i)
        name = raw[i + 8:i + ENTRY].split(b"\0")[0].decode("latin1")
        out.append((name, start, sectors))
    return out


def md5(path):
    return hashlib.md5(open(path, "rb").read()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description="Valida el .dir de una imagen suelta.")
    ap.add_argument("--img", default="gta_vc_browser/streamed/models/gta3.img",
                    help="carpeta de ficheros sueltos (la caja autoritativa)")
    ap.add_argument("--dir", dest="dirpath", help="el .dir (por defecto, al lado de la carpeta)")
    ap.add_argument("--mirror", action="append", default=[], metavar="DIR",
                    help="copias del .dir que deben ser byte a byte idénticas")
    ap.add_argument("--manifest", default="gta_vc_browser/web/public/manifest.json")
    ap.add_argument("--bootseed-list", default="gta_vc_browser/bootseed.list")
    a = ap.parse_args()

    carpeta = a.img.rstrip("/\\")
    dirpath = a.dirpath or (os.path.splitext(carpeta)[0] + ".dir")
    errores = []
    avisos = []

    if not os.path.exists(dirpath):
        raise SystemExit("ERROR: no existe %s" % dirpath)
    entries = read_dir(dirpath)

    huecos = []
    solapes = []
    for i in range(1, len(entries)):
        prev = entries[i - 1]
        esperado = prev[1] + prev[2]
        if entries[i][1] > esperado:
            huecos.append((entries[i][0], entries[i][1], esperado))
        elif entries[i][1] < esperado:
            solapes.append((prev[0], entries[i][0], entries[i][1], esperado))
    if huecos:
        errores.append("%d entradas con HUECO (empiezan después de donde terminó la anterior): %s"
                       % (len(huecos), ", ".join("%s start=%d≠%d" % h for h in huecos[:5])))
    if solapes:
        errores.append("%d entradas con SOLAPE (dos modelos reclaman el mismo sector): %s"
                       % (len(solapes), ", ".join("%s|%s start=%d≠%d" % s for s in solapes[:5])))

    duplicados = {}
    malos = []
    ausentes = []
    es_caja = os.path.isdir(carpeta)
    narchivos = len([f for f in os.listdir(carpeta) if os.path.isfile(os.path.join(carpeta, f))]) if es_caja else 0
    if not es_caja:
        avisos.append("%s no es una carpeta de ficheros sueltos: es archivo, solo se comprueba el indice" % carpeta)
    for name, start, sectors in entries:
        duplicados.setdefault(name.lower(), []).append(name)
        if not es_caja:
            continue
        p = os.path.join(carpeta, name)
        if not os.path.isfile(p):
            p2 = os.path.join(carpeta, name.lower())
            p = p2 if os.path.isfile(p2) else None
        if p is None:
            ausentes.append(name)
            continue
        real = (os.path.getsize(p) + SECTOR - 1) // SECTOR
        if real != sectors:
            malos.append((name, sectors, real))
    if malos:
        errores.append("%d entradas con sectores que no cuadran con el fichero servido: %s"
                       % (len(malos), ", ".join("%s índice=%d fichero=%d" % m for m in malos[:5])))
    if ausentes:
        avisos.append("%d entradas sin fichero en %s (el índice reserva su sitio; se sirven bajo demanda): %s"
                      % (len(ausentes), carpeta, ", ".join(ausentes[:5])))
    amb = {k: v for k, v in duplicados.items() if len(v) > 1}
    if amb:
        avisos.append("%d nombres que sólo se distinguen en mayúsculas (un FS sensible a mayúsculas "
                      "abre un fichero distinto del que abre este): %s"
                      % (len(amb), ", ".join("/".join(v) for v in list(amb.values())[:4])))

    total = sum(e[2] for e in entries)
    ultimo = entries[-1][1] + entries[-1][2]
    if total != ultimo:
        errores.append("el total de sectores (%d) no coincide con el final de la última entrada (%d)"
                       % (total, ultimo))

    for mirror in a.mirror:
        if not os.path.exists(mirror):
            errores.append("falta la copia %s" % mirror)
        elif md5(mirror) != md5(dirpath):
            errores.append("%s no es byte a byte igual a %s (md5 %s vs %s): el motor lee el índice "
                           "hornado en el .data, así que esa copia es la que resuelve los sectores"
                           % (mirror, dirpath, md5(mirror)[:8], md5(dirpath)[:8]))

    rel = os.path.splitext(carpeta)[0] + ".dir"
    parts = rel.replace("\\", "/").split("/")
    if "streamed" in parts:
        rel = "/".join(parts[parts.index("streamed") + 1:])
    if os.path.exists(a.manifest):
        man = json.load(open(a.manifest))
        key = next((k for k in man if k.lower() == rel.lower()), None)
        if key is None:
            errores.append("%s no está en %s: bajo demanda no se puede refetch" % (rel, a.manifest))
        else:
            size = man[key].get("s") if isinstance(man[key], dict) else man[key]
            if size is not None and size != os.path.getsize(dirpath):
                errores.append("%s declara %d bytes en el manifiesto y el fichero tiene %d"
                               % (rel, size, os.path.getsize(dirpath)))
    if os.path.exists(a.bootseed_list):
        listed = set(l.strip().lower() for l in open(a.bootseed_list) if l.strip())
        if rel.lower() not in listed:
            avisos.append("%s no está en %s: no va en el paquete de arranque" % (rel, a.bootseed_list))

    print("%s: %d entradas, %d sectores (%.1f MB), %d ficheros en %s"
          % (dirpath, len(entries), total, total * SECTOR / 1048576.0, narchivos, carpeta))
    for w in avisos:
        print("AVISO: %s" % w)
    for e in errores:
        print("ERROR: %s" % e)
    if errores:
        raise SystemExit(1)
    print("OK: índice contiguo, sin solapes y con las copias sincronizadas")


if __name__ == "__main__":
    sys.exit(main())
