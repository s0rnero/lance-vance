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
  "WEBFT n=|20.0 tiempo por frame (min/avg/max/step)|wasm"
  "SWIM2 move|R7  nado: avance real medido|wasm"
  "SIRENA tipo=|SR  sirena audible: muestra que encola el motor|wasm"
  "LBAR w=|LB  contorno de la barra de carga escalado (SilentPatch 723)|wasm"
  "LOADSCR begin|L6  carga de partida: una portada y una barra (sección 1)|wasm"
  "camz=|H1/H2 cámara medida agachado y nadando|wasm"
  "servicelightson|§6  nombres de luces de servicio del mod|wasm"
  "pedido=|R12 autocentrado de coche: retorno pedido vs pasivo|wasm"
  "SWIMCAM|R14 nado: objetivo de cámara en la superficie|wasm"
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
  "Crouch_Roll_L|R20 agachado: los clips de costado del mod, registrados|wasm"
  "ODSTA|R2  capa on-demand: emisora y precargas|pre"
  "serega=|nado Serega: curva deriva/crucero/sprint+timeout en SWIM2|wasm"
  "P4 kind=overflow|P4  plan4: buffer de traza desbordado (no PASS)|wasm"
  "P4 kind=firewait schema=1|P4  plan71-4c: espera previa al fuego con asociacion congelada o estado/cargador|wasm"
  "P4 kind=turners schema=1|P4  plan41: intermitentes (motivo/steer/anchorErr)|wasm"
  "ViceExtIndicatorAtomicCB|42.1 T103: el nodo de giro se recorre en TODOS los atomics y materiales|wasm"
  "ViceExtIndicatorCorona|42.1 T109: destello de la lente que parpadea, anclado en su geometria real|wasm"
  "ViceExtIndicatorCoronas|42.1 T110: barrido de destellos de coches y motos, solo lente ambar medida|wasm"
  "ViceExtIndicatorLensKind|42.1 T110: ambar/propio/ninguno por nodo; sin ambar no hay destello nuevo|wasm"
  "INDCORONA k=%d g=%d/ng=%d model=%d nombre=%s|44.0 N4: traza por tramo de lente partida (grupo y total de grupos)|wasm"
  "BODYLENS model=|58.0 T3: lentes ambar de carroceria resueltas por frame, atlas derivado <atlas>_lens|wasm"
  "BODYLENSROW model=|59.0 T0/T2: una linea por fila con el frame resuelto por triangulo medido, excl=dentro/fuera y tint|wasm"
  "AMBERGATE model=|59.0 T0: gate de ambar por costado (lf rf lr rr); con lf=1 el faro blanco deja de parpadear|wasm"
  "ViceExtBodyLensExclusiveCB|59.0 T1: material de carroceria exclusivo de lente, medido en runtime|wasm"
  "ViceExtBodyLensCoronas|58.0 T3: corona por fila de carroceria en su centroide medido (banda +120)|wasm"
  "ViceExtCarLampsAlive|42.1 T110: calcinado/wrecked corta la instalacion; cada lente, su dano|wasm"
  "ViceExtBreakLamp|42.1 T110: romper solo la lupa de la pieza golpeada (newLightsData del mod)|wasm"
  "ViceExtIndGroupAdd|44.0 N4: el barrido de anclas acumula UN grupo por atomic con lente|wasm"
  "ViceExtMeasuredLensRow|44.0 N4: el color del tramo g sale de la fila g-esima medida del mismo costado|wasm"
  "ViceExtIsBodyTexture|44.0 N3: la mascara de lente tambien rige al pintar el ambient del nodo|wasm"
  "P4 kind=polbike schema=1|P4  plan41: moto policial con un solo ocupante (occ/passengers)|wasm"
  "P4 kind=radio2 schema=1|P4  plan9.1(1)+10.0-C: icono de emisora al cambiar (emisora/icono)|wasm"
  "P4 kind=cheat2 schema=1|P4  plan9.1(4)+10.0-C: trucos nuevos del mod (cheat/efecto)|wasm"
  "P4 kind=ini schema=1|P4  plan9.2: opciones del features.ini del mod (opt/flag/a/val)|wasm"
  "P4 kind=inifile schema=1|11  familia 11: su features.ini leido al arrancar (clave/ini/aplica/origen/impl)|wasm"
  "ViceExtRotateSteeringWheel|plan12.0: volante que gira (v2.5) - la pieza del mod sigue la direccion|wasm"
  "steeringwheel|plan12.0: volante que gira (v2.5): el dummy que traen 18 modelos del gta3.img|wasm"
  "P4 kind=wheelrot schema=1|P4  plan12.0: volante que gira (v2.5): angulo aplicado vs direccion|wasm"
  "VICEEXT wheel modelo=|plan12.0/15.0 T7b: el volante 3D anadido a los coches que no lo traian (modelo, estado y posicion)|wasm"
  "VICEEXT wheel tex stwheel=|plan12.0/15.0 T7b: la textura stwheel del volante, creada en memoria|wasm"
  "VICEEXT wheel sin-nodo modelo=|15.0 T7c: el volante sale de la tabla de nodos (m_aCarNodes), como en el mod|wasm"
  "VEX wanted|13.0 T7: nucleo de busqueda del mod (nivel, perdido/escondido, perseguidores, ultima vez visto)|wasm"
  "VEX limits|13.0 T1: limits.ini del mod leido al arrancar (pools)|wasm"
  "VEX ini|13.0 T1: features.ini del mod leido al arrancar|wasm"
  "VEX scm|13.0 T1: opcodes propios del main.scm del mod (4000-4099)|wasm"
  "VEX input|13.0 T2: dispositivo activo (teclado/raton o mando) y mando de la Gamepad API|wasm"
  "ViceExtTouchFrame|57.0: puente tactil exportado y enlazado en el motor|wasm"
  "VEX touch enable=|57.0: activacion/desactivacion de la capa tactil|wasm"
  "VEX touch autoaim|57.0: autoaim solo para mando o tactil|wasm"
  "VEX mov|13.0 T3: agachado, rueda y escalada del mod (cambio de estado del jugador)|wasm"
  "VEX 17 sirena|17.0 B2: sirenas del 2510 (posiciones, pareja trasera y coronas)|wasm"
  "VEX 17 menu autosave|17.0 B1: fila de autosave con la clave FEM_SL9 del GXT|wasm"
  "VEX 17 menu franja autosave|17.0 B1: la franja cian (25,150,200) que da fondo a la fila del autosave|wasm"
  "VEX 17 alfa save|17.0 B4: guardado y restaurado del alfa del cristal|wasm"
  "MATVEX atom=|17.0 B4: traza por material con el atomic que lo dibuja (interior del coche)|wasm"
  "ODDASH modelo=|36.4 T84: materiales del chassis del coche del jugador (textura, si existe, triangulos de malla vs DFF)|wasm"
  "GEOPROPS struct=|36.4 T83: geometria DFF alineada: surface properties detectadas por tamano, no por version|wasm"
  "VEX apunta|13.0 T4: apuntado del mod (bIsPlayerAiming), modo de camara y coche|wasm"
  "VEX armas|13.0 T5: segundo banco de sonido del mod (ViceEx) cargado en el motor|wasm"
  "VEX vehi|13.0 T6: familia 6 del mod (IMPROVED_VEHICLES/_2) procesando el coche|wasm"
  "CDSKIP web:|web: el aplazamiento on-demand no se pinta como disco roto ni congela el juego|wasm"
  "VEX pedvar|13.0 T6 familia 8: rotacion de la variante de ropa del txd (*remap*/*shade*) en cada aparicion|wasm"
  "VEX hud|13.0 T9: triangulo de salud del objetivo apuntado (9.08) y estado de coleccionables (9.03)|wasm"
  "VEX mundo|13.0 T8: familia 8 del mod (zonas de oclusion, luces lejanas, Bryx y luna movil)|wasm"
  "VICEEXT spec power=|15.0 T15: el especular Blinn por luz, coloreado con el color de la luz (el del retail de D3D8)|wasm"
  "VEX sp |14.0 T1: SilentPatch (retardo del chat de los peds, puas sin pool)|wasm"
  "VEX ws |16.0 T1: WidescreenFixesPack (opciones de HUD, radar, FOV, bordes y texto)|wasm"
  "VEX sky|16.0 S1: SkyGfx (env map, pipe de mundo, pipes Leeds, trails y gotas)|wasm"
  "MESHBAD meshes=|20.0 T15: malla corrupta (cabecera imposible) se salta en vez de tirar la pestaña|wasm"
  "MESHOVER total=|30.0 T54: la malla pide mas indices de los reservados; se recorta en vez de escribir fuera del buffer|wasm"
  "ODSFXNO sfx=|20.0 T15: muestra de audio que no estaba al pedirla (diagnostico de sonido mudo)|wasm"
  "ODINIT7 pend=|20.0 T16: el paso 7 del init dice si quedaron peticiones pendientes|wasm"
  "ANIMNOGROUP grupo=|20.0 T16: grupo de animacion sin clump (modelo no cargado) se salta, no revienta|wasm"
  "ODWKTMO |20.0 T17: espera al worker vencida (no puede congelar el juego para siempre)|pre"
  "ODWKDEAD |20.0 T17: worker muerto: sus esperas se cierran en el acto|pre"
  "VEX listaciclo|21.1 T19: el recorrido de la lista de entidades no recicla nodos ni da vueltas|wasm"
  "VEHDOOR modelo=|40.0 T92: puertas del coche cargado desde el guardado (estado y atomics del dummy)|wasm"
  "VEHDOOR2 atomic=|40.0 T92: descarte por angulo de la puerta del coche del jugador (dot/flags/dist)|wasm"
  "WEBLOOP vsync=|40.0 T92: reloj efectivo del bucle (vsync/menu/cap/limit/wait)|wasm"
  "DIRTAG |47.0 T1: el indice .dir que el motor lleva dentro (n/total/hash)|wasm"
  "HUDGATE wants=|47.0 T1: quien abre y quien cierra el porton del HUD|wasm"
  "DRUNKSET val=|47.0 T1: el script y la IP que ponen la embriaguez|wasm"
  "DFFGARBAGE |47.0 T1: BIN que declara mas bytes que la rebanada leida|wasm"
  "RETRYCAP |47.0 T1: tope de reintentos por streamId|wasm"
  "HORNHIST mps=|48.0 T2: anillo del claxon muestreado a 30 Hz (sirenas con doble toque)|wasm"
  "RELOJ sim=|48.0 T5: minutos de juego vs tiempo real; la deriva de los hitches|wasm"
  "EMPUJE who=|48.0 T6: quien recibe el empuje coche<->peon y con que escala|wasm"
  "comp=%.3f pednom=|48.0 T7: compensacion del asiento por altura del ped y su clave|wasm"
  "SPLASH dur=|49.0 T16: gotas de lente, un rearm por chapotazo y duracion en ticks logicos|wasm"
  "ODLOAD id=|49.0 T14: que se iba a parsear cuando aborta el monton (crash del Hunter)|wasm"
  "TXDBAD ntext=|49.0 T14: cota de texturas por TXD, rechazada antes del malloc|wasm"
  "ODTXD slot=|49.0 T14: TXD grandes y sus recuentos, con nombre|wasm"
  "COLBAD|54.0 T1: colision rechazada por corrupcion|wasm"
  "COLBIG|54.0 T1: colision valida que no cabe en work_buff|wasm"
  "COLMEM nom=|54.0 T2: peticion de cada array de colision y reparto|wasm"
  "COLMEMFAIL nom=|54.0 T2: RwMalloc devolvio nil en colision|wasm"
  "COLCTX slot=|54.0 T3: slot y nombre en la carga de colision|wasm"
  "COLFAIL slot=|54.0 T3: fallo de carga de colision con contexto|wasm"
  "TXDFREE slot=|54.0 T4: TXD liberado con bytes vivos|wasm"
  "TXDLOAD slot=|54.0 T4: recarga de un TXD ya contado|wasm"
  "txdb=|54.0 T4: bytes vivos de TXD en WEBHB|wasm"
  "ODTEXEV id=|54.0 T5: modelo/TXD/anim desalojado del streaming|wasm"
  "evict=%d fail=%d freed=|54.0 T5: resultado del desalojo de 512 MB|wasm"
  "HEAPGROW antes=|54.0 T6: sonda de memory.grow|wasm"
  "used=%uMB free=%uMB|54.0 T6: heap pedido/libre en WEBHB|wasm"
  "TXCONV txd=|54.0 T5b: TXD/textura/raster antes de convertir|wasm"
  "TXDBAD where=|54.0 T5b: raster invalido rechazado antes de rwMalloc|wasm"
  "NEUMATICO fwd=|49.0 T12: medida del neumatico (fwd/right/adhesion por tick)|wasm"
  "sfov=%.1f zoom=%.3f mode=|50.0 T18: el CUTSKIP nombra el FOV y el modo que deja el salto|wasm"
  "PSA ev=|50.0 T19: armar/rendir el anuncio de radio (openfail, watchdog, deadstation)|wasm"
  "2026-11-07-43.1-T1|43.1 T1: dataTag servido con el rotulo FES_ISK en los 6 idiomas|pre"
  "FES_ISK|43.1 T1: boton Importar apariencias en la configuracion de apariencia|wasm"
  "ViceExtRescanSaves|43.1 T1: refresco de ranuras tras importar una partida|wasm"
  "FEM_GSK|53.0 T1: confirmacion nativa de importar partidas|wasm"
  "FEM_SKO|53.0 T1: confirmacion nativa de importar apariencias|wasm"
  "ViceExtRescanSkins|43.1 T1: re-escaneo de skins tras subir apariencias|wasm"
  "__vcSkinImport|43.1 T1: gancho EM_ASM del importador de apariencias|pre"
  "SKIN rescan|fix 11/11: re-escaneo de skins dentro del frame del motor|wasm"
  "SKIN borrada|fix 11/11: apariencia borrada por el jugador|wasm"
  "FES_DSK|fix 11/11: boton Eliminar apariencia junto a Importar skins|wasm"
  "MUS rescan|fix 11/11: re-escaneo de musica dentro del frame del motor|wasm"
  "FEM_MOK|fix 11/11: menu de musica importada|wasm"
  "pos=%.3f|fix 11/11: el coche aparta al peon contra el que avanza|wasm"
  "__vcWebSkinRescan|fix 11/11: bandera del re-escaneo de skins que consume el motor (literal EM_ASM, glue desde emcc 6.0.9)|pre"
  "INI leido|fix 11/11: opciones leidas del ini al arrancar|wasm"
  "INI boot save|fix 11/11: el ini de arranque solo se reescribe con lo ya leido (literal EM_ASM, glue desde emcc 6.0.9)|pre"
  "FEM_GAL|43.0 T1: pagina nativa de galeria en Configuracion extra|wasm"
  "FEM_MUS|43.0 T1: pagina nativa de musica en Sonido|wasm"
  "FET_GSP|43.0 T1: pagina nativa de partidas en Iniciar partida|wasm"
  "FES_QGF|43.0 T1: confirmacion nativa antes de borrar la galeria|wasm"
  "ViceExtRescanUserMusic|43.0 T1: re-escaneo de /MP3 expuesto al shell|wasm"
  "__vcSaveDownload|43.0 T1: ganchos EM_ASM de las tres gestiones|pre"
  "SPLASH sitio=|49.1 T23: chapuzones del jugador con reloj unico (entrada, seco, someras)|wasm"
  "VUELO z=|49.1 T25: techo de vuelo medido (z, vz, empuje, factor y fps en aereos)|wasm"
  "DRUNKW src=|49.1 T24: quien escribe la embriaguez y con que valor|wasm"
  "FOVLN |49.1 T21: el FOV escalado cuadro a cuadro en la frontera cinematica->juego|wasm"
  "PART ida=|49.1 T26: hito de partida nueva del motor, el que parte los logs|wasm"
  "ODNOFETCH |49.1 T22: la peticion del anuncio que no llego a MEMFS (path, canon, motivo)|pre"
  "SWIMCLIMB ok=|49.2 T34: salir del agua trepando, con el climbing que ya portamos|wasm"
  "SWIMHOP ok=|49.2 T34: el subidito al reborde, con las cotas medidas en el binario del mod|wasm"
  "SWIMRAMPA sw=|49.2 T34b: la rampa de mezcla de la brazada, muestreada cada 50 ms al entrar|wasm"
  "SWIMBORDER guard=|52.0: la puerta del agua medida sobre los pies, con su motivo|wasm"
  "2026-11-08-53.0-T1|53.0 T1: dataTag con los GXT de opciones extra y las confirmaciones nativas|pre"
  "no-cruce|51.0 T3: IA de cruce del trafico (nodo con >=3 enlaces, angulo del mod, aproximacion)|wasm"
  "parqueo-apagando|51.1 T1: el NPC que aparco con las luces puestas las apaga al volver a andar|wasm"
  "2026-11-08-56.0-T3|56.0 T3b: dataTag con la tabla de lentes regenerada y los nodos movidos|pre"
  "IsInteriorlessVehicle|56.0 T4: sin 1a persona en los modelos sin interior medido|wasm"
  "ViceExtSkipFirstPersonZoom|56.0 T4: el ciclo de camara se salta el hueco de 1a persona|wasm"
  "ViceExtLightShadowOffset|56.0 T5: el charco de luz del suelo anclado a su lampara|wasm"
  "2026-11-08-56.0-T7|56.0 T7: dataTag con los nodos traseros creados en fbiranch y fbicar|pre"
  "2026-11-08-56.0-T8|56.0 T8: tanda de direccionales revertida, solo quedan las dos filas del benson|pre"
  "2026-11-08-58.0-T3|58.0 T3: dataTag con los atlas de lente de carroceria (esperant/boxville/fbiranch/mule/trash) inyectados|pre"
  "2026-11-08-58.0-T4|58.0 T4: dataTag con el material de lente partido por costado (slot izquierdo/derecho) y la tabla regenerada|pre"
  "2026-11-09-58.1-T1|58.1 T1: dataTag con atlas de cara completa (topfun/rumpo/pony), mrwhoop de carroceria, zebra re-parentado y trash sin fila inventada|pre"
"2026-11-09-62.0-T1|62.0 T1: dataTag con el cristal en slot propio (mrwhoop/rumpo/pony/topfun/boxville/esperant/fbiranch/fbicar) y las filas inventadas fuera|pre"
"2026-11-09-62.1-T1|62.1 T1: dataTag con fbicar revertido y el gate del nodo (sin fila ambar no hay ambient)|pre"
"2026-11-09-62.2-T2|62.2 T2: dataTag con el cristal por matiz+saturacion y el fbicar copiado del washing|pre"
"2026-11-09-62.3-T2|62.3 T2: dataTag con la huella completa del cristal declarado y la corona de las filas de carrocer�a|pre"
"2026-11-09-62.2-T1|62.2 T1: dataTag con las UVs del atlas del Banshee alineadas contra el PC (T7)|pre"
"2026-11-10-63.0-T1|63.0 T1: Banshee completo de The Hero 1:1 (16 atomics, 9 materiales, TXD 32 bits)|pre"
  "ODBUILD id=2026-11-10-63.0-T1|63.0 T1: marca de build en la traza|wasm"
  "RGPASS schema=2|63.0 T1: colector de coste de frame en ms reales|wasm"
  "PEDCLUMP schema=1|63.0 T1: observador del clump del peaton|wasm"
)

# Datos del mod que tienen que estar SERVIDOS (no basta con que el motor sepa
# leerlos): ruta servida|tamano (o -)|sha256 (12 hex, o -)|que es. La familia 13
# (script del mod) y la 3/5 (particulas) van por aqui: si el fichero servido
# sigue siendo el vanilla, en el log no se nota y se lee como "no hace nada".
DATOS=(
  "data/particle.cfg|18363|49130d2521a1|3/5 particle.cfg del mod (5 tipos nuevos)"
  "data/main.scm|1191180|41a932cc593e|13  script del mod (antes se servia el vanilla, 1.269.133 B)"
  "models/coll/newWeapons.col|224|-|13  colisiones de las armas nuevas (antes no estaba)"
  "ViceExtended/features.ini|567|5bb030d48ff5|11  features.ini del mod: ahora lo LEE el motor (familia 11)"
  "ViceExtended/limits.ini|237|94fa49646fe8|13.0 T1 limits.ini del mod (pools de NUMPEDS, NUMVEHICLES...)"
  "models/ps4btns.txd|528424|fbe233dbe34c|13.0 T2 iconos de mando DualShock 4"
  "models/ps5btns.txd|528424|c4a388d6f21d|13.0 T2 iconos de mando DualSense"
  "models/x1btns.txd|528424|1b5f1d10d468|13.0 T2 iconos de mando Xbox One"
  "models/frontend_ds5.txd|327976|c28ac979a57a|13.0 T2 diagrama de mando DualSense (menu)"
  "anim/ped.ifp|2486356|59929c978b4a|13.0 T3 ped.ifp del mod tal cual (rueda original, sin los clips del sa-crouch)"
  "models/weaponsights.txd|164776|cc5c55188368|13.0 T4 miras por arma del mod (EX_WEAPON_SIGHT)"
  "Audio/sfx.SDT|199080|8a52e71aa0af|13.0 T5 tabla de efectos con el banco ViceEx detras (ids 9941..9953, frecuencia real del mp3)"
  "Audio/sfx/9941.mp3|2205|4a248e981d25|13.0 T5 primera muestra del banco ViceEx (ViceEx.RAW recortado a mp3)"
  "Audio/sfx/9953.mp3|8195|c19feea75474|13.0 T5 ultima muestra del banco ViceEx"
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

# El dataTag del motor tiene que ser el ultimo: si no, el navegador sigue con
# los datos viejos en la cache (los mp3/SDT nuevos no llegan). Se aceptan las
# dos formas de comilla: el enlazador de Emscripten reescribe las literales del
# `--pre-js`, y con la sonda puesta (SAFE_HEAP) el paquete sale con comillas
# dobles mientras el fuente la tiene simple. Con el patron viejo esto salia
# `dataTag servido: ?` y daba FALTA sin serlo.
dt_on=$(grep -ao "dataTag: [\"'][^\"']*[\"']" "$PRE" | head -1 | sed -E "s/.*dataTag: [\"']([^\"']*)[\"'].*/\1/")
dt_src=$(grep -ao "dataTag: [\"'][^\"']*[\"']" web/ondemand.js | head -1 | sed -E "s/.*dataTag: [\"']([^\"']*)[\"'].*/\1/")
echo "== datos =="
echo "   dataTag servido: ${dt_on:-?}  (compáralo con ondemand.js)"
# `ondemand.js` entra al paquete por `--pre-js`, y CMake NO lo apunta como
# dependencia: cambiar solo su dataTag deja `ninja: no work to do` y el navegador
# sigue con el tag viejo, que es justo lo que este check tiene que evitar. Pasó el
# 05/11 con el 45.0 (27 DFF servidos con `42.1-T109` vivo).
if [ -n "$dt_src" ] && [ "$dt_on" != "$dt_src" ]; then
  echo "   FALTA el paquete lleva ${dt_on:-?} y ondemand.js $dt_src"
  echo "          fuerza el relink (toca un .cpp y vuelve a lanzar build.sh)"
  fallos=$((fallos + 1))
fi
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
