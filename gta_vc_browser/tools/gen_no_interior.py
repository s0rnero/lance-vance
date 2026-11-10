#!/usr/bin/env python3
"""Genera src/vehicles/NoInteriorModelTable.h: los vehiculos sin interior.

Por que hace falta: el modo 1a persona de un coche coloca la camara en el hueco
que hay por delante del asiento del conductor (`ped_frontseat`) y mira a traves
del salpicadero del propio modelo. Los DFF servidos no llevan ninguna bandera de
"tiene interior" (no existe `m_bNoInterior`, ni bit de IDE, ni texdic `carinfo`:
comprobado sobre src/modelinfo/ en el plan 56.0), asi que la unica senal real es
la geometria.

La medida (la misma que pide el plan 56.0 T4): numero de caras cuyo centroide
cae dentro de la cabina del conductor, en el espacio local del modelo,

    |x - sx| <= 0,55   sy + 0,15 <= y < sy + 1,30   sz + 0,10 <= z < sz + 1,00

siendo (sx,sy,sz) la posicion mundial del marco `ped_frontseat` del propio DFF.
Es el volumen por delante del pecho del conductor: salpicadero, consola,
parabrisas interior. Umbral: menos de 20 caras = no hay interior.

Medido sobre los 115 modelos de vehiculo del IDE servido, de menor a mayor:
rcbandit 0, sanchez 0, manchez 0, **coach 6**, **rhino 11**, coastg 24, dinghy
24, marquis 26, patriot 34, predator 36, barracks 36, rio 38, ambulan 54, bus
54... y la mediana de la flota es 113. El hueco entre 11 y 54 es el que sostiene
el umbral: no es una eleccion al azar ni una lista escrita a mano.

Motos, barcos y avionetas no salen aqui porque el motor ya tiene camara propia
para ellos; la lista solo se consulta en vehiculos de tipo coche.

Uso:
    python gta_vc_browser/tools/gen_no_interior.py [--out ARCHIVO] [--umbral 20]
"""
import argparse
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
ROOT = os.path.join(REPO, "streamed", "models", "gta3.img")
SRC = os.path.join(os.path.dirname(REPO), "src", "vehicles", "NoInteriorModelTable.h")
sys.path.insert(0, HERE)

import dff_geom as dg   # noqa: E402

SEATS = "ped_frontseat"


def cabin_faces(model):
    """(num_caras_dentro_de_la_cabina, posicion del asiento) o None si no mide."""
    path = os.path.join(ROOT, model + ".dff")
    if not os.path.exists(path):
        return None
    try:
        frames, geoms, atomics = dg.parse(open(path, "rb").read())
    except Exception:
        return None
    seat = None
    for f in frames:
        if (f["name"] or "").lower() == SEATS:
            m = dg.world_of(frames, f["i"])
            seat = (m[0][3], m[1][3], m[2][3])
            break
    if seat is None:
        return None
    sx, sy, sz = seat
    n = 0
    for a in atomics:
        fi, gi = a[0], a[1]
        if gi >= len(geoms) or not geoms[gi]["verts"]:
            continue
        g = geoms[gi]
        m = dg.world_of(frames, fi)
        for t in g["tris"]:
            vs = [g["verts"][k] for k in t[:3]]
            c = [sum(v[k] for v in vs) / 3.0 for k in range(3)]
            w = [sum(m[r][k] * c[k] for k in range(3)) + m[r][3] for r in range(3)]
            if abs(w[0] - sx) <= 0.55 and 0.15 <= w[1] - sy <= 1.30 and 0.10 <= w[2] - sz <= 1.00:
                n += 1
    return n, seat


def fleet():
    ide = os.path.join(REPO, "streamed", "data", "default.ide")
    names, sec = [], None
    for raw in io.open(ide, encoding="latin-1").read().replace("\r\n", "\n").split("\n"):
        line = raw.split("#")[0].strip()
        if not line:
            continue
        low = line.lower()
        if low in ("cars", "bikes", "boats", "planes", "helis", "trains"):
            sec = low
            continue
        if sec in ("cars", "bikes", "boats", "planes", "helis", "trains"):
            parts = [p.strip().lower() for p in low.split(",")]
            if len(parts) > 1 and parts[1] and not parts[1].isdigit():
                names.append(parts[1])
    out = []
    for n in names:
        if n not in out:
            out.append(n)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=SRC)
    ap.add_argument("--umbral", type=int, default=20)
    a = ap.parse_args()

    medida = []
    for m in sorted(fleet()):
        got = cabin_faces(m)
        if got is None:
            continue
        medida.append((m, got[0]))
    medida.sort(key=lambda r: (r[1], r[0]))
    sin_interior = [r for r in medida if r[1] < a.umbral]
    con_interior = [r for r in medida if r[1] >= a.umbral]
    siguiente = con_interior[0] if con_interior else None
    mediana = sorted(r[1] for r in medida)[len(medida) // 2] if medida else 0

    lineas = []
    lineas.append("// Generado por gta_vc_browser/tools/gen_no_interior.py -- no editar a mano.")
    lineas.append("// Vehiculos sin interior medidos sobre los DFF servidos: caras dentro del")
    lineas.append("// volumen de la cabina del conductor (delante del marco `ped_frontseat`):")
    lineas.append("//   |x-sx| <= 0,55   sy+0,15 <= y < sy+1,30   sz+0,10 <= z < sz+1,00")
    lineas.append("// Menos de %d caras en ese volumen = no hay a traves de donde mirar en 1a" % a.umbral)
    lineas.append("// persona. Modelos medidos: %d. El primero que SI tiene interior es %s con" % (
        len(medida), siguiente[0] if siguiente else "-"))
    lineas.append("// %d caras y la mediana de la flota medida es %d. No cae ningun modelo entre" % (
        siguiente[1] if siguiente else 0, mediana))
    lineas.append("// %d y %d: ese hueco es lo que sostiene el umbral, no una cifra a mano." % (
        sin_interior[-1][1] if sin_interior else 0, siguiente[1] if siguiente else 0))
    lineas.append("// El motor solo consulta esta lista en vehiculos de tipo coche; las filas de")
    lineas.append("// moto/avion quedan aqui inertes (su camara ya es otra).")
    lineas.append("#pragma once")
    lineas.append("")
    lineas.append("static const char *const s_odNoInteriorVehicles[] = {")
    for m, n in sin_interior:
        lineas.append("\t\"%s\",\t// %d caras en la cabina" % (m, n))
    lineas.append("};")

    with io.open(a.out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lineas) + "\n")
    print("modelos medidos: %d   sin interior: %s" % (
        len(medida), ", ".join("%s(%d)" % r for r in sin_interior)))
    print("-> %s" % a.out)


if __name__ == "__main__":
    main()
