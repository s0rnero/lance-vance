#!/usr/bin/env python3
"""Descifra el censo de audio (T114) del log de traza.

Convierte los numeros del log en nombres legibles:
  * sfx=<id>        -> SFX_NOMBRE   (enum de src/audio/AudioSamples.h)
  * create=/prod=   -> fichero:linea nombre-funcion   (linea de __LINE__ del productor)
  * DRUNKCL/MAREO/CUTSKIP -> el estado del efecto de pantalla en ese momento
  * RGPASS               -> ms por pase de render (quien se come la fase S)
  * LOOPSET/PEDCAM/ODCHCLASH -> que bucle entrega OpenAL, como se mueve la camara
    y que diferido de sample se ha perdido por reutilizar el canal

Uso:
    python gta_vc_browser/tools/sfx-audit-check.py [log]        # por defecto web/odtrace.log
    python gta_vc_browser/tools/sfx-audit-check.py --top 30 log
"""

import os
import re
import sys

RAIZ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SAMPLES = os.path.join(RAIZ, "src", "audio", "AudioSamples.h")
PRODUCTORES = [
    os.path.join("src", "audio", "AudioLogic.cpp"),
    os.path.join("src", "audio", "AudioManager.cpp"),
    os.path.join("src", "audio", "AudioCollision.cpp"),
    os.path.join("src", "audio", "PolRadio.cpp"),
]

RE_ID = re.compile(
    r"^\t(?:FIRST_PLAYER_COMMENT|LAST_PLAYER_COMMENT)\((SFX_[A-Za-z0-9_]+)\),|^\t(SFX_[A-Za-z0-9_]+)(?: *= *\d+)?,")
RE_FUNC = re.compile(r"^[A-Za-z_~][\w:<>,\s\*&]*?([A-Za-z_~][\w<>:]*)\s*\(")
RE_ARG = re.compile(r"(create|prod)=(\d+)")
RE_SFX = re.compile(r"(?:sfx|top|SNDLIST sfx)=(\d+)")
RE_TAG = re.compile(r"^(?:\d{4}-\d\d-\d\dT\S+\s+)?([A-Z][A-Z0-9]*)\b")
RE_LIST = re.compile(r"SNDLIST sfx=(\d+) reqs=(\d+) births=(\d+) ms=(\d+) max=(\d+)")
RE_CALL = re.compile(r"AddSampleToRequestedQueue\s*\(\s*\)")


def etiqueta_de(fila):
    """El log antepone un reloj UTC a cada fila; esto devuelve solo la marca."""
    m = RE_TAG.match(fila)
    return m.group(1) if m else ""


def nombres_muestra():
    """id -> nombre de muestra, en el orden exacto del enum eSfxSample."""
    nombres = {}
    if not os.path.exists(SAMPLES):
        return nombres
    with open(SAMPLES, encoding="utf-8", errors="replace") as fh:
        texto = fh.read()
    inicio = texto.find("TOTAL_AUDIO_SAMPLES")
    if inicio < 0:
        return nombres
    n = 0
    for linea in texto[:inicio].split("\n"):
        m = RE_ID.match(linea)
        if m:
            nombres[n] = m.group(1) or m.group(2)
            n += 1
    return nombres


def funciones():
    """(fichero, linea) -> nombre de la funcion que la contiene."""
    tabla = {}
    for rel in PRODUCTORES:
        ruta = os.path.join(RAIZ, rel)
        if not os.path.exists(ruta):
            continue
        actual = None
        con_llaves = False
        with open(ruta, encoding="utf-8", errors="replace") as fh:
            for i, linea in enumerate(fh, 1):
                sin = linea.rstrip("\n")
                if not sin or sin.lstrip().startswith("//"):
                    continue
                if sin[0] not in (" ", "\t", "#", "}"):
                    if "(" in sin:
                        m = RE_FUNC.match(sin)
                        if m:
                            actual = m.group(1)
                            con_llaves = False
                        elif sin.rstrip().endswith(")") and actual:
                            pass
                    elif sin.rstrip() == "" :
                        pass
                if actual and sin.rstrip() == "{":
                    con_llaves = True
                if sin[0] == "}":
                    if con_llaves:
                        actual = None
                        con_llaves = False
                if actual:
                    tabla[(rel, i)] = actual
    return tabla


def llamadas():
    """(fichero, linea) solo donde hay una llamada real al productor.

    `create=`/`prod=` son __LINE__ sin fichero, y el mismo numero existe en los
    cuatro; sin esto la linea se atribuia al primer fichero que la cubriera.
    """
    sitios = set()
    for rel in PRODUCTORES:
        ruta = os.path.join(RAIZ, rel)
        if not os.path.exists(ruta):
            continue
        with open(ruta, encoding="utf-8", errors="replace") as fh:
            for i, linea in enumerate(fh, 1):
                if RE_CALL.search(linea):
                    sitios.add((rel, i))
    return sitios


def etiqueta(linea, nombres, funcs, sitios):
    """Una linea de productor -> fichero:linea funcion(), sin ambiguedad."""
    hits = [rel for rel in PRODUCTORES if (rel, linea) in sitios]
    for rel in hits:
        nombre = funcs.get((rel, linea))
        if nombre:
            return "%s:%d %s()" % (rel, linea, nombre)
    if hits:
        return "%s:%d" % (hits[0], linea)
    return "linea %d (sin llamada al productor en los ficheros de audio)" % linea


def main():
    argv = [a for a in sys.argv[1:] if not a.startswith("--")]
    top = 40
    i = 0
    while i < len(sys.argv):
        a = sys.argv[i]
        if a.startswith("--top"):
            if "=" in a:
                top = int(a.split("=", 1)[1])
            elif i + 1 < len(sys.argv):
                top = int(sys.argv[i + 1])
                argv = [x for x in argv if x != sys.argv[i + 1]]
        i += 1
    log = argv[0] if argv else os.path.join(RAIZ, "gta_vc_browser", "web", "odtrace.log")
    if not os.path.exists(log):
        print("no existe el log:", log)
        return 1

    nombres = nombres_muestra()
    funcs = funciones()
    sitios = llamadas()

    def sfx(n):
        return "%s(%s)" % (nombres.get(int(n), "?"), n)

    filas = []
    with open(log, encoding="utf-8", errors="replace") as fh:
        for linea in fh:
            if any(k in linea for k in ("SNDTOP", "SNDLIST", "SNDNEW", "SNDDIE",
                                        "SNDSTUCK", "LOOPLONG", "LOOPKILL", "LOOPCOLLIDE",
                                        "LOOPORPHAN", "DRUNKCL", "MAREO", "CUTSKIP", "RGPASS",
                                        "LOOPSET", "PEDCAM", "ODCHCLASH")):
                filas.append(linea.strip())

    if not filas:
        print("El log no tiene filas del censo (T115). ¿Build anterior a 43.0-T115?")
        return 0

    print("log:", log)
    print("muestras nombradas en el enum:", len(nombres))
    print()

    tops = [f for f in filas if etiqueta_de(f) == "SNDTOP"]
    if tops:
        ultima = tops[-1]
        m = re.search(r"dt=(\d+)", ultima)
        dt = int(m.group(1)) if m else 15000
        if "rq=" in ultima:
            secciones = ultima.split("br=")
            rq_txt = secciones[0]
            br_txt = secciones[-1] if len(secciones) > 1 else None
            print("=== Censo periodico (ultima fila, ventana %d ms) ===" % dt)
            for token in re.findall(r"(\d+):\+(\d+)/(\d+)ms", rq_txt):
                print("   %-38s peticiones=%-8s (%.1f/s)  ms_reproduciendo=%s" %
                      (sfx(token[0]), token[1], int(token[1]) * 1000.0 / dt, token[2]))
            if br_txt:
                print("   -- nacimientos en la ventana (one-shot re-disparado = suena a bucle) --")
                for token in re.findall(r"(\d+):\+(\d+)", br_txt):
                    print("   %-38s births=%-8s (%.1f/s)" %
                          (sfx(token[0]), token[1], int(token[1]) * 1000.0 / dt))
        else:
            print("=== Censo periodico (ultima fila) ===")
            print(ultima)
            for token in re.findall(r"(\d+):(\d+)/(\d+)ms", ultima):
                print("   %-38s peticiones=%-8s ms_reproduciendo=%s" %
                      (sfx(token[0]), token[1], token[2]))
        print()

    lists = [f for f in filas if etiqueta_de(f) == "SNDLIST"]
    if lists:
        print("=== Volcado completo (SNDLIST) ===")
        for f in lists[:top]:
            m = RE_LIST.search(f)
            if m:
                print("   %-40s reqs=%-7s births=%-7s ms=%-9s max=%s" %
                      (sfx(m.group(1)), m.group(2), m.group(3), m.group(4), m.group(5)))
        if len(lists) > top:
            print("   ... %d filas mas" % (len(lists) - top))
        print()

    stuck = [f for f in filas if etiqueta_de(f) in ("SNDSTUCK", "LOOPLONG", "LOOPKILL",
                                                    "LOOPCOLLIDE", "LOOPORPHAN")]
    print("=== Bucles y efectos largos (%d filas) ===" % len(stuck))
    for f in stuck[:top]:
        producers = RE_ARG.findall(f)
        cab = f.split(" create=")[0]
        if producers:
            det = "; ".join("%s -> %s" % (k, etiqueta(int(v), nombres, funcs, sitios)) for k, v in producers)
            print("   %s | %s" % (cab, det))
        else:
            print("   %s" % f)
    if len(stuck) > top:
        print("   ... %d filas mas" % (len(stuck) - top))
    print()

    muertos = [f for f in filas if etiqueta_de(f) == "SNDDIE"]
    print("=== Sonidos que vivieron >=3 s (%d filas, ultimas 15) ===" % len(muertos))
    for f in muertos[-15:]:
        producers = RE_ARG.findall(f)
        cab = f.split(" create=")[0]
        col = re.search(r"ent=(-?\d+) vivo=(-?\d+)", f)
        det = "; ".join("%s -> %s" % (k, etiqueta(int(v), nombres, funcs, sitios)) for k, v in producers)
        if col:
            det += "  |  %s (entidad viva=%s)" % (col.group(0), col.group(2))
        print("   %s | %s" % (cab, det))
    if any("vivo=0" in f for f in muertos):
        print("   ATENCION: hay SNDDIE con vivo=0 -> el sonido sobrevivio a su entidad emisora.")
    print()

    nuevos = [f for f in filas if etiqueta_de(f) == "SNDNEW"]
    print("=== Nacimientos de bucle (%d filas, ultimas 12) ===" % len(nuevos))
    for f in nuevos[-12:]:
        producers = RE_ARG.findall(f)
        cab = f.split(" create=")[0]
        det = "; ".join("%s -> %s" % (k, etiqueta(int(v), nombres, funcs, sitios)) for k, v in producers)
        print("   %s | %s" % (cab, det))
    print()

    otros = [f for f in filas if etiqueta_de(f) in ("DRUNKCL", "MAREO", "CUTSKIP")]
    if otros:
        print("=== Efectos de pantalla (mareo) ===")
        for f in otros[-25:]:
            print("   %s" % f)
        print()

    pases = [f for f in filas if etiqueta_de(f) == "RGPASS"]
    if pases:
        acm = {}
        calls = {}
        maximo = {}
        ventana_ms = 0.0
        historico = 0
        for f in pases:
            if "schema=2" in f:
                m = re.search(r"name=(\S+) ovf=", f)
                w = re.search(r"window_ms=(\d+)", f)
                c = re.search(r"calls=(\d+)", f)
                t = re.search(r"total_ms=([\d.]+)", f)
                x = re.search(r"max_ms=([\d.]+)", f)
                if not (m and w and c and t and x):
                    historico += 1
                    continue
                nombre = m.group(1)
                acm[nombre] = acm.get(nombre, 0.0) + float(t.group(1))
                calls[nombre] = calls.get(nombre, 0) + int(c.group(1))
                maximo[nombre] = max(maximo.get(nombre, 0.0), float(x.group(1)))
                ventana_ms = max(ventana_ms, float(w.group(1)))
                continue
            for ms, nombre in re.findall(r"([\d.]+)=([^;]+);", f):
                nombre = nombre.strip()
                historico += 1
        print("=== Coste por pase de render (schema=2, %d ventanas reales de ~2 s%s) ===" % (len(pases), ", %d historicas no fiables" % historico if historico else ""))
        ranking = sorted(acm, key=lambda k: -acm[k])
        total = sum(acm.values()) or 1.0
        for nombre in ranking[:top]:
            n = calls.get(nombre, 0) or 1
            print("   %7.3f ms/call  %7.3f max  %8.2f ms/s  %5.1f%%  %s" % (acm[nombre] / n, maximo.get(nombre, 0.0), acm[nombre] * 1000.0 / max(ventana_ms, 1.0), acm[nombre] * 100.0 / total, nombre))
        print()

    loopset = [f for f in filas if etiqueta_de(f) == "LOOPSET"]
    if loopset:
        por = {}
        raros = []
        viejo = 0
        for f in loopset:
            m = re.search(r"ch=(\d+) sfx=(\d+) nsamp=(\d+) lp=(\d+),(\d+) freq=(\d+) cnt=(\d+)", f)
            if not m:
                viejo += 1
                continue
            ch, sid, nsamp, ini, fin, freq, cnt = [int(x) for x in m.groups()]
            dur = 1000.0 * nsamp / freq if freq else 0.0
            por[sid] = por.get(sid, 0) + 1
            if cnt == 0 and ini == 0 and fin == nsamp - 1 and dur < 1500.0:
                raros.append((dur, sid, ch, f.split("LOOPSET")[1].strip()))
        print("=== Region de bucle entregada a OpenAL (%d filas%s) ===" % (
            len(loopset), "" if viejo == 0 else ", %d sin sfx= : build anterior a 44.0-T117" % viejo))
        for sid, n in sorted(por.items(), key=lambda kv: -kv[1])[:top]:
            print("   %-38s filas=%d" % (sfx(sid), n))
        if raros:
            print("   ATENCION: bucle infinito que cubre TODA una muestra corta (suena como")
            print("   un disparo repetido; si ademas no esta en SNDNEW, el bucle lo sostiene")
            print("   OpenAL, no el gestor).")
            for dur, sid, ch, cuerpo in sorted(raros)[:top]:
                print("   %6.0f ms  %-30s ch=%-3s %s" % (dur, sfx(sid), ch, cuerpo))
        print()

    pedcam = [f for f in filas if etiqueta_de(f) == "PEDCAM"]
    if pedcam:
        print("=== Vaiven de camara (%d ventanas de 1 s, ultimas %d) ===" % (len(pedcam), top))
        for f in pedcam[-top:]:
            print("   %s" % f.split("PEDCAM")[1].strip())
        print()

    clashes = [f for f in filas if etiqueta_de(f) == "ODCHCLASH"]
    if clashes:
        print("=== Diferidos aplastados por otra muestra (%d) ===" % len(clashes))
        for f in clashes[-15:]:
            print("   %s" % f.split("ODCHCLASH")[1].strip())
        print()

    print("Lectura: un bucle pegado es el que aparece en LOOPLONG/SNDSTUCK con `prod` distinto")
    print("de `create` (otro productor lo renueva cada frame) o con `refresco` alto y `reqs`")
    print("creciendo. Si `prod` es la linea de AddReleasingSounds, el bucle no lo pide ya")
    print("nadie: es la cola de liberacion la que no termina.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
