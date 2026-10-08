#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Mina el binario del mod Vice Extended (fork de reVC).

Por qué existe: el `ViceEx.exe` del mod es un build de reVC **sin símbolos pero con
las cadenas de sus asserts**, y de ahí sale información que no está documentada en
ningún sitio: su árbol de fuentes (`extended/src/...`), las acciones de control que
añade (`PED_RELOAD`, `PED_1RST_PERSON_LOOK_*`...), los nombres exactos de los
dummies que busca (`servicelights`, `servicelightson`), sus claves de
`features.ini` (`RecoilWhenFiring`, `EnableSwimming`...) y los nombres de sus
funciones nuevas (`CCam::Process_1stPerson`).

Uso (desde la raíz del repo):
    python gta_vc_browser/tools/viceex-strings.py                     # volcado completo -> tmp/viceex-strings.txt
    python gta_vc_browser/tools/viceex-strings.py servo               # cadenas que contienen "servo"
    python gta_vc_browser/tools/viceex-strings.py -i "swim|crouch"    # patrón (regex, sin distinguir mayúsculas)
    python gta_vc_browser/tools/viceex-strings.py --src               # ficheros fuentes del mod
    python gta_vc_browser/tools/viceex-strings.py --ini               # claves de configuración probables

El binario se busca, en este orden: variable VC_VICEEXE, y las rutas conocidas del
paquete del mod dentro del repo.
"""

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # gta_vc_browser/
ROOT = os.path.dirname(REPO)                                          # raíz del port

CANDIDATES = [
    "vice-extended-october-2025-update_1788102643_908113/GameFiles/ViceEx.exe",
    "vice-extended-october-2025-update_1788102643_908113/Bonus/x64_beta/ViceEx.exe",
]

STRING_RE = re.compile(rb"[\x20-\x7e]{5,}")


def find_exe():
    env = os.environ.get("VC_VICEEX")
    if env and os.path.isfile(env):
        return env
    for rel in CANDIDATES:
        p = os.path.join(ROOT, rel)
        if os.path.isfile(p):
            return p
    # Último intento: cualquier ViceEx.exe dentro del repo.
    for base, _dirs, files in os.walk(ROOT):
        for f in files:
            if f.lower() == "viceex.exe":
                return os.path.join(base, f)
    return None


def load_strings(path):
    with open(path, "rb") as fh:
        data = fh.read()
    seen = set()
    out = []
    for m in STRING_RE.finditer(data):
        s = m.group().decode("latin-1")
        if s not in seen:
            seen.add(s)
            out.append(s)
    out.sort()
    return out


def main():
    ap = argparse.ArgumentParser(description="Cadena del binario del mod Vice Extended (fork de reVC).")
    ap.add_argument("pattern", nargs="?", help="patrón (regex) a buscar; sin él, vuelca todo")
    ap.add_argument("-i", "--ignore-case", action="store_true", help="sin distinguir mayúsculas (solo con PATTERN)")
    ap.add_argument("--src", action="store_true", help="lísta los ficheros fuentes que llevan sus asserts")
    ap.add_argument("--ini", action="store_true", help="lista claves de configuración probables (PascalCase con bloqueo de teclas)")
    ap.add_argument("--out", default=os.path.join(REPO, "tmp", "viceex-strings.txt"),
                    help="ruta del volcado completo")
    args = ap.parse_args()

    exe = find_exe()
    if not exe:
        print("No encuentro ViceEx.exe (define VC_VICEEX o revisa el paquete del mod).", file=sys.stderr)
        return 2

    strings = load_strings(exe)
    print("# %s -> %d cadenas únicas" % (os.path.relpath(exe, ROOT), len(strings)), file=sys.stderr)

    if args.src:
        # Las rutas llevan prefijo (`..\extended\src\...`), así que se busca en
        # cualquier posición y se normaliza a `/`.
        pat = re.compile(r"extended[\\/]src[\\/][A-Za-z0-9_\\/]+\.(cpp|h)$")
        for s in strings:
            if pat.search(s):
                print(s.replace("\\", "/"))
        return 0

    if args.ini:
        # Heurística: claves del estilo `RecoilWhenFiring`, `EnableSwimming`,
        # `WaypointColorRGB`, `CameraShakeInVehicleAtHighSpeed`. Se descartan los
        # nombres de la API de Windows que cumplen el mismo patrón (Window, Handle,
        # Stream, Thread...). Es una lista de candidatas: sirve para mirar cerca de
        # contexto conocido, no como verdad.
        NOISE = ("Window", "Handle", "Console", "Thread", "Stream", "Cache", "Alloc",
                 "Memory", "Region", "Frame", "Video", "Rect", "Sync", "Init", "File",
                 "Class", "Object", "Process", "Module", "Section", "Device", "Buffer",
                 "String", "Path", "Lock", "Event", "Entry", "Policy", "Method",
                 "Address", "Context", "Version", "Policy", "Pointer", "Bitmap", "Screen")
        pat = re.compile(r"^(?=[A-Za-z]{14,}$)[A-Z][a-z]+(?:[A-Z][a-z0-9]+){1,3}$")
        for s in strings:
            if pat.match(s) and not any(n in s for n in NOISE):
                print(s)
        return 0

    if args.pattern:
        flags = re.IGNORECASE if args.ignore_case else 0
        try:
            pat = re.compile(args.pattern, flags)
        except re.error as e:
            print("patrón inválido: %s" % e, file=sys.stderr)
            return 2
        for s in strings:
            if pat.search(s):
                print(s)
        return 0

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write("\n".join(strings) + "\n")
    print("volcado en %s" % os.path.relpath(args.out, ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
