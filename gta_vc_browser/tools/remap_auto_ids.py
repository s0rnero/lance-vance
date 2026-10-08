#!/usr/bin/env python3
"""Asigna IDs concretos a las entradas IDE/IPL que usan **-1** (auto-ID).

Motivo medido: Vice Extended trae `maps/bryx/bryx.ide` y `bryx.ipl` con ID
`-1` ("asígname un hueco"), que es una comodidad de *modloader* — el motor
vanilla NO la entiende:

    -1, bryx_lights, od_chariot, 1, 200, 164, 0, 0      (tobj)
    -1, bryx_lights, 0, 568.5, -4.8, 13.2, ...          (inst)

En reVC `CFileLoader::LoadTimeObject` hace `CModelInfo::AddTimeModel(-1)`, que
escribe **fuera** de `ms_modelInfoPtrs`; después `LoadObjectInstance(-1)` lee
esa posición y trabaja con un modelo basura → `RuntimeError: null function`
(llamada a un método virtual sobre un puntero corrupto). Reproducido en el
harness: `Not in cdimage lodbryx_lights` y traza en
`CFileLoader::LoadObjectInstance`.

Qué hace: recorre `<dir>` (recursivo) y

1. en cada `.ide`, en las secciones `objs`/`tobj`, asigna a las líneas con ID
   `-1` el siguiente ID libre desde `--base`, **por nombre de modelo**;
2. en cada `.ipl` (secciones `inst`), sustituye el `-1` por el ID asignado a
   ese nombre (si el nombre no se asignó antes, lo dice y no toca la línea).

Conserva el formato (solo cambia el primer campo). Con `--backup-dir` copia el
original ahí ANTES de tocar (por defecto no deja nada: los ficheros de mapas del
port se reescriben desde su fuente, y un `.bak` dentro de `streamed/` acabaría
en el manifiesto y en bootseed).

Uso:
    python tools/remap_auto_ids.py <dir_de_maps> --base 6670 [--backup-dir /tmp/x] [--dry-run]
"""
import argparse
import os
import re
import shutil


def backup(path, root, backup_dir):
    if not backup_dir:
        return
    rel = os.path.relpath(path, root)
    dst = os.path.join(backup_dir, rel)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(path, dst)

SECTION = re.compile(r"^\s*(objs|tobj|inst|end)\s*$", re.I)
LINE_NEG = re.compile(r"^(\s*)-1(\s*,.*)$")


def main():
    ap = argparse.ArgumentParser(description="Asigna IDs concretos a entradas IDE/IPL con -1.")
    ap.add_argument("dir", help="carpeta de maps (se recorre recursivamente)")
    ap.add_argument("--base", type=int, default=6670, help="primer ID a asignar (por defecto 6670)")
    ap.add_argument("--backup-dir", help="carpeta donde copiar los originales antes de tocar")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()

    # 1) IDEs: -1 -> ID nuevo, por nombre
    assigned = {}
    next_id = a.base
    for root, dirs, files in os.walk(a.dir):
        for f in sorted(files):
            if not f.lower().endswith(".ide"):
                continue
            p = os.path.join(root, f)
            lines = open(p, errors="latin1").read().split("\n")
            changed = False
            for i, line in enumerate(lines):
                m = LINE_NEG.match(line)
                if not m:
                    continue
                parts = line.split(",")
                if len(parts) < 3:
                    continue
                model = parts[1].strip()
                if model in assigned:
                    continue
                assigned[model] = next_id
                lines[i] = "%s%d%s" % (m.group(1), next_id, m.group(2))
                print("  %-28s %-22s -> %d" % (os.path.relpath(p, a.dir), model, next_id))
                next_id += 1
                changed = True
            if changed and not a.dry_run:
                backup(p, a.dir, a.backup_dir)
                open(p, "w", errors="latin1", newline="").write("\n".join(lines))

    # 2) IPLs: -1 -> ID asignado
    for root, dirs, files in os.walk(a.dir):
        for f in sorted(files):
            if not f.lower().endswith(".ipl"):
                continue
            p = os.path.join(root, f)
            lines = open(p, errors="latin1").read().split("\n")
            changed = False
            for i, line in enumerate(lines):
                m = LINE_NEG.match(line)
                if not m:
                    continue
                parts = line.split(",")
                if len(parts) < 3:
                    continue
                model = parts[1].strip()
                if model not in assigned:
                    print("  !! %s: %s sin ID asignado (¿falta el .ide?)" % (os.path.relpath(p, a.dir), model))
                    continue
                lines[i] = "%s%d%s" % (m.group(1), assigned[model], m.group(2))
                print("  %-28s %-22s -> %d" % (os.path.relpath(p, a.dir), model, assigned[model]))
                changed = True
            if changed and not a.dry_run:
                backup(p, a.dir, a.backup_dir)
                open(p, "w", errors="latin1", newline="").write("\n".join(lines))

    print("asignados %d IDs%s" % (len(assigned), " (dry-run)" if a.dry_run else ""))


if __name__ == "__main__":
    main()
