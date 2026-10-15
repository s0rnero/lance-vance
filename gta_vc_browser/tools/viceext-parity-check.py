#!/usr/bin/env python3
"""Paridad de DATOS del mod Vice Extended contra lo que sirve el port.

Para que: la consola del port puede servir datos del mod y no del original, y
mas de una vez se ha dado por hecho sin medirlo. Esta comprobacion recorre el
overlay del mod tal como se entrega (`GameFiles/ViceExtended/` y
`GameFiles/modloader/ViceExtended/`), lo compara fichero a fichero con el arbol
servido (`streamed/`, donde el overlay va APLANADO en la raiz) y **clasifica**
cada diferencia. Una diferencia sin clasificar es una regresion: o el port se ha
dejado por el camino un dato del mod, o alguien ha servido algo que no tocaba.

Auditoria del 2026-10-14 (tras servir los datos de las familias 13 y 3/5): los
tres huecos que quedaban sin codigo ya no son diferencia, porque el port sirve
`data/particle.cfg`, `models/coll/newWeapons.col` y `data/main.scm` bit a bit del
mod (si vuelven a aparecer, es una regresion); unas pocas son datos de mundo
pendientes de decision (familia 5/7: `gta_vc.dat`, `object.dat`, `occlu.ipl`,
`maps/`) y el resto no se sirve a proposito (los dos ini, `ViceEx.dat`, arte de
mandos, el taller excluido) o ya entra por otra via (vehiculos y armas nuevas
importadas, el banco de sonido en `Audio/sfx`). Aparte queda una clase `pendiente`
para las decisiones ya conocidas (los GXT: plan `mods/07` seccion 14). Ese reparto
deja pasar el check.

Uso:
    python tools/viceext-parity-check.py

Salida: los tres recuentos, una linea por diferencia (clase, hash y tamano de
cada lado) y el veredicto. Exit 0 = toda diferencia esta clasificada; exit 1 =
hay alguna sin explicar (se listan al final).
"""
import hashlib
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # gta_vc_browser/
REPO = os.path.dirname(RAIZ)                                        # raiz del port
SERVIDO = os.path.join(RAIZ, "streamed")
FUENTES = [
    (os.path.join(REPO, "mods", "extended", "GameFiles", "ViceExtended"), "overlay"),
    (os.path.join(REPO, "mods", "extended", "GameFiles", "modloader", "ViceExtended"),
     "modloader"),
]
# El contenido de los IMG del mod se funde en esta carpeta (IMG virtual).
GTA3IMG = os.path.join(SERVIDO, "models", "gta3.img")

# Nombres que el port NO sirve a proposito.
BY_DESIGN_BASE = {
    # configuracion del mod: el port la lleva en src/core/config.h (DESIGN 5).
    # `features.ini` YA NO esta aqui: desde la familia 11 el port lo sirve y lo
    # lee al arrancar (`src/core/ViceExtFeatures.cpp`), asi que cuenta como
    # identico al del mod y su dato servido lo vigila `check-served-build.sh`.
    "limits.ini", "ViceEx.dat", "ViceEx.SDT",
    "desktop.ini", "Changes.txt",
    # arte de mandos: el port sirve su propio juego de botones
    "ps4btns.txd", "ps5btns.txd", "x1btns.txd",
    "frontend_ds2.txd", "frontend_ds3.txd", "frontend_ds4.txd", "frontend_ds5.txd",
    "frontend_x360.txd", "frontend_xone.txd", "frontend_nsw.txd",
    "fronten2.txd", "fonts_u.txd", "ukrainian.gxt", "newspapers.txd",
    # ruedas: las del mod son el juego del taller, que esta fuera del alcance
    "wheels.DFF", "wheels.TXD",
}
BY_DESIGN_MOTIVO = {
    "wheels.DFF": "taller excluido: la version del mod solo aporta con el taller",
    "wheels.TXD": "taller excluido: la version del mod solo aporta con el taller",
}

# Ya entra por otra via (no se sirve como fichero, pero su contenido si esta).
CUBIERTO_BASE = {
    "weapon.dat",     # el servido ES el del mod: solo cambian los fin de linea
    "default.ide",    # el port construye su propia version fusionada
    "default.dat",    # idem: anade la linea COLFILE de NEWVEHICLES.COL
    "ped.ifp",        # el servido mezcla clips de otros mods
    "ViceEx.RAW",     # su contenido vive en streamed/Audio/sfx (ids 9941-9953)
}
# Vehiculos nuevos: el port los integro por su propia via, no copiando ficheros.
CUBIERTO_PREFIJO = ("newVehicles/",)
CUBIERTO_MOTIVO = {
    "newVehicles/": "sus 8 vehiculos (6500-6507) van en el default.ide, handling.cfg "
                    "y carcols.dat servidos, los modelos en streamed/models/gta3.img/ "
                    "y newvehicles.col es identico al servido (48f417725a60)",
}

# Decision pendiente YA conocida: no es una regresion nueva.
PENDIENTE_BASE = {
    "american.gxt", "french.gxt", "german.gxt",
    "italian.gxt", "russian.gxt", "spanish.gxt",
}
PENDIENTE_MOTIVO = "GXT: decision pendiente ya conocida (plan mods/07 seccion 14): " \
                   "el servido lleva los cambios propios del port (~214-258 B de delta)"

# Huecos: vacio. Los tres que estaban aqui (`particle.cfg`, `newWeapons.col` y
# `main.scm`) los sirve ya el port, bit a bit del mod (2026-10-14).

# Datos de mundo: diferencia real del mod, pendiente de decision (familia 5/7).
MUNDO_BASE = {"gta_vc.dat", "object.dat", "occlu.ipl"}
MUNDO_PREFIJO = ("maps/",)
MUNDO_MOTIVO = ("familia 7 (mundo): el mundo del mod YA se sirve (object.dat, occlu.ipl y "
                "maps/bryx/*, vigilado por `tools/viceext-world-check.py`), pero su "
                "`gta_vc.dat` no puede sustituir al servido: el suyo abre con el bloque "
                "`CDIMAGE MODELS\\cdimages\\*.img` (8 lineas) que en este port no existe "
                "porque sus IMG van fundidos en `models/gta3.img/`; el servido lista los "
                "mismos IDE/IPL y ademas `COLFILE NEWVEHICLES.COL`")


def huella(p):
    """(sha256[:12], bytes) del fichero, o (None, None) si no existe."""
    try:
        with open(p, "rb") as f:
            d = f.read()
    except OSError:
        return None, None
    return hashlib.sha256(d).hexdigest()[:12], len(d)


def servido_de(rel, origen):
    """Ruta servida equivalente a `rel` (el port aplana el overlay en la raiz)."""
    if rel.startswith("cdimages/"):
        return os.path.join(SERVIDO, "models", os.path.basename(rel))
    if origen == "overlay" and rel == "features.ini":
        # La configuracion del mod vive en `<juego>/ViceExtended/features.ini` (su
        # lector la abre asi, y el port desde la familia 11 igual:
        # `src/core/ViceExtFeatures.cpp`). Es la unica pieza del overlay que NO se
        # aplana, para que la ruta del port sea la del mod y el jugador edite el
        # fichero donde el mod dice que esta.
        return os.path.join(SERVIDO, "ViceExtended", "features.ini")
    if origen == "modloader":
        base = os.path.basename(rel)
        if base in ("gta_vc.dat", "info.zon", "map.zon", "navig.zon", "object.dat",
                    "occlu.ipl"):
            return os.path.join(SERVIDO, "data", base)
        if rel.startswith("maps/"):
            return os.path.join(SERVIDO, "data", rel)
    return os.path.join(SERVIDO, rel)


def clasifica(rel):
    """(clase, motivo) de una diferencia, o None si no esta clasificada."""
    base = os.path.basename(rel)
    if base in BY_DESIGN_BASE:
        return "by-design", BY_DESIGN_MOTIVO.get(base, "el port no lo sirve a proposito")
    if rel.startswith("models/vehmods/"):
        return "by-design", "taller excluido: piezas de tuning (spoilers/skirts/vents/scoops)"
    if rel.startswith("cdimages/"):
        if os.path.isdir(GTA3IMG):
            return "cubierto", "su contenido va fundido en streamed/models/gta3.img/"
        return None
    if base in CUBIERTO_BASE:
        return "cubierto", "ya entra por otra via"
    for pre in CUBIERTO_PREFIJO:
        if rel.startswith(pre):
            return "cubierto", CUBIERTO_MOTIVO[pre]
    if base in PENDIENTE_BASE:
        return "pendiente", PENDIENTE_MOTIVO
    if base in MUNDO_BASE or rel.startswith(MUNDO_PREFIJO):
        return "mundo", MUNDO_MOTIVO
    return None


def main():
    if not os.path.isdir(SERVIDO):
        print("ERROR: falta %s" % SERVIDO)
        return 1

    identicos = distintos = ausentes = 0
    lineas = []
    sin_clasificar = []
    por_clase = {}

    for fuente, origen in FUENTES:
        if not os.path.isdir(fuente):
            print("ERROR: falta %s" % fuente)
            return 1
        for raiz, _dirs, ficheros in os.walk(fuente):
            for f in sorted(ficheros):
                s = os.path.join(raiz, f)
                rel = os.path.relpath(s, fuente).replace("\\", "/")
                ha, ta = huella(s)
                hb, tb = huella(servido_de(rel, origen))
                if hb is None:
                    estado = "AUSENTE"
                    ausentes += 1
                elif ha == hb:
                    identicos += 1
                    continue
                else:
                    estado = "DISTINTO"
                    distintos += 1
                par = clasifica(rel)
                if par is None:
                    sin_clasificar.append(rel)
                    clase, motivo = "SIN-CLASIFICAR", "revisar: dato del mod que no llega"
                else:
                    clase, motivo = par
                    por_clase[clase] = por_clase.get(clase, 0) + 1
                lineas.append("   %-8s %-34s %-14s %s/%s -> %s/%s  [%s] %s"
                              % (estado, rel, clase, ha, ta, hb or "-", tb or "-",
                                 clase, motivo))

    print("overlay del mod -> servido: identicos=%d distintos=%d ausentes=%d"
          % (identicos, distintos, ausentes))
    print("no identicos: %d (por clase: %s)"
          % (distintos + ausentes,
             ", ".join("%s=%d" % kv for kv in sorted(por_clase.items())) or "-"))
    for l in lineas:
        print(l)
    print()

    if sin_clasificar:
        for r in sin_clasificar:
            print("   FAIL: %s no esta clasificado (diferencia nueva o dato perdido)" % r)
        print("PARIDAD FALLA (%d sin explicar)" % len(sin_clasificar))
        return 1

    print("PARIDAD OK - %d diferencias, todas clasificadas (%s)"
          % (distintos + ausentes,
             ", ".join("%s=%d" % kv for kv in sorted(por_clase.items()))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
