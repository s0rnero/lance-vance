#!/usr/bin/env python3
"""Paso 4 del plan 42.1: escribe en los DFF servidos el nodo que dicta la tabla.

El contrato es `.agents/plans/data/421_clasifica.json` (una fila por extremo,
con su objetivo, su accion y el estado de hoy). Por modelo:

  - con extremos `NODO-AMBAR`/`NODO-PROPIO`: se rehace el DFF **desde la base
    limpia** (`tmp/base421`) con `indicator_audit.own_faces_patch`, dejando que
    la regla escriba solo esos extremos. Como el punto de partida es la base,
    los nodos inventados de octubre desaparecen y los del mod vuelven a su
    geometria (los dos trabajos que `--own-faces` nunca hizo por si solos).
  - sin extremos que escribir pero con `BORRA-NODO`, `TRIGGER` en estado `borra`
    o `RESTAURA-MOD`: se copia el DFF de la base tal cual.
  - el resto no se toca.

Escribe siempre en las tres copias servidas, y solo donde el fichero **ya
existe**: en `bootseed` no se crean ficheros nuevos. Antes de tocar nada se
guarda la copia actual en `tmp/backup-421-paso4/<copia>/<modelo>.dff`.

Uso:
    python tools/indicator_plan421.py --dry           # que se va a escribir
    python tools/indicator_plan421.py                # aplica
    python tools/indicator_plan421.py --only pheonix # un modelo
"""
import argparse
import collections
import json
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REPO = os.path.dirname(ROOT)
sys.path.insert(0, HERE)

import indicator_audit as ia  # noqa: E402

BASE = os.path.join(ROOT, "tmp", "base421")
DATA = os.path.join(REPO, ".agents", "plans", "data")
BACKUP = os.path.join(ROOT, "tmp", "backup-421-paso4")
COPIES = [("streamed", os.path.join(ROOT, "streamed", "models", "gta3.img")),
          ("gamefiles", os.path.join(ROOT, "gamefiles", "models", "gta3.img")),
          ("bootseed", os.path.join(ROOT, "bootseed", "models", "gta3.img"))]
WRITE = ("NODO-AMBAR", "NODO-PROPIO")
END_OF = {"lf": ("front", "left"), "rf": ("front", "right"),
          "lr": ("rear", "left"), "rr": ("rear", "right")}


def load_table():
    rows = json.load(open(os.path.join(DATA, "421_clasifica.json"), encoding="utf-8"))
    by = collections.defaultdict(dict)
    for r in rows:
        by[r["vehiculo"]][r["lado"]] = r
    return by


def plan(model, sides):
    """(extremos a escribir -> objetivo, hay que borrar/restaurar, celdas que cambian)."""
    wanted = {}
    for tag, row in sides.items():
        if row["accion"] in WRITE:
            wanted[END_OF[tag]] = row["objetivo"]
    deletes = [tag for tag, row in sides.items()
               if row["accion"] in ("BORRA-NODO", "RESTAURA-MOD")
               or (row["accion"] == "TRIGGER" and row["estado"] == "borra")]
    changed = [tag for tag, row in sides.items()
               if (row["accion"] in WRITE and row["estado"] != "igual") or
               row["accion"] in ("BORRA-NODO", "RESTAURA-MOD") or
               (row["accion"] == "TRIGGER" and row["estado"] == "borra")]
    return wanted, deletes, changed


def write_copies(model, data):
    """Sobrescribe las copias que ya existen; devuelve las escritas."""
    done = []
    for label, d in COPIES:
        target = os.path.join(d, model + ".dff")
        if not os.path.exists(target):
            continue
        src_backup = os.path.join(BACKUP, label)
        os.makedirs(src_backup, exist_ok=True)
        bpath = os.path.join(src_backup, model + ".dff")
        if not os.path.exists(bpath):
            shutil.copy2(target, bpath)
        tmp = target + ".tmp"
        with open(tmp, "wb") as f:
            f.write(data)
        os.replace(tmp, target)
        done.append(label)
    return done


def apply_model(model, wanted, dry):
    """Rehace el DFF desde la base con solo los extremos `wanted`.

    `wanted` asocia (extremo, lado) -> objetivo ('AMBAR'|'PROPIO'). La ruta la
    elige la tabla, no el gusto: AMBAR son TODOS los racimos de lente real de
    ese extremo (la lente partida entre parachoques y salpicadero se escribe
    entera), y PROPIO es la cara propia del faro/piloto del propio modelo.
    """
    path = os.path.join(BASE, model + ".dff")
    with open(path, "rb") as f:
        buf = f.read()
    orig = ia._own_selection

    def filtered(m, frames, geoms, atomics):
        lens = ia._lens_sources(m, frames, geoms, atomics)
        out = {}
        for key, obj in wanted.items():
            if obj == "AMBAR":
                out[key] = list(lens.get(key) or [])
            else:
                b = ia._borrow_source(m, frames, geoms, atomics, key[0], key[1])
                out[key] = [b] if b else []
        return out

    ia._own_selection = filtered
    try:
        out = ia.own_faces_patch(buf, model, allow_nodes=(model,))
    except SystemExit as e:
        # La regla no encuentra caras para los extremos que la tabla pide: se
        # queda como esta y la divergencia sale en la verificacion del paso 4.
        print("%s: SIN PARCHEAR (%s)" % (model, e))
        return 0
    finally:
        ia._own_selection = orig
    if dry:
        print("%s: (dry) %d -> %d bytes" % (model, len(buf), len(out)))
        return 1
    return len(write_copies(model, out))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dry", action="store_true", help="no escribe")
    ap.add_argument("--only", metavar="MODELO", nargs="+", help="solo estos modelos")
    ap.add_argument("--force", action="store_true",
                    help="reconstruye tambien los que la tabla da por iguales "
                         "(sirve para los nodos escritos con la version de STRUCT mal)")
    a = ap.parse_args()

    ia.BASE = BASE
    ia.MODELS = BASE
    tabla = load_table()
    todo = sorted(tabla)
    if a.only:
        todo = [m for m in todo if m in [x.lower() for x in a.only]]

    n_patch = n_copy = n_skip = 0
    for model in todo:
        sides = tabla[model]
        wanted, deletes, changed = plan(model, sides)
        if a.force and wanted:
            changed = sorted(wanted)
        if not changed:
            n_skip += 1
            continue
        if wanted:
            if apply_model(model, wanted, a.dry):
                n_patch += 1
        else:
            src = os.path.join(BASE, model + ".dff")
            with open(src, "rb") as f:
                data = f.read()
            if a.dry:
                print("%s: (dry) copia de base, %d bytes, borra/restaura %s"
                      % (model, len(data), ",".join(sorted(deletes))))
            else:
                write_copies(model, data)
            n_copy += 1
    print("modelos: %d  parche=%d  copia-base=%d  intacto=%d"
          % (len(todo), n_patch, n_copy, n_skip))
    if not a.dry:
        print("respaldo previo en %s" % BACKUP)
    return 0


if __name__ == "__main__":
    sys.exit(main())
