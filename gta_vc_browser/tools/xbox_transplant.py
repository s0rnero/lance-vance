#!/usr/bin/env python3
"""Trasplanta nodos de volante e intermitentes del modelo actual al modelo Xbox.

El motor no guarda el volante ni las lentes en una tabla aparte: los busca por
nombre dentro del clump (`steeringwheel`, `indicator_lf`, ...). Por eso el
trasplante es una operacion de DFF, no una capa de runtime:

  * copia el frame donante (matriz local + padre) al frame list destino;
  * copia su(s) atomic(s) y la geometria cruda (vertices, materiales, UVs);
  * conserva la textura del material (el TXD destino debe tenerla);
  * actualiza los contadores de frame list, geometry list y atomics.

El piloto escribe en `tmp/base60/pilot/`; no toca `streamed` ni `bootseed`
hasta que se pase `--install`. El donante se lee de `streamed` (el modelo que
hoy tiene las features) y el destino del pack Xbox.

Uso:
    python tools/xbox_transplant.py --pilot banshee
    python tools/xbox_transplant.py --pilot banshee --feature wheel
    python tools/xbox_transplant.py --pilot banshee --install
    python tools/xbox_transplant.py --restore banshee
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
from collections import namedtuple

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import dff_geom as dg          # noqa: E402
import txd_merge as TM         # noqa: E402
import txd_raster as TR        # noqa: E402

ROOT = os.path.dirname(REPO)
DONOR = os.path.join(REPO, "streamed", "models", "gta3.img")
XBOX = os.path.join(ROOT, "new-mods", "models-cars-xbox",
                    "[VC][A610]Xbox-Cars-v0.91", "Script & Files")
WORK = os.path.join(REPO, "tmp", "base60")
PILOT = os.path.join(WORK, "pilot")
BACKUP = os.path.join(WORK, "backup")
TREES = [os.path.join(REPO, "streamed", "models", "gta3.img"),
         os.path.join(REPO, "bootseed", "models", "gta3.img"),
         os.path.join(REPO, "gamefiles", "models", "gta3.img")]

Chunk = namedtuple("Chunk", "typ ver body pos nxt")
FEATURES = ("steeringwheel",)
INDICATORS = ("indicator_lf", "indicator_rf", "indicator_lr", "indicator_rr",
              "indicator2_lf", "indicator2_rf", "indicator2_lr", "indicator2_rr")


def u32(buf, off):
    return struct.unpack_from("<I", buf, off)[0]


def chunks(buf, start, end):
    pos = start
    while pos + 12 <= end:
        typ, size, ver = struct.unpack_from("<III", buf, pos)
        nxt = pos + 12 + size
        if nxt > end:
            break
        yield Chunk(typ, ver, buf[pos + 12:nxt], pos, nxt)
        pos = nxt


def find(chs, typ):
    for c in chs:
        if c.typ == typ:
            return c
    return None


def ch(typ, ver, body):
    return struct.pack("<III", typ, len(body), ver) + body


def parse_dff(path):
    buf = open(path, "rb").read()
    top = list(chunks(buf, 0, len(buf)))
    clump = find(top, 0x10)
    if clump is None:
        raise SystemExit("%s: sin clump 0x10" % path)
    cc = list(chunks(clump.body, 0, len(clump.body)))
    cs = find(cc, 0x01)
    fl = find(cc, 0x0E)
    gl = find(cc, 0x1A)
    if cs is None or fl is None or gl is None:
        raise SystemExit("%s: clump incompleto" % path)
    fch = list(chunks(fl.body, 0, len(fl.body)))
    fs = find(fch, 0x01)
    name_bodies = [c.body for c in fch if c.typ == 0x03]
    names = []
    for nb in name_bodies:
        ext = find(list(chunks(nb, 0, len(nb))), 0x253F2FE)
        names.append(ext.body.split(b"\0")[0].decode("latin-1") if ext else "")
    count = u32(fs.body, 0)
    if len(names) != count:
        raise SystemExit("%s: frame list %d nombres %d" % (path, count, len(names)))
    records = [fs.body[4 + i * 56:4 + (i + 1) * 56] for i in range(count)]
    parents = [struct.unpack_from("<i", r, 48)[0] for r in records]
    gch = list(chunks(gl.body, 0, len(gl.body)))
    gs = find(gch, 0x01)
    geoms = [c for c in gch if c.typ == 0x0F]
    if u32(gs.body, 0) != len(geoms):
        raise SystemExit("%s: geometry list %d geoms %d" % (path, u32(gs.body, 0), len(geoms)))
    atomic_raw = []
    atoms = []
    for c in cc:
        if c.typ != 0x14:
            continue
        ach = list(chunks(c.body, 0, len(c.body)))
        ast = find(ach, 0x01)
        ext = [x for x in ach if x.typ == 0x03]
        frame, geom = struct.unpack_from("<II", ast.body, 0)
        atoms.append({"frame": frame, "geom": geom, "struct": ast.body,
                      "struct_ver": ast.ver, "ext": ext, "outer_ver": c.ver})
        atomic_raw.append((c.ver, c.body))
    return {
        "path": path, "buf": buf, "top": top, "top_extra": [c for c in top if c.typ != 0x10],
        "clump": clump, "cc": cc, "cs": cs, "fl": fl, "gl": gl, "fs": fs, "gs": gs,
        "names": names, "name_bodies": name_bodies, "parents": parents, "records": records, "geoms": geoms,
        "atoms": atoms, "atomic_raw": atomic_raw,
        "exts": [c for c in cc if c.typ == 0x03],
    }


def rebuild(target):
    fs_body = bytearray(struct.pack("<I", len(target["records"])))
    fs_body += b"".join(target["records"])
    fl = ch(0x01, target["fs"].ver, bytes(fs_body)) + b"".join(
        ch(0x03, 0xC02FFFF, nb) for nb in target["name_bodies"])
    gs_body = bytearray(struct.pack("<I", len(target["geoms"])))
    gl = ch(0x01, target["gs"].ver, bytes(gs_body)) + b"".join(
        ch(0x0F, g.ver, g.body) for g in target["geoms"])
    cs_body = bytearray(target["cs"].body)
    struct.pack_into("<I", cs_body, 0, len(target["atomic_raw"]))
    body = ch(0x01, target["cs"].ver, bytes(cs_body))
    body += ch(0x0E, target["fl"].ver, fl)
    body += ch(0x1A, target["gl"].ver, gl)
    body += b"".join(ch(0x14, ver, raw) for ver, raw in target["atomic_raw"])
    body += b"".join(ch(0x03, c.ver, c.body) for c in target["exts"])
    out = ch(0x10, target["clump"].ver, body)
    out += b"".join(ch(c.typ, c.ver, c.body) for c in target["top_extra"])
    return out


def add_feature(target, donor, name):
    if name in target["names"]:
        print("  %s: ya existe en destino; se salta" % name)
        return []
    try:
        dfi = donor["names"].index(name)
    except ValueError:
        print("  %s: no existe en donante; se salta" % name)
        return []
    dst_atoms = [a for a in donor["atoms"] if a["frame"] == dfi]
    if not dst_atoms:
        print("  %s: sin atomic (dummy); se salta" % name)
        return []
    dparent = donor["parents"][dfi]
    pname = donor["names"][dparent] if 0 <= dparent < len(donor["names"]) else ""
    if pname in target["names"]:
        pidx = target["names"].index(pname)
    elif "chassis_hi" in target["names"]:
        pidx = target["names"].index("chassis_hi")
        print("  %s: padre %r ausente; se cuelga de chassis_hi" % (name, pname))
    elif "chassis_dummy" in target["names"]:
        pidx = target["names"].index("chassis_dummy")
        print("  %s: padre %r ausente; se cuelga de chassis_dummy" % (name, pname))
    else:
        pidx = 0
        print("  %s: padre %r ausente; se cuelga de la raiz" % (name, pname))
    new_fi = len(target["records"])
    rec = bytearray(donor["records"][dfi])
    struct.pack_into("<i", rec, 48, pidx)
    target["records"].append(bytes(rec))
    target["names"].append(name)
    target["name_bodies"].append(ch(0x253F2FE, 0xC02FFFF, name.encode("latin-1")))
    donor_geoms = dg.parse(open(donor["path"], "rb").read())[1]
    texs = set()
    for a in dst_atoms:
        g = donor["geoms"][a["geom"]]
        new_gi = len(target["geoms"])
        target["geoms"].append(g)
        astruct = bytearray(a["struct"])
        struct.pack_into("<II", astruct, 0, new_fi, new_gi)
        abody = ch(0x01, a["struct_ver"], bytes(astruct)) + b"".join(
            ch(0x03, x.ver, x.body) for x in a["ext"])
        target["atomic_raw"].append((a["outer_ver"], abody))
        for _color, tex in donor_geoms[a["geom"]]["mats"]:
            if tex:
                texs.add(tex)
    print("  %s: frame %d padre %s, %d atomic(s), geom %s, texturas %s"
          % (name, new_fi, pname, len(dst_atoms),
             ",".join(str(len(target["geoms"]) - len(dst_atoms) + i)
                      for i in range(len(dst_atoms))),
             ",".join(sorted(texs)) or "-"))
    return sorted(texs)


def txf_names(path):
    return {n for _p, _q, n in TM.read_txd(path)[2]}


def cmd_pilot(model, feature, merge_textures):
    os.makedirs(PILOT, exist_ok=True)
    donor_dff = os.path.join(DONOR, model + ".dff")
    donor_txd = os.path.join(DONOR, model + ".txd")
    target_dff = os.path.join(XBOX, model + ".dff")
    target_txd = os.path.join(XBOX, model + ".txd")
    for p in (donor_dff, donor_txd, target_dff, target_txd):
        if not os.path.exists(p):
            raise SystemExit("falta %s" % p)
    donor = parse_dff(donor_dff)
    target = parse_dff(target_dff)
    if feature == "wheel":
        feats = ["steeringwheel"]
    elif feature == "indicators":
        feats = [n for n in INDICATORS if n in donor["names"]]
    else:
        feats = [n for n in ("steeringwheel",) + INDICATORS if n in donor["names"]]
    print("%s: donante %d frames/%d geoms, destino %d frames/%d geoms"
          % (model, len(donor["records"]), len(donor["geoms"]),
             len(target["records"]), len(target["geoms"])))
    texs = set()
    for n in feats:
        texs.update(add_feature(target, donor, n))
    out_dff = os.path.join(PILOT, model + ".dff")
    with open(out_dff, "wb") as f:
        f.write(rebuild(target))
    # TXD: se parte del destino. Si falta una textura del material donante, se
    # puede fusionar solo esa textura; nunca se sobrescribe una del destino.
    have = txf_names(target_txd)
    missing = sorted(t for t in texs if t not in have)
    out_txd = os.path.join(PILOT, model + ".txd")
    if missing and merge_textures:
        TM.merge(target_txd, donor_txd, out_txd, only=set(missing))
    else:
        shutil.copy2(target_txd, out_txd)
        if missing:
            print("  AVISO: texturas ausentes en destino: %s" % ", ".join(missing))
    print("  escrito %s (%d bytes) y %s" % (out_dff, os.path.getsize(out_dff), out_txd))
    # Validacion estructural
    r = subprocess.run([sys.executable, os.path.join(HERE, "dff_structver.py"),
                        "--check", "--quiet", out_dff],
                       capture_output=True, text=True)
    print("  dff_structver: rc=%d %s" % (r.returncode, (r.stdout + r.stderr).strip()[-300:]))
    fr, geoms, atoms = dg.parse(open(out_dff, "rb").read())
    by = {}
    for fi, gi in atoms:
        by.setdefault(fi, []).append(gi)
    for n in feats:
        if n not in [f["name"] for f in fr]:
            print("  FALLO: %s no quedo en el DFF" % n)
            continue
        fi = [i for i, f in enumerate(fr) if f["name"] == n][0]
        print("  %s: frame %d padre %s atomics %s" %
              (n, fi, fr[fr[fi]["parent"]]["name"] if 0 <= fr[fi]["parent"] < len(fr) else "-",
               [(g, len(geoms[g]["verts"]), len(geoms[g]["tris"])) for g in by.get(fi, [])]))
    return out_dff, out_txd


def backup(model, tree):
    src = os.path.join(tree, model + ".dff")
    if not os.path.exists(src):
        return
    dst = os.path.join(BACKUP, os.path.basename(os.path.dirname(os.path.dirname(tree))))
    os.makedirs(dst, exist_ok=True)
    for ext in (".dff", ".txd"):
        p = os.path.join(tree, model + ext)
        if os.path.exists(p) and not os.path.exists(os.path.join(dst, model + ext)):
            shutil.copy2(p, os.path.join(dst, model + ext))


def assert_pc_txd(path):
    """Rechaza TXD Xbox crudos: el port solo pinta 32 bits BGRA sin comprimir."""
    _, _, texs, _, data = TM.read_txd(path)
    bad = []
    for pos, _q, name in texs:
        info = TR.read_native(data, pos)
        if info is None or info["compression"] != 0 or info["depth"] != 32:
            bad.append(name)
    if bad or not texs:
        raise SystemExit("TXD no valido para el port (comprimido o no 32 bits): %s"
                         % (", ".join(bad) or "vacio"))
    print("  txd: %d texturas comp=0/32 bits" % len(texs))


def cmd_install(model, out_dff, out_txd):
    assert_pc_txd(out_txd)
    for tree in TREES:
        if not os.path.exists(os.path.join(tree, model + ".dff")):
            continue
        backup(model, tree)
        shutil.copy2(out_dff, os.path.join(tree, model + ".dff"))
        txd_dst = os.path.join(tree, model + ".txd")
        if os.path.exists(txd_dst):
            shutil.copy2(out_txd, txd_dst)
        print("instalado en %s" % tree)
    subprocess.run([sys.executable, os.path.join(HERE, "repack_dir.py"),
                    TREES[0], "--mirror", os.path.join(TREES[1], "..", "gta3.dir")], check=True)
    print("repack_dir OK")


def cmd_restore(model):
    n = 0
    for tree in TREES:
        label = os.path.basename(os.path.dirname(os.path.dirname(tree)))
        src = os.path.join(BACKUP, label)
        for ext in (".dff", ".txd"):
            p = os.path.join(src, model + ext)
            if os.path.exists(p):
                shutil.copy2(p, os.path.join(tree, model + ext))
                n += 1
    print("restaurados %d ficheros de %s" % (n, model))
    subprocess.run([sys.executable, os.path.join(HERE, "repack_dir.py"),
                    TREES[0], "--mirror", os.path.join(TREES[1], "..", "gta3.dir")], check=True)
    print("repack_dir OK")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pilot", metavar="MODELO")
    ap.add_argument("--feature", choices=("all", "wheel", "indicators"), default="all")
    ap.add_argument("--merge-textures", action="store_true")
    ap.add_argument("--install", action="store_true")
    ap.add_argument("--restore", metavar="MODELO")
    a = ap.parse_args()
    if a.restore:
        cmd_restore(a.restore)
        return
    if not a.pilot:
        ap.error("falta --pilot MODELO")
    dff, txd = cmd_pilot(a.pilot, a.feature, a.merge_textures)
    if a.install:
        cmd_install(a.pilot, dff, txd)


if __name__ == "__main__":
    main()
