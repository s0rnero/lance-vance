#!/usr/bin/env python3
"""Audita los sectores naranjas de los vehiculos y rehace sus intermitentes.

El plan 28.0 exige que el intermitente sea **el sector naranja real** del modelo:
si el coche/moto trae un nodo `indicator_*` se usa su material (como el mod);
si el naranja esta pintado en la textura de la carroceria y no hay nodo, se
separan las caras de ese sector en un nodo `indicator_*` nuevo (misma
geometria, un solo material). Nunca se inventa una corona en el faro.

El plan 33.0 sustituye ese nodo por la **lente del mod**: un chunk de 9
vertices / 8 triangulos con material `lights`, copiado byte a byte de
`polwintergreen.dff`, colocado en el centro de la lente. `--lens MODELO`
rehace los nodos viejos y anade los que falten; la carroceria no se toca.

La lente puede vivir en cualquier atomic: la carroceria (`chassis_hi`), un
bumper, una aleta o una puerta. El nodo nuevo se cuelga del marco de ese
atomic con matriz identidad, asi que las coordenadas locales de los vertices
siguen valiendo y la lente no se mueve. Para decidir delante/detras y
izquierda/derecha se usa la matriz **completa** del marco (con traslacion).

Uso:
    python tools/indicator_audit.py                # audita y escribe el JSON
    python tools/indicator_audit.py --check        # falla si hay contradicciones
    python tools/indicator_audit.py --patch faggio # separa las caras naranjas
    python tools/indicator_audit.py --patch pony --dry   # solo muestra el plan

El JSON canonico vive en `gamefiles/models/indicator_caps.json` (versionado) y
se copia a `streamed/models/` y `bootseed/models/` para que viaje con los datos.
"""
import argparse
import base64
import collections
import hashlib
import json
import math
import os
import shutil
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dff_geom as dg
import txd_raster as tr

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS = os.path.join(ROOT, "streamed", "models", "gta3.img")
# BASE es de donde se LEE (la base vanilla con --from); las escrituras van
# siempre a las tres copias servidas aunque BASE apunte a otra parte.
BASE = MODELS
GAMEFILES = os.path.join(ROOT, "gamefiles", "models")
BOOTSEED = os.path.join(ROOT, "bootseed", "models")
JSON_NAME = "indicator_caps.json"

INDICATOR_NODES = {
    "indicator_lf": ("front", "left"),
    "indicator_rf": ("front", "right"),
    "indicator_lr": ("rear", "left"),
    "indicator_rr": ("rear", "right"),
    "indicator2_lf": ("front", "left"),
    "indicator2_rf": ("front", "right"),
    "indicator2_lr": ("rear", "left"),
    "indicator2_rr": ("rear", "right"),
}

IDENTITY = (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0)
TEXTURE_CACHE = {}
AMBER_CACHE = {}


# --------------------------------------------------------------------------
# chunks RW
# --------------------------------------------------------------------------
def chunks(buf, start, end):
    out = []
    p = start
    while p + 12 <= end:
        typ, size, ver = struct.unpack_from("<III", buf, p)
        if p + 12 + size > end:
            raise ValueError("chunk truncado en %d (tipo %x)" % (p, typ))
        out.append((typ, ver, buf[p + 12:p + 12 + size]))
        p += 12 + size
    return out


def chunk(typ, ver, body):
    return struct.pack("<III", typ, len(body), ver) + body


def find(children, typ):
    for t, v, b in children:
        if t == typ:
            return v, b
    return None, None


def parse_geometry(struct_body):
    flags, ntris, nverts, nmorph = struct.unpack_from("<Iiii", struct_body, 0)
    ntex = (flags >> 16) & 0xFFFF
    size = len(struct_body)

    def tam(extra):
        return (16 + extra
                + (4 * nverts if flags & 0x08 else 0)
                + 8 * ntex * nverts
                + 8 * ntris
                + 16 + 8 + 12 * nverts * 2 * max(1, nmorph))

    extra = 0
    if tam(0) != size:
        if tam(12) == size:
            extra = 12
        else:
            raise ValueError("geometria de tamano inesperado: %d" % size)
    p = 16 + extra
    if flags & 0x08:
        p += 4 * nverts
    q = p
    uvs = []
    for _ in range(ntex):
        vals = struct.unpack_from("<%df" % (2 * nverts), struct_body, q)
        uvs.append([(vals[i * 2], vals[i * 2 + 1]) for i in range(nverts)])
        q += 8 * nverts
    tri_start = q
    tri_end = q + 8 * ntris
    tris = []
    for i in range(ntris):
        t0, t1 = struct.unpack_from("<II", struct_body, q + 8 * i)
        tris.append((t0 >> 16, t0 & 0xFFFF, t1 >> 16, t1 & 0xFFFF))
    verts = []
    q = tri_end
    for _ in range(max(1, nmorph)):
        q += 16
        hasv, hasn = struct.unpack_from("<ii", struct_body, q)
        q += 8
        if hasv:
            vs = struct.unpack_from("<%df" % (3 * nverts), struct_body, q)
            verts = [vs[i * 3:i * 3 + 3] for i in range(nverts)]
            q += 12 * nverts
        if hasn:
            q += 12 * nverts
    if q != size:
        raise ValueError("geometria mal medida: %d != %d" % (q, size))
    return {"flags": flags, "nverts": nverts, "nmorph": nmorph,
            "prefix": struct_body[:tri_start], "tris": tris,
            "suffix": struct_body[tri_end:], "uvs": uvs, "verts": verts}


def build_struct(geo, tris, mat_index=None, clear_tristrip=True):
    """Reescribe la lista de triangulos conservando el material original.

    `mat_index` solo se usa en la geometria clonada del intermitente (todas
    sus caras comparten el material naranja). En la geometria original NO se
    debe tocar el material de cada cara: la carroceria usa cuatro materiales.
    """
    body = bytearray(geo["prefix"])
    flags = geo["flags"] & ~1 if clear_tristrip else geo["flags"]
    struct.pack_into("<I", body, 0, flags)
    struct.pack_into("<i", body, 4, len(tris))
    for (a, b, c, m) in tris:
        if mat_index is not None:
            m = mat_index
        body += struct.pack("<II", (a << 16) | b, (c << 16) | m)
    body += geo["suffix"]
    return bytes(body)


def build_mesh(tris):
    """Malla en el orden que lee librw: cabecera de cada grupo y SUS indices.

    `readMesh` recorre las mallas intercaladas: lee `{numIndices, matIndex}` y
    a continuacion `numIndices` indices de 32 bits. Escribir todas las
    cabeceras primero y luego todos los indices (lo que hacia esta funcion)
    desplaza el lector, que toma indices como materiales: el modelo falla con
    `MESHBADMAT`/`FAIL model` y el clump roto revienta en
    `Frame::updateObjects` (plan 34.0, visto con `esperant` y `glendale`).
    """
    groups = {}
    order = []
    for (a, b, c, m) in tris:
        if m not in groups:
            groups[m] = []
            order.append(m)
        groups[m].extend((a, b, c))
    body = struct.pack("<III", 0, len(order), 3 * len(tris))
    for m in order:
        idx = groups[m]
        body += struct.pack("<Ii", len(idx), m)
        body += struct.pack("<%dI" % len(idx), *idx)
    return chunk(0x50E, 0xC02FFFF, body)


def build_geometry(struct_ver, matlist_ver, geo, tris, matlist_body, mat_index=None):
    return (chunk(0x01, struct_ver, build_struct(geo, tris, mat_index)) +
            chunk(0x08, matlist_ver, matlist_body) +
            chunk(0x03, 0xC02FFFF, build_mesh(tris)))


# --------------------------------------------------------------------------
# TXD: buscar pixeles naranjas del sector del intermitente
# --------------------------------------------------------------------------
def _decode_native(data, nt):
    """Nivel 0 decodificado con el alfa que el MOTOR ve.

    Plan 56.0 T1b: en las texturas sin canal de alfa (`hasAlpha == 0` y sin
    compresion DXT) el byte alfa del fichero es basura del conversor de VC, no
    transparencia: medido en `fbirancher868bit128` (128x128, depth 32,
    fmt 0x8600, hasAlpha 0, compression 0) las texelas de la lente son
    (156,80,8,0) y las contiguas (164,89,8,0) mientras el gris del capó viene
    con alfa 146. `decode` lo devuelve tal cual, `_face_render` descartaba toda
    texela con alfa 0 y la lente entera quedaba fuera de la cuenta: de ahi el
    `amb=0` de las 1696 caras medida-pixel del fbiranch, cuando el atlas tiene
    828 texelas naranja. El motor, en cambio, pinta esas texelas (no muestrea
    alfa porque la textura declara no tenerlo), asi que aqui se normalizan a
    opacas. Con DXT1/DXT3/DXT5 el alfa SI es real (el modo c0<=c1 de DXT1 es
    transparencia de 1 bit) y no se toca.
    """
    w, h = nt["w"], nt["h"]
    sz, off = nt["levels"][0]
    px = tr.decode(data[off:off + sz], w, h, nt["depth"], nt["fmt"],
                   nt["has_alpha"], nt["compression"])
    if not px:
        return None
    if not nt["has_alpha"] and not nt["compression"]:
        px = [(c[0], c[1], c[2], 255) for c in px]
    return px


def load_txd(model):
    path = os.path.join(BASE, model + ".txd")
    if model in TEXTURE_CACHE:
        return TEXTURE_CACHE[model]
    out = {}
    if os.path.exists(path):
        data = open(path, "rb").read()
        try:
            posiciones = tr.dict_textures(data)
        except Exception:
            posiciones = []
        for pos in posiciones:
            try:
                nt = tr.read_native(data, pos)
                if nt is None:
                    continue
                px = _decode_native(data, nt)
                w, h = nt["w"], nt["h"]
            except Exception:
                continue   # textura comprimida que el lector no soporta
            if not px:
                continue
            xs, ys = [], []
            for y in range(h):
                for x in range(w):
                    r, g, b, a = px[y * w + x]
                    if r > 150 and 60 < g < 200 and b < 80 and r > g + 30 and a > 0:
                        xs.append(x)
                        ys.append(y)
            if xs:
                out[nt["nombre"]] = {
                    "u0": min(xs) / w, "u1": (max(xs) + 1) / w,
                    "v0": min(ys) / h, "v1": (max(ys) + 1) / h,
                    "count": len(xs), "w": w, "h": h,
                }
    TEXTURE_CACHE[model] = out
    return out


def orange_for_material(orange_tex, name):
    if not name:
        return None
    key = name.lower()
    for tex, box in orange_tex.items():
        if tex.lower() == key:
            return box
    return None


def texture_pixels(model):
    """Pixeles decodificados de cada textura del TXD: {nombre: (w, h, px)}."""
    if model in AMBER_CACHE:
        return AMBER_CACHE[model]
    path = os.path.join(BASE, model + ".txd")
    out = {}
    if os.path.exists(path):
        data = open(path, "rb").read()
        try:
            posiciones = tr.dict_textures(data)
        except Exception:
            posiciones = []
        for pos in posiciones:
            try:
                nt = tr.read_native(data, pos)
                if nt is None:
                    continue
                px = _decode_native(data, nt)
                w, h = nt["w"], nt["h"]
            except Exception:
                continue
            if px:
                out[nt["nombre"].lower()] = (w, h, px)
    AMBER_CACHE[model] = out
    return out


def _hue(r, g, b):
    """Angulo de color 0..360 (0 = rojo puro, 30 = naranja de lente)."""
    hi, lo = max(r, g, b), min(r, g, b)
    d = hi - lo
    if not d:
        return 0.0
    if hi == r:
        return 60.0 * (((g - b) / d) % 6.0)
    if hi == g:
        return 60.0 * ((b - r) / d + 2.0)
    return 60.0 * ((r - g) / d + 4.0)


def _is_amber(c):
    """Naranja de lente: hue 15..55, no rojo de piloto ni amarillo de pintura.

    La ventana de `g` sola no separaba: el piloto rojo del pheonix (180,63,29)
    y el del hermes (148,28,8) entraban con g en 45..115 y salian tintados
    ambar, que es la alucinacion que reporta el jugador. El hue si los separa.
    El amarillo de carroceria lo saca `r-g >= 45` (el taxi mide r-g ~ 20-36), no
    el tope de g: con tope de g el brillo de la propia lente (222,137,57 del
    boxville) se quedaba fuera y el nodo salia con medio quad, la regresion que
    el plan 42.0 ya probio.

    El corte bajo de 22 a 15 con el plan 45.0, medido sobre el arbol servido
    (`C:\\tmp\\iav42\\cut45.py`): las puertas de color intactas, bajando 22->15
    entran 18 caras, 7 de ellas dentro de nodos `indicator_*` (baggage, washing
    y moonbeam) y el resto en carrocería, donde manda la compuerta geometrica y
    no el matiz. Ninguna cara de `headlight_*`, `taillight_*`, `brakelight_*` o
    `reversinglight_*` pasa las puertas de color A NINGUN matiz (0 de 183 y 0 de
    668), asi que el corte 15 no puede inventar ambar en una lampara nombrada.
    Los rojos que el 42.x sacaba a mano siguen fuera: pheonix hue 13,5 y hermes
    hue 8,6.
    """
    r, g, b, a = c
    if a == 0 or r < 105 or b > 75:
        return False
    if r - g < 45 or g - b < 30:
        return False
    return 15.0 <= _hue(r, g, b) <= 55.0


# --------------------------------------------------------------------------
# analisis
# --------------------------------------------------------------------------
def load_model(model, with_geoms=False):
    path = os.path.join(BASE, model + ".dff")
    buf = open(path, "rb").read()
    if with_geoms:
        return (buf,) + dg.parse(buf)
    clump = None
    for c in dg.children(buf, 0, len(buf)):
        if c.type == 0x10:
            clump = c
            break
    if clump is None:
        raise ValueError("sin clump")
    frames, atomics = [], []
    for c in dg.children(buf, clump.start, clump.end):
        if c.type == 0x0E:
            frames = dg.parse_frames(buf, c)
        elif c.type == 0x14:
            st = dg.first(buf, c, 0x01)
            if st is not None:
                atomics.append(struct.unpack_from("<2I", buf, st.start))
    return buf, frames, atomics


def side_caps(frames, atomics):
    """{'front': {'left': bool, 'right': bool}, ...} con los nodos reales."""
    caps = {"front": {"left": False, "right": False},
            "rear": {"left": False, "right": False}}
    node_frames = {}
    for f in frames:
        if f["name"] in INDICATOR_NODES:
            where, side = INDICATOR_NODES[f["name"]]
            node_frames[f["i"]] = (where, side)
    for fi, _gi in atomics:
        if fi in node_frames:
            where, side = node_frames[fi]
            caps[where][side] = True
    return caps


def analyze(model):
    _buf, frames, atomics = load_model(model)
    caps = side_caps(frames, atomics)
    has_node = any(caps[w][s] for w in caps for s in caps[w])
    orange = load_txd(model)
    source = "node" if has_node else "none"
    if not has_node:
        if any(t.lower() == "lightson" and b["count"] for t, b in orange.items()):
            source = "lightsOn"
        else:
            body = {t: b for t, b in orange.items()
                    if t.lower() not in ("lights", "lightson")}
            if body:
                source = "body"
    return {
        "model": model,
        "frontLeft": caps["front"]["left"],
        "frontRight": caps["front"]["right"],
        "rearLeft": caps["rear"]["left"],
        "rearRight": caps["rear"]["right"],
        "hasNode": has_node,
        "orangeTextures": sorted(orange),
        "source": source,
    }


def vehicle_models():
    """Modelos de vehiculo segun default.ide (evita auditar 1700 DFFs)."""
    ide = os.path.join(ROOT, "streamed", "data", "default.ide")
    names = []
    section = None
    for raw in open(ide, encoding="latin-1").read().replace("\r\n", "\n").split("\n"):
        line = raw.split("#")[0].strip()
        if not line:
            continue
        low = line.lower()
        if low in ("cars", "bikes", "boats", "planes", "helis", "trains"):
            section = low
            continue
        if low == "end":
            section = None
            continue
        if section is None:
            continue
        campos = [c.strip() for c in line.split(",")]
        if len(campos) >= 3 and campos[0].isdigit():
            names.append(campos[1].lower())
    return [n for n in names if os.path.exists(os.path.join(BASE, n + ".dff"))]


# --------------------------------------------------------------------------
# split de caras naranjas (carroceria, bumper, puerta o aleta)
# --------------------------------------------------------------------------
def _xform(m, v):
    """Matriz 4x4 completa: rotacion + traslacion (dg.mul ignora esta ultima)."""
    return [m[a][0] * v[0] + m[a][1] * v[1] + m[a][2] * v[2] + m[a][3]
            for a in range(3)]


def _is_identity_frame(f):
    return (all(abs(f["rot"][i] - (1.0 if i in (0, 4, 8) else 0.0)) < 1e-5
                for i in range(9)) and all(abs(x) < 1e-5 for x in f["pos"]))


def _world_bbox(frames, geoms, atomics):
    lo, hi = [1e9] * 3, [-1e9] * 3
    for fi, gi in atomics:
        m = dg.world_of(frames, fi)
        for v in geoms[gi]["verts"]:
            w = _xform(m, v)
            for k in range(3):
                lo[k] = min(lo[k], w[k])
                hi[k] = max(hi[k], w[k])
    return lo, hi


def _clusters(points, rad=0.35):
    """Union-find por distancia; devuelve listas de indices de `points`."""
    n = len(points)
    par = list(range(n))

    def root(a):
        while par[a] != a:
            par[a] = par[par[a]]
            a = par[a]
        return a

    for i in range(n):
        for j in range(i + 1, n):
            d = [points[i][k] - points[j][k] for k in range(3)]
            if sum(x * x for x in d) < rad * rad:
                ri, rj = root(i), root(j)
                if ri != rj:
                    par[rj] = ri
    groups = {}
    for i in range(n):
        groups.setdefault(root(i), []).append(i)
    return list(groups.values())


def _candidate_lenses(model, frames, geoms, atomics):
    """Racimos compactos y simetricos de caras naranjas, en coordenadas del coche.

    El color se comprueba en el pixel real de la textura en el centro UV de
    cada cara (no en una caja envolvente, que con madera o pintura amarilla
    arrastra media carroceria) y la posicion se calcula con la matriz completa
    del atomic, asi que el criterio vale para la carroceria, un bumper, una
    aleta o una puerta. Un racimo cuenta como lente si al menos un tercio de
    sus caras son naranja saturado.
    """
    pix = texture_pixels(model)
    lo, hi = _world_bbox(frames, geoms, atomics)
    largo = max(hi[1] - lo[1], 1.0)
    alto = max(hi[2] - lo[2], 1.0)
    raw = []
    for fi, gi in atomics:
        name = frames[fi]["name"].lower()
        if name in INDICATOR_NODES:
            continue      # lente ya separada en una pasada anterior
        if "vlo" in name or name.startswith("extra") or name.endswith("_dam"):
            continue      # LOD, extras y variante dañada: no se tocan
        g = geoms[gi]
        if not g["uvs"]:
            continue
        m = dg.world_of(frames, fi)
        for mi, (col, tex) in enumerate(g["mats"]):
            key = (tex or "").lower()
            if not key or key not in pix:
                continue
            w, h, px = pix[key]
            if not any(_is_amber(c) for c in px):
                continue
            faces = []
            for tri in g["tris"]:
                if tri[3] != mi:
                    continue
                a, b, c = tri[0], tri[1], tri[2]
                us = [g["uvs"][0][v][0] for v in (a, b, c)]
                vs = [g["uvs"][0][v][1] for v in (a, b, c)]
                mu, mv = sum(us) / 3.0, sum(vs) / 3.0
                # Las UV de varios vanilla pasan de 1 (repiten la textura:
                # el Rumpo mapea la lente en v=1.025). El rasterizador del
                # juego envuelve; recortar a [0,1] leia la ultima fila y estos
                # coches nunca encontraban su sector naranja.
                x = int((mu % 1.0) * w) % w
                y = int((mv % 1.0) * h) % h
                if not _is_amber(px[y * w + x]):
                    continue     # cara de pintura, madera o cromado
                local = [sum(g["verts"][v][k] for v in (a, b, c)) / 3.0
                         for k in range(3)]
                faces.append((tri, _xform(m, local)))
            if not faces:
                continue
            # Se separa por lado ANTES de agrupar: las dos lentes del mismo
            # eje quedan a mas de un metro y con radio 0.35 se partian en
            # trozos de una cara que luego no pasaban el filtro de simetria.
            for lado in (-1, 1):
                lado_faces = [f for f in faces if (f[1][0] < 0) == (lado < 0)]
                if not lado_faces:
                    continue
                for idx in _clusters([f[1] for f in lado_faces], rad=0.5):
                    grupo = [lado_faces[i] for i in idx]
                    pts = [f[1] for f in grupo]
                    xs = [p[0] for p in pts]
                    ys = [p[1] for p in pts]
                    zs = [p[2] for p in pts]
                    raw.append({
                        "fi": fi, "gi": gi, "mi": mi, "tex": tex,
                        "faces": grupo,
                        "n": len(grupo), "cx": sum(xs) / len(xs),
                        "cy": sum(ys) / len(ys), "cz": sum(zs) / len(zs),
                        "dx": max(xs) - min(xs), "dy": max(ys) - min(ys),
                        "dz": max(zs) - min(zs),
                    })

    def compact(c):
        return (c["dx"] <= 0.55 and c["dy"] <= 0.60 and c["dz"] <= 0.60 and
                abs(c["cx"]) >= 0.22)

    def mirror(a, b):
        return (abs(a["cx"] + b["cx"]) <= 0.30 and
                abs(a["cy"] - b["cy"]) <= 0.30 and
                abs(a["cz"] - b["cz"]) <= 0.30)

    compactos = [c for c in raw if compact(c)]
    keep = []
    for c in compactos:
        rel = (c["cy"] - lo[1]) / largo
        if 0.34 < rel < 0.66:
            continue      # en el centro del coche no hay lente de intermitente
        zrel = (c["cz"] - lo[2]) / alto
        if zrel < 0.12 or zrel > 0.94:
            continue
        if not any(mirror(c, o) for o in compactos if o is not c):
            continue
        keep.append(c)

    # como mucho dos racimos por lado (indicator_* e indicator2_*); de cada
    # lado se quedan los que estan mas cerca del extremo del coche, que es
    # donde van las luces de verdad.
    por_lado = {}
    for c in sorted(keep, key=lambda c: -abs((c["cy"] - lo[1]) / largo - 0.5)):
        where = "front" if (c["cy"] - lo[1]) / largo > 0.5 else "rear"
        side = "left" if c["cx"] < 0 else "right"
        por_lado.setdefault((where, side), []).append(c)
    keep = []
    for cs in por_lado.values():
        keep.extend(cs[:2])
    return keep, lo, hi


def _free_node(where, side, used):
    """Primer nombre indicator_*/indicator2_* libre para ese lado.

    Un modelo puede necesitar dos pasadas (el Cabbie tenia delante y detras en
    la misma carroceria y la primera pasada solo vio delante); la segunda no
    debe chocar con el nombre que ya existe.
    """
    for n in (0, 1):
        name = "indicator%s_%s%s" % ("2" if n else "",
                                     "l" if side == "left" else "r",
                                     "f" if where == "front" else "r")
        if name not in used:
            return name
    return None


def split_model(model, write=True, dry=False):
    """Separa el sector naranja real en nodos `indicator_*`.

    Sirve para cualquier atomic del vehiculo. El nodo nuevo cuelga del marco
    del propio atomic (o de su dummy, cuando el atomic no tiene transformacion)
    con matriz identidad, asi que los vertices conservan sus coordenadas
    locales y la lente se queda exactamente donde estaba.

    Si el modelo ya tiene algun nodo indicator_* se anaden los que falten (las
    lentes ya separadas se ignoran), no se vuelve a partir el resto.
    """
    buf, frames, geoms, atomics = load_model(model, with_geoms=True)
    top = chunks(buf, 0, len(buf))
    clump_ver, clump_body = None, None
    for t, v, b in top:
        if t == 0x10:
            clump_ver, clump_body = v, b
            break
    if clump_body is None:
        raise SystemExit("%s: sin clump" % model)

    cc = chunks(clump_body, 0, len(clump_body))
    clump_struct = bytearray(find(cc, 0x01)[1])
    framelist_ver, framelist_body = find(cc, 0x0E)
    geomlist_ver, geomlist_body = find(cc, 0x1A)
    if geomlist_body is None:
        raise SystemExit("%s: sin lista de geometrias" % model)

    gchunks = chunks(geomlist_body, 0, len(geomlist_body))
    geoms_raw = [b for t, v, b in gchunks if t == 0x0F]
    geoms_ver = [v for t, v, b in gchunks if t == 0x0F]
    if len(geoms_raw) != len(geoms):
        raise SystemExit("%s: geometrias fuera de la lista; split no soportado" % model)

    if not texture_pixels(model):
        raise SystemExit("%s: no se pudo leer ninguna textura" % model)

    keep, lo, hi = _candidate_lenses(model, frames, geoms, atomics)
    if not keep:
        raise SystemExit("%s: sin lentes naranjas compactas y simetricas" % model)
    used_names = {f["name"] for f in frames}
    largo = max(hi[1] - lo[1], 1.0)

    # agrupar por geometria y lado; el primer racimo de cada lado es el 1 y el
    # segundo (p. ej. el repetidor de la aleta) usa indicator2_*.
    buckets = {}
    for c in sorted(keep, key=lambda c: -(c["cy"] - lo[1]) / largo):
        where = "front" if (c["cy"] - lo[1]) / largo > 0.5 else "rear"
        side = "left" if c["cx"] < 0 else "right"
        key = (c["gi"], where, side)
        b = buckets.setdefault(key, {"fi": c["fi"], "mi": c["mi"],
                                     "tex": c["tex"], "tris": []})
        b["tris"].extend(f[0] for f in c["faces"])

    fchunks = chunks(framelist_body, 0, len(framelist_body))
    fstruct = bytearray(find(fchunks, 0x01)[1])
    numframes = struct.unpack_from("<I", fstruct, 0)[0]
    names = [b for t, v, b in fchunks if t == 0x03]
    if len(names) != numframes:
        raise SystemExit("%s: frame list y nombres no cuadran" % model)

    def add_frame(name, parent):
        idx = struct.unpack_from("<I", fstruct, 0)[0]
        struct.pack_into("<I", fstruct, 0, idx + 1)
        fstruct.extend(struct.pack("<9f3fiI", *(IDENTITY + (0.0, 0.0, 0.0, parent, 0))))
        names.append(chunk(0x253F2FE, 0xC02FFFF, name.encode("latin-1")))
        return idx

    atomic_index = {(fi, gi): k for k, (fi, gi) in enumerate(atomics)}
    atomic_bodies = [b for t, v, b in cc if t == 0x14]
    parsed = {}

    def raw_geo(gi):
        """Struct crudo (prefix/suffix) de la geometria gi, para build_struct."""
        if gi not in parsed:
            gch = chunks(geoms_raw[gi], 0, len(geoms_raw[gi]))
            parsed[gi] = parse_geometry(find(gch, 0x01)[1])
        return parsed[gi]
    geoms_new = list(geoms_raw)
    geo_index = len(geoms_new)
    dropped = {}
    added = []
    for key in sorted(buckets, key=lambda k: (k[1] != "front", k[2], k[0])):
        gi, where, side = key
        b = buckets[key]
        gch = chunks(geoms_raw[gi], 0, len(geoms_raw[gi]))
        # La version del STRUCT decide si su cuerpo lleva los 12 bytes de
        # surface props. `raw_geo(gi)` copia el prefix del STRUCT origen, asi
        # que el nodo nuevo DEBE declarar la misma version que la geometria de
        # la que sale: si no, el lector consume 12 bytes de menos y el modelo
        # falla con "Couldn't find chunk MATLIST" (plan 32.0).
        stv_gi, _ = find(gch, 0x01)
        mlv_gi, ml = find(gch, 0x08)
        mats = [x for t, v, x in chunks(ml, 0, len(ml)) if t == 0x07]
        matlist = chunk(0x01, 0xC02FFFF, struct.pack("<Ii", 1, -1)) + \
            chunk(0x07, 0xC02FFFF, mats[b["mi"]])
        tris = b["tris"]
        geoms_new.append(
            build_geometry(stv_gi, mlv_gi, raw_geo(gi), tris, matlist, 0))
        dropped.setdefault(gi, set()).update(id(t) for t in tris)
        f = frames[b["fi"]]
        parent = f["parent"] if _is_identity_frame(f) else f["i"]
        name = _free_node(where, side, used_names)
        if name is None:
            continue      # ya tiene indicator_* e indicator2_* en ese lado
        used_names.add(name)
        gi_idx = atomic_index[(b["fi"], gi)]
        added.append((add_frame(name, parent), geo_index, name, len(tris),
                      frames[b["fi"]]["name"], atomic_bodies[gi_idx]))
        geo_index += 1

    # la geometria original pierde las caras de la lente (no se duplica)
    for gi, ids in dropped.items():
        rest = [t for t in geoms[gi]["tris"] if id(t) not in ids]
        if len(rest) == len(geoms[gi]["tris"]):
            raise SystemExit("%s: no se quito ninguna cara del geom %d" % (model, gi))
        gch = chunks(geoms_raw[gi], 0, len(geoms_raw[gi]))
        stv = find(gch, 0x01)[0]
        mlv, mlb = find(gch, 0x08)
        geoms_new[gi] = build_geometry(stv, mlv, raw_geo(gi), rest, mlb)

    for entry in added:
        frame, gidx, _name, _ntris, _src, template = entry
        extra_child = [b for t, v, b in chunks(template, 0, len(template))
                       if t == 0x03][0]
        atomic_bodies.append(
            chunk(0x01, 0xC02FFFF, struct.pack("<4I", frame, gidx, 5, 0)) +
            chunk(0x03, 0xC02FFFF, extra_child))
    struct.pack_into("<I", clump_struct, 0, len(atomic_bodies))

    framelist_body = chunk(0x01, 0xC02FFFF, bytes(fstruct)) + b"".join(
        chunk(0x03, 0xC02FFFF, n) for n in names)
    geomlist_body = chunk(0x01, 0xC02FFFF, struct.pack("<I", len(geoms_new))) + \
        b"".join(chunk(0x0F, geoms_ver[i] if i < len(geoms_ver) else 0xC02FFFF, g)
                 for i, g in enumerate(geoms_new))

    clump_ext = [b for t, v, b in cc if t == 0x03]
    body = chunk(0x01, 0xC02FFFF, bytes(clump_struct))
    body += chunk(0x0E, framelist_ver, framelist_body)
    body += chunk(0x1A, geomlist_ver, geomlist_body)
    body += b"".join(chunk(0x14, 0xC02FFFF, a) for a in atomic_bodies)
    body += b"".join(chunk(0x03, 0xC02FFFF, b) for b in clump_ext)
    out = chunk(0x10, clump_ver, body)

    resumen = ", ".join("%s(%d en %s)" % (n, c, src)
                        for _f, _g, n, c, src, _t in added)
    if dry:
        print("%s: (dry) separaria %s; %d bytes" % (model, resumen, len(out)))
        return added
    for d in (os.path.join(GAMEFILES, "gta3.img"), MODELS):
        os.makedirs(d, exist_ok=True)
        open(os.path.join(d, model + ".dff"), "wb").write(out)
    print("%s: separadas %s -> %d bytes" % (model, resumen, len(out)))
    return added


# --------------------------------------------------------------------------
def geometry_has_mesh(gbody):
    subs = chunks(gbody, 0, len(gbody))
    for t, _v, body in subs:
        if t == 0x03:
            if any(x == 0x50E for x, _xv, _xb in chunks(body, 0, len(body))):
                return True
    return False


def repair_model(model, write=True, dry=False):
    path = os.path.join(MODELS, model + ".dff")
    if not os.path.exists(path):
        raise SystemExit("%s: no existe" % model)
    buf = open(path, "rb").read()
    clump_ver, clump_body = None, None
    for t, v, b in chunks(buf, 0, len(buf)):
        if t == 0x10:
            clump_ver, clump_body = v, b
            break
    if clump_body is None:
        raise SystemExit("%s: sin clump" % model)

    cc = chunks(clump_body, 0, len(clump_body))
    geomlist_ver, geomlist_body = find(cc, 0x1A)
    if geomlist_body is None:
        raise SystemExit("%s: sin lista de geometrias" % model)
    gchunks = chunks(geomlist_body, 0, len(geomlist_body))
    gstruct_ver, gstruct = find(gchunks, 0x01)

    geoms = []
    fixed = 0
    for t, gver, gbody in gchunks:
        if t != 0x0F:
            continue
        if geometry_has_mesh(gbody):
            geoms.append((gver, gbody))
            continue
        subs = chunks(gbody, 0, len(gbody))
        stv, stb = find(subs, 0x01)
        mlv, mlb = find(subs, 0x08)
        if stb is None or mlb is None:
            raise SystemExit("%s: geometria sin struct o matlist" % model)
        geo = parse_geometry(stb)
        nmats = struct.unpack_from("<I", mlb, 12)[0]
        tris = [(a, b, c, m if 0 <= m < nmats else 0)
                for (a, b, c, m) in geo["tris"]]
        geoms.append((gver, build_geometry(stv, mlv, geo, tris, mlb)))
        fixed += 1

    if fixed == 0:
        print("%s: nada que reparar" % model)
        return 0
    new_geomlist = chunk(0x01, gstruct_ver, gstruct) + b"".join(
        chunk(0x0F, v, b) for v, b in geoms)
    new_body = b"".join(
        chunk(0x1A, v, new_geomlist) if t == 0x1A else chunk(t, v, b)
        for t, v, b in cc)
    out = chunk(0x10, clump_ver, new_body)
    if dry:
        print("%s: (dry) repararia %d geometrias -> %d bytes" % (model, fixed, len(out)))
        return fixed
    for d in (os.path.join(GAMEFILES, "gta3.img"), MODELS, os.path.join(BOOTSEED, "gta3.img")):
        target = os.path.join(d, model + ".dff")
        if os.path.exists(target):
            open(target, "wb").write(out)
    print("%s: %d geometrias con malla -> %d bytes" % (model, fixed, len(out)))
    return fixed


def needs_repair(model):
    path = os.path.join(MODELS, model + ".dff")
    if not os.path.exists(path):
        return False
    try:
        buf = open(path, "rb").read()
        for t, v, b in chunks(buf, 0, len(buf)):
            if t != 0x10:
                continue
            cc = chunks(b, 0, len(b))
            _lv, lb = find(cc, 0x1A)
            if lb is None:
                return False
            for gt, _gv, gb in chunks(lb, 0, len(lb)):
                if gt == 0x0F and not geometry_has_mesh(gb):
                    return True
    except Exception:
        return False
    return False


# --------------------------------------------------------------------------
def _matlist_needs_fix(mlb):
    if not mlb:
        return False
    sub = chunks(mlb, 0, len(mlb))
    t, _v, b = sub[0]
    return t == 0x01 and len(b) == 4 and struct.unpack_from("<I", b, 0)[0] == 1


def _fixed_matlist(mlb):
    if not _matlist_needs_fix(mlb):
        return mlb
    sub = chunks(mlb, 0, len(mlb))
    t, v, b = sub[0]
    return chunk(t, v, struct.pack("<Ii", 1, -1)) + b"".join(
        chunk(a, av, ab) for a, av, ab in sub[1:])


def _mesh_of(eb):
    if eb is None:
        return None, None, None
    ver, mesh = None, None
    for t, v, b in chunks(eb, 0, len(eb)):
        if t == 0x50E:
            ver, mesh = v, b
    return ver, mesh, None


def _mesh_to_fix(mesh, ver):
    if mesh is None or len(mesh) < 12:
        return ver is not None
    flags, n, tot = struct.unpack_from("<III", mesh, 0)
    if n > 100000 or 12 + 8 * n > len(mesh):
        return True
    suma = 0
    for i in range(n):
        suma += struct.unpack_from("<I", mesh, 12 + 8 * i)[0]
    return suma > tot or 12 + 8 * n + 4 * suma > len(mesh)


def _clamped_mesh(mesh):
    flags, n, tot = struct.unpack_from("<III", mesh, 0)
    ents = [struct.unpack_from("<Ii", mesh, 12 + 8 * i) for i in range(n)]
    suma = sum(e[0] for e in ents)
    avail = max(0, (len(mesh) - 12 - 8 * n) // 4)
    used = min(suma, avail, tot)
    budget = used
    body = bytearray(struct.pack("<III", flags, n, used))
    for ni, mi in ents:
        take = ni if ni < budget else budget
        body += struct.pack("<Ii", take, mi)
        budget -= take
    base = 12 + 8 * n
    for i in range(used):
        body += struct.pack("<I", struct.unpack_from("<I", mesh, base + 4 * i)[0])
    return bytes(body)


def _geometry_is_consistent(gbody):
    subs = chunks(gbody, 0, len(gbody))
    stb = find(subs, 0x01)[1]
    mlb = find(subs, 0x08)[1]
    if stb is None or mlb is None or _matlist_needs_fix(mlb):
        return False
    eb = find(subs, 0x03)[1]
    ver, mesh, _ = _mesh_of(eb)
    return mesh is not None and not _mesh_to_fix(mesh, ver)


def _normalized_geometry(gbody, clamp=True):
    subs = chunks(gbody, 0, len(gbody))
    stv, stb = find(subs, 0x01)
    mlv, mlb = find(subs, 0x08)
    ev, eb = find(subs, 0x03)
    if stb is None or mlb is None:
        raise SystemExit("geometria sin struct o matlist")
    mlb = _fixed_matlist(mlb)
    plugins = []
    subs_ext = chunks(eb, 0, len(eb)) if eb is not None else []
    for t, v, b in subs_ext:
        if t == 0x50E:
            if clamp and _mesh_to_fix(b, v):
                b = _clamped_mesh(b)
            plugins.append(chunk(t, v, b))
        else:
            plugins.append(chunk(t, v, b))
    return (chunk(0x01, stv, stb) +
            chunk(0x08, mlv, mlb) +
            chunk(0x03, ev if ev is not None else 0xC02FFFF,
                  b"".join(plugins)))


def normalize_model(model, dry=False):
    path = os.path.join(MODELS, model + ".dff")
    if not os.path.exists(path):
        raise SystemExit("%s: no existe" % model)
    buf = open(path, "rb").read()
    clump_ver, clump_body = None, None
    for t, v, b in chunks(buf, 0, len(buf)):
        if t == 0x10:
            clump_ver, clump_body = v, b
            break
    if clump_body is None:
        raise SystemExit("%s: sin clump" % model)
    cc = chunks(clump_body, 0, len(clump_body))
    geomlist_body = find(cc, 0x1A)[1]
    if geomlist_body is None:
        raise SystemExit("%s: sin lista de geometrias" % model)
    gch = chunks(geomlist_body, 0, len(geomlist_body))
    counts = {"mesh": 0, "mat": 0, "ok": 0, "skip": 0}

    def handle(t, v, b):
        if t != 0x0F:
            return chunk(t, v, b)
        subs = chunks(b, 0, len(b))
        mlb = find(subs, 0x08)[1]
        eb = find(subs, 0x03)[1]
        ver, mesh, _ = _mesh_of(eb)
        fix_mesh = mesh is not None and _mesh_to_fix(mesh, ver)
        fix_mat = mlb is not None and _matlist_needs_fix(mlb)
        # No se toca lo que no se entiende:
        #  - NATIVE (0x01000000): los indices van en uint16, el recorte de 4
        #    bytes cambiaria el tamano del plugin y desalinearia la geometria.
        #  - tabla que no cabe en el chunk (n imposible): no hay nada que
        #    recortar con criterio.
        # El `matIndex` basura no descalifica: el motor ya lo acota a material 0
        # (`MESHBADMAT`) y el guardia `MESHOVER` de `readMesh` acota la escritura.
        if fix_mesh:
            _flags, n, _tot = struct.unpack_from("<III", mesh, 0)
            gst = find(subs, 0x01)[1]
            gflags = struct.unpack_from("<II", gst, 0)[1] if gst and len(gst) >= 8 else 0
            if n > 100000 or 12 + 8*n > len(mesh) or (gflags & 0x01000000):
                counts["skip"] += 1
                fix_mesh = False
        if not fix_mesh and not fix_mat:
            counts["ok"] += 1
            return chunk(t, v, b)
        if fix_mesh:
            counts["mesh"] += 1
        if fix_mat:
            counts["mat"] += 1
        return chunk(t, v, _normalized_geometry(b, clamp=fix_mesh))

    nuevo = []
    for t, v, b in cc:
        if t == 0x1A:
            nuevo.append(chunk(t, v, b"".join(handle(a, av, ab)
                                               for a, av, ab in gch)))
        else:
            nuevo.append(handle(t, v, b))
    out = chunk(0x10, clump_ver, b"".join(nuevo))
    changed = counts["mesh"] + counts["mat"]
    resumen = "%d mallas recortadas, %d matlist corregidos, %d ya correctas, %d sin tocar (NATIVE/ilegible)" % (
        counts["mesh"], counts["mat"], counts["ok"], counts["skip"])
    if changed == 0:
        print("%s: %s" % (model, resumen))
        return 0
    if dry:
        print("%s: (dry) %s -> %d bytes" % (model, resumen, len(out)))
        return changed
    for d in (os.path.join(GAMEFILES, "gta3.img"), MODELS,
              os.path.join(BOOTSEED, "gta3.img")):
        target = os.path.join(d, model + ".dff")
        if not os.path.exists(target):
            continue
        tmp = target + ".tmp"
        open(tmp, "wb").write(out)
        os.replace(tmp, target)
    print("%s: %s -> %d bytes" % (model, resumen, len(out)))
    return changed


# --------------------------------------------------------------------------
FAGGIO_SHA256 = "2dac117c1c4858c839312b899b45a4625ea8d3a21165a220374e9778f4f0a29a"


def _faggio_split_faces(buf):
    """METODO DESCARTADO (plan 33.0): parte las caras de la carroceria.

    Clonaba el chasis (596 vertices) en dos geometrias de 8 triangulos; el
    lector en dos fases del clump rechazaba el fichero y el modelo se
    reintentaba cada frame hasta tumbar la partida. Se conserva solo como
    referencia historica; la Faggio se parchea con `faggio_split_nodes`.
    """
    if hashlib.sha256(buf).hexdigest() != FAGGIO_SHA256:
        raise ValueError("faggio: modelo distinto al inspeccionado; no se modifica")
    frames, geoms, atomics = dg.parse(buf)
    chassis = next((fi, gi) for fi, gi in atomics if frames[fi]["name"].lower() == "chassis_hi")
    fi, gi = chassis
    parent = frames[fi]["parent"]
    if not _is_identity_frame(frames[fi]) or frames[parent]["name"].lower() != "chassis_dummy":
        raise ValueError("faggio: jerarquia del chasis inesperada")
    left_ids = (186, 187, 216, 409, 410, 411, 412, 413)
    right_ids = (414, 415, 416, 417, 418, 419, 420, 421)
    geo = geoms[gi]
    if any(geo["tris"][i][3] != 2 for i in left_ids + right_ids):
        raise ValueError("faggio: material de lente inesperado")

    top = chunks(buf, 0, len(buf))
    cv, cb = find(top, 0x10)
    cc = chunks(cb, 0, len(cb))
    fv, fb = find(cc, 0x0E)
    gv, gb = find(cc, 0x1A)
    fc, gc = chunks(fb, 0, len(fb)), chunks(gb, 0, len(gb))
    raw_geos = [b for t, _v, b in gc if t == 0x0F]
    if len(raw_geos) != len(geoms):
        raise ValueError("faggio: lista de geometrias dividida")
    sub = chunks(raw_geos[gi], 0, len(raw_geos[gi]))
    stv, st = find(sub, 0x01)
    mlv, ml = find(sub, 0x08)
    ev, ext = find(sub, 0x03)
    extensions = chunks(ext, 0, len(ext))
    _mv, mesh = find(extensions, 0x50E)
    flags, count, total = struct.unpack_from("<III", mesh)
    if flags != 1 or count == 0:
        raise ValueError("faggio: no es la malla de tiras esperada")
    p, seen, rendered = 12, 0, []
    for _ in range(count):
        n, mat = struct.unpack_from("<Ii", mesh, p)
        p += 8
        if n == 0 or mat < 0 or mat >= len(geo["mats"]) or p + 4*n > len(mesh):
            raise ValueError("faggio: malla o material invalido")
        indices = struct.unpack_from("<%dI" % n, mesh, p)
        p += 4*n
        seen += n
        if any(v >= len(geo["verts"]) for v in indices):
            raise ValueError("faggio: indice fuera de rango")
        for i in range(n - 2):
            a, b, c = indices[i:i+3]
            if len({a, b, c}) < 3:
                continue
            if i & 1:
                a, b = b, a
            rendered.append((a, b, c, mat))
    if p != len(mesh) or seen != total:
        raise ValueError("faggio: longitud de malla invalida")
    key = lambda tri: (tri[3], tuple(sorted(tri[:3])))
    original = collections.Counter(key(t) for t in geo["tris"] if len(set(t[:3])) == 3)
    if collections.Counter(map(key, rendered)) != original:
        raise ValueError("faggio: malla y triangulos no coinciden")
    selected = {key(geo["tris"][i]): side for side, ids in (("lr", left_ids), ("rr", right_ids)) for i in ids}
    if len(selected) != 16 or any(original[k] != 1 for k in selected):
        raise ValueError("faggio: caras ambiguas")
    parts = {"lr": [], "rr": [], "body": []}
    for tri in rendered:
        parts[selected.get(key(tri), "body")].append(tri)
    if len(parts["lr"]) != 8 or len(parts["rr"]) != 8:
        raise ValueError("faggio: seleccion incompleta")
    raw = parse_geometry(st)
    mats = [b for t, _v, b in chunks(ml, 0, len(ml)) if t == 0x07]
    single_mat = chunk(0x01, 0xC02FFFF, struct.pack("<Ii", 1, -1)) + chunk(0x07, 0xC02FFFF, mats[2])

    def new_geometry(tris, matlist, remap=False):
        tris = [(a, b, c, 0 if remap else m) for a, b, c, m in tris]
        extbody = b"".join(build_mesh(tris) if t == 0x50E else chunk(t, v, b) for t, v, b in extensions)
        return (chunk(0x01, stv, build_struct(raw, tris)) + chunk(0x08, mlv, matlist) +
                chunk(0x03, ev, extbody))

    raw_geos[gi] = new_geometry(parts["body"], ml)
    for side in ("lr", "rr"):
        raw_geos.append(new_geometry(parts[side], single_mat, True))
    fsv, fsbody = find(fc, 0x01)
    fstruct = bytearray(fsbody)
    numframes = struct.unpack_from("<I", fstruct)[0]
    if numframes != len(frames):
        raise ValueError("faggio: numero de marcos inesperado")
    for side in ("lr", "rr"):
        fstruct.extend(struct.pack("<9f3fiI", *(IDENTITY + (0.0, 0.0, 0.0, parent, 0))))
    struct.pack_into("<I", fstruct, 0, numframes + 2)
    newfb = b"".join(chunk(t, v, bytes(fstruct) if t == 1 else b) for t, v, b in fc)
    for side in ("lr", "rr"):
        newfb += chunk(0x03, 0xC02FFFF, chunk(0x253F2FE, 0xC02FFFF, ("indicator_" + side).encode("latin-1")))
    newgb = b""
    pos = 0
    for t, v, b in gc:
        if t == 1:
            b = struct.pack("<I", len(raw_geos))
        elif t == 0x0F:
            b = raw_geos[pos]
            pos += 1
        newgb += chunk(t, v, b)
    newgb += b"".join(chunk(0x0F, 0xC02FFFF, raw_geos[i]) for i in range(len(geoms), len(raw_geos)))
    atoms = [(v, b) for t, v, b in cc if t == 0x14]
    template = atoms[next(i for i, a in enumerate(atomics) if a == chassis)][1]
    atom_ext = find(chunks(template, 0, len(template)), 0x03)
    added = b""
    for i in range(2):
        body = chunk(0x01, 0xC02FFFF, struct.pack("<4I", numframes+i, len(geoms)+i, 5, 0))
        body += chunk(0x03, atom_ext[0], atom_ext[1])
        added += chunk(0x14, 0xC02FFFF, body)
    newcb = b""
    for t, v, b in cc:
        if t == 1:
            b = bytearray(b)
            struct.pack_into("<I", b, 0, len(atoms)+2)
            b = bytes(b)
        elif t == 0x0E:
            b = newfb
        elif t == 0x1A:
            b = newgb
        elif t == 0x03:
            newcb += added
            added = b""
        newcb += chunk(t, v, b)
    if added:
        raise ValueError("faggio: extension del clump no encontrada")
    return b"".join(chunk(t, v, newcb if t == 0x10 else b) for t, v, b in top)


# --------------------------------------------------------------------------
# METODO ANTIGUO (32.0), NO USAR: `faggio_split` y sus ayudantes generan a
# mano una lente de 6/4 escalada a la caja de las caras naranjas. El plan 33.0
# lo sustituyo por el chunk real del mod (`LENS_CHUNKS`, 9/8, material
# `lights`) que usan `lens_patch`/`--lens`. Se conserva solo como historial.
#
# Lente del mod: nodo indicator_* copiado de una lente buena de Vice Extended.
#
# Fuente: `admiral.dff` del vehicles.img del mod (GameFiles/modloader/...).
# Sus nodos son geometrias pequeñas con el material `lights` y el frame en el
# centro de la lente: 6 vertices / 4 triangulos detras y 9 / 8 delante. Es el
# patron que el plan 33.0 manda copiar en vez de partir la carroceria.
LENS_REAR_VERTS = ((0.08484691, -0.03344059, 0.07576309),
                   (-0.02882224, -0.01928544, 0.07649065),
                   (0.08484691, -0.04045820, -0.07661086),
                   (-0.02882224, -0.02802253, -0.07606675),
                   (-0.05546230, 0.06060362, 0.07649065),
                   (-0.05658716, 0.06060362, -0.07606674))
LENS_REAR_TRIS = ((0, 1, 2, 0), (2, 1, 3, 0), (1, 4, 3, 0), (3, 4, 5, 0))
LENS_FRONT_VERTS = ((-0.01801002, 0.00270009, -0.11449558),
                    (-0.06213409, -0.05674934, -0.05852652),
                    (-0.01801002, 0.00923204, 0.01206406),
                    (-0.06023985, -0.05425334, 0.01601088),
                    (-0.05467576, -0.05126333, 0.04716748),
                    (-0.01801002, -0.00091195, 0.09573945),
                    (0.07702661, 0.04826689, -0.11702888),
                    (0.07702661, 0.05528069, 0.01206403),
                    (0.07702672, 0.04769731, 0.10700510))
LENS_FRONT_TRIS = ((0, 1, 2, 0), (1, 3, 2, 0), (3, 4, 2, 0), (2, 4, 5, 0),
                   (6, 0, 7, 0), (0, 2, 7, 0), (7, 2, 8, 0), (2, 5, 8, 0))


def _lens_mesh(tris):
    """Malla 0x50E del nodo: un solo grupo, indices de 32 bits como el mod."""
    idx = [v for t in tris for v in t[:3]]
    body = struct.pack("<III", 0, 1, len(idx))
    body += struct.pack("<Ii", len(idx), 0)
    body += struct.pack("<%dI" % len(idx), *idx)
    return chunk(0x50E, 0x1003FFFF, body)


def _lens_struct(verts, tris, uvs, normal):
    """STRUCT de la lente con la version 0x1003ffff (sin surface props).

    El lector deduce los 12 bytes de props del tamano (plan 32.0); esta
    variante es la que usan los nodos reales del mod y cuadra exacta.
    """
    body = struct.pack("<Iiii", 0x00010076, len(tris), len(verts), 1)
    for u, v in uvs:
        body += struct.pack("<2f", u, v)
    for a, b, c, m in tris:
        body += struct.pack("<II", (a << 16) | b, (c << 16) | m)
    body += struct.pack("<4f", 0.0, 0.0, 0.0, 0.0)	 # bounding sphere
    body += struct.pack("<ii", 1, 1)			 # vertices y normales
    for x, y, z in verts:
        body += struct.pack("<3f", x, y, z)
    for _ in verts:
        body += struct.pack("<3f", *normal)
    return body


def _lens_geometry(verts, tris, uvs, normal, ml_ver, ml_struct_ver, mat_ver, mat_body):
    """Cuerpo de la geometria (sin la cabecera 0x0F, que pone la lista)."""
    matlist = chunk(0x01, ml_struct_ver, struct.pack("<Ii", 1, -1)) + \
        chunk(0x07, mat_ver, mat_body)
    return (chunk(0x01, 0x1003FFFF, _lens_struct(verts, tris, uvs, normal)) +
            chunk(0x08, ml_ver, matlist) +
            chunk(0x03, 0x1003FFFF, _lens_mesh(tris)))


def _lens_at_cluster(template_verts, template_tris, geo, tris_ids, normal):
    """Coloca la forma del mod sobre las caras naranjas reales del vehiculo.

    La caja de la lente sale de los vertices de esas caras y los UV del cuadro
    de pixeles naranjas que ya muestrean; no se inventa posicion ni tamano.
    """
    vs = sorted({v for i in tris_ids for v in geo["tris"][i][:3]})
    pts = [geo["verts"][v] for v in vs]
    mn = [min(p[a] for p in pts) for a in range(3)]
    mx = [max(p[a] for p in pts) for a in range(3)]
    uvs_src = [geo["uvs"][0][v] for v in vs]
    ub = (min(u for u, _v in uvs_src), max(u for u, _v in uvs_src),
          min(v for _u, v in uvs_src), max(v for _u, v in uvs_src))
    tmn = [min(p[a] for p in template_verts) for a in range(3)]
    tmx = [max(p[a] for p in template_verts) for a in range(3)]
    verts, uvs = [], []
    for p in template_verts:
        n = [(p[a] - tmn[a]) / (tmx[a] - tmn[a]) for a in range(3)]
        verts.append(tuple((n[a] - 0.5) * (mx[a] - mn[a]) for a in range(3)))
        uvs.append((ub[0] + n[0] * (ub[1] - ub[0]),
                    ub[2] + n[2] * (ub[3] - ub[2])))
    center = tuple((mn[a] + mx[a]) * 0.5 for a in range(3))
    return verts, uvs, center


def faggio_split(buf):
    """Añade los nodos indicator_lr/indicator_rr de la Faggio copiando la
    lente del mod, sin tocar la carroceria.

    El frame nuevo va en el centro de cada lente (matriz identidad) y la
    geometria es la del mod escalada a la caja de las caras naranjas. El
    material es el de esas mismas caras, asi que la lente usa la textura real
    de la moto y su sector ambar; no se inventa una textura nueva.
    """
    if hashlib.sha256(buf).hexdigest() != FAGGIO_SHA256:
        raise ValueError("faggio: modelo distinto al inspeccionado; no se modifica")
    frames, geoms, atomics = dg.parse(buf)
    chassis = next((fi, gi) for fi, gi in atomics
                   if frames[fi]["name"].lower() == "chassis_hi")
    fi, gi = chassis
    if not _is_identity_frame(frames[fi]):
        raise ValueError("faggio: el chasis no tiene matriz identidad")
    geo = geoms[gi]
    left_ids = (186, 187, 216, 409, 410, 411, 412, 413)
    right_ids = (414, 415, 416, 417, 418, 419, 420, 421)
    if any(geo["tris"][i][3] != 2 for i in left_ids + right_ids):
        raise ValueError("faggio: material de lente inesperado")

    top = chunks(buf, 0, len(buf))
    clump_ver, clump_body = find(top, 0x10)
    cc = chunks(clump_body, 0, len(clump_body))
    clump_struct = bytearray(find(cc, 0x01)[1])
    framelist_ver, framelist_body = find(cc, 0x0E)
    geomlist_ver, geomlist_body = find(cc, 0x1A)
    if geomlist_body is None:
        raise ValueError("faggio: sin lista de geometrias")

    fch = chunks(framelist_body, 0, len(framelist_body))
    fstruct = bytearray(find(fch, 0x01)[1])
    names = [b for t, _v, b in fch if t == 0x03]
    if struct.unpack_from("<I", fstruct, 0)[0] != len(names):
        raise ValueError("faggio: frame list y nombres no cuadran")

    gch = chunks(geomlist_body, 0, len(geomlist_body))
    gstruct_ver, gstruct = find(gch, 0x01)
    geoms_raw = [(v, b) for t, v, b in gch if t == 0x0F]
    if len(geoms_raw) != len(geoms):
        raise ValueError("faggio: lista de geometrias dividida")
    sub = chunks(geoms_raw[gi][1], 0, len(geoms_raw[gi][1]))
    ml_ver, ml_body = find(sub, 0x08)
    msub = chunks(ml_body, 0, len(ml_body))
    ml_struct_ver, _ = find(msub, 0x01)
    mats = [(v, b) for t, v, b in msub if t == 0x07]
    if len(mats) <= 2:
        raise ValueError("faggio: materiales insuficientes")
    mat_ver, mat_body = mats[2]

    def add_frame(name, parent, pos):
        idx = struct.unpack_from("<I", fstruct, 0)[0]
        struct.pack_into("<I", fstruct, 0, idx + 1)
        fstruct.extend(struct.pack("<9f3fiI", *(IDENTITY + (pos[0], pos[1], pos[2], parent, 0))))
        names.append(chunk(0x253F2FE, 0xC02FFFF, name.encode("latin-1")))
        return idx

    atomic_bodies = [b for t, _v, b in cc if t == 0x14]
    chassis_atomic = atomic_bodies[next(k for k, a in enumerate(atomics) if a == chassis)]
    atom_ext = find(chunks(chassis_atomic, 0, len(chassis_atomic)), 0x03)[1]
    new_geoms = list(geoms_raw)
    for name, ids in (("indicator_lr", left_ids), ("indicator_rr", right_ids)):
        verts, uvs, center = _lens_at_cluster(
            LENS_REAR_VERTS, LENS_REAR_TRIS, geo, ids, (0.0, -1.0, 0.0))
        new_geoms.append((0x1003FFFF, _lens_geometry(
            verts, LENS_REAR_TRIS, uvs, (0.0, -1.0, 0.0),
            ml_ver, ml_struct_ver, mat_ver, mat_body)))
        gidx = len(new_geoms) - 1
        frame = add_frame(name, fi, center)
        atomic_bodies.append(
            chunk(0x01, 0xC02FFFF, struct.pack("<4I", frame, gidx, 5, 0)) +
            chunk(0x03, 0xC02FFFF, atom_ext))
    struct.pack_into("<I", clump_struct, 0, len(atomic_bodies))

    new_framelist = chunk(0x01, 0xC02FFFF, bytes(fstruct)) + b"".join(
        chunk(0x03, 0xC02FFFF, n) for n in names)
    new_geomlist = chunk(0x01, gstruct_ver,
                         struct.pack("<I", len(new_geoms))) + b"".join(
        chunk(0x0F, v, b) for v, b in new_geoms)
    clump_ext = [b for t, _v, b in cc if t == 0x03]
    body = chunk(0x01, 0xC02FFFF, bytes(clump_struct))
    body += chunk(0x0E, framelist_ver, new_framelist)
    body += chunk(0x1A, geomlist_ver, new_geomlist)
    body += b"".join(chunk(0x14, 0xC02FFFF, a) for a in atomic_bodies)
    body += b"".join(chunk(0x03, 0xC02FFFF, b) for b in clump_ext)
    out = chunk(0x10, clump_ver, body)

    # La copia tiene que leerse entera y conservar los dos nodos nuevos.
    chk_frames, chk_geoms, chk_atomics = dg.parse(out)
    if len(chk_geoms) != len(geoms) + 2 or len(chk_frames) != len(frames) + 2:
        raise ValueError("faggio: la copia no conserva la estructura")
    nodes = {f["name"] for f in chk_frames if f["name"].startswith("indicator_")}
    if nodes != {"indicator_lr", "indicator_rr"}:
        raise ValueError("faggio: nodos nuevos mal escritos")
    # El DFF del juego va alineado a sector; el reempaquetado de la IMG lo
    # necesita y el fichero original ya traia ese relleno.
    if len(out) % 2048:
        out += b"\0" * (2048 - len(out) % 2048)
    return out


def stage_faggio(output):
    """Escribe una copia aislada, sin tocar los modelos servidos."""
    with open(os.path.join(BASE, "faggio.dff"), "rb") as f:
        out = lens_patch(f.read(), "faggio")
    with open(output, "xb") as f:
        f.write(out)
    print("faggio: piloto aislado en %s; %d bytes" % (output, len(out)))


def apply_faggio(dry=False):
    """Rehace los nodos de lente de la Faggio en las tres copias canonicas."""
    return lens_apply("faggio", dry=dry)


# --------------------------------------------------------------------------
# Lente del mod: nodo de 9 vertices / 8 triangulos con material `lights`.
#
# Los cuatro chunks se copiaron byte a byte del DFF del mod
# (`polwintergreen.dff`, nodos indicator_lr/rr/lf/rf): geometria 0x0F de 716
# bytes, STRUCT 0x1003FFFF (9 vertices / 8 triangulos, UV y normales del mod),
# material unico con textura `lights` y malla 0x50E de un solo grupo. No se
# generan a mano: se pega el chunk del mod, como manda el plan 33.0.
# --------------------------------------------------------------------------
LENS_CHUNKS = {
    ('rear', 'left'): "DwAAAMwCAAD//wMQAQAAAIgBAAD//wMQdgABAAgAAAAJAAAAAQAAAFGIBT+oEF0/NUTtPkz3az81RO0+VuVWPwEfwz4W92s/zHfPPoPdej81RO0+RIWAP1GIBT+D3Xo/s7QLPxb3az/Md88+qBBdPwEAAAAAAAIAAQADAAAABAABAAUAAAAGAAEABgAAAAcAAQAEAAAABQABAAIAAAAIAAEACAAAAAMAAQAHAAAAAACDkH6+UFOpv+a2JT2PPJg9AQAAAAEAAAAAlvi8AJWQO8CV9jwAAICyAD9pvAAwkToAAAAAAMuzO3iSLj3wyi89ADzpOgBwD7n4lfg8ALhfumDT+LwAAAAAALj8unC/L70Alvi8ALhfumDT+Lz4yi+9ADzpOgBwD7n4lfg8AJWQO8CV9jwnT32+am9pv9m7pz76r9+y0RB/v6jQrj2fzVYzojpnvyG42z5ueLM+Vdluv8vOpj0iS34+ikx0v3hKKr7X3dYyx4p2v0Tiib4hS36+ikx0v4BKKr5ueLO+V9luv8vOpj0pT30+am9pv9W7pz4IAAAAoAAAAP//AxABAAAACAAAAP//AxABAAAA/////wcAAACAAAAA//8DEAEAAAAcAAAA//8DEAAAAAD/////AQAAAAEAAAAAAIA/AAAAAAAAgD8GAAAAQAAAAP//AxABAAAABAAAAP//AxAAAAAAAgAAAAgAAAD//wMQbGlnaHRzAAACAAAABAAAAP//AxAAAAAAAwAAAAAAAAD//wMQAwAAAAAAAAD//wMQAwAAAIAAAAD//wMQDgUAAHQAAAD//wMQAAAAAAEAAAAYAAAAGAAAAAAAAAAAAAAAAQAAAAIAAAADAAAAAQAAAAQAAAAFAAAAAQAAAAYAAAAGAAAAAQAAAAcAAAAEAAAAAQAAAAUAAAACAAAAAQAAAAgAAAAIAAAAAQAAAAMAAAAHAAAAAQAAAAAAAAA=",
    ('rear', 'right'): "DwAAAMwCAAD//wMQAQAAAIgBAAD//wMQdgABAAgAAAAJAAAAAQAAAFGIBT+oEF0/NUTtPlblVj81RO0+TPdrPwEfwz4W92s/zHfPPoPdej81RO0+RIWAP1GIBT+D3Xo/s7QLPxb3az/Md88+qBBdPwEAAAAAAAIABAADAAAAAgAGAAUAAAACAAcABgAAAAIABQAEAAAAAgAIAAEAAAACAAMACAAAAAIAAAAHAAAAAgCDkH4+UFOpv+a2JT2PPJg9AQAAAAEAAAAAlvg8AJWQO8CV9jwAAAAAAMuzO3iSLj0AAIAyAD9pvAAwkTrwyi+9ADzpOgBwD7n4lfi8ALhfumDT+LwAAAAAALj8unC/L70Alvg8ALhfumDT+Lz4yi89ADzpOgBwD7n4lfi8AJWQO8CV9jwnT30+am9pv9m7pz6fzVazojpnvyG42z63Ro0yzxB/v6bQrj1ueLO+Vdluv8vOpj0iS36+ikx0v3hKKr7X3dayx4p2v0Tiib4hS34+ikx0v4BKKr5ueLM+V9luv8vOpj0pT32+am9pv9W7pz4IAAAAoAAAAP//AxABAAAACAAAAP//AxABAAAA/////wcAAACAAAAA//8DEAEAAAAcAAAA//8DEAAAAAD/////AQAAAAEAAAAAAIA/AAAAAAAAgD8GAAAAQAAAAP//AxABAAAABAAAAP//AxAAAAAAAgAAAAgAAAD//wMQbGlnaHRzAAACAAAABAAAAP//AxAAAAAAAwAAAAAAAAD//wMQAwAAAAAAAAD//wMQAwAAAIAAAAD//wMQDgUAAHQAAAD//wMQAAAAAAEAAAAYAAAAGAAAAAAAAAAAAAAAAQAAAAIAAAADAAAABAAAAAIAAAAFAAAABgAAAAIAAAAGAAAABwAAAAIAAAAEAAAABQAAAAIAAAABAAAACAAAAAIAAAAIAAAAAwAAAAIAAAAHAAAAAAAAAAIAAAA=",
    ('front', 'left'): "DwAAAMwCAAD//wMQAQAAAIgBAAD//wMQdgABAAgAAAAJAAAAAQAAAFGIBT+oEF0/NUTtPkz3az81RO0+VuVWPwEfwz4W92s/zHfPPoPdej81RO0+RIWAP1GIBT+D3Xo/s7QLPxb3az/Md88+qBBdPwEAAAAAAAIAAQADAAAABAABAAUAAAAGAAEABgAAAAcAAQAEAAAABQABAAIAAAAIAAEACAAAAAMAAQAHAAAAAADA+F++fPNzP+zPwj6NPJg9AQAAAAEAAAAAlvg8wJWQu8iV9jwAAIAygD5pPAAskToAAAAAgMyzu4SSLj30yi+9AEDpugBsD7n4lfi8ALBfOmjT+LwAAAAAALP8Omy/L70Alvg8ALBfOmjT+Lzwyi89AEDpugBsD7n4lfi8wJWQu8iV9jy9Tn0+dG9pP8q7pz6P+QI00hB/PwHQrj1pp3GzpDpnPxe42z5ieLO+WdluP6jOpj0aS36+iEx0P6FKKr7N3VaywYp2P3Liib4hS34+iEx0P59KKr5ueLM+WNluP6bOpj24Tn2+dW9pP827pz4IAAAAoAAAAP//AxABAAAACAAAAP//AxABAAAA/////wcAAACAAAAA//8DEAEAAAAcAAAA//8DEAAAAAD/////AQAAAAEAAAAAAIA/AAAAAAAAgD8GAAAAQAAAAP//AxABAAAABAAAAP//AxAAAAAAAgAAAAgAAAD//wMQbGlnaHRzAAACAAAABAAAAP//AxAAAAAAAwAAAAAAAAD//wMQAwAAAAAAAAD//wMQAwAAAIAAAAD//wMQDgUAAHQAAAD//wMQAAAAAAEAAAAYAAAAGAAAAAAAAAAAAAAAAQAAAAIAAAADAAAAAQAAAAQAAAAFAAAAAQAAAAYAAAAGAAAAAQAAAAcAAAAEAAAAAQAAAAUAAAACAAAAAQAAAAgAAAAIAAAAAQAAAAMAAAAHAAAAAQAAAAAAAAA=",
    ('front', 'right'): "DwAAAMwCAAD//wMQAQAAAIgBAAD//wMQdgABAAgAAAAJAAAAAQAAAFGIBT+oEF0/NUTtPlblVj81RO0+TPdrPwEfwz4W92s/zHfPPoPdej81RO0+RIWAP1GIBT+D3Xo/s7QLPxb3az/Md88+qBBdPwEAAAAAAAIABAADAAAAAgAGAAUAAAACAAcABgAAAAIABQAEAAAAAgAIAAEAAAACAAMACAAAAAIAAAAHAAAAAgDA+F8+fPNzP+zPwj6NPJg9AQAAAAEAAAAAlvi8wJWQu8iV9jwAAAAAgMyzu4SSLj0AAICygD5pPAAskTr0yi89AEDpugBsD7n4lfg8ALBfOmjT+LwAAAAAALP8Omy/L70Alvi8ALBfOmjT+Lzwyi+9AEDpugBsD7n4lfg8wJWQu8iV9jy9Tn2+dG9pP8q7pz5pp3EzpDpnPxe42z6/O/ez0hB/PwDQrj1ieLM+WdluP6jOpj0aS34+iEx0P6FKKr7N3VYywYp2P3Liib4hS36+iEx0P59KKr5ueLO+WNluP6bOpj24Tn0+dW9pP827pz4IAAAAoAAAAP//AxABAAAACAAAAP//AxABAAAA/////wcAAACAAAAA//8DEAEAAAAcAAAA//8DEAAAAAD/////AQAAAAEAAAAAAIA/AAAAAAAAgD8GAAAAQAAAAP//AxABAAAABAAAAP//AxAAAAAAAgAAAAgAAAD//wMQbGlnaHRzAAACAAAABAAAAP//AxAAAAAAAwAAAAAAAAD//wMQAwAAAAAAAAD//wMQAwAAAIAAAAD//wMQDgUAAHQAAAD//wMQAAAAAAEAAAAYAAAAGAAAAAAAAAAAAAAAAQAAAAIAAAADAAAABAAAAAIAAAAFAAAABgAAAAIAAAAGAAAABwAAAAIAAAAEAAAABQAAAAIAAAABAAAACAAAAAIAAAAIAAAAAwAAAAIAAAAHAAAAAAAAAAIAAAA=",
}
LENS_GEOM_VER = 0x1003FFFF


def _lens_body(where, side):
    """Cuerpo de la geometria (sin la cabecera 0x0F) del chunk del mod."""
    return base64.b64decode(LENS_CHUNKS[(where, side)])[12:]


def _is_mod_lens(geo):
    """Nodo ya hecho como el mod: lente pequena con material `lights`."""
    if len(geo['verts']) > 24:
        return False
    return any((t or '').lower() == 'lights' for _c, t in geo['mats'])


def _frame_pos(fstruct, fi):
    return struct.unpack_from('<3f', fstruct, 4 + fi * 56 + 36)


def _set_frame_pos(fstruct, fi, pos):
    struct.pack_into('<3f', fstruct, 4 + fi * 56 + 36, pos[0], pos[1], pos[2])


def _inv_world(m):
    """Inversa de la 3x4 de dg.world_of (rotacion ortonormal + traslacion)."""
    r = [[m[k][a] for k in range(3)] for a in range(3)]      # R^T
    t = (m[0][3], m[1][3], m[2][3])
    ti = [-sum(r[a][k] * t[k] for k in range(3)) for a in range(3)]
    return r, ti


def _to_local(m, p):
    r, ti = _inv_world(m)
    return [sum(r[a][k] * p[k] for k in range(3)) + ti[a] for a in range(3)]


def lens_patch(buf, model, crear_faltantes=False):
    """Rehace los nodos indicator_* con la lente del mod.

    `crear_faltantes` (plan 56.0 T7): ademas de rehacer los que existen, anade
    los huecos que el modelo no tiene (fbiranch, esperant y fbicar no tienen
    `indicator_lr/rr` y su lente trasera esta medida). Es opcion explicita por
    modelo porque la creacion automatica solo se disparaba cuando el modelo no
    tenia NINGUN nodo; sin opt-in ningun otro modelo cambia de comportamiento.

    Los nodos viejos (geometria clonada de la carroceria o cualquier lente que
    no sea la del mod) conservan su sitio: se les cambia la geometria por el
    chunk de 9/8 con material `lights` y, si el nodo no estaba centrado en la
    lente, se mueve su frame al centro de las caras que tenia.

    Si el modelo no tiene ningun nodo indicator_*, se anaden sobre los racimos
    naranjas reales del modelo (mismo criterio que `split_model`).
    """
    frames, geoms, atomics = dg.parse(buf)
    top = chunks(buf, 0, len(buf))
    clump_ver, clump_body = find(top, 0x10)
    cc = chunks(clump_body, 0, len(clump_body))
    clump_struct = bytearray(find(cc, 0x01)[1])
    framelist_ver, framelist_body = find(cc, 0x0E)
    geomlist_ver, geomlist_body = find(cc, 0x1A)
    if geomlist_body is None:
        raise SystemExit("%s: sin lista de geometrias" % model)
    fch = chunks(framelist_body, 0, len(framelist_body))
    fstruct = bytearray(find(fch, 0x01)[1])
    names = [b for t, _v, b in fch if t == 0x03]
    if struct.unpack_from('<I', fstruct, 0)[0] != len(names):
        raise SystemExit("%s: frame list y nombres no cuadran" % model)
    gch = chunks(geomlist_body, 0, len(geomlist_body))
    gstruct_ver, gstruct = find(gch, 0x01)
    raw_geoms = [(v, b) for t, v, b in gch if t == 0x0F]
    if len(raw_geoms) != len(geoms):
        raise SystemExit("%s: lista de geometrias dividida" % model)

    new_geoms = list(raw_geoms)
    atomic_bodies = [b for t, _v, b in cc if t == 0x14]
    if len(atomic_bodies) != len(atomics):
        raise SystemExit("%s: atomics y lista no cuadran" % model)

    replaced = []
    for fi, gi in atomics:
        name = frames[fi]['name'].lower()
        if name not in INDICATOR_NODES or _is_mod_lens(geoms[gi]):
            continue
        where, side = INDICATOR_NODES[name]
        ids = {v for tri in geoms[gi]['tris'] for v in tri[:3]}
        if not ids:
            continue
        pts = [geoms[gi]['verts'][v] for v in ids]
        c = [(min(p[i] for p in pts) + max(p[i] for p in pts)) / 2.0
             for i in range(3)]
        # Una lente ya centrada en su frame (la que puso 32.0/33.0) no se
        # mueve: su caja ya esta en el origen. Solo se recoloca el nodo viejo
        # que clonaba la carroceria y lleva la lente en coordenadas del chasis.
        if sum(abs(x) for x in c) > 0.05:
            p = _frame_pos(fstruct, fi)
            _set_frame_pos(fstruct, fi, (p[0] + c[0], p[1] + c[1], p[2] + c[2]))
        new_geoms[gi] = (LENS_GEOM_VER, _lens_body(where, side))
        replaced.append(name)

    # Los nodos viejos se habian llevado las caras de la lente: devolverlas a
    # su geometria antes de sustituir el nodo (si no, queda un hueco).
    for verts, tris, key, _src in _clone_specs(frames, geoms, atomics):
        _restore_into(new_geoms, geoms, frames, atomics, verts, tris, key)

    added = []
    have = {frames[fi]['name'].lower() for fi, _gi in atomics}
    if not have & set(INDICATOR_NODES) or crear_faltantes:
        keep, lo, hi = _candidate_lenses(model, frames, geoms, atomics)
        largo = max(hi[1] - lo[1], 1.0)
        used = {f['name'] for f in frames}
        for cl in sorted(keep, key=lambda c: (-(c['cy'] - lo[1]) / largo, c['cx'])):
            where = 'front' if (cl['cy'] - lo[1]) / largo > 0.5 else 'rear'
            side = 'left' if cl['cx'] < 0 else 'right'
            if crear_faltantes:
                # Con opt-in no se duplica un lado que ya tiene su nodo: se rehizo
                # arriba. `_free_node` devolveria `indicator2_lf` y el motor
                # pintaria dos lentes en el mismo sitio.
                name = "indicator_%s%s" % ("l" if side == "left" else "r",
                                           "f" if where == "front" else "r")
                if name in have:
                    continue
            else:
                name = _free_node(where, side, used)
            if name is None:
                continue
            used.add(name)
            local = _to_local(dg.world_of(frames, cl['fi']),
                              [cl['cx'], cl['cy'], cl['cz']])
            idx = struct.unpack_from('<I', fstruct, 0)[0]
            struct.pack_into('<I', fstruct, 0, idx + 1)
            fstruct.extend(struct.pack('<9f3fiI', *(IDENTITY + (
                local[0], local[1], local[2], cl['fi'], 0))))
            names.append(chunk(0x253F2FE, 0xC02FFFF, name.encode('latin-1')))
            gidx = len(new_geoms)
            new_geoms.append((LENS_GEOM_VER, _lens_body(where, side)))
            src = None
            for i, a in enumerate(atomics):
                if a == (cl['fi'], cl['gi']):
                    src = atomic_bodies[i]
                    break
            ext = find(chunks(src, 0, len(src)), 0x03) if src else (None, None)
            atomic_bodies.append(
                chunk(0x01, 0xC02FFFF, struct.pack('<4I', idx, gidx, 5, 0)) +
                (chunk(0x03, ext[0], ext[1]) if ext[1] is not None else b''))
            added.append(name)

    if not replaced and not added:
        print("%s: sin cambios (todos los nodos ya son la lente del mod)" % model)
        return buf

    struct.pack_into('<I', clump_struct, 0, len(atomic_bodies))
    new_framelist = chunk(0x01, 0xC02FFFF, bytes(fstruct)) + b''.join(
        chunk(0x03, 0xC02FFFF, n) for n in names)
    new_geomlist = chunk(0x01, gstruct_ver,
                         struct.pack('<I', len(new_geoms))) + b''.join(
        chunk(0x0F, v, b) for v, b in new_geoms)
    clump_ext = [b for t, _v, b in cc if t == 0x03]
    body = chunk(0x01, 0xC02FFFF, bytes(clump_struct))
    body += chunk(0x0E, framelist_ver, new_framelist)
    body += chunk(0x1A, geomlist_ver, new_geomlist)
    body += b''.join(chunk(0x14, 0xC02FFFF, a) for a in atomic_bodies)
    body += b''.join(chunk(0x03, 0xC02FFFF, b) for b in clump_ext)
    out = chunk(0x10, clump_ver, body)

    # La copia tiene que leerse entera y las lentes recien puestas tienen que
    # ser exactamente el chunk del mod: 9 vertices, 8 triangulos y `lights`.
    chk_frames, chk_geoms, chk_atomics = dg.parse(out)
    if len(chk_frames) != len(frames) + len(added):
        raise SystemExit("%s: frames nuevos mal escritos" % model)
    targets = set(replaced) | set(added)
    for fi, gi in chk_atomics:
        n = chk_frames[fi]['name'].lower()
        if n not in targets:
            continue
        if len(chk_geoms[gi]['verts']) != 9 or len(chk_geoms[gi]['tris']) != 8:
            raise SystemExit("%s: %s no tiene la lente 9/8" % (model, n))
        if not any((t or '').lower() == 'lights' for _c, t in chk_geoms[gi]['mats']):
            raise SystemExit("%s: %s sin material lights" % (model, n))

    resumen = []
    if replaced:
        resumen.append("rehechos " + ", ".join(sorted(set(replaced))))
    if added:
        resumen.append("anadidos " + ", ".join(added))
    print("%s: %s -> %d bytes" % (model, "; ".join(resumen), len(out)))
    return out


def _same_verts(a, b):
    if len(a) != len(b) or not a:
        return False
    for va, vb in zip(a, b):
        if abs(va[0] - vb[0]) > 1e-6 or abs(va[1] - vb[1]) > 1e-6 or \
           abs(va[2] - vb[2]) > 1e-6:
            return False
    return True


def _material_key(mats):
    if not mats:
        return None
    col, tex = mats[0]
    return ((tex or '').lower(), tuple(col))


def _material_index(mats, key):
    if key is None:
        return 0
    tex, _col = key
    for i, (_c, t) in enumerate(mats):
        if (t or '').lower() == tex:
            return i
    return 0


def _clone_specs(frames, geoms, atomics):
    """Nodos indicator que clonaron una geometria: (verts, tris, mat, src_gi).

    Solo esos nodos dejaron un hueco en su geometria origen cuando se crearon
    (plan 28.0): el nodo se llevo las caras de la lente y la carroceria se
    quedo sin ellas. Una lente ex-profeso (la Faggio de 32.0) no coincide con
    ninguna geometria y no se restaura: su carroceria nunca perdio las caras.
    """
    out = []
    for fi, gi in atomics:
        if frames[fi]['name'].lower() not in INDICATOR_NODES:
            continue
        g = geoms[gi]
        if not g['tris']:
            continue
        for fj, gj in atomics:
            if gj != gi and _same_verts(geoms[gj]['verts'], g['verts']):
                out.append((g['verts'], g['tris'], _material_key(g['mats']), gj))
                break
    return out


def _replace_geomlist(buf, new_geoms):
    """Reescribe solo la lista de geometrias (frames/atomics intactos)."""
    top = chunks(buf, 0, len(buf))
    clump_ver, clump_body = find(top, 0x10)
    cc = chunks(clump_body, 0, len(clump_body))
    geomlist_ver, geomlist_body = find(cc, 0x1A)
    gch = chunks(geomlist_body, 0, len(geomlist_body))
    gstruct_ver, gstruct = find(gch, 0x01)
    new_gb = chunk(0x01, gstruct_ver,
                   struct.pack('<I', len(new_geoms))) + b''.join(
        chunk(0x0F, v, b) for v, b in new_geoms)
    body = b''.join(chunk(t, v, new_gb if t == 0x1A else b)
                    for t, v, b in cc)
    return chunk(0x10, clump_ver, body)


def _restore_into(new_geoms, cur_geoms, cur_frames, cur_atomics,
                  verts, tris, mat_key):
    """Devuelve las caras del nodo a su geometria origen, si le faltan."""
    src = None
    for fj, gj in cur_atomics:
        if _same_verts(cur_geoms[gj]['verts'], verts):
            src = gj
            break
    if src is None:
        return 0
    gch = chunks(new_geoms[src][1], 0, len(new_geoms[src][1]))
    stv, stb = find(gch, 0x01)
    mlv, mlb = find(gch, 0x08)
    if stb is None or mlb is None:
        return 0
    geo = parse_geometry(stb)
    have = {tuple(sorted(t[:3])) for t in geo['tris']}
    idx = _material_index(cur_geoms[src]['mats'], mat_key)
    extra = [(a, b, c, idx) for (a, b, c, _m) in tris
             if tuple(sorted((a, b, c))) not in have]
    if not extra:
        return 0
    new_geoms[src] = (new_geoms[src][0],
                      build_geometry(stv, mlv, geo, list(geo['tris']) + extra, mlb))
    return len(extra)


def restore_faces(model, backup_dir, dry=False):
    """Devuelve a la carroceria las caras de lente que se llevaron los nodos.

    Los nodos viejos de `split_model` clonaban la geometria origen y borraban
    de ella las caras de la lente. Al sustituirlos por la lente del mod queda
    un hueco del tamano del intermitente viejo. Esta pasada lee los nodos del
    backup previo (que aun los conservan) y reinserta esas caras en su
    geometria origen; si ya estan (Faggio) no toca nada.
    """
    bak_path = os.path.join(backup_dir, model + '.dff')
    if not os.path.exists(bak_path):
        raise SystemExit('%s: no hay backup %s' % (model, bak_path))
    bframes, bgeoms, batomics = dg.parse(open(bak_path, 'rb').read())
    specs = _clone_specs(bframes, bgeoms, batomics)
    if not specs:
        print('%s: el backup no tiene nodos clonados' % model)
        return 0

    path = os.path.join(BASE, model + '.dff')
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dg.parse(buf)
    top = chunks(buf, 0, len(buf))
    _cv, cb = find(top, 0x10)
    _gv, gb = find(chunks(cb, 0, len(cb)), 0x1A)
    new_geoms = [(v, b) for t, v, b in chunks(gb, 0, len(gb)) if t == 0x0F]
    if len(new_geoms) != len(geoms):
        raise SystemExit('%s: lista de geometrias dividida' % model)

    total = 0
    for verts, tris, key, _src in specs:
        total += _restore_into(new_geoms, geoms, frames, atomics, verts, tris, key)
    if not total:
        print('%s: la carroceria ya tiene las caras de la lente' % model)
        return 0
    out = _replace_geomlist(buf, new_geoms)
    if len(out) % 2048:
        out += b'\0' * (2048 - len(out) % 2048)
    if dry:
        print('%s: (dry) restauraria %d caras -> %d bytes' % (model, total, len(out)))
        return total
    for d in (os.path.join(GAMEFILES, 'gta3.img'), MODELS,
              os.path.join(BOOTSEED, 'gta3.img')):
        target = os.path.join(d, model + '.dff')
        if not os.path.exists(target):
            continue
        tmp = target + '.tmp'
        with open(tmp, 'wb') as f:
            f.write(out)
        os.replace(tmp, target)
    print('%s: restauradas %d caras de lente en la carroceria -> %d bytes'
          % (model, total, len(out)))
    return total


def lens_apply(model, dry=False, crear_faltantes=False):
    """Aplica `lens_patch` a las tres copias canonicas del modelo."""
    path = os.path.join(BASE, model + '.dff')
    with open(path, 'rb') as f:
        buf = f.read()
    out = lens_patch(buf, model, crear_faltantes=crear_faltantes)
    if out is buf or out == buf:
        return 0
    # La IMG se lee por sectores: el DFF va alineado a 2048, como los
    # originales. `repack_dir` calcula las entradas con este tamano.
    if len(out) % 2048:
        out += b'\0' * (2048 - len(out) % 2048)
    if dry:
        print("%s: (dry) %d -> %d bytes" % (model, len(buf), len(out)))
        return 1
    for d in (os.path.join(GAMEFILES, 'gta3.img'), MODELS,
              os.path.join(BOOTSEED, 'gta3.img')):
        target = os.path.join(d, model + '.dff')
        if not os.path.exists(target):
            continue
        tmp = target + '.tmp'
        with open(tmp, 'wb') as f:
            f.write(out)
        os.replace(tmp, target)
    return 1


# --------------------------------------------------------------------------
# Plan 41.0: nodos con las caras reales del coche (lente propia o prestada).
#
# Via A: caras ambar del extremo, separadas de la carroceria.
# Via B: caras del piloto (rojo) o del faro (claro) alrededor de los dummies
#        `taillights`/`headlights`; el nodo se enciende en su color original.
# Nunca se crea geometria: se mueven caras que ya existen.
#
# No hay vetos por modelo: el ambar se decide por hue, asi que un piloto rojo
# (hue 8..19, p.ej. el hermes) nunca reclama la Via A y cae en su rojo propio.
#
# Motos con lente ambar real detras: el filtro de coches no las juzga (la
# carlinga no tiene zrel) y se buscan las caras por posicion.
BIKE_LENSES = {"faggio", "pizzaboy"}


def _whole_islands(out):
    """Regla 5 (enmendada 03/11): la lente ambar real se queda tambien si es UNA cara.

    Antes la isla de una cara se descartaba y la celda caia al color propio, que
    en la practica es el faro blanco del modelo: el nodo acababa parpadeando en el
    morro equivocado. Medido sobre `tmp/base421` (`C:\\tmp\\iav42\\islas421.py`):
    cambian 30 celdas de 460, 27 quedan en una sola cara, en 14 modelos; el caso
    tipo es `rumpo`/`pony` delante, donde la unica cara ambar real del modelo
    (164,85,24 en x +/0.96, y +2.55) se tiraba y el nodo se reconstruia con 3 y 4
    caras blancas pedidas al faro.

    Lo que sigue prohibido es vaciar medio quad: los gemelos los trae
    `_close_twins` por arista completa dentro del mismo color, y una celda sin
    caras no se escribe.

    La admision de una sola cara no es libre: `_veto_paint_islands` exige despues
    el blob del atlas y el borde exterior del coche, y descarta la cara cuyo
    centroide mintio. Sin ese veto el relajo se llevaba la pintura naranja del
    mrwhoop (raya de 32x9 texeles en el atlas del cuerpo) y las costuras de
    antialiasing de washing/linerun/topfun detras.
    """
    return {k: v for k, v in
            ((k, [(fi, gi, mi, ts) for (fi, gi, mi), ts in b.items() if ts])
             for k, b in out.items()) if v}
# --------------------------------------------------------------------------


def _close_twins(g, mi, tris, color_ok=None):
    """Trae al nodo los gemelos que comparten arista completa con el.

    El corte de la textura parte quads por la mitad: un gemelo da ambar/rojo
    en su centro UV y el otro no. La arista compartida (2 vertices) los une
    aunque el color los separe. Con color_ok, el gemelo solo entra si pasa
    ese filtro (evita arrastrar cromado vecino en la via B delantera).
    """
    want = set(id(t) for t in tris)
    by_edge = {}
    for t in g["tris"]:
        if t[3] != mi:
            continue
        vs = t[:3]
        for a, b in ((vs[0], vs[1]), (vs[1], vs[2]), (vs[2], vs[0])):
            by_edge.setdefault((a, b) if a < b else (b, a), []).append(t)
    frontier = list(tris)
    cache = {}
    while frontier:
        t = frontier.pop()
        vs = t[:3]
        for a, b in ((vs[0], vs[1]), (vs[1], vs[2]), (vs[2], vs[0])):
            for o in by_edge.get((a, b) if a < b else (b, a), ()):
                if id(o) not in want:
                    if color_ok is not None and not color_ok(o):
                        continue
                    want.add(id(o))
                    frontier.append(o)
    return [t for t in g["tris"] if id(t) in want]


def _is_red(c):
    r, g, b, a = c
    return a > 0 and r >= 90 and g <= 70 and b <= 70 and r - g >= 50


def _is_light(c):
    r, g, b, a = c
    if a == 0:
        return False
    hi, lo = max(r, g, b), min(r, g, b)
    return lo >= 150 and hi - lo <= 60


def _dummy_world(frames, name):
    for f in frames:
        if f["name"].lower() == name:
            m = dg.world_of(frames, f["i"])
            return (m[0][3], m[1][3], m[2][3])
    return None


def _face_color(g, pix, tri, cache):
    mi = tri[3]
    if mi >= len(g["mats"]) or not g["uvs"]:
        return None
    key = (g["mats"][mi][1] or "").lower()
    if not key or key not in pix:
        return None
    if key not in cache:
        cache[key] = pix[key]
    w, h, px = cache[key]
    uv = g["uvs"][0]
    us = [uv[v][0] for v in tri[:3]]
    vs = [uv[v][1] for v in tri[:3]]
    if any(x != x or x in (float("inf"), float("-inf")) for x in us + vs):
        return None
    x = int((sum(us) / 3.0 % 1.0) * w) % w
    y = int((sum(vs) / 3.0 % 1.0) * h) % h
    return px[y * w + x]


def _face_texel(g, pix, tri, cache):
    """(texel_x, texel_y, w, h, px) del centroide UV de la cara, o None."""
    mi = tri[3]
    if mi >= len(g["mats"]) or not g["uvs"]:
        return None
    key = (g["mats"][mi][1] or "").lower()
    if not key or key not in pix:
        return None
    if key not in cache:
        cache[key] = pix[key]
    w, h, px = cache[key]
    uv = g["uvs"][0]
    us = [uv[v][0] for v in tri[:3]]
    vs = [uv[v][1] for v in tri[:3]]
    if any(x != x or x in (float("inf"), float("-inf")) for x in us + vs):
        return None
    return int((sum(us) / 3.0 % 1.0) * w) % w, int((sum(vs) / 3.0 % 1.0) * h) % h, w, h, px


def _uv_tri(g, tri):
    """UVs del triangulo con la costura del atlas corregida y recortada.

    Varias caras del parque tienen un vertice en u=1,003 y los otros en 0,02:
    el baricentro cae fuera del atlas y la medida lee la franja equivocada. Se
    envuelve el eje discrepante (medido cara a cara en el plan 45.0: es el caso
    del stretch `indicator_rf`, cuya cara de 114,2 cm2 perdio el ambar por leer
    vecina, y del regina/packer que salian en la banda de duda).
    """
    uv = g["uvs"][0]
    pts = [[uv[v][0], uv[v][1]] for v in tri[:3]]
    for k in (0, 1):
        lo = min(p[k] for p in pts)
        hi = max(p[k] for p in pts)
        if hi - lo > 0.5:
            shift = 0.5 if k else 0.0
            for p in pts:
                p[k] = (p[k] + shift) % 1.0 - shift
    # Plan 56.0 T1 (el de verdad): las UVs se envuelven SOLO cuando el triangulo
    # entero cae fuera de [0,1). Aqui se recortaba con min/max a 0.9999, y eso
    # aplasta contra la ultima fila del atlas las caras cuyas tres coordenadas
    # estan por encima de 1,000: medido en fbiranch (`C:\tmp\iav42\px0_debug.py`)
    # 1193 caras devolvian px=0 y las de su lente trasera (`bump_rear_hi_ok` mi=3,
    # v=1,00..1,05) quedaban con los tres vertices en v=0,9999, area UV 0,
    # `_face_render` None y color de respaldo (255,255,255). La GPU envuelve esas
    # texturas, asi que la texela real es v-1.
    #
    # NO se envuelve un triangulo que CRUZA el borde (un vertice en 1,003 y otro
    # en 0,02): eso es costura, lo resuelve el bloque de arriba, y envolverlo
    # convertira 1,0 en 0,0 fabricando un triangulo que abarca el atlas entero.
    # Medido con ese error: el stretch perdia sus dos celdas delanteras (la cara
    # de 114,2 cm2 que el plan 45.0 rescato justamente de la costura).
    def envuelve(v):
        return v % 1.0
    for k in (0, 1):
        lo = min(p[k] for p in pts)
        hi = max(p[k] for p in pts)
        if lo >= 1.0 or hi < 0.0:
            for p in pts:
                p[k] = envuelve(p[k])
    return [(min(max(p[0], 0.0), 0.9999), min(max(p[1], 0.0), 0.9999)) for p in pts]


def _poly_clip_half(pols, nx, ny, c):
    """Recorta los poligonos contra el semiplano nx*x + ny*y <= c."""
    out = []
    for poly in pols:
        keep = []
        for i in range(len(poly)):
            ax, ay = poly[i]
            bxi, byi = poly[(i + 1) % len(poly)]
            da = nx * ax + ny * ay - c
            db = nx * bxi + ny * byi - c
            if da <= 0.0:
                keep.append((ax, ay))
            if (da < 0.0 < db) or (da > 0.0 > db):
                t = da / (da - db)
                keep.append((ax + t * (bxi - ax), ay + t * (byi - ay)))
        if len(keep) >= 3:
            out.append(keep)
    return out


def _poly_area(poly):
    s = 0.0
    for i in range(len(poly)):
        ax, ay = poly[i]
        bxi, byi = poly[(i + 1) % len(poly)]
        s += ax * byi - bxi * ay
    return abs(s) / 2.0


def _cell_cover(p0, p1, p2, tx, ty):
    """Area del triangulo UV que cae DENTRO de la texela (tx, ty), en texelas.

    Plan 56.0 T1c: es el «y su peso» que el plan pedia y el T1 no implemento.
    Contar la texela una vez si su CENTRO cae dentro del triangulo (lo que hizo
    el T1, con supermuestreo de relleno) no es una medida de area: una cara que
    cruza una texela por la mitad cuenta entera o no cuenta, segun donde este el
    centro. Medido sobre la tabla servida del plan 51.0: el racimo delantero
    izquierdo del sandking (`indicator_lf`, 2 caras, xyz -83,+254,-52) pasa de
    frac 0,502 a 0,4947 con el conteo por centros y pierde su fila ambar por
    media centesima, cuando lo que el jugador ve desde el plan 45.0 es una lente
    ambar encendida. Son dos caras a medias lente y a medias moldura: el umbral
    del 50 % no se puede decidir con una cuenta cuantizada de a una texela.
    """
    if min(p0[0], p1[0], p2[0]) >= tx and max(p0[0], p1[0], p2[0]) < tx + 1.0 \
            and min(p0[1], p1[1], p2[1]) >= ty and max(p0[1], p1[1], p2[1]) < ty + 1.0:
        # El triangulo entero cae dentro de la celda: peso = su area, sin recorte.
        # Es el caso comun (una texela pequena dentro de la lente) y evita los
        # cuatro cortes de Sutherland-Hodgman por cara.
        return _poly_area([p0, p1, p2])
    pols = [[p0, p1, p2]]
    for nx, ny, c in ((-1.0, 0.0, -float(tx)), (1.0, 0.0, float(tx + 1)),
                      (0.0, -1.0, -float(ty)), (0.0, 1.0, float(ty + 1))):
        pols = _poly_clip_half(pols, nx, ny, c)
        if not pols:
            return 0.0
    return sum(_poly_area(p) for p in pols)


def _face_render(g, pix, tri, cache):
    """(texelos, color renderido, texelos ambar, color de los texelos ambar).

    Las dos ultimas cuentas son de la cara ENTERA.

    El color que ve el shader es `texura x material`, y `_face_color` mira un
    solo texel (el centroide UV). Con esa medida el Washington trasero sale rojo
    (piloto rojo + lente ambar en el mismo nodo, la media del nodo manda) y una
    cara grande de carrocería cuyo centroide recae en una raya naranja pasa por
    lente. Aqui se rasteriza el triangulo sobre el atlas y cada texel se
    multiplica por el color de su material: `n` texelos validos, la media
    renderida y cuantos de esos texelos pasan `_is_amber`. None si la cara no
    tiene textura medible.

    No entra el vertex-colour prelight: en los DFF servidos del parque el
    prelight de las lentes es blanco (medido con `C:\\tmp\\iav42\\area45.py`,
    las caras con prelight distincto de blanco son 0 en los nodos de intermitente).
    """
    mi = tri[3]
    if mi >= len(g["mats"]) or not g["uvs"]:
        return None
    col, tex = g["mats"][mi]
    key = (tex or "").lower()
    if not key or key not in pix:
        return None
    if key not in cache:
        cache[key] = pix[key]
    w, h, px = cache[key]
    pts = _uv_tri(g, tri)
    xs = [p[0] * w for p in pts]
    ys = [p[1] * h for p in pts]
    ax, ay = xs[0], ys[0]
    bx, by = xs[1], ys[1]
    cx, cy = xs[2], ys[2]
    area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
    if abs(area) < 1e-9:      # cara degenerada: un solo texel, el del baricentro
        cand = [(int((ax + bx + cx) / 3.0) % w, int((ay + by + cy) / 3.0) % h)]
        pesos = [1.0]
    else:
        # Plan 56.0 T1c: cobertura por AREA exacta. Se recorta el triangulo UV
        # contra cada celda del atlas que toca su bounding box y se pesa la
        # texela por el area recortada (ver `_cell_cover`). Lo que habia antes
        # eran las funciones de borde sobre el CENTRO de la texela (que admitian
        # una cuña infinita si se escribian mal: lovefist, romero y stretch) mas
        # un supermuestreo de relleno: contaban la texela entera o no la
        # contaban, segun donde cayera el centro, y eso ya no es una medida de
        # area. El signo del recorte no interviene: se trabaja con el poligono.
        cand = []
        pesos = []
        for ty in range(max(int(math.floor(min(ys))), 0),
                        min(int(math.ceil(max(ys))), h)):
            for tx in range(max(int(math.floor(min(xs))), 0),
                            min(int(math.ceil(max(xs))), w)):
                wt = _cell_cover((ax, ay), (bx, by), (cx, cy), tx, ty)
                if wt > 1e-9:
                    cand.append((tx, ty))
                    pesos.append(wt)
        if not cand:
            return None
    mc = tuple(col[:3]) if col else (255, 255, 255)
    n = 0.0
    acc = [0.0, 0.0, 0.0]
    aacc = [0.0, 0.0, 0.0]
    amb = 0.0
    for (tx, ty), wt in zip(cand, pesos):
        c = px[ty * w + tx]
        if c[3] == 0:
            continue
        rr = tuple(int(c[k] * mc[k] / 255) for k in range(3))
        n += wt
        for k in range(3):
            acc[k] += wt * rr[k]
        if _is_amber(rr + (255,)):
            amb += wt
            for k in range(3):
                aacc[k] += wt * rr[k]
    if n <= 0.0:
        return None
    # La media de los texelos AMBAR, aparte de la de la cara entera: una cara de
    # lente que mitad cristal mitad marco tiene su media fuera de la ventana de
    # matiz, y el destello se pintaria con ese tono apagado en vez de con el
    # ambar que el jugador ve (medido: 104 de las 244 filas ambar de la tabla
    # del plan 45.0 no pasaban `_is_amber` usando la media de la cara).
    # Se devuelven enteros redondeados para no cambiar el contrato con quien
    # llama; la fraccion `amb/n` es la que decide y llega intacta.
    return (int(n + 0.5), tuple(int(x / n) for x in acc), int(amb + 0.5),
            tuple(int(x / amb) for x in aacc) if amb > 0.0 else None)


def _amber_blob(g, pix, tri, cache, rad=24):
    """Parche ambar 4-conexo bajo el centroide UV: (texeles, ancho, alto).

    Es la medida que falta cuando la isla es UNA cara: el centroide puede caer
    en una costura de antialiasing del atlas y la cara pasa el filtro con dos
    texeles naranjas. Medido cara a cara (`C:\\tmp\\iav42\\islas_atlas421.py`):
    las celdas falsas miden 2 y 3 texeles con un lado de 1-2 (washing r-l 2x1,
    topfun r-l 2x1, linerun r-l 2x2) y las lentes reales de una cara empiezan
    en 9 (pony f-l 3x4), 21 (topfun f-l 6x4) y 39 (rumpo f-l 6x8). Las cotas
    se ponen en el hueco, como el hue del plan 42.0.
    """
    hit = _face_texel(g, pix, tri, cache)
    if hit is None:
        return None
    cx, cy, w, h, px = hit
    x0, x1 = max(0, cx - rad), min(w, cx + rad + 1)
    y0, y1 = max(0, cy - rad), min(h, cy + rad + 1)
    seen = set([(cx, cy)])
    stack = [(cx, cy)]
    n = 0
    minx = maxx = cx
    miny = maxy = cy
    while stack:
        x, y = stack.pop()
        n += 1
        if x < minx:
            minx = x
        if x > maxx:
            maxx = x
        if y < miny:
            miny = y
        if y > maxy:
            maxy = y
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if (nx, ny) in seen or not (x0 <= nx < x1 and y0 <= ny < y1):
                continue
            seen.add((nx, ny))
            if _is_amber(px[ny * w + nx]):
                stack.append((nx, ny))
    return n, maxx - minx + 1, maxy - miny + 1


def _amber_face_cover(g, pix, tri, cache):
    """(texeles dentro de la cara, de los cuales ambar); None si la cara no mide.

    El detector de siempre mira UN pixel: una cara de carroceria grande cuyo
    centroide recae en una raya naranja del atlas pasa el filtro sin ser naranja
    (boxville f-l: 2 caras, 0 texeles ambar bajo los triangulos; mrwhoop f-l
    igual). Exigir que la cara este pintada de ambar encima de la mayor parte de
    su area es la misma medida cara a cara que pide el plan, aplicada a la cara
    entera. Con menos de 64 texelos dentro la medida no vale: una lente chica
    (pony 3x4) queda casi toda fuera de su propio parche por cuantizacion.
    """
    hit = _face_texel(g, pix, tri, cache)
    if hit is None:
        return None
    _cx, _cy, w, h, px = hit
    uv = g["uvs"][0]
    pts = [((uv[k][0] % 1.0) * w, (uv[k][1] % 1.0) * h) for k in tri[:3]]
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    x0, x1 = max(0, int(min(xs))), min(w, int(max(xs)) + 2)
    y0, y1 = max(0, int(min(ys))), min(h, int(max(ys)) + 2)
    if x1 <= x0 or y1 <= y0:
        return None
    if (x1 - x0) * (y1 - y0) > 2 * w * h // 5:
        return None          # cara estirada por la costura UV: no juzga
    (ax, ay), (bx, by), (cx, cy) = pts

    def edge(px_, py_, ux, uy, vx, vy):
        return (vx - ux) * (py_ - uy) - (vy - uy) * (px_ - ux)

    tot = edge(ax, ay, bx, by, cx, cy)
    if tot == 0.0:
        return None
    inside = 0
    amber = 0
    for yy in range(y0, y1):
        base = yy * w
        py = yy + 0.5
        for xx in range(x0, x1):
            pxv = xx + 0.5
            e0 = edge(pxv, py, ax, ay, bx, by)
            e1 = edge(pxv, py, bx, by, cx, cy)
            if (e0 >= 0.0) != (e1 >= 0.0):
                continue
            e2 = edge(pxv, py, cx, cy, ax, ay)
            if (e2 >= 0.0) != (e0 >= 0.0):
                continue
            inside += 1
            if _is_amber(px[base + xx]):
                amber += 1
    if not inside:
        return None
    return inside, amber


def _veto_paint_islands(model, frames, geoms, bbox, out):
    """Regla 5, segunda parte (plan 42.1, 03/11): ni costura ni pintura.

    Dos vetos, cada uno con su medida en `C:\\tmp\\iav42`:

    1. Cara grande sin ambar debajo (menos de 1 texel de 10 dentro de la cara,
      con al menos 64 texelos medidos): el centroide mintio. Cae mrwhoop f-l/
      f-r y boxville f-l/f-r (cobertura 0.000) y la cara central del cuban
      (nx -0.33, pintura del capot).
    2. Isla de UNA cara: el parche del atlas ha de ser un blob de al menos 8
      texelos con los dos lados >= 3, y la cara ha de colgar del borde exterior
      del coche (|x| >= 0.70 de la caja). Cae washing r-l/r-r, linerun r-l/r-r
      y topfun r-l/r-r (parches de 2 y 3 texelos) y mrwhoop f-l/f-r (x -0.61 y
      +0.40). Se salva lo que el jugador pidio: pony y rumpo delante (9 y 39
      texelos en x +/-0.96), hotrina/hotring, romero, coach rf, topfun f-l.

      El |x| se mide contra la caja del modelo, que en el mrwhoop esta
      ensanchada por la escalera lateral (+1.69 frente a -1.31): aun con esa
      desventaja la celda delantera queda fuera (0.61 < 0.70).
    """
    lo, hi = bbox
    dx = max(hi[0] - lo[0], 0.01)
    cx0 = (lo[0] + hi[0]) / 2.0
    pix = texture_pixels(model)
    cache = {}
    kept = {}
    for key, buckets in out.items():
        bk = {}
        for (fi, gi, mi), tris in buckets.items():
            g = geoms[gi]
            live = []
            for t in tris:
                cov = _amber_face_cover(g, pix, t, cache)
                if cov is not None and cov[0] >= 64 and cov[1] * 10 < cov[0]:
                    continue
                live.append(t)
            if not live:
                continue
            if len(live) == 1:
                blob = _amber_blob(g, pix, live[0], cache)
                if blob is None or blob[0] < 8 or min(blob[1], blob[2]) < 3:
                    continue
                p = _face_center(dg.world_of(frames, fi), g, live[0])
                if abs((p[0] - cx0) / (dx / 2.0)) < 0.70:
                    continue
            bk[(fi, gi, mi)] = live
        if bk:
            kept[key] = bk
    return kept


def _face_center(m, g, tri):
    local = [sum(g["verts"][v][k] for v in tri[:3]) / 3.0 for k in range(3)]
    return _xform(m, local)


def _expand_island(g, mi, tris, center, rad=0.30, color_ok=None):
    """Cierra la isla de malla de la lente: los dos triangulos del piloto.

    Con color_ok la isla solo crece por caras que pasan el mismo filtro que la
    semilla (regla 5 del plan 42.1): sin el filtro el flood se llevaba el panel
    vecino y el nodo salia mezclado, el pegote que hay que evitar.
    """
    verts = g["verts"]
    used = set(id(t) for t in tris)
    by_vert = {}
    for t in g["tris"]:
        if t[3] != mi:
            continue
        for v in t[:3]:
            by_vert.setdefault(v, []).append(t)
    frontier = list(tris)
    while frontier:
        t = frontier.pop()
        for v in t[:3]:
            for o in by_vert.get(v, ()):
                if id(o) in used:
                    continue
                c = [sum(verts[w][k] for w in o[:3]) / 3.0 for k in range(3)]
                if sum((c[k] - center[k]) ** 2 for k in range(3)) > rad * rad:
                    continue
                if color_ok is not None and not color_ok(o):
                    continue
                used.add(id(o))
                frontier.append(o)
    return [t for t in g["tris"] if id(t) in used]


def _cluster_items(items, rad):
    """Agrupa por cercania (items: (fi, gi, tri, world, color))."""
    left = list(items)
    out = []
    while left:
        seed = left.pop(0)
        group = [seed]
        changed = True
        while changed:
            changed = False
            rest = []
            for it in left:
                if any(sum((it[3][k] - o[3][k]) ** 2 for k in range(3)) <= rad * rad
                       for o in group):
                    group.append(it)
                    changed = True
                else:
                    rest.append(it)
            left = rest
        out.append(group)
    return out


def _lens_sources(model, frames, geoms, atomics):
    """Caras ambar reales por (extremo, lado), agrupadas por geometria."""
    pix = texture_pixels(model)
    lo, hi = _world_bbox(frames, geoms, atomics)
    largo = max(hi[1] - lo[1], 1.0)
    out = {}
    if model in BIKE_LENSES:
        # La moto no pasa el filtro de coches (zrel): su lente trasera son las
        # dos caras por lado a |x| >= 0.10, en la mitad trasera.
        cache = {}
        for fi, gi in atomics:
            name = frames[fi]["name"].lower()
            if name in INDICATOR_NODES or "vlo" in name or name.endswith("_dam"):
                continue
            g = geoms[gi]
            if not g["verts"] or not g["uvs"]:
                continue
            m = dg.world_of(frames, fi)
            for tri in g["tris"]:
                if max(tri[:3]) >= len(g["verts"]):
                    continue
                c = _face_color(g, pix, tri, cache)
                if c is None or not _is_amber(c):
                    continue
                p = _face_center(m, g, tri)
                if p[1] > -0.8 or abs(p[0]) < 0.10 or p[2] > 0.0:
                    continue
                side = "left" if p[0] < 0 else "right"
                out.setdefault((("rear"), side), {}).setdefault(
                    (fi, gi, tri[3]), []).append(tri)
        return _whole_islands(out)

    keep, _lo, _hi = _candidate_lenses(model, frames, geoms, atomics)
    pcache = {}

    def _amber_of(gg):
        def ok(t, _g=gg, _pix=pix, _cache=pcache):
            c = _face_color(_g, _pix, t, _cache)
            return c is not None and _is_amber(c)
        return ok

    for c in keep:
        rel = (c["cy"] - lo[1]) / largo
        where = "front" if rel > 0.5 else "rear"
        side = "left" if c["cx"] < 0 else "right"
        key = (where, side)
        bucket = out.setdefault(key, {})
        entry = bucket.setdefault((c["fi"], c["gi"], c["mi"]), [])
        entry.extend(t[0] for t in c["faces"])
        gg = geoms[c["gi"]]
        ctr = [sum(gg["verts"][v][k] for t in entry for v in t[:3]) /
               (3.0 * len(entry)) for k in range(3)]
        ok = _amber_of(gg)
        entry[:] = _expand_island(gg, c["mi"], entry, ctr, color_ok=ok)
        entry[:] = _close_twins(gg, c["mi"], entry, color_ok=ok)
    return _whole_islands(_veto_paint_islands(model, frames, geoms, (lo, hi), out))


def _borrow_source(model, frames, geoms, atomics, end, side):
    """Caras del piloto (rojo) o del faro (claro) de ese extremo y lado."""
    pix = texture_pixels(model)
    dummy = "taillights" if end == "rear" else "headlights"
    pos = _dummy_world(frames, dummy)
    if pos is None:
        return None
    if abs(pos[0]) < 0.12:
        return None          # lampara unica centrada: no hay lado
    tx = -abs(pos[0]) if side == "left" else abs(pos[0])
    target = (tx, pos[1], pos[2])
    cache = {}
    items = []
    for fi, gi in atomics:
        name = frames[fi]["name"].lower()
        if name in INDICATOR_NODES or "vlo" in name or name.endswith("_dam"):
            continue
        g = geoms[gi]
        if not g["verts"] or not g["uvs"]:
            continue
        m = dg.world_of(frames, fi)
        for tri in g["tris"]:
            if max(tri[:3]) >= len(g["verts"]):
                continue
            c = _face_color(g, pix, tri, cache)
            if c is None:
                continue
            if not (_is_red(c) if end == "rear" else _is_light(c)):
                continue
            p = _face_center(m, g, tri)
            if (p[0] < 0) != (side == "left"):
                continue
            if abs(p[1] - pos[1]) > 0.60:
                continue
            d = sum((p[k] - target[k]) ** 2 for k in range(3)) ** 0.5
            if d > 0.55:
                continue
            items.append((d, fi, gi, tri, p, c))
    if not items:
        return None
    items.sort(key=lambda it: it[0])
    best = items[0]
    same = [it for it in items if (it[1], it[2]) == (best[1], best[2])]
    same = [it for it in same if ((it[4][0] < 0) == (side == "left"))]
    if not same:
        return None
    same.sort(key=lambda it: it[0])
    nseed = min(3, len(same))
    group = [it for it in same
             if any(all(abs(it[4][k] - s[4][k]) <= 0.16 for k in range(3))
                    for s in same[:nseed])]
    bg = geoms[best[2]]
    ctr = [sum(bg["verts"][v][k] for it in group for v in it[3][:3]) /
           (3.0 * len(group)) for k in range(3)]
    kind = _is_red if end == "rear" else _is_light
    def _color_ok(o, _g=bg, _pix=pix, _cache={}, _k=kind):
        c = _face_color(_g, _pix, o, _cache)
        return c is not None and _k(c)

    island = _expand_island(bg, best[3][3], [it[3] for it in group], ctr,
                            color_ok=_color_ok)
    if len(island) > len(group):
        ids = set(id(t) for t in island)
        group = [it for it in items if id(it[3]) in ids]
    group = [it for it in items if id(it[3]) in
             set(id(t) for t in _close_twins(bg, best[3][3],
                                             [it[3] for it in group],
                                             _color_ok))]
    pts = [it[4] for it in group]
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    zs = [p[2] for p in pts]
    if (len(group) > 40 or max(xs) - min(xs) > 0.45 or
            max(ys) - min(ys) > 0.45 or max(zs) - min(zs) > 0.45):
        print("  AVISO %s %s %s: racimo grande (%d caras, %.2fx%.2fx%.2f); se omite"
              % (model, end, side, len(group),
                 max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)))
        return None
    return (best[1], best[2], best[3][3], [it[3] for it in group])


def _pick_source(sources, frames, geoms, end, side):
    """Si hay varios racimos, el mas cercano al dummy de la lampara."""
    if not sources or len(sources) == 1:
        return sources
    pos = _dummy_world(frames, "taillights" if end == "rear" else "headlights")
    if pos is None:
        return sources[:1]
    tx = -abs(pos[0]) if side == "left" else abs(pos[0])
    best, bd = None, 1e9
    for src in sources:
        fi, gi, _mi, tris = src
        m = dg.world_of(frames, fi)
        pts = [_face_center(m, geoms[gi], t) for t in tris]
        c = [sum(p[k] for p in pts) / len(pts) for k in range(3)]
        d = (c[0] - tx) ** 2 + (c[1] - pos[1]) ** 2 + (c[2] - pos[2]) ** 2
        if d < bd:
            best, bd = src, d
    return [best]


def _own_selection(model, frames, geoms, atomics):
    lens = _lens_sources(model, frames, geoms, atomics)
    out = {}
    for end in ("front", "rear"):
        for side in ("left", "right"):
            key = (end, side)
            src = lens.get(key)
            if src:
                out[key] = _pick_source(src, frames, geoms, end, side)
                continue
            b = _borrow_source(model, frames, geoms, atomics, end, side)
            out[key] = [b] if b else []
    return out


def _rel_matrix(frames, src_fi, dst_fi):
    """3x4 que lleva coordenadas locales de `src_fi` a las de `dst_fi`."""
    if src_fi == dst_fi:
        return None
    a = dg.world_of(frames, src_fi)
    inv_r, inv_t = _inv_world(dg.world_of(frames, dst_fi))
    rot = [[sum(inv_r[x][k] * a[k][b] for k in range(3)) for b in range(3)]
           for x in range(3)]
    t = [sum(inv_r[x][k] * a[k][3] for k in range(3)) + inv_t[x] for x in range(3)]
    return [rot[x] + [t[x]] for x in range(3)]


def _struct_remap(g, tris0, used, xfrm=None):
    """STRUCT nuevo con solo los vertices usados (sin el array de 596)."""
    flags = g["flags"] & ~1
    body = bytearray(struct.pack("<Iiii", flags, len(tris0), len(used), 1))
    if (g["flags"] & 0x08) and g.get("colors"):
        for v in used:
            body += struct.pack("<4B", *g["colors"][v])
    for uvs in g["uvs"]:
        for v in used:
            body += struct.pack("<2f", uvs[v][0], uvs[v][1])
    # El cuarto entero del triangulo (los 16 bits bajos de la segunda palabra)
    # es el INDICE DE MATERIAL: `dff_geom.parse_geometry` lo lee ahi, no en el
    # chunk de malla. Escribir 0 fijo ponía toda la isla con el material de la
    # semilla -> el pixel muestreado cambia y el nodo sale con el color de otra
    # pieza (los 9 cromados de bloodrb delante, el ambar inventado de trash).
    for a, b, c, m in tris0:
        body += struct.pack("<II", (a << 16) | b, (c << 16) | m)
    body += struct.pack("<4f", 0.0, 0.0, 0.0, 0.0)
    body += struct.pack("<ii", 1, 1)
    for v in used:
        p = _xform(xfrm, g["verts"][v]) if xfrm else g["verts"][v]
        body += struct.pack("<3f", *p)
    norms = g.get("norms") or []
    for v in used:
        n = norms[v] if norms else (0.0, 0.0, 1.0)
        if xfrm:
            n = [sum(xfrm[a][k] * n[k] for k in range(3)) for a in range(3)]
        body += struct.pack("<3f", *n)
    return bytes(body)


def _node_geometry(g, raw, mi, tris, xfrm=None):
    sub = chunks(raw, 0, len(raw))
    stv, _ = find(sub, 0x01)
    mlv, mlb = find(sub, 0x08)
    ml_struct_ver, _ = find(chunks(mlb, 0, len(mlb)), 0x01)
    mats = [(v, b) for t, v, b in chunks(mlb, 0, len(mlb)) if t == 0x07]
    # La isla puede traer caras de varios materiales del mismo geometry (el
    # faro con su material de lente y el marco con el de carroceria). Forzar el
    # material de la semilla cambiaba el pixel muestreado de las demas: caras
    # blancas que se volvian negras o, peor, ámbar inventado (trash delante).
    keep = []
    for _a, _b, _c, m in tris:
        m = m if m < len(mats) else 0
        if m not in keep:
            keep.append(m)
    if mi < len(mats):
        if mi in keep:
            keep.remove(mi)
        keep.insert(0, mi)
    matmap = {m: i for i, m in enumerate(keep)}
    # El STRUCT de la lista de materiales es `numMaterials` seguido de
    # `numMaterials` indices (todos -1 en GTA: el material se usa tal cual).
    # Escribir uno solo dejaba al lector leer la cabecera del chunk siguiente
    # como indice: `numMaterials=2 no cabe en STRUCT de 8` (dff_readcheck).
    matstruct = struct.pack("<i", len(keep)) + struct.pack("<%di" % len(keep), *([-1] * len(keep)))
    matlist = chunk(0x01, ml_struct_ver, matstruct) + \
        b"".join(chunk(0x07, mats[m][0], mats[m][1]) for m in keep)
    used = sorted({v for tri in tris for v in tri[:3]})
    remap = {v: i for i, v in enumerate(used)}
    tris0 = [(remap[a], remap[b], remap[c], matmap.get(m, 0))
             for a, b, c, m in tris]
    # `_struct_remap` escribe el STRUCT **sin** los 12 bytes de surface props,
    # asi que el chunk debe declarar la version 0x34003 (0x1003FFFF). Copiar la
    # version del geometry de origen (0xC02FFFF) hace que el lector consuma 12
    # bytes de mas y el modelo no cargue (plan 34.0).
    return (chunk(0x01, 0x1003FFFF, _struct_remap(g, tris0, used, xfrm)) +
            chunk(0x08, mlv, matlist) +
            chunk(0x03, 0xC02FFFF, build_mesh(tris0)))


def _nodes_present(frames):
    """Nodos indicator_* ya escritos en el DFF: sena de que viene parcheado.

    El vanilla no tiene ninguno, asi que cualquier nodo en la entrada es obra de
    un parche previo. own_faces_patch corta las caras de la carroceria: aplicado
    dos veces, la segunda solo encuentra lo que quedo y vacia medio quad
    (regresion del plan 42.0 T98).
    """
    return sorted({f["name"].lower() for f in frames
                   if f["name"].lower() in INDICATOR_NODES})


def own_faces_patch(buf, model, allow_nodes=None):
    """Sustituye los nodos indicator_* por las caras reales del coche."""
    frames, geoms, atomics = dg.parse(buf)
    present = _nodes_present(frames)
    if present and model not in (allow_nodes or ()):
        raise SystemExit("%s: la entrada ya trae nodos %s; se parchea desde la "
                         "base vanilla (--from)" % (model, ",".join(present)))
    sel = _own_selection(model, frames, geoms, atomics)
    if not any(sel.values()):
        raise SystemExit("%s: sin caras para ningun extremo" % model)

    top = chunks(buf, 0, len(buf))
    clump_ver, cb = find(top, 0x10)
    cc = chunks(cb, 0, len(cb))
    clump_struct = bytearray(find(cc, 0x01)[1])
    flv, flb = find(cc, 0x0E)
    glv, glb = find(cc, 0x1A)
    fch = chunks(flb, 0, len(flb))
    fstruct = bytearray(find(fch, 0x01)[1])
    names = [b for t, _v, b in fch if t == 0x03]
    if struct.unpack_from("<I", fstruct, 0)[0] != len(names):
        raise SystemExit("%s: frame list y nombres no cuadran" % model)
    gch = chunks(glb, 0, len(glb))
    gstruct_ver, _ = find(gch, 0x01)
    raw_geoms = [(v, b) for t, v, b in gch if t == 0x0F]
    if len(raw_geoms) != len(geoms):
        raise SystemExit("%s: lista de geometrias dividida" % model)
    atomic_bodies = [b for t, _v, b in cc if t == 0x14]
    atomic_index = {(fi, gi): k for k, (fi, gi) in enumerate(atomics)}
    new_geoms = list(raw_geoms)

    existing = {}
    for f in frames:
        n = f["name"].lower()
        if n not in INDICATOR_NODES:
            continue
        for fi, gi in atomics:
            if fi == f["i"]:
                existing[n] = (fi, gi)
                break

    desired = {("front", "left"): "indicator_lf",
               ("front", "right"): "indicator_rf",
               ("rear", "left"): "indicator_lr",
               ("rear", "right"): "indicator_rr"}
    assigned = {}
    used_frames = set()
    for key, name in desired.items():
        if name in existing and sel.get(key):
            fi, gi = existing[name]
            assigned[name] = [fi, gi, sel[key]]
            used_frames.add(fi)
    extras = [(n, v) for n, v in existing.items() if v[0] not in used_frames]
    for key, name in desired.items():
        if name in assigned or not sel.get(key) or not extras:
            continue
        suffix = ("l" if key[1] == "left" else "r") + ("f" if key[0] == "front" else "r")
        pick = next((e for e in extras if e[0].endswith(suffix)), None)
        if pick is None:
            continue
        extras.remove(pick)
        _old, (fi, gi) = pick
        names[fi] = chunk(0x253F2FE, 0xC02FFFF, name.encode("latin-1"))
        assigned[name] = [fi, gi, sel[key]]
        used_frames.add(fi)
    added_frames = 0
    for key, name in desired.items():
        if name in assigned or not sel.get(key):
            continue
        fi = struct.unpack_from("<I", fstruct, 0)[0]
        struct.pack_into("<I", fstruct, 0, fi + 1)
        added_frames += 1
        src_fi = sel[key][0][0]
        f = frames[src_fi]
        parent = f["parent"] if _is_identity_frame(f) else f["i"]
        fstruct.extend(struct.pack("<9f3fiI", *(IDENTITY + (0.0, 0.0, 0.0, parent, 0))))
        names.append(chunk(0x253F2FE, 0xC02FFFF, name.encode("latin-1")))
        assigned[name] = [fi, None, sel[key]]

    resumen = []
    drop = {}
    for name in ("indicator_lf", "indicator_rf", "indicator_lr", "indicator_rr"):
        if name not in assigned:
            continue
        fi, gi, sources = assigned[name]
        sources = [s for s in sources if s]
        if not sources:
            continue
        gch_src = None
        first = True
        parent_fi = None
        for src_fi, src_gi, mi, tris in sources:
            if src_gi is None:
                continue
            # El nodo cuelga del marco de origen y su matriz es identidad: los
            # vertices del nodo estan en coordenadas de ESE marco. Si el nodo
            # viejo tenia el frame en el centro de la lente, hay que devolverlo
            # a 0 o la geometria se desplaza dos veces.
            sf = frames[src_fi]
            parent = sf["parent"] if _is_identity_frame(sf) else sf["i"]
            if parent_fi is None:
                parent_fi = parent
                off = 4 + fi * 56
                struct.pack_into("<9f", fstruct, off, *IDENTITY)
                struct.pack_into("<3f", fstruct, off + 36, 0.0, 0.0, 0.0)
                struct.pack_into("<i", fstruct, off + 48, parent)
            # La lente partida entre parachoques y salpicadero son dos racimos
            # en marcos distintos: el nodo tiene UN padre, asi que el segundo se
            # reescribe en las coordenadas de ese padre. Sin esto el parachoques
            # salia desplazado y el racimo sobrante se tiraba a la carroceria
            # (media lente encendida: landstal, packer, sandking, rancher,
            # esperant, zebra).
            xfrm = _rel_matrix(frames, parent, parent_fi)
            gch_src = chunks(raw_geoms[src_gi][1], 0, len(raw_geoms[src_gi][1]))
            geo = geoms[src_gi]
            drop.setdefault(src_gi, set()).update(id(t) for t in tris)
            body = _node_geometry(geo, raw_geoms[src_gi][1], mi, tris, xfrm)
            gidx = len(new_geoms)
            new_geoms.append((raw_geoms[src_gi][0], body))
            if gi is None:
                k = atomic_index.get((src_fi, src_gi))
                ext = find(chunks(atomic_bodies[k], 0, len(atomic_bodies[k])), 0x03) \
                    if k is not None else (None, None)
                atomic_bodies.append(
                    chunk(0x01, 0xC02FFFF, struct.pack("<4I", fi, gidx, 5, 0)) +
                    (chunk(0x03, ext[0], ext[1]) if ext[1] is not None else b""))
            else:
                k = atomic_index.get((fi, gi))
                if k is None:
                    raise SystemExit("%s: atomic del nodo %s no encontrado" % (model, name))
                if first:
                    ba = bytearray(atomic_bodies[k])
                    struct.pack_into("<I", ba, 16, gidx)
                    atomic_bodies[k] = bytes(ba)
                else:
                    ext0 = find(chunks(atomic_bodies[k], 0, len(atomic_bodies[k])), 0x03)
                    atomic_bodies.append(
                        chunk(0x01, 0xC02FFFF, struct.pack("<4I", fi, gidx, 5, 0)) +
                        (chunk(0x03, ext0[0], ext0[1]) if ext0[1] is not None else b""))
            resumen.append("%s(%d caras)" % (name, len(tris)))
            first = False

    # La carroceria pierde las caras que se llevo el nodo (no se duplican).
    for gi, ids in drop.items():
        sub = chunks(raw_geoms[gi][1], 0, len(raw_geoms[gi][1]))
        stv, st = find(sub, 0x01)
        mlv, mlb = find(sub, 0x08)
        rest = [t for t in geoms[gi]["tris"] if id(t) not in ids]
        if len(rest) == len(geoms[gi]["tris"]):
            raise SystemExit("%s: no se quito ninguna cara del geom %d" % (model, gi))
        new_geoms[gi] = (raw_geoms[gi][0],
                         build_geometry(stv, mlv, parse_geometry(st), rest, mlb))

    struct.pack_into("<I", clump_struct, 0, len(atomic_bodies))
    new_fl = chunk(0x01, 0xC02FFFF, bytes(fstruct)) + b"".join(
        chunk(0x03, 0xC02FFFF, n) for n in names)
    new_gl = chunk(0x01, gstruct_ver, struct.pack("<I", len(new_geoms))) + b"".join(
        chunk(0x0F, v, b) for v, b in new_geoms)
    clump_ext = [b for t, _v, b in cc if t == 0x03]
    body = chunk(0x01, 0xC02FFFF, bytes(clump_struct))
    body += chunk(0x0E, flv, new_fl)
    body += chunk(0x1A, glv, new_gl)
    body += b"".join(chunk(0x14, 0xC02FFFF, a) for a in atomic_bodies)
    body += b"".join(chunk(0x03, 0xC02FFFF, b) for b in clump_ext)
    out = chunk(0x10, clump_ver, body)
    if len(out) % 2048:
        out += b"\0" * (2048 - len(out) % 2048)

    chk_frames, chk_geoms, chk_atomics = dg.parse(out)
    if len(chk_frames) != len(frames) + added_frames:
        raise SystemExit("%s: frames nuevos mal escritos" % model)
    for fi, gi in chk_atomics:
        n = chk_frames[fi]["name"].lower()
        if n in assigned:
            if not chk_geoms[gi]["tris"]:
                raise SystemExit("%s: %s sin triangulos" % (model, n))
            if not any((t or "").lower() != "lights" for _c, t in chk_geoms[gi]["mats"]):
                raise SystemExit("%s: %s conserva material lights" % (model, n))
    print("%s: %s -> %d bytes" % (model, ", ".join(resumen), len(out)))
    return out


def own_faces_apply(model, dry=False, keep=False):
    path = os.path.join(BASE, model + ".dff")
    with open(path, "rb") as f:
        buf = f.read()
    out = own_faces_patch(buf, model, allow_nodes=(model,) if keep else None)
    if dry:
        print("%s: (dry) %d -> %d bytes" % (model, len(buf), len(out)))
        return 1
    for d in (os.path.join(GAMEFILES, "gta3.img"), MODELS,
              os.path.join(BOOTSEED, "gta3.img")):
        target = os.path.join(d, model + ".dff")
        if not os.path.exists(target):
            continue
        tmp = target + ".tmp"
        with open(tmp, "wb") as f:
            f.write(out)
        os.replace(tmp, target)
    return 1


def _world_eq(frames, a, b, tol=1e-4):
    wa = dg.world_of(frames, a)
    wb = dg.world_of(frames, b)
    return all(abs(wa[i][j] - wb[i][j]) < tol for i in range(4) for j in range(4))


def lens_move(buf, model, ops):
    """Aplica la lista de caras del plan 45.0 sobre el DFF, sin heuristicas.

    Dos clases de operacion, ambas identificadas por el indice de triangulo
    dentro de su geometry:

      * `lens`  -- la lente medida en la carroceria pasa al nodo `indicator_*`
        que dicta el veredicto. Si el nodo tiene otra matriz mundial que el frame
        de origen, los vertices se reescriben en coordenadas del nodo (medido en
        los 10 racimos de matriz distinta: la diferencia es pura traslacion, asi
        que la lente queda exactamente donde estaba).
      * `panel` -- las caras que compartian el material de esa lente salen del
        nodo y cuelgan de un frame `panel_*` copia del nodo (mismo padre y misma
        matriz local): la malla no se mueve y `ViceExtSetIndicatorMaterial` ya no
        las recorre, que es lo que pinta el panel de ámbar. Cuando el atomic del
        nodo no tiene ninguna cara de lente, lo que se muda es el atomic entero
        (se re-punta su indice de frame) en lugar de partir la geometria, porque
        partirla lo dejaria sin caras.

    Las primitivas son las de `own_faces_patch`: geometria nueva con los vertices
    y las normales reales de las caras que se van, material clonado dentro de esa
    geometria, y la geometria de origen reconstruida sin esas caras. El total de
    triangulos se conserva; si no cuadra, no se escribe.
    """
    frames, geoms, atomics = dg.parse(buf)
    top = chunks(buf, 0, len(buf))
    clump_ver, cb = find(top, 0x10)
    cc = chunks(cb, 0, len(cb))
    clump_struct = bytearray(find(cc, 0x01)[1])
    flv, flb = find(cc, 0x0E)
    glv, glb = find(cc, 0x1A)
    fch = chunks(flb, 0, len(flb))
    fstruct = bytearray(find(fch, 0x01)[1])
    names = [b for t, _v, b in fch if t == 0x03]
    numframes = struct.unpack_from("<I", fstruct, 0)[0]
    if numframes != len(names):
        raise SystemExit("%s: frame list y nombres no cuadran" % model)
    gch = chunks(glb, 0, len(glb))
    gstruct_ver, _ = find(gch, 0x01)
    raw_geoms = [(v, b) for t, v, b in gch if t == 0x0F]
    if len(raw_geoms) != len(geoms):
        raise SystemExit("%s: lista de geometrias dividida; move no soportado" % model)
    atomic_bodies = [b for t, _v, b in cc if t == 0x14]
    atomic_index = {(fi, gi): k for k, (fi, gi) in enumerate(atomics)}
    by_name = {}
    for f in frames:
        n = (f["name"] or "").lower()
        if n in by_name:
            raise SystemExit("%s: frame %s duplicado; el motor resuelve los nodos "
                             "por nombre" % (model, n))
        by_name[n] = f

    panel_de = {}

    def panel_frame(node):
        """Frame copia del nodo: mismo padre y misma matriz local, nombre nuevo."""
        if node in panel_de:
            return panel_de[node]
        name = "panel" + node[len("indicator"):]
        if name in by_name:
            raise SystemExit("%s: el frame %s ya existe" % (model, name))
        proto = 4 + by_name[node]["i"] * 56
        idx = struct.unpack_from("<I", fstruct, 0)[0]
        struct.pack_into("<I", fstruct, 0, idx + 1)
        fstruct.extend(bytes(fstruct[proto:proto + 56]))
        names.append(chunk(0x253F2FE, 0xC02FFFF, name.encode("latin-1")))
        panel_de[node] = idx
        return idx

    # `panel` sobre TODAS las caras de un atomic del nodo (romero indicator_lf: 1
    # cara, 84,9 cm2 de panel, ninguna de lente). Partirla dejaria un atomic sin
    # caras; la reubicacion correcta es mudar el atomic entero al frame `panel_*`:
    # misma geometria, mismos vertices, y el recorrido del nodo ya no lo ve.
    mudadas = {}
    nodo_de = {}
    for op in ops:
        if op["kind"] != "panel":
            continue
        k = (op["fi"], op["gi"])
        mudadas.setdefault(k, set()).update(ti for ti, _mi in op["tris"])
        nodo_de[k] = (op["nodo"] or "").lower()
    relocate = {}
    for k, ids in sorted(mudadas.items()):
        gi = k[1]
        if len(ids) < len(geoms[gi]["tris"]):
            continue
        if any(op["kind"] == "lens" and (op["fi"], op["gi"]) == k for op in ops):
            raise SystemExit("%s: la geom %d pierde todas sus caras por una op de "
                             "lente; habria que quitar su atomic" % (model, gi))
        n = sum(1 for (_fi, g) in atomics if g == gi)
        if n != 1:
            raise SystemExit("%s: la geom %d la comparten %d atomics; mudar el "
                             "frame mudaria tambien el otro" % (model, gi, n))
        relocate[k] = panel_frame(nodo_de[k])

    new_geoms = list(raw_geoms)
    drop = {}
    movidas = {"lens": 0, "panel": 0, "mudar": 0}
    for op in ops:
        kind = op["kind"]
        if kind not in ("lens", "panel"):
            raise SystemExit("%s: operacion %s desconocida" % (model, kind))
        if kind == "panel" and (op["fi"], op["gi"]) in relocate:
            movidas["mudar"] += 1
            continue
        gi = op["gi"]
        g = geoms[gi]
        sel = []
        for (ti, mi) in op["tris"]:
            if ti >= len(g["tris"]):
                raise SystemExit("%s: la cara %d no existe en la geom %d; la lista "
                                 "esta vencida" % (model, ti, gi))
            t = g["tris"][ti]
            if t[3] != mi:
                raise SystemExit("%s: la cara %d de la geom %d usa el material %d y "
                                 "la lista decia %d" % (model, ti, gi, t[3], mi))
            sel.append(t)
            drop.setdefault(gi, set()).add(ti)
        if not sel:
            raise SystemExit("%s: operacion %s sin caras" % (model, kind))
        node = (op["nodo"] or "").lower()
        if node not in by_name:
            raise SystemExit("%s: no existe el nodo %s" % (model, node))
        if kind == "lens":
            dst_fi = by_name[node]["i"]
            xfrm = None if _world_eq(frames, op["fi"], dst_fi) else \
                _rel_matrix(frames, op["fi"], dst_fi)
        else:
            dst_fi = panel_frame(node)
            xfrm = None
        body = _node_geometry(g, raw_geoms[gi][1], op["tris"][0][1], sel, xfrm)
        gidx = len(new_geoms)
        new_geoms.append((raw_geoms[gi][0], body))
        k = atomic_index.get((op["fi"], gi))
        ext = find(chunks(atomic_bodies[k], 0, len(atomic_bodies[k])), 0x03) \
            if k is not None else (None, None)
        atomic_bodies.append(
            chunk(0x01, 0xC02FFFF, struct.pack("<4I", dst_fi, gidx, 5, 0)) +
            (chunk(0x03, ext[0], ext[1]) if ext[1] is not None else b""))
        movidas[kind] += 1

    for k, dst_fi in sorted(relocate.items()):
        j = atomic_index[k]
        ab = bytearray(atomic_bodies[j])
        # El struct RpAtomic esta dentro del sub-chunk 0x01 y esa cabecera ocupa
        # los 12 primeros bytes: frame, geometry, flags, parent. Se comprueban los
        # ids que la lista promete antes de re-puntar el frame.
        if struct.unpack_from("<2I", ab, 12) != k:
            raise SystemExit("%s: el atomic (frame %d, geom %d) dice otra cosa en "
                             "sus bytes; no se re-punta" % (model, k[0], k[1]))
        struct.pack_into("<I", ab, 12, dst_fi)
        atomic_bodies[j] = bytes(ab)

    for gi, ids in sorted(drop.items()):
        sub = chunks(raw_geoms[gi][1], 0, len(raw_geoms[gi][1]))
        stv, st = find(sub, 0x01)
        mlv, mlb = find(sub, 0x08)
        rest = [t for i, t in enumerate(geoms[gi]["tris"]) if i not in ids]
        if not rest:
            raise SystemExit("%s: la geom %d se queda sin caras (%d de %d movidas); "
                             "habria que quitar tambien su atomic"
                             % (model, gi, len(ids), len(geoms[gi]["tris"])))
        new_geoms[gi] = (raw_geoms[gi][0],
                         build_geometry(stv, mlv, parse_geometry(st), rest, mlb))

    struct.pack_into("<I", clump_struct, 0, len(atomic_bodies))
    new_fl = chunk(0x01, 0xC02FFFF, bytes(fstruct)) + b"".join(
        chunk(0x03, 0xC02FFFF, n) for n in names)
    new_gl = chunk(0x01, gstruct_ver, struct.pack("<I", len(new_geoms))) + b"".join(
        chunk(0x0F, v, b) for v, b in new_geoms)
    clump_ext = [b for t, _v, b in cc if t == 0x03]
    body = chunk(0x01, 0xC02FFFF, bytes(clump_struct))
    body += chunk(0x0E, flv, new_fl)
    body += chunk(0x1A, glv, new_gl)
    body += b"".join(chunk(0x14, 0xC02FFFF, a) for a in atomic_bodies)
    body += b"".join(chunk(0x03, 0xC02FFFF, b) for b in clump_ext)
    out = chunk(0x10, clump_ver, body)
    if len(out) % 2048:
        out += b"\0" * (2048 - len(out) % 2048)

    chk_frames, chk_geoms, chk_atomics = dg.parse(out)
    if len(chk_frames) != numframes + len(panel_de):
        raise SystemExit("%s: %d frames nuevos, se esperaban %d"
                         % (model, len(chk_frames) - numframes, len(panel_de)))
    antes = sum(len(g["tris"]) for g in geoms)
    despues = sum(len(g["tris"]) for g in chk_geoms)
    if antes != despues:
        raise SystemExit("%s: %d triangulos antes y %d despues" % (model, antes, despues))
    bajo = {}
    for fi, gi in chk_atomics:
        bajo.setdefault(chk_frames[fi]["name"].lower(), 0)
        bajo[chk_frames[fi]["name"].lower()] += len(chk_geoms[gi]["tris"])
    for op in ops:
        nodo = op["nodo"].lower()
        if not bajo.get(nodo):
            raise SystemExit("%s: el nodo %s acaba sin triangulos" % (model, nodo))
        if op["kind"] == "panel" and not bajo.get("panel" + nodo[len("indicator"):]):
            raise SystemExit("%s: el frame panel_%s acaba sin triangulos"
                             % (model, nodo[len("indicator"):]))
    print("%s: %d lente + %d panel + %d mudadas (%d frames nuevos) %d -> %d bytes"
          % (model, movidas["lens"], movidas["panel"], movidas["mudar"],
             len(panel_de), len(buf), len(out)))
    return out


def lens_move_apply(model, ops, dry=False, backup=None):
    """Lee el DFF servido, aplica `lens_move` y lo escribe en las tres copias."""
    path = os.path.join(BASE, model + ".dff")
    with open(path, "rb") as f:
        buf = f.read()
    out = lens_move(buf, model, ops)
    if dry:
        print("%s: (dry) no se escribe" % model)
        return 1
    if backup:
        os.makedirs(backup, exist_ok=True)
        shutil.copy2(path, os.path.join(backup, model + ".dff"))
    for d in (os.path.join(GAMEFILES, "gta3.img"), MODELS,
              os.path.join(BOOTSEED, "gta3.img")):
        target = os.path.join(d, model + ".dff")
        if not os.path.exists(target):
            continue
        tmp = target + ".tmp"
        with open(tmp, "wb") as f:
            f.write(out)
        os.replace(tmp, target)
    return 1


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--stage-faggio", metavar="SALIDA",
                    help="prepara una copia aislada de la Faggio, sin modificar los modelos servidos")
    ap.add_argument("--apply-faggio", action="store_true",
                    help="rehace los nodos indicator_lr/indicator_rr de la Faggio con la lente 9/8 del mod")
    ap.add_argument("--lens", metavar="MODELO", nargs="+",
                    help="rehace/anade los nodos indicator_* con la lente 9/8 del mod")
    ap.add_argument("--crear-faltantes", action="store_true",
                    help="con --lens: tambien los lados cuyo nodo no existe (plan 56.0 T7)")
    ap.add_argument("--own-faces", metavar="MODELO", nargs="+",
                    help="sustituye los nodos indicator_* por las caras reales del coche (plan 41.0)")
    ap.add_argument("--restore-faces", metavar="MODELO", nargs="+",
                    help="devuelve a la carroceria las caras de lente que quitaron los nodos viejos")
    ap.add_argument("--backup", metavar="DIR",
                    default=os.path.join(ROOT, "tmp", "gta3-img-backup-30"),
                    help="carpeta con los DFF previos a la separacion de caras")
    ap.add_argument("--from", dest="base", metavar="DIR",
                    help="DFF/TXD de lectura (por defecto streamed/models/gta3.img)")
    ap.add_argument("--keep-nodes", action="store_true",                    help="con --own-faces: permite re-parchear un DFF que ya trae nodos")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--patch", metavar="MODELO")
    ap.add_argument("--dry", action="store_true", help="con --patch/--repair: no escribe")
    ap.add_argument("--all", action="store_true", help="audita todos los DFF, no solo default.ide")
    ap.add_argument("--repair", metavar="MODELO",
                    help="devuelve la malla a las geometrias sin extension")
    ap.add_argument("--repair-all", action="store_true",
                    help="repara todos los DFF con geometrias sin extension")
    ap.add_argument("--normalize", metavar="MODELO",
                    help="recorta las mallas que se salen de su chunk")
    ap.add_argument("--normalize-all", action="store_true",
                    help="normaliza las mallas de los vehiculos de default.ide")
    ap.add_argument("--normalize-img", action="store_true",
                    help="normaliza las mallas de TODOS los DFF del gta3.img")
    a = ap.parse_args()

    global BASE
    if a.base:
        BASE = os.path.abspath(a.base)
        print("base de lectura: %s" % BASE)

    if a.stage_faggio:
        stage_faggio(a.stage_faggio)
        return 0
    if a.apply_faggio:
        print("faggio: %s" % ("(dry)" if a.dry else "aplicado"))
        return lens_apply('faggio', dry=a.dry)
    if a.lens:
        for m in a.lens:
            lens_apply(m.lower(), dry=a.dry, crear_faltantes=a.crear_faltantes)
        return 0
    if a.own_faces:
        for m in a.own_faces:
            own_faces_apply(m.lower(), dry=a.dry, keep=a.keep_nodes)
        return 0
    if a.restore_faces:
        for m in a.restore_faces:
            restore_faces(m.lower(), a.backup, dry=a.dry)
        return 0
    if a.normalize:
        normalize_model(a.normalize.lower(), dry=a.dry)
        return 0
    if a.normalize_all:
        total = 0
        for m in vehicle_models():
            total += normalize_model(m, dry=a.dry) or 0
        print("normalizadas %d geometrias en total" % total)
        return 0
    if a.normalize_img:
        total = 0
        models = sorted(os.path.splitext(f)[0] for f in os.listdir(MODELS)
                        if f.lower().endswith(".dff"))
        for m in models:
            try:
                total += normalize_model(m, dry=a.dry) or 0
            except SystemExit as e:
                print("%s: omitido (%s)" % (m, e))
        print("normalizadas %d geometrias en total" % total)
        return 0
    if a.repair:
        repair_model(a.repair.lower(), dry=a.dry)
        return 0
    if a.repair_all:
        models = sorted(os.path.splitext(f)[0] for f in os.listdir(MODELS)
                        if f.lower().endswith(".dff"))
        total = 0
        for m in models:
            if needs_repair(m):
                total += repair_model(m, dry=a.dry) or 0
        print("reparadas %d geometrias en total" % total)
        return 0
    if a.patch:
        split_model(a.patch.lower(), dry=a.dry)
        return 0

    models = vehicle_models() if not a.all else sorted(
        os.path.splitext(f)[0] for f in os.listdir(MODELS) if f.lower().endswith(".dff"))
    caps = {}
    problems = []
    for m in models:
        try:
            info = analyze(m)
        except Exception as e:
            problems.append("%s: %s" % (m, e))
            continue
        if info["hasNode"] or info["orangeTextures"]:
            caps[m] = info
            if info["hasNode"] and not (info["frontLeft"] or info["frontRight"] or
                                        info["rearLeft"] or info["rearRight"]):
                problems.append("%s: nodo indicator sin material" % m)
            if info["source"] == "body" and not info["hasNode"]:
                print("  info  %s: naranja en carroceria sin nodo (candidato a split)" % m)

    data = {"version": 1,
            "note": "Sectores naranjas por modelo; source=node|lightsOn|body|none",
            "models": caps}
    # El juego lee `models/indicator_caps.json` desde bootseed. Se escribe en las
    # tres copias canonicas y SOLO ahi: `MODELS` apunta dentro de `gta3.img/`, y
    # un fichero suelto en esa carpeta no tiene entrada en el `.dir`, así que el
    # motor no lo lee nunca y `repack_dir` lo avisa como sobrante (medido
    # 2026-11-03: ninguna herramienta lo buscaba ahí, pese al comentario viejo).
    streamed_models = os.path.join(ROOT, "streamed", "models")
    for d in (GAMEFILES, streamed_models, BOOTSEED):
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, JSON_NAME), "w", encoding="utf-8") as f:
            json.dump(data, f, indent=1, sort_keys=True)
            f.write("\n")

    print("modelos auditados: %d; con nodo: %d; con naranja: %d" % (
        len(models), sum(1 for v in caps.values() if v["hasNode"]), len(caps)))
    for p in problems[:40]:
        print("  AVISO", p)
    print("JSON: gamefiles/models/%s" % JSON_NAME)
    if a.check and problems:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
