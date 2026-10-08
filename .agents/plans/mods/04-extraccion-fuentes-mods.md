---
name: 04-extraccion-fuentes-mods
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-09-23
---

# Extracción desde `mods/` — análisis de fuentes y qué copiar/adaptar

> Fecha: 23/09/2026 (hilo Buffy).
> Encargo: leer TODO `mods/` línea a línea, comparar cómo funciona cada mecánica
> allí y cómo está en nuestro motor, y dejar constancia de lo que se puede
> **copiar o adaptar** (nada inventado) para avanzar más rápido en la integración
> de Vice Extended (≈50% hecho; faltan sus cosas más vitales y notorias).
> Método: diffs `diff -u` de los fuentes del mod de escalada contra nuestro `src/`,
> lectura de INIs/docs, extracción de cadenas de binarios, y comparación de
> `ped.ifp`s con `tools/ifp_inspect.py`.

---

## 0. Inventario — qué hay realmente en cada carpeta

| Carpeta | Qué es | ¿Trae código fuente? | Valor para el port |
|---|---|---|---|
| `climbing/` | "Climbing [reVC]" — escalada estilo SA para la rama **miami** | **SÍ, fuente reVC completo (7 ficheros)** | ★★★★☆ JOYA: implementación de referencia de la escalada |
| `sa-crouch-movement_1774634386_626056/` | "SA Crouch Movement" para ClassicAxis | No (ASI + CLEO compilado) + **`.dat` legible** | ★★★☆☆ tabla de grupos/flags de anims + 2 clips que nos faltan |
| `Classic AXIS/` + `1510564741_ca-1/` | ClassicAxisVC 1.6 (gennariarmando) | No (ASI) + `movements.img` + INI | ★★★☆☆ piernas en 4 direcciones (pendiente del agachado) |
| `1498977446_107784/` | "Swimming with the new animation" (CLEO, Serega) | No (CLEO compilado) + `ped.ifp` | ★☆☆☆☆ ya tenemos nado; 2 clips menores |
| `1487678468_firstperson/` | FirstPerson.asi (GeniusZ) | No (binario) | ★☆☆☆☆ ya tenemos VICEEXT_FIRST_PERSON |
| `SACarCam.asi` | cámara de coche estilo SA | No (binario suelto) | ★☆☆☆☆ ya tenemos Process_FollowCar_SA |
| `silent patch/` | SilentPatchVC 1.1 build 12.1 | No (ASI) + **INI con listas de datos** + readme | ★★★☆☆ listas DrawBackfaces + placements de sirenas |
| `skygfx/` | SkyGfx (Xbox/PS2 pipelines) | No (ASI) + INI + **`neo/*.dat`** | ★★★☆☆ = "Detalle de vehículo" (NeoVehicleShininess) §2.10 |
| `widescreen/` | WidescreenFixesPack | No (ASI) + INI | ★★☆☆☆ constantes HUD/FOV (plan 03) |
| `framerate/` | FramerateVigilante | No (ASI) + INI | ★☆☆☆☆ FPSlimit=60 (plan 02) |
| `ginput/` | GInput VC (Silent) — mando | No (ASI) + **`GInputAPI.h` (428 l.)** + docs | ★★☆☆☆ 5 setups de mando completos para el web |
| `extended/` | **Vice Extended v2510 completo** | Solo `SourceCode/MVLConverter/` | ★★★☆☆ checklist de pendientes + datos |
| `vice-extended-october-2025-update_1788102643_908113/` (raíz del repo) | v2510 otra copia | Solo MVLConverter | mismo contenido; `ped.ifp` md5-idéntico |

**Lo único que es código C++ legible en todo `mods/`:**
1. `climbing/source_code/` — fuente reVC completo con `#ifdef CLIMBING`.
2. `extended/SourceCode/MVLConverter/` (MVLConverter.cpp 321 l. + tinyxml) — ya reimplementado en `tools/import_mvl_vehicles.py`.
3. `ginput/(docs, api)/GInputAPI (for modders)/GInputAPI.h` — API del gamepad (documentación viva del estado de input).

Los `.cs` de CLEO (`swim.cs`, `CrouchMovement(forClassicAxis).cs`) son bytecode compilado: no se leen, sólo strings. Los `.asi`/`.exe` (`SilentPatchVC.asi`, `skygfx.asi`, `ViceEx.exe`, etc.) son binarios: su **spec** está en los INIs/readmes/ChangesEN, y `ViceEx.exe` se mina con `tools/viceex-strings.py`.

---

## 1. `climbing/` — fuente COMPLETO del mod de escalada ★ la pieza más valiosa

El `ReadMe.txt` (ruso) dice literalmente: *"con un repo de reVC, copiar el
contenido de `source_code` en la **rama miami**, confirmar la sustitución y
compilar"* — o sea que los 7 ficheros del mod son diffs aplicables tal cual a
nuestro árbol (la misma base). Lo comparé con `diff -u` contra nuestro `src/`
actuales (guardado en `/tmp/cbd6c006/diffs/`): el mod añade **~315 líneas en
Ped.cpp + 44 en Ped.h + 10 en PedFight.cpp + 20 en AnimManager.cpp + 9 en
AnimationId.h + 20 en re3.cpp + 1 define en config.h**, todo bajo `#ifdef CLIMBING`.

### 1.1 Cómo funciona allí (arquitectura)

**Detección** — `CPed::CanPedClimbingThis(hitForward, hitBackward, hitJumpB)`:
1. Puertas: NPC sin `bClimbingPeds` → no; interiores sin `bClimbingInInteriors` → no.
2. `CheckObjectFrontPlayer`: **8 rayos** `CWorld::ProcessLineOfSight` (hacia delante
   a las alturas 0, +1.45, +1.55, +1.65; y desde +3.0 hacia 0.25/0.5/0.75/1.0×forward).
3. `CheckObjectAbovePlayer`: rayo vertical hasta z+2.75 **+ 5 esferas**
   (`TestSphereAgainstWorld` r=0.25 en z+1.0…2.0) → si hay techo, no trepa.
4. `CheckPotentialClimbingPlaceFind`: rayo vertical desde el borde
   (`hit + (-normal*0.25) + z*maxPossibleClimbingHeight`) hacia abajo → encuentra
   la superficie de aterrizaje (`hitBackward`).
5. `CheckClimbingPlaceFree`: **5 esferas** en el aterrizaje (z+0.3 r=0.215,
   z+0.5 r=0.2, z+0.75 r=0.4, z+1.5 r=0.4, y `forward*0.25`+z+0.5 r=0.1) → sitio libre.
   Además pide `hitBackward.normal.z >= 0.8` (suelo llano).
6. Si no hay "repisa", `CheckClimbingTheFence` (**vallas**, la gran diferencia con
   nuestro E1): `IsNeedFixFenceSideNormal` (si `DotProduct(normal, forward) > 0` la
   colisión dio la normal al revés → se invierte) y
   `CheckPotentialClimbingTheFencePlaceFind`: itera **40 pasos de +0.01 z** subiendo
   por el canto de la valla con rayos ±2.0 y esferas r=0.5 (delante y arriba).
   Si `Distance(hitForward.point, hitJumpB.point) < 0.25` → valla FINA →
   `bIsClimbingJumpB` (salto de lado con `CLIMB_jump_B`). Si el canto supera
   `maxHighClimbingHeight` → `bIsClimbingHighJump` (subida alta).

**Estados y animación** — nuevo estado `PED_CLIMBING` (enum de `CPed`). En
`StartClimbing`: `bAffectedByGravity=0`, `bUsesCollision=0`, rumbo = `-normal`
(mira hacia el obstáculo), y tres rutas:
- **Baja** (≤1.5 m): `CLIMB_Stand` → CB `FinishClimbingCB` → `CLIMB_Stand_finish`
  (`FinishFinallyStandClimbingCB`) → `EndClimbing(false)`.
- **Alta** (>1.5 m): `CLIMB_jump` → CB `FinishHighClimbingCB` → se queda colgado en
  `CLIMB_idle` (`bIsClimbingIdle`) esperando `JumpJustDown` → `CLIMB_Pull`
  (`FinishClimbingPullCB`) → `CLIMB_Stand` → fin. (Sólo jugador; triángulo cancela
  salvo en la fase alta inicial.)
- **Valla**: `CLIMB_jump_B` (**`ASSOC_HAS_TRANSLATION`** — el clip empuja al ped
  hacia el otro lado) → fin directo.

**Desplazamiento trepando** (en `ProcessControl`, al inicio): interpolación suave
hacia el destino con `SetPosition(pos + normal*delta)` — velocidades **1.5 m/s**
(normal), **1.3** (pull), **0.75** (valla), `highClimbingOffsetSpeed=3.0` (colgada→repisa).

**Ganchos en código existente** (los 7 sitios que toca):
- `SetJump`: si `CanPedClimbingThis` → `jumpAssoc->blendDelta = 4.25f`.
- `FinishLaunchCB`: si escalable → `StartClimbing` y return; si no →
  `bIsReadyToClimbing = true` (y **reemplaza** el chequeo vanilla de "muro delante
  → bIsLanding": la escalada gana al wall-check).
- `ProcessControl` (inicio): `bIsReadyToClimbing` → reintenta `CanPedClimbingThis`
  → `StartClimbing`; bloque completo de `bIsClimbing` (input y desplazamiento).
- Rama de caída: `if (m_vecMoveSpeed.z > playerVerticalVelocityAtWhichStartsToFall)
  return;` — el ped **no entra en FALL** mientras sube/todavía puede agarrarse.
- `ProcessEntityCollision`: `if (bIsClimbing) return 0;` — sin colisión trepando.
- `PedFight.cpp`: golpear a un ped que trepa es válido
  (`victimPed->IsPedInControl() || victimPed->bIsClimbing`).
- `RestorePreviousState`: limpia `bIsReadyToClimbing`.
- Chequeo de "muro" para NPC (`@@ -9428`): los NPC suman
  `maxPossibleCheckHeightForPeds` al z (trepan cosas más altas que el jugador).
- Salto de NPC: velocidad forward `0.15×GetForward()` (hacia el obstáculo).

**Animaciones** (6 IDs al final de `ANIM_STD_*`, nombres al final de
`aStdAnimations`, flags al final de `aStdAnimDescs` — orden consecutivo, ver trampa
R20 sobre `firstAnimId`):

| Clip | Flags del mod | Nuestro E1 (aClimbAnimDescs) |
|---|---|---|
| `CLIMB_idle` | `REPEAT\|FADEOUTWHENDONE\|PARTIAL` | `REPEAT\|PARTIAL` |
| `CLIMB_jump` | `DELETEFADEDOUT\|PARTIAL` | `FADEOUTWHENDONE\|PARTIAL` |
| `CLIMB_jump_B` | `FADEOUTWHENDONE\|PARTIAL\|**HAS_TRANSLATION**` | `FADEOUTWHENDONE\|PARTIAL` ← **sin traslación** |
| `CLIMB_Pull` | `DELETEFADEDOUT\|PARTIAL` | `FADEOUTWHENDONE\|PARTIAL` |
| `CLIMB_Stand` | `DELETEFADEDOUT\|PARTIAL` | `FADEOUTWHENDONE\|PARTIAL` |
| `CLIMB_Stand_finish` | `DELETEFADEDOUT\|PARTIAL` | `FADEOUTWHENDONE\|PARTIAL` |

(El mod NO usa `CLIMB_jump2fall`; nuestro E1 sí lo mapea — se queda, no estorba.)

**Constantes exactas del mod** (defaults + INI `[Climbing]` vía `re3.cpp`
ReadIniIfExists/StoreIni, 8 claves):

```
bClimbingInInteriors = false        ClimbingInInteriors
bClimbingOnVehicles  = false        ClimbingOnVehicles
bClimbingPeds        = true         ClimbingPeds          ← ¡NPCs trepan!
maxPossibleClimbingHeight = 2.25f   MaxPossibleClimbingHeight
maxPossibleCheckHeightForPeds = 2.85f  MaxPossibleCheckHeightForPeds
maxHighClimbingHeight = 1.5f        MaxClimbingWithRaisedHandsHeight
highClimbingOffsetSpeed = 3.0f      ClimbingWithRaisedHandsOffsetSpeed
playerVerticalVelocityAtWhichStartsToFall = -0.2f   PlayerVerticalVelocityAtWhichStartsToFall
```
Offsets internos: `correctedPedZOffset` 0.1 (stand) / 2.0 (alta);
`newClimbingIdlePosition` = inicio + z*0.95; destino del pull = stand + z*0.5;
destino de valla = `hitForwardPoint - normal*5.0`; blendDeltas: cancelación
−1000/−5/−2.5, finish stand −1/−3, idle blend 10.0.

### 1.2 Cómo está hoy en nuestro port (bloque E1)

Nuestro `VICEEXT_CLIMB` es un **subconjunto**: sólo salto hacia un borde bajo con
`CLIMB_jump`/`CLIMB_Stand`. Sin vallas (`CLIMB_jump_B` sin `HAS_TRANSLATION` no
desplaza), sin subida alta con colgada+pull, sin NPCs, sin cancelación por tecla,
sin INI. El `ped.ifp` de 272 clips que ya servimos trae TODOS los clips del mod
(comprobado: los 241 del `ped.ifp` de `climbing/` ⊂ nuestros; sólo cambia que el
nuestro trae además crouch/swim/drive-by de Extended).

### 1.3 Qué extraer y cómo (mapa fichero→fichero)

| Fichero del mod | Nuestro fichero | Acción |
|---|---|---|
| `src/peds/Ped.cpp` (bloques `#ifdef CLIMBING`) | `src/peds/Ped.cpp` | copiar bloques + ganchos, **adaptando a `VICEEXT_CLIMB`** |
| `src/peds/Ped.h` (campos, 12 métodos, 4 CBs, `PED_CLIMBING`) | `src/peds/Ped.h` | copiar bajo `VICEEXT_CLIMB` |
| `src/peds/PedFight.cpp:374` | `src/peds/PedFight.cpp` | copiar la condición de víctima trepando |
| `src/animation/AnimManager.cpp` | `src/animation/AnimManager.cpp` | usar sus flags (tabla 1.1) en nuestro `aClimbAnimDescs` |
| `src/animation/AnimationId.h` | `src/animation/AnimationId.h` | ya tenemos los IDs (E1); mantener el bloque al final |
| `src/core/re3.cpp` (INI `[Climbing]`) | `src/core/re3.cpp` | copiar las 8 claves (nuestro ini va en `/userfiles/reVC.ini`) |
| `src/core/config.h` `#define CLIMBING` | `src/core/config.h` | ya existe `VICEEXT_CLIMB`; no hace falta el define suyo |

### 1.4 Trampas pagadas del mod (a respetar al adaptar)

- **`CVector �urrentPedPosition`**: el fuente tiene la `C` inicial en cirílico
  (`\xd0\xa1`) en `ProcessControl`. Renombrar a `currentPedPosition` al copiar.
- El mod **no** tiene nuestros fixes: su `CanStrafeOrMouseControl` sólo mira
  `CCamera::bFreeCam` (nosotros corregimos con `!m_bUseMouse3rdPerson` — NO
  adoptar la suya), ni nuestro `odCrouchMove` del corte de cámara, ni
  `odSwimmingPlayer`, ni el drive-by, ni `IsWeaponType` en `GiveWeapon` (el mod usa
  `weaponType < WEAPONTYPE_TOTALWEAPONS`). Todo lo nuestro se conserva; los
  bloques `CLIMBING` suyos se **suman**.
- `ProcessControl` con `#ifdef CLIMBING` al principio: meter el bloque como hace
  el mod (antes de la colisión) respetando que no haya control-flow nuevo dentro
  de `LoadAllRequestedModels` (trampa Asyncify conocida — aquí no toca ese sitio).
- `PED_CLIMBING` va en el enum **antes de `PED_STATES_NO_ST`** (no al final).

---

## 2. `sa-crouch-movement_.../` — agachado SA + la tabla de anims completa

Contenido: `anim/ped.ifp` (240 clips), `scripts/VC.CustomAnimsData.asi` (carga
grupos de anim extra), `scripts/VC.CustomAnimsData.dat` (616 l. legibles),
`cleo/CrouchMovement(forClassicAxis).cs` (bytecode, sólo string `ROLL`).

### 2.1 `VC.CustomAnimsData.dat` — referencia de grupos y flags

Formato: `%GroupName BlockName InitModelIndex GroupAnimCount` y luego
`nombreClip id flags` (GroupAnimCount líneas, ids consecutivos). Es la **tabla
completa de grupos de VC reescrita en texto**: `man/ped` (179 anims, ids 0-178),
`van`, `coach`, `bikes/bikev/bikeh/biked` (18), `unarmed/screwdrv/knife/baseball/
golfclub/chainsaw/python/colt45/shotgun/buddy/tec/uzi/rifle/m60/sniper/grenade/
flame`, `medic/sunbathe/playidles/riot/strip/lance`, los grupos de marcha
(`player`, `playerrocket`, `player1armed`, `player2armed`, `playerBBBat`,
`playercsaw`, `shuffle`, `oldman`, `gang1/2`, `fatman`, `oldfatman`, `jogger`,
`woman`, `shopping`, `busywoman`, `sexywoman`, `fatwoman`, `oldwoman`, `jogwoman`,
`panicchunky`, `skate`) y los de **strafe** (`playerback`, `playerleft`,
`playerright`, `rocketback/left/right`, `csawback/left/right`).

Los IDs nuevos del mod al final de `man/ped` (patrón R20 nuestro: consecutivos
desde `firstAnimId`):

| id | clip | flags | uso |
|---|---|---|---|
| 173 | `GunCrouchFwd` | 88 | andar agachado **apuntando** hacia delante |
| 174 | `GunCrouchBwd` | 88 | ídem hacia atrás |
| 175 | `Crouch_Roll_L` | 212 | desplazarse agachado de lado (izq.) |
| 176 | `Crouch_Roll_R` | 212 | ídem (der.) |
| 177 | `GunMove_L` | 212 | strafe de pie apuntando (izq.) |
| 178 | `GunMove_R` | 212 | strafe de pie apuntando (der.) |

Los flags numéricos del `.dat` casan con las máscaras de `AnimAssocDesc` que
usamos (212 para los clips con traslación de movimiento, 2 = idle, 24/88 =
parciales de arma). Sirve como **tabla cruzada** para validar nuestros descs.

### 2.2 Clips que nos faltan (comparado con nuestro `ped.ifp` de 272)

`GunCrouchFwd` y `GunCrouchBwd` **NO están** en nuestro `ped.ifp` (verificado con
`ifp_inspect.py`); `Crouch_Roll_L/R`, `Crouch_Idle/Forward/Backward`, `GunMove_*`
sí. Si se quiere el agachado apuntado completo (avanzar/retroceder con arma en
mano), estos 2 clips se pueden **extraer de su `ped.ifp`** (mismo formato ANPK) e
inyectar en el nuestro — o dejar el actual que emula la traslación con
`ViceExtCrouchMoveSpeed` (R22/R25).

---

## 3. `Classic AXIS/` y `1510564741_ca-1/` — piernas en 4 direcciones

ClassicAxisVC 1.6 (Plugin SDK): cámara libre con ratón, andar en 4 direcciones,
cámara movible en vehículo, **apuntado simple** (en vez de 1ª persona) con ciertas
armas. Su `ClassicAXIS.ini`:

```
bEnable=1  bMoveCameraOnVehicle=1  bEnableFirstPersonMode=1
bIVOnFootCamera=0  bIVVehicleCamera=0      (offsets de cámara estilo IV)
bForceLegsMovements=0   ← requiere addon ClassicAXIS/anim/movements.img
bForceAutoAim=0  bForceManualAim=0
```

**`addon/ClassicAXIS/anim/movements.img` + `movements.dir`** (288 b, legible):
9 IFPs — `baseball, buddy, chainsaw, flame, grenade, m60, python, rifle, shotgun`.
⚠️ CORREGIDO en `05-cleos-adaptador-y-paridad-scm.md` §5 (verificado con
`tools/ifp_inspect.py`): NO son clips de piernas (`walk_left/right/back` están en
`ped.ifp`, mapeados por el `VC.CustomAnimsData.dat` §2.1). Son **34 clips de arma
extendidos**: 8 de fuego/recarga AGACHADO (`*_crouchfire/*_crouchload`, bloque C7),
`buddy_fireRELOAD`, y clips de arma con traslación de raíz para
`bForceLegsMovements` (torso con arma + piernas libres). Inventario exacto en el `05`.

---

## 4. `1498977446_107784/` — nado CLEO (poco que extraer)

`CLEO/swim.cs` (2.288 b, bytecode; vars `SWIM`, `PLAYER_CHAR`…) + `anim/ped.ifp`
(236 clips). Sus clips `Swim_Breast/Crawl/Tread/jumpout` son los mismos que
nuestro bloque C4 (ya implementado, E3). Único material nuevo: los clips
`IDLE_Player` y `turn` (menores). **Nada imprescindible aquí.**

## 5. `1487678468_firstperson/` y `SACarCam.asi` — binarios sueltos

`FirstPerson.asi` (GeniusZ; V para entrar/salir, Alt+B para fijar; readme en
cirílico cp1251 ilegible en UTF-8). `SACarCam.asi` sin más. Ambas mecánicas ya
están en nuestro port (`VICEEXT_FIRST_PERSON` C1 y `Process_FollowCar_SA`). Sólo
útiles como comparación visual en PC si se quiere.

---

## 6. `silent patch/` — INI con listas de datos listas para usar

Readme = catálogo de ~90 fixes (ya analizado en `.agents/plans/mods/01-silentpatch-vc.md`
desde su repo fuente). El **INI** aporta datos que no están en el plan:

### 6.1 `[DrawBackfaces]` — ~280 modelos de mapa
Lista exacta (mobile `DrawBackfaces.txt` portada): modelos que se pintan con
backface culling DESACTIVADO (`4x4racecotop2`, `ammu_windows1`, `ap_*`,
`basketballcourt05`, `BillBd1-4`, `bnk_*`, `bodyarmour`, `bouy`, … hasta
`wshxrefhse2`). Directamente usable en nuestro renderer (o el mecanismo de
librw `geometry->flags` / material). `[DontDrawBackfaces]` (vacío) es la lista
inversa para skins de mods.

### 6.2 `[ExtraCompSpecularityExceptions]`
`stallion`, `mesa` — forzar NO especular en sus extras (techos de cuero); con
env-maps en extras (ver §7) sin esto salen todos metálicos.

### 6.3 Fixes de coronas de sirena (placements)
`EnableVehicleCoronaFixes=1`: posiciones de sirena corregidas en Police, Enforcer,
Firetruck, Ambulance, FBI Rancher, Vice Cheetah; **añade** sirena al FBI
Washington; arregla la luz del Taxi; y la linterna de búsqueda + luz trasera del
Police Maverick. Complementa nuestro `VICEEXT_POLICE_BIKE_LIGHTS` y la §2.12
(luces de servicio). Los offsets concretos están en el `.asi` (minables) o en el
repo público de SilentPatch (ya referenciado en el plan 01).

### 6.4 Toggles de HUD/HINTS (espec para §2 pendientes)
- `MinimalHUD=0` (feature inacabada del juego: salud/arma/dinero se difuminan).
- `SlidingMissionTitleText=0` / `SlidingOddJobText=0` (textos deslizantes beta).
- `ShowPropertyBlips=0` (blips de propiedades comprables en radar/mapa).
- `ScaleScriptSprites=1`, `DontShrinkRadardisc=0`, `SpeechDelayTimer=0` (ms entre
  comentarios de NPC; −1 = 6000 ms — útil para "peds más habladoras").
- `Units=-1` (métrico/imperial por locale).

---

## 7. `skygfx/` — EL material del "Detalle de vehículo" (§2.10 NeoVehicleShininess)

SkyGfx trae el pipeline "neo" (Xbox) de vehículos. Vice Extended v3.0 integra
literalmente SkyGFX ("YCbCrCorrection from SkyGFX" en su ChangesEN), y su
`NeoVehicleShininess` ES este mecanismo.

### 7.1 `neo/carTweakingTable.dat` (104 l.) y `neo/rimTweakingTable.dat` (130 l.)
Tablas **24 horas × 7 climas** (SUNNY, CLOUDY, RAINY, FOGGY, EXTRASUNNY,
HURRICANE, EXTRACOLOURS), idénticas en `skygfx/SkyGfx/neo/` y
`extended/GameFiles/ViceExtended/neo/` (mismo contenido; verificar md5 al copiar):
- **Fresnel RO**: 0.4 en todo (24×7).
- **Specular Power** (más alto = brillo más ceñido): 25 sunny / 15 cloudy /
  70 rainy / 10 foggy / 15 extrasunny / 15 hurricane / 18 extracolours.
- **Diffuse Colour Modifier** (R,G,B,Amount): 0,0,0,0 en todo.
- **Specular Colour** (R,G,B,–): 178,178,178,100 (sunny), 140,140,150,50 (cloudy),
  220,220,220,75 (rainy), 255,255,255,20 (foggy)…

**Extraíble directo**: el parser de estas tablas + el mapeo hora×clima →
parámetros de especular del vehículo. Es dato, no código: se implementa en
nuestro renderer GL3 sin adivinar nada.

### 7.2 Semántica del `skygfx.ini` (la spec del comportamiento)
```
replaceDefaultPipeline=1  fix de vertex colors transparentes (objetos)
texblendSwitch=0          0=PS2 1=PC 2=Mobile (mezcla del env-map MatFX)
texgenSwitch=0            0=PS2 1=PC 2=Mobile (generación de texcoords env)
ps2light=1  ps2Water=1  dualPass=1  disableBackfaceCulling=0
neoRimLightPipe=0         rim light en peds (Xbox)
neoGlossPipe=0            gloss en carreteras (Xbox)
neoWaterDrops=1           gotas de agua en pantalla (0-3)
envMapSize=128            tamaño del mapa de reflexión
YCbCrCorrection (lumaScale 0.8588, lumaOffset 0.0627, Cb/Cr Scale 1.22)
carPipe / worldPipe / trailsSwitch / radiosity (VCS)...
```
Nota: fijos en SkyGfx VC (no conmutables): blend del auto-aim syphon y punto del
sniper arreglados, y **sniper trails habilitados**.

---

## 8. `widescreen/` y `framerate/` — constantes (complementan planes 03 y 02)

`GTAVC.WidescreenFix.ini`: `HudWidthScale/HudHeightScale=0.8`,
`RadarWidthScale=0.9`, `SubtitlesScale=0.8`, `FixVehicleLights=1` (**las luces de
vehículo son demasiado grandes en pantalla** — conecta con §2.11/2.12),
`RestoreCutsceneFOV=1`, `CarSpeedDependantFOV=20.0` (menor = más FOV),
`HideAABug=1|2`, `SmartCutsceneBorders=1`, `IVRadarScaling=1`,
`ReplaceTextShadowWithOutline=0|1|2`, `SmallerTextShadows=1`,
`ForceMultisamplingLevel`, `AllowAltTabbingWithoutPausing=1`.
`FramerateVigilante.ini`: `FPSlimit=60` (único ajuste; el fix real es interno).

---

## 9. `ginput/` — el mando, mapeados por completo

`GInputAPI.h` (428 l., para modders) documenta el estado del pad (botones,
analógicos, vibración, cheats desde el mando). `GAME CONTROLS FULL LIST.txt`:
**5 setups completos** (Setup 1 clásico PS2 … Setup 5 estilo GTA IV). Resumen del
Setup 1 (referencia para nuestro web/Gamepad API):
- A pie: stick izq. = movimiento/mira, stick der. = cámara, B dispara, A
  sprint/zoom−, X salto/zoom+, Y entrar, LB teléfono/centrar/recoger, R1 apuntar,
  L2/R2 arma ant./sig., **L3 = agacharse**, R3 = mirar atrás.
- En vehículo: A acelerar, X frenar, Y salir, LB emisora (mantener = MP3),
  R1 freno de mano, **L2/L2 = mirar a los lados** (L2+R2 = atrás), L3 bocina.

Opciones del INI útiles: `SAStyleSniperZoom=1` (R2 zoom in / L2 zoom out),
`ControlsSet=1..5`, `Southpaw`, `LeftStickDeadzone=24` / `RightStickDeadzone=27`,
`LeftStickSensitivity=100` (0 = cúbica … 200 = raíz cúbica),
`FaceButtonsSensitivity=50` (umbral de pulsación del botón),
`DrivebyWithAnalog`, `HotkeyToDriveby`, `SwapSticksDuringAiming`,
`ApplyGXTFixes` (los hints muestran el botón correcto — idea para §2.3/HINT_KEYS).
Conexión: el "Modern controls" de Extended (sensibilidad separada de mira y
apuntado, dead zone, vibración, auto-aim) se puede replicar con estos parámetros.

---

## 10. `extended/` — Vice Extended v2510 (el checklist de lo que falta)

### 10.1 `ChangesEN.txt` = la lista oficial de features (1.0 → 2510)
Sirve como **checklist de paridad**. Cruce con nuestro `.agents/plans/vice-extended-pendientes.md` §2 (X-list):

| Pendiente (X) | Entrada del ChangesEN | ¿Referencia en mods/? |
|---|---|---|
| X2.1 iconos de emisora | 2.6 "Radio station icons when switching radio stations" | sólo binario (`ViceEx.exe` / `.dat`) |
| X2.2 sonido del agua nadando | 2510 "Changed the sound of water while swimming" | audio en `GameFiles/ViceExtended/audio/` |
| X2.3 teclas con Shift en hints | 2510 "Fixed incorrect display of keys in game tips…" | idea = `ApplyGXTFixes` de ginput |
| X2.4 crash Micro-UZI | 2510 "Fixed a crash … added Micro-UZI" | minar `ViceEx.exe` |
| X2.5 triángulo de salud | 3.0 "Triangle above the ped's head, showing health" | binario |
| X2.6 muertes en coche | 3.0 "in-car death animations and random behavior for dead drivers" | clips `CAR_die_*_DB` en su ped.ifp (¡ya en el nuestro!) |
| X2.7 variaciones de peds | 2.6 "Ped variations" | datos de peds |
| X2.8 ALT caminar/sentarse | 2.0 "Walking on ALT for the keyboard" | trivial (input) |
| X2.9 sensibilidad de apuntado | 2506 "Aim sensitivity fix" | parámetros ginput §9 |
| X2.10 detalle de vehículo | 3.0 NeoVehicleShininess | **§7 tablas neo listas** |
| X2.11-12 luces de coche/servicio | 2.5 "texture of car lights", "lights can break", 3.0 "Service lights for service cars" | `AdaptingVehiclesEN.txt` (dummies) + coronas SilentPatch §6.3 |
| X2.15 paneles Ammu-Nation | 2510 "Boards at Ammu-Nation and tool stores" | objetos nuevos en `object.dat` |
| X2.16 night vertex colors | 2506 "Night vertex colors support for d3d9" | binario; spec = render d3d9 |
| X2.17 radio de aparición | 1.0 "Increased spawn radius of vehicles and pedestrians" | parámetros en su código (minables) |
| X2.18 luna móvil | 1.5 "Moving Moon. Its position depends on the position of the Sun" | trivial astronómico |

Otras features de su changelog NO contempladas aún y de bajo coste con lo que
tenemos: "Steer angle random when spawning a parked car" (2506), "in-car death
animations" (clips ya servidos), "cops can crouch first and then go to cover"
(2.6), "A projectile will explode if you shoot it" (2.6), "gas station explodes"
(2.6), "switch weapons while holding the detonator" (2.6), "open vehicle doors can
slam" (2.5), "wheel can fall off with a hard impact" (2.0), "front wheels keep
rotation after exiting" (2.0).

### 10.2 Datos legibles
- `features.ini` (18 toggles) — los 18 ya transcritos en el plan de inclusión.
  Valores del mod: `PlayerDoesntBounceAwayFromMovingCar=1`, `RecoilWhenFiring=1`,
  `EnableSwimming=1`, `RocketLauncherThirdPersonAiming=1`,
  `RandomVehicleModsInTraffic=1`, `EnableDistantLights=1`, `EnableClimbing=0`
  (¡ellos lo traen APAGADO por defecto! Nosotros lo activamos a petición),
  el resto en 0.
- `limits.ini` — pools: `NUMVEHICLES=130`, `NUMPEDS=140`, `NUMOBJECTS=460`,
  `NUMDUMMIES=2340`, `NUMCOLMODELS=4400`, `MAXWHEELMODELS=12` → ya reflejados en
  nuestro `config.h` (6700/1500/48/47…).
- `weapon.dat` (102 l., 28 columnas; la 27 = weapon sight → D6 hecho).
- `object.dat` (432 l.) — comportamiento de objetos (masas, colisiones…): base
  para X2.15 y los objetos rompibles/luces.
- `AddingVehicles/Sounds.txt` — **formato de sonidos de vehículos**:
  `ModelName, AccelerationSampleIndex, Bank, HornSample, HornFrequency,
  SirenOrAlarmSample, SirenOrAlarmFrequency, DoorType` (+ tabla de barcos:
  `Volume, fVolumeModificator, Frequency, fFrequencyModificator, EngineType`).
  Necesario para que los 8 vehículos nuevos (6500-6507) suenen.
- `AddingVehicles/MVLConverter/output.txt` (627 l.) — salida IDE de referencia
  para validar `tools/import_mvl_vehicles.py`.
- `AdaptingVehiclesEN.txt` — dummies de luces de servicio (ya usado en C3.x).
- `Bonus/Black PC buttons/pcbtns.txd` — iconos de tecla negros (alternativa al
  `pcbtns.txd` de D7/HINT_KEYS).

### 10.3 `SourceCode/MVLConverter/MVLConverter.cpp` (321 l.)
Único fuente propio de Extended: XML de vehículo MVL → líneas IDE. Ya replicado
en `tools/import_mvl_vehicles.py`; útil como oráculo ante dudas de formato
(comparar su salida con `output.txt`).

---

## 11. Descubrimiento adicional (fuera de `mods/`)

`vice-extended-october-2025-update_1788102643_908113/` en la raíz del repo es
**otra copia de Extended v2510** (mismo `ChangesEN.txt`, `ped.ifp` con md5
idéntico al nuestro y al de `mods/extended/`: `c61a251f…`). Confirmación: el
`ped.ifp` que ya servimos (272 clips) es EXACTAMENTE el de la última versión del
mod — no habrá sorpresas de clips.

**Comparativa de `ped.ifp`s** (con `tools/ifp_inspect.py`):
| IFP | clips | Clips suyos que nos faltan | Clips nuestros que a él le faltan |
|---|---|---|---|
| nuestro (Extended servido) | 272 | — | — |
| `climbing/ANIM/ped.ifp` | 241 | ninguno | crouch, swim, DB, GunMove… |
| `1498977446/anim/ped.ifp` | 236 | `IDLE_Player`, `turn` | CLIMB, crouch, swim… |
| `sa-crouch/anim/ped.ifp` | 240 | **`GunCrouchFwd`, `GunCrouchBwd`** | CLIMB, swim, DB… |
| `extended/.../anim/ped.ifp` | 272 | ninguno (idéntico) | — |

---

## 12. Prioridades — orden sugerido para avanzar más rápido

1. **Escalada completa (E1 → E1+)** copiando los bloques `#ifdef CLIMBING` de
   §1.3: vallas (`CLIMB_jump_B` + `HAS_TRANSLATION`), subida alta con
   colgada+pull, NPCs trepando, cancelación por tecla, INI `[Climbing]`. Es
   implementación de referencia probada (el mod se compila sobre miami) y
   materializa `EnableClimbing`/"Land climbs" de Extended. Pasos: flags de
   `aClimbAnimDescs` (tabla §1.1) → bloques de Ped.cpp/Ped.h → ganchos →
   PedFight → INI. Traza ODTRACE (`CLIMB1`…) + criterio PASS como E1.
2. **Piernas/strafe a ojo (fin del agachado R26+)** con el material de §2-3:
   `GunMove_L/R` (ya en nuestro ifp) para de pie, `Crouch_Roll_L/R` (ya hecho),
   y opcionalmente extraer `GunCrouchFwd/Bwd` del ifp de sa-crouch para el
   agachado apuntado; `movements.img` de ClassicAXIS como referencia de poses.
3. **Detalle de vehículo §2.10**: parser de `neo/carTweakingTable.dat` +
   `rimTweakingTable.dat` (§7.1) + spec `skygfx.ini` (§7.2) → especular por
   hora×clima en el pipeline GL3.
4. **Luces §2.11/2.12**: placements de sirena (§6.3) sobre nuestro
   `VICEEXT_POLICE_BIKE_LIGHTS` + dummies de `AdaptingVehiclesEN.txt`.
5. **HUD/hints §2 (blips, textos)**: toggles de §6.4 (MinimalHUD, Sliding*,
   ShowPropertyBlips, ScaleScriptSprites, SpeechDelayTimer).
6. **Mando web**: setups de §9 (Gamepad API + sensibilidades/deadzones).
7. **X-list restante** (agua, emisora, triángulo, ALT, luna, spawn radius,
   night vertex colors…): spec = §10.1; `ViceEx.exe` se mina con
   `tools/viceex-strings.py` cuando haga falta confirmar un valor.

**Regla de oro del encargo**: nada se crea de cero si el mod ya lo resuelve —
copiar el bloque del fuente (`climbing/`) o adaptar su dato/INI (resto) y marcar
la adaptación con la misma disciplina de defines `VICEEXT_*` al final de
`config.h` sin reordenar.

---

## 13. Resumen ejecutivo (mod → mecánica → estado → acción)

| Mod | Mecánica | Estado en nuestro port | Acción |
|---|---|---|---|
| climbing (fuente) | escalada SA (vallas, alta, NPCs) | E1 parcial | **copiar bloques** §1.3 |
| sa-crouch-movement | agachado SA + tabla anims | C5/R20-R26 hecho | extraer `GunCrouchFwd/Bwd` + flags |
| Classic AXIS | piernas 4 dir., apuntado simple | parcial (R26d) | usar `movements.img` §3 |
| swim CLEO | nado con anims | E3 hecho | nada |
| FirstPerson.asi | 1ª persona | C1 hecho | nada |
| SACarCam.asi | cámara coche SA | hecho | nada |
| SilentPatch | fixes + DrawBackfaces + sirenas | plan 01 activo | listas §6 |
| SkyGfx | NeoVehicleShininess, PS2 pipes | pendiente §2.10 | tablas §7 |
| WidescreenFix | HUD/FOV | plan 03 | constantes §8 |
| FramerateVigilante | FPS limit | plan 02 | trivial |
| GInput | mando | pendiente | setups §9 |
| Extended v2510 | todo lo demás | ~50% | checklist §10.1 |
