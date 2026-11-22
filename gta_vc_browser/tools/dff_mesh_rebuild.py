#!/usr/bin/env python3
"""Reconstruye la malla 0x50E de cada geometria desde su lista de triangulos.

El motor no dibuja el STRUCT: dibuja el chunk `0x50E` de la extension
(`vendor/librw/src/gl/gl3pipe.cpp`, `instanceMesh`). Los pases 61, 89 y 90
editaron la lista de triangulos y la malla se quedo atras, asi que el coche
seguia igual en pantalla: las caras retiradas se seguian dibujando, y las
tiradas del trasplante dibujaban sus costuras (indices a 0) contra el vertice 0,
que es de donde salen los triangulos estirados de la carroceria.

Este pase escribe la malla en modo lista y desde la lista de triangulos, y apaga
el bit de tira del STRUCT. No toca la lista, ni vertices, ni materiales, ni UVs,
ni ningun otro chunk de la extension. Una geometria que ya coincide se deja
byte a byte.

Uso:
    python tools/dff_mesh_rebuild.py --report
    python tools/dff_mesh_rebuild.py pheonix taxi --report
    python tools/dff_mesh_rebuild.py --all --install
"""
import argparse
import collections
import math
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import dff_geom as dg
import indicator_audit as ia
import vehicle_mesh_audit as vm
import xbox_transplant as xt

GB = HERE.parent
SERV = GB / "streamed" / "models" / "gta3.img"
TREES = [SERV, GB / "gamefiles" / "models" / "gta3.img",
         GB / "bootseed" / "models" / "gta3.img"]
OUT = GB / "tmp" / "base91" / "fix"
BACKUP = GB / "tmp" / "base91" / "pre91"
MESH = 0x50E


def build_mesh(tris):
    groups = {}
    order = []
    for (a, b, c, m) in tris:
        if a == b or b == c or a == c:
            continue
        if m not in groups:
            groups[m] = []
            order.append(m)
        groups[m].extend((a, b, c))
    body = struct.pack("<III", 0, len(order), sum(len(groups[m]) for m in order))
    for m in order:
        idx = groups[m]
        body += struct.pack("<Ii", len(idx), m)
        body += struct.pack("<%dI" % len(idx), *idx)
    return body


def with_mesh(ext, mesh_body):
    body = b""
    found = False
    for c in xt.chunks(ext.body, 0, len(ext.body)):
        if c.typ == MESH:
            body += xt.ch(MESH, c.ver, mesh_body)
            found = True
        else:
            body += xt.ch(c.typ, c.ver, c.body)
    return body if found else None


def rebuild_geometry(gchunk, tris):
    body = b""
    ok = False
    for c in xt.chunks(gchunk.body, 0, len(gchunk.body)):
        if c.typ == 0x01:
            st = bytearray(c.body)
            flags = struct.unpack_from("<I", st, 0)[0]
            struct.pack_into("<I", st, 0, flags & ~1)
            body += xt.ch(0x01, c.ver, bytes(st))
        elif c.typ == 0x03:
            new = with_mesh(c, build_mesh(tris))
            if new is None:
                return None
            body += xt.ch(0x03, c.ver, new)
            ok = True
        else:
            body += xt.ch(c.typ, c.ver, c.body)
    return xt.Chunk(0x0F, gchunk.ver, body, 0, 0) if ok else None


def measure(buf):
    frames, geoms, atomics = dg.parse(buf)
    raws = vm.raw_geometries(buf)
    if len(raws) != len(geoms):
        return None, []
    rows = []
    for fi, gi in atomics:
        g = geoms[gi]
        flags, meshes, errors = vm.read_mesh(buf, raws[gi], len(g["verts"]))
        drawn, degen = vm.expand(flags, meshes)
        source = collections.Counter((t[3], vm.canon(t[:3])) for t in g["tris"]
                                     if len(set(t[:3])) == 3)
        shown = collections.Counter((m, vm.canon(t)) for m, t in drawn)
        extra = shown - source
        rows.append({
            "gi": gi, "frame": frames[fi]["name"], "flags": flags,
            "source": sum(source.values()), "shown": sum(shown.values()),
            "extra": sum(extra.values()), "missing": sum((source - shown).values()),
            "stretched": sum(1 for t in extra if vm.edge(g, t[1]) > 0.8),
            "degenerate": degen, "errors": errors,
        })
    return geoms, rows


def process(model):
    src = SERV / (model + ".dff")
    if not src.exists():
        return None
    buf = src.read_bytes()
    geoms, rows = measure(buf)
    if geoms is None:
        return {"model": model, "skip": "geometrias y chunks no cuadran"}
    target = xt.parse_dff(str(src))
    if len(target["geoms"]) != len(geoms):
        return {"model": model, "skip": "geometrias sueltas en el clump"}
    bad = {r["gi"] for r in rows if r["extra"] or r["missing"] or r["errors"]}
    if not bad:
        return {"model": model, "clean": True, "before": rows}
    new_geoms = list(target["geoms"])
    done = 0
    for gi in sorted(bad):
        g = new_geoms[gi]
        rebuilt = rebuild_geometry(g, geoms[gi]["tris"])
        if rebuilt is None:
            continue
        new_geoms[gi] = rebuilt
        done += 1
    if done == 0:
        return {"model": model, "skip": "ninguna geometria reescribible"}
    target["geoms"] = new_geoms
    OUT.mkdir(parents=True, exist_ok=True)
    dst = OUT / (model + ".dff")
    dst.write_bytes(xt.rebuild(target))
    _, after = measure(dst.read_bytes())
    return {"model": model, "done": done, "before": rows, "after": after,
            "path": dst}


def install(only):
    ok = 0
    for p in sorted(OUT.glob("*.dff")):
        if only and p.stem not in only:
            continue
        for tree in TREES:
            f = tree / p.name
            if f.exists():
                shutil.copy2(p, f)
        ok += 1
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("models", nargs="*")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--install", action="store_true")
    a = ap.parse_args()
    models = a.models or ia.vehicle_models()
    BACKUP.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    tot = collections.Counter()
    written = set()
    for mo in models:
        src = SERV / (mo + ".dff")
        if src.exists():
            bk = BACKUP / (mo + ".dff")
            if not bk.exists():
                shutil.copy2(src, bk)
        r = process(mo)
        if r is None:
            continue
        if r.get("skip"):
            print("%-12s se salta: %s" % (mo, r["skip"]))
            tot["skipped"] += 1
            continue
        if r.get("clean"):
            tot["clean"] += 1
            continue
        before = {x["gi"]: x for x in r["before"]}
        lines = []
        for x in r["after"]:
            b = before.get(x["gi"])
            if not b or not (b["extra"] or b["missing"]):
                continue
            lines.append("%s antes extra=%d estirados=%d -> despues extra=%d "
                         "falta=%d" % (x["frame"], b["extra"], b["stretched"],
                                       x["extra"], x["missing"]))
        print("%-12s %d geometrias | %s" % (mo, r["done"], "; ".join(lines)))
        tot["cars"] += 1
        tot["geoms"] += r["done"]
        written.add(mo)
    print("\nTOTAL %d coches, %d geometrias reescritas, %d limpios, %d saltados"
          % (tot["cars"], tot["geoms"], tot["clean"], tot["skipped"]))
    if a.report or not a.install:
        print("DRY: no se instala nada (usa --install)")
        return
    n = install(written)
    subprocess.run([sys.executable, str(HERE / "repack_dir.py"), str(SERV),
                    "--mirror", str(GB / "bootseed" / "models" / "gta3.dir")],
                   check=True)
    shutil.copy2(GB / "bootseed" / "models" / "gta3.dir",
                 GB / "gamefiles" / "models" / "gta3.dir")
    print("instalado en %d modelos; gta3.dir reempaquetado" % n)


if __name__ == "__main__":
    main()
