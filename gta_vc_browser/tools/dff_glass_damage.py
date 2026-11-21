#!/usr/bin/env python3
"""Sirena de la patrulla: una sola barra por coche.

El modelo del `police` trae tres barras de sirena montadas a la vez. El juego
elige una por instancia a traves de las piezas `extra*` que declara la comprule
del IDE, asi que cada barra vuelve a su nombre de componente (con sus
`servicelights_*` dentro) y la caja lisa se descarta.

Escribe en `tmp/glass606/out/`; no toca `streamed`.

Uso:
    python tools/dff_glass_damage.py
"""
import os
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(HERE))

import xbox_transplant as xt   # noqa: E402

SERVED = Path(os.environ.get("GLASS_SRC") or (REPO / "streamed" / "models" / "gta3.img"))
OUT = REPO / "tmp" / "glass606" / "out"

MODEL = "police"

SIREN_DROP = ["sirenbar_r"]
SIREN_RENAME = {"sirenbar_l": "extra1", "sirenbar_c": "extra3"}


def _children(d):
    out = {i: [] for i in range(len(d["names"]))}
    for i, p in enumerate(d["parents"]):
        if p >= 0:
            out[p].append(i)
    return out


def _atomic_pieces(body):
    chs = list(xt.chunks(body, 0, len(body)))
    st = next(c for c in chs if c.typ == 0x01)
    return chs, st, struct.unpack_from("<II", st.body, 0)


def _drop_frames(d, names):
    kids = _children(d)
    doomed = set()
    for name in names:
        if name not in d["names"]:
            continue
        stack = [d["names"].index(name)]
        while stack:
            j = stack.pop()
            if j in doomed:
                continue
            doomed.add(j)
            stack.extend(kids[j])
    if not doomed:
        return []
    keep = [i for i in range(len(d["names"])) if i not in doomed]
    remap = {o: n for n, o in enumerate(keep)}
    gone = [d["names"][i] for i in sorted(doomed)]
    d["names"] = [d["names"][i] for i in keep]
    d["name_bodies"] = [d["name_bodies"][i] for i in keep]
    recs = []
    for i in keep:
        rec = bytearray(d["records"][i])
        p = d["parents"][i]
        struct.pack_into("<i", rec, 48, remap.get(p, -1))
        recs.append(bytes(rec))
    d["records"] = recs
    d["parents"] = [struct.unpack_from("<i", r, 48)[0] for r in recs]
    raw = []
    used = []
    for ver, body in d["atomic_raw"]:
        chs, st, (fi, gi) = _atomic_pieces(body)
        if fi in doomed:
            continue
        used.append(gi)
        parts = []
        for c in chs:
            if c.typ == 0x01:
                sb = bytearray(c.body)
                struct.pack_into("<II", sb, 0, remap[fi], gi)
                parts.append(xt.ch(0x01, c.ver, bytes(sb)))
            else:
                parts.append(xt.ch(c.typ, c.ver, c.body))
        raw.append((ver, b"".join(parts)))
    d["atomic_raw"] = raw
    keepg = sorted(set(used))
    gremap = {o: n for n, o in enumerate(keepg)}
    d["geoms"] = [d["geoms"][i] for i in keepg]
    fixed = []
    for ver, body in d["atomic_raw"]:
        chs, st, (fi, gi) = _atomic_pieces(body)
        parts = []
        for c in chs:
            if c.typ == 0x01:
                sb = bytearray(c.body)
                struct.pack_into("<II", sb, 0, fi, gremap[gi])
                parts.append(xt.ch(0x01, c.ver, bytes(sb)))
            else:
                parts.append(xt.ch(c.typ, c.ver, c.body))
        fixed.append((ver, b"".join(parts)))
    d["atomic_raw"] = fixed
    return gone


def _rename_frames(d, mapping):
    done = []
    for i, name in enumerate(d["names"]):
        new = mapping.get(name)
        if new is None:
            continue
        raw = new.encode("latin-1") + b"\0"
        raw += b"\0" * (-len(raw) % 4)
        d["name_bodies"][i] = xt.ch(0x0253F2FE, 0x1003FFFF, raw)
        d["names"][i] = new
        done.append("%s->%s" % (name, new))
    return done


def fix():
    d = xt.parse_dff(str(SERVED / (MODEL + ".dff")))
    before = len(d["names"])
    gone = _drop_frames(d, SIREN_DROP)
    renamed = _rename_frames(d, SIREN_RENAME)
    data = xt.rebuild(d)
    OUT.mkdir(parents=True, exist_ok=True)
    dst = OUT / (MODEL + ".dff")
    dst.write_bytes(data)
    print("%-8s frames %d->%d | fuera=%s ren=%s"
          % (MODEL, before, len(d["names"]), ",".join(gone) or "-",
             ",".join(renamed) or "-"))
    return dst


if __name__ == "__main__":
    fix()
