#!/usr/bin/env bash
# ¿El motor que se sirve lleva de verdad lo que creemos que lleva?
#
# Motivo (21/09/2026, 9ª partida): el jugador estuvo varias partidas con el
# `.wasm` de por la mañana. Todo lo implementado ese día (R5-R11, D8b, capa web)
# estaba compilado como objeto, pero el paquete NUNCA se enlazó: probó código
# viejo y su informe fue "no ha cambiado nada". Desde el log eso NO se ve (la
# traza sólo dice qué código corrió, no qué se esperaba que corriera).
#
# Uso:
#   bash tools/check-served-build.sh          # desde gta_vc_browser/
#   bash tools/check-served-build.sh --listar  # qué marcas busca
#
# Salida: una línea por bloque (marca -> presente/ausente) y veredicto.
# Exit 0 = el motor servido lleva TODAS las marcas.
set -uo pipefail

cd "$(dirname "$0")/.." || exit 2
BUILD=web/public/build
WASM=$BUILD/reVC.wasm
PRE=$BUILD/reVC.js

# marca|bloque|dónde (wasm = motor, pre = capa web/ondemand)
MARCAS=(
  "VICEEX sfx arma|D8  sonidos: qué muestra pide cada arma|wasm"
  "odViceExSample|D8b sonidos del mod admitidos por el motor|wasm"
  "SWIM2 move|R7  nado: avance real medido|wasm"
  "SVLIGHTS|R11 luces de servicio por dummies|wasm"
  "extras=|R11 luces de servicio: variante y extras elegidos|wasm"
  "SIRENA tipo=|SR  sirena audible: muestra que encola el motor|wasm"
  "LBAR w=|LB  contorno de la barra de carga escalado (SilentPatch 723)|wasm"
  "LOADSCR begin|L6  carga de partida: una portada y una barra (sección 1)|wasm"
  "camz=|H1/H2 cámara medida agachado y nadando|wasm"
  "servicelightson|§6  nombres de luces de servicio del mod|wasm"
  "pedido=|R12 autocentrado de coche: retorno pedido vs pasivo|wasm"
  "WINFO arma=|R13 ficha por arma: flags, canaim/witharm, reload y nombres de clip|wasm"
  "clips=|R13 los NOMBRES de clip que resuelve el grupo del arma|wasm"
  "SWIMCAM|R14 nado: objetivo de cámara en la superficie|wasm"
  "ODSFXPROT|R14 sonidos de arma del mod reservados (sin cola ni reciclado)|wasm"
  "swim move spd=|R17 nado: velocidad y cadencia dictadas por el clip del mod|wasm"
  "SWIMCAM no-fallen-water|R17 cámara: el jugador vivo en el agua no usa la cámara de ahogado|wasm"
  "ViceExtSwimClipSpeed|R17 nado: m/s natural del clip (braza/crol) desde hierarchy->totalLength|wasm"
  "ahogando=%d|R18 cámara de nado: el motor marca ahogado y la cámara sigue siendo la de seguir-al-ped|wasm"
  "velo=%.2f dt=|R18 velocidad de MUNDO (m/s de simulación) además del avance de reloj|wasm"
  "hondo=|R30 nado Extended: pin al pecho con puerta por profundidad (1.3)|wasm"
  "glide=|R31 nado Extended: la pose de caída no se cuela al nada (1.4)|wasm"
  "pie=%d col=%d|R32 nado Extended: quién manda en el eje vertical y quién no (1.6)|wasm"
  "PEDCLAIM cap=|R33 arbitro de capacidades: un dueño por pieza, y la traza de cada cambio (3.0)|wasm"
  "odcrouch=%d|R34 nado Extended: el agachado que se ve es el del mod, no el del motor (1.7)|wasm"
  "SWIMHUD cruz=|R34 nado Extended: delata si la puerta del HUD deja pasar la retícula (1.7)|wasm"
  "ViceExtPedOwns|R35 arbitro: las consultas son de PROPIEDAD, no de permiso (1.8)|wasm"
  "SWIMEF braz=1 splash=1 medido=|R37 nado: splash por brazada, el avance MEDIDO en m/s y el id de sonido del brazo (1.10)|wasm"
  "SWIMFOV fov=%.2f aim=%d aplica=%d modo=%d camdist=%.2f paso=%d|R37 nado: el FOV real, el boton de apuntar DE VERDAD y la distancia de camara, por flanco (1.10)|wasm"
  "SOUND_NADO_BRAZO_LS_B|R37 nado: los cuatro sonidos de brazo, del banco del propio Vice City (1.10)|wasm"
  "SWIMNAT clip=%d totalLength=|R36 nado: el totalLength REAL de cada clip y su naturalidad, que la 1.8 imprimia como constantes (1.9)|wasm"
  "SWIMERR esperada=|R30 nado Serega: traza de error solo ante desvio|wasm"
  "aim=%d law=|R30 nado Serega: axis-compat (ley de apuntado apagada nadando)|wasm"
  "PEDAT x=%.1f y=%.1f z=%.1f|R19 posición, velocidad real y camz del ped (sondas y navegación)|wasm"
  "dist=%.0f|R19c la sonda mide la distancia al agua (navegación hasta el mar)|wasm"
  "WLOAD arma=|ve37 arnés: qué modelo y diccionario del mod deja cargado el truco de armas|wasm"
  "ViceExtWeaponInfoOf|ve37 arnés: la ficha WINFO de un arma concreta (medir las 8 sin teclado)|wasm"
  "txdCargado=|ve37 arnés: el diccionario del arma está en memoria, no sólo mapeado|wasm"
  "Crouch_Roll_L|R20 agachado: los clips de costado del mod, registrados|wasm"
  "ODSTA|R2  capa on-demand: emisora y precargas|pre"
  "serega=|nado Serega: curva deriva/crucero/sprint+timeout en SWIM2|wasm"
  "P4 kind=service schema=1|P4  plan4: ancla de luces de servicio por instancia|wasm"
  "P4 kind=overflow|P4  plan4: buffer de traza desbordado (no PASS)|wasm"
  "P4 kind=firewait schema=1|P4  plan71-4c: espera previa al fuego con asociacion congelada o estado/cargador|wasm"
  "P4 kind=turners schema=1|P4  plan41: intermitentes (motivo/steer/anchorErr)|wasm"
  "P4 kind=polbike schema=1|P4  plan41: moto policial con un solo ocupante (occ/passengers)|wasm"
  "P4 kind=radio2 schema=1|P4  plan9.1(1)+10.0-C: icono de emisora al cambiar (emisora/icono)|wasm"
  "P4 kind=cheat2 schema=1|P4  plan9.1(4)+10.0-C: trucos nuevos del mod (cheat/efecto)|wasm"
  "P4 kind=uzi schema=1|P9.1(5): flanco de disparo Micro-UZI (medida previa al fix)|wasm"
  "P4 kind=cardet schema=1|P4  plan10.0-B: rueda reventada/desprendida (paso/rueda/impulso)|wasm"
  "P4 kind=cardead schema=1|P4  plan10.0-B: muerte NPC en choque duro (muertos/impulso)|wasm"
  "P4 kind=carluz schema=1|P4  plan10.0-B: faro roto por impacto (luz/impulso)|wasm"
  "P4 kind=ini schema=1|P4  plan9.2: opciones del features.ini del mod (opt/flag/a/val)|wasm"
  "P4 kind=inifile schema=1|11  familia 11: su features.ini leido al arrancar (clave/ini/aplica/origen/impl)|wasm"
  "P4 kind=worlddata schema=1|7   familia 7: mundo del mod ingerido (zonas de oclusion del occlu.ipl)|wasm"
  "ViceExtRotateSteeringWheel|plan12.0: volante que gira (v2.5) - la pieza del mod sigue la direccion|wasm"
  "steeringwheel|plan12.0: volante que gira (v2.5): el dummy que traen 18 modelos del gta3.img|wasm"
  "P4 kind=wheelrot schema=1|P4  plan12.0: volante que gira (v2.5): angulo aplicado vs direccion|wasm"
  "P4 kind=moon schema=1|P4  plan12.0: luna movil (v1.5): el disco sale del rumbo del sol|wasm"
  "P4 kind=distlights schema=1|P4  plan12.0: luces lejanas (v2.6): edificios y dummies con efectos 2D|wasm"
  "P4 kind=gangaim schema=1|P4  plan12.0: punteria de la IA (v1.0/2506): peds 73 y polis del mod|wasm"
  "P4 kind=partcfg schema=1|3/5 particulas: particle.cfg con el tipo resuelto por NOMBRE|wasm"
  "VEX limits|13.0 T1: limits.ini del mod leido al arrancar (pools)|wasm"
  "VEX ini|13.0 T1: features.ini del mod leido al arrancar|wasm"
  "VEX scm|13.0 T1: opcodes propios del main.scm del mod (4000-4099)|wasm"
  "VEX input|13.0 T2: dispositivo activo (teclado/raton o mando) y mando de la Gamepad API|wasm"
  "VEX mov|13.0 T3: agachado, rueda y escalada del mod (cambio de estado del jugador)|wasm"
  "VEX apunta|13.0 T4: apuntado del mod (bIsPlayerAiming), modo de camara y coche|wasm"
)

# Datos del mod que tienen que estar SERVIDOS (no basta con que el motor sepa
# leerlos): ruta servida|tamano (o -)|sha256 (12 hex, o -)|que es. La familia 13
# (script del mod) y la 3/5 (particulas) van por aqui: si el fichero servido
# sigue siendo el vanilla, en el log no se nota y se lee como "no hace nada".
DATOS=(
  "data/particle.cfg|18363|49130d2521a1|3/5 particle.cfg del mod (5 tipos nuevos)"
  "data/main.scm|1191180|41a932cc593e|13  script del mod (antes se servia el vanilla, 1.269.133 B)"
  "models/coll/newWeapons.col|224|-|13  colisiones de las armas nuevas (antes no estaba)"
  "ViceExtended/features.ini|567|bf884c129adb|11  features.ini del mod: ahora lo LEE el motor (familia 11)"
  "ViceExtended/limits.ini|237|94fa49646fe8|13.0 T1 limits.ini del mod (pools de NUMPEDS, NUMVEHICLES...)"
  "models/ps4btns.txd|528424|fbe233dbe34c|13.0 T2 iconos de mando DualShock 4"
  "models/ps5btns.txd|528424|c4a388d6f21d|13.0 T2 iconos de mando DualSense"
  "models/x1btns.txd|528424|1b5f1d10d468|13.0 T2 iconos de mando Xbox One"
  "models/frontend_ds5.txd|327976|c28ac979a57a|13.0 T2 diagrama de mando DualSense (menu)"
  "anim/ped.ifp|2486356|59929c978b4a|13.0 T3 ped.ifp del mod tal cual (rueda original, sin los clips del sa-crouch)"
  "models/weaponsights.txd|164776|cc5c55188368|13.0 T4 miras por arma del mod (EX_WEAPON_SIGHT)"
  "data/object.dat|26779|22b718c7796c|7  mundo del mod servido (familia 7)"
  "data/occlu.ipl|21945|e46192296f2e|7  mundo del mod servido (familia 7)"
  "data/maps/bryx/bryx.ide|111|b82b774a8958|7  mundo del mod servido (familia 7)"
  "data/maps/bryx/bryx.ipl|172|b9b2c1c424f6|7  mundo del mod servido (familia 7)"
)

# Datos del mod que NO se sirven todavia, con su motivo: no son una regresion,
# pero el fichero servido tiene que ser uno de los dos artefactos conocidos.
# ruta|sha256 (12 hex) del vanilla|sha256 del mod|que es|motivo
# 2026-10-14: vacio. El unico que estaba aqui (`data/main.scm`) ya se sirve del
# mod, asi que su comprobacion vive en DATOS, arriba.
PENDIENTE=()

if [ "${1:-}" = "--listar" ]; then
  for m in "${MARCAS[@]}"; do echo "  ${m%%|*}  (${m##*|})"; done
  for d in "${DATOS[@]}"; do echo "  servido/${d%%|*}  (${d##*|})"; done
  if [ "${#PENDIENTE[@]}" -gt 0 ]; then
    for d in "${PENDIENTE[@]}"; do echo "  servido/${d%%|*}  (pendiente: ${d##*|})"; done
  fi
  exit 0
fi

[ -f "$WASM" ] || { echo "FAIL: no encuentro $WASM"; exit 2; }
[ -f "$PRE" ] || { echo "FAIL: no encuentro $PRE"; exit 2; }

echo "== motor servido =="
ls -l --time-style=+%Y-%m-%d\ %H:%M "$WASM" "$PRE" | awk '{printf "   %10d  %s %s  %s\n", $5, $6, $7, $8}'

fallos=0
echo "== marcas de lo implementado =="
for m in "${MARCAS[@]}"; do
  marca="${m%%|*}"; resto="${m#*|}"; bloque="${resto%%|*}"; donde="${resto##*|}"
  fichero="$WASM"; [ "$donde" = "pre" ] && fichero="$PRE"
  if grep -aq -- "$marca" "$fichero"; then
    printf "   OK    %-22s %s\n" "$marca" "$bloque"
  else
    printf "   FALTA %-22s %s  <-- el paquete servido NO lleva esto\n" "$marca" "$bloque"
    fallos=$((fallos + 1))
  fi
done

# El dataTag del motor tiene que ser el último: si no, el navegador sigue con
# los datos viejos en la caché (los mp3/SDT nuevos no llegan).
dt_on=$(grep -ao "dataTag: '[^']*'" "$PRE" | head -1 | sed "s/.*'\(.*\)'/\1/")
echo "== datos =="
echo "   dataTag servido: ${dt_on:-?}  (compáralo con ondemand.js)"
# La versión que el jugador ve la pone la capa web (`web/lib/index.js`), no el
# paquete: si ahí sigue la anterior, el navegador pide el motor viejo por `?v=`.
LIBJS="web/lib/index.js"
ver_servida=""
if [ -f "$LIBJS" ]; then
  ver=$(grep -o "VERSION = '[^']*'" "$LIBJS" | head -1 | sed "s/.*'\(.*\)'/\1/")
  echo "   VERSION de la capa web: ${ver:-?}"
else
  echo "   AVISO: no encuentro web/lib/index.js"
fi

# Lo que el NAVEGADOR recibe no siempre es lo del disco: el Vite cachea la
# transformada de `lib/index.js` y sigue sirviendo la anterior. Pasó el 29/09 y
# este check dio verde con `swim16` servido y `swim17` en disco, que es peor que
# no comprobar nada. Si el servidor responde, se compara de verdad; si no
# responde, se avisa y se sigue con el resto.
PUERTO=2077
if command -v curl >/dev/null 2>&1; then
  ver_servida=$(curl -s --max-time 10 "http://localhost:$PUERTO/lib/index.js" 2>/dev/null \
    | grep -o "VERSION = '[^']*'" | head -1 | sed "s/.*'\(.*\)'/\1/")
  if [ -n "$ver_servida" ]; then
    if [ "$ver_servida" = "${ver:-?}" ]; then
      echo "   OK    el navegador recibe la misma VERSION: $ver_servida"
    else
      echo "   FALTA el navegador recibe $ver_servida y el disco tiene ${ver:-?}"
      echo "          (toca gta_vc_browser/web/lib/index.js para que lo relea el watcher)"
      fallos=$((fallos + 1))
    fi
  else
    echo "   AVISO: el puerto $PUERTO no responde; no se puede comprobar lo servido"
  fi
fi
if ! grep -aq "ve13" "$PRE"; then
  echo "   FALTA ve13 (muestras de audio re-codificadas): los sonidos nuevos no llegarán"
  fallos=$((fallos + 1))
fi

# Los datos servidos, uno a uno: tamano y sha256 contra los del mod.
echo "== datos del mod servidos =="
for d in "${DATOS[@]}"; do
  ruta="${d%%|*}"; resto="${d#*|}"; tam="${resto%%|*}"; resto="${resto#*|}"
  sha="${resto%%|*}"; que="${resto##*|}"
  f="streamed/$ruta"
  if [ ! -f "$f" ]; then
    printf "   FALTA %-32s %s  <-- no esta servido\n" "$ruta" "$que"
    fallos=$((fallos + 1))
    continue
  fi
  t=$(wc -c < "$f" | tr -d ' ')
  if [ "$tam" != "-" ] && [ "$t" != "$tam" ]; then
    printf "   FALTA %-32s tamano %s (se esperan %s)  <-- %s\n" "$ruta" "$t" "$tam" "$que"
    fallos=$((fallos + 1))
    continue
  fi
  if [ "$sha" != "-" ]; then
    h=$(sha256sum "$f" 2>/dev/null | cut -c1-12)
    if [ -z "$h" ]; then
      printf "   AVISO %-32s sin sha256sum: no se comprueba el contenido  <-- %s\n" "$ruta" "$que"
    elif [ "$h" != "$sha" ]; then
      printf "   FALTA %-32s sha256 %s (se espera %s)  <-- %s\n" "$ruta" "$h" "$sha" "$que"
      fallos=$((fallos + 1))
      continue
    fi
  fi
  printf "   OK    %-32s %s\n" "$ruta" "$que"
done

# La linea COLFILE del .col nuevo (sin ella el fichero esta pero no entra).
if grep -qi "COLFILE.*NEWWEAPONS\.COL" streamed/data/default.dat 2>/dev/null; then
  printf "   OK    %-32s %s\n" "data/default.dat COLFILE" "carga NEWWEAPONS.COL (13)"
else
  printf "   FALTA %-32s %s  <-- sin la linea COLFILE el .col no se carga\n" "data/default.dat COLFILE" "13"
  fallos=$((fallos + 1))
fi

# Los datos pendientes por decision: el servido tiene que ser el vanilla o el del
# mod (si es un tercero, es una regresion o un fichero roto).
if [ "${#PENDIENTE[@]}" -gt 0 ]; then
  echo "== datos del mod pendientes por decision =="
  for d in "${PENDIENTE[@]}"; do
    ruta="${d%%|*}"; resto="${d#*|}"; sha_va="${resto%%|*}"; resto="${resto#*|}"
    sha_mod="${resto%%|*}"; resto="${resto#*|}"; que="${resto%%|*}"; motivo="${resto##*|}"
    f="streamed/$ruta"
    if [ ! -f "$f" ]; then
      printf "   FALTA %-32s %s  <-- no esta servido (ni el vanilla)\n" "$ruta" "$que"
      fallos=$((fallos + 1))
      continue
    fi
    h=$(sha256sum "$f" 2>/dev/null | cut -c1-12)
    if [ "$h" = "$sha_mod" ]; then
      printf "   PENDIENTE %-28s ya servido el del mod (%s): %s\n" "$ruta" "$h" "$motivo"
      echo "      ^ ya no esta pendiente: pasalo de PENDIENTE a DATOS"
    elif [ "$h" = "$sha_va" ]; then
      printf "   PENDIENTE %-28s servido el vanilla (%s): %s\n" "$ruta" "$h" "$motivo"
    else
      printf "   FALTA %-32s sha256 %s desconocido (vanilla %s / mod %s)  <-- %s\n" "$ruta" "${h:--}" "$sha_va" "$sha_mod" "$que"
      fallos=$((fallos + 1))
    fi
  done
fi

echo
if [ "$fallos" -eq 0 ]; then
  echo "OK: el motor servido lleva TODAS las marcas (y los datos nuevos)."
  exit 0
fi
echo "FALLO: faltan $fallos marca(s). Enlaza el paquete:"
echo "   bash gta_vc_browser/build.sh"
exit 1
