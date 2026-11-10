#!/usr/bin/env python3
"""Compara, para cada pieza de cada vehiculo, la malla que dibuja el motor
(chunk 0x50E del DFF) con la lista de triangulos del propio DFF.

El motor dibuja la malla (0x50E); la lista de triangulos es la referencia de
lo que el modelo debe ser. Cuando no coinciden, el coche muestra triangulos
estirados o huecos aunque sus triangleos originales esten bien. Los modelos
del mod se convirtieron con una herramienta que sustituyo algunos indices de
las tiras por 0 (marcador de costura); si esa costura no se resuelve, el
triangulo se dibuja contra el vertice 0 y atraviesa la cabina.

Uso:
    python tools/vehicle_mesh_audit.py                 # resumen por coche
    python tools/vehicle_mesh_audit.py admiral police  # detalle de modelos
    python tools/vehicle_mesh_audit.py --all           # lista cada pieza mala
"""
import collections
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dff_geom as dg
import indicator_audit as ia

MODELS = Path(__file__).resolve().parent.parent / "streamed" / "models" / "gta3.img"
MESH = 0x50E
TRISTRIP = 1


class Bad(Exception):
    pass


def raw_geometries(buf):
    """Chunks 0x0F en el mismo orden que dg.parse (lista + sueltas del clump)."""
    clump = None
    for c in dg.children(buf, 0, len(buf)):
        if c.type == 0x10:
            clump = c
            break
    if clump is None:
        raise Bad("sin clump")
    out = []
    for c in dg.children(buf, clump.start, clump.end):
        if c.type == 0x1A:
            out.extend(g for g in dg.children(buf, c.start, c.end) if g.type == 0x0F)
        elif c.type == 0x0F:
            out.append(c)
    return out


def mesh_chunk(buf, geom):
    ext = dg.first(buf, geom, 3)
    if ext is None:
        return None
    for c in dg.children(buf, ext.start, ext.end):
        if c.type == MESH:
            return c
    return None


def read_mesh(buf, geom, numverts):
    """(flags, [(matIndex, indices)], errores) de la malla de una geometria."""
    c = mesh_chunk(buf, geom)
    if c is None:
        return None, [], ["sin malla 0x50E"]
    body = buf[c.start:c.end]
    if len(body) < 12:
        return None, [], ["malla de %d bytes" % len(body)]
    flags, count, total = struct.unpack_from("<III", body, 0)
    if count > 65535 or 12 + 8 * count > len(body):
        return flags, [], ["cabecera de malla invalida (meshes=%d)" % count]
    p = 12
    meshes = []
    errors = []
    seen = 0
    for m in range(count):
        if p + 8 > len(body):
            errors.append("entrada de malla %d fuera del chunk" % m)
            break
        n, mat = struct.unpack_from("<Ii", body, p)
        p += 8
        if n == 0 or p + 4 * n > len(body):
            errors.append("malla %d con %d indices" % (m, n))
            break
        ind = struct.unpack_from("<%dI" % n, body, p)
        p += 4 * n
        seen += n
        if mat < 0:
            errors.append("malla %d con material %d" % (m, mat))
        if any(v >= numverts for v in ind):
            errors.append("malla %d con indices fuera de rango" % m)
        meshes.append((mat, ind))
    if p != len(body):
        errors.append("sobran %d bytes de malla" % (len(body) - p))
    if seen != total:
        errors.append("totalIndices=%d y suma=%d" % (total, seen))
    return flags, meshes, errors


def canon(t):
    a, b, c = t
    return min((a, b, c), (b, c, a), (c, a, b))


def expand(flags, meshes):
    """Triangulos que el motor dibuja: [(mat, (v0,v1,v2))], sin degenerados."""
    out = []
    degenerate = 0
    for mat, ind in meshes:
        if flags == TRISTRIP:
            for i in range(len(ind) - 2):
                a, b, c = ind[i:i + 3]
                if a == b or b == c or a == c:
                    degenerate += 1
                    continue
                if i & 1:
                    a, b = b, a
                out.append((mat, (a, b, c)))
        else:
            for i in range(0, len(ind) - 2, 3):
                a, b, c = ind[i:i + 3]
                if a == b or b == c or a == c:
                    degenerate += 1
                    continue
                out.append((mat, (a, b, c)))
    return out, degenerate


def edge(g, t):
    v = g["verts"]
    try:
        return max(math.dist(v[t[0]], v[t[1]]), math.dist(v[t[1]], v[t[2]]),
                   math.dist(v[t[2]], v[t[0]]))
    except IndexError:
        return float("inf")


def audit(model, detail=False):
    path = MODELS / (model + ".dff")
    if not path.exists():
        return None
    buf = path.read_bytes()
    frames, geoms, atomics = dg.parse(buf)
    raws = raw_geometries(buf)
    if len(raws) != len(geoms):
        print("%s: %d geometrias y %d chunks" % (model, len(geoms), len(raws)))
        return None
    rows = []
    for fi, gi in atomics:
        g = geoms[gi]
        if not g["tris"] or len(g["verts"]) < 4:
            continue
        frame = frames[fi]["name"]
        flags, meshes, errors = read_mesh(buf, raws[gi], len(g["verts"]))
        drawn, degenerate = expand(flags, meshes)
        source = collections.Counter((t[3], canon(t[:3])) for t in g["tris"]
                                     if len(set(t[:3])) == 3)
        shown = collections.Counter((m, canon(t)) for m, t in drawn)
        extra = shown - source
        missing = source - shown
        bad = [t for t in (k for k in extra) if edge(g, t[1]) > 0.8] if extra else []
        rows.append({
            "model": model, "frame": frame, "frameIndex": fi, "geom": gi,
            "flags": flags, "meshes": len(meshes), "tris": len(g["tris"]),
            "drawn": sum(shown.values()), "source": sum(source.values()),
            "extra": sum(extra.values()), "missing": sum(missing.values()),
            "degenerate": degenerate, "errors": errors,
            "stretched": len(bad), "maxEdge": max([edge(g, t[1]) for t in bad], default=0.0),
        })
    return rows


def main(argv):
    names = [a for a in argv if not a.startswith("--")]
    show_all = "--all" in argv
    models = names or ia.vehicle_models()
    report = []
    for model in models:
        rows = audit(model, detail=bool(names))
        if not rows:
            continue
        report.extend(rows)
        for r in rows:
            if names:
                print("%-12s %-24s flags=%d meshes=%d tris=%d (dibuja %d) extra=%d falta=%d "
                      "degen=%d estirados=%d max=%.2f %s" % (
                          r["model"], r["frame"], r["flags"], r["meshes"], r["tris"],
                          r["drawn"], r["extra"], r["missing"], r["degenerate"],
                          r["stretched"], r["maxEdge"], "; ".join(r["errors"])))
    broken = [r for r in report if r["extra"] or r["missing"] or r["errors"]]
    cars = sorted({r["model"] for r in broken})
    print("piezas con malla distinta de sus triangulos: %d en %d coches"
          % (len(broken), len(cars)))
    counts = collections.Counter(r["model"] for r in broken)
    for model, n in counts.most_common():
        print("  %-14s %d" % (model, n))
    if show_all:
        print()
        for r in sorted(broken, key=lambda r: -r["stretched"]):
            print("%-12s %-24s extra=%d falta=%d estirados=%d max=%.2f" % (
                r["model"], r["frame"], r["extra"], r["missing"], r["stretched"], r["maxEdge"]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
