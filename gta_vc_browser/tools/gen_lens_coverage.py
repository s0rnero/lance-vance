#!/usr/bin/env python3
"""Plan 59.0 T3 — la intermitente enciende la CARA ENTERA de la lente.

La cara de una lente esta partida: unas caras viven en el nodo `indicator_*` y
sus vecinas coplanares (misma textura, centroide a <= 0,25 m) en `chassis_hi`,
`wing_*` o `bump_*`. El atlas derivado del 58.1 solo re-tenia el racimo del
nodo, asi que el resto de la cara quedaba oscuro: la cuna y los triangulos
muertos de la captura del jugador.

Lo que hace aqui, y lo que NO hace:
  * pinta en el atlas derivado el footprint UV de TODAS las caras ambar de la
    lente (nodo + companeras). Una cara es lente cuando >= 50 % de las texelas
    de su propio footprint pasan `ia._is_amber` medida sobre la textura base:
    el alojamiento gris de la lente NO se pinta (el intento anterior si lo
    hacia, y ademas pintaba companeras de un nodo sin lente).
  * el matiz es el medido en el racimo (G.chroma_of) y cada texela conserva su
    luminancia relativa, igual que en gen_lens_atlas.
  * emite el derivado SIN comprimir (ARGB8888). Con DXT1 era imposible: un
    bloque 4x4 que roza el borde de la cara tiene texelas dentro y fuera, y al
    re-codificarlo cambiaban hasta 66/255 en ~146 texelas fuera de la lente
    (medido en rumpo/pony/topfun/zebra/fbiranch). Sin bloques, lo de fuera es
    bit a bit lo que estaba.
  * no mueve un vertice, no crea nodos, no cambia tamarios y no toca las demas
    texturas del TXD: el estado OFF sigue siendo identico.

Uso:
    python tools/gen_lens_coverage.py --check
    python tools/gen_lens_coverage.py
    python tools/gen_lens_coverage.py --model rumpo,topfun

Escribe el TXD en streamed/ y sus espejos (bootseed/, gamefiles/) solo cuando
el espejo coincide con lo servido (si no, para: trabajo concurrente).
"""
import argparse
import io
import json
import math
import os
import re
import struct
import sys
from collections import defaultdict

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import dff_geom as dg              # noqa: E402
import indicator_audit as ia       # noqa: E402
import gen_lens_atlas as G         # noqa: E402
import txd_merge as TM             # noqa: E402
import txd_raster as tr            # noqa: E402

MODELS = os.path.join(REPO, "streamed", "models", "gta3.img")
TREES = [os.path.join(REPO, "streamed", "models", "gta3.img"),
         os.path.join(REPO, "bootseed", "models", "gta3.img"),
         os.path.join(REPO, "gamefiles", "models", "gta3.img")]
SCOPE = ["rumpo", "pony", "topfun", "zebra", "cabbie", "mrwhoop", "trash",
         "fbiranch", "boxville", "esperant", "mule", "enforcer", "securica",
         "spand", "gangbur", "manana", "rancher"]
PLANAR_DOT = 0.97
PLANAR_DIST = 0.025
NEAR_M = 0.25
AMBER_FACE_MIN = 0.50    # una cara es lente si >= 50 % de sus texelas son ambar
AMBER_DESIGN = (253, 165, 0)   # ambar medido de la captura del mod (51.0 D2)

RE_BODY = re.compile(r'\{\s*"([a-z0-9_]+)",\s*"([a-z0-9_]+)",\s*CAR_INDICATOR_(\w+),\s*(-?\d+),'
                     r'\s*\{\s*\d+,\s*\d+,\s*\d+\s*\},\s*(0x[0-9a-fA-F]+),\s*\d+,'
                     r'\s*\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}(?:,\s*\d+)?\s*\}')

# ARGB8888 sin comprimir: el formato que ya usan en estos TXD las texturas de
# efecto del juego (`lights`: fmt 0x8600, hasAlpha 22, depth 32, comp 0).
FMT_ARGB8888, HAS_ALPHA_ARGB = 0x8600, 22
OFF_NAME, OFF_FMT, OFF_ALPHA, OFF_DNRC = 32, 96, 100, 108


def _xf(m, v):
    return [sum(m[a][k] * v[k] for k in range(3)) + m[a][3] for a in range(3)]


def _sub(a, b):
    return [a[i] - b[i] for i in range(3)]


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]]


def _dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def _norm(a):
    n = math.sqrt(_dot(a, a))
    return [x / n for x in a] if n else [0.0, 0.0, 0.0]


def faces(model):
    """Todas las caras del modelo con frame, textura, normal, centroide y UV."""
    buf = open(os.path.join(MODELS, model + ".dff"), "rb").read()
    frames, geoms, atomics = dg.parse(buf)
    names = [(f.get("name") or "").lower() for f in frames]
    out = []
    for fi, gi in atomics:
        g = geoms[gi]
        W = dg.world_of(frames, fi)
        for ti, t in enumerate(g["tris"]):
            mi = t[3]
            if mi >= len(g["mats"]):
                continue
            pts = [_xf(W, g["verts"][v]) for v in t[:3]]
            out.append({"fn": names[fi], "gi": gi, "ti": ti, "mi": mi,
                        "tex": (g["mats"][mi][1] or "").lower(),
                        "uv": ia._uv_tri(g, t),
                        "n": _norm(_cross(_sub(pts[1], pts[0]), _sub(pts[2], pts[0]))),
                        "c": [sum(p[i] for p in pts) / 3.0 for i in range(3)]})
    return out


def cara_ambar(f, pix):
    """(fraccion ambar, footprint UV) de la cara, medida sobre su textura base.

    Las caras con UV NaN (geometria degenerada de algunos servidos) se leen
    como vacias: no son lente de nada.
    """
    if not f["tex"] or f["tex"] not in pix:
        return 0.0, set()
    if any(not all(math.isfinite(c) for c in uv) for uv in f["uv"]):
        return 0.0, set()
    w, h, px = pix[f["tex"]]
    foot = G.foot(f["uv"], w, h)
    if not foot:
        return 0.0, set()
    na = sum(1 for (y, x) in foot if ia._is_amber(px[y * w + x]))
    return na / float(len(foot)), foot


def lentes(model):
    """Una entrada por lente: su nodo, sus caras de carrocería y que texelas
    se pueden pintar sin tocar a nadie.

    Reglas medidas, no de nombre:
      * companera de una lente es la cara de carrocería coplanar, con la misma
        textura y a <= 0,25 m de una cara AMBAR del nodo (un nodo gris como el
        del fbiranch no es lente: su trabajo es otra causa, la del faro).
      * solo se pintan texelas que el color base ya declara ambar: no se
        inventa cristal donde la textura no lo pinta.
      * una texela que tambien muestrea una cara que no es lente de este modelo
        se DESCARTA: esa texela es compartida con el panel y encenderla seria
        pintar la pintura del coche.
      * las texelas que cubre el propio nodo no se tocan: esas ya las enciende
        la ruta del nodo con el color del material, y duplicar el encendido
        las lavado.
    """
    fl = faces(model)
    pix = ia.texture_pixels(model)
    for f in fl:
        f["fr"], f["foot"] = cara_ambar(f, pix)
    nodos = [f for f in fl if f["fn"].startswith("indicator")]
    nodos_ambar = [f for f in nodos if f["fr"] >= AMBER_FACE_MIN]
    cuerpo = [f for f in fl if not f["fn"].startswith("indicator") and "_dam" not in f["fn"]]
    filas = filas_del_modelo(model)
    grupos = defaultdict(list)
    for n in nodos_ambar:
        grupos[n["fn"]].append(n)
    for b in cuerpo:
        nod = grupo_de(b, filas)
        if nod is not None:
            grupos[nod].append(b)
            continue
        if b["fr"] < AMBER_FACE_MIN:
            continue
        mejor, d = None, NEAR_M + 1e-9
        for t in nodos_ambar:
            if t["tex"] != b["tex"] or not t["tex"]:
                continue
            if abs(_dot(t["n"], b["n"])) < PLANAR_DOT:
                continue
            if abs(_dot(t["n"], _sub(b["c"], t["c"]))) > PLANAR_DIST:
                continue
            dd = math.dist(t["c"], b["c"])
            if dd <= d:
                mejor, d = t, dd
        if mejor is not None:
            grupos[mejor["fn"]].append(b)
    swap = set()
    for frame, mats, xyz, _nod, _side in filas:
        for f in fl:
            if f["fn"] == frame and (mats >> f["mi"]) & 1 and _dist(f["c"], xyz) <= 0.30:
                swap.add((f["gi"], f["mi"]))
    for f in nodos_ambar:
        swap.add((f["gi"], f["mi"]))
    exclusivo = set()
    for key in swap:
        us = [f for f in fl if (f["gi"], f["mi"]) == key]
        if all(f["fr"] >= AMBER_FACE_MIN or declarada(f, filas) for f in us if f["foot"]):
            exclusivo.add(key)
    de_lente = set((f["gi"], f["ti"]) for v in grupos.values() for f in v)
    ajena = set()
    for f in fl:
        if (f["gi"], f["ti"]) in de_lente or not f["foot"]:
            continue
        if (f["gi"], f["mi"]) not in swap:
            continue
        ajena |= f["foot"]
    out = []
    for node, fs in sorted(grupos.items()):
        texes = sorted({f["tex"] for f in fs if f["tex"]})
        if not texes:
            continue
        out.append({"node": node, "tex": texes[0],
                    "nodo": [f for f in fs if f["fn"].startswith("indicator")],
                    "cuerpo": [f for f in fs if not f["fn"].startswith("indicator")],
                    "exclusivo": exclusivo, "filas": filas, "ajena": ajena, "pix": pix})
    return out


def _dist(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def huellas(entrada):
    """(texelas a pintar, texelas descartadas por dueño ajeno, ancla de luz)."""
    pix = entrada["pix"]
    tex = entrada["tex"]
    w, h, px = pix[tex]
    del_nodo = set()
    for f in entrada["nodo"]:
        del_nodo |= f["foot"]
    ambar = set()
    for f in entrada["cuerpo"]:
        if (f["gi"], f["mi"]) in entrada["exclusivo"] or declarada(f, entrada["filas"]):
            ambar |= set(f["foot"])
        else:
            ambar |= set(k for k in f["foot"] if ia._is_amber(px[k[0] * w + k[1]]))
    sin_dueno = set(k for k in ambar if k not in del_nodo and k not in entrada["ajena"])
    descartadas = sorted(ambar - sin_dueno - del_nodo)
    if not sin_dueno:
        return set(), descartadas, 0.0
    lum_ref = max(0.299 * px[k[0] * w + k[1]][0] + 0.587 * px[k[0] * w + k[1]][1] +
                  0.114 * px[k[0] * w + k[1]][2] for k in sin_dueno)
    return sin_dueno, descartadas, lum_ref


def pinta(dst, keys, lum_ref, px, w):
    """Las texelas de la huella al ambar de diseno, con su luminancia relativa.

    El ancla es la texela mas clara del propio racimo: esa pasa a (253,165,0),
    el ambar medido de la captura del mod (51.0 D2), y las demas bajan en
    proporcion, asi el cristal conserva sus sombras en vez de quedar plano.
    """
    pintadas = 0
    if lum_ref <= 0.0:
        return 0
    for (y, x) in sorted(keys):
        c = px[y * w + x][:3]
        lum = 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]
        k = max(0.35, min(1.0, lum / lum_ref))
        dst[y, x, 0] = int(min(255, round(AMBER_DESIGN[0] * k)))
        dst[y, x, 1] = int(min(255, round(AMBER_DESIGN[1] * k)))
        dst[y, x, 2] = int(min(255, round(AMBER_DESIGN[2] * k)))
        pintadas += 1
    return pintadas


def arr_de(pix, tex):
    w, h, px = pix[tex]
    return np.array(px, dtype=np.uint8).reshape(h, w, 4), w, h


def emit_native(chunk, name, dst, w, h):
    """Copia del chunk con el nivel 0 como ARGB8888 y los campos corregidos."""
    new = bytearray(chunk[:112])
    struct.pack_into("<I", new, OFF_FMT, FMT_ARGB8888)
    struct.pack_into("<i", new, OFF_ALPHA, HAS_ALPHA_ARGB)
    new[OFF_DNRC] = 32              # depth
    new[OFF_DNRC + 1] = 1           # nlevels
    new[OFF_DNRC + 3] = 0           # compression
    lvl = np.empty((h, w, 4), dtype=np.uint8)
    lvl[..., 0] = dst[..., 2]
    lvl[..., 1] = dst[..., 1]
    lvl[..., 2] = dst[..., 0]
    lvl[..., 3] = dst[..., 3]
    new += struct.pack("<I", lvl.nbytes) + lvl.tobytes()
    struct.pack_into("<I", new, 4, len(new) - 12)
    rel = name.encode("latin1")[:31] + b"\0"
    new[OFF_NAME:OFF_NAME + 32] = rel.ljust(32, b"\0")
    return bytes(new)


def rebuild_txd(btexs, bstruct, bcola, bdata, ver, replacements):
    """TXD con los chunks indicados sustituidos; el resto, byte a byte igual."""
    st = bytearray(bstruct)
    struct.pack_into("<I", st, 12, len(btexs))
    body = bytes(st)
    for pos, fin, name in btexs:
        body += replacements.get(name, bdata[pos:fin])
    body += bcola
    return struct.pack("<III", TM.DICT, len(body), ver) + body


def filas_del_modelo(model):
    """[(frame, mascara, xyz)] de BodyLensTable.h: quien declara el swap."""
    src = open(os.path.join(REPO, "..", "src", "vehicles", "BodyLensTable.h"),
               encoding="utf-8").read()
    out = []
    for mm in RE_BODY.finditer(src):
        if mm.group(1) != model:
            continue
        out.append((mm.group(2), int(mm.group(5), 16),
                    [int(mm.group(6)) / 100.0, int(mm.group(7)) / 100.0,
                     int(mm.group(8)) / 100.0],
                    "indicator_" + mm.group(3).lower(), int(mm.group(4))))
    return out


def declarada(f, filas):
    for frame, mats, xyz, _nod, _side in filas:
        if frame == f["fn"] and (mats & (1 << f["mi"])) and math.dist(f["c"], xyz) <= 0.30:
            return True
    return False


def grupo_de(f, filas):
    # La fila que declara la cara dice tambien a que lente pertenece.
    for frame, mats, xyz, nod, _side in filas:
        if frame == f["fn"] and (mats & (1 << f["mi"])) and math.dist(f["c"], xyz) <= 0.30:
            return nod
    return None


def procesa(model, _rows_unused=None):
    ents = lentes(model)
    if not ents:
        return None
    pix = ia.texture_pixels(model)
    filas = filas_del_modelo(model)
    path = os.path.join(MODELS, model + ".txd")
    ver, bstruct, btexs, bcola, data = TM.read_txd(path)
    res = {"model": model, "atlas": [], "sin_derivado": [], "sin_fila": []}
    por_tex = defaultdict(list)
    for e in ents:
        if e["tex"] and e["tex"] not in G.SKIP_TEX:
            por_tex[e["tex"]].append(e)
    replacements = {}
    for tex, es in sorted(por_tex.items()):
        derived = tex + G.SUFFIX
        pos, fin, _info = G.find_native(data, derived)
        if pos is None:
            res["sin_derivado"].append({"tex": tex, "nodo": [e["node"] for e in es]})
            continue
        src_arr, w, h = arr_de(pix, derived)
        _pxw, _pxh, px = pix[tex]
        dst = src_arr.copy()
        huella = set()
        grupos = []
        for e in es:
            keys, descartadas, lum_ref = huellas(e)
            sin_fila = [f for f in e["cuerpo"] if not declarada(f, filas)]
            for f in sin_fila:
                res["sin_fila"].append({"node": e["node"], "frame": f["fn"],
                                         "mat": f["mi"],
                                         "xyz": [round(v, 2) for v in f["c"]]})
            n = pinta(dst, keys, lum_ref, px, w)
            huella |= keys
            grupos.append({"node": e["node"], "pintadas": n, "huella": len(keys),
                           "descartadas": len(descartadas), "caras_cuerpo": len(e["cuerpo"]),
                           "caras_sin_fila": len(sin_fila),
                           "ancla_lum": round(lum_ref, 1)})
        if not huella:
            continue
        dif = set((int(y), int(x)) for y, x in
                  np.argwhere(np.any(dst[:, :, :3] != src_arr[:, :, :3], axis=2)))
        fuera = len(dif - huella)
        if fuera:
            raise SystemExit("STOP: %s %s pinta %d texelas fuera de la huella"
                             % (model, derived, fuera))
        if not dif:
            continue
        replacements[derived] = emit_native(data[pos:fin], derived, dst, w, h)
        res["atlas"].append({"tex": tex, "new": derived, "px": [w, h],
                             "huella": len(huella), "pintadas": len(dif), "fuera": fuera,
                             "grupos": grupos})
    if not replacements:
        return res
    res["out"] = rebuild_txd(btexs, bstruct, bcola, data, ver, replacements)
    res["texs_antes"] = [(p0, p1, n) for p0, p1, n in btexs]
    res["src_bytes"] = data
    return res


def verifica(res, tmp):
    """Releyendo el TXD emitido: nombres y contador intactos, el derivado
    ARGB8888 y decodificable, y las demas texturas byte a byte como estaban."""
    data = res["src_bytes"]
    antes = {n: (p0, p1) for p0, p1, n in res["texs_antes"]}
    io.open(tmp, "wb").write(res["out"])
    _v, _s, texs_new, _c, d2 = TM.read_txd(tmp)
    nuevos = {n: (p0, p1) for p0, p1, n in texs_new}
    if set(antes) != set(nuevos):
        raise SystemExit("STOP: el TXD emitido cambio de nombres: %s" %
                         sorted(set(antes) ^ set(nuevos)))
    if len(nuevos) != len(antes):
        raise SystemExit("STOP: %d texturas emitidas, eran %d" % (len(nuevos), len(antes)))
    tocados = {e["new"] for e in res["atlas"]}
    for name, (p0, p1) in antes.items():
        if name in tocados:
            continue
        q0, q1 = nuevos[name]
        if data[p0:p1] != d2[q0:q1]:
            raise SystemExit("STOP: se altero la textura %s" % name)
    for e in res["atlas"]:
        _p, _f, info = G.find_native(d2, e["new"])
        if info is None:
            raise SystemExit("STOP: no relee %s" % e["new"])
        if info["compression"] != 0 or info["depth"] != 32 or info["nlevels"] != 1:
            raise SystemExit("STOP: %s no salio ARGB8888 (comp=%d depth=%d nv=%d)"
                             % (e["new"], info["compression"], info["depth"], info["nlevels"]))
        size, off = info["levels"][0]
        w, h = info["w"], info["h"]
        if size != w * h * 4:
            raise SystemExit("STOP: %s nivel 0 de %d, se esperaban %d"
                             % (e["new"], size, w * h * 4))
        px = tr.decode(d2[off:off + size], w, h, info["depth"], info["fmt"],
                       info["has_alpha"], 0)
        got = np.array(px, dtype=np.uint8).reshape(h, w, 4)
        _p2, _f2, bi = G.find_native(data, e["tex"])
        bs, bo = bi["levels"][0]
        base = np.array(tr.decode(data[bo:bo + bs], w, h, bi["depth"], bi["fmt"],
                                  bi["has_alpha"], bi["compression"]),
                        dtype=np.uint8).reshape(h, w, 4)
        if not np.all(got[..., 3] == base[..., 3]):
            raise SystemExit("STOP: %s cambio el canal alfa" % e["new"])
        iguales = int(np.all(got[:, :, :3] == base[:, :, :3], axis=2).sum())
        e["iguales_base"] = iguales
        e["diferentes_base"] = w * h - iguales
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--model", action="append", default=[])
    ap.add_argument("--json", default=os.path.join(REPO, "tmp", "59", "coverage.json"))
    a = ap.parse_args()
    ia.BASE = MODELS
    ia.MODELS = MODELS
    modelos = [x.strip().lower() for m in a.model for x in m.split(",")] or SCOPE
    os.makedirs(os.path.dirname(a.json), exist_ok=True)
    tmp = os.path.join(REPO, "tmp", "59", "verif.txd")
    informe = []
    total = 0
    sin_fila_todo = []
    for m in modelos:
        if not os.path.isfile(os.path.join(MODELS, m + ".dff")):
            print("%-10s sin DFF" % m)
            continue
        res = procesa(m)
        if res is None:
            print("%-10s sin nodos con lente ambar" % m)
            continue
        for x in res["sin_fila"]:
            sin_fila_todo.append((m, x))
        if not res["atlas"]:
            print("%-10s sin atlas que reescribir  (sin derivado: %s)" %
                  (m, ",".join(x["tex"] for x in res["sin_derivado"]) or "-"))
            informe.append({k: v for k, v in res.items()
                            if k not in ("out", "texs_antes", "src_bytes")})
            continue
        verifica(res, tmp)
        for e in res["atlas"]:
            total += e["pintadas"]
            print("  %-10s %-26s huella=%4d pintadas=%4d fuera=%d vs_base=%4d" % (
                m, e["new"][:26], e["huella"], e["pintadas"], e["fuera"],
                e["diferentes_base"]))
            for g in e["grupos"]:
                print("      %-14s texelas=%4d descartadas=%3d carasCuerpo=%2d sinFila=%2d "
                      "anclaLum=%.0f" % (g["node"], g["huella"], g["descartadas"],
                                         g["caras_cuerpo"], g["caras_sin_fila"],
                                         g["ancla_lum"]))
        informe.append({k: v for k, v in res.items()
                        if k not in ("out", "texs_antes", "src_bytes")})
        if not a.check:
            for tree in TREES:
                path = os.path.join(tree, m + ".txd")
                if not os.path.isfile(path):
                    continue
                if open(path, "rb").read() != res["src_bytes"]:
                    raise SystemExit("STOP: %s difiere de streamed; trabajo concurrente" % path)
                io.open(path + ".tmp", "wb").write(res["out"])
                os.replace(path + ".tmp", path)
                print("     -> %s (%d bytes)" % (path, len(res["out"])))
    io.open(a.json, "w", encoding="utf-8").write(json.dumps(informe, indent=1))
    print("\nmodelos con atlas reescrito: %d | texelas pintadas: %d | json -> %s" %
          (len([r for r in informe if r["atlas"]]), total, a.json))
    if sin_fila_todo:
        print("\ncaras de lente en carrocería SIN fila que las declare (quedan oscuras):")
        for m, x in sin_fila_todo:
            print("   %-10s %-16s mat%-2d %s  (%s)" % (
                m, x["frame"], x["mat"], x["xyz"], x["node"]))


if __name__ == "__main__":
    main()
