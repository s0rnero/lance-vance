# Vice Extended · matriz de cobertura (baseline 2026-10-13 · auditada 2026-10-14)

> AVISO DEL JUGADOR (2026-10-14), leer antes de usar esta tabla: los estados
> "PORTADO" de abajo NO son prueba de nada. Lo unico realmente hecho y
> validado es la mecanica de NADO y el trabajo reciente de RADIO (iconos de
> emisora + logo); el resto de features que describe el mod NO estan
> implementadas y el jugador las ve igual que vanilla. Esta tabla sirve como
> inventario de donde mirar, no como certificado. Objetivo vigente: plan
> `2026-10-14-12.0-mod-completo-extended`.
>
>
> SESION 2026-10-14 (tarde-noche), tandas `ve81`-`ve83`: el mod tiene arbol fuente
> publico (`x87/gta-extended`, rama `miami`) y el port es un fork anterior. De las 7
> etiquetas vacias quedan **2**. Cerradas y verificadas (build + suite de contratos
> + chequeo de lo servido): luna movil `ve81` (familia 5), luces lejanas `ve81`
> (familia 5), punteria de la IA `ve82` (familia 1) y ALT/sentarse `ve83` (familia 1).
> Detalle y anclas de lo que queda en la sesion del plan 12.0.

> SESION 2026-10-14 (noche, tanda `ve84`): familia 11 CERRADA. Su `features.ini` se
> sirve (`streamed/ViceExtended/features.ini`, 567 B, `bf884c129adb`, tambien en el
> paquete inicial) y el motor lo LEE al arrancar (`src/core/ViceExtFeatures.cpp`,
> traza `P4 kind=inifile` con clave/ini/aplica/origen/impl). De las 7 etiquetas vacias
> que habia al empezar el dia queda **1**: `VICEEXT_MAPDATA` (familia 7, mundo).

> SESION 2026-10-14 (noche, tanda `ve85`): familia 7 con lo suyo hecho. El mundo
> del mod se sirve (`data/object.dat`, `data/occlu.ipl`, `data/maps/bryx/*`) y lo
> vigila `tools/viceext-world-check.py` (cada referencia resuelve); su `gta_vc.dat`
> NO se sustituye, con motivo medido (bloque `CDIMAGE` + orden). `VICEEXT_MAPDATA`
> ya no esta vacia: traza `P4 kind=worlddata` (zonas del `occlu.ipl`) y bloque
> P4.16. **Etiquetas vacias: 0** (eran 7 al empezar el dia).

> SESION 2026-10-14 (noche, tanda `ve85c`): **el arranque real, hecho — y con un
> fallo dentro**. Servir ese mundo tumbaba el arranque: el `inst` del `bryx.ipl`
> del mod trae **`-1` como id** (2 de las 9135 lineas `inst` servidas; el vanilla
> nunca usa negativo) y el motor lo metia como indice de un array **sin comprobar
> rango** (`CModelInfo::GetModelInfo(int)`), leyendo la palabra de antes del array
> y llamando a la vtable que hubiera ahi (`RuntimeError: null function` en
> `LoadObjectInstance`, reproducible). Arreglado resolviendo el modelo **por
> nombre** cuando el id es negativo (ADR-015), que es lo que el fichero del mod
> quiere decir y lo mismo que ya validaba el chequeo del mundo. Tras el arreglo el
> arranque real pasa `initstep 0..18` y deja `P4 kind=worlddata ... zonas=631`,
> sin la caida; P4.16 y P4.15 dan **PASS** sobre ese log real. Sigue faltando la
> **partida** (la identidad P4 sale del ped en movimiento, y el save del mod no
> carga por el bloqueo de la familia 13: hay que **empezar partida nueva**).
> AUDITORÍA DE DATOS (2026-10-14, medida fichero a fichero). Instalación real
> del jugador: `C:\Users\s0rno\Desktop\Gamez\(g)old\Grand Theft Auto Vice City`,
> arrancada con `ViceEx.exe` (3.822.080 B, PE32 i386, sha256 `4df2726e0365…`),
> **idéntico byte a byte** al del repo (`mods/extended/GameFiles/ViceEx.exe`) y al
> de REA (`gta_vc_browser/tmp/rea/bin/ViceEx.exe`); el de `Bonus/x64_beta/` es
> otro binario (x86-64, 4.289.536 B, `8353745383…`). Ver «Deuda de datos medida»
> al final: de los datos del mod, **casi todo está servido o integrado por otra
> vía**. Había **3 huecos reales**, los tres bloqueados por código; el 2026-10-14
> se cerraron **los tres**: `particle.cfg`, `newWeapons.col` y el `main.scm` van
> ya bit a bit del mod. Lo que queda de la familia 13 es su **veredicto en
> partida**, y ese sí está bloqueado hoy, pero por otra cosa: el save del mod
> **no lo carga este motor** (ver la fila de `main.scm` en v2.5 y «Deuda de datos
> medida»).

Fuente: `mods/extended/ChangesEN.txt` (el changelog del mod = "todo lo del mod").
Estados: PORTADO · EN-CURSO (lote en vuelo) · EXCLUIDO (decisión del jugador)
· FUERA (sacado de la lista; hoy solo CLEO) · RECUPERADO (lo excluido que vuelve:
hoy el script del mod, familia 13 del plan 12.0) · INTEGRADO (ya está por otra vía,
p. ej. los vehículos nuevos) · N/A (no portable: empaquetado, plataforma, idiomas)
· POR-VERIFICAR (hay que mirar el código al cierre; se resuelve con grep).

Planes: 9.0 EXECUTED · 9.1 parcial (1,3,4,5-medida hechos; 2 excluido; 5b y 6
condicionales) · 9.2/9.3/9.4 ENRICHED, ejecución en paralelo en curso ·
**12.0 ENRICHED (objetivo vigente: TODO el mod)**.

## v2510

| Ítem | Estado |
|---|---|
| Grenade launcher (arma) | POR-VERIFICAR (sonidos mapeados ve57; falta confirmar el arma) |
| Boards Ammu-Nation/ferreterías | EN-CURSO 9.3 |
| Edificios Little Haiti | EN-CURSO 9.3 |
| Sonidos armas añadidas | PORTADO (ve57) |
| Sonido agua nadando | EXCLUIDO (9.1, jugador) |
| Shift en avisos | PORTADO 9.1.3 (combo pendiente de rebind) |
| Crash Micro-UZI | EN-CURSO 9.1.5 medida (fix pendiente de log) |
| Lanzagranadas comprable (Phil) | **RECUPERADO** (2026-10-14) → plan 12.0 familia 13: lo reparte el `main.scm` del mod, **ya servido**, y sus opcodes propios están en el motor; falta su veredicto en partida |

## v2506

| Ítem | Estado |
|---|---|
| Periódicos únicos | POR-VERIFICAR |
| Ángulo volante aleatorio al aparecer | POR-VERIFICAR |
| Mirror mode | POR-VERIFICAR |
| Crash al reiniciar partida | POR-VERIFICAR (¿lo cubre FIX_BUGS?) |
| Vértices noche d3d9 | EN-CURSO 9.4 medida (probable N/A técnico) |
| HD radar | POR-VERIFICAR (radardisc servido; reemplazo procedural descartado por jugador) |
| Camino tierra aeropuerto | POR-VERIFICAR (¿dato?) |
| Aim sensitivity fix | HECHO `ve80` (R3: `FUN_0049b290` elige par de mirar/apuntar por dispositivo; portado con `VICEEXT_AIM_SENSITIVITY`, traza `P4 kind=aimsens`, bloque `AW`). Falta su veredicto en partida |
| AI accuracy | EN-CURSO 9.2 |
| Pistas coleccionables en mapa | EN-CURSO 9.3 |
| Cambios librw | N/A (upstream) |
| Metal detector (script) | **RECUPERADO** (2026-10-14) → plan 12.0 familia 13: vive en el `main.scm` del mod, **ya servido** |

## v3.0

| Ítem | Estado |
|---|---|
| Modloader | N/A (ASI de terceros, LINK/2012; el port **aplana** el overlay en su lugar). Medido 2026-10-14: en la instalación del jugador su log muestra que `gta3/std.asi.dll` y `gta3/std.bank.dll` **fallan al arrancar** |
| Armas nuevas (DE/AK/Beretta/AUG/M16/Micro-UZI/escopeta) | PORTADO (lane D). Dato verificado 2026-10-14: el `weapon.dat` **servido ya es el del mod** (idéntico línea a línea, 103 líneas, con Beretta/Ak47/Gr_launch y la columna 27 `weapon sight`); solo cambian los fines de línea (CRLF→LF) |
| Tienda ropa/armario | POR-VERIFICAR (probable script/modelos) |
| Muertes en coche | EN-CURSO 9.2 |
| Carpeta separada | N/A |
| PC key icons | PORTADO (D7 + 9.1.3) |
| Wintergreen en tráfico | PORTADO (VICEEXT_POLICE_BIKE) |
| Photo Mode / fotos con cámara | POR-VERIFICAR |
| Miras de arma | PORTADO (D6) |
| Luces de servicio | PORTADO (R11/SVLIGHTS) |
| Triángulo de salud | PORTADO 9.1.6 (visto bueno visual) |
| Climbing / swimming / tuning aleatorio | PORTADO / PORTADO / POR-VERIFICAR |
|ង្គ Rocket 3ª persona | PORTADO |
| YCbCr SkyGFX | N/A (otro mod) |
| Coleccionables en mapa | EN-CURSO 9.3 |
| Blips/iconos HD | EN-CURSO 9.3 (+ radio DONE 9.1.1). **Medido 2026-10-14 en partida nueva**: el juego **se cae dibujando un blip** (`CSprite2d::Draw` ← `CRadar::DrawRadarSprite` ← `CRadar::DrawCoordBlip`, `memory access out of bounds`) desde los primeros segundos de juego; `DrawRadarSprite` indexa los sprites del radar SIN comprobar rango y los carga por NOMBRE desde `hud.txd`, asi que un blip del script del mod con un icono que ese TXD no trae revienta el frame. Es el primer muro para jugar y va con esta fila (9.3) |
| Ucraniano | EXCLUIDO (idioma) |
| CRAZYTOOLS | PORTADO (preexistente) |
| Script (Phil, misiones, reparto, autosave-script, GPS-script) | **RECUPERADO** (decisión del jugador, 2026-10-14) → plan 12.0 familia 13. El bloqueo por opcodes **ya no existe** (2026-10-14): los 10 opcodes propios (`0x0FA0`–`0x0FA9` = comandos 4000-4009) están implementados en `src/control/Script8.cpp` tras la bandera `VICEEXT_MOD_SCRIPT`, con sus constantes en `ScriptCommands.h` y su traza `P4 kind=modscript`. **Su `main.scm` ya se sirve** (2026-10-14, `41a932cc593e`): lo que falta es el veredicto en partida, ver la fila de v2.5 |

## v2.6

| Ítem | Estado |
|---|---|
| Ped variations | EXCLUIDO (9.4: otro plan de datos) |
| Distant lights | MEDIDO (R3, `.agents/mediciones-rea-r3.md`): bandera `[0xa0627f]` consumida en `0x50ba37` (`FUN_0050ba20`) y `0x4e72bd` (`FUN_004e7030`), que recorren los pools de **dummies** (`0x93839c`) y **edificios** (`0x938390`) hasta **530,0 m**. **NO portado**: falta la linea base vanilla para saber que anade |
| Iconos de emisora | PORTADO 9.1.1 |
| Borderless | N/A |
| Polis se agachan a cubierto | POR-VERIFICAR |
| Física blade sin jugador en heli | POR-VERIFICAR |
| Texturas iconos radio mejoradas | PORTADO (radio.txd 32 bits servido) |
| Proyectil explota al dispararle | POR-VERIFICAR |
| Object fixes | POR-VERIFICAR (¿datos?) |
| Gasolinera explota al chocar | POR-VERIFICAR (distinto de GAS_TANK) |
| Cambiar arma con detonador | POR-VERIFICAR |
| Recoil desactivable | PORTADO (flag off = vanilla) |
| Script (escena inicial, pizzero, pickups cámara/humo, garajes, ropa haitiana, coches aparcados, modelo Lance) | **RECUPERADO** (2026-10-14) → plan 12.0 familia 13: todo eso vive dentro del `main.scm` del mod, y sus opcodes propios ya están en el motor (ver v3.0). Su `models/coll/newWeapons.col` **ya está servido** (224 B, `a5b85b2b6cd5`) con su línea `COLFILE 0 MODELS\COLL\NEWWEAPONS.COL` en `streamed/data/default.dat`; **el script ya se sirve** y solo le falta el veredicto en partida |
| Blips farmacias/bomba/estadio/casas | EN-CURSO 9.3 |

## v2.5

| Ítem | Estado |
|---|---|
| Vehículos nuevos / conversor MVL | **INTEGRADOS por la vía del port** (familia 14 del plan 12.0): `tools/import_mvl_vehicles.py` escribió los IDs 6500–6507 en `default.ide` (con sus grupos de animación), `handling.cfg` y `carcols.dat`; `import_img.py` fusionó los modelos en `streamed/models/gta3.img/` (carpeta de 6.085 ficheros con `streetfi`, `manchez`, `wintergreen`, `polwintergreen`, `ak47`, `beretta`, `gr_launch`); `newvehicles.col` servido **idéntico** (`48f417725a60…`) y declarado en `default.dat` + `gta_vc.dat` |
| Autosave / save anywhere | PORTADO / PORTADO |
| Volante que gira | HECHO ve79 (dummy `steeringwheel`, `VICEEXT_CAR_ROTATE_WHEEL`; falta el veredicto visual del multiplicador 1,0 y del eje) |
| Depósito gasolina | PORTADO |
| Intermitentes | PORTADO |
| Textura luces coche | POR-VERIFICAR (¿9.2?) |
| Luces rompibles (+ experimental) | PORTADO |
| Humo/fuego parrilla | EN-CURSO 9.2. **Dato resuelto 2026-10-14**: su `particle.cfg` **ya se sirve** (`streamed/data/particle.cfg` = el del mod bit a bit, 18.363 B, `49130d2521a1`), porque el parser resuelve el tipo por **nombre** (`src/renderer/ParticleMgr.cpp`, bandera `VICEEXT_PARTICLE_NAMES`) en vez de por posición. Delta medido contra el que se servía: los **83 tipos comunes tienen valores idénticos** y el mod solo **añade 5 al final** (`ENGINE_SMOKE2_SMALL`, `ENGINE_STEAM_SMALL`, `ENGINE_SMOKE_SMALL`, `CARFLAME_SMALL`, `CARFLAME_SMOKE_SMALL32`); traza de carga `P4 kind=partcfg cargados=88 nuevos=5 desconocidos=0 total=88`, **ya medida dentro del motor** en el arranque headless del 2026-10-14 |
| 4 colores | POR-VERIFICAR |
| GPS | **RECUPERADO** (2026-10-14) → plan 12.0: los iconos/paneles son familia 7 y su parte de script (el GPS por script del changelog) va con la familia 13 |
| features.ini: coche volcado, regen salud, estrellas ocultas, shake cámara, tanque dispara, sin trazas bala, color GPS | **HECHO ve84 (familia 11)**: su `features.ini` se sirve y el motor lo lee al arrancar; cada clave se consulta en UN punto por mecanica (`ViceExtFeatures::On`/`Waypoint`): arbitro del nado, cola del retroceso, `CWorld::Process`, `CBulletTraces::AddTrace`, HUD (dinero y estrellas), radar (color del waypoint), escalada, lanzacohetes, intermitentes, tanque, coche volcado, no-rebote y regen. **7 claves quedan como decision del jugador** (tabla `kOverrides`, trazadas `origen=jugador`): escalar, ceros del dinero, coche volcado, regen, estrellas, tanque e intermitentes. **3 no tienen efecto** en el motor y la traza lo dice (`impl=0`): `CameraShakeInVehicleAtHighSpeed`, `RandomVehicleModsInTraffic` (taller excluido) y `MirrorModeByDefault` |
| features.ini: ceros dinero, no-rebote, intermitentes | PORTADO (los tres) |
| Recarga con tecla | PORTADO |
| Ruedas cambiadas | POR-VERIFICAR |
| Salida moto por lado de giro | PORTADO |
| Motor/luces off al salir | POR-VERIFICAR |
| Iconos farmacia/estadio/bomba/negocios | EN-CURSO 9.3 |
| Mundo del mod (object.dat, occlu.ipl, maps/bryx) | **HECHO ve85/ve85c (familia 7)**: los cuatro ficheros del mod se sirven bit a bit (con su tamano y hash exigidos por `check-served-build.sh`) y `tools/viceext-world-check.py` comprueba que cada referencia de esas piezas resuelve a un modelo/TXD servido (`MUNDO OK`; `occlu.ipl` = 342 zonas). Su `gta_vc.dat` no se sustituye (el suyo trae el bloque `CDIMAGE` que este port no tiene: sus IMG van fundidos en `gta3.img`). **Arranque real medido (ve85c)**: el motor completa `initstep 0..18` y publica `P4 kind=worlddata ... zonas=631` (P4.16 **PASS** sobre el log del motor); para llegar ahi hubo que arreglar el `-1` del `inst` de su `bryx.ipl`, que con el id como indice sin comprobar rango tumbaba el arranque con `RuntimeError: null function` (ADR-015; 2 de las 9135 lineas `inst` servidas usan id negativo, el vanilla ninguna). Cinco entradas del `object.dat` sin modelo servido ya estaban asi en el port (AVISO del chequeo, no de esta familia). **Falta el ojo en partida**: sin poder cargar el save del mod (familia 13) hay que empezar partida nueva |
| main.scm | **SERVIDO** (2026-10-14) → plan 12.0 familia 13: `streamed/data/main.scm` y el precargado `bootseed/data/main.scm` son el del mod **bit a bit** (1.191.180 B, `41a932cc593e`); antes se servía el vanilla (1.269.133 B, `0fb3fd45b6a6`). Con él van sus 10 opcodes propios (ver v3.0) y ya no queda ningún dato del mod sin servir: `check-served-build.sh` lo exige en su lista de datos (tamaño y hash del mod) y `viceext-parity-check.py` lo da por **idéntico**. **Lo que falta es el veredicto en partida**, y tiene un bloqueo medido: el primer arranque headless con el motor real se cae **al cargar la partida** del jugador, en `CRunningScript::Load` (`memory access out of bounds`), y el control con el script **vanilla** servido revienta **igual y en el mismo sitio** — el save está escrito por su `ViceEx.exe` y su bloque de script no encaja con este cargador. Para juzgar la familia 13 hace falta un save hecho por el **port** (o empezar partida nueva) |
| Land climbs | POR-VERIFICAR (¿qué es?) |
| Garajes del mod | **RECUPERADO** (2026-10-14) → los 4 garajes que añade el script van con la familia 13 (plan 12.0); el taller de tuning sigue **EXCLUIDO** |
| Saltar llamada | PORTADO |
| limits.ini | PORTADO 9.0. **No se sirve a propósito**: su lector corre en el arranque estático del EXE y aquí los pools son los del port; la paridad vive en `src/core/config.h` (`VICEEXT_*`). Ojo: el `features.ini` **sí** se sirve y se lee desde la familia 11 (`ve84`) |
| Botones PS/Xbox en avisos | POR-VERIFICAR (frontends servidos) |
| Puertas que golpean | POR-VERIFICAR (¿9.2?) |
| Textura luz separada | POR-VERIFICAR (¿9.2?) |
| Ruso | EXCLUIDO |
| BIGHEADS / RCROCKET | PORTADO trace-only 9.1.4 (sin mecanismo) |
| img dividido | N/A |

## v2.0

| Ítem | Estado |
|---|---|
| Taller tuning / coches tuneados / ruedas del taller | **EXCLUIDO** (decisión del jugador, 2026-10-14; es la familia 12 del plan 12.0): con él quedan fuera `models/vehmods/*` (piezas de tuning) y `wheels.DFF`/`wheels.TXD` |
| Vehículos nuevos | **INTEGRADOS** (familia 14 del plan 12.0): los IDs 6500–6507 están en el `default.ide`/`handling.cfg`/`carcols.dat` servidos y sus modelos en `streamed/models/gta3.img/` (detalle en la fila de v2.5 «Vehículos nuevos / conversor MVL»). Lo que queda de esa familia es el `MAXWHEELMODELS` de los límites |
| Esconderse polis | PORTADO |
| Ruedas delanteras guardan giro | POR-VERIFICAR |
| Rueda se cae | POR-VERIFICAR |
| Caminar con ALT | EN-CURSO 9.2 |
| Animaciones (genérico) | POR-VERIFICAR (varios lanes; acotar al cierre) |
| Deadzone stick | POR-VERIFICAR |
| Salud heli policial | POR-VERIFICAR |
| Bala no atraviesa rueda | PORTADO |
| AIRWAYS a todos | PORTADO 9.1.4 (verificación pendiente) |

## v1.5

| Ítem | Estado |
|---|---|
| Primera persona / drive-by / moverse apuntando / sprint pesadas | PORTADO (los cuatro) |
| CLEO | **FUERA de la lista** (decisión del jugador, 2026-10-14): no se implementa lo que no se usa. El extended no trae ni un `.cs`, y los dos que hay en el repo son de otros mods y son memory-hacks del `gta-vc.exe` x86 (offsets de `CPed` + llamadas al exe), no lógica portable a wasm |
| Velocidad movimiento/peds | POR-VERIFICAR |
| Luna móvil | EN-CURSO 9.2 |
| Recoil / anims trepa-agachado-nado / anims apuntado | PORTADO (los tres) |
| Idle/run anims | POR-VERIFICAR |
| AEZAKMI | PORTADO 9.1.4 (verificación pendiente: cero trazas en logs) |

## v1.0

| Ítem | Estado |
|---|---|
| Trepar / nadar / apuntado / hide-wanted / radio aparición | PORTADO (los cinco) |
| Seated movement | POR-VERIFICAR (¿agachado? ¿sit 9.2?) |
| Controles modernos (sens/mando/vibración/autoaim) | POR-VERIFICAR (lanes Classic AXIS; probable PORTADO) |
| NPC accuracy | EN-CURSO 9.2 |
| SWAT gas / 2 helis / daño Maverick / cañón agua | **Pendiente → familia 4 (polis y wanted)** del plan 12.0, que la da como TODO y enumera «esconderse, polis se agachan a cubierto, sistema wanted, SWAT con gas, dos helicópteros, daño al Maverick por arma, salud del heli, luces del FBI». El baseline del 2026-10-13 la daba como «EXCLUIDO (los cuatro)»: la decisión del jugador del 2026-10-14 devolvió a la lista lo que estaba aparcado, así que esto es trabajo pendiente, no exclusión. El **cañón de agua** no aparece en esa fila del plan (el motor ya trae `src/renderer/WaterCannon.cpp`): si el mod lo cambia, se mide al cerrar |
| Conductores reaccionan / muro / partículas | EN-CURSO 9.4 (los tres) |
| FBI lights | POR-VERIFICAR (¿cubre service lights?) |
| IAMINVINCIBLE / AIRWAYS / BIGSMOKE | PORTADO 9.1.4 (verificación pendiente) |

## Deuda de datos medida (2026-10-14)

Auditoría del overlay del mod (`mods/extended/GameFiles/ViceExtended/**` y
`mods/extended/GameFiles/modloader/ViceExtended/**`, con el mapeo
`cdimages/x.img → models/x.img`, `gta_vc.dat`/`*.zon`/`object.dat`/`occlu.ipl` →
`data/` y el resto aplanado a la raíz): la primera pasada dio **97 idénticos ·
26 distintos · 32 ausentes**, con **3 huecos reales**, los tres bloqueados por
código. El mismo 2026-10-14 entró el código de los tres y hoy el recuento es
**100 idénticos · 24 distintos · 39 ausentes = 63 no idénticos**, todos
clasificados (`by-design=28, cubierto=24, mundo=5, pendiente=6`):

| Hueco | Medida | Estado hoy |
|---|---|---|
| `data/main.scm` | mod 1.191.180 B (`41a932cc593e`) vs el que se servía, 1.269.133 B (`0fb3fd45b6a6`, vanilla) | **Servido ya**: el del mod bit a bit (familia 13, con sus 10 opcodes `0x0FA0`–`0x0FA9` en el motor); lo que falta es el veredicto en partida, y para eso hace falta un save del **port** |
| `data/particle.cfg` | mod 18.363 B (`49130d2521a1`) vs el que se servía, 17.559 B (`421e1df9cf7c`) | **Servido ya**: el fichero del mod bit a bit, tras resolver el tipo por nombre (familia 3/5, `VICEEXT_PARTICLE_NAMES`); los 83 tipos comunes, valores idénticos |
| `models/coll/newWeapons.col` (+ su `COLFILE`) | 224 B (`a5b85b2b6cd5`), antes ausente | **Servido ya**, con su línea `COLFILE 0 MODELS\COLL\NEWWEAPONS.COL` en `streamed/data/default.dat` |

> Servido con `dataTag`/`VERSION` nuevos (`2026-10-14-ve78`, la convención `-veN`
> del proyecto): el `main.scm` y el `particle.cfg` van también en el paquete
> inicial (`stage_bootseed.py`) y el `.col` nuevo va **precargado** en
> `bootseed.list`, junto a los demás (`vehicles.col`, `newvehicles.col`,
> `weapons.col`). Sin esa precarga el motor se caía al arrancar (`RuntimeError:
> null function` en `CFileMgr::Read`), porque los `.col` no se leen on-demand.

> **Vía para cerrar la familia 13** (medida 2026-10-14): el análisis de su
> `ViceEx.exe` se hace por la **CLI** de REA (`tools/viceex-rea.py`), que no tiene el
> límite del cliente MCP, y **una sola** decompilación (`dec 0x4948B6`) devuelve el
> switch completo de los 10 opcodes. Por qué el MCP directo no sirve para eso y qué
> sí hace bien: plan 11.0, «Sesión 2026-10-14 (tarde)».

Cubierto por otra vía (**no es deuda**): `weapon.dat` (el servido es el del
mod), `newVehicles/*` (importados a `default.ide`/`handling.cfg`/`carcols.dat` y
a `models/gta3.img/`), `newvehicles.col` (idéntico y declarado),
`ViceEx.SDT`/`ViceEx.RAW` (las 13 muestras están en `streamed/Audio/sfx.SDT`,
ids 9941–9953, con sus 13 mp3), `neo/*` y `freeroam_miami.scm` (idénticos), y
`cdimages/*` (fusionados en `streamed/models/gta3.img/`).

No se sirven **a propósito** (tampoco es deuda): `features.ini` y `limits.ini`
(DESIGN §5: se quedan en la instalación y la paridad vive en `config.h`) y
`ViceEx.dat` (manifiesto de carga de *su* motor; la vía del port es importar +
`COLFILE`).

Decisión pendiente ya conocida (`mods/07` §14), fuera de esta cuenta: los 6
`TEXT/*.gxt` (delta constante ~214–258 B), los sets de botones de mando
(`ps4btns`/`ps5btns`/`x1btns`/`frontend_ds5`/`fonts_u`), `anim/ped.ifp` (el
servido lleva clips de otros mods) y `fronten2`/`wheels`/`vehmods/*` (taller
excluido).

Oráculos de una sesión en la instalación (lo único que se escribe ahí desde el
2026-10-01): `ViceEx.ini`, `modloader/modloader.log` y
`ViceExtended/userfiles/Gallery/photo_00000.png`. Auditoría repetible:
`gta_vc_browser/tools/viceext-parity-check.py` (exit 0 con **63 diferencias**,
todas clasificadas; los tres datos del mod —`main.scm`, `particle.cfg` y
`newWeapons.col`— salen ya **idénticos**, así que si vuelven a aparecer es una
regresión).

## Método de cierre (al integrar 9.2-9.4)

1. Cada POR-VERIFICAR se resuelve con grep (flag/mecanismo) o partida.
2. Marcadores servidos + suite verde + `P4 kind` por cosa nueva.
3. Tu partida única por build + log por el verificador (el save tiene que ser del
   **port**: el de tu instalación del mod no carga, ver la fila de `main.scm` en
   v2.5).
4. Veredicto final por ítem aquí; lo no portable queda N/A con motivo y lo
   excluido solo vuelve con tu orden.
