// GTA Vice City (reVC) en el navegador, como librería.
//
// Uso mínimo (Vue, React, TS o HTML suelto):
//
//   import { startGame } from '@re3/gtavc-web';
//   const game = startGame({ el: document.getElementById('vc') });
//
// La función monta el juego DENTRO del div que le pases: crea el canvas, la
// barra de progreso rosa pegada al top del viewport, y arranca el motor sin
// pedir clic (el audio se desbloquea solo con el primer gesto del usuario).
//
// El motor (wasm) y los datos NO van en el paquete: se sirven por HTTP y aquí
// solo se apunta dónde están (buildUrl / streamedUrl / manifestUrl / assetsUrl).
import { CSS, STYLE_ID } from './styles.js';
import { createTouchControls, touchDetected } from './touch.js';
import { downloadGame } from './download.js';
import { downloadAllGallery, clearGallery } from './gallery.js';
import { downloadSlot, downloadAll, importSavesFromPicker } from './saves.js';
import { importMusic, deleteAllMusic, downloadAllMusic, musicInfo, rescanUserMusic } from './music.js';
import { restoreSkins, importSkinsFromPicker, skinInfo, deleteSkin } from './skins.js';

// Tag de esta ronda: sale en la consola, en el título y en las trazas.
// Sirve para saber QUÉ código corre (adiós confusión de cachés).
// ve12: código de la sección 3 (C3.5 luces rompibles al disparo + `reload done
// manual=1` + `CAM1P` sin ruido + NPC en los intermitentes). Sólo etiqueta: los
// DATOS no cambian y `dataTag` sigue `ve10`.
// ve14: reenlace del 20/09 14:41 con la sección 3 del PLAN v2 **completa** (que
// ve13 se quedó en el enlace de las 14:20): 1ª persona con V + autocentrado de
// cámara en vehículo + ranura de autoguardado primera/rotulada/protegida +
// `reload key`/`motivo` en todas las salidas + depósito que EXPLOTA (con
// respaldo por caja de colisión para los modelos sin `petrolcap`) + nadar (C4),
// agachado (C5), esprint con arma de 2 manos (C6) y apuntar con la escopeta (C7).
// Sólo etiqueta: sigue sin cambios de datos, `dataTag` = `ve10`.
// ve15: sección 1, 21/09 — sonidos de las 8 armas nuevas del mod (13 muestras
// de su banco, ids 9941..9953 en el banco del port), corte de `CARRATE`
// rehacido y +20 m de modelo bueno antes de que entre el LOD (D10).
// Con CAMBIO DE DATOS: los 13 mp3 nuevos y `Audio/sfx.SDT` ampliado, así que
// `dataTag` sube a `ve11`.
// ve16: sección 1, 21/09 — cierre de los pendientes de la sección: la
// frecuencia de los vehículos del mod vuelve a SUS valores (10 y 7; el 100 era
// temporal para verlos circular, ya visto), con el cargador ponderado que se
// queda. Cambia DATO ⇒ `dataTag` a `ve11`→`ve12`.
// ve17: sección 3, 21/09 (7ª partida del jugador, "todas mentiras") — el fallo
// que más dolía era invisible en el log porque la traza decía una cosa y el
// juego hacía otra. Lo que cambia el CÓDIGO (los datos siguen en `ve12`):
//   * la tecla del conmutador de 1ª persona (V) y la del agachado (C) se leen con
//     **respaldo**: la config de controles que el navegador guardó con un build
//     anterior deja esas acciones SIN tecla (`rsNULL`) y la tecla no llegaba al
//     motor (mismo fallo que ya se corrigió en la recarga con R);
//   * el autocentrado de cámara estaba escrito SÓLO en la cámara LCS, que no es
//     la que se juega (`bFreeCam` nace apagado): ahora también está en la cámara
//     de coche de serie; y el detector de "estoy mirando" pasa de `delta != 0`
//     (con ratón apoyado siempre da "mirando") a una ventana de ~1 s;
//   * el agachado deja de ser el de DISPARO del motor (que clava al jugador en
//     el sitio): ahora permite andar agachado, con los clips del mod;
//   * el nadado reconoce la CAÍDA al agua (antes sólo valía con control a pie,
//     así que el ped caía al vacío sin estado de nado);
//   * el depósito de gasolina sólo explota con el impacto EN el tanque
//     (0,22 m del tapón / el cuadro del respaldo) y en las motos hace falta el
//     tanque real, no la caja;
//   * la lista de CARGAR vuelve a tener 8 filas: autoguardado + ranuras 1..7.
// ve18: sección 1, 21/09 (8ª partida) — **"las armas nuevas sonaban mudas"**: el
// motor descartaba sus 13 muestras. `InitialiseChannel` sólo reproducía ids <
// `SAMPLEBANK_MAX` (el fin de la tabla SDT ORIGINAL) y las del mod son
// 9941..9953 ⇒ caían en la rama de comentarios de ped y devolvían FALSE, sin
// dejar ni una traza (medido: 40 disparos `VICEEX sfx arma=51 sample=9943` y
// cero `ODSFXMISS sfx=9943`). Ahora se sirven por el camino on-demand, y las
// muestras se re-codifican con su frecuencia REAL (el banco del mod declara
// 36000/33000/22000 Hz, que no existen en MP3) ⇒ `dataTag` sube a `ve13`.
// Dato nuevo para no repetirlo: `tools/viceex-sfx-check.py` (tabla + fichero +
// formato + rango del motor) y el bloque D8 de `tools/viceext-log-check.py`, que
// detecta "disparó y no arrancó" = arma muda.
// ve19: 21/09 (9ª partida) — **el paquete NUNCA se había enlazado**: el jugador
// estuvo varias partidas con el `.wasm` del 21/09 08:38 (build `ve17`), así que no
// podía ver nada de lo hecho hoy (R5-R11, D8b, R2/R3 de la capa web). Lección y
// regla nueva: antes de pedir una partida, `tools/check-served-build.sh` dice si
// el motor servido lleva las marcas de lo implementado. Este build las lleva
// todas.
// ve20: 21/09 (sección 1) — pantalla de carga de partida: UNA sola portada y UNA
// sola barra progresiva. Antes cada fase (shutdown, parse, drenado, escena)
// reescribía `gWebLoadFrac` desde abajo y las pantallas vanilla pedían portada
// aleatoria (`loadscN`), con lo que el splash se recargaba a media carga y el
// arte cambiaba ("veo la portada del juego"). Ahora `WebBeginLoadScreen()` fija
// `splash1` y enciende `gWebLoadScreenActive`: `LoadSplash` ignora otro nombre y
// `LoadingScreen` delega en `WebDrawLoadScreen`, que además es monótono (nunca
// retrocede) y no se re-entra. Traza: `LOADSCR begin/end` y `LOADSCR clamp`
// (frontera de fase que intentaba volver atrás; el recorte deja la barra en un
// solo barrido 0->100 y el log lo demuestra).
// ve22: 21/09 (parte B, subagente 3) — H1 cámara del agachado (−0,55 + `camz=`),
// H2 nado (margen 0,25, velocidades en m/frame /50 y cámara a la superficie),
// H3 apuntado (snap del rumbo en el control con ratón + `peso` del
// CROUCHFIRE) y §6 luces de servicio (el fallo ya no se cachea y se buscan
// todos los nombres del `.dff`). Enlazado junto a lo de la sección 1: build
// único con las dos partes. Bloque `H` nuevo en `tools/viceext-log-check.py`
// (agachado: `camz`; nado: `avance` real y salidas del agua).
// ve23: 21/09 (sección 1, 9ª partida) — CAUSA RAÍZ del apuntado y del control
// con ratón: `CPed::CanStrafeOrMouseControl` devolvía false SIEMPRE porque
// `bFreeCam` (que en este port es la cámara moderna, no la libre de depuración)
// la cortaba; con false, el cuerpo no encaraba la cámara (`AIMDIR desv` hasta
// 86°), el pase a pie con ratón no corría y la 1ª persona quedaba bloqueada
// (`1p set ... free=1`). Además: conmutador de 1ª persona sin `bFreeCam` en su
// puerta, recarga a mano con animación (`reload anim`), agachado sin doble
// pulsación (`repetido=`), autocentrado de coche suave + empujón al soltar la
// mirada (`camauto2 ... pedido=`) y apuntado del lanzagranadas.
// ve24: 21/09 (sección 1, 10ª partida) — LA SIRENA DE LAS PATRULLAS. Causa
// raíz encontrada por datos (el `police.dff` servido se parseó entero): los
// `servicelights_1..3` del mod son HIJOS de `extra1..3`, y `PreprocessHierarchy`
// saca esos extras del clump; `CVehicleModelInfo::CreateInstance` clonaba SÓLO
// el atomic del extra, así que los hijos (las lentes de la barra) nunca
// llegaban a la instancia: barra sin luces y `FindDummyFrame` sin encontrar
// nada (`SVLIGHTS dummies=0` en toda partida). Ahora se clona el subárbol
// completo del marco del componente (con el NOMBRE del marco copiado a mano:
// `RwFrameCreate` no lo trae). Además las coronas ya no se anclan al dummy
// (está en (0,0,0): la malla se coloca por vértices) sino al centro de la
// **esfera envolvente** de la malla de las luces ± su radio, en el eje derecho
// del coche; la caché pasa a ser por (modelo, extras elegidos) porque cada
// patrulla lleva una de las tres variantes de barra. Traza con `var=` y
// `extras=` y bloque nuevo `SR` (sirena audible) en el verificador.
// ve25: 21/09 (10ª partida, plan de mods) — primer arreglo portado de
// SilentPatch: el CONTORNO de la barra de carga no escalaba con la resolución
// (`hpos-1.0f`/`top-1.0f` eran 1 px literales). Ahora usa la misma escala que la
// barra (`SCREEN_SCALE_X/Y(1.0)`); traza `LBAR w= h= borde=` y bloque `LB` del
// verificador. Manifiesto de atribución de los tres mods en
// `docs/mods/ATTRIBUTION.md`.
// ve26: 21/09 noche (11ª partida) — el jugador: "vi la v25 y todo sigue igual".
// El log de su sesión (`04-04-07`, 2 min) lo explica: NO hay una sola línea
// `VICEEXT 1p key` ni `v=1` en `CAM1P`, o sea que la 'V' no llegó al motor en
// toda la sesión; y `AIMDIR` ya sale `desv=0.0` en las cinco armas nuevas (el
// cuerpo SÍ encara la cámara: el arreglo anterior funciona). Se atacan las tres
// causas que quedaban sin dato:
//  - R13: la 'V' se lee SIEMPRE por la acción Y por la tecla cruda. La config de
//    controles guardada en el navegador puede tener la acción con OTRA tecla
//    (cuando el hueco era la `B`), y entonces ni la V ni el respaldo la veían.
//    Traza nueva `VICEEXT 1p bind tecla=` (una por sesión) con lo que hay guardado.
//  - `WINFO` (una línea por arma que el jugador saca): flags reales de
//    `weapon.dat`, `canaim`, `witharm`, `reload`, `sight` y los NOMBRES de clip
//    que el grupo resuelve para disparo/agachado/recarga. Es lo que faltaba para
//    decir si "no hay animación de apuntado/recarga" es un flag o un clip ausente.
//  - `AIMDIR` añade `tgt=` y `ik=`, y `SIGHT` añade `por=` (1=conduciendo,
//    2=modo de cámara, 3=apuntando) para saber por qué se pinta la mira.
// ve27: 22/09 (12ª partida, vídeos `pc.mp4` + `browser.mp4`) — vuelta al
// diagnóstico con DATOS y tres causas raíz nuevas:
//  - R14 CÁMARA DE NADO: `CCam::IsTargetInWater` daba verdadero nadando (el ped
//    va unos centímetros bajo la superficie) y el motor pedía
//    `MODE_PLAYER_FALLEN_WATER`: la cámara de "jugador que se ahoga", clavada en
//    la última posición sobre el agua. El vídeo del navegador lo enseña tal cual
//    (la calzada desde arriba y Tommy fuera de cuadro) y el log también
//    (`SWIM2 camz=12.02` constante con el ped a z=5.4 sobre nivel 6.1). Ahora,
//    nadando, ese predicado devuelve falso y la cámara sigue al ped. Traza nueva
//    `SWIMCAM objetivo= nivel= cam= modo=` (1/s nadando).
//  - R14 AUTOCENTRADO DE COCHE: el flanco de soltado que disparaba el empujón de
//    2,2 rad/s miraba también el RATÓN (`camauto2 pedido=1` decenas de veces por
//    minuto con `vel=0.0`). El ratón sólo reinicia el temporizador del retorno
//    suave; el empujón rápido queda para cruzeta/palo. El retorno pasivo pasa a
//    ser proporcional al error (arranca suave y frena al llegar).
//  - R14 RETROCESO: la retícula deja de moverse (`m_f3rdPersonCHairMultY` vuelve
//    a quedarse en 0,400) y el retroceso lo acusa la CÁMARA, con patada visible
//    en todas las armas (0,30° SMG … 1,40° francotirador). Traza `RECOIL3
//    miracheck multY=` para demostrar que la mira no se mueve.
//  - H4 CAMINADO APUNTANDO: el clip lo elige la velocidad SIN tope (el de andar
//    normal) y la cadencia se escala con el cociente real: mismo caminado, más
//    lento, sin patinar.
//  - R14 SONIDOS DE ARMA: las 13 muestras del mod se reservan (no se posponen ni
//    se reciclan) al primer disparo de cada arma; el reparto de decodes sube a 3
//    inline y 2 en cola por frame. Traza `ODSFXPROT sfx=`.
//  - R28 AGACHADO CALIBRADO (crouch2): el agachado va a la MISMA velocidad que
//    el andar de pie (1,13 m/s, era 0,90), la rueda sale con A/D (también en
//    diagonal) manteniendo apuntar, el disparo agachado usa el clip de agachado
//    del ARMA (`colt45_crouchfire`) y la pose del mod cede ante él: dos poses
//    parciales a la vez eran el cuerpo estirado/aplastado y la "lucha entre 2
//    animaciones". Traza: `CROUCH2 ... otros= pose= pesoarma= nomarma=`.
// OJO: `VERSION` va TAMBIÉN en la URL del motor (`reVC.js`/`reVC.wasm` con
// `?v=VERSION`): cambiarla es lo ÚNICO que fuerza al navegador a bajar el .wasm
// nuevo. Un build enlazado con el mismo tag se sigue sirviendo de la caché (pasó
// el 23/09: dos corridas del arnés midieron el binario anterior y salían "sin
// pesomira en CROUCH2" con el build bueno en disco). Subir el tag en CADA build.
// T55: barra de carga real por paso del init de partida nueva y FOV de
// cinemática convertido al aspecto actual (antes salía estirada).
// T103 (plan 42.1): el nodo de giro se recorre en TODOS los atomics y materiales
// (la lente real de 11 modelos está repartida) y las motos sin nodo `indicator_*`
// parpadean con su faro/piloto como los coches. Datos: 66 DFF reconstruidos desde
// la base limpia y `gta3.dir` reempaquetado (13 entradas crecen, 25 encogen).
// T108: los bucles de OpenAL reescriben sus puntos de loop recortados al buffer
// real (antes heredaban la región del sample anterior y sonaban repetidos) y el
// `ResetTimers` corta los canales activos; un ped sentado ya no arrastra el
// fundido de entrada, que lo desviaba por el pase alfa y el asiento se lo comía.
// T109 (plan 42.1): `IndicatorLensTable.h` vuelve a medir el árbol servido tras
// reconstruir 18 DFF desde la base limpia (306 filas: 176 ámbar, 84 rojas, 46
// blancas). El ámbar de `rumpo`/`pony` vuelve porque era nuestro, no del mod;
// `boxville`/`mrwhoop` delante pasan a su color propio.
// T110 (plan 42.1, reporte del jugador de 3/11): tres correcciones de motor.
// El daño global (`m_fHealth`) ya no apaga los faros — solo el calcinado, el
// wrecked y el estado de LA ESENA (dañar una puerta deja alumbrar). La tabla
// lleva el veredicto ambar como dato (`amber` por fila: 176 sí / 130 no), asi
// que un nodo sin ambar parpadea con SU propio faro/piloto (ruta Banshee) y no
// se inventa ningun destello rojo ni blanco nuevo. El ancla del destello se
// mide con la LTM de la raiz del jerarquia (cancela el transform del mundo que
// se colaba en la primera medida) mas un filtro de plausibilidad, lo que arregla
// los coches que solo destellaban a la izquierda (cheetah); y las motos/faggio
// reciben su propio barrido de destellos, que antes no existia porque la ruta
// vivia solo en `CAutomobile::RenderLights`.
// T110b: la puerta de plausibilidad del ancla sale de medir las 162 celdas ambar
// del arbol servido (|y| hasta 6.90 en el packer, 19 lentes de UN triangulo con
// spread 0): con la caja de 6.0 y el corte por debajo de 0.01 se caían 23
// destellos. Y el daño deja de apagar los dos faros con cualquier golpe de
// parachoques: el mod solo rompe la lupa que cuelga de la pieza golpeada
// (`newLightsData`, 6 modelos con faro en el parachoques, 14 con luces en el ala;
// el resto cuelga del chasis).
// T111 (rendimiento/estabilidad, reporte del jugador de 3/11):
// El LOD de los edificios se quedaba pintado para siempre porque el corte vanilla de
// busqueda de disco (`m_loadingPriority` + cola > 5/10) jamas se despeja en la web: el
// backlog cronico del worker deja `prio=1` (153/153 sondas CARBLOCK) y los gemelos
// hi-res nunca se pidian. En `__EMSCRIPTEN__` esos dos cortes pasan a ser solo un tope
// de cola (32) y `LODLEFT` se sustituye por `LODSTUCK`/`LODHEAL`, que miden cuanto dura
// el enganche y prueban si san. Audio: el reloj de rachas del vigilante de loops se
// pisaba cada frame (dursec siempre 0, por eso LOOPORPHAN/KILL/COLLIDE salian a 0) y
// ahora un loop de mas de 8 s se traza con su numero de sample (`LOOPLONG`).
// T113 (estabilidad, crasheo del jugador del 4/11 a las 15:23:52Z):
// El fallo no era falta de memoria (heap0=480 MB de 2 GB, y la sesion siguiente sin
// crash subio a 499 MB) sino el heap roto: la firma `memory access out of bounds` en
// `emscripten_builtin_malloc` es cronica desde el 10/10 (8234 fallos) y la funcion donde
// muere va cambiando (`CRunningScript::Load`, `RpAtomicGetGeometry`, `CSprite`, `memchr`),
// o sea que cae el siguiente que toca el heap, no el que lo rompio. La guardia GEOSTRUCT
// del 24/10 no disparo ni una vez (0 filas), asi que el DFF pasaba el filtro y corrompia
// despues: `numTexCoordSets` llega a 255 contra un `texCoords[8]` fijo, `addMorphTargets`
// reservaba `n*sz` en int32 (desborda y el bucle de inicializacion rebasa el bloque) y el
// lector de morph targets escribia 12*numVertices bytes en la direccion 0 cuando el DFF
// decia `hasNormals` sin traer la bandera NORMALS. Los tres agujeros se cierran en
// `vendor/librw/src/geometry.cpp`; ahora el atomic se rechaza y el motor lo reintenta con
// `GEOSTRUCT/GEOMORPH rechazado` + `FAIL model <nombre>` en la traza.
// T114 (42.1): REA del `ViceEx.exe` enviado (sha256 4df2726e…, FUN_00612670 =
// CAutomobile::DoVehicleLights). El binario solo puerta con
// `bStandardCarsUseTurnSignals` la copia VIEJA de los cuatro booleanos; dentro de
// `if (mi->bNewLights)` se calculan sin leer el ini. Como el DFF del Banshee
// conserva `headlight_l`/`taillight_l` (bNewLights = 1) y no tiene ningun nodo
// `indicator_*`, su faro blanco y su piloto rojo parpadean con su propia corona
// aunque el features.ini del mod traiga el flag a 0. Ademas la segunda corona del
// mismo faro (ids this+26/27 del binario) se mueve con ese estado, no con
// `bLightsOn`: antes dejaba en el Banshee un foco fijo detras del que si guiñaba.
// T115 (43.0, rendimiento/estabilidad, reporte del jugador del 4/11):
// Audio: censo por muestra (`SNDTOP`/`SNDLIST` cada 15 s con reqs y ms reales por
// sample) y etiqueta del productor (`__LINE__`) en cada ranura, con `SNDNEW`/
// `SNDSTUCK`/`SNDDIE` y `prod=`/`refresco=` para que un bucle quede asignado a la
// linea que lo renueva en vez de adivinarlo. Mareo: `CMBlur::Drunkness` es estatica
// y solo la escribe `SetDrunkBlur` cuando `m_nDrunkenness != 0`, asi que al bajar a
// cero nunca se desvanecia: el temblor de camara y el fantasma del frame anterior
// (postfx.cpp mezcla `pFrontBuffer` con `175*Drunkness`) sobrevivian a la embriaguez
// y seguian pintados tras saltar la cinematica o terminar la mision. Se añade el
// `ClearDrunkBlur` y las trazas `DRUNKCL`/`MAREO`/`CUTSKIP`.
// T116 (44.0, intermitentes): el metodo del Banshee deja de ser privilegio del
// Banshee. Antes la ruta de lampara propia estaba doblada por dos puertas
// distintas: los materiales parpadeaban con el veredicto de ambar MEDIDO, pero
// las coronas y las sombras de luz pasaban por `odStdSignals = bNewLights ||
// bStandardCarsUseTurnSignals`, y con el features.ini del mod (byte = 0) los 90
// modelos normales se quedaban fijos. Resultado: mesa, regina, landstalker,
// sentinel, esperanto, hermes y pheonix no echaban ningun destello al girar. Ahora
// manda una sola cosa - la lente medida del costado - y los 460 costados consultan
// el mismo veredicto. Ademas: (a) la mascara de lente (la misma formula del dato,
// `lens_mats`) se aplica tambien al subir `ambient`, asi que el capo del Hermes
// deja de encenderse (pintaba 16 caras, 14 de ellas de capon; pinta 2); (b) fuera
// los dos unicos colores inventados que quedaban (el ambar por textura `lights` y
// el {255,127,0} del destello): sin fila medida no hay destello, y la tabla llega
// a 338 filas / 214 ambar porque el atlas `lights` del mod es lente ambar por
// diseno del mod - el Sabre delantero parpadea ambar como las demas direccionales;
// (c) una lente partida en varios atomics (landstal, esperant, packer, rancher,
// sandking, zebra: 14 celdas, maximo 2 grupos) echa UN destello por tramo con su
// ancla, su tamano y su color medidos, en ids +48..+55 (coches) y +44..+47 (motos),
// y el destello del Packer deja de flotar a media caja (4.2 m entre tramos).
// Sin cambios en DFF/TXD: el `dataTag` sigue `2026-11-03-42.1-T109`.
// T117 (estabilidad y coste): del log de dos partidas. (a) Cuelgue: la linea
// fatal sin capturar sale de `CReplay::StorePedAnimation` ->
// `RpAnimBlendClumpGetMainAssociation` sobre el clump de un ped ya liberado.
// Aqui no hay grabacion que reproducir (F1/F2/F3), asi que `CReplay::Update` y
// `RecordParticle` salen directos: se quita el fallo y dos barridos completos
// de los pools de vehiculos y peds por frame. (b) Mareo: 150 filas `MAREO` con
// `add=150 drunk=0.000 sw=0` durante 78 s seguidos prueban que NO era la
// embriaguez - es la pantalla ancha que se queda encendida. `add=150` solo lo
// escribe `ProcessWideScreenOn`, y ese solo corre si `m_WideScreenOn`; el guion
// la apagaba en `DeleteCutsceneData`, que puede tardar o no llegar. Ahora
// `FinishCutscene` la apaga el, como la enciende `SetupCutsceneToStart`.
// (c) Audio en bucle: el censo no reprodujo ningun bucle pegado (SNDSTUCK =
// LOOPORPHAN = LOOPCOLLIDE = 0), y eso apuntaba a un candidato que ninguna
// medida de duracion ve: un disparo unico que otro productor renueva tan
// rapido que suena continuo. `SNDDIE` lleva `ent`/`vivo` y `SNDTOP` cambia de
// acumulado a delta de ventana con nacimientos, que es lo que lo delata.
// (d) FPS: el tiempo perdido esta dentro del tick de `RenderScene` (la espera
// cae a ~0,8 ms cuando la fase S pasa de 8 ms), asi que `RGPASS` reparte ese
// coste entre los 40 pases de render ya nombrados con `PUSH_RENDERGROUP`.
// T5 (plan 45.0): la tabla de lentes pasa a ser por RACIMO con su color MEDIDO
// (262 filas, 244 ambar, 166 celdas en 58 modelos) y lleva mascara de materiales
// `mats`, flag `atlas` y area. El motor suelda las caras de lente por vertice
// comun (DSU, tope 4096 vertices; si se pasa, un grupo por atomic como antes),
// empareja fila<->grupo por orden con contraprueba de centroide a 25 cm y pinta
// SOLO los materiales que la fila declara, con el color de la fila: en el atlas
// `lights` del mod (texelos blancos por diseno) se escribe el ambar; en una lente
// de textura propia el material se queda blanco porque el tono ya viene del pixel
// y pintarlo multiplicaria dos ambares. El tamano del destello sale del area real.
// Ids: el 3r..5t grupo va a +100 hacia arriba, que +56..+71 chocaba con las
// coronas de faro/piloto (+60..+69) del mismo `Automobile.cpp`. Datos: 27 DFF
// retocados (T4) -> sube tambien el `dataTag` a `2026-11-05-45.0-T6`.
// T1 (plan 47.0, estabilidad): tres cosas de la partida jugada tras el 45.
// (a) Crash a los segundos de saltar la 2a cinematica: el motor resuelve los
// sectores contra el `models/gta3.dir` hornado en `reVC.data`, y ese `.dir` se
// habia recalculado recorriendo el arbol de cada caja, no el layout autoritativo
// -> 367 huecos y 368 solapes: dos modelos reclamaban el mismo sector y el DFF
// que llegaba no era el suyo. `repack_dir.py` arma ahora un unico layout
// contiguo y `--mirror` escribe las copias con los mismos bytes;
// `check_img_index.py` (puerta dura en `build.sh`) no deja construir encima de
// un indice desincronizado, y `OdDoRead` ya no rellena de ceros un sector entero
// que falta: devuelve STREAM_ERROR y el streaming re-pide. `DFFGARBAGE` marca el
// rechazo de un BIN que declara mas bytes que la rebanada leida, y `DIRTAG` dice
// el indice que el motor tiene en la cabeza (n=6096 total=178498).
// (b) HUD visible durante la cinematica: `FinishCutscene` apaga la pantalla
// ancha (T117) y el HUD solo se cortaba por ese flag, asi que volvia con la
// mision sin empezar. El porton mira tambien `CCutsceneMgr::IsRunning()` y
// `HUDGATE` traza cada cambio de estado.
// (c) Mareo: sigue puesto por el juego (`COMMAND_SET_PLAYER_DRUNKENNESS`, unico
// escritor); `DRUNKSET` traza valor, script y IP para saber si es intencionado o
// leftover del skip. Sin cambio de comportamiento hasta tener esa medida.
// Datos: solo cambia `models/gta3.dir` (mismos bytes en streamed y bootseed,
// mismo tamano) -> el sello `__vc__` del manifiesto ya purga la cache IDB, y
// `dataTag` sube a `2026-11-05-47.0-T1` por trazabilidad.
// T1 (plan 48.0, lo que el motor media en FRAMES y no en segundos): seis puntos,
// cada uno detras de su `#define` en `config.h`. (a) Sirenas: el anillo del
// claxon se muestreaba por frame, asi que la ventana del doble toque pasaba de
// 166 ms a 42 ms a 120 FPS y el toggle casi nunca disparaba -> ahora se empuja a
// 30 Hz logicos (`HORNHIST`). (b) Helicoptero: el decaimiento del rotor iba por
// frame (se paraba 4x mas rapido a 120 FPS) y la rampa del mod pasa de 3 s a 6 s
// con `OD_HELI_RAMP` en `config.h`. (c) NPC sentado en coche invisible: la rampa
// de alpha de los coches de trafico era por frame y los jinetes se dibujaban con
// alpha plena -> escalada, y no se dibuja al ocupante por debajo de
// `OD_SEAT_GHOST_ALPHA`. (d) Reloj: perdia tiempo en cada hitch por el clamp de
// 60 ms de `Timer.cpp`; `CClock::Update` acumula con el reloj no recortado y
// recupera hasta 20 minutos (`RELOJ`). (e) Peso de los coches: el empuje
// peon<->coche estaba ANULADO, ahora se atenua a `OD_PUSH_VS_PED` (0.25) y el
// desplazamiento contra coche en movimiento a `OD_SHIFT_VS_PED` (`EMPUJE`). (f)
// NPCs hundidos en el asiento: el anclaje va a la cadera, asi que el ped bajo
// pierde cabeza; la tabla de 107 esqueletos medida con `ped_seat_height.py`
// levanta el asiento hasta 19 cm (`SILLACAM comp=`).
// T1..T16 (plan 49.0, el censo): se recorrio todo `src/` buscando logica que
// contaba frames presentados. Doce puntos corregidos, cada uno detras de su
// `#define`: mando escalado por `GetTimeStepFix()` (CAM), amortiguacion del
// giro en el aire (PHYS), cadencias de streaming y de peones por tick logico
// (STREAM), baches de suspension (AUTO), ventanas de camara (CAMERA), nado
// (PLAYERPED), sangre (PED), parpadeos y rampas del HUD/mareo/peces (HUD,
// MBLUR, WATER), lluvia y retune (AUDIO) y las gotas de lente del salpicado
// (DROPLETS), que ya no rearman la pantalla blanca al salir del agua.
// Ademas: el asiento vuelve a ser UNIFORME para todos los NPCs (T15) — se
// apaga `VICEEXT_SEAT_BY_PED_HEIGHT` y queda `OD_SEAT_LIFT_ALL` (0.0, la
// postura de Tommy). T14 añade cota de texturas por TXD (`TXDBAD`) y trazas
// `ODLOAD`/`ODTXD` para el crash del Hunter; T12 deja la medida `NEUMATICO`.
// Datos: no cambia ninguno -> `dataTag` sube solo por trazabilidad.
// T3/T4 (plan 46.0, triángulos apagados y destello sobre el capó): (a) el atlas
// de lente derivado `<atlas>_lens` se resuelve en `CVehicleModelInfo::SetClump`
// por puntero (pareja base/derivada) y se monta solo en el flanco encendido del
// parpadeo, igual que el motor hace con `lights`/`lightson`; con el atlas viejo
// una cara cuyo footprint cae a medias sobre una zona cromada del mismo atlas
// sale media gris. (b) La corona de intermitente pasa `useNearDist=false,
// nearDist=VICEEXT_IND_NEAR` (0,10 m): el render de coronas resta siempre el
// nearDist y el default de 1,5 m plantaba el disco delante del capó. Datos: 17
// TXDs del gta3.img servido llevan el atlas derivado (925 texelas re-tingidas al
// matiz medido) y `gta3.dir` reempaquetado -> `dataTag` `2026-11-06-46.0-T6`.
// La etiqueta solo nombra el plan que ejecuta hoy; el contenido del 49.0 y el
// atlas de lente del 46.0-T6 (esos son los DATOS, dataTag) siguen intactos en
// el árbol y en esta build.
// 49.1 (2026-11-07), los cinco puntos del jugador tras el 50.0-T1. (1) La
// frontera cinematica->juego deja de saltar de FOV: se integra en
// OD_FOV_RAMP_MS y `FOVLN` publica el valor cuadro a cuadro. (2) El anuncio de
// puente suena: los dos .MP3 se traen a MEMFS al empezar la partida y
// `ODNOFETCH` nombra la peticion que no llego. (3) El techo de vuelo vanilla se
// evalua al doble de alto (OD_CEILING_Z_SCALE=0.5) con `VUELO` midiendo empuje,
// factor y fps. (4) El mareo ya no vuelve al saltar la segunda cinematica: el
// desvanecido restaba ticks a un uint8 sin signo y se llenaba de 255
// (VICEEXT_DRUNK_FADE_NO_WRAP), y `DRUNKW` nombra a quien escribe la embriaguez.
// (5) El agua: en mar abierto el calado de 0,20 m ya no expulsa al nadador al
// cuadro siguiente (entrada y salida dejan de parpadear), la pose de caida se
// corta en el mismo cuadro y un reloj unico deja un chapuzon por ventana
// (`SPLASH sitio= num= dur=`). Cada partida acaba ademas en su propio
// `logs/odtrace-<fecha>_<hora>_p<n>_<tag>.log` y el tope de 200 000 lineas del
// enrutador esta quitado.
// 49.2 (2026-11-07), tres puntos de sus dos partidas. (1) FOV: fuera la rampa
// del 49.1 (era EL zoom que veia: 811 ms al arrancar la cinematica y 900 ms +
// meseta + salto seco al omitirla) y, por su decision, un solo FOV dentro y
// fuera (`VICEEXT_FOV_SINGLE`); `FOVLN` ya no habla por reloj, solo cuando el
// valor cambia. (2) Vuelo: la build que se sirve lleva la sonda puesta en
// build.sh (SAFE_HEAP + STACK_OVERFLOW_CHECK, sin banderas ni parámetros) para
// que el PRIMER acceso invalido nombre donde ocurrio, no el malloc que lo
// hereda veinte segundos despues (RetryLoadFile -> ConvertBufferToObject ->
// rw::Light::create -> malloc). Se quitan las dos banderas con el culpable en
// la mano. (3) Agua: la frontera
// entrar/salida copiada del binario del mod (un solo criterio de calado +
// flotacion, gravedad y v.z en la entrada, brazadas a -6.0 al salir y repaso
// cada cuadro de salto/caida/salida-de-coche) y salida trepando con el
// climbing que ya portamos, mas el subidito al reborde; `SWIMCLIMB`/`SWIMHOP`
// lo demuestran y `fall=` deja de ser el centinela -1.00. El anuncio del puente
// queda como esta: el jugador lo oyo bien en coche y en helicoptero.
// 49.2-T2 (mismo dia): la T1 se sirvio con la sonda de accesos puesta
// (SAFE_HEAP + STACK_OVERFLOW_CHECK) y el motor abortaba en el arranque con
// `Aborted(segmentation fault)`: pantalla negra. Retiradas las dos banderas,
// subo la etiqueta para que el navegador pida otra URL y no recargue el
// paquete roto que ya tenia en cache.
// 49.2-T3: el clavado seguia tardando en verse nadar. Causa medida: mientras
// se nada, el motor seguia mezclando el `quieto` del estado PED_IDLE cada
// cuadro y la brazada peleaba con el (el mod no lo hace: su nadador esta en un
// estado propio). Ahora `SetMoveAnim` sale antes cuando el carril del nado es
// nuestro, y el trepar desde el agua funciona (el climbing se vetaba a si
// mismo al estar nadando). Traza nueva `SWIMRAMPA` para ver la rampa real.
// 51.0-T6 (7/11): cierre de la familia de intermitentes. Cuatro cosas del motor
// y una de datos: el disco del destello pasa a 1,6 x el diametro de la lente con
// clamp [0,09 ; 0,24] m (medido contra la captura del mod: estaba 3,4 veces
// grande), el ambar de las 117 filas del atlas `lights` es el (253,165,0) medido
// en esa captura (antes (255,225,0), amarillo chillon), el trafico solo senala
// el giro que va a hacer en un cruce al que ya se acerca (nodo con >= 3 enlaces,
// umbral de angulo del mod 0,5 y ultima fraccion del tramo) y echa el parqueo
// cuando va a pararse a bajarse, y lo que el jugador latcheó con la tecla sigue
// parpadeando despues de bajarse hasta volver a pulsarla (el parqueo se cae al
// empezar a andar). Los GXT traen las etiquetas nuevas del menu de controles.
// 51.1-T1 (7/11): dos correcciones del 51.0 pedidas por el jugador. (a) El
// parqueo del jugador NO se apaga al acelerar -eso no se pidio-; lo que se pidio
// es que el NPC que aparco con las luces puestas tenga el criterio de apagarlas al
// volver a andar (`parqueo-apagando` en la traza turners). (b) En la moto, el
// destello se quedaba en la posicion de la lente sin tumbar (captura del 07/11):
// la inclinacion vive en el frame `chassis_dummy`, y los nodos `indicator_*`
// cuelgan de ahi, asi que el ancla se guarda en espacio del nodo y se transforma
// por su LTM viva. No cambia ningun dato servido: sube VERSION para que el
// navegador pida el wasm nuevo; `dataTag` se queda en la del 51.0.
// 55.0 (8/11): REVERTIDO por pedido del jugador. La deteccion relajada pinto en
// el benson las marcas laterales del parachoques en lugar de la lente que esta
// debajo del faro; se devolvieron el DFF y la tabla de lentes a su estado anterior
// y la herramienta de deteccion tambien. El trabajo vuelve como plan 56.0 con
// otra base de medida (las capturas del jugador del 08/11), no como parche.
// 56.0 (8/11) T1/T1b/T1c/T4/T5 en el MOTOR: la herramienta de deteccion pesa
// ahora el AREA que la cara pisa en cada texela (recorte exacto contra la celda)
// y respeta el alfa solo cuando la textura lo declara, que es lo que devolvio la
// lente del fbiranch. La tabla de lentes servida es la regenerada con esa medida.
// Ademas el coach (autocar) ya no admite la 1a persona -no tiene interior medido,
// 6 caras en la cabina frente a la mediana de la flota de 113- y el charco de luz
// del suelo se ancla a la lampara en vez de crecer con la longitud del modelo.
// Sucesor en NOMBRE de `2026-11-08-54.0-T1`, cuyo trabajo (colision, TXD, heap)
// sigue en la build. T8 (8/11): REVERTIDA la tanda de direccionales por pedido
// del jugador - la tabla vuelve a las 262 filas servidas mas las dos del benson
// (264), unicas que el jugador confirma funcionando. Las correcciones de la
// herramienta de deteccion (T1/T1b/T1c) se quedan: son las que permiten ver la
// lente sin inventarla, y las reglas de camara y de luces del autocar/panzer (T4
// y T5) el jugador las da por buenas.
// 58.0 T1 (8/11): la tabla recibe UNICAMENTE las 28 filas que el jugador dicto
// modelo por modelo (rumpo/spand/gangbur/manana/pony delante, cabbie/zebra/
// rancher/boxville detras, enforcer detras con sus dos racimos, securica las
// cuatro esquinas). Diff contra la servida: 28 anadidas, 0 perdidas, ningun otro
// modelo cambia. Con la fila declarada el nodo enciende su lente y emite su aro
// de luz: el halo que faltaba en securica/enforcer era exactamente esto, sin fila
// ambar no hay corona. Los focos cuya cara comparte material con la pintura
// (esperant detras, boxville delante, fbiranch) NO entran: pintar por material
// iluminaria medio panel y mover la cara esta prohibido; quedan preguntados.
// 58.0 T3 (8/11): esos focos de carroceria YA entran, sin geometria: atlas
// derivado <atlas>_lens (via 46.0) que solo enciende las texelas ambar del
// footprint medido. BodyLensTable.h (14 filas, 6 sitios) + VICEEXT_BODY_LENS.
// dataTag cambia porque los TXD de esperant/boxville/fbiranch/mule/trash cambian.
// 58.0 T4 (8/11): el material compartido de la lente se parte en slot izquierdo
// y derecho (copias identicas, cero cambio de geometria) y la tabla regenerada
// pinta cada costado con SU slot: el giro a la izquierda enciende su cristal y
// el derecho ya no enciende los dos. VERSION anterior: `2026-11-08-58.0-T3`.
// 58.1 T1 (9/11): los seis defectos de la prueba en juego del 58.0. topfun recibe
// sus 2 filas de nodo delanteras (atlas de cara completa, atlas=1); rumpo/pony
// delantera pasa a atlas=1 con el footprint entero de la cara iluminado (no solo
// el hilo ambar); zebra recupera la pintura que un plan viejo metio en el nodo
// (re-parent del clon al root con la matriz mundo intacta, cero cambio de forma);
// mrwhoop enciende el ambar sobre el faro blanco (ruta carroceria 58.0, material
// partido por costado); trash pierde la fila inventada del porton (stop rojo) y
// su atlas derivado. fbiranch NO se toca (pendiente de log nuevo). Cambian DFF y
// TXD -> dataTag sube. VERSION anterior: `2026-11-09-57.0-T1`.
// 59.0 (T0-T4): el respaldo sin nombre de las lentes de carroceria pasa de
// centroide de material a triangulo medido (a <= 0,25 m del xyz de la fila), el
// material de cada fila se clasifica en runtime como exclusivo de lente y, solo
// entonces, la ruta de carroceria sube el ambient al ambar medido igual que la
// ruta del nodo — es lo que enciende la mitad de la cara que vive en chassis_hi
// (rumpo, pony y topfun 4/4 filas; cabbie, zebra, securica, enforcer, manana,
// spand y rancher completas). Trash estrena atlas de cara completa sin
// comprimir. gangbur pasa a tintarse como pony. Trazas nuevas: BODYLENSROW con
// excl=/tint= y AMBERGATE por coche. VERSION anterior: `2026-11-09-59.1-T2`.
// 61.0 (pedido directo del jugador, sin plan): el mapa del radar y sus blips se
// colocaban con SCREEN_SCALE_X(8.0f) mientras el marco rosa se coloca en
// RADAR_LEFT (40) — el cristal salia fuera del aro. src/core/Radar.cpp,
// TransformRadarPointToScreenSpace: ahora las tres piezas comparten el origen
// del marco, igual en PC que en tactil. VERSION anterior: `2026-11-09-60.0-T1`.
// 61.0 T2 (piloto Xbox `banshee`): el TXD del pack se convierte a 32 bits BGRA
// sin comprimir y se instala en streamed/bootseed. El sello sube para que el
// navegador no reutilice el .data ni la cache IDB con el TXD DXT viejo.
// VERSION anterior: `2026-11-09-61.0-T1`.
// 61.0 T3: marcador de pintura del cuerpo del banshee; el DFF cambia sin
// cambiar de tamano, asi que la recarga se fuerza por VERSION/dataTag.
// 61.0 T4: el chassis_hi usa la textura de carroceria banshee86body128a;
// el DFF cambia de tamano (386685), asi que el sello sube otra vez.
// 61.0 T5: el chassis_hi reparte el atlas banshee868bit128 (1946 tris) y
// conserva la carroceria (2010 tris) con la pintura; sello nuevo.
// 61.0 T6: el chasis Xbox usa el atlas completo (3956 tris); la textura
// banshee86body128a es de puertas/aletas. El cuerpo deja de salir liso.
// 62.2 T7: UVs del atlas alineadas contra el Banshee PC y malla reconstruida
// desde el STRUCT. VERSION anterior: 2026-11-09-62.1-T1.
// 62.3 T8: UVs reales de los triángulos PC para cuerpo y atlas; sin
// interpolación y sin degenerados. VERSION anterior: 2026-11-09-62.2-T1.
// 63.0 T9: Banshee completo de The Hero 1:1 (DFF 342016 B, 16 atomics, 9
// materiales; TXD 1552204 B, 32 bits). VERSION anterior: 2026-11-09-62.3-T1.
// 63.1 T1 (10/11): MatFX recibe env-map dinámico en carrocerías con specular
// (Banshee de The Hero y el resto de la flota Xbox), sin cambiar el pipeline
// global a Neo. VERSION anterior: 2026-11-09-62.2-T2.
// 63.2 T1 (10/11): Banshee Xbox con la capa de reflejo del Banshee PC
// (bansheespeca + coeficiente 1.0 en pintura y cristales, DFF 344176 B,
// TXD 1574200 B). VERSION anterior: 2026-11-10-63.0-T1.
// 63.1 rendimiento T1-T3 (11/11): S3TC/ASTC de GPU activados en WebGL2, batching de mallas del mismo material en librw, envmap solo con edificios. VERSION anterior: 2026-11-10-63.2-T1.
// 64.0 M1 (11/11): medición de dibujos por tipo de objeto, por tamaño y por
// pasada (traza ODDRAWS), sin cambios visibles. VERSION anterior: 2026-11-11-64.0-T1.
// 66.1 M2 (11/11): build de control con la sonda de dibujos apagada, para
// separar las caidas del registro de trazas. VERSION anterior: 2026-11-13-66.2-T1.
// 67.1 (11/11): se revierte el cambio de mipmaps DXT de 67.0: los niveles
// pequenos quedaban con contenido cruzado y los peds a distancia salian a
// rayas. VERSION anterior: 2026-11-11-67.0-T1.
// 60.5 (15/11): carroceria PC + piezas Xbox en los 11; volante propio, nunca el
// negro generico (SteeringWheel). VERSION anterior: 2026-11-15-60.4-T4b.
// 60.7 (15/11): sirena unica en la patrulla y ventanilla danada en los 11.
// VERSION anterior: 2026-11-15-60.6-R2.
// 60.8 T14 (16/11): los 9 coches restantes (kaufman, cabbie, stinger, landstal,
// hotrinb, police, ambulan, firetruk, vicechee) con interior, motor, bajos y
// paneles de puerta Xbox sobre carroceria PC; lunas danadas en los que no
// tenian malla propia. TXD base convertidos a 32 bits y texturas nuevas.
// VERSION anterior: 2026-11-16-60.9-T13.
// 60.10 T8 (17/11): corrige la T7 servida. La retirada de las cartas del chassis
// leia el buffer ORIGINAL (snapshot de geoms_of) en vez del clump ya modificado,
// asi que deshacia la retirada de cabina y volvia a dejar el cuerpo PC entero
// (vicechee 1074 caras en vez de 921, police 966 en vez de 774). Ahora la
// retirada se integra en la misma pasada sobre el clump clonado.
// VERSION anterior: 2026-11-17-60.10-T7.
// 60.10 T7 (17/11): la mancha negra/gris sobre la luna rota NO era el od_interior
// (T6 lo dejo como admiral): era el chassis PC, que conserva cartas planas OPACAS
// tapando el hueco de la luna (respaldo negro None + respaldo body inboard).
// Admiral no las tiene. Se retiran por componente completo solo en coches con
// luna lateral (stinger/hotrinb sin lunas quedan intactos). vicechee ademas tenia
// un cabezal de retrovisor permanente en el chassis que se sumaba al de od_mirrors
// (3 retrovisores en algunos angulos): se retira => quedan exactamente 2.
// 60.10 T6 (17/11): firetruk: el chorro nace en la boca REAL del dispersor
// (y1.80,z2.00; el tubo rojo apunta hacia adelante) anclada al nodo bump_rear que
// pivota con el canon, no en la tapa trasera (0,1.26,2.14) que uso T5. Vidrio
// roto: se REVIERTE el trim de T5 (refutado: admiral conserva esas cartas); eran
// el respaldo interior opaco de la ventanilla (cheetah868bit128, por dentro del
// cristal), no moldura danina. Quitarlas dejaba el hueco sin respaldo y flotaba
// el material de la otra puerta. od_interior de los 9 vuelve a su respaldo.
// 60.10 T5 (17/11): firetruk chorro a (0,1.26,2.14) + trim de molduras coplanares
// del od_interior. AMBOS refutados en T6: la boca estaba mal medida y el trim
// retiraba el respaldo legitimo de la ventanilla (admiral lo conserva).
// 60.10 T4 (17/11): vicechee. La farola de la sirena del parabrisas (chassis PC,
// comps x0.33..0.48 z0.14..0.27) se injerta como od_siren permanente (T3 la retiro
// porque su bbox sobresalia del KEEP_BAND). Los retrovisores dejan de ser
// componentes aleatorios: extra1 (1 retrovisor) y extra2 (la pareja) se vacian y
// la pareja se injerta como od_mirrors permanente => siempre 2 retrovisores.
// 60.10 T3 (17/11): correcciones de pilotos SIN plan (el plan 61 solo guarda los
// guards para su ejecucion futura): tinte/alfa de vidrios uniforme en los 9
// (referencia admiral: sano 89/89/89/89, grieta 255/255/255/242, todos ALFA);
// sirena del vicechee conservada por componentes completos y od_interior fuera
// de la banda expandida (sin cortes en el salpicadero); volante fijo Xbox retirado
// donde hay steeringwheel PC movil, y anclaje del procedural bajado a <=100 tris
// en landstal para que lo sustituya por el movil.
// 60.10 T2 (16/11): corrige los 4 defectos del T1 tras prueba en juego: vidrio
// sano solo None CON alfa (la junta opaca se queda en la puerta) para que el
// atomic window_* caiga al pase transparente; od_interior/motor/bajos justo tras
// chassis_hi (antes de las grietas, que escriben profundidad); sirena PC del
// vicechee conservada y fuera del od_interior; piel exterior de puerta landstal
// conservada y incluida en la chapa de proyeccion del panel danado.
// 60.10 T1 (16/11): correccion del lote de los 9 con el metodo admiral. El
// chassis PC solo pierde el interior de cabina (por cara, caja por coche);
// morro/cola/parachoques/luces y atlas *body* quedan intactos. od_interior solo
// cabina, encogido 8 mm y sin solapes reales con el casco. Vidrios/grietas y
// paneles de puerta danados rehechos. dataTag sube para purgar la IDB.
// VERSION anterior: 2026-11-17-60.9-T17.
// 60.10 T13 (17/11): vicechee, marco del parabrisas restaurado (el filtro de
// lunas lo tomaba por respaldo) y retirada la pala vertical del pilar A que se
// veia como tercer retrovisor. VERSION anterior: 2026-11-17-60.10-T12.
// 61.0 T1 (18/11): wheel_lightvan64back pasa de paletizada 8 bits a 32 bits sin comprimir. VERSION anterior: 2026-11-18-61.0.
// 82.4 T2 (20/11): quita el reloj por entidad de la instrumentacion de grupos
// (un cruce wasm->JS menos por entidad y frame), agrupa las senales del perro
// guardian a una cada 200 ms (antes: un postMessage por entidad y frame dentro
// del bucle de ProcessControl) y compila sin DWARF para que wasm-opt aplique
// todos los pases de -O3. VERSION anterior: 2026-11-20-82.3-T1.
// 90.0 (22/11): el forro `od_interior` visible desde fuera se mete detras de la
// chapa (tope 10 cm, sin tocar chapa ni faros) y el taxi deja de tener 4 caras
// del faro trasero tapadas. VERSION anterior: 2026-11-22-89.0-T1.
// 92.0 (22/11): las caras de faro y piloto de 51 coches recuperan la UV del DFF
// del PC (la servida tenia la V espejada y caia en el negro del atlas).
// VERSION anterior: 2026-11-22-91.0-T1.
export const VERSION = '2026-11-22-92.0-T2';

const DEFAULTS = {
  el: null,                 // div destino (elemento o selector). Obligatorio.
  // --- De dónde salen las cosas ---
  buildUrl: '/build/',      // reVC.js + reVC.wasm + reVC.data
  streamedUrl: '/streamed/',// datos sueltos on-demand
  packsUrl: '/packs/',      // paquetes .vcp de la descarga inicial
  manifestUrl: '/manifest.json', // índice de streamed/ (claves en minúsculas)
  assetsUrl: '/vc/',        // copia legal original, SOLO se mira si falta streamed/
  runMode: 'local',
  bucketUrl: 'https://vicecity-data.carv-portfolio.workers.dev/',
  // --- Comportamiento ---
  fill: 'viewport',         // 'viewport' (toda la pantalla) | 'parent' (la caja del div)
  title: true,              // poner el título de la pestaña con el tag de build
  audio: true,              // desbloquear audio con el primer gesto (sin botones)
  pointerLock: true,        // capturar el ratón al hacer clic en el canvas
  touch: 'auto',             // 'auto' | 'on' | 'off'
  touchIconsUrl: '/touch/icons/',
  touchLookSensitivity: 0.9,
  guardUnload: true,        // pedir confirmación al cerrar/recargar en partida
  fullscreenKey: true,      // F11 = pantalla completa del juego
  showFps: false,           // contador de FPS del motor (esquina superior izq.)
  idbCapMB: 0,              // 0 = sin techo: lo descargado se queda para jugar sin conexión
  warmMB: 64,               // MB de "arranque en caliente" durante la carga (0 = off)
  worker: true,             // cargar ficheros en un hilo aparte (false = como antes)
  traceUrl: null,           // p.ej. '/odtrace' en desarrollo; null = sin envíos
  console: true,            // espejo a la consola del navegador
  noInitialRun: false,      // cargar runtime+FS sin arrancar el juego (diagnóstico)
  requireBuild: true,       // si falta el build, no arrancar y avisar
  requireData: true,        // si faltan los datos, no arrancar y avisar
  // --- Ganchos ---
  onProgress: null,         // ({ phase, pct, label, busy })
  onLog: null,              // (line)
  onError: null,            // (error)  -> también por consola
  onReady: null,            // ()  runtime wasm listo
  onAudio: null,            // (active:boolean)
  onFullscreen: null,       // (active:boolean)
};

// El motor emscripten es global (Module, FS, OD): una instancia por página.
let current = null;

// 81.2: Chrome limita a 64 KB el cuerpo de `sendBeacon` y de `fetch` con
// keepalive. Los lotes de 1200/3000 lineas pasaban de ese tope justo cuando el
// motor mas escribe (carga del mundo y partida): el envio se descartaba sin
// error y la traza se cortaba al empezar a jugar. Ahora cada lote se corta por
// caracteres y, si el beacon falla, las lineas vuelven a la cola.
const TRACE_CHUNK = 48000;
function takeTrace(q, maxLines) {
  let n = 0;
  let k = 0;
  while (k < q.length && k < maxLines) {
    const add = String(q[k]).length + 1;
    if (k > 0 && n + add > TRACE_CHUNK) break;
    n += add;
    k++;
  }
  return k ? q.splice(0, k) : [];
}
const ODPRI = /^(loadtick|webload|BART|ASYNC|RETUNE|RADIOTRACK|FPSLOG|ENGAP|PERF|ODCAP|GEAR|initstep|od-trace|FPHASE|ODLOAD|ODSFXDEFER|ODSFXSTAT|ODSFXBUDGET|ODCMIG|ODTEXREL|ODWORLD)/;
const NOISY = /took \d+ ?ms|\[DBG\]: (Request|Remove) Ped|\.txd took |REMOVE_SOUND - Sound doesn't exist/;

function base(url, fallback) {
  if (url === null || url === undefined || url === '') return fallback;
  return String(url).endsWith('/') ? String(url) : String(url) + '/';
}

// HEAD con guardia de content-type: un dev server tipo Vite responde su
// index.html (200 text/html) a rutas inexistentes, así que "ok" no basta.
async function exists(url) {
  try {
    const r = await fetch(url, { method: 'HEAD' });
    if (!r.ok) return false;
    const ct = (r.headers.get('content-type') || '').toLowerCase();
    return !ct.includes('text/html');
  } catch (e) {
    // 77.4 (sin conexión): el service worker guarda el motor pero solo atiende
    // GET, así que un HEAD sin red no tiene respuesta y el arranque se negaba a
    // empezar («falta el build») con reVC.js/wasm/data ya guardados. Un GET sale
    // de la caché del service worker y el cuerpo se cancela en el acto para no
    // pagar el fichero entero solo por mirar si está.
    try {
      const r = await fetch(url, { cache: 'force-cache' });
      const ct = (r.headers.get('content-type') || '').toLowerCase();
      try { if (r.body) r.body.cancel(); } catch (ee) {}
      return r.ok && !ct.includes('text/html');
    } catch (ee) {
      return false;
    }
  }
}

function injectStyles(doc) {
  if (doc.getElementById(STYLE_ID)) return;
  const s = doc.createElement('style');
  s.id = STYLE_ID;
  s.textContent = CSS;
  (doc.head || doc.documentElement).appendChild(s);
}

function markup(root, fill) {
  root.dataset.fill = fill;
  root.innerHTML = `
    <canvas class="vc-canvas" tabindex="0"></canvas>
    <div class="vc-bar" aria-hidden="true"><i></i></div>
    <div class="vc-load" hidden>
      <div class="vc-load-title">VICE CITY</div>
      <div class="vc-load-sub">Cargando… no cierres la pestaña</div>
      <div class="vc-load-bar"><i></i></div>
      <div class="vc-load-label"></div>
    </div>
    <div class="vc-msg" hidden></div>`;
}

// ---------------------------------------------------------------------------
// startGame: única función que necesita el host.
// ---------------------------------------------------------------------------
export function startGame(options = {}) {
  const opts = Object.assign({}, DEFAULTS, options);
  const doc = opts.doc || (typeof document !== 'undefined' ? document : null);
  if (!doc) throw new Error('gtavc-web: hace falta un documento (navegador).');

  if (current) {
    current.log('[lib] ya hay un juego arrancado; se devuelve la instancia viva');
    return current;
  }

  const el = typeof opts.el === 'string' ? doc.querySelector(opts.el) : opts.el;
  if (!el) throw new Error('gtavc-web: falta { el } (el div donde montar el juego).');

  const live = opts.runMode === 'live';
  const bucketUrl = live ? base(opts.bucketUrl, 'https://vicecity-data.carv-portfolio.workers.dev/') : null;
  const buildParam = (typeof location !== 'undefined') ? new URLSearchParams(location.search).get('build') : null;
  const buildUrl = bucketUrl ? bucketUrl + 'build/' : base(buildParam || opts.buildUrl, '/build/');
  const streamedUrl = bucketUrl ? bucketUrl + 'streamed/' : base(opts.streamedUrl, '/streamed/');
  const packsUrl = bucketUrl ? bucketUrl + 'packs/' : base(opts.packsUrl, '/packs/');
  const manifestUrl = bucketUrl ? bucketUrl + 'manifest.json' : (opts.manifestUrl || '/manifest.json');
  const assetsUrl = (bucketUrl || opts.assetsUrl === null) ? null : base(opts.assetsUrl, '/vc/');
  const traceUrl = opts.traceUrl || null;
  const recoilTraceId = null;   // Sin query param: id siempre automático.
  // Recoil siempre activo si hay endpoint de trazas: sin query param.
  // ?trace=<nombre> solo etiqueta la sesión (y el fichero en desarrollo).
  const recoilTraceActive = !!traceUrl;
  let recoilEventSeq = 0;
  let recoilDropped = 0;
  let recoilEndSent = false;
  let odDropPending = 0;   // descartes de cola pendientes de constancia
  // Volcado local (E3): si el envío falla, la sesión sobrevive en el
  // navegador y se reenvía al siguiente arranque con endpoint.
  const ODSPILL_KEY = 'vc_od_spill_v1';
  const ODSPILL_CAP = 2000;
  let odSpillLost = 0;
  function odSpill(lines) {
    try {
      const raw = localStorage.getItem(ODSPILL_KEY);
      const arr = raw ? JSON.parse(raw) : [];
      for (const ln of lines) arr.push(ln);
      while (arr.length > ODSPILL_CAP) { arr.shift(); odSpillLost++; }
      localStorage.setItem(ODSPILL_KEY, JSON.stringify(arr));
    } catch (e) {}
  }
  function odReplay() {
    try {
      if (!traceUrl) return;
      const raw = localStorage.getItem(ODSPILL_KEY);
      if (!raw) return;
      const arr = JSON.parse(raw);
      if (!arr.length) return;
      localStorage.removeItem(ODSPILL_KEY);
      const body = ['ODTRACE_REPLAY n=' + arr.length + (odSpillLost ? ' perdidas=' + odSpillLost : '')].concat(arr.slice(0, 3000)).join('\n');
      fetch(traceUrl, { method: 'POST', body, keepalive: true }).then((r) => { if (!r || !r.ok) odSpill(arr); }).catch(() => { odSpill(arr); });
    } catch (e) {}
  }

  // El motor (WebDrawFps en main.cpp) lee este global en cada frame: así el
  // contador de FPS se enciende por API, sin parámetros en la URL.
  globalThis.__vcShowFps = !!opts.showFps;

  injectStyles(doc);
  el.classList.add('vc-root');
  markup(el, opts.fill === 'parent' ? 'parent' : 'viewport');

  const canvas = el.querySelector('.vc-canvas');
  const barEl = el.querySelector('.vc-bar');
  const barFill = barEl.querySelector('i');
  const loadEl = el.querySelector('.vc-load');
  const loadBar = loadEl.querySelector('.vc-load-bar > i');
  const loadLabel = loadEl.querySelector('.vc-load-label');
  const msgEl = el.querySelector('.vc-msg');

  const touchParam = typeof location !== 'undefined' ? new URLSearchParams(location.search).get('touch') : null;
  const touchOn = touchParam === '1' ? true : touchParam === '0' ? false
    : opts.touch === 'on' ? true : opts.touch === 'off' ? false : touchDetected();
  const logLines = [];
  const listeners = { progress: [], log: [], error: [], ready: [], audio: [], fullscreen: [], booted: [] };
  const timers = [];
  let destroyed = false;
  let runtimeReady = false;
  let dataOk = false;
  let audioUnlocked = false;
  let idleTimer = 0;
  let lastProgress = { phase: null, pct: 0, label: '', busy: false };

  // Los eventos salen por dos vías: la API (game.on) y el propio div
  // (CustomEvent 'vc-progress', 'vc-ready', 'vc-error', 'vc-booted', ...).
  // Así un index.html cualquiera puede escuchar sin importar la librería.
  const emit = (evt, arg) => {
    for (const fn of listeners[evt] || []) { try { fn(arg); } catch (e) {} }
    try { el.dispatchEvent(new CustomEvent('vc-' + evt, { detail: arg })); } catch (e) {}
  };

  // ---- log --------------------------------------------------------------
  let logBuf = [];
  let logTimer = 0;
  function writeLogDom(lines) {
    const g = doc.getElementById('gamelog');           // opcional: host o página dev
    if (!g) return;
    try {
      const t = performance.now();
      g.textContent += lines.join('\n') + '\n';
      if (window.__jankM) { window.__jankM.domN++; window.__jankM.domMs += performance.now() - t; }
      if (g.textContent.length > 300000) g.textContent = g.textContent.slice(-200000);
      g.scrollTop = g.scrollHeight;
    } catch (e) {}
  }
  function log(t) {
    const s = String(t);
    logLines.push(s);
    if (logLines.length > 4000) logLines.splice(0, 2000);
    emit('log', s);
    logBuf.push(s);
    if (!logTimer) {
      logTimer = setTimeout(() => { logTimer = 0; writeLogDom(logBuf); logBuf = []; }, 500);
      timers.push(logTimer);
    }
    return s;
  }
  function fail(text, detail) {
    log('[err] ' + text);
    if (msgEl) msgEl.hidden = false;
    if (msgEl) msgEl.innerHTML = '<h3>El juego no puede arrancar</h3><p>' + text + '</p>' + (detail || '');
    const err = new Error(text);
    emit('error', err);
    if (opts.onError) opts.onError(err);
  }

  const touch = createTouchControls({
    root: el,
    log,
    enabled: touchOn,
    iconBase: opts.touchIconsUrl,
    sensitivity: opts.touchLookSensitivity,
    getModule: () => (typeof Module !== 'undefined' ? Module : null),
  });

  touch.mount();

  // ---- barra de progreso (rosa, top del viewport) ------------------------
  // Cubre SOLO la preparación previa al motor:
  //   'probe' → comprobación de ficheros (build y datos)
  //   'build' → descarga y arranque del motor (reVC.js/wasm/data)
  // El fin es determinista: el motor avisa por window.__vcFrame la primera vez
  // que presenta un frame (glfw.cpp, EmscriptenTick). Ahí la barra se retira y
  // NO vuelve nunca: a partir de ese momento manda la pantalla del juego (menú,
  // splash de carga de partida). __loadProgress (fases del init) solo llega al
  // inicializar el mundo, así que no sirve como señal de "ya hay juego".
  // Reparto del progreso GLOBAL (21/09, pedido del jugador: "una sola subida de
  // 0 a 100, que solo cargue una vez").
  //
  // Antes cada fase empezaba en 0 — `probe` era un barrido sin medida y `build`
  // volvía a arrancar desde cero — así que la barra parecía cargar DOS veces y no
  // se leía como un progreso. Ahora:
  //   * `probe` (comprobar build y datos) aporta el primer 4%;
  //   * `data` (bajar todos los datos del juego al navegador) del 4% al 86%;
  //   * `build` (guardar el motor para sin conexión y arrancarlo) del 86% al 100%.
  // El valor es MONÓTONO: nunca baja, ni al cambiar de fase ni cuando el cargador
  // reporta algo sin medida (antes eso volvía a poner el barrido y parecía
  // "empezar de nuevo").
  const BAR_TRAMO = { probe: [0.0, 0.04], data: [0.04, 0.86], build: [0.86, 1.0] };
  let barOverall = 0;      // progreso global 0..1 (nunca baja)
  let barMedido = false;   // ya hay una medida real: el barrido indeterminado no vuelve
  let engineUp = false;
  let lastReportKey = '';
  // La descarga pregunta el estado cientos de veces por segundo: la barra se
  // pinta como mucho una vez por frame. Sin esto eran miles de escrituras de
  // DOM por segundo y la barra parpadeaba.
  let barQueued = false;
  let barPending = null;     // { indet: true } | { indet: false, pct }
  let barTraceNext = 0;      // siguiente punto (5%) que se anota en la traza
  function barTrace(msg) {
    try { (window.__odq = window.__odq || []).push('BART ' + msg); } catch (e) {}
  }
  function applyBar() {
    barQueued = false;
    const p = barPending;
    barPending = null;
    if (!p || engineUp) return;
    // Ojo: el atributo vale '' (falso), así que hay que comparar con undefined:
    // si no, se reescribe en cada frame sin necesidad.
    if (barEl.dataset.on === undefined) barEl.dataset.on = '';
    if (p.indet) {
      if (barEl.dataset.indet === undefined) barEl.dataset.indet = '';
      // Sin ancho inline: si queda el '100%' de una fase anterior, gana al 32%
      // del CSS y el barrido se ve como una barra llena que destella.
      barFill.style.width = '';
    } else {
      if (barEl.dataset.indet !== undefined) delete barEl.dataset.indet;
      const pctMostrado = 100 * Math.max(0, Math.min(1, p.pct));
      barFill.style.width = pctMostrado.toFixed(1) + '%';
      // Traza de la subida: el jugador pidió "una sola subida de 0 a 100"; con
      // esto se comprueba en el log, sin mirar la pantalla, que el progreso es
      // monótono y lineal (un `BART pct=…` por cada 5 puntos).
      if (pctMostrado >= barTraceNext) {
        barTraceNext = Math.floor(pctMostrado) + 5;
        barTrace('pct=' + Math.round(pctMostrado));
      }
    }
  }
  function queueBar(p) {
    barPending = p;
    if (barQueued) return;
    barQueued = true;
    if (typeof requestAnimationFrame === 'function') requestAnimationFrame(applyBar);
    else setTimeout(applyBar, 16);
  }
  function showBar() { if (!engineUp) queueBar({ indet: false, pct: 1 }); }
  function hideBar() { delete barEl.dataset.on; delete barEl.dataset.indet; }
  function report(phase, pct, label, busy) {
    lastProgress = { phase, pct, label: label || '', busy: !!busy };
    // Mismo estado repetido (la descarga lo reporta cientos de veces por
    // segundo): no se dispara otro evento.
    const key = phase + '|' + (pct === null || pct === undefined ? 'x' : (+pct).toFixed(2)) + '|' + lastProgress.label + '|' + lastProgress.busy;
    if (key === lastReportKey) return;
    lastReportKey = key;
    emit('progress', lastProgress);
    if (opts.onProgress) opts.onProgress(lastProgress);
  }
  // El motor ya es dueño de la pantalla: la barra del top se apaga y ninguna
  // fuente vuelve a encenderla. 'reason' queda en las trazas.
  function engineOwnsScreen(reason) {
    if (engineUp) return;
    engineUp = true;
    barPending = null;
    clearTimeout(idleTimer);
    hideBar();
    barTrace('off reason=' + (reason || '?'));
  }
  function progress(phase, pct, label, busy) {
    const unknown = pct === null || pct === undefined;
    report(phase, pct, label, unknown ? true : (busy === undefined ? pct < 1 : !!busy));
    if (engineUp || !BAR_TRAMO[phase]) return;
    const [ini, fin] = BAR_TRAMO[phase];
    if (!unknown) {
      const v = ini + (fin - ini) * Math.max(0, Math.min(1, +pct));
      if (v > barOverall) barOverall = v;   // nunca baja
      barMedido = true;
      queueBar({ indet: false, pct: barOverall });
    } else if (!barMedido) {
      // Todavía sin ninguna medida (comprobar ficheros): ahí sí, barrido.
      queueBar({ indet: true });
    }
    // Si ya hay medida y esta llamada no trae número (p. ej. "Descargando datos
    // del juego…" cuando el cargador no da total), la etiqueta cambia y el ancho
    // se queda: antes el barrido reiniciaba la barra a media carga.
    // Red de seguridad: si el motor no llegara a avisar (build antigua), la
    // barra se retira sola un poco después de tener el runtime listo.
    clearTimeout(idleTimer);
    idleTimer = setTimeout(() => { if (runtimeReady) engineOwnsScreen('idle-timeout'); }, 6000);
  }
  // Cierre de la fase de preparación (100% y fuera).
  function done() {
    if (engineUp) return;
    barPending = null;
    barEl.dataset.on = '';
    delete barEl.dataset.indet;
    barFill.style.width = '100%';
    barTrace('on pct=100 (done)');
    clearTimeout(idleTimer);
    idleTimer = setTimeout(() => engineOwnsScreen('done-timeout'), 400);
  }

  // ---- cola de trazas (lo que manda el motor C++) ------------------------
  // window.__odq lo llenan EM_ASM del motor y el pre-js. Aquí se envía por
  // lotes async (nunca XHR síncrono en el hilo del juego) y se acota para que
  // una tormenta de líneas no se coma la memoria.
  const odq = (window.__odq = window.__odq || []);
  try {
    if (traceUrl) {
      const rigW = (typeof screen !== 'undefined' && screen.width) || 0;
      const rigUa = (typeof navigator !== 'undefined' && navigator.userAgent) || '';
      const rigNav = /Edg\//.test(rigUa) ? 'Edge' : /Chrome\//.test(rigUa) ? 'Chrome' : /Firefox\//.test(rigUa) ? 'Firefox' : /Safari\//.test(rigUa) ? 'Safari' : 'Nav';
      odq.push('RIG id=' + rigW + '-' + rigNav + ' ancho=' + rigW + ' nav=' + rigNav);
    }
  } catch (e) {}
  const recoilSession = window.__odRecoilSession = { active: recoilTraceActive, id: (recoilTraceId && /^recoil-[a-zA-Z0-9_-]{1,40}$/.test(recoilTraceId)) ? recoilTraceId : ('auto-' + Date.now().toString(36)) };
  const recoilEmit = window.__odRecoilEmit = (event) => {
    if (!recoilSession.active) return;
    const seq = ++recoilEventSeq;
    const line = `RECOIL sessionId=${recoilSession.id} eventSeq=${seq} build=${VERSION} data=${(typeof OD !== 'undefined' && OD.dataTag) || 'unavailable'} wasm=OD_RECOIL_SCHEMA_1 ${String(event).replace(/\\s+/g, ' ').slice(0, 640)}`;
    const q = window.__odq = window.__odq || [];
    if (q.length >= 30000) {
      recoilDropped++;
      return;
    }
    q.push(line);
    if (/^RECOIL_READY\b/.test(event)) { const rc = /recoil=(\d+)/.exec(event); recoilEmit(`RECOIL_DIAG schema=1 sessionId=${recoilSession.id} build=${VERSION} data=${(typeof OD !== 'undefined' && OD.dataTag) || 'unavailable'} wasm=OD_RECOIL_SCHEMA_1 recoilCompiled=${rc ? rc[1] : 'unknown'} enabled=1 dropped=${recoilDropped}`); }
  };
  function endRecoilSession() {
    if (!recoilSession.active || recoilEndSent) return;
    recoilEndSent = true;
    recoilSession.active = false;
    const line = `RECOIL sessionId=${recoilSession.id} eventSeq=${++recoilEventSeq} build=${VERSION} data=${(typeof OD !== 'undefined' && OD.dataTag) || 'unavailable'} wasm=OD_RECOIL_SCHEMA_1 RECOIL_DIAG_END lastSeq=${recoilEventSeq - 1} dropped=${recoilDropped} terminal=pagehide`;
    (window.__odq = window.__odq || []).push(line);
    flush(true);
  }
  // T17: sonda de cuelgues. Distingue un hilo atrapado en un bucle de CPU (los
  // timers no corren) de un motor SUSPENDIDO esperando un await (los timers
  // sí corren). Sólo habla tras 8 s de silencio estando en partida, así que en
  // juego normal no añade ni una línea.
  let lastEngineLineAt = Date.now();
  setInterval(() => {
    try {
      if (window.__vcInGame !== true) { lastEngineLineAt = Date.now(); return; }
      const silent = Date.now() - lastEngineLineAt;
      if (silent < 3000) return;
      lastEngineLineAt = Date.now();
      (window.__odq = window.__odq || []).push('ODJSALIVE silencio=' + Math.round(silent / 1000) +
        's bloqueando=' + ((window.__vcODBlock | 0)) +
        ' esperas=' + Object.keys((typeof OD !== 'undefined' && OD.wkWaiters) || {}).length +
        ' fetch=' + (window.__vcFetchNow || '-'));
    } catch (e) {}
  }, 2000);
  function flush(sync) {
    try {
      const q = window.__odq;
      if (!q || !q.length) return;
      lastEngineLineAt = Date.now();
      if (q.length > 30000) {
        let drop = q.length - 30000;
        for (let i = 0; i < q.length && drop > 0; i++) if (!ODPRI.test(q[i]) && !/^RECOIL\\b/.test(q[i])) { q.splice(i, 1); i--; drop--; }
        if (drop > 0) {
          for (let i = 0; i < q.length && drop > 0; i++) if (!/^RECOIL\\b/.test(q[i])) { q.splice(i, 1); i--; drop--; }
        }
        if (drop > 0) { recoilDropped += drop; odDropPending += drop; q.splice(0, drop); }
      }
      const batch = takeTrace(q, 3000);
      if (odDropPending > 0) { batch.unshift('ODTRACE_DROP descartes=' + odDropPending); odDropPending = 0; }
      if (!traceUrl) {
        // Sin endpoint de trazas: se guardan acotadas (para diagnóstico desde
        // la consola) sin inundar ni la consola ni la memoria.
        logLines.push(...batch);
        if (logLines.length > 4000) logLines.splice(0, logLines.length - 4000);
        return;
      }
      batch.sort((a, b) => (ODPRI.test(b) ? 1 : 0) - (ODPRI.test(a) ? 1 : 0));
      const body = batch.join('\n');
      if (sync) { const x = new XMLHttpRequest(); x.open('POST', traceUrl, false); x.send(body); }
      else fetch(traceUrl, { method: 'POST', body, keepalive: true }).then((r) => { if (!r || !r.ok) odSpill(batch); }).catch(() => { odSpill(batch); });
    } catch (e) {}
  }
  timers.push(setInterval(() => flush(false), 1000));
  if (!traceUrl) log('[lib] sin endpoint de trazas: solo memoria (4000 lineas)');
  try { odReplay(); } catch (e) {}
  // Traza a prueba de cuelgue (sección 1, 21/09). El envío normal va cada
  // segundo y luego, si el motor se queda en un bucle infinito DENTRO de un
  // frame, ese segundo no llega nunca: el log se corta y no hay forma de saber
  // en qué punto del juego se quedó.
  //
  // La cola se entrega al navegador en CADA frame con `navigator.sendBeacon`
  // (siempre activo con endpoint: sin query param): aunque el hilo se bloquee
  // o se cierre la pestaña, la última línea emitida SÍ llega.
  if (traceUrl) {
    // 24/10: antes salía un beacon POR FRAME (60 POST/s al servidor de trazas,
    // cada uno con su Blob y su entrada en la cola de red). En una partida de
    // 20-30 minutos eso es presión constante sobre el navegador. Ahora como
    // mucho 4/s, y al instante si la cola crece (no se pierde nada de un
    // cuelgue: lo último sale en <=250 ms, y pagehide/handlers fuerzan el envío
    // sincrono).
    let lastBeat = 0;
    // 24/10: perro guardian fuera del hilo del juego. Si el wasm se queda en un
    // bucle infinito, el hilo principal deja de latir y la traza se corta en
    // mitad de un frame: hoy eso es indistinguible de que el navegador haya
    // matado la pestaña (OOM/GPU), porque en los dos casos no hay JSERR ni
    // PAGEHIDE. El worker vive en su propio hilo, sigue escribiendo en el
    // endpoint aunque el juego este clavado y deja `WDTALIVE` con los ms que
    // lleva el hilo principal sin latir, asi que el proximo cuelgue dice cual
    // de las dos cosas fue. Si el navegador no tiene Worker, no se monta nada.
    let wd = null;
    let wdNext = 0;
    try {
      if (typeof Worker !== 'undefined' && typeof Blob !== 'undefined' && typeof URL !== 'undefined' && URL.createObjectURL) {
        // 25/10: el worker se crea desde una URL `blob:`, y una ruta relativa
        // resuelta contra esa base no llega a la pagina (`fetch('/odtrace')`
        // desde un blob falla). Ese era el motivo de que no hubiera ni un
        // WDTALIVE en las trazas: el perro guardian no llegaba a escribir.
        // Se le pasa la URL absoluta ya resuelta por la pagina.
        const wdUrl = new URL(traceUrl, location.href).href;
        const src = 'let url=' + JSON.stringify(wdUrl) + ';' +
          'let lastMain=Date.now(),started=Date.now(),hidden=false,mark=-1,ent=-1;' +
          'self.onmessage=function(e){var d=e.data||{};if(d.beat)lastMain=Date.now();if(typeof d.hidden===\'boolean\')hidden=d.hidden;if(typeof d.mark===\'number\')mark=d.mark;if(typeof d.ent===\'number\')ent=d.ent;};' +
          'setInterval(function(){if(Date.now()-started<8000)return;' +
          'var silent=Date.now()-lastMain;' +
          'var stall=(!hidden&&silent>3000)?1:0;' +
          'var line=new Date().toISOString()+\' WDTALIVE mainSilentMs=\'+Math.round(silent)+\' stall=\'+stall+\' mark=\'+mark+\' ent=\'+ent;' +
          'try{fetch(url,{method:\'POST\',body:line,keepalive:true});}catch(e){}},5000);';
        wd = new Worker(URL.createObjectURL(new Blob([src], { type: 'text/javascript' })));
        window.__odMark = (v) => { if (wd) { try { wd.postMessage({ mark: v }); } catch (e) {} } };
        window.__odEnt = (v) => { if (wd) { try { wd.postMessage({ ent: v }); } catch (e) {} } };
        doc.addEventListener('visibilitychange', () => {
          if (!wd) return;
          try { wd.postMessage({ hidden: !!doc.hidden }); } catch (e) {}
        });
        log('[lib] perro guardian activo (WDTALIVE cada 5 s)');
      }
    } catch (e) { wd = null; }
    const beat = (now) => {
      const tnow = typeof now === 'number' ? now : performance.now();
      window.__odMainBeat = tnow;
      if (wd && tnow >= wdNext) {
        wdNext = tnow + 500;
        try { wd.postMessage({ beat: tnow, hidden: !!doc.hidden }); } catch (e) {}
      }
      try {
        const q = window.__odq;
        if (q && q.length && typeof navigator !== 'undefined' && navigator.sendBeacon) {
          const t = tnow;
          if (q.length >= 1200 || t - lastBeat >= 250) {
            lastBeat = t;
            const batch = takeTrace(q, 1200);
            if (batch.length && !navigator.sendBeacon(traceUrl, batch.join('\n'))) {
              for (let i = batch.length - 1; i >= 0; i--) q.unshift(batch[i]);
              lastBeat = t + 5000;
            }
          }
        }
      } catch (e) {}
      if (!destroyed) requestAnimationFrame(beat);
    };
    requestAnimationFrame(beat);
    log('[lib] synctrace activo: cada frame se envía la traza (diagnóstico de cuelgues)');
    // 83.0: si el motor se cuelga esperando un fichero en od_fetch, el wasm
    // queda suspendido en Asyncify y la cola de trazas ya no se vacía. Este
    // timer vive en la página y sigue corriendo: nombra el fichero exacto
    // (`window.__vcFetchNow`) y, si no hay ninguno, avisa de que el cuelgue
    // ocurre fuera de od_fetch. Solo traza; no cambia el arranque.
    let odFetchLast = null;
    let odFetchSince = 0;
    let odFrozenAt = 0;
    let odBeatSeen = 0;
    let odBeatAt = Date.now();
    setInterval(() => {
      try {
        const now = Date.now();
        const b = window.__odMainBeat || 0;
        if (b !== odBeatSeen) { odBeatSeen = b; odBeatAt = now; }
        const silent = now - odBeatAt;
        const p = window.__vcFetchNow || null;
        if (p) {
          if (p !== odFetchLast) { odFetchLast = p; odFetchSince = now; }
          try {
            navigator.sendBeacon(traceUrl,
              'ODFETCHNOW p=' + p + ' espera=' + (now - odFetchSince) + ' silent=' + silent);
          } catch (e) {}
          return;
        }
        odFetchLast = null;
        if (silent > 3000 && now - odFrozenAt >= 5000) {
          odFrozenAt = now;
          try { navigator.sendBeacon(traceUrl, 'ODFROZEN fetch=null silent=' + silent); } catch (e) {}
        }
      } catch (e) {}
    }, 1000);
  }
  // E1: cierre garantizado. El fetch async puede no salir al cerrar la
  // pestaña; el beacon lo entrega el navegador aunque la página muera.
  const onPageHide = () => {
    // 24/10: separa "la pestaña murió" de "la pagina se recargó o se cerró".
    // Si el log termina sin esta línea, el proceso desapareció (cuelgue real);
    // si está, fue una recarga (Vite/HMR, F5, pestaña cerrada).
    pushFatal('PAGEHIDE', 'la pagina se esta yendo (recarga o cierre, no un cuelgue)');
    endRecoilSession();
    try {
      const q = window.__odq;
      if (traceUrl && q && q.length) {
        while (q.length) {
          const body = takeTrace(q, 1000000).join('\n');
          if (!body) break;
          if (typeof navigator !== 'undefined' && navigator.sendBeacon && navigator.sendBeacon(traceUrl, body)) continue;
          const x = new XMLHttpRequest(); x.open('POST', traceUrl, false); x.send(body);
        }
      }
    } catch (e) {}
  };
  // 20/09 (sección 1): el fallo duro DEJA CONSTANCIA en odtrace.log.
  //
  // Hasta ahora el motivo del cuelgue sólo salía por la consola de la página
  // (`[FATAL] ...`): cuando el jugador reportaba "cinemática → crashea" la traza
  // simplemente se cortaba en mitad de un frame y no había forma de saber si fue
  // un assert del motor, un OOM o una excepción de JS. Ahora el texto se encola
  // en la misma cola de trazas (`JSERR`/`ENGERR`) y se manda de forma SÍNCRONA
  // antes de que la página muera; el latido `FPSLOG` de cada segundo que falta
  // después marca el instante exacto del cuelgue.
  window.__vcPedClumpFatal = (text) => {
    try {
      const full = 'PEDCLUMPDUMP ' + String(text || '');
      const trunc = full.length > 49152;
      const line = full.slice(0, 49152) + (trunc ? ' truncado=1' : '');
      const q = (window.__odq = window.__odq || []);
      q.unshift(line);
      flush(true);
    } catch (e) {}
  };
  const pushFatal = (tag, what) => {
    try {
      // 21/10: 600 caracteres cortaban el stack del crash a la mitad (el frame
      // que lo peta es el que faltaba). Con 4000 entra el stack entero.
      const txt = String(what == null ? what : what).replace(/\s+/g, ' ').slice(0, 4000);
      if (txt) (window.__odq = window.__odq || []).push(tag + ' ' + txt);
    } catch (e) {}
    try { flush(true); } catch (e) {}
  };
  // 24/10: el stderr del motor no llegaba a la traza. `printErr` estaba cableado
  // a `log()`, que escribe en la consola y en el DOM pero NO encola en
  // `window.__odq`: un aborto del motor ("Aborted(...)", "Out of memory",
  // "Cannot enlarge memory arrays") se perdia justo en el crash. El hook de
  // `console.error` tampoco lo veia, porque `printErr` nunca pasa por ahi.
  const traceOut = (tag, what, sync) => {
    try {
      const txt = String(what == null ? what : what).replace(/\s+/g, ' ').slice(0, 4000);
      if (txt) (window.__odq = window.__odq || []).push(tag + ' ' + txt);
    } catch (e) {}
    if (sync) { try { flush(true); } catch (e) {} }
  };
  window.addEventListener('pagehide', onPageHide);
  window.addEventListener('error', (e) => {
    const msg = String(e?.error?.stack || e?.message || e).slice(0, 2000);
    console.log('[FATAL] ' + msg);
    pushFatal('JSERR', msg);
  });
  window.addEventListener('unhandledrejection', (e) => {
    const msg = String(e?.reason?.stack || e?.reason || e).slice(0, 2000);
    console.log('[REJECT] ' + msg);
    pushFatal('JSERR_REJ', msg);
  });
  // El motor avisa de sus fallos duros por stderr (emscripten `printErr` →
  // console.error): "Aborted(...)", "RuntimeError: ...", "Out of memory".
  // Se reenvía el texto a la traza y se deja pasar a la consola como siempre.
  try {
    const origErrLog = console.error ? console.error.bind(console) : null;
    console.error = function (...args) {
      try {
        const s = args.map((a) => (typeof a === 'string' ? a : String(a))).join(' ');
        if (/Aborted|RuntimeError|Out of memory|Cannot enlarge memory|abort\(|unreachable/i.test(s)) pushFatal('ENGERR', s);
      } catch (e) {}
      if (origErrLog) origErrLog(...args);
    };
  } catch (e) {}
  // 24/10: un cuelgue SIN mensaje (ni JSERR ni ENGERR) no es una excepción del
  // motor: es el proceso de la pestaña muriendo (memoria del renderer o GPU).
  // Lo único que la página puede distinguir en ese caso es la pérdida del
  // contexto WebGL, así que queda escrito en la traza con el consumo del
  // momento antes de que la pestaña desaparezca.
  // 81.3: Chrome puede tirar el contexto WebGL (memoria del proceso GPU) sin
  // matar la pestaña. Hasta ahora se anotaba y el motor seguia 30 s pintando
  // contra un contexto muerto: `WEBGL_LOST` en la sesion 81.2 y luego 3 fps con
  // el canvas en blanco, que es la "muerte" que ve el jugador. Ahora se para el
  // bucle (no se gasta bateria contra un GL muerto) y se ofrece recargar. El
  // preventDefault es lo que permite que el navegador llegue a restaurarlo.
  const ctxOverlay = () => {
    if (document.getElementById('vc-ctx-lost')) return;
    const d = document.createElement('div');
    d.id = 'vc-ctx-lost';
    d.style.cssText = 'position:fixed;inset:0;z-index:2147483647;display:flex;align-items:center;' +
      'justify-content:center;flex-direction:column;gap:16px;background:rgba(8,8,12,.93);color:#fff;' +
      'font:16px/1.5 system-ui,sans-serif;text-align:center;padding:24px';
    d.innerHTML = '<div style="font-size:19px;font-weight:600">Se perdió el contexto gráfico</div>' +
      '<div style="opacity:.8;max-width:420px">El navegador cerró WebGL (memoria de la GPU). ' +
      'Lo que tengas guardado no se pierde.</div>' +
      '<button id="vc-ctx-reload" style="padding:12px 22px;border-radius:10px;border:0;' +
      'background:#e8b400;color:#111;font-size:16px;font-weight:600">Recargar</button>';
    document.body.appendChild(d);
    d.querySelector('#vc-ctx-reload').addEventListener('click', () => location.reload());
  };
  const onCtxLost = (e) => {
    let mem = '';
    try {
      const m = performance.memory;
      if (m) mem = ' heapJS=' + Math.round(m.usedJSHeapSize / 1048576) + 'MB/' +
                     Math.round(m.jsHeapSizeLimit / 1048576) + 'MB';
    } catch (err) {}
    pushFatal('WEBGL_LOST', 'contexto WebGL perdido (' + ((e && e.statusMessage) || 'sin motivo') + ')' + mem);
    try { if (e && e.preventDefault) e.preventDefault(); } catch (err) {}
    try { if (typeof Module !== 'undefined' && Module.pauseMainLoop) Module.pauseMainLoop(); } catch (err) {}
    try { if (typeof touch !== 'undefined' && touch.setEnabled) touch.setEnabled(false); } catch (err) {}
    try { ctxOverlay(); } catch (err) {}
  };
  try {
    canvas.addEventListener('webglcontextlost', onCtxLost, false);
    canvas.addEventListener('webglcontextrestored', () => {
      pushFatal('WEBGL_OK', 'contexto WebGL restaurado');
      // El motor no sabe reconstruir sus objetos GL: se recarga la pagina.
      location.reload();
    }, false);
  } catch (e) {}

  // ---- contrato con el motor (globals que llama el C++) ------------------
  // El motor avisa aquí la PRIMERA vez que presenta un frame (glfw.cpp,
  // EmscriptenTick). Es la señal buena de "ya hay juego en pantalla".
  // Identidad del paquete que está corriendo la PÁGINA (para que el arnés pueda
  // comprobar que mide lo que cree: ficheros de disco ≠ lo que sirve la caché).
  window.__vcVersion = VERSION;
  window.__vcFrame = () => {
    window.__vcFirstFrame = true;
    try { touch.syncVisibility(); } catch (e) {}
    barTrace('on motivo=motor-primer-frame');
    log('[lib] el motor está pintando: se retira la barra de preparación.');
    engineOwnsScreen('motor-primer-frame');
  };
  window.__loadProgress = (step, total) => {           // CGame::InitialiseStep
    // El motor está inicializando el mundo (empezar o cargar partida): se
    // acabó la preparación previa -> barra del top fuera.
    engineOwnsScreen('init-step');
    report('engine', total ? step / total : null, `Fase ${step}/${total}`, total ? step < total : true);
  };
  // Portada DOM de carga de partida: reserva para hosts donde el splash GL no
  // se ve (hoy el motor la pinta él mismo y esta no se usa). No toca la barra
  // del top: si aparece, ya es pantalla del motor.
  window.__loadOverlay = (show, pct, label) => {
    engineOwnsScreen('load-overlay');
    loadEl.hidden = !show;
    if (show) {
      loadBar.style.width = Math.max(0, Math.min(100, pct)).toFixed(1) + '%';
      if (label) loadLabel.textContent = label;
      report('engine', pct / 100, label, pct < 100);
    }
  };
  window.__vcLog = logLines;
  window.__vcDataReady = () => !!(dataOk && runtimeReady);
  window.__vcGalleryDownload = () => downloadAllGallery();
  window.__vcGalleryClear = () => clearGallery();
  window.__vcMusicImport = () => importMusic();
  window.__vcMusicDeleteAll = () => deleteAllMusic();
  window.__vcMusicDownloadAll = () => downloadAllMusic();
  window.__vcMusicRescan = () => rescanUserMusic();
  window.__vcMusicInfo = () => musicInfo();
  window.__vcSaveManager = () => { window.__vcWebSaveManager = 1; };
  window.__vcSaveDownload = (slot) => downloadSlot(Number(slot));
  window.__vcSaveDownloadAll = () => downloadAll();
  window.__vcSaveImport = () => importSavesFromPicker();
  window.__vcWebSaveNoSpace = 0;
  window.__vcWebSaveImported = 0;
  window.__vcWebSaveManager = 0;
  window.__vcWebSkinResult = 0;
  window.__vcWebSkinRescan = 0;
  window.__vcWebMusicRescan = 0;
  window.__vcWebMusicImported = 0;
  window.__vcSkinImport = () => importSkinsFromPicker();
  window.__vcSkinDelete = (name) => deleteSkin(String(name));

  // ---- audio: sin botón, se abre con el primer gesto ---------------------
  async function unlockAudio() {
    let ok = false, note = 'sin contexto aún';
    try {
      const ctxs = [];
      if (typeof Module !== 'undefined' && Module?.SDL2?.audioContext) ctxs.push(Module.SDL2.audioContext);
      if (typeof AL !== 'undefined' && AL.currentCtx?.audioCtx) ctxs.push(AL.currentCtx.audioCtx);
      for (const ctx of ctxs) {
        if (ctx.state === 'suspended') await ctx.resume();
        if (ctx.state === 'running') ok = true;
      }
      note = ctxs.length ? ctxs.map((c) => c.state).join(',') : note;
    } catch (e) { note = String(e).slice(0, 80); }
    if (ok !== audioUnlocked) {
      audioUnlocked = ok;
      emit('audio', ok);
      if (opts.onAudio) opts.onAudio(ok);
    }
    return ok ? true : note;
  }
  const gestureEvents = ['pointerdown', 'keydown', 'touchstart'];
  const onGesture = async () => {
    if (!opts.audio || audioUnlocked) return;
    if (await unlockAudio() === true) for (const ev of gestureEvents) window.removeEventListener(ev, onGesture, true);
  };
  if (opts.audio) for (const ev of gestureEvents) window.addEventListener(ev, onGesture, true);

  // ---- pantalla completa (F11 y método público) --------------------------
  async function enterFullscreen() {
    try { await el.requestFullscreen({ navigationUI: 'hide' }); } catch (e) { log('[lib] pantalla completa: ' + e); }
    try { if (navigator.keyboard?.lock) await navigator.keyboard.lock(['Escape', 'KeyW', 'KeyA', 'KeyS', 'KeyD']); } catch (e) {}
  }
  async function exitFullscreen() { try { if (doc.fullscreenElement) await doc.exitFullscreen(); } catch (e) {} }
  function isFullscreen() { return doc.fullscreenElement === el; }
  function toggleFullscreen() { return isFullscreen() ? exitFullscreen() : enterFullscreen(); }
  const onFsChange = () => {
    const on = isFullscreen();
    emit('fullscreen', on);
    if (opts.onFullscreen) opts.onFullscreen(on);
    if (on) hideBar();
  };
  doc.addEventListener('fullscreenchange', onFsChange);

  // Esc corto = nada; mantener Esc 3 s = salir de pantalla completa (Esc sigue
  // sirviendo al juego). SIN aviso en pantalla a propósito: no queremos un
  // cartel ni una barra encima del juego.
  const HOLD_MS = 3000;
  let escTimer = 0;
  const onKeyDown = (e) => {
    if (opts.fullscreenKey && (e.code === 'F11' || e.key === 'F11')) {
      e.preventDefault();
      toggleFullscreen();
      return;
    }
    if (e.key !== 'Escape' || escTimer) return;
    escTimer = setTimeout(() => { escTimer = 0; exitFullscreen(); }, HOLD_MS);
  };
  const onKeyUp = (e) => {
    if (e.key !== 'Escape') return;
    if (escTimer) { clearTimeout(escTimer); escTimer = 0; }
  };
  window.addEventListener('keydown', onKeyDown);
  window.addEventListener('keyup', onKeyUp);

  // ---- ratón (solo al hacer clic en el juego) ----------------------------
  //
  // Un solo `click` como disparador no bastaba: si el navegador concede el
  // bloqueo pero la ventana no tiene el foco (pasa al cargar partida, al volver
  // de ESC o de otro programa), el cursor se queda oculto y el juego no recibe
  // clics hasta que el jugador cambia de ventana y vuelve. Ahora el disparador
  // es `pointerdown` en captura (nadie puede comerse el evento) y el bloqueo se
  // vuelve a pedir en cada pulsación mientras falte, con la ventana y el canvas
  // enfocados de forma explícita.
  let wantLock = false;
  async function grabPointer() {
    if (!opts.pointerLock || touch.enabled) return;
    try { window.focus(); } catch (e) {}
    try { canvas.focus({ preventScroll: true }); } catch (e) {}
    if (doc.pointerLockElement === canvas) return;
    try { await canvas.requestPointerLock(); } catch (e) { log('pointer lock: ' + e); }
  }
  const onPointerDownCapture = (e) => {
    if (e && e.target && touch.layer && touch.layer.contains(e.target)) {
      if (opts.audio) unlockAudio();
      return;
    }
    if (e && e.target && e.target !== canvas && !canvas.contains(e.target)) return;
    wantLock = true;
    if (opts.audio) unlockAudio();
    if (doc.pointerLockElement !== canvas) grabPointer();
  };
  window.addEventListener('pointerdown', onPointerDownCapture, true);
  window.addEventListener('mousedown', onPointerDownCapture, true);
  const onPointerLockChange = () => {
    const locked = doc.pointerLockElement === canvas;
    canvas.style.cursor = locked ? 'none' : 'default';
  };
  doc.addEventListener('pointerlockchange', onPointerLockChange);
  doc.addEventListener('pointerlockerror', () => {
    // Chrome rechaza el bloqueo si se pide demasiado pronto tras un ESC: no se
    // insiste en bucle, se reintenta en la siguiente pulsación.
    log('[lib] pointer lock rechazado; se reintenta al hacer clic');
  });
  // Al perder el foco, Chrome conserva el pointer lock pero el canvas deja de
  // recibir eventos: el raton queda oculto y "muerto" hasta soltarlo con Esc.
  // Se suelta al perder el foco y el siguiente clic lo vuelve a pedir.
  window.addEventListener('blur', () => {
    canvas.style.cursor = 'default';
    if (doc.pointerLockElement === canvas) { try { doc.exitPointerLock(); } catch (e) {} }
  });
  // Al recuperar el foco, el canvas tiene que ser el elemento activo o el
  // navegador entrega los clics al documento y no al juego.
  window.addEventListener('focus', () => {
    if (wantLock && doc.pointerLockElement === canvas) { try { canvas.focus({ preventScroll: true }); } catch (e) {} }
  });
  doc.addEventListener('visibilitychange', () => {
    if (!doc.hidden && wantLock && doc.pointerLockElement === canvas) {
      try { canvas.focus({ preventScroll: true }); } catch (e) {}
    }
  });

  // ---- protecciones e IDBFS ---------------------------------------------
  const onBeforeUnload = (e) => {
    try { if (typeof OD !== 'undefined') OD.syncUserfiles(); else if (typeof FS !== 'undefined') FS.syncfs(false, () => {}); } catch (err) {}
    if (opts.guardUnload && window.__gameBooted && !window.__quitting) e.preventDefault();
  };
  const onVisibility = () => {
    if (!doc.hidden) return;
    try { if (typeof OD !== 'undefined') OD.syncUserfiles(); } catch (err) {}
  };
  window.addEventListener('beforeunload', onBeforeUnload);
  doc.addEventListener('visibilitychange', onVisibility);

  // ---- jank probe: peor frame de cada segundo a las trazas (F4c/FPSLOG) --
  function jankProbe() {
    if (!traceUrl) return;
    try {
      window.__jankM = { xhrN: 0, xhrMs: 0, domN: 0, domMs: 0 };
      let t0 = performance.now(), maxd = 0, idx = 0, n = 0, over17 = 0, heap0 = 0, minHeap = 0;
      const ivs = [];
      let x0 = { n: 0, ms: 0 }, d0 = { n: 0, ms: 0 };
      const tick = (now) => {
        const d = now - t0; t0 = now; idx++;
        n++; if (d > 17.5) over17++;
        ivs.push(d); if (d > maxd) maxd = d;
        const used = performance.memory ? performance.memory.usedJSHeapSize : 0;
        if (idx === 1) { heap0 = used; minHeap = used; }
        if (used && used < minHeap) minHeap = used;
        // Parada larga cazada en el acto (tag ODSTALL): el FPSLOG de cada 60
        // frames dice que hubo tirón, pero no con qué coincidió. Aquí se copian
        // los contadores del pool de modelos en ese mismo instante, así un
        // maxdelta grande se puede atar (o descartar) a un desalojo o a una
        // re-descarga. Máximo una línea cada 2 s para no inundar la traza.
        if (d > 150) {
          const tn = now | 0;
          if (!tick.stall || tn - tick.stall > 2000) {
            tick.stall = tn;
            try {
              if (typeof OD !== 'undefined' && OD.liveBytes !== undefined) {
                window.__odq.push('ODSTALL d=' + Math.round(d) + 'ms heap=' +
                  Math.round((used || 0) / 1048576) + 'MB pool=' + Math.round(OD.liveBytes / 1048576) +
                  'MB ev=' + (OD.evicted | 0) + '/' + Math.round((OD.evictedBytes | 0) / 1048576) +
                  'MB fmb=' + Math.round((OD.fetchedBytes | 0) / 1048576) + 'MB');
              }
            } catch (e) {}
          }
        }
        if (idx >= 60) {
          const s = ivs.slice().sort((a, b) => a - b);
          const med = s[Math.floor(s.length / 2)];
          const xr2 = (window.__jankM.xhrN - x0.n), xm2 = (window.__jankM.xhrMs - x0.ms);
          const dn = (window.__jankM.domN - d0.n), dm = (window.__jankM.domMs - d0.ms);
          let pool = '';
          try {
            if (typeof OD !== 'undefined' && OD.liveBytes !== undefined) {
              pool = ' pool=' + Math.round(OD.liveBytes / 1048576) + 'MB ev=' + (OD.evicted | 0) +
                '/' + Math.round((OD.evictedBytes | 0) / 1048576) + 'MB fmb=' +
                Math.round((OD.fetchedBytes | 0) / 1048576) + 'MB';
            }
          } catch (e) {}
          window.__odq.push('FPSLOG frames=' + n + ' over17=' + over17 + ' maxdelta=' + Math.max(0, Math.round(maxd)) +
            ' med=' + Math.round(med) + ' heap0=' + Math.round(heap0 / 1048576) + 'MB freed=' + Math.round((heap0 - minHeap) / 1048576) + 'MB' +
            ' xhr=' + xr2 + '/' + Math.round(xm2 * 10) / 10 + 'ms@' + xr2 + ' dom=' + dn + '/' + Math.round(dm * 10) / 10 + 'ms' + pool);
          t0 = now; maxd = 0; idx = 0; n = 0; over17 = 0; ivs.length = 0;
          heap0 = used; minHeap = used;
          x0 = { n: window.__jankM.xhrN, ms: window.__jankM.xhrMs };
          d0 = { n: window.__jankM.domN, ms: window.__jankM.domMs };
        }
        if (!destroyed) requestAnimationFrame(tick);
      };
      requestAnimationFrame(tick);
    } catch (e) {}
  }

  // ---- ¿están el build y los datos? --------------------------------------
  async function probeBuild() {
    const [js, wasm, data] = await Promise.all([
      exists(buildUrl + 'reVC.js'), exists(buildUrl + 'reVC.wasm'), exists(buildUrl + 'reVC.data'),
    ]);
    return { js, wasm, data, ok: js && wasm };
  }

  // Los datos se comprueban contra el manifiesto (1 petición, sin 404s):
  // si trae los ficheros centinela, NO se toca assetsUrl para nada.
  async function probeData() {
    const need = ['models/gta3.dir', 'audio/sfx.sdt', 'data/main.scm', 'text/spanish.gxt'];
    let manifest = null;
    try {
      const r = await fetch(manifestUrl, { cache: 'no-store' });
      if (r.ok) manifest = await r.json();
    } catch (e) {}
    if (manifest) {
      const missing = need.filter((k) => !manifest[k]);
      if (!missing.length) return { ok: true, manifest, missing: [] };
      return { ok: false, manifest, missing, reason: 'streamed-incompleto', assets: assetsUrl ? await exists(assetsUrl + 'data/main.scm') : false };
    }
    return {
      ok: false, manifest: null, missing: need, reason: 'sin-manifiesto',
      assets: assetsUrl ? await exists(assetsUrl + 'data/main.scm') : false,
    };
  }

  // ---- app instalable y sin conexión --------------------------------------
  async function registerServiceWorker() {
    if (!('serviceWorker' in navigator) || !window.isSecureContext) return null;
    try {
      await navigator.serviceWorker.register('/sw.js?v=' + encodeURIComponent(VERSION));
      const reg = await Promise.race([
        navigator.serviceWorker.ready,
        new Promise((res) => setTimeout(() => res(null), 10000)),
      ]);
      if (!reg) return null;
      const worker = reg.active || reg.waiting || reg.installing;
      if (!worker || worker.state === 'activated') return reg;
      await new Promise((res) => {
        const stop = () => { worker.removeEventListener('statechange', stop); res(); };
        worker.addEventListener('statechange', stop);
        setTimeout(stop, 8000);
      });
      return reg;
    } catch (e) {
      log('[lib] service worker: ' + e);
      return null;
    }
  }

  async function cacheBuild(reg) {
    const worker = reg && (reg.active || reg.waiting);
    if (!worker) return false;
    try { if (new URL(buildUrl, location.href).origin !== location.origin) return false; } catch (e) { return false; }
    const urls = ['reVC.js', 'reVC.wasm', 'reVC.data'].map((f) => buildUrl + f + '?v=' + VERSION);
    const id = 'build-' + Date.now().toString(36);
    return await new Promise((res) => {
      let timer = 0;
      const stop = (ok) => {
        navigator.serviceWorker.removeEventListener('message', onMsg);
        clearTimeout(timer);
        res(ok);
      };
      const onMsg = (e) => {
        const m = e.data || {};
        if (m.type !== 'build' || m.id !== id) return;
        if (m.total) progress('build', m.done / m.total, 'Motor guardado para jugar sin conexión: ' + Math.round(100 * m.done / m.total) + '%');
        if (m.finished) stop(true);
      };
      navigator.serviceWorker.addEventListener('message', onMsg);
      timer = setTimeout(() => stop(false), 20 * 60 * 1000);
      try { worker.postMessage({ type: 'prefetch-build', id, urls }); } catch (e) { stop(false); }
    });
  }

  // ---- arranque ----------------------------------------------------------
  function boot() {
    canvas.hidden = false;
    window.__gameBooted = true;
    emit('booted', true);

    window.Module = {
      canvas,
      noInitialRun: !!opts.noInitialRun,
      // El build vive en buildUrl, la página en otro sitio: sin esto el .data
      // se pediría a la raíz.
      //
      // ve19: `?v=<VERSION>` en el código (`.js`/`.wasm`): el navegador puede
      // quedarse con el motor anterior — el nombre no cambia entre builds — y
      // el jugador prueba código viejo sin saberlo.
      // T57: tambien en el `.data`. El paquete de arranque lleva los modelos
      // (DFF) dentro, asi que sin parametro el navegador seguia arrancando con
      // los modelos de la ronda anterior aunque el build fuese nuevo. Se paga
      // una descarga completa por version; es el precio de que lo servido y lo
      // jugado coincidan.
      locateFile: (path) => buildUrl + path + (/\.(js|wasm|data)$/.test(path) ? '?v=' + VERSION : ''),
      print: (t) => { log(t); traceOut('ENGSTDOUT', t, false); },
      printErr: (t) => { log('[err] ' + t); traceOut('ENGERR', t, true); },
      // Guardados persistentes: /userfiles vive en IndexedDB (IDBFS). El resto
      // de assets va en MEMFS (lectura síncrona del motor).
      // 82.9-T7: copiar los ficheros de arranque (DATA/*, TEXT/*) a MEMFS antes
      // de que empiece el motor. OD.bootPreload aguanta main con addRunDependency.
      preRun: [() => {
        // 77.3: topes del pool on-demand aplicados desde aquí, sin esperar al
        // reenlace del motor (el pre-js vive dentro de reVC.js). Medido en la
        // sesión 77.0 del 19:33: con el heap JS en 562-604 MB el aviso del pre-js
        // desalojó 6.100 ficheros (live 233->168 MB) y el heap NO bajó ni un MB;
        // el motor los volvió a pedir acto seguido (`evict=6419/222MB`). Con el
        // aviso alto solo se traza, y CAP sube un escalón (+16 MB) porque el pool
        // tocaba techo en partida (`live=238MB` de 240). Si el móvil matase la
        // pestaña, volver CAP a 240 aquí abajo antes que tocar nada más.
        // 79.0 · El techo de 256 MB era el que se comía la partida: medido en
        // la sesión 77.3 de las 21:00 (`ODCAP`/`FPSLOG`), MEMFS llegaba a 464 MB
        // (live=253 de 256) y el heap JS a 480-532 MB con `freed=0MB`. A partir
        // del minuto 1:30 los fps caían de ~100 a 30-40 y los parones subían de
        // 150 ms a 6.129 ms (`ODSTALL`): es el GC de V8 con casi 1 GB de renderer
        // en un móvil, no el render. Con CAP 144 el working set queda por debajo
        // del de 256 y cubre los ~134 MB medidos; no se sube más por el
        // historial de GC. Lo desalojado sigue en IndexedDB y vuelve sin red.
        // dlMax 48 → 8: 48 descargas simultáneas dejaban hasta ~200 MB de
        // buffers vivos de golpe (el heap JS saltaba a 516 MB antes de terminar
        // de cargar el mundo) y V8 no devolvía esa memoria en toda la sesión.
        try {
          if (typeof OD !== 'undefined') {
            // 81.7: 256 -> 192 MB. En la sesión 81.6 el pool se pegó al tope
            // (`live=255/256MB`), MEMFS subió a 466 MB de heap JS y las texturas
            // a 212 MB, donde el contexto WebGL murió. Con OPFS una relectura
            // cuesta 10-20 ms, no los 200-1428 ms medidos por IDB.
            OD.CAP = 192 * 1048576;
            OD.HEAP_HI = 900 * 1048576;
            OD.dlMax = 8;
            try {
              (window.__odq = window.__odq || []).push('ODCFG cap=' +
                Math.round(OD.CAP / 1048576) + 'MB dlmax=' + OD.dlMax +
                ' gracia=' + Math.round((OD.GRACE_MS || 0) / 1000) + 's');
            } catch (err) {}
          }
        } catch (e) {}
        // 77.4 · Las emisoras se comían el pool de modelos. Medido en la sesión
        // 77.0 de las 19:48 (`ODCAP`): `/Audio:206-236MB` de los 414 MB de MEMFS,
        // con `live=211-217MB` sobre un tope de 256. Cada emisora son ~30 MB
        // (`manifest.json`: 274 MB las nueve) y la anterior se quedaba residente
        // al cambiar, así que al motor le quedaban ~40 MB para modelos:
        // `evict=8163/396MB`, 8.282 re-lecturas de caché y 4.696 ficheros
        // aplazados en 22 minutos. Aquí se sueltan las emisoras que no suenan
        // (se conservan la actual y la que se precarga); los bytes siguen en
        // IndexedDB y vuelven en cuanto el motor las pida.
        try {
          if (typeof OD !== 'undefined' && !OD._shedStations) {
            OD._shedStations = true;
            OD.dropStations = (keepA, keepB) => {
              const keep = new Set([keepA, keepB].filter(Boolean).map((s) => String(s).toUpperCase()));
              for (const p of Array.from(OD.live.keys())) {
                if (!/\.adf$/i.test(p)) continue;
                const name = p.slice(p.lastIndexOf('/') + 1).replace(/\.adf$/i, '').toUpperCase();
                if (keep.has(name)) continue;
                const e = OD.live.get(p);
                try { FS.unlink(p); } catch (err) {}
                OD.live.delete(p);
                OD.liveBytes -= (e && e.s) || 0;
              }
            };
            const noteStation = OD.noteStation;
            if (typeof noteStation === 'function') {
              OD.noteStation = function (station) {
                const r = noteStation.apply(OD, arguments);
                try {
                  const i = OD.radios ? OD.radios.indexOf(station) : -1;
                  const next = i >= 0 ? OD.radios[(i + 1) % OD.radios.length] : null;
                  OD.dropStations(station, next);
                } catch (err) {}
                return r;
              };
            }
          }
        } catch (e) {}
        try {
          FS.mkdir('/userfiles');
          FS.mount(IDBFS, {}, '/userfiles');
          try { FS.mkdir('/MP3'); } catch (e) {}
          try { FS.mount(IDBFS, {}, '/MP3'); } catch (e) {}
          // El juego NO espera solo: sin la dependencia leía MEMFS vacío
          // antes de que bajara IDB (carrera de lectura).
          if (typeof addRunDependency !== 'undefined') addRunDependency('idbfs-populate');
          window.__vcIdbReady = false;
          FS.syncfs(true, (err) => {
            window.__vcIdbReady = !err;
            traceOut('IDBFS', err ? 'load-fail ' + err : 'load-ok', false);
            log(err ? '[err] IDBFS load: ' + err : 'IDBFS listo (guardados persistentes).');
            try { restoreSkins(); } catch (e) {}
            try { if (typeof removeRunDependency !== 'undefined') removeRunDependency('idbfs-populate'); } catch (e) {}
          });
          setTimeout(() => {
            try { if (typeof removeRunDependency !== 'undefined') removeRunDependency('idbfs-populate'); } catch (e) {}
            if (!window.__vcIdbReady) traceOut('IDBFS', 'timeout-15s sin poblar', false);
          }, 15000);
        } catch (e) { log('[err] IDBFS: ' + e); }
      }],
      // El loader reporta "Downloading data... (123/456)": va a la barra.
      setStatus: (t) => {
        if (!t || engineUp) return;
        const m = t.match(/\((\d+)\s*\/\s*(\d+)\)/);
        // Con varios ficheros el contador es un progreso real. Con uno solo
        // (reVC.data) un "0%" clavado durante toda la descarga sería mentira:
        // la barra pasa a "trabajando" hasta que el motor arranque.
        if (m && +m[2] > 1) {
          progress('build', +m[1] / +m[2], `Descargando datos: ${Math.round(100 * m[1] / m[2])}%`);
        } else if (m) {
          progress('build', null, 'Descargando datos del juego…');
        } else {
          progress('build', null, t);
          if (!/Downloading data/.test(t)) log('[status] ' + t);
        }
      },
      onRuntimeInitialized: () => {
        runtimeReady = true;
        log('Runtime WASM listo, arrancando juego…');
        // El motor avisa por __vcFrame en cuanto presenta su primer frame
        // (unos cientos de ms). Este plazo es solo la red por si esa señal no
        // llegara (build antigua); antes eran 5 s y la barra se quedaba encima
        // del menú ya visible.
        clearTimeout(idleTimer);
        idleTimer = setTimeout(() => engineOwnsScreen('runtime-timeout'), 2500);
        jankProbe();
        emit('ready', true);
        if (opts.onReady) opts.onReady();
      },
    };

    const s = doc.createElement('script');
    // `?v=` = tag de esta ronda: el motor (`reVC.js`/`reVC.wasm`) no se puede
    // quedar en la caché del navegador de un build anterior.
    s.src = buildUrl + 'reVC.js?v=' + VERSION;
    s.async = true;
    s.onerror = () => fail(`No se pudo cargar <code>${buildUrl}reVC.js</code>.`);
    el.appendChild(s);
    log('Descargando datos del juego… (' + buildUrl + ' motor ' + VERSION + ')');
  }

  const handle = {
    ok: true,
    version: VERSION,
    el, root: el, canvas, config: { buildUrl, streamedUrl, manifestUrl, assetsUrl, fill: opts.fill, traceUrl },
    get module() { return typeof Module !== 'undefined' ? Module : null; },
    get progress() { return lastProgress; },
    get logs() { return logLines; },
    log,
    // showBar explícito del host manda (la política automática solo la
    // enciende en la preparación previa al motor).
    showBar: () => { if (!engineUp) { barEl.dataset.on = ''; barTrace('on (host)'); } },
    hideBar, setProgress: progress, done,
    // Caché de datos del navegador (IndexedDB 'vcod2'): la llena el propio
    // juego. Cuánto ocupa y cómo vaciarla sin tocar DevTools.
    cacheInfo() {
      const od = typeof OD !== 'undefined' ? OD : null;
      return { capMB: od?.cfg?.idbCapMB ?? null, warmMB: od?.cfg?.warmMB ?? null, trimmed: od?.trimmed || 0 };
    },
    // Vacía la caché de datos (los ficheros se re-descargan cuando hagan falta)
    // y olvida la lista de arranque en caliente. Los guardados NO se tocan:
    // viven en /userfiles (IDBFS de Emscripten), no en esta caché.
    async clearDataCache() {
      const od = typeof OD !== 'undefined' ? OD : null;
      if (!od) return false;
      try { od.warmSeen = null; od.warmList = null; localStorage.removeItem(od.warmKey); } catch (e) {}
      await od.idbClear();
      log('[lib] caché de datos vaciada (se re-descargará lo que el juego pida)');
      return true;
    },
    // Contador de FPS del motor, en caliente (sin recargar).
    showFps(on = true) { globalThis.__vcShowFps = !!on; return !!on; },
    fpsVisible: () => !!globalThis.__vcShowFps,
    isFullscreen,
    enterFullscreen, exitFullscreen, toggleFullscreen,
    unlockAudio,
    audioActive: () => audioUnlocked,
    openSaveManager: () => { window.__vcWebSaveManager = 1; },
    importMusic, downloadSlot, downloadAll,
    deleteAllMusic, downloadAllMusic, musicInfo, rescanUserMusic, importSavesFromPicker,
    importSkinsFromPicker, skinInfo, restoreSkins,
    dataReady: () => !!(dataOk && runtimeReady),
    setTouchControls(on = true) { return touch.setEnabled(on); },
    touchControls: () => touch.enabled,
    touchContext: () => touch.context(),
    on(evt, fn) {
      if (!listeners[evt]) throw new Error('gtavc-web: evento desconocido ' + evt);
      listeners[evt].push(fn);
      return () => { const i = listeners[evt].indexOf(fn); if (i >= 0) listeners[evt].splice(i, 1); };
    },
    // Diagnóstico del FS virtual (con noInitialRun:true, sin arrancar el juego).
    fsProbe() {
      const out = (t) => log('[probe] ' + t);
      try {
        for (const e of performance.getEntriesByType('resource')) {
          if (/reVC\.(data|wasm|js)/.test(e.name)) {
            out(`${e.name.split('/').pop()} transfer=${e.transferSize} encoded=${e.encodedBodySize} decoded=${e.decodedBodySize} dur=${Math.round(e.duration)}ms`);
          }
        }
        out('cwd=' + FS.cwd());
        out('root=' + JSON.stringify(FS.readdir('/').slice(0, 20)));
        out('TEXT=' + JSON.stringify(FS.readdir('/TEXT')));
        out('spanish.gxt size=' + FS.stat('/TEXT/spanish.gxt').size);
        FS.chdir('/TEXT');
        const fd = FS.open('spanish.gxt', 'r');
        const buf = new Uint8Array(12);
        const n = FS.read(fd, buf, 0, 12, 0);
        out(`lower open+read -> n=${n} magic=${String.fromCharCode(...buf.slice(0, 4))}`);
        FS.close(fd);
      } catch (e) { out('FAIL: ' + (e && e.message || e)); }
      out('probe done');
    },
    // Descarga best-effort: el módulo wasm no se puede liberar de verdad en
    // una página, así que se para el bucle y se limpia lo nuestro.
    destroy() {
      if (destroyed) return;
      destroyed = true;
      try { endRecoilSession(); } catch (e) {}
      try { Module?.pauseMainLoop?.(); } catch (e) {}
      for (const t of timers) { clearTimeout(t); clearInterval(t); }
      window.removeEventListener('pagehide', onPageHide);
      window.removeEventListener('error', onAnyError);
      window.removeEventListener('unhandledrejection', onAnyError);
      window.removeEventListener('beforeunload', onBeforeUnload);
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
      for (const ev of gestureEvents) window.removeEventListener(ev, onGesture, true);
      doc.removeEventListener('visibilitychange', onVisibility);
      doc.removeEventListener('fullscreenchange', onFsChange);
      doc.removeEventListener('pointerlockchange', onPointerLockChange);
      try { touch.destroy(); } catch (e) {}
      el.innerHTML = '';
      el.classList.remove('vc-root');
      window.__gameBooted = false;
      current = null;
    },
  };
  current = handle;
  try { el.__vcGame = handle; } catch (e) {}

  // ---- puesta en marcha (ARRANCA SOLO, sin botones ni gestos) ------------
  (async () => {
    log('[lib] ' + VERSION + ' · build=' + buildUrl + ' streamed=' + streamedUrl);
    if (typeof location !== 'undefined') {
      log('buildTag ' + VERSION + ' (si no ves este tag, recarga con Ctrl+Shift+R)');
      if (opts.title) { try { doc.title = '[vc ' + VERSION + '] GTA Vice City'; } catch (e) {} }
    }
    if (!window.crossOriginIsolated) {
      log('[err] crossOriginIsolated=false: hacen falta COOP/COEP (same-origin + require-corp) para los pthreads del motor.');
    }
    progress('probe', null, 'Comprobando build…');

    const b = await probeBuild();
    // 77.4: una línea para saber qué ve el arranque cuando no hay red (el HEAD
    // no pasa por el service worker; con `sinred=1` y todo a 1 el GET de caché
    // hizo su trabajo).
    try {
      (window.__odq = window.__odq || []).push('OFFBUILD js=' + (b.js ? 1 : 0) + ' wasm=' + (b.wasm ? 1 : 0) +
        ' data=' + (b.data ? 1 : 0) + ' sinred=' + (navigator.onLine === false ? 1 : 0) + ' tag=' + VERSION);
    } catch (e) {}
    if (!b.ok) {
      handle.ok = false;
      if (opts.requireBuild) {
        return fail(
          `Falta el build de WebAssembly en <code>${buildUrl}</code> (reVC.js / reVC.wasm / reVC.data).`,
          '<ul><li>Compílalo: <code>gta_vc_browser\\build.bat</code> + <code>emmake ninja -C gta_vc_browser\\build\\web</code>.</li>' +
          '<li>O apunta <code>buildUrl</code> a donde lo tengas servido.</li></ul>');
      }
      log('[lib] build incompleto: se arranca igual (requireBuild:false)');
    }

    const d = await probeData();
    dataOk = !!d.ok;
    if (!d.ok) {
      handle.ok = false;
      const list = '<ul>' + d.missing.map((k) => `<li><code>${k}</code></li>`).join('') + '</ul>';
      if (opts.requireData) {
        if (d.reason === 'sin-manifiesto') {
          if (d.assets) return fail(`No se encontró <code>${manifestUrl}</code>: sin ese índice no se pueden pedir los datos sueltos.`,
            '<ul><li>Genera los datos y su manifiesto (pipeline del README, pasos 2-7).</li></ul>');
          return fail('Faltan los datos del juego (ni <code>' + manifestUrl + '</code> ni la copia original en <code>' + assetsUrl + '</code>).',
            '<ul><li>Coloca tu copia legal del juego en <code>' + assetsUrl + '</code> y corre el pipeline de datos (README).</li></ul>');
        }
        if (d.assets) return fail('Los datos sueltos están incompletos en <code>' + streamedUrl + '</code>.', list +
          '<ul><li>Corre el pipeline de datos (README, pasos 2-7).</li></ul>');
        return fail('No hay datos servidos: falta <code>' + streamedUrl + '</code> y no hay copia original en <code>' + assetsUrl + '</code>.', list);
      }
      log('[lib] datos incompletos: se arranca igual (requireData:false)');
    }

    // El pre-js (ondemand.js) lee esta config al cargarse: de aquí salen las
    // rutas de streamed/ y del manifiesto para el motor.
    globalThis.__VC_CFG = {
      streamedUrl, manifestUrl, odtraceUrl: traceUrl, version: VERSION,
      recoilTraceId: recoilTraceActive ? recoilTraceId : null,
      idbCapMB: opts.idbCapMB, warmMB: opts.warmMB, worker: opts.worker !== false,
    };

    let sw = null;
    try { sw = await registerServiceWorker(); } catch (e) { sw = null; }
    log('[lib] ' + (sw ? 'app instalable: el envoltorio y el motor quedan guardados para jugar sin conexión' : 'sin service worker: hará falta conexión para abrir el juego'));
    if (sw) {
      try {
        const worker = sw.active || sw.waiting;
        const assets = performance.getEntriesByType('resource')
          .map((e) => e.name)
          .filter((n) => n.startsWith(location.origin + '/') && /\.(js|css)(\?|$)/.test(n));
        if (worker) worker.postMessage({ type: 'prefetch-shell', urls: assets });
      } catch (e) {}
    }
    try { if (navigator.storage && navigator.storage.persist) await navigator.storage.persist(); } catch (e) {}

    if (d.manifest) {
      progress('data', 0, 'Descargando datos del juego…');
      const r = await downloadGame({
        manifest: d.manifest, streamedUrl, packsUrl, log,
        onProgress: (p) => {
          const pct = p.totalBytes ? p.bytes / p.totalBytes : 1;
          progress('data', pct, 'Datos del juego: ' + p.done + '/' + p.total + ' · ' +
            Math.round(p.bytes / 1048576) + ' de ' + Math.round(p.totalBytes / 1048576) + ' MB');
        },
      });
      (window.__odq = window.__odq || []).push('OFFLINE datos=' + r.done + '/' + r.total +
        ' mb=' + Math.round(r.bytes / 1048576) + ' fallos=' + r.failed +
        ' completa=' + (r.complete ? 1 : 0) + ' sinred=' + (r.offline ? 1 : 0) + ' tag=' + VERSION);
    }

    if (sw) await cacheBuild(sw);

    progress('build', 0, 'Cargando motor…');
    boot();
  })();

  return handle;
}

export function getGame() { return current; }
export function isRunning() { return !!current && !!current.ok; }

export default startGame;

// 83.0-T9: el SW viejo servia /build/reVC.js cacheado aunque ?v cambiara
// (cache.match con ignoreSearch). Se registra en cada arranque y, cuando el
// nuevo toma el control, se recarga una vez para que el motor sea el de esta
// build. Sin esto, la pagina era T8 pero el motor seguia siendo T6.
try {
  if ('serviceWorker' in navigator && window.isSecureContext) {
    let swReloaded = false;
    navigator.serviceWorker.addEventListener('controllerchange', () => {
      if (swReloaded) return;
      swReloaded = true;
      const k = 'vc_sw_reload_' + VERSION;
      try { if (sessionStorage.getItem(k)) return; sessionStorage.setItem(k, '1'); } catch (e) {}
      location.reload();
    });
    navigator.serviceWorker.register('/sw.js?v=' + encodeURIComponent(VERSION)).catch(() => {});
  }
} catch (e) {}
