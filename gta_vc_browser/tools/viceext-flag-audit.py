#!/usr/bin/env python3
"""Auditoria de las banderas Vice Extended: cual tiene codigo detras y cual no.

Para que: la matriz de cobertura (`.agents/viceex-cobertura.md`) dice que sus
estados "PORTADO" no son prueba de nada y que los "POR-VERIFICAR" se resuelven
con grep. Este script mecaniza ese grep: para cada `#define VICEEXT_*` de
`src/core/config.h` cuenta cuantas veces aparece en el resto de `src/` y en que
ficheros. Una bandera sin ningun uso es **una etiqueta vacia**: no implementa
nada, por mucho que este encendida.

Uso:
    python tools/viceext-flag-audit.py            # resumen por bandera
    python tools/viceext-flag-audit.py --vacias    # solo las que no hacen nada
    python tools/viceext-flag-audit.py --familia 6 # filtra por familia (marca en config.h)
    python tools/viceext-flag-audit.py --markdown  # tabla lista para pegar

Salida: recuento total, las banderas sin uso (etiqueta vacia) y el detalle de
uso por fichero. Exit 1 solo si hay alguna bandera sin uso; exit 0 si todas
tienen codigo.
"""
import argparse
import os
import pathlib
import re
import sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent          # gta_vc_browser/
REPO = RAIZ.parent                                            # raiz del port
CONFIG = REPO / "src" / "core" / "config.h"
FUENTE = REPO / "src"

RE_DEFINE = re.compile(r"^\s*#define\s+(VICEEXT_[A-Z0-9_]+)", re.M)
# La familia va en el comentario de la linea, p.ej. "// familia 13: ..." o "// 9.2 / features.ini"
RE_FAMILIA = re.compile(r"//\s*(?:familia\s*)?([0-9]+(?:\.[0-9]+)?)")


def banderas():
    """[(nombre, linea, familia)] de config.h, en orden de aparicion."""
    out = []
    for i, linea in enumerate(CONFIG.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        m = RE_DEFINE.match(linea)
        if not m:
            continue
        f = RE_FAMILIA.search(linea)
        out.append((m.group(1), i, f.group(1) if f else "-"))
    return out


def fuentes():
    for raiz, _dirs, ficheros in os.walk(FUENTE):
        for f in ficheros:
            if not f.endswith((".cpp", ".h")):
                continue
            p = pathlib.Path(raiz) / f
            if p.resolve() == CONFIG.resolve():
                continue
            yield p, p.read_text(encoding="utf-8", errors="replace")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--vacias", action="store_true", help="solo las banderas sin ningun uso")
    ap.add_argument("--familia", default=None, help="filtra por familia (numero del comentario)")
    ap.add_argument("--markdown", action="store_true", help="tabla en markdown")
    a = ap.parse_args()

    if not CONFIG.exists():
        print("FAIL: no encuentro %s" % CONFIG)
        return 1

    todo = list(fuentes())
    filas = []
    for nombre, linea, fam in banderas():
        if a.familia and fam != a.familia:
            continue
        usos = [(p.relative_to(REPO).as_posix(), s.count(nombre)) for p, s in todo if nombre in s]
        total = sum(c for _, c in usos)
        filas.append((nombre, linea, fam, total, sorted(usos, key=lambda x: -x[1])))

    vacias = [f for f in filas if f[3] == 0]
    print("banderas VICEEXT_* en config.h: %d" % len(filas))
    print("con codigo detras: %d" % (len(filas) - len(vacias)))
    print("etiquetas vacias (sin un solo uso en src/): %d" % len(vacias))
    print()

    if a.markdown:
        print("| Bandera | Linea | Familia | Usos | Ficheros |")
        print("|---|---|---|---|---|")
        for nombre, linea, fam, total, usos in filas:
            top = ", ".join("%s x%d" % (p, c) for p, c in usos[:3]) if usos else "-"
            print("| `%s` | %d | %s | %d | %s |" % (nombre, linea, fam, total, top))
    else:
        for nombre, linea, fam, total, usos in (vacias if a.vacias else filas):
            if not a.vacias:
                top = ", ".join("%s x%d" % (p, c) for p, c in usos[:3]) if usos else "-"
                print("  %-34s fam %-5s linea %-4d usos %-4d %s" % (nombre, fam, linea, total, top))
            else:
                print("  %-34s fam %-5s linea %d" % (nombre, fam, linea))

    if vacias:
        if not a.vacias:
            print()
            print("etiquetas vacias (no implementan nada, aunque esten encendidas):")
            for nombre, linea, fam, _t, _u in vacias:
                print("   %s  (familia %s, linea %d)" % (nombre, fam, linea))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
