#!/usr/bin/env python3
"""Verifica, SÓLO desde los logs, que los bloques de la sección 1 se ejecutan.

Para qué: los bloques D4 (tráfico), D5 (`ped.ifp`), D6 (mira por arma) y D7
(iconos de tecla en los avisos) dejan trazas en `odtrace.log` mientras se juega.
Este verificador las lee y dice qué se ha visto y qué no, sin lanzar Chrome: se
juega una vez y luego se pasa esto sobre el log.

Uso:
    python tools/viceext-log-check.py                     # usa web/odtrace.log
    python tools/viceext-log-check.py <ruta.log> [--desde-marca]

Trazas que busca (las emite el motor):

    CARPED    n=.. rating=.. cargadas=x/y model=.. veh=../..   pedido de modelo
              del cargador de tráfico (D4). El ritmo entre líneas debe ser
              ~11,7 s (el temporizador pasó de fotogramas a tiempo real).
    CARPOOL   n=.. c0=.. c2=..                                  cuántos modelos
              hay cargados por clase (el fondo del que elige la calle).
    CARBLOCK  disable=.. cut=.. prio=.. area=.. play=.. nreq=..  por qué NO se
              llamó al cargador (diagnóstico; ver el plan de D4).
    CARSPAWN  model=<id> clase=<c>                              coche de calle que
              entra en el mundo: es la medida directa de qué circula.
    CARLOAD   model=<id> clase=<c> freq=<f> fondo=x/y            modelo de vehículo
              que termina de cargarse (entra en el fondo del que elige la calle).
              Emparejado con CARPED: pedido sin CARLOAD = su lectura falló.
    CARFAIL   ch=.. status=.. intentos=.. id=..                  lectura del
              cargador fallida (en web suele ser un aplazamiento de la descarga).
    CARZONE   umbrales=0,..,0                                    los 8 umbrales de
              clase de la zona (los pone el script): un umbral repetido = clase
              con probabilidad cero en esa zona.
    CARRATE   rating=<c> n=..                                    clase sorteada
              (lo que la zona puede sacar a la calle; 1=pobre, 5=grande).
    ODSHORT open-fail <ruta> fallo=<n> total=<n>                 fichero suelto del
              .img que la capa on-demand todavía no tenía (aplazado a propósito).
    ODSRECOVER <ruta> fallos=<n>                                 ese fichero SÍ se
              abrió después: el aplazamiento se recuperó.
    IFPFILE   <ruta> clips=<n> total=<n>                         qué .ifp se cargó y
              cuántos clips trae (D5: el del mod son 272, y 274 desde R27 con los dos
              clips del sa-crouch; el de serie tiene 234).
    SIGHTS    cargadas=<n>/7                                     las miras de D6.
    SIGHT     arma=<id> mira=<col27> textura=<0/1> cargadas=n/7   mira elegida al
              apuntar (D6).
    VICEEX sfx arma=<tipo> sample=<id>                           disparo de un arma
              nueva (D8). Emparejado con `ODSFXMISS sfx=<id>` / `CHINIT ... sfx=<id>`:
              disparo sin arranque = arma MUDA.
    KEYICONS  txd=pcbtns cargado                                  TXD de iconos de
              tecla de D7.
    HINTKEY   accion=<NOMBRE> vk=<código> icono=<0/1>             un aviso resolvió
              su tecla: `icono=1` = se pintó el icono; `0` = siguió el nombre en
              texto (rueda del ratón, mando, tecla sin icono).
    VICEEXT 1p key / 1p set puerta= flanco= tog=                       (R5) el
              conmutador de 1ª persona: `tog=1` en algún `set` = se encendió.
    AIMDIR    arma=<id> desv=<°> grupo=<id> peso=<0-1>        (R9) apuntado de las
              armas nuevas: desviación cámara-ped y peso de la pose; `peso=0` = el
              arma no tiene animación de apuntar, `desv` grande = apunta al costado.
    SVLIGHTS  model=<id> dummies=<n> on=<0/1> var=<n> extras=<a>,<b> radio=<m> (R11)
              luces de servicio: con `police` (=156) `dummies` debe ser >=1. `var`
              es la variante (nombre `servicelights_N`) y `extras` los extras
              elegidos (-1 = esa patrulla no lleva barra, es aleatorio).
    SIRENA    tipo= model= sample= freq= vol= dist2=              (SR) la sirena
              AUDIBLE: si no aparece ni una línea con la policía cerca, el motor
              nunca encoló la muestra.
    LBAR      w= h= borde=x,y                                     (LB) contorno de
              la barra de carga escalado por resolución (SilentPatch :723):
              `borde_y/alto` tiene que ser constante entre tamaños.
    HUDICON   arma=<id> modelo=<id> txd=<n> cargado=<0/1> textura=<0/1>  (L2) el
              icono del arma: si `textura=0` el HUD no tiene qué pintar.
    SCRTXT3   x= y= texto="..."                               (R3b) caza-todo de
              los textos dibujados en la mitad inferior de la pantalla.

Salida: un bloque por bloque con lo medido y un veredicto por bloque
(OK / PARCIAL / SIN DATOS; los bloques del plan de recoil y el R27 del agachado
calibrado usan PASS / FAIL / INCONCLUSIVE). Exit code 0 si los cuatro bloques
están OK.
"""
import argparse
import collections
import math
import os
import re
import statistics
import sys

DEFECTO = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "web", "odtrace.log")

# ids del default.ide servido para los vehículos del mod
VEH_MOD = {6500: "Streetfighter", 6501: "Perennial", 6502: "Trashmaster",
           6503: "Hellenbach", 6504: "Premier", 6505: "Manchez",
           6506: "Wintergreen"}
VEH_POLI = 6507
# clase de tráfico de cada uno (columna `vehclass` de default.ide, la que el
# motor guarda en `m_vehicleClass`): el id de clase de abajo. 6507 va `ignore`,
# por eso no tiene clase (no está en el sorteo del tráfico).
VEH_CLASE = {6500: 8, 6501: 1, 6502: 5, 6503: 0, 6504: 0, 6505: 8, 6506: 8}
CLASE_NOMBRE = {0: "normal", 1: "pobre", 2: "rico", 3: "ejecutivo",
                4: "obrero", 5: "grande", 6: "taxi", 7: "ciclomotor",
                8: "motocicleta"}
# armas del mod -> esperado (sólo informativo)
MIRAS = {1: "dot", 2: "pistol", 3: "SMG", 4: "shotgun", 5: "rifle",
         6: "heavy", 7: "rocket"}


def leer(ruta, desde_marca):
    with open(ruta, "r", encoding="utf-8", errors="replace") as f:
        texto = f.read()
    if desde_marca:
        i = texto.rfind("rotado: sesion nueva")
        if i >= 0:
            texto = texto[i:]
    return texto.split("\n")


def ts(linea):
    m = re.match(r"(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d+)?Z)", linea)
    return m.group(1) if m else None


def segundos(lineas):
    """Segundos entre líneas consecutivas (por marca de tiempo, si la traen)."""
    t = [x for x in (ts(l) for l in lineas) if x]
    if len(t) < 2:
        return []
    out = []
    for a, b in zip(t, t[1:]):
        try:
            import datetime
            fa = datetime.datetime.strptime(a, "%Y-%m-%dT%H:%M:%S.%fZ")
            fb = datetime.datetime.strptime(b, "%Y-%m-%dT%H:%M:%S.%fZ")
            out.append((fb - fa).total_seconds())
        except ValueError:
            pass
    return out


def bloque_d2(lineas):
    print("\n== D2 · nombres del HUD (claves de texto) ==")
    miss = []
    for l in lineas:
        m = re.search(r"TXTMISS key=(\S+)", l)
        if m:
            miss.append(m.group(1))
    print("   claves de texto que no resuelven (TXTMISS): %d" % len(miss))
    if miss:
        resumen = collections.Counter(x.replace(" missing", "") for x in miss)
        print("   " + ", ".join("%s×%d" % kv for kv in resumen.most_common(12)))
    eh = [x for x in miss if re.match(r"^(STRTFTR|HELLENB|PREMIER|MANCHEZ|WINTERG|VCPDWTR)$", x)]
    if eh:
        return "PARCIAL", "siguen faltando claves de los vehículos nuevos: %s" % ", ".join(eh)
    if miss:
        return "PARCIAL", "hay %d claves sin texto (mira la lista: el VC original ya traía huecos)" % len(miss)
    return "OK", "ninguna clave del HUD se quedó sin texto en esta sesión"


def bloque_d4(lineas):
    print("\n== D4 · tráfico de los vehículos del mod ==")
    carped = [l for l in lineas if "CARPED" in l]
    carpool = [l for l in lineas if "CARPOOL" in l]
    carblock = [l for l in lineas if "CARBLOCK" in l]
    cars = {}
    for l in lineas:
        m = re.search(r"CARSPAWN model=(\d+)", l)
        if m:
            cars[int(m.group(1))] = cars.get(int(m.group(1)), 0) + 1

    total = sum(cars.values())
    distintos = len(cars)
    print("   coches de calle vistos (CARSPAWN): %d en %d modelos distintos" % (total, distintos))
    if cars:
        top = sorted(cars.items(), key=lambda kv: -kv[1])[:10]
        print("   los que más salen: " + " ".join("%d=%d" % kv for kv in top))
    print("   ticks del cargador (CARPED): %d" % len(carped))
    d = segundos(carped)
    ritmo = "%d ticks" % len(carped)
    if len(d) >= 3:
        med = statistics.median(d)
        # La traza sella por LOTE: muchas líneas comparten marca y la mediana
        # sale 0 (no es un fallo del cargador, es de la medida).
        if med > 0.05:
            print("   ritmo entre ticks: mediana %.1f s (el original era 350 fotogramas"
                  " = 11,7 s a 30 FPS; el arreglo lo ancla a 11,7 s reales)" % med)
            ritmo = "%.1f s/tick" % med
        else:
            print("   ritmo entre ticks: no medible (timestamps en lote); el dato es el"
                  " número de ticks, que sin el arreglo era 0)")
    if carpool:
        print("   fondo cargado (CARPOOL): primer=%s último=%s"
              % (carpool[0].split("CARPOOL")[-1].strip()[:60],
                 carpool[-1].split("CARPOOL")[-1].strip()[:60]))
    if carblock:
        print("   veces que el cargador NO se llamó: %d líneas; última: %s"
              % (len(carblock), carblock[-1].split("CARBLOCK")[-1].strip()[:110]))
    else:
        print("   CARBLOCK: ninguna línea (la puerta no bloqueó al cargador)")

    delmod = {k: v for k, v in cars.items() if k in VEH_MOD}
    poli = cars.get(VEH_POLI, 0)
    print("   del mod en la calle: " + (", ".join("%d %s×%d" % (k, VEH_MOD[k], v)
          for k, v in sorted(delmod.items())) if delmod else "(ninguno)"))
    print("   moto policial (clase `ignore`, no debe salir): %d" % poli)

    # --- que un modelo del mod NO salga casi nunca tiene dos causas distintas:
    # (a) su modelo no llega a cargarse, (b) su CLASE no se sortea en la zona.
    # CARLOAD/CARFAIL y CARZONE/CARRATE separan las dos.
    carload = {}
    for l in lineas:
        m = re.search(r"CARLOAD model=(\d+)", l)
        if m:
            k = int(m.group(1))
            carload[k] = carload.get(k, 0) + 1
    carfail = [l for l in lineas if "CARFAIL" in l]
    osd = [l for l in lineas if "ODSHORT open-fail" in l]
    osr = [l for l in lineas if "ODSRECOVER" in l]
    carzone = [l for l in lineas if "CARZONE" in l]
    carrate = [l for l in lineas if "CARRATE" in l]
    cargados_mod = {k: v for k, v in carload.items() if k in VEH_MOD}
    pedidos_mod = set()
    for l in carped:
        m = re.search(r"model=(\d+)", l)
        if m and int(m.group(1)) in VEH_MOD:
            pedidos_mod.add(int(m.group(1)))
    if carload:
        print("   modelos cargados al fondo (CARLOAD): %d en total; del mod: %s"
              % (len(carload), ", ".join("%d %s" % (k, VEH_MOD[k])
                                          for k in sorted(cargados_mod)) or "(ninguno)"))
    if pedidos_mod:
        print("   del mod pedidos por el cargador: %s" % ", ".join(
            "%d %s" % (k, VEH_MOD[k]) for k in sorted(pedidos_mod)))
        if carload:
            sinCargar = sorted(pedidos_mod - set(cargados_mod))
            if sinCargar:
                print("   pedidos SIN llegar a cargar: %s  <- leer CARFAIL/ODSHORT" % ", ".join(
                    "%d %s" % (k, VEH_MOD[k]) for k in sinCargar))
        else:
            print("   (el log no trae CARLOAD: build anterior a `ve11`; no se puede"
                  " saber si un pedido llegó a cargar)")
    if carfail:
        print("   lecturas fallidas del cargador (CARFAIL): %d; última: %s"
              % (len(carfail), carfail[-1].split("CARFAIL")[-1].strip()[:90]))
    if osd or osr:
        print("   ficheros sueltos del .img que no estaban aún: %d fallos, %d recuperados"
              % (len(osd), len(osr)))
        if osr:
            print("      recuperado: %s" % osr[-1].split("ODSRECOVER")[-1].strip()[:90])
        elif not any("total=" in l for l in osd):
            print("      (el log no trae el conteo `total=`: build anterior a `ve11`)")
        else:
            print("      OJO: ningún ODSRECOVER — puede haber modelo perdido de verdad")
    clases = set()
    if carzone or carrate:
        umbral = carzone[-1].split("CARZONE")[-1].strip() if carzone else ""
        clases = {int(m.group(1)) for m in
                  (re.search(r"rating=(\d+)", l) for l in carrate) if m}
        print("   umbrales de la zona (CARZONE): %s" % (umbral or "(sin datos)"))
        print("   clases de coche sorteadas (CARRATE): %s%s"
              % (", ".join(str(c) for c in clases) or "(sin datos)",
                 "   <- 1=pobre 5=grande: una clase que no aparece aquí no puede"
                 " salir en ESTA zona" if clases else ""))
        clases_mod = {VEH_CLASE.get(k) for k in VEH_MOD}
        faltan = sorted(c for c in clases_mod - clases if c is not None)
        if clases and faltan:
            print("   clases del mod que esta zona NO sortea: %s"
                  % ", ".join("%d=%s" % (c, CLASE_NOMBRE.get(c, "?")) for c in faltan))

    if not carped and not cars:
        return "SIN DATOS", "no hay trazas de tráfico en el log"
    if not carped:
        return "PARCIAL", "la calle se movió (%d coches) pero el cargador no rotó modelos" % total
    if poli > 0:
        return "PARCIAL", "la moto policial (clase `ignore`) salió en tráfico %d veces" % poli
    cola = ""
    if clases:
        # Cobertura real: cuántos de los 7 puede sacar la zona jugada, cuántos se
        # cargaron y cuántos se vieron. Distingue "su modelo no carga" de "su
        # clase no se sortea en esta zona".
        alcanzables = {k for k, c in VEH_CLASE.items() if c in clases}
        cola = " (en esta zona eran alcanzables %d de los 7: %d cargados, %d vistos)" % (
            len(alcanzables), len(cargados_mod), len(delmod))
    if len(delmod) >= 1:
        return "OK", ("cargador rotando (%s) y %d modelo(s) del mod en la calle%s,"
                      " sin la moto policial" % (ritmo, len(delmod), cola))
    if cargados_mod:
        return "PARCIAL", ("%d modelo(s) del mod entraron en el fondo (%s) pero no se"
                            " vieron coches suyos en la calle%s"
                            % (len(cargados_mod),
                               ", ".join(VEH_MOD[k] for k in sorted(cargados_mod)), cola))
    return "PARCIAL", "el cargador rotó pero no se cargó ningún modelo del mod"


def bloque_d5(lineas):
    print("\n== D5 · `ped.ifp` del mod ==")
    ifps = [l for l in lineas if "IFPFILE" in l]
    if not ifps:
        print("   no hay líneas IFPFILE (¿build anterior a la traza?)")
        return "SIN DATOS", "sin trazas IFPFILE"
    for l in ifps[:8]:
        print("   " + l.split("IFPFILE", 1)[-1].strip())
    # el ped.ifp es el primero en cargarse
    ped = [l for l in ifps if "PED.IFP" in l.upper()]
    if not ped:
        return "PARCIAL", "se cargaron IFP pero ninguno era PED.IFP"
    m = re.search(r"clips=(\d+)", ped[0])
    clips = int(m.group(1)) if m else -1
    if clips == 274:
        return "OK", "ped.ifp con 274 clips (el del mod + los 2 del sa-crouch de R27)"
    if clips == 272:
        return "OK", "ped.ifp con 272 clips (el del mod sin los 2 del sa-crouch de R27: dato viejo)"
    if clips > 0:
        return "PARCIAL", "ped.ifp con %d clips (el del mod son 272, y 274 con R27)" % clips
    return "PARCIAL", "no se pudo leer el número de clips"


def bloque_d6(lineas):
    print("\n== D6 · mira por arma (`weapon.dat` columna 27) ==")
    carg = [l for l in lineas if "SIGHTS " in l]
    vistos = collections.OrderedDict()
    for l in lineas:
        m = re.search(r"SIGHT arma=(\d+) mira=(\d+) textura=(\d+)", l)
        if m:
            vistos[(int(m.group(1)), int(m.group(2)))] = int(m.group(3))
    if carg:
        print("   " + carg[0].split("SIGHTS", 1)[-1].strip())
    if not carg and not vistos:
        return "SIN DATOS", "no se apuntó con ninguna arma en esta sesión"
    for (arma, mira), tex in vistos.items():
        print("   arma=%d -> columna27=%d (%s) textura=%s"
              % (arma, mira, MIRAS.get(mira, "?"), "cargada" if tex else "NO cargada"))
    conmira = [k for k, t in vistos.items() if k[1] >= 1 and t == 1]
    if conmira:
        return "OK", ("%d arma(s) con mira propia y textura cargada: %s"
                      % (len(conmira), ", ".join("arma %d (mira %d)" % k for k in conmira)))
    if vistos:
        return "PARCIAL", "se apuntó pero ninguna arma mostró mira propia con textura"
    return "PARCIAL", "las miras se cargaron pero no se llegó a apuntar"


def bloque_d7(lineas):
    print("\n== D7 · iconos de tecla en los avisos (v3.0) ==")
    ico = [l for l in lineas if "KEYICONS" in l]
    hint = [l for l in lineas if "HINTKEY" in l]
    resueltos = {}
    for l in hint:
        m = re.search(r"HINTKEY accion=(\S+) vk=(-?\d+) icono=(\d)", l)
        if m:
            resueltos[m.group(1)] = (int(m.group(2)), int(m.group(3)))
    if ico:
        print("   " + ico[0].split("KEYICONS", 1)[-1].strip())
    print("   avisos con tecla resuelta: %d distintas" % len(resueltos))
    for acc, (vk, hay) in list(resueltos.items())[:15]:
        print("      %-34s vk=%-4d %s" % (acc, vk, "icono" if hay else "texto"))
    conicono = [a for a, (vk, h) in resueltos.items() if h == 1]
    if conicono:
        return "OK", ("%d aviso(s) con icono de tecla (p. ej. %s)"
                      % (len(conicono), ", ".join(conicono[:3])))
    if resueltos:
        return "PARCIAL", "se resolvieron teclas pero ninguna tenía icono en el TXD"
    if ico:
        return "PARCIAL", "el TXD de iconos se cargó pero no se mostró ningún aviso con tecla"
    return "SIN DATOS", "ni TXD de iconos ni avisos con tecla en esta sesión"


def bloque_j1(lineas):
    print("\n== J1 · motivo del cuelgue y LOD pegado ==")
    err = [l for l in lineas if re.search(r"\b(JSERR|JSERR_REJ|ENGERR)\b", l)]
    if err:
        print("   fallo duro registrado: %d línea(s)" % len(err))
        for l in err[:4]:
            print("      " + l.split("Z ", 1)[-1].strip()[:160])
    else:
        print("   sin JSERR/ENGERR (o el build es anterior a la captura de motivo)")
    lod = []
    for l in lineas:
        m = re.search(r"LODLEFT model=(\d+) dist=([0-9.]+) relacion=(\d) rwobj=(\d) alpha=(-?\d+)", l)
        if m:
            lod.append((int(m.group(1)), float(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))))
    if not lod:
        print("   LODLEFT: ninguna línea (o el build es anterior a D10)")
        if err:
            return "FALLO", "el motor dejó un motivo de cuelgue en la traza"
        return "SIN DATOS", "ni cuelgue registrado ni LOD pegado observado"
    print("   LODs dibujados de cerca (<=60 m), uno por modelo: %d" % len(lod))
    sinrel = [x for x in lod if x[2] == 0]
    sinobj = [x for x in lod if x[2] == 1 and x[3] == 0]
    alfa = [x for x in lod if x[2] == 1 and x[3] == 1 and x[4] != 255]
    print("      sin modelo real relacionado: %d" % len(sinrel))
    print("      real relacionado pero SIN cargar (rwobj=0): %d" % len(sinobj))
    print("      real cargado pero alfa != 255 (fundido sin terminar): %d" % len(alfa))
    for x in lod[:8]:
        print("      model=%d dist=%.1f rwobj=%d alpha=%d" % (x[0], x[1], x[3], x[4]))
    if err:
        return "FALLO", "el motor dejó un motivo de cuelgue en la traza (y además hay LOD pegado)"
    return "AVISO", ("hay LOD pegado: %d modelos (mirar el desglose de arriba)" % len(lod))


def bloque_r2(lineas):
    """R2 (plan 06): emisora / congelado. Espera por .adf y copias grandes a MEMFS.

    La huella del bloqueo de hilo está en FPHASE: `wait` es el tiempo que el frame
    pasó esperando fuera de la lógica (en la 5ª partida: gap=1256.8, wait=1256.5,
    con la lógica a 0,3 ms).
    """
    print("\n== R2 · emisora y congelados de la capa web ==")
    stalls = []
    for l in lineas:
        m = re.search(r"FPHASE .*gap=([0-9.]+).*?wait=([0-9.]+)", l)
        if m and float(m.group(2)) > 100:
            stalls.append((float(m.group(1)), float(m.group(2))))
    if stalls:
        print("   tirones de frame con espera > 100 ms: %d" % len(stalls))
        for g, w in sorted(stalls, key=lambda x: -x[1])[:5]:
            print("      gap=%.0f ms  wait=%.0f ms" % (g, w))
    else:
        print("   sin tirones de frame (espera > 100 ms)")

    odsta = [l for l in lineas if "ODSTA" in l]
    preload = [l for l in lineas if "STREAM preload" in l]
    writes = [l for l in lineas if "ODWRITE" in l]
    print("   peticiones de emisora (ODSTA): %d" % len(odsta))
    for l in odsta[:6]:
        print("      " + l.split("Z ", 1)[-1].strip()[:150])
    print("   aperturas de stream (STREAM preload): %d" % len(preload))
    for l in preload[:4]:
        print("      " + l.split("Z ", 1)[-1].strip()[:150])
    peor_copia, peor_ms = None, 0.0
    for l in writes:
        m = re.search(r"ms=([0-9.]+)", l)
        if m and float(m.group(1)) > peor_ms:
            peor_ms, peor_copia = float(m.group(1)), l
    print("   escrituras grandes a MEMFS (ODWRITE): %d%s" % (
        len(writes), ("  (peor: %.0f ms)" % peor_ms) if writes else ""))
    if peor_copia:
        print("      " + peor_copia.split("Z ", 1)[-1].strip()[:150])

    if not odsta:
        return "SIN DATOS", ("el build no lleva ODSTA; los tirones (%d) hay que "
                             "explicarlos con la próxima partida" % len(stalls))
    esperas = [l for l in odsta if "espera=1" in l]
    bloques = [l for l in odsta if re.search(r"block=[1-9]", l)]
    if esperas:
        return "FALLO", ("%d petición(es) de emisora ESPERARON (espera=1): es el "
                         "camino que quita el freeze" % len(esperas))
    if stalls and peor_ms > 200:
        return "AVISO", ("hay %d tirón(es) y una copia a MEMFS de %.0f ms: la copia "
                         "es candidata (el motor lee por fopen, no se puede evitar)"
                         % (len(stalls), peor_ms))
    if stalls:
        return "AVISO", ("%d tirón(es) sin emisora de por medio: mirar si el "
                          "STREAM preload coincide en el tiempo" % len(stalls))
    if bloques:
        return "AVISO", ("%d petición(es) llegaron con carga bloqueante del motor "
                         "(block>0) y NO se esperó: correcto" % len(bloques))
    return "OK", ("emisora sin esperas y sin tirones (%d peticiones, %d preloads)"
                  % (len(odsta), len(preload)))


def bloque_r3(lineas):
    """R3 (plan 06): textos que salen en pantalla. Con SCRTXT se sabe QUIÉN los pide."""
    print("\n== R3 · textos en pantalla (SCRTXT) ==")
    txt = [l for l in lineas if "SCRTXT" in l]
    if not txt:
        print("   sin líneas SCRTXT (o el build es anterior a R3)")
        return "SIN DATOS", "no se encoló ningún texto en esta sesión"
    fuera = [l for l in txt if re.search(r"mis=0", l)]
    # R19e: `clave=""` en el cajón de ayuda NO es un texto sin contenido: son
    # las llamadas de SERIE `CHud::SetHelpMessage(nil, ...)` (Script.cpp:409,
    # Script4.cpp:2198, Pickups.cpp:575) que VACÍAN el cajón. Contarlas como
    # fallo hacía que el bloque cantara en una partida normal (visto en la 15ª).
    vacias = [l for l in txt if 'clave=""' in l and 'canal=help' not in l]
    vaciados = [l for l in txt if 'clave=""' in l and 'canal=help' in l]
    canales = {}
    for l in txt:
        m = re.search(r"canal=([a-zA-Z+]+)", l)
        if m:
            canales[m.group(1)] = canales.get(m.group(1), 0) + 1
    print("   textos encolados: %d  (%s)" % (
        len(txt), " ".join("%s=%d" % kv for kv in sorted(canales.items()))))
    print("   fuera de misión (mis=0): %d   con clave vacía: %d   (cajón de ayuda vaciado a propósito: %d)"
          % (len(fuera), len(vacias), len(vaciados)))
    for l in fuera[:8]:
        print("      " + l.split("Z ", 1)[-1].strip()[:160])
    if vacias:
        return "FALLO", ("%d texto(s) con la clave VACÍA: eso es un texto que sale "
                         "sin contenido (revisar el .gxt y el que lo pide)" % len(vacias))
    if fuera:
        return "AVISO", ("%d texto(s) FUERA de misión: es lo que vio el jugador; "
                          "mira las claves de arriba para saber qué los pide" % len(fuera))
    return "OK", ("%d textos, todos dentro de misión y con clave" % len(txt))


def bloque_d8(lineas):
    """D8 (sección 1): sonidos de las 8 armas nuevas del mod.

    El motor anota cada disparo (`VICEEX sfx arma=<tipo> sample=<id>`) y la capa
    de audio anota cada muestra que ARRANCA (`ODSFXMISS sfx=<id>` al decodificar
    el mp3 y `CHINIT ch=.. sfx=<id>` al ocupar un canal). Un disparo sin ninguna
    de las dos = arma MUDA (dato de la partida del 21/09: 40 disparos de
    `sample=9943` y cero arranques: el motor descartaba la muestra por el límite
    `SAMPLEBANK_MAX`, arreglado en D8b).
    """
    print("\n== D8 · sonidos de las armas nuevas del mod (VICEEX sfx / arranque) ==")
    disparos = collections.defaultdict(set)
    for l in lineas:
        m = re.search(r"VICEEX sfx arma=(\d+) sample=(\d+)", l)
        if m:
            disparos[int(m.group(2))].add(int(m.group(1)))
    if not disparos:
        print("   sin líneas VICEEX sfx (no se disparó ninguna arma del mod)")
        return "SIN DATOS", "no se disparó ninguna arma nueva en esta sesión"

    arrancadas = set()
    for l in lineas:
        m = re.search(r"(?:ODSFXMISS|CHINIT \w+=\d+) sfx=(\d+)", l)
        if m:
            arrancadas.add(int(m.group(1)))

    mudas = sorted(s for s in disparos if s not in arrancadas)
    print("   disparos del mod: %d muestras distintas (%s)" % (
        len(disparos), " ".join("%d=%s" % (s, ",".join(map(str, sorted(t))))
                                for s, t in sorted(disparos.items()))))
    print("   muestras que ARRANCARON (decodificadas/ocuparon canal): %d" % len(arrancadas))
    if mudas:
        print("   MUDAS (dispararon y nunca arrancaron): %s" % ", ".join(map(str, mudas)))
        return "FALLO", ("%d muestra(s) del mod dispararon sin sonar: el motor las "
                         "descarta (D8b / rango de muestras)" % len(mudas))
    print("   todas las muestras disparadas arrancaron")
    return "OK", "%d muestras del mod disparadas y todas suenan" % len(disparos)


# armas nuevas del mod (WeaponType.h: 48-55), para nombrarlas en los informes
ARMAS_MOD = {48: "Beretta", 49: "DesertEagle", 50: "Shotgun2", 51: "Uziold",
             52: "AK47", 53: "M16", 54: "Steyr/AUG", 55: "Gr.launcher"}
PB_POLICE = 156   # default.ide: el coche `police`


def bloque_r5(lineas):
    """1ª persona (R5/R5b): ¿llegó la tecla Y se aplicó el conmutador?"""
    print("\n== R5 · primera persona (conmutador) ==")
    teclas = [l for l in lineas if "VICEEXT 1p key" in l]
    sets = [l for l in lineas if "VICEEXT 1p set" in l]
    encendidos = [l for l in sets if re.search(r"tog=1", l)]
    print("   pulsaciones=%d   resultados=%d   encendido=%d"
          % (len(teclas), len(sets), len(encendidos)))
    for l in sets[:6]:
        m = re.search(r"puerta=(\d) flanco=(\d) tog=(\d).*incontrol=(\d) target=(\d) replay=(\d) free=(-?\d)", l)
        if m:
            print("      puerta=%s flanco=%s tog=%s incontrol=%s target=%s replay=%s free=%s"
                  % m.groups())
    if not teclas and not sets:
        return "SIN DATOS", "no se pulsó la tecla V en esta partida"
    if not sets:
        return "FALLO", ("hubo %d pulsaciones y NINGUNA línea de resultado: el arreglo "
                         "del conmutador no está en el motor servido" % len(teclas))
    if encendidos:
        return "OK", "el conmutador se encendió %d vez/veces" % len(encendidos)
    return "FALLO", "la tecla llega pero la puerta no deja pasar (ver puerta/incontrol/target)"


def bloque_r9(lineas):
    """Apuntado de las armas nuevas: desviación cámara-ped y peso de la pose."""
    print("\n== R9 · apuntado de las armas del mod (AIMDIR) ==")
    datos = collections.defaultdict(list)
    for l in lineas:
        m = re.search(r"AIMDIR arma=(\d+) desv=([\d.]+) grupo=(\d+) clip=(\d+) peso=([\d.]+)", l)
        if m:
            datos[int(m.group(1))].append((float(m.group(2)), float(m.group(5))))
    if not datos:
        return "SIN DATOS", "no se apuntó con ninguna arma (falta `AIMDIR`): ¿build viejo?"
    malas, sinpose = [], []
    for arma in sorted(datos):
        desv = [x[0] for x in datos[arma]]
        peso = max(x[1] for x in datos[arma])
        med = statistics.median(desv)
        nombre = ARMAS_MOD.get(arma, "(de serie)")
        estado = "OK"
        if peso <= 0.0:
            estado = "SIN POSE"
            sinpose.append(arma)
        elif med > 5.0:
            estado = "DESVIADA"
            malas.append(arma)
        print("   arma=%-3d %-13s n=%-3d desv med=%5.1f max=%5.1f  peso max=%.2f  %s"
              % (arma, nombre, len(desv), med, max(desv), peso, estado))
    if sinpose:
        return "FALLO", "armas sin animación de apuntar (peso=0): %s" % \
            ", ".join(str(a) for a in sinpose)
    if malas:
        return "FALLO", "armas que apuntan al costado (desv media >5°): %s" % \
            ", ".join(str(a) for a in malas)
    return "OK", "todas las armas apuntadas alinean y tienen pose"


def bloque_r11(lineas):
    """Luces de servicio por dummies (sirena del coche de policía).

    10ª partida: la traza trae además la **variante** (`var`, el nombre
    `servicelights_N` que apareció), los **extras elegidos** (`extras=`,
    `m_aExtras`; `-1` = esa patrulla no lleva barra, que es aleatorio por datos:
    `comprules=0` en `default.ide`) y el **radio** con el que se separan las
    coronas (esfera envolvente de la malla de las luces).
    """
    print("\n== R11 · luces de servicio (SVLIGHTS) ==")
    sv = [l for l in lineas if "SVLIGHTS" in l]
    if not sv:
        return "SIN DATOS", "no se pidió ninguna luz de servicio (¿no hubo policía?)"
    pol = []
    for l in sv:
        m = re.search(r"SVLIGHTS model=(\d+) dummies=(\d+) on=(\d)"
                      r"(?: var=(-?\d+) extras=(-?\d+),(-?\d+) radio=([\d.]+))?", l)
        if not m:
            continue
        var, e0, e1, radio = m.group(4), m.group(5), m.group(6), m.group(7)
        extra = ""
        if var is not None:
            extra = " var=%s extras=%s,%s radio=%s" % (var, e0, e1, radio)
        mp = re.search(r" pos=([-\d.]+),([-\d.]+),([-\d.]+)", l)
        pos = (float(mp.group(1)), float(mp.group(2)), float(mp.group(3))) if mp else None
        print("   model=%s dummies=%s on=%s%s%s" % (m.group(1), m.group(2), m.group(3), extra,
                                                     "   (police)" if int(m.group(1)) == PB_POLICE else ""))
        if int(m.group(1)) == PB_POLICE:
            pol.append((int(m.group(2)), var, e0, pos))
    if not pol:
        return "SIN DATOS", "no apareció el coche de policía en la traza"
    maxd = max(p[0] for p in pol)
    if maxd > 0:
        var = next(p[1] for p in pol if p[0] > 0)
        sanas = [p for p in pol if p[0] > 0 and p[3] is not None
                 and all(abs(v) <= 20000.0 for v in p[3])]
        if not sanas:
            return "PARCIAL", ("el coche de policía resolvió %d dummy(s) (variante %s) pero "
                               "ninguna muestra trae una posición observada y sana: el 'on=1' "
                               "no basta para dictaminar que las coronas vayan ancladas"
                               % (maxd, var))
        return "OK", ("el coche de policía resolvió %d dummy(s) de luces (variante %s: la barra "
                       "va con sus lentes y las coronas van ancladas a la malla; %d muestra(s) "
                       "con posición observada)" % (maxd, var, len(sanas)))
    if all(p[2] is not None and int(p[2]) < 0 for p in pol):
        return "PARCIAL", ("dummies=0 en todas las muestras y `extras=-1`: esas patrullas no "
                            "llevan ninguna de las tres barras (lo elige el juego al azar; "
                            "`comprules=0` en default.ide)")
    return "FALLO", "el coche de policía sigue con dummies=0 (la barra no se puede pintar)"


def bloque_lbar(lineas):
    """Contorno de la barra de carga escalado por resolución (SilentPatch :723).

    La traza es `LBAR w= h= borde=x,y` (una línea cada vez que cambia el tamaño
    de la ventana, dibujando la pantalla de carga). En nuestro motor el margen
    del contorno pasa por la MISMA escala que la barra:
    `borde_y = alto/448` (constante con el alto) y `borde_x = ancho/640 *
    corrección de aspecto`. PASS: `borde_y/alto` constante entre tamaños y, en el
    mayor, `borde_y > 1.5` (si no, seguiría clavado en 1 px, que era el fallo).
    """
    print("\n== LB · contorno de la barra de carga (LBAR) ==")
    rows = []
    for l in lineas:
        m = re.search(r"LBAR w=(\d+) h=(\d+) borde=([\d.]+),([\d.]+)", l)
        if m:
            w, h, bx, by = int(m.group(1)), int(m.group(2)), float(m.group(3)), float(m.group(4))
            rows.append((w, h, bx, by))
            print("   %dx%d  borde=%.2f,%.2f  (borde_y/alto=%.5f)" % (w, h, bx, by, by / h))
    if not rows:
        return "SIN DATOS", "no se dibujó la pantalla de carga en esta sesión"
    ratios = [by / h for _, h, _, by in rows]
    if len(rows) == 1:
        w, h, bx, by = rows[0]
        return "PARCIAL", ("un solo tamaño en el log (%dx%d, borde=%.2f,%.2f): para probar que "
                            "escala hace falta otro tamaño (redimensionar o jugar en otra "
                            "resolución)" % (w, h, bx, by))
    if max(ratios) - min(ratios) > 0.02 * max(ratios):
        return "FALLO", ("el contorno NO escala con el alto: borde_y/alto va de %.5f a %.5f "
                         "(debería ser constante)" % (min(ratios), max(ratios)))
    if max(by for _, _, _, by in rows) <= 1.5:
        return "FALLO", "borde_y sigue siendo ~1 px en pantallas grandes (no escaló)"
    return "OK", ("contorno escalado: borde_y/alto constante (%.5f) en %d tamaño(s) y "
                  "hasta %.2f px de margen" % (ratios[0], len(rows), max(by for _, _, _, by in rows)))


def bloque_sirena(lineas):
    """Sirena AUDIBLE de las patrullas (traza SIRENA de AudioLogic).

    Si el jugador dice que las patrullas no suenan, esto distingue las dos
    causas: sin ningún `SIRENA` el motor nunca encoló la muestra (el fallo está
    antes, en `m_bSirenOrAlarm`/`UsesSiren`); con muestras, el sonido sale del
    motor y el problema sería de mezcla/muestras.
    """
    print("\n== SR · sirena audible de las patrullas (SIRENA) ==")
    sr = [l for l in lineas if "SIRENA " in l]
    if not sr:
        return "SIN DATOS", ("el motor no encoló ninguna muestra de sirena (si hubo policía con "
                             "la sirena puesta, el fallo está antes: `m_bSirenOrAlarm`)")
    parejas = collections.Counter()
    for l in sr:
        m = re.search(r"SIRENA tipo=(\d+) model=(\d+) sample=(\d+) freq=(\d+) vol=(\d+) dist2=([\d.]+)", l)
        if m:
            print("   tipo=%s model=%s sample=%s freq=%s vol=%s dist2=%s" % m.groups())
            parejas[(m.group(2), m.group(3))] += 1
    if not parejas:
        return "SIN DATOS", "la traza SIRENA no trae el formato esperado"
    return "OK", "%d encolados de sirena, %d combinaciones (model, sample)" % (len(sr), len(parejas))


def bloque_icon(lineas):
    """Icono de arma del HUD (L2): ¿encontró textura para cada arma nueva?"""
    print("\n== L2 · icono de arma en el HUD (HUDICON) ==")
    iconos = [l for l in lineas if "HUDICON" in l]
    if not iconos:
        return "SIN DATOS", "el motor servido no trae la traza HUDICON (build viejo)"
    malos = []
    for l in iconos:
        m = re.search(r"HUDICON arma=(\d+) modelo=(-?\d+) txd=(-?\d+) cargado=(\d) textura=(\d) nombre=(\S*)", l)
        if not m:
            continue
        arma = int(m.group(1))
        print("   arma=%-3d %-12s modelo=%-5s txd=%-5s cargado=%s textura=%s nombre=%s"
              % (arma, ARMAS_MOD.get(arma, "(de serie)"), m.group(2), m.group(3),
                 m.group(4), m.group(5), m.group(6)))
        if arma in ARMAS_MOD and m.group(5) == "0":
            malos.append(arma)
    if malos:
        return "FALLO", "armas nuevas sin textura de icono: %s" % \
            ", ".join(str(a) for a in malos)
    return "OK", "todas las armas vistas resolvieron su icono"


def bloque_r3b(lineas):
    """Textos dibujados abajo (R3b): inventario, para cazar los que salen solos."""
    print("\n== R3b · textos en la mitad inferior de la pantalla (SCRTXT3) ==")
    vistos = collections.Counter()
    pos = {}
    for l in lineas:
        m = re.search(r"SCRTXT3 x=([\d.]+) y=([\d.]+) texto=\"(.*)\"", l)
        if m:
            vistos[m.group(3)] += 1
            pos.setdefault(m.group(3), (m.group(1), m.group(2)))
    if not vistos:
        return "SIN DATOS", ("sin líneas SCRTXT3: o no se pintó texto abajo, o el motor "
                             "servido no trae la traza")
    for texto, n in vistos.most_common(20):
        print("   n=%-4d y=%-4s texto=\"%s\"" % (n, pos[texto][1], texto))
    return "OK", "%d textos distintos dibujados abajo (inventario arriba)" % len(vistos)


def _mediana(vals):
    if not vals:
        return None
    v = sorted(vals)
    return v[len(v) // 2]


def bloque_h(lineas):
    """H1/H2 · agachado y nado medidos, no mirados.

    H1: la camara tiene que BAJAR con el ped agachado. Se mide como CAMARA SOBRE
    EL PED (`camz` − `z`), que no depende de DONDE este el jugador: antes se
    comparaba la `camz` absoluta de agachado con la de pie de toda la sesion (dos
    sitios distintos) y en la 15a partida canto un falso fallo (11,18 vs 11,33)
    con la camara funcionando. De pie: `PEDAT camz=` (fuera de los tramos de
    agachado); agachado: `CROUCH2 camz=` con `peso>0,7`.
    H2: el nado se juzga por metros REALES (`SWIM2 avance=`) y por que ya no
    entre y salga del agua a cada ola (`exit motivo=poco-hondo`).
    """
    print("\n== H1/H2 · agachado (camara) y nado (avance real) ==")
    import datetime as _dt
    def _seg(l):
        m = re.match(r"(\S+)", l)
        if not m:
            return None
        try:
            return _dt.datetime.fromisoformat(m.group(1).replace("Z", "+00:00"))
        except ValueError:
            return None
    crouch, stand, avance = [], [], []
    crouch_seg, crouch_alt = [], []     # segundos con agachado y altura de camara sobre el ped
    exits = collections.Counter()
    swim_velo = collections.defaultdict(list)  # serega -> [velo]: bandas de la curva
    swim_sin_curva = 0
    for l in lineas:
        m = re.search(r"CROUCH2 h=([-\d.]+) .*peso=([\d.]+) camz=([-\d.]+)", l)
        if m:
            t = _seg(l)
            if float(m.group(2)) > 0.7:
                crouch.append(float(m.group(3)))
                if t:
                    crouch_seg.append(t)
                    crouch_alt.append(float(m.group(3)) - float(m.group(1)))
        m = re.search(r"PEDAT .* z=([-\d.]+) .* camz=([-\d.]+)", l)
        if m:
            t = _seg(l)
            if t:
                stand.append(float(m.group(2)))
        m = re.search(r"SWIM2 exit motivo=(\S+)", l)
        if m:
            exits[m.group(1)] += 1
        m = re.search(r"SWIM2 move .*avance=([\d.]+)", l)
        if m:
            avance.append(float(m.group(1)))
            mv = re.search(r"SWIM2 move .*velo=([\d.]+).*serega=([\d.]+)", l)
            if mv:
                swim_velo[mv.group(2)].append(float(mv.group(1)))
            else:
                swim_sin_curva += 1
    if not crouch and not stand and not avance and not exits:
        return "SIN DATOS", "ni agachado ni nado en esta sesion"

    # Altura de la camara SOBRE EL PED de pie: `PEDAT` fuera de los tramos de
    # agachado (3 s a cada lado). Sin `camz` en PEDAT (build viejo) queda vacio.
    alturas_pie = []
    for l in lineas:
        m = re.search(r"PEDAT .* z=([-\d.]+) .* camz=([-\d.]+)", l)
        if not m:
            continue
        t = _seg(l)
        if not t:
            continue
        if any(abs((t - c).total_seconds()) < 3.0 for c in crouch_seg):
            continue
        alturas_pie.append(float(m.group(2)) - float(m.group(1)))

    fallos = []
    avisos = []
    alta_agachado = _mediana(crouch_alt)
    alta_pie = _mediana(alturas_pie)
    if alta_agachado is not None and alta_pie is not None:
        print("   camara sobre el ped: agachado=%.2f m (n=%d)  de pie=%.2f m (n=%d)"
              % (alta_agachado, len(crouch_alt), alta_pie, len(alturas_pie)))
        if alta_agachado - alta_pie > -0.35:
            fallos.append("la camara no baja agachado (sobre el ped: %.2f vs %.2f m)"
                          % (alta_agachado, alta_pie))
    elif alta_agachado is not None:
        print("   camara agachado: %.2f m sobre el ped (n=%d) — sin `PEDAT camz=` de pie "
              "para comparar (build sin R19e)" % (alta_agachado, len(crouch_alt)))
        avisos.append("no se puede comparar la camara de pie vs agachado en este log")
    poco = exits.get("poco-hondo", 0)
    if exits:
        print("   salidas del agua: " + ", ".join("%s=%d" % kv for kv in exits.most_common()))
    if poco > 2:
        fallos.append("entra y sale del agua %d veces (poco-hondo): el estado no engancha" % poco)
    idle = sum(1 for l in lineas if "SWIMIDLE " in l)
    if idle:
        print("   contacto sin nado: %d SWIMIDLE" % idle)
    if idle > 5 and not avance:
        fallos.append("contacto con agua sin estado de nado (%d SWIMIDLE, 0 avance): la entrada no ve el agua" % idle)
    if avance:
        print("   nado: avance/s mediana=%.2f max=%.2f (n=%d)"
              % (_mediana(avance), max(avance), len(avance)))
        if max(avance) > 8.0:
            fallos.append("avance=%.1f m/s: el ped sale disparado (unidades de m_vecMoveSpeed)"
                          % max(avance))
        elif _mediana(avance) < 0.2 and len(avance) > 3:
            fallos.append("avance mediana=%.2f: nada pero no avanza" % _mediana(avance))
    # Curva Serega (plan nado-curva-serega): bandas deriva ~1.1/1.25, crucero
    # ~2.5, sprint ~6.0 m/s. Sin serega= es build vieja (no se juzga la curva).
    if avance and swim_sin_curva and not swim_velo:
        avisos.append("SWIM2 sin serega= (build vieja): la curva no se puede juzgar")
    # 1.9 - `serega=0.022` (entrada) y `0.025` (deriva) son las bandas de PARADO
    # en este puerto: cuando no se pulsa delante, `PlayerPed.cpp:2763-2764` pone la
    # velocidad horizontal a 0 y el nivel solo afecta a la profundidad y a la
    # velocidad que se declara. Julgarlas por velocidad daba un falso fallo con
    # velo=0.00, que es lo correcto en esas bandas. Las que se juzgan son 0.05
    # (2,5 m/s) y 0.12 (6,0 m/s), que son las que mueven al ped.
    for ser, (lo, hi) in (("0.022", (0.5, 2.0)), ("0.025", (0.5, 2.0)),
                           ("0.050", (1.5, 3.5)), ("0.120", (4.0, 8.0))):
        v = swim_velo.get(ser, [])
        if len(v) < 3:
            continue
        med = _mediana(v)
        if ser in ("0.022", "0.025"):
            print("   serega=%s: velo mediana=%.2f (n=%d, banda de parado: no se juzga)"
                  % (ser, med, len(v)))
            continue
        print("   serega=%s: velo mediana=%.2f (n=%d, banda %.1f-%.1f)"
              % (ser, med, len(v), lo, hi))
        if med < lo or med > hi:
            fallos.append("curva serega=%s: velo mediana=%.2f fuera de banda" % (ser, med))
    if fallos:
        return "FALLO", "; ".join(fallos)
    if avisos:
        return "PARCIAL", "; ".join(avisos)
    return "OK", "agachado baja la camara y el nado avanza a velocidad de persona"


def bloque_r13(lineas):
    """R13 · la ficha de cada arma del mod, leída del motor.

    `WINFO` imprime, una vez por arma que el jugador saca: los flags reales de
    `weapon.dat`, las dos puertas del apuntado (`canaim`, `witharm`), si el arma
    trae flag de recarga y los NOMBRES de clip que el grupo resuelve para
    disparo/agachado/recarga. Con esto, "no tiene animación de apuntado/recarga"
    se dictamina desde el log: o es un flag, o es un clip que no está.

    Además recoge `VICEEXT 1p bind tecla=` (la tecla que el navegador tiene
    GUARDADA para la 1ª persona): si no es 86 ('V') ni 1056 (`rsNULL`), pulsar V
    no podía hacer nada — el motivo se ve aquí, sin adivinar.
    """
    print("\n== R13 · ficha por arma (WINFO) y tecla de 1ª persona guardada ==")
    binds = []
    armas = []
    sight_por = collections.Counter()
    for l in lineas:
        m = re.search(r"VICEEXT 1p bind tecla=(-?\d+)", l)
        if m:
            binds.append(int(m.group(1)))
        m = re.search(r"WINFO arma=(\d+) grupo=(\d+) flags=0x([0-9a-fA-F]+) canaim=(\d) "
                      r"witharm=(\d) reload=(\d) crouchr=(\d) sight=(-?\d+) clips=([^ ]+)", l)
        if m:
            armas.append(m.groups())
        m = re.search(r"SIGHT arma=.*\bpor=(\d)", l)
        if m:
            sight_por[int(m.group(1))] += 1

    if binds:
        for t in sorted(set(binds)):
            nota = ""
            if t not in (86, 1056, 0):
                nota = "  <-- NO es la V: pulsar V no podía conmutar (ya se acepta de todas formas)"
            print("   1p bind tecla=%d (86='V', 1056=rsNULL)%s" % (t, nota))
    else:
        print("   1p bind: sin línea (motor anterior a ve26)")

    fallos = []
    if armas:
        print("   %-5s %-16s %-7s %-6s %-7s %-6s %s"
              % ("arma", "flags", "canaim", "witharm", "reload", "sight", "clips fire|agachado|recarga|agachado"))
        for (arma, grupo, flags, canaim, witharm, reload, crouchr, sight, clips) in armas:
            nombre = ARMAS_MOD.get(int(arma), "")
            print("   %-5s %-16s %-7s %-6s %-7s %-6s %s"
                  % (arma, "0x" + flags, canaim, witharm,
                     "%s/%s" % (reload, crouchr), sight, clips))
            if canaim == "1":
                partes = clips.split("|")
                faltan = [n for n, p in zip(("disparo", "agachado", "recarga", "recarga-agachado"), partes)
                          if p in ("-", "?")]
                # Sólo es fallo el clip de DISPARO (pose de apuntado) o, si el arma
                # trae flag de recarga, el de recarga.
                if partes[0] in ("-", "?"):
                    fallos.append("arma %s (%s) sin clip de disparo: el grupo %s no lo trae"
                                  % (arma, nombre or "?", grupo))
                elif reload == "1" and partes[2] in ("-", "?"):
                    fallos.append("arma %s (%s) con flag de recarga pero sin clip de recarga en el grupo %s"
                                  % (arma, nombre or "?", grupo))
    else:
        print("   sin fichas WINFO: ¿motor anterior a ve26 o no se sacó ninguna arma?")

    if sight_por:
        nombres = {1: "conduciendo", 2: "modo de cámara", 3: "apuntando"}
        print("   mira pintada por: " +
              ", ".join("%s=%d" % (nombres.get(k, k), v) for k, v in sorted(sight_por.items())))
        if sight_por.get(3, 0) == 0 and sum(sight_por.values()) > 3:
            fallos.append("la mira se pinta sin apuntar (%d veces y ninguna por el botón de apuntar)"
                          % sum(sight_por.values()))

    if fallos:
        return "FALLO", "; ".join(fallos)
    if not armas and not binds and not sight_por:
        return "SIN DATOS", "motor anterior a ve26 (sin WINFO/1p bind) o no se usó ninguna arma"
    return "OK", "cada arma usada dice sus flags, su pose y sus clips; y la tecla guardada queda a la vista"


def bloque_r12(lineas):
    """R12 · los arreglos de la 9ª partida, dictaminados desde el log.

    - RECARGA a mano: cada `VICEEXT reload start` debe llevar su `reload anim`
      (antes el cargador se rellenaba y no se veía animación ninguna).
    - AGACHADO: `repetido=1` son las pulsaciones que el antirrebote rechaza (el
      bug era que la C contaba dos veces: `off` + `on` y no se podía levantar).
    - AUTOCENTRADO de coche: retornos pasivos (`pedido=0`) y pedidos (`pedido=1`,
      el empujón al soltar la mirada).
    """
    print("\n== R12 · recarga con animación, agachado y autocentrado ==")
    starts = anims = noanim = 0
    repetido = 0
    crouch_on = crouch_off = 0
    pasivo = pedido = 0
    for l in lineas:
        if "VICEEXT reload start" in l:
            starts += 1
        if "VICEEXT reload anim grupo=" in l:
            if "clip=0" in l:
                noanim += 1
            else:
                anims += 1
        if re.search(r"VICEEXT crouch (on|off) .*repetido=1", l):
            repetido += 1
        if re.search(r"VICEEXT crouch on ", l):
            crouch_on += 1
        if re.search(r"VICEEXT crouch off ", l):
            crouch_off += 1
        m = re.search(r"camauto2 auto=1 .*pedido=(\d)", l)
        if m:
            if m.group(1) == "1":
                pedido += 1
            else:
                pasivo += 1
    if not (starts or anims or noanim or repetido or crouch_on or pasivo or pedido):
        return "SIN DATOS", "no se recargó, no se agachó ni se condujo en esta sesión"
    print("   recarga a mano: start=%d  con animación=%d  sin clip (arma sin flag)=%d"
          % (starts, anims, noanim))
    print("   agachado: on=%d off=%d  pulsaciones rechazadas por antirrebote=%d"
          % (crouch_on, crouch_off, repetido))
    print("   autocentrado de coche: pasivo=%d  pedido (soltar la mirada)=%d"
          % (pasivo, pedido))
    fallos = []
    if starts > 0 and anims == 0:
        fallos.append("recarga sin animacion (%d start, 0 anim)" % starts)
    if crouch_off == 0 and crouch_on >= 2:
        fallos.append("se agacha %d veces y no se levanta ni una" % crouch_on)
    if fallos:
        return "FALLO", "; ".join(fallos)
    return "OK", "recarga con animacion, agachado conmutable y autocentrado con sus dos caminos"


def bloque_l6(lineas):
    """L6 · pantalla de carga de partida: UNA secuencia y una barra sin reinicios.

    `LOADSCR begin` la abre (fija splash1 + reinicia la barra monótona) y
    `LOADSCR end` la cierra. `LOADSCR clamp a->b` es el motor intentando
    retroceder la barra en una frontera de fase: si aparece, el recorte funcionó
    (la barra es un único barrido 0->100).
    """
    print("\n== L6 · pantalla de carga de partida (LOADSCR) ==")
    begins = [l for l in lineas if "LOADSCR begin" in l]
    ends = [l for l in lineas if "LOADSCR end" in l]
    clamps = []
    for l in lineas:
        m = re.search(r"LOADSCR clamp ([\d.]+)->([\d.]+)", l)
        if m:
            clamps.append((float(m.group(1)), float(m.group(2))))
    if not begins and not ends:
        return "SIN DATOS", ("sin LOADSCR: o no se cargó partida en esta sesión, o el "
                             "motor servido no trae la pantalla nueva (build viejo)")
    print("   secuencias: begin=%d end=%d" % (len(begins), len(ends)))
    if clamps:
        print("   recortes de barra (fase quiso retroceder y se mantuvo el máximo): %d" % len(clamps))
        for a, b in clamps[:8]:
            print("      %.3f -> %.3f (recortado a %.3f)" % (a, b, b))
    else:
        print("   recortes: 0 (la secuencia fue monótona por sí sola)")
    if len(begins) != len(ends):
        return "FALLO", ("%d secuencias abiertas y %d cerradas: una carga no terminó "
                         "(el splash quedaría fijado a splash1)" % (len(begins), len(ends)))
    return "OK", ("%d carga(s) con una sola secuencia de pantalla; %d reinicios de "
                  "barra recortados" % (len(begins), len(clamps)))


def bloque_r14(lineas):
    """R14 · los cuatro arreglos de la 12ª partida, dictaminados desde el log.

    - CÁMARA DE NADO: mientras `VICEEXT swim move` diga que se nada, `SWIMCAM`
      tiene que traer el objetivo en la superficie (nivel+0,5) y la cámara
      siguiendo al ped. Antes de R14 el motor pedía la cámara de "jugador que se
      ahoga" (clavada en la última posición sobre el agua) y el vídeo del
      navegador la enseñaba fija en la calzada con el ped fuera de cuadro.
    - RETROCESO: `RECOIL3 miracheck multY=` tiene que quedarse en 0,400: la
      retícula no se mueve (el retroceso lo lleva la cámara, `RECOIL2`).
    - AUTOCENTRADO: `pedido=1` son los empujones rápidos. Sólo pueden venir de
      soltar la cruceta/palo; con el ratón (el caso del jugador en PC) tienen que
      ser contados, no decenas por minuto.
    - SONIDOS DE ARMA: `ODSFXPROT sfx=` reserva la muestra del mod al primer
      disparo, y a partir de ahí no debe haber `ODSFXDEFER` de esas muestras.
    """
    print("\n== R14 · cámara de nado, retroceso, autocentrado y sonidos ==")
    swimcam = []
    for l in lineas:
        m = re.search(r"SWIMCAM objetivo=([\d.-]+) nivel=([\d.-]+) cam=([\d.-]+) modo=(\d+)", l)
        if m:
            swimcam.append((float(m.group(1)), float(m.group(2)), float(m.group(3))))
    swim_moves = sum(1 for l in lineas if "VICEEXT swim move" in l)
    miracheck = [float(m.group(1)) for l in lineas
                 for m in [re.search(r"RECOIL3 miracheck multY=([\d.]+)", l)] if m]
    pedido = sum(1 for l in lineas if re.search(r"camauto2 auto=1 .*pedido=1", l))
    camauto = sum(1 for l in lineas if "camauto2" in l)
    prot = {m.group(1) for l in lineas
            for m in [re.search(r"ODSFXPROT sfx=(\d+)", l)] if m}
    defer = sum(1 for l in lineas if "ODSFXDEFER" in l)
    if not (swimcam or miracheck or camauto or prot):
        return "SIN DATOS", "no se nadó, no se disparó ni se condujo en esta sesión"
    print("   nado: %d muestras de SWIMCAM (%d trazas de movimiento)" % (len(swimcam), swim_moves))
    if swimcam:
        objetivo_ok = sum(1 for o, n, c in swimcam if o >= n)
        print("      objetivo en la superficie o por encima: %d/%d"
              % (objetivo_ok, len(swimcam)))
        if len(swimcam) > 2:
            print("      cámara: min=%.2f max=%.2f  variación=%.2f m"
                  % (min(c for _, _, c in swimcam), max(c for _, _, c in swimcam),
                     max(c for _, _, c in swimcam) - min(c for _, _, c in swimcam)))
        else:
            objetivo_ok = len(swimcam)   # con dos muestras no se juzga la deriva
    else:
        objetivo_ok = 0
    print("   retroceso: %d comprobaciones de mira; multY=%s"
          % (len(miracheck), ("%.3f..%.3f" % (min(miracheck), max(miracheck))) if miracheck else "-"))
    print("   autocentrado: %d líneas camauto2, %d con empujón pedido" % (camauto, pedido))
    print("   sonidos de arma: %d muestras reservadas, %d decodificaciones aplazadas"
          % (len(prot), defer))
    fallos = []
    if swim_moves > 3 and not swimcam:
        fallos.append("se nadó y no hay SWIMCAM (motor sin R14): la cámara sigue siendo la de ahogado")
    if miracheck and (min(miracheck) < 0.38 or max(miracheck) > 0.42):
        fallos.append("la retícula se movió con el retroceso (multY=%.3f)" % max(miracheck))
    if pedido >= 20:
        fallos.append("el empujón rápido del autocentrado salta %d veces (el ratón lo dispara)" % pedido)
    if camauto > 0 and pedido > camauto:
        fallos.append("más empujones pedidos que líneas de autocentrado: revisa el detector")
    if fallos:
        return "FALLO", "; ".join(fallos)
    return "OK", "nado con cámara en la superficie, retícula fija, autocentrado sin tirones del ratón"

# Modos de cámara de CCam que significan "1ª persona / francotirador": con el
# lanzacohetes en la mano y el botón de apuntar pulsado NO deben aparecer (mod
# Vice Extended, `RocketLauncherThirdPersonAiming=1`). Valores del enum de
# `src/core/Camera.h` (orden): ROCKETLAUNCHER=8, ROCKETLAUNCHER_RUNABOUT=40,
# 1STPERSON=16, M16_1STPERSON=34, M16_1STPERSON_RUNABOUT=42,
# HELICANNON_1STPERSON=45, CAMERA=46.
MODOS_1P_ROCKET = {8, 40, 16, 34, 42, 45, 46}



def bloque_ax(lineas):
    """AX · ClassicAXIS (mod gennariarmando/DK22Pac, sin LICENSE: reimplementación).

    Bloque del plan `apuntado-classicaxis-100`, que lleva la ley de apuntado del
    mod (CamNew.cpp `Process_AimWeapon`) al 100% y sus 9 ajustes de la sección
    [ClassicAxis] del INI de 2022.

    «Un PASS sin medición es mentira»: si el log no trae la traza, `INCONCLUSIVE`,
    nunca PASS. Los criterios se añaden con los builds que emiten su traza:
    · AX0  (build aim1) los 9 ajustes y el `WalkKey`.
    · AX1-AX6 (builds aim2-aim5) la ley, el hombro, el clamp, el FOV 50, el
      auto-aim sin ratón y la no-regresión del recoil.
    Los criterios §8.14 (auto-centrado), §8.15 (agachado) y §8.16 (nado) NO se
    duplican aquí: los cubren los bloques H, R27 y R5 del verificador.
    """
    print("\n== AX · ClassicAXIS: ajuste y tecla de andar (build 1) ==")

    # ---- AX0 · los 9 ajustes de [ClassicAxis] came leídos del INI
    cfg = [m for l in lineas
           for m in [re.search(r"AIMCFG forceauto=(-?\d+) lock=(-?\d+) tri=(-?\d+) "
                               r"mcx=([\d.]+) mcy=([\d.]+) brazo=(-?\d+) "
                               r"sensx=([\d.]+) sensy=([\d.]+) fov=(-?\d+) ley=(-?\d+)", l)]
           if m]
    # ---- AX0b · el WalkKey (tecla de andar) llega a poner la velocidad a 0
    walk = [m for l in lineas
            for m in [re.search(r"AIMWALK tecla=(-?\d+) spd=(-?[\d.]+)", l)]
            if m]

    if not cfg:
        return "INCONCLUSIVE", ("el log no trae ninguna línea AIMCFG: la build servida no lleva "
                                "los 9 ajustes de ClassicAXIS (o no se llegó a la línea de cámara)")
    # Los 9 defaults son VERBATIM del INI del mod (ClassicAxisVC.ini), salvo
    # `zoomForAssaultRifles` que es decisión del jugador del 27/09 (§5.3a): el INI del
    # mod no trae esa clave, así que aquí se espera 1, no el 0 del INI.
    esperado = dict(forceauto=0, lock=1, tri=1, mcx=0.530, mcy=0.400,
                    brazo=0, sensx=1.00, sensy=1.00, fov=1)
    c = cfg[0]
    leido = dict(forceauto=int(c.group(1)), lock=int(c.group(2)), tri=int(c.group(3)),
                 mcx=float(c.group(4)), mcy=float(c.group(5)), brazo=int(c.group(6)),
                 sensx=float(c.group(7)), sensy=float(c.group(8)), fov=int(c.group(9)))
    print("   ajustes leídos: %s" % " ".join("%s=%.3f" % (k, leido[k]) for k in
                                                 ("forceauto", "lock", "tri", "mcx", "mcy",
                                                  "brazo", "sensx", "sensy", "fov")))
    malos = []
    for k, v in esperado.items():
        got = leido[k]
        okk = abs(got - v) < 0.001 if isinstance(v, float) else got == v
        if not okk:
            malos.append("%s=%s (se esperaba %s)" % (k, got, v))
    if malos:
        return "FALLO", ("los 9 ajustes de [ClassicAxis] no salen como deben: %s" % "; ".join(malos))
    if not walk:
        print("   (sin AIMWALK: la tecla de andar no se tocó en la sesión, no se juzga)")
    else:
        teclas = sorted(set(int(m.group(1)) for m in walk))
        spds = [float(m.group(2)) for m in walk]
        ceros = sum(1 for s in spds if abs(s) < 0.001)
        print("   WalkKey: teclas vistas=%s, eventos=%d, con spd=0.00: %d (min=%.2f max=%.2f)"
              % (teclas, len(walk), ceros, min(spds), max(spds)))
        if ceros == 0:
            return "FALLO", ("hay líneas AIMWALK pero ninguna con spd=0.00: la tecla de andar "
                             "(LALT) no está parando la velocidad al pulsar")
    print("\n== AX1-5 · ClassicAXIS: la ley de apuntado (build 2) ==")

    cam = [m for l in lineas
           for m in [re.search(r"AIMCAM m=(-?\d+) dist=([\d.]+) alt=([\d.]+) zoff=([\d.]+) "
                               r"amax=([\d.]+) fight=(-?\d+) hombro=([\d.]+) obj=(-?\d+) "
                               r"ex=(-?[\d.]+) ey=(-?[\d.]+) ez=(-?[\d.]+) lado=(-?\d+) trans=(\d+)", l)]
           if m]
    fov = [m for m in
           (re.search(r"AIMFOV fov=([\d.]+) arma=(-?\d+) alcance=([\d.]+) taken=(-?\d+)", l)
            for l in lineas) if m]
    vec = [m for m in
           (re.search(r"AIMVEC chx=(-?[\d.]+) chy=(-?[\d.]+) cfov=([\d.]+) dfov=([\d.]+)"
                     r" ax=(-?[\d.]+) ay=(-?[\d.]+) tv=(-?[\d.]+) ph=(-?[\d.]+) dth=(-?[\d.]+)"
                     r" scx=([\d.]+) scy=([\d.]+) asp=([\d.]+)", l)
            for l in lineas) if m]
    law = [m for m in
           (re.search(r"AIMLAW aim=(-?\d+) modo=(-?\d+) estado=(-?\d+) arma=(-?\d+) "
                      r"lock=(-?\d+) aut=(-?\d+) esp=([\d.]+) aim2=(-?\d+)", l)
            for l in lineas) if m]
    col = [m for m in
           (re.search(r"AIMCOL los=(-?\d+) losD=([\d.]+) sph=(-?\d+) sphM=(-?\d+) "
                      r"sphPed=(-?\d+) dSph=([\d.]+) dRaw=([\d.]+) nc=([\d.]+) app=(-?\d+) pedes=(-?\d+)", l)
            for l in lineas) if m]

    fallos = []

    # ---- AX1 · la ley existe y con los números del mod (§8.1) ----
    if not cam:
        return "INCONCLUSIVE", ("los 9 ajustes se leen (AX0 pasó) pero el log no trae ninguna "
                                "línea AIMCAM: la build servida no lleva la ley ClassicAXIS, "
                                "o no se apuntó en la sesión")
    modos = sorted(set(int(m.group(1)) for m in cam))
    dists = [float(m.group(2)) for m in cam]
    print("   AX1 ley: muestras=%d modos=%s dist min=%.2f max=%.2f (el mod: 2.70 fija)"
          % (len(cam), modos, min(dists), max(dists)))
    malos_modo = [x for x in modos if x != 5]
    if malos_modo:
        fallos.append("AX1: hay AIMCAM con modo %s y la ley de apuntado es el 5 (MODE_AIMING)"
                      % malos_modo)
    fuera = [d for d in dists if abs(d - 2.70) > 0.35]
    if fuera:
        fallos.append("AX1: %d de %d muestras con dist fuera de 2.70 +-0.35 (min %.2f max %.2f)"
                      % (len(fuera), len(dists), min(dists), max(dists)))

    # ---- AX2 · hombro 0.20 en espacio de objeto (§8.2 / §8.3) ----
    homb = sorted(set(float(m.group(7)) for m in cam))
    objs = sorted(set(int(m.group(8)) for m in cam))
    # `ex/ey/ez` es el vector offset REAL aplicado, o sea `GetRight() del ped * 0.20`.
    # Su componente X depende del rumbo del ped (0.20*cos a), así que lo que se
    # comprueba es la MAGNITUD, que sí tiene que medir 0.20 para cualquier rumbo.
    offs = [(float(m.group(9)), float(m.group(10)), float(m.group(11))) for m in cam]
    mags = [(ex * ex + ey * ey + ez * ez) ** 0.5 for (ex, ey, ez) in offs]
    print("   AX2 hombro: valores=%s obj=%s |offset real| min=%.3f max=%.3f (debe medir 0.20)"
          % (homb, objs, min(mags), max(mags)))
    if any(abs(h - 0.20) > 0.001 for h in homb):
        fallos.append("AX2: hombro=%s y el mod usa 0.20" % homb)
    if any(o != 1 for o in objs):
        fallos.append("AX2: obj=%s y debe ser 1 (el offset va en el espacio de OBJETO del ped)" % objs)
    if any(abs(mag - 0.20) > 0.02 for mag in mags):
        fallos.append("AX2: el |offset| REAL aplicado mide %.3f-%.3f y debe medir 0.20; el hombro "
                      "no va en el espacio de objeto del ped"
                      % (min(mags), max(mags)))

    # ---- AX8 · el numero crudo del apuntado (AIMVEC, 04/10) ----
    # Traza anadida porque el jugador reporto "apunto y gira ~180 grados" tres sesiones
    # seguidas y A no tenia forma de dictaminarlo. Se mide, no se supone.
    if vec:
        dths = [m.group(9) for m in vec]
        cfovs = [m.group(3) for m in vec]
        dfovs = [m.group(4) for m in vec]
        chxs = [m.group(1) for m in vec]
        chys = [m.group(2) for m in vec]
        asps = [m.group(10) for m in vec]
        fuera = [d for d in dths if abs(d) > 60.0]
        print("   AX8 mira: %d muestras | dth min %+.1f max %+.1f | fuera de +-60: %d (%.0f%%)"
              % (len(vec), min(dths), max(dths), len(fuera), 100.0 * len(fuera) / len(vec)))
        print("        (dth se calcula contra `ph`, que sale de la POSICIÓN del ped: NO mide "
              "giro de cámara; el giro se dictamina con yaw/rayo de la matriz renderizada)")
        print("        chx=%s chy=%s (el mod: 0.53 / 0.4)  asp=%s"
              % (sorted(set(chxs)), sorted(set(chys)), sorted(set(asps))))
        desinc = sum(1 for a, b in zip(cfovs, dfovs) if abs(float(a) - float(b)) > 0.5)
        print("        cfov=%s dfov=%s  desincronizados: %d de %d"
              % (sorted(set(cfovs)), sorted(set(dfovs)), desinc, len(vec)))
        if fuera:
            print("        OJO: %d muestras con el punto de mira a mas de 60 grados del rumbo"
                  " del ped. Es el giro que reporto el jugador" % len(fuera))
        for cx in sorted(set(chxs)):
            if abs(float(cx) - 0.53) > 0.01:
                fallos.append("AX8: el multiplicador de la cruz en X es %s y el .ini del mod"
                              " dice 0.53" % cx)
        for cy in sorted(set(chys)):
            if abs(float(cy) - 0.4) > 0.01:
                fallos.append("AX8: el multiplicador de la cruz en Y es %s y el .ini del mod"
                              " dice 0.4" % cy)
        if desinc == len(vec) and len(vec) > 3:
            fallos.append("AX8: el vector de mira se calcula con cfov=%s y se dibuja con"
                          " dfov=%s en TODAS las muestras: el punto de mira cae donde no es"
                          % (cfovs[0], dfovs[0]))
    else:
        print("   (sin AIMVEC: build anterior a la del 04/10, no se puede medir el apuntado)")

    # ---- AX3 · clamp ±50° del Alpha (§8.5) ----
    amax = max(float(m.group(5)) for m in cam)
    print("   AX3 clamp: amax=%.1f° (tope del mod: 50)" % amax)
    if amax > 50.5:
        fallos.append("AX3: el Alpha llegó a %.1f° y la ley lo clampa a 50 (CamNew.cpp:350-353)" % amax)

    # ---- AX4 · FOV 50 al rifle de alcance >= 70 (§8.6) ----
    if fov:
        TOMADOS = [m for m in fov if int(m.group(4)) == 1]
        TOMADOS_MAL = [m for m in TOMADOS if float(m.group(3)) < 70.0]
        MINIGUN = [m for m in TOMADOS if int(m.group(2)) == 33]   # WEAPONTYPE_MINIGUN (WeaponType.h)
        fv_tom = [float(m.group(1)) for m in TOMADOS]
        fv_no = [float(m.group(1)) for m in fov if int(m.group(4)) == 0]
        print("   AX4 FOV: muestras=%d tomadas=%d (fov min %.2f) | no tomadas=%d (fov max %.2f)"
              % (len(fov), len(TOMADOS), min(fv_tom) if fv_tom else -1.0,
                 len(fv_no), max(fv_no) if fv_no else -1.0))
        if TOMADOS_MAL:
            fallos.append("AX4: el FOV se tomó con armas de alcance < 70 (%s); el umbral del "
                          "mod es wepMinRange=70" % [int(m.group(2)) for m in TOMADOS_MAL])
        if MINIGUN:
            fallos.append("AX4: el Minigun (arma 33) tomó el FOV (CamNew.cpp:484-486 lo EXCLUYE en VC)")
        if fv_tom and min(fv_tom) > 50.5:
            fallos.append("AX4: con el rifle apuntado el FOV se quedó en %.2f y debería bajar a 50"
                          % min(fv_tom))
    else:
        print("   (sin AIMFOV: no se puede juzgar el FOV 50)")

    # ---- AX5 · §5.2(a): con ratón no hay auto-aim ----
    if law:
        aut1 = [m for m in law if int(m.group(6)) == 1]
        print("   AX5 auto-aim: flancos=%d con aut=1 (autoapuntado DESACTIVADO en esos "
              "flancos: ViceExtIsAutoAimDisabled; con ratón es lo esperado)"
              % len(aut1))
    else:
        print("   (sin AIMLAW: no se puede juzgar el auto-aim)")

    # ---- colisiones: no-regresión del refactor (B3c) ----
    if col:
        print("   colisiones: muestras=%d nc min=%.3f max=%.3f app max=%d"
              % (len(col), min(float(m.group(8)) for m in col),
                 max(float(m.group(8)) for m in col), max(int(m.group(9)) for m in col)))
    else:
        print("   (sin AIMCOL: no se puede juzgar las colisiones de la ley)")

    # ---- AX6 · el CUERPO sigue a la camara (el fallo del 28/09) ----
    # El 28/09 el verificador dio OK con el cuerpo 90 grados desviado, porque solo
    # miraba dist/hombro/amax. Este criterio mide la ALINEACION, que es lo que el
    # jugador ve, y es la que caza el fallo (AIMDIR, el bloque R9).
    aimdir = [m for m in
              (re.search(r"AIMDIR arma=(-?\d+) desv=([\d.]+)", l)
               for l in lineas) if m]
    if aimdir:
        peor = {}
        for m in aimdir:
            a = int(m.group(1)); d = float(m.group(2))
            peor[a] = max(peor.get(a, 0.0), d)
        print("   AX6 cuerpo: desv máx por arma %s (R9 falla con desv media >5 grados)"
              % {k: round(v, 1) for k, v in sorted(peor.items())})
        malos = {k: v for k, v in peor.items() if v > 5.0}
        if malos:
            fallos.append("AX6: el cuerpo NO sigue a la camara al apuntar (desv max %s grados); "
                          "se espera <=5. El 28/09 fueron 90 grados, por usar `Beta` y "
                          "forzar `SetHeading` en vez del valor del motor"
                          % {k: round(v, 1) for k, v in malos.items()})
    else:
        print("   (sin AIMDIR: no se puede juzgar la alineacion del cuerpo)")

    # ---- AX7 · el cuerpo hacia la MARCHA y no hacia la camara (28/09, ronda 2) ----
    # El defecto que reporto el jugador: con A/D solo, sin mover el raton, el cuerpo
    # debe girarse hacia la direccion de marcha; con el raton y el cuerpo quieto, solo
    # debe girar (360). `tipo` sale de `ViceExtStrafeAiming`, que es el port del
    # `playerMovementType` del mod: 0 = WALKAROUND (siga la marcha), 1 = STRAFE
    # (apuntando, de costado), 2 = un frame de locomocion real (`forceRealMoveAnim`).
    ax7 = [m for m in
           (re.search(r"AX7 tipo=(\d) lr=(-?[\d.]+) ud=(-?[\d.]+) desv=([\d.]+)", l)
            for l in lineas) if m]
    if ax7:
        andando_lado = [m for m in ax7 if abs(float(m.group(2))) > 20.0]
        quietos = [m for m in ax7
                   if abs(float(m.group(2))) < 1.0 and abs(float(m.group(3))) < 1.0]
        if andando_lado:
            Desv = [float(m.group(4)) for m in andando_lado]
            tipos = sorted({int(m.group(1)) for m in andando_lado})
            print("   AX7 andando de lado: %d muestras, desv media %.1f max %.1f, tipos %s"
                  % (len(Desv), sum(Desv) / len(Desv), max(Desv), tipos))
            malos = [d for d in Desv if d > 60.0]
            if malos:
                fallos.append("AX7: andando de LADO sin apuntar el cuerpo sigue a la camara "
                              "(desv >60 grados en %d de %d muestras: %s); el port de "
                              "`playerMovementType` (WALKAROUND) no esta efecto"
                              % (len(malos), len(Desv), [round(d) for d in malos[:5]]))
        else:
            print("   AX7: sin muestras de lateralidad (hay que andar con A/D o D)")
        if quietos:
            t2 = [int(m.group(1)) for m in quietos]
            print("   AX7 cuerpo quieto con raton: %d muestras, tipos %s (2 = frame de "
                  "locomocion real, el 360 del mod)" % (len(t2), sorted(set(t2))))
            if any(v == 1 for v in t2):
                fallos.append("AX7: con el cuerpo quieto y el raton hay muestras en STRAFE "
                              "(tipo=1); el mod solo fuerza STRAFE apuntando, asi que el "
                              "cuerpo no deberia quedarse de costado parado")
        else:
            print("   AX7: sin muestras de cuerpo quieto (no hay ninguna con lr=ud=0)")
    else:
        print("   (sin AX7: no se puede juzgar si el cuerpo sigue la marcha o la camara)")

    if fallos:
        return "FALLO", "; ".join(fallos)
    return "OK", ("ley ClassicAXIS viva en el modo 5 con dist 2.70, hombro 0.20 en espacio de "
                  "objeto, Alpha dentro de ±50°, FOV 50 solo con rifles de alcance >= 70 "
                  "(Minigun excluido) y los 9 ajustes verbatim del INI del mod")


def bloque_r16(lineas):
    """R16 · el lanzacohetes apunta en TERCERA persona (mod Vice Extended).

    El mod trae `RocketLauncherThirdPersonAiming=1`: apuntar con el lanzacohetes
    NO debe meter al jugador en el modo francotirador (cámara de 1ª persona,
    `PED_SNIPER_MODE`, velocidad a 0). Antes de R16 el motor nunca sacaba la
    línea `R3P` (la rama no existía) y el jugador quedaba clavado.

    Evidencia: `R3P arma= modo= ped= mira= apunta= spd=` (1/s con el arma en la
    mano). Con `mira=1`: el `modo` tiene que ser de tercera persona (no estar en
    MODOS_1P_ROCKET) y `spd` es la velocidad a la que se puede andar apuntando.
    """
    print("\n== R16 · lanzacohetes: apuntado en 3ª persona (Vice Extended) ==")
    r3p = [(int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)),
            int(m.group(5)), float(m.group(6)))
           for l in lineas
           for m in [re.search(r"R3P arma=(\d+) modo=(\d+) ped=(\d+) mira=(\d+) apunta=(\d+) spd=([\d.]+)", l)]
           if m]
    if not r3p:
        return "SIN DATOS", ("no se sacó el lanzacohetes en esta sesión (si lo sacaste, el motor "
                              "servido no lleva R16)")
    armas = sorted(set(a for a, _, _, _, _, _ in r3p))
    apuntando = [t for t in r3p if t[3] == 1]
    modos = sorted(set(m for _, m, _, _, _, _ in apuntando))
    print("   armas vistas: %s; muestras=%d (apuntando: %d)"
          % (",".join(str(a) for a in armas), len(r3p), len(apuntando)))
    print("   con el botón de apuntar pulsado: modos de cámara=%s estados del ped=%s"
          % (modos or "-", sorted(set(p for _, _, p, _, _, _ in apuntando)) or "-"))
    if apuntando:
        spds = [s for _, _, _, _, _, s in apuntando]
        con_punta = [t for t in apuntando if t[4] == 1]
        print("   velocidad apuntando: máx=%.1f (apuntando de verdad brazo/arma: %d/%d muestras)"
              % (max(spds), len(con_punta), len(apuntando)))
    malos = [m for m in modos if m in MODOS_1P_ROCKET]
    if malos:
        return "FALLO", ("apuntando con el lanzacohetes el modo de cámara volvió a ser de 1ª "
                         "persona/francotirador (modo=%s): el jugador queda clavado"
                         % ",".join(str(m) for m in malos))
    return "OK", ("apuntando con el lanzacohetes la cámara sigue en 3ª persona (modos=%s) y se "
                  "puede andar (spd máx=%.1f)"
                  % (",".join(str(m) for m in modos) or "-",
                     max((s for _, _, _, _, _, s in apuntando), default=0.0)))


def bloque_r15(lineas):
    """R15 · desagacharse devuelve la pose de pie (13ª partida).

    Síntoma del jugador: "estando agachado le vuelvo a dar para pararse y sólo
    cambia la altura de la cámara: el personaje sigue agachado". Los clips del
    mod (`crouch_*`) son parciales y de otro grupo, así que el motor no los
    retiraba al mezclar el caminar/idle: el cuerpo se quedaba agachado.

    Evidencia que se lee aquí:
      VICEEXT crouch pose off clips=<0..3>   el motor retiró los clips (R15).
      CROUCHPOSE desde=<ms> idle= fwd= back=  pesos reales de los clips de
        agachado MIENTRAS no se está agachado, con los ms desde el desagachado.
        El fundido es de 0,25 s: cualquier línea con `desde >= 700` tiene que
        salir con los tres pesos a 0. Un peso alto ahí = el cuerpo sigue
        agachado (el fallo del jugador).
    """
    print("\n== R15 · desagacharse: la pose vuelve a la de pie ==")
    on = sum(1 for l in lineas if re.search(r"VICEEXT crouch on", l))
    off = [l for l in lineas if re.search(r"VICEEXT crouch off", l)]
    motivos = [m.group(1) for l in off
               for m in [re.search(r"VICEEXT crouch off motivo=(\w+)", l)] if m]
    pose_off = [int(m.group(1)) for l in lineas
                for m in [re.search(r"VICEEXT crouch pose off clips=(\d+)", l)] if m]
    pose = []
    for l in lineas:
        m = re.search(r"CROUCHPOSE desde=(\d+) idle=([\d.]+) fwd=([\d.]+) back=([\d.]+)"
                      r"(?: izq=([\d.]+) der=([\d.]+))?"
                      r"(?: aimfwd=([\d.]+) aimbwd=([\d.]+) aimizq=([\d.]+) aimder=([\d.]+))?"
                      r"(?: pose=([\d.]+))?", l)
        if m:
            pesos = [float(m.group(i)) for i in range(2, 12) if m.group(i) is not None]
            pose.append((int(m.group(1)), max(pesos)))
    if not (on or off or pose):
        return "SIN DATOS", "no se pulsó la tecla de agacharse en esta sesión"
    print("   conmutaciones: %d on, %d off%s"
          % (on, len(off), (" (" + ", ".join(sorted(set(motivos))) + ")") if motivos else ""))
    print("   retiradas de clip: %d líneas 'pose off' (clips retirados: %s)"
          % (len(pose_off), ", ".join(str(c) for c in pose_off) if pose_off else "-"))
    if pose:
        tarde = [w for d, w in pose if d >= 700]
        print("   peso de los clips de agachado estando de pie: %d muestras, máx=%.2f;"
              " con `desde>=700`: %s"
              % (len(pose), max(w for _, w in pose),
                 ("máx=%.2f en %d muestras" % (max(tarde), len(tarde))) if tarde else "sin muestras"))
    else:
        print("   peso de los clips de agachado estando de pie: sin muestras")
    fallos = []
    if off and not pose_off:
        fallos.append("se desagachó y no hay 'VICEEXT crouch pose off' (motor sin R15): el clip del mod se queda puesto")
    if off and pose_off and not pose:
        fallos.append("no hay ninguna muestra CROUCHPOSE: la retirada de clips (pose off) no basta "
                      "para afirmar que la pose vuelva a estar de pie")
    tarde_altas = [w for d, w in pose if d >= 700 and w > 0.05]
    if tarde_altas:
        fallos.append("el cuerpo sigue agachado tras desagacharse (peso %.2f con 700+ ms desde el 'off')"
                      % max(tarde_altas))
    if fallos:
        return "FALLO", "; ".join(fallos)
    return "OK", "clips de agachado retirados al desagacharse y peso a 0 (vuelve a estar de pie)"


def bloque_r17(lineas):
    """R17 · agachado y nado: el personaje AVANZA y la cámara lo sigue (14ª partida).

    Síntoma del jugador: "la cámara se mantiene fija y el personaje sin
    desplazamiento al nadar y agacharse". Dos causas distintas, una prueba por
    cada una:

    - AGACHADO: en III/VC el ped anda por la TRASLACIÓN DE LA RAÍZ del clip de
      movimiento (`m_vecAnimMoveDelta` -> `CalculateNewVelocity` -> `UpdatePosition`
      -> `ApplyMoveSpeed`), y `UpdatePosition` sólo corre con `bIsStanding`.
      Agachado, `SetRealMoveAnim` no mezclaba caminar/correr y el clip del mod era
      `ASSOC_PARTIAL` (sin `ASSOC_MOVEMENT`/`ASSOC_HAS_TRANSLATION`): ni aportaba
      velocidad ni se le extraía la traslación. Resultado: el ped no se movía y la
      cámara, que sigue al ped, tampoco. Ahora los clips son de movimiento.
      Evidencia: `CROUCH2 ... avance=<m/s>` (metros/s RECORRIDOS de verdad). Con
      el palo empujado (`VICEEXT crouch move ... andando=` > 0) el avance tiene
      que ser > 0,4 m/s; antes de R17 salía 0,00 clavado.

    - NADO: la cámara del jugador sólo puede ser la de seguir-al-ped. El motor
      pedía `MODE_PLAYER_FALLEN_WATER` (`Process_Player_Fallen_Water`: fija la
      cámara 4 m sobre `m_vecLastAboveWaterCamPosition` y la deja CLAVADA) en
      cuanto el ped quedaba por debajo de la superficie sin estar nadando "de
      verdad" (p. ej. al salir a agua poco honda). Vídeo de la sesión de las
      14:38: la cámara sobre el agua, el ped fuera de cuadro y `camz=12,79`
      constante mientras avanzaba a 1,3 m/s.
      Evidencia: `SWIM2 ... camdist=<m> modo=<n>`: `modo` tiene que ser 4
      (MODE_FOLLOWPED) y `camdist` la distancia real cámara-ped (1,5-6 m con la
      cámara de serie; con la de ahogado se dispara y `modo` deja de ser 4), más
      la línea única `SWIMCAM no-fallen-water`.
    """
    print("\n== R17 · agachado y nado: avance real y cámara que sigue al ped ==")
    # Las dos trazas salen en el mismo tick de 1 s y llevan la misma marca de
    # tiempo: se cruzan por ella (el palo y el avance medido son del mismo
    # instante).
    def marca(l):
        m = re.match(r"(\S+)", l)
        return m.group(1) if m else ""
    crouch_palo = {}   # marca -> andando
    for l in lineas:
        m = re.search(r"VICEEXT crouch move spd=([\d.-]+) andando=([\d.-]+) clip=(\d+)", l)
        if m:
            crouch_palo[marca(l)] = float(m.group(2))
    # OJO al orden de campos: desde R20 la línea lleva `clip=<id> nom=<nombre>
    # peso=<peso>` (antes era `clip= peso=` pegados). El patrón viejo no casaba
    # NINGUNA línea y este bloque daba FALLO falso ("motor sin R17") en los logs
    # del 26/09 con la traza buena delante.
    crouch_av = [(marca(l), float(m.group(1)), int(m.group(2)), float(m.group(3)))
                 for l in lineas
                 for m in [re.search(r"CROUCH2 h=[\d.-]+ avance=([\d.-]+) .*clip=(\d+) nom=\S+ peso=([\d.-]+)", l)]
                 if m]   # (marca, avance, clip, peso)
    crouch = [v for v in crouch_palo.values()]
    # R19: el orden de campos de `SWIM2` cambia (velo/dt/dz se añadieron
    # después); se buscan por nombre en vez de por posición, que es lo que dejó
    # este bloque en FALLO el 22/09 con la traza buena delante.
    swim = [(float(m.group(1)), float(m.group(2)), int(m.group(3)))
            for l in lineas
            for m in [re.search(r"SWIM2 move .*? avance=([\d.-]+).*? camdist=([\d.-]+) modo=(\d+)", l)]
            if m]   # (avance, camdist, modo)
    swim_cam = [(float(m.group(1)), float(m.group(3)), int(m.group(4)))
                for l in lineas
                for m in [re.search(r"SWIMCAM objetivo=([\d.-]+) nivel=([\d.-]+) cam=([\d.-]+) modo=(\d+)", l)]
                if m]   # (objetivo, camz, modo)
    no_fallen = sum(1 for l in lineas if "SWIMCAM no-fallen-water" in l)
    if not (crouch or crouch_av or swim or swim_cam):
        return "SIN DATOS", "no se anduvo agachado ni se nadó en esta sesión"
    fallos = []

    if crouch_av:
        # R19: sólo cuentan las muestras de estado ESTABLE (el palo en la misma
        # posición que en la muestra anterior). En la primera muestra tras
        # soltar el palo el ped todavía frena (0,4-0,8 m/s) y en la primera tras
        # agacharse lleva la velocidad de cuando iba de pie: sin este filtro el
        # bloque cantaba "parado pero se mueve" (falso fallo, visto el 22/09).
        con_palo, sin_palo, palo_prev = [], [], 0.0
        for odts, av, _, _ in crouch_av:
            palo = crouch_palo.get(odts, 0.0)
            if palo > 0.0 and palo_prev > 0.0:
                con_palo.append(av)
            elif palo <= 0.0 and palo_prev <= 0.0:
                sin_palo.append(av)
            palo_prev = palo
        print("   agachado: %d avances medidos (con el palo empujado: %d, parado: %d), "
              "máx andando=%.2f m/s, máx parado=%.2f m/s"
              % (len(crouch_av), len(con_palo), len(sin_palo),
                 max(con_palo) if con_palo else 0.0, max(sin_palo) if sin_palo else 0.0))
        clavado = [av for av in con_palo if av < 0.4]
        if con_palo and len(clavado) == len(con_palo):
            fallos.append("andando agachado el ped NO se desplaza (avance máx=%.2f m/s): "
                          "los clips del mod siguen sin ser de movimiento" % max(con_palo))
        elif len(clavado) > len(con_palo) / 2:
            fallos.append("la mitad o más de las muestras agachado-andando salen clavadas "
                          "(avance < 0,4 m/s)")
        elif sin_palo and max(sin_palo) > 0.4:
            fallos.append("agachado PARADO el ped se mueve (avance máx=%.2f m/s): el clip "
                          "de avance no se retira al soltar el palo" % max(sin_palo))
    elif crouch:
        fallos.append("hay trazas de agachado sin `CROUCH2 ... avance=` (motor sin R17): "
                      "no se puede saber si el ped se mueve")

    if swim:
        avances = [av for av, _, _ in swim]
        modos = sorted(set(m for _, _, m in swim))
        dist = [d for _, d, _ in swim]
        print("   nado: %d muestras, avance máx=%.2f m/s, modos de cámara=%s, camdist=%.2f..%.2f m"
              % (len(swim), max(avances), modos, min(dist), max(dist)))
        malos = [m for m in modos if m != 4]
        if malos:
            fallos.append("nadando la cámara NO es la de seguir-al-ped (modo=%s): el ped se sale de cuadro"
                          % ",".join(str(m) for m in malos))
        if max(dist) > 8.0:
            fallos.append("nadando la cámara se queda lejos del ped (camdist máx=%.2f m)" % max(dist))
    elif swim_cam:
        print("   nado: %d muestras de SWIMCAM (sin la línea de avance/camdist: motor sin R17)"
              % len(swim_cam))
        fallos.append("hay trazas de nado sin `SWIM2 ... camdist= modo=` (motor sin R17): no se puede "
                      "saber si la cámara sigue al ped")
    if swim and no_fallen == 0:
        fallos.append("no hay 'SWIMCAM no-fallen-water': el motor servido no lleva el arreglo R17 de la cámara")
    if no_fallen:
        print("   cámara: %d línea(s) 'SWIMCAM no-fallen-water' (jugador vivo en el agua -> seguir-al-ped)"
              % no_fallen)

    # 1.4 · POSE: nadando, la animación de CAÍDA no puede colarse por encima del
    # clip de nado. `glide` es la mezcla de `FALL_GLIDE`, que es la que pedía
    # `SetInTheAir` (Ped.cpp:5577) cuando el motor declaraba «en el aire» al
    # nadador por no encontrar suelo debajo; `fall` es la de `FALL`. Las dos a 0
    # y el clip de nado (`sw`) bien mezclado es la prueba de que ya no se cuela.
    pose = [(float(m.group(1)), float(m.group(2)), float(m.group(3)))
            for l in lineas
            for m in [re.search(r"SWIM2 move .*? sw=([\d.-]+) fall=([\d.-]+) .*? glide=([\d.-]+)", l)]
            if m]   # (sw, fall, glide)
    if pose:
        coladas = sum(1 for _, _, gl in pose if gl > 0.0)
        caidas = sum(1 for _, fa, _ in pose if fa > 0.0)
        flojo = sum(1 for sw, _, _ in pose if sw < 0.5)
        print("   pose: %d muestras · glide>0 %d · fall>0 %d · clip de nado sw<0.5 %d"
              % (len(pose), coladas, caidas, flojo))
        if coladas:
            fallos.append("nadando se cuela la pose de CAÍDA (glide>0 en %d de %d muestras): "
                          "el motor sigue declarando 'en el aire' al nadador" % (coladas, len(pose)))
        if flojo:
            fallos.append("nadando el clip de nado casi no se mezcla (sw<0.5 en %d de %d muestras): "
                          "no se ve la brazada" % (flojo, len(pose)))
        if caidas:
            print("   aviso: fall>0 en %d muestra(s) (transición de entrada)" % caidas)
    else:
        print("   pose: sin `glide=` en las trazas de nado (motor sin la 1.4)")

    # 1.5 · ALTURA: nadando, el estado de nado es el único que manda en el eje
    # vertical, así que la profundidad sobre el punto del ped (`hondo`, que lleva
    # FEET_OFFSET = 1.04 m por encima de los pies) debe quedarse en el pin
    # (VICEEXT_SWIM_CHEST_M - FEET_OFFSET = 0.41). Antes el empuje de fluidos del
    # motor escribía la velocidad DESPUÉS del pin y el ped bajaba hasta el fondo:
    # en la sesión 21:47-36 `hondo` llegó a 15.84 con el pin al máximo subiendo.
    # El umbral de 6.0 m deja pasar los picos de entrada desde un clavado (unos
    # pocos metros) y caza el hundimiento real.
    prof = [float(m.group(1))
            for l in lineas
            for m in [re.search(r"SWIM2 move .*? hondo=([-\d.]+)", l)]
            if m]
    if prof:
        prof_ord = sorted(prof)
        mediana = prof_ord[len(prof_ord) // 2]
        fondo = max(prof)
        print("   altura: %d muestras · hondo mediana=%.2f max=%.2f · >1.0 m en %d"
              % (len(prof), mediana, fondo, sum(1 for v in prof if v > 1.0)))
        if fondo > 6.0:
            fallos.append("nadando se hundió %.1f m bajo la superficie (objetivo 0.41): el motor le sigue "
                          "metiendo su empuje de agua por encima del pin" % fondo)
        elif mediana > 1.0:
            fallos.append("nadando se mantiene %.2f m bajo la superficie de media (objetivo 0.41): el pin no "
                          "manda en el eje vertical" % mediana)

    # 1.6 · QUIÉN MANDA: nadando, el estado de nado es el dueño del ped. Si
    # `bIsStanding` sigue a 1, el motor conserva la vía de andar y el pin no
    # manda (fue lo que hundía al ped junto a las rocas); si `bIsDucking` o
    # `m_pPointGunAt` siguen activos, hay agachado o retícula en el agua. Los
    # campos se buscan POR NOMBRE, así que añadir campos no rompe el parseo.
    quien = [(int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)),
              float(m.group(5)))
             for l in lineas
             for m in [re.search(r"SWIM2 move .*? pie=(-?\d+) col=(-?\d+) duke=(-?\d+) "
                                 r"obj=(-?\d+) suelo=([-\d.]+)", l)]
             if m]   # (pie, col, duke, obj, suelo)
    if quien:
        de_pie = sum(1 for pie, _, _, _, _ in quien if pie == 1)
        colision = sum(1 for _, col, _, _, _ in quien if col == 1)
        agachado = sum(1 for _, _, du, _, _ in quien if du == 1)
        objetivo = sum(1 for _, _, _, ob, _ in quien if ob == 1)
        suelos = [su for _, _, _, _, su in quien if su >= 0.0]
        print("   mando: %d muestras · de pie %d · colisión %d · agachado %d · objetivo %d"
              % (len(quien), de_pie, colision, agachado, objetivo))
        if suelos:
            print("   suelo: suelo a %.1f m de media, %d de %d muestras con suelo cerca (<2 m)"
                  % (sum(suelos) / len(suelos), sum(1 for v in suelos if v < 2.0), len(suelos)))
        if de_pie:
            fallos.append("nadando el motor todavía le %s de PIE en %d de %d muestras: la vía de andar le "
                          "gana al pin (por eso bajaba pegado al fondo)"
                          % ("tiene", de_pie, len(quien)))
        if agachado:
            fallos.append("nadando con el AGACHADO puesto en %d de %d muestras" % (agachado, len(quien)))
        if objetivo:
            fallos.append("nadando con OBJETIVO fijado en %d de %d muestras: sale la retícula en el agua"
                          % (objetivo, len(quien)))
    else:
        print("   mando: sin `pie=` en las trazas de nado (motor sin la 1.6)")

    # 3.0 · ÁRBITRO: una pieza del ped, un dueño. `PEDCLAIM` sale en cada cambio
    # de dueño y en cada reclamación rechazada. En esta fase solo el nado
    # reclamationa, así que cualquier carril distinto o cualquier `ok=0` es la
    # clase de fallo (dos carriles turnos la misma pieza) haciéndose visible en el log.
    # `cap` 0=movimiento 1=postura 2=apuntar 3=camara 4=arma
    # `carril` 0=motor 1=nado 2=agachado del mod 3=ley de apuntado
    reclam = [(int(m.group(1)), int(m.group(2)), int(m.group(3)),
               int(m.group(4)), int(m.group(5)), ts(l))
              for l in lineas
              for m in [re.search(r"PEDCLAIM cap=(\d+) carril=(\d+) de=(\d+) a=(\d+) ok=(\d)", l)]
              if m]   # (cap, carril, de, a, ok, t)
    if reclam:
        caps = {0: "movimiento", 1: "postura", 2: "apuntar", 3: "camara", 4: "arma"}
        tomas = {}
        sueltas = {}
        for cap, carril, de, a, ok, _odt in reclam:
            if ok == 1:
                if a == 1:
                    tomas[cap] = tomas.get(cap, 0) + 1
                if de == 1 and a == 0:
                    sueltas[cap] = sueltas.get(cap, 0) + 1
        netas = {c: tomas.get(c, 0) - sueltas.get(c, 0) for c in caps}
        piezas = sorted(caps[c] for c in caps if netas[c] != 0)
        suelta = sum(sueltas.values())
        toma = sum(tomas.values())
        malas = [r for r in reclam if r[4] == 0]
        otros = [r for r in reclam if r[4] == 1 and r[1] != 1]
        print("   arbitro: %d cambios · el nado toma %d y suelta %d"
              % (len(reclam), toma, suelta))
        print("            en manos del nado ahora: %s" % (", ".join(piezas) or "ninguna"))
        if malas:
            fallos.append("el arbitro rechazó %d reclamación(es) (`ok=0`): dos carriles intentan tomar "
                          "la misma pieza del ped a la vez" % len(malas))
        if otros:
            fallos.append("el arbitro registró %d reclamación(es) de un carril que no es el nado "
                          "(solo el nado reclama en la 3.0): %s"
                          % (len(otros), ", ".join(str(r[1]) for r in otros[:4])))
        ultimo = ""
        for l in lineas:
            if "swim enter" in l:
                ultimo = "entra"
            elif "swim exit" in l:
                ultimo = "sale"
        esperado = 4 if ultimo == "entra" else 0
        lo_que_queda = sum(abs(v) for v in netas.values() if v > 0)
        print("            invariante: %d pieza(s) en turno · se esperaban %d (último evento: %s)"
              % (lo_que_queda, esperado, ultimo or "no hubo nado"))
        if lo_que_queda != esperado:
            fallos.append("el arbitro tiene %d pieza(s) en turno y esperaba %d (último evento de nado: "
                          "%s): el dueño se quedó pegado o se liberó de más"
                          % (lo_que_queda, esperado, ultimo or "ninguno"))
    else:
        print("   arbitro: sin `PEDCLAIM` en el log (motor sin la 3.0 o nunca hubo nado)")

    # 1.7 · LOS DOS ADOPTANTES DEL 3.0, MEDIDOS. `odcrouch` es el agachado del
    # mod (el que el jugador ve; `duke` es el del motor y va a 0 porque
    # VICEEXT_ENGINE_DUCK_KEY() vale false cuando VICEEXT_CROUCH está activo).
    # `SWIMHUD` solo se emite si la puerta del HUD dejó pasar el bloque de la
    # retícula con el nado encima, así que su presencia ES el fallo.
    if quien:
        crouch_mal = sum(1 for l in lineas
                         for m in [re.search(r"SWIM2 move .*? odcrouch=(\d)", l)]
                         if m and m.group(1) == "1")
        if crouch_mal:
            fallos.append("nadando con el AGACHADO DEL MOD puesto en %d de %d muestras: la puerta de "
                          "postura del 3.0 no lo está sacando" % (crouch_mal, len(quien)))
        print("   odcrouch: %d de %d muestras con el agachado del mod puesto en el agua"
              % (crouch_mal, len(quien)))
    # 1.9 · LAS DOS CONDICIONES DEL HUD SON COMPLEMENTARIAS. La traza sale
    # cuando hay retícula que dibujar Y el nado tiene el apuntado; el dibujo
    # ocurre cuando hay retícula Y el nado NO lo tiene. No pueden cumplirse a
    # la vez, así que una línea `SWIMHUD` es el VETO FUNCIONANDO. La 1.8 lo
    # leía al revés y daba un falso positivo: «la puerta dejó dibujar la
    # retícula 50 veces». El criterio de la 1.8 que comparaba «glide>0 con cero
    # SWIMHUD» también se va: con la traza honesta, cero líneas quiere decir
    # que el motor no pidió retícula, que es lo normal.
    # Lo que se dictamina es de quién es la pieza: en cada muestra de nado, el
    # nado tiene que tener `PEDCAP_APUNTAR` (cap 2) tomado.
    import datetime as _odt
    hud_veto = [l for l in lineas if re.match(r"^\S+ +SWIMHUD ", l)]
    if quien:
        if hud_veto:
            print("   retícula: el veto se activó %d vez/veces con el nado encima (el motor "
                  "pidió retícula y la puerta la frenó; su presencia es buena noticia)"
                  % len(hud_veto))
        else:
            print("   retícula: 0 líneas `SWIMHUD`: el motor no pidió retícula en el agua, "
                  "así que la puerta no llegó a tener que frenarla")
    elif hud_veto:
        print("   retícula: %d líneas `SWIMHUD` sin muestras de nado (no se puede dictaminar)"
              % len(hud_veto))

    muestras_od = [(ts(l), l) for l in lineas if " SWIM2 move" in l]
    eventos = sorted((r[5], r[3]) for r in reclam
                     if r[0] == 2 and r[4] == 1 and r[5])
    if muestras_od and eventos:
        def _od_t(x):
            try:
                return _odt.datetime.fromisoformat(x.replace("Z", "+00:00"))
            except ValueError:
                return None
        sueltas, n_od = [], 0
        for xt, _l in muestras_od:
            momento = _od_t(xt) if xt else None
            if momento is None:
                continue
            n_od += 1
            dueno = 0
            for te, a in eventos:
                t_ev = _od_t(te)
                if t_ev is not None and t_ev <= momento:
                    dueno = a
            if dueno != 1:
                sueltas.append(xt)
        if sueltas:
            fallos.append("el nado NO tenía el APUNTADO en %d de %d muestras de nado: la retícula "
                          "del HUD podía dibujarse en el agua" % (len(sueltas), n_od))
        else:
            print("            el nado tuvo el APUNTADO en las %d muestras con marca de tiempo "
                  "(nunca se soltó durante el nado)" % n_od)

    # 1.10 - BRAZADA, SONIDO POR BRAZO Y ZOOM. Tres cosas se juzgan aqui.
    #
    # (a) El avance medido tiene que estar en m/s. El fallo de la 1.9 fue medir
    #     con GetTimeStep() en vez de GetTimeStepInSeconds(), o sea metros por
    #     frame: salia 1/50 de lo real y el ritmo del clip se iba a 0,04. Si la
    #     mediana con LSHIFT vuelve a salir por debajo de 0,5, el error ha vuelto.
    # (b) Los ids del par de sonido tienen que alternar: una brazada son dos
    #     brazos, y el par lo elige el LSHIFT. `sonido=` lo publica la traza.
    #     Par normal = 204/205 (SOUND_NADO_BRAZO_A/B), par LSHIFT = 206/207.
    # (c) El zoom se juzga por las muestras DE FLANCO de `SWIMFOV`, y con la
    #     camdist al lado del FOV, que es lo que faltaba.
    PAR_N = (204, 205)
    PAR_LS = (206, 207)
    braz = [l for l in lineas if re.match(r"^\S+ +SWIMEF ", l)]
    if quien:
        if not braz:
            print("   brazada: 0 lineas `SWIMEF`: no hay splash ni sonido por brazada")
        else:
            medir = [float(m.group(1)) for l in braz
                     for m in [re.search(r"SWIMEF .*? medido=([-\d.]+)", l)] if m]
            if medir:
                print("   brazada: %d con splash y sonido - avance medido mediana=%.2f m/s (n=%d)"
                      % (len(braz), _mediana(medir), len(medir)))
                if len(medir) >= 6 and _mediana(medir) < 0.3:
                    fallos.append("%d brazadas con el avance medido a %.2f m/s: la brazada suena con "
                                  "el ped parado" % (len(medir), _mediana(medir)))
            else:
                print("   brazada: %d lineas `SWIMEF` sin `medido=` (build vieja)" % len(braz))

            sprint_med = [float(m.group(1)) for l in braz
                          for m in [re.search(r"medido=([-\d.]+)", l)]
                          for s in [re.search(r"sprint=(\d+)", l)] if m and s and s.group(1) == "1"]
            if sprint_med:
                mm = _mediana(sprint_med)
                print("            avance medido con LSHIFT: mediana=%.2f m/s (n=%d; se esperan 4,5-4,9)"
                      % (mm, len(sprint_med)))
                if mm < 0.5:
                    fallos.append("con LSHIFT el avance medido sale a %.2f m/s: se esta midiendo en "
                                  "METROS POR FRAME otra vez (se divide por GetTimeStep() en vez de "
                                  "GetTimeStepInSeconds())" % mm)

            sonid = []
            for l in braz:
                ms = re.search(r"sprint=(\d+)", l)
                mi = re.search(r"sonido=(\d+)", l)
                if ms and mi:
                    sonid.append((int(mi.group(1)), int(ms.group(1))))
            if sonid:
                for nombre, par, sprint in (("normal", PAR_N, 0), ("LSHIFT", PAR_LS, 1)):
                    xs = [s for s, sp in sonid if sp == sprint and s in par]
                    fuera = [s for s, sp in sonid if sp == sprint and s not in par]
                    if len(xs) >= 4:
                        repetidas = sum(1 for a, c in zip(xs, xs[1:]) if a == c)
                        print("   sonido par %s: %d brazadas - %d veces seguidas el mismo brazo"
                              % (nombre, len(xs), repetidas))
                        if repetidas == 0:
                            print("            alterna bien (uno por brazo)")
                        elif repetidas > len(xs) // 3:
                            fallos.append("el par de sonido %s no alterna: %d de %d brazadas repiten el "
                                          "mismo brazo y tiene que ser uno por brazo"
                                          % (nombre, repetidas, len(xs)))
                    if fuera and sprint == 0:
                        fallos.append("%d brazadas con el par de LSHIFT en nado normal: el par no "
                                      "cambia con el LSHIFT" % len(fuera))
    elif braz:
        print("   brazada: %d lineas `SWIMEF` sin muestras de nado (no se puede dictaminar)"
              % len(braz))

    fovs = []
    for l in lineas:
        m = re.search(r"SWIMFOV fov=([-\d.]+) aim=(\d) aplica=(\d) modo=(\d+) "
                      r"camdist=([-\d.]+) paso=(\d+)", l)
        if m:
            fovs.append((float(m.group(1)), int(m.group(2)), int(m.group(3)),
                         int(m.group(4)), float(m.group(5)), int(m.group(6))))
    if fovs:
        pasos = sorted(set(f[-1] for f in fovs))
        print("   zoom: %d muestras de flanco (pasos %s; 0 = justo al pulsar)"
              % (len(fovs), pasos))
        f_aim = [f[0] for f in fovs if f[1] == 1]
        f_sin = [f[0] for f in fovs if f[1] == 0]
        d_aim = [f[4] for f in fovs if f[1] == 1]
        d_sin = [f[4] for f in fovs if f[1] == 0]
        if f_aim and f_sin:
            print("            FOV: sin apuntar min=%.2f - apuntando min=%.2f"
                  % (min(f_sin), min(f_aim)))
            if min(f_sin) - min(f_aim) > 5.0:
                fallos.append("apuntando en el agua el FOV baja de %.2f a %.2f (%.0f grados): el zoom "
                              "se cuela aunque el nado tenga el apuntado y la camara"
                              % (min(f_sin), min(f_aim), min(f_sin) - min(f_aim)))
        if d_aim and d_sin:
            print("            camdist: sin apuntar min=%.2f - apuntando min=%.2f"
                  % (min(d_sin), min(d_aim)))
            if min(d_sin) - min(d_aim) > 0.8:
                fallos.append("apuntando en el agua la camara se acerca %.2f m (de %.2f a %.2f): lo que "
                              "se ve NO es un estrechamiento de FOV sino que la camara se acerca"
                              % (min(d_sin) - min(d_aim), min(d_sin), min(d_aim)))
    elif quien:
        print("   zoom: sin `SWIMFOV` (build sin la 1.9/1.10)")

    if fallos:
        return "FALLO", "; ".join(fallos)
    return "OK", ("agachado se anda (avance máx=%.2f m/s) y nadando la cámara sigue al ped (modo 4)"
                  % max([av for ts, av, _, _ in crouch_av if crouch_palo.get(ts, 0.0) > 0.0] or [0.0]))


def bloque_r19(lineas):
    """R19 · la velocidad agachado/nadando es LA DEL CLIP, no la mitad (o el doble).

    R17 comprobó que el ped se MUEVE y que la cámara lo sigue. Faltaba comprobar
    que se mueve A LA VELOCIDAD CORRECTA, y ahí había un fallo de MEDIDA que daba
    un FALSO fallo (22/09): la función del agachado se llama dos veces por frame
    (el clip se pide también desde `SetRealMoveAnim`), así que el tiempo de mundo
    se sumaba dos veces y `velo = avance / dt` salía a la mitad (0,44 m/s de un
    andar de 0,9). Un jugador no lo nota (el ped se mueve bien), pero la sonda
    cantaba un fallo que no existía.

    Evidencia (traza `PEDAT` en cualquier estado + `CROUCH2`/`SWIM2`):
      - `dt` (segundos de mundo entre dos líneas de agachado) aprox.  1 s por segundo
        de reloj del motor (`ms`). Si sale aprox.  2, el tiempo se cuenta doble.
      - `CROUCH2 ... avance=` (metros/segundo de reloj) y `velo=` (metros/segundo
        de mundo) tienen que COINCIDIR: son la misma magnitud medida de dos
        formas. Si uno es la mitad del otro, hay doble conteo.
      - agachado andando: `velo` aprox.  0,9 m/s (el clip escalado, ver R17). Antes de
        R19 con el clip mal escalado salía 0,4-0,5.
      - nadando: `velo` aprox.  2,32 (braza) / 2,78 (crol), las velocidades naturales
        de los clips del mod (defines de nado).
      - `PEDAT ... ts=` (segundos de mundo acumulados por frame) frente a `ms/1000`
        (reloj): aprox.  1,00 en cualquier estado = la simulación va a tiempo real.
    """
    print("\n== R19 · velocidad agachado/nadando: la del clip, medida sin doble conteo ==")
    crouch = [(float(m.group(1)), float(m.group(2)), float(m.group(3)))
              for l in lineas
              for m in [re.search(r"CROUCH2 h=[\d.-]+ avance=([\d.-]+) velo=([\d.-]+) dt=([\d.-]+)", l)]
              if m]   # (avance, velo, dt)
    swim = [(float(m.group(1)), float(m.group(2)), float(m.group(3)))
            for l in lineas
            for m in [re.search(r"SWIM2 move spd=[\d.-]+ avance=([\d.-]+) velo=([\d.-]+) dt=([\d.-]+)", l)]
            if m]   # (avance, velo, dt)
    pedat = [(float(m.group(1)), float(m.group(2)), float(m.group(3)), float(m.group(4)))
             for l in lineas
             for m in [re.search(r"PEDAT .* av=([\d.-]+) velo=([\d.-]+) ms=(\d+) nf=\d+ ts=([\d.-]+)", l)]
             if m]   # (av, velo, ms, ts)
    if not (crouch or swim or pedat):
        return "SIN DATOS", "no se anduvo agachado ni se nadó en esta sesión"
    fallos = []

    # 1) El reloj de mundo no se cuenta dos veces.
    dts = [dt for _, _, dt in crouch] + [dt for _, _, dt in swim]
    if dts:
        med_dt = _mediana(dts)
        print("   reloj de mundo: dt mediana=%.2f s por segundo de reloj (%d muestra(s))" % (med_dt, len(dts)))
        if med_dt > 1.35:
            fallos.append("el tiempo de mundo se cuenta DOBLE agachado (dt mediana=%.2f): la velocidad "
                          "medida sale a la mitad" % med_dt)
    if pedat:
        ratios = [ts / (ms / 1000.0) for av, velo, ms, ts in pedat if ms > 500]
        if ratios:
            r = _mediana(ratios)
            print("   PEDAT: mundo/reloj mediana=%.2f en %d muestra(s) (1,00 = tiempo real)" % (r, len(ratios)))
            if r < 0.8 or r > 1.25:
                fallos.append("la simulación no va a tiempo real (mundo/reloj=%.2f): las velocidades "
                              "medidas no son comparables" % r)

    # 2) Las dos medidas de la misma velocidad coinciden (avance de reloj vs velo de mundo).
    descuadres = [(av, velo) for av, velo, _ in (crouch + swim) if abs(av - velo) > 0.3]
    if descuadres:
        av, velo = descuadres[0]
        fallos.append("%d muestra(s) con las dos medidas descuadradas (p. ej. avance=%.2f vs velo=%.2f): "
                      "alguna de las dos está mal" % (len(descuadres), av, velo))

    # 3) La velocidad es la del clip, en los dos modos.
    av_crouch = [velo for _, velo, _ in crouch if velo > 0.2]   # andando (no quieto)
    if av_crouch:
        med = _mediana(av_crouch)
        print("   agachado andando: velo mediana=%.2f m/s (clip escalado aprox. 0,9; %d muestra(s))"
              % (med, len(av_crouch)))
        if med < 0.5 or med > 1.4:
            fallos.append("andando agachado la velocidad no es la del clip (velo mediana=%.2f m/s, "
                          "se espera aprox. 0,9)" % med)
    # 1.9 - LA VELOCIDAD DE NADO SE JUZGA POR BANDA. El criterio viejo comparaba
    # la mediana contra la naturalidad del clip (2,32-2,78) y era de antes de la
    # curva Serega, cuando el motor no escalaba el ritmo del clip. Ahora el motor
    # escala el ritmo a proposito para ir a `odLevel`, asi que la velocidad medida
    # tiene que seguir a la banda: 0.05 -> 2,5 m/s y 0.12 -> 6,0 m/s. Las bandas
    # 0.022/0.025 son las de parado y no se juzgan. Y la primera muestra tras un
    # cambio de banda se descarta, porque `velo` sale de la diferencia entre dos
    # muestras de 1 Hz y se lleva metida la velocidad de la anterior.
    av_swim = [velo for _, velo, _ in swim if velo > 0.5]
    BANDA_OD = (("0.050", 2.5), ("0.120", 6.0))
    por_banda_od = collections.defaultdict(list)
    banda_previa = None
    for l in lineas:
        m = re.search(r"SWIM2 move .*? velo=([-\d.]+) .*? serega=([\d.]+)", l)
        if not m:
            continue
        banda = m.group(2)
        if banda == banda_previa:
            por_banda_od[banda].append(float(m.group(1)))
        banda_previa = banda
    for banda, esperado in BANDA_OD:
        v = [x for x in por_banda_od.get(banda, []) if x > 0.2]
        if len(v) < 3:
            continue
        med = _mediana(v)
        print("   nadando banda %s: velo mediana=%.2f m/s (se espera ~%.1f; n=%d)"
              % (banda, med, esperado, len(v)))
        if not (0.6 * esperado <= med <= 1.4 * esperado):
            fallos.append("nadando banda %s: velo mediana=%.2f m/s, se espera ~%.1f"
                          % (banda, med, esperado))
    if av_swim:
        print("   nadando: velo mediana=%.2f m/s mezclando bandas (%d muestra(s); solo "
              "informativo, el juicio es por banda)" % (_mediana(av_swim), len(av_swim)))

    if fallos:
        return "FALLO", "; ".join(fallos)
    return "OK", ("velocidad de clip medida sin doble conteo (agachado %.2f m/s, nado %s)"
                  % (_mediana(av_crouch) if av_crouch else 0.0,
                     ("%.2f m/s" % _mediana(av_swim)) if av_swim else "sin datos"))


def _recoil_envueltas(lineas):
    """Líneas RECOIL con correlación (sessionId/eventSeq) y su evento interno."""
    out = []
    for l in lineas:
        # El servidor antepone timestamp: buscar, no anclar al inicio.
        m = re.search(r"RECOIL sessionId=(\S+) eventSeq=(\d+) build=(\S+) data=(\S+) wasm=(\S+) (.*)$", l)
        if m:
            out.append({"sid": m.group(1), "seq": int(m.group(2)), "build": m.group(3),
                        "data": m.group(4), "wasm": m.group(5), "ev": m.group(6), "raw": l})
    return out


def _recoil_campo(ev, campo):
    m = re.search(r"%s=([^\s]+" % campo, ev)
    return m.group(1) if m else None


def bloque_recoil(lineas):
    """RC · recoil por arma y ráfaga (plan recoil-por-arma-y-cadencia).

    Vocabulario propio del plan: PASS / FAIL / INCONCLUSIVE (no el OK/FALLO
    del resto de bloques). Logs de builds viejas (RECOIL2 sin RECOIL_SHOT) o
    capturas incompletas dan INCONCLUSIVE, nunca PASS por ausencia.
    """
    print("\n== RC · recoil por arma y ráfaga ==")
    env = _recoil_envueltas(lineas)
    viejo = sum(1 for l in lineas if "RECOIL2" in l)
    shots = [e for e in env if e["ev"].startswith("RECOIL_SHOT ")]
    if not env:
        if viejo:
            return "INCONCLUSIVE", "log de build vieja (RECOIL2 sin esquema nuevo): juega con el build recoil2 o posterior"
        return "INCONCLUSIVE", "sin líneas RECOIL (build sin esquema RC o traza desactivada)"
    if viejo and not shots:
        return "INCONCLUSIVE", "mezcla de build vieja (RECOIL2) sin RECOIL_SHOT del esquema nuevo"
    sids = sorted(set(e["sid"] for e in env))
    if len(sids) > 1:
        print("   sesiones: %s (se juzga la última)" % ",".join(sids))
    sid = sids[-1]
    ev = [e for e in env if e["sid"] == sid]
    seqs = [e["seq"] for e in ev]
    if any(b - a != 1 for a, b in zip(seqs, seqs[1:])):
        return "INCONCLUSIVE", "hueco en eventSeq de la sesión %s (captura incompleta)" % sid
    diag = [e for e in ev if "RECOIL_DIAG " in e["ev"] and "DIAG_END" not in e["ev"]]
    end = [e for e in ev if "RECOIL_DIAG_END" in e["ev"]]
    if not diag:
        return "INCONCLUSIVE", "sin RECOIL_DIAG (sesión sin cabecera)"
    if not end:
        return "INCONCLUSIVE", "sin RECOIL_DIAG_END: cierra o recarga la pestaña para emitir el cierre y vuelve a pasar el log"
    if any(e["wasm"] == "unavailable" for e in ev):
        return "INCONCLUSIVE", "puente sin librería (wasm=unavailable): juega desde la página con startGame"
    shots = [e for e in ev if e["ev"].startswith("RECOIL_SHOT ")]
    applies = [e for e in ev if e["ev"].startswith("RECOIL_APPLY ")]
    inputs = [e for e in ev if e["ev"].startswith("RECOIL_INPUT ")]
    overflows = [e for e in ev if "QUEUE_OVERFLOW" in e["ev"]]
    if overflows:
        return "INCONCLUSIVE", "RECOIL_QUEUE_OVERFLOW (%d): la cola perdió solicitudes" % len(overflows)
    ap_by_seq = {}
    for e in applies:
        s = _recoil_campo(e["ev"], "shotSeq")
        ap_by_seq.setdefault(s, []).append(e)
    dup = [s for s, v in ap_by_seq.items() if len(v) > 1]
    if dup:
        return "FAIL", "RECOIL_APPLY duplicado para shotSeq=%s" % ",".join(sorted(dup)[:5])
    fallos = []
    inc = []

    def _rad(e, campo):
        try:
            return float(_recoil_campo(e["ev"], campo))
        except (TypeError, ValueError):
            return None

    # Correlación solicitud→aplicación: toda patada no nula, exactamente un APPLY.
    # Un tap con APPLY es válido si una CLASS lo reclasificó a ráfaga (el primer
    # tiro retenido se encola retroactivamente al confirmarse la ráfaga).
    excused = {(_recoil_campo(e["ev"], "shotSeq"))
               for e in ev if e["ev"].startswith("RECOIL_CLASS ") and "decision=burst" in e["ev"]}
    for e in shots:
        s = _recoil_campo(e["ev"], "shotSeq")
        req = _rad(e, "requestedRad")
        naps = len(ap_by_seq.get(s, []))
        if req is not None and req > 0 and naps != 1:
            fallos.append("shotSeq=%s pidió %.4f rad y tiene %d APPLY" % (s, req, naps))
        if (req is not None and req == 0) and naps != 0 and s not in excused:
            fallos.append("shotSeq=%s suprimido (0 rad) y tiene APPLY" % s)

    # 1. Colt45: patada pequeña pero aplicada.
    colt = [e for e in shots if "type=Colt45" in e["ev"] and _recoil_campo(e["ev"], "decision") != "tap"]
    if not colt:
        inc.append("criterio 1 (Colt45): sin disparos Colt45 en la captura")
    elif not any((_rad(e, "requestedRad") or 0) > 0 and len(ap_by_seq.get(_recoil_campo(e["ev"], "shotSeq"), [])) == 1 for e in colt):
        fallos.append("criterio 1 (Colt45): ningún tiro con patada aplicada")

    # 2. Python/escopeta: más subida por disparo que Colt45.
    colt_max = max([_rad(e, "requestedRad") or 0 for e in colt] or [0])
    grandes = [e for e in shots if any(k in e["ev"] for k in ("type=Python", "type=Shotgun", "type=Spas12", "type=Stubby", "type=Shotgun2"))]
    if not grandes:
        inc.append("criterio 2 (Python/escopeta): sin disparos de esas armas")
    elif not any((_rad(e, "requestedRad") or 0) > colt_max for e in grandes):
        fallos.append("criterio 2: Python/escopeta no superan a Colt45 (colt=%.4f)" % colt_max)
    perdigon = [e for e in grandes if _recoil_campo(e["ev"], "decision") == "not-SMG"]
    _ = perdigon  # la escopeta da una patada por disparo: un SHOT por cartucho (sin duplicados de applies, ya verificado arriba)

    # 3/4. SMG taps vs ráfaga sostenida.
    taps = [e for e in shots if _recoil_campo(e["ev"], "decision") == "tap"]
    bursts = [e for e in shots if _recoil_campo(e["ev"], "decision") == "burst"]
    presses = [e for e in inputs if "edge=press" in e["ev"]]
    releases = [e for e in inputs if "edge=release" in e["ev"]]
    if not [e for e in shots if "type=Tec9" in e["ev"] or "type=Uzi" in e["ev"] or "type=Ingram" in e["ev"] or "type=Mp5" in e["ev"] or "type=UziOld" in e["ev"]]:
        inc.append("criterios 3-4 (SMG): sin disparos de SMG")
    else:
        if taps and releases:
            if any((_rad(e, "requestedRad") or 0) != 0 for e in taps):
                fallos.append("criterio 3: hay taps SMG con patada no nula")
        else:
            inc.append("criterio 3 (taps): sin taps con release registrado")
        if bursts:
            if not any((_rad(e, "requestedRad") or 0) > 0 and len(ap_by_seq.get(_recoil_campo(e["ev"], "shotSeq"), [])) == 1 for e in bursts):
                fallos.append("criterio 4: ráfaga SMG sin patadas aplicadas")
        else:
            inc.append("criterio 4 (ráfaga): sin ráfaga sostenida (2+ tiros ≤350 ms con gatillo retenido)")
    if presses and any(_recoil_campo(e["ev"], "source") == "unknown" for e in presses):
        inc.append("input con fuente unknown: no se puede clasificar taps/ráfaga con garantías")

    # 5. Persistencia: sin retorno automático. Ecuación de conservación: lo
    # aplicado solo puede salir por compensación manual, clamp o reset
    # estructural. (El autoDelta de PERSIST mezcla el solver pasivo de la
    # cámara y no sirve como medida de retorno.)
    persists = [e for e in ev if e["ev"].startswith("RECOIL_PERSIST ")]
    if not persists:
        inc.append("criterio 5 (persistencia): sin muestras RECOIL_PERSIST")
    else:
        applied = sum(_rad(e, "appliedRad") or 0.0 for e in applies)
        mandown = sum(-min(0.0, _rad(e, "deltaAlphaRad") or 0.0)
                      for e in ev if e["ev"].startswith("RECOIL_CONTROL "))
        clamp_loss = sum(((_rad(e, "requestedRad") or 0.0) - (_rad(e, "appliedRad") or 0.0))
                         for e in applies if "clamp=1" in e["ev"])
        reset_lost = sum(_rad(e, "residualBeforeRad") or 0.0
                         for e in ev if e["ev"].startswith("RECOIL_RESET "))
        finals = [(_rad(e, "residualAfterRad") or 0.0)
                  for e in ev if e["ev"].startswith("RECOIL_APPLY ") or e["ev"].startswith("RECOIL_CONTROL ")]
        final_res = finals[-1] if finals else 0.0
        comp = applied - clamp_loss - reset_lost - final_res
        if comp < -0.01:
            fallos.append("criterio 5: recoil creado de la nada (comp=%.4f)" % comp)
        elif comp > mandown + 0.01:
            fallos.append("criterio 5: %.4f rad se perdieron sin compensación manual (manual=%.4f)" % (comp - mandown, mandown))

    # 6. Cambio de arma: aplica el perfil nuevo sin borrar lo previo.
    changes = [e for e in ev if e["ev"].startswith("RECOIL_WEAPON_CHANGE ")]
    if not changes:
        inc.append("criterio 6 (cambio de arma): sin cambios de arma en la captura")
    else:
        resets = [e for e in ev if e["ev"].startswith("RECOIL_RESET ")]
        if resets and not all("ResetStatics" in e["ev"] for e in resets):
            fallos.append("criterio 6: hay resets que no son ResetStatics estructural")

    # 7. Retícula fija + cámara.
    mira = [float(m.group(1)) for e in env
            for m in [re.search(r"multY=([\d.]+)", e["ev"])] if m]
    if mira and (min(mira) < 0.38 or max(mira) > 0.42):
        fallos.append("criterio 7: la retícula se movió (multY=%s)" % ",".join("%.3f" % v for v in sorted(set(mira))[:5]))
    if not mira:
        inc.append("criterio 7: sin RECOIL3 (multY no observado)")
    if not applies:
        inc.append("criterio 7: sin RECOIL_APPLY (cámara no observada)")

    print("   sesión=%s eventos=%d disparos=%d applies=%d inputs=%d/%d persists=%d" %
          (sid, len(ev), len(shots), len(applies), len(presses), len(releases), len(persists)))
    if fallos:
        return "FAIL", "; ".join(fallos)
    if inc:
        return "INCONCLUSIVE", "; ".join(inc)
    return "PASS", ("sesión %s íntegra (%d eventos, %d disparos correlacionados 1:1, cierre presente)" % (sid, len(ev), len(shots)))


def bloque_r27(lineas):
    """R27 · agachado calibrado: clips del sa-crouch servidos, velocidad y rueda.

    Plan agachado-calibrado (26/09). Vocabulario propio del plan: PASS / FAIL /
    INCONCLUSIVE, nunca PASS por ausencia. La marca `cal=1` la pone el motor
    desde R27: sin ella la build servida es anterior a la calibración (el
    `ped.ifp` con `GunCrouchFwd`/`GunCrouchBwd` y el motor a 3,75 m/s) y este
    bloque no puede juzgar nada.

    Evidencia que se lee aquí:
      VICEEXT crouch move ... medida=1x cal=1 clipr=<m/s>   el clip en curso y la
        raíz DECLARADA con la que se escala su ritmo.
      CROUCH2 ... nom= mps= obju= clipr= pies= velo= mira= gat= rueda= arma= peso=
        entrada=   la muestra de 1/s del agachado: nombre del clip, m/s
        objetivo (`mps`), ritmo real del clip (`pies`), apuntado, rueda, arma
        en mano y ms desde el cambio de clip (ventana del blend 10).
      CROUCHMOVE fin ... mps= maxfr=   el tramo entero al soltar: `maxfr` es el
        mayor desplazamiento de UN frame (un catapultazo se ve por su tamaño).
      VICEEXT crouch roll lado=... motivo=ok      una rueda.
      VICEEXT crouch roll fin peso=... motivo=dura  (R29) fin de rueda con el
        peso REAL con el que entró el clip (>= 0,5: si no, no se ve rodar).
      VICEEXT crouch roll skip motivo=...         por qué NO salió.
      CROUCH2 ... otros= pose= pesoarma= nomarma=   (R28) presupuesto: `otros` es
        lo que ocupan las parciales AJENAS (el clip del arma), `pose` el clip que
        lleva la pose, y `pesoarma`/`nomarma` la parcial del arma más pesada con
        su nombre (`*_crouchfire` = disparo agachado con su clip, `*_fire` = de
        pie, la "lucha entre 2 animaciones").

    Bandas (decisiones D1-D5 del jugador del 27/09; plan
    agachado-correcciones-axis): cuerpo 1,13 m/s ±15 %; `pies` = mps/clipr ±15 %
    SIN multiplicador (R29: la raíz servida se reescaló a ±1,50 m/ciclo; la
    build vieja, clipr≈3,75, llevaba ×1,7 y se acepta como régimen antiguo);
    `maxfr` < 0,05 m; rueda sólo apuntando, lateral SIN arriba/abajo y con arma
    17-27 (fuera las pesadas 28-33), REPETIBLE manteniendo la tecla (D5=(a));
    `roll fin peso=` >= 0,5; `giro=3` apuntando y `giro=0` sin apuntar
    (`giro=5` sólo con |ang|>=130: el atrás puro de D3); pose `WEAPON_crouch`
    al apuntar sin disparar (`gat=0`) y `nomarma=-` (el `*_crouchfire` sólo con
    `gat=1` y el flag, C18-1); y el PRESUPUESTO de parciales (`otros` + `peso`
    <= 1,02).
    """
    print("\n== R27 · agachado calibrado: clips del sa-crouch servidos y rueda ==")

    def campo(l, nombre):
        m = re.search(r"(?:^|\s)%s=(\S+)" % nombre, l)
        return m.group(1) if m else None

    def num(l, nombre):
        v = campo(l, nombre)
        try:
            return float(v)
        except (TypeError, ValueError):
            return None

    def marca(l):
        m = re.match(r"(\S+)", l)
        return m.group(1) if m else ""

    c2 = [l for l in lineas if "CROUCH2 h=" in l]
    move = [l for l in lineas if "VICEEXT crouch move spd=" in l]
    fin = [l for l in lineas if "CROUCHMOVE fin" in l]
    roll = [l for l in lineas if "VICEEXT crouch roll" in l]
    skips = [l for l in roll if " skip " in l]
    ruedas = [l for l in roll if " skip " not in l]
    if not (c2 or fin or roll):
        return "INCONCLUSIVE", "no hay trazas de agachado en la sesión (no se usó)"

    cal = [l for l in c2 if "cal=1" in l]
    if not cal:
        return "INCONCLUSIVE", ("el agachado no lleva la marca `cal=1`: la build servida es "
                                "anterior a la calibración (ped.ifp con los clips del sa-crouch "
                                "y motor R27); no se puede juzgar")

    palo = {}
    for l in move:
        a = num(l, "andando")
        palo[marca(l)] = a if a is not None else 0.0

    muestras = [{"ts": marca(l), "nom": campo(l, "nom"), "mps": num(l, "mps"),
                 "clipr": num(l, "clipr"), "pies": num(l, "pies"), "velo": num(l, "velo"),
                 "mira": campo(l, "mira"), "gat": campo(l, "gat"),
                 "giro": num(l, "giro"), "ang": num(l, "ang"),
                 "rueda": campo(l, "rueda"), "arma": num(l, "arma"),
                 "peso": num(l, "peso"), "entrada": campo(l, "entrada"),
                 "otros": num(l, "otros"), "pose": campo(l, "pose"),
                 "pesoarma": num(l, "pesoarma"), "nomarma": campo(l, "nomarma"),
                 "piesMps": num(l, "piesMps"),
                 "palo": palo.get(marca(l), 0.0)}
                for l in cal]
    caminando = [m for m in muestras if m["palo"] > 0.0]

    fallos, inc = [], []
    print("   muestras CROUCH2 con `cal=1`: %d (andando: %d); CROUCHMOVE fin: %d; "
          "ruedas: %d; avisos de motivo: %d"
          % (len(muestras), len(caminando), len(fin), len(ruedas), len(skips)))

    # 1. La línea no se corta (el buffer ya se quedó corto una vez, R23).
    if [m for m in muestras if m["entrada"] is None]:
        fallos.append("hay CROUCH2 sin `entrada=`: la línea se está CORTANDO (sube el buffer)")
    entradas = [float(m["entrada"]) for m in muestras if m["entrada"] is not None]
    if entradas:
        print("   entrada (ms desde el cambio de clip): %d muestra(s), mín=%.0f, máx=%.0f"
              % (len(entradas), min(entradas), max(entradas)))

    # 2. Los clips que pide el motor: los del mod, no los de VC.
    viejos = sorted({m["nom"] for m in caminando if m["nom"] in ("Crouch_Forward", "Crouch_Backward")})
    if viejos:
        fallos.append("andando agachado el motor sigue pidiendo los clips viejos (%s): el "
                      "`ped.ifp` servido no lleva los del sa-crouch" % ", ".join(viejos))
    else:
        nombres = sorted({m["nom"] for m in caminando if m["nom"]})
        print("   clips pedidos andando: %s" % (", ".join(nombres) or "sin muestras andando"))
        raros = [n for n in nombres if n not in ("GunCrouchFwd", "GunCrouchBwd",
                                                "Crouch_Roll_L", "Crouch_Roll_R")]
        if raros:
            fallos.append("andando agachado sale un clip que no es del carril: %s"
                          % ", ".join(raros))

    # 3. Velocidad objetivo (la del código) y medida (la real).
    mps = [m["mps"] for m in caminando if m["mps"] is not None]
    if mps:
        fuera = [v for v in mps if not (0.9605 <= v <= 1.2995)]
        print("   velocidad objetivo (`mps`): mediana=%.2f en %d muestra(s); fuera de 1,13±15%%: %d"
              % (_mediana(mps), len(mps), len(fuera)))
        if fuera:
            fallos.append("`mps` fuera de 1,13 m/s ±15 %% en %d muestra(s)" % len(fuera))
    else:
        inc.append("sin muestras andando: no se puede medir la velocidad del agachado")
    velo = [m["velo"] for m in caminando if m["velo"] is not None and m["velo"] > 0.2]
    if velo:
        med = _mediana(velo)
        print("   velocidad medida (`velo`): mediana=%.2f m/s en %d muestra(s) andando"
              % (med, len(velo)))
        if med < 0.9605 or med > 1.2995:
            fallos.append("andando agachado la velocidad medida no es 1,13±15 %% (velo mediana=%.2f)"
                          % med)

    # 4. Los pies: el ritmo real del clip contra el que le toca por su raíz.
    # R29 (D1=(a)): SIN multiplicador — la raíz servida se reescaló
    # (clipr≈2,05 = 1,50 m / 0,731 s). La build vieja (clipr≈3,75) llevaba el
    # ×1,7 de R26c: se sigue aceptando como régimen antiguo para no falsear los
    # logs de partidas previas.
    pies_mal = []
    regimenes = set()
    for m in caminando:
        if m["pies"] is None or m["pies"] <= 0 or not m["clipr"] or m["clipr"] <= 0 \
                or m["mps"] is None:
            continue
        esperado = m["mps"] / m["clipr"]
        if m["clipr"] > 3.0:
            esperado *= 1.7   # régimen R26c/R28 (raíz de 3,75 m/s)
            regimenes.add("x1,7 (build vieja)")
        else:
            regimenes.add("sin multiplicador (R29)")
        if abs(m["pies"] - esperado) > 0.15 * esperado:
            pies_mal.append((m["nom"], m["pies"], esperado))
    if pies_mal:
        print("   pies: %d muestra(s) mal de %d andando" % (len(pies_mal), len(caminando)))
        nom, real, esp = pies_mal[0]
        fallos.append("%d muestra(s) con `pies` fuera de su raíz (p. ej. %s: %.2f, se espera %.2f)"
                      % (len(pies_mal), nom, real, esp))
    elif caminando:
        print("   pies: OK en %d muestra(s) andando (`pies` = mps/clipr; régimen: %s)"
              % (len(caminando), ", ".join(sorted(regimenes)) or "sin muestras"))

    v44, m44 = p4_9(lineas)
    if v44 == "FALLO":
        fallos.append("%s (plan 4.4)" % m44)
    elif v44 in ("SIN DATOS", "PARCIAL"):
        inc.append("plan 4.4: %s" % m44)

    # 5. El tramo: sin catapultazos.
    if fin:
        # El tramo corto (medio arranque, un resbalón al soltar) no sirve para
        # juzgar el paso por frame: sólo se miran los tramos de 1 s o más.
        largos = [l for l in fin if (num(l, "t") or 0.0) >= 1.0]
        maxfr = [v for v in (num(l, "maxfr") for l in largos) if v is not None]
        malos = [v for v in maxfr if v >= 0.05]
        print("   CROUCHMOVE fin: %d tramo(s) (%d de 1 s o más), maxfr máx=%.3f m"
              % (len(fin), len(largos), max(maxfr) if maxfr else 0.0))
        for l in fin:
            if (num(l, "t") or 0.0) < 1.0:
                print("      (tramo corto de %.2f s, no se juzga: maxfr=%.3f)"
                      % (num(l, "t") or 0.0, num(l, "maxfr") or 0.0))
        if malos:
            fallos.append("%d tramo(s) con `maxfr` >= 0,05 m (tirón o teletransporte): máx=%.3f"
                          % (len(malos), max(malos)))
    else:
        inc.append("sin `CROUCHMOVE fin`: no se midió ningún tramo andando agachado")

    # 5b. R28: el PRESUPUESTO de parciales. `otros` es lo que ocupan las
    # animaciones parciales ajenas (el clip del arma) y `peso` lo que ocupa la
    # pose del mod. Si suman más de 1 el motor reparte pesos negativos a las de
    # movimiento, la suma de cuaterniones puede quedar casi nula y el cuerpo sale
    # deformado: es la captura del jugador del 22/09.
    presup = [(m["otros"], m["peso"]) for m in muestras
              if m["otros"] is not None and m["peso"] is not None]
    if presup:
        sumas = [t[0] + t[1] for t in presup]
        print("   presupuesto de parciales (`otros` + `peso`): máx=%.2f en %d muestra(s)"
              % (max(sumas), len(presup)))
        malos = [v for v in sumas if v > 1.02]
        if malos:
            fallos.append("%d muestra(s) con `otros` + `peso` > 1 (hasta %.2f): dos poses parciales "
                          "a la vez deforman el cuerpo" % (len(malos), max(malos)))
    else:
        fallos.append("las muestras no traen `otros`/`peso`: la build servida es anterior a R28 "
                      "(presupuesto de parciales)")

    # 5c. R28: disparo agachado con el clip del ARMA de agachado. Informativo
    # (sólo falla si la parcial del arma va a peso alto y NINGUNO de sus nombres
    # es de agachado: ahí el motor está usando el clip de pie).
    tiros = [m for m in muestras if (m["pesoarma"] or 0.0) > 0.5 and m["nomarma"]]
    if tiros:
        nombres = collections.Counter(m["nomarma"] for m in tiros)
        print("   parcial del arma a peso alto: %s"
              % ", ".join("%s×%d" % kv for kv in nombres.most_common(6)))
        if not any(re.search(r"crouch", n) for n in nombres):
            # R29 (costura C18-1/R28): sin `WEAPONFLAG_CROUCHFIRE` el arma
            # dispara DE PIE a propósito (C18-1 literal, decisión del 27/09),
            # así que esto es informativo: lo estricto es `gat=` con
            # `nomarma`/`pose` (bloque 5e).
            print("   AVISO: el arma usa el clip de pie (%s): correcto si no trae "
                  "`WEAPONFLAG_CROUCHFIRE` (C18-1); sería fallo si lo trae"
                  % ", ".join(sorted(nombres)))
    else:
        print("   parcial del arma: sin muestras a peso alto (no se apuntó ni disparó agachado)")

    # 5d. R29: el GIRO. `giro=3` apuntando (el encaramiento lo manda el axis),
    # `giro=0` girando hacia donde se camina (las 8 direcciones) y `giro=5`
    # sólo con el atrás puro (|ang| >= 130, decisión D3=(a)). Cualquier otra
    # combinación es el síntoma F ("no gira hacia donde camina").
    giros = [m for m in muestras if m["giro"] is not None]
    if giros:
        cuenta = collections.Counter(int(m["giro"]) for m in giros)
        print("   giro: %s" % ", ".join("giro=%d×%d" % kv for kv in sorted(cuenta.items())))
        malos = []
        for m in giros:
            g = int(m["giro"])
            ang = abs(m["ang"] or 0.0)
            if m["mira"] == "1" and g == 0:
                malos.append("giro=0 apuntando (debe ser 3: el axis manda el encaramiento)")
            elif m["mira"] != "1" and g == 3:
                malos.append("giro=3 sin apuntar")
            elif g == 5 and ang < 130.0:
                malos.append("giro=5 (no-giro) con ang=%.0f: el corte es el atrás puro (>=130)" % ang)
        if malos:
            fallos.append("giro inconsistente en %d muestra(s): %s" % (len(malos), malos[0]))
        vueltas = {(int(abs(m["ang"] or 0.0)) // 45) for m in giros
                   if m["mira"] != "1" and int(m["giro"]) == 0}
        if len(vueltas) < 3:
            inc.append("el giro sólo se vio en %d sector(es) de 45º: para el PASS hacen falta "
                       "varias direcciones sin apuntar (las 8 del plan)" % len(vueltas))
    else:
        inc.append("sin `giro=` en las muestras: la build servida es anterior a R26b")

    # 5e. R29: la POSE de apuntado (`WEAPON_crouch`) y el clip del arma. Sin
    # disparar (`gat=0`) la pose manda y NO puede haber parcial del arma encima
    # (era el `colt45_crouchfire` sostenido del síntoma D); con gatillo
    # (`gat=1`) el `*_crouchfire` es correcto (sólo con el flag, C18-1).
    poses = [m for m in muestras if m["mira"] == "1" and m["gat"] == "0" and m["rueda"] != "1"]
    if any(m["gat"] is None for m in muestras):
        fallos.append("hay CROUCH2 sin `gat=`: la línea se está CORTANDO o la build es "
                      "anterior a R29")
    if poses:
        sin_pose = [m for m in poses if not m["pose"] or m["pose"] == "-"]
        con_arma = [m for m in poses if m["nomarma"] and m["nomarma"] != "-"]
        print("   pose al apuntar sin disparar: %d muestra(s), con `WEAPON_crouch`: %d, "
              "con parcial del arma: %d"
              % (len(poses), len(poses) - len(sin_pose), len(con_arma)))
        if sin_pose:
            fallos.append("%d muestra(s) apuntando sin disparar y SIN pose `WEAPON_crouch`"
                          % len(sin_pose))
        if con_arma:
            fallos.append("%d muestra(s) apuntando sin disparar con el clip del arma encima "
                          "(p. ej. %s): el `*_crouchfire` es sólo con gatillo"
                          % (len(con_arma), con_arma[0]["nomarma"]))
    else:
        inc.append("sin muestras apuntando sin disparar: no se puede juzgar la pose")

    # 5f. R29: la rueda REPETIBLE y su peso real (`roll fin peso=`).
    fins = [l for l in roll if " roll fin " in l]
    if fins:
        pf = [v for v in (num(l, "peso") for l in fins) if v is not None]
        print("   `roll fin peso=`: %d evento(s), peso máx=%.2f"
              % (len(fins), max(pf) if pf else 0.0))
        if pf and max(pf) < 0.5:
            fallos.append("la rueda no entra con peso alto (máx %.2f < 0,5): las parciales "
                          "la aplastan (RollClearPartials)" % max(pf))
    elif ruedas:
        inc.append("sin `roll fin peso=`: la build servida es anterior a R29 (peso de la rueda)")

    def _ms(s):
        mm = re.match(r"(\d\d):(\d\d):(\d\d)\.(\d+)", s)
        if not mm:
            return None
        h, mn, sc, ms = (int(x) for x in mm.groups())
        return ((h * 60 + mn) * 60 + sc) * 1000 + ms // 1000

    ok = [l for l in ruedas if "motivo=ok" in l]
    if len(ok) >= 2:
        encadenadas = 0
        pares = 0
        for a, b in zip(ok, ok[1:]):
            ta, tb = _ms(marca(a)), _ms(marca(b))
            if ta is None or tb is None:
                continue
            pares += 1
            if 0 <= (tb - ta) % 86400000 < 3000:
                encadenadas += 1
        print("   ruedas encadenadas (<3 s entre `motivo=ok`): %d de %d par(es) con reloj"
              % (encadenadas, pares))
        if pares and encadenadas == 0:
            inc.append("no se vio la rueda repetirse manteniendo la tecla (D5=(a)): "
                       "¿se soltó entre ruedas?")
    elif ok:
        inc.append("sólo 1 rueda `motivo=ok`: no se puede juzgar la repetición (D5=(a))")

    # 6. Peso del clip de agachado.
    pesos = [m["peso"] for m in muestras if m["peso"] is not None]
    if pesos:
        print("   peso del clip agachado: máx=%.2f en %d muestra(s)" % (max(pesos), len(pesos)))
        if max(pesos) < 0.5:
            fallos.append("el clip de agachado no llega a peso 0,5 (máx=%.2f): otro clip compite "
                          "por el cuerpo" % max(pesos))

    # 7. La rueda: sólo apuntando, lateral sin arriba/abajo y con arma 17-27,
    #    REPETIBLE manteniendo la tecla (D5=(a)), con motivo.
    razones = collections.OrderedDict()
    for l in skips:
        m = re.search(r"motivo=(\S+)", l)
        if m:
            razones[m.group(1)] = razones.get(m.group(1), 0) + 1
    if razones:
        print("   rueda que NO salió: %s"
              % ", ".join("%s=%d" % (k, v) for k, v in razones.items()))
    if ruedas:
        con_motivo = sum(1 for l in ruedas if "motivo=ok" in l)
        print("   ruedas que salieron: %d (motivo=ok: %d)" % (len(ruedas), con_motivo))
        if con_motivo == 0:
            fallos.append("hay ruedas sin `motivo=ok`: el motor servido no lleva R27")
        con_rueda = [m for m in muestras if m["rueda"] == "1"]
        mal = [m for m in con_rueda
               if m["mira"] != "1" or m["arma"] is None or not (17 <= m["arma"] <= 27)]
        if mal:
            fallos.append("%d muestra(s) con `rueda=1` sin `mira=1` o con arma fuera de 17-27 "
                          "(p. ej. arma=%s mira=%s)" % (len(mal), mal[0]["arma"], mal[0]["mira"]))
        else:
            print("   `rueda=1` sólo con mira y arma 17-27 (%d muestra(s))" % len(con_rueda))
        if len(con_rueda) > 2 * max(1, len(ruedas)):
            fallos.append("hay %d muestras con la rueda puesta para %d rueda(s): se queda rodando"
                          % (len(con_rueda), len(ruedas)))
    elif razones:
        fallos.append("la rueda NO salió en toda la sesión (motivos: %s)"
                      % ", ".join("%s=%d" % (k, v) for k, v in razones.items()))
    else:
        inc.append("no se intentó la rueda (ni eventos ni avisos de motivo)")

    if fallos:
        return "FAIL", "; ".join(fallos)
    if inc:
        return "INCONCLUSIVE", "; ".join(inc)
    return "PASS", ("agachado con los clips servidos (%.2f m/s de cuerpo, pies a la par), "
                    "presupuesto de parciales <= 1 y rueda con motivo=ok (%d)"
                    % (_mediana(mps) if mps else 0.0, len(ruedas)))



def od_build_del_log(ruta):
    """El build con el que se hizo la sesion, leido de la cabecera del log."""
    try:
        with open(ruta, encoding="utf-8", errors="replace") as f:
            for i, linea in enumerate(f):
                if i > 40:
                    break
                m = re.search(r"JS build=(\S+)", linea)
                if m:
                    return m.group(1)
    except OSError:
        pass
    return None


def od_version_servida():
    """El VERSION actual de web/lib/index.js, que es lo que baja el navegador."""
    aqui = os.path.dirname(os.path.abspath(__file__))
    ruta = os.path.join(aqui, "..", "web", "lib", "index.js")
    try:
        with open(ruta, encoding="utf-8", errors="replace") as f:
            m = re.search(r"VERSION\s*=\s*'([^']*)'", f.read())
        return m.group(1) if m else None
    except OSError:
        return None


def od_comprueba_build(ruta):
    """FALLA si la sesion se hizo con una build que ya no es la servida.

    Sin esto, validar sobre un wasm cacheado sale como 'el arreglo no funciona'
    cuando en realidad no se estaba probando nada. Ocurrio el 02/10/2026.
    """
    del_log = od_build_del_log(ruta)
    servida = od_version_servida()
    if not del_log:
        return True, "sin build tag en la cabecera del log"
    if not servida:
        return True, "no se encuentra web/lib/index.js para comparar"
    if del_log == servida:
        return True, "build del log = build servida (%s)" % del_log
    return False, ("la sesion se hizo con build %s y ahora se sirve %s -> el navegador "
                   "tiene el index.js cacheado y esta probando el wasm viejo. Recarga "
                   "con Ctrl+Shift+R" % (del_log, servida))


def bloque_build(ruta):
    ok, msg = od_comprueba_build(ruta)
    print("== BUILD de la sesion ==")
    print(("B  OK        " if ok else "B  FALLO     ") + msg)
    return ok



P4_KINDS = ("move_begin", "move_end", "pose_begin", "pose_end", "input_edge",
            "input_hold_end", "roll_start", "roll_end", "roll_cancel",
            "cam_begin", "cam_first", "cam_end", "cam_cancel", "service",
            "identity", "overflow",
            "legs", "hud_cross", "turners", "polbike",
            "sight", "crouchspeed", "move_apply", "pose_exit",
            "aim", "jump", "fire", "armsight",
            "roll_step", "jump_end", "stand_end", "partials")
P4_REQ = {
    "move_begin": ("lr", "ud", "aim", "policy", "baseF", "baseR", "req"),
    "move_end": ("elapsed", "dist", "req", "baseF", "baseR", "dirErrMax", "bodyErrMax"),
    "pose_begin": ("posture",),
    "pose_end": ("posture", "weights"),
    "input_edge": ("lr", "ud", "aim", "crouch", "side", "edge", "neutral", "eligible"),
    "input_hold_end": ("heldSim",),
    "roll_start": ("side", "edge", "aim", "crouch", "weapon", "req", "clip", "length", "speed"),
    "roll_end": ("side", "clip", "current", "progress", "rateIntegral", "travel", "maxWeight"),
    "roll_cancel": ("side", "cause", "progress"),
    "cam_begin": ("from", "to", "visibleFront"),
    "cam_first": ("visibleFront",),
    "cam_end": ("visibleFront", "fov", "fovTarget"),
    "cam_cancel": ("cause",),
    "service": ("vehicle", "model", "siren", "var", "selectedBars", "visibleLenses",
                "anchorError", "registered"),
    "legs": ("body", "leg", "mis", "skip"),
    "hud_cross": ("draw", "pc", "law", "lock"),
    "turners": ("quien", "motivo", "luces"),
    "polbike": ("model", "occ", "passengers"),
    "sight": ("weapon", "sight", "loaded", "cruz"),
    "crouchspeed": ("mps", "rate", "clip"),
    "move_apply": ("src", "spdReq", "spdEff", "aim", "crouch", "stickBlocked"),
    "pose_exit": ("n", "sample", "w"),
    "aim": ("why", "aim", "sprint", "jump", "policy", "speedGame"),
    "jump": ("lr", "ud", "rumbo", "dirX", "dirY"),
    "fire": ("aim", "gat", "arma", "hip"),
    "armsight": ("pose", "errDeg"),
    "roll_step": ("clip", "current", "step", "total", "mps", "speedTarget"),
    "jump_end": ("cause", "req", "from", "to", "dist", "elapsed", "dirErr"),
    "stand_end": ("aim", "mfs", "t", "m", "mps", "maxfr"),
    "partials": ("aim", "crouch", "desde", "n", "w1", "cgroup"),
}
P4_VECTORES = ("baseF", "baseR", "req", "local", "world", "carPos", "carRot", "visibleFront")
P4_NUM_FINITOS = ("lr", "ud", "aim", "crouch", "walk", "sprint", "elapsed", "dist",
                  "dirErrMax", "bodyErrMax", "posture", "t80", "tClear",
                  "maxOwnAfter500", "boneNonfinite", "boneDegenerate", "ownPoseW",
                  "fadeOthersMax", "boneSamples", "heldSim", "neutral", "eligible",
                  "length", "speed", "current", "progress", "rateIntegral", "simElapsed",
                  "travel", "maxWeight", "releaseSeen", "beta", "alpha", "pitchRequested",
                  "pitchClamped", "fov", "fovTarget", "mouse", "lock", "collision",
                  "transition", "pixelErr", "vehicle", "model", "siren", "var",
                  "selectedBars", "visibleLenses", "damage", "registered", "hasOnDummy",
                  "anchorError", "speedTarget", "speedReal",
                  "body", "leg", "mis", "skip", "draw", "pc", "law", "luces",
                  "steer", "anchorErr", "occ", "passengers", "wanted",
                  "sight", "loaded", "cruz", "px", "py",
                  "mps", "rate", "mfs", "target",
                  "src", "spdReq", "spdEff", "mag", "dir", "frames",
                  "n", "sample", "w", "delta",
                  "jump", "speedGame", "rumbo", "dirX", "dirY",
                  "gat", "arma", "hip", "errDeg", "stickBlocked",
                  "canon", "piesMps", "pesosum",
                  "posepeso", "w1", "w2", "cgroup", "step", "total", "dirErr",
                  "stepM", "stepT")
P4_INICIO = {"move": "move_begin", "pose": "pose_begin", "roll": "roll_start", "cam": "cam_begin"}
P4_FAMILIA = {"move_begin": "move", "move_end": "move", "pose_begin": "pose", "pose_end": "pose",
              "roll_start": "roll", "roll_end": "roll", "roll_cancel": "roll",
              "cam_begin": "cam", "cam_first": "cam", "cam_end": "cam", "cam_cancel": "cam"}
P4_FIN = {"move": "move_end", "pose": "pose_end", "roll": "roll_end", "cam": "cam_end"}
P4_CANCEL = {"roll": "roll_cancel", "cam": "cam_cancel"}
P4_PESOS = ("idle", "fwd", "bwd", "rollL", "rollR", "aimFwd", "aimBwd", "aimLeft", "aimRight")
P4_TRAMO_MIN = 3.0
P4_CAUSAS_CORTE = ("release", "solt", "hold", "manten", "raton", "mouse", "lado", "side")


def _p4_entero(v):
    try:
        return int(v)
    except (TypeError, ValueError):
        return None


def _p4_num(v):
    try:
        f = float(v)
    except (TypeError, ValueError):
        return None
    if f != f or f in (float("inf"), float("-inf")):
        return None
    return f


def p4_vector(v):
    partes = v.split(",")
    if len(partes) != 3:
        return None
    out = [_p4_num(p) for p in partes]
    if None in out:
        return None
    return out


def _p4_dot(a, b):
    na = math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])
    nb = math.sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2])
    if na < 1e-9 or nb < 1e-9:
        return None
    return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (na * nb)


def _p4_angulo(a, b):
    d = _p4_dot(a, b)
    if d is None:
        return None
    return math.degrees(math.acos(max(-1.0, min(1.0, d))))


def _p4_signo(v):
    if v > 0.05:
        return 1
    if v < -0.05:
        return -1
    return 0


def _p4_bucket(f):
    a = math.degrees(math.atan2(f[1], f[0]))
    return int(round(a / 45.0)) % 8


def _p4_horiz(v):
    return (v[0], v[1], 0.0)


def _p4_lado(v):
    s = str(v or "").upper()
    if s in ("L", "A", "LEFT", "IZQ", "IZQUIERDA"):
        return "L"
    if s in ("R", "D", "RIGHT", "DER", "DERECHA"):
        return "R"
    return None


def p4_evento(linea):
    m = re.search(r"\bP4\s+(\S.*)$", linea)
    if not m or "=" not in m.group(1).split()[0]:
        return None, None
    campos = {}
    for par in m.group(1).split():
        if "=" not in par:
            return None, "evento P4 truncado: campo sin '=' (%s)" % par
        k, v = par.split("=", 1)
        if k in campos:
            return None, "evento P4 con clave duplicada (%s)" % k
        campos[k] = v
    if "kind" not in campos:
        return None, None
    return campos, None


def p4_cargar(lineas):
    ev = {"eventos": [], "invalidos": [], "identidad": None, "overflow": [],
          "headers": set(), "gens": set(), "por_id": {}}
    for l in lineas:
        mh = re.search(r"JS build=(\S+)\s+data=(\S+)", l)
        if mh:
            ev["headers"].add((mh.group(1), mh.group(2)))
        campos, err = p4_evento(l)
        if campos is None:
            if err:
                ev["invalidos"].append(err)
            continue
        kind = campos.get("kind")
        if kind not in P4_KINDS:
            ev["invalidos"].append("kind P4 desconocido: %s" % kind)
            continue
        errores = []
        faltan = [k for k in ("schema", "gen", "frame", "sim", "case", "id") if k not in campos]
        if faltan:
            errores.append("%s sin campos comunes (%s)" % (kind, ",".join(faltan)))
        if not errores and campos["schema"] != "1":
            errores.append("%s schema=%s (se exige schema=1)" % (kind, campos["schema"]))
        if not errores:
            gen = _p4_entero(campos["gen"])
            marco = _p4_entero(campos["frame"])
            sim = _p4_num(campos["sim"])
            caso = _p4_entero(campos["case"])
            ident = _p4_entero(campos["id"])
            if None in (gen, marco, sim, caso, ident):
                errores.append("%s con campos comunes no numéricos" % kind)
            elif not 0 <= caso <= 7:
                errores.append("%s case=%s fuera de 0..7" % (kind, campos["case"]))
        if not errores:
            for k in P4_REQ.get(kind, ()):
                if k not in campos:
                    errores.append("%s sin campo obligatorio %s" % (kind, k))
            for k in P4_NUM_FINITOS:
                if k in campos and _p4_num(campos[k]) is None:
                    errores.append("%s con %s no finito/NaN" % (kind, k))
            for k in P4_VECTORES:
                if k in campos and p4_vector(campos[k]) is None:
                    errores.append("%s con vector %s inválido" % (kind, k))
        if not errores and kind in ("identity", "overflow") and (caso != 0 or ident != 0):
            errores.append("%s con case/id distintos de 0" % kind)
        if not errores and kind == "identity" and ("version" not in campos or "data" not in campos):
            errores.append("identity sin version/data")
        if not errores and kind == "pose_end":
            w = campos.get("weights", "").split(",")
            if len(w) != len(P4_PESOS) or any(_p4_num(x) is None for x in w):
                errores.append("pose_end weights no es una lista de 9 números finitos")
        if not errores and kind in ("roll_start", "roll_end", "roll_cancel") and "clip" in campos:
            if "roll" not in campos["clip"].lower():
                errores.append("%s con clip ajeno (%s)" % (kind, campos.get("clip")))
        if not errores and kind in ("pose_begin", "pose_end"):
            p = _p4_num(campos.get("posture", ""))
            if p is None or p not in (0.0, 1.0):
                errores.append("%s posture fuera de 0/1" % kind)
        if errores:
            ev["invalidos"].extend(errores)
            continue
        ev["gens"].add(gen)
        ev["eventos"].append(campos)
        if kind == "identity":
            if ev["identidad"] is None:
                ev["identidad"] = campos
            elif (ev["identidad"].get("version"), ev["identidad"].get("data")) != (campos.get("version"), campos.get("data")):
                ev["invalidos"].append("identidad P4 mezclada (builds distintos)")
            continue
        if kind == "overflow":
            ev["overflow"].append(campos)
            continue
        fam = P4_FAMILIA.get(kind)
        if fam:
            reg = ev["por_id"].setdefault((fam, ident), {})
            if kind == P4_INICIO[fam]:
                if "begin" in reg:
                    ev["invalidos"].append("id duplicado %s/%d" % (fam, ident))
                    continue
                reg["begin"] = campos
                reg["_gen"] = gen
            else:
                if "begin" not in reg:
                    ev["invalidos"].append("terminal %s sin inicio (%s/%d)" % (kind, fam, ident))
                    continue
                if reg.get("_gen") != gen:
                    ev["invalidos"].append("generación cruzada en %s/%d" % (fam, ident))
                    continue
                if kind in reg:
                    ev["invalidos"].append("terminal duplicado %s/%d" % (kind, ident))
                    continue
                reg[kind] = campos
    return ev


def p4_pares(ev, fam):
    out = []
    for (f, ident), reg in ev["por_id"].items():
        if f != fam or "begin" not in reg:
            continue
        mu = dict(reg["begin"])
        fin = reg.get(P4_FIN[fam])
        if fin:
            mu.update(fin)
        can = reg.get(P4_CANCEL.get(fam, ""))
        if can:
            mu.update(can)
        mu["_id"] = ident
        mu["_fin"] = fin
        mu["_cancel"] = can
        out.append(mu)
    return out


def p4_1(ev):
    print("\n== P4.1 · S sin aim: hacia la cámara, de pie y agachado ==")
    muestras = collections.defaultdict(list)
    fallos = []
    incompletos = 0
    vistos = 0
    for mu in p4_pares(ev, "move"):
        lr = _p4_num(mu.get("lr", ""))
        ud = _p4_num(mu.get("ud", ""))
        if lr is None or ud is None or _p4_entero(mu.get("aim", "")) != 0:
            continue
        if abs(lr) > 8.0 or ud < 16.0:
            continue
        req = p4_vector(mu.get("req", ""))
        base = p4_vector(mu.get("baseF", ""))
        if req is None or base is None:
            incompletos += 1
            continue
        vistos += 1
        d = _p4_dot(req, base)
        derr = _p4_num(mu.get("dirErrMax", ""))
        berr = _p4_num(mu.get("bodyErrMax", ""))
        pos = _p4_entero(mu.get("crouch", ""))
        if d is None:
            incompletos += 1
        elif d > -0.9962:
            fallos.append("S #%s no avanza hacia la cámara (pedido·base=%.3f)" % (mu.get("_id"), d))
        elif derr is None or berr is None or pos is None:
            incompletos += 1
        elif derr > 5.0:
            fallos.append("S #%s con error pedido-real %.1f° (>5°)" % (mu.get("_id"), derr))
        elif berr > 10.0:
            fallos.append("S #%s con cuerpo a %.1f° de la marcha (>10°)" % (mu.get("_id"), berr))
        else:
            muestras[_p4_bucket(base)].append(pos)
    completos = [b for b, ps in muestras.items() if 0 in ps and 1 in ps]
    print("   tramos S sin aim: %d; incompletos: %d; orientaciones con de pie+agachado: %d"
          % (vistos, incompletos, len(completos)))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if vistos == 0:
        return "SIN DATOS", "sin tramos S sin aim con base de cámara y vector pedido"
    if len(completos) >= 4 and incompletos == 0:
        return "PASS", ("S gira y avanza hacia la cámara con error pedido-real <=5° y cuerpo <=10° "
                        "en %d orientaciones, de pie y agachado" % len(completos))
    return "PARCIAL", ("se exigen 4 cámaras (0/90/180/270) con de pie y agachado; completas: %d; "
                       "incompletas: %d" % (len(completos), incompletos))


def p4_2(ev):
    print("\n== P4.2 · diagonales: W+A, W+D, S+A, S+D combinan ambos ejes ==")
    diag = []
    refs = collections.defaultdict(list)
    fallos = []
    con_ref = set()
    sin_ref = 0
    for mu in p4_pares(ev, "move"):
        lr = _p4_num(mu.get("lr", ""))
        ud = _p4_num(mu.get("ud", ""))
        req = p4_vector(mu.get("req", ""))
        f = p4_vector(mu.get("baseF", ""))
        r = p4_vector(mu.get("baseR", ""))
        el = _p4_num(mu.get("elapsed", ""))
        di = _p4_num(mu.get("dist", ""))
        if None in (lr, ud, req, f, r, el, di) or el <= 0:
            continue
        if el < P4_TRAMO_MIN:
            continue
        vel = di / el
        cond = (_p4_entero(mu.get("aim", "")) or 0, _p4_entero(mu.get("crouch", "")) or 0,
                mu.get("weapon", ""))
        if abs(lr) > 8.0 and abs(ud) > 8.0:
            diag.append({"lr": lr, "ud": ud, "req": req, "f": f, "r": r, "vel": vel, "cond": cond})
        elif abs(lr) >= 60.0 and abs(ud) <= 8.0:
            refs[("lr", _p4_signo(lr), cond)].append(vel)
        elif abs(ud) >= 60.0 and abs(lr) <= 8.0:
            refs[("ud", _p4_signo(ud), cond)].append(vel)
    combos = collections.defaultdict(list)
    for mu in diag:
        lr = mu["lr"]
        ud = mu["ud"]
        req = mu["req"]
        f = mu["f"]
        r = mu["r"]
        vel = mu["vel"]
        cond = mu["cond"]
        d_f = _p4_dot(req, f)
        d_r = _p4_dot(req, r)
        if d_f is None or d_r is None:
            continue
        norma = math.hypot(lr, ud)
        if norma < 1e-9:
            continue
        coseno = max(-1.0, min(1.0, d_f * (-ud / norma) + d_r * (lr / norma)))
        err = math.degrees(math.acos(coseno))
        ang = math.degrees(math.atan2(abs(d_r), abs(d_f)))
        combo = (_p4_signo(lr), _p4_signo(ud))
        combos[combo].append(vel)
        if err > 5.0:
            fallos.append("diagonal %s no combina el input (%.1f°)" % (combo, err))
        elif ang < 40.0 or ang > 50.0:
            fallos.append("diagonal %s a %.1f° de los ejes (45±5)" % (combo, ang))
        ra = refs.get(("ud", _p4_signo(ud), cond), [])
        rb = refs.get(("lr", _p4_signo(lr), cond), [])
        if ra and rb:
            ref = (statistics.median(ra) + statistics.median(rb)) / 2.0
            if ref > 0:
                ratio = vel / ref
                if ratio < 0.95 or ratio > 1.05:
                    fallos.append("diagonal %s a %.2f m/s vs ejes %.2f (%.0f%%)"
                                  % (combo, vel, ref, ratio * 100.0))
                else:
                    con_ref.add(combo)
        else:
            sin_ref += 1
    print("   diagonales distintas: %d; con referencia simple: %d; sin referencia: %d"
          % (len(combos), len(con_ref), sin_ref))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if not combos:
        return "SIN DATOS", "sin tramos diagonales (|lr|>8 y |ud|>8) con vector pedido y base"
    faltan = sorted(set((a, b) for a in (-1, 1) for b in (-1, 1)) - set(combos))
    if len(combos) == 4 and len(con_ref) >= 4 and sin_ref == 0:
        return "PASS", ("las 4 diagonales combinan los dos ejes a 45±5° y conservan la velocidad "
                        "de los ejes simples ±5%")
    if faltan:
        return "PARCIAL", "faltan diagonales %s" % faltan
    return "PARCIAL", "diagonales medidas sin referencia simple para comparar la velocidad"


def p4_3(ev):
    print("\n== P4.3 · postura: entrada rápida y salida sin deformación ==")
    ends = [e for e in ev["eventos"] if e["kind"] == "pose_end"]
    fallos = []
    entradas = salidas = 0
    entrada_sin_t80 = salida_sin_medida = 0
    muestras9 = 0
    for e in ends:
        w = e.get("weights", "").split(",")
        if len(w) == 9 and all(_p4_num(x) is not None for x in w):
            muestras9 += 1
        p = _p4_entero(e.get("posture", ""))
        t80 = _p4_num(e.get("t80", ""))
        tc = _p4_num(e.get("tClear", ""))
        mo = _p4_num(e.get("maxOwnAfter500", ""))
        nf = _p4_num(e.get("boneNonfinite", ""))
        dg = _p4_num(e.get("boneDegenerate", ""))
        if p == 1:
            entradas += 1
            if t80 is None:
                entrada_sin_t80 += 1
            elif t80 > 0.250:
                fallos.append("entrada #%s tardó %.3f s en el 80%% del peso" % (e.get("id"), t80))
        elif p == 0:
            salidas += 1
            if tc is None and mo is None:
                salida_sin_medida += 1
            if tc is not None and tc > 0.500:
                fallos.append("salida #%s con clips propios %.3f s después" % (e.get("id"), tc))
            if mo is not None and mo > 0.01:
                fallos.append("salida #%s con peso propio residual %.4f" % (e.get("id"), mo))
        if nf is not None and nf > 0:
            fallos.append("postura #%s con %d muestras de hueso no finitas" % (e.get("id"), int(nf)))
        if dg is not None and dg > 0:
            fallos.append("postura #%s con %d muestras degeneradas" % (e.get("id"), int(dg)))
    print("   pose_end: %d (entradas %d, salidas %d); con pesos 9/9: %d"
          % (len(ends), entradas, salidas, muestras9))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if not ends:
        return "SIN DATOS", "sin trazas pose_begin/pose_end: la postura no se observó"
    if entradas >= 20 and salidas >= 20 and entrada_sin_t80 == 0 and salida_sin_medida == 0:
        return "PASS", ("20+ entradas al 80%% de peso <=0.250 s y 20+ salidas con clips propios "
                        "<=0.01 en <=0.500 s; huesos finitos y no degenerados")
    return "PARCIAL", ("entradas=%d salidas=%d (se exigen 20+20); sin t80: %d; salidas sin medida: %d"
                       % (entradas, salidas, entrada_sin_t80, salida_sin_medida))


def p4_4(ev):
    print("\n== P4.4 · velocidad agachado = WASD normal, sin sprint ni LALT ==")

    def limpia(mu):
        el = _p4_num(mu.get("elapsed", ""))
        di = _p4_num(mu.get("dist", ""))
        lr = _p4_num(mu.get("lr", ""))
        ud = _p4_num(mu.get("ud", ""))
        cr = _p4_entero(mu.get("crouch", ""))
        sp = _p4_num(mu.get("sprint", ""))
        wk = _p4_num(mu.get("walk", ""))
        if None in (el, di, lr, ud, sp, wk) or cr is None or el <= 0:
            return None
        if sp != 0.0 or wk != 0.0:
            return None
        clave = (_p4_entero(mu.get("aim", "")) or 0, mu.get("weapon", ""),
                 _p4_signo(lr), _p4_signo(ud), round(abs(lr), 1), round(abs(ud), 1))
        return {"vel": di / el, "el": el, "crouch": cr, "clave": clave, "mu": mu}

    pie = collections.defaultdict(list)
    aga = collections.defaultdict(list)
    for mu in p4_pares(ev, "move"):
        m = limpia(mu)
        if not m:
            continue
        (aga if m["crouch"] == 1 else pie)[m["clave"]].append(m)
    pares = []
    for clave, ags in aga.items():
        for a in ags:
            for p in pie.get(clave, []):
                pares.append((a, p))
    fallos = []
    largos = []
    sin_clamp = 0
    cortos = 0
    for a, p in pares:
        if p["vel"] <= 0:
            continue
        if a["el"] < P4_TRAMO_MIN or p["el"] < P4_TRAMO_MIN:
            cortos += 1
            continue
        largos.append((a, p))
        ratio = a["vel"] / p["vel"]
        if ratio < 0.95 or ratio > 1.05:
            fallos.append("agachado %.2f vs de pie %.2f (%.0f%%)" % (a["vel"], p["vel"], ratio * 100.0))
        for m in (a, p):
            st = _p4_num(m["mu"].get("speedTarget", ""))
            sr = _p4_num(m["mu"].get("speedReal", ""))
            if st is None or sr is None or st <= 0:
                sin_clamp += 1
                continue
            if abs(sr - st) > 0.05 * st:
                fallos.append("velocidad %.2f lejos del objetivo %.2f (clamp)" % (sr, st))
    print("   pares pie/agachado: %d (de >=%.1f s: %d; tirados por cortos: %d); sin objetivo declarado: %d"
          % (len(pares), P4_TRAMO_MIN, len(largos), cortos, sin_clamp))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if not pares:
        return "SIN DATOS", "sin pares pie/agachado comparables (mismo input, arma y aim; sin sprint/LALT)"
    if len(largos) >= 3 and sin_clamp == 0:
        return "PASS", "3+ pares >=3 s con ratio 0.95–1.05 y velocidad efectiva pegada al objetivo (sin clamp)"
    return "PARCIAL", ("se exigen 3 pares >=3 s con objetivo declarado; largos=%d; sin objetivo=%d"
                       % (len(largos), sin_clamp))


def p4_5(ev):
    print("\n== P4.5 · rodada completa por toque (sin bucle) ==")
    starts = [(ident, reg) for (fam, ident), reg in ev["por_id"].items()
              if fam == "roll" and "begin" in reg]
    starts.sort(key=lambda t: _p4_num(t[1]["begin"].get("sim", "")) or 0.0)
    fallos = []
    gaps = 0
    lados = collections.Counter()
    con_fin = collections.Counter()
    edges_vistos = set()
    rate_ok = 0
    for ident, reg in starts:
        b = reg["begin"]
        e = reg.get("roll_end")
        c = reg.get("roll_cancel")
        lado = _p4_lado(b.get("side"))
        if lado is None:
            fallos.append("rodada #%d sin lado A/D" % ident)
        else:
            lados[lado] += 1
        edge = b.get("edge")
        if edge is not None:
            if edge in edges_vistos:
                fallos.append("dos inicios con el mismo flanco (%s)" % edge)
            edges_vistos.add(edge)
        if e is None and c is None:
            gaps += 1
            continue
        if c is not None:
            causa = str(c.get("cause", "")).lower()
            if any(x in causa for x in P4_CAUSAS_CORTE):
                fallos.append("rodada #%d cancelada por soltar/mantener/cambiar lado (%s)" % (ident, causa))
            continue
        if _p4_lado(e.get("side")) != lado:
            fallos.append("rodada #%d cambió de lado" % ident)
        if b.get("clip") != e.get("clip"):
            fallos.append("rodada #%d cambió de clip" % ident)
        prog = _p4_num(e.get("progress", ""))
        if prog is None or prog < 0.999:
            fallos.append("rodada #%d terminó con progreso %s" % (ident, prog if prog is not None else "?"))
        largo = _p4_num(b.get("length", ""))
        ri = _p4_num(e.get("rateIntegral", ""))
        if largo is not None and largo > 0 and ri is not None:
            if abs(ri - largo) > 0.10 * largo:
                fallos.append("rodada #%d con reloj efectivo %.3f vs clip %.3f" % (ident, ri, largo))
            else:
                rate_ok += 1
        se = _p4_num(e.get("simElapsed", ""))
        sp = _p4_num(b.get("speed", ""))
        if se is not None and sp is not None and sp > 0 and largo:
            esp = largo / sp
            if abs(se - esp) > 0.10 * esp:
                fallos.append("rodada #%d duró %.3f s vs %.3f s esperados" % (ident, se, esp))
        con_fin[lado] += 1
    for (i1, r1), (i2, r2) in zip(starts, starts[1:]):
        s2 = _p4_num(r2["begin"].get("sim", "")) or 0.0
        e1 = r1.get("roll_end") or r1.get("roll_cancel")
        if e1 is None:
            continue
        t1 = _p4_num(e1.get("sim", ""))
        if t1 is not None and t1 > s2:
            fallos.append("rodada #%d sigue activa cuando empieza #%d" % (i1, i2))
    edges = [x for x in ev["eventos"] if x["kind"] == "input_edge"]
    eleg = [x for x in edges if _p4_entero(x.get("neutral", "")) == 1
            and _p4_entero(x.get("eligible", "")) == 1]
    if edges and len(starts) > len(eleg):
        fallos.append("%d inicios para %d pulsaciones elegibles (retrigger)" % (len(starts), len(eleg)))
    holds = [x for x in ev["eventos"] if x["kind"] == "input_hold_end"]
    hold3 = [x for x in holds if (_p4_num(x.get("heldSim", "")) or 0) >= 3.0]
    print("   rodadas: %d (L=%d R=%d); con fin: %d; sin cierre: %d; flancos elegibles: %d; holds>=3 s: %d"
          % (len(starts), lados["L"], lados["R"], sum(con_fin.values()), gaps, len(eleg), len(hold3)))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if not starts:
        return "SIN DATOS", "sin trazas roll_start/roll_end"
    if (con_fin["L"] >= 10 and con_fin["R"] >= 10 and gaps == 0 and rate_ok >= 20
            and hold3 and len(eleg) >= 20):
        return "PASS", ("10+ toques por lado con un fin completo por flanco, soltar no cancela, "
                        "sin retrigger y reloj de clip coherente ±10%")
    return "PARCIAL", ("se exigen 10+10 rodadas completas, flancos elegibles y hold>=3 s; "
                       "L=%d R=%d sin cierre=%d" % (con_fin["L"], con_fin["R"], gaps))


def p4_6(ev):
    print("\n== P4.6 · apuntar sin reorientar la vista (yaw/pitch/zoom/rayo) ==")
    pares = [(ident, reg) for (fam, ident), reg in ev["por_id"].items()
             if fam == "cam" and "begin" in reg]
    fallos = []
    excl_lock = con_colision = con_clamp = 0
    completos = rifle = pistola = rayos = 0
    posturas = set()
    for ident, reg in pares:
        b = dict(reg["begin"])
        for k in ("cam_first", "cam_end"):
            if reg.get(k):
                b.update(reg[k])
        v0 = p4_vector(reg["begin"].get("visibleFront", ""))
        if v0 is None:
            fallos.append("transición #%d sin vista presentada inicial" % ident)
            continue
        if reg.get("cam_cancel"):
            continue
        vf = p4_vector(reg["cam_first"]["visibleFront"]) if reg.get("cam_first") else None
        ve = p4_vector(reg["cam_end"]["visibleFront"]) if reg.get("cam_end") else None
        if vf is None and ve is None:
            continue
        completos += 1
        if _p4_entero(b.get("mouse", "")) == 1 or _p4_entero(b.get("lock", "")) == 1:
            excl_lock += 1
            continue
        if _p4_entero(b.get("collision", "")) == 1:
            con_colision += 1
            continue
        pc = _p4_entero(b.get("pitchClamped", ""))
        for v, et in ((vf, "primer frame"), (ve, "fin")):
            if v is None:
                continue
            d = _p4_dot(v0, v)
            if d is not None and d < 0.0:
                fallos.append("transición #%d: vista invertida en %s (dot=%.3f)" % (ident, et, d))
            yaw = _p4_angulo(_p4_horiz(v0), _p4_horiz(v))
            if yaw is not None and yaw > 1.0:
                fallos.append("transición #%d: yaw %.2f° en %s" % (ident, yaw, et))
        if ve is not None:
            if pc == 1:
                con_clamp += 1
            else:
                p0 = math.atan2(v0[2], math.hypot(v0[0], v0[1]))
                p1 = math.atan2(ve[2], math.hypot(ve[0], ve[1]))
                if abs(math.degrees(p1 - p0)) > 1.0:
                    fallos.append("transición #%d: pitch %.2f° vs referencia %.2f°"
                                  % (ident, math.degrees(p1), math.degrees(p0)))
        fov = _p4_num(b.get("fov", ""))
        ft = _p4_num(b.get("fovTarget", ""))
        if fov is not None and ft is not None:
            if abs(fov - ft) > 0.5:
                fallos.append("transición #%d: FOV %.2f no converge a %.2f" % (ident, fov, ft))
            if abs(ft - 50.0) <= 0.5:
                rifle += 1
                if abs(fov - 50.0) > 0.5:
                    fallos.append("transición #%d: rifle no converge a 50° (%.2f)" % (ident, fov))
            elif abs(ft - 70.0) <= 0.5:
                pistola += 1
                if abs(fov - 70.0) > 0.5:
                    fallos.append("transición #%d: pistola no converge a 70° (%.2f)" % (ident, fov))
        pix = _p4_num(b.get("pixelErr", ""))
        if pix is not None:
            if pix > 2.0:
                fallos.append("transición #%d: rayo a %.1f px de la retícula" % (ident, pix))
            else:
                rayos += 1
        p = _p4_entero(b.get("crouch", ""))
        if p is not None:
            posturas.add(p)
    print("   transiciones con receptor: %d; rifle=%d pistola=%d rayo<=2px=%d; lock=%d colisión=%d clamp=%d"
          % (completos, rifle, pistola, rayos, excl_lock, con_colision, con_clamp))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if completos == 0:
        return "SIN DATOS", "sin trazas cam_begin/cam_first/cam_end"
    if completos >= 20 and rifle >= 1 and pistola >= 1 and rayos >= 1 and len(posturas) == 2:
        return "PASS", "20+ conmutaciones sin reorientar la vista; rifle/pistola convergen; rayo <=2 px"
    return "PARCIAL", ("faltan: %s" % ", ".join(n for n, c in (
        ("20 transiciones", completos >= 20), ("rifle", rifle >= 1), ("pistola", pistola >= 1),
        ("rayo", rayos >= 1), ("ambas posturas", len(posturas) == 2)) if not c))


def p4_7(ev):
    print("\n== P4.7 · sirenas: barra, lentes, ancla y estado ==")
    serv = [e for e in ev["eventos"] if e["kind"] == "service"]
    if not serv:
        return "SIN DATOS", "sin trazas service (barra/sirena) para ninguna patrulla"
    pol = [e for e in serv if _p4_entero(e.get("model", "")) == PB_POLICE]
    if not pol:
        return "SIN DATOS", "sin muestras de la patrulla police (model=%d)" % PB_POLICE
    fallos = []
    varis = set()
    por_coche = collections.defaultdict(list)
    on = off = con_dano = 0
    caches = set()
    con_rot = set()
    for e in pol:
        veh = e.get("vehicle")
        por_coche[veh].append(e)
        var = _p4_entero(e.get("var", ""))
        if var is not None:
            varis.add(var)
        sirena = _p4_entero(e.get("siren", ""))
        sel = _p4_entero(e.get("selectedBars", ""))
        vis = _p4_entero(e.get("visibleLenses", ""))
        anc = _p4_num(e.get("anchorError", ""))
        reg = _p4_entero(e.get("registered", ""))
        cache = str(e.get("cache", "")).lower()
        if cache in ("cold", "frio", "fria", "0"):
            caches.add("cold")
        elif cache in ("hot", "caliente", "1"):
            caches.add("hot")
        if (_p4_entero(e.get("damage", "")) or 0) == 1:
            con_dano += 1
        rot = p4_vector(e.get("carRot", ""))
        if rot is not None:
            con_rot.add(_p4_bucket(rot))
        if sirena == 1:
            on += 1
            if anc is None:
                fallos.append("coche %s: sirena encendida sin error de ancla medido" % veh)
            elif anc > 0.10:
                fallos.append("coche %s: ancla a %.3f m (>0.10)" % (veh, anc))
            if vis is not None and vis <= 0:
                fallos.append("coche %s: barra elegida y ninguna lente visible" % veh)
            if sel is not None and sel <= 0:
                fallos.append("coche %s: ninguna barra seleccionada con sirena encendida" % veh)
        elif sirena == 0:
            off += 1
            if (reg or 0) > 0:
                fallos.append("coche %s: %d coronas nuevas con la sirena apagada" % (veh, reg))
    mundos = {}
    for e in pol:
        w = p4_vector(e.get("world", ""))
        if w is None:
            continue
        clave = tuple(round(x, 3) for x in w)
        if clave in mundos and mundos[clave] != e.get("vehicle"):
            fallos.append("dos coches comparten la misma posición de mundo (heredada)")
        mundos[clave] = e.get("vehicle")
    pos = {}
    for e in pol:
        p = p4_vector(e.get("carPos", ""))
        if p is not None:
            pos[e.get("vehicle")] = p
    vals = list(pos.values())
    dist_max = 0.0
    for i in range(len(vals)):
        for j in range(i + 1, len(vals)):
            dist_max = max(dist_max, math.hypot(vals[i][0] - vals[j][0], vals[i][1] - vals[j][1]))
    print("   police: %d muestras, %d coche(s); sirena on=%d off=%d; variantes=%s; cache=%s; daño=%d; dist máx=%.1f m"
          % (len(pol), len(por_coche), on, off, sorted(v for v in varis if v is not None) or "-",
             sorted(caches) or "-", con_dano, dist_max))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    faltas = []
    if len(por_coche) < 2 or dist_max < 50.0:
        faltas.append("2 coches a >=50 m")
    if len(con_rot) < 2:
        faltas.append("rotaciones distintas")
    if len(varis) < 3:
        faltas.append("3 variantes")
    if not (on and off):
        faltas.append("sirena on/off")
    if caches != {"cold", "hot"}:
        faltas.append("cache frío/caliente")
    if con_dano < 1:
        faltas.append("daño/restream")
    if not faltas:
        return "PASS", "barra y lentes juntas, ancla <=0.10 m, sin coronas nuevas en off, sin mundo heredado"
    return "PARCIAL", "cobertura incompleta: falta %s" % ", ".join(faltas)


def p4_8(ev):
    print("\n== P4.8 · desagacharse: el grupo de agachado queda a peso 0 a las 30 muestras ==")
    pe = [e for e in ev["eventos"] if e["kind"] == "pose_exit"]
    if not pe:
        return "SIN DATOS", "sin trazas pose_exit: no se midió ningún desagachado"
    grupos = collections.defaultdict(list)
    for e in pe:
        grupos[_p4_entero(e.get("id", ""))].append(e)
    fallos = []
    incompletos = []
    completos = 0
    for ident in sorted(grupos, key=lambda k: str(k)):
        mejor = None
        for e in grupos[ident]:
            smp = _p4_entero(e.get("sample", ""))
            if smp is None:
                continue
            if mejor is None or smp > mejor[0]:
                mejor = (smp, e)
        if mejor is None:
            incompletos.append("id=%s sin muestra de sample" % ident)
            continue
        smp, ult = mejor
        if smp < 29:
            incompletos.append("id=%s se quedó en la muestra %d (no llegó a 30)" % (ident, smp))
            continue
        w = _p4_num(ult.get("w", ""))
        cuantas = _p4_entero(ult.get("n", ""))
        completos += 1
        if w is None:
            fallos.append("id=%s sin peso medido en la última muestra" % ident)
        elif w > 0.05:
            fallos.append("id=%s sigue con %s asociación(es) del grupo a peso %.4f (>0.05) a las 30 muestras"
                          % (ident, cuantas if cuantas is not None else "?", w))
    print("   desagachados: %d grupo(s) pose_exit; %d con las 30 muestras; %d incompleto(s)"
          % (len(grupos), completos, len(incompletos)))
    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if incompletos:
        return "PARCIAL", "cobertura incompleta: %s" % ", ".join(incompletos[:4])
    if not completos:
        return "SIN DATOS", "pose_exit sin ningún desagachado con las 30 muestras completas"
    return "PASS", ("%d desagachado(s) con el grupo de agachado a peso <=0.05 a las 30 muestras"
                     % completos)


def _p44_campo(linea, campo):
    m = re.search(r"(?:^|\s)%s=([^\s\"]+)" % campo, linea)
    return m.group(1) if m else None


def _p44_num(linea, campo):
    v = _p44_campo(linea, campo)
    try:
        return float(v)
    except (TypeError, ValueError):
        return None


def p4_9(lineas):
    """Plan 4.4 · pies al suelo contra el avance real, mezcla de la salida del agachado,
    cañón contra retícula y gatillo sin apuntar con el estado RESULTANTE."""
    print("\n== P4.9 · plan 4.4: pies al suelo, salida del agachado, cañón y gatillo ==")
    fallos = []
    faltan = []
    nuevo = any(("canon=" in l) or ("piesMps=" in l) for l in lineas)

    pares = []
    for l in lineas:
        if "CROUCH2 " not in l:
            continue
        p, v = _p44_num(l, "piesMps"), _p44_num(l, "velo")
        if p is not None and v is not None and v > 0.2:
            pares.append((p, v))
    if pares:
        desv = sorted(abs(a - b) / b for a, b in pares)
        malos = [d for d in desv if d > 0.05]
        print("   pies al suelo vs avance (`piesMps`/`velo`): %d muestra(s), mediana=%.1f %%, máximo=%.1f %%"
              % (len(pares), 100.0 * desv[len(desv) // 2], 100.0 * desv[-1]))
        if malos:
            fallos.append("los pies no van al avance en %d muestra(s) (criterio 3): máximo=%.1f %%"
                          % (len(malos), 100.0 * desv[-1]))
    else:
        print("   pies al suelo: sin `piesMps` (build anterior a ctrl47)")
        faltan.append("`piesMps` (criterio 3)")

    pes = [v for v in (_p44_num(l, "pesosum") for l in lineas if "kind=pose_exit" in l)
           if v is not None]
    if pes:
        print("   salida del agachado (`pesosum`): %d muestra(s), máx=%.2f" % (len(pes), max(pes)))
        if max(pes) > 1.05:
            fallos.append("`pesosum` > 1,05 (máx=%.2f): al desagacharse la mezcla de movimiento pasa "
                          "de 1 y el reparto del motor deforma el cuerpo (criterio 5)" % max(pes))
    else:
        print("   salida del agachado: sin `pesosum` (build anterior a ctrl47)")
        faltan.append("`pesosum` (criterio 5)")
    tcl = [v for v in (_p44_num(l, "tClear") for l in lineas if "kind=pose_end" in l)
           if v is not None and v >= 0.0]
    if tcl:
        print("   salida del agachado (`tClear`): %d muestra(s), máx=%.3f s" % (len(tcl), max(tcl)))
        if max(tcl) > 0.35 and nuevo:
            fallos.append("la salida del agachado tarda más de 0,35 s en limpiar "
                          "(tClear máx=%.3f s): criterio 5" % max(tcl))
        elif max(tcl) > 0.35:
            print("      (build anterior a ctrl47: el reloj de la salida cambió en esta build, no se juzga)")

    can = [v for v in (_p44_num(l, "canon") for l in lineas if "kind=armsight" in l)
           if v is not None]
    if can:
        print("   cañón contra retícula (`armsight canon`): %d muestra(s), máx=%.2fº"
              % (len(can), max(can)))
        if max(can) > 3.0:
            fallos.append("el cañón no apunta al centro de la cámara (criterio 8): máx=%.2fº > 3º"
                          % max(can))
    else:
        print("   cañón contra retícula: sin `canon` (build anterior a ctrl47)")
        faltan.append("`canon` (criterio 8)")

    gatillos = []
    for l in lineas:
        if "kind=fire" not in l:
            continue
        a, g = _p44_num(l, "aim"), _p44_num(l, "gat")
        arma, move, est = _p44_num(l, "arma"), _p44_num(l, "move"), _p44_num(l, "estado")
        if None in (a, g, arma, move, est) or a != 0 or g != 1 or arma <= 0 or move == 5:
            continue
        gatillos.append((int(arma), int(est)))
    if gatillos:
        cortos = [t for t in gatillos if t[1] < 17]
        print("   gatillo sin apuntar (criterio 9): %d muestra(s), con estado>=17: %d"
              % (len(gatillos), len(gatillos) - len(cortos)))
        if cortos and nuevo:
            fallos.append("disparo sin apuntar que no entra en ataque (arma=%d estado=%d): criterio 9"
                          % cortos[0])
        elif cortos:
            print("      (build anterior a ctrl47: la sonda mide el estado ANTES de la puerta del gatillo, "
                  "no se juzga)")
    else:
        print("   gatillo sin apuntar: la sesión no tiene ninguna muestra `fire` con `aim=0` y `gat=1`")
        faltan.append("`fire` con `aim=0` y `gat=1` (criterio 9)")

    if fallos:
        return "FALLO", "; ".join(fallos[:4])
    if len(faltan) == 4:
        return "SIN DATOS", "el log no trae los campos del plan 4.4 (build anterior a ctrl47)"
    if faltan:
        return "PARCIAL", "sin datos para %s" % ", ".join(faltan)
    return "PASS", "los cuatro criterios de 4.4 dentro de sus topes"


def p4_esperados(a):
    v = a.expected_version
    d = a.expected_data_tag
    if not v:
        v = od_version_servida()
    if not d:
        try:
            ruta = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "web", "ondemand.js")
            with open(ruta, encoding="utf-8", errors="replace") as f:
                m = re.search(r"dataTag\s*:\s*'([^']+)'", f.read())
            d = m.group(1) if m else None
        except OSError:
            d = None
    return v, d


def p4_identidad(ev, exp_v, exp_d):
    problemas = []
    ident = ev["identidad"]
    if not ident:
        return "SIN DATOS", ["no hay traza P4 kind=identity: la identidad no se puede afirmar"]
    if len(ev["headers"]) > 1:
        problemas.append("mezcla de sesiones en las cabeceras: %s" % sorted(ev["headers"]))
    hv = hd = None
    if ev["headers"]:
        hv, hd = sorted(ev["headers"])[0]
    v = ident.get("version")
    d = ident.get("data")
    if exp_v is None or exp_d is None:
        problemas.append("faltan --expected-version/--expected-data-tag y no se pudieron derivar")
    if exp_v is not None and v != exp_v:
        problemas.append("VERSION del log %s != esperada %s" % (v, exp_v))
    if exp_d is not None and d != exp_d:
        problemas.append("dataTag del log %s != esperado %s" % (d, exp_d))
    if hv is not None and (hv != v or hd != d):
        problemas.append("la cabecera JS (build=%s data=%s) no coincide con P4 identity (%s/%s)"
                         % (hv, hd, v, d))
    return ("OK" if not problemas else "FALLO"), problemas


def p4_main(a, lineas):
    exp_v, exp_d = p4_esperados(a)
    ev = p4_cargar(lineas)
    print("== plan 4.0 · evaluación estricta (P4) ==")
    print("   log: %s" % a.log)
    print("   esperado: VERSION=%s dataTag=%s" % (exp_v or "?", exp_d or "?"))
    if not ev["eventos"]:
        print("   P4.0  SIN DATOS  el log no trae ninguna traza P4 (cero cobertura)")
        return 2
    if ev["invalidos"]:
        for msg in ev["invalidos"][:10]:
            print("   P4.0  FALLO     evidencia inválida: %s" % msg)
        return 1
    if ev["overflow"]:
        print("   P4.0  FALLO     buffers P4 desbordados (kind=overflow x%d)" % len(ev["overflow"]))
        return 1
    idestado, iprob = p4_identidad(ev, exp_v, exp_d)
    if idestado != "OK":
        for msg in iprob:
            print("   P4.0  %-9s %s" % (idestado, msg))
        return 1
    print("   P4.0  OK        identidad VERSION=%s dataTag=%s; gens=%s"
          % (ev["identidad"].get("version"), ev["identidad"].get("data"),
             ",".join(str(g) for g in sorted(ev["gens"]))))
    res = (("P4.1", p4_1(ev)), ("P4.2", p4_2(ev)), ("P4.3", p4_3(ev)),
           ("P4.4", p4_4(ev)), ("P4.5", p4_5(ev)), ("P4.6", p4_6(ev)),
           ("P4.7", p4_7(ev)), ("P4.8", p4_8(ev)), ("P4.9", p4_9(lineas)))
    print("\n== resumen plan 4.0 ==")
    for nombre, (estado, motivo) in res:
        print("   %s %-9s %s" % (nombre, estado, motivo))
    if any(estado == "FALLO" for _, (estado, _) in res):
        return 1
    base = [(n, v) for n, v in res if n != "P4.9"]
    if all(estado == "PASS" for _, (estado, _) in base):
        return 0
    return 2


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", nargs="?", default=DEFECTO)
    ap.add_argument("--desde-marca", action="store_true",
                    help="sólo desde la última 'rotado: sesion nueva' (una partida)")
    ap.add_argument("--plan-4", action="store_true",
                    help="evaluación estricta del plan 4.0: P4.1..P4.7 con salida 0/1/2")
    ap.add_argument("--expected-version", default=None,
                    help="VERSION que debe declarar la sesión en modo --plan-4")
    ap.add_argument("--expected-data-tag", default=None,
                    help="dataTag que debe declarar la sesión en modo --plan-4")
    a = ap.parse_args()
    if not os.path.exists(a.log):
        print("FAIL: no encuentro el log (%s)" % a.log)
        return 1
    lineas = leer(a.log, a.desde_marca)
    if a.plan_4:
        return p4_main(a, lineas)
    build_ok = bloque_build(a.log)
    if not build_ok:
        print("   AVISO: todo lo de abajo se midio sobre una build que no es la")
        print("   servida. Un 'no funciona' aqui no significa que el arreglo falle.")

    print("== log: %s (%d líneas%s)" % (a.log, len(lineas),
                                        ", desde la última sesión" if a.desde_marca else ""))

    # Aviso útil: la rotación de trazas (vite.js) deja la sesión anterior en
    # odtrace.prev.log. Si el log que se pasa es sólo el menú (ingame=0 y sin
    # marcas de juego), la partida probablemente está en ese fichero.
    juego = [l for l in lineas if re.search(r"CARPED|CARSPAWN|IFPFILE|SIGHT |HINTKEY|BSIREN", l)]
    if not juego:
        prev = re.sub(r"\.log$", ".prev.log", a.log)
        print("   AVISO: este log no tiene marcas de juego (¿sólo menú?).")
        if os.path.exists(prev):
            print("   La sesión anterior está en %s: pásala como argumento." % prev)

    res = {}
    for nombre, fn in (("D2", bloque_d2), ("D4", bloque_d4), ("D5", bloque_d5),
                       ("D6", bloque_d6), ("D7", bloque_d7), ("D8", bloque_d8),
                       ("R2", bloque_r2), ("R3", bloque_r3), ("J1", bloque_j1),
                       ("R5", bloque_r5), ("R9", bloque_r9), ("R11", bloque_r11),
                       ("L2", bloque_icon), ("R3b", bloque_r3b),
                       ("L6", bloque_l6), ("H", bloque_h), ("R12", bloque_r12),
                       ("SR", bloque_sirena), ("LB", bloque_lbar), ("R13", bloque_r13),
                        ("R14", bloque_r14), ("R15", bloque_r15), ("R16", bloque_r16),
                        ("R17", bloque_r17), ("R19", bloque_r19), ("R27", bloque_r27),
                        ("RC", bloque_recoil), ("AX", bloque_ax)):
        res[nombre] = fn(lineas)

    print("\n== resumen ==")
    if not build_ok:
        print("   B   FALLO    build de la sesion distinta de la servida (ver arriba)")
    for nombre in ("D2", "D4", "D5", "D6", "D7", "D8", "R2", "R3", "J1",
                   "R5", "R9", "R11", "L2", "R3b", "L6", "H", "R12", "R13", "R14", "R15", "R16",
                   "R17", "R19", "R27", "SR", "LB", "AX"):
        estado, motivo = res[nombre]
        print("   %-3s %-9s %s" % (nombre, estado, motivo))
    print("   RC  %-9s %s" % res["RC"])
    # J1 es diagnóstico (cuelgue / LOD pegado): informa, pero no tumba el
    # veredicto de los bloques D.
    ok = all(res[n][0] == "OK" for n in ("D2", "D4", "D5", "D6", "D7", "D8"))
    print("\n" + ("TODO OK — los cuatro bloques dejan evidencia en el log"
                  if ok else "hay bloques SIN DATOS o PARCIALES: mira el detalle de arriba"))
    if res["J1"][0] == "FALLO":
        print("ADEMÁS: hay un cuelgue registrado en la traza (J1).")
    for n in ("R2", "R3", "D8"):
        if res[n][0] == "FALLO":
            print("ADEMÁS: %s en FALLO (%s)." % (n, res[n][1]))
    # Bloques de la 7ª partida (21/09): 1ª persona, apuntado, sirenas e iconos.
    # Cuentan como fallo sólo si hay datos y salen FALLO; "SIN DATOS" es que esa
    # mecánica no se usó en la partida (no se puede juzgar).
    nuevas = [n for n in ("R5", "R9", "R11", "L2", "L6", "H", "R12", "R13", "R14", "R15", "R16",
                          "R17", "R19", "R27", "SR", "LB", "AX")
              if res[n][0] == "FALLO"]
    if nuevas:
        for n in nuevas:
            print("FALLO %s: %s" % (n, res[n][1]))
        ok = False
    if not build_ok:
        print("FALLO B: la sesion se hizo con una build vieja; los PASS de arriba no valen")
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
