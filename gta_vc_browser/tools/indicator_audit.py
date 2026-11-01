#!/usr/bin/env python3
"""Audita los sectores naranjas de los vehiculos y separa los que no tienen nodo.

El plan 28.0 exige que el intermitente sea **el sector naranja real** del modelo:
si el coche/moto trae un nodo `indicator_*` se usa su material (como el mod);
si el naranja esta pintado en la textura de la carroceria y no hay nodo, se
separan las caras de ese sector en un nodo `indicator_*` nuevo (misma
geometria, un solo material). Nunca se inventa una corona en el faro.

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
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dff_geom as dg
import txd_raster as tr

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS = os.path.join(ROOT, "streamed", "models", "gta3.img")
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
def load_txd(model):
    path = os.path.join(MODELS, model + ".txd")
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
                w, h = nt["w"], nt["h"]
                sz, off = nt["levels"][0]
                px = tr.decode(data[off:off + sz], w, h, nt["depth"], nt["fmt"],
                               nt["has_alpha"], nt["compression"])
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
    path = os.path.join(MODELS, model + ".txd")
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
                w, h = nt["w"], nt["h"]
                sz, off = nt["levels"][0]
                px = tr.decode(data[off:off + sz], w, h, nt["depth"], nt["fmt"],
                               nt["has_alpha"], nt["compression"])
            except Exception:
                continue
            if px:
                out[nt["nombre"].lower()] = (w, h, px)
    AMBER_CACHE[model] = out
    return out


def _is_amber(c):
    """Naranja de una lente, no amarillo de pintura ni marron de madera.

    El amarillo del taxi tiene g casi igual a r (r-g ~ 20-36) y la madera es
    apagada; la lente naranja mantiene r-g >= 45 incluso en su sombra.
    """
    r, g, b, a = c
    return (a > 0 and r >= 105 and 45 <= g <= 115 and b <= 75 and
            r - g >= 45 and g - b >= 30)


# --------------------------------------------------------------------------
# analisis
# --------------------------------------------------------------------------
def load_model(model, with_geoms=False):
    path = os.path.join(MODELS, model + ".dff")
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
    return [n for n in names if os.path.exists(os.path.join(MODELS, n + ".dff"))]


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
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
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
    # El juego lee `models/indicator_caps.json` desde bootseed (no dentro de
    # gta3.img). Se escribe en las tres copias canonicas; MODELS se conserva
    # por compatibilidad con las herramientas que ya lo buscan ahi.
    streamed_models = os.path.join(ROOT, "streamed", "models")
    for d in (GAMEFILES, streamed_models, BOOTSEED, MODELS):
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
