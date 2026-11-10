"""Genera src/vehicles/IndicatorLensTable.h: una fila por RACIMO de lente.

Por que hace falta: en los DFF servidos el `material.color` de las lentes es
blanco (255,255,255) y el color vive en los pixel de la textura, que el motor no
puede leer (las texturas ya estan subidas a la GPU). Sin esta tabla el destello
sale blanco sobre una lente ambar o roja. La tabla es dato medido, no inventado:
la regla del proyecto es cero ambar inventado.

Hasta el plan 45.0 la fila era por NODO Y COSTADO y su clase salia del color
MEDIO del atomic entero, con una excepcion escrita a mano: si todas las caras
usaban el atlas `lights` del mod se declaraba ambar aunque el pixel dijera otra
cosa (`k = "A" if n_modlens == n else classify(mean)`). Dos cosas se corrigieron:

  * la unidad es el racimo (caras de lente conectadas por vertice), no el atomic:
    un nodo con lente ambar y piloto rojo en el mismo atomic promediaba a rojo y
    perdia el destello, y un nodo con media lente apagada promediaba a ambar y se
    encendia entero;
  * la regla es la fraccion de area ambar del propio racimo (>= 50 %), y esa
    fraccion ya la mide `indicator_lens45.lens_area` cara a cara sobre el
    footprint UV. El atlas `lights` del mod entra por la misma puerta: a esa
    textura la medida la declara lente con toda su area ambar, asi que deja de
    existir una excepcion que se salte la ventana de matiz.

Por eso este generador no mide nada por su cuenta: importa la medida de
`indicator_lens45`, la herramienta de produccion del plan y la unica con el
raster de funciones de borde.

Orden de las filas: (modelo, nodo, costado) y dentro de la celda el orden en que
`measure` recorre los atomics, que es el orden en que el motor los barre; la
fila n-esima sigue siendo la medida del grupo n-esimo (`occ` en Automobile.cpp).
`xyz` y `area_cm2` permiten al motor comprobar el emparejamiento en vez de
confiar en el orden.

Se ejecuta sobre el arbol SERVIDO, asi que describe lo que de verdad se dibuja.
Cambios en los DFF/TXD servidos -> volver a generar.

Uso:
    python gta_vc_browser/tools/gen_indicator_lens.py [--root DIR] [--out ARCHIVO]
"""
import argparse
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SRC = os.path.join(os.path.dirname(REPO), "src", "vehicles", "IndicatorLensTable.h")
sys.path.insert(0, HERE)

import indicator_audit as ia      # noqa: E402
import indicator_lens45 as L      # noqa: E402

NODE_ENUM = {
    "indicator_lf": "CAR_INDICATOR_LF",
    "indicator_rf": "CAR_INDICATOR_RF",
    "indicator_lr": "CAR_INDICATOR_LR",
    "indicator_rr": "CAR_INDICATOR_RR",
    "indicator2_lf": "CAR_INDICATOR_2_LF",
    "indicator2_rf": "CAR_INDICATOR_2_RF",
    "indicator2_lr": "CAR_INDICATOR_2_LR",
    "indicator2_rr": "CAR_INDICATOR_2_RR",
}

# El Vice Extended redibuja sus lentes sobre un atlas comun llamado `lights`
# (o `lightson`) cuyos texelos son BLANCOS a proposito: el binario del mod cambia
# el color del material en cada destello, y ese color es (255,225,0). Medido sobre
# ViceEx.exe (tarea REA del plan 44.0) y portado tal cual en el plan 42.1.
#
# Aqui ese mismo valor se veia AMARILLO CHILLON (plan 51.0 D2): el mod escribe
# (255,225,0) sobre SUS coches rediseñados, cuya texela de lente ya trae matiz,
# y el producto sale naranja; en el port la texela del atlas `lights` es blanca
# de diseño, así que el color de material es todo lo que llega a la pantalla.
# El valor de abajo es el RESULTADO medido sobre la captura del jugador jugando
# el mod en local (ind47_measure.py, 07/11/2026): las dos lentes del rear dan un
# nucleo de (252,162,0) y (254,168,0), media (253,165,0). No se inventa ningun
# ambar: se sustituye el color que el mod aplica sobre texelas que aqui no
# existen por el ambar con el que el mod realmente se ve. -> ADR-021.
MOD_DESIGN_RGB = (253, 165, 0)


def rows_for(model):
    """Una fila por racimo de lente de cada celda (nodo, costado).

    `amber` es la fraccion de area ambar del propio racimo, no el color medio de
    la celda. `mats` es la mascara de indices de material del racimo dentro de su
    geometry: con ella el motor pinta la lente sin tocar el panel que comparte
    atomic (packer indicator_lf: el material 0 es lente, el panel vive en otro).
    """
    faces = L.measure(model)[4]
    cells = L.node_faces(faces)
    out = []
    for key in sorted(cells):
        frame, side = key
        if frame not in NODE_ENUM:
            continue
        for cl in cells[key]["cl"]:      # en el orden del barrido de atomics
            atlas = cl["mod"] >= cl["area"] - 1e-12
            out.append({"node": NODE_ENUM[frame], "side": side,
                        "rgb": MOD_DESIGN_RGB if atlas else cl["amean"],
                        "atlas": atlas,
                        "amber": cl["frac"] >= L.FILL_MIN,
                        "cm2": 1e4 * cl["amber"],
                        "xyz": (cl["cx"], cl["cy"], cl["cz"]),
                        "mats": _mask(cl["mis"]),
                        "caras": cl["caras"],
                        "tex": ",".join(t for t in cl["tex"] if t)})
    return out


def _mask(mis):
    m = 0
    for i in mis:
        if i < 32:
            m |= 1 << i
    return m


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(REPO, "streamed", "models", "gta3.img"))
    ap.add_argument("--out", default=SRC)
    a = ap.parse_args()

    ia.BASE = a.root
    ia.MODELS = a.root
    rows = []
    avisos = []
    for model in sorted(L.fleet(a.root)):
        try:
            rows += [(model, r) for r in rows_for(model.lower())]
        except SystemExit as e:
            avisos.append("%s: %s" % (model, e))
        except Exception as e:
            avisos.append("%s: %s: %s" % (model, type(e).__name__, e))

    rows.sort(key=lambda r: (r[0], r[1]["node"], r[1]["side"]))
    _emit(a.out, rows, a.root)
    am = sum(1 for _m, r in rows if r["amber"])
    print("filas: %d  racimos ambar: %d  modelos con fila: %d"
          % (len(rows), am, len(set(m for m, _r in rows))))
    print("celdas con racimo de lente: %d  de ellas ambar: %d"
          % (len(set((m, r["node"], r["side"]) for m, r in rows)),
             len(set((m, r["node"], r["side"]) for m, r in rows if r["amber"]))))
    if avisos:
        print("avisos (%d):" % len(avisos))
        for w in avisos[:10]:
            print("   ", w)
    print("->", a.out)


def _emit(path, rows, root):
    am = sum(1 for _m, r in rows if r["amber"])
    with io.open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Generado por gta_vc_browser/tools/gen_indicator_lens.py -- no editar a mano.\n")
        f.write("// Fuente medida: %s\n" % root.replace("\\", "/"))
        f.write("// Una fila por RACIMO de lente (caras de lente conectadas por vertice) de\n")
        f.write("// cada (modelo, nodo, costado). El color es la media de los TEXELOS AMBAR del\n")
        f.write("// racimo, medida sobre el footprint UV de cada cara (funciones de borde) y\n")
        f.write("// multiplicada por el color del material (el color del DFF es blanco: el\n")
        f.write("// tono vive en la textura, que el motor ya no tiene en CPU). La media de la\n")
        f.write("// cara entera no vale: mezclas el cristal con el marco y sale un tono que ya\n")
        f.write("// no pasa la ventana de matiz.\n")
        f.write("// amber: 1 solo si >= %d%% del area del racimo es ambar medida\n"
                  % round(100 * L.FILL_MIN))
        f.write("// (_is_amber + ventana de matiz + cobertura de texel). El motor no reinventa\n")
        f.write("// la regla: un nodo sin fila ambar es el propio faro o piloto y parpadea con\n")
        f.write("// su propia lampara (ruta Banshee del plan 42.1). Sin fila no hay destello.\n")
        f.write("// mats: mascara de indices de material del racimo dentro de su geometry; el\n")
        f.write("// motor pinta ESOS materiales y nada mas, que es lo que deja el panel del\n")
        f.write("// atomic estatico. xyz: centroide del racimo en cm del modelo; area_cm2: area\n")
        f.write("// ambar medida. Sirven para comprobar el emparejamiento fila<->grupo barrido\n")
        f.write("// en vez de confiar en el orden. Racimos: %d, ambar %d, de textura propia %d,\n"
                  "// del atlas `lights`/`lightson` del mod %d (con su ambar medido de captura\n"
                  "// (253,165,0): el mod aplica (255,225,0) sobre texelas de lente con matiz;\n"
                  "// aqui la texela es blanca a proposito, asi que el tono vive en el material).\n"
                  % (len(rows), am, sum(1 for _m, r in rows if not r["atlas"]),
                     sum(1 for _m, r in rows if r["atlas"])))
        f.write("// atlas: 1 cuando TODO el racimo pinta sobre ese atlas del mod: sus texelos son\n")
        f.write("// blancos y el unico sitio donde cabe escribir el ambar es el color del material.\n")
        f.write("// Con texelos propios el pixel YA trae el tono y el material tiene que quedarse\n")
        f.write("// blanco: pintarlo multiplicaria los dos ambares y la lente saldria mas oscura.\n")
        f.write("// mats es una mascara de 32 bits (tope medido por geometry tras el split: 10).\n")
        f.write("#pragma once\n\n")
        f.write("struct ViceExtIndLens {\n")
        f.write("\tconst char *name;\n")
        f.write("\tint8 node;\n")
        f.write("\tint8 side;\n")
        f.write("\tuint8 rgb[3];\n")
        f.write("\tint8 amber;\n")
        f.write("\tuint8 atlas;\n")
        f.write("\tuint32 mats;\n")
        f.write("\tuint16 area_cm2;\n")
        f.write("\tint16 xyz[3];\n")
        f.write("};\n\n")
        f.write("static const ViceExtIndLens s_odIndLens[] = {\n")
        for model, r in rows:
            f.write('\t{ "%s", %s, %d, { %d, %d, %d }, %d, %d, 0x%08X, %d, '
                      '{ %d, %d, %d } },\t// %s %d caras %s%s\n'
                      % (model, r["node"], r["side"], r["rgb"][0], r["rgb"][1],
                         r["rgb"][2], 1 if r["amber"] else 0, 1 if r["atlas"] else 0,
                         r["mats"],
                         min(65535, int(round(r["cm2"]))),
                         int(round(100 * r["xyz"][0])), int(round(100 * r["xyz"][1])),
                         int(round(100 * r["xyz"][2])),
                         "A" if r["amber"] else "o", r["caras"], r["tex"],
                         " [atlas del mod]" if r["atlas"] else ""))
        f.write("};\n")


if __name__ == "__main__":
    main()
