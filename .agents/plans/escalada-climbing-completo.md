---
name: escalada-climbing-completo
status: ENRICHED
type: feature
domain: gameplay-climb
owner_rules: .agents
created: 2026-09-28 00:25
updated: 2026-09-28 (enriquecido: código del mod leído línea a línea, anclas verificadas en el árbol, decisiones E-1..E-4 resueltas por el jugador)
---

# Plan: Escalada completa (`climbing`) — port del mod con fuente

> Origen: petición del jugador (28/09): *«podrías investigar y armar el plan
> completo para portear el de climbing? buscar fuentes código del mod,
> compatibilidad, déjalo todo en el plan»*. Segundo pase (28/09): *«las
> decisiones vamos con tus recomendaciones, tómate tu tiempo y enriquece el
> plan»* → E-1…E-4 quedan **resueltas** (§11) y se añaden D7/D8, el anexo de
> código literal (§12) y los hechos verificados (§9).
>
> Es el **ítem 1** de `mods/04` §12 y de `13-inventario-mods-fuente.md` §13 (la
> prioridad más alta de lo que queda por portear, tras recoil/nado/agachado/
> apuntado ya `EXECUTED`).
>
> Estado: **ENRICHED**. Investigación hecha; nada de código tocado. Pendiente el
> gate de ejecución.

## 0. Objetivo

Portar la mecánica completa de escalada del mod **Climbing [reVC]** —jugador y
NPCs: subida baja, subida alta con colgada y tirón, y salto de **valla**— a
nuestro port, sustituyendo el bloque propio E1 (`ViceExtClimbControl`), según la
regla permanente del jugador (`12-handoff-tanda2` §14, ADR-003): **el mod manda;
se reescribe desde su spec, no se parchea lo existente**.

**Criterio PASS** (definido *antes* de que el jugador juegue):

1. Trepar una **valla fina** (clip `CLIMB_jump_B`) y una **valla gruesa**.
2. Una **subida alta** con colgada (`CLIMB_idle`) + **tirón** (`CLIMB_Pull`).
3. Ver a un **NPC** trepar.
4. Cancelar a media subida con triángulo.
5. **Sin regresión** de salto, agachado, nado, apuntado, cámara y disparo.

## 1. La fuente (código real, no spec)

`mods/climbing/` — **único mod de `mods/` con el fuente reVC completo**:

| Fichero del mod | Tamaño | Qué trae (bajo `#ifdef CLIMBING`) |
|---|---|---|
| `source_code/src/peds/Ped.cpp` | 10.164 l. | Estáticos `:57-66`; las 8 funciones de detección + `StartClimbing`/`EndClimbing` `:68-360`; bloque de `ProcessControl` `:2057-2128`; `ProcessEntityCollision` `:3278-3282`; `PedLandCB` `:4356-4358`; `InTheAir` `:5720-5726`; `CanPedJumpThis` `:9559-9562`; `SetJump` `:9577-9585`; `FinishLaunchCB` `:9595-9600`, `:9610-9624`, `:9665-9671`; los 4 callbacks `:9761-9815` |
| `source_code/src/peds/Ped.h` | 1.255 l. | `PED_CLIMBING` `:331-332`; estáticos+estado `:656-689`; 12 métodos `:700-716`; 4 callbacks `:951-955` |
| `source_code/src/peds/PedFight.cpp` | 4.207 l. | 1 condición (`:374-377`) |
| `source_code/src/animation/AnimationId.h` | 295 l. | 6 ids `:202-209`, **antes** de `ANIM_STD_NUM` |
| `source_code/src/animation/AnimManager.cpp` | 1.470 l. | 6 descs de flags `:196-202` (en `aStdAnimDescs`) |
| `source_code/src/core/re3.cpp` | 1.331 l. | `#include "Ped.h"` `:54-56`; lectura INI `:560-568`; escritura `:679-687` |
| `source_code/src/core/config.h` | 503 l. | `#define CLIMBING` (`:504`) |
| `game_folder/ANIM/ped.ifp` | — | 241 clips (los `CLIMB_*` **ya están** en el nuestro de 274) |
| `ReadMe.txt` | — | Ruso: instala el `source_code` **en la rama `miami`** (nuestra base); INI `[Climbing]` de `reVC.ini`; config Release win-x86-librw-d3d9-mss |

El ReadMe confirma que los 7 ficheros son **diffs aplicables tal cual** a nuestra
rama (mismo origen reVC). Todo lo de §5 está leído del código, no de un plan
intermedio.

## 2. Qué hay hoy en el árbol (E1) — inventario verificado

- **`core/config.h:417`** `#define VICEEXT_CLIMB` (existe ya).
- **`animation/AnimationId.h:300-306`**: 7 ids `ANIM_STD_CLIMB_{IDLE,JUMP,JUMP_B,
  JUMP2FALL,PULL,STAND,STAND_FINISH}` — **después** de `ANIM_STD_NUM` (`:202`).
- **`animation/AnimManager.cpp:1027-1043`**: arrays `aClimbAnimations` (`:1028-1034`)
  / `aClimbAnimDescs` (`:1037-1043`) + grupo propio **`"playerclimb"`**
  (`:1119`, `ASSOCGRP_PLAYERCLIMB` en `AnimManager.h:85`).
- **`peds/PlayerPed.cpp:2940-3109`**: bloque propio **E1** completo —
  `ViceExtClimbAbort()` (`:2967`), `ViceExtClimbLedgeHeight()` (`:2980`),
  `CPlayerPed::ViceExtClimbControl()` (`:3018`); estáticos `odClimbActive/
  odClimbStart/odClimbDur/odClimbFrom/odClimbTo` (`:2962-2976`); 5 `#define`
  (`:2956-2960`); declaración en `PlayerPed.h:123`.
- **Llamadas de E1**: `CPlayerPed::ProcessControl` (`:2142`, tras
  `#ifdef VICEEXT_CLIMB` en `:2139`), forward (`:2473`, `#ifdef` en `:2470`) y
  aborto desde el nado (`:2820`, `#ifdef` en `:2819`).
- **Trazas E1**: `VICEEXT climb start borde=%.2f ms=%u` (`:3103`),
  `VICEEXT climb fin` (`:3050`), `VICEEXT climb no: destino en agua` (`:3084`),
  por `ODTRACES`/`snprintf` (patrón a copiar en §8).
- **Sin** marcas en `check-served-build.sh` ni bloque en `viceext-log-check.py`.
- **Cero** de la lógica del mod: `Ped.cpp`, `Ped.h`, `PedFight.cpp` y `re3.cpp`
  tienen **0** ocurrencias de `bIsClimbing` / `CanPedClimbingThis` / `PED_CLIMBING`
  / `IsNeedFixFenceSideNormal`.

**Conclusión**: lo que había no es "el mod a medias" — es una mecánica propia
(player-only, un solo tipo de borde, interpolación por tiempo) que **se reescribe**
por el sistema del mod.

## 3. Compatibilidad (medida contra nuestro árbol, fichero y línea)

Todas las funciones/anchas del mod **existen** en nuestro árbol, con firma
idéntica o compatible. Verificado por grep directo (no por parecido):

| Necesidad del mod (llamada literal) | En nuestro árbol | Estado |
|---|---|---|
| `CWorld::ProcessLineOfSight(p1, p2, col, ent, b, v, p, o, d, seeThru, …)` (10 args) | `core/World.h:91` (11 args, 2 con default) | ✅ idéntica |
| `CWorld::TestSphereAgainstWorld(centre, r, ignore, b,v,p,o,d, ignoreSome)` (9 args) | `core/World.h:101` (9 args) | ✅ idéntica |
| `CGame::IsInInterior()` | `core/Game.h:79` (inline) | ✅ |
| `CTimer::GetTimeStepInSeconds()` | `core/Timer.h:24` → `ms_fTimeStep / 50.0f` (**segundos reales**) | ✅ |
| `CAnimManager::AddAnimation(clump, group, animId)` | `animation/AnimManager.cpp:1300` | ✅ |
| `CAnimManager::BlendAnimation(clump, group, animId, delta)` | `animation/AnimManager.cpp:1340` | ✅ |
| `assoc->SetFinishCallback(cb, arg)` / `assoc->blendDelta` | `animation/AnimBlendAssociation.h:44-45` | ✅ |
| `CPad::ExitVehicleJustDown()` / `JumpJustDown()` / `Clear(false)` | `core/Pad.h:238` / `:252` / `:194` | ✅ |
| `CPed::SetPedState(PED_CLIMBING)` | `peds/Ped.h:981` (inline; **no** tiene `switch`) | ✅ no rompe |
| `CPed::SetStoredState()` / `RestorePreviousState()` | `peds/Ped.cpp:777` / `:795` (`Ped.h:749`/`:691`) | ✅ |
| `CAnimBlendAssociation::flags` (`ASSOC_DELETEFADEDOUT`, `ASSOC_FADEOUTWHENDONE`, `ASSOC_PARTIAL`, `ASSOC_REPEAT`, `ASSOC_HAS_TRANSLATION`) | `animation/AnimBlendAssociation.h:7-20` | ✅ los 5 existen |
| Clips `CLIMB_*` en el `ped.ifp` servido | `streamed/anim/ped.ifp` (274 clips) | ✅ **los 7** |
| `CVector`, `CColPoint`, `SetPosition/GetPosition/GetForward/SetHeading`, `DotProduct`, `Distance` | base | ✅ |
| `m_vecMoveSpeed`, `bIsInTheAir`, `bIsStanding`, `bAffectedByGravity`, `bUsesCollision`, `m_fRotationCur/Dest` | `peds/Ped.h` | ✅ |
| `ODTRACES` / `ODTRACEI` | `skel/ondemand.h:8-9`, ya incluido en `peds/Ped.cpp:10` | ✅ |

### 3.1 Tres hallazgos que cambian el plan

1. **Cadena de ejecución.** `CPlayerPed::ProcessControl` llama a
   `CPed::ProcessControl()` (`PlayerPed.cpp:4840`), así que el bloque de escalada
   del mod (que va **al principio** de `CPed::ProcessControl`) corre **también
   para el jugador**. No hay que tocar `PlayerPed.cpp` para la mecánica; solo
   retirar E1 (T7).
2. **D1=(a) no es cosmética, es obligatoria.** `CAnimBlendAssocGroup::GetAnimation(id)`
   devuelve `&assocList[id - firstAnimId]` **sin comprobar rango**
   (`animation/AnimBlendAssocGroup.cpp:47-50`), y `CreateAnimAssocGroups` fija
   `firstAnimId = def->animDescs[0].animId` (`AnimManager.cpp:1411`). Si se
   copiase el mod literal (`AddAnimation(clump, ASSOCGRP_STD, ANIM_STD_CLIMB_*)`)
   con nuestros ids 300-306, el índice sería `300 - firstAnimId(STD)` → **lectura
   fuera de rango**. Hay que usar `ASSOCGRP_PLAYERCLIMB` + nuestros ids.
3. **`bIsClimbingJumpB` es un bug latente del mod** (ver D7 en §4 y §9.2).

### 3.2 Dato de build

**No hay cambio de datos** → `dataTag` (`web/ondemand.js:130`) **no sube**.
`VERSION` (`web/lib/index.js:171`) sube en cada build (ritual de casa).

## 4. Decisiones

| # | Diferencia | Decisión | Motivo |
|---|---|---|---|
| **D1** | **Modelo de animación**: el mod mete los clips en `ASSOCGRP_STD` (ids antes de `ANIM_STD_NUM`); nosotros en el grupo `playerclimb` (ids después). | **(a) adaptar** a `ASSOCGRP_PLAYERCLIMB` + nuestros ids, **sin tocar la tabla STD** | Ver §3.1.2 (lo literal = lectura OOB) y coherencia con `playercrouch`/`playerswim` |
| **D2** | **E1 propio** (`ViceExtClimbControl`). | **(a) retirarlo** y quedarse con el sistema del mod | ADR-003 |
| **D3** | `FinishLaunchCB` **bajo `#ifdef` sustituye** el chequeo vanilla de obstáculo (`ANIM_STD_HIT_WALL`). | **(a) fiel al mod** | Efecto real: tras un salto contra un muro el ped **reintenta escalar** en vez de hacer el gesto de choque. Es lo que vende el mod |
| **D4** | `bClimbingPeds = true` (NPCs trepan). | **sí** | Default del mod; el ReadMe lo vende como característica |
| **D5** | `bClimbingInInteriors = false`, `bClimbingOnVehicles = false`. | **sí** (defaults) | Defaults del mod |
| **D6** | **Cancelar** = triángulo (`ExitVehicleJustDown`), solo jugador y solo si `!bIsClimbingHighJump`. | **sí** (verbatim) | Spec del mod |
| **D7** | **Bug latente del mod**: `bIsClimbingJumpB` no se reinicia cuando la detección toma la rama de **repisa** (`CheckPotentialClimbingPlaceFind`), solo en `CheckClimbingTheFence` (rama de fallo) y en `EndClimbing`. Tras *una* valla fina, una subida normal se trataría como valla (posiciones y `ANIM_STD_CLIMB_B` equivocados). | **Desviación mínima**: reiniciar `bIsClimbingJumpB = false` (y `bIsClimbingHighJump = false`) **al entrar** en `CanPedClimbingThis` | Es un **bug**, no una decisión de diseño: el mod no puede querer que una repisa use el camino de valla. No cambia la física ni los valores, solo deja el estado limpio. Se documenta aquí para poder revertirlo a literal si el jugador lo prefiere |
| **D8** | **Coste**: `CheckPotentialClimbingTheFencePlaceFind` hace hasta **40 iteraciones** con 2 rayos + 2 esferas cada una (~160 consultas al mundo) y `CanPedClimbingThis` se re-evalúa **cada frame** mientras `bIsReadyToClimbing`. | **Medir primero** con la traza de §8; si aparece coste, capar la ventana (no el bucle) | El bucle sale en el primer hueco, así que el caso típico es corto; el peor caso es caer lejos de todo. No tocar antes de tener dato |

## 5. Mecánica completa del mod (leída del código)

### 5.1 Constantes (`Ped.cpp:57-66`, estáticos de `CPed`)

```
bClimbingInInteriors = false      maxPossibleClimbingHeight   = 2.25f
bClimbingOnVehicles  = false      maxPossibleCheckHeightForPeds = 2.85f
bClimbingPeds        = true       maxHighClimbingHeight       = 1.5f
                                  highClimbingOffsetSpeed     = 3.0f
                                  playerVerticalVelocityAtWhichStartsToFall = -0.2f
```

> **Unidades (trampa)**: `highClimbingOffsetSpeed = 3.0f` y las velocidades de
> §5.5 se aplican como `SetPosition(pos + dir * GetTimeStepInSeconds() * speed)`
> → son **unidades-mundo/segundo** (÷1) y **se copian literales**.
> `playerVerticalVelocityAtWhichStartsToFall` se compara contra
> `m_vecMoveSpeed.z`, que está en **unidades-motor** (×50 = m/s); también literal.
> **No aplicar `METERS_PER_SECOND_TO_GAME_SPEED`** en ninguna de las dos.
> El único sitio que escribe `m_vecMoveSpeed` es `FinishLaunchCB` (ranura NPC,
> `0.15f`) y los reseteos a 0.

### 5.2 Estado (`Ped.h:656-689`)

Estáticos: los 8 de §5.1. De instancia: `bIsReadyToClimbing`, `bIsClimbing`,
`currentClimbingHeight`, `newClimbingPosition`, `correctedClimbingStandPosition`,
`currentClimbingStandAnim`, `bIsStartHighClimbing`, `bIsClimbingHighJump`,
`bIsClimbingIdle`, `bIsClimbingPull`, `newClimbingIdlePosition`,
`newStartClimbingIdlePosition`, `currentClimbingIdleAnim`, `bIsClimbingJumpB`,
`newStartClimbingJumpBPosition`, `newClimbingJumpBPosition`,
`currentClimbingJumpBAnim`, `currentJumpGlideAnim`.

> `bIsStartHighClimbing` y `newStartClimbingIdlePosition` /
> `newStartClimbingJumpBPosition` se declaran pero **el mod no los usa** como
> estado vivo (solo aparecen en el diff de `Ped.h`). Portarlos como campo
> declarado es lo más barato; si se prefiere, se omiten y se anota.

Los inicializadores del mod (`= false` / `= nullptr`) **se copian**: sin ellos,
un ped del pool recién creado arranca con basura.

### 5.3 Detección — 8 funciones, lógica paso a paso

**`CanPedClimbingThis(hitFwd, hitBwd, hitJumpB)` — el portero**
1. `if (!IsPlayer() && !bClimbingPeds) return false;`
2. `if (CGame::IsInInterior() && !bClimbingInInteriors) return false;`
3. **D7**: poner `bIsClimbingJumpB = false; bIsClimbingHighJump = false;`
4. `if (CheckObjectFrontPlayer(hitFwd) && !CheckObjectAbovePlayer(hitFwd))`
   * `if (CheckPotentialClimbingPlaceFind(hitFwd, hitBwd))`
     → `return CheckClimbingPlaceFree(hitBwd) && hitBwd.normal.z >= 0.8f;`
   * `return CheckClimbingTheFence(hitFwd, hitBwd, hitJumpB);`
5. si no → `return false;`

**`CheckObjectFrontPlayer(hitFwd)`** — 8 rayos, todos con
`ProcessLineOfSight(a, b, hitFwd, ent, true, bClimbingOnVehicles, false, true, false, false)`:
- origen `start = GetPosition() + (0,0,-0.25)`, `fwd = GetForward()`
- (1) `start → start + fwd` (1 m, a la altura de los pies)
- (2) `start+(0,0,1.45) → +fwd` · (3) `1.55` · (4) `1.65` (pecho/hombros)
- (5..8) desde `start+(0,0,3.0)` hacia `+fwd*0.25`, `*0.5`, `*0.75`, `*1.0`
- **en el camino de éxito se fuerza `hitFwd.point.z = start.z`** (machaca la `z`
  real del impacto por la de los pies). Es **load-bearing**: la usa `StartClimbing`
  para `currentClimbingHeight`.

**`CheckObjectAbovePlayer(hitFwd)`** — ¿hay techo? (el parámetro no se usa):
- 1 rayo vertical `GetPosition() → GetPosition()+(0,0,2.75)` con `CColPoint{}`
  temporal y `(true, false, false, true, false, false)`
- **5 esferas** `TestSphereAgainstWorld(GetPosition()+(0,0,z), 0.25f, this, true,true,true,true,false,false)`
  con `z = 1.0, 1.25, 1.5, 1.75, 2.0`
- devuelve `true` si **cualquiera** pega (hay techo → no se trepa).

**`CheckPotentialClimbingPlaceFind(hitFwd, hitBwd)`** — ¿hay repisa?
- `d = -hitFwd.normal * 0.25`
- `ProcessLineOfSight(hitFwd.point + d + (0,0,maxPossibleClimbingHeight),
  hitFwd.point + d, hitBwd, ent, true, bClimbingOnVehicles, false, true, false, false)`
- `hitFwd` **por valor** aquí (el mod lo pasa por copia): no propaga mutación.

**`CheckClimbingPlaceFree(hitBwd)`** — ¿cabe el ped de pie ahí?
- 4 esferas sobre `hitBwd.point`: `z=0.3 r=0.215`, `z=0.5 r=0.2`,
  `z=0.75 r=0.4`, `z=1.5 r=0.4`
- 1 esfera en `hitBwd.point + GetForward()*0.25 + (0,0,0.5)` con `r=0.1`
- devuelve `true` solo si **ninguna** pega.

**`CheckClimbingTheFence(hitFwd, hitBwd, hitJumpB)`** — rama de valla
1. `if (IsNeedFixFenceSideNormal(hitFwd)) hitFwd.normal = -hitFwd.normal;`
2. si `CheckPotentialClimbingTheFencePlaceFind(...)`:
   - `if (Distance(hitFwd.point, hitJumpB.point) < 0.25f)` → **valla fina**:
     `newClimbingJumpBPosition = hitFwd.point - hitFwd.normal * 5.0f;`
     `bIsClimbingJumpB = true; m_vecMoveSpeed = 0;` → `return true`
   - si no → `bIsClimbingHighJump = false; return false;`
3. si no → `bIsClimbingHighJump = false; bIsClimbingJumpB = false; return false;`

**`IsNeedFixFenceSideNormal(hitFwd)`** (nueva, no existe en nuestro árbol)
- `return DotProduct(hitFwd.normal, GetForward()) > 0.0f;`
- (Por copia en el mod; en el port puede ir por referencia `const&`.)

**`CheckPotentialClimbingTheFencePlaceFind(hitFwd, hitBwd, hitJumpB)`** — busca el canto
- `base = hitFwd.point.z`; `add = (0,0,0)`; `prevFound = false`
- bucle `i in [0,40)`:
  - `found = LOS(hitFwd.point - hitFwd.normal*2.0 + add, hitFwd.point + add, hitJumpB, …, true, false, false, true, false, false)`
  - si `!found`: **invertir** → `found = LOS(hitFwd.point + hitFwd.normal*2.0 + add, hitFwd.point + add, …)`
    (comentario del mod: *«Some collision normals of fences give the opposite surface normal»*)
  - `esf1 = TestSphere(hitFwd.point - hitFwd.normal + (0,0,0.25), 0.5f, this, true,true,true,true,true,false)`
  - `esf2 = TestSphere(hitFwd.point + (0,0,0.525), 0.5f, this, true,true,true,true,true,false)`
  - `add += (0,0,0.01)`
  - `if (!found && !esf1 && !esf2)`: si `IsClimbingHeightHigherThanHigh(hitJumpB.point.z, base)`
    → `bIsClimbingHighJump = true;` → `break`
  - `prevFound = found;` **`hitFwd.point.z = hitJumpB.point.z;`** (muta la `z` de
    `hitFwd` a la del canto → la usa `StartClimbing`)
  - si `IsClimbingHeightHigherThanPossible(hitJumpB.point.z, base)` → `prevFound = false; break`
- `return prevFound;`

**`IsClimbingHeightHigherThanPossible/HigherThanHigh`** (inline, `Ped.h:711-712`)
- `currentHeight - startHeight > maxPossibleClimbingHeight` (2.25) / `> maxHighClimbingHeight` (1.5)

> **Mutación por referencia (crítico)**: `hitFwd` se pasa por referencia a
> `CheckClimbingTheFence` y `CheckPotentialClimbingTheFencePlaceFind`, que
> modifican `normal` (invertida) y `point.z` (canto). `StartClimbing` depende de
> esos valores. **Mantener las firmas por referencia tal cual.**

### 5.4 `StartClimbing(hitFwd, hitBwd, hitJumpB)` (`Ped.cpp:257-321`)

1. si `currentJumpGlideAnim` → `blendDelta = -1000.0f`
2. `bIsInTheAir = false; bIsStanding = true;`
3. rumbo: `n = -hitFwd.normal; m_fRotationCur = m_fRotationDest = n.Heading(); SetHeading(m_fRotationCur);`
4. `bAffectedByGravity = 0; bUsesCollision = 0;`
5. `SetPedState(PED_CLIMBING); bIsClimbing = true; bIsReadyToClimbing = false;`
6. `newClimbingPosition = hitBwd.point + (0,0,1.0);`
7. `offset = 0.1f;` → `correctedClimbingStandPosition = (hitBwd.point + hitFwd.normal*0.6f) - (0,0,offset);`
8. `currentClimbingHeight = hitBwd.point.z - hitFwd.point.z;`
9. **3 ramas**:
   - **A. alta** (`height > maxHighClimbingHeight(1.5) && !bIsClimbingJumpB`):
     `bIsClimbingHighJump = true; offset = 2.0f;`
     `p = (hitBwd.point + hitFwd.normal*0.6f) - (0,0,2.0)`
     `newClimbingIdlePosition = p + (0,0,0.95f)`
     `AddAnimation(ASSOCGRP_STD, ANIM_STD_CLIMBING_JUMP_JUMP)` + `SetFinishCallback(FinishHighClimbingCB)`
   - **B. valla** (`bIsClimbingJumpB`):
     - **B1. valla alta** (`bIsClimbingHighJump`): `offset = 2.0f`;
       `p = (hitFwd.point + hitFwd.normal*0.325f) - (0,0,2.0)`;
       `newClimbingIdlePosition = p + (0,0,0.95f)`;
       `correctedClimbingStandPosition = hitFwd.point + hitFwd.normal*0.4f`;
       `AddAnimation(JUMP_JUMP)` + `FinishHighClimbingCB`
     - **B2. valla normal**: `correctedClimbingStandPosition = hitFwd.point + hitFwd.normal*0.3f`;
       `SetPosition(correctedClimbingStandPosition)`;
       `currentClimbingJumpBAnim = AddAnimation(JUMP_JUMP_B)` + `SetFinishCallback(FinishClimbingCB)`
   - **C. baja**: `SetPosition(correctedClimbingStandPosition + (0,0,0.1f))`;
     `currentClimbingStandAnim = AddAnimation(JUMP_STAND)` + `SetFinishCallback(FinishClimbingCB)`

> El mod usa `ANIM_STD_CLIMBING_JUMP_JUMP` para la **subida alta** (clip
> `CLIMB_jump`, 0.562 s) y `ANIM_STD_CLIMBING_JUMP_JUMP_B` para la valla
> (`CLIMB_jump_B`, 0.962 s, raíz 0.891 m). Mapeo de ids en §12.2.

### 5.5 `ProcessControl` (bloque al principio, `Ped.cpp:2057-2128`)

**a) Arranque por reintento**
```cpp
if (bIsReadyToClimbing) {
    CColPoint f, b, j;
    if (CanPedClimbingThis(f, b, j)) StartClimbing(f, b, j);
}
```
**b) Ya escalando** (`if (bIsClimbing)`):
1. **Cancelar** (jugador, triángulo, no subida alta):
   `if (pad0->ExitVehicleJustDown() && !bIsClimbingHighJump && IsPlayer())`
   → `pad0->Clear(false); EndClimbing(true); return;`
   (¡Es el **único** camino que hace `return` aquí!)
2. **Pasar de colgada a tirón** (`bIsClimbingIdle`):
   `if (!IsPlayer() || pad0->JumpJustDown())` → (`NPC` trepa solo; el jugador
   pulsa salto) `bIsClimbingIdle = false; bIsClimbingPull = true;`
   `currentClimbingIdleAnim->blendDelta = -1000.0f; bUsesCollision = 0;`
   `AddAnimation(JUMP_PULL)` + `SetFinishCallback(FinishClimbingPullCB)`
3. **Desplazamiento** (interpolación, no velocidad):
   ```cpp
   CVector cur = GetPosition(), distance; float speed;
   if (bIsClimbingHighJump || bIsClimbingIdle) { speed = highClimbingOffsetSpeed;
       distance = newClimbingIdlePosition - cur; }
   else if (bIsClimbingPull) { speed = 1.3f;
       distance = (correctedClimbingStandPosition + CVector(0,0,0.5f)) - cur; }
   else if (bIsClimbingJumpB) { speed = 0.75f; distance = newClimbingJumpBPosition - cur; }
   else { speed = 1.5f; distance = newClimbingPosition - cur; }
   float mag = distance.Magnitude();
   float step = CTimer::GetTimeStepInSeconds() * speed;
   if (mag > 0.1f) {
       if (bIsClimbingIdle) { m_vecMoveSpeed = CVector(0,0,0); bUsesCollision = 0; }
       SetPosition(cur + distance / mag * step);
   } else if (bIsClimbingIdle) { bUsesCollision = 1; }   // llegó: recupera colisión
   ```
   | Fase | Velocidad | Destino |
   |---|---|---|
   | alta / colgada | `highClimbingOffsetSpeed` (3.0) | `newClimbingIdlePosition` |
   | tirón (`pull`) | 1.3 | `correctedClimbingStandPosition + (0,0,0.5)` |
   | valla (`jump_B`) | 0.75 | `newClimbingJumpBPosition` (5 m delante) |
   | baja | 1.5 | `newClimbingPosition` |

### 5.6 Los 4 callbacks (`Ped.cpp:9761-9815`)

| Callback | Qué hace |
|---|---|
| `FinishHighClimbingCB` | `assoc->blendDelta = -1.0f`; `bIsClimbingIdle = true`; `bIsClimbingHighJump = false`; `m_vecMoveSpeed = 0`; `currentClimbingIdleAnim = BlendAnimation(JUMP_IDLE, 10.0f)` |
| `FinishClimbingPullCB` | `assoc->blendDelta = -1000.0f`; `bIsClimbingPull = false`; si `bIsClimbingJumpB` → `AddAnimation(JUMP_JUMP_B)` + `FinishClimbingCB`; si no → `AddAnimation(JUMP_STAND)` + `FinishClimbingCB` |
| `FinishClimbingCB` | si `bIsClimbing` y `!bIsClimbingJumpB` → `assoc->blendDelta = -1000.0f` + `AddAnimation(JUMP_STAND_FINISH)` + `FinishFinallyStandClimbingCB`; luego **siempre** `EndClimbing(false)` |
| `FinishFinallyStandClimbingCB` | `assoc->blendDelta = -3.0f` (solo deja desvanecerse el clip de pie) |

> `FinishClimbingCB` es el que **cierra** la escalada en las ramas baja y valla;
> `FinishHighClimbingCB` **no** cierra (deja colgado en `CLIMB_idle` hasta que el
> jugador pulse salto o un NPC siga solo).

### 5.7 `EndClimbing(bIsCancel)` (`Ped.cpp:322-360`)

- solo actúa `if (bIsClimbing)`
- **si cancelación** (no si fin normal): desvanece los 3 anims
  (`idle −5.0`, `stand −2.5`, `jumpB −5.0`) y **recoloca**:
  `SetPosition(bIsClimbingIdle ? newClimbingIdlePosition : correctedClimbingStandPosition)`
- limpia: `bIsReadyToClimbing/bIsClimbing/bIsClimbingHighJump/bIsClimbingIdle/
  bIsClimbingPull/bIsClimbingJumpB = false`; los 3 punteros de anim y
  `currentJumpGlideAnim = nullptr`
- `bAffectedByGravity = 1; bUsesCollision = 1; m_vecMoveSpeed = 0;`
- `RestorePreviousState();`

### 5.8 Los 7 ganchos

| Función nuestra | Línea | Qué añade el mod |
|---|---|---|
| `CPed::ProcessControl` | `:1874` | El bloque de §5.5 **al principio** (antes de `CColPoint foundCol;`) |
| `CPed::ProcessEntityCollision` | `:3023` | `if (bIsClimbing) return 0;` (al principio) |
| `CPed::PedLandCB` | `:4102` | `ped->bIsReadyToClimbing = false;` (al final) |
| `CPed::InTheAir` | `:5599` | En la rama `else if (m_nPedState != PED_ABSEIL && !RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FALL))`: `if (m_vecMoveSpeed.z > playerVerticalVelocityAtWhichStartsToFall) return;` + `bIsReadyToClimbing = false;` **antes** del `BlendAnimation(FALL)` |
| `CPed::CanPedJumpThis` | `:9420` | `if (!IsPlayer() && bClimbingPeds) pos.z += maxPossibleCheckHeightForPeds;` (los NPC trepan 2.85 m más alto) |
| `CPed::SetJump` | `:9452` | Tras `BlendAnimation(JUMP_LAUNCH, 8.0f)`: `if (CanPedClimbingThis(f,b,j)) jumpAssoc->blendDelta = 4.25f;` (acelera el clip para encadenar con la escalada) |
| `CPed::FinishLaunchCB` | `:9466` | (1) si `CanPedClimbingThis(f,b,j)` → `assoc->blendDelta = -1000.0f; StartClimbing(f,b,j); return;` (2) tras `if (m_nPedState != PED_JUMP) return;`, el bloque `#ifdef/:else` de D3: con escalada `if ((!IsInInterior() \|\| bClimbingInInteriors) && (IsPlayer() \|\| bClimbingPeds)) bIsReadyToClimbing = true;`, **sin** el chequeo vanilla de obstáculo/`HIT_WALL`; (3) al final, sustituir `AddAnimation(ASSOCGRP_STD, ANIM_STD_JUMP_GLIDE)` por `ped->currentJumpGlideAnim = AddAnimation(...)` |

### 5.9 `PedFight.cpp` (`:374-377` del mod)

`if (IsPlayer() || (!victimPed || (victimPed->IsPedInControl() || victimPed->bIsClimbing)))`
→ **nuestra línea de ancla es `PedFight.cpp:415`** (la misma condición sin el
`|| victimPed->bIsClimbing`). Sin este cambio, pegar a un ped colgado no lo
alcanza.

### 5.10 INI `[Climbing]` — 8 claves

| Clave (verbatim) | Destino | Tipo |
|---|---|---|
| `ClimbingInInteriors` | `CPed::bClimbingInInteriors` | bool |
| `ClimbingOnVehicles` | `CPed::bClimbingOnVehicles` | bool |
| `ClimbingPeds` | `CPed::bClimbingPeds` | bool |
| `MaxPossibleClimbingHeight` | `CPed::maxPossibleClimbingHeight` | float |
| `MaxPossibleCheckHeightForPeds` | `CPed::maxPossibleCheckHeightForPeds` | float |
| `MaxClimbingWithRaisedHandsHeight` | `CPed::maxHighClimbingHeight` | float |
| `ClimbingWithRaisedHandsOffsetSpeed` | `CPed::highClimbingOffsetSpeed` | float |
| `PlayerVerticalVelocityAtWhichStartsToFall` | `CPed::playerVerticalVelocityAtWhichStartsToFall` | float |

> **Aviso de tipo**: nuestro `re3.cpp` tiene `StoreIni` para `uint32/uint8/int32/
> int8/float/char*` pero **no para `bool`** (`:285-320`); los 3 bools se escriben
> con cast `(int32)`, como ya hace el bloque `ClassicAxis` (`:636-644`).
> `ReadIniIfExists(…, bool*)` **sí** existe (`:230`).
> `re3.cpp` **no incluye `Ped.h`** (0 ocurrencias de `CPed`) → el
> `#include "Ped.h"` bajo `#ifdef VICEEXT_CLIMB` del mod (`re3.cpp:54-56`) es
> **necesario**.

## 6. Mapa fichero → fichero (anclas verificadas por grep en el árbol)

| Bloque del mod | Nuestro fichero / ancla (nº de línea **nuestra**, verificado) |
|---|---|
| `Ped.h:331-332` `PED_CLIMBING` | `peds/Ped.h:330-332` — entre `PED_DIVE_AWAY,` (`:330`) y `PED_STATES_NO_ST,` (`:332`) |
| `Ped.h:656-689` estáticos + estado | `peds/Ped.h:651-653` — tras `float m_radiusToGuard;` (`:651`) y antes de `static void *operator new(size_t) throw();` (`:653`) |
| `Ped.h:700-716` 12 métodos | `peds/Ped.h` — bloque nuevo, junto a `void RestorePreviousState(void);` (`:691`) |
| `Ped.h:951-955` 4 callbacks | `peds/Ped.h:892-893` — tras `static void FinishHitHeadCB(...)` (`:892`), dentro del grupo `// Callbacks` (`:885`) |
| `Ped.cpp:57-66` estáticos | `peds/Ped.cpp:57-58` — tras `CVector2D CPed::ms_vec2DFleePosition;` (`:57`) |
| `Ped.cpp:68-360` detección + `StartClimbing`/`EndClimbing` | `peds/Ped.cpp` — bloque nuevo (los 4 CBs van junto a `PedLandCB`) |
| `Ped.cpp:9761-9815` 4 callbacks | `peds/Ped.cpp:4101-4113` — junto a `CPed::PedLandCB` (`:4102`) |
| `Ped.cpp:2057-2128` `ProcessControl` | `peds/Ped.cpp:1874-1877` — tras `CPed::ProcessControl(void)\r\n{` y **antes** de `CColPoint foundCol;` (`:1876`) |
| `Ped.cpp:3278-3282` colisión | `peds/Ped.cpp:3023-3026` — al principio de `CPed::ProcessEntityCollision` |
| `Ped.cpp:4356-4358` `PedLandCB` | `peds/Ped.cpp:4102-4112` — al final (tras el `RestorePreviousState` de `:4111`) |
| `Ped.cpp:5720-5726` `InTheAir` | `peds/Ped.cpp:5599+` — rama `else if (m_nPedState != PED_ABSEIL …)` |
| `Ped.cpp:9559-9562` `CanPedJumpThis` | `peds/Ped.cpp:9420` — antes de `CVector forwardPos = pos + forwardOffset;` |
| `Ped.cpp:9577-9585` `SetJump` | `peds/Ped.cpp:9452` — tras `BlendAnimation(..., ANIM_STD_JUMP_LAUNCH, 8.0f)` |
| `Ped.cpp:9595-9600` + `9610-9624` + `9665-9671` `FinishLaunchCB` | `peds/Ped.cpp:9466` (+ `:9491` `HIT_WALL` a retirar; `:9500` el `ApplyMoveForce` de `:9610`; `ANIM_STD_JUMP_GLIDE` al final) |
| `PedFight.cpp:374-377` | `peds/PedFight.cpp:415` |
| `AnimationId.h:202-209` (6 ids) | `animation/AnimationId.h:300-306` (7 ids; ver §12.2) |
| `AnimManager.cpp:196-202` (6 descs, tabla STD) | `animation/AnimManager.cpp:1037-1043` (`aClimbAnimDescs` del grupo `playerclimb`) |
| `re3.cpp:54-56` `#include "Ped.h"` | `core/re3.cpp:42-52` (zona de includes; `crossplatform.h` en `:52`) |
| `re3.cpp:560-568` lectura INI | `core/re3.cpp:505-521` (bloque `ClassicAxis`, patrón a copiar) |
| `re3.cpp:679-687` escritura INI | `core/re3.cpp:636-644` (bloque `StoreIni("ClassicAxis", …)`) |
| `config.h:504` `#define CLIMBING` | `core/config.h:417` (`#define VICEEXT_CLIMB`, **ya existe**) |

## 7. Pasos ejecutables

> Orden: T1 (anim) → T2 (`Ped.h`) → T3 (`Ped.cpp` nuevo) → T4 (ganchos) → T5
> (`PedFight`) → T6 (INI) → T7 (retirada de E1) → T8 (traza/build/test).
> **E-1=(a)**: la retirada de E1 va **al final**, para no dejar ninguna ventana de
> build sin escalada. Un bloque = un build + PASS del jugador.

- **T1 · Animación (D1=(a))**. En `AnimManager.cpp:1037-1043` alinear los flags a
  los del mod (§12.2) y **añadir `ASSOC_HAS_TRANSLATION` a `CLIMB_jump_B`**
  (raíz 0.891 m). Mantener ids, nombres y el grupo `playerclimb`. `CLIMB_jump2fall`
  lo usaba E1 y el mod **no** lo usa: se deja registrado (inofensivo).
- **T2 · `Ped.h`**. `PED_CLIMBING`; los 8 estáticos + el estado de §5.2 (+
  `currentJumpGlideAnim`); los 12 métodos (`CanPedClimbingThis`,
  `CheckObjectFrontPlayer`, `CheckObjectAbovePlayer`,
  `CheckPotentialClimbingPlaceFind`, `CheckClimbingPlaceFree`,
  `CheckClimbingTheFence`, `IsNeedFixFenceSideNormal`,
  `CheckPotentialClimbingTheFencePlaceFind`, los 2 inline de altura,
  `StartClimbing`, `EndClimbing`); los 4 callbacks. Todo bajo `#ifdef VICEEXT_CLIMB`.
- **T3 · `Ped.cpp` (bloque nuevo)**. Los estáticos con los defaults de §5.1 y las
  10 funciones de §5.3/§5.4/§5.6/§5.7 desde el anexo §12.1.
  **Renombrar `СurrentPedPosition` → `currentPedPosition`** (la `С` cirílica del
  fuente rompería el build). Poner `currentJumpGlideAnim = nullptr` en los resets.
  Aplicar **D7**.
- **T4 · `Ped.cpp` (ganchos)**. Los 7 puntos de §5.8, con las anclas exactas de §6.
- **T5 · `PedFight.cpp`**. La condición de víctima melee (`victimPed->bIsClimbing`).
- **T6 · `re3.cpp` (+ `config.h` sin cambios)**. `#include "Ped.h"` bajo
  `#ifdef VICEEXT_CLIMB`; bloque de lectura en `LoadINISettings` (junto a
  `ClassicAxis`, `:505-521`) y de escritura en `SaveINISettings` (`:636-644`),
  con los tipos de §5.10 (cast `(int32)` para los 3 bools).
- **T7 · retirada de E1**. Borrar el bloque `#ifdef VICEEXT_CLIMB` de
  `PlayerPed.cpp:2940-3109` (las 5 `#define`, los 5 estáticos `odClimb*`, las 3
  funciones y sus trazas), la llamada en `:2142` (+ `#ifdef` `:2139`), el forward
  `:2470-2473` y la llamada desde el nado `:2819-2820`; y
  `PlayerPed.h:123`. **Las 3 trazas de E1 se retiran** (§8 las sustituye).
- **T8 · traza + build + verificación**. Trazas nuevas (§8), objeto único
  (`ninja …/peds/Ped.cpp.o …/peds/PedFight.cpp.o …/animation/AnimManager.cpp.o
  …/core/re3.cpp.o …/peds/PlayerPed.cpp.o`), `VERSION++`
  (`web/lib/index.js:171`), `bash gta_vc_browser/build.sh`,
  `bash gta_vc_browser/tools/check-served-build.sh` (debe dar exit 0) y marcas
  nuevas. `dataTag` **no** se toca.

## 8. Verificación

**a) Trazas (eventos, nunca por-frame)**. `Ped.cpp` ya incluye `ondemand.h`
(`:10`). Patrón de E1 (`snprintf` + `ODTRACES`) para las que llevan números;
`ODTRACEI(tag, v)` no vale con floats:

| Evento | Texto propuesto | Dónde |
|---|---|---|
| arranque | `VICEEXT climb start tipo=baja\|alta\|valla\|valla_alta colgado=0/1 h=%.2f npc=0/1` | fin de `StartClimbing` |
| tirón | `VICEEXT climb pull` | paso a `bIsClimbingPull` en `ProcessControl` |
| colgado | `VICEEXT climb idle` | `FinishHighClimbingCB` |
| fin | `VICEEXT climb fin tipo=baja\|alta\|valla` | `FinishClimbingCB` antes de `EndClimbing(false)` |
| cancelar | `VICEEXT climb cancel` | `EndClimbing(true)` |
| NPC | `VICEEXT climb npc` | en `StartClimbing` si `!IsPlayer()` |
| reintento | `VICEEXT climb ready` (máx. 1/s) | `bIsReadyToClimbing = true` en `FinishLaunchCB` (para D8) |

**b) `check-served-build.sh`**: 3-4 marcas nuevas que prueben que el wasm
enlazado las lleva (patrón de las marcas R29 en `:90-92`), p. ej.
`CanPedClimbingThis|climb`, `VICEEXT climb start tipo=|climb`,
`VICEEXT climb fin tipo=|climb`, `IsNeedFixFenceSideNormal|climb`.

**c) `viceext-log-check.py`**: bloque nuevo que:
- empareje `start → (pull → idle)? → fin|cancel` y cuente escaladas completas;
- detecte **bucles**: más de N `VICEEXT climb ready` sin `start`/`fin`
  (indicador del coste de D8 y de reintentos sin limpieza);
- exija **cancelaciones limpias** (`cancel` seguido de `fin`, sin `idle` colgado);
- compruebe la secuencia de una **subida alta** (`start tipo=alta` → `idle` →
  `pull` → `fin`).

**d) PASS del jugador** (su partida + log, sin arnés headless): los 5 puntos de
§0. Medir además, en la valla, que el desplazamiento horizontal es coherente con
la raíz del clip (`CLIMB_jump_B` 0.891 m) + la fase de 0.75 u/s — si sale el
doble, revisar la interacción `ASSOC_HAS_TRANSLATION` × `SetPosition` (§9.4).

## 9. Riesgos y trampas

### 9.1 Trampas de port
1. **`С` cirílica** en `СurrentPedPosition` (fuente ruso) → renombrar al copiar.
2. **Mutación por referencia** de `hitFwd` (§5.3): mantener firmas `&`.
3. **`AddAnimation` no deduplica**: los callbacks se disparan por fin de clip
   (una vez). No convertir en por-frame.
4. **`bIsReadyToClimbing`**: se pone en `FinishLaunchCB` y se reintenta **cada
   frame** en `ProcessControl`; hay que garantizar su limpieza (`StartClimbing`,
   `PedLandCB`, `InTheAir`) para no dejar un reintento infinito.
5. **Unidades** (§5.1): no aplicar `METERS_PER_SECOND_TO_GAME_SPEED`.
6. **Nuestro `Ped.cpp` ha divergido** del mod (~1.950 líneas): usar las
   **funciones** como ancla, nunca los números de línea del mod.
7. **Asyncify**: no introducir control-flow nuevo en `LoadAllRequestedModels`
   (aquí no aplica: `Ped.cpp` no está en ese camino).

### 9.2 Bug latente del mod → D7
`bIsClimbingJumpB` no se limpia al tomar la rama de **repisa**
(`CheckPotentialClimbingPlaceFind`), solo en el fallo de valla y en
`EndClimbing`. Tras una valla fina, una subida normal heredaría `true` y
`StartClimbing` tomaría la rama **B** con posiciones de valla. Mitigación: D7.
**Verificar en el PASS**: valla fina → aterrizar → subida normal, y que sale
`start tipo=baja` (no `valla`).

### 9.3 `PED_CLIMBING` y los `switch(m_nPedState)`
Hay **4** en el árbol: `CivilianPed.cpp:241`, `EmergencyPed.cpp:57`,
`Ped.cpp:2774`, `PlayerPed.cpp:4994`. **Los 4 tienen `default: break;`**
(verificado: `CivilianPed.cpp:401`, `EmergencyPed.cpp:69/82/158`,
`Ped.cpp:2964`, `PlayerPed.cpp:5060/5124/5393`) → un estado nuevo **no** rompe
ninguno. Ojo: tras el `default` de `PlayerPed.cpp:5124` corre
`if (padUsed && IsPedShootable() && …) { ProcessWeaponSwitch(padUsed); GetWeapon()->Update(…); }`
→ vigilar que escalar no meta un cambio de arma (el mod no lo guarda; si pasa,
cortar con `&& !bIsClimbing`).

### 9.4 `ASSOC_HAS_TRANSLATION` × `SetPosition`
`CLIMB_jump_B` lleva raíz (0.891 m) **y** el mod mueve el ped con
`SetPosition` a 0.75 u/s (§5.5). Puede sumar dos desplazamientos. Es lo que hace
el mod → se porta fiel y se **mide** en el PASS (§8.d). Si sale el doble, la
corrección es quitar el `SetPosition` de esa fase, no el flag.

### 9.5 Coste en WASM (D8)
`CheckPotentialClimbingTheFencePlaceFind` = hasta **40 × (2 rayos + 2 esferas)**.
Se re-evalúa por frame mientras `bIsReadyToClimbing`. Medir con
`VICEEXT climb ready` (§8.a) y los FPS; si hay coste, **capar la ventana** de
`bIsReadyToClimbing` (no el bucle, que sale pronto cuando hay valla delante).

### 9.6 Presupuesto de parciales
Los 6 clips son `ASSOC_PARTIAL` (como los del agachado/aim, R28). Agachado ≠
escalada, así que no deberían coincidir, pero **anotarlo** y mirar que no se
dispare el total de parciales activos (regla del R28: la pose parcial se come la
traslación).

### 9.7 Carriles
`Ped.cpp` R20c (`CalculateNewVelocity`) y `PedFight.cpp` R28 (agachado) están en
**otro carril**: el port toca funciones **distintas** (`ProcessControl`,
`ProcessEntityCollision`, `PedLandCB`, `InTheAir`, `CanPedJumpThis`, `SetJump`,
`FinishLaunchCB`) y la condición de `PedFight.cpp:415`. **Reanclar antes de
editar** y no reordenar bloques ajenos. No tocar `Cam.cpp`/`Camera.cpp`/`Hud.cpp`/
`Pad.cpp` ni `ViceExtSwim*`.

## 10. Restricciones (reglas de casa)

- `.cpp`/`.h` = **CRLF**, edición con python binario (`assert count == 1`);
  consola cp1252 → `PYTHONIOENCODING=utf-8`. Los `.md` del harness son **LF**
  (salvo `.agents/HISTORIAL.md`, que es **CRLF**).
- `VERSION` sube en cada build (`web/lib/index.js:171`); `dataTag` **no** sube.
- `bash gta_vc_browser/tools/check-served-build.sh` OK **antes** de pedir partida.
- Build: `export PATH=/c/Users/s0rno/emsdk/upstream/emscripten:$PATH` +
  `bash gta_vc_browser/build.sh` (emsdk_env.sh no basta). Objeto único:
  `cd gta_vc_browser/build/web && ninja src/CMakeFiles/reVC.dir/<path>.cpp.o`.
- Sin commits ni `git add`; no matar `node`.
- Todo el bloque detrás de `#ifdef VICEEXT_CLIMB` (apagable). Atribución:
  cabecera de cortesía (código del mod `Climbing [reVC]`) + fila en
  `docs/mods/ATTRIBUTION.md` y entrada en `.agents/HISTORIAL.md`.
- Licencias: **irrelevantes** desde el 27/09 (`00-INDICE` §2, ADR-002): se toma
  el código del mod tal cual, adaptado a nuestras clases.

## 11. Decisiones resueltas (28/09)

| # | Pregunta | Respuesta | Nota |
|---|---|---|---|
| **E-1** | Orden de la retirada de E1 (T7) | **(a) al final** | Sin ventana de build sin escalada |
| **E-2** | NPCs trepando (`bClimbingPeds`) | **sí** | Default del mod |
| **E-3** | Cancelar con triángulo, tal cual | **sí** | verbatim |
| **E-4** | `FinishLaunchCB` fiel (pierde el gesto de choque contra muro) | **fiel** | D3 |

Decisiones técnicas que se asumen con este plan (revertibles por el jugador):
**D1=(a)** grupo `playerclimb` en vez de `ASSOCGRP_STD`; **D7** limpiar
`bIsClimbingJumpB` al entrar (corrige un bug del mod); **D8** medir el coste antes
de capar nada.

## 12. Anexo A — código de referencia

> Copiado del mod con la **sustitución D1** aplicada en la anotación:
> `ASSOCGRP_STD` → `ASSOCGRP_PLAYERCLIMB`; `ANIM_STD_CLIMBING_JUMP_IDLE` →
> `ANIM_STD_CLIMB_IDLE`, `…_JUMP_JUMP` → `ANIM_STD_CLIMB_JUMP`, `…_JUMP_JUMP_B` →
> `ANIM_STD_CLIMB_JUMP_B`, `…_JUMP_PULL` → `ANIM_STD_CLIMB_PULL`, `…_JUMP_STAND` →
> `ANIM_STD_CLIMB_STAND`, `…_JUMP_STAND_FINISH` → `ANIM_STD_CLIMB_STAND_FINISH`
> (§12.2). `СurrentPedPosition` → `currentPedPosition`. Todo bajo
> `#ifdef VICEEXT_CLIMB`. Este anexo es **la fuente de T3**; no hace falta
> releer los 10.164 líneas del mod para implementarlo.

### 12.1 Funciones (T3)

```cpp
// --- estáticos (junto a ms_vec2DFleePosition, Ped.cpp) ---
bool  CPed::bClimbingInInteriors = false;
bool  CPed::bClimbingOnVehicles  = false;
bool  CPed::bClimbingPeds        = true;
float CPed::maxPossibleClimbingHeight         = 2.25f;
float CPed::maxPossibleCheckHeightForPeds     = 2.85f;
float CPed::maxHighClimbingHeight             = 1.5f;
float CPed::highClimbingOffsetSpeed           = 3.0f;
float CPed::playerVerticalVelocityAtWhichStartsToFall = -0.2f;

// --- detección ---
bool CPed::CanPedClimbingThis(CColPoint &hitForwardPoint, CColPoint &hitBackwardPoint, CColPoint &hitJumpBPoint)
{
	// D7: el mod no limpia estas banderas en la rama de repisa; sin esto, una
	// subida normal hereda el camino de valla de una escalada anterior.
	bIsClimbingJumpB = false;
	bIsClimbingHighJump = false;

	if (!IsPlayer() && !bClimbingPeds)
		return false;
	if (CGame::IsInInterior() && !bClimbingInInteriors)
		return false;

	if (CheckObjectFrontPlayer(hitForwardPoint) && !CheckObjectAbovePlayer()) {
		if (CheckPotentialClimbingPlaceFind(hitForwardPoint, hitBackwardPoint))
			return CheckClimbingPlaceFree(hitBackwardPoint) && hitBackwardPoint.normal.z >= 0.8f;
		return CheckClimbingTheFence(hitForwardPoint, hitBackwardPoint, hitJumpBPoint);
	}
	return false;
}

bool CPed::CheckObjectFrontPlayer(CColPoint &hitForwardPoint)
{
	CVector startPosition = GetPosition() + CVector(0.0f, 0.0f, -0.25f);
	CVector forward = GetForward();
	CEntity *hitEntity;
	// (1) a la altura de los pies; (2..4) pecho/hombros; (5..8) desde +3,00
	if (CWorld::ProcessLineOfSight(startPosition, startPosition + forward, hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,1.45f), startPosition + forward + CVector(0,0,1.45f), hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,1.55f), startPosition + forward + CVector(0,0,1.55f), hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,1.65f), startPosition + forward + CVector(0,0,1.65f), hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,3.0f), startPosition + forward * 0.25f, hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,3.0f), startPosition + forward * 0.5f, hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,3.0f), startPosition + forward * 0.75f, hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	if (CWorld::ProcessLineOfSight(startPosition + CVector(0,0,3.0f), startPosition + forward, hitForwardPoint, hitEntity, true, bClimbingOnVehicles, false, true, false, false)) {
		hitForwardPoint.point.z = startPosition.z; return true;
	}
	return false;
}

bool CPed::CheckObjectAbovePlayer(void)   // el mod pasa hitForwardPoint y no lo usa
{
	CEntity *hitEntity;
	bool above = CWorld::ProcessLineOfSight(GetPosition(), GetPosition() + CVector(0,0,2.75f), CColPoint{}, hitEntity, true, false, false, true, false, false);
	CEntity *s1 = CWorld::TestSphereAgainstWorld(GetPosition() + CVector(0,0,1.00f), 0.25f, this, true, true, true, true, false, false);
	CEntity *s2 = CWorld::TestSphereAgainstWorld(GetPosition() + CVector(0,0,1.25f), 0.25f, this, true, true, true, true, false, false);
	CEntity *s3 = CWorld::TestSphereAgainstWorld(GetPosition() + CVector(0,0,1.50f), 0.25f, this, true, true, true, true, false, false);
	CEntity *s4 = CWorld::TestSphereAgainstWorld(GetPosition() + CVector(0,0,1.75f), 0.25f, this, true, true, true, true, false, false);
	CEntity *s5 = CWorld::TestSphereAgainstWorld(GetPosition() + CVector(0,0,2.00f), 0.25f, this, true, true, true, true, false, false);
	return above || s1 || s2 || s3 || s4 || s5;
}

bool CPed::CheckPotentialClimbingPlaceFind(CColPoint hitForwardPoint, CColPoint &hitBackwardPoint)
{
	CEntity *hitEntity;
	CVector distance = -hitForwardPoint.normal * 0.25f;
	return CWorld::ProcessLineOfSight(hitForwardPoint.point + distance + CVector(0,0,maxPossibleClimbingHeight),
	                                   hitForwardPoint.point + distance, hitBackwardPoint, hitEntity,
	                                   true, bClimbingOnVehicles, false, true, false, false);
}

bool CPed::CheckClimbingPlaceFree(CColPoint hitBackwardPoint)
{
	CEntity *b1 = CWorld::TestSphereAgainstWorld(hitBackwardPoint.point + CVector(0,0,0.30f), 0.215f, this, true, true, true, true, false, false);
	CEntity *b2 = CWorld::TestSphereAgainstWorld(hitBackwardPoint.point + CVector(0,0,0.50f), 0.200f, this, true, true, true, true, false, false);
	CEntity *b3 = CWorld::TestSphereAgainstWorld(hitBackwardPoint.point + CVector(0,0,0.75f), 0.400f, this, true, true, true, true, false, false);
	CEntity *b4 = CWorld::TestSphereAgainstWorld(hitBackwardPoint.point + CVector(0,0,1.50f), 0.400f, this, true, true, true, true, false, false);
	CEntity *b5 = CWorld::TestSphereAgainstWorld(hitBackwardPoint.point + GetForward() * 0.25f + CVector(0,0,0.5f), 0.1f, this, true, true, true, true, false, false);
	return !b1 && !b2 && !b3 && !b4 && !b5;
}

bool CPed::IsNeedFixFenceSideNormal(CColPoint hitForwardPoint)
{
	return DotProduct(hitForwardPoint.normal, GetForward()) > 0.0f;
}

bool CPed::CheckClimbingTheFence(CColPoint &hitForwardPoint, CColPoint &hitBackwardPoint, CColPoint &hitJumpBPoint)
{
	if (IsNeedFixFenceSideNormal(hitForwardPoint))
		hitForwardPoint.normal = -hitForwardPoint.normal;

	if (CheckPotentialClimbingTheFencePlaceFind(hitForwardPoint, hitBackwardPoint, hitJumpBPoint)) {
		if (Distance(hitForwardPoint.point, hitJumpBPoint.point) < 0.25f) {
			newClimbingJumpBPosition = (hitForwardPoint.point - hitForwardPoint.normal * 5.0f);
			bIsClimbingJumpB = true;
			m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
			return true;
		}
		bIsClimbingHighJump = false;
		return false;
	}
	bIsClimbingHighJump = false;
	bIsClimbingJumpB = false;
	return false;
}

bool CPed::CheckPotentialClimbingTheFencePlaceFind(CColPoint &hitForwardPoint, CColPoint &hitBackwardPoint, CColPoint &hitJumpBPoint)
{
	(void)hitBackwardPoint;
	bool isSideFenceFind = false;
	bool IsPreviousSideFenceFind = false;
	CVector valueAddition = { 0.0f, 0.0f, 0.0f };
	float startHitForwardPointZ = hitForwardPoint.point.z;

	for (int i = 0; i < 40; i++) {
		CEntity *hitEntity;
		isSideFenceFind = CWorld::ProcessLineOfSight(hitForwardPoint.point - hitForwardPoint.normal * 2.0f + valueAddition,
		                                             hitForwardPoint.point + valueAddition, hitJumpBPoint, hitEntity,
		                                             true, false, false, true, false, false);
		if (!isSideFenceFind) {
			// Some collision normals of fences give the opposite surface normal, therefore
			isSideFenceFind = CWorld::ProcessLineOfSight(hitForwardPoint.point + hitForwardPoint.normal * 2.0f + valueAddition,
			                                             hitForwardPoint.point + valueAddition, hitJumpBPoint, hitEntity,
			                                             true, false, false, true, false, false);
		}
		CEntity *hitEntityFrontFence = CWorld::TestSphereAgainstWorld(hitForwardPoint.point - hitForwardPoint.normal + CVector(0,0,0.25f), 0.5f, this, true, true, true, true, true, false);
		CEntity *hitEntityAboveFence = CWorld::TestSphereAgainstWorld(hitForwardPoint.point + CVector(0,0,0.525f), 0.5f, this, true, true, true, true, true, false);
		valueAddition += CVector(0.0f, 0.0f, 0.01f);
		if (!isSideFenceFind && !hitEntityFrontFence && !hitEntityAboveFence) {
			if (IsClimbingHeightHigherThanHigh(hitJumpBPoint.point.z, startHitForwardPointZ))
				bIsClimbingHighJump = true;
			break;
		}
		IsPreviousSideFenceFind = isSideFenceFind;
		hitForwardPoint.point.z = hitJumpBPoint.point.z;   // muta hitFwd.point.z
		if (IsClimbingHeightHigherThanPossible(hitJumpBPoint.point.z, startHitForwardPointZ)) {
			IsPreviousSideFenceFind = false;
			break;
		}
	}
	return IsPreviousSideFenceFind;
}
```

`StartClimbing` / `EndClimbing` (§5.4 / §5.7) y los 4 callbacks (§5.6) se
transcriben del mod con las tres sustituciones anotadas arriba; el texto literal
está en `mods/climbing/source_code/src/peds/Ped.cpp:257-360` y `:9761-9815` y ya
está destilado en §5.

### 12.2 Mapeo de ids y flags (D1)

| Clip (nuestro `aClimbAnimations`) | Nuestro id | Id del mod | Flags del mod (**copiar**) | Flags que tenemos hoy | Delta |
|---|---|---|---|---|---|
| `CLIMB_idle` | `ANIM_STD_CLIMB_IDLE` (300) | `…_JUMP_IDLE` | `REPEAT \| FADEOUTWHENDONE \| PARTIAL` | `REPEAT \| PARTIAL` | **+FADEOUTWHENDONE** |
| `CLIMB_jump` | `ANIM_STD_CLIMB_JUMP` (301) | `…_JUMP_JUMP` | `DELETEFADEDOUT \| PARTIAL` | `FADEOUTWHENDONE \| PARTIAL` | **FADEOUTWHENDONE→DELETEFADEDOUT** |
| `CLIMB_jump_B` | `ANIM_STD_CLIMB_JUMP_B` (302) | `…_JUMP_JUMP_B` | `FADEOUTWHENDONE \| PARTIAL \| HAS_TRANSLATION` | `FADEOUTWHENDONE \| PARTIAL` | **+HAS_TRANSLATION** |
| `CLIMB_Pull` | `ANIM_STD_CLIMB_PULL` (304) | `…_JUMP_PULL` | `DELETEFADEDOUT \| PARTIAL` | `FADEOUTWHENDONE \| PARTIAL` | **FADEOUTWHENDONE→DELETEFADEDOUT** |
| `CLIMB_Stand` | `ANIM_STD_CLIMB_STAND` (305) | `…_JUMP_STAND` | `DELETEFADEDOUT \| PARTIAL` | `FADEOUTWHENDONE \| PARTIAL` | **FADEOUTWHENDONE→DELETEFADEDOUT** |
| `CLIMB_Stand_finish` | `ANIM_STD_CLIMB_STAND_FINISH` (306) | `…_JUMP_STAND_FINISH` | `DELETEFADEDOUT \| PARTIAL` | `FADEOUTWHENDONE \| PARTIAL` | **FADEOUTWHENDONE→DELETEFADEDOUT** |
| `CLIMB_jump2fall` | `ANIM_STD_CLIMB_JUMP2FALL` (303) | — (no existe) | — | `FADEOUTWHENDONE \| PARTIAL` | sin uso en el mod; se deja |

Grupo: `playerclimb` (`AnimManager.cpp:1119`, `ASSOCGRP_PLAYERCLIMB`,
`AnimManager.h:85`). `firstAnimId = aClimbAnimDescs[0].animId = 300`, 7
asociaciones (300-306) → `GetAnimation(id)` indexa 0..6 **con** nuestros ids.

## Closure (persistent memory)

> Se rellena al cerrar el plan. Sin esta entrada el plan no se da por
> terminado (DoD).
