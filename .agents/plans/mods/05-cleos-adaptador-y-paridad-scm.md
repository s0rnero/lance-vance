---
name: 05-cleos-adaptador-y-paridad-scm
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 05 — CLEO a fondo, adaptador CLEO-lite, paridad SCM y tabla % por mod

Segunda pasada sobre `mods/` (continuación de `04-extraccion-fuentes-mods.md`).
Incluye: decodificación real del bytecode CLEO, hallazgo de paridad de `main.scm`, corrección
del análisis de `movements.img`, diffs de los IPL de SilentPatch, verificación del `weapon.dat`
servido, y la **tabla final de % de integración por mod**.

---

## 1. El dispatch de nuestro intérprete SCM (clave para todo lo CLEO)

`src/control/Script2..8.cpp` despachan con **`case COMMAND_XXX:`** (constantes con nombre de
`commands.h`), no literales numéricos. Cubren **0–1499 opcodes** (`ProcessCommands0To99` …
`ProcessCommands1400To1499`, repartidos 100-150 por función, ver `Script2.cpp:7` y `Script5.cpp:7`).

Implicación: los **opcodes vanilla de VC** que usan los scripts CLEO (p.ej. `0x03A4` name_thread,
`0x0001` wait, `0x004D` goto_if_false, `0x00D6` if) **ya están implementados** por nuestro motor.
Lo único que falta para ejecutar CLEO son los **opcodes custom de CLEO, que empiezan en 1500
(0x05DC)** — por encima del techo de nuestro dispatcher.

## 2. Formato real de los `.cs` / `.cm` (decodificado en hex)

Fuentes analizadas en binario:
- `mods/1498977446_107784/CLEO/swim.cs` (mod Swim)
- `mods/sa-crouch-movement_1774634386_626056/cleo/CrouchMovement(forClassicAxis).cs` (mod Agachado)

Estructura: **bytecode SCM estándar** (mismísimo formato que `main.scm`) **+ tabla de símbolos de
Sanny Builder al final** (marcador `E VAR`, índice 0x0500, nombres de variables locales: `WC`,
`WC3`… `WC10` en swim = "wait child"). Decodificado confirmado:

| Bytes | Instrucción | Nota |
|---|---|---|
| `a4 03 "SWIM\0\0…"` | `0x03A4` name_thread | label de 8 bytes |
| `01 00` + arg | `0x0001` wait | args con prefijo de tipo: `0x01`=int32 LE, `0x02`=var global, `0x03`=var local, `0x04`=int16 |
| `4d 00 01 f6 ff ff ff` | `0x004D` goto_if_false | offset relativo int32 (−10) |
| `d6 00 04 02 …` | `0x00D6` if (and, 2 condiciones) | |
| `e6 05`, `df 05`, `e0 05`, `fe 05`, `f9 05` | **0x05E6 / 0x05DF / 0x05E0 / 0x05FE / 0x05F9** | **opcodes custom CLEO** (≥1500) |
| `29 80 …`, `e1 80 …` | sufijos de flag negado 0x80xx en condiciones | |

Los 6 custom usados por estos dos mods son típicos CLEO 1.x/VC: test de teclas, estado del jugador,
animaciones, flags de script. El mod de crouch tiene hilo `ROLL` (rueda agachado) y el de swim
hilos `SWIM`/hijos (`WC*` = waits de hijas de nado).

**Procedencia de los CLEO** (verificado con `find`): `swim.cs` y `CrouchMovement(forClassicAxis).cs`
son los dos únicos sueltos. El resto de artefactos que aparecieron en listados del material
(`Scm2cleo.exe` = conversor scm→cs del mod Unity "AnimCam", `gxt-compiler` en Go,
`ConvertGXTTable.cs`, UABE, `freecam.cs`) viajan **dentro del material archivado** del pack, no
extraídos en `mods/`. `Scm2cleo` confirma el pipeline estándar: `.scm` de Sanny → `Scm2cleo.exe`
→ `.cs` CLEO.

## 3. Plan adaptador «CLEO-lite» (recomendado a medio plazo)

Objetivo: que el **catálogo entero de mods CLEO** corra en nuestro port sin reimplementar su
lógica, traduciéndola a nuestras mecánicas `VICEEXT_*`. No hace falta para swim/crouch (ya portados
nativamente) ni freecam (bloque C2), pero desbloquea **miles de scripts futuros**.

1. **Loader (≈100-150 líneas)** — la mecánica YA EXISTE en nuestro motor para cargar misiones
   (`src/control/Script6.cpp:403`): `CFileMgr::Read(handle, &ScriptSpace[SIZE_MAIN_SCRIPT],
   SIZE_MISSION_SCRIPT); CTheScripts::StartNewScript(SIZE_MAIN_SCRIPT);` + nombre de hilo vía
   `03A4`. Un `.cs` = volcarlo en una ranura de script + `StartNewScript(offset)`. Lo único nuevo:
   escanear `cleo/*.cs` al arrancar (y opcionalmente `.cm` = compiled metas).
2. **Opcodes custom (≈15-30 casos)** — ampliar el dispatch con `COMMAND_CLEO_XXXX` para cada opcode
   ≥1500 usado por los scripts que queramos. Implementar cada uno nativo contra nuestras APIs
   (pad/teclado, anims, cámara, player). Mapeo típico: test de tecla → nuestro pad, reproducción de
   clip → `CAnimManager`, flags → variables de script.
3. **Puente VICEEXT** — cada opcode custom delega en la mecánica nativa ya portada (nado,
   agachado, cámara…), así un script CLEO de "agachado" no reimplementa nada: llama a lo nuestro.
4. **Riesgos** (los que NO funcionarán): scripts que pisen memoria cruda del `ScriptSpace` por
   offsets, usen imports de Windows/DLLs, o el set de opcodes de **CLEO Redux** (distinto al de
   CLEO 1.x; el changelog de Extended menciona soporte de CLEO Redux — decidir cuál soportar).

Coste/beneficio: ~300-500 líneas de runtime frente a reimplementar a mano cada script de cada mod.
**Ruta recomendada para el futuro; innecesaria para las mecánicas Extended núcleo.**

## 4. HALLAZGO: nuestro `main.scm` servido NO es el de Extended v2510 ⚠️

| | Tamaño | md5 |
|---|---|---|
| servido (`bootseed/data/main.scm` = `streamed/data/main.scm`) | 1.269.133 B | `9739e0e9…` |
| mod v2510 (`mods/extended/GameFiles/ViceExtended/data/main.scm`) | 1.191.180 B | `56c322c7…` |
| `freeroam_miami.scm` servido | — | **idéntico al del mod** ✅ |

Comparativa de cadenas largas (≥10 chars): casi idénticas; **exclusivas del mod: `DESERT_EAGLE`,
`LODNGST2MESH`, `LODNGST2MESHDAM`**; exclusivas nuestras: ninguna. Interpretación: mismo esqueleto
de scripts, bytecode distinto (78 KB menos el del mod) = nuestro fichero es una compilación
base/vanilla y **el del mod incorpora los «Script changes» del changelog** (garages, compra de
tablones en Ammu-Nation, icono de armadura, pickups de cámara, coches que no desaparecen, teclas
de recarga con Shift, cutscenes saltables, metal detector fix, DEagle en script, LODs…).

⚠️ CORREGIDO (07 §14): el GXT servido **NO es idéntico** al del mod: los 6 idiomas servidos son
**214 bytes más grandes** cada uno (p.ej. american.gxt 429.584 vs 429.370 B) — diferencia
constante, pendiente de decodificar TKEY/TDAT para listar las claves de más/de menos.

**Acción recomendada (prioridad alta de paridad):** servir el `main.scm` del mod (subir `dataTag`),
regresión de misiones + freeroam en partida. Si algo rompe, documentar por qué servíamos el otro.

## 5. `movements.img` de ClassicAXIS — CORRECCIÓN del MD 04

En el `04` dije que contenía clips de piernas (`walk_left/right/back`) — **incorrecto**, ver §3 del
`04` (corregido). Realidad (verificado con `tools/ifp_inspect.py` sobre los 9 IFPs extraídos):
`movements.img` + `movements.dir` = **9 IFPs de bloques de arma**
(`baseball, buddy, chainsaw, flame, grenade, m60, python, rifle, shotgun`) con **34 clips
extendidos**:

- **8 clips de fuego/recarga AGACHADO** (= bloque C7 de nuestro plan): `buddy_crouchfire`,
  `M60_crouchfire`, `M60_crouchload`, `python_crouchfire`, `python_crouchreload`,
  `RIFLE_crouchfire`, `RIFLE_crouchload`, `shotgun_crouchfire`. Todos con **traslación de raíz**
  (`KRT0`) = el ped retrocede/avanza mientras dispara agachado.
- **Fuego+recarga combinado**: `buddy_fireRELOAD` (2,53 s).
- **Cuerpo a cuerpo con traslación**: `WEAPON_bat_h2` (0,07 m/s), `WEAPON_golfclub` (`KRTS`),
  `base_idle`, `WEAPON_bat_h/v`, `WEAPON_csawlo`, `FLAME_fire`, `WEAPON_mg`, `WEAPON_pump`,
  granadas (`WEAPON_start_throw/throw/throwu`) = clips de arma con raíz móvil para
  `bForceLegsMovements`: el torso hace el clip del arma mientras **las piernas quedan libres**
  para andar en 4 direcciones.

✅ VERIFICADO (07 §14): los 8 clips `crouchfire/crouchload` **ya están en nuestros IFPs servidos**
(`streamed/models/gta3.img/{rifle,shotgun,python,m60,buddy}.ifp`) — C7 no necesita acción de
datos; solo elegir set (duraciones ligeramente distintas al de `movements.img`).

Los clips de piernas **sí existen**, pero en `ped.ifp` (nuestro de 272 ya los tiene: `walk_left`,
`walk_right`, `walk_back`, `run_left/right/back`, `sprint_*`… confirmado por
`VC.CustomAnimsData.dat`: `%crouchmove` crouch_walk*, `%move` WALK…/walk_left 124/125/126/127…).
Esa tabla `.dat` es el mapa exacto de grupos/ids/flags → referencia para C5 («piernas a ojo»).

**Acción para C5/C7:** piernas con `walk_left/right/back` de `ped.ifp`; disparo agachado con los
`*_crouchfire/*_crouchload` de `movements.img` (extraer a nuestros IFPs de arma o empaquetar IFP
adicional que `AnimManager` cargue).

## 6. SilentPatch `Data/` — diffs reales de los 8 IPL contra nuestros datos

`mods/silent patch/Data/maps/` (club, littleha, mansion, nbeachbt, stripclb, washints…)
vs `gta_vc_browser/bootseed/data/maps/`. Categorías de cambio (⚠️ aplicar con validación: nuestros
ficheros ya difieren, posible solape con datos de Extended):

1. **Props restauradas**: `club_exterior03/04` añadidos al Malibu (`club.ipl`), mesas
   `propbeerglass*` del Pole Position (`stripclb.ipl`) — coincide con su readme: "props in Malibu
   Club, Ocean View Hotel, Pole Position Club restored".
2. **Columna área/interior** (3ª columna que nuestro `CFileLoader::LoadObjectInstance` lee como
   `area` → `entity->m_area`): SilentPatch usa 13/17/5/1 donde nosotros tenemos 0/-1/2 — su fix
   "more environment shows outside when the player is in the interior too (just like on PS2)".
3. **Escalas normalizadas**: `0.9999998808…` → `1` (`washints.ipl`).
4. **Secciones vacías**: nuestro `stripclb.ipl` tiene `end/cull/end/pick/end/path` que el suyo no.

✅ SEMÁNTICA RESUELTA en `07-verificacion-ejecutada-por-mod.md` §8: la 3ª columna es
`eAreaName` (`core/Game.h`) donde **13=AREA_EVERYWHERE, 17=AREA_MALIBU_CLUB, 5=AREA_STRIP_CLUB**;
el fix = ver el entorno exterior desde interiores (como PS2). Seguro de aplicar.

## 7. `weapon.dat` — ✅ verificado, no hay nada que hacer

Servido (`streamed/data/weapon.dat`) vs el del mod: 47 armas cada uno, **todas las filas de datos
idénticas tras normalizar espacios** (solo comentarios difieren). Nuestro
`CWeaponInfo::LoadWeaponData` ya parsea el de Extended (armas 48-56: DEagle, SKORPION, P90,
CALICO, M60, MINIGUN, STEYR, LASER, GRENADE LAUNCHER… con sus modelos y slots).

## 8. Resto de material documentado esta pasada

- **SilentPatch `Readme (ORIGINAL).txt`**: ~90 fixes concretos en texto plano + sus `Data/*.txt`
  (DrawBackfaces = listas EXACTAS de objetos que deben dibujar cara trasera: carteles huecos,
  camiones, naves, papeleras, mallplanters, nightlight01…). Especificación de implementación pura.
- **GInputAPI.h + docs**: API completa (`Ginput_GetPadState`, `GetBrakePressure`, vibración…) y los
  5 setups de mapeo (basic=estilo SA sin switch de arma, standard=VC con switch, modern, classic,
  manual) + deadzones/sensibilidad por stick. Spec exacta para nuestro mando (en web: Gamepad API).
- **SkyGfx README**: descripción de pipelines (PS2, PC, Xbox, mobile) y efectos (specular, fresnel,
  env map, dual pass, YCbCr→RGB, Neon 2D, postfx PC/mobile) = spec de la X2.10.
- **widescreen `global.ini`**: 200+ valores (escala de HUD, mensajes, subtítulos, FOV por
  situación, smart borders) = spec completa de X2.2.
- **`VC.CustomAnimsData.asi`** (binario): acompaña al `.dat`; en nuestro port solo necesitamos el
  `.dat` como referencia (nuestro `AnimManager` ya tiene las tablas `a*` equivalentes).

---

## 9. TABLA FINAL — % de cada mod incluible en nuestro port

Rutas de integración: **[FUENTE]** copiar/adaptar código fuente · **[NATIVO]** reimplementar en
nuestro motor (la lógica del ASI, dirigida por su config como spec) · **[DATO]** copiar datos ·
**[CLEO]** vía adaptador CLEO-lite.

| Mod | % incluible | Ruta | ✅ SÍ (se integra) | ❌ NO / limitación web |
|---|---|---|---|---|
| **climbing** (fuente reVC) | **95%** | [FUENTE] | Escalada completa: vallas `CLIMB_jump_B` (con translación), subida alta + `CLIMB_idle/Pull`, trepada de NPCs, cancelación por tecla, 8 params INI, anims CLIMB_* (ya en nuestro IFP) | ~5%: retoque de defines/teclas, limpiar `C` cirílica del fuente |
| **sa-crouch-movement** | **90%** | [DATO]+[NATIVO] | `VC.CustomAnimsData.dat` (mapa completo grupos/ids/flags), clips `GunCrouchFwd/Bwd`, `walk_left/right/back` para piernas; el port agachado ya está en motor | Su CLEO `(forClassicAxis)` — ya suplido por C5/C6 nativos (o [CLEO] si se quiere literal) |
| **swim** (1498977446) | **95%** | [NATIVO]+[DATO] | Nado completo (ya portado) + su `ped.ifp` con `Swim_*` | El `.cs` como tal (redundante; requeriría CLEO-lite) |
| **FirstPerson.asi** | **100%** | [NATIVO] | 1ª persona (bloque C1 hecho) | Su UI de configuración (no procede en web) |
| **SACarCam.asi** | **100%** | [NATIVO] | Cámara coche estilo SA (hecho) | — |
| **Classic AXIS** (2 versiones en `mods/`) | **70%** | [NATIVO]+[DATO] | `movements.img` 100% (34 clips: 8 `crouchfire/crouchload` = disparo agachado C7, `fireRELOAD`, poses de arma con raíz móvil = piernas libres), `ClassicAXIS.ini` como spec (zoom, sensibilidad, miras), clips `walk_*` de `ped.ifp` para strafe | Cámara IV exacta + mouse-cam estilo PC: el 30% fino de "sensación" |
| **SilentPatch VC** | **80%** | [NATIVO]+[DATO] | ~85 fixes de código (transparencias, coronas dobles, ráfagas 10s, timer brazo, puertas, gasolina, blips, pantalla partida 4:3, mirrors, multi2/3…), `DrawBackfaces` (listas exactas), fixes de IPL (§6, con validación) | Fixes Win32 (DEP, Alt+F4, DirectPlay, refresh rate, ventana, mouse-capture, voz de red) ≈ 15%: no aplican en web |
| **SkyGfx** | **60%** | [DATO]+[NATIVO] | `neo/carTweakingTable`, `worldTweakingTable`, `rimTweakingTable` 100% (= X2.10/11/12), `ps2Water`/`ps2Light`, `dualPass`, `YCbCr`/`ColorFilter`, envmap, building pipeline base, `skygfx.ini` completo como spec | Pipelines d3d9 exactos (rim/gloss con dual pass), wrapper d3d8/9, rain/blood-on-cam, motion blur (post-proceso) ≈ 40% de fidelidad cara |
| **WidescreenFix** | **70%** | [NATIVO] | Escalados HUD/radar/mensajes/subtítulos, FOV (cutscenes/speed/borders), `FixVehicleLights`, `SmartCutsceneBorders`, `global.ini` = spec de 200+ valores | Hooks de resolución/inyección: en web ya somos widescreen nativo (no aplican) |
| **FramerateVigilante** | **100%** | [NATIVO] | FPS limit 60 + timers independientes (ya hecho en T1/T2) | — |
| **GInput VC** | **75%** | [NATIVO]+[DATO] | 5 setups de mapeo (listas exactas docs), deadzones/sensibilidad, sniper zoom con gatillo, iconos btn (txd), crosshair, asignación vehículo→mando | XInput/SCP/Sixaxis (hardware Win) → Gamepad API; vibración XInput → Gamepad rumble (parcial) |
| **Vice Extended v2510** (global) | **65%** | mixto | Packs 1-4 100% (radar HD, peds, armas 48-56, vehículos, GXT, sfx), `weapon.dat` ✅ idéntico, `limits.ini`, MVL (ya), dummies de luces, `Sounds.txt`, custom anims | Night vertex colors (falta en nuestro GL3), modloader, tuning garages (talleres), photo mode, launcher; "Script changes" pendientes → §4 |
| **Ecosistema CLEO** (runtime nuevo) | **85% viable** | [CLEO] | Loader `.cs` (reutiliza la carga de misiones, `Script6.cpp:403`) + ~15-30 opcodes custom (≥1500) + puente `VICEEXT_*` → desbloquea el catálogo CLEO entero | Scripts con memoria cruda por offsets, imports Windows, o set CLEO Redux |
| **MVLConverter** | **100%** | — | Ya reimplementado (`gta_vc_browser/tools/import_mvl_vehicles.py`) | — |
| **Herramientas GXT** (`gxt-compiler` Go, `ConvertGXTTable` C#) | **100%** | — | Nuestro `tools/` ya cubre GXT/DBG | Solo si queremos el DBG v2 oficial |

### % global del objetivo «VC web = reVC + Extended + mods»

| Área | % |
|---|---|
| Motor web (reVC + on-demand + IDBFS) | ~90% |
| Datos/contenido Extended (packs 1-4, GXT, sfx, vehículos) | ~80% |
| Mecánicas Extended (X-list vital) | ~40% |
| Fixes SilentPatch | ~10% |
| Visuales SkyGfx | ~5% |
| Input/mando (GInput) | ~15% |
| SCM/scripts (main.scm + CLEO-lite) | ~70% |
| **GLOBAL** | **≈ 30% o menos (cifra del jugador, 24/09 — E22)** |

> **Corrección del jugador (24/09/2026 — E22):** el % global NO es ≈50 %: es
> menos — «falta lo más vital y notorio de Extended». El ≈45 %/≈50 % previo era
> optimista: no descontaba que las mecánicas más vitales y notorias de Extended
> (X-list de `vice-extended-pendientes.md`) siguen sin portar. Las tablas
> por-mod de arriba se quedan (siguen válidas). **Cifra fijada por el jugador:
> ≈30 % o menos.**

### Orden de ataque recomendado (mayor retorno primero)

1. **Servir el `main.scm` del mod** (§4) — 1 fichero, desbloquea todos los "Script changes".
2. **Escalada [FUENTE]** (climbing) — código ya escrito, copiar bloques del diff.
3. **C5 piernas/strafe** con `walk_*` + tabla `.dat` (§5).
4. **C7 disparo agachado** con `*_crouchfire/*_crouchload` de `movements.img` (§5).
5. **SilentPatch selectivo** (los ~20 fixes más visibles de su readme + DrawBackfaces).
6. **NeoVehicleShininess** con las tablas `neo/` (X2.10-12) + `FixVehicleLights` (X2.3).
7. **CLEO-lite** cuando empecemos a querer mods CLEO de terceros en masa.
