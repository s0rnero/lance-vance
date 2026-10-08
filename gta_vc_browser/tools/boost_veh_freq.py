#!/usr/bin/env python3
"""Sube (o restaura) la FRECUENCIA de tráfico de los vehículos del mod.

Por qué existe: los 7 vehículos nuevos ya se sirven con la frecuencia que trae
el mod (10/7, igual que los de serie), así que aparecen por la calle pero
repartidos entre todos los modelos de su clase. Para **verlos al jugar** (y para
poder medir el bloque D4 a ojo) conviene subirles el peso una temporada y
después devolverlo a su valor.

Cómo funciona: la columna 9 de una línea de coche de `default.ide`
(`CVehicleModelInfo::m_frequency`, leída por `CFileLoader::LoadVehicleObject`)
es el peso con el que `CCarCtrl::ChooseCarModel` sortea ese modelo **dentro de
su clase**. Subirla de 10 a 100 multiplica por ~10 su probabilidad relativa.

Guardas: antes del primer cambio apunta los valores originales en
`tools/veh_freq_backup.json`; `--revert` los restaura **exactamente** desde ahí.
El fichero no se regenera solo: cualquier cambio en `default.ide` exige
`python tools/stage_bootseed.py` y **recompilar** (el directorio `data/` va en el
`reVC.data`), además de subir `dataTag`/`VERSION`.

Uso:
    python tools/boost_veh_freq.py                 # 6500..6506 -> 100
    python tools/boost_veh_freq.py --freq 250
    python tools/boost_veh_freq.py --revert        # valores del mod (10/7)
    python tools/boost_veh_freq.py --dry-run       # enseña el cambio y no escribe
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
IDE = os.path.join(HERE, "..", "streamed", "data", "default.ide")
BACKUP = os.path.join(HERE, "veh_freq_backup.json")

# Los 7 con clase real de tráfico (6507, la moto policial, es `ignore`: no debe
# salir en la calle por diseño, así que no se toca).
IDS = [6500, 6501, 6502, 6503, 6504, 6505, 6506]
# Columna de la frecuencia dentro de la línea (0 = id, ... 8 = frecuencia).
COL_FREQ = 8


def lineas_vehiculo(texto):
    """Devuelve {id: contenido de la línea} de las líneas `default` de coches."""
    out = {}
    for linea in texto.split("\n"):
        m = re.match(r"\s*(\d+)\s*,", linea)
        if m:
            out[int(m.group(1))] = linea
    return out


def campos(linea):
    """Parte la línea por comas conservando los separadores (índices pares=valor)."""
    return re.split(r"(,)", linea)


def leer_freq(linea):
    """Frecuencia actual (campo 8: id, model, txd, type, handling, gamename,
    anims, class, frequency, ...), tal como la lee el sscanf de CFileLoader."""
    partes = campos(linea)
    if COL_FREQ * 2 >= len(partes):
        raise SystemExit("ERROR: línea con menos de 9 campos: %s" % linea)
    m = re.search(r"\d+", partes[COL_FREQ * 2])
    if not m:
        raise SystemExit("ERROR: no encuentro la frecuencia en: %s" % linea)
    return int(m.group(0))


def cambiar_freq(linea, nuevo):
    """Sustituye la frecuencia conservando tabulaciones y todo lo demás."""
    partes = campos(linea)
    idx = COL_FREQ * 2
    viejo = leer_freq(linea)
    partes[idx] = re.sub(r"\d+", str(nuevo), partes[idx], count=1)
    return "".join(partes), viejo


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--freq", type=int, default=100, help="peso nuevo (100)")
    ap.add_argument("--revert", action="store_true", help="restaura los valores guardados")
    ap.add_argument("--ids", default=",".join(str(i) for i in IDS),
                    help="ids separados por comas")
    ap.add_argument("--dry-run", action="store_true", help="enseña el cambio y no escribe")
    ap.add_argument("--show", action="store_true", help="sólo enseña las frecuencias actuales")
    a = ap.parse_args()

    ids = [int(x) for x in a.ids.split(",") if x.strip()]
    with open(IDE, "r", encoding="latin1") as f:
        texto = f.read()
    antes = lineas_vehiculo(texto)

    if a.show:
        for i in ids:
            if i in antes:
                print("   %d: frecuencia %d" % (i, leer_freq(antes[i])))
        return 0

    if a.revert:
        if not os.path.exists(BACKUP):
            print("FAIL: no hay %s (no se ha subido nada todavía)" % BACKUP)
            return 1
        with open(BACKUP) as f:
            objetivo = {int(k): v for k, v in json.load(f).items()}
        print("== restaurando frecuencias originales del mod")
    else:
        objetivo = {i: a.freq for i in ids}
        # primera vez: apuntar los originales
        if not a.dry_run and not os.path.exists(BACKUP):
            originales = {i: leer_freq(antes[i]) for i in ids if i in antes}
            os.makedirs(os.path.dirname(BACKUP), exist_ok=True)
            with open(BACKUP, "w") as f:
                json.dump(originales, f, indent=1, sort_keys=True)
            print("== guardados los originales en %s: %s"
                  % (os.path.relpath(BACKUP, HERE), originales))
        print("== subiendo frecuencias a %d" % a.freq)

    salida = []
    for linea in texto.split("\n"):
        m = re.match(r"\s*(\d+)\s*,", linea)
        if m and int(m.group(1)) in objetivo:
            nueva, viejo = cambiar_freq(linea, objetivo[int(m.group(1))])
            print("   %d: frecuencia %d -> %d" % (int(m.group(1)), viejo, objetivo[int(m.group(1))]))
            salida.append(nueva)
        else:
            salida.append(linea)
    nuevo_texto = "\n".join(salida)

    if a.dry_run:
        print("== --dry-run: no se escribe nada")
        return 0
    if nuevo_texto == texto:
        print("== sin cambios (ya estaba así)")
        return 0
    with open(IDE, "w", encoding="latin1", newline="") as f:
        f.write(nuevo_texto)
    print("== escrito %s" % os.path.relpath(IDE, HERE))
    print("== recuerda: python tools/stage_bootseed.py && recompilar (data/ va en el .data)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
