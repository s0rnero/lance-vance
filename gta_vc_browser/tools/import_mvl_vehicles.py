#!/usr/bin/env python3
"""Convierte vehículos de Vice Extended (formato MVL) al formato nativo del port.

Vice Extended no trae los coches nuevos en un `default.ide`/`handling.cfg`/
`carcols.dat` normales, sino en **un solo fichero** con el formato de Maxo's
Vehicle Loader (`newVehicles.ide`), donde cada dato del vehículo vive en la
sección que le toca:

    default,  <ID, campos de default.ide [cars]>
    handling, <ID, campos de handling.cfg (coche)>
    handling2,<ID, campos de handling.cfg (bici, líneas '!')>
    carcols,  <ID, parejas de colores de carcols.dat>
    sounds,   <ID, parámetros de audio del vehículo>
    shadow,   <ID, multiplicadores de sombra>

El motor no conoce ese formato, así que **la conversión se hace en los datos**
(en vez de escribir un parser MVL en C++):

1. una línea `cars` en `default.ide`, con el `HandlingId` que le falta (MVL lo
   deja en `null` porque el handling viene en su propia sección);
2. la línea de `handling.cfg` del coche, con el nombre que pide el punto 1;
3. la línea `!` de `handling.cfg` de la bici (si la tiene);
4. la línea de colores de `carcols.dat`, que va **por nombre de modelo**.

El nombre de handling se deriva del modelo (mayúsculas, máx. 13 caracteres) y
tiene que existir en `src/vehicles/HandlingMgr.cpp` (`VehicleNames[]`, en el
mismo orden que el enum `tVehicleType`); el script avisa si no es así.

Uso:
    python tools/import_mvl_vehicles.py <newVehicles.ide> [--data streamed/data]
                                        [--check src/vehicles/HandlingMgr.cpp]
                                        [--dry-run]
"""
import argparse
import os
import re
import sys

# Campos de la línea `default` de MVL. Todo lo anterior a `class` es idéntico a
# la sección `cars` de default.ide; MVL añade un último campo (clase de IA /
# radio de policía) que el motor no usa y aquí se descarta.
DEFAULT_FIELDS = (
    "id", "model", "txd", "type", "handling", "gamename",
    "anims", "vehclass", "frequency", "level", "comprules",
    "misc", "wheelscale",
)

# Nombres de handling que no salen del modelo (el derivado pasar\u00eda de 13
# caracteres, el m\u00e1ximo de `VehicleNames[]`).
NAME_OVERRIDE = {
    "polwintergreen": "POLWINTERG",
}

# El campo `gamename` de MVL es el **texto a mostrar**, NO una clave GXT. El
# motor lo copia tal cual en `CVehicleModelInfo::m_gameName[10]`
# (`CFileLoader::LoadVehicleObject`, src/core/FileLoader.cpp) y luego lo busca
# con `strcmp` + `BinarySearch` en el GXT (`CCurrentVehicle::Display`); las
# claves del GXT son de 8 bytes (7 caracteres + NUL), así que los nombres largos
# del mod ("Streetfighter", "VCPD_WinterGreen") imprimen "<nombre> missing" en
# el HUD **y** desbordan `m_gameName[10]`. Aquí se traducen a claves cortas: se
# reutiliza la vanilla cuando el vehículo es un clon declarado (Perennial,
# Trashmaster) y se inventa una nueva para el resto. Las claves nuevas hay que
# añadirlas al GXT de cada idioma (`tools/gxt_inspect.py add`); este script
# escribe además `tools/gamename_keys.tsv` con la lista exacta para no
# equivocarse (y `tools/check_ide_gxt.py` comprueba que están todas).
GAMENAME_KEY = {
    "streetfi": "STRTFTR",
    "peren2": "PEREN",        # clave vanilla (Perennial)
    "trash2": "TRASHM",       # clave vanilla (Trashmaster)
    "hellenbach": "HELLENB",
    "premier": "PREMIER",
    "manchez": "MANCHEZ",
    "wintergreen": "WINTERG",
    "polwintergreen": "VCPDWTR",
}
# Texto a mostrar cuando el del mod no vale tal cual (el motor cambia '_' por
# espacio, así que el resto se puede reusar).
GAMENAME_DISPLAY = {
    "wintergreen": "Winter Green",
    "polwintergreen": "VCPD WinterGreen",
}
MAX_GAMENAME_KEY = 7

# El Hellenbach pide la rueda nueva del mod (6600, `wheel_classic2`), que vive
# en el bloque de llantas/vehmods que este plan deja fuera: su `wheels.DFF`
# (1,2 MB) sustituir\u00eda el juego de ruedas vanilla y su `wheels.TXD` no trae
# texturas que las ruedas vanilla usan (`wheel_alloy64`, `whee_rim64`,
# `wheel_smallcar64`, `tyre64a`), as\u00ed que se queda con la cl\u00e1sica vanilla.
WHEEL_OVERRIDE = {
    6503: 253,
}


def read_text(path):
    with open(path, "rb") as f:
        return f.read().decode("latin1")


def write_text(path, text):
    with open(path, "wb") as f:
        f.write(text.encode("latin1"))


def split_lines(text):
    """[(contenido, terminador)] sin perder CRLF."""
    out = []
    for line in text.split("\n")[:-1]:
        if line.endswith("\r"):
            out.append((line[:-1], "\r\n"))
        else:
            out.append((line, "\n"))
    tail = text.split("\n")[-1]
    return out, tail


def parse_mvl(text):
    """{id: {seccion: [campos]}} de un newVehicles.ide.

    Cada línea empieza por el nombre de la sección; unas la separan con coma
    (`default, 6500, ...`) y otras con espacio (`handling 6500 ...`).
    """
    veh = {}
    order = []
    for raw in text.replace("\r\n", "\n").split("\n"):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        head = line.split(None, 1)
        section = head[0].strip().rstrip(",").strip()
        if section == "end" or len(head) < 2:
            continue
        rest = head[1].replace("\t", " ")
        if section == "default":
            # Campos separados por comas; MVL añade al final un campo extra
            # (clase de IA / radio de policía) que aquí se descarta.
            fields = [p.strip() for p in rest.split(",")]
            fields = [f for f in fields if f]
            if len(fields) > len(DEFAULT_FIELDS):
                fields = fields[:len(DEFAULT_FIELDS)]
            if len(fields) < len(DEFAULT_FIELDS):
                raise SystemExit("default incompleto (ID %s): %s" % (fields[0], line))
            # el campo extra puede venir pegado al último ("0.654 motorbike")
            fields[-1] = fields[-1].split()[0]
        else:
            # Ojo: MVL escribe a veces `carcols, 6503 57,8, ...` (ID y primer
            # color sin coma), así que aquí se tokeniza por espacios/comas.
            fields = [w for w in rest.replace(",", " ").split() if w]
        vid = int(fields[0])
        veh.setdefault(vid, {})[section] = fields[1:]
        if vid not in order:
            order.append(vid)
    return veh, order


def handling_name(model):
    """Nombre de handling estable, <=13 caracteres (el máximo de VehicleNames)."""
    name = re.sub(r"[^A-Za-z0-9_]", "", model).upper()
    return NAME_OVERRIDE.get(model, name[:13])


def gamename_key(model):
    """Clave GXT (<=7 caracteres, mayúsculas) del nombre del vehículo."""
    if model in GAMENAME_KEY:
        return GAMENAME_KEY[model]
    return re.sub(r"[^A-Za-z0-9]", "", model).upper()[:MAX_GAMENAME_KEY]


def drop_lines(text, pred):
    """Quita las líneas cuyo contenido cumple `pred` (para reescribir sin duplicar)."""
    lines, tail = split_lines(text)
    kept = [l for l in lines if not pred(l[0])]
    if len(kept) == len(lines):
        return text, 0
    return "".join(l + t for l, t in kept) + tail, len(lines) - len(kept)


def find_insert(text, predicate, from_end=False):
    """Índice de la línea donde insertar (antes de la primera que cumple)."""
    lines, _ = split_lines(text)
    rng = range(len(lines) - 1, -1, -1) if from_end else range(len(lines))
    for i in rng:
        if predicate(lines[i][0]):
            return i
    return None


def append_before(text, index, new_lines):
    lines, tail = split_lines(text)
    terminador = lines[index][1] if index < len(lines) else "\n"
    ins = [(l, terminador) for l in new_lines]
    lines[index:index] = ins
    return "".join(l + t for l, t in lines) + tail


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ide", help="ruta de newVehicles.ide del mod")
    ap.add_argument("--data", default="streamed/data", help="carpeta DATA del port")
    ap.add_argument("--check", default=None,
                    help="HandlingMgr.cpp donde validar los nombres de handling")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()

    veh, order = parse_mvl(read_text(a.ide))
    print("%s: %d vehículos" % (a.ide, len(order)))

    known_names = None
    if a.check and os.path.exists(a.check):
        src = read_text(a.check)
        m = re.search(r"VehicleNames\[NUMHANDLINGS\]\[14\]\s*=\s*\{(.*?)\n\};", src, re.S)
        if m:
            known_names = re.findall(r'"([^"]+)"', m.group(1))

    default_ide = read_text(os.path.join(a.data, "default.ide"))
    handling = read_text(os.path.join(a.data, "handling.cfg"))
    carcols = read_text(os.path.join(a.data, "carcols.dat"))

    nuevos_cars, nuevos_handling, nuevos_bike, nuevos_cols, avisos, claves = [], [], [], [], [], []

    for vid in sorted(order):
        v = veh[vid]
        d = dict(zip(DEFAULT_FIELDS[1:], v["default"]))
        model = d["model"].lower()
        if vid in WHEEL_OVERRIDE:
            d["misc"] = str(WHEEL_OVERRIDE[vid])
        name = handling_name(model)
        if known_names is not None and name not in known_names:
            avisos.append("ID %d: el handling '%s' no está en VehicleNames[]" % (vid, name))
        if len(name) > 13:
            avisos.append("ID %d: nombre de handling demasiado largo: %s" % (vid, name))
        if v.get("handling") is None:
            avisos.append("ID %d (%s): sin línea 'handling'" % (vid, model))
            continue

        gamename = gamename_key(model)
        if len(gamename) > MAX_GAMENAME_KEY:
            avisos.append("ID %d: clave GXT demasiado larga: %s" % (vid, gamename))
        claves.append("%s\t%s\t%s" % (model, gamename,
                                       GAMENAME_DISPLAY.get(model,
                                                            d["gamename"].replace("_", " "))))

        nuevos_cars.append(
            "%d,\t%s,\t%s,\t%s,\t%s,\t%s,\t\t%s,\t%s,\t%s,\t%s,\t%s,\t\t%s, %s" %
            (vid, model, d["txd"].lower(), d["type"].lower(), name, gamename,
             d["anims"], d["vehclass"], d["frequency"], d["level"], d["comprules"],
             d["misc"], d["wheelscale"]))
        nuevos_handling.append("%-16s %s" % (name, " ".join(v["handling"])))
        if v.get("handling2"):
            nuevos_bike.append("!\t%-12s %s" % (name, " ".join(v["handling2"])))
        if v.get("carcols"):
            pares = v["carcols"]
            nuevos_cols.append("%s, %s" % (model, ", ".join(", ".join(pares[i:i + 2])
                                                               for i in range(0, len(pares), 2))))

    print("  a convertir: %d coches, %d líneas de handling, %d de bici, %d de colores"
          % (len(nuevos_cars), len(nuevos_handling), len(nuevos_bike), len(nuevos_cols)))

    if a.dry_run:
        for l in nuevos_cars:
            print("   car:", l)
        for l in nuevos_handling:
            print("   hnd:", l)
        for l in nuevos_bike:
            print("   bik:", l)
        for l in nuevos_cols:
            print("   col:", l)
        for w in avisos:
            print("   AVISO:", w)
        return 0

    # Idempotencia: si estos IDs ya estaban (reejecución tras cambiar el
    # nombre/clave), se quitan sus líneas antes de volver a añadirlas.
    ids = set(order)
    modelos = {veh[i]["default"][0].lower() for i in order}
    nombres = {handling_name(veh[i]["default"][0].lower()) for i in order}

    def es_id(line):
        m = re.match(r"\s*(\d+)\s*,", line)
        return m is not None and int(m.group(1)) in ids

    def es_nombre(line):
        tok = line.split()
        return bool(tok) and tok[0] in nombres

    def es_nombre_bici(line):
        tok = line.split()
        return len(tok) > 1 and tok[0] == "!" and tok[1] in nombres

    def es_modelo(line):
        tok = line.split()
        return bool(tok) and tok[0].rstrip(",") in modelos

    default_ide, quitar_ide = drop_lines(default_ide, es_id)
    handling, quitar_h = drop_lines(handling, es_nombre)
    handling, quitar_b = drop_lines(handling, es_nombre_bici)
    carcols, quitar_c = drop_lines(carcols, es_modelo)
    if quitar_ide or quitar_h or quitar_b or quitar_c:
        print("  reescribiendo: %d líneas en default.ide, %d handling, %d bici, %d colores"
              % (quitar_ide, quitar_h, quitar_b, quitar_c))

    idx = find_insert(default_ide, lambda l: l == "end", from_end=False)
    # el `end` de la sección cars: el primero después de la cabecera `cars`
    lines, _ = split_lines(default_ide)
    car_header = next(i for i, (l, _t) in enumerate(lines) if l.strip() == "cars")
    end_cars = next(i for i, (l, _t) in enumerate(lines) if i > car_header and l.strip() == "end")
    default_ide = append_before(default_ide, end_cars, nuevos_cars)

    # handling de coches: antes del primer bloque especial ($ volar, % barcos, ! motos)
    idx = find_insert(handling, lambda l: l[:1] in ("$", "%", "!"))
    if idx is None:
        idx = find_insert(handling, lambda l: l.startswith(";the end"))
    handling = append_before(handling, idx, nuevos_handling)

    # handling de motos: al final del bloque '!', antes de ";the end"
    if nuevos_bike:
        idx = find_insert(handling, lambda l: l.startswith(";the end"))
        handling = append_before(handling, idx, nuevos_bike)

    # colores: antes del `end` de la sección car
    lines, _ = split_lines(carcols)
    car_header = next(i for i, (l, _t) in enumerate(lines) if l.strip() == "car")
    end_car = next(i for i, (l, _t) in enumerate(lines) if i > car_header and l.strip() == "end")
    carcols = append_before(carcols, end_car, nuevos_cols)

    write_text(os.path.join(a.data, "default.ide"), default_ide)
    write_text(os.path.join(a.data, "handling.cfg"), handling)
    write_text(os.path.join(a.data, "carcols.dat"), carcols)
    print("  escrito: default.ide, handling.cfg, carcols.dat")
    print("  nombres de handling: %s" % ", ".join(handling_name(v["default"][1]) for v in veh.values()))
    claves_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "gamename_keys.tsv")
    with open(claves_path, "w", newline="\n") as f:
        f.write("# model\tclave_gxt\ttexto (para `gxt_inspect.py add CLAVE=texto`)\n")
        f.write("\n".join(sorted(claves)) + "\n")
    print("  claves GXT -> %s" % claves_path)
    print("  (los campos 'sounds' y 'shadow' de MVL no van aquí: el audio vive en")
    print("   AudioLogic.cpp y el tamaño de sombra no lo lee este motor)")
    for w in avisos:
        print("  AVISO:", w)
    return 0


if __name__ == "__main__":
    sys.exit(main())
