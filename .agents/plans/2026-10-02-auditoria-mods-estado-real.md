---
name: 2026-10-02-auditoria-mods-estado-real
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-10-02
---

# Plan: auditoría de los mods pendientes y su estado real

Fecha: 2026-10-02 · sección: `mods` · estado: **EXECUTED**

## Por qué este plan

El jugador da por hecho que **solo el nado funciona** y pide revisar qué se
implementó de verdad de SilentPatch, WFP y FramerateVigilante, porque los planes
afirman más de lo que el código contiene. Esta auditoría se hizo el 02/10 sobre
`docs/mods/ATTRIBUTION.md` y los tres planes de mod, **leyendo el código**, no el
documento.

## La regla que manda (y que corrige a esta auditoría)

`.agents/plans/mods/12-handoff-tanda2.md` §4.2, **anulada el 28/09/2026 por el
jugador**, y vigente:

> No preguntes «¿el motor ya lo hace?» ni anotes `YA ESTABA` para saltarte un port.
> Ese estado sigue existiendo en `ATTRIBUTION.md` como **registro**, pero ya no es
> **excusa para no trabajar**. El jugador: *«mucho de lo hecho actual no sirve para
> nada o tiene un enfoque totalmente erróneo al del mod»*, y pidió portar al máximo
> *«para ahorrarnos reinventar la rueda»*: **copiar lo que funciona en vez de
> rehacerlo a ojo**. **Parecerse no es ser lo mismo**: la equivalencia hay que
> **demostrarla leyendo los dos lados**, y si no se demuestra, **se porta el del mod**.

**Consecuencia sobre el §2 de este plan:** los siete "PENDIENTE que ya están
resueltos" **no quedan absueltos**. Que el síntoma no se vea demuestra que el arreglo
no se nota, **no** que el mecanismo sea el del mod. Pasan de §2 (constatación) a la
tarea 5 (hay que **leer los dos lados** y portar si no se demuestra la equivalencia).
Lo que sí queda zanjado es el caso opuesto: un arreglo **ausente** del todo, que no
puede depender de nada.

Lo que **sí** sigue vetado, y es otro caso (RULES 0.6): escribir dos veces lo mismo
**dentro de nuestro propio código**.


**Y lo de las licencias, que ya esta decidido y no vuelve a salir como filtro:** la norma vigente del 27/09 (`00-INDICE.md` §2) y la decision `002` dicen que la licencia **no es una puerta**. De `mods/` se toma todo lo que haga falta, con o sin LICENSE; la cabecera de atribucion y `docs/mods/ATTRIBUTION.md` quedan por cortesia y trazabilidad. Lo unico excluido es lo **tecnicamente** inaplicable en WASM (direcciones x86, `hook::pattern`, `.asi`, D3D8/DLL injection), y eso no es por la licencia: es que esas direcciones no existen en un motor recompilado desde fuente. **La licencia no entra en el orden de dificultad de ningun port.**
## Findings

### 1 · Los grupos apagables no existen

`VICEEXT_FIX_SILENTPATCH`, `VICEEXT_FIX_WFP`, `VICEEXT_FIX_FV` y
`VICEEXT_FIX_FV_ROTOR` aparecen **solo** en `.agents/plans/mods/00-INDICE.md:92-94`,
que los exige como requisito. En `src/` y `docs/` hay **cero** coincidencias. Los 25
defines `VICEEXT_*` que hay en `src/core/config.h:382-452` son de características, no
de ports. Consecuencia: **hoy no se puede apagar ningún port de mod desde
`config.h`**, ni en bloque ni suelto.

### 2 · El manifiesto sobrevende y subvende a partes iguales

Conteo sobre `docs/mods/ATTRIBUTION.md`: **37 filas PENDIENTE, 18 PORTADO, 10 YA
ESTABA, 0 NO APLICA**. Contra el código real:

- **PORTADO que sí está vivo** (7): `AudioLogic.cpp:2896-2903` (sirena FBI),
  `Weapon.cpp:1933-1940` (casquillos), `Frontend.cpp:2476-2496` (fade del outro),
  `Frontend.cpp:5915-5934` (duración outro), `main.cpp:661-682` (contorno barra de
  carga, **el único port de T2**), `Heli.cpp:578` (rotor), `CarCtrl.cpp:2757`
  (spool IA) + `Automobile.cpp:1423-1428` (spool jugador).
- **PORTADO incompleto**: `1397` (coronas de sirena) está en
  `Automobile.cpp:1866-1995,2375,2393,2414,2510-2532` pero **sin cabecera `PORTADO`**
  (el propio formato del proyecto la exige) y solo cubre `POLICE`/`FIRETRUCK`/
  `AMBULAN`; `MI_ENFORCER` (`:2423`) y `FBICAR`/`FBIRANCH`/`VICECHEE` (`:2514`) siguen
  en posición fija.
- **PENDIENTE que ya está resuelto** (7): `2469` `PedAttractor.cpp:116-120`,
  `2168` `Weapon.cpp:695-711`, `2400` `PedFight.cpp:319`, `1646` `Game.cpp:848` +
  `Stats.cpp:118-206`, `1545` `Plane.cpp:829-836`, y en T2 ocho filas que el motor ya
  escala (`947`, `1071`, `1189`, `541`, `613`, `662`, `773`, `2063`).
- **Declaración falsa en FV**: la fila `AutoPilotTimerFix_VC` dice «0 contadores en
  frames», pero `CarAI.cpp:192` y `:252` siguen con `(CTimer::GetFrameCounter() & 7)
  == 0`. Hay más sin documentar: `CarCtrl.cpp:1025`, `Heli.cpp:313`,
  `Automobile.cpp:4768`, `CarCtrl.cpp:199/221/250/279`.

### 3 · Rutas del manifiesto que no existen (rompen cualquier auditoría por el doc)

| El doc dice | El real |
|---|---|
| `src/control/Radar.cpp` (6 filas) | `src/core/Radar.cpp` |
| `src/core/ModelInfo.cpp` (2 filas) | `src/modelinfo/ModelInfo.cpp` |
| `src/renderer/VisibilityPlugins.cpp` | `src/rw/VisibilityPlugins.cpp` |
| créditos en `src/core/Frontend.cpp` | `src/renderer/Credits.cpp:50-54` |

### 4 · WFP: nada portado, y no es no-op en web

**Cero cabeceras `PORTADO` de WFP en todo `src/`**, y `FOVManager`, `HudOffset`,
`ScaleRect`, `pillarbox`, `letterbox`, `TransparentMenu` dan 0 coincidencias. El doc
es honesto aquí. Pero la **base de aspecto sí está activa** (`config.h:332-333`
`ASPECT_RATIO_SCALE` + `PROPER_SCALING`, sin `//`) y `Draw.cpp:38-39` devuelve el
aspecto real del canvas, así que WFP **sí es aplicable** en web. Dos avisos:
`Draw.cpp:100-103` desactiva la conversión de FOV **durante cinemáticas**, que es
justo donde entraría el WFP `CutsceneMgr`; y `CutsceneMgr.cpp` **no tiene ni una
línea de dibujo**, o sea que el letterbox de cinemáticas no tiene punto de anclaje.

### 5 · Bugs reales y baratos que se han escapado

> Y un bloque que no es un bug sino **una verificación pendiente**: la escalada, en el §8.bis.

Ordenados por relación esfuerzo/beneficio:

1. **`Radar.cpp:1666-1671`** — el tercer vértice del blip de destino repite el cuarto
   (`SCREEN_SCALE_X(8.f)` dos veces) en vez de `x + SCREEN_SCALE_X(14.0f)`:
   triángulo degenerado. Es el bug literal del mod (`:1105`), una línea.
2. **`Hud.cpp:333` + `:867`** — el icono del arma pasa el alfa por el color del
   vértice pero con `rwRENDERSTATEVERTEXALPHAENABLE=FALSE` puesto en `:333` y nunca
   reactivado ⇒ **el fundido del icono de arma no ocurre**. Es la parte (c) del
   `minimal HUD` (`:896`).
3. **`VICEEXT_TURN_SIGNALS`** — `config.h:441` está comentado **a propósito**, y el
   motivo está escrito en el propio comentario: el `features.ini` del mod trae
   `StandardCarsUseTurnSignals=0`. **No es un descuido** (esta auditoría lo había
   marcado mal). Lo real: 80 líneas escritas y portadas pero sin efecto, y el flag
   1 de los 17 es una decisión ya tomada, no un agujero.
4. **`1478` (env mapping en extras)** — la maquinaria está
   (`custompipes.cpp:126,344`, `Renderer.cpp:302`) pero **apagada** por defecto: sólo
   corre con `VehiclePipeSwitch==VEHICLEPIPE_NEO`, y el default es `MATFX`.
5. **`1701` (backface)** y **`1931` (flares de corona**) — sin código;
   `Coronas.cpp:414-424` escala sin `SCREEN_STRETCH_*`.
6. **`2196` (zonas seguras de MBlur)** — `MBlur.cpp:566-581` mezcla `±10.0f`
   literales con `PosInside(rect,400,0,W,90)` sin escala y aún tiene un
   `TODO: fix aspect ratio scaling` en `:569`.

### 6 · El resto de `mods/` que no estaba en el inventario de `ATTRIBUTION.md`

`docs/mods/ATTRIBUTION.md` **solo cubre 3 de las 13 entradas** de `mods/` (las tres
del §2 del índice). Las otras diez no tienen fila, así que nadie puede saber si están
portadas:

| Entrada de `mods/` | Qué es | Estado real |
|---|---|---|
| `climbing/` | Climbing, **con `source_code/`** | Hecho (E1). Es **el modelo a seguir**: fuente real → port directo |
| `1487678468_firstperson/` | 1ª persona de GeniusZ (`.asi`, cp1251) | Cero cobertura en el manifiesto; hay trabajo hecho (`VICEEXT_FIRST_PERSON`) |
| `SACarCam.asi` | Shim de cámara, sin matemática propia | **Nada que portar** (`13` §3.3) |
| `1498977446_107784/` | Nado de Serega (CLEO + `ped.ifp` 236) | Es la 1.11 que acabamos de cerrar |
| `sa-crouch-movement_.../` | SA Crouch (CLEO + anim + `.dat` 616 líneas) | Hecho (R27) |
| `Classic AXIS/` + `1510564741_ca-1/` | ClassicAXIS, apuntado y cámara | En curso (`apuntado-classicaxis-100.md`); `R9`/`AX6` son suyos |
| `skygfx/` | Solo binarios `d3d8.dll`/`rwd3d9.dll` | Sin portar; D14 (§5 tarea 4) |
| `ginput/` | GInput, cerrado (solo spec) | Reimplementar sobre Gamepad API |
| `widescreen/` | WFP (`.ixx` + ini) | Cero ports (§4) |
| `framerate/` | FV, **solo `.asi` + `.ini`** local | 3 ports vivos + 6 sitios frágiles sin portar |
| `extended/` | Vice Extended, **sin fuente** (solo `ViceEx.exe`) | 4 mecánicas hechas desde cero → el origen del «enfoque erróneo» |

**El patrón que se repite y que conviene notar:** de las 13 entradas, **las dos con
fuente real en el repo** (`climbing/` con su `source_code/`, y `silent patch/`) son
las que están bien portadas. Las que solo traen binario (`extended/`, `skygfx/`,
`ginput/`, `SACarCam`) no se pueden portar y hubo que reimplementarlas a ciegas — y
justo esas son las que el jugador dice que quedaron «con un enfoque totalmente
erróneo». La regla de §4.2 se aplica en el otro sentido también: **cuando hay
fuente, se copia; cuando no hay, hay que decirlo en vez de fingir equivalencia.**

### 7 · El Vice Extended es el mod que manda, y estaba sin inventariar

`mods/extended/` es un **mod de packs**: el `ViceEx.exe` (3,7 MB) **no tiene
fuente** (`SourceCode/` solo trae el `MVLConverter`), y lo que define al mod son
tres ficheros de texto mas sus datos:

- `features.ini` — **17 flags**, cada uno con `= 0` o `= 1`.
- `ChangesEN.txt` — el changelog **v1.0 → 2510**, unos **150 cambios**.
- `limits.ini` — 11 limites de pool.

De los 17 flags, contra el codigo real: **6 portados y vivos, 1 escrito pero
apagado, 10 ausentes**. De los 6 que el mod trae **encendidos** (`=1`), 4 estan y
2 no: `EnableDistantLights` y `RandomVehicleModsInTraffic` (el segundo esta
justamente fuera por decision tuya, porque depende del taller de tuning).

**El "apagado"**: `StandardCarsUseTurnSignals` -> `VICEEXT_TURN_SIGNALS` comentado
en `config.h:441`, con las 80 lineas de `Automobile.cpp:1749-1812` ya escritas y
compilando. **Pero está apagado a propósito** (el mod lo trae en `0`, y el motivo
está en el comentario del define): no es un agujero, es una decisión ya tomada. Lo
único a decidir sería si se enciende o se borran las 80 líneas.

**Los 6 que el mod trae apagados y se portaron igual por peticion tuya**:
`RemoveMoneyZerosInTheHud`, `EnableClimbing` (y con ellos `RecoilWhenFiring`,
`PlayerDoesntBounceAwayFromMovingCar`, `EnableSwimming`,
`RocketLauncherThirdPersonAiming`).

#### Los 9 huecos que no estan en NINGUN plan

Cruce del changelog contra los tres planes de Extended, la lista X1-X26 de
`mecanicas/06-plan-correcciones-5a-partida.md` y `vice-extended-pendientes.md`.
No salen ni como PENDIENTE ni como FUERA:

| Funcion del changelog | Version | Donde la harias | Esfuerzo |
|---|---|---|---|
| La bala ya no atraviesa la rueda del coche a traves del chasis | 2.0 | `Weapon.cpp` (proyectil vs modelo) | bajo |
| Salir de la moto por la derecha al girar a la derecha | 2.5 | `Bike.cpp` | bajo |
| Apagar motor y luces manteniendo el boton de salir | 2.5 | `Automobile.cpp`/`Bike.cpp` | bajo |
| Saltarse la llamada telefonica con una tecla | 2.5 | `AudioManager.cpp:1122` | bajo |
| **Volante que gira** (el coche tiene dummy, no se usa) | 2.5 | `Automobile.cpp:3075-3115` | medio |
| Selector de 3 modos de mira (Default/Simple/Complex) en el menu | 3.0 | `MenuScreensCustom.cpp:40-100` | medio |
| Botones de pista PS4/PS5/Xbox One (faltan los 3 `.txd` en `streamed/`) | 2.5 | datos | bajo, pero **re-empaqueta** |
| Idioma ucraniano (el mod trae `ukrainian.gxt`) | 3.0 | `Text.cpp:37-49` + menu | bajo |
| Arreglado el crash al reiniciar partida nueva | 2506 | sin dueno | por medir |
| Arreglo del script del detector de metales | 2506 | depende del `main.scm` (E21) | colgado |

Las tres primeras son mecanicas de una decena de lineas y **no estan pedidos en
ningun sitio**.

#### Lo que parece vivo y solo esta a medias

Esto es lo que mas engaña, porque el codigo esta y la traza sale:

1. **La moto policial no circula.** `polwintergreen` esta en `default.ide:298` con
   `vehClass = ignore`, y `CarCtrl::ChoosePoliceCarModel` (`CarCtrl.cpp:943-988`)
   nunca devuelve 6507. El changelog dice *"Can be seen in the traffic"*: hoy solo
   se ve con el cheat `CRAZYRIDES`. **Mismo tipo de fallo que el `streetfighter`
   missing por clave GXT mal formada**: es un dato del IDE, no una mecanica.
2. **13 muestras de audio del banco `ViceEx.{SDT,RAW}`**: solo 4 de 8 armas
   mapeadas por oido, y 2/7/8/12 sin enganchar (`AudioLogic.cpp:4654-4665`).
3. **La 1a persona** es un conmutador con tecla (V), no una opcion de menu como
   en el mod.
4. **Las miras por arma** existen, pero **sin el selector de 3 modos**.
5. **`WaypointColorRGB`**: el color esta *quemado* en el raster que se genera por
   codigo (`Radar.cpp:1103-1105`, `255,72,77`) y el del mod es `180,24,24`. No es
   que falte el toggle: es que el valor elegido no es el suyo.

#### `limits.ini`: casi nada que hacer

De los 11 limites, **10 ya coinciden con los de reVC de serie**. El unico cambio
real que hicimos es `NUMVEHICLES 110 → 130` (`config.h:55`), que necesitan los 8
coches 6500-6507. `MAXWHEELMODELS` **no existe en nuestro motor** y es del taller de
tuning (excluido). Ademas subimos por cuenta propia `MAX_CDIMAGES 8→12`,
`MODELINFOSIZE 6500→6700`, `TXDSTORESIZE→1500`, `COLSTORESIZE 31→48`,
`WEAPONMODELSIZE 37→47`, `NUMANIMBLOCKS 29→40`, `NUMANIMATIONS 410→512`.

#### El manifiesto no tiene seccion de Vice Extended

`ATTRIBUTION.md` cubre 3 de las 13 entradas de `mods/` (SilentPatch, FV, WFP) mas
ClassicAXIS y `sa-crouch`. Del Extended solo hay **menciones de pasada** dentro de
filas de otros mods (sirena FBI, banco de audio, luces de servicio, recoil). De los
**22 `VICEEXT_*`** de `config.h:382-452`, **ninguno tiene fila propia con
`fichero:línea`**. Y falta la advertencia de que `ViceEx.exe` no tiene fuente: hoy
un lector del manifiesto puede creer que hay codigo del mod en el arbol, cuando
todo lo del Extended es **reimplementado por nosotros**.

### 8.bis · La escalada: implementada, nunca validada (medido el 03/10)

`escalada-climbing-completo.md` estaba **PENDING**, y al mirarlo con dato sale que
es el segundo bloque con código escrito que **nunca se ha comprobado**:

| Qué | Medido |
|---|---|
| El código existe | **21 usos** de `VICEEXT_CLIMB`, casi todos en `src/peds/PlayerPed.cpp` (`:2318,2603,2765,3059-3130`) + `PlayerPed.h:122` |
| Los clips existen | **7 en nuestro enum**: `ANIM_STD_CLIMB_{IDLE,JUMP,JUMP_B,JUMP2FALL,PULL,STAND,STAND_FINISH}` |
| Los tunes existen | `VICEEXT_CLIMB_MIN/MAX` (0,55-1,85 m), `MIN_MS/MAX_MS` (700-1100), `REACH` (0,95) |
| Las trazas existen | 3: `VICEEXT climb fin`, `VICEEXT climb no: destino en agua`, `VICEEXT swim trepa-off` |
| **La validación** | **NO HECHA**: sus 5 criterios de PASS están sin comprobar y hay **cero trazas `climb` en los últimos 6 logs** |
| La fuente del mod | `mods/climbing/source_code/src/peds/Ped.cpp` (328 KB) — **el único mod de `mods/` con el reVC completo**, así que aquí sí hay código que copiar |

**Clasificación por dificultad: NIVEL 1 · fácil, y por un motivo concreto**: no hay
que escribir nada, hay que **comprobar**. El bloque está entero, con sus tunes y sus
trazas. Si los 5 criterios pasan, la escalada está hecha y no cuesta más que una
partida. Si alguno falla, es un bug localizado dentro de un bloque que ya existe (y
no un sistema por construir, al contrario que el WFP o el `COLT45`).

Es además el **segundo candidato a «funcional»** junto al nado, y encaja con lo que
dijo el jugador: *«el único mod funcional hasta ahora es el de nadar»*. Puede que la
escalada sea el segundo y no lo supiéramos, porque nunca se midió.

-> verify: los 5 criterios de su plan (valla fina, valla gruesa, subida con tirón, un
NPC trepando, cancelar a media subida) y `VICEEXT climb fin` apareciendo en el log.

### 8 · Qué no se puede volver atrás con estos planes

`mods/silent patch/` sólo trae `SilentPatchVC.asi` + `.ini` + 8 IPL. **Las ~20
líneas de origen que cita el manifiesto (`1864`, `1397`, `1967`, `4086`…) no son
verificables en este repo.** El único dato de mod legible es el `.ini`, que sí
sirve para `1701` y `1478` (sus listas de modelos están en
`SilentPatchVC.ini:45` y `:36`). Cualquier arreglo que necesite la línea exacta del
mod hay que sacarlo de la fuente del `.asi` o de internet.

## Scope

Fuera de alcance aquí: implementar nada. Este plan **audita y ordena**. La
implementación va en planes por tanda, con un plan por arreglo o por grupo corto.

## Success criteria

- El estado de cada fila del manifiesto refleja el código, con `fichero:línea`.
- Las 6 rutas erróneas del manifiesto corregidas.
- El juego de flags `VICEEXT_FIX_*` existe de verdad, o se borra del índice la
  promesa de que existe.
- Los 6 bugs del §5 tienen dueño y prioridad.

## Tasks

Las cuatro primeras no dependen de la sesión del jugador y son de ejecución
directa. La 5 es la grande y es la que aplica la regla §4.2.

1. **Corregir `docs/mods/ATTRIBUTION.md`**: las 6 rutas, y el estado real de las 7
   filas PENDIENTE que ya están resueltas + `1397` marcado incompleto + la
   declaración falsa de `AutoPilotTimerFix_VC`. -> verify: cada fila con
   `fichero:línea` real, releyendo el fichero.
2. **Meter los grupos apagables** `VICEEXT_FIX_SILENTPATCH` / `_WFP` / `_FV` en
   `src/core/config.h` y envolver los ports que hoy son incondicionales
   (Heli, CarCtrl, Automobile, AudioLogic, Weapon, Frontend, main.cpp) -> verify:
   compila con los tres apagados y con los tres encendidos, y el cambio se nota en
   el `reVC.wasm`.
3. **Los 4 bugs baratos del §5** (`Radar.cpp:1666`, `Hud.cpp:333`, `TURN_SIGNALS`,
   y decidir `1478`) -> verify: objeto exit 0 y marca en el verificador para el del
   blip y el del icono.
4. **Portar lo que no está y es fuente real** (esto es el corazón de la regla):
   - **FV = FramerateVigilante** (`GTAmodding/FramerateVigilante` +
     `JuniorDjjr/CLEOPlus`, fuente pública). Los **6 sitios frágiles vivos** que la
     auditoría encontró sin documentar, todos con el mismo patrón
     `GetFrameCounter() & N`, que a FPS altos NO es una frecuencia sino un aliasing:
     `CarAI.cpp:192,252` · `CarCtrl.cpp:1025` · `Heli.cpp:313` ·
     `Automobile.cpp:4768` · `CarCtrl.cpp:199,221,250,279`.
     El rotor ya está portado (`Heli.cpp:578`); esto es **el resto de la misma
     familia**, con su cabecera `PORTADO` y su fila en el manifiesto.
   - **SkyGfx (D14, `nightParam`)**: uniform + `mix()` del prelight en los shaders
     GL3. La referencia HLSL está ya extraída en
     `gta_vc_browser/tmp/extsrc/skygfx_shaders.txt`.
   -> verify: objeto exit 0, marca en el verificador, y la traza que demuestra que
   ya no depende del contador de frames (una sesión a 35 y otra a 120 fps).
5. **Los 9 huecos del §9 que no estan en ningun plan**, empezando por los tres
   baratos (bala que atraviesa la rueda, salir de la moto por el lado correcto,
   apagar motor con el boton de salir) y por **arreglar `polwintergreen`** para que
   circule (`default.ide:298`): son 80 lineas escritas y ya muertas, y un dato mal
   puesto. -> verify: objeto exit 0, y el traza del dato (`RIDE`) sale con la moto
   en el trafico.
6. **Seccion de Vice Extended en `docs/mods/ATTRIBUTION.md`**: una fila por cada uno
   de los 17 flags y por las ~150 funciones del changelog, con `fichero:línea` y la
   advertencia de que es reimplementacion (el `ViceEx.exe` no tiene fuente) -> verify:
   cada `VICEEXT_*` de `config.h` tiene su fila.
7. **Leer los dos lados de cada fila PENDIENTE y portar si no se demuestra**
   (§4.2). Por este orden, que es el de `00-INDICE` §8 (tanda 3):
   1. `Sprite2d` — geometría 4:3 de cinemáticas, letterbox, «textured vs UI».
      Nota: **no hay punto de anclaje todavía**, `CutsceneMgr.cpp` no tiene ni una
      línea de dibujo ⇒ primero hay que decidir dónde se dibuja la barra.
   2. `Hud` anclaje (`HudOffset`) y `Radardisc` dibujado por código.
   3. `1701` backface (lista de ~320 modelos ya en `SilentPatchVC.ini:45`) y
      `1478` env mapping en extras (excepciones en `SilentPatchVC.ini:36`) — la
      maquinaria existe pero está apagada: `custompipes.cpp:126` sólo corre con
      `VehiclePipeSwitch==VEHICLEPIPE_NEO` y el default es `MATFX`.
   4. `2196` zonas seguras de `MBlur` (tiene un `TODO: fix aspect ratio scaling`
      vivo en `MBlur.cpp:569`).
   5. SilentPatch T1 que sigue pendiente: `1967` sombras, `1576` sensibilidad,
      `1610` `IS_PLAYER_TARGETTING_CHAR`, `1952` atraco, `2082` ascensor,
      `2138` policía, `2309` púas, `1258` timers a 30 fps.
   8. Los siete "ya resueltos" del §2: **no se dan por buenos** hasta leer los dos
      lados.
   -> verify: cada uno con cabecera `PORTADO`, fila en el manifiesto con
   `fichero:línea`, traza `ODTRACES` si es medible, y el criterio PASS del jugador
   definido **antes** de que juegue.
8. **El `COLT45`** (`WEAPONTYPE_COLT45` = arma 17): `R9` (17 armas sin animación de
   apuntar, peso 0) y `AX6` (cuerpo sin seguir a la cámara, 180°). Es la parte de
   armas/apuntado, no del nado, y ya está diagnosticado por el log del 02/10.
9. **`E21`** (main.scm del mod): aplazada a decisión del jugador desde el 24/09.
   Los 10 handlers `0xFA0-0xFA9` del `ViceEx.exe` son portables; sigue abierta.

### Verification

- Declarados: los objetos ninja de los ficheros tocados, `py_compile` del checker, y
  `viceext-log-check.py <log>` sobre la sesión real.
- Del jugador: que el blip del mapa del menú no salga degenerado, que el icono del
  arma se funda al cambiarla, y el ojo del nado y de las armas.

### Closure (persistent memory)

- What changed:
  - Este plan **no cambió código**: es una auditoría. Su entrega son tres cosas:
    (1) el diagnóstico (§1-§8), que corrigió el manifiesto y rectification;
    (2) las **6 rutas falsas** de `docs/mods/ATTRIBUTION.md` corregidas, sin lo cual
    cualquier auditoría hecha desde el documento buscaba ficheros inexistentes;
    (3) **de aquí salieron tres planes**: el 1.12 (lote fácil, EXECUTED), el 1.13
    (apuntado/ClassicAXIS, ENRICHED) y ahora la escalada al §8.bis.
  - Se aplicó la **regla §4.2** de `12-handoff-tanda2.md`: los "PENDIENTE ya
    resueltos" **no** se dieron por buenos. Pasan a tarea de comparar ambos lados.
  - Se añadió la **escalada** (§8.bis) con su estado medido: 21 usos, 7 clips,
    3 trazas, **cero** validación.
  - Se corrigió la **regla de finales de línea** de `CODING_STANDARDS.md`, que decía
    "los `.cpp`/`.h` son CRLF" y era falsa (368 CRLF / 134 LF medidos), y se
    normalizaron los 2 ficheros que estaban mezclados.
  - Se añadió al checker el **bloque `B`**: compara el build del log con el `VERSION`
    servido y tumba el veredicto si no coinciden. Existe porque el 02/10 se validó
    una build vieja y se 지급 por hecho que los cambios fallaban.
- Verification:
  - La clasificación por dificultad sale de **medir el código**, no de leer el
    manifiesto: 368+134 ficheros contados, `VICEEXT_FIX_*` buscados en todo el árbol
    (0 en `src/`), 17 sitios `GetFrameCounter()&N` (no 6), rutas del manifiesto
    comprobadas una a una.
  - Bloque `B` probado contra el log del 02/10: da `FALLO` nombrando las dos builds.
- Outcome:
  - El objetivo del plan era «auditar y ordenar, sin implementar», y se cumplió.
  - De 37 filas PENDIENTE que decía el manifiesto, **8 no lo estaban** (ya resueltas
    o ya portadas), y **3 rutas de destino no existían**.
  - **Lo que este plan NO hizo y no puede hacer**: decir si un port es equivalente
    al del mod. Eso exige leer los dos lados, y es la regla §4.2.
- Pending items:
  - Los planes que salieron de aquí: **1.13** (apuntado/ClassicAXIS) y la
    **escalada** (§8.bis, nivel 1: solo verificar).
  - **El trabajo de mods sin plan numerado**: la tanda 3 de `mods/00-INDICE.md`
    (WFP `Sprite2d`/`Radardisc`/`Loading`, SkyGfx `nightParam`, los SilentPatch de
    render) y la lista de `vice-extended-pendientes.md` (iconos de emisora, sonido
    del agua al nadar, teclas de pista, 6 trucos, crash del Micro-UZI).
  - **Falta una sección de Vice Extended en el manifiesto**: cubre 3 de las 13
    entradas de `mods/`, y de los 22 `VICEEXT_*` de `config.h` ninguno tiene fila
    con `fichero:línea`.