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
  "CROUCH2|R6  agachado: peso del clip|wasm"
  "SVLIGHTS|R11 luces de servicio por dummies|wasm"
  "extras=|R11 luces de servicio: variante y extras elegidos|wasm"
  "SIRENA tipo=|SR  sirena audible: muestra que encola el motor|wasm"
  "LBAR w=|LB  contorno de la barra de carga escalado (SilentPatch 723)|wasm"
  "RECOIL_APPLY shotSeq=|R8  retroceso que mueve la CÁMARA (esquema RC: era RECOIL2)|wasm"
  "AIMDIR|R9  apuntado: desviación cámara-ped|wasm"
  "CAM1P|R5  primera persona: puerta y tecla|wasm"
  "LOADSCR begin|L6  carga de partida: una portada y una barra (sección 1)|wasm"
  "camz=|H1/H2 cámara medida agachado y nadando|wasm"
  "servicelightson|§6  nombres de luces de servicio del mod|wasm"
  "reload anim|R12 recarga a mano: animación del clip del arma|wasm"
  "repetido=|R12 agachado: antirrebote del conmutador|wasm"
  "pedido=|R12 autocentrado de coche: retorno pedido vs pasivo|wasm"
  "WINFO arma=|R13 ficha por arma: flags, canaim/witharm, reload y nombres de clip|wasm"
  "clips=|R13 los NOMBRES de clip que resuelve el grupo del arma|wasm"
  "1p bind tecla=|R13 la tecla de 1ª persona que tiene guardada el navegador|wasm"
  "por=|R13 motivo por el que se pinta la mira (SIGHT)|wasm"
  "manoH=|R13b hacia dónde apunta la mano que lleva el arma|wasm"
  "SWIMCAM|R14 nado: objetivo de cámara en la superficie|wasm"
  "teclas=|R14 autocentrado: teclas (empujón) vs ratón (suave)|wasm"
  "RECOIL3 miracheck|R14 retroceso: la mira NO se mueve|wasm"
  "ODSFXPROT|R14 sonidos de arma del mod reservados (sin cola ni reciclado)|wasm"
  "CROUCHPOSE|R15 desagacharse: la pose vuelve a la de pie|wasm"
  "R3P arma=|R16 lanzacohetes apuntando en 3ª persona (Vice Extended)|wasm"
  "crouch move spd=|R17 agachado: clip de MOVIMIENTO (la raíz avanza) y avance medido|wasm"
  "CROUCH2 h=%.2f avance=|R17 agachado: traza de avance real + distancia de cámara|wasm"
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
  "medida=1x|R19b la velocidad del agachado se mide una sola vez por frame|wasm"
  "dist=%.0f|R19c la sonda mide la distancia al agua (navegación hasta el mar)|wasm"
  "WLOAD arma=|ve37 arnés: qué modelo y diccionario del mod deja cargado el truco de armas|wasm"
  "ViceExtWeaponInfoOf|ve37 arnés: la ficha WINFO de un arma concreta (medir las 8 sin teclado)|wasm"
  "txdCargado=|ve37 arnés: el diccionario del arma está en memoria, no sólo mapeado|wasm"
  "nom=%s|R20 agachado: NOMBRE del clip que pide el motor (8 direcciones medibles)|wasm"
  "ang=%.1f|R20 agachado: ángulo del mando en el sistema del ped|wasm"
  "strafe=%d|R20 agachado: modo de control (ratón en 3ª persona = clips de costado)|wasm"
  "Crouch_Roll_L|R20 agachado: los clips de costado del mod, registrados|wasm"
  "ViceExtCrouchRate|R20 agachado: ritmo de los clips (palanca en caliente)|wasm"
  "veloc=%.2f rueda=|R22 agachado: la velocidad OBJETIVO (1,13 m/s desde R28) y el estado de la rueda se publican en CROUCH2|wasm"
  "moved=%.2f mvec=|R22 agachado: la velocidad DEL MOTOR (m_moved) se publica para medir sin depender del vaivén de la calle|wasm"
  "VICEEXT crouch roll lado=|R22 agachado: cada rueda se anuncia como evento (se cuenta cuántas hace por pulsación)|wasm"
  "pesomira=%.2f|R21 apuntar agachado: peso de la pose WEAPON_crouch del mod|wasm"
  "mira=%d gat=%d pesomira=|R21 apuntar agachado: el apuntado y su pose se publican en CROUCH2|wasm"
  "ViceExtCrouchAimPose|R21 apuntar agachado: la pose del mod se mezcla sin bIsDucking|wasm"
  "ViceExtCrouchMoveSpeed|R22 agachado: el desplazamiento usa la velocidad objetivo (1,13 m/s desde R28), no la del clip|wasm"
  "ViceExtCrouchRateFor|R22 agachado: el ritmo del clip se calcula de la velocidad (pies sin patinar)|wasm"
  "giro=%d hdgr=|R26 agachado: motivo del giro a los lados + headingRate|wasm"
  "cal=1 clipr=|R27 agachado calibrado: clips del sa-crouch servidos y raíz declarada (sin esto el verificador no da PASS)|wasm"
  "GunCrouchFwd|R27 agachado: el motor pide los dos clips del sa-crouch servidos|wasm"
  "VICEEXT crouch roll skip motivo=|R27 rueda: por qué NO salió (sin-mira/arma/disparo/bloqueada/activa)|wasm"
  "motivo=ok|R27 agachado: cada rueda sale por su evento con motivo|wasm"
  "entrada=%.0f|R27 agachado: ms desde el cambio de clip (ventana del blend 10)|wasm"
  "P4 kind=roll_end schema=1|P4  plan4: fin de rueda con maxWeight y rateIntegral|wasm"
  "GunMove_FWD|R29 movimiento apuntando: los cuatro GunMove_* del mod en el grupo de agachado|wasm"
  "gat=|R29 CROUCH2: gatillo para separar pose de apuntado y disparo (costura C18-1)|wasm"
  "ViceExtCrouchShooting|R28 agachado: el motor elige sus clips de agachado (pose de apuntar y disparo del arma) sin bIsDucking|wasm"
  "ViceExtCrouchOtherPartials|R28 agachado: presupuesto de parciales (la pose del mod cede ante el clip del arma)|wasm"
  "ViceExtCrouchRollClearPartials|R28 rueda: mientras rueda se retiran las parciales (si no, el clip de la rueda vale 0)|wasm"
  "otros=%.2f pose=%s pesoarma=|R28 agachado: presupuesto, clip de la pose y parcial del arma en CROUCH2|wasm"
  "nomarma=%s|R28 agachado: NOMBRE de la parcial del arma (*_crouchfire = disparo agachado)|wasm"
  "ViceExtCrouchSideHeading|R26 agachado: el cuerpo gira al avance a los lados|wasm"
  "ODSTA|R2  capa on-demand: emisora y precargas|pre"
  "sphOwn=%d dSph=|plan camara-coche-sin-lucha: la esfera de oclusion ya no choca con el propio vehiculo (de frente no se pega a 2,00 m)|wasm"
  "sph=%d sphM=%d|plan camara-coche-sin-lucha: CAMB2b/CAMB3b publican el impacto de las 5 esferas|wasm"
  "base=%.2f obs=%d|plan camara-coche-sin-lucha ve73: la distancia vuelve a maxDist si no hubo obstaculo (de frente ya no se pega a 2,00 m)|wasm"
  "RECOIL_SHOT shotSeq=|RC  recoil nuevo: cada disparo exitoso emite su diagnóstico (una línea por tiro)|wasm"
  "RECOIL_APPLY shotSeq=|RC  recoil nuevo: cada patada se aplica una vez a la cámara activa|wasm"
  "RECOIL_INPUT edge=|RC  recoil nuevo: flancos press/release del gatillo con fuente|wasm"
  "RECOIL_CLASS shotSeq=|RC  recoil nuevo: clasificación tap/ráfaga del SMG|wasm"
  "RECOIL_PERSIST reason=|RC  recoil nuevo: persistencia sin retorno automático|wasm"
  "follow-ped-passive|RC  recoil nuevo: FollowPed drena la cola (no retiene patadas)|wasm"
  "OD_RECOIL_SCHEMA_1|RC  recoil nuevo: esquema de telemetría acotada por sesión|wasm"
  "serega=|nado Serega: curva deriva/crucero/sprint+timeout en SWIM2|wasm"
  "AIMCFG forceauto=|AX  ClassicAXIS: los 9 ajustes de la seccion [ClassicAxis] leidos (una vez)|wasm"
  "AIMWALK tecla=|AX  ClassicAXIS: WalkKey, la tecla de andar (LALT) pone la velocidad a 0|wasm"
  "AIMLAW aim=|AX  ClassicAXIS: flanco de isAiming con el modo de camara y el auto-aim|wasm"
  "AIMCAM m=|AX  ClassicAXIS: la ley de apuntado: modo, dist 2.70, altura y hombro 0.20|wasm"
  "AIMFOV fov=|AX  ClassicAXIS: FOV 50 al apuntar por CLASE de arma (pesada: alcance >= 70 o AK47/M16/STEYR)|wasm"
  "AIMCOL los=|AX  ClassicAXIS: colisiones de la ley (LOS + 5 esferas, near-clip re-leido)|wasm"
  "AIMLOCK hx=|AX  ClassicAXIS: lock-on, la ventana de 250 ms y por que se suelta|wasm"
  "AIMIK pitch=|AX  ClassicAXIS: el brazo: pitch, torso, cabeza y brazo inferior|wasm"
  "AIHTRI h=|AX  ClassicAXIS: triangulo sobre el objetivo blando de raton|wasm"
  "AIMWPN block=|AX  ClassicAXIS: cambio de arma bloqueado con el apuntado activo|wasm"
  "AIMHUD cruz=|AX  ClassicAXIS: cruz del mod, marca de fijado por tipo y triangulo|wasm"
  "AX7 tipo=|AX  ClassicAXIS: el cuerpo sigue la marcha (WALKAROUND) y no la camara, y el 360 con el raton parado|wasm"
  "P4 kind=identity schema=1|P4  plan4: identidad version/dataTag (cadena EM_ASM del .js)|pre"
  "P4 kind=move_begin schema=1|P4  plan4: movimiento (policy, base y vector pedido)|wasm"
  "P4 kind=pose_begin schema=1|P4  plan4: postura, los 9 IDs de PLAYERCROUCH|wasm"
  "P4 kind=input_edge schema=1|P4  plan4: flancos de rueda (neutral/rearm/elegible)|wasm"
  "P4 kind=roll_start schema=1|P4  plan4: rueda por flanco y clip|wasm"
  "P4 kind=cam_begin schema=1|P4  plan4: camara, vista presentada y transferencia|wasm"
  "P4 kind=service schema=1|P4  plan4: ancla de luces de servicio por instancia|wasm"
  "P4 kind=overflow|P4  plan4: buffer de traza desbordado (no PASS)|wasm"
  "P4 kind=move_end schema=1|P4  plan4: terminal de movimiento (speedTarget/speedReal/dirErrMax)|wasm"
  "P4 kind=sight schema=1|P4  plan42: mira del mod por clase de arma al apuntar|wasm"
  "P4 kind=crouchspeed schema=1|P4  plan42: objetivo y cadencia del agachado|wasm"
  "P4 kind=move_apply schema=1|P4  plan42: que magnitud se aplico al movimiento|wasm"
  "P4 kind=pose_exit schema=1|P4  plan42: asociaciones del grupo de agachado tras desagacharse|wasm"
  "P4 kind=aim schema=1|P4  plan43: por que se apunta o se deja de apuntar (why/sprint/jump/policy)|wasm"
  "P4 kind=jump schema=1|P4  plan43: direccion del salto frente a la marcha (lr/ud/rumbo/dir)|wasm"
  "P4 kind=fire schema=1|P4  plan43: puerta del gatillo sin apuntar (aim/gat/arma/hip)|wasm"
  "P4 kind=armsight schema=1|P4  plan43: pose de brazos aplicada al apuntar (pose/errDeg)|wasm"
  "P4 kind=roll_step schema=1|P4  plan50: avance MEDIDO de la rueda frame a frame (E1)|wasm"
  "P4 kind=jump_end schema=1|P4  plan50: salto con vector real y dirErr (E5)|wasm"
  "P4 kind=stand_end schema=1|P4  plan50: tramo de pie medido, apuntando y sin apuntar (E3)|wasm"
  "P4 kind=partials schema=1|P4  plan50: parciales del arma al dejar de apuntar (E2)|wasm"
  "posenom=%s posepeso=%.2f aim=%d|P4  plan50: NOMBRE del clip de las piernas y de la pose (E6)|wasm"
  "pose=%s posepeso=%.2f errDeg=%.4f canon=%.4f px=%.1f|P4  plan50: armsight continuo con peso, canon y pixeles (E4)|wasm"
  "piesMps=%.2f|P4  plan44: pies AL SUELO (ritmo del clip x su raiz) junto al avance real (E1)|wasm"
  "pesosum=%.2f|P4  plan44: suma de pesos de las animaciones de movimiento al desagacharse (E2)|wasm"
  "canon=%.4f|P4  plan44: angulo entre el canon del arma y la direccion de la reticula (E4)|wasm"
  "det=%s|P4  plan44: detalle de la retirada de la pose (pesos y blendDelta por clip, E3)|wasm"
  "ViceExtCrouchBlend|P4  plan44: un solo reloj para agacharse y desagacharse (altura y pose)|wasm"
  "P4 kind=legs schema=1|P4  plan41: piernas sin doble angulo (bodyYaw/legYaw/legMis/skip)|wasm"
  "P4 kind=hud_cross schema=1|P4  plan41: reticula con la ley de apuntado (draw/pc/law/lock)|wasm"
  "P4 kind=turners schema=1|P4  plan41: intermitentes (motivo/steer/anchorErr)|wasm"
  "P4 kind=polbike schema=1|P4  plan41: moto policial con un solo ocupante (occ/passengers)|wasm"
)

if [ "${1:-}" = "--listar" ]; then
  for m in "${MARCAS[@]}"; do echo "  ${m%%|*}  (${m##*|})"; done
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

echo
if [ "$fallos" -eq 0 ]; then
  echo "OK: el motor servido lleva TODAS las marcas (y los datos nuevos)."
  exit 0
fi
echo "FALLO: faltan $fallos marca(s). Enlaza el paquete:"
echo "   bash gta_vc_browser/build.sh"
exit 1
