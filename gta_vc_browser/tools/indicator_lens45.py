"""Dónde vive la lente ámbar de cada coche (plan 45.0).

El plan 44.0 declaró ámbar solo lo que la media de color del nodo `indicator_*`
permitía. Medido sobre el árbol servido, eso deja fuera dos cosas que el jugador
ve en juego:

  * la lente ámbar que el modelador dejó en la carrocería (`wing_lf_hi_ok` del
    virgo, `chassis_hi` del washington) — el nodo existe pero su contenido no es
    una lente, así que el motor parpadea el faro o el piloto en su lugar;
  * la parte de la lentija que está dentro del nodo pero comparte el material con
    el piloto rojo, con lo que pintar el nodo pintaría también el piloto.

Esta herramienta mide el parque entero y emite el veredicto de cada racimo con
sus compuertas, para que el parche (`indicator_audit.lens_move`) y el generador
(`gen_indicator_lens.py`) trabajen sobre la lista y no sobre heurísticas.

Las reglas, todas con su medida en el plan 45.0:

  color     `textura x material` rasterizado sobre el footprint UV de la cara
            (`indicator_audit._face_render`), no un texel del centroide
  ámbar     `_is_amber` con el corte de matiz 15
  celda     es ámbar si dentro tiene un racimo de caras conectadas cuyo área
            ámbar es >= 50 % del área del racimo (la media de color mentía: el
            washington trasero sale rojo con la lente dentro; y medir la fracción
            sobre el atomic entero expulsaba lentes reales mezcladas con el
            piloto rojo del mismo nodo)
  racimos   componentes conexas por índice de vértice compartido, fuera de los
            nodos y sin `*_dam` ni `vlo`
  compuertas clase del campo 4 de default.ide (el parque entero vive en la
            seccion `cars`: ahi estan tambien las motos y los barcos), nombre
            (clearance*/servicelights = gáboro), `extra*`, área en la banda de
            lente real, |x| e |y| en el extremo del coche, z en la banda medida
  soldadura el racimo comparte vertices world (<=1 cm) con la lente del nodo ->
            es el trozo que falta en ESA lentija: las bandas de area y de
            posicion no se le aplican (son percentiles del parque y castigan
            modelos con la direccional alta o pegada al centro, que es justo el
            «faro encendido a medias» del jugador). Las de clase, nombre y frame
            siguen aplicando.
  decisión  hueco < 0,20 m con la lente del nodo del mismo eje y costado ->
            FUSIONAR; < 0,20 m con la del costado espejo -> FUSIONAR-par; nodo
            del eje sin lente medida -> TRASLADAR; nodo con lente y hueco mayor
            -> REFLEJO; sin nodo para ese eje -> NO-INVENTAR

Uso:
    python gta_vc_browser/tools/indicator_lens45.py --report
    python gta_vc_browser/tools/indicator_lens45.py --json salida.json
    python gta_vc_browser/tools/indicator_lens45.py --diff
    python gta_vc_browser/tools/indicator_lens45.py --model virgo --bands-from j.json \
        --json virgo.json                      # una sola fila, para probar el parche
    python gta_vc_browser/tools/indicator_lens45.py --move virgo.json --dry
    python gta_vc_browser/tools/indicator_lens45.py --move lens45c.json \
        --backup /c/tmp/iav42/pre45             # aplica la lista congelada (T4)
"""
import argparse
import collections
import io
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import indicator_audit as ia      # noqa: E402
import dff_geom as dg             # noqa: E402

NODES = ("indicator_lf", "indicator_rf", "indicator_lr", "indicator_rr",
         "indicator2_lf", "indicator2_rf", "indicator2_lr", "indicator2_rr")
MIRROR = {"indicator_lf": "indicator_rf", "indicator_rf": "indicator_lf",
          "indicator_lr": "indicator_rr", "indicator_rr": "indicator_lr",
          "indicator2_lf": "indicator2_rf", "indicator2_rf": "indicator2_lf",
          "indicator2_lr": "indicator2_rr", "indicator2_rr": "indicator2_lr"}
FILL_MIN = 0.50          # fraccion de area ambar que declara una celda ambar
HUE_COVER_MIN = 3        # texelos minimos para que la medida de una cara valga
GAP_FUSE = 0.20          # m: racimo y lente de nodo son la misma lentija
MOD_LENS_TEX = ("lights", "lightson")
# Los 25 modelos que el Vice Extended rediseño pintan su lente sobre un atlas
# blanco (`lights`) y el binario la tiñe cambiando el color del material: el
# pixel es blanco pero la lente es real y dibujada por el mod. Dentro de un
# nodo `indicator_*` esa cara cuenta como lente aunque el matiz no la declare
# (regla 2 del plan 42.1, que el 45.0 hereda). Fuera del nodo NO: una cara con
# textura `lights` en el capo es un faro, no una direccional, y ahi la regla
# del mod no aplica.


def pct(xs, q):
    xs = sorted(xs)
    if not xs:
        return 0.0
    return xs[min(len(xs) - 1, int(q * len(xs)))]


def rad(area_m2):
    """Radio de un circulo de la misma area: el hueco se descuenta a ambos lados."""
    return (max(area_m2, 0.0) / 3.141592653589793) ** 0.5


def types():
    """{modelo: clase de default.ide} -- el tipo manda en la compuerta de clase.

    El parque de VC mete EN LA MISMA SECCION `cars` los coches, las motos, los
    barcos y los aviones: la seccion no dice nada. La clase real es el campo 4 de
    la fila (`car`/`bike`/`boat`/`heli`/`plane`/`train`), que es el que lee
    CFileLoader::LoadVehicleObject. Usar la seccion convertia al faggio y al
    pizzaboy en `car` y les abria la compuerta de clase.
    """
    ide = os.path.join(REPO, "streamed", "data", "default.ide")
    clases = ("car", "bike", "boat", "heli", "plane", "train")
    out, section = {}, None
    for raw in io.open(ide, encoding="latin-1").read().replace("\r\n", "\n").split("\n"):
        line = raw.split("#")[0].strip()
        if not line:
            continue
        low = line.lower()
        if low in ("cars", "bikes", "boats", "planes", "helis", "trains"):
            section = low[:-1]
            continue
        if low == "end":
            section = None
            continue
        if section is None:
            continue
        campos = [c.strip() for c in line.split(",")]
        if len(campos) >= 4 and campos[0].isdigit():
            out[campos[1].lower()] = (campos[3].lower()
                                      if campos[3].lower() in clases else section)
    return out


def measure(model, node_only=False):
    """Caras del modelo con su area 3D y su color renderido (textura x material).

    Con `node_only` solo se miden los `indicator_*`: la primera pasada (las
    bandas de area y z) no necesita el resto del modelo y rasterizar el parque
    entero dos veces cuesta el doble.
    """
    buf, frames, geoms, atomics = ia.load_model(model, with_geoms=True)
    pix = ia.texture_pixels(model)
    if not pix:
        raise SystemExit(
            "%s: %s no tiene .txd legible; sin textura no hay color medido y "
            "toda la regla degenera a «no es ambar». Apunta --root al directorio "
            "que contiene los DFF y sus TXD." % (model, ia.BASE))
    cache = {}
    faces = []
    for fi, gi in atomics:
        fn = (frames[fi]["name"] or "").lower()
        if node_only and fn not in NODES:
            continue
        g = geoms[gi]
        if not g["tris"] or not g["uvs"]:
            continue
        world = dg.world_of(frames, fi)
        for ti, t in enumerate(g["tris"]):
            mi = t[3]
            if mi >= len(g["mats"]):
                continue
            col, tex = g["mats"][mi]
            area = _face_area(g, t)
            if area <= 0.0:
                continue
            hit = ia._face_render(g, pix, t, cache)
            rend = None
            arend = None
            pxn = 0
            amb = 0
            if hit:
                pxn, rend, amb, arend = hit
            if rend is None:
                rend = tuple(col[:3]) if col else (255, 255, 255)
            ctr = [sum(g["verts"][v][k] for v in t[:3]) / 3.0 for k in range(3)]
            faces.append({"fi": fi, "gi": gi, "ti": ti, "mi": mi, "frame": fn,
                          "v": [int(x) for x in t[:3]],
                          "verts": [(round(x, 4), round(y, 4), round(z, 4)) for x, y, z in
                                    (ia._xform(world, g["verts"][v]) for v in t[:3])],
                          "tex": (tex or "").lower(), "area": area, "px": pxn,
                          "rend": rend, "amb": amb, "arend": arend,
                          "mod": (tex or "").lower() in MOD_LENS_TEX,
                          "world": ia._xform(world, ctr)})
    return buf, frames, geoms, atomics, faces


def _face_area(g, t):
    v = g["verts"]
    try:
        a, b, c = [v[i] for i in t[:3]]
    except IndexError:
        return 0.0
    ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    wx, wy, wz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
    cx, cy, cz = uy * wz - uz * wy, uz * wx - ux * wz, ux * wy - uy * wx
    return 0.5 * (cx * cx + cy * cy + cz * cz) ** 0.5


def is_amber(c):
    return ia._is_amber((c[0], c[1], c[2], 255))


def amber_gates(c):
    """Las cuatro puertas de color de `_is_amber` sin la ventana de matiz."""
    r, g, b = c[0], c[1], c[2]
    return r >= 105 and b <= 75 and r - g >= 45 and g - b >= 30


LAMP_FRAMES = ("headlight", "taillight", "brakelight", "reversinglight",
               "indicatorlight", "signal")


def lamp_check(faces):
    """(caras de lampara nombrada, pasan las puertas de color, pasan con matiz).

    El corte de matiz 15 del plan 45.0 solo es seguro si ninguna cara de una
    lampara con nombre propio (faro, piloto, freno, marcha atras) pasa las
    puertas de color ni siquiera sin mirar el matiz: si pasara, el corte 22->15
    podria pintar de ambar una lampara que no lo es.
    """
    tot = gates = win = 0
    for f in faces:
        if not any(f["frame"].startswith(k) for k in LAMP_FRAMES):
            continue
        if f["frame"] in NODES:
            continue
        tot += 1
        if amber_gates(f["rend"]):
            gates += 1
            if is_amber(f["rend"]):
                win += 1
    return tot, gates, win


def axle_of(fn):
    return "F" if (fn.endswith("_lf") or fn.endswith("_rf")) else "R"


def side_of(x):
    return -1 if x < 0.0 else 1


SAT_MIN = 0.60
HUE_MIN = 15.0
HUE_MAX = 55.0


def is_dark_lens_face(f):
    """Cristal dibujado oscuro: matiz de lente y saturacion de lente, sin mirar el valor.

    `_is_amber` corta por `r >= 105` y por `g - b >= 30`, y la mitad del cristal
    de pony/rumpo/topfun esta pintada en un ambar de valor bajo ((95,41,14),
    (87,36,11), (90,38,12)): mismo matiz y misma saturacion que la cara que si
    pasa, mas oscura. El matiz y la saturacion separan el cristal del cromo
    (sat 0,00-0,04) y de los bajos (sat 0,22), que es donde el corte por
    porcentaje de texelas metia una cara de 1034 cm2.
    """
    r, g, b = f["rend"][:3]
    hi = max(r, g, b)
    if hi == 0 or not HUE_MIN <= ia._hue(r, g, b) <= HUE_MAX:
        return False
    return (hi - min(r, g, b)) / float(hi) >= SAT_MIN


def is_any_lens_face(f):
    """Cualquiera de las dos caras de lente: la medida por `_is_amber` o la oscura."""
    return is_lens_face(f) or is_dark_lens_face(f)


def is_lens_face(f):
    """Cara de lente dentro de un nodo: ambar medida, o atlas del mod."""
    return f["mod"] or (f["px"] >= HUE_COVER_MIN and is_amber(f["rend"]))


def lens_area(f):
    """Area 3D de la cara que es lente: la parte de texel ambar, o toda si es
    cara del atlas `lights` del mod (el mod la tiñe entera)."""
    if f["mod"]:
        return f["area"]
    if f["px"] >= HUE_COVER_MIN and is_amber(f["rend"]):
        return f["area"] * (f["amb"] / float(f["px"]))
    return 0.0


def _components(faces):
    """Componentes conexas por indice de vertice compartido."""
    par = list(range(len(faces)))

    def find(i):
        while par[i] != i:
            par[i] = par[par[i]]
            i = par[i]
        return i

    porv = collections.defaultdict(list)
    for i, f in enumerate(faces):
        for v in f["v"]:
            porv[(f["gi"], v)].append(i)      # el indice de vertice solo vale dentro de su geometry
    for vs in porv.values():
        for i in vs[1:]:
            x, y = find(vs[0]), find(i)
            if x != y:
                par[y] = x
    grupos = collections.defaultdict(list)
    for i, f in enumerate(faces):
        grupos[find(i)].append(f)
    return list(grupos.values())


def _cluster_stats(fs):
    """Area, area ambar, fraccion, caja y vertices de mundo de un racimo de caras."""
    area = sum(f["area"] for f in fs)
    amber = sum(lens_area(f) for f in fs)
    wv = [p for f in fs for p in f["verts"]]
    return {"caras": len(fs), "area": area, "amber": amber,
            "frac": (amber / area) if area else 0.0,
            "mod": sum(f["area"] for f in fs if f["mod"]),
            "cx": _mean([f["world"][0] for f in fs]),
            "cy": _mean([f["world"][1] for f in fs]),
            "cz": _mean([f["world"][2] for f in fs]),
            "xyz": [round(_mean([f["world"][k] for f in fs]), 3) for k in range(3)],
            "tris": [[f["ti"], f["mi"]] for f in fs],
            "mis": sorted({f["mi"] for f in fs}),
            "tex": sorted({(f["tex"] or "") for f in fs}),
            "amean": _mean_weighted(fs, lens_area, lambda f: f["arend"] or f["rend"]),
            "wv": wv,
            "bb": [(min(p[k] for p in wv), max(p[k] for p in wv)) for k in range(3)]}


def _mean_weighted(faces, weight, color=None):
    """Color medio de las caras ponderando por `weight` y leyendo `color`.

    El generador de la tabla pinta la lente, asi que la media que necesita es la
    de la **parte ambar**: peso = area ambar de la cara y color = la media de los
    texelos que pasan `_is_amber` (campo `arend`). La media de la cara entera
    (`rend`) mezcla el cristal ambar con el marco o el piloto del mismo triangulo
    y sale un tono que ya no es ambar.
    """
    color = color or (lambda f: f["rend"])
    acc = [0.0, 0.0, 0.0]
    tot = 0.0
    for f in faces:
        w = weight(f)
        tot += w
        c = color(f)
        for k in range(3):
            acc[k] += c[k] * w
    if tot <= 0.0:
        return _weighted_mean(faces)
    return [int(x / tot) for x in acc]


def _weld(a, b):
    """(vertices del racimo a sueldados con b, fraccion del area de a dentro de la caja de b).

    El modelador que parte una lentija entre dos atomics deja la costura en el
    MISMO punto del mundo: los vertices coinciden. Un brillo pintado junto a la
    lente no comparte ninguno y queda fuera de su caja (mrwhoop: 0 de 3 vertices
    y 0 % dentro, con el nodo ya ambar por si mismo -> REFLEJO).
    """
    n = 0
    for p in a["wv"]:
        if any(abs(p[0] - q[0]) <= 0.01 and abs(p[1] - q[1]) <= 0.01
               and abs(p[2] - q[2]) <= 0.01 for q in b["wv"]):
            n += 1
    bb = b["bb"]
    dentro = 0.0
    for f in a["faces"]:
        c = f["world"]
        if all(bb[k][0] - 0.02 <= c[k] <= bb[k][1] + 0.02 for k in range(3)):
            dentro += f["area"]
    return n, (dentro / a["area"]) if a["area"] else 0.0


def _soldado(a, b):
    """El racimo a es continuacion de la lente b: comparte costura o esta en su caja."""
    n, dentro = _weld(a, b)
    return n > 0 or dentro >= 0.5


def node_faces(faces):
    """Caras de los nodos agrupadas por (frame, costado).

    La celda es ambar cuando DENTRO tiene un racimo de lente cuya fraccion ambar
    pasa el corte: medir la fraccion sobre el atomic entero mezclaba la lente con
    el piloto rojo del mismo nodo y expulsaba lentes reales (stinger trasero: 75.8
    cm2 de lente ambar dentro de 174.6 cm2 de atomic = 32 %). Lo que no puede
    pintarse entero es el atomic, y eso lo resuelve el split de T4, no la tabla.
    """
    cells = collections.defaultdict(list)
    for f in faces:
        if f["frame"] in NODES:
            cells[(f["frame"], side_of(f["world"][0]))].append(f)
    out = {}
    for key, g in cells.items():
        tot = sum(f["area"] for f in g)
        lens = [f for f in g if is_lens_face(f)]
        cl = [_cluster_stats(fs) for fs in _components(lens)]
        best = max(cl, key=lambda c: c["amber"]) if cl else None
        out[key] = {
            "frame": key[0], "side": key[1], "axle": axle_of(key[0]),
            "faces": g, "area": tot, "lens": lens, "cl": cl, "best": best,
            "lens_area": sum(f["area"] for f in lens),
            "amber_area": sum(lens_area(f) for f in g),
            "mod_area": sum(f["area"] for f in lens if f["mod"]),
            "cell_frac": (sum(lens_area(f) for f in g) / tot) if tot else 0.0,
            "frac": best["frac"] if best else 0.0,
            "amber": bool(best and best["frac"] >= FILL_MIN),
        }
        ref = best or {"cx": _mean([f["world"][0] for f in g]),
                       "cy": _mean([f["world"][1] for f in g]),
                       "cz": _mean([f["world"][2] for f in g])}
        out[key]["cx"] = ref["cx"]
        out[key]["cy"] = ref["cy"]
        out[key]["cz"] = ref["cz"]
    return out


def _mean(xs):
    return sum(xs) / len(xs) if xs else 0.0


def clusters(faces):
    """Racimos de caras ambar fuera de los nodos (componentes por vertice)."""
    by_geom = collections.defaultdict(list)
    for f in faces:
        if f["frame"] in NODES or "_dam" in f["frame"] or "vlo" in f["frame"]:
            continue
        if any(f["frame"].startswith(k) for k in LAMP_FRAMES):
            continue      # faro/piloto con nombre propio: no es una direccional
        if f["mod"]:          # faro/piloto del atlas del mod: no es direccional
            continue
        if f["px"] < HUE_COVER_MIN or not is_amber(f["rend"]):
            continue
        by_geom[(f["fi"], f["gi"])].append(f)
    out = []
    for (_fi, _gi), fs in by_geom.items():
        for gg in _components(fs):
            st = _cluster_stats(gg)
            st.update({"frame": gg[0]["frame"], "fi": gg[0]["fi"], "gi": gg[0]["gi"],
                       "mi": gg[0]["mi"], "tex": gg[0]["tex"], "faces": gg,
                       "n": st["caras"],
                       "amb": sum(f["amb"] for f in gg),
                       "px": sum(f["px"] for f in gg)})
            out.append(st)
    return out


def extent(frames, geoms, atomics):
    """Semiancho y semilargo del modelo sin el LOD, para las compuertas de extremidad."""
    xs, ys = [], []
    for fi, gi in atomics:
        if "vlo" in (frames[fi]["name"] or "").lower():
            continue
        w = dg.world_of(frames, fi)
        for v in geoms[gi]["verts"]:
            p = ia._xform(w, v)
            xs.append(p[0])
            ys.append(p[1])
    if not xs:
        return None
    return {"hw": (max(xs) - min(xs)) / 2.0, "hl": (max(ys) - min(ys)) / 2.0}


def node_frames(model):
    """Los `indicator_*` que de verdad existen en el DFF (el mod no invento nodos)."""
    _buf, frames, _atomics = ia.load_model(model)
    return set((f["name"] or "").lower() for f in frames) & set(NODES)


def bands(fleet):
    """Bandas de area y de z que salen de las lentes medidas dentro de su nodo."""
    ref_area, ref_z = [], []
    for m in fleet:
        try:
            faces = measure(m, node_only=True)[4]
        except Exception:
            continue
        for c in node_faces(faces).values():
            if c["best"] and c["frac"] >= FILL_MIN:
                ref_area.append(c["best"]["area"])
                ref_z.append(c["best"]["cz"])
    return {"area_lo": pct(ref_area, .05) * 0.55, "area_hi": pct(ref_area, .95) * 1.35,
            "z_lo": pct(ref_z, .02) - 0.35, "z_hi": pct(ref_z, .98) + 0.35,
            "ref_n": len(ref_area)}


def review(model, tmap, bd):
    """Veredicto de cada racimo ambar del modelo, con la compuerta que lo corta."""
    buf, frames, geoms, atomics, faces = measure(model)
    cells = node_faces(faces)
    cls = tmap.get(model, "?")
    ext = extent(frames, geoms, atomics)
    have = node_frames(model)
    res = {"model": model, "type": cls, "cells": [], "clusters": [],
           "lamp": lamp_check(faces),
           "lamp_amber": ["%s/%s tex=%s color=%s"
                          % (model, f["frame"], f["tex"], f["rend"])
                          for f in faces
                          if any(f["frame"].startswith(k) for k in LAMP_FRAMES)
                          and f["frame"] not in NODES and amber_gates(f["rend"])],
           "mod_out": [f["frame"] for f in faces
                       if f["mod"] and f["frame"] not in NODES
                       and "_dam" not in f["frame"] and "vlo" not in f["frame"]]}
    for key in sorted(cells):
        c = cells[key]
        res["cells"].append({"frame": c["frame"], "side": c["side"], "axle": c["axle"],
                             "area_cm2": round(1e4 * c["area"], 1),
                             "lens_cm2": round(1e4 * c["lens_area"], 1),
                             "amber_cm2": round(1e4 * c["amber_area"], 1),
                             "mod_cm2": round(1e4 * c["mod_area"], 1),
                             "frac": round(c["frac"], 3),
                             "cell_frac": round(c["cell_frac"], 3),
                             "lente_cm2": round(1e4 * c["best"]["area"], 1) if c["best"] else 0.0,
                             "racimos_lente": len(c["cl"]),
                             "cx": round(c["cx"], 3), "cy": round(c["cy"], 3),
                             "cz": round(c["cz"], 3),
                             "amber": c["amber"],
                             "caras": len(c["faces"]),
                             "mixed": _mixed_material(c),
                             "mean": _weighted_mean(c["faces"])})
    for c in sorted(clusters(faces), key=lambda x: -x["area"]):
        res["clusters"].append(_verdict(model, c, cells, ext, have, cls, bd))
    res["ops"] = _ops(cells, res["clusters"])
    return res


def _mixed_groups(cell):
    """Grupos (geometry, material) del nodo que tienen caras de lente y caras que no.

    La clave lleva la geometry porque el material vive DENTRO de su geometria: dos
    atomics con el mismo indice de material son instancias distintas y pintar uno
    no toca al otro. Con la clave (material, textura) el repaso contaba 3321,5
    cm2 como mixtos; medida la instancia, 281,2 cm2 de esos estan en otro atomic y
    ya estaban a salvo.
    """
    by = collections.defaultdict(list)
    for f in cell["faces"]:
        by[(f["gi"], f["mi"], f["tex"])].append(f)
    out = []
    for k in sorted(by):
        fs = by[k]
        lente = [f for f in fs if is_lens_face(f)]
        otras = [f for f in fs if not is_lens_face(f)]
        if lente and otras:
            out.append((k, lente, otras))
    return out


def _mixed_material(cell):
    """Materiales del nodo que tienen caras de lente y caras que no lo son."""
    return [{"tex": k[2], "lente": len(lente), "otras": len(otras),
             "cm2_otras": round(1e4 * sum(f["area"] for f in otras), 1)}
            for k, lente, otras in _mixed_groups(cell)]


def _ops(cells, verdicts):
    """Las operaciones que T4 aplica sobre el DFF, en el orden de la medida.

    `lens` lleva la lente medida de la carroceria al nodo que dicta el veredicto.
    `panel` saca del nodo las caras que no son lente, con dos motivos medidos:

      * `material` -- compartian el material de esa lente dentro de la misma
        geometry: pintar el material pintaria el panel (las celdas mixtas del
        repaso);
      * `destino`  -- la celda recibe la lente en atomic aparte, asi que el panel
        que ya estaba dentro sigue siendo un atomic del nodo y el motor le sube el
        ambient en cada destello (virgo lf: 344,6 cm2 de panel sobre 417,8 de
        atomic sin ninguna cara de material mixto).

    El invariante del plan es «el atomic bajo `indicator_*` solo lleva caras de
    lente» y se exige en las celdas que este plan enciende; las que ya estaban
    encendidas por los planes anteriores se resuelven con la mascara de
    materiales medida en T5, no moviendo su malla.
    """
    ops = [{"kind": "lens", "nodo": c["nodo"], "fi": c["fi"], "gi": c["gi"],
            "side": c["side"], "axle": c["axle"], "caras": c["caras"],
            "cm2": c["cm2"], "tris": c["tris"], "mean": c["mean"],
            "tex": c["tex"], "xyz": c["xyz"], "v": c["v"]}
           for c in verdicts if c["v"] in ACTION and c["nodo"]]
    destino = set((o["nodo"], o["side"]) for o in ops)
    for key in sorted(cells):
        cell = cells[key]
        by = collections.defaultdict(list)
        for f in cell["faces"]:
            by[(f["gi"], f["mi"], f["tex"])].append(f)
        for k in sorted(by):
            lente = [f for f in by[k] if is_lens_face(f)]
            otras = [f for f in by[k] if not is_lens_face(f)]
            if not otras:
                continue
            if lente:
                motivo = "material"
            elif key in destino:
                motivo = "destino"
            else:
                continue
            ops.append({"kind": "panel", "motivo": motivo, "nodo": cell["frame"],
                        "fi": otras[0]["fi"], "gi": k[0], "side": cell["side"],
                        "mi": k[1], "tex": k[2], "caras": len(otras),
                        "cm2": round(1e4 * sum(f["area"] for f in otras), 1),
                        "tris": [[f["ti"], f["mi"]] for f in otras],
                        "mean": _weighted_mean(otras)})
    return ops


def _weighted_mean(faces):
    acc = [0.0, 0.0, 0.0]
    tot = 0.0
    for f in faces:
        tot += f["area"]
        for k in range(3):
            acc[k] += f["rend"][k] * f["area"]
    return [int(x / tot) for x in acc] if tot else [0, 0, 0]


def _verdict(model, c, cells, ext, have, cls, bd):
    axle = "F" if c["cy"] > 0.0 else "R"
    side = side_of(c["cx"])
    area_cm2 = 1e4 * c["area"]
    fails = []
    if cls != "car":
        fails.append("TIPO=%s" % cls)
    if c["frame"].startswith("extra"):
        fails.append("FRAME=extra")
    if c["frame"].startswith("clearance") or c["tex"] == "servicelights":
        fails.append("NOMBRE=%s/%s (gaboro declarado por el modelo)" % (c["frame"], c["tex"]))

    lente = [(gap_cc(c, k), x, k) for x in cells.values() for k in x["cl"]
             if x["side"] == side and x["axle"] == axle and k["frac"] >= FILL_MIN]
    espejo = [(gap_cc(c, k), x, k) for x in cells.values() for k in x["cl"]
              if x["side"] == -side and x["axle"] == axle and k["frac"] >= FILL_MIN]
    # Hueco y continuidad: un hueco pequeno dice que el parche y la lente del nodo
    # son el mismo sitio; la soldadura dice si de verdad son la misma lentija
    # partida o un brillo pintado encima.
    g_own, C_own, K_own = _mejor(lente, c)
    g_mir, C_mir, K_mir = _mejor(espejo, c)
    w = _weld(c, K_own) if K_own is not None else (0, 0.0)
    # Las bandas de area y de posicion son percentiles del parque, y un modelo
    # puede tener su direccional fuera de ellas sin ser un parche de carroceria:
    # el mrwhoop lleva sus pilotos traseros a z=+1,41 y su propia lente de nodo
    # esta a esa altura, y la lentija del moonbeam queda a |x|=0,58 del semiancho
    # del coche. Cuando el racimo COMPARTE VERTICES world con la lente del nodo
    # (continuidad de malla demostrada, no proximidad) es el trozo que falta en
    # ESA lentija, y castigarlo por la banda deja el faro encendido a medias: la
    # queja D2 del jugador. Las bandas no se le aplican, pero se registran.
    soldado = w[0] > 0
    bandas = []
    if area_cm2 < 1e4 * bd["area_lo"]:
        bandas.append("AREA=%.0f<%.0f" % (area_cm2, 1e4 * bd["area_lo"]))
    elif area_cm2 > 1e4 * bd["area_hi"]:
        bandas.append("AREA=%.0f>%.0f" % (area_cm2, 1e4 * bd["area_hi"]))
    if ext:
        if abs(c["cx"]) / ext["hw"] < 0.62:
            bandas.append("|x|=%.2f*ancho" % (abs(c["cx"]) / ext["hw"]))
        if abs(c["cy"]) / ext["hl"] < 0.62:
            bandas.append("|y|=%.2f*largo" % (abs(c["cy"]) / ext["hl"]))
    if c["cz"] < bd["z_lo"] or c["cz"] > bd["z_hi"]:
        bandas.append("Z=%.2f" % c["cz"])
    if not soldado:
        fails += bandas
    lr = "l" if side < 0 else "r"
    fp = "f" if axle == "F" else "r"
    nodo_eje = [n for n in ("indicator_%s%s" % (lr, fp), "indicator2_%s%s" % (lr, fp))
               if n in have]
    if fails:
        v = "FUERA"
    elif g_own < GAP_FUSE and K_own is not None and _soldado(c, K_own):
        v = "FUSIONAR"
    elif g_mir < GAP_FUSE and K_mir is not None and _soldado(c, K_mir):
        v = "FUSIONAR-par"
    elif nodo_eje:
        v = "TRASLADAR" if C_own is None else "REFLEJO"
    else:
        v = "NO-INVENTAR"
    target = (C_own["frame"] if v in ("FUSIONAR", "REFLEJO") and C_own
              else (MIRROR.get(C_mir["frame"]) if v == "FUSIONAR-par" and C_mir
                    else (nodo_eje[0] if v == "TRASLADAR" else None)))
    return {"frame": c["frame"], "fi": c["fi"], "gi": c["gi"], "mi": c["mi"],
            "tex": c["tex"], "axle": axle, "side": side, "caras": c["n"],
            "cm2": round(area_cm2, 1), "hueco_own": round(g_own, 2),
            "hueco_mir": round(g_mir, 2), "nodo": target, "v": v, "fails": fails,
            "soldadura": [w[0], round(100 * w[1], 1)],
            "bandas_no_aplicadas": (bandas if soldado else []),
            "geom": ("eximida por soldadura" if soldado else "bandas aplicadas"),
            "xyz": [round(c["cx"], 3), round(c["cy"], 3), round(c["cz"], 3)],
            "tris": [[f["ti"], f["mi"]] for f in c["faces"]],
            "mean": _weighted_mean(c["faces"])}


def _mejor(parejas, c):
    """La lente del nodo mas cercana AL RACIMO, no al centroide del atomic.

    Las parejas vienen ordenadas por hueco; se reordena por distancia cruda porque
    el hueco descuenta radios y dos lentes de tamano distinto no son comparables.
    """
    if not parejas:
        return 99.0, None, None
    d, x, k = min(parejas, key=lambda p: ((p[2]["cx"] - c["cx"]) ** 2 +
                                          (p[2]["cy"] - c["cy"]) ** 2 +
                                          (p[2]["cz"] - c["cz"]) ** 2) ** 0.5)
    return d, x, k


def gap_cc(a, b):
    """Distancia entre racimos descontando los radios de ambos.

    Se compara el racimo de carroceria con CADA racimo de lente del nodo, no con
    el centroide del nodo entero: un atomic con dos lentes (la ambar y una franja
    blanca) tiene un centroide que no esta en ninguna de las dos.
    """
    d = ((a["cx"] - b["cx"]) ** 2 + (a["cy"] - b["cy"]) ** 2 +
         (a["cz"] - b["cz"]) ** 2) ** 0.5
    return d - rad(a["area"]) - rad(b["area"])


def fleet(root):
    ia.BASE = root
    ia.MODELS = root
    return ia.vehicle_models()


def _move(path, names, backup, dry):
    """Aplica la lista congelada de `ops` con `indicator_audit.lens_move`.

    La lista es la prueba: si el DFF cambio desde la medida, el indice de cara o
    su material ya no cuadran y el parche se niega en lugar de mover otra cara.
    """
    d = json.load(io.open(path, encoding="utf-8"))
    n = lente = panel = 0
    for r in d["models"]:
        m = r["model"]
        if names and m not in names:
            continue
        ops = r.get("ops") or []
        if not ops:
            continue
        ia.lens_move_apply(m, ops, dry=dry, backup=backup)
        n += 1
        lente += sum(1 for o in ops if o["kind"] == "lens")
        panel += sum(1 for o in ops if o["kind"] == "panel")
    print("%s: %d modelos, %d ops de lente, %d de panel"
          % ("(dry)" if dry else "parcheados", n, lente, panel))
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(REPO, "streamed", "models", "gta3.img"))
    ap.add_argument("--model", action="append", default=[])
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--json", default=None)
    ap.add_argument("--move", default=None, metavar="JSON",
                    help="aplica las ops del JSON con indicator_audit.lens_move")
    ap.add_argument("--backup", default=None, metavar="DIR",
                    help="donde se guardan los DFF antes de tocarlos")
    ap.add_argument("--dry", action="store_true", help="con --move: no escribe")
    ap.add_argument("--diff", default=None, help="tabla .h servida para comparar")
    ap.add_argument("--frozen", default=None,
                    help="lista congelada del repaso (review45d.json) para cotejarla")
    ap.add_argument("--bands-from", default=None, metavar="JSON",
                    help="reuse las bandas de un lens45.json ya medido: las compuertas "
                         "de area y z del parque entero, para repasar modelos sueltos")
    a = ap.parse_args()
    ia.BASE = a.root

    if a.move:
        return _move(a.move, [m.lower() for m in a.model], a.backup, a.dry)

    names = [m.lower() for m in a.model] if a.model else fleet(a.root)
    tmap = types()
    if a.bands_from:
        bd = json.load(io.open(a.bands_from, encoding="utf-8"))["bands"]
        print("bandas tomadas de %s" % a.bands_from)
    else:
        bd = bands(sorted(set(names)))
    print("bandas: area [%.0f, %.0f] cm2   z [%.2f, %.2f]   (%d celdas de referencia)"
          % (1e4 * bd["area_lo"], 1e4 * bd["area_hi"], bd["z_lo"], bd["z_hi"], bd["ref_n"]))
    print("corte de matiz %s   celda ambar si su racimo de lente tiene >= %.0f%% "
          "de area ambar" % ("15", 100 * FILL_MIN))

    data = []
    for m in sorted(set(names)):
        try:
            data.append(review(m, tmap, bd))
        except SystemExit as e:
            print("  AVISO %s: %s" % (m, e))
        except Exception as e:
            print("  AVISO %s: %s: %s" % (m, type(e).__name__, e))

    act = [c for r in data for c in r["clusters"] if c["v"] in
           ("FUSIONAR", "FUSIONAR-par", "TRASLADAR")]
    cnt = collections.Counter(c["v"] for r in data for c in r["clusters"])
    print("\nveredictos: %s" % json.dumps(dict(cnt)))
    print("ACCION: %d racimos en %d modelos (%d caras, %.0f cm2)"
          % (len(act), len(set(r["model"] for r in data for c in r["clusters"]
                                if c["v"] in ("FUSIONAR", "FUSIONAR-par", "TRASLADAR"))),
             sum(c["caras"] for c in act), sum(c["cm2"] for c in act)))
    cells = [c for r in data for c in r["cells"]]
    print("celdas de nodo: %d   ambar por fraccion: %d   con lente medida: %d   "
          "mixtas (lente + panel en el mismo material): %d"
          % (len(cells), sum(1 for c in cells if c["amber"]),
             sum(1 for c in cells if c["lens_cm2"] > 0),
             sum(1 for c in cells if c["mixed"])))
    act_models = sorted(set(r["model"] for r in data for c in r["clusters"]
                            if c["v"] in ("FUSIONAR", "FUSIONAR-par", "TRASLADAR")))
    print("modelos de accion (%d): %s" % (len(act_models), ", ".join(act_models)))
    ops = [(r["model"], o) for r in data for o in r["ops"]]
    por_modelo = sorted(set(m for m, _o in ops))
    for kind in ("lens", "panel"):
        ks = [o for _m, o in ops if o["kind"] == kind]
        print("ops %s: %d en %d modelos, %d caras, %.1f cm2"
              % (kind, len(ks), len(set(m for m, o in ops if o["kind"] == kind)),
                 sum(o["caras"] for o in ks), sum(o["cm2"] for o in ks)))
    print("modelos que se parchean en T4 (%d): %s"
          % (len(por_modelo), ", ".join(por_modelo)))
    lamp = [sum(r["lamp"][i] for r in data) for i in range(3)]
    cand_lamp = [c for r in data for c in r["clusters"] if c["v"] in ACTION
                 and any(c["frame"].startswith(k) for k in LAMP_FRAMES)]
    print("caras de lampara nombrada (faro/piloto/freno/marcha-atras): %d   pasan las "
          "puertas de color: %d   pasan con matiz>=15: %d   y de esas, racimos de accion "
          "montados sobre una lampara con nombre: %d (clusters() descarta el frame por nombre)"
          % (lamp[0], lamp[1], lamp[2], len(cand_lamp)))
    sucias = collections.Counter(f for r in data for f in r["lamp_amber"])
    if sucias:
        print("   las que pasan las puertas (fuera del corte por matiz): %s"
              % ", ".join("%s x%d" % kv for kv in sucias.most_common(8)))
    fuera_mod = collections.Counter(f for r in data for f in r["mod_out"])
    print("caras con atlas del mod fuera de nodo: %d en %d modelos (no cuentan como "
          "lente: %s)"
          % (sum(fuera_mod.values()), len(fuera_mod),
             ", ".join("%s x%d" % k for k in fuera_mod.most_common(6))))

    if a.report:
        for r in data:
            if not r["clusters"] and not r["cells"]:
                continue
            if any(c["v"] != "FUERA" for c in r["clusters"]) or any(c["amber"] for c in r["cells"]):
                print("\n%s (%s)" % (r["model"], r["type"]))
                for c in r["cells"]:
                    if c["lens_cm2"] > 0 or c["amber"]:
                        print("   nodo %-13s lado%s atomic %7.1f cm2  lente %6.1f "
                              "(mod %6.1f) racimos %d  ambar del racimo %5.1f%%  "
                              "del atomic %5.1f%%%s  mixta=%s"
                              % (c["frame"], "L" if c["side"] < 0 else "R", c["area_cm2"],
                                 c["lens_cm2"], c["mod_cm2"], c["racimos_lente"],
                                 100 * c["frac"], 100 * c["cell_frac"],
                                 " AMBAR" if c["amber"] else "",
                                 ",".join(x["tex"] for x in c["mixed"]) or "-"))
                for c in r["clusters"]:
                    if c["v"] == "FUERA":
                        continue
                    print("   %-12s %-18s %6.1f cm2 c=%-2d (%+.2f,%+.2f,%+.2f) -> %s %s"
                          % (c["v"], c["frame"], c["cm2"], c["caras"], c["xyz"][0],
                             c["xyz"][1], c["xyz"][2], c["nodo"] or "-", c["tex"]))
                    print("      hueco propio %+.2f  espejo %+.2f  soldadura %d vertex, "
                          "%.0f%% dentro de la caja de la lente"
                          % (c["hueco_own"], c["hueco_mir"], c["soldadura"][0],
                             c["soldadura"][1]))
                    if c["bandas_no_aplicadas"]:
                        print("      (sus bandas del parque habrian dicho: %s)"
                              % "; ".join(c["bandas_no_aplicadas"]))
                    if c["fails"]:
                        print("      descartado por: %s" % "; ".join(c["fails"]))

    if a.frozen:
        _frozen_cmp(a.frozen, data)

    if a.diff:
        _diff(a.diff, data)

    if a.json:
        io.open(a.json, "w", encoding="utf-8").write(json.dumps(
            {"version": 1, "bands": {k: (round(v, 5) if isinstance(v, float) else v)
                                     for k, v in bd.items()},
             "fill_min": FILL_MIN, "hue_cut": 15.0, "models": data}, indent=1))
        print("\nJSON: %s" % a.json)
    return 0


ACTION = ("FUSIONAR", "FUSIONAR-par", "TRASLADAR")


def _nearest(lst, x, y, tol=0.30):
    """Racimo nuevo mas cercano al sitio del racimo congelado.

    El cotejo no puede ir por clave cuantizada: corregir el raster de
    `_face_render` cambia QUE caras son ambar, y con ellas el centroide se mueve
    unos centimetros. Emparejar por proximidad (0.30 m, menos del hueco de
    fusion) deja que cada racimo congelado encuentre su sucesor aunque haya
    crecido, y que lo que no encuentre sucesor se reporte como salida real.
    """
    best = None
    bd = tol
    for c in lst:
        d = ((c["xyz"][0] - x) ** 2 + (c["xyz"][1] - y) ** 2) ** 0.5
        if d < bd:
            bd = d
            best = c
    return best


def _frozen_cmp(path, data):
    """Cotejo con la lista congelada del repaso (T3): que entro, que salio y por que."""
    old = json.load(io.open(path, encoding="utf-8"))
    by_mf = {}
    for r in data:
        for c in r["clusters"]:
            by_mf.setdefault((r["model"], c["frame"]), []).append(c)
    act_old = [e for e in old if e["v"] in ACTION]
    act_new = [(r["model"], c) for r in data for c in r["clusters"] if c["v"] in ACTION]
    keeps = outs = news = dups = crece = 0
    taken = set()
    print("\ncotejo con %s: %d racimos de accion congelados, %d de esta pasada"
          % (path, len(act_old), len(act_new)))
    for e in act_old:
        c = _nearest(by_mf.get((e["m"], e["f"]), []), e["x"], e["y"])
        if c is None:
            outs += 1
            print("   SALE   %-10s %-18s %6.1f cm2 c=%-2d era %s -> sin racimo nuevo a 0.30 m"
                  % (e["m"], e["f"], e["ar"], e["caras"], e["v"]))
            continue
        if c["v"] not in ACTION:
            outs += 1
            print("   SALE   %-10s %-18s %6.1f cm2 c=%-2d era %s -> ahora %s (%s)"
                  % (e["m"], e["f"], e["ar"], e["caras"], e["v"], c["v"],
                     "; ".join(c["fails"]) or "-"))
            continue
        if id(c) in taken:
            dups += 1
            print("   DUP    %-10s %-18s %6.1f cm2: cae en el mismo racimo nuevo (%.1f cm2)"
                  % (e["m"], e["f"], e["ar"], c["cm2"]))
            continue
        taken.add(id(c))
        keeps += 1
        if abs(c["cm2"] - e["ar"]) > 0.20 * max(e["ar"], 1.0):
            crece += 1
            print("   CAMBIA %-10s %-18s c=%d->%d  %6.1f -> %6.1f cm2  %s"
                  % (e["m"], e["f"], e["caras"], c["caras"], e["ar"], c["cm2"], c["v"]))
    for model, c in act_new:
        if id(c) in taken:
            continue
        news += 1
        e = _nearest([{"xyz": [x["x"], x["y"], x["z"]], "v": x["v"], "cm2": x["ar"]}
                      for x in old if x["m"] == model and x["f"] == c["frame"]],
                     c["xyz"][0], c["xyz"][1])
        print("   ENTRA  %-10s %-18s %6.1f cm2 c=%-2d %s -> %s   (congelado: %s %.1f cm2)"
              % (model, c["frame"], c["cm2"], c["caras"], c["v"], c["nodo"] or "-",
                 e["v"] if e else "no habia", e["cm2"] if e else 0.0))
    print("   coinciden %d (%d con area distinta >20%%), salen %d, entran %d, "
          "duplicados %d" % (keeps, crece, outs, news, dups))
    sin_pareja = []
    for model, c in act_new:
        if any(o["side"] == -c["side"] and o["axle"] == c["axle"]
               and abs(abs(o["xyz"][0]) - abs(c["xyz"][0])) <= 0.12
               and abs(o["xyz"][1] - c["xyz"][1]) <= 0.15
               for m2, o in act_new if m2 == model):
            continue
        sin_pareja.append("%s/%s lado%+d (%+.2f,%+.2f)"
                          % (model, c["frame"], c["side"], c["xyz"][0], c["xyz"][1]))
    print("   racimos de accion sin pareja en el costado espejo: %d   %s"
          % (len(sin_pareja), "; ".join(sin_pareja) or "-"))


def _diff(path, data):
    """Que filas de la tabla servida cambian con la medida nueva, y por que.

    La tabla servida la hizo el plan 44.0 con la media de color del nodo; cada
    salida hay que poder enseñarla con el numero que la expulsa (la fraccion de
    area) y cada entrada con el racimo que la trae.
    """
    cur = re_rows(io.open(path, encoding="utf-8").read())
    new = {}
    why = {}
    carroceria = {}
    for r in data:
        for c in r["cells"]:
            new[(r["model"], c["frame"], c["side"])] = c["amber"]
            why[(r["model"], c["frame"], c["side"])] = (
                "nodo: racimo de lente %.1f cm2 al %.0f%% de ambar, atomic de %.1f cm2"
                % (c["lente_cm2"], 100 * c["frac"], c["area_cm2"]))
        for c in r["clusters"]:
            if c["v"] in ACTION and c["nodo"]:
                k = (r["model"], c["nodo"], c["side"])
                new[k] = True
                # Un nodo puede recibir mas de un racimo (washing trasero: 155,1 +
                # 11,2 cm2). La prueba de la entrada es el total, no el ultimo.
                a = carroceria.setdefault(k, [c["v"], 0.0, 0])
                a[1] += c["cm2"]
                a[2] += c["caras"]
    for k, (v, cm2, caras) in carroceria.items():
        why[k] = "%s: %.1f cm2 de carroceria (%d caras)" % (v, cm2, caras)
    entra = sorted(k for k in new if new[k] and not cur.get(k))
    sale = sorted(k for k in cur if cur[k] and not new.get(k))
    print("\ndiff contra %s: %d celdas servidas ambar, %d celdas medidas"
          % (path, sum(1 for v in cur.values() if v), len(new)))
    print("   entran %d" % len(entra))
    for k in entra:
        print("      + %-10s %-13s lado%+d   %s" % (k[0], k[1], k[2], why.get(k, "-")))
    print("   salen %d" % len(sale))
    for k in sale:
        print("      - %-10s %-13s lado%+d   %s" % (k[0], k[1], k[2],
                                                    why.get(k, "sin celda medida")))


ROW = re.compile(r'^\s*\{\s*"([^"]+)"\s*,\s*(\w+)\s*,\s*(-?\d+)\s*,\s*\{\s*(\d+),'
                 r'\s*(\d+)\s*,\s*(\d+)\s*\}\s*,\s*(\d+)', re.M)


def re_rows(txt):
    """{(modelo, nodo, costado): amber} leido de la tabla .h generada.

    Desde el plan 45.0 una celda tiene tantas filas como racimos de lente, y la
    celda es ambar si CUALQUIERA de sus racimos lo es: con `=` el veredicto se lo
    quedaba la ultima fila leida y un racimo rojo registrado despues de uno ambar
    borraba el ambar del diff. La cola del patron es abierta porque la fila lleva
    campos detras del amber (mascara de materiales, area y centroide).
    """
    out = {}
    for m in ROW.finditer(txt):
        model, enum, side, _r, _g, _b, amber = m.groups()
        node = ENUM2NODE.get(enum)
        if node:
            key = (model, node, int(side))
            out[key] = out.get(key, False) or int(amber) == 1
    return out


ENUM2NODE = {
    "CAR_INDICATOR_LF": "indicator_lf", "CAR_INDICATOR_RF": "indicator_rf",
    "CAR_INDICATOR_LR": "indicator_lr", "CAR_INDICATOR_RR": "indicator_rr",
    "CAR_INDICATOR_2_LF": "indicator2_lf", "CAR_INDICATOR_2_RF": "indicator2_rf",
    "CAR_INDICATOR_2_LR": "indicator2_lr", "CAR_INDICATOR_2_RR": "indicator2_rr",
}


if __name__ == "__main__":
    main()
