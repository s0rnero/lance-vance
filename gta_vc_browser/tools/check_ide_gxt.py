#!/usr/bin/env python3
"""Comprueba que los nombres de vehículo de `default.ide` se puedan mostrar.

El motor hace `CCurrentVehicle::Display()` ->
`TheText.Get(((CVehicleModelInfo*)...)->m_gameName)`
(`src/core/User.cpp`), y `CFileLoader::LoadVehicleObject` copia el campo
`gamename` del IDE tal cual en `m_gameName[10]` (`src/core/FileLoader.cpp`).
Consecuencias que este chequeo vigila:

  * si la clave **no existe** en el GXT, el HUD imprime "<clave> missing"
    (el bug "streetfighter missing" que reportó el jugador);
  * las claves del GXT son de **8 bytes** (7 caracteres + NUL) y la búsqueda es
    `strcmp` + `BinarySearch`: hace falta que quepan, vayan en **mayúsculas** y
    el array esté **ordenado**;
  * `m_gameName` son **10 bytes**: más de 9 caracteres desborda el buffer (el
    `strcpy` no comprueba nada, así que es corrupción de memoria, no un warning).

Uso:
  python tools/check_ide_gxt.py [--data streamed/data] [--text streamed/TEXT]
                                [--lang american] [--all-langs]
Salida: lista de problemas y código 1 si hay alguno.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gxt_inspect  # noqa: E402

MAXKEY = gxt_inspect.MAXKEY
MAX_GAMENAME = 9  # m_gameName[10] -> hasta 9 caracteres + NUL

# Huecos que YA venían en el VC original de 2003: comprobado contra
# `gamefiles/TEXT/american.gxt` (vanilla), donde tampoco existen. No los
# provocamos nosotros y el HUD imprimía "<clave> missing" también en el juego
# original, así que se informan pero no hacen fallar el chequeo.
GAPS_VANILLA = {
    "AEROPL": "airtrain (el tren del aeropuerto)",
    "RCGOBLI": "rcgoblin (el helic\u00f3ptero de control remoto)",
}


def vehiculos_del_ide(path):
    """[(id, model, gameName)] de la sección `cars` de un default.ide de VC.

    Línea: `id, model, txd, type, handlingId, gameName, anims, class, freq,
    level, comprules, [wheelModel, wheelScale]`.
    """
    out = []
    dentro = False
    with open(path, "rb") as f:
        for raw in f.read().decode("latin1").replace("\r\n", "\n").split("\n"):
            line = raw.split("#")[0].strip()
            if not line:
                continue
            if line.lower() == "cars":
                dentro = True
                continue
            if dentro and line.lower() == "end":
                break
            if not dentro:
                continue
            campos = [c.strip() for c in line.split(",")]
            if len(campos) < 6 or not campos[0].isdigit():
                continue
            out.append((int(campos[0]), campos[1], campos[5]))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", default="streamed/data")
    ap.add_argument("--text", default="streamed/TEXT")
    ap.add_argument("--lang", default="american")
    ap.add_argument("--all-langs", action="store_true")
    a = ap.parse_args()

    ide = os.path.join(a.data, "default.ide")
    if not os.path.exists(ide):
        raise SystemExit("ERROR: no existe %s" % ide)
    vehiculos = vehiculos_del_ide(ide)

    idiomas = ["american", "french", "german", "italian", "russian", "spanish"]
    if not a.all_langs:
        idiomas = [a.lang]

    gxt = {}
    for lang in idiomas:
        path = os.path.join(a.text, "%s.gxt" % lang)
        if not os.path.exists(path):
            raise SystemExit("ERROR: no existe %s" % path)
        _, _, entries = gxt_inspect.parse(path)
        gxt[lang] = {k for k, _, _ in entries}

    problemas = []
    for vid, model, game in vehiculos:
        clave = game.replace("_", " ")  # el motor cambia '_' por ' '
        if clave != game:
            problemas.append("ID %d (%s): el motor convertiría '%s' en '%s' al "
                             "buscar en el GXT; escribe la clave con espacio o "
                             "sin '_'" % (vid, model, game, clave))
        if len(clave) > MAXKEY:
            problemas.append("ID %d (%s): clave '%s' de %d caracteres (máx. %d "
                             "en el GXT)" % (vid, model, clave, len(clave), MAXKEY))
        if len(clave) > MAX_GAMENAME:
            problemas.append("ID %d (%s): clave '%s' de %d caracteres desborda "
                             "m_gameName[10] (%d máx.)"
                             % (vid, model, clave, len(clave), MAX_GAMENAME))
        if clave != clave.upper():
            problemas.append("ID %d (%s): clave '%s' no está en mayúsculas; el "
                             "GXT se busca con strcmp" % (vid, model, clave))
        for lang in idiomas:
            if clave not in gxt[lang]:
                if clave in GAPS_VANILLA:
                    continue
                problemas.append("ID %d (%s): la clave '%s' FALTA en %s.gxt "
                                 "(el HUD diría \"%s missing\")"
                                 % (vid, model, clave, lang, clave))

    print("%s: %d vehículos; GXT comprobados: %s"
          % (ide, len(vehiculos), ", ".join(idiomas)))
    print("claves distintas: %d" % len({g for _, _, g in vehiculos}))
    presentes = {g for _, _, g in vehiculos if g in GAPS_VANILLA}
    if presentes:
        print("huecos del VC original (no cuentan como problema): %s"
              % "; ".join("%s = %s" % (k, GAPS_VANILLA[k]) for k in sorted(presentes)))
    if problemas:
        print("\n%d PROBLEMA(S):" % len(problemas))
        for p in problemas:
            print("  - " + p)
        return 1
    print("OK: todas las claves existen, caben y están en mayúsculas")
    return 0


if __name__ == "__main__":
    sys.exit(main())
