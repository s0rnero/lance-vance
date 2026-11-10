#!/usr/bin/env python3
"""La huella del atlas derivado y quien la pisa.

Dos preguntas que el plan 59.0 no supo responder y que producen los dos
defectos que el jugador ve:

  huella:   que texelas cambiaron de verdad entre la textura base y su `_lens`,
            si ese cambio esta dentro del rectangulo de la lente medida, y que
            caras ajenas las muestrean (pintura, bajos, faro blanco, piloto
            rojo). Responde «por que se pintan de amarillo lo que no es
            intermitente».
  lente:    el rectangulo UV del cristal ambar y TODAS las caras que lo
            muestrean, separando cristal de no-cristal. Responde «por que
            quedan triangulos apagados dentro del foco».

Solo lectura. Uso:
    python tools/lens_footprint.py huella rumpo,mrwhoop
    python tools/lens_footprint.py lente  rumpo,mrwhoop
"""
import argparse
import collections
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import indicator_audit as ia          # noqa: E402
import indicator_lens45 as L          # noqa: E402

N = 11


def cristal(f):
    """La regla del proyecto: `is_lens_face` (matiz medido sobre el
    footprint UV de la cara, con el minimo de texelas)."""
    return L.is_any_lens_face(f)


def uv_cells(g, t, w, h):
    try:
        pts = ia._uv_tri(g, t)
    except Exception:
        return set()
    xs = [p[0] * w for p in pts]
    ys = [p[1] * h for p in pts]
    out = set()
    for a in range(N + 1):
        for b in range(N + 1 - a):
            c = N - a - b
            u = (a * xs[0] + b * xs[1] + c * xs[2]) / N
            v = (a * ys[0] + b * ys[1] + c * ys[2]) / N
            out.add((int(u) % w, int(v) % h))
    return out


def face_of(f):
    return (f["v"][0], f["v"][1], f["v"][2], f["mi"])


def amber_cells(faces, geoms, pix, tex):
    cells = set()
    n = 0
    for f in faces:
        if f["tex"] != tex or not cristal(f):
            continue
        n += 1
        w, h, _p = pix[tex]
        cells |= uv_cells(geoms[f["gi"]], face_of(f), w, h)
    return cells, n


def bbox(cells):
    xs = [c[0] for c in cells]
    ys = [c[1] for c in cells]
    return (min(xs), min(ys), max(xs), max(ys))


def huella(model):
    _b, frames, geoms, atomics, faces = L.measure(model)
    pix = ia.texture_pixels(model)
    print("=" * 78)
    print("%s --huella" % model)
    for tex in sorted(pix):
        if not tex.endswith("_lens") or tex[:-5] not in pix:
            continue
        base = tex[:-5]
        w, h, pb = pix[base]
        _w2, _h2, pl = pix[tex]
        diff = set()
        for i in range(w * h):
            a, b = pb[i], pl[i]
            if max(abs(a[0] - b[0]), abs(a[1] - b[1]), abs(a[2] - b[2])) > 6:
                diff.add((i % w, i // w))
        lens, nlens = amber_cells(faces, geoms, pix, base)
        if not diff:
            print("   %-26s derivado identico (0 texelas)" % tex)
            continue
        dentro = diff & lens
        fuera = diff - lens
        ok = not fuera
        print("   %-26s cambiadas=%-5d bbox=%s  lente=%-5d caras_lente=%-3d  "
              "HUELLA %s  dentro=%d fuera=%d"
              % (tex, len(diff), bbox(diff), len(lens), nlens,
                 "OK" if ok else "FUERA", len(dentro), len(fuera)))
        pis = {}
        for f in faces:
            if f["tex"] != base:
                continue
            c2 = uv_cells(geoms[f["gi"]], face_of(f), w, h)
            ov = c2 & diff
            if not ov:
                continue
            cls = "CRISTAL" if cristal(f) else "ajena"
            k = (f["frame"], f["mi"], cls)
            e = pis.setdefault(k, [0, 0.0])
            e[0] += 1
            e[1] += f["area"] * 10000
        for (fn, mi, cls), (n, area) in sorted(pis.items(), key=lambda kv: -kv[1][1]):
            print("        %-18s mat%-2d %-7s tris=%-4d area=%9.1f cm2"
                  % (fn, mi, cls, n, area))


def lente(model):
    _b, frames, geoms, atomics, faces = L.measure(model)
    pix = ia.texture_pixels(model)
    print("=" * 78)
    print("%s --lente" % model)
    texs = sorted(set(f["tex"] for f in faces
                      if cristal(f)))
    if not texs:
        print("   (ninguna cara del modelo es cristal por la regla del proyecto)")
    for tex in texs:
        w, h, _p = pix[tex]
        cells, nlens = amber_cells(faces, geoms, pix, tex)
        print("   %-26s cristal=%-4d texelas=%-5d bbox=%s de %dx%d"
              % (tex, nlens, len(cells), bbox(cells), w, h))
        pis = {}
        for f in faces:
            if f["tex"] != tex:
                continue
            c2 = uv_cells(geoms[f["gi"]], face_of(f), w, h)
            if not (c2 & cells):
                continue
            cls = "CRISTAL" if cristal(f) else "ajena"
            k = (f["frame"], f["mi"], cls)
            e = pis.setdefault(k, [0, 0.0])
            e[0] += 1
            e[1] += f["area"] * 10000
        for (fn, mi, cls), (n, area) in sorted(pis.items(), key=lambda kv: -kv[1][1]):
            print("        %-18s mat%-2d %-7s tris=%-4d area=%9.1f cm2"
                  % (fn, mi, cls, n, area))


def partir(model):
    """Que hay que partir en cada material, y en cuantos slots.

Dos motivos distintos de partir, y se informan por separado:

  mezcla    el material tiene caras de cristal y caras que no lo son: si el
            motor tinte el material, pinta la pintura o los bajos.
  ejes      el material tiene cristal en dos costados o dos ejes distintos: un
            solo slot no puede parpadear por separado (es lo que obligo al 59.0
            a declarar la trasera del rumpo para que el tinte delantero pasara).
    """
    _b, frames, geoms, atomics, faces = L.measure(model)
    grupos = {}
    for f in faces:
        k = (f["frame"], f["gi"], f["mi"], f["tex"])
        e = grupos.setdefault(k, {"c": 0, "a": 0, "ac": 0.0, "aa": 0.0,
                                  "q": collections.Counter()})
        if cristal(f):
            e["c"] += 1
            e["ac"] += f["area"] * 10000
            e["q"]["%s%s" % ("L" if f["world"][0] < 0 else "R",
                             "F" if f["world"][1] > 0 else "R")] += 1
        else:
            e["a"] += 1
            e["aa"] += f["area"] * 10000
    print("=" * 78)
    print("%s --partir" % model)
    n = 0
    for (fn, gi, mi, tex), e in sorted(grupos.items(),
                                       key=lambda kv: (kv[0][0], kv[0][2])):
        if not e["c"]:
            continue
        n += 1
        q = dict(e["q"])
        slots = len(q) + (1 if e["a"] else 0)
        print("   %-18s geo%-2d mat%-2d %-24s cristal=%-3d (%7.1f cm2) ajeno=%-4d "
              "(%9.1f cm2)" % (fn, gi, mi, tex, e["c"], e["ac"], e["a"], e["aa"]))
        print("        quadrantos %s -> %d slot(s)%s"
              % (q, slots,
                 "  [MEZCLA + EJES]" if (e["a"] and len(q) > 1) else
                 ("  [MEZCLA]" if e["a"] else
                  ("  [EJES]" if len(q) > 1 else "  [exclusivo: no hay que partir]"))))
    if not n:
        print("   (ningun material del modelo tiene caras de cristal)")


def caras(model):
    """Cada cara que pisa las texelas del cristal, con su medida.

Dice si lo que el jugador ve apagado dentro del foco es cristal que la regla
del 50 % rechazo (fraccion ambar alta pero no mayoritaria) o es marco gris
(fraccion ambar baja de verdad). Sin esto, partir el material no arregla nada.
    """
    _b, frames, geoms, atomics, faces = L.measure(model)
    pix = ia.texture_pixels(model)
    print("=" * 78)
    print("%s --caras" % model)
    texs = sorted(set(f["tex"] for f in faces
                      if cristal(f)))
    for tex in texs:
        w, h, _p = pix[tex]
        cells, _n = amber_cells(faces, geoms, pix, tex)
        print("   texelas del cristal en %s: %d  bbox=%s" % (tex, len(cells), bbox(cells)))
        pis = []
        for f in faces:
            if f["tex"] != tex:
                continue
            c2 = uv_cells(geoms[f["gi"]], face_of(f), w, h)
            ov = c2 & cells
            if not ov:
                continue
            pis.append((len(ov) / max(1, len(c2)), f))
        for frac, f in sorted(pis, key=lambda p: -p[0]):
            print("      %-16s geo%-2d mat%-2d fr_ambar=%5.1f%% area=%7.1f cm2 "
                  "huella=%3.0f%% color=%s xyz=(%.2f,%.2f,%.2f)"
                  % (f["frame"], f["gi"], f["mi"], 100.0 * f["amb"] / max(1, f["px"]),
                     f["area"] * 10000, frac * 100, f["rend"],
                     f["world"][0], f["world"][1], f["world"][2]))


def soldar(model):
    """La continuacion de cada lente de nodo: los racimos sueldados por vertice.

    Es la regla del plan 45.0 (`_weld`/`_soldado`): el modelador que parte una
    lentija entre dos atomics deja la costura en el MISMO punto del mundo. Los
    racimos sueldados son el trozo de cristal que falta; los que no comparten
    costura ni caen en la caja son reflejo o pintura, y no se pintan.
    """
    _b, frames, geoms, atomics, faces = L.measure(model)
    cells = L.node_faces(faces)
    cl = L.clusters(faces)
    print("=" * 78)
    print("%s --soldar" % model)
    for key in sorted(cells, key=lambda k: (k[0], k[1])):
        c = cells[key]
        if not c["amber"] or c["best"] is None:
            continue
        sueldos = [x for x in cl if L._soldado(x, c["best"])]
        print("   %-14s side=%+d cristal=%-3d (%6.1f cm2) fr=%4.0f%%  sueldados=%d"
              % (key[0], c["side"], len(c["lens"]), c["lens_area"] * 10000,
                 c["frac"] * 100, len(sueldos)))
        for x in sueldos:
            print("        %-16s geo%-2d mat%s tex=%-22s caras=%-3d area=%7.1f cm2 "
                  "fr=%4.0f%% xyz=%s"
                  % (x["frame"], x["gi"], x["mis"], x["tex"], x["n"],
                     x["area"] * 10000, x["frac"] * 100, x["xyz"]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["huella", "lente", "partir", "caras", "soldar"])
    ap.add_argument("models")
    a = ap.parse_args()
    for m in a.models.split(","):
        {"huella": huella, "lente": lente, "partir": partir, "caras": caras,
         "soldar": soldar}[a.mode](m)


if __name__ == "__main__":
    main()
