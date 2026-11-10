#!/usr/bin/env python3
"""Que lente ambar vive pegada a cada lampara (plan 56.0 T2).

El 55.0 fallo por elegir el parche ambar MAS GRANDE en vez del que esta PEGADO A
LA LAMPARA: en el benson eso pint6 los marcadores laterales y dejo sin tocar la
direccional que el jugador ve debajo del faro. Este barrido cambia el criterio y
NO escribe nada: lista, por modelo y por (eje, costado), los parches ambar
medidos cara a cara con su distancia al ancla de faro/piloto declarada en el DFF,
y propone cual es la direccional. La lista la aprueba el jugador antes de tocar
ningun DFF.

Reglas, todas con su numero en la salida:
  * cara contada   : `amb/px >= --frac` (por defecto 0.25) y `px >= 1`. Con la
    correccion de T1 (`indicator_audit._uv_tri` envuelve la V en vez de recortarla
    y `_face_render` supermuestrea triangulos finos) estas caras existen: medido
    en pony `chassis_hi` mi=0 en (-0,94, +2,57, -0,24) y en mule `chassis_vlo`
    mi=1 en (+0,31, +2,78, +0,13), ambas en el plano del faro.
  * racimo         : caras conectadas por vertice dentro del mismo
    (frame, material). Su centroide decide el eje (signo de y) y el costado.
  * lente          : a <= --dist (0,50 m) del ancla del faro o piloto de ESE eje
    y en el mismo costado (|x| del parche y del ancla con el mismo signo).
  * marcador       : el resto. Se lista igual, para que el jugador vea por que se
    descarta, pero no es integrable.

Uso:
    python tools/indicator_lamp_anchor.py                     # flota servida
    python tools/indicator_lamp_anchor.py --model pony,mule    # modelos sueltos
    python tools/indicator_lamp_anchor.py --json /c/tmp/iav42/anclaje56.json
"""
import argparse
import collections
import io
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import indicator_audit as ia      # noqa: E402
import indicator_lens45 as L      # noqa: E402

ANCHOR_FRAMES = {"headlights": ("F", 1), "taillights": ("R", -1)}


def anchors_of(frames):
    """Anclas declaradas en el propio DFF: faro y piloto, con su eje y costado."""
    out = []
    for f in frames:
        n = (f["name"] or "").lower()
        if n in ANCHOR_FRAMES:
            axle, _ = ANCHOR_FRAMES[n]
            p = f["pos"]
            out.append({"frame": n, "axle": axle, "x": p[0], "y": p[1], "z": p[2]})
    return out


def clusters(faces, frac_min):
    """Racimos de caras con ambar, por (frame, material) y vertice compartido."""
    buenos = [f for f in faces if f["px"] >= 1
              and (f["amb"] / max(f["px"], 1)) >= frac_min]
    by = collections.defaultdict(list)
    for f in buenos:
        by[(f["frame"], f["mi"], f["gi"])].append(f)
    out = []
    for key, fs in by.items():
        # componentes conexas por indice de vertice
        adj = collections.defaultdict(set)
        for i, a in enumerate(fs):
            for j, b in enumerate(fs):
                if i != j and set(a["v"]) & set(b["v"]):
                    adj[i].add(j)
        vistos = set()
        for i in range(len(fs)):
            if i in vistos:
                continue
            pila = [i]
            grupo = []
            vistos.add(i)
            while pila:
                k = pila.pop()
                grupo.append(fs[k])
                for m in adj[k]:
                    if m not in vistos:
                        vistos.add(m)
                        pila.append(m)
            xs = [f["world"][0] for f in grupo]
            ys = [f["world"][1] for f in grupo]
            zs = [f["world"][2] for f in grupo]
            tex = sum(f["amb"] for f in grupo)
            pxs = sum(f["px"] for f in grupo)
            out.append({"frame": key[0], "mi": key[1], "tex": key[2],
                        "caras": len(grupo),
                        "x": sum(xs) / len(xs), "y": sum(ys) / len(ys),
                        "z": sum(zs) / len(zs),
                        "cm2": 1e4 * sum(f["area"] for f in grupo),
                        "cobertura": tex / max(pxs, 1),
                        "texelas_ambar": tex,
                        "rend": grupo[0]["rend"],
                        "en_nodo": key[0] in L.NODES})
    return out


def clasifica(cl, anch, lente_min=0.50):
    """La etiqueta del racimo: lente si esta pegada al ancla del SU eje y costado
    Y su area ambar pasa la misma regla con la que la tabla declara una lente
    (>= 50 % del area del racimo, plan 45.0).

    Plan 58.0 T0: antes `lente` salia solo de la distancia, con `--frac` (0,25)
    como unico suelo de cobertura, y eso entrego al jugador un falso positivo: el
    manana trasero, 67 cm2 al 27 % de area ambar, que a ojo es el piloto rojo
    («manana detras no tiene ambar, eso es falso»). El candidato y el veredicto
    tienen que hablar el mismo idioma que la tabla; lo que queda entre los dos
    umbrales se lista como `bajo-umbral`, medido pero sin declarar, para que lo
    mire el jugador en vez de entrar solo en ningun cambio.
    """
    axle = "F" if cl["y"] > 0.0 else "R"
    lado = -1 if cl["x"] < 0.0 else 1
    mejor, d = None, 1e9
    for a in anch:
        if a["axle"] != axle:
            continue
        # El ancla del DFF esta declarada para UN costado (x positivo: es el
        # desplazamiento del faro derecho), asi que el costado se compara en
        # valor absoluto, no con el signo.
        dd = ((abs(a["x"]) - abs(cl["x"])) ** 2 + (a["y"] - cl["y"]) ** 2
              + (a["z"] - cl["z"]) ** 2) ** 0.5
        if dd < d:
            mejor, d = a, dd
    if mejor is None:
        return "sin ancla", axle, lado, None, 99.0
    dz = cl["z"] - mejor["z"]
    donde = "debajo" if dz < -0.05 else ("encima" if dz > 0.05 else "a la altura")
    if d > 0.50:
        return "marcador", axle, lado, donde, d
    if cl["cobertura"] >= lente_min:
        return "lente", axle, lado, donde, d
    return "bajo-umbral", axle, lado, donde, d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(REPO, "streamed", "models", "gta3.img"))
    ap.add_argument("--model", default=None, help="lista separada por comas")
    ap.add_argument("--frac", type=float, default=0.25)
    ap.add_argument("--dist", type=float, default=0.50)
    ap.add_argument("--json", default=None)
    a = ap.parse_args()
    ia.BASE = a.root
    ia.MODELS = a.root

    models = [m.strip().lower() for m in a.model.split(",")] if a.model else \
        [m.lower() for m in L.fleet(a.root)]
    filas = []
    for m in models:
        try:
            buf, frames, geoms, atomics, faces = L.measure(m)
        except SystemExit:
            continue
        except Exception:
            continue
        anch = anchors_of(frames)
        if not anch:
            continue
        cls = clusters(faces, a.frac)
        if not cls:
            continue
        for cl in sorted(cls, key=lambda x: -x["texelas_ambar"]):
            v, axle, lado, donde, d = clasifica(cl, anch)
            if d > 2.5 and v != "lente":
                continue          # lejos de cualquier lampara: ruido del atlas
            filas.append(dict(model=m, verdict=v, axle=axle, side=lado,
                              distancia=round(d, 2), donde=donde, **cl))
        if a.model:
            print("==== %s" % m)
            for f in [x for x in filas if x["model"] == m]:
                print("   %-8s %-16s mi=%-2d eje=%s lado=%+d a %.2f m %s "
                      "caras=%2d %6.1f cm2 cobert=%3.0f%% texelas=%4d UV=%s "
                      "en_nodo=%s"
                      % (f["verdict"], f["frame"], f["mi"], f["axle"], f["side"],
                         f["distancia"], f["donde"] or "", f["caras"], f["cm2"],
                         100 * f["cobertura"], f["texelas_ambar"], f["tex"],
                         "SI" if f["en_nodo"] else "no"))

    if not a.model:
        cand = collections.OrderedDict()
        for f in filas:
            if f["verdict"] != "lente":
                continue
            cand.setdefault(f["model"], []).append(f)
        print("modelos con LENTE ambar pegada a la lampara: %d" % len(cand))
        for m, fs in cand.items():
            print("  %-11s %d racimo(s): %s" % (
                m, len(fs), "; ".join("%s/%s%s %.0f cm2 a %.2f m %s cobert %2.0f%%%s"
                                      % (f["frame"], f["axle"],
                                         "L" if f["side"] < 0 else "R",
                                         f["cm2"], f["distancia"], f["donde"],
                                         100 * f["cobertura"],
                                         " [ya en nodo]" if f["en_nodo"] else "")
                                      for f in fs)))
        descartados = collections.Counter(f["model"] for f in filas
                                          if f["verdict"] == "marcador")
        print("\nmodelos con parches ambar lejos de la lampara (marcadores, "
              "no integrables): %d" % len(descartados))
        for m, n in descartados.most_common(20):
            print("   %-11s %d" % (m, n))

    if a.json:
        with io.open(a.json, "w", encoding="utf-8") as fh:
            json.dump(filas, fh, ensure_ascii=False, indent=1)
        print("\n-> %s (%d racimos listados)" % (a.json, len(filas)))


if __name__ == "__main__":
    main()
