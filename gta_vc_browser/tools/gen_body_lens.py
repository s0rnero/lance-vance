#!/usr/bin/env python3
"""Genera src/vehicles/BodyLensTable.h y los atlas de lente derivados (plan 58.0 T3/T4).

Hay focos ambar reales cuya cara NO esta dentro de ningun nodo `indicator_*`:
viven pintados en un panel de carroceria (chassis_hi, wing_*_hi_ok,
bump_front_hi_ok) y COMPARTEN material con la pintura del coche. Mover la cara,
crear un hueso o tintar el material esta prohibido por el jugador (55.0/56.0
revertidos). La salida aprobada es la del plan 46.0: un atlas derivado
`<atlas>_lens` que SOLO difiere del original en las texelas ambar del footprint
de las lentes aprobadas; el motor cambia la textura del material en el flanco
encendido y, con la luz apagada, el coche se ve bit a bit como hoy.

La version anterior de esta herramienta emitia 0 filas porque exigia >= 75 %
del area de TODO el material en la geometry: un material compartido con pintura
nunca pasa. Esta version mide por RACIMO (caras conectadas por vertice dentro
del mismo frame/material/geometry, regla de indicator_lamp_anchor.py) y califica
el racimo por su distancia al ancla de faro/piloto del DFF:

  * sitio de cara completa: el racimo es `lente` para indicator_lamp_anchor
    (<= 0.50 m del ancla de su eje y costado) y >= 75 % de su area es ambar.
  * excepciones parciales aprobadas por el jugador: mule delante (chassis_hi,
    43 %) y mrwhoop delante (chassis_hi, 69 %, 58.1). Entran explicitas por
    modelo+frame+eje aunque su cobertura no llegue al 75 %: solo se encienden
    las texelas AMBAR bajo el footprint de esa cara, nunca el panel alrededor.

Solo se emiten filas para los sitios aprobados (esperant detras, boxville
delante, fbiranch delante aletazos+parachoques y detras, mule delante, mrwhoop
delante). El centroide medido tiene que caer a <= 0.20 m del sitio aprobado:
asi el racimo de al lado (el piloto, el marcador, la lentija de abajo del
esperant) no entra.

El atlas derivado se emite en el MISMO formato que su hermana (RGB565 sin
comprimir en estos modelos): se editan solo los words de las texelas ambar del
footprint, el resto del nivel 0 queda bit a bit identico. Los atlas DXT1 del
46.0 no se tocan. La inyeccion en los TXD servidos la hace tools/txd_merge.py
(plan 46.0 T2), no este script.

Uso:
    python gta_vc_browser/tools/gen_body_lens.py                 # tabla + atlas
    python gta_vc_browser/tools/gen_body_lens.py --check         # no escribe
    python gta_vc_browser/tools/gen_body_lens.py --atlas-dir DIR --json F.json
"""
import argparse
import collections
import io
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
ROOT = os.path.join(REPO, "streamed", "models", "gta3.img")
SRC = os.path.join(os.path.dirname(REPO), "src", "vehicles", "BodyLensTable.h")
sys.path.insert(0, HERE)

import indicator_audit as ia        # noqa: E402
import indicator_lens45 as L        # noqa: E402
import indicator_lamp_anchor as A   # noqa: E402
import gen_lens_atlas as G          # noqa: E402
import txd_raster as tr             # noqa: E402

NODE = {"F": {-1: "CAR_INDICATOR_LF", 1: "CAR_INDICATOR_RF"},
        "R": {-1: "CAR_INDICATOR_LR", 1: "CAR_INDICATOR_RR"}}
SITE_TOL = 0.20         # m: el centroide del racimo cae en el sitio aprobado
MIN_FACE = 0.75         # cobertura de area ambar para un sitio de cara completa
GLOW = 1.9              # factor de brillo de la texela ambar encendida (conserva matiz)
FRAC_FACE = 0.25        # suelo por cara para entrar en un racimo (indicator_lamp_anchor)

# Los sitios del plan 58.0 mas mrwhoop delante (58.1). `xyz` es el centroide
# aprobado por COSTADO (el signo de x se aplica al medir); `partial` = excepcion
# del jugador sin puerta de cobertura. frame + eje + costado + centroide
# identifican el racimo. trash salio en 58.1: su fila de carroceria apuntaba al
# stop rojo del porton (z=+1.44); sus 4 intermitentes reales ya los pinta el nodo.
SITES = {
    "esperant": [("chassis_hi", "R", (-0.92, -2.96, 0.01), False)],
    "boxville": [("chassis_hi", "F", (-0.88, 3.10, 0.23), False)],
    "fbiranch": [("wing_lf_hi_ok", "F", (-1.12, 3.10, 0.05), False),
                 ("wing_rf_hi_ok", "F", (-1.12, 3.10, 0.05), False),
                 ("bump_front_hi_ok", "F", (-0.86, 3.26, -0.41), False),
                 ("chassis_hi", "R", (-1.12, -3.07, 0.20), False)],
    "mule": [("chassis_hi", "F", (-0.73, 2.78, 0.01), True)],
    "mrwhoop": [("chassis_hi", "F", (-0.76, 2.23, 0.05), True)],
}
MODELOS = sorted(SITES)


def clusters_with_faces(faces, frac_min=FRAC_FACE):
    """Racimos de indicator_lamp_anchor pero devolviendo sus caras.

    Cara contada: px >= 1 y amb/px >= frac_min. Racimo: caras conectadas por
    indice de vertice dentro del mismo (frame, material, geometry). Es la misma
    regla que califica el ancla; aqui se conserva la lista de caras para medir
    el footprint UV y el area ambar.
    """
    buenos = [f for f in faces if f["px"] >= 1
              and (f["amb"] / max(f["px"], 1)) >= frac_min]
    by = collections.defaultdict(list)
    for f in buenos:
        by[(f["frame"], f["mi"], f["gi"])].append(f)
    out = []
    for key, fs in by.items():
        for gg in L._components(fs):
            area = sum(f["area"] for f in gg)
            amber = sum(f["area"] * (f["amb"] / float(f["px"])) for f in gg)
            xs = [f["world"][0] for f in gg]
            ys = [f["world"][1] for f in gg]
            zs = [f["world"][2] for f in gg]
            out.append({"frame": key[0], "mi": key[1], "gi": key[2],
                        "faces": gg, "caras": len(gg), "area": area,
                        "amber": amber, "cov": (amber / area) if area else 0.0,
                        "x": sum(xs) / len(xs), "y": sum(ys) / len(ys),
                        "z": sum(zs) / len(zs),
                        "texelas": sum(f["amb"] for f in gg),
                        "px": sum(f["px"] for f in gg)})
    return out


def site_of(model, frame, axle, side, cx, cy, cz):
    """El sitio aprobado cuyo centroide esta a <= SITE_TOL del racimo, o None."""
    for (sf, sa, (sx, sy, sz), partial) in SITES.get(model, []):
        if sf != frame or sa != axle:
            continue
        ex = sx if side < 0 else -sx
        d = ((cx - ex) ** 2 + (cy - sy) ** 2 + (cz - sz) ** 2) ** 0.5
        if d <= SITE_TOL:
            return (sf, sa, partial)
    return None


def measure_model(model):
    """Filas aprobadas del modelo y sus caras (para el atlas)."""
    ia.BASE = ROOT
    ia.MODELS = ROOT
    _buf, frames, geoms, _atomics, faces = L.measure(model)
    anch = A.anchors_of(frames)
    cls = clusters_with_faces(faces)
    # agrupar racimos aprobados por (frame, nodo, costado): un mismo sitio puede
    # partir su lente en dos racimos (fbiranch wing_lf: 273.7 + 297.2 cm2).
    grupos = collections.defaultdict(list)
    detalle = []
    for c in cls:
        axle = "F" if c["y"] > 0.0 else "R"
        side = -1 if c["x"] < 0.0 else 1
        site = site_of(model, c["frame"], axle, side, c["x"], c["y"], c["z"])
        if site is None:
            continue
        # puerta del ancla: el racimo vive pegado a la lampara de su eje/costado
        cl_a = {"x": c["x"], "y": c["y"], "z": c["z"],
                "cobertura": c["texelas"] / max(c["px"], 1)}
        verdict, _ax, _lado, _donde, dist = A.clasifica(cl_a, anch)
        partial = site[2]
        if not partial:
            if verdict != "lente" or c["cov"] < MIN_FACE:
                detalle.append((c, "RECHAZA cov=%.2f verdict=%s" % (c["cov"], verdict)))
                continue
        else:
            if dist > 0.50 or c["amber"] <= 0.0:
                detalle.append((c, "RECHAZA parcial sin ancla dist=%.2f" % dist))
                continue
        node = NODE[axle][side]
        grupos[(c["frame"], node, side)].append(c)
        detalle.append((c, "APRUEBA %s%s cov=%.2f dist=%.2f%s" % (
            axle, "L" if side < 0 else "R", c["cov"], dist,
            " (parcial)" if partial else "")))
    filas = []
    caras_atlas = []
    for (frame, node, side), cs in sorted(grupos.items()):
        fs = [f for c in cs for f in c["faces"]]
        area = sum(f["area"] for f in fs)
        amber = sum(f["area"] * (f["amb"] / float(f["px"])) for f in fs)
        # centroide ponderado por area de las caras del sitio
        cx = sum(f["world"][0] * f["area"] for f in fs) / area
        cy = sum(f["world"][1] * f["area"] for f in fs) / area
        cz = sum(f["world"][2] * f["area"] for f in fs) / area
        mats = 0
        for c in cs:
            mats |= 1 << c["mi"]
        filas.append({"model": model, "frame": frame, "node": node, "side": side,
                      "mats": mats, "area_cm2": int(round(amber * 1e4)),
                      "xyz": [int(round(cx * 100)), int(round(cy * 100)), int(round(cz * 100))],
                      "cov": (amber / area) if area else 0.0, "faces": fs})
        caras_atlas.extend(fs)
    return filas, caras_atlas, geoms, detalle


def footprint_texels(g, tri, w, h):
    """Texelas (tx, ty) que pisa el triangulo UV (recorte exacto de _face_render)."""
    pts = ia._uv_tri(g, tri)
    xs = [p[0] * w for p in pts]
    ys = [p[1] * h for p in pts]
    ax, ay = xs[0], ys[0]
    bx, by = xs[1], ys[1]
    cx, cy = xs[2], ys[2]
    area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
    keys = set()
    if abs(area) < 1e-9:
        keys.add((int((ax + bx + cx) / 3.0) % w, int((ay + by + cy) / 3.0) % h))
        return keys
    for ty in range(max(int(min(ys)), 0), min(int(max(ys)) + 1, h)):
        for tx in range(max(int(min(xs)), 0), min(int(max(xs)) + 1, w)):
            if ia._cell_cover((ax, ay), (bx, by), (cx, cy), tx, ty) > 1e-9:
                keys.add((tx, ty))
    return keys


def _brighten(r, g, b):
    """Sube el brillo conservando el matiz (mismo factor en los tres canales)."""
    return (min(255, int(round(r * GLOW))),
            min(255, int(round(g * GLOW))),
            min(255, int(round(b * GLOW))))


def build_derived(model, faces, geoms, out_dir, check):
    """Atlas `<tex>_lens` por textura de lente: solo texelas ambar del footprint.

    Dos formatos, una garantia: fuera del footprint no cambia ni una texela.
      * DXT1 (esperant/boxville/mule/trash): el nivel 0 son bloques 4x4. Se
        re-encodifican SOLO los bloques que contienen alguna texela ambar del
        footprint; los demas bloques se copian bit a bit del original. Dentro
        del bloque tocado solo se sube el brillo de las texelas ambar (el
        encoder puede redondear las vecinas del mismo bloque: por eso el
        footprint de la comprobacion es el bloque, no el texel suelto).
      * 8888/565 sin comprimir (fbiranch): se edita word a word / texela a
        texela sobre los bytes originales, footprint exacto del UV.
    El color de la fila es la media de las texelas ambar del footprint.
    """
    pix = ia.texture_pixels(model)
    por_tex = collections.defaultdict(list)
    for f in faces:
        tn = (f["tex"] or "").lower()
        if tn:
            por_tex[tn].append(f)
    data = io.open(os.path.join(ROOT, model + ".txd"), "rb").read()
    emits = []
    for tex, fs in sorted(por_tex.items()):
        if tex not in pix:
            continue
        w, h, px = pix[tex]
        foot = set()
        for f in fs:
            g = geoms[f["gi"]]
            foot |= footprint_texels(g, g["tris"][f["ti"]], w, h)
        # subconjunto ambar: la texela renderida (material blanco) pasa _is_amber
        amber = set()
        acc = [0, 0, 0]
        for (tx, ty) in foot:
            c = px[ty * w + tx]
            if ia._is_amber(c):
                amber.add((tx, ty))
                acc[0] += c[0]
                acc[1] += c[1]
                acc[2] += c[2]
        rgb = tuple(int(round(a / len(amber))) for a in acc) if amber else (253, 165, 0)
        pos, fin, info = G.find_native(data, tex)
        if info is None:
            raise SystemExit("ERROR: %s: no hay TextureNative %s" % (model, tex))
        size0, off0 = info["levels"][0]
        orig = data[off0:off0 + size0]
        level0 = bytearray(orig)
        painted = 0
        if info["compression"] == 1:
            # DXT1: bloques 4x4; solo se re-encodifican los que tienen ambar.
            if size0 != (w // 4) * (h // 4) * 8:
                raise SystemExit("ERROR: %s: nivel0 DXT1 %d inesperado" % (model, size0))
            dirty = set()
            for (tx, ty) in amber:
                dirty.add((tx // 4, ty // 4))
            # footprint de comprobacion = texelas de los bloques tocados
            foot_check = set()
            for (bx, by) in dirty:
                for yy in range(by * 4, by * 4 + 4):
                    for xx in range(bx * 4, bx * 4 + 4):
                        foot_check.add((xx, yy))
            for (bx, by) in sorted(dirty):
                blk = []
                for yy in range(by * 4, by * 4 + 4):
                    for xx in range(bx * 4, bx * 4 + 4):
                        c = px[yy * w + xx]
                        if (xx, yy) in amber:
                            nr, ng, nb = _brighten(c[0], c[1], c[2])
                            if (nr, ng, nb) != (c[0], c[1], c[2]):
                                painted += 1
                            blk.append([nr, ng, nb])
                        else:
                            blk.append([c[0], c[1], c[2]])
                c0, c1, bits = G.encode_block(blk)
                o = (by * (w // 4) + bx) * 8
                level0[o:o + 8] = struct.pack("<HHI", c0, c1, bits)
        else:
            # sin comprimir: edicion por texela, footprint exacto
            foot_check = foot
            bpp = info["depth"] // 8
            if size0 != w * h * bpp:
                raise SystemExit("ERROR: %s: nivel0 %d != w*h*bpp %d" % (model, size0, w * h * bpp))
            for (tx, ty) in sorted(amber):
                i = ty * w + tx
                c = px[i]
                nr, ng, nb = _brighten(c[0], c[1], c[2])
                if info["depth"] == 16:
                    v = struct.unpack_from("<H", level0, i * 2)[0]
                    nv = ((nr >> 3) << 11) | ((ng >> 2) << 5) | (nb >> 3)
                    if nv != v:
                        level0[i * 2:i * 2 + 2] = struct.pack("<H", nv)
                        painted += 1
                elif info["depth"] == 32:
                    # el fichero guarda B,G,R,A (little-endian 8888)
                    o = i * 4
                    if (nr, ng, nb) != (c[0], c[1], c[2]):
                        level0[o] = nb
                        level0[o + 1] = ng
                        level0[o + 2] = nr
                        painted += 1
                else:
                    raise SystemExit("ERROR: %s: prof=%d no soportada" % (model, info["depth"]))
        # verificacion: texelas distintas fuera del footprint (bloque en DXT1) = 0
        back = tr.decode(bytes(level0), w, h, info["depth"], info["fmt"],
                         info["has_alpha"], info["compression"])
        base = tr.decode(orig, w, h, info["depth"], info["fmt"],
                         info["has_alpha"], info["compression"])
        diff_out = 0
        diff_in = 0
        for i in range(w * h):
            if base[i][:3] != back[i][:3]:
                if (i % w, i // w) in foot_check:
                    diff_in += 1
                else:
                    diff_out += 1
        new_name = tex + G.SUFFIX
        if len(new_name) > 31:
            raise SystemExit("ERROR: %s no cabe en 32 bytes" % new_name)
        chunk = G.rewrite_native(data, pos, fin, new_name, bytes(level0))
        out = G.build_txd(data, chunk)
        _p, _f, chk = G.find_native(out, new_name)
        if chk is None:
            raise SystemExit("ERROR: %s: el TXD emitido no relee %s" % (model, new_name))
        path = None
        if not check:
            os.makedirs(out_dir, exist_ok=True)
            path = os.path.join(out_dir, model + G.SUFFIX + ".txd")
            io.open(path, "wb").write(out)
        emits.append({"model": model, "tex": tex, "new": new_name,
                      "px": [w, h], "fmt": ("DXT1" if info["compression"] == 1
                                            else "%dbit" % info["depth"]),
                      "footprint": len(foot), "foot_check": len(foot_check),
                      "amber": len(amber), "painted": painted,
                      "diff_in": diff_in, "diff_out": diff_out,
                      "rgb": list(rgb), "txd_bytes": len(out), "txd_out": path})
    return emits


def write_table(filas, out):
    lineas = ["// Generado por gta_vc_browser/tools/gen_body_lens.py -- no editar a mano.",
              "// Lentes ambar medidas en la CARROCERIA (no en un nodo indicator_*): se",
              "// encienden en su sitio con un atlas derivado <atlas>_lens, sin mover ni",
              "// crear geometria ni tintar el material compartido (plan 58.0 T3/T4).",
              "// Puerta: racimo pegado al ancla de faro/piloto (<= 0.50 m, mismo eje y",
              "// costado) con >= 75 % de area ambar; mule delante y mrwhoop delante",
              "// son excepciones parciales del jugador (43 %/69 %): solo encienden las",
              "// texelas ambar bajo el footprint de su cara. area_cm2 = area ambar;",
              "// xyz = centroide del sitio en cm; mats = mascara del material de la lente.",
              "#pragma once",
              "",
              "struct ViceExtBodyLens {",
              "\tconst char *name;",
              "\tconst char *frame;",
              "\tint8 node;",
              "\tint8 side;",
              "\tuint8 rgb[3];",
              "\tuint32 mats;",
              "\tuint16 area_cm2;",
              "\tint16 xyz[3];",
              "};",
              "",
              "static const ViceExtBodyLens s_odBodyLens[] = {"]
    for f in filas:
        rgb = f.get("rgb", (253, 165, 0))
        lineas.append('\t{ "%s", "%s", %s, %d, { %d, %d, %d }, 0x%08x, %d, { %d, %d, %d } },\t// %d%% ambar' % (
            f["model"], f["frame"], f["node"], f["side"], rgb[0], rgb[1], rgb[2],
            f["mats"], f["area_cm2"], f["xyz"][0], f["xyz"][1], f["xyz"][2],
            int(round(100 * f["cov"]))))
    lineas.append("};")
    with io.open(out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lineas) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=SRC)
    ap.add_argument("--atlas-dir", default=os.path.join(REPO, "tmp", "58", "lens"))
    ap.add_argument("--json", default=os.path.join(REPO, "tmp", "58", "body_lens.json"))
    ap.add_argument("--check", action="store_true", help="no escribe tabla ni TXD")
    a = ap.parse_args()
    ia.BASE = ROOT
    ia.MODELS = ROOT

    todas = []
    emits = []
    for m in MODELOS:
        filas, caras, geoms, detalle = measure_model(m)
        print("==== %s" % m)
        for c, why in detalle:
            print("   %-16s mi=%d gi=%d %6.1f cm2 cov=%.2f xyz=(%.2f,%.2f,%.2f) %s" % (
                c["frame"], c["mi"], c["gi"], 1e4 * c["area"], c["cov"],
                c["x"], c["y"], c["z"], why))
        em = build_derived(m, caras, geoms, a.atlas_dir, a.check)
        # el color de la fila sale de las texelas ambar del footprint medido
        rgb_by_frame = {}
        for e in em:
            for f in filas:
                rgb_by_frame.setdefault((f["frame"], f["side"]), e["rgb"])
        for f in filas:
            f["rgb"] = tuple(rgb_by_frame.get((f["frame"], f["side"]), (253, 165, 0)))
        for e in em:
            print("   atlas %-22s -> %-27s %-5s footprint=%4d ambar=%4d pintadas=%4d "
                  "diff_in=%4d diff_fuera=%d rgb=%s" % (
                      e["tex"], e["new"], e["fmt"], e["footprint"], e["amber"],
                      e["painted"], e["diff_in"], e["diff_out"], e["rgb"]))
            if e["diff_out"] != 0:
                raise SystemExit("ERROR: %s: %d texelas cambiadas FUERA del footprint"
                                 % (m, e["diff_out"]))
        emits.extend(em)
        todas.extend(filas)

    if not a.check:
        write_table(todas, a.out)
        os.makedirs(os.path.dirname(a.json), exist_ok=True)
        io.open(a.json, "w", encoding="utf-8").write(json.dumps(
            {"rows": [{k: v for k, v in f.items() if k != "faces"} for f in todas],
             "atlases": emits}, indent=1, ensure_ascii=False))
    print("\nfilas: %d" % len(todas))
    for f in todas:
        print("  %-9s %-18s %s%+d  %4d cm2  %d%%  rgb%s  mats=0x%x  xyz=%s" % (
            f["model"], f["frame"], f["node"].replace("CAR_INDICATOR_", ""), f["side"],
            f["area_cm2"], int(round(100 * f["cov"])), f.get("rgb"), f["mats"], f["xyz"]))
    print("atlas derivados: %d  texelas pintadas: %d  diff fuera del footprint: %d" % (
        len(emits), sum(e["painted"] for e in emits), sum(e["diff_out"] for e in emits)))
    if not a.check:
        print("-> %s" % a.out)
        print("-> %s" % a.json)


if __name__ == "__main__":
    main()