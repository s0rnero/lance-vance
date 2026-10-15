#!/usr/bin/env python3
"""Familia 7 (`VICEEXT_MAPDATA`): el mundo del mod servido RESUELVE.

El fallo que este chequeo existe para no repetir (2026-10-14): un `.col`/IPL del
mod servido sin sus modelos hace que el juego se caiga al arrancar, y eso no se ve
en un `sha256` — el fichero esta, pero el mundo que describe no.

Alcance (a proposito, no se promete mas): las **piezas de mundo del mod** que el
port sirve, que son las unicas que este trabajo cambia:
  · `data/object.dat`   — sus entradas nombran modelos (formato por NOMBRE).
  · `data/occlu.ipl`    — solo volumenes numericos: no referencia modelos.
  · `data/maps/bryx/*`  — su IDE (`objs`/`tobj`) y su IPL (`inst`).

Se comprueba, para cada una:
  1. que la pieza este servida y sea la del mod (tamano + sha256),
  2. que cada modelo/TXD que nombra exista en el arbol servido,
  3. que cada `inst` del IPL apunte a un modelo declarado en el IDE (o servido).

Uso:
    python gta_vc_browser/tools/viceext-world-check.py
Exit 0 = las piezas del mod resuelven; 1 = algo del mundo se queda sin modelo.
"""
import hashlib
import io
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))      # gta_vc_browser/
REPO = os.path.dirname(RAIZ)
SERVIDO = os.path.join(RAIZ, "streamed")
MOD = os.path.join(REPO, "mods", "extended", "GameFiles", "modloader", "ViceExtended")
MODELOS = os.path.join(SERVIDO, "models")

# ruta servida | ruta en el mod | que es
PIEZAS = (
    ("data/object.dat", os.path.join(MOD, "object.dat"),
     "tabla de comportamiento de objetos (por NOMBRE)"),
    ("data/occlu.ipl", os.path.join(MOD, "occlu.ipl"),
     "zonas de oclusion (solo numeros)"),
    ("data/maps/bryx/bryx.ide", os.path.join(MOD, "maps", "bryx", "bryx.ide"),
     "objetos del bryx (tobj)"),
    ("data/maps/bryx/bryx.ipl", os.path.join(MOD, "maps", "bryx", "bryx.ipl"),
     "instancias del bryx (inst)"),
)


# Entradas del `object.dat` del mod sin modelo servido que YA estaban activas en el
# `object.dat` del port antes de servir el suyo (el delta del mod son tres lineas:
# `rcgrenade`, `ak47`, `m16`, y las tres resuelven). El motor no se cae con una
# entrada cuyo modelo no llega, y quitarlas es decision aparte (no de esta familia):
# aqui se miden y se dejan dichas. Cualquier nombre NUEVO sin modelo es un fallo.
SIN_MODELO_YA_CONOCIDOS = {
    "nbspraydoor", "nbdcoshpwin", "nbdcoshpwin2", "nbdcoshpwin3", "lh_showdoor02",
}


def leer(p, binario=False):
    if binario:
        with io.open(p, "rb") as f:
            return f.read()
    with io.open(p, "r", encoding="utf-8", errors="replace") as f:
        return f.read().replace("\r\n", "\n")


def huella(p):
    try:
        d = leer(p, True)
    except OSError:
        return None, None
    return hashlib.sha256(d).hexdigest()[:12], len(d)


def existe_modelo(nombre, ext):
    """.dff/.txd del modelo en el arbol servido (sin distinguir mayusculas)."""
    objetivo = ("%s.%s" % (nombre, ext)).lower()
    for base in (os.path.join(MODELOS, "gta3.img"), MODELOS):
        if not os.path.isdir(base):
            continue
        try:
            for f in os.listdir(base):
                if f.lower() == objetivo:
                    return os.path.join(base, f)
        except OSError:
            continue
    return None


def nombres_ide(texto):
    """[(seccion, modelo, txd)] de un IDE (`objs` y `tobj`)."""
    salida = []
    seccion = None
    for l in texto.split("\n"):
        l = l.strip()
        if not l or l.startswith("#"):
            continue
        if l.lower() in ("objs", "tobj", "weap", "anim", "peds", "cars"):
            seccion = l.lower()
            continue
        if l.lower() == "end":
            seccion = None
            continue
        if seccion not in ("objs", "tobj"):
            continue
        partes = [p.strip() for p in l.split(",")]
        if len(partes) >= 3:
            salida.append((seccion, partes[1], partes[2]))
    return salida


def modelos_object_dat(texto):
    """Nombres de la primera columna (las lineas `;` estan apagadas)."""
    salida = []
    for l in texto.split("\n"):
        l = l.strip()
        if not l or l.startswith(";") or l.startswith("#") or l.startswith("*"):
            continue
        partes = re.split(r"[\s,]+", l)
        if partes and partes[0]:
            salida.append(partes[0])
    return salida


def instancias_ipl(texto):
    """Nombres de modelo de la seccion `inst`."""
    salida = []
    seccion = None
    for l in texto.split("\n"):
        l = l.strip()
        if not l or l.startswith("#"):
            continue
        if l.lower() in ("inst", "cull", "zone", "occl", "path"):
            seccion = l.lower()
            continue
        if l.lower() == "end":
            seccion = None
            continue
        if seccion != "inst":
            continue
        partes = [p.strip() for p in l.split(",")]
        if len(partes) >= 2:
            salida.append(partes[1])
    return salida


def main():
    fallos = []
    print("== familia 7: piezas de mundo del mod servidas ==")
    cache = {}
    for servida, origen, que in PIEZAS:
        f = os.path.join(SERVIDO, servida)
        h_serv, t_serv = huella(f)
        h_mod, t_mod = huella(origen)
        if h_serv is None:
            fallos.append("%s no esta servido" % servida)
            print("   FALTA %-28s %s" % (servida, que))
            continue
        if h_serv != h_mod:
            fallos.append("%s servido (%s) NO es el del mod (%s)" % (servida, h_serv, h_mod))
            print("   FALTA %-28s servido %s, el del mod es %s  <-- %s"
                  % (servida, h_serv, h_mod, que))
            continue
        cache[servida] = leer(f)
        print("   OK    %-28s %s  (%s, %d B)" % (servida, que, h_serv, t_serv))

    print("== las referencias de esas piezas resuelven ==")
    # bryx: los modelos de su IDE y las instancias de su IPL
    ide = cache.get("data/maps/bryx/bryx.ide", "")
    ipl = cache.get("data/maps/bryx/bryx.ipl", "")
    declarados = {}
    for seccion, modelo, txd in nombres_ide(ide):
        declarados[modelo] = txd
        for nombre, ext in ((modelo, "dff"), (txd, "txd")):
            ruta = existe_modelo(nombre, ext)
            if ruta is None:
                fallos.append("bryx.ide: %s.%s no esta en el arbol servido" % (nombre, ext))
                print("   FALTA bryx.ide %s.%s  <-- el mundo se cae al arrancar" % (nombre, ext))
            else:
                print("   OK    bryx.ide %s.%s" % (nombre, ext))
    for modelo in instancias_ipl(ipl):
        if modelo not in declarados and existe_modelo(modelo, "dff") is None:
            fallos.append("bryx.ipl: la instancia %s no esta declarada ni servida" % modelo)
            print("   FALTA bryx.ipl inst %s  <-- instancia sin modelo" % modelo)
        else:
            print("   OK    bryx.ipl inst %s" % modelo)

    # object.dat: cada nombre con entrada tiene que existir como modelo
    od = cache.get("data/object.dat", "")
    nombres = modelos_object_dat(od)
    sin_modelo = [n for n in nombres if existe_modelo(n, "dff") is None]
    nuevos = [n for n in sin_modelo if n not in SIN_MODELO_YA_CONOCIDOS]
    print("   object.dat: %d entradas activas, %d sin modelo servido (%d ya conocidas)"
          % (len(nombres), len(sin_modelo), len(sin_modelo) - len(nuevos)))
    for n in sin_modelo:
        if n in SIN_MODELO_YA_CONOCIDOS:
            print("   AVISO object.dat %s  <-- ya estaba asi en el port (no es de esta familia)" % n)
    if nuevos:
        fallos.append("object.dat: sin modelo servido: %s" % ",".join(nuevos[:8]))
        for n in nuevos[:8]:
            print("   FALTA object.dat %s  <-- entrada sin modelo" % n)

    # occlu.ipl: solo numeros; se comprueba que tenga zonas y que su cabecera sea la esperada
    oc = cache.get("data/occlu.ipl", "")
    lineas = [l for l in oc.split("\n") if l.strip()]
    zonas = len([l for l in lineas[1:] if re.match(r"^[-0-9.,\s]+$", l.strip())])
    print("   occlu.ipl: cabecera=%r, %d zona(s) numericas" % (lineas[0].strip() if lineas else "", zonas))
    if not lineas or lineas[0].strip().lower() != "occl" or zonas < 100:
        fallos.append("occlu.ipl: cabecera o zonas raras (%d)" % zonas)

    print()
    if fallos:
        for f in fallos:
            print("   FAIL: %s" % f)
        print("MUNDO FALLA (%d referencia(s))" % len(fallos))
        return 1
    print("MUNDO OK - las piezas del mod estan servidas y todas sus referencias resuelven")
    return 0


if __name__ == "__main__":
    sys.exit(main())
