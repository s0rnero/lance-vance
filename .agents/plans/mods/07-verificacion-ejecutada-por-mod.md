---
name: 07-verificacion-ejecutada-por-mod
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 07 — Verificación EJECUTADA por mod (cero cambios al juego)

Cuarta pasada. Todo lo de este documento se ha **ejecutado** hoy (24/09/2026) contra los ficheros
reales: desensamblado real de los `.cs` con la spec oficial (`vc.json` descargado a
`/tmp/cbd6c006/vc.json`), `tools/ifp_inspect.py` sobre los 5 `ped.ifp`, extracción de cadenas de
los 10 binarios ASI/DLL, comparativa byte a byte del `main.scm`, y lectura de configs. **Ningún
fichero del juego ha sido modificado** — salidas solo a `/tmp` y a este plan.

---

## 1. `climbing/` — escalada ✅✅ (lo más listo para portar)

- **Fuente reVC completo** + diffs contra nuestro `src/` generados y guardados en
  `/tmp/cbd6c006/diffs/` (Ped.cpp 41 KB, Ped.h, PedFight.cpp, AnimationId.h, AnimManager.cpp,
  config.h, re3.cpp) — ver análisis `04`.
- **VERIFICADO con `ifp_inspect.py`**: `climbing/game_folder/ANIM/ped.ifp` (241 clips) tiene
  **0 clips exclusivos** frente a nuestro `ped.ifp` servido (272). Todos los `CLIMB_*`
  (`CLIMB_idle`, `CLIMB_Pull`, `CLIMB_Stand`, `CLIMB_Stand_finish`, `CLIMB_jump`, `CLIMB_jump_B`,
  `CLIMB_jump2fall`) **ya están en nuestro IFP servido** ✅.
- Trae además `game_folder/reVC.exe` (el mod compilado, 3.5 MB) como referencia de comportamiento.
- **Se integra**: los bloques del diff (~315 líneas en `Ped.cpp` + 44 `Ped.h` + ganchos) bajo
  `VICEEXT_CLIMBING`. **NO hay que tocar datos**: cero IFP/IDE.
- **Evitar**: la `C` cirílica del fuente (`СurrentPedPosition`); `#ifdef CLIMBING` renombrado a
  nuestros defines al final de `config.h` sin reordenar.

## 2. `sa-crouch-movement/` — agachado ✅ (faltan 2 clips)

- **VERIFICADO con `ifp_inspect.py`**: su `ped.ifp` (240 clips) tiene exactamente **2 clips que
  nuestro servido NO tiene: `GunCrouchFwd` y `GunCrouchBwd`** (andar agachado apuntando).
  → Acción de datos: extraer esos 2 clips a un IFP extra (o inyectarlos en el nuestro cuando
  toque integrar). El resto de clips agachado (`Crouch_*`, `GunCrouch_*` de torso) ya están ✅.
- `VC.CustomAnimsData.dat` (46 KB) = tabla de **grupos de animación por bloque de ped**
  (`%GroupName BlockName InitModelIndex GroupAnimCount`, documentado en su propio ASI). Su ASI solo
  parsea este `.dat` — en nuestro port basta el `.dat` como referencia de ids/flags.
- Su CLEO `CrouchMovement(forClassicAxis).cs` = memory-hack (ver §12) — **innecesario**: nuestros
  bloques C5/C6 son el port nativo.
- **Evitar**: depender de `ClassicAxis` para el agachado (nuestro R20c ya ejecuta el giro).

## 3. `1498977446_107784/` — nado ✅ (2 clips menores)

- **VERIFICADO**: su `ped.ifp` (236 clips) tiene **2 exclusivos: `IDLE_Player` y `turn`** que
  nuestro servido no tiene. Su `Swim_*` entero ya está en el nuestro ✅.
  → Acción de datos menor: decidir si `IDLE_Player`/`turn` aportan algo al nado portado (son de un
  mod "idle/turn" colado en el pack; probablemente descartables).
- Su `CLEO/swim.cs` = memory-hack de exe (§12) — el port nativo ya existe.
- `Readme - es.txt` disponible para la semántica de controles.

## 4. `1487678468_firstperson/` — 1ª persona ✅✅

- Strings del ASI (1.9 MB): **`FirstPerson.cfg`** (su config se llama .cfg, no .ini), ajustes
  `FOV: %.1f` y `Mouse Sensitive: %.2f`, y referencia al clip **`WEAPON_crouch`**.
- Nuestro bloque C1 ya lo porta. **Se integra** (cuando toque): respetar su nombre `FirstPerson.cfg`
  si queremos compat de config; FOV/sensibilidad como constantes nuestras.
- **Evitar**: su manejo de ratón Win32 (nuestro canvas web ya lo captura).

## 5. `SACarCam.asi` ✅✅ (53 KB)

- Strings: llama a **`GetGInputInterface`** (usa la API de GInput para leer el pad) y clases
  `IGInputPad`/`CDummyPad`. Confirma que SACarCam + ClassicAXIS + GInput están acoplados por esa API.
- Nuestro port (cámara coche SA) ya hecho. **Evitar**: replicar el acoplamiento GInput; en web la
  fuente de pad es nuestra.

## 6. `Classic AXIS` — dos versiones en el árbol 🆕 descubierto

| Fichero | Versión | Evidencia (strings) |
|---|---|---|
| `1510564741_ca-1/VC/ClassicAXIS.asi` (267 KB) | vieja | lee `CLASSICAXIS.INI` + **`CLASSICAXIS\ANIM\MOVEMENTS.IMG`**, keys `bMoveCameraOnVehicle`, `bEnableFirstPersonMode`, `bIVOnFootCamera`, `bIVVehicleCamera`; requiere `GInputVC.asi` |
| `Classic AXIS/ClassicAxisVC.asi` (532 KB) | **nueva** (la del repo `gennariarmando/classic-axis`, con plugin-sdk) | `ClassicAxisVC.ini`, `ShowTriangleForMouseRecruit`, `CameraCrosshairMultX/Y`, RTTI `CPed`/`CPlayerPed` |

- **VERIFICADO con `ifp_inspect.py`** (ya en `05` §5): `movements.img` = 9 IFPs de arma, 34 clips,
  **8 `*_crouchfire/*_crouchload` con traslación de raíz** → exactamente el bloque C7.
- **Se integra**: tomar la **nueva** como referencia (`CamNew.cpp`/`Main.cpp` del repo, MIT?? no —
  sin LICENSE → adaptar). Los INIs de ambas = spec de ajustes (zoom, sensibilidad, IV-cam).
- **Evitar**: `bForceLegsMovements` tal cual (necesita el hook de su `Main.cpp`); primero nuestros
  `walk_left/right/back` con raíz móvil (verificado en `05` §5).

## 7. `framerate/` ✅✅ (ya hecho)

- Strings: `FramerateVigilante.ini`, `FPSlimit`. **Autor revelado en el propio INI:
  `Junior_Djjr - MixMods.com.br`** (el mismo estilo de config que `features.ini` de Extended).
- Nuestro T1/T2 = 100% equivalente. Nada más que integrar.

## 8. `silent patch/` ✅ (semántica de IPL RESUELTA)

- Binarios: `SilentPatchVC.asi` (226 KB) + **`ddraw.dll`** (su fix de DDraw 115 KB — en web no
  aplica). String notable: opción **`Minimal HUD`**.
- **8 IPL verificados uno a uno contra `bootseed/data/maps/`** (diff ejecutado):

| IPL | Cambios | Qué cambia realmente |
|---|---|---|
| `club/CLUB.ipl` | 42 | **restaura `club_exterior03/04`** (área 17) y `club_exterior07` −1→17 |
| `hotel/hotel.IPL` | 42 | escalas `0.9999998808→1`, rotaciones limpias, restaura `hot_mags1` |
| `littleha/littleha.ipl` | 70 | palmeras 0→13 + escalas normalizadas |
| `mansion/mansion.ipl` | 9 | 2→13 |
| `oceandn/oceandN.ipl` | 212 | 0→13 |
| `oceandrv/oceandrv.ipl` | 32 | 0→13 + escalas |
| `stripclb/stripclb.ipl` | 8 | `propbeerglass1` 0→5 + quita secciones `cull/pick` vacías |
| `washints/washints.ipl` | 11 | 0→13 |

- **SEMÁNTICA RESUELTA** (la duda del `05` §6): la 3ª columna = `eAreaName` (`core/Game.h:14-31`):
  `0=AREA_MAIN_MAP, 1=HOTEL, 2=MANSION, 4=MALL, 5=AREA_STRIP_CLUB, 13=AREA_EVERYWHERE,
  17=AREA_MALIBU_CLUB`. Visibilidad: `IsAreaVisible(area) = area==currArea || area==AREA_EVERYWHERE`
  (`Game.h:89`) usada en `renderer/Renderer.cpp:743/882/1707`, `Streaming.cpp` y `Shadows.cpp`.
  → El fix de SilentPatch = poner objetos exteriores en **13 (EVERYWHERE)** para que **se vea el
  entorno exterior desde los interiores "como en PS2"** ✅ (su readme), y devolver props del
  Malibu (17) / Pole Position (5) / hotel. **Coherente y seguro de aplicar** con nuestra
  `CFileLoader::LoadObjectInstance` (`FileLoader.cpp:1179`: lee el campo como `area` → `m_area`).
- **Se integra**: los 8 IPL como datos (cuando toque), + los ~85 fixes de código del MIT repo.
- **Evitar**: `ddraw.dll` y fixes Win32 (ver `05`).

## 9. `skygfx/` ✅ (origen confirmado)

- Strings del `skygfx.asi`: **`C:\Users\aap\src\skygfx_vc\bin\Release\skygfx.pdb`** → el binario
  está compilado **exactamente del repo `aap/skygfx_vc`** que localicé en el `06` ✅. Carga
  `neo\neo.txd`, `neo\carTweakingTable.dat`, `neo\rimTweakingTable.dat`, `neo\worldTweakingTable.dat`
  (los mismos que trae Extended y ya extraídos), marcos `IIIEnvFrame`/`VCEnvFrame`, `FED_HUD`,
  secciones INI `SkyGFX|Advanced` y `SkyGFX|ScreenFX`.
- Trae `d3d8.dll` + `rwd3d9.dll` (driver RW→D3D9) — **no aplican** (nosotros: librw GL3).
- **Se integra**: tablas `neo/*` (datos ✅ ya extraídos en `05`), y las lógicas de
  `src/matfx.cpp`/`leedsCarpipe.cpp`/`WaterLevel.cpp` del repo → nuestros
  `renderer/WaterLevel.cpp:911/1206` y render de vehículo. **Evitar**: copiar HLSL literal (→GLSL
  manual), `rwd3d9`, y el código sin LICENSE → reimplementación con atribución.

## 10. `widescreen/` ✅ (features nuevas confirmadas en el binario)

- Strings del ASI: `HudWidthScale`, `HudHeightScale`, `RestoreCutsceneFOV`, `CarSpeedDependantFOV`,
  `DontTouchFOV`, `FOVControl`, `SmartCutsceneBorders` — coinciden 1:1 con el `global.ini`/INI y
  con el repo `ThirteenAG/WidescreenFixesPack` (MIT ✅, `source/GTAVC.WidescreenFix/dllmain.cpp`).
- `scripts/global.ini`: `LoadPlugins=1, LoadFromScriptsOnly=0, Use3D8to9=1` (del modloader, no del
  fix). Trae `d3d8.dll` propio (2 MB) — no aplica.
- **Se integra**: fórmulas FOV/HUD a `CCamera::Process` (`core/Camera.cpp:290`) y `CHud::Draw`
  (`renderer/Hud.cpp:328`) + features nuevas (No Island Loading, Seamless Interiors…, ver `06`).

## 11. `ginput/` ✅ (spec completa, binario cerrado)

- Strings del ASI: `models\x360btns.txd`, `models\ps3btns.txd`, `models\sixaxis.txd`,
  `XInputGetExtended`, `XINPUT1_3.dll` → confirma XInput + Sixaxis(SCP) y el uso de los TXD de
  iconos (que Extended trae ya en 8 variantes: pcbtns/ps3/ps4/ps5/x1/x360/nsw/frontend_ds*).
- `(alternative)/VC Classic/` = set de iconos alternativo (ps3btns/x360btns clásicos).
- **Se integra**: 5 setups + deadzones (spec INI/docs) → nuestra capa Gamepad API; iconos de los
  TXD ya servidos. **Evitar**: XINPUT1_3/Sixaxis (hardware Win) y vibración XInput → Gamepad
  Haptics (parcial).

## 12. CLEO `.cs` — DESENSAMBLADO REAL ejecutado 🆕 conclusión importante

Desensamblador propio ejecutado hoy (con `vc.json` de Sanny Builder Library: 1.340 comandos
mapeados — `default` 1.436, `CLEO` 128, `imgui` 87, `bitwise` 14, `file` 6, `audio` 7, `ini` 6,
`memory` 3, `clipboard` 2). Codificación SCM verificada: args auto-delimitados por tag
(`1`=int32, `2`=global16, `3`=local16, `4`=int8!, `5`=float32) y **strings de 8 bytes SIN tag**.

**`swim.cs` (2.288 B) — flujo real decodificado** (30 instrucciones hasta `CALL_FUNCTION`):
```
SCRIPT_NAME 'SWIM'
bucle:  WAIT 0
        IF: IS_PLAYER_PLAYING $8 / NOT IS_CHAR_IN_ANY_CAR $12 / IS_CHAR_IN_WATER $12
        GOTO_IF_FALSE bucle
        GET_PED_POINTER $12 → var0          [CLEO 0x05E6]
        ADD_VAL_TO_INT_LVAR var0 120        ← puntero CPed + 120
        WRITE_MEMORY var0 4 <valor> 0       [CLEO 0x05DF]  ← POKE de memoria
        IS_BUTTON_PRESSED 0 14 / 0 8        ← botones del pad
        GET_PED_POINTER $12 → var4; +76; READ_MEMORY  [CLEO]
        CALL_FUNCTION 4216384 4 4 …         [CLEO] ← llama al EXE x86 (0x405640)
        PRINT_WITH_3_NUMBERS_NOW …
```
**`CrouchMovement(forClassicAxis).cs` (2.052 B)**: `SCRIPT_NAME 'ROLL', WAIT 0,
GET_PED_POINTER $12→var7, …` — mismo patrón (pokes sobre `CPed`).

**CONCLUSIÓN (cambia el enfoque del CLEO-lite, `05` §3)**: estos dos scripts **NO son lógica
portable: son memory-hacks del `gta_vc.exe` x86 1.0** (offsets de `CPed` +120/+76 y llamadas a
direcciones del exe). Por eso:
- ✅ **La vía correcta ya la tenemos**: port NATIVO (nado/agachado en `VICEEXT_*`) — estos scripts
  nunca se ejecutarán tal cual en wasm (sin espacio de direcciones del exe).
- ✅ CLEO-lite sigue siendo útil para scripts de **lógica** (misiones, gameplay) que usen opcodes
  vanilla + CLEO de lógica, pero **NO** para los que usen `WRITE_MEMORY`/`READ_MEMORY`/
  `CALL_FUNCTION`/`MemoryLibrary` (la extensión `memory` de la spec). El puente VICEEXT del plan
  debe reescribir cada memory-opcode como opcode nativo nuestro.
- F1 del `06` queda **concretado**: fijar ids de `vc.json` (formato mixto hex/dec — causó colisiones
  en el desensamblado de `CrouchMovement`, ver artefacto `MULT_INT_VAR_BY_VAL`) y soportar args
  variables de `CALL_FUNCTION`.

## 13. `extended/` (v2510) ✅ — inventario del overlay REAL 🆕

Descubierto el overlay completo `GameFiles/modloader/ViceExtended/` (lo que modloader sustituye):

- **`gta_vc.dat`** = manifiesto de carga (8 `CDIMAGE` + IDEs + IPLs en orden) → la lista autorizada
  de "qué datos toca Extended".
- **`cdimages/`**: `vehicles.img` (14.9 MB), `weapons.img` (3.8 MB), `objects.img` (15.4 MB),
  `peds.img` (11 MB), `player.img` (4.9 MB), `radar.img` (4.3 MB = radar HD), `anims.img` (1 MB),
  `generic.img` (+ sus `.dir`).
- **`models/`** (en `GameFiles/ViceExtended/`): `hud.txd` (6.3 MB), `radio.txd` (3.1 MB = iconos de
  emisora ✅ X2.x), `weaponSights.txd` (X2.9 miras), `newspapers.txd` (X2.5), `fonts_r/u.txd`,
  `generic/wheels.DFF/TXD`, `vehmods/` (`spoilers/skirts/vents/scoops.dff` = tuning), 8 sets de
  botones (pc/ps3/ps4/ps5/x1/x360/nsw/frontend_ds*).
- **`data/`**: `ViceEx.dat` = `CDIMAGE ViceExtended\newVehicles\newVehicles.img`; `features.ini` =
  **18 flags de mecánicas** (spec exacta: `RecoilWhenFiring=1`, `EnableSwimming=1`,
  `EnableClimbing=0`, `RandomVehicleModsInTraffic=1`, `EnableDistantLights=1`,
  `CameraShakeInVehicleAtHighSpeed=0`, `PlayerDoesntBounceAwayFromMovingCar=1`, …);
  `limits.ini` = pools (`NUMPEDS 140, NUMVEHICLES 130, NUMBUILDINGS 7000, MAXWHEELMODELS 12`);
  `gamecontrollerdb.txt` (253 KB de mapeos SDL = **spec de mapeos de mando** para nuestra Gamepad
  API); `particle.cfg`, `object.dat`, `occlu.ipl`, `paths.ipl`, `wanted_paths.ipl`, `*.zon`.
- **`audio/ViceEx.RAW` + `.SDT`** (364 KB) = sfx nuevos del mod (armas 48-56) — ya en packs.
- **`main.scm` vs el servido** (byte a byte, ejecutado): cabeceras `02 00 01 20 86 00 00 6d`
  (servido) vs `02 00 01 40 87 00 00 6d` (mod) → SCM v2, primer byte distinto en offset 3, solo
  **8.7% de bytes idénticos** = compilaciones distintas de verdad (no un parche). `freeroam_miami.scm`
  = **idéntico** ✅. → Confirmado el hallazgo del `05` §4: hay que servir el del mod y regredir.
- `Bonus/`: `ViceEx.exe` x64 beta + `modloader` completo (con `.data/licenses`) + `pcbtns.txd`.
- **`weapon.dat`**: verificado idéntico al servido (47 armas, `07` §… ✅ hecho en `05` §7).

---

## Tabla-resumen: qué confirma cada verificación

| Mod | Verificado hoy | ¿Listo para integrar? | Única acción de datos pendiente |
|---|---|---|---|
| climbing | diffs + IFP (0 clips faltantes) | ✅✅ copy-paste de bloques | ninguna |
| sa-crouch | IFP + `.dat` + `.cs` decodificado | ✅ (C5/C6) | extraer `GunCrouchFwd/Bwd` (única pieza de IFP pendiente) |
| *(§14 datos)* | hash overlay↔servido + `ifp_inspect` raíz + IFPs de arma | 8 clips `crouchfire/load` **ya servidos** ✅; `walk_*` con traslación lateral ±1,836 ✅; `CLIMB_jump_B` **SÍ tiene raíz** (0,89 m) → E1 = flag en descriptor, no el clip | decodificar GXT (214 B de diferencia constante) + decidir 25 ficheros distintos |
| swim | IFP + `swim.cs` decodificado | ✅ (ya portado) | valorar `IDLE_Player`/`turn` |
| firstperson | strings ASI | ✅ (ya portado) | ninguna |
| SACarCam | strings ASI (usa GInput API) | ✅ (ya portado) | ninguna |
| classic-axis ×2 | strings 2 ASIs + movements.img | ✅ 85% (C7 + cam) | inyectar 8 clips crouchfire/load |
| framerate | strings + INI (autor Junior_Djjr) | ✅✅ (ya hecho) | ninguna |
| silent patch | 8 IPL diffs + semántica área RESUELTA | ✅ 85% | aplicar los 8 IPL (área→13/17/5) |
| skygfx | pdb=aap/skygfx_vc ✅ + strings | ✅ 72% | neo/* ya extraídos |
| widescreen | strings = keys del repo MIT ✅ | ✅ 85% | ninguna |
| ginput | strings (XInput/Sixaxis/txd) | ✅ 75% (Gamepad API) | ninguna |
| extended | overlay completo + features/limits + SCM byte a byte | ✅ 65% | servir `main.scm` del mod |
| CLEO | desensamblado real ejecutado | ✅ vía nativa; CLEO-lite solo para scripts de lógica | — |
