#!/usr/bin/env python3
"""Atlas de lente derivado por modelo (plan 46.0, C1).

El defecto no es de pintura: el motor pinta POR MATERIAL y hay caras de lente
cuyo footprint UV cae a caballo entre la zona ámbar/roja del atlas y una zona
cromada o de sombra del MISMO atlas. La mitad plata multiplica gris x color de
material y sale «apagada». Es dato, no código, así que el arreglo es un dato
derivado: una copia del atlas del coche en la que las texelas SIN MATIZ del
footprint de las lentes se re-tiñen al matiz templado MEDIDO en ese mismo
footprint, conservando la luminancia texel a texel.

Una sola regla para toda la flota, para todas las formas (línea, banda, zona) y
para ámbar y rojo a la vez. Las reglas de la casa que cumple el generador:

  * ningún color inventado: el destino es la media de canales de las texelas
    templadas del propio racimo; con muestra corta se prueba el costado espejo
    y, si no, la fila medida de IndicatorLensTable.h. Nunca un color escrito a
    mano: sin muestra y sin fila el racimo se salta y se reporta.
  * ninguna luminancia inventada: se conserva la de cada texela, así que una
    sombra dentro de la lente sigue siendo sombra.
  * el atlas original no se toca: la copia se emite como textura nueva
    `<atlas>_lens` y el motor solo la enlaza en el flanco encendido del
    parpadeo, igual que hace con `lights`/`lightson`.

Emite DXT1 (el formato de sus hermanas en el mismo TXD) con un encoder propio
validado por round-trip contra `txd_raster.dxt1_decode`, y re-mide las caras
sobre los bytes YA codificados: lo que se reporta es lo que va a leer el motor.

Uso:
    python tools/gen_lens_atlas.py                      # toda la flota servida
    python tools/gen_lens_atlas.py --model sandking,trash
    python tools/gen_lens_atlas.py --check              # sin escribir TXD
    python tools/gen_lens_atlas.py --json /c/tmp/iav42/lens46.json
"""
import argparse
import io
import json
import os
import re
import struct
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import numpy as np                # noqa: E402
import indicator_audit as ia      # noqa: E402
import indicator_lens45 as L      # noqa: E402
import txd_raster as tr           # noqa: E402

ROOT = os.path.join(REPO, "streamed", "models", "gta3.img")
TABLE = os.path.join(os.path.dirname(REPO), "src", "vehicles", "IndicatorLensTable.h")
SUFFIX = "_lens"
N = 40                    # rejilla baricéntrica: la misma del detector
NEUTRA_DIR = 0.18         # delta/max por debajo de esto = texela sin matiz
LUM_MIN = 60.0            # por debajo, re-tenirla no la enciende: es sombra
POOL_WARM_MIN = 30        # muestra mínima para fiarse del matiz del racimo
SKIP_TEX = ("lights", "lightson")


# -----------------------------------------------------------------------------
# clasificación de texelas: idéntica a la del detector del plan (lamp46t.py)
# -----------------------------------------------------------------------------
def hue(r, g, b):
    mx, mn = max(r, g, b), min(r, g, b)
    d = mx - mn
    if d == 0:
        return 0.0
    if mx == r:
        h = 60.0 * (((g - b) / d) % 6)
    elif mx == g:
        h = 60.0 * ((b - r) / d + 2)
    else:
        h = 60.0 * ((r - g) / d + 4)
    return h % 360.0


def cls(c):
    """t templada, c clara (cromo/bisel), o sombra, n otra, None vacía."""
    r, g, b = int(c[0]), int(c[1]), int(c[2])
    if int(c[3]) < 40:
        return None
    mx, mn = max(r, g, b), min(r, g, b)
    d = mx - mn
    mean = (r + g + b) / 3.0
    if d >= 45 and r >= g and r >= b and hue(r, g, b) <= 60.0:
        return "t"
    if d <= 45 and 60 <= mean <= 215:
        return "c"
    if mean < 60 and d <= 60:
        return "o"
    return "n"


def is_warm(c):
    return cls(c) == "t"


def _hue_grid(rgb):
    mx = rgb.max(axis=2)
    mn = rgb.min(axis=2)
    d = np.maximum(mx - mn, 1e-9)
    h = np.where(rgb[:, :, 0] == mx, 60.0 * (((rgb[:, :, 1] - rgb[:, :, 2]) / d) % 6.0),
                 np.where(rgb[:, :, 1] == mx,
                          60.0 * ((rgb[:, :, 2] - rgb[:, :, 0]) / d + 2.0),
                          60.0 * ((rgb[:, :, 0] - rgb[:, :, 1]) / d + 4.0)))
    return np.mod(h, 360.0)


def metrics(rgba):
    """(templada, neutra_absoluta, neutra_direccion) sobre la muestra pintable.

    Las mismas clases y la misma muestra (alpha>=40) que el detector, para que
    la cuenta de partida sea la de las 135 caras del plan.
    """
    valid = (rgba[:, :, 0] >= 0) & (rgba[:, :, 3] >= 40)
    rgb = rgba[:, :, :3]
    mx = rgb.max(axis=2)
    mn = rgb.min(axis=2)
    d = mx - mn
    mean = rgb.mean(axis=2)
    warm = valid & (d >= 45) & (rgb[:, :, 0] >= rgb[:, :, 1]) & \
        (rgb[:, :, 0] >= rgb[:, :, 2]) & (_hue_grid(rgb) <= 60.0)
    neut_abs = valid & (((d <= 45) & (mean >= 60) & (mean <= 215)) |
                        ((mean < 60) & (d <= 60)))
    rel = np.where(mx > 0, d / np.maximum(mx, 1.0), 0.0)
    # «apagada de verdad»: sin matiz Y con luz suficiente para que el ámbar se
    # vea. Una texela oscura re-tenida sigue oscura (el arreglo conserva la
    # luminancia: no inventa brillo), así que no entra en la cuenta.
    neut_dir = valid & (rel <= NEUTRA_DIR) & (mean >= LUM_MIN)
    v = int(valid.sum())
    if not v:
        return 0.0, 0.0, 0.0
    return (float(warm.sum()) / v, float(neut_abs.sum()) / v,
            float(neut_dir.sum()) / v)


def sample(uv, arr, w, h):
    """RGBA baricéntrico N x N; -1 fuera del triángulo (lo que ve el rasterizador)."""
    out = np.full((N, N, 4), -1.0)
    for i in range(N):
        for j in range(N):
            a = i / float(N - 1)
            b = j / float(N - 1)
            if a + b > 1.0:
                continue
            c = 1.0 - a - b
            u = a * uv[0][0] + b * uv[1][0] + c * uv[2][0]
            v = a * uv[0][1] + b * uv[1][1] + c * uv[2][1]
            out[i, j] = arr[int(v * h) % h, int(u * w) % w]
    return out


def foot(uv, w, h):
    """Texelas (fila, columna) que pisa el triángulo UV."""
    keys = set()
    for i in range(N):
        for j in range(N):
            a = i / float(N - 1)
            b = j / float(N - 1)
            if a + b > 1.0:
                continue
            c = 1.0 - a - b
            u = a * uv[0][0] + b * uv[1][0] + c * uv[2][0]
            v = a * uv[0][1] + b * uv[1][1] + c * uv[2][1]
            keys.add((int(v * h) % h, int(u * w) % w))
    return keys


# -----------------------------------------------------------------------------
# encoder DXT1 (BC1) — validado por round-trip contra txd_raster.dxt1_decode
# -----------------------------------------------------------------------------
def to_565(c):
    r, g, b = [max(0, min(255, int(round(float(x))))) for x in c]
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def from_565(v):
    """Exactamente la conversión del decoder: no introduce error de lectura."""
    return (((v >> 11) & 0x1F) * 255 // 31,
            ((v >> 5) & 0x3F) * 255 // 63,
            (v & 0x1F) * 255 // 31)


def encode_block(px):
    """16 colores (fila a fila) -> (c0, c1, bits) con c0 > c1 (modo opaco)."""
    a = np.asarray(px, dtype=np.float64)
    mean = a.mean(axis=0)
    d = a - mean
    cov = d.T.dot(d)
    axis = np.array([1.0, 1.0, 0.5])
    for _ in range(24):
        nv = cov.dot(axis)
        n = float(np.linalg.norm(nv))
        if n < 1e-9:
            break
        axis = nv / n
    t = d.dot(axis)
    c0 = to_565(mean + axis * float(t.max()))
    c1 = to_565(mean + axis * float(t.min()))
    if c0 < c1:
        c0, c1 = c1, c0
    if c0 == c1:                    # si no, el decoder entra en modo 3-bit alfa
        c1 = c0 - 1 if c0 > 0 else 1
    p0 = np.array(from_565(c0), dtype=np.float64)
    p1 = np.array(from_565(c1), dtype=np.float64)
    pal = [p0, p1, (2.0 * p0 + p1) / 3.0, (p0 + 2.0 * p1) / 3.0]
    bits = 0
    for k in range(16):
        err = [float(((pal[i] - a[k]) ** 2).sum()) for i in range(4)]
        idx = err.index(min(err))
        bits |= idx << (2 * (4 * (k // 4) + (k % 4)))
    return c0, c1, bits


def dxt1_encode(arr):
    """(h, w, 4) uint8 -> bytes DXT1 en el orden de bloques del decoder."""
    h, w = arr.shape[0], arr.shape[1]
    out = bytearray()
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            blk = [arr[by + y, bx + x, :3].astype(np.float64)
                   for y in range(4) for x in range(4)]
            c0, c1, bits = encode_block(blk)
            out += struct.pack("<HHI", c0, c1, bits)
    return bytes(out)


# -----------------------------------------------------------------------------
# lectura/escritura del TextureNative
# -----------------------------------------------------------------------------
NAME_OFF, MASK_OFF = 32, 64      # dentro del chunk, tras cabecera(12)+STRUCT(12)+plat(4)+filt(4)


def find_native(data, nombre):
    """-> (pos, fin, info) del TextureNative cuyo nombre coincide."""
    for pos in tr.dict_textures(data):
        t = tr.read_native(data, pos)
        if t is not None and t["nombre"].lower() == nombre:
            return pos, pos + 12 + tr.u32(data, pos + 4), t
    return None, None, None


def rewrite_native(data, pos, fin, nombre, level0):
    """Copia del chunk con nombre y píxeles nuevos; cabecera intacta."""
    chunk = bytearray(data[pos:fin])
    # hasta el tamaño del nivel 0: cabecera(12) + STRUCT(12) + platform(4) +
    # filter(4) + name(32) + mask(32) + rasterfmt(4) + hasAlpha(4) + tam(4) +
    # depth/nlevels/rtype/comp(4)
    lvl_hdr = 12 + 12 + 4 + 4 + 32 + 32 + 4 + 4 + 4 + 4
    size = struct.unpack_from("<I", chunk, lvl_hdr)[0]
    if size != len(level0):
        raise SystemExit("ERROR: %s: el nivel 0 mide %d, el encoder emitió %d"
                         % (nombre, size, len(level0)))
    rel = nombre.encode("latin1")[:31] + b"\0"
    chunk[NAME_OFF:NAME_OFF + 32] = rel.ljust(32, b"\0")
    chunk[MASK_OFF:MASK_OFF + 32] = b"\0" * 32
    chunk[lvl_hdr + 4:lvl_hdr + 4 + len(level0)] = level0
    return bytes(chunk)


def build_txd(base_data, tex_chunk):
    """TXD mínimo: cabecera de diccionario + STRUCT con contador 1 + textura."""
    ver = tr.u32(base_data, 8)
    s = tr.u32(base_data, 16)
    st = bytearray(base_data[12:24 + s])
    struct.pack_into("<I", st, 12, 1)
    body = bytes(st) + tex_chunk
    return struct.pack("<III", 0x16, len(body), ver) + body


# -----------------------------------------------------------------------------
# tabla medida (tercer escalón de la cadena de matiz)
# -----------------------------------------------------------------------------
NODE_OF_SYMBOL = {
    "CAR_INDICATOR_LF": "indicator_lf", "CAR_INDICATOR_RF": "indicator_rf",
    "CAR_INDICATOR_LR": "indicator_lr", "CAR_INDICATOR_RR": "indicator_rr",
    "CAR_INDICATOR_2_LF": "indicator2_lf", "CAR_INDICATOR_2_RF": "indicator2_rf",
    "CAR_INDICATOR_2_LR": "indicator2_lr", "CAR_INDICATOR_2_RR": "indicator2_rr",
}
ROW_RE = re.compile(
    r'\{\s*"([a-z0-9_]+)",\s*(CAR_[A-Z0-9_]+),\s*-?\d,\s*\{\s*(\d+),\s*(\d+),'
    r'\s*(\d+)\s*\},\s*(\d),\s*\d+,\s*0x[0-9A-Fa-f]+,\s*\d+,\s*\{[^}]*\}\s*\},\s*'
    r'//\s*\S+\s+\d+\s+caras\s+(\S+)')


def table_rows():
    """{(modelo, nodo, tex): [rgb]} de las filas con veredicto ámbar."""
    if not os.path.exists(TABLE):
        return {}
    txt = io.open(TABLE, encoding="utf-8").read()
    out = defaultdict(list)
    for m in ROW_RE.finditer(txt):
        model, sym, r, g, b, amber, tex = m.groups()
        node = NODE_OF_SYMBOL.get(sym)
        if node is None or amber != "1":
            continue
        out[(model, node, tex.lower())].append([int(r), int(g), int(b)])
    return out


def table_chroma(rows, model, node, tex):
    rs = rows.get((model, node, tex)) or []
    if not rs:
        return None, 0
    acc = np.zeros(3)
    for c in rs:
        acc += np.array(c, dtype=np.float64)
    return acc / acc.sum() * 3.0, len(rs)


# -----------------------------------------------------------------------------
# el modelo
# -----------------------------------------------------------------------------
def chroma_of(arr, keys):
    """Dirección (r,g,b) normalizada a suma 3 de las texelas templadas del pool."""
    acc = np.zeros(3)
    n = 0
    for (y, x) in keys:
        c = arr[y, x]
        if is_warm(c):
            acc += np.array(c[:3], dtype=np.float64)
            n += 1
    if n == 0 or acc.sum() <= 0:
        return None, 0
    return acc / acc.sum() * 3.0, n


def paint(src, groups):
    """Copia de `src` con las texelas sin matiz de los racimos re-tenidas.

    `src` no se toca: por eso no hay colateral sobre otras piezas del coche.
    """
    out = src.copy()
    cand = defaultdict(list)
    for g in groups:
        g["lum"] = 0.299 * g["chroma"][0] + 0.587 * g["chroma"][1] + 0.114 * g["chroma"][2]
        for (y, x) in g["keys"]:
            cand[(y, x)].append(g)
    painted = 0
    dsum = np.zeros(3)
    conflicts = 0
    for key, gs in cand.items():
        c = src[key]
        if cls(c) in ("t", None):
            continue                       # templada real o vacía: no se toca
        mx, mn = int(max(c[:3])), int(min(c[:3]))
        rel = (mx - mn) / float(max(mx, 1))
        lum = 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]
        if rel > NEUTRA_DIR or lum < LUM_MIN:
            continue                       # con matiz, o sombra: no se toca
        best = max(gs, key=lambda g: g["warm"])
        if len(gs) > 1 and gs[0]["node"] != best["node"]:
            conflicts += 1
        ch = best["chroma"]
        if best["lum"] <= 0:
            continue
        new = np.clip(np.round(ch * (lum / best["lum"])), 0, 255)
        dsum += np.abs(new - np.array(c[:3], dtype=np.float64))
        out[key[0], key[1], :3] = new.astype(np.uint8)
        painted += 1
    if painted == 0:
        return out, 0, [0, 0, 0], conflicts
    return out, painted, [int(round(v / painted)) for v in dsum], conflicts


def faces_of(model):
    """Caras de lente del modelo con su textura, su UV y el atlas decodificado."""
    ia.BASE = ROOT
    ia.MODELS = ROOT
    _b, _f, geoms, _a, faces = L.measure(model, node_only=True)
    pix = {}
    for k, v in ia.texture_pixels(model).items():
        if k.endswith(SUFFIX):
            continue                     # una inyección anterior no es la fuente
        w, h, px = v
        pix[k] = (w, h, np.array(px, dtype=np.uint8).reshape(h, w, 4))
    out = []
    for f in faces:
        tn = (f["tex"] or "").lower()
        if tn not in pix or tn in SKIP_TEX:
            continue
        w, h, arr = pix[tn]
        g = geoms[f["gi"]]
        uv = ia._uv_tri(g, g["tris"][f["ti"]])
        out.append({"tex": tn, "frame": (f["frame"] or "").lower(),
                    "key": (f["gi"], f["mi"], f["ti"]),
                    "uv": uv, "cm2": 1e4 * f["area"], "arr": arr, "w": w, "h": h})
    return out


def cnt(fl, kind, src="b", thr=0.15):
    """Caras (y cm2) que superan el umbral con la vara pedida, antes o después.

    kind `abs`  -> templada >=15% y neutra absoluta >=thr  (la de las 135 caras)
    kind `dir`  -> templada >=15% y sin matiz >=thr        (la que ve el ojo)
    kind `any`  -> sin matiz >=thr, sin exigir templada
    """
    n, cm2 = 0, 0.0
    for f in fl:
        m = f[src]
        if kind == "abs":
            hit = m[0] >= 0.15 and m[1] >= thr
        elif kind == "dir":
            hit = m[0] >= 0.15 and m[2] >= thr
        else:
            hit = m[2] >= thr
        if hit:
            n += 1
            cm2 += f["cm2"]
    return n, round(cm2, 1)


def process(model, rows, pool_min=POOL_WARM_MIN):
    fl = faces_of(model)
    if not fl:
        return None
    for f in fl:
        f["b"] = metrics(sample(f["uv"], f["arr"], f["w"], f["h"]))
    mixed = [f for f in fl if f["b"][0] >= 0.15 and (f["b"][1] >= 0.15 or
                                                    f["b"][2] >= 0.15)]
    if not mixed:
        return None
    res = {"model": model,
           "before": {"abs": cnt(fl, "abs"), "dir": cnt(fl, "dir"),
                      "any": cnt(fl, "any")},
           "emitted": [], "blocked": []}
    bytex = defaultdict(list)
    for f in mixed:
        bytex[f["tex"]].append(f)
    for tex, fs in sorted(bytex.items()):
        w, h = fs[0]["w"], fs[0]["h"]
        src = fs[0]["arr"]
        per_node = defaultdict(set)
        for f in fs:
            per_node[f["frame"]] |= foot(f["uv"], w, h)
        groups = []
        for node, keys in sorted(per_node.items()):
            ch, nwarm = chroma_of(src, keys)
            origin = "own"
            if (ch is None or nwarm < pool_min) and L.MIRROR.get(node) in per_node:
                ch2, n2 = chroma_of(src, per_node[L.MIRROR[node]])
                if ch2 is not None and n2 > nwarm:
                    ch, nwarm, origin = ch2, n2, "mirror"
            if ch is None or nwarm < pool_min:
                ch3, n3 = table_chroma(rows, model, node, tex)
                if ch3 is not None and n3 > 0:
                    ch, nwarm, origin = ch3, n3, "table"
            ok = ch is not None and (nwarm >= pool_min or origin == "table")
            groups.append({"node": node, "keys": keys, "chroma": ch,
                           "warm": nwarm, "origin": origin, "ok": ok})
        for g in groups:
            if not g["ok"]:
                res["blocked"].append({"model": model, "tex": tex, "node": g["node"],
                                       "warm": g["warm"],
                                       "faces": sum(1 for f in fs
                                                    if f["frame"] == g["node"])})
        usable = [g for g in groups if g["ok"]]
        if not usable:
            continue
        dst, painted, davg, conflicts = paint(src, usable)
        if painted == 0:
            continue
        level0 = dxt1_encode(dst)
        back = np.array(tr.dxt1_decode(level0, w, h), dtype=np.uint8).reshape(h, w, 4)
        err = np.abs(back[:, :, :3].astype(int) - dst[:, :, :3].astype(int))
        aft = []
        for f in fs:
            f["a"] = metrics(sample(f["uv"], back, w, h))
            aft.append(f)
        res["emitted"].append({
            "tex": tex, "new": tex + SUFFIX, "px": [w, h], "painted": painted,
            "delta_rgb": davg, "conflicts": conflicts, "bytes_dxt1": len(level0),
            "rt_max_err": int(err.max()), "rt_mean_err": round(float(err.mean()), 3),
            "groups": [{"node": g["node"], "warm": g["warm"], "origin": g["origin"],
                        "keys": len(g["keys"]),
                        "chroma_dir": [round(float(v), 3) for v in g["chroma"]],
                        "chroma_rgb": [int(round(255 * v / 3.0)) for v in g["chroma"]]}
                       for g in usable],
            "before": {"abs": cnt(fs, "abs"), "dir": cnt(fs, "dir"),
                       "any": cnt(fs, "any")},
            "after": {"abs": cnt(aft, "abs", src="a"),
                      "dir": cnt(aft, "dir", src="a"),
                      "any": cnt(aft, "any", src="a")},
            "_level0": level0,
        })
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--model", action="append", default=[],
                    help="solo estos modelos (por defecto, toda la flota servida)")
    ap.add_argument("--out-dir", default=os.path.join(REPO, "tmp", "46", "lens"))
    ap.add_argument("--json", default=os.path.join(REPO, "tmp", "46", "lens_atlas.json"))
    ap.add_argument("--check", action="store_true", help="no escribe ningún TXD")
    ap.add_argument("--pool-min", type=int, default=POOL_WARM_MIN,
                    help="muestra templada mínima del racimo para fiarse de su "
                         "matiz (defecto %d)" % POOL_WARM_MIN)
    a = ap.parse_args()
    ia.BASE = ROOT
    ia.MODELS = ROOT
    names = [x.strip().lower() for m in a.model for x in m.split(",")] or \
        sorted(L.fleet(ROOT))
    rows = table_rows()
    dirs = [os.path.dirname(a.json)] + ([] if a.check else [a.out_dir])
    for d in dirs:
        if d and not os.path.isdir(d):
            os.makedirs(d)
    results = []
    for m in names:
        try:
            r = process(m, rows, a.pool_min)
        except SystemExit:
            continue
        except Exception as e:
            print("  AVISO %s: %s: %s" % (m, type(e).__name__, e))
            continue
        if r is None:
            continue
        results.append(r)
        data = io.open(os.path.join(ROOT, m + ".txd"), "rb").read()
        for e in r["emitted"]:
            lvl = e.pop("_level0")
            pos, fin, info = find_native(data, e["tex"])
            if info is None:
                print("  AVISO %s: no hay TextureNative %s en su TXD" % (m, e["tex"]))
                continue
            if info["compression"] != 1:
                print("  AVISO %s: %s no es DXT1 (comp=%d), no se emite"
                      % (m, e["tex"], info["compression"]))
                continue
            if len(e["tex"]) + len(SUFFIX) > 31:
                print("  AVISO %s: %s_lens no cabe en 32 bytes" % (m, e["tex"]))
                continue
            chunk = rewrite_native(data, pos, fin, e["tex"] + SUFFIX, lvl)
            out = build_txd(data, chunk)
            _, _, chk = find_native(out, (e["tex"] + SUFFIX).lower())
            if chk is None:
                raise SystemExit("ERROR: %s: el TXD emitido no relee %s_lens"
                                 % (m, e["tex"]))
            e["txd_bytes"] = len(out)
            if not a.check:
                path = os.path.join(a.out_dir, m + SUFFIX + ".txd")
                io.open(path, "wb").write(out)
                e["txd_out"] = path
    io.open(a.json, "w", encoding="utf-8").write(json.dumps(results, indent=1))

    em = [e for r in results for e in r["emitted"]]
    b_abs = sum(e["before"]["abs"][0] for e in em)
    a_abs = sum(e["after"]["abs"][0] for e in em)
    b_dir = sum(e["before"]["dir"][0] for e in em)
    a_dir = sum(e["after"]["dir"][0] for e in em)
    print("\nmodelos tratados: %d | atlas derivados: %d | puerta de muestra: %d"
          % (len(results), len(em), a.pool_min))
    print("texelas re-tenidas: %d | DXT1: %.0f KB"
          % (sum(e["painted"] for e in em),
             sum(e["bytes_dxt1"] for e in em) / 1024.0))
    print("round-trip del encoder: error max %d, medio %.3f "
          "(las caras se re-miden sobre los bytes ya codificados)"
          % (max([e["rt_max_err"] for e in em] or [0]),
             float(np.mean([e["rt_mean_err"] for e in em] or [0]))))
    print("\nVARA ABSOLUTA  MIXTAS:  %3d -> %3d  (%.0f -> %.0f cm2)"
          % (b_abs, a_abs, sum(e["before"]["abs"][1] for e in em),
             sum(e["after"]["abs"][1] for e in em)))
    print("VARA DIRECCION >=15%%:   %3d -> %3d  (%.0f -> %.0f cm2)"
          % (b_dir, a_dir, sum(e["before"]["dir"][1] for e in em),
             sum(e["after"]["dir"][1] for e in em)))
    blocked = [x for r in results for x in r["blocked"]]
    print("\nsin muestra (se dejan como están): %d racimos" % len(blocked))
    for x in blocked:
        print("  %-9s %-20s %-14s %d texelas templadas"
              % (x["model"], x["tex"], x["node"], x["warm"]))
    print("\nmatiz por atlas (nodo:muestra/origen):")
    for r in results:
        for e in r["emitted"]:
            print("  %-9s %-20s %4d texelas  err %3d  %s"
                  % (r["model"], e["tex"], e["painted"], e["rt_max_err"],
                     " ".join("%s:%d/%s" % (g["node"].replace("indicator", ""),
                                            g["warm"], g["origin"])
                              for g in e["groups"])))
    print("\njson -> %s" % a.json)


if __name__ == "__main__":
    main()
