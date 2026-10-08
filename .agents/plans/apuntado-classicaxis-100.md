---
name: apuntado-classicaxis-100
status: EXECUTED
type: feature
domain: gameplay-aiming-camera
owner_rules: .agents
created: 2026-09-27 22:40
enriched: 2026-09-27 (Enrichment · sin ejecución · 0 build · 0 test)
ready: 2026-09-27 (aprobación del jugador del enrichment + las 6 decisiones D1-D5/R1)
ejecutado: 2026-09-27 (gate «ejecuta el plan»)
decisiones_27_09: TODAS CERRADAS
  "1-ley-coche: (a) se queda y se corrige la atribucion"
  "2-ForceAutoAim: (a) fiel -> con raton NO hay auto-aim ni marca"
  "3-FOV-50: (a) activo, umbral del mod (rifle >= 70), expuesto como ajuste"
  "4-fidelidad: (a) shoulder en espacio de objeto SI; b, c, d, e NO (se queda lo nuestro)"
  "5-crouch-fixes: SI, los 5; el #6 (ANIM_UNARMED_PUNCHR) descartado por bug del mod"
  "6-near-clip: se queda como esta ( GeniusZ, otro carril)"
  "7-callsite-recoil: SI, anadir Begin/Apply en la ley de apuntado sin tocar la matematica"
---

# Plan técnico: Apuntado ClassicAXIS al 100% (reescritura desde la spec del mod)

> Petición del jugador (27/09): *«lee todos los planes, contexto de `mods/` y `docs/mods/`, todo lo
> necesario para portar el classic axis a este proyecto, este mod de lo que hay actualmente nada
> funciona, crea un plan que integre al 100% toda la funcionalidad del mod»*.
> Carril: ítem 4 del `12-handoff` §13. Sustituye al ítem 4 como «Apuntado ClassicAXIS+GeniusZ».

---

## 0. Veredicto del análisis: por qué «nada funciona»

El `ATTRIBUTION.md` §4 declara **«Apuntado ClassicAXIS+GeniusZ → PORTADO (`ve58`)»** y §6 lo repite
(`ve59`). **Esa fila es falsa.** Lo fiel del mod en el árbol son **dos líneas**:

- `m_f3rdPersonCHairMultX/Y = 0.53/0.4` (`src/core/Camera.cpp:283-284`) — que además ya es un campo
  de la propia GTA, no un invento nuestro.
- el hombro `0.2f` (`src/core/Cam.cpp:606-610`), que además **se aplica en el espacio equivocado**.

Todo lo demás que el mod hace al apuntar **no existe**:

| Lo que el mod hace al apuntar | Nuestro estado real |
|---|---|
| Una ley de cámara propia (`Process_AimWeapon`) | **No existe.** El apuntado 3ª persona sigue siendo `Process_Syphon` (vanilla). `MODE_AIMING` está en el enum (`Camera.h:41`) pero **su `case` está comentado** (`Cam.cpp:396`) |
| `dist` fija 2.7 m, `heightOffset` 0.25 | No existe: el `ve58` copió shoulder 0.2 y el clamp ±50, pero **nunca escribió la ley** |
| El hombro en **espacio de objeto del ped** (`TransformFromObjectSpace(mat, heading, offset)`, `CamNew.cpp:265`) | Aplicado en **espacio de cámara** (`TargetCoors += CamTargetEntity->GetRight() * s`, `Cam.cpp:1386/1782/5714`) ⇒ hombro que gira con la cámara, no con el cuerpo |
| `StoriesAimingCoords` (0.2 ↔ 0.55) | `s_odAimStoriesShoulder` es **`static bool` que nadie escribe**: rama 0.55 **muerta** (`Cam.cpp:604-610`) |
| FOV a **50°** al apuntar un rifle (`Process_FOVLerp`, `wepMinRange=70`) | **No existe.** No hay FOV de apuntado en ningún sitio |
| `horShift`/`verShift` de lock-on con `CrosshairMult` (`CamNew.cpp:297-300`) | **No existe** |
| `LockOnTargetType` 0/1/2 con las dos marcas (SA/LCS), `timeLockOn` 250 ms, `rotMult` 0.5/3.0 | Solo un **marco de 4 rectángulos** dibujado a mano (`Hud.cpp:541-571`), sin tipos, sin temporizador, sin giro, sin la forma del mod |
| `ShowTriangleForMouseRecruit` + `Find3rdPersonMouseTarget` | **No existe** (y el comentario `Hud.cpp:544-547` lo declara pendiente) |
| `ForceAutoAim` | **No existe** |
| `WalkKey` (`LALT`) | **No existe** |
| `StoriesPointingArm` + los **3 `PedIK.MoveLimb`** (cabeza/torso/brazo) + `torsoPitch` | **No existe** ninguno de los tres |
| `RightAnalogStickSensitivityX/Y` | **No existe** como ajuste (la fórmula `*FOV/80` sí, con `0.01f` fijo) |
| Lectura de stick **cruda** (`-NewState.RightStickX`, `CamNew.cpp:131`) | Usamos `LookAroundLeftRight()` con **zona muerta 85** (`Pad.cpp:3524-3557`) |
| Ratón vertical con **`m_fMouseAccelHorzntl`** (quirk del mod, `CamNew.cpp:150/332`) | Usamos `m_fMouseAccelVertical` |
| `Process_CrouchOffset` (`duckOffset`, `CamNew.cpp:447-459`) | **No existe** |
| `modernCamera`: `target += GetRight()*-0.25 + GetUp()*0.075` (`CamNew.cpp:77-80`) | **No existe** |
| Esconder peds a **<0.5 m** en la transición (`CamNew.cpp:425-429`) | **No existe** |
| Agua: `nearClip 0.2` + `z = nivel + 0.6` + recolocar por `Magnitude2D` (`CamNew.cpp:215-220`) | Tenemos `z = nivel` a secas, y **en la ley de coche**, no en la de a pie |
| `using3rd → false` (`Main.cpp:180-187`): la ley de apuntado manda siempre | Elegimos `FollowPed` vs `FollowPedWithMouse` por `m_bUseMouse3rdPerson` |
| Sin fight cam / sin point-gun cam / duración de transición (`Main.cpp:114-115/220-229`) | **No existe**; `Process_Fight_Cam` sigue vivo |
| `SwitchTransitionSpeed` + `previousHor/VerAngle` + `camUseCurrentAngle` | **No existe** |

**Y un hallazgo de atribución que hay que corregir:** la «ley de coche» que el `ve59` atribuye a
CamNew **no existe en el mod**. `CamNew.cpp` sólo tiene `Process_FollowPed` y `Process_AimWeapon`
(los dos de **a pie**). Las constantes `minDist 2.0 / heightOffset 0.4` que alimentan
`Process_Cam_On_A_String` vienen de `Process_FollowPed`, aplicado al coche **por analogía**
(`Cam.cpp:2321` y `:2317`, altura `0.8*dimZ`). Eso es una **adaptación nuestra, no un port**.

---

## 1. La fuente (leída, no de segunda mano)

Ya está descargada y **es código real del mod**, no una transcripción de plan:

| Fichero | Líneas | Qué es |
|---|---|---|
| `gta_vc_browser/tmp/extsrc/CamNew.cpp` | 498 | **La ley de cámara del mod.** `Process_FollowPed :52-231`, `Process_AimWeapon :233-388`, `Process_AvoidCollisions :390-445`, `Process_CrouchOffset :447-459`, `GetVectorsReadyForRW :461-475`, `Process_FOVLerp :477-498`, constantes `:25-28` |
| `gta_vc_browser/tmp/extsrc/classicaxis_Main.cpp` | 1596 | **Los hooks + el HUD.** `ProcessPlayerPedControl :1203-1461`, `IsAbleToAim :605`, `IsWeaponPossiblyCompatible :652`, `Find3rdPersonMouseTarget :1474`, `Find3rdPersonQuickAimPitch :1464`, `DrawCrosshair :745`, `DrawAutoAimTarget :783`, `DrawTriangleForMouseRecruitPed :864`, `WalkKeyDown :1196`, redirecciones de `playerMovementType :121-142` y `playerShootingDirection :144-178` |
| `mods/Classic AXIS/ClassicAxisVC.ini` | 898 B | INI de 2022, sección `[ClassicAxis]`, **10 claves** |
| `mods/1510564741_ca-1/VC/ClassicAXIS.ini` | 954 B | INI de 2017, 4 secciones, **8 claves** |
| `mods/1510564741_ca-1/ReadMe.txt` | 1700 B | descripción y requisitos (ASI loader + GInput) |

**Huecos de fuente (no inventar, preguntar):**

1. `Settings.h` / `Settings.cpp` **no se bajaron** a `tmp/extsrc/`. Faltan los **rangos, validación y
   valores por defecto** de `zoomForAssaultRifles`, `modernCamera` y `crouchKey`. Los ajustes que sí
   conocemos son los que aparecen en el `ini` + los que se leen en el código.
2. `DrawSATarget`, `DrawLCSTarget` y `DrawSATriangleForMouseRecruit` se **llaman** (`Main.cpp:847/853/900`)
   pero **no están en las fuentes extraídas**: viven en otro fichero del mod que no se bajo. Conocemos
   su firma y sus parámetros (`dist = w/128`, `rotMult` 0.5/3.0, color), **no su geometría**.
3. `MODERNCAMERA` y `bIVOnFootCamera`/`bIVVehicleCamera` son dos cosas distintas; el `ini` de 2017 y el
   de 2022 las llaman distinto. Sin `Settings.h` no se sabe si son la misma opción.

**Licencia: sin LICENSE** (`09-numeracion:33-35`, `06-fuentes:108-110`). ⇒ **reimplementación con
atribución en comentarios, nunca copia literal** (regla de casa del `12-handoff` §1.1 y §3).

---

## 2. Inventario 100% de la funcionalidad del mod (18 ajustes + 6 leyes + 18 hooks)

### 2.1 Los 18 ajustes de los dos INI, con destino real en nuestro árbol

| Clave (defecto) | INI | Semántica **en el código del mod** | Destino nuestro | Estado |
|---|---|---|---|---|
| `CameraCrosshairMultX = 0.53f` | 22 | `Main.cpp:284` → `m_f3rdPersonCHairMultX` | `Camera.cpp:283` | **ya está** (valor) |
| `CameraCrosshairMultY = 0.4f` | 22 | `Main.cpp:285` | `Camera.cpp:284` | **ya está** (valor) |
| `LockOnTargetType = 1` | 22 | 0/1(SA)/2(LCS). `Main.cpp:338` parchea `0x5D5064+6=0`; `:784` corta si 0; `:841-855` elige marca | `Hud.cpp` + `Camera.cpp` | **no** (marco a mano) |
| `ShowTriangleForMouseRecruit = true` | 22 | `Main.cpp:334-335` → `DrawTriangleForMouseRecruitPed :864` | `Hud.cpp` | **no** |
| `StoriesAimingCoords = false` | 22 | `CamNew.cpp:254-257` → hombro `0.55` vs `0.2` | `Cam.cpp:606-610` | **flag muerto** |
| `StoriesPointingArm = false` | 22 | `Main.cpp:1299-1300` `m_fFPSMoveHeading -= 8°` | `PlayerPed.cpp` | **no** |
| `RightAnalogStickSensitivityX = 1.0f` | 22 | `CamNew.cpp:145/327` multiplica `betaOffset` | `Cam.cpp` | **no** |
| `RightAnalogStickSensitivityY = 1.0f` | 22 | `CamNew.cpp:146/328` multiplica `alphaOffset` | `Cam.cpp` | **no** |
| `ForceAutoAim = false` | 22 | `Main.cpp:330/475/1250` `disableAutoAim = !p XboxPad->HasPadInHands() && !forceAutoAim` | `PlayerPed.cpp` | **no** |
| `WalkKey = LALT` | 22 | `Main.cpp:1196-1201, 1215-1217` → `m_fMoveSpeed = 0` | `Pad.cpp`/`PlayerPed.cpp` | **no** |
| `bEnable = 1` | 17 | maestro | guarda de compilación | **no** |
| `bMoveCameraOnVehicle = 1` | 17 | legacy; en 2022 es la string-cam de serie + los nops de transición | `Cam.cpp` | ver §5.1 |
| `bEnableFirstPersonMode = 1` | 17 | ya cubierto por el camino de 1ª persona | — | **fuera** (GeniusZ) |
| `bIVOnFootCamera = 0` | 17 | offset estilo IV | — | **rechazado** (`13:118`) |
| `bIVVehicleCamera = 0` | 17 | ídem en vehículo | — | **rechazado** |
| `bForceLegsMovements = 0` | 17 | `Main.cpp:121-142` + `:1307-1308` (`TYPE_STRAFE`/`TYPE_WALKAROUND`) | `PlayerPed.cpp` | **re-auditar** (§2.3) |
| `bForceAutoAim = 0` | 17 | = `ForceAutoAim` | ídem | — |
| `bForceManualAim = 0` | 17 | forzar **manual** con mando | — | **NO APLICA** (en web es Gamepad API, sin el acoplo GInput del mod) |

### 2.2 Las 6 leyes de `CamNew.cpp`

| Ley | Ancla del mod | Qué hace |
|---|---|---|
| `Process_FollowPed` | `:52-231` | A pie sin apuntar. `minDist 2.0`, `maxDist = 2.0 + PedZoomValueScript` (VC: **Script**, no Smooth, `:70`), `heightOffset 0.4`, `m_fSyphonModeTargetZOffSet`, `modernCamera` offset, `duckOffset`, yaw derive **sólo con mando** (`:117-121`), wrap ±π, clamp `+60/-89.5`, `ForceCameraBehindPlayer` → `Rotating`, `WellBufferMe(0.1,0.06)` umbral `0.06`, agua `nivel+0.6` |
| `Process_AimWeapon` | `:233-388` | **El corazón del encargo.** `doFovChanges=true`, `maxDist 2.7` fijo, `heightOffset 0.25`, hombro en espacio de objeto, **LOS sobre el punto de hombro con repliegue a `target.x/y`**, `z += m_fSyphonModeTargetZOffSet + 0.05`, rama de **lock-on** con `horShift`/`verShift`, `lockMovement` (congela el input), clamp `±50` |
| `Process_AvoidCollisions` | `:390-445` | LOS con `pIgnoreEntity = CamTargetEntity` (`nearClip = max(d-0.3, 0.05)` si `d<1.3`) + **5 esferas** con `pIgnoreEntity = NULL` y **nearClip re-leído cada iteración**; peds a `<0.5 m` y visibles → **invisibles ese frame** y restaurados al siguiente |
| `Process_CrouchOffset` | `:447-459` | `end = -0.5 + ((f-FOV)/minFOV*f)/100`, `interpF(offset, end, 0.1*ts)` |
| `GetVectorsReadyForRW` | `:461-475` | Recalcula `Up`/`Right` desde `Front` (guarda `Front.xy==0`) |
| `Process_FOVLerp` | `:477-498` | `!CanAimWithArm && CanAim && range >= 70` ⇒ `fovLerp → 50`; si no ⇒ `→ 70`; `interpF 0.1*ts`; **Minigun excluido en VC**; `doFovChanges` se consume y se pone a `false` |

### 2.3 Los 18 hooks de comportamiento de `classicaxis_Main.cpp`

| # | Ancla | Qué hace | Choca con |
|---|---|---|---|
| C1 | `:121-142` | `GetMoveAnimTaskType` → `TYPE_STRAFE` si `isAiming && !ignoreRotation`; `TYPE_WALKAROUND` si `forceRealMoveAnim` (que se pone cuando `|moveSpeed| < 0.01`, `:1307`) | H4 (declarado «YA ESTABA») |
| C2 | `:144-178` | `playerShootingDirection` → `TYPE_STRAFE` si `isAiming && !m_bHasLockOnTarget` | idem |
| C3 | `:180-187` | `using3rd` → **siempre `false`** ⇒ la ley de apuntado manda siempre | `m_bUseMouse3rdPerson` |
| C4 | `:114-115` | **sin fight cam** | `Process_Fight_Cam` |
| C5 | `:220-222` | **sin point-gun cam** | camino de `SetPointGunAt` |
| C6 | `:227-229` | duración de transición (5 sitios) | `JUMP_CUT`/`INTERPOLATION` |
| C7 | `:118` | fix de salto (salto que no dispara) | — |
| C8 | `:372-384` | **cambio de arma bloqueado mientras apunta** | `ProcessWeaponSwitch` |
| C9 | `:387-404` | spray suave: `0.00001f` si agachado, `-1.0f` si no | `PedFight` |
| C10 | `:159-170` | al disparar, `MODE_FOLLOW_PED` temporal (arregla *mvl* y parabrisas rompibles) | `CWeapon::Fire` |
| C11 | `:207-211` | `ClearAimFlag` si no apunta | `Ped.cpp` |
| C12 | `:338-344` | parchea el crosshair del juego si `LockOnTargetType > 0` | `Hud.cpp` || C13 | `:372`→`:1203-1244` | `TakeControl(ped, MODE_AIMWEAPON, 1, 0)` + `SwitchTransitionSpeed` + `previousHor/VerAngle` + `previousCamMode` | `Camera.h/.cpp` |
| C14 | `:1252-1286` | con lock-on: `front` al objetivo, `height` = `Find3rdPersonQuickAimPitch(out.y/SCREEN_HEIGHT)`, **suelta el fijado** si el ratón pasa de 1.0 o el objetivo está a `<0.5 m` | `PlayerPed.cpp` |
| C15 | `:1288-1312` | `RotatePlayer` + `SetLookFlag` + `SetAimFlag` + `m_fFPSMoveHeading` clamp ±45° + `torsoPitch` + **3× `PedIK.MoveLimb`** | `PedIK` |
| C16 | `:1341-1378` | fuerza `PEDSTATE_AIMGUN` + el assoc del arma (ya lo hace el motor) | — |
| C17 | `:1196-1201, 1215` | `WalkKeyDown` → `m_fMoveSpeed = 0` | `Pad.cpp` |
| C18 | `:408-441` | fixes de agachado con `CrouchFire` (`bCrouchWhenShooting`), `ClearPointGunAt` al cambiar de fijado | **carril de agachado** |

> **CORREGIDO (enriquecimiento 27/09, C-8, C-9, C-17): tres de estos 18 no requieren código.**
>
> | # | Por qué |
> |---|---|
> | **C4** (sin fight cam) | `TakeControl` deja `m_bLookingAtPlayer = false` (`Camera.cpp:2354`), y **todo** el bloque que decide y aplica el modo —`if(m_bLookingAtPlayer)` en `Camera.cpp:1729-1938`— queda fuera. `ReqMode` se calcula igual (puede valer `MODE_FIGHT_CAM`, `Camera.cpp:1345/1348`) pero **nadie lo aplica**. El mod nopa 2 llamadas; aquí el mismo efecto sale gratis. Sin cambio. |
> | **C5** (sin point-gun cam) | Idéntico: la rama que mete `MODE_SYPHON_CRIM_IN_FRONT` al fijar a menos de 3,5 m está en `Camera.cpp:1553-1559`, dentro de ese mismo bloque ignorado. Sin cambio. |
> | **C12** (parchea el crosshair del juego) | `Main.cpp:338-344` escribe un **byte de datos** (`0x5D5064 + 6 = 0`), no código. En nuestro árbol el camino de mira de 3.ª persona es código (`Hud.cpp:389-424`) y lo sustituye B11 → **NO APLICA**, y además §6 prohíbe tocar datos. |
>
> **C7** (fix de salto, `Main.cpp:118`, un solo byte) y el parche de 12 argumentos de
> `Main.cpp:323-325` (VC) son **parches de bytes sobre direcciones cuyo destino no se puede
> identificar con el código que hay en `tmp/extsrc/`** → `PENDIENTE`: sin la tabla de
> desensamblado de esas direcciones no se sabe a qué `H_CALL` apuntan y, por tanto, no se pueden
> portar. No se inventan. No bloquean nada (§9 los deja fuera).

### 2.4 El HUD del mod (3 funciones)

| Función | Ancla | Qué dibuja |
|---|---|---|
| `DrawCrosshair` | `:745-781` | caja de **±14 px** en `multX/multY` — sólo si `isAiming && !inVehicle && (FOLLOW_PED\|\|AIMWEAPON) && !DisablePlayerControls && **!m_bHasLockOnTarget** && IsWeaponPossiblyCompatible` |
| `DrawAutoAimTarget` | `:783-862` | color = salud (`CRGBA((1-h)*255, h*255, 0, 255)`; negro si `h<=0`; LCS = `CRGBA(0, h*255, 0, 255)` con `alpha 150`), cabeza del ped (`PedIK` bone 1) + `z += 0.25`, `timeLockOn = 250 ms`, `dist = w/128` (SA escala por el tiempo restante), `rotMult = 0.5` / **`3.0` si acabas de perder el fijado** |
| `DrawTriangleForMouseRecruitPed` | `:864-904` | triángulo sobre el objetivo blando de ratón, `alpha 150`, `z = cabeza + 1.0`, `dist = w/128` |

---

## 3. Arquitectura del port

### 3.1 La decisión de fondo: revivir `MODE_AIMING`, no parchear `Process_Syphon`

El mod **no toca** la cámara de apuntado de la GTA: **crea la suya** en el modo `MODE_AIMWEAPON`
(= nuestro `MODE_AIMING` = 5; el plugin-sdk sólo le puso otro nombre) y **se la queda** mientras
apunta (`Main.cpp:1237-1238`). Por eso el enum de la GTA lo tiene desde el principio y en nuestro
árbol está muerto desde siempre (`Cam.cpp:396`).

Port fiel = **revivir ese modo** y reescribir la toma de control al estilo del mod. Parchear
`Process_Syphon` (la cámara syphon de la GTA, que el mod no toca) sería exactamente lo que la
regla §14 prohíbe: «no se parcha por encima de lo existente».

```
ProcessPlayerPedControl (PlayerPed.cpp)          <- Main.cpp:1203-1461
  ├─ isAiming = !DisablePlayerControls && IsAbleToAim && GetTarget()
  │             && GetLookDirection()!=0 && IsWeaponPossiblyCompatible
  │             && (Mode==FOLLOWPED || Mode==AIMING) && HasWeaponAmmoToBeUsed
  │             && !JumpJustDown && !GetSprint
  ├─ si isAiming y Mode != AIMING  -> TakeControl(ped, MODE_AIMING, 1, 0)   [C13]
  ├─ lock-on: front/height, soltar si ratón>1.0 o dist<0.5                   [C14]
  ├─ RotatePlayer / SetLookFlag / SetAimFlag / FPSMoveHeading / 3× PedIK     [C15]
  └─ si !isAiming -> ClearPointGunAt + ClearWeaponTarget + volver a previousCamMode
CCam::Process -> case MODE_AIMING: Process_AimWeapon(...)                   [Cam.cpp:396]
```

### 3.2 Reparto de ficheros y **conflicto de propiedad**

| Fichero | Qué entra | Riesgo de conflicto |
|---|---|---|
| `src/core/Camera.h` | declarar `Process_AimWeapon`; ajustes de la ley | **bajo** (nadie lo toca ahora) |
| `src/core/Cam.cpp` | revivir `MODE_AIMING` + `Process_AimWeapon` + `Process_AvoidCollisions` + `Process_CrouchOffset` + `Process_FOVLerp` + hombro en espacio de objeto + `using3rd=false` | **ALTO** — es el carril de cámara; el swim lane lo ha tocado hasta hace poco |
| `src/core/Camera.cpp` | `Find3rdPersonQuickAimPitch` con la fórmula del mod; `m_f3rdPersonCHairMultX/Y` desde config | **bajo** |
| `src/renderer/Hud.cpp` | las 3 funciones de HUD del mod | **bajo** |
| `src/peds/PlayerPed.cpp` | `ProcessPlayerPedControl` completo, `Find3rdPersonMouseTarget`, `IsAbleToAim`, `IsWeaponPossiblyCompatible`, `WalkKey`, C1/C2/C8/C10 | **MUY ALTO — el swim lane (nado) está editando este fichero AHORA** |
| `src/peds/PedFight.cpp` | C9, C18 (si entran) | **ALTO** — carril de agachado `EXECUTED` |
| `src/core/Pad.cpp`, `ControllerConfig.cpp` | `WalkKey`, sensibilidades por eje | medio |
| `src/core/config.h`, `src/core/re3.cpp` | define + ajustes INI | bajo (dueño: sección 1) |
| `gta_vc_browser/tools/*`, `web/lib/index.js` | checker, marcas, `VERSION` | bajo |

> **Bloqueo de coordinación (no técnico):** `PlayerPed.cpp` es territorio del carril de nado, que está
> en `EXECUTED` con el jugador jugando `swim8`. **Este plan no se puede ejecutar hasta que el carril de
> nado cierre y su dueño suelte el fichero.** Es lo primero que hay que resolver.

### 3.3 Mapeo de la API del mod a la nuestra (verificado, sin inventar)

| Del mod (plugin-sdk) | Nuestro (`src/`) |
|---|---|
| `cam->m_fHorizontalAngle` / `m_fVerticalAngle` | `CCam::Beta` / `CCam::Alpha` |
| `cam->m_fBetaSpeed` | `CCam::BetaSpeed` |
| `cam->m_fTargetBeta` / `m_bRotating` | `CCam::m_fTargetBeta` / `CCam::Rotating` |
| `cam->m_bResetStatics` | `CCam::ResetStatics` |
| `cam->m_fFOV` | `CCam::FOV` (+ `FOVSpeed`) |
| `cam->m_vecSourceBeforeLookBehind` | `CCam::SourceBeforeLookBehind` |
| `cam->m_vecTargetCoorsForFudgeInter` | `CCam::m_cvecTargetCoorsForFudgeInter` |
| `cam->m_fDistanceBeforeChanges` | `CCam::m_fDistanceBeforeChanges` |
| `cam->m_bCollisionChecksOn` | `CCam::m_bCollisionChecksOn` |
| `cam->m_bLookingBehind` | `CCam::LookingBehind` |
| `cam->m_fSyphonModeTargetZOffSet` | `CCam::m_fSyphonModeTargetZOffSet` (`Camera.h:111`) ✅ |
| `TheCamera.m_f3rdPersonCHairMultX/Y` | `CCamera::m_f3rdPersonCHairMultX/Y` (`Camera.h:465-466`) ✅ |
| `TheCamera.TakeControl(ped, MODE_AIMWEAPON, 1, 0)` | `CCamera::TakeControl` (`Camera.cpp:2330`) ✅ + `CCam::MODE_AIMING` |
| `TheCamera.m_bUseMouse3rdPerson` | `CCamera::m_bUseMouse3rdPerson` — **el mod lo anula (C3)** |
| `TheCamera.GetLookDirection()` | `CCamera::GetLookDirection` (`Camera.h:616`) ✅ |
| `info->m_bCanAim` | `info->IsFlagSet(WEAPONFLAG_CANAIM)` (`WeaponInfo.h:15`) |
| `info->m_bCanAimWithArm` | `IsFlagSet(WEAPONFLAG_CANAIM_WITHARM)` (`:16`) |
| `info->m_b1stPerson` | `IsFlagSet(WEAPONFLAG_1ST_PERSON)` (`:17`) |
| `info->m_bThrow` | `IsFlagSet(WEAPONFLAG_THROW)` (`:18`) |
| `info->m_bCrouchFire` | `IsFlagSet(WEAPONFLAG_CROUCHFIRE)` (`:26`) |
| `info->m_bHeavy` | `IsFlagSet(WEAPONFLAG_HEAVY)` (`:19`) |
| `info->m_nAnimToPlay` / `m_fAnimLoopEnd` | `m_AnimToPlay` / `m_fAnimLoopEnd` (`WeaponInfo.h:52/54`) |
| `pad->NewState.RightStickX/Y` | `CPad::GetPad(0)->NewState.RightStickX/Y` (crudo, sin zona muerta) |
| `pad->NewMouseControllerState.x/y` | `CPad::GetPad(0)->GetMouseX()/GetMouseY()` |
| `playa->SetLookFlag(f, true, true)` | `CPed::SetLookFlag(float, bool, bool)` (`Ped.h:680`) ✅ |
| `playa->SetAimFlag(f)` | `CPed::SetAimFlag(float)` (`Ped.h:765`) ✅ |
| `playa->OurPedCanSeeThisOne(t, true)` | `CPed::OurPedCanSeeThisOne` (`Ped.h:686`) ✅ |
| `target->CanSeeEntity(ped, 60°*2)` | `CPed::CanSeeEntity(CEntity*, float)` (`Ped.h:711`) ✅ |
| `thirdPersonMouseTarget->ReactToPointGun(ped)` | `CPed::ReactToPointGun` (`Ped.h:837`) ✅ |
| `playa->ClearWeaponTarget()` | `CPlayerPed::ClearWeaponTarget` (`PlayerPed.h:65`) ✅ |
| `playa->ClearAimFlag()` | `CPed::ClearAimFlag` (`Ped.h:689`) ✅ |
| `TheCamera.Find3rdPersonCamTargetVector(...)` | `CCamera::Find3rdPersonCamTargetVector` (`Camera.cpp:4228`) ✅ |
| `CSprite::CalcScreenCoors` / `CHud::Sprites[HUD_SITEM16]` | igual ✅ (ya se usan en `Hud.cpp`) |
| `settings.*` | **nuevo**: tabla de ajustes del mod (fichero §2.1) |

**⚠️ Punto de riesgo alto:** el mod **muta la tabla global** `CWeaponInfo` en caliente
(`m_bCanAim = false/true` para flamethrower/minigun según esté agachado, `Main.cpp:668-671/735-738`).
En nuestro árbol los flags son una máscara (`m_Flags`), y `GetWeaponInfo` devuelve un puntero
compartido por **todos** los peds y la IA. Mutarlo en el bucle de control es una bomba: hay que
decidir si se replica (con guarda de.restore) o se sustituye por una consulta sin mutar.

---

## 4. Cambios por fichero

### 4.1 `src/core/Camera.h` + `src/core/Cam.cpp`

- **Revivir `MODE_AIMING`**: descomentar el `case` (`Cam.cpp:396`) y añadir
  `void Process_AimWeapon(const CVector &CameraTarget, float TargetOrientation, float, float);`
  junto a las demás declaraciones (`Camera.h:216-234`).
- **Nuevo bloque `CCamNew`-equivalente**, con cabecera de atribución
  (`// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) …`) y **cero direcciones x86,
  cero `plugin::patch`, cero nops**: cada hook del mod se traduce a código nuestro, no a un parche.
- `Process_AimWeapon` = transcripción fiel de `CamNew.cpp:233-388` con el mapa de §3.3:
  `maxDist 2.7` fijo · `heightOffset 0.25` · hombro `TransformFromObjectSpace(placement, heading, (0.2|0.55,0,0))`
  (espacio de **objeto**, no `GetRight()`) · LOS sobre el punto de hombro con repliegue a `target.xy`
  · `z += m_fSyphonModeTargetZOffSet + 0.05f` · rama de lock-on con `horShift`/`verShift` exactos
  (`(multX-0.5f + multX-0.5f) * viewPlaneWidth` y `(viewPlaneHeight*0.0174f) * ((0.5f-multY + 0.5f-multY) * (1/ar))`)
  · `lockMovement` · clamp `±50°` · `doFovChanges = true`.
- **`Process_FOVLerp`**: estado `fovLerp` + `doFovChanges`, `interpF` a `0.1*ts`, `minFOV 50` /
  `maxFOV 70` / `wepMinRange 70`, **Minigun excluido**. Se consume una vez por frame de apuntado.
- **`Process_CrouchOffset`**: estado `duckOffset` con `end = -0.5f + ((f-FOV)/50.f*f)/100.f`.
- **`Process_AvoidCollisions`** compartido por la ley de a pie y la de apuntado: LOS con
  `CamTargetEntity` como ignorar + 5 esferas con el **nearClip re-leído cada vuelta** +
  `d = Max(Min(nearClip, d), 0.1f)` + esconder peds a `<0.5 m` (restaurar en el frame siguiente).
- **Hombro en espacio de objeto** en los tres sitios donde hoy se usa `GetRight()`
  (`Cam.cpp:1386/1782/5714`) — o, si el jugador decide mantener cámara, se documenta la desviación.
  > **CORREGIDO (enriquecimiento 27/09, C-1): esta viñeta SE CAE y el motivo cambia la premisa de
  > §0 y §5.4(a).** `CamTargetEntity->GetRight()` **ya es el eje del propio ped, no el de la cámara**:
  > `CPlaceable::GetRight()` devuelve `m_matrix.GetRight()` (`Placeable.h:20`), y `m_matrix` es el
  > miembro de la **entidad** (`Placeable.h:6`), no de la cámara. El propio árbol ya lo documenta en
  > `Cam.cpp:1778-1781` («aimOffset en espacio objeto = derecha del ped en mundo»). Por tanto los tres
  > sitios **no se tocan**: son leyes que este plan no sustituye y que el jugador validó.
  > La ley nueva usa la misma forma (`GetMatrix().GetPosition() + GetRight() * hombro`), que es
  > numéricamente **idéntica** a `TransformFromObjectSpace(mat, heading, (0.2,0,0))` para un ped con
  > sólo rotación Z, que es lo único que el mod escribe (`ClassicAxis::RotatePlayer` ->
  > `m_placement.SetRotateZOnly`, `Main.cpp:571`). Ver B5 y §10.18-3.
- **Lectura de stick cruda** (`-NewState.RightStickX/Y`) en vez de `LookAroundLeftRight/UpDown`,
  en las dos leyes.
  > **CORREGIDO (27/09, C-7): NO ENTRA** — es §5.4(b), decidido NO. Se queda la zona muerta 85.
- **Ratón vertical con `m_fMouseAccelHorzntl`** (quirk fiel del mod) o se documenta la desviación.
  > **CORREGIDO (27/09, C-7): NO ENTRA** — es §5.4(d), decidido NO. Se queda `m_fMouseAccelVertical`
  > (`Cam.cpp:1811`, que es lo que ya se usa).
- **Sensibilidades por eje** `RightAnalogStickSensitivityX/Y` multiplicando los offsets.
- **`Process_FollowPed`/`Process_FollowPedWithMouse`**: reescritas desde `CamNew.cpp:52-231`
  (`PedZoomValueScript` en VC, `modernCamera` offset, `heightOffset 0.4`, agua `nivel+0.6`).
  **El `modernCamera` y el agua son en sí mismos dudosos** (el offset de la derecha va en sentido
  contrario al hombro) → punto a decidir (§5.4).
  > **CORREGIDO (enriquecimiento 27/09, C-14): esta viñeta SE CAE.** No se reescriben las leyes de
  > a pie. Motivo verificado: `Process_FOVLerp` sólo actsúa si `doFovChanges` es cierto, y eso sólo
  > lo pone `Process_AimWeapon` (`CamNew.cpp:240`) y lo consume en `:497`; en la ley de a pie la rama
  > viva es la `else` (`CamNew.cpp:491-492`), cuyo destino es `maxFOV = 70.0f` (`:26`) — y
  > `maxFOVModern` también vale `70.0f` (`:27`), o sea que `modernCamera` es un **no-op de FOV**.
  > `DefaultFOV` en nuestro árbol ya es `70.0f` (`Camera.h:29`, puesto en `Cam.cpp:1367`). Además
  > `modernCamera` está descartado (§5.4c) y la cámara la validar el jugador en `ve65`-`ve75`.
  > Lo único que sí entra de las leyes de a pie son **dos multiplicadores** por eje (B3) y, si el
  > jugador lo reaffirma, el agua.
- **Agua de la ley de coche**: `Cam.cpp:2710-2718` pasa de `z = nivel` a `z = nivel + 0.6` con
  `nearClip 0.2` y recolocación por `Magnitude2D()`, como `CamNew.cpp:215-220`.
- **`C3 using3rd=false`**: `CCam::Using3rdPersonMouseCam()` deja de decidir entre
  `Process_FollowPed` y `Process_FollowPedWithMouse` **mientras se apunta**.
  > **CORREGIDO (27/09, C-15): SE CAE, es AUTOMÁTICO y no hay que tocar nada.** Mientras la ley esté
  > activa el modo **no es `MODE_FOLLOWPED`**, es `MODE_AIMING`, y
  > `CCam::Using3rdPersonMouseCam()` es `CCamera::m_bUseMouse3rdPerson && Mode == MODE_FOLLOWPED`
  > (`Cam.cpp:1131-1134`): ya devuelve `false`. El reparto follow/mouse de `Cam.cpp:383-395` no se
  > puede alcanzar en `MODE_AIMING` porque su `case` es `MODE_FOLLOWPED`. Lo que sí hay que hacer es
  > el `case MODE_AIMING` en el mismo `switch`.

### 4.2 `src/core/Camera.cpp`

- **`Find3rdPersonQuickAimPitch`** con la fórmula del mod (`CamNew` la reimplementa en
  `Main.cpp:1464-1472`): `-(atan2(tan(FOV*0.5*0.01403292f) * (0.5f-multY + 0.5f-multY) * (1/ar), 1) + Alpha)`.
  Hoy usamos `1.8f*0.5f` (`Camera.cpp:4249-4257`): **cambia el pitch del brazo al**.
- `m_f3rdPersonCHairMultX/Y` se leen del ajuste (§2.1) en vez de las constantes `0.53/0.4` de `Camera.cpp:283-284`.
- **Near-clip dual** (`Camera.cpp:289-295`): los dos valores siguen siendo `DEFAULT_NEAR` ⇒ hoy es un
  no-op. Se deja así salvo decisión (§5.6); el GeniusZ es otro carril.

### 4.3 `src/renderer/Hud.cpp`

- **`DrawCrosshair`**: la caja de **±14 px** en `multX/multY` **sólo** en las condiciones exactas del
  mod (`:774-777`), sustituyendo la caja por arma de `Hud.cpp:389-424`. Con lock-on **no** se dibuja.
- **`DrawAutoAimTarget`**: las dos marcas del mod, con `timeLockOn = 250 ms`, `dist = w/128`,
  `rotMult` 0.5/3.0, color por salud y negro al morir, y `CalcScreenCoors` sobre la cabeza del ped
  (`PedIK` bone 1) + `z += 0.25`. Sustituye el marco de 4 rects de `Hud.cpp:541-571`.
  `LockOnTargetType = 0` ⇒ no dibuja nada (como `Main.cpp:784`).
- **`DrawTriangleForMouseRecruitPed`**: el triángulo sobre el objetivo blando, `alpha 150`.
- **Geometría**: `DrawSATarget`/`DrawLCSTarget`/`DrawSATriangleForMouseRecruit` **no están** en las
  fuentes (§1 hueco 2). Se dibujan **por código** con `CSprite2d::DrawRect`/`Draw` como ya se hace en
  `Hud.cpp` (regla de casa: «si hace falta una textura mejor se dibuja por código»), documentando que
  la forma es **reimplementación**, no copia.
- Los parches de estado de render del mod (`:756-761`/`:787-792`) se traducen al estado de
  `CSprite2d` que ya usa nuestro `Hud.cpp`.

### 4.4 `src/peds/PlayerPed.cpp`

- **`ViceExtProcessPlayerPedControl(CPlayerPed*)`**: el `ProcessPlayerPedControl` del mod
  (`:1203-1461`) sin su parte de GTA3. Debe corre en el mismo punto del frame que el
  `onProcessingPlayerControl.before` del mod: **el principio de `ProcessPlayerControl`**.
  > **CORREGIDO (enriquecimiento 27/09, C-2): `CPlayerPed::ProcessPlayerControl` NO EXISTE en
  > nuestro árbol** (búsqueda: 0 resultados en `src/`). La función equivalente, y la que corresponde
  > al `0x53739F` del mod, es **`CPlayerPed::ProcessControl` (`src/peds/PlayerPed.cpp:4283-4667`)**.
  > Punto de inserción exacto: **inmediatamente después de la llave de apertura (línea 4284)**, antes
  > del bloque `m_nEvadeAmount` (4287) y por tanto **antes** de `CPed::ProcessControl()` (4321).
  > Razón de esa posición, verificada: (a) es el literal `.before` del mod; (b) `CWorld::Process()`
  > corre en `Game.cpp:1336` y `TheCamera.Process()` en `Game.cpp:1349`, así que el `TakeControl` de
  > este frame llega a la cámara en el **mismo** frame; (c) `AimGun()` —que es quien escribe
  > `m_pedIK.m_torsoOrient/m_lowerArmOrient` (`Ped.cpp:2693` → `Ped.cpp:1271` `PointGunInDirection`)—
  > corre **después** (dentro de `CPed::ProcessControl()`), con lo que los 3 `MoveLimb` del mod se
  > leer como objetivos y el motor los re-deriva con los mismos dos números que el mod acaba de
  > escribir (`m_fLookDirection` vía `SetLookFlag`, `m_fFPSMoveHeading`): es cooperación, no pelea;
  > (d) ninguna de las 4 salidas tempranas de la función (`4324` `bWasPostponed`, `4363` `PED_DEAD`,
  > `4369` `PED_DIE`, `4443` `m_objective`) se salta el punto de inserción, así que la devolución de
  > la cámara (§3.1, última línea) se ejecuta siempre.
- **`IsAbleToAim` / `IsType1stPerson` / `IsWeaponPossiblyCompatible` / `IsTypeMelee` / `IsTypeTwoHanded`**
  (`:605-742`) con el mapa de flags de §3.3. **Decidir** lo de mutar `m_Flags` (§3.3).
- **Toma y devolución del modo** (`:1237-1244` / `:1417-1425`) con `SwitchTransitionSpeed`,
  `previousHor/VerAngle` y `previousCamMode` — sin esto la transición a `MODE_AIMING` da un salto.
- **Lock-on**: `front` al objetivo, `height` recalculado desde la posición en pantalla, y la regla de
  suelta (`ratón > 1.0` o `dist < 0.5`).
- **Rotación e IK**: `RotatePlayer` + `SetLookFlag(f, true, true)` + `SetAimFlag(f)` +
  `m_fFPSMoveHeading` clamp ±45° + `torsoPitch` + **los 3 `PedIK.MoveLimb`** (cabeza con su yaw,
  torso `0/torsoPitch`, brazo inferior `0/FPSMoveHeading`).
- **`Find3rdPersonMouseTarget`** (`:1474-1517`): `Find3rdPersonCamTargetVector(info->m_fRange, ...)`
  + LOS; si es ped, no es el jugador, no está muerto → `CanSeeEntity(ped, 2*60°)` →
  `ReactToPointGun(ped)` + `Say(117)`. Es la adquisición que hoy falta y sin la cual no hay triángulo.
- **`WalkKey`**: `m_fMoveSpeed = 0` mientras el ajuste esté pulsado.
- **C1/C2**: la decisión `TYPE_STRAFE` vs `TYPE_WALKAROUND` en el equivalente de `GetMoveAnimTaskType`
  (revisar el H4 actual, `odAimWalkActive/Uncapped`, que se declaró «YA ESTABA» con otra semántica).
- **C8**: `ProcessWeaponSwitch` no cambia de arma mientras `isAiming`.
- **C10**: al disparar, la cámara es `MODE_FOLLOW_PED` durante `CWeapon::Fire` (arregla *mvl* y
  parabrisas rompibles). Ojo: **toca `Weapon.cpp`**, que es el carril del recoil.
  > **CORREGIDO (enriquecimiento 27/09, C-3): NO hace falta tocar `Weapon.cpp`, luego el carril del
  > recoil no se pisa en absoluto.** `CWeapon::Fire` (`Weapon.cpp:596`) tiene exactamente **4**
  > callsites en todo el árbol: `src/peds/PedFight.cpp:875`, `:919`, `src/peds/PlayerPed.cpp:978`,
  > `:1118`. El `MODE_FOLLOW_PED` temporal se pone y se quita **alrededor de esas 4 llamadas**, que es
  > el equivalente funcional del par `onFiring.before/after` del mod (`Main.cpp:159-170`): el
  > `before` del SDK también corre sobre los `return` falsos, y aquí el `after` es la línea siguiente
  > al `Fire`, que se ejecuta igual porque ninguna de las 4 está dentro de una rama condicional.
  > Además `Weapon.cpp` no es el único conflicto: `PedFight.cpp:875/919` es carril de agachado (R28).

### 4.5 `src/peds/PedFight.cpp` — C9 y los cinco C18 (decidido el 27/09)

- C9: el spray suave devuelve `0.00001f` agachado / `-1.0f` si no.
- **C18-1** (`Main.cpp:408-420`): en `SetPointGunAt`, si el arma **no** tiene `WEAPONFLAG_CROUCHFIRE`
  → `bCrouchWhenShooting = false` + `RestorePreviousState()`. **Choca de frente** con
  `CPed::ViceExtCrouchShooting()` de R28 (`crouch2`), que hizo lo contrario. Resolucion propuesta en
  §5.5: aplicar el del mod y medir la perdida de los `*_crouchfire` sin flag.
- **C18-2** (`:422-429`): al limpiar el agachado → `ClearPointGunAt()`.
- **C18-3** (`:432-441`): al encontrar **nuevo** fijado, si no agachado (o con `CrouchFire`) →
  `bCrouchWhenShooting = true`. Va en `PlayerPed.cpp`, no aqui.
- **C18-4** (`:1393-1415`): al soltar el apuntado con municion y sin recargar →
  `bCrouchWhenShooting = true` + `BlendAnimation(ANIM_GROUP_MAN, ANIM_WEAPON_CROUCH, 4.0f)` +
  `SetDuck(60000, 1)`. **El que mas se nota.** Va en `PlayerPed.cpp`.
- **C18-5** (`:1368-1378`): el assoc del arma entra con `m_fBlendAmount = 0` y `m_fBlendDelta = 8.0f`
  en vez de `AddAnimation` (lenta).
- **C18-6**: **NO se porta** (bug del mod en su rama de VC: apuntaria con `ANIM_UNARMED_PUNCHR`).
  Documentado en §5.5.

### 4.6 `src/core/Pad.cpp` / `ControllerConfig.cpp` / `re3.cpp` / `config.h`

- `WalkKey` como acción de teclado configurable.
- Sensibilidades por eje del stick derecho (ajuste, no constante `0.01f`).
- Bloque de ajustes: `ClassicAxisEnabled`, `CameraCrosshairMultX/Y`, `LockOnTargetType`,
  `ShowTriangleForMouseRecruit`, `ForceAutoAim`, `StoriesAimingCoords`, `StoriesPointingArm`,
  `RightAnalogStickSensitivityX/Y`, `WalkKey` — leídos y guardados como el resto de preferencias
  (`re3.cpp:505/614` es el patrón de las existentes).
  > **CORREGIDO (enriquecimiento 27/09, C-4, C-5, C-16).**
  > **(a) Las líneas del patrón son otras.** El patrón real de **lectura** es `re3.cpp:504-508`
  > (`ReadIniIfExists(cat, key, ptr)`) y el de **escritura** es `re3.cpp:613-616` (`StoreIni(cat, key,
  > valor)`), ambos con el mejor ejemplofloat en `HorizantalMouseSens` /
  > `TheCamera.m_fMouseAccelHorzntl` (`:505` lee, `:614` escribe) y el mejor ejemplo bool en
  > `DisableMouseSteering` / `CVehicle::m_bDisableMouseSteering` (`:507` lee, `:616` escribe). No
  > existen las líneas `505/614` como «el patrón» porque `505` es la lectura y `614` la escritura de
  > la **misma** clave: el plan se confundió. **Ojo, trampa real:** las sobrecargas de `StoreIni`
  > (`re3.cpp:285-323`) son `uint32/uint8/int32/int8/float/char*` — **no hay `bool`**. Un bool se
  > guarda por la sobrecarga `int32` (promoción implícita, que es justo lo que hace `:616`) y se lee
  > con la `bool*` de `re3.cpp:230`.
  > **(b) `StoriesAimingCoords` NO lleva clave INI.** §9 dice que no se implementa (y §5.4 lo
  > confirma); una clave que no hace nada es ruido. Se documenta, no se expone.
  > **(c) La lista real son 9 escalares + 1 binding, no 10 ajustes.** Ver B0.
  > **(d) `config.h`:** el bloque es el de los `VICEEXT_*` en `config.h:382-427` (el comment de
  > cabecera es `config.h:378-381`). El plan dice «lo pide el dueño del fichero, sección 1»: §1 de
  > `.agents/AGENTS.md` sigue con `[Fill in]` (perfil sin rellenar), así que esa vía no resuelve nada →
  > se aplica la **práctica observada** (definir en el bloque `VICEEXT_*` con el comentario de la
  > convención: clave del `features.ini`/INI del mod entre paréntesis + sección de la tanda).
- `config.h`: un solo define `VICEEXT_AIM_CLASSICAXIS` (lo pide el dueño del fichero, sección 1).

### 4.7 Verificador, marcas y documentos

- `gta_vc_browser/tools/viceext-log-check.py`: bloque nuevo `AX` con `PASS/FAIL/INCONCLUSIVE` por
  criterio, y **`INCONCLUSIVE` si el log no trae `aim=1`** (build vieja nunca pasa).
- `gta_vc_browser/tools/check-served-build.sh`: marcas de la ley nueva.
- `docs/mods/ATTRIBUTION.md`: **corregir la fila falsa** «Apuntado ClassicAXIS+GeniusZ → PORTADO
  (`ve58`)» y la de `ve59`; este plan es quien las reescribe con la verdad.
- `gta_vc_browser/web/lib/index.js`: `VERSION` en cada build. **`dataTag` NO sube**: aquí no hay
  cambios de datos (ni clips, ni `weapon.dat`, ni `ped.ifp`).

---

## 5. Los 7 puntos que chocan con lo que ya está — **TODOS DECIDIDOS el 27/09**

### 5.1 La ley de coche no es del mod — **DECIDIDO 27/09: OPCION (a)**

**Opcion (a): se queda nuestra y se corrige la atribucion.** `Process_Cam_On_A_String` sigue como
esta (el jugador la valido en `ve65`-`ve75`: delante al subir, Q/E, 1,5 s, no clavarse a 2,00 m).
Lo que cambia es la **verdad documental**:

- `docs/mods/ATTRIBUTION.md` §6 deja de decir que la ley de coche viene de `CamNew.cpp`.
- Pasa a estado **propio del proyecto** (adaptacion de `Process_FollowPed` aplicada al coche por
  analogia, validada por el jugador), con la fila de `CamNew.cpp` limitada a lo que el mod tiene de
  verdad: `Process_FollowPed` y `Process_AimWeapon`, **ambos a pie**.
- El unico punto del coche que si viene del mod y se corrige: el **agua** (near-clip 0.2 +
  `z = nivel + 0.6` + recolocacion por `Magnitude2D`, `CamNew.cpp:215-220`) frente a nuestro
  `z = nivel` a secas (`Cam.cpp:2710-2718`).
- El auto-centrado (contrato de 1,5 s) se conserva **intacto**.

### 5.2 `ForceAutoAim` — **DECIDIDO 27/09: OPCION (a), FIEL AL MOD**

Con teclado+raton el mod **no** tiene auto-aim. Se porta literal, con estas tres consecuencias
concretas (`Main.cpp:330, 475, 1250, 1277-1280`):

1. **No se busca fijado** nunca con raton. Se elimina la llamada a `FindWeaponLockOnTarget()` del
   camino de automatico; el cambio de fijado manual (`ShiftTargetLeft/RightJustDown`, `Pad.cpp:3267/3275`)
   **se conserva**, que es como se fija uno a mano en el mod.
2. **El fijado se suelta** en cuanto el raton se mueve mas de `1.0` (`abs(mouseX) > 1.0f || abs(mouseY) > 1.0f`),
   o si el objetivo queda a `dist < 0.5 m`. Traza `AIMLOCK off=raton|cerca`.
3. **`DrawAutoAimTarget()` no se llama** con raton: la marca de lock-on **desaparece**. El
   `LockOnTargetType` sigue Having efecto sobre el *aspecto* cuando hay fijado manual.

Con mando (`hasPadInHands`) el comportamiento es el de serie: busca y mantiene. El ajuste
`ForceAutoAim` queda expuesto para poder volver al de serie con raton sin build.

**Aviso al jugador antes de su ronda:** esto **elimina una funcion que hoy existe** (el auto-aim
automatico con raton). Es lo que pidio el mod, pero es una perdida de comodidad a proposito.

### 5.3 FOV 50 al apuntar un rifle (`zoomForAssaultRifles`) — **DECIDIDO 27/09: ACTIVADO**

El jugador lo activa. Al apuntar un rifle de alcance >= 70 el FOV interpola de 70 a **50** y vuelve
a 70 al soltar (`interpF` a `0.1*ts`, `CamNew.cpp:477-498`). Minigun excluido (quirk del mod en VC).

*Lo que no sabemos y no se inventa:* el **default** del mod para `zoomForAssaultRifles` (falta
`Settings.h`). Como el jugador lo quiere **activo**, se implementa con el umbral del propio mod
(`wepMinRange = 70`, `minFOV = 50`) y ademas queda **exponerlo como ajuste** para poder apagarlo sin
build. Decision de ejecucion: default = activo.

### 5.4 Fidelidades — **DECIDIDO 27/09: SOLO (a)**

| # | Cambio | Decisión |
|---|---|---|
| **a** | **Hombro en espacio de OBJETO** (`CamNew.cpp:265`) en vez de espacio de camara (`Cam.cpp:1386/1782/5714`) | **SE PORTA.** Es el corazon del disparo en tercera persona: el ped deja de deslizarse de lado en pantalla y la camara orbita un punto de su hombro |
| **b** | Stick derecho **crudo** (`-NewState.RightStickX`, `CamNew.cpp:131`) en vez de `LookAroundLeftRight()` con zona muerta 85 (`Pad.cpp:3524-3557`) | **NO.** Se queda la zona muerta actual |
| **c** | `modernCamera`: objetivo a pie 0.25 m a la izquierda + 0.075 arriba (`CamNew.cpp:77-80`) | **NO.** Se queda el objetivo centrado. Ademas no sabemos el default del mod (falta `Settings.h`) |
| **d** | Ratón vertical con el acelerador **horizontal** (`CamNew.cpp:150, 332`) — typo del mod | **NO.** Se queda `m_fMouseAccelVertical` |
| **e** | `ForceCameraBehindPlayer()` → recentrar con boton (`CamNew.cpp:173-176`) | **NO.** El boton se quito en `ve65` por decision del jugador. El recentrado pasivo de 1,5 s (intocable) intacto |

**Consecuencia de (a) que hay que vigilar:** el hombro pasa a depender de la matriz del ped, asi que
`ViceExtAimShoulderSmoothed` (el `WellBufferMe(0.2, 0.1)` de `Cam.cpp:647-660`) sigue igual pero el
punto de aplicacion cambia. Y `s_odAimStoriesShoulder` (rama 0.55, `Cam.cpp:604-610`) **sigue muerta**:
`StoriesAimingCoords` se deja como no implementado y se documenta, porque con (b)-(e) descartadas no
tiene un conmutador natural y `Cam.cpp:600-602` sigue sin respuesta del jugador.

### 5.5 Los fixes de agachado (C18) — **DECIDIDO 27/09: SE APLICAN** (punto a punto)

El jugador los quiere dentro de este plan, con la lista explicada. Origen:
`classicaxis_Main.cpp:406-441` y `:1393-1415`. **Los cinco que aplican en VC:**

| # | Qué hace el mod | Ancla | Qué hay que tocar | Conflicto |
|---|---|---|---|---|
| **C18-1** | Apuntando agachado, si el arma **NO** tiene `WEAPONFLAG_CROUCHFIRE` -> apaga `bCrouchWhenShooting` + `RestorePreviousState()` | `:408-420` | `PedFight.cpp`, el gate de `SetPointGunAt` | **DIRECTO**: R28 (`crouch2`) hizo justo lo contrario con `CPed::ViceExtCrouchShooting()`. Los dos no pueden ser verdad a la vez |
| **C18-2** | Al levantarse del agachado -> `ClearPointGunAt()` + `wasCrouching = false` | `:422-429` | `PedFight.cpp` | ninguno |
| **C18-3** | Al **cambiar de fijado** -> si no esta agachado (o el arma tiene `CrouchFire`), reactiva `bCrouchWhenShooting` | `:432-441` | `PlayerPed.cpp` (`FindWeaponLockOnTarget`) | ninguno |
| **C18-4** | Al **soltar el apuntado** agachado, con municion y sin recargar -> `bCrouchWhenShooting = true` + `BlendAnimation(ANIM_GROUP_MAN, ANIM_WEAPON_CROUCH, 4.0f)` + `SetDuck(60000, 1)` (**te quedas agachado tras disparar**) | `:1393-1415` | `PlayerPed.cpp` | **ALTO**: solapa con la logica de agachado de R28. Es el que mas se nota en juego |
| **C18-5** | El assoc del arma entra con `m_fBlendAmount = 0` + `m_fBlendDelta = 8` (entrada rapida) en vez de `AddAnimation` (lenta) | `:1368-1378` | `PedFight.cpp` | ninguno |

**C18-6 = DESCARTADO, y hay que decirlo en el informe.** En su rama de VC el mod apunta al ped al
clip `ANIM_UNARMED_PUNCHR` (el puñetazo) mientras dispara (`Main.cpp:1355-1357`); en GTA3 usa el
correcto. Es un **bug del mod en su rama de VC**, no una decision de diseno: portado tal cual, el
jugador veria un puñetazo en vez de la pose de apuntado. **No se porta** salvo instruccion expresa.

**El conflicto C18-1 es el unico que necesita una regla, no una eleccion binaria.** R28 (`crouch2`)
creo `CPed::ViceExtCrouchShooting()` para que el motor use los clips `*_crouchfire` que **si**
existen en los `.ifp` servidos y en el `weapon.dat`. C18-1 dice lo contrario para armas sin
`CrouchFire`. resolucion propuesta (a confirmar): **aplicar C18-1 literal** (es la spec del mod y la
regla §14 es que el mod manda) y **anotar la perdida** de los `*_crouchfire` en armas sin el flag,
midiendo si el jugador lo echa de menos en su ronda de agachado. El resto (2, 3, 4, 5) no chocan.

### 5.6 Near-clip dual — **DECIDIDO 27/09: SE QUEDA COMO ESTA**

`ViceExtNearClipOnFoot` e `ViceExtNearClipInCar` (`Camera.cpp:289-295`) siguen valiendo `DEFAULT_NEAR`
(0,9): no hay separacion y **este plan no la crea**. Es requisito del **GeniusZ** (el otro mod del
item 4), que tiene su propia fuente (`mods/1487678468_firstperson/`) y su propio carril. Se documenta
el no-op para que no se lea como un olvido.

### 5.7 Los intocables — **DECIDIDO 27/09**

- **Auto-centrado de camara: INTACTO.** El contrato de `camara-coche-sin-lucha.md` (mientras se mira no
  recentra nada; 1,5 s de quietud → `WellBufferMe(TargetOrientation, 0.1/0.06)`; la radio no recentra;
  vale parado) no se toca. Se documenta que `CamNew.cpp:173-185` (que solo recentra con boton) es
  **distinto** y queda fuera por decision de §5.4(e).
- **Recoil: MATEMATICA INTACTA + un callsite nuevo.** No se toca `recoil-por-arma-y-cadencia.md`.
  Pero el recoil se aplica desde dentro de funciones de camara concretas (`Cam.cpp:4100-4102` y
  `:4161-4171` en `Process_Syphon`, `:1820-1822` en `Process_FollowPedWithMouse`), y al apuntar la
  camara pasa a ser `Process_AimWeapon`: **sin callsite, apuntar dejaria de mover la camara con el
  recoil**. Se anaden las mismas dos llamadas (`ViceExtRecoilBegin` / `ViceExtRecoilApply`) en la
  ley nueva, con los mismos limites de Alpha que usa `Process_Syphon` (`-PI`/`+PI`, porque la ley de
  apuntado tiene clamp ±50 y no +60/-89,5). Criterio de no-regresion en §8.13.
  > **CORREGIDO (enriquecimiento 27/09, C-10): las líneas son `Cam.cpp:4162` y `Cam.cpp:4170`** (no
  > `4100-4102`/`4161-4171`), y las de `Process_FollowPedWithMouse` son `Cam.cpp:1844-1847` (no
  > `1820-1822`). Firma real (`src/weapons/Weapon.h:45-47`):
  > `static void ViceExtRecoilBegin(float &alpha, bool reset, int32 mode, const char *source);` y
  > `static void ViceExtRecoilApply(float &alpha, float manualDeltaRad, float inputY, const char
  > *source, int32 mode, float minAlpha, float maxAlpha);`. `Process_FollowPedWithMouse` **ya** pasa
  > `±50°` cuando el hombro está activo (`Cam.cpp:1846-1847`), así que el callsite nuevo es una copia
  > literal del de `Process_Syphon` con `"aim-weapon"` como `source`. Detalle literal en B6.

---

## 6. Restricciones (reglas de casa)

- **El mod manda** (`12-handoff` §14): se reescribe desde la spec, no se parcha encima de lo existente.
  **Intocables**: auto-centrado de cámara y recoil (matemática).
- **Sin LICENSE** ⇒ reimplementación con atribución en cabecera. **Nunca** copia literal.
- **Nunca**: direcciones x86, `plugin::patch`, `Nop`, `RedirectCall`, `hook::pattern`, `plugin-sdk`,
  `MemoryModule`, DLL/ASI, memoria cruda en WASM, `bForceLegsMovements=1`, modos IV, acoplo GInput,
  inventar defaults de un INI que no vino.
- **`.cpp` = CRLF**: editar con python binario, `assert data.count(old) == 1` por reemplazo.
  Consola Windows cp1252 ⇒ `PYTHONIOENCODING=utf-8`.
- **`VERSION` sube en cada build**; `dataTag` **no** sube (este plan no toca datos).
- Antes de pedir partida: `check-served-build.sh` en verde.
- **Medición = jugador + log.** Nada de arnés headless para el apuntado.
- **Sin commits** ni staging amplio.
- **No tocar** el carril de nado (`PlayerPed.cpp` está en uso) ni el carril de agachado sin coordinar.
- **Alcance cerrado por §5**: solo entra lo decidido. Nada de lo que el jugador descartó (§5.4b-d-e,
  §5.6, `StoriesAimingCoords`, `modernCamera`).
- Escritura permitida: `src/`, `gta_vc_browser/tools/`, `gta_vc_browser/web/`, `docs/mods/`,
  `.agents/plans/`.

---

## 7. Pasos ejecutables

1. **Resolver el bloqueo de propiedad** (§3.2): cerrar el carril de nado o acordar el reparto de
   `PlayerPed.cpp`. **Sin esto no se pasa de aquí.**
2. **Ajustes primero** (`config.h`, `re3.cpp`, `Pad/ControllerConfig`): la tabla de §2.1 leída del INI
   con los 10 valores por defecto del `ini` de 2022, más `ForceAutoAim=false` (§5.2a) y
   `ZoomForAssaultRifles=true` (§5.3a). Sin esto nada más es medible.
3. **`Camera.h` + `Cam.cpp`**: revivir `MODE_AIMING`; `Process_AimWeapon`, `Process_FOVLerp`,
   `Process_CrouchOffset`, `Process_AvoidCollisions`; **hombro en espacio de objeto (§5.4a, lo único de
   §5.4 que entra)**; sensibilidades por eje; agua `nivel+0.6`; `using3rd=false` mientras se apunta.
   **NO entran** (§5.4): stick crudo, `modernCamera`, acelerador horizontal del ratón, botón de recentrar.
4. **`Camera.cpp`**: `Find3rdPersonQuickAimPitch` con la fórmula del mod (`0.01403292f`);
   `m_f3rdPersonCHairMultX/Y` desde el ajuste.
5. **`PlayerPed.cpp`**: `ViceExtProcessPlayerPedControl` + los predicados (`IsAbleToAim`,
   `IsType1stPerson`, `IsWeaponPossiblyCompatible`, `IsTypeMelee`, `IsTypeTwoHanded`) + toma/devolución
   del modo con `previousHor/VerAngle` + lock-on (con la suelta por ratón > 1.0 o `dist < 0.5` de
   §5.2) + `RotatePlayer`/`SetLookFlag`/`SetAimFlag`/`m_fFPSMoveHeading` + **los 3 `PedIK.MoveLimb`**
   + `Find3rdPersonMouseTarget` + `WalkKey` + C1/C2/C8 + **C18-3 y C18-4**.
   Con §5.2(a): `FindWeaponLockOnTarget` **solo** por `ShiftTargetLeft/RightJustDown`.
6. **Recoil**: el callsite `ViceExtRecoilBegin/Apply` dentro de `Process_AimWeapon` (§5.7), con los
   límites `-PI/+PI` de `Process_Syphon`. Sin tocar la matemática. Con prueba de no-regresión.
7. **`Hud.cpp`**: `DrawCrosshair` (caja ±14 px, sólo sin fijado), `DrawAutoAimTarget` (SA/LCS,
   `timeLockOn` 250 ms, `rotMult` 0.5/3.0, color por salud) y `DrawTriangleForMouseRecruitPed`.
   Geometría dibujada por código (`DrawSATarget`/`DrawLCSTarget`/`DrawSATriangleForMouseRecruit` no
   están en las fuentes).
8. **`PedFight.cpp`**: **C9** (spray `0.00001f`/`-1.0f`) y **C18-1, C18-2, C18-5**. Coordinado con el
   carril de agachado por el choque de C18-1. **C18-6 no se porta.**
9. **Compilar objeto** (`ninja …/core/Cam.cpp.o`, `…/core/Camera.cpp.o`, `…/peds/PlayerPed.cpp.o`,
   `…/peds/PedFight.cpp.o`, `…/renderer/Hud.cpp.o`) — cada comando con su autorización independiente.
10. **Verificador + marcas**: bloque `AX` en `gta_vc_browser/tools/viceext-log-check.py` con
    `PASS/FAIL/INCONCLUSIVE` y **`INCONCLUSIVE` si el log no trae `aim=1`** (build vieja nunca pasa);
    marcas nuevas en `gta_vc_browser/tools/check-served-build.sh`.
11. **Build** con `VERSION` nueva → `check-served-build.sh` → avisar con el criterio PASS de §8.
12. **Sesión del jugador** → log → verificador. Ronda por ronda: (1) la ley de apuntado y el hombro,
    (2) el HUD, (3) los ajustes que elija cambiar, (4) los fixes de agachado.
13. **Documentar**: `docs/mods/ATTRIBUTION.md` (corregir las dos filas falsas: la del `ve58` y la del
    `ve59`/§5.1), este plan, y el cierre con la memoria persistente.

---

## 8. Verificación — criterio PASS (definido ANTES de que juegue)

1. **Ley de apuntado**: al pulsar apuntar la cámara entra en `MODE_AIMING` con `dist 2.70` y
   `altura = 0.25 + m_fSyphonModeTargetZOffSet + 0.05`. Traza `AIMCAM m=5 dist=2.70 alt=`.
2. **Hombro en espacio de objeto (§5.4a)**: con el cuerpo girado, el hombro sigue al cuerpo — prueba:
   girar 180° sin tocar el ratón y el hombro cambia de lado en pantalla. Traza
   `AIMCAM hombro=0.20 obj=1 ex= ey= ez= lado=`.
3. **Fidelidad de espacio**: el punto de hombro se valida contra `TransformFromObjectSpace`
   (posición + ejes del ped) en la propia traza, no a ojo.
4. **Retícula sobre el hombro**: la mira de 3.ª persona cae sobre el hombro, no detrás de la cabeza
   (es la razón del offset; con la ley nueva se comprueba en partida y en el log, no se supone).
5. **Clamps**: `|Alpha| <= 50°` durante el apuntado; el verificador falla si el máximo de la sesión lo
   pasa (`AIMCAM amax=`).
6. **FOV (§5.3a)**: apuntando rifle de alcance >= 70 el FOV baja a 50 y vuelve a 70 al soltar
   (`AIMFOV fov= arma= alcance=`); el Minigun **no** cambia de FOV.
7. **Lock-on**: con fijado manual la cámara compensa el `CrosshairMult` (`horShift`/`verShift`), la
   bala va a la retícula y el objetivo queda en la retícula. La marca de salud aparece 250 ms y se va.
   Traza `AIMLOCK hx= vx= tmo=250 off=`.
8. **Triángulo del ratón**: apuntando con ratón sobre un ped vivo, triángulo sobre su cabeza con el
   color de su salud; desaparece al soltar el gatillo o al morir (`AIHTRI h=1/0`).
9. **Sin auto-aim con ratón (§5.2a)**: pulsando apuntar **no** aparece ningún fijado solo; el fijado
   sólo sale con `ShiftTargetLeft/RightJustDown`; mover el ratón > 1.0 lo suelta
   (`AIMLOCK solo=0 off=raton`). Con mando, el auto-aim de serie sigue funcionando.
10. **Sin fight cam**: daño cuerpo a cuerpo mientras se apunta no cambia la cámara de modo
    (`AIMCAM fight=0`).
11. **Cambio de arma bloqueado** con el aimed activo (`AIMWPN block=1`).
12. **Colisiones**: las 5 esferas y el rayo se comportan como el mod; un ped a <0.5 m se oculta un
    frame y vuelve (`AIMCOL pedes= dist= app=`); sin atravesar paredes en ningún lado.
13. **SIN REGRESIÓN DEL RECOIL (intocable, §5.7)**: ráfaga con SMG → la cámara sube a escalones y se
    queda arriba; `multY = 0.400` constante; `RECOIL3` y el bloque `RC` del verificador dan lo mismo
    que antes del cambio. **Si esto falla, el plan no pasa.**
14. **SIN REGRESIÓN DEL AUTO-CENTRADO (intocable)**: en coche, mover el ratón y parar → nada empuja
    mientras se mira; ~1,5 s después la cámara vuelve sola; la radio no la mueve
    (`camauto2 auto=1 pedido=0`).
15. **SIN REGRESIÓN DEL AGACHADO**: los clips `GunCrouchFwd/Bwd` siguen sirviendo; la rueda y el
    disparo agachado no se rompen; se registra si con C18-1 se pierden los `*_crouchfire` en armas sin
    `CrouchFire` (esperado: es el coste de §5.5).
16. **SIN REGRESIÓN DEL NADO** (carril ajeno): `SWIM2`/`SWIMNAT` igual que en `swim8`.
17. **Build/datos**: `check-served-build.sh` OK con las marcas nuevas; `VERSION` nueva (base leída el
    27/09: `2026-09-26-swim9` en `web/lib/index.js:171`); **`dataTag` sigue `2026-09-26-ve14`**
    (`web/ondemand.js:125`); el verificador da `INCONCLUSIVE` (nunca `PASS`) con un log sin `aim=1`.

---

## 9. Lo que este plan NO cubre (y por qué)

- **GeniusZ / 1.ª persona** (near-clip dual, offsets por vehículo, FOV configurable, menú de
  calibración): es el otro bloque del ítem 4, con su propia fuente (`mods/1487678468_firstperson/`).
  Aquí solo se roza el near-clip dual, y **se queda como está** (§5.6).
- **WFP** (FOV/HUD/letterbox/`Radardisc`/menús/`Loading`): `PENDIENTE` con motivo en
  `docs/mods/ATTRIBUTION.md` §5 y §6. Spec en `10-plan-pasada-siguiente` B7. Carril aparte.
- **FV-MIT** (`*ms_fTimeStep`): ítem 5 del `12-handoff` §13, con su propio plan.
- **La geometría exacta de las marcas SA/LCS y del triángulo**: no está en las fuentes
  (`DrawSATarget`/`DrawLCSTarget`/`DrawSATriangleForMouseRecruit` se llaman pero no se bajaron).
  Se reimplementa por código y **se dice** en la cabecera de atribución.
- **Los defaults de `zoomForAssaultRifles`/`modernCamera`/`crouchKey`**: no están (`Settings.h` no se
  bajó). No se inventan: §5.3 lo resuelve activando el umbral del mod; `modernCamera` y `crouchKey`
  quedan **fuera** por §5.4(c) y por ser lógica del carril de agachado.
- **`ForceAutoAim` con mando** (`bForceManualAim`): `NO APLICA` en web (el pad es Gamepad API, sin el
  acoplo GInput del mod). Se documenta, no se implementa.
- **`StoriesAimingCoords`**: sin conmutador natural y sin respuesta del jugador
  (`Cam.cpp:600-602` sigue abierta), se deja **sin implementar** y documentado.
- **La ley de coche**: se queda como está (§5.1a). Solo se le corrige el agua y la atribución.

---

---

## 10. Enriquecimiento técnico (27/09)

> **Qué es esto y qué no es.** Esta sección la escribió el subagente de Enriquecimiento leyendo el
> árbol real: los 4 ficheros de cámara, los 2 del ped, el HUD, el pad, la config, las 2 herramientas
> de verificación y las 3 fuentes del mod. **No se ha ejecutado nada** (RULES 0.4: ni build, ni test,
> ni script, ni juego) y **no se ha modificado ningún fichero salvo este plan**. Cada bloque lleva
> fichero + ancla exacta + acción + detalle + restricciones + verificación, como pide
> `.agents/subagents/enrichment-process.md` §3.
>
> **Alcance intacto.** No entra nada de lo que §5 descartó. Donde la spec del mod y una decisión
> cerrada chocan, manda la decisión (`12-handoff` §14: *el mod manda **salvo** lo decidido*).

### 10.0 Correcciones al plan original (con el motivo verificado)

Las 17 correcciones están aplicadas en su sitio, marcadas con `> **CORREGIDO (enriquecimiento 27/09,
C-n)**`. Resumen, porque siete cambian trabajo real:

| # | Afirmación del plan | Realidad verificada | Efecto |
|---|---|---|---|
| **C-1** | El hombro de `Cam.cpp:1386/1782/5714` va en «espacio de cámara» y por eso hay que cambiarlo | `CPlaceable::GetRight()` → `m_matrix.GetRight()` (`Placeable.h:20`) y `m_matrix` es miembro **de la entidad** (`Placeable.h:6`). El propio árbol lo dice en `Cam.cpp:1778-1781` | **La premisa de §5.4(a) es falsa.** Los 3 sitios no se tocan. La ley nueva usa la misma forma y da el mismo número. Aviso al jugador en §10.21-D1 |
| **C-2** | «el principio de `ProcessPlayerControl`» | Esa función **no existe** en `src/`. El equivalente es `CPlayerPed::ProcessControl` (`PlayerPed.cpp:4283`), inserción en `:4284` | Ancla corregida; el resto de B9 se mantiene |
| **C-3** | C10 «toca `Weapon.cpp`, que es el carril del recoil» | `CWeapon::Fire` tiene 4 callsites: `PedFight.cpp:875/919`, `PlayerPed.cpp:978/1118` | **Conflicto con el carril del recoil eliminado** |
| **C-4** | «`re3.cpp:505/614` es el patrón» | `505` es la lectura y `614` la escritura de la **misma** clave. Y `StoreIni` **no tiene** sobrecarga `bool` (`re3.cpp:285-323`) | Trampa de compilación evitada |
| **C-5** | §4.6 lista `StoriesAimingCoords` como ajuste | §9 lo declara no implementado | Sin clave INI (una clave que no hace nada es ruido) |
| **C-6** | §4.1 cambia el hombro en los 3 sitios | Consecuencia de C-1 | Se cae |
| **C-7** | §4.1 lista stick crudo y acelerador horizontal como trabajo | Son §5.4(b) y §5.4(d), decididos **NO** | Se caen |
| **C-8** | C4 (sin fight cam) y C5 (sin point-gun cam) son trabajo | `TakeControl` deja `m_bLookingAtPlayer=false` (`Camera.cpp:2354`) y eso **anula** todo `Camera.cpp:1729-1938`, que es donde se aplica `ReqMode` | **Dos de los 18 hooks no necesitan una línea de código** |
| **C-9** | C12 parchea el crosshair del juego | `Main.cpp:338-344` escribe un **byte de datos** (`0x5D5064+6`) | **NO APLICA** (§6 prohíbe tocar datos) |
| **C-10** | §5.7: callsites en `Cam.cpp:4100-4102` / `:4161-4171` | Son `Cam.cpp:4162` y `Cam.cpp:4170`; los de `FollowPedWithMouse` son `:1844-1847` | Citas corregidas |
| **C-11** | §8.17 `VERSION` sin valor | Hoy `2026-09-26-swim9` (`web/lib/index.js:171`) | Base de nombres de build en §10.20 |
| **C-12** | §3.3 no mapea `m_sHead`/`m_sTorso`/`m_sLowerArm` ni el «bone 1» | Son `m_headOrient`/`m_torsoOrient`/`m_lowerArmOrient` (`PedIK.h:38/39/41`) y el bone 1 es `PED_HEAD` (`PedModelInfo.h:10`) | Tabla §3.3 ampliada en B9 y B11 |
| **C-13** | §4.4 C10 «Ojo: toca `Weapon.cpp`» | Ver C-3 | Corregido |
| **C-14** | §4.1 reescribe `Process_FollowPed` y `Process_FollowPedWithMouse` enteras | `Process_FOVLerp` en la ley de a pie sólo cae en la rama `else` cuyo destino es `maxFOV = 70.0f` (`CamNew.cpp:26`, `:491`), y `maxFOVModern` **también** vale 70 (`:27`) ⇒ `modernCamera` es no-op de FOV. `DefaultFOV` ya es 70 (`Camera.h:29`, puesto en `Cam.cpp:1367`) | **Dos leyes de ~1.100 líneas fuera de alcance.** Sólo entran 2 multiplicadores por eje |
| **C-15** | §4.1: `using3rd=false` mientras se apunta | `Using3rdPersonMouseCam()` exige `Mode == MODE_FOLLOWPED` (`Cam.cpp:1133`) | Automático, sin código |
| **C-16** | §4.6: `config.h` «lo pide el dueño del fichero, sección 1» | §1 de `.agents/AGENTS.md` sigue con `[Fill in]` ⇒ esa vía no resuelve. Se aplica la práctica: bloque `VICEEXT_*` (`config.h:382-427`) | Bloque B0 |
| **C-17** | §2.3 da C7 y el parche de `Main.cpp:323-325` por trabajo | Son parches de bytes sobre direcciones cuyo destino no se puede identificar con lo que hay en `tmp/extsrc/` | `PENDIENTE` con motivo, no bloquean |

---

### 10.1 Las 12 preguntas abiertas: respuesta corta

Las 12 se han resuelto con evidencia de fichero. El desarrollo, con el detalle, en §10.18.

| # | Pregunta | Respuesta | Bloque |
|---|---|---|---|
| 1 | ¿Dónde se engancha `ViceExtProcessPlayerPedControl`? | **`CPlayerPed::ProcessControl` (`PlayerPed.cpp:4283`), justo tras la llave `:4284`**, antes de `CPed::ProcessControl()` (`:4321`) | B9 |
| 2 | ¿Dónde el `case MODE_AIMING` y cómo se deja de enrutar a `MODE_SYPHON`? | `case` en `Cam.cpp:396` (ya está ahí, comentado). **No hay que tocar `CamControl`**: `TakeControl` deja `m_bLookingAtPlayer=false` (`Camera.cpp:2354`) y el `else` de `Camera.cpp:1939-1990` honra `m_iModeToGoTo` | B2, B7 |
| 3 | `TransformFromObjectSpace` | **No existe** (0 resultados en `src/`). Equivalente por construcción: `GetMatrix().GetPosition() + GetRight() * hombro`, numéricamente idéntico al del mod para un ped con sólo rotación Z | B5 |
| 4 | Near-clip | `Scene.camera` (`main.h:14-19`, `RwCamera *camera`) y `RwCameraGet/SetNearClipPlane`: **los mismos nombres** que el mod | B3 |
| 5 | `interpF` | No existe. Se usa la macro de casa `lerp(norm,min,max)` (`common.h:396`), idéntica a `a + (b-a)*t` | B3 |
| 6 | Ajustes | `ReadIniIfExists` (`re3.cpp:504-508`) y `StoreIni` (`re3.cpp:613-616`); define en el bloque `VICEEXT_*` (`config.h:382-427`) | B0 |
| 7 | `CPedIK::MoveLimb` y los 3 `m_s*` | `MoveLimb` ✓ (`PedIK.h:59`), los 3 `ms_*Info` ✓ (`PedIK.h:44/45/48`); los `m_s*` **se llaman distinto**: `m_headOrient`/`m_torsoOrient`/`m_lowerArmOrient` (`PedIK.h:38/39/41`) | B9 |
| 8 | Geometría de las 3 marcas | `CSprite2d::DrawRect` y `CSprite2d::Draw2DPolygon` (4 vértices) — el primo exacto de una marca rotada. **Reimplementación, no copia** | B11 |
| 9 | Mutar `CWeaponInfo` | **No**: la tabla es global y compartida por todos los peds y la IA (`WeaponInfo.cpp:72`, `:129-131`). Se expresa por **consulta** en un helper | B9 |
| 10 | Callsite del recoil | `Cam.cpp:4162` / `:4170`; firmas en `Weapon.h:45-47`; copia literal con `source` propio `"aim-weapon"` | B6 |
| 11 | Bloque `AX` del verificador | 6 criterios, 8 trazas, cadencia 1 Hz o por flanco | B14, B15 |
| 12 | Registro de conflictos | 9 pares fichero/región, 3 con mitigación obligatoria | §10.19 |

---

### 10.2 Bloques de trabajo

Orden de ejecución = orden de dependencia. **B0 y B1 no se pueden saltar**: sin ellos nada es medible
y los bloques de cámara no ven el estado.

---

#### B0 · Ajustes: `src/core/config.h` + `src/core/re3.cpp`

**Ficheros / anclas**
- `src/core/config.h` — bloque `VICEEXT_*`, líneas **382-427**; cabecera del bloque en 378-381.
- `src/core/re3.cpp` — lectura en `LoadINISettings`, dentro del bloque que empieza en `:504` (el
  sitio es justo después de `:508`); escritura en `SaveINISettings`, después de `:616`.

**Acción**: `edit` (los dos).

**Detalle técnico**

`config.h`, **un único define**, con el comentario de la convención de casa, insertado al final del
bloque `VICEEXT_*` (después de `VICEEXT_BREAKABLE_LIGHTS`, `config.h:426`, antes de la línea 428
comentada `TURN_SIGNALS`):

```
// ClassicAXIS (gennariarmando/DK22Pac, sin LICENSE): su seccion [ClassicAxis] del
// INI de 2022 (`mods/Classic AXIS/ClassicAxisVC.ini`, 10 claves) y la ley de camara
// propia `CamNew.cpp Process_AimWeapon` (seccion 2, item 4 del 12-handoff).
// Sin este define el motor NO compila ninguna de las 6 leyes ni las 3 funciones de
// HUD de ClassicAXIS (plan apuntado-classicaxis-100 §10.2, B0-B11).
#define VICEEXT_AIM_CLASSICAXIS
```

**La lista real son 9 escalares + 1 binding, no 10 ajustes.** §2.1 cuenta 10 claves del INI de 2022,
pero una es `StoriesAimingCoords` (§9: no se implementa) y otra es `WalkKey`, que no es un ajuste sino
un *binding* (→ B13). Los 9 escalares, con **nombre de clave verbatim del INI del mod** y default
**verbatim**:

| Clave INI (sección `ClassicAxis`) | Tipo | Default | Fuente del default | Sobrecarga de `re3.cpp` |
|---|---|---|---|---|
| `ForceAutoAim` | `bool` | `false` | `ClassicAxisVC.ini:3` | lee `bool*` (`:230`), escribe `int32` (promoción, como `:616`) |
| `LockOnTargetType` | `int32` | `1` | `ClassicAxisVC.ini:5` | `int32*` (`:241`) / `int32` (`:299`) |
| `ShowTriangleForMouseRecruit` | `bool` | `true` | `ClassicAxisVC.ini:7` | idem bool |
| `CameraCrosshairMultX` | `float` | `0.53f` | `ClassicAxisVC.ini:13` | `float*` (`:263`) / `float` (`:313`) |
| `CameraCrosshairMultY` | `float` | `0.4f` | `ClassicAxisVC.ini:14` | idem float |
| `StoriesPointingArm` | `bool` | `false` | `ClassicAxisVC.ini:16` | idem bool |
| `RightAnalogStickSensitivityX` | `float` | `1.0f` | `ClassicAxisVC.ini:18` | idem float |
| `RightAnalogStickSensitivityY` | `float` | `1.0f` | `ClassicAxisVC.ini:19` | idem float |
| `ZoomForAssaultRifles` | `bool` | `true` | **decisión §5.3(a)**, NO del INI: el INI de 2022 no la trae y su default está en el `Settings.h` que no se bajó | idem bool |

Los que **no** llevan clave, con el motivo escrito en el propio bloque para que no se lean como olvido:
`StoriesAimingCoords` (§9), `crouchKey` y `modernCamera` (§5.4c y carril de agachado), `bEnable` del
INI de 2017 (es el define de este bloque, no un ajuste).

Los 9 valores viven en el `struct` estático de B1. `CameraCrosshairMultX/Y` **no** se leen aquí: se
copian a `CCamera::m_f3rdPersonCHairMultX/Y` en B8, porque ese par ya existe y ya se inicializa en
`Camera.cpp:283-284`.

**Restricciones**
- `.cpp` = **CRLF**: se edita con **python binario** y `assert data.count(old) == 1` por reemplazo.
  Consola Windows cp1252 ⇒ `PYTHONIOENCODING=utf-8`.
- **Nunca** un default inventado de un INI que no vino (§6). `ZoomForAssaultRifles = true` es una
  decisión del jugador (§5.3a) y se dice así en el comentario, no disfrazado de default del mod.
- Sin `dataTag`: esto no toca datos servidos (§6).
- No tocar `config.h` fuera del bloque `VICEEXT_*`.
- Sección INI: `ClassicAxis` (verbatim del INI de 2022), para que el jugador pueda copiar su fichero.

**Verificación**
- Traza nueva `AIMCFG` (B14) con los 9 valores, **una vez** (no 1 Hz: son valores de carga):
  `AIMCFG forceauto=%d lock=%d tri=%d mcx=%.3f mcy=%.3f brazo=%d sensx=%.2f sensy=%.2f fov=%d ley=%d`.
- Criterio: los valores del log son exactamente los de la tabla →
  `forceauto=0 lock=1 tri=1 mcx=0.530 mcy=0.400 brazo=0 sensx=1.00 sensy=1.00 fov=1`.
- Regresión: el bloque `RC` del verificador da lo mismo (los ajustes no tocan el recoil).

---

#### B1 · Estado compartido de la ley: `src/core/Camera.h` + `src/core/Camera.cpp`

**Por qué el estado va en `CCamera` y no en `PlayerPed.h`.** El estado lo necesitan **tres** unidades
de compilación: `PlayerPed.cpp` lo escribe; `Cam.cpp` y `Hud.cpp` lo leen. Includes reales:

| Fichero | ¿incluye `Camera.h`? | ¿incluye `PlayerPed.h`? |
|---|---|---|
| `src/peds/PlayerPed.cpp` | **sí**, `:9` | sí, `:4` |
| `src/core/Cam.cpp` | sí, `:54` | **sí**, `:11` (ya usa `CPlayerPed::ViceExtIsCrouched()` en `:1745`) |
| `src/renderer/Hud.cpp` | **sí**, `:3` | **NO** |

`Hud.cpp` no incluye `PlayerPed.h`, y añadirlo sería el único include nuevo de todo el plan. Con el
estado en `CCamera` **no hace falta ningún include nuevo en ningún sitio**, y el dueño natural es
`CCamera`, que ya tiene `m_f3rdPersonCHairMultX/Y` como `static` **precisamente** para que `Cam.cpp` los
lea (`Camera.h:465-466`). El mod tiene la misma forma: un `CCamNew` con estado propio al que todo el
mundo apunta (`CamNew.cpp:35-46`).

**Ficheros / anclas**
- `src/core/Camera.h` — en `class CCamera`, justo después de `static bool m_bUseMouse3rdPerson;`
  (`Camera.h:538`).
- `src/core/Camera.cpp` — `CCamera::Init()`, después de `Camera.cpp:284`.

**Acción**: `edit` (los dos).

**Detalle técnico.** En `Camera.h`, antes de `class CCamera` (para que el `struct` sea visible por
todos los que incluyen la cabecera), el `struct` de ajustes; y dentro de `CCamera`, `public:`, los
`static`:

```
struct CAimClassicAxisSettings {
	bool  forceAutoAim;         // ForceAutoAim                = false
	int32 lockOnTargetType;     // LockOnTargetType            = 1  (0 / 1=SA / 2=LCS)
	bool  showTriangle;         // ShowTriangleForMouseRecruit = true
	float crosshairMultX;       // CameraCrosshairMultX        = 0.53f
	float crosshairMultY;       // CameraCrosshairMultY        = 0.4f
	bool  storiesPointingArm;   // StoriesPointingArm          = false
	float stickSensX;           // RightAnalogStickSensitivityX = 1.0f
	float stickSensY;           // RightAnalogStickSensitivityY = 1.0f
	bool  zoomForAssaultRifles; // decision 5.3(a)             = true
};
```

```
	static CAimClassicAxisSettings s_viceExtAim;
	// Estado de la ley ClassicAXIS (mod: los `static inline` de `ClassicAxis`
	// Main.cpp:27-39 y el `CCamNew` CamNew.cpp:20-22,37-46). El mod lo escribe en
	// ProcessPlayerPedControl y lo leen la camara y el HUD; aqui igual, en CCamera,
	// que es el unico sitio visible desde los tres ficheros sin anadir includes.
	static bool    s_viceExtAimLawActive;     // ClassicAxis::isAiming
	static bool    s_viceExtAimSwitchSpeed;   // switchTransitionSpeed
	static int16   s_viceExtAimPrevCamMode;   // previousCamMode
	static float   s_viceExtAimPrevHor;       // previousHorAngle
	static float   s_viceExtAimPrevVer;       // previousVerAngle
	static CEntity *s_viceExtAimMouseTarget;  // thirdPersonMouseTarget
	static uint32  s_viceExtAimLockOnUntil;   // timeLockOn (250 ms)
	static CVector s_viceExtAimLastLockPos;   // lastLockOnPos
	static CRGBA   s_viceExtAimLastLockCol;   // lastLockOnColor
```

Prefijo `s_viceExtAim` y **no** `s_odAim*`: `s_odAim*` ya significa «lo midió el carril B4 antiguo»
(p. ej. `s_odAimStoriesShoulder`, `Cam.cpp:604`) y reutilizarlo haría ambigua la trazabilidad.

Inicialización en `CCamera::Init`, después de `Camera.cpp:284`, con los defaults de la tabla de B0
como literales (así el motor arranca bien aunque el INI no exista: es el mismo patrón que
`m_f3rdPersonCHairMultX = 0.53f` en `:283`); `re3.cpp` los sobreescribe en B0 si el INI trae la sección.

**Restricciones**
- Sin include nuevo. Sin tocar `PlayerPed.h` para esto.
- Estado de **un solo jugador** (`PlayerInFocus`), igual que el mod (`CWorld::Players[PlayerInFocus]`
  en `Main.cpp:147`). No se generaliza.
- Nada de esto se llama desde `CEntity`/`CPed`: respeta `DESIGN.md` §1 (el estado de la *ley de
  cámara* vive en la capa de cámara; la capa de ped lo publica).

**Verificación**
- `AIMCFG` incluye `ley=%d` para confirmar que el estado se inicializa.
- Los bloques `H` (agachado) y `RC` (recoil) del verificador no se mueven: B1 no altera ninguna ruta
  vigente hasta que B2/B9 lo activen.

---

#### B2 · Declaración de la ley y el `case MODE_AIMING`: `src/core/Camera.h` + `src/core/Cam.cpp`

**Ficheros / anclas**
- `src/core/Camera.h` — en `class CCam`, junto a las declaraciones de leyes (`Camera.h:215-242`).
- `src/core/Cam.cpp` — `CCam::Process`, `switch(Mode)`: la línea **396** es literalmente
  `//	case MODE_AIMING:` (comentada desde siempre). Se descomenta **en el sitio**, entre el `break;`
  de `MODE_FOLLOWPED` (`:395`) y `case MODE_DEBUG:` (`:397`).

**Acción**: `edit` (los dos).

**Detalle técnico.** Tres líneas en el bloque de declaraciones de leyes de `Camera.h`, con cabecera de
atribución en el formato de casa (`12-handoff` §3 punto 3):

```
	// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) — CamNew.cpp:233
	//   «void CCamNew::Process_AimWeapon(const CVector&, float, float, float)»
	// Qué se toma: la ley de apuntado DEL MOD (CamNew.cpp:233-388): maxDist 2.7 fijo,
	//   heightOffset 0.25, hombro en el espacio de objeto del ped, LOS sobre el punto
	//   de hombro con repliegue a target.x/y, z += m_fSyphonModeTargetZOffSet + 0.05,
	//   rama de lock-on con horShift/verShift, lockMovement, clamp +-50, doFovChanges.
	// Adaptación: el motor ya tiene el modo en el enum (MODE_AIMING = 5, Camera.h:41)
	//   pero su `case` estuvo comentado siempre; el mod se crea el suyo y se lo queda
	//   mientras se apunta (Main.cpp:1237-1238). NO se parchea Process_Syphon: el mod
	//   no lo toca, y parcharlo es lo que §14 prohibe.
	// Medible: PASS = §8.1 (AIMCAM m=5 dist=2.70 alt=) y §8.5 (AIMCAM amax= <= 50).
	void Process_AimWeapon(const CVector &CameraTarget, float TargetOrientation, float, float);
	void Process_AimWeaponFovLerp(void);          // CamNew.cpp:477-498
	void Process_AimWeaponCrouchOffset(float&);   // CamNew.cpp:447-459
```

Los tres son **miembros de `CCam`** (no funciones libres) porque necesitan `FOV`, `Mode`,
`m_fSyphonModeTargetZOffSet`, `Source`, `Front`, `Alpha`, `Beta`, `Rotating`, `ResetStatics` y
`CamTargetEntity`, todos miembros (`Camera.h:85-180`); una función libre necesitaría 10 parámetros
(`CODING_STANDARDS.md` §1: *helpers first, orchestrators last*).

`Cam.cpp:396`, sustituyendo la línea comentada:

```
	case MODE_AIMING:
		Process_AimWeapon(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
```

**Segundo punto del bloque:** la lista de `ClipIfPedInFrontOfPlayer` (`Cam.cpp:563-565`) **NO** lleva
`MODE_AIMING` y no se le añade: el recorte es cosa de los modos de 1.ª persona
(`Camera.cpp:1999-2008`) y el mod no recorta al jugador en su ley. Se deja escrito para que nadie lo
«arregle» después.

**Restricciones**
- `.cpp` CRLF, python binario, `assert count == 1`.
- El `case` va **descomentando la línea existente**, no añadiendo una nueva: duplicarlo rompe la
  compilación y deja dos rutas a la misma ley.
- Cero direcciones x86, `plugin::patch`, `Nop`, `RedirectCall`, `hook::pattern` (§6).
- No tocar el `default:` de `Cam.cpp:498-501`.

**Verificación**
- Compila el objeto `core/Cam.cpp.o` (§7.9) sin *duplicate case*.
- En partida: `AIMCAM m=5`. Antes de B2 no sale ninguna línea `AIMCAM` ⇒ el verificador da
  `INCONCLUSIVE`, que es exactamente lo pedido (§7.10).

---

#### B3 · Los tres ayudantes de la ley: `src/core/Cam.cpp`

**Ficheros / anclas**
- `src/core/Cam.cpp` — `Process_Cam_On_A_String` **ya tiene** escrita una reimplementación de
  `Process_AvoidCollisions` en `:2722-2798`; el comentario de `:2722-2724` lo dice explícitamente
  («Colisiones CamNew (Process_AvoidCollisions :390-445): LOS + 5 esferas»). Ese bloque es la
  **fuente a extraer**, no algo que haya que escribir.
- `fStickSens` está en `Cam.cpp:1698` (`float fStickSens = 0.01f;`).
- La macro `lerp` de casa está en `src/core/common.h:396`.

**Acción**: `edit`.

**Detalle técnico — (a) `Process_AimWeaponFovLerp` (mod `Process_FOVLerp`, `CamNew.cpp:477-498`)**

Estado: `static float s_odAimFovLerp = 70.0f;` (el mod lo inicializa en el constructor de `CCamNew`,
`CamNew.cpp:43`).

| Del mod | Nuestro | Nota |
|---|---|---|
| `minFOV = 50.0f` (`CamNew.cpp:25`) | `AIM_MIN_FOV = 50.0f;` | constante del bloque |
| `maxFOV = 70.0f` (`:26`) | `AIM_MAX_FOV = 70.0f;` | = `DefaultFOV` (`Camera.h:29`) |
| `maxFOVModern = 70.0f` (`:27`) | **no se porta** | §5.4(c) y además es idéntico: rama muerta |
| `wepMinRange = 70.0f` (`:28`) | `AIM_WEP_MIN_RANGE = 70.0f;` | |
| `CTimer::ms_fTimeStep` | `CTimer::GetTimeStep()` | `Timer.h:22` devuelve `ms_fTimeStep`: 1:1 exacto |
| `interpF(a, b, 0.1f * ts)` | `lerp(0.1f * CTimer::GetTimeStep(), a, b)` | `common.h:396`: `(norm)*((max)-(min))+(min)`, idéntico a `a+(b-a)*t` |
| `info->m_fRange` | `info->m_fRange` | `WeaponInfo.h:42`, mismo nombre |
| Minigun excluido (`:484-486`) | igual | quirk del mod en VC, se porta literal |
| `doFovChanges` se consume (`:497`) | `s_odAimDoFov = false;` al final | |
| `TheCamera.m_nTransitionState == 0` (`:482`) | `m_uiTransitionState == 0` (`Camera.h:383`) | |

El caso `zoomForAssaultRifles == false` del mod pone `FOV = maxFOV` (`:244`) = 70 = `DefaultFOV`: se
escribe `FOV = DefaultFOV;` con un comentario que lo diga, para que no parezca un número mágico.

**Detalle técnico — (b) `Process_AimWeaponCrouchOffset` (mod `Process_CrouchOffset`, `CamNew.cpp:447-459`)**

`offset = lerp(0.1f * CTimer::GetTimeStep(), offset, end);` con
`end = bIsDucking ? -0.5f + (((AIM_MAX_FOV - FOV) / AIM_MIN_FOV) * AIM_MAX_FOV) / 100.0f : 0.0f;` —
la expresión de `CamNew.cpp:454-455` tal cual, con `f = maxFOV` porque `modernCamera` no entra
(`:452`). `bIsDucking` es `Ped.h` (bitfield de `m_nPedFlags`).

**Detalle técnico — (c) `Process_AvoidCollisions`: EXTRACCIÓN, no copia** (`RULES 0.6`)

El bloque `Cam.cpp:2722-2798` es el mismo algoritmo que `CamNew.cpp:390-445`, ya escrito y validado
por el jugador en `ve65`-`ve75`. La casa no admite dos copias del mismo código. Se **extrae** a un
miembro de `CCam` y se llama desde los dos sitios:

```
	// Colisiones de la ley ClassicAXIS (CamNew.cpp:390-445): LOS + 5 esferas.
	// Extraido de Process_Cam_On_A_String, que ya lo tenia escrito en :2722-2798,
	// para que la ley de a pie y la de apuntado no sean dos copias (RULES 0.6).
	// Comportamiento IDENTICO al ya validado en ve65-ve75.
	void Process_AvoidCollisions(const CVector &targetCoors, float length, bool hideClosePeds);
```

Sólo **dos** cosas cambian entre los dos usos, y se anotan como tales:
- `hideClosePeds`: `true` en la ley de apuntado (el mod esconde los peds a <0,5 m y visibles,
  `CamNew.cpp:425-429`); `false` en la de coche, que hoy **no** los esconde y **no** se le añade
  (sería cambiar una ley validada). El coche corta el bucle de las 5 esferas en cuanto no hay impacto
  (`Cam.cpp:2769-2772`) y el mod itera las 5 enteras: con `hideClosePeds == false` ese `break` se
  conserva literal.
- El resto sale **literal** de `Cam.cpp:2729-2792`: `pIgnoreEntity = CamTargetEntity` en el LOS y
  `nil` en las esferas, `Max(distFromPoint - 0.3f, 0.05f)` si `distFromPoint < 1.3f`,
  `viewPlaneWidth = Tan(DEGTORAD(FOV)/2.0f) * CDraw::GetAspectRatio() * 1.05f`, el **near-clip
  re-leído en cada una de las 5 vueltas**, `d = Max(Min(nearClip, d), 0.1f)` y
  `Source += (targetCoors - Source) * (d / length)`.

Dos desajustes de nombre entre el mod y nuestro árbol, que hay que escribir en el bloque:
1. `CDraw::CalculateAspectRatio()` (`Cam.cpp:2750`) **muta** `CDraw::ms_fAspectRatio` como efecto
   secundario (`Draw.cpp:56-67`). La nueva ley usa **`CDraw::GetAspectRatio()`** (sólo lectura,
   `src/renderer/Draw.h:65`), que es además lo que ya usa la aritmética de puntería (`Camera.cpp:4236`).
2. `CColPoint::m_vecPoint` (mod) = `CColPoint::point` (nuestro, `Cam.cpp:2781`).

**Detalle técnico — (d) sensibilidades por eje** (`RightAnalogStickSensitivityX/Y`)

`Cam.cpp:1815-1816` (ley de a pie con ratón), `:2438` y `:5784-5785` (runabouts) ya multiplican por
`fStickSens` (0.01f, `Cam.cpp:1698`), que es el `0.01f` del mod (`CamNew.cpp:143`). El ajuste entra
como un factor más y es la **única** modificación de las leyes de a pie que entra (C-14):

```
	// ClassicAXIS RightAnalogStickSensitivityX/Y (CamNew.cpp:145-146 / :327-328):
	// el mod multiplica los offsets por el ajuste; el 0.01 base es nuestro fStickSens.
	BetaOffset  *= CCamera::s_viceExtAim.stickSensX;
	AlphaOffset *= CCamera::s_viceExtAim.stickSensY;
```

en los 3 sitios, **después** del `if(UseMouse){...}else{...}` de cada uno. En la **rama de ratón** el
ajuste **no** se aplica, porque el mod tampoco lo aplica (`CamNew.cpp:148-151` pisa los offsets con
los del ratón y no vuelve a multiplicar).

**Restricciones**
- La extracción de (c) es un **refactor puro**: cero cambios de comportamiento en la ley de coche. Si al
  extraer cambia un número, es un bug del refactor, no una mejora.
- `lerp` es una macro (`common.h:396`): ojo con los paréntesis y con no pasar nada con efecto doble.
- `fovLerp` es estado entre frames: se inicializa a `DefaultFOV` y se resetea con `ResetStatics`. El
  mod no lo resetea; sin eso el primer apuntado tras un cambio de cámara hereda el FOV de otro modo.
  Desviación mínima y defendible, se documenta.

**Verificación**
- `AIMFOV fov= arma= alcance= taken=` (B14), 1 Hz **sólo mientras se apunta`: rifle de alcance ≥ 70 ⇒
  `fov` baja a 50 y vuelve a 70; **Minigun** ⇒ `taken=0` y `fov` se queda en 70 (§8.6).
- `AIMCOL los= losD= sph= sphM= sphPed= dSph= dRaw= nc= app=` (B14): el formato es el de `CAMB2b`
  (`Cam.cpp:2643`), con `nc` = el near-clip re-leído. §8.12.
- **No-regresión de (c):** las traizas `CAMB2b` de un mismo recorrido de coche tienen que dar
  **exactamente** los mismos números antes y después del refactor.

---

#### B4 · `Process_AimWeapon`: el cuerpo de la ley — `src/core/Cam.cpp`

**Ficheros / anclas**
- `src/core/Cam.cpp` — función nueva. Se coloca **inmediatamente antes** de `CCam::Process_Syphon`
  (cuyo bloque `#ifdef VICEEXT_RECOIL` de cabecera está en `:4100-4102`), para que las leyes con
  recoil queden juntas. **Un único sitio, elegido y anotado** (`CODING_STANDARDS.md` §1).

**Acción**: `edit` (crear la función; es el bloque más largo del plan).

**Detalle técnico.** Traducción de `CamNew.cpp:233-388`. Cada número viene de un fichero leído.

| Línea del mod | Expresión del mod | Nuestro destino |
|---|---|---|
| `:234-238` | `if (!cam) return;` + `if (!cam->m_pCamTargetEntity \|\| m_nType != ENTITY_TYPE_PED) return;` | `if(!CamTargetEntity \|\| !CamTargetEntity->IsPed()) return;` — el mismo early-out que `Process_FollowPed` (`Cam.cpp:1341-1342`) |
| `:240` | `doFovChanges = true;` | `s_odAimDoFov = true;` |
| `:241-244` | FOV | `Process_AimWeaponFovLerp();` (B3a) |
| `:248-250` | `maxDist = 2.7f; heightOffset = 0.25f; length = maxDist;` | `const float maxDist = 2.7f; const float heightOffset = 0.25f; const float length = maxDist;` — **`length` es constante**: `:250` es su única asignación en toda la función (verificado), y es el `length` que se pasa a `Process_AvoidCollisions` |
| `:252-257` | `aimOffset = (0.55,0,0)` si `storiesAimingCoords`, si no `(0.2,0,0)` | `aimOffset = CVector(0.2f, 0.0f, 0.0f);` con comentario de que la rama 0.55 es `StoriesAimingCoords` (§9). **0.20, no 0.55** |
| `:265-266` | `vec = TransformFromObjectSpace(mat, e->GetHeading(), aimOffset); targetCoords = vec;` | B5 / §10.18-3 |
| `:268-278` | LOS de `targetCoords` a `m_vecSource`; si impacta, `targetCoords.x = target.x; targetCoords.y = target.y;` (**sólo x/y: z se queda**) | `CWorld::ProcessLineOfSight(targetCoords, Source, colPoint, entity, true, false, false, true, false, true, true, false)` — los 12 argumentos en el **mismo orden** que el `Cam.cpp:4181` (que es el mismo del vanilla) |
| `:280` | `Process_CrouchOffset(duckOffset);` | `Process_AimWeaponCrouchOffset(duckOffset);` con `static float duckOffset = 0.0f;` (el mod lo tiene en `CCamNew`, `CamNew.cpp:45`) |
| `:282-284` | `z += m_fSyphonModeTargetZOffSet + 0.05f; z += heightOffset; z += duckOffset;` | idéntico, con `m_fSyphonModeTargetZOffSet` (`Camera.h:111`) |
| `:288-303` | rama de **lock-on** | B4-bis, abajo |
| `:305-308` | wrap de los dos ángulos a ±PI | `while(Beta >= PI) Beta -= 2*PI; while(Beta < -PI) Beta += 2*PI; while(Alpha >= PI) Alpha -= 2*PI; while(Alpha < -PI) Alpha += 2*PI;` — **`m_fHorizontalAngle` → `Beta` y `m_fVerticalAngle` → `Alpha`** (mapa §3.3) |
| `:310-312` | `if (pad->DisablePlayerControls) lockMovement = true;` | `if(CPad::GetPad(0)->ArePlayerControlsDisabled()) lockMovement = true;` (`Pad.h:473`, `!= PLAYERCONTROL_ENABLED`): mismo valor, con la regla de casa escrita |
| `:313-333` | offsets de stick y ratón | **Se queda la zona muerta** (§5.4b) y **el acelerador vertical** (§5.4d): `LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight(); LookUpDown = CPad::GetPad(0)->LookAroundUpDown();` y con ratón `LookLeftRight = -2.5f*MouseX; LookUpDown = 4.0f*MouseY;` — los factores **`-2.5f` y `4.0f` SÍ entran** (`CamNew.cpp:139-140`, y ya son los nuestros en `Cam.cpp:1803-1804`). La fórmula del ratón con `m_fMouseAccelHorzntl` **no** entra; la del stick `* fStickSens * (1.0f/20.0f) * FOV/80.0f * CTimer::GetTimeStep()` y `(0.6f/20.0f)` sí, más los dos factores de B3d |
| `:335-345` | `if (betaOffset \|\| alphaOffset \|\| lockMovement) m_bRotating = false;` y `if (!lockMovement) { ángulos += offsets; }` | idéntico, con `Rotating` (`Camera.h:96`) |
| `:347-353` | wrap de `Beta` + **clamp `±50°`** de `Alpha` | idéntico con `DEGTORAD(50.0f)` |
| `:355-366` | `m_bCamDirectlyBehind` / `m_bCamDirectlyInFront` | idéntico, con `TheCamera.m_bCamDirectlyBehind` (`Camera.h:330`) |
| `:368-372` | `if (camUseCurrentAngle) { HorizontalAngle = previousHorAngle; VerticalAngle = previousVerAngle; camUseCurrentAngle = false; }` | idéntico, con `s_viceExtAimPrevHor/Ver` (B1). Se consume **antes** del input, como en el mod |
| `:374-377` | `m_fDistanceBeforeChanges = (m_vecSource - targetCoords).Magnitude();` `m_vecFront = (cos(Alpha)cos(Beta), cos(Alpha)sin(Beta), sin(Alpha));` `m_vecSource = targetCoords - m_vecFront*length;` `m_vecSourceBeforeLookBehind = targetCoords + m_vecFront;` | idéntico con `m_fDistanceBeforeChanges` (`Camera.h:125`), `Front` (`:175`), `Source` (`:176`), `SourceBeforeLookBehind` (`:177`) |
| `:378-383` | `targetCoords.z -= heightOffset; m_vecTargetCoorsForFudgeInter = targetCoords; m_vecFront = targetCoords - m_vecSource; m_vecFront.Normalise();` | idéntico, con `m_cvecTargetCoorsForFudgeInter` (`Camera.h:167`) |
| `:385-387` | `Process_AvoidCollisions(length); GetVectorsReadyForRW();` | `Process_AvoidCollisions(targetCoords, length, true); GetVectorsReadyForRW();` — **`GetVectorsReadyForRW` ya existe** (`Camera.h:193`) y es idéntico a `CamNew.cpp:461-475` (Up = (0,0,1), normaliza Front, guarda `Front.xy == 0`, `right = Cross(Front, Up)`, `Up = Cross(right, Front)`). **No se reescribe: se llama** |

**B4-bis · la rama de lock-on (`CamNew.cpp:288-303`)**, donde están `horShift`/`verShift`:

```
	if (m_pPointGunAt && m_bHasLockOnTarget && !LookingBehind) {   // LookingBehind = Camera.h:92
		CVector t = m_pPointGunAt->GetPosition();
		CVector distfromTarget = Source - t;
		float viewPlaneHeight = Tan(DEGTORAD(FOV) * 0.5f);
		float viewPlaneWidth  = viewPlaneHeight * CDraw::GetAspectRatio() * 1.05f;
		float horShift = CGeneral::GetATanOfXY(1.0f,
			(m_f3rdPersonCHairMultX - 0.5f + m_f3rdPersonCHairMultX - 0.5f) * viewPlaneWidth);
		float verShift = CGeneral::GetATanOfXY(1.0f,
			(viewPlaneHeight * 0.0174f) * ((0.5f - m_f3rdPersonCHairMultY + 0.5f - m_f3rdPersonCHairMultY) * (1.0f / CDraw::GetAspectRatio())));
		Beta  = ((CPed*)CamTargetEntity)->m_fRotationCur + (PI * 0.5f) + horShift;
		Alpha = CGeneral::GetATanOfXY(distfromTarget.Magnitude2D(), -distfromTarget.z) - verShift;
		lockMovement = true;
	}
```

Tres números que no hay que redondear: el `1.05f` del ancho de vista, el `0.0174f` del `verShift` y el
`PI * 0.5f` del `Beta`. `m_fRotationCur` es `Ped.h:559`.
El mod calcula además `distFromCamEntity` (`:293`) y **no lo usa** en el resto de la función: se omite.

**Restricciones**
- **No tocar** `Process_Syphon`, `Process_FollowPed`, `Process_FollowPedWithMouse`,
  `Process_Fight_Cam` ni `Process_Cam_On_A_String` más allá de B3(c)/B3(d). La ley nueva es una
  función **nueva**; las de serie no se reescriben (§5 y C-14).
- `GetAspectRatio()` de lectura, **nunca** `CalculateAspectRatio()`.
- `Min`/`Max` de casa, no `std::min`/`std::max`.
- CRLF, python binario, `assert count == 1`, `PYTHONIOENCODING=utf-8`. Sin `printf`, sin direcciones.

**Verificación**
- `AIMCAM m=5 dist=2.70 alt=%.3f zoff=%.3f amax=%.1f aim=%d fight=%d hombro=0.20 obj=1 ex=%.2f ey=%.2f
  ez=%.2f lado=%+d` (1 Hz apuntando + una línea por flanco al entrar/salir): `alt` debe ser
  `0.25 + m_fSyphonModeTargetZOffSet + 0.05` (§8.1) y `|amax| <= 50` (§8.5).
- `dist` vale 2.70 salvo que la colisión lo acerque: por eso existe `AIMCOL`, no para juzgar el 2.70.
- §8.14 `camauto2 auto=1 pedido=0` idéntico (el auto-centrado de coche no lo llama esta ley).

---

#### B5 · El hombro, y por qué **no** se tocan los 3 sitios de `ve58` — `src/core/Cam.cpp`

**Ficheros / anclas**
- `src/core/Cam.cpp` — dentro de `Process_AimWeapon`, en el punto que sustituye a `CamNew.cpp:265`.
- `src/core/Placeable.h:6` (`m_matrix`) y `:20` (`GetRight`).
- `src/math/Matrix.h:56-58` (`GetRight`/`GetForward`/`GetUp`) y `:94` (`SetRotateZOnly`).
- `src/math/Matrix.cpp:215-228` — `SetRotateZOnly`: `right = (cos a, sin a, 0)`,
  `forward = (-sin a, cos a, 0)`, `up = (0,0,1)`.

**Acción**: `edit`.

**Detalle técnico — el hecho (§10.18-3)**

`TransformFromObjectSpace` **no existe** en `src/` (0 resultados). Y la premisa del plan —que
`Cam.cpp:1386` aplica el hombro «en espacio de cámara»— es **falsa**: `CamTargetEntity->GetRight()` es
`CPlaceable::GetRight()` → `m_matrix.GetRight()` (`Placeable.h:20`), y `m_matrix` es miembro **de la
entidad** (`Placeable.h:6`). El hombro **ya va en el espacio de objeto del ped**; el propio árbol lo
documenta en `Cam.cpp:1778-1781`.

Consecuencia: para un ped con sólo rotación Z —lo único que el propio mod escribe
(`Main.cpp:571`: `m_placement.SetRotateZOnly(m_fRotationCur)`)— las dos formas coinciden numéricamente,
porque `rotZ(a) · (0.2,0,0) = (0.2·cos a, 0.2·sin a, 0) = right · 0.2` (`Matrix.cpp:218-219`).

```
	// Hombro (CamNew.cpp:252-266). El mod pasa la matriz DEL PED y su heading a
	// TransformFromObjectSpace. Esa funcion no existe aqui y no hace falta:
	// CPlaceable::GetRight() (Placeable.h:20) es m_matrix.GetRight(), o sea el eje
	// DERECHO DEL PROPIO PED en mundo; y para un ped con solo rotacion Z
	// (SetRotateZOnly, Matrix.cpp:215-228) rotZ(a)*(0.2,0,0) == right*0.2 exacto.
	// La rama 0.55 es `storiesAimingCoords`, que §9 deja sin implementar.
	const CVector aimOffset(0.2f, 0.0f, 0.0f);
	CVector shoulder = CamTargetEntity->GetMatrix().GetPosition()
	                 + CamTargetEntity->GetRight() * aimOffset.x;
```

Y con eso **`Cam.cpp:1386`, `:1782` y `:5714` NO SE TOCAN**: son leyes que este plan no sustituye, que
el jugador validó en `ve58`-`ve75`, y cuyo hombro ya es de objeto.

**`PENDIENTE`**: la firma exacta de `TransformFromObjectSpace(mat, heading, offset)` del plugin-sdk, que
**no está en `tmp/extsrc/`** y por tanto no se puede verificar. Resolverlo requiere la tabla de
símbolos del plugin-sdk (`DK22Pac/plugin-sdk`, `Common.h`/`CMatrix`) o el `.map` del mod. **No lo
afecta**: la diferencia sólo aparece con un ped que tenga pitch/roll en su matriz (peds muertos, en
vehículo, ragdoll), y el objetivo de esta ley es el jugador, que va de pie. Queda escrito en el bloque.

**Restricciones**
- Prohibido tocar `Cam.cpp:1386/1782/5714`, y prohibido tocar `s_odAimStoriesShoulder` (`:604`) ni
  `ViceExtAimShoulderSmoothed` (`:647-660`): son del carril B4 antiguo.
- El suavizado `WellBufferMe(0.2f, 0.1f)` de `ViceExtAimShoulderSmoothed` **no** se quita (§5.4 lo
  deja como está), pero **sólo** en las leyes de a pie. La ley nueva usa el valor **crudo** 0.20,
  porque el mod no suaviza (`CamNew.cpp:257` es un literal).

**Verificación**
- `AIMCAM hombro=0.20 obj=1 ex= ey= ez= lado=` (1 Hz apuntando): `ex/ey/ez` es el offset **real**
  aplicado, no el teórico, y `lado` el signo de `GetRight().x`. §8.2 y §8.3 (validado contra la
  posición y los ejes del ped **en la traza**, no a ojo).
- `AIMDIR` (`PlayerPed.cpp:1576`, R9) sin cambios: no se toca el clip de apuntado.

---

#### B6 · El callsite del recoil en la ley nueva — `src/core/Cam.cpp` (sólo lectura: `Weapon.h`)

**Ficheros / anclas**
- `src/core/Cam.cpp` — dentro de `Process_AimWeapon`, justo donde el `Alpha` del frame ya está
  decidido: después del `Beta` de la rama de lock-on (o de `Beta += betaOffset`) y **antes** de
  `Alpha += alphaOffset`.
- **Referencia literal**: `Cam.cpp:4162` y `Cam.cpp:4170` (`Process_Syphon`).
- Declaraciones: `src/weapons/Weapon.h:45-47`, bajo `#ifdef VICEEXT_RECOIL`.

**Acción**: `edit` (`Cam.cpp` **sólo**; `Weapon.h`/`Weapon.cpp` no se tocan).

**Detalle técnico — copia literal, con `source` propio**

```
#ifdef VICEEXT_RECOIL
	// ClassicAXIS §5.7: sin callsite, apuntar dejaria de mover la camara con el recoil
	// (el recoil se aplica desde dentro de la ley de camara, no desde el arma).
	// Copia LITERAL de Process_Syphon (:4162 / :4170). Limites -PI/+PI y NO +-50: la
	// ley de apuntado CLAMPA Alpha a +-50 (CamNew.cpp:350-353) DESPUES de este Apply,
	// igual que Process_Syphon hace con los suyos.
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "aim-weapon");
	CWeapon::ViceExtRecoilApply(Alpha, 0.0f, 0.0f, "unknown", Mode, -PI, PI);
#endif
```

y `bool recoilReset = ResetStatics;` en el bloque `#ifdef VICEEXT_RECOIL` de la cabecera de la función,
**exactamente** como en `Process_Syphon` (`:4100-4102`), `Process_FollowPed` (`:1343-1345`) y
`Process_FollowPedWithMouse`.

Argumento a argumento contra la firma (`Weapon.h:46-47`):
- `manualDeltaRad = 0.0f` e `inputY = 0.0f`: **`Process_AimWeapon` no lleva input manual medible** —
  `lockMovement` congela el input (`CamNew.cpp:302` y `:312`) y el `Alpha` lo decide la rama de lock-on
  o el `target` del mod, no el ratón. Es el mismo argumento que da `Process_Syphon` (`:4170`), y el
  comentario de `:4169` («Syphon es un solver de seguimiento, no input manual medible») es la razón
  que se repite aquí.
- `minAlpha = -PI`, `maxAlpha = PI`: **idéntico a `Process_Syphon`**, y es lo que dice §5.7. `±50` lo
  aplica el clamp de la propia ley, no el `Apply`.
- `source = "aim-weapon"`: cadena nueva, para que el log diga de qué ley salió la patada. Es la misma
  técnica que `"follow-mouse"` (`:1844`) y `"follow-ped-passive"` (`:1688`).

**Restricciones**
- **Intocable**: la matemática de `recoil-por-arma-y-cadencia.md`. Aquí sólo hay dos llamadas en un
  sitio nuevo; ni una línea de `Weapon.cpp`, ni de la cola (`s_odRecoilQueue`, `Cam.cpp:63-71`), ni
  del residual.
- `ViceExtRecoilApply` es un `static` de `CWeapon` (`Weapon.h:46`): no se añade ningún método nuevo.

**Verificación**
- `RECOIL_APPLY shotSeq= … source=aim-weapon` en el log mientras se apunta ⇒ el callsite existe.
- **§8.13, el criterio que tumba el plan si falla:** `RECOIL3` y el bloque `RC` dan **exactamente** lo
  mismo que antes del cambio; `multY` sigue en 0.400; y la patada se ve con SMG sin que la retícula se
  mueva.
- `camauto2 auto=1 pedido=0` sin cambios: el `WellBufferMe` del auto-centrado es de la ley de coche y
  no lo llama esta función.

---

#### B7 · La reconciliación con `CamControl` y con la 1.ª persona — `src/core/Camera.cpp`

**Ficheros / anclas**
- `src/core/Camera.cpp` — `CCamera::TakeControl` (`:2330-2357`), `CCamera::StartTransition`
  (`:2483-2767`), y el bloque `if(m_bLookingAtPlayer)` de `CamControl` (`:1729-1938`).
- Bloque de modo de arma en `CamControl`: `:1507-1592` (Syphon en `:1528-1591`).

**Acción**: `edit`. **Y este bloque es casi todo *documentación*, no código.**

**Detalle técnico — (a) por qué no hay que tocar `CamControl` (la pregunta 2)**

El mod no parchea la lógica de modo de arma: llama `TheCamera.TakeControl(playa, MODE_AIMWEAPON, 1, 0)`
(`Main.cpp:1238`). En nuestro árbol `TakeControl` (`Camera.cpp:2330`) hace, entre otras cosas, **cuatro**
cosas que juntas producen exactamente el bypass que busca el mod:

| `TakeControl` |-effects- | Efecto sobre el enrutado |
|---|---|
| `Camera.cpp:2352` `m_iModeToGoTo = mode;` | — | el modo pedido es `MODE_AIMING` |
| `Camera.cpp:2353` `m_iTypeOfSwitch = typeOfSwitch;` | con `1` = `INTERPOLATION` (`Camera.h:313-316`) | transición, no salto |
| `Camera.cpp:2354` `m_bLookingAtPlayer = false;` | **← la clave** | el bloque `if(m_bLookingAtPlayer)` de `CamControl` (`:1729-1938`) **no se ejecuta**, así que el `ReqMode` que calculó el bloque de modo de arma (`:1507-1592`) **nunca se aplica** |
| `Camera.cpp:2355` `m_bStartInterScript = true;` | — | el `else` de `:1939-1990` arranca la transición a `m_iModeToGoTo` en `:1959-1960` |

Es decir: **`ReqMode = MODE_SYPHON` (`:1551`) y `ReqMode = PlayerWeaponMode.Mode` (`:1527`) se siguen
calculando, pero se descartan.** No hay que añadir ningún `if` en `CamControl`, y **no se toca
`Camera.cpp:1507-1592`**. Ése es el bypass «de la spec del mod», verbatim, sin reescribir la lógica de
la GTA — que es justo lo que §3.1 exige.

Y en el sentido inverso (`Main.cpp:1417-1425`): al soltar,
`TheCamera.TakeControl(FindPlayerPed(), previousCamMode, 1, 0);` seguido de
`TheCamera.m_bLookingAtPlayer = true;` — **esas dos líneas, en ese orden**. La segunda es la que
devuelve la autoridad a `CamControl`; sin ella el jugador se queda en `MODE_AIMING` para siempre.

**Detalle técnico — (b) que no se rompan francotirador / lanzacohetes / M16 / 1.ª persona**

El mod protege esos modos con `IsType1stPerson` (`Main.cpp:636-650`) y **sale antes** de calcular
`isAiming` (`Main.cpp:1227-1228`):
```
	switch (weaponType) {           // Main.cpp:640-647
	case WEAPONTYPE_SNIPERRIFLE:
	case WEAPONTYPE_ROCKETLAUNCHER:
	case WEAPONTYPE_LASERSCOPE:  return true;
	};
	return !info->m_bCanAim && !info->m_bCanAimWithArm && info->m_b1stPerson;   // :649
```
Nuestro equivalente va en B9 (`IsType1stPerson`). Con el early-return, la ley **nunca** toma el control
con esas armas y `CamControl` sigue gobernando: `ReqMode = PlayerWeaponMode.Mode` (`:1527`) y el salto
a 1.ª persona de `:1758-1767` siguen igual. Con mando, el mismo camino.

**⚠️ Conflicto real con una decisión cerrada** — el lanzacohetes. `IsType1stPerson` del mod incluye
`WEAPONTYPE_ROCKETLAUNCHER`, pero nuestro `VICEEXT_ROCKET_3RD_PERSON` (`config.h:421`, validado en R16,
`PlayerPed.cpp:1557-1560` y `:1634-1664`) **saca el lanzacohetes del modo francotirador a propósito**.
Si se copia la lista del mod tal cual, el lanzacohetes entra en `MODE_AIMING` y eso **anula R16**.
→ **Punto de decisión D2 (§10.21).** Recomendación del enriquecimiento: honorar R16, o sea quitar
`WEAPONTYPE_ROCKETLAUNCHER` de nuestra `IsType1stPerson`, y decirlo en el comentario de la función.

**Detalle técnico — (c) C6, la duración de la transición (`Main.cpp:227-248`)**

El mod engancha `onStartingTransition.after` para `MODE_AIMWEAPON || MODE_FOLLOW_PED` y pone
`m_nTransitionDuration = 500`, `m_nTransitionDurationTargetCoors = 500`,
`m_fFractionInterToStopMoving = 0.1f`, `m_fFractionInterToStopCatchUp = 0.9f`, y consume su
`switchTransitionSpeed` de un tiro.

Nuestro anclaje es `CCamera::StartTransition` (`Camera.cpp:2483`). **Dónde insertarlo importa**: la
función sobrescribe esos valores en dos sitios (`:2711-2716` y `:2750-2766`), así que el override va
**al final de la función, después de `:2766` y antes de la llave de `:2767`**, y con el flag de un
solo tiro:
```
	if (s_viceExtAimSwitchSpeed && (newMode == CCam::MODE_AIMING || newMode == CCam::MODE_FOLLOWPED)) {
		m_uiTransitionDuration = 500;
		m_uiTransitionDurationTargetCoors = 500;
		m_fFractionInterToStopMoving = 0.1f;
		m_fFractionInterToStopCatchUp = 0.9f;
		s_viceExtAimSwitchSpeed = false;
	}
```
`uint32 m_uiTransitionDuration` y `m_uiTransitionDurationTargetCoors` son `Camera.h:389-390` (el 500 va
como entero, no `500u` — el resto del fichero asigna literales). El flag lo pone B9 al tomar el
control (`Main.cpp:1240`) y lo pone **también** al devolverlo (`Main.cpp:1420`), igual que el mod.
El override tiene que ir **fuera** del `if(m_bLookingAtPlayer)` porque en la vuelta a la cámara
`m_bLookingAtPlayer` ya es `true` y ese `if` pondría 600/0.0/1.0 (`:2754-2756`).

**Lo que NO se toca en `StartTransition`:** las listas de clasificación de `:2501-2512` y el
`switch(Cams[ActiveCam].Mode)` de `:2561-2684`. `MODE_AIMING` no se añade a ninguna: el mod no tiene
esa clasificación y el override del final ya fija los cuatro números. Queda escrito para que no se
«complete» después.

**Detalle técnico — (d) C4 y C5 no requieren código** (C-8). Se deja un comentario en
`Process_AimWeapon` que lo diga, porque si no alguien «comprueba» que el fight cam está anulado y lo
desanula.

**Restricciones**
- `Camera.cpp:1507-1592` (**la lógica de modo de arma de `CamControl`**) es **intocable**: es el núcleo
  de la cámara de la GTA y el mod tampoco lo toca. Bypass por `TakeControl`, no por edición.
- `TakeControl` **no se modifica**: se llama con los mismos argumentos que el mod.
- Los intocables siguen intactos: el auto-centrado de coche (`:2595-2602`) y la matemática del recoil.

**Verificación**
- `AIMCAM m=5` mientras se apunta y `AIMCAM m=4` (o `m=1`) al soltar: el modo vuelve.
- Con el francotirador: `AIMCAM` **no** aparece y el `modo` de `R3P` (`PlayerPed.cpp:1576`) no cambia →
  el bloqueo funciona (y avisa de si D2 está mal resuelto).
- La transición no da un salto: la duración se mide en el log con la traza de flanco de B14
  (`AIMCAM` lleva `trans=%u`, los ms de `m_uiTimeTransitionStart` al siguiente cambio de
  `m_uiTransitionState`); el objetivo es **500 ms**, no 1350 (el default de `:2689/2716`).
- §8.10 sin fight cam: `AIMCAM fight=0` (el campo dice si `ReqMode` habría pedido `MODE_FIGHT_CAM`
  durante el apuntado — informativo, el bloqueo es estructural).
- §8.14 `camauto2 auto=1 pedido=0` idéntico: `StartTransition` se toca, pero el auto-centrado no.

---

#### B8 · `Camera.cpp`: el pitch del brazo y los multiplicadores de retícula

**Ficheros / anclas**
- `src/core/Camera.cpp` — `CCamera::Find3rdPersonQuickAimPitch` (`:4249-4257`) y `CCamera::Init`
  (`:283-284`).
- `src/core/Camera.h` — la declaración ya existe (`:625`), **no se toca**.

**Acción**: `edit`.

**Detalle técnico — (a) `Find3rdPersonQuickAimPitch`, la fórmula del mod**

El mod la reimplementa en `classicaxis_Main.cpp:1464-1472` (`Find3rdPersonQuickAimPitch(float y)`) y
la llama en dos sitios: `Main.cpp:1208` (con `m_f3rdPersonCHairMultY`) y `Main.cpp:1270` (con
`out.y / SCREEN_HEIGHT`). **El parámetro `y` de la función es código muerto**: el cuerpo usa
`TheCamera.m_f3rdPersonCHairMultY` directamente (`:1468`) e ignora el argumento.

Nuestro `CCamera::Find3rdPersonQuickAimPitch()` **no tiene parámetro** (`Camera.h:625`), lo cual encaja
con ese código muerto. Se reescribe así:
```
	// ClassicAXIS Find3rdPersonQuickAimPitch (Main.cpp:1464-1472).
	// El mod pasa un `y` que no usa (usa m_f3rdPersonCHairMultY del cuerpo, :1468);
	// nuestra firma ya no lo tiene (Camera.h:625), asi que la misma cuenta cabe igual.
	// La diferencia con la version de serie (:4252-4256) NO es cosmetica:
	//  (a) el factor de la retícula pasa de (0.5f - multY) a (0.5f - multY + 0.5f - multY)
	//      = (1 - 2*multY), o sea el DOBLE, y se divide por el aspect ratio (mod :1469);
	//  (b) el término angular pasa de Asin(Front.z) (el rumbo real de la camara) a
	//      Cams[ActiveCam].Alpha (el angulo de la ley) (mod :1471).
	float clampedFrontZ = Clamp(Cams[ActiveCam].Front.z, -1.0f, 1.0f);
	(void)clampedFrontZ;
	float tanHalf = Tan(DEGTORAD(Cams[ActiveCam].FOV) * 0.5f * 0.01403292f);
	return -(Atan(tanHalf * (0.5f - m_f3rdPersonCHairMultY + 0.5f - m_f3rdPersonCHairMultY)
	                    * (1.0f / CDraw::GetAspectRatio())) + Cams[ActiveCam].Alpha);
```
El `0.01403292f` es **el** número que hay que copiar tal cual (mod `:1467`); es la constante que
convierte FOV en grados a radianes/2. `DEGTORAD(FOV) * 0.5f * 0.01403292f` es el equivalente exacto de
`tan(cam->m_fFOV * 0.5 * 0.01403292f)` del mod, con el FOV en grados (que es como lo guarda `CCam::FOV`).
`Asin(...)` se queda por la rama de la versión de serie que se borra; se declara y se anota, o
directamente se borra si el compilador no lo exige (no se exige).

**Detalle técnico — (b) `m_f3rdPersonCHairMultX/Y` desde el ajuste**

`CCamera::Init` pone `0.53f` / `0.4f` en `Camera.cpp:283-284`. Se sustituyen por los valores de B0:
```
	// ClassicAXIS CameraCrosshairMultX/Y (Main.cpp:284-285, ini :13-14). El mod los
	// reescribe CADA FRAME en su hook de camControl; aqui se leen una vez al cargar
	// (no hay menu en el juego para cambiarlos), que es el mismo valor en partida.
	m_f3rdPersonCHairMultX = s_viceExtAim.crosshairMultX;
	m_f3rdPersonCHairMultY = s_viceExtAim.crosshairMultY;
```
**Consecuencia que hay que vigilar** (ya la dice §5.4): al ser `static`, el valor pasa a valer también
para la mira 3.ª persona de `Hud.cpp:390-391` y para `Find3rdPersonCamTargetVector`
(`Camera.cpp:4236-4237`) — igual que en el mod, que también los sobreescribe globalmente
(`Main.cpp:284-285`). Es correcto, pero si el jugador cambia `CameraCrosshairMultX` en el INI verá que
**también** mueve la mira de la HUD. Se dice en el informe.

**Restricciones**
- `Find3rdPersonQuickAimPitch` tiene 5 callsites (`PedFight.cpp:365`, `:376`, `:1049`,
  `PlayerPed.cpp:1742`, `:1780`) y **todos** están dentro de `if(Using3rdPersonMouseCam())`, que exige
  `MODE_FOLLOWPED` (`Cam.cpp:1133`) ⇒ mientras la ley nueva esté activa **no corren**. El cambio de
  fórmula sólo se nota en el primer frame de volver a `MODE_FOLLOWPED`. Es lo correcto (el mod tiene el
  mismo solape) y hay que decirlo, porque si no parecerá un cambio sin efecto.
- No añadir el parámetro `y` (Código muerto del mod).
- `CDraw::GetAspectRatio()`, no `CalculateAspectRatio()`.

**Verificación**
- `AIMIK pitch=%.4f alto=%.4f torso=%.1f` (B14, 1 Hz apuntando) + `AIMCAM` con el `Alpha` de la ley:
  la igualdad `pitch == -(Atan(...) + Alpha)` se puede comprobar con dos muestras del log. Éste es el
  criterio verificable de la fórmula, porque a ojo el pitch del brazo no se mide.
- `AIMCAM mcx=0.530 mcy=0.400` una vez, y después `RECOIL3 multY=0.400` **sin cambios** (§8.13): el
  `m_f3rdPersonCHairMultY` no se mueve al apuntar.

---

#### B9 · `ViceExtProcessPlayerPedControl` — `src/peds/PlayerPed.cpp` + `src/peds/PlayerPed.h`

**Ficheros / anclas**
- `src/peds/PlayerPed.cpp` — `CPlayerPed::ProcessControl`, línea **4284** (justo tras la llave de
  apertura de `:4283`). Bloque nuevo.
- `src/peds/PlayerPed.h` — el bloque de declaraciones de la zona `VICEEXT_*` (`:77-98`), que es donde
  viven `ViceExtIsSwimming` (`:79`) y `ViceExtIsCrouched` (`:90`).
- El bloque de los predicados, junto a `ViceExtCanAim` (`PlayerPed.cpp:77-91`).

**Acción**: `edit` (los dos).

**Detalle técnico — (a) el predicado `isAiming` (mod `Main.cpp:1230-1235`)**

```
	if (!pad->ArePlayerControlsDisabled() && ViceExtIsAbleToAim(playa)
	    && pad->GetTarget() && TheCamera.GetLookDirection() != 0
	    && ViceExtIsWeaponPossiblyCompatible(playa)
	    && (mode == CCam::MODE_FOLLOWPED || mode == CCam::MODE_AIMING)
	    && GetWeapon()->HasWeaponAmmoToBeUsed()
	    && !pad->JumpJustDown() && !pad->GetSprint())
```
`GetLookDirection` es `Camera.h:616`; `HasWeaponAmmoToBeUsed` es `Weapon.h:91`; `GetTarget`,
`JumpJustDown` y `GetSprint` son `Pad.h:248`, `:252`, `:253`.

`ViceExtIsAbleToAim` (mod `:605-634`), con los estados traducidos a los **nombres reales** de nuestro
enum (`Ped.h:282-304`):

| Mod | Nuestro |
|---|---|
| `PEDSTATE_NONE` / `IDLE` / `FLEE_POSITION` / `FLEE_ENTITY` / `ATTACK` / `FIGHT` / `AIMGUN` | `PED_NONE` / `PED_IDLE` / `PED_FLEE_POS` / `PED_FLEE_ENTITY` / `PED_ATTACK` / `PED_FIGHT` / `PED_AIM_GUN` |
| `m != PEDMOVE_SPRINT && IsPedInControl()` | `m_nMoveState != PEDMOVE_SPRINT && IsPedInControl()` |
| `(!bIsDucking \|\| info->m_bCrouchFire)` (sólo VC, `:628`) | `(!bIsDucking \|\| ViceExtCanAimCrouchFire(info))` — **ver §10.18-9**: la consulta de `WEAPONFLAG_CROUCHFIRE` va por el helper sin mutar |
| `WEAPONTYPE_UNARMED` / `WEAPONTYPE_BRASSKNUCKLE` → false | igual |

`ViceExtIsTypeMelee` (mod `:692-717`) y `ViceExtIsTypeTwoHanded` (mod `:719-743`) también, usando
`ViceExtCanAim` (`PlayerPed.cpp:77`) en vez de `info->m_bCanAim` directo, para no duplicar el caso de
las escopetas de §2 (C7) — que es exactamente el `VICEEXT_SHOTGUN_AIM` ya presente.

**Detalle técnico — (b) `IsType1stPerson`: el early-return (mod `Main.cpp:1227-1228`)**

Primera línea de la función, antes de calcular `isAiming`:
```
	// ClassicAXIS IsType1stPerson (Main.cpp:636-650). El mod DEVUELVE (no apunta) con
	// francotirador, lanzacohetes y laser: esos modos son de 1a persona y los lleva
	// CamControl (Camera.cpp:1507-1527), no nuestra ley. Si la ley tomara el control
	// con esas armas, se romperian el modo francotirador y el 1a persona.
	// D2 (§10.21): WEAPONTYPE_ROCKETLAUNCHER esta en la lista del mod pero en nuestro
	// arbol lo saca VICEEXT_ROCKET_3RD_PERSON (config.h:421, validado en R16).
	if (wt == WEAPONTYPE_SNIPERRIFLE || wt == WEAPONTYPE_LASERSCOPE /* || wt == WEAPONTYPE_ROCKETLAUNCHER */
	    || (!ViceExtCanAim(wt, info) && !info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM)
	        && info->IsFlagSet(WEAPONFLAG_1ST_PERSON)))
		return;
```

**Detalle técnico — (c) `CWeaponInfo`: SIN MUTAR (§10.18-9, la pregunta 9)**

El mod parchea `info->m_bCanAim` en caliente para `FLAMETHROWER` y `MINIGUN` según esté agachado
(`Main.cpp:663-673` en `IsWeaponPossiblyCompatible` y `:730-740` en `IsTypeTwoHanded`).

**Eso no se puede portar.** En nuestro árbol `CWeaponInfo` tiene **un solo array global**
`CWeaponInfo aWeaponInfo[WEAPONTYPE_TOTALALLTYPES]` (`WeaponInfo.cpp:72`) y
`GetWeaponInfo()` devuelve `&aWeaponInfo[weaponType]` (`:129-131`): es el **mismo objeto** que leen
todos los peds, la IA y el motor. Los flags son una máscara en `m_Flags` (`WeaponInfo.h:62`) y se leen
con `IsFlagSet` (`:78`). Ponerlo a `false` en el bucle de control del jugador **apaga el apuntado con
llamas del Minigun también para la IA y para el resto del frame**, y restaurarlo exige un par
guarda/restaurar que es frágil por orden de ejecución.

**Recomendación fail-closed: no se muta nada; la intención del mod se expresa por consulta.**
```
	// ClassicAXIS IsWeaponPossiblyCompatible (Main.cpp:652-676) / IsTypeTwoHanded
	// (:719-743): el mod PARCHEA info->m_bCanAim segun agachado para flamethrower y
	// minigun. Aqui NO se muta: aWeaponInfo (WeaponInfo.cpp:72) es la tabla GLOBAL que
	// leen todos los peds y la IA, y GetWeaponInfo devuelve el mismo puntero
	// (:129-131); cambiarla en el bucle de control apaga el apuntado de esos armas
	// para todo el mundo y hasta el final del frame. La misma intencion, por consulta.
	static bool ViceExtAimCanAimWithCrouch(eWeaponType wt, CWeaponInfo *info, bool ducking)
	{
		if (wt == WEAPONTYPE_FLAMETHROWER || wt == WEAPONTYPE_MINIGUN)
			return !ducking;
		return ViceExtCanAim(wt, info);
	}
```
y luego `return (ViceExtAimCanAimWithCrouch(...) || info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
&& !info->IsFlagSet(WEAPONFLAG_THROW) && !info->IsFlagSet(WEAPONFLAG_1ST_PERSON);` — que es
`Main.cpp:675` con el mapa de flags de §3.3.

**Si el jugador insiste en el comportamiento literal**, lo único que no rompe a la IA es un
**override por ped** que no toque la tabla: un predicado `CPed::ViceExtCanAimNow()` que devuelva el
valor efectivo, y cambiar **sólo** los lectores del camino de apuntado del jugador por ese predicado,
dejando `IsFlagSet` intacto. Eso implica tocar más sitios (los readers de `CANAIM` en el camino del
jugador) y se anota como coste. **Lo que no se puede hacer es mutar la tabla**: no hay dueño, no hay
momento seguro y el fallo es silencioso. → **Punto de decisión D3 (§10.21).**

**Detalle técnico — (d) toma y devolución del modo (mod `Main.cpp:1237-1244` y `:1417-1425`)**

```
	if (mode != CCam::MODE_AIMING && IsPedInControl() && TheCamera.m_uiTransitionState == 0) {
		// Main.cpp:1238-1244, literal. Los argumentos 1 y 0 son, en nuestro arbol,
		// INTERPOLATION (Camera.h:314) y CAMCONTROL_GAME (Camera.h:320).
		// OJO: TakeControl deja m_bLookingAtPlayer = false (Camera.cpp:2354) y eso es
		// lo que hace que CamControl deje de enrutar a MODE_SYPHON mientras apuntamos
		// (el bloque if(m_bLookingAtPlayer) de Camera.cpp:1729-1938 no corre). NO se
		// toca CamControl. Al soltar hay que devolver m_bLookingAtPlayer = true
		// (Main.cpp:1419) o el jugador se queda en MODE_AIMING para siempre.
		TheCamera.TakeControl(playa, CCam::MODE_AIMING, INTERPOLATION, CAMCONTROL_GAME);
		CCamera::s_viceExtAimLawActive = true;
		CCamera::s_viceExtAimSwitchSpeed = true;
		CCamera::s_viceExtAimPrevHor = TheCamera.Cams[TheCamera.ActiveCam].Beta;
		CCamera::s_viceExtAimPrevVer = TheCamera.Cams[TheCamera.ActiveCam].Alpha;
		CCamera::s_viceExtAimPrevCamMode = mode;
	}
```
`m_uiTransitionState` es `Camera.h:383` (el mod escribe `m_nTransitionState`; el mismo campo, nombre
cambiado). Nota: el mod **no** pone `m_bLookingAtPlayer = false` explícitamente porque
`TakeControl` ya lo hace; nuestro `TakeControl` también (`Camera.cpp:2354`) ⇒ **no se añade la línea**.

**Detalle técnico — (e) rotación, flags e IK (mod `Main.cpp:1288-1312`, C15)**

```
	// Main.cpp:1288-1294, literal.
	ViceExtRotatePlayer(playa, front, false);          // Main.cpp:547-574
	SetLookFlag(front, true, true);                   // Ped.h:680 (float, keepTrying, cancelPrevious)
	SetAimFlag(front);                                // Ped.h:765
	// Main.cpp:1296-1300
	m_fFPSMoveHeading = height;                       // PlayerPed.h:41
	if (m_fFPSMoveHeading >  DEGTORAD(45.0f)) m_fFPSMoveHeading =  DEGTORAD(45.0f);
	if (m_fFPSMoveHeading < -DEGTORAD(45.0f)) m_fFPSMoveHeading = -DEGTORAD(45.0f);
	if (CCamera::s_viceExtAim.storiesPointingArm && info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
		m_fFPSMoveHeading -= DEGTORAD(8.0f);         // Main.cpp:1300: el -8 grados
	// Main.cpp:1302-1305
	float torsoPitch = 0.0f;
	if (!info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || bIsDucking)
		torsoPitch = m_fFPSMoveHeading;
	// Main.cpp:1307-1308
	if (m_vecMoveSpeed.Magnitude() < 0.01f)
		CCamera::s_viceExtAimForceRealMoveAnim = true;   // Main.cpp:31; lo lee C1 (B10)
	// Main.cpp:1310-1312 — los TRES MoveLimb. OJO con los nombres: el mod usa
	// m_PedIK.m_sHead / m_sTorso / m_sLowerArm; en nuestro arbol son
	// m_pedIK.m_headOrient / m_torsoOrient / m_lowerArmOrient (PedIK.h:38/39/41).
	// Las tablas estaticas SI tienen el mismo nombre: ms_headInfo (PedIK.h:45),
	// ms_torsoInfo (:44), ms_lowerArmInfo (:48). Firma: PedIK.h:59.
	m_pedIK.MoveLimb(m_pedIK.m_headOrient,    m_pedIK.m_headOrient.yaw, 0.0f,            CPedIK::ms_headInfo);
	m_pedIK.MoveLimb(m_pedIK.m_torsoOrient,   0.0f,                       torsoPitch,      CPedIK::ms_torsoInfo);
	m_pedIK.MoveLimb(m_pedIK.m_lowerArmOrient, 0.0f,                      m_fFPSMoveHeading, CPedIK::ms_lowerArmInfo);
```
El primer `MoveLimb` usa `m_sHead.m_fYaw` como yaw destino (`Main.cpp:1310`), o sea **el yaw que
tiene la cabeza ahora mismo**: es un «no-op conHistory» que asegura que la cabeza no se mueva. Se
copia literal.

`ViceExtRotatePlayer` se copia de `Main.cpp:547-574` con dos cambios **imprescindibles**:
`ped->m_matrix.SetRotateZOnly(...)` (`Main.cpp:571` → nuestro `GetMatrix().SetRotateZOnly(...)`,
`Matrix.h:94`) y `CGeneral::LimitRadianAngle` (ya existe). El `ignoreRotation` del mod se mantiene
como static en B1 (aunque en VC el mod nunca lo pone a `true`: `Main.cpp:1225` lo pone a `false` y no
hay más asignaciones — se comprueba en el bloque).

`m_vecMoveSpeed` es `Ped.h` (`CPed`), y `bIsDucking` es el bitfield de `m_nPedFlags`.

**Detalle técnico — (f) lock-on y suelta (mod `Main.cpp:1252-1286`, C14 + §5.2a)**

Con `§5.2(a)` (con ratón no hay auto-aim) la rama se queda así:
```
	// §5.2(a): con raton NO se busca fijado. El cambio de fijado MANUAL
	// (ShiftTargetLeftJustDown/RightJustDown) se conserva: es como se fija uno a mano.
	if (!ViceExtIsAutoAimDisabled()) {
		if (m_bHasLockOnTarget && m_pPointGunAt) {
			... front = CGeneral::GetATanOfXY(diff.x, diff.y) - HALFPI;   // Main.cpp:1255
			... altura desde la posicion en pantalla del objetivo            // Main.cpp:1257-1271
			if ((transitionDone && (Abs(MouseX) > 1.0f || Abs(MouseY) > 1.0f))
			    || diff.Magnitude() < 0.5f)
				ClearWeaponTarget();                                        // Main.cpp:1280
		} else if (pad->ShiftTargetLeftJustDown() || pad->ShiftTargetRightJustDown()) {
			FindWeaponLockOnTarget();                                       // Main.cpp:1284
		}
	}
```
`Abs` es el de casa. `FindWeaponLockOnTarget` es `PlayerPed.h:118` / `PlayerPed.cpp:1417`.
`ClearWeaponTarget` es `PlayerPed.cpp:299` y ya llama a `TheCamera.ClearPlayerWeaponMode()` y
`CWeaponEffects::ClearCrossHair()` — idéntico al `ClearWeaponTarget` propio del mod (`Main.cpp:906-911`),
así que **no** se escribe una copia (RULES 0.6).

**La altura (el «pitch del brazo»)**: `Main.cpp:1269-1271` calcula
`CSprite::CalcScreenCoors(in, &out, &in.x, &in.y, false)` y luego
`height = Find3rdPersonQuickAimPitch(out.y / SCREEN_HEIGHT)`. Nuestro equivalente es
`TheCamera.Find3rdPersonQuickAimPitch()` (B8) y **`SCREEN_HEIGHT` es `RsGlobal.screenHeight`**
(Hud.cpp:391 usa `SCREEN_HEIGHT`, y `Hud.cpp:463` `CSprite::RenderOneXLUSprite(SCREEN_WIDTH / 2, ...)`).
Como el parámetro del mod es código muerto, el `out.y / SCREEN_HEIGHT` **no se necesita**: nuestra
firma no lo tiene. Se anota.

El «bone 1» del objetivo es `PED_HEAD` (`PedModelInfo.h:10`), y `m_pedIK.GetComponentPosition(in,
PED_HEAD)` (`PedIK.h:54`); `in.z += 0.25f` (mod `:1266`).

`ViceExtIsAutoAimDisabled()` = `!hasPadInHands && !s_viceExtAim.forceAutoAim` (`Main.cpp:330`,
`:475`, `:1250`). **La equivalencia de `hasPadInHands` en web es `PENDIENTE`**: ver §10.18-D; el
enriquecimiento propone `CPad::GetPad(0)->GetRightStickX() || GetRightStickY() || <botón de stick
conAxis>` como detector de «hay mando», y **no** se fija hasta que el jugador confirme la regla.

**Detalle técnico — (g) `WalkKey` (mod `Main.cpp:1196-1201`, `1215-1217`, C17)**

El efecto es una línea, al principio de la función: `if (ControlsManager.ms_apBindings
[PED_WALK]->m_sValue.iKey != KEY_NULL) ...` — la forma exacta la da B13. El efecto, `m_fMoveSpeed = 0.0f;`
(`PlayerPed.h:11`).

**Detalle técnico — (h) C18-3 y C18-4 (mod `Main.cpp:432-441` y `:1393-1415`)**

- **C18-3** (al encontrar **nuevo** fijado): va en `CPlayerPed::FindWeaponLockOnTarget`, justo antes
  del `SetWeaponLockOnTarget(nextTarget)` de `PlayerPed.cpp:1460`, con la condición del mod invertida
  (`Main.cpp:440-446`): el mod hace `ClearPointGunAt() + ClearWeaponTarget()` cuando **NO** se cumple
  `(!bIsDucking || info->m_bCrouchFire)`. Ojo: el comentario de §5.5 lo describe como «reactiva
  `bCrouchWhenShooting`», y el código del mod hace lo contrario (limpia el fijado). **Se porta el
  código**, y el texto de §5.5 se corrige en el informe.
- **C18-4** (al soltar el apuntado agachado, con munición y sin recargar): `Main.cpp:1399-1408` son, en
  nuestro árbol, `bCrouchWhenShooting = true; SetDuck(60000, true);` — **`SetDuck` ya mezcla
  `ANIM_STD_DUCK_WEAPON` con `delta 4.0f`** cuando `bCrouchWhenShooting` (`PedAI.cpp:5928-5930`), que
  es exactamente el `BlendAnimation(ANIM_GROUP_MAN, ANIM_MAN_WEAPON_CROUCH, 4.0f)` del mod
  (`Main.cpp:1406`). **Son dos líneas, no una reimplementación de la mezcla.** `ANIM_MAN_WEAPON_CROUCH` =
  `ANIM_STD_DUCK_WEAPON` (`AnimationId.h:185`) y `ANIM_GROUP_MAN` = `ASSOCGRP_STD`
  (`AnimManager.h:8`).

**Restricciones**
- **`PlayerPed.cpp` es territorio del carril de nado** (§3.2). Bloqueo de propiedad: este bloque no se
  ejecuta hasta que ese carril cierre. Es el paso 1 de §7 y lo primero de §10.20.
- Prohibido tocar `ViceExtSwimControl` (`:4465`), `ViceExtCrouchControl` (`:4472`),
  `ViceExtCrouchLimitSpeed` (`:4512`) y `odAimWalkActive/Uncapped` (`:50-51`): son de otros carriles.
- El bloque va **antes** de `CPed::ProcessControl()` (`:4321`) por la razón de B9-verificación (§10.18-1).
- `CRLF`, python binario, `assert count == 1`.
- Lo que entra en `PlayerPed.h` es **una sola** declaración pública por helper nuevo, dentro del bloque
  `VICEEXT_*` (`:77-98`), con el patrón de `ViceExtIsSwimming`/`ViceExtIsCrouched`.

**Verificación**
- `AIMLAW aim=1 modo=%d estado=%d arma=%d lock=%d aut=0 esp=%.2f aim2=%d` — 1 línea por **flanco** de
  `isAiming` (patrón edge de `Cam.cpp:2605-2608`), no 1 Hz: lo que hay que ver es la transición.
- `AIMLOCK hx= vx= tmo=250 off=raton|cerca|solo` — edge-triggered; §8.7 y §8.9.
- `AIMIK pitch= alto= torso= h= t= a=` — 1 Hz apuntando; §8.3.
- `AIMWPN block=1` (C8) y `AIMSPRAY spray=%.5f` (C9) en B12.
- `SWIM2`/`SWIMNAT` **sin cambios** (§8.16): es el criterio de no-regresión del carril ajeno.

---

#### B10 · Adquisición del objetivo blando, C1/C2, C8 y C10 — `src/peds/PlayerPed.cpp` (+ `PedFight.cpp`)

**Ficheros / anclas**
- `src/peds/PlayerPed.cpp` — `ViceExtFind3rdPersonMouseTarget`, nueva, junto a
  `CPlayerPed::FindWeaponLockOnTarget` (`:1417`).
- `C1`/`C2`: el equivalente de `GetMoveAnimTaskType` / `playerShootingDirection` en nuestro árbol.
- `C8`: `CPlayerPed::ProcessWeaponSwitch` (`:992`), y el `bDontAllowWeaponChange` estático
  (`PlayerPed.h:54`), que ya se limpia en `PlayerPed.cpp:4657-4660`.
- `C10`: los 4 callsites de `CWeapon::Fire` — `PedFight.cpp:875`, `:919`, `PlayerPed.cpp:978`, `:1118`.

**Acción**: `edit` (los dos).

**Detalle técnico — (a) `Find3rdPersonMouseTarget` (mod `Main.cpp:1474-1517`)**

```
	// Main.cpp:1474-1517. El mod retorna si hay mando (HasPadInHands, PENDIENTE-D).
	CVector source, target;
	CColPoint point = {};
	CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType);
	if (CCamera::s_viceExtAimLawActive && !bInVehicle && (!m_bHasLockOnTarget || !m_pPointGunAt)) {
		// Main.cpp:1488: dist = info->m_fRange (WeaponInfo.h:42), pos = cam->m_vecSource.
		if (TheCamera.Find3rdPersonCamTargetVector(info->m_fRange,
			TheCamera.Cams[TheCamera.ActiveCam].Source, source, target)) {
			CEntity *e = nil;
			if (CWorld::ProcessLineOfSight(source, target, point, e,
			    false, false, true, false, false, false, false, false)) {   // Main.cpp:1489-1492
				if (e && e->IsPed()) {
					CPed *t = (CPed*)e;
					if (t != this && t->m_nPedState != PED_DEAD) {
						if (CCamera::s_viceExtAimMouseTarget != t) {
							CCamera::s_viceExtAimMouseTarget = t;
							if (t->CanSeeEntity(this, DEGTORAD(60.0f) * 2)) {   // Ped.h:711
								t->ReactToPointGun(this);                        // Ped.h:837
								Say(SOUND_PED_AIMING);                           // Ped.h:677
							}
						}
					}
				}
			}
		}
	} else
		CCamera::s_viceExtAimMouseTarget = nil;
```
Los 12 flags de `ProcessLineOfSight` del `Main.cpp:1489-1493` son `false,false,true,false,false,
false,false` + el `false` de VC: **sólo `checkPeds`**. Es un rayo que sólo ve peds, que es lo que
permite dibujar el triángulo sobre un ped y no sobre una pared.

`Find3rdPersonCamTargetVector` es `Camera.h:624` / `Camera.cpp:4228-4247` y **ya es** la
reimplementación de la del mod (`Main.cpp:510-539`). **No se toca** (§9 y §7 no lo listan), con una
nota escrita: la rama de `GetLookBehindForPed` del mod usa `dist * mat.up` (`Main.cpp:522`) donde
nuestro árbol usa `dist * GetForward()` (`Camera.cpp:4233`) — el del mod es un **bug** (mirar-atrás
apuntaría al cielo) y portarlo sería empeorar el código. Queda documentado como desviación consciente.

**`Say(117)` del mod (`Main.cpp:1504`) es `PENDIENTE`**: no sabemos qué índice es `117` en la tabla de
sonidos de ped servida. Recomendación: usar el `SOUND_PED_AIMING` con nombre, que es lo que ya
reproduce nuestro propio `FindWeaponLockOnTarget` (`PlayerPed.cpp:1463`) — si `117` fuera otra cosa,
es un sonido que el jugador oye una vez al pasar el ratón por un ped, con consecuencia nula. Se
documenta la desviación.

**Detalle técnico — (b) C1 y C2: `TYPE_STRAFE` / `TYPE_WALKAROUND`**

El mod redirige 6 llamadas a `GetMoveAnimTaskType` y 4 a `playerShootingDirection` (`Main.cpp:138`,
`:175`). En nuestro árbol **el equivalente no es una redirección de parche sino un predicado**: el
clip y el modo de control los elige ya `PlayerPed.cpp` (el bloque de `odAimWalkActive`, `:1995-1996`,
y la cadena de `SetMoveAnim`/`ReApplyMoveAnims`, `:1477-1540`), y §2.1/H4 lo declararon
`**YA ESTABA**` con otra semántica.

Lo que sí esTranslator sin ambigüedad es el **modo de control** (`strafe`), que el log ya publica
como `CROUCH2 ... strafe=%d` (`check-served-build.sh:73`, marca `strafe=%d`, R20). Por eso:
- **C1** (`:129-130`): con `isAiming && !ignoreRotation` el modo es de **costado**. Nuestro árbol ya
  lo hace por `CanStrafeOrMouseControl()` / `Using3rdPersonMouseCam()` (`PedFight.cpp:1963`,
  `PlayerPed.cpp:4495`). **No se añade código**: se verifica con `CROUCH2 strafe=`.
- **C2** (`:147-148`): con `isAiming && !m_bHasLockOnTarget` también costado. **Misma** condición
  que el motor ya usa, así que tampoco hay código; lo que se documenta es que *con fijado* el
  jugador **no** va de costado (el mod tampoco).
- `forceRealMoveAnim` (`:1307-1308`, `Main.cpp:121-127`): `|moveSpeed| < 0.01` fuerza
  `TYPE_WALKAROUND` un frame. **Copia sí**: en `PlayerPed.cpp`, cuando
  `m_vecMoveSpeed.Magnitude() < 0.01f` yappointando, se llama `SetRealMoveAnim()` una vez (el
  equivalente de nombre de `TYPE_WALKAROUND` en nuestro árbol es `CPed::SetRealMoveAnim`,
  `PlayerPed.h:102`), protegido por el flag de un tiro. Se anota que la condición del mod es
  `m_vecMoveSpeed.Magnitude() < 0.01f` y que la nuestra lista H4 sigue mandando en el resto.

**Detalle técnico — (c) C8: cambio de arma bloqueado mientras se apunta (mod `Main.cpp:372-384`)**

El mod envuelve `ped->ProcessWeaponSwitch(pad)`. Nuestro equivalente es más barato porque
`CPlayerPed::ProcessWeaponSwitch` ya está **condicionado por `!m_pPointGunAt && !bDontAllowWeaponChange`**
(`PlayerPed.cpp:997`) y `bDontAllowWeaponChange` se pone a `true` en `FindWeaponLockOnTarget`
(`:1461`) y se limpia en `:4657-4660`. La línea que falta es la del «con el *aimed* activo pero **sin**
fijado» (que es el caso normal de §5.2a: con ratón nunca hay fijado):
```
	// ClassicAXIS C8 (Main.cpp:372-384): con el apuntado activo no se cambia de arma.
	// El motor ya bloquea por bDontAllowWeaponChange, pero solo cuando hay fijado
	// (PlayerPed.cpp:1461); con raton y §5.2(a) no hay fijado, asi que se pone aqui.
	if (CCamera::s_viceExtAimLawActive)
		bDontAllowWeaponChange = true;
```
en la cabeza de `ProcessWeaponSwitch`, y el `bDontAllowWeaponChange = false;` que ya hay en
`PlayerPed.cpp:4657-4660` (cuando `!GetTarget()`) es lo que lo devuelve. Traza `AIMWPN block=1`
(edge, no 1 Hz).

**Detalle técnico — (d) C10: `MODE_FOLLOW_PED` temporal durante el disparo (mod `Main.cpp:159-170`)**

Sin tocar `Weapon.cpp` (C-3). En **cada** uno de los 4 callsites:
```
	// ClassicAXIS C10 (Main.cpp:159-170): el mod mete MODE_FOLLOW_PED mientras dura
	// CWeapon::Fire y lo devuelve al salir (el SDK lo hace en el .after, que corre
	// tambien con return false). Aqui es un par de lineas alrededor de la llamada, que
	// es el unico sitio donde sabemos cuando empieza y cuando acaba. Sin esto, apuntar
	// rompe el mvl y los parabrisas rompibles (el bug que el mod arregla).
	int16 odSaved = TheCamera.Cams[TheCamera.ActiveCam].Mode;
	if (odSaved != CCam::MODE_FOLLOWPED && CCamera::s_viceExtAimLawActive)
		TheCamera.Cams[TheCamera.ActiveCam].Mode = CCam::MODE_FOLLOW_PED;
	GetWeapon()->Fire(this, &firePos);
	if (odSaved != CCam::MODE_FOLLOWPED && CCamera::s_viceExtAimLawActive)
		TheCamera.Cams[TheCamera.ActiveCam].Mode = odSaved;
```
El guardia `s_viceExtAimLawActive` es lo que hace que **fuera** de apuntar no cambie nada (el mod
tampoco lo haría: su hook guarda `savedCamMode` y sólo actúa si es `-1`, y lo pone siempre… matiz:
el mod lo pone **siempre**, sin mirar si se apunta. Se decide aquí **con** el guardia, que es más
conservador y se anota como desviación mínima; el jugador puede pedirlo literal cambiando dos
condiciones).

**Restricciones**
- `PedFight.cpp:875/919` es **carril de agachado (R28)**: esos dos callsites se tocan en el mismo
  commit que B12, no antes (§10.19).
- `ProcessWeaponSwitch` tiene dos listas de `PlayerWeaponMode` (`:1000-1004` y `:1022-1026`) que son de
  la 1.ª persona: **no se tocan**. El bloqueo del aimed va antes, y el motor ya sale por
  `switchDetectDone` si hace falta.
- C1/C2 **no** reconvierten el sistema de locomotion (H4). Nada de `ped.ifp` ni datos.
- `SetRealMoveAnim` es 1 llamada con flag de un tiro; no se introduce estado nuevo en `PlayerPed.h`.

**Verificación**
- `AIHTRI h=1` con `rec=1` cuando hay objetivo blando, `AIHTRI h=0` al soltarlo (§8.8).
- `CROUCH2 strafe=1` apuntando sin fijado y `CAMB2b`/dem—no toca la cámara de coche (§8.12/§8.16).
- `AIMWPN block=1` al pulsar la rueda con el aimed activo: la traza sale del **flanco**, y el arma no
  cambia (§8.11).
- `AIMSHOT modo=4 n=1` (edge, una por disparo) mientras se dispara apuntando: prueba de que C10
  entra y sale. Sin esta línea, C10 no es medible y no se puede dar por bueno.

---

#### B11 · El HUD: las 3 funciones del mod — `src/renderer/Hud.cpp`

**Ficheros / anclas**
- `src/renderer/Hud.cpp` — `CHud::Draw()`, que empieza en `:328`; la caja por arma en `:389-424`; el
  marco de 4 rects del fijado en `:541-571`.
- Primitivas: `src/renderer/Sprite2d.h:24-29` (`Draw`), `:41-45` (`DrawRect`, `DrawAnyRect`),
  `:47` (`Draw2DPolygon`).

**Acción**: `edit`.

**Detalle técnico — (a) el hallazgo que obliga a tocar `Hud.cpp`**

`DrawCrossHairPC` sólo se pone a `true` si `Using3rdPersonMouseCam()` (`Hud.cpp:362-364`), y eso
exige `Mode == MODE_FOLLOWPED` (`Cam.cpp:1133`). **En `MODE_AIMING` la mira de 3.ª persona actual
desaparece** en cuanto el jugador apunta. No es opcional: sin tocar `Hud.cpp` el plan deja al jugador
**sin retícula al apuntar**, que es justo lo que el mod arregla. Por eso la rama de `:389` se
extiende (o se sustituye) en vez de dejarse como está.

Las condiciones del mod (`Main.cpp:750-777`), literales y con el mapa:
```
	// ClassicAXIS DrawCrosshair (Main.cpp:745-781). Solo si la LEY esta activa: el mod
	// exige m_nCamMode == MODE_AIMWEAPON (:750) y m_nTransitionState == 0 (:753).
	if (CCamera::s_viceExtAimLawActive && TheCamera.m_uiTransitionState == 0) {
		if (playerPed && !playerPed->bInVehicle && !playerPed->m_bHasLockOnTarget
		    && !CPad::GetPad(0)->ArePlayerControlsDisabled()
		    && ViceExtIsWeaponPossiblyCompatible(playerPed)) {
			float x = SCREEN_WIDTH  * TheCamera.m_f3rdPersonCHairMultX;
			float y = SCREEN_HEIGHT * TheCamera.m_f3rdPersonCHairMultY;
			CHud::Sprites[HUD_SITEM16].Draw(
				CRect(x - SCREEN_SCALE_X(14.0f), y - SCREEN_SCALE_Y(14.0f),
				      x + SCREEN_SCALE_X(14.0f), y + SCREEN_SCALE_Y(14.0f)),
				CRGBA(255, 255, 255, 255), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
		}
	}
```
Los **`±14 px`** son del mod (`Main.cpp:776`, `ScaleX/ScaleY(14.0f)`) y **entran**: `SCREEN_SCALE_X/Y`
es nuestro `ScaleX/ScaleY` (el propio `Hud.cpp:400-403` los usa igual). `HUD_SITEM16` y la sobrecarga
`Draw` de 10 parámetros son los mismos (`Hud.cpp:408-409`).
`RsGlobal.screenWidth/Height` del mod = `SCREEN_WIDTH`/`SCREEN_HEIGHT` nuestros (los usa
`Hud.cpp:390-391`).
`CHud::Sprites` es `CHud` estático, y `Hud.cpp` **ya** es la casa de estas tres funciones.
El estado de render del mod (`Main.cpp:756-761`: `VERTEXALPHAENABLE`, `SRCBLEND`, `DESTBLEND`,
`FOGENABLE`, `ZWRITEENABLE`, `ZTESTENABLE`) **ya está puesto** por `Hud.cpp:379-384` y restaurado en
`:533-535` ⇒ no hay que añadirlo (se documenta).

**Detalle técnico — (b) `DrawAutoAimTarget`: las dos marcas (mod `Main.cpp:783-862`)**

Y **§5.2(a)**: con ratón esta función **no se llama** (`:330-332`), así que la marca de lock-on
desaparece. El ajuste `LockOnTargetType` sigue teniendo efecto sobre el aspecto cuando hay fijado manual.

Numeros, del mod, sin redondear:
- `timeLockOn = 250 + CTimer::GetTimeInMilliseconds()` (`:828`) ⇒ ventana de **250 ms**.
  `CTimer::GetTimeInMilliseconds()` existe (`Timer.h`).
- `lastLockOnPos` = cabeza del ped vía `m_PedIK.GetComponentPosition(lastLockOnPos, 1)` (`:810`) ⇒
  nuestro `m_pedIK.GetComponentPosition(pos, PED_HEAD)` (`PedIK.h:54`, `PedModelInfo.h:10`), y
  `lastLockOnPos.z += 0.25f` (`:824`).
- color SA: `CRGBA((1.0f - health) * 255, health * 255, 0, 255)` (`:815`); negro si `health <= 0`
  (`:819-820`); `health = m_fHealth / 100.0f` (`:812`).
- color LCS: `CRGBA(0, health * 255, 0, 255)` con `col.a = 150` (`:817`, `:851`).
- `dist = w / 128.0f` (`:850`); para SA además `dist = (w/128.0f) * (float)(timeLockOn / (250 +
  now))` (`:843`) — la escala por el tiempo restante.
- `rotMult = 0.5f` y **`rotMult = 3.0f` si se acaba de perder el fijado** (`targetMode == 0`,
  `:836`, `:844-846`).
- `LockOnTargetType == 0` ⇒ no dibuja nada (`:784-785`).

**Detalle técnico — (c) la geometría: reimplementación, y por qué eso es lo correcto**

`DrawSATarget`, `DrawLCSTarget` y `DrawSATriangleForMouseRecruit` **no están** en `tmp/extsrc/`
(§1 hueco 2): se llaman (`:847`, `:853`, `:900`) y no se bajaron. **No se copian y no se inventan sus
puntos**: se dibujan con las primitivas que ya usa nuestro `Hud.cpp`:
- `CSprite2d::DrawRect(const CRect&, const CRGBA&)` (`Sprite2d.h:42`) — 4 lados, ya usado en
  `Hud.cpp:566-569` para el marco del fijado. Es el mismo «4 rectángulos» de siempre pero con la
  rotación y el `dist` del mod.
- `CSprite2d::Draw2DPolygon(x1,y1,x2,y2,x3,y3,x4,y4, col)` (`Sprite2d.h:47`) — **cuadrilátero libre**,
  y es exactamente el primo de una marca **rotada** por `rotMult`: se calculan los 4 vértices
  girados alrededor del centro con `Cos`/`Sin` de `rotMult * tiempo` y se pasan. Precedente de uso en
  el árbol: `Frontend.cpp:4087-4090` (marcadores de radio, 4 puntos).
- Para el **triángulo**: `Draw2DPolygon` con el cuarto vértice degenerado sobre el segundo, o
  `CSprite2d::SetVertices(int n, float*, float*, const CRGBA&)` (`Sprite2d.h:36`) con `n = 3` +
  `RenderVertexBuffer()` (`Sprite2d.h:53`). **La reimplementación de la forma es nuestra** y se dice
  en la cabecera de atribución del bloque, literalmente: *«la geometría de las marcas no está en las
  fuentes del mod; lo que hay aquí es una reimplementación con las primitivas de
  `CSprite2d`, no una copia»* (§9, `docs/mods/ATTRIBUTION.md`).

Estado de render: el mod pone los mismos 6 estados en las tres funciones (`:787-792`, `:868-873`); en
nuestro `Hud.cpp` **ya están** puestos por la cadena de `:379-384`, así que las tres llamadas nuevas no
los tocan (y el bloque de `:541-571` que sustituyen tampoco los tocaba).

**Detalle técnico — (d) `DrawTriangleForMouseRecruitPed` (mod `Main.cpp:864-904`)**

`alpha 150` en el color, `z = cabeza + 1.0f` (`:891`), `dist = w / 128.0f` (`:900`), color por salud
como el SA pero con `alpha = 150` en el cuarto argumento (`:894`), negro si `health <= 0` (`:896-897`),
`z = cabeza + 1.0f` vía `GetComponentPosition(in, 1)` (`:890-891`). El objetivo es
`CCamera::s_viceExtAimMouseTarget` (B10) y la condición es
`OurPedCanSeeThisOne(thirdPersonMouseTarget, true)` (`Ped.h:686`, el `true` es el `, true` de VC del
mod en `:878-881`).

**Restricciones**
- La geometría es **nuestra**, y el comentario de atribución lo dice. Prohibido «aproximar» la del mod
  y llamarlo port.
- `Hud.cpp` está bajo `#ifdef __EMSCRIPTEN__` para algunas trazas: las nuevas trazas también, con
  `#else` no-op (el patrón de `PlayerPed.cpp:1580-1582`).
- No tocar `Radar.cpp`, `Messages.cpp` ni el resto de la pantalla del HUD.
- El bloque de `:541-571` (marco del fijado) **se sustituye**: es la misma idea que la marca SA del
  mod, sólo que sin tiempo, sin giro y sin tipo. Si se sustituye, `LockOnTargetType == 0` se queda
  **sin marca** (mod `:784`) ⇒ es un cambio visible que el jugador tiene que ver en la ronda.
- `Hud.cpp` es territorio de nadie (riesgo bajo en §3.2): el único carril que lo ha tocado es el de
  las miras (D6/`VICEEXT_WEAPON_SIGHTS`, `Hud.cpp:405-423`), que **no** se solapa con `:541-571` ni
  con el nuevo bloque. Aun así, si `VICEEXT_WEAPON_SIGHTS` está activo, la caja por arma se dibuja en
  `ViceExtDrawSight` y la del mod se dibuja en `Sprites[HUD_SITEM16]`: **no se sustituye una por la
  otra**, se.Documenta y el jugador ve las dos si las condiciones se solapan (→ D4).

**Verificación**
- `AIMHUD cruz=%d marco=%d tipo=%d tri=%d rots=%.1f d=%.4f` — 1 línea por segundo **sólo si hay algo
  dibujado** (edge sobre el conjunto de banderas, patrón `Hud.cpp:304-315`), para no inundar.
- §8.4: la mira de 3.ª persona cae sobre el hombro, no detrás de la cabeza.
- §8.7: la marca de salud aparece 250 ms tras fijar y se va (`AIMLOCK tmo=250`).
- §8.8: `AIHTRI h=1/0`.
- **Antes/después:** el `strafe=%d` y el resto de marcas de `check-served-build.sh` siguen dando lo
  mismo: B11 no toca el motor.

---

#### B12 · `PedFight.cpp`: C9 y los tres C18 que caen aquí — `src/peds/PedFight.cpp`

**Ficheros / anclas**
- `C9`: `CPlayerPed::DoWeaponSmoothSpray` (`PlayerPed.cpp:823-`), cuyo único uso es
  `PlayerPed.cpp:1962` (`float smoothSprayRate = DoWeaponSmoothSpray();`). El mod parchea
  `doWeaponSmoothSpray` (VC, `Main.cpp:403`) con retorno `float`.
- **C18-1**: `CPed::SetPointGunAt` (`PedFight.cpp:161-210`), sobre todo el bloque R28 de `:182-207`.
- **C18-2**: `CPed::ClearPointGunAt` (`PedFight.cpp:240-`).
- **C18-5**: el bloque `if(!aimAssoc || aimAssoc->blendDelta < 0.0f)` de `SetPointGunAt`
  (`PedFight.cpp:198-207`), donde R28 ya pone `blendAmount = 0.0f; blendDelta = 8.0f` (`:205-206`).

**Acción**: `edit`.

**Detalle técnico — (a) C9: el spray suave (`Main.cpp:387-404`)**

El retorno de la versión VC es `0.00001f` si `bIsDucking` y `-1.0f` si no (`:394-397`). El mod
**sustituye la función entera** por esos dos `return` (`Main.cpp:403` redirige la llamada; el cuerpo
de la ORIGINAL se descarta). Nuestro `DoWeaponSmoothSpray` (`PlayerPed.cpp:823`) tiene un `switch` por
tipo de arma (`:828`) con `GOLFCLUB`/`NIGHTSTICK`/`BASEBALLBAT`/`CHAINSAW` que devuelve `PI / 176.f`
o `-1.0f`, y su único consumidor es `PlayerPed.cpp:1962`, donde el gate es
`if (smoothSprayRate > 0.0f && upDown > 0.0f)` (`:1974`).

Traducción fiel = **sustituir el cuerpo**, con una consecuencia que hay que decir: los casos de arma
del `switch` dejan de aplicar, y con ello el bateo de piadas (el `PI / 176.f` de `:833`, con los `case` en `:829-832`). Es
exactamente lo que hace el mod, y con el valor del mod el gate de `:1974` se abre **sólo** agachado
(`0.00001f > 0`), que es la intención del mod.
```
	// ClassicAXIS C9 (Main.cpp:387-404, VC): el mod SUSTITUYE la funcion entera por
	// estos dos returns: 0.00001f si esta agachado, -1.0f si no. Se pierde el caso de
	// bateo del switch de armas que hay antes (PI/176.f): es lo que hace el mod, y su
	// efecto es que el gate de PlayerPed.cpp:1974 (smoothSprayRate > 0) solo se abra
	// agachado, que es justo lo que se quiere.
	if (bIsDucking)
		return 0.00001f;
	return -1.0f;
```
Y se **borra** el `switch` de `PlayerPed.cpp:829-…` (si no, es código muerto y el compilador avisa).
La consecuencia —«el bateo de piadas ya no tiene spray suave»— se anota en el informe para el jugador.

**Detalle técnico — (b) C18-1: el choque con R28 (`Main.cpp:408-420`)**

El mod, en el `SetDuck` del jugador y apuntando: si el arma **no** tiene `CrouchFire` y no es cuerpo a
cuerpo → `bCrouchWhenShooting = false;` + `RestorePreviousState()`.

R28 hizo justo lo contrario: `CPed::ViceExtCrouchShooting()` (`Ped.h:1080`, cuerpo en `PedFight.cpp`)
acepta `bCrouchWhenShooting && bIsDucking` **o** el agachado del port (R6) para que el motor use los
clips `*_crouchfire` que sí están en los `.ifp` servidos y en el `weapon.dat` (`agachado-calibrado`
R28, `:302-306`). Los dos no pueden ser verdad a la vez: §5.5 lo reconoce y pide aplicar el del mod
midiendo la pérdida.

Implementación, sin tocar `ViceExtCrouchShooting` (es la base de 6 llamadas en `PedFight.cpp`):
```
	// ClassicAXIS C18-1 (Main.cpp:408-420): apuntando agachado, si el arma NO tiene
	// WEAPONFLAG_CROUCHFIRE, se apaga bCrouchWhenShooting y se restaura el estado.
	// CHOCA con R28 (crouch2), que hizo lo contrario para poder usar los clips
	// *_crouchfire. Se aplica el del mod (§5.5) y se mide lo que se pierde (§8.15).
	if (IsPlayer() && bIsDucking && !curWeapon->IsFlagSet(WEAPONFLAG_CROUCHFIRE)
	    && !GetCrouchFireAnim(curWeapon)) {
		bCrouchWhenShooting = false;
		RestorePreviousState();
		return;
	}
```
colocado **al principio** de `SetPointGunAt` (`PedFight.cpp:161`), antes del bloque R28 de `:182`. El
`!GetCrouchFireAnim(curWeapon)` es lo que hace la versiónVC del mod equivalente (su
`!IsTypeMelee(ped)` es el guard de «no es cuerpo a cuerpo»); se usa el predicado del árbol porque
`IsTypeMelee` es de `CPlayerPed` (`PlayerPed.h`) y aquí estamos en `CPed`.
`RestorePreviousState` es `Ped.cpp:795`. `SetCrouchFireAnim`/`GetCrouchFireAnim` ya existen
(los usa R28 en `:189`, `:192`).

**Detalle técnico — (c) C18-2: al levantarse (`Main.cpp:422-429`)**

`ClearPointGunAt()` en el camino de «se levantó del agachado». Nuestro `CPed::ClearPointGunAt`
(`PedFight.cpp:240`) es el sitio natural: se le añade, **para el jugador**, un
`if (IsPlayer()) { … }` con la condición de que el estado anterior era agachado. El «bandera de
estado» es `bCrouchWhenShooting` (el mod usa `wasCrouching`, `Main.cpp:29`, que es un static suyo). En
nuestro árbol **no hace falta bandera nueva**: `wasCrouching ≡ m_bWasDuckingCrouch`, un static de B1
que se pone en el bloque de apuntado y se limpia al final, exactamente como el mod (`:1384-1390`).

**Detalle técnico — (d) C18-5: la entrada del assoc (`Main.cpp:1368-1378`)**

El mod, en vez de `AddAnimation` (lenta), pone `assoc->m_fBlendAmount = 0.0f;` y
`assoc->m_fBlendDelta = 8.0f` (`:1376-1377`). **Nuestro `SetPointGunAt` ya lo hace exactamente así**:
`PedFight.cpp:205-206` es `aimAssoc->blendAmount = 0.0f; aimAssoc->blendDelta = 8.0f;`. **C18-5 es
`YA ESTABA`** (se anota en `docs/mods/ATTRIBUTION.md` con ese estado, y no se toca el bloque).

**Restricciones**
- `PedFight.cpp` es **carril de agachado (R28, `EXECUTED`)**. B12 requiere cerrar el carril de nado
  (§3.2) **y** que el dueño de R28 libere el fichero. Los bloques B10(d), B12(b) y B12(c) caen en la
  misma zona.
- **Prohibido tocar** `ViceExtCrouchShooting` (su cuerpo en `PedFight.cpp`, sus 6 usos en `:189`,
  `:192`, `:199`, `:410`, `:525`, `:547`, `:605`, `:699`) ni `GetCrouchFireAnim` /
  `GetCrouchReloadAnim`. C18-1 se añade **por encima**, con `return` propio.
- Prohibido tocar la matemática de `smoothSprayRate` en su consumidor (`PlayerPed.cpp:1962`) y
  prohibido tocar `recoil-por-arma-y-cadencia.md`.
- Sin `ped.ifp` ni `weapon.dat`: **esto no toca datos** (ni `C18-1` ni `C9`).

**Verificación**
- `AIMSPRAY spray=%.5f agach=%d` — 1 línea por **flanco** del valor de retorno (edge), no 1 Hz: con
  el valor y si está agachado. Criterio: agachado ⇒ `0.00001`, de pie ⇒ `-1.00000`. Con el bateo, se anota
  que ya no aparece `PI/176.f` (es el coste de portar la spec literal).
- `AIMCROUCH crouchfire=%d fue=%d arma=%d` en el bloque de C18-1 (edge, una vez por cambio de arma o
  de estado de agachado). Criterio: arma **con** `WEAPONFLAG_CROUCHFIRE` ⇒ `crouchfire=1` y el motor
  sigue usando el clip `*_crouchfire` (§8.15, `CROUCH2 nomarma=*_crouchfire`); arma **sin** el flag ⇒
  `crouchfire=0` y se anota la pérdida esperada.
- `CROUCH2` (R27) **da lo mismo** en todo lo que no sea `nomarma`: es el criterio de no-regresión del
  carril ajeno (§8.15). Si el bloque `R27` pasa a `FAIL` por `nomarma`, es el **coste esperado** de
  §5.5 y hay que decirlo, no arreglarlo por la espalda.

---

#### B13 · `PED_WALK`: el `WalkKey` como acción rebindable — `ControllerConfig.h/.cpp`, `re3.cpp`, `PlayerPed.cpp`

**Ficheros / anclas**
- `src/core/ControllerConfig.h` — `enum e_ControllerAction`, justo antes de `MAX_CONTROLLERACTIONS`
  (`ControllerConfig.h:67-68`).
- `src/core/ControllerConfig.cpp` — el `SetControllerKeyAssociatedWithAction` de `PED_RELOAD`
  (`:303`), el `SETACTIONNAME` de `PED_RELOAD` (`:534`) y el `case PED_RELOAD` de
  `GetControllerType`/`ACTIONTYPE` (`:2011`).
- `src/core/re3.cpp` — `iniControllerActions[]` (`:325-333`).
- `src/peds/PlayerPed.cpp` — el consumidor, con el patrón exacto de `PED_RELOAD` (`:2229-2232`).

**Acción**: `edit` (los cuatro).

**Detalle técnico**

Todo lo que el `WalkKey = LALT` del mod necesita **ya existe** en el árbol, verificado:
- `iniKeyboardButtons[]` (`re3.cpp:339-343`) contiene **`"LALT"`** en `:342` y **`"NULL"`** en `:343` —
  o sea el nombre de tecla del INI del mod y su «NULL para desactivar» están ya en la lista de casa.
- La lectura de una acción de teclado ya está escrita: `ControlsManager.
  GetControllerKeyAssociatedWithAction(ACCION, KEYBOARD)` + `ControlsManager.GetIsKeyboardKeyDown(key)`
  (`ControllerConfig.h:230` y `:184`). `PlayerPed.cpp:2229-2230` lo usa con `PED_RELOAD`, así que la
  forma está probada.

Lo que falta es **la acción**, en 4 sitios:
1. `ControllerConfig.h`, antes de `MAX_CONTROLLERACTIONS`:
   `PED_WALK,	// ClassicAXIS WalkKey (ini :9, LALT; NULL = desactivar). Se anade AL FINAL para no mover ningun indice.`
   **Al final**, no en el hueco de un sitio: `PED_RELOAD` (§3, C3.1) ya ocupó el último hueco
   (`ControllerConfig.h:67`, con el comentario de que antes era `UNKNOWN_ACTION`), y **reindexar** el
   enum rompería las configs de controles guardadas. Añadir al final no mueve ningún índice.
2. `ControllerConfig.cpp:303`-al-lado: `SetControllerKeyAssociatedWithAction(PED_WALK, 'L', LALT)`.
   Ojo: **el `SetControllerKeyAssociatedWithAction` de casa toma un `int32` de carácter**, no un
   nombre de la lista: `'L'` es la tecla y `VK_LMENU`/`VK_MENU` es el modificador Alt. El par
   *tecla+modificador* se expresa con la convención de las demás acciones de la casa (mismo patrón que
   `PED_TOGGLE_1RST_PERSON` con `'V'`, `:298`). **PENDIENTE menor**: el par exacto
   «L + Alt izquierdo» en la notación de `SetControllerKeyAssociatedWithAction` **no se ha verificado
   leyendo la función**; se resuelve leyendo su firma en el momento de ejecutar (una línea) o copiando
   cómo se representa una combinación en `re3.cpp:394-410` (`"kbd:LALT"` es un token que el parser
   `strtok` de `re3.cpp:394` ya sabe separar). Recomendación: **usar el token del INI**, o sea que el
   default sea la cadena `LALT` y no un par de VK.
3. `ControllerConfig.cpp:534`-al-lado: `SETACTIONNAME(PED_WALK);`
4. `ControllerConfig.cpp:2011`-al-lado: el `case PED_WALK:` devolviendo `ACTIONTYPE_1RST3RDPERSON`
   (la misma clasificación que `PED_RELOAD`, «a pie»).
5. `re3.cpp:333`, al final de `iniControllerActions[]`: `"PED_WALK"`.

Y el consumidor, en `ViceExtProcessPlayerPedControl` (B9-g), **al principio**, antes del predicado de
apuntado (el mod lo hace en `Main.cpp:1215-1217`):
```
	// ClassicAXIS C17 WalkKey (Main.cpp:1196-1201, 1215-1217): con la tecla de andar
	// pulsada la velocidad es 0. Se lee de la accion rebindable (mismo patron que
	// PED_RELOAD, PlayerPed.cpp:2229), asi que vale con cualquier metodo de control.
	{
		RsKeyCodes odWalk = (RsKeyCodes)ControlsManager.GetControllerKeyAssociatedWithAction(PED_WALK, KEYBOARD);
		if (odWalk && ControlsManager.GetIsKeyboardKeyDown(odWalk))
			m_fMoveSpeed = 0.0f;
	}
```
`if (odWalk && ...)` es el «NULL = desactivar» del mod (`ClassicAxisVC.ini:8`: *«NULL to disable»*),
implementado como tecla 0.

**Restricciones**
- No tocar los índices de `e_ControllerAction` existentes (§6: no romper saves/configs).
- `ControllerConfig.cpp` y `ControllerConfig.h` los toca también el carril de 1.ª persona
  (`PED_TOGGLE_1RST_PERSON`, C1) y el de recarga (`PED_RELOAD`, C3.1). Se añade **al lado** de sus
  líneas, nunca se reescriben.
- `WalkKey` es la única de las 10 claves del INI que **no** es un ajuste: no va por `ReadIniIfExists`,
  va por `Bindings` (`re3.cpp:389`).

**Verificación**
- `AIMWALK tecla=%d spd=%.2f` — una línea por **flanco** (al pulsar y al soltar). Criterio: con LALT
  pulsado, `spd=0.00`; al soltar, la velocidad vuelve a la de antes.
- `CROUCH2`/`R27` sin cambios: el `WalkKey` no toca el agachado.

---

#### B14 · El bloque `AX` del verificador — `gta_vc_browser/tools/viceext-log-check.py`

**Ficheros / anclas**
- El fichero tiene 1.900 líneas. Los bloques se registran en **tres** sitios, los tres hay que tocarlos:
  1. el `for nombre, fn in (...)` de `main()` (`:1857-1866`) → añadir `("AX", bloque_ax)`;
  2. el `for nombre in (...)` del resumen (`:1870-1872`) → añadir `"AX"`;
  3. la lista `nuevas = [...]` (`:1889-1890`) → añadir `"AX"`, para que un `FAIL` de AX tumbe el
     verificador.
- Y el docstring de cabecera (`:72-75`), que hoy dice «Exit code 0 si los cuatro bloques están OK» (ya
  desactualizado respecto a los 25 que hay) → se añade la línea de `AX` sin arreglar lo otro (fuera de
  alcance).
- Precedente a copiar: `bloque_r16` (`:1096-1137`) por la forma corta, y `bloque_r27` (`:1587-1826`)
  por el vocabulario `PASS/FAIL/INCONCLUSIVE` y por las bandas.

**Acción**: `edit`.

**Detalle técnico — las 8 trazas que `AX` consume**

Formato exacto (los `%d`/`%.Nf` son los que el regex del verificador tiene que buscar):

| Traza | Formato | Cadencia | Bloque |
|---|---|---|---|
| `AIMLAW` | `AIMLAW aim=%d modo=%d estado=%d arma=%d lock=%d aut=%d esp=%.2f solape=%d` | **edge** (flanco de `isAiming`) | B9 |
| `AIMCAM` | `AIMCAM m=%d dist=%.2f alt=%.3f zoff=%.3f amax=%.1f fight=%d hombro=0.20 obj=1 ex=%.2f ey=%.2f ez=%.2f lado=%+d trans=%u` | 1 Hz apuntando + 1 por flanco | B4, B5, B7 |
| `AIMFOV` | `AIMFOV fov=%.2f arma=%d alcance=%.1f taken=%d` | 1 Hz apuntando | B3a |
| `AIMLOCK` | `AIMLOCK hx=%.4f vx=%.4f tmo=%u off=%s` | **edge** (`raton` / `cerca` / `soltar` / `nuevo`) | B9-f, B11-b |
| `AIMIK` | `AIMIK pitch=%.4f alto=%.4f torso=%.3f h=%.3f t=%.3f a=%.3f` | 1 Hz apuntando | B9-e, B8 |
| `AIHTRI` | `AIHTRI h=%d rec=%d d=%.4f` | **edge** (aparece/desaparece) | B10-a, B11-d |
| `AIMWPN` | `AIMWPN block=%d arma=%d` | **edge** (pulsación de rueda) | B10-c |
| `AIMCOL` | `AIMCOL los=%d losD=%.2f sph=%d sphM=%d sphPed=%d dSph=%.2f dRaw=%.2f nc=%.3f app=%d pedes=%d` | 1 Hz apuntando | B3c |
| `AIMFOV`, `AIMSHOT` | `AIMSHOT modo=%d n=%d` | edge por disparo | B10-d |
| `AIMSPRAY` | `AIMSPRAY spray=%.5f agach=%d` | **edge** del valor | B12-a |
| `AIMCROUCH` | `AIMCROUCH crouchfire=%d fue=%d arma=%d` | edge | B12-b |
| `AIMWALK` | `AIMWALK tecla=%d spd=%.2f` | **edge** | B13 |
| `AIMHUD` | `AIMHUD cruz=%d marco=%d tipo=%d tri=%d rots=%.1f d=%.4f` | 1 Hz **sólo si dibuja algo** | B11 |

Patrón de guardia 1 Hz, **copiado literal** de `Cam.cpp:2622-2626`:
```
	static uint32 s_odNextAx = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (s_odNextAx > odNow + 60000) s_odNextAx = 0;   // el reloj retrocedio (carga)
	if (odNow >= s_odNextAx) { s_odNextAx = odNow + 1000; ... }
```
Patrón de flanco, **copiado literal** de `Cam.cpp:2605-2608`:
```
	static int8 s_odLastAx = -1;
	int8 cur = (int8)(condicion ? 1 : 0);
	if (cur != s_odLastAx) { s_odLastAx = cur; ... }
```

**Detalle técnico — los 6 criterios `AX` y su veredicto**

Cada uno devuelve `(estado, motivo)` como `bloque_r27` (`:1587-…`).

| # | Criterio §8 | PASS | FAIL | INCONCLUSIVE |
|---|---|---|---|---|
| **AX1** | la ley existe y es la del mod (§8.1) | hay ≥1 `AIMCAM` con `m=5` y `|dist - 2.70| <= 0.35` en la parte sin obstruir, y `alt` = `0.25 + zoff + 0.05 ± 0.02` | algún `AIMCAM` con `m != 5` mientras `aim=1` | **sin ninguna línea `AIMCAM`** ⇒ «build vieja: el log no trae la ley nueva» |
| **AX2** | hombro (`:0.20`) en espacio de objeto (§8.2/§8.3) | todos los `AIMCAM` traen `hombro=0.20 obj=1` y `|ex|`, `|ey|`, `|ez|` consistentes con un offset de 0.20 sobre `GetRight()` del ped (se recalcula en el verificador con los 3 números publicados) | `hombro` ≠ 0.20 o `ex/ey/ez` no casan con 0.20 | sin `AIMCAM` |
| **AX3** | clamp `±50°` (§8.5) | `max(amax) <= 50.0 + 0.5` | `max(amax) > 50.5` | sin `AIMCAM` |
| **AX4** | FOV 50 al rifle (§8.6) | con `zoomForAssaultRifles=1`: algún `AIMFOV taken=1 fov<=50.5` **y** `arma` con `alcance>=70`; y al soltar vuelve a `fov>=69.5`; y el **Minigun nunca** da `taken=1` | `taken=1` con `alcance<70`, o el Minigun da `taken=1`, o no vuelve a 70 | sin `AIMFOV` |
| **AX5** | sin auto-aim con ratón (§8.9) | con `aut=0`: **ningún** `AIMLAW` pasa de `lock=0` a `lock=1` sin un `AIMLOCK` de `nuevo`; y hay al menos un `AIMLOCK off=raton` o `off=cerca` | algún `lock 0->1` sin evento, o `off=` con un motivo desconocido | sin `AIMLAW` |
| **AX6** | no-regresión del recoil (§8.13) | el bloque `RC` **no** ha cambiado de veredicto respecto a la build anterior, y hay `RECOIL3 multY=0.400` | el bloque `RC` pasa de `PASS` a `FAIL`, o `multY` se mueve | sin `RECOIL3` |

Los criterios §8.14 (auto-centrado), §8.15 (agachado) y §8.16 (nado) **no se duplican en AX**: ya los
cubren los bloques `H`/`H2` (cámara), `R27` (agachado) y `R5` (nado) del verificador, y lo que AX
hace es **no dejarlos pasar** por añadirlos a la lista `nuevas` de `main()` junto con su propio FAIL.
Se dice en el docstring, para que nadie busque un `AX7` que no existe.

**Detalle técnico — la guarda de build vieja**

Es la misma idea que `bloque_r27:1645-1650` y `bloque_recoil:1441-1442`:
```
	ax = [l for l in lineas if "AIMCAM " in l]
	if not ax:
		return "INCONCLUSIVE", ("el log no trae ninguna linea AIMCAM: la build servida no lleva la ley "
		                        "ClassicAXIS (o no se ha apuntado en la sesion)")
```
Es decir: **`INCONCLUSIVE` por ausencia de datos, nunca `PASS` por ausencia** (`RULES 0.3` de honestidad
del `12-handoff` §4 punto 8: *«un PASS sin medición es mentira»*).

**Restricciones**
- `viceext-log-check.py` es **Python**, no C++: se edita con el editor normal, **no** con el python
  binario de CRLF (esa regla es para los `.cpp`). `PYTHONIOENCODING=utf-8` al ejecutarlo (cp1252).
- **No se ejecuta** el verificador en este plan (RULES 0.4): se escribe y lo corre el jugador o el
  Executor con autorización.
- No tocar los otros 25 bloques.
- Cada criterio imprime su medida antes del veredicto (así hacen `r16`, `r27`): es lo que permite
  auditar un `PASS`.

**Verificación**
- `python tools/viceext-log-check.py` sobre un log **sin** `AIMCAM` ⇒ `AX INCONCLUSIVE` con el motivo.
- Tras la sesión del jugador: cada criterio con su medida y su veredicto, y el resumen con `AX`.
- El exit code sólo cambia si `AX` está en `nuevas` (`:1889-1890`).

---

#### B15 · Marcas del `check-served-build.sh` — `gta_vc_browser/tools/check-served-build.sh`

**Ficheros / anclas** — el array `MARCAS` (`:24-108`), con el formato
`"marca|bloque|dónde"` (`:23`) y el `grep -aq` de `:126`.

**Acción**: `edit`.

**Detalle técnico** — las 13 líneas a añadir a `MARCAS`, con la marca **exacta** que se va a buscar en el
`.wasm` (que es donde caen los `snprintf` de las trazas) y el bloque descriptivo con el mismo estilo
que las 90 que ya hay:

```
  "AIMCFG forceauto=|AJuste ClassicAXIS: los 9 escalares leidos del INI (B0)|wasm"
  "AIMLAW aim=|AX ley de apuntado: flanco de isAiming con el modo de camara (B9)|wasm"
  "AIMCAM m=|AX ley: modo, distancia 2.70, altura, hombro 0.20 y duracion de transicion (B4/B5)|wasm"
  "AIMFOV fov=|AX FOV 50 al apuntar un rifle de alcance >= 70 (B3a)|wasm"
  "AIMLOCK hx=|AX lock-on: horShift/verShift, ventana de 250 ms y por que se suelta (B9-f)|wasm"
  "AIMIK pitch=|AX el brazo: pitch del brazo, torso, cabeza y brazo inferior (B9-e)|wasm"
  "AIHTRI h=|AX triángulo sobre el objetivo blando de raton (B10/B11)|wasm"
  "AIMWPN block=|AX cambio de arma bloqueado con el aimed activo (B10-c)|wasm"
  "AIMCOL los=|AX colisiones: LOS + 5 esferas con el near-clip re-leido (B3c)|wasm"
  "AIMSPRAY spray=|AX spray suave 0.00001f agachado / -1.0f de pie (B12-a)|wasm"
  "AIMCROUCH crouchfire=|AX C18-1: arma con y sin WEAPONFLAG_CROUCHFIRE (B12-b)|wasm"
  "AIMWALK tecla=|AX WalkKey: tecla de andar (LALT) y velocidad a 0 (B13)|wasm"
  "AIMHUD cruz=|AX HUD: cruz, marca de fijado por tipo y triángulo (B11)|wasm"
```

Dos detalles de la casa que hay que respetar:
- **`marca` = la cadena que va a aparecer en el `.wasm`.** Los `snprintf` los mete el enlizador; si
  alguien parte la cadena de formato, la marca no aparece y el checker da `FALTA` aunque la build esté
  bien. Por eso se toma **el prefijo de una sola palabra** más `=`, nunca un `%.2f` (que en el binario
  puede quedar reordenado) — y las 90 marcas existentes siguen esa regla, con cuatro excepciones
  (`"CROUCH2 h=%.2f avance="`, `"camz="`, `"base=%.2f obs=%d"`, `"PEDAT x=%.1f y=%.1f z=%.1f"`) que
  **ya** incluyen especificadores. Para las nuevas se usa la forma segura, sin `%.Nf`, y se anota.
- La columna `dónde` va a `wasm` (todas son trazas del motor, ninguna de la capa web). La capa web sólo
  tiene `ODSTA` (`pre`).

Y `check-served-build.sh:148-151` exige `ve13` en el `.pre`:
```
if ! grep -aq "ve13" "$PRE"; then
	echo "   FALTA ve13 (muestras de audio re-codificadas): los sonidos nuevos no llegarán"
```
**No se toca**: `dataTag` sigue en `ve14` (§8.17) y `ve14` contiene `ve13` como subcadena, así que la
comprobación sigue dando OK. Se comprueba y se anota; si algún día dejara de cumplirse, es cosa del
carril de datos, no de este plan.

**Restricciones**
- No añadir ni quitar marcas **existentes**: el checker tiene 90 marcas de 25 carriles y borrar una es
  romper el verificador de otro plan.
- Este fichero **no** se ejecuta en este plan (RULES 0.4).
- Sin `dataTag`.

**Verificación**
- `bash gta_vc_browser/tools/check-served-build.sh --listar` lista las 13 nuevas con su bloque.
- Tras el build: las 13 en `OK` y el veredicto final `OK: el motor servido lleva TODAS las marcas`.

---

#### B16 · `VERSION` por build — `gta_vc_browser/web/lib/index.js`

**Ficheros / anclas** — `export const VERSION = '2026-09-26-swim9';` en `web/lib/index.js:171`.

**Acción**: `edit`, **una vez por build** (§10.20).

**Detalle técnico** — la convención observada en el fichero es `AAAA-MM-DD-<tag>` y el `<tag>` es el
nombre corto de la mecánica. Los nombres de este plan, con la regla de la casa escrita en el comentario
de la línea 166-170 («subir el tag en CADA build»), son los de §10.20:
`2026-09-27-aim1` … `2026-09-27-aim5`.

Además, y **sólo en el último build**, el comentario de cabecera que explica el build (el bloque
`:130-170`, que es la bitácora por partida) se amplía con las 5 líneas del resumen del jugador. No es
obligatorio y no bloquea: se marca como opcional en §10.20.

**Restricciones**
- `VERSION` **sube en cada build**; si se olvida, el navegador sirve el `.wasm` de la caché y el
  jugador mide el binario viejo (pasó el 23/09, está escrito en `:166-170`).
- `dataTag` en `web/ondemand.js:125` **no se toca**: sigue `2026-09-26-ve14`. Este plan no cambia
  datos (§4.7, §6).
- No tocar `check-served-build.sh:141-147` (lee `VERSION` de ahí): funciona solo.

**Verificación**
- `grep -o "VERSION = '[^']*'" web/lib/index.js` devuelve la nueva.
- El log del jugador trae `build=<VERSION>` en las líneas `RECOIL ...` (`web/lib/index.js:481`) ⇒ es
  la prueba en el propio log de que la build nueva es la que se midió. **Éste es el criterio**.

---

#### B17 · Las dos filas falsas de la atribución — `docs/mods/ATTRIBUTION.md`

**Ficheros / anclas** — `docs/mods/ATTRIBUTION.md:100` (la fila `ve58` de «Apuntado
ClassicAXIS+GeniusZ») y `:132` (la fila `ve59` de «Seguimiento coche ClassicAXIS»). La fila de
`bForceLegsMovements` es `:127` y la de H4 ya dice `**YA ESTABA**`.

**Acción**: `edit`.

**Detalle técnico**

- **`:100`** — la fila dice «**PORTADO** (`ve58`)» para *«Apuntado ClassicAXIS+GeniusZ (hombro,
  CrosshairMult, sensibilidades, near-clip dual, seguimiento coche)»*. Eso es **falso** y §0 ya lo
  demuestra. Se reescribe el estado a lo que es verdad hoy y se añade la fila de este plan:
  - El `CrosshairMult 0.53/0.4` es `YA ESTABA` (`Camera.cpp:283-284`, un campo de la propia GTA).
  - El hombro 0.2 es `YA ESTABA` y **en espacio de objeto del ped** (`Cam.cpp:1386`, C-1).
  - Todo lo demás es `PENDIENTE (motivo: este plan)`, y se pasa a `**PORTADO**` con el `VERSION` del
    build que lo entregue (§10.20). **La atribución se escribe al cerrar, con el número del build
    real**, no antes.
- **`:132`** — la fila `ve59` atribuye a `CamNew.cpp` la ley de coche. §5.1(a) ya lo decidió: se
  reescribe a estado **propio del proyecto** (adaptación de `Process_FollowPed` por analogía,
  validada por el jugador en `ve65`-`ve75`) y la fila de `CamNew.cpp` se limita a
  `Process_FollowPed` y `Process_AimWeapon`, **ambos a pie**. El único punto del coche que sí viene
  del mod es el **agua** (`CamNew.cpp:215-220`), que B3/de `Cam.cpp:2710-2716` corrige.
- **Nuevas filas** (una por bloque con destino real, formato de la casa: origen `fichero:línea`, qué
  arregla, destino `fichero:línea`, estado):
  `CamNew.cpp:233` → `src/core/Cam.cpp` (`Process_AimWeapon`) · `CamNew.cpp:447` →
  `Process_AimWeaponCrouchOffset` · `CamNew.cpp:477` → `Process_AimWeaponFovLerp` · `CamNew.cpp:390`
  → `Process_AvoidCollisions` · `CamNew.cpp:265` → hombro (con la nota de que la función del SDK no
  existe aquí) · `Main.cpp:1203` → `ViceExtProcessPlayerPedControl` · `Main.cpp:1474` →
  `ViceExtFind3rdPersonMouseTarget` · `Main.cpp:745/783/864` → las 3 funciones de HUD (**con la nota
  de que la geometría de las marcas es reimplementación**) · `Main.cpp:387` → C9 · `Main.cpp:408` →
  C18-1 · `Main.cpp:1368` → C18-5 `**YA ESTABA**` (ya está en `PedFight.cpp:205-206`) ·
  `Main.cpp:118` y `:323` → `PENDIENTE (parches de bytes sin destino identificado)`.

**Restricciones**
- Escribir las atribuciones **con el `VERSION` real de cada build**, no antes. Una fila `PORTADO (veNN)`
  sin build es una mentira (`12-handoff` §4 punto 8).
- `docs/mods/ATTRIBUTION.md` está en la lista de escritura permitida (§6).
- No tocar las otras ~130 filas.

**Verificación**
- `grep -n "ve58\|ve59" docs/mods/ATTRIBUTION.md` ya no devuelve «PORTADO» para el apuntado, y sí
  devuelve las filas nuevas con su `VERSION`.
- La revisión del checklist de `reviewer.md` lo pide como DoD («el plan se ha actualizado con los
  cambios reales»).

---

### 10.18 Las 12 preguntas, en detalle

**1 · Dónde va `ViceExtProcessPlayerPedControl` y por qué ahí.**
`CPlayerPed::ProcessControl` (`src/peds/PlayerPed.cpp:4283-4667`) — **justo después de la llave de
apertura (`:4284`)**, antes de `CPed::ProcessControl()` (`:4321`). Cuatro razones, todas verificadas:
(1) es el literal `.before` del mod (`Main.cpp:298-308` sobre `0x53739F`); (2) `CWorld::Process()`
corre en `Game.cpp:1336` y `TheCamera.Process()` en `Game.cpp:1349`, o sea que el `TakeControl` de
este frame llega a la cámara en el **mismo** frame; (3) `AimGun()` —el que escribe
`m_pedIK.m_torsoOrient`/`m_lowerArmOrient` (`Ped.cpp:2693` → `Ped.cpp:1271`)— corre **después**, con lo
que los 3 `MoveLimb` se leen como objetivos y el motor los re-deriva con los mismos dos números que
el mod acaba de escribir: cooperación, no pelea; (4) ninguna de las 4 salidas tempranas (`bWasPostponed`
`:4324`, `PED_DEAD` `:4363`, `PED_DIE` `:4369`, `m_objective` `:4443`) se salta el punto, así que la
devolución de la cámara se ejecuta siempre. `CPlayerPed::ProcessPlayerControl` **no existe** en
`src/` (C-2).

**2 · El `case MODE_AIMING` y cómo se deja de enrutar a `MODE_SYPHON`.**
`case` en `Cam.cpp:396` (ya existe, comentado): descomentar y llamar a `Process_AimWeapon`. Y
**`CamControl` no se toca**: `TakeControl` (la misma llamada que hace el mod) pone
`m_bLookingAtPlayer = false` (`Camera.cpp:2354`), con lo que el bloque `if(m_bLookingAtPlayer)` de
`Camera.cpp:1729-1938` —que es **donde se aplica** el `ReqMode` calculado en `:1507-1592`— no se
ejecuta, y el `else` de `:1939-1990` arranca la transición a `m_iModeToGoTo` (`:1959-1960`).
`ReqMode = MODE_SYPHON` (`:1551`) y `ReqMode = PlayerWeaponMode.Mode` (`:1527`) se siguen calculando y
se **descartan**. Al soltar: `TakeControl(ped, previousCamMode, 1, 0)` y acto seguido
`m_bLookingAtPlayer = true` (`Main.cpp:1418-1419`), en ese orden, o el jugador se queda en
`MODE_AIMING` para siempre. Para que la 1.ª persona (francotirador/láser/M16/cámara) siga siendo de
`CamControl`, el early-return de `IsType1stPerson` (mod `:1227-1228`) va **antes** de calcular
`isAiming`. Con eso no se rompe nada de ese camino. **Y con el mismo mecanismo caen gratis el fight cam
y el point-gun cam (C4/C5, C-8)**, porque sus dos `ReqMode` (`Camera.cpp:1345/1348` y `:1559`) viven
en el bloque ignorado. ⚠️ El lanzacohetes es el único roce: el mod lo incluye en `IsType1stPerson`
(`:642`) y R16 lo saca a propósito ⇒ **D2**.

**3 · `TransformFromObjectSpace`.**
**No existe** en `src/` (0 resultados). Nuestro equivalente verificable: `CMatrix &mat = ped->
GetMatrix()` (`Placeable.h:19`, el mismo `m_matrix` que el mod escribe con `SetRotateZOnly` en
`Main.cpp:571`), ejes por `GetRight()`/`GetForward()`/`GetUp()` (`Matrix.h:56-58`), y el heading con
`CGeneral::GetATanOfXY(GetForward().x, GetForward().y)` — que es exactamente lo que ya usa el motor
en `Cam.cpp:370`. La reimplementación que se especifica es
`mat.GetPosition() + GetRight() * 0.2f`, y es **numéricamente idéntica** a la del mod para un ped con
sólo rotación Z, porque `SetRotateZOnly(a)` pone `right = (cos a, sin a, 0)` (`Matrix.cpp:218-219`) y
`rotZ(a)·(0.2,0,0) = (0.2 cos a, 0.2 sin a, 0)`. Diferencia real: sólo con un ped que tenga pitch/roll
en su matriz, que no es el jugador. **La firma exacta del helper del plugin-sdk es `PENDIENTE`**
(no está en `tmp/extsrc/`; se resuelve con la tabla de símbolos del SDK o el `.map` del mod).
**Y se corrige la premisa del plan (C-1):** `Cam.cpp:1386/1782/5714` **ya** aplican el hombro en
espacio de objeto del ped, porque `CPlaceable::GetRight()` es `m_matrix.GetRight()` de la entidad
(`Placeable.h:20` + `:6`), y el propio árbol lo dice en `Cam.cpp:1778-1781`. Esos 3 sitios no se tocan.

**4 · El near-clip.**
`Scene.camera` (`src/core/main.h:14-19`: `struct GlobalScene { RpWorld *world; RwCamera *camera; }`,
`extern GlobalScene Scene;`) es el equivalente exacto del `Scene.m_pCamera` del mod. Y las dos
funciones tienen **el mismo nombre** en nuestro árbol: `RwCameraGetNearClipPlane(Scene.camera)` y
`RwCameraSetNearClipPlane(Scene.camera, x)` — ya se usan en `Cam.cpp:2765`, `:2787`, `:2745`. El
"re-leído en cada una de las 5 vueltas" es el `Cam.cpp:2765` de dentro del bucle, y es lo que hay que
extraer tal cual a `Process_AvoidCollisions` (B3c).

**5 · `interpF`.**
No existe en `src/`. `CTimer::ms_fTimeStep` sí, y su envoltorio también:
`CTimer::GetTimeStep()` devuelve `const float&` de `ms_fTimeStep` (`Timer.h:22`) — 1:1 exacto. El lerp
se hace con la macro de casa `lerp(norm, min, max)` (`src/core/common.h:396`):
`lerp(norm,min,max)` = `((norm) * ((max) - (min)) + (min))`, o sea `min + norm*(max-min)`, que con
`norm = 0.1f * CTimer::GetTimeStep()`, `min = offset`, `max = end` es exactamente `interpF(offset, end,
0.1f*ms_fTimeStep)`. **No se escribe una función nueva** (RULES 0.6): se usa la macro.

**6 · Los ajustes.**
`src/core/re3.cpp`. **Lectura**, patrón real en `LoadINISettings`:
`ReadIniIfExists("Controller", "HorizantalMouseSens", &TheCamera.m_fMouseAccelHorzntl);` (`:505`) para
un `float`, y `ReadIniIfExists("Controller", "DisableMouseSteering", &CVehicle::
m_bDisableMouseSteering);` (`:507`) para un `bool`. Sobrecargas: `uint32` `:208`, `uint8` `:219`,
`bool` `:230`, `int32` `:241`, `int8` `:252`, `float` `:263`, `char[]` `:274`. **Escritura**, patrón
real en `SaveINISettings`: `StoreIni("Controller", "HorizantalMouseSens", TheCamera.m_fMouseAccel
Horzntl);` (`:614`) y `StoreIni("Controller", "DisableMouseSteering",
CVehicle::m_bDisableMouseSteering);` (`:616`). Sobrecargas de `StoreIni`: `uint32` `:285`, `uint8`
`:292`, `int32` `:299`, `int8` `:306`, `float` `:313`, `char[]` `:320` — **no hay `bool`**, y el
ejemplo de `:616` funciona por la promoción implícita a `int32`. El `define` va en el bloque
`VICEEXT_*` de `src/core/config.h:382-427` (la cabecera del bloque, con su comentario de convención,
está en `:378-381`); el «dueño del fichero, sección 1» del plan **no resuelve** porque
`.agents/AGENTS.md` §1 sigue con `[Fill in]` (C-16) ⇒ se aplica la práctica observada. La tabla
completa de los 9 escalares con su default y su línea de fuente está en B0.

**7 · `CPedIK::MoveLimb` y los miembros.**
`LimbMoveStatus CPedIK::MoveLimb(LimbOrientation &limb, float targetYaw, float targetPitch,
LimbMovementInfo &moveInfo)` — `src/peds/PedIK.h:59`, cuerpo en `PedIK.cpp:63-…`. Las tablas
estáticas existen con el mismo nombre: `ms_headInfo` (`PedIK.h:45`), `ms_torsoInfo` (`:44`),
`ms_lowerArmInfo` (`:48`). **Los 3 `m_s*` se llaman distinto en nuestro árbol**:
`m_sHead` → **`m_headOrient`**, `m_sTorso` → **`m_torsoOrient`**, `m_sLowerArm` →
**`m_lowerArmOrient`** (`PedIK.h:38`, `:39`, `:41`); y `m_PedIK` → **`m_pedIK`** (`Ped.h:525`). Todos
`LimbOrientation` (`PedIK.h:5-9`, `{ yaw, pitch }`). **Nada falta**: los 6 identificadores existen, con
3 nombres distintos. Y el «bone 1» del objetivo es **`PED_HEAD`** (`PedModelInfo.h:10`), que ya se usa
en `Hud.cpp:554`.

**8 · La geometría de las tres marcas.**
`DrawSATarget`, `DrawLCSTarget` y `DrawSATriangleForMouseRecruit` **no están** en `tmp/extsrc/`: se
llaman en `Main.cpp:847`, `:853`, `:900` y no se bajaron. Sus **parámetros sí se conocen** (del log de
la llamada): `out.x, out.y, dist, rotMult, col` y `out.x, out.y, w/128.0f, col`. Lo que **no** se
conoce es la forma, y por §6 + §9 **no se inventa una copia**: se dibuja por código con lo que ya usa
nuestro `Hud.cpp`:
- `CSprite2d::DrawRect(const CRect&, const CRGBA&)` — `src/renderer/Sprite2d.h:42`, ya en
  `Hud.cpp:566-569`.
- `CSprite2d::Draw2DPolygon(x1,y1,x2,y2,x3,y3,x4,y4, const CRGBA&)` — `Sprite2d.h:47`, **cuadrilátero
  libre**: es el primo exacto de una marca rotada por `rotMult` (4 vértices girados alrededor del
  centro). Precedente en el árbol: `Frontend.cpp:4087-4090`.
- Triángulo: `Draw2DPolygon` con un vértice degenerado, o `CSprite2d::SetVertices(int n, float*, float*,
  const CRGBA&)` (`Sprite2d.h:36`) con `n = 3` + `RenderVertexBuffer()` (`:53`); el buffer es de 8
  vértices (`Sprite2d.h:10`), así que 3 cabe de sobra.
- `CSprite2d::Draw(const CRect&, const CRGBA&, u0,v0,u1,v1,u3,v3,u2,v2)` — `Sprite2d.h:26-27`, la
  sobrecarga de 10 parámetros que ya usa `Hud.cpp:408-409` con `Sprites[HUD_SITEM16]`: es la que
  necesita la cruz del mod.
Y en la cabecera de atribución del bloque se escribe, literal, que **la geometría de las marcas es una
reimplementación con primitivas de `CSprite2d` y no una copia**, porque no está en las fuentes.

**9 · El riesgo de mutar `CWeaponInfo` — la recomendación fail-closed.**
**No se muta.** Hechos: `CWeaponInfo aWeaponInfo[WEAPONTYPE_TOTALALLTYPES];` es **un array global**
(`src/weapons/WeaponInfo.cpp:72`) y `GetWeaponInfo` devuelve `&aWeaponInfo[weaponType]`
(`:129-131`): es el **mismo objeto** para el jugador, para cada ped de la IA y para el motor. Los flags
son una máscara en `m_Flags` (`WeaponInfo.h:62`) leída con `IsFlagSet` (`:78`). El mod hace
`info->m_bCanAim = false/true` en `IsWeaponPossiblyCompatible` (`Main.cpp:668-671`) y en
`IsTypeTwoHanded` (`:735-738`) para `FLAMETHROWER`/`MINIGUN` según esté agachado. Portado literal,
eso apaga el apuntado con llama y con Minigun **para todos los peds y hasta el final del frame**, y el
`restore` que lo arreglaría depende del orden de ejecución entre la IA y el jugador, que no está
controlado desde aquí.
**La intención del mod, sin estado global compartido:**
```
	static bool ViceExtAimCanAimWithCrouch(eWeaponType wt, CWeaponInfo *info, bool ducking)
	{
		if (wt == WEAPONTYPE_FLAMETHROWER || wt == WEAPONTYPE_MINIGUN)
			return !ducking;
		return ViceExtCanAim(wt, info);
	}
```
y a partir de ahí, `IsWeaponPossiblyCompatible` y `IsTypeTwoHanded` devuelven exactamente lo que
devuelven en el mod (`:675` y `:742`) con el mapa de flags de §3.3. Es el mismo comportamiento **para el
jugador** y none para la IA, que es donde está el bug.
**Si el jugador insiste en el literal**, lo único aceptable es un override **por ped**, no una
mutación de la tabla: un `CPed::ViceExtCanAimNow()` que devuelva el valor efectivo y cambiar **sólo**
los lectores del camino de apuntado del jugador, dejando `IsFlagSet` intacto. Eso obliga a tocar más
sitios (los lectores de `WEAPONFLAG_CANAIM` del camino del jugador) y es un coste de alcance real.
**Lo que no es una opción: guardar/restaurar `m_Flags` alrededor del bloque de control.** No hay
dueño, no hay momento seguro y el fallo es silencioso. → **D3**.

**10 · Los límites del callsite del recoil.** Confirmados sobre el fichero:
`src/weapons/Weapon.h:45` `static void ViceExtRecoilBegin(float &alpha, bool reset, int32 mode, const
char *source);` · `:46-47` `static void ViceExtRecoilApply(float &alpha, float manualDeltaRad, float
inputY, const char *source, int32 mode, float minAlpha, float maxAlpha);`
En `Process_Syphon`: `Cam.cpp:4162` `CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "syphon");`
y `Cam.cpp:4170` `CWeapon::ViceExtRecoilApply(Alpha, 0.0f, 0.0f, "unknown", Mode, -PI, PI);` con
`recoilReset` declarado en `:4100-4102`. El `Process_FollowPedWithMouse` ya pasa los límites del
apuntado cuando el hombro está activo: `Cam.cpp:1846-1847`
`… ? -DEGTORAD(50.0f) : -DEGTORAD(89.5f), … ? DEGTORAD(50.0f) : DEGTORAD(60.0f)`. El callsite nuevo es
**una copia literal del de `Process_Syphon`** con `"aim-weapon"` como `source` y `-PI/+PI` como
límites (§5.7), y el `±50` lo aplica el clamp de la propia ley después. Detalle en B6.

**11 · El bloque `AX` del verificador.** Especificado entero en B14: 13 trazas con su formato y su
cadencia, 6 criterios con su `PASS`/`FAIL`/`INCONCLUSIVE` uno a uno, y la guarda de build vieja
(`INCONCLUSIVE` si no hay ni una línea `AIMCAM`). Las cadencias respetan las dos convenciones que ya
existen en el árbol, copiadas literalmente: 1 Hz con reanclaje de reloj (`Cam.cpp:2622-2626`) y
edge-triggered con `static int8` inicializado a `-1` (`Cam.cpp:2605-2608`). Los tres sitios de
`viceext-log-check.py` que hay que tocar están identificados por línea (`:1857-1866`, `:1870-1872`,
`:1889-1890`) y el bloque del recoil **no** se toca. Las 13 marcas de `check-served-build.sh`, con su
cadena exacta, están en B15.

**12 · El registro de conflictos.** §10.19.

---

### 10.19 Registro de conflictos (pregunta 12)

Ordenado por riesgo. Un carril = un plan vivo. «Mitigación obligatoria» = no se ejecuta sin ella.

| # | Fichero / región | Dueño | Qué entra aquí | Riesgo | Mitigación |
|---|---|---|---|---|---|
| **1** | `src/peds/PlayerPed.cpp` — bloque de `CPlayerPed::ProcessControl` `:4283-4667` entero, y `PlayerPed.h:77-98` | **carril de nado** (`swim8`, `EXECUTED`): `ViceExtSwimControl` (`:4465`), `ViceExtSwimClipSpeed`, `ViceExtIsSwimming` | B9 entero (inserción en `:4284`), B10(a), B10(b), B10(c), B10(d) en `:978` y `:1118`, B13 (consumidor), B12(a) (`DoWeaponSmoothSpray` `:823`) | **MUY ALTO** | **Bloqueo duro** (§3.2). Además: (a) el punto de inserción `:4284` está **antes** de `ViceExtSwimControl` (`:4465`) y de `ViceExtCrouchControl` (`:4472`), así que no los solapa; (b) las 4 zonas de edición dentro del fichero son `:4284` (1 línea), `:823-` (bloque de C9), `:978`/`:1118` (2 líneas cada una) y `:823`; (c) **nada** de este plan toca `odAimWalkActive/Uncapped` (`:50-51`), `odSwimming` (`:4465`) ni `ViceExtCrouchLimitSpeed` (`:4512`). Reanclar con `ninja …/peds/PlayerPed.cpp.o` y comparar `CROUCH2`/`SWIM2` |
| **2** | `src/peds/PedFight.cpp` — `SetPointGunAt` `:161-210` (bloque R28 `:182-207`), `ClearPointGunAt` `:240-`, y los callsites de `Fire` `:875`/`:919` | **carril de agachado** (R28/`crouch2`, `agachado-calibrado`): `ViceExtCrouchShooting` (`Ped.h:1080`, 8 usos en `PedFight.cpp`) | B12(b) C18-1, B12(c) C18-2, B12(d) C18-5 (`YA ESTABA`), B10(d) C10 en los 2 callsites | **ALTO** | C18-1 se añade **por encima** del bloque R28 con `return` propio, sin tocar `ViceExtCrouchShooting` ni `GetCrouchFireAnim` (`Ped.h:1057/1064`). Los 2 callsites de `Fire` se tocan en el **mismo commit** que B12, no antes. Criterio de no-regresión: el bloque `R27` del verificador |
| **3** | `src/core/Cam.cpp` — `Process_FollowPedWithMouse` `:1784-1818` y runabouts `:2438`, `:5784-5785` | **carril de cámara/auto-centrado** (`camara-coche-sin-lucha`, `ve65`-`ve75`): el `WellBufferMe(TargetOrientation, 0.1/0.06)` de `:2596-2602` y el contrato de 1,5 s | B3(d), 2 líneas por sitio (los multiplicadores por eje) | **ALTO** | Las 2 líneas se insertan **después** del `if(UseMouse){...}else{...}`, sin tocar el `WellBufferMe` ni el clamp `±50/+60/-89,5` de `:1852-1858`. Con `RightAnalogStickSensitivityX/Y = 1.0f` (el default del INI) el resultado es **idéntico bit a bit**, así que el riesgo es nulo mientras no se cambie el ajuste. Se verifica que `camauto2` y `CAMB2` dan lo mismo |
| **4** | `src/core/Cam.cpp` — `Process_Cam_On_A_String` `:2722-2798` | el mismo carril de cámara | B3(c), la **extracción** de `Process_AvoidCollisions` | **ALTO** | Refactor puro de comportamiento, verificado con las trazas `CAMB2b` (`Cam.cpp:2643`) antes/después. Si cambia un número, es un bug del refactor. Y la ley de coche **no** cambia de comportamiento: ni el near-clip del agua, ni la altura, ni el autocentrado (§5.1a) |
| **5** | `src/core/Camera.cpp` — `StartTransition` `:2483-2767` | el mismo carril (el `switchPedMode`/`switchPedToCar` de `:2501-2512` y los de `:2711-2716` son suyos) | B7(c), el override de la duración de transición | **MEDIO** | El override va **al final** de la función (después de `:2766`), con flag de un tiro ⇒ no puede pisar la clasificación del carril. Y **sólo** cuando `s_viceExtAimSwitchSpeed` está puesto, que sólo lo pone B9 |
| **6** | `src/weapons/Weapon.cpp`, `src/weapons/Weapon.h` | **carril del recoil** (`recoil-por-arma-y-cadencia`, `EXECUTED`): `ViceExtRecoil*` (9 `static`), la cola `s_odRecoilQueue` (`Cam.cpp:63-71`) | **nada** | **NINGUNO** (C-3) | B6 y B10(d) están fuera: el callsite del recoil es una llamada más en `Cam.cpp` y el `MODE_FOLLOW_PED` temporal va en los 4 callsites de `Fire`, no en `Fire`. **Verificar antes de empezar** que `grep -rn "ViceExtRecoil" src/` sólo da `Weapon.h`/`Weapon.cpp`/`Cam.cpp` y que este plan no añade ninguno nuevo |
| **7** | `src/renderer/Hud.cpp` `:379-424` (miras) y `:541-571` (marco del fijado) | carril de las **miras** (D6 / `VICEEXT_WEAPON_SIGHTS`, `config.h:390`): `ViceExtDrawSight` | B11, los dos bloques | **MEDIO** | B11 **no** toca `ViceExtDrawSight` ni `ViceExtSightSprites`; sustituye `:541-571` (que es del carril B4 antiguo) y **extiende** la condición de `:389`. Con las dos miras activas a la vez (sugerencia del jugador: mira de arma + cruz del mod) el jugador ve las dos ⇒ **D4** |
| **8** | `src/core/Pad.cpp`, `src/core/ControllerConfig.h/.cpp` | carriles de 1.ª persona (C1, `PED_TOGGLE_1RST_PERSON`) y de recarga (C3.1, `PED_RELOAD`) | B13 (una acción nueva al final del enum) | **BAJO** | Se añade **al final** de `e_ControllerAction` (antes de `MAX_CONTROLLERACTIONS`), con lo que **ningún índice se mueve** y las configs guardadas siguen valiendo. `Pad.cpp` **no se toca**: la tecla se lee con `ControlsManager.GetIsKeyboardKeyDown` (`ControllerConfig.h:184`), no por el camino del pad |
| **9** | `src/peds/Ped.cpp` | el motor (R20c) | **nada** | **NINGUNO** | Ni `C1`/`C2` ni `C11` tocan `Ped.cpp`: `C11` (`ClearAimFlag` si no se apunta, `Main.cpp:207-211`) es una condición que se resuelve en B9 con `ClearPointGunAt`/`ClearWeaponTarget`, que ya lo hacen (`PedFight.cpp:240`, `PlayerPed.cpp:299`). `C1`/`C2` se resuelven con `SetRealMoveAnim` (`PlayerPed.cpp`), no en `Ped.cpp` |

**Regla de propiedad (ownership) que se aplica a todos**: si al ejecutar aparece una línea que no coincide
con la ancla de este §10, **se para y se informa**; no se «busca el sitio a ojo» (regla de casa: *«si
hay duda de propiedad de un fichero, dejarlo sin tocar y explicarlo»*, `12-handoff` §3).

---

### 10.20 Ejecución por builds

§7 partido en **5 builds**. Cada uno es **independientemente verificable** por el jugador y **no toca
los intocables** (auto-centrado de cámara, matemática del recoil). En **todos**:
`dataTag` **no sube** (§6), `VERSION` **sube** (B16), `check-served-build.sh` en verde antes de pedir
partida (§6), sin commits, y el bloque `AX` del verificador y las marcas de `check-served-build.sh` se
**amplían en el mismo commit** que la mecánica que miden (si no, el criterio no existe cuando se juega).

| Build | `VERSION` | Bloques | Qué entra | Qué ve el jugador | Criterio PASS (§8) |
|---|---|---|---|---|---|
| **1** | `2026-09-27-aim1` | B0, B1, B13, B14(§AX0), B15(3 marcas), B17(2 filas) | Los 9 ajustes, el estado compartido, la acción `PED_WALK` + su consumidor. **Ninguna ley de cámara** | Los ajustes se leen bien (log) y **LALT camina lento** | `AIMCFG` con los 9 valores exactos; `AIMWALK spd=0.00`; `RC`, `H` y `R27` **sin cambio** |
| **2** | `2026-09-27-aim2` | B2, B3, B4, B5, B6, B7, B9(a)(b)(c)(d) | **La ley y sólo la ley**: `MODE_AIMING` vivo, `Process_AimWeapon` completa, hombro, colisiones extraídas, FOV, callsite del recoil, y el `TakeControl`/devolución con `previousCamMode` | La cámara de apuntado cambia: entra a 2,70 m, hombro que sigue al cuerpo, FOV 50 con el rifle, y **la cámara vuelve** al soltar | §8.1, §8.2, §8.3, §8.4, §8.5, §8.6, §8.10, §8.14, §8.13 (**si §8.13 falla, el build no pasa**) |
| **3** | `2026-09-27-aim3` | B8, B9(e)(f)(g)(h), B10(c) | El pitch del brazo (fórmula del mod), el lock-on y su suelta por §5.2(a), los 3 `MoveLimb`, `WalkKey` en la ley, **C18-3** y **C18-4**, y el bloqueo de cambio de arma (**C8**) | El brazo apunta donde mira la cámara; con fijado la bala va a la retícula; con ratón **no** sale ningún fijado solo; **te quedas agachado tras disparar**; la rueda no cambia el arma | §8.7, §8.9, §8.11, §8.15 (parcial) + `AIMIK`, `AIMLOCK` |
| **4** | `2026-09-27-aim4` | B11, B10(a) | Las 3 funciones de HUD y la adquisición del objetivo blando | La cruz del mod, la marca de fijado con su color de salud y su giro, y el triángulo sobre el ped | §8.4, §8.7 (marca), §8.8, §8.12 + `AIHTRI`, `AIMHUD` |
| **5** | `2026-09-27-aim5` | B12, B10(b)(d) | **C9**, **C18-1**, **C18-2**, **C10** en los 4 callsites, y el cierre documental | El spray suave al agachar; el `MODE_FOLLOW_PED` temporal al disparar; el compromiso de C18-1 medido | §8.12, §8.15 (**completo**), §8.16, §8.17 + `AIMSPRAY`, `AIMCROUCH`, `AIMSHOT` |

**Por qué 5 y no 1.** Un solo build mezclaría la ley de cámara, el HUD, el agachado y los hooks de arma en
una sola partida de medición: si algo falla, no se sabe en qué bloque, y las rondas del jugador (§7.12)
son por bloques. Además hay **dependencias reales** entre builds (B3(c) toca la ley de coche, que es
lo más delicado; B9 completa necesita B8; B11 necesita B10(a)), y así cada round del jugador cierra
un bloque con PASS antes de abrir el siguiente.

**Por qué el HUD va en el 4 y no en el 2.** Porque la cruz de 3.ª persona actual **desaparece** en
`MODE_AIMING` (`Hud.cpp:362-364` exige `Using3rdPersonMouseCam()`, que exige `MODE_FOLLOWPED`), y esa
regresión sólo la arregla B11. Poner B11 en el mismo build que la ley es lo correcto; separarlo
introduce un build intermedio con una regresión conocida. → Se ha metido B11 en el 4, y el build 2
**deja la cruz como esté**, o sea que en `aim2` el jugador apunta sin retícula. Eso es una decisión que
el jugador tiene que ver antes de jugar el build 2 → **D5** (§10.21).

**Restricciones de la secuencia**
- El build 1 **no** puede empezar hasta que el carril de nado cierre (§3.2, y el conflicto #1).
- Los builds 2 y 3 tocan `Cam.cpp` y `Camera.cpp` **a la vez**: son un solo carril (el de cámara), no
  dos carriles. No se abren en paralelo.
- Los builds 4 y 5 no dependen del 2 más que en el estado (B1), así que podrían ir en cualquier orden
  **después** del 2. Se numeran por dependencia de verificación, no por posibilidad.

---

### 10.21 Lo que el jugador tiene que resolver ANTES de `READY`

Cinco puntos. **Ninguno bloquea el enriquecimiento**; tres sí cambian trabajo, y dos son decisiones de
alcance que el jugador cerró el 27/09 con una premisa que este enriquecimiento ha encontrado falsa.

**D1 · La premisa de §5.4(a) es falsa — el hombro YA era de objeto.** (Riesgo: **alto**, cambia una
decisión cerrada.)
El plan, y §0, dicen que `Cam.cpp:1386` aplica el hombro «en espacio de cámara ⇒ hombro que gira con la
cámara, no con el cuerpo», y de ahí la decisión «SE PORTA el hombro en espacio de objeto». Hecho:
`CamTargetEntity->GetRight()` es `CPlaceable::GetRight()` → `m_matrix.GetRight()` (`Placeable.h:20`) y
`m_matrix` es miembro **de la entidad** (`Placeable.h:6`). El hombro **ya es de objeto**; el árbol lo
documenta en `Cam.cpp:1778-1781`. Consecuencias: (a) el trabajo de §5.4(a) se reduce a «la ley nueva
calcula el hombro con la misma forma, sin suavizado» — que hay que hacer igualmente porque la ley es
nueva; (b) **los 3 sitios de `ve58` no se tocan** (menos riesgo); (c) **la razón** que dio el jugador
(«el ped deja de deslizarse de lado en pantalla») **no se sostiene tal como está escrita**: puede que
el síntoma real tenga otra causa (p. ej. el `WellBufferMe` del hombro, o el hecho de que la ley nueva
sea 3.ª persona de verdad en vez de la 3.ª persona de `Syphon`), y eso hay que averiguarlo en partida.
**Lo que pido**: confirmar que se mantiene la decisión con la premisa corregida, y que en la ronda del
build 2 se mire explícitamente si el deslizamiento lateral sigue ahí. Si sigue, se investiga con el
log antes de seguir — no se «soluciona» tocando los 3 sitios.

**D2 · El lanzacohetes: `IsType1stPerson` del mod vs R16.** (Riesgo: **medio**.)
El mod devuelve temprano (no apunta) con `WEAPONTYPE_ROCKETLAUNCHER` (`Main.cpp:642`), pero nuestro
`VICEEXT_ROCKET_3RD_PERSON` (`config.h:421`, validado en R16) **saca el lanzacohetes del modo
francotirador a propósito** (`PlayerPed.cpp:1557-1560`). Si se copia la lista tal cual, el lanzacohetes
entra en `MODE_AIMING` y **R16 deja de funcionar**. **Lo que pido**: elegir (a) se saca el
lanzacohetes de nuestra `IsType1stPerson` y R16 manda (recomendación del enriquecimiento), o (b) se
acepta que el lanzacohetes use la ley nueva y se documenta la pérdida de R16.

**D3 · `CWeaponInfo`: consulta o literal.** (Riesgo: **alto** si se elige mal.)
La recomendación fail-closed es **no mutar la tabla global** y expresar la intención del mod por
consulta (B9-c, §10.18-9), que para el jugador es idéntica y para la IA no toca nada. La alternativa
aceptable es un override **por ped** (`CPed::ViceExtCanAimNow()`), que cuesta más alcance. Lo que
**no** se puede hacer es `info->m_bCanAim = false/true` en caliente. **Lo que pido**: confirmar «no se
muta», o pedir la variante del override por ped asumiendo su coste.

**D4 · Si con el arma de la mira activa se ven dos retículas.** (Riesgo: **bajo**, cosmético.)
`VICEEXT_WEAPON_SIGHTS` pinta la mira de arma en `Hud.cpp:405-423` (`:389` exige `MODE_FOLLOWPED`, o
sea que **no** se solapa con `MODE_AIMING`) y B11 pinta la cruz del mod. En la práctica no se solapan
porque los modos son excluyentes, así que probablemente no haya que hacer nada. **Lo que pido**:
confirmar que si aparece alguna doble retícula, se documenta y no se «arregla» en este plan.

**D5 · El build 2 deja al jugador apuntando sin retícula.** (Riesgo: **medio**, de experiencia de
juego.)
Motivo verificado: `DrawCrossHairPC` sólo se pone con `Using3rdPersonMouseCam()`
(`Hud.cpp:362-364`), que exige `MODE_FOLLOWPED` (`Cam.cpp:1133`), y B11 llega en el build 4. O sea que
en `aim2` y `aim3` no hay mira de 3.ª persona al apuntar. **Alternativas**: (a) aceptarlo y avisar al
jugador antes de la ronda; (b) adelantar la rama de la cruz (sólo la cruz, `Main.cpp:745-781`, sin las
marcas) al build 2, y dejar las marcas para el 4. **Lo que pido**: elegir (a) o (b). La recomendación
es **(b)**, porque una ley de apuntado sin mira es difícil de juzgar y falsea §8.4.

**Y un punto informativo, no decisión** — la equivalencia de `hasPadInHands`. El mod decide si hay mando
con `GInput_Load` + `pXboxPad->HasPadInHands()` (`Main.cpp:52`, `:1212`), que es el acoplo de GInput y
**no existe en web** (§6 lo prohíbe). Se usa en `disableAutoAim` (`:330`, `:475`, `:1250`) y en
`Find3rdPersonMouseTarget` (`:1475`). La propuesta es un detector de «hay mando en la mano» sobre lo que
`Pad.cpp` ya expone, pero **la regla exacta es `PENDIENTE`**: depende de qué considere el jugador «tener
el mando en la mano» (stick derecho con_axis, botón, o ambos) y no se inventa. Afecta a §5.2 y a la
adquisición del triángulo.

---

*Decisiones cerradas el 27/09 (§5): §5.1(a) se queda la ley de coche y se corrige la atribución ·
§5.2(a) fiel al mod, con ratón NO hay auto-aim ni marca · §5.3(a) FOV 50 activo con el umbral del mod ·
§5.4 solo (a), el hombro en espacio de objeto · §5.5 los cinco fixes de agachado, el #6 descartado por
bug del mod · §5.6 el near-clip dual se queda · §5.7 callsite del recoil sí, matemática intacta.
**Alcance cerrado.** Gate para `EXECUTED`: «ejecuta el plan», y cada comando de proyecto necesita su
autorización aparte (RULES 0.4).*

## 11. Ejecución (27/09)

**Gate «ejecuta el plan»** autorizado por el jugador el 27/09, tras responder «todas como
recomiendas» a las 6 puntos de §10.21. `READY` y `EXECUTED` se dieron en el mismo mensaje, así
que el estado salta de `ENRICHED` a `EXECUTED` con la aprobación del jugador ya hecha.

### 11.1 Decisiones tomadas sobre §10.21 (las 6, como recomendó el enrichment)

| | Decisión | Efecto en el trabajo |
|---|---|---|
| **D1** | Shoulder: **se mantiene**, con la premisa corregida (ya era de objeto, `Placeable.h:20`). **No se tocan** los 3 sitios de `ve58` (B5) | B5 queda en «solo lectura»; en el build 2 se mira con log si el deslizamiento lateral persiste y, si persiste, se investiga antes de tocar código |
| **D2** | **R16 manda**: el lanzacohetes **se saca** de nuestra `IsType1stPerson`, no entra en la ley nueva | `VICEEXT_ROCKET_3RD_PERSON` (`config.h:421`) intacto; B9(b) lo documenta en el predicado |
| **D3** | **`CWeaponInfo` no se muta**: la intención del mod (`FLAMETHROWER`/`MINIGUN` `CanAim` off agachado) se expresa por **consulta** (B9-c) | La tabla global `weapon.dat` queda intacta para la IA y para los demás peds |
| **D4** | Doble retícula: **se documenta, no se arregla** en este plan | Si aparece, va al informe; no hay commit que la "arregle" |
| **D5** | La cruz del mod **se adelanta al build 2** (no al 4) | El reparto de builds de §10.20 se modifica: B11 entra en `aim2` |
| **R1** | Crouch offset, agua y esconder-peds **también al caminar**, no solo al apuntar | La ley de a pie entra en el alcance (B3-e) |

**Reparto de builds tras D5 y R1** (ajuste de §10.20):

| Build | `VERSION` | Bloques | Nota |
|---|---|---|---|
| 1 | `2026-09-27-aim1` | B0, B1, B13, B14, B15, B17 | Settings + `PED_WALK` + Checker/marcas + atribución. Sin ley de cámara |
| 2 | `2026-09-27-aim2` | B2…B7, B9(a-d), B11 | **La ley + la cruz** (D5), shoulder, FOV, colisiones, callsite del recoil, `TakeControl`/devolución |
| 3 | `2026-09-27-aim3` | B8, B9(e-h), B10(c) | Pitch del brazo, lock-on + suelta §5.2, 3× `MoveLimb`, C18-3/C18-4, C8 |
| 4 | `2026-09-27-aim4` | B10(a) | `Find3rdPersonMouseTarget` + triángulo |
| 5 | `2026-09-27-aim5` | B12, B10(b)(d) | C9, C18-1, C18-2, C10 + cierre documental |

### 11.2 BLOQUEO ACTIVO: el carril de nado tiene `PlayerPed.cpp`

Comprobado el 27/09 20:52, con el plan ya en `EXECUTED`:

| Fichero | Última modificación | Dueño | ¿Ejecutable? |
|---|---|---|---|
| `src/peds/PlayerPed.cpp` | **6 min** | **carril de nado** (build `swim10` en curso) | **NO** |
| `gta_vc_browser/web/lib/index.js` (`VERSION`) | **5,6 min** (subió a `2026-09-26-swim10`) | carril de nado | **NO** |
| enlace (`reVC.wasm`, `.ninja_log`) | **5,4 min** | carril de nado | **NO** |
| `src/core/Cam.cpp` | 31 min | anterior | sí, con precaution |
| `src/core/Camera.h` / `Camera.cpp` | 16 d / 73 min | libre | sí |
| `src/core/config.h` | 77 min | libre | sí |
| `src/core/re3.cpp` | 8 d | libre | sí |
| `src/core/ControllerConfig.*` | 6 d / 5 d | libre | sí |
| `gta_vc_browser/tools/*` | 22,7 h | libre | sí |
| `docs/mods/ATTRIBUTION.md` | 22,6 h | libre | sí |

Es el **bloqueo duro** de §3.2 y el paso 1 de §7, previsto: `PlayerPed.cpp` es el fichero que
necesitan B9, B10 y el consumidor de B13, y `VERSION` + el enlace los usa el carril de nado en este
momento. **Editar a la vez el mismo fichero desde dos carriles corrompe los dos.**

**Lo que se hace ahora (build 1, parte sin colisión):** B0, B1, B13 ( definición del binding,
sin su consumidor), B14, B15, B17 — todo en `src/core/` y `tools/`, ficheros que el carril de nado
no está tocando.

**Lo que queda retenido:** el consumidor de `WalkKey` (B13) y B9/B10 en `PlayerPed.cpp`, B12 en
`PedFight.cpp`, el bump de `VERSION` (B16) y el enlace, porque dependen de que el carril de nado cierre
y suelte el fichero.

### 11.3 Progreso: B0 y B1 HECHOS (build 1, parte sin colisión)

**B0 · Ajustes** — `src/core/config.h` + `src/core/re3.cpp`

- `config.h`: un único `#define VICEEXT_AIM_CLASSICAXIS` al final del bloque `VICEEXT_*`, con el
  comentario que dice **qué NO entra y por qué** (§5.4b-e, §5.1a, §5.6, §9), para que no se lea como
  Forgetfulness.
- `re3.cpp`: **9 `ReadIniIfExists`** con sección `ClassicAxis` y nombre de clave **verbatim del INI del
  mod**, y **9 `StoreIni`**. Los `bool` se castean a `int32` al guardar porque **`StoreIni` no tiene
  sobrecarga `bool`** (`re3.cpp:285-320` solo tiene `uint32/uint8/int32/int8/float/char`): comprobado.
  `StoreIni` devuelve `void` y `ReadIniIfExists` devuelve `bool` cuyo resultado **no** se usa, igual que
  las 20 líneas de al lado.

**B1 · Estado compartido** — `src/core/Camera.h` + `src/core/Camera.cpp`

- `Camera.h`: `struct CAimClassicAxisSettings` **antes** de `class CCamera` (para que la vean los tres
  ficheros sin include nuevo) + **10 `static`** en `public`, prefijo `s_viceExtAim` (NO `s_odAim*`, que ya
  significa "lo midió el carril B4"). El `//` de por qué está en `CCamera` y no en `PlayerPed.h`.
- `Camera.cpp`: definición de los 10 `static` con los **defaults verbatim del INI del mod** (salvo
  `zoomForAssaultRifles = true`, que va rotulado como **decisión §5.3a, no default del mod**), y reset del
  estado en `CCamera::Init` (los 9 ajustes **no** se tocan ahí: viven en el literal y los sobreescribe
  `re3.cpp`).

**Verificación hecha por inspección (NO hay compilación: falta autorización, RULES 0.4):**

| Comprobación | Resultado |
|---|---|
| Sobrecargas `ReadIniIfExists` (`bool*` `:230`, `int32*` `:241`, `float*` `:263`) | existen, usadas las 3 |
| Sobrecargas `StoreIni` (`int32` `:299`, `float` `:313`) | existen; **no** hay `bool` → casts correctos |
| `CRGBA` visible desde `Camera.h` | sí: los 5 TUs incluyen `common.h` en la **línea 1**, antes de `Camera.h` (mismo patrón que el `CVector` que `Camera.h` ya usaba) |
| `nil`, `CEntity*`, `CVector`, `int16`, `uint32` | disponibles |
| `MODE_NONE` dentro de `CCamera::Init` | **NO compilaba**: el enum es de `CCam`. Corregido a `CCam::MODE_NONE` en las 2 apariciones (definición y `Init`), como hacen las 20 líneas vecinas |
| Finales de línea | los 4 ficheros a **CRLF puro** (`LF-sin-CR = 0`). `config.h` era mixto y mis 14 líneas entraron con LF; `Camera.h/.cpp` y `re3.cpp`se informeraron 3 líneas en blanco. Todo corregido |
| `git diff --check` | limpio |
| Cambios | **puro insert**: las 12 líneas que `git diff` marca como borradas son de carriles anteriores sin commitear (`DEFAULT_NEAR` del bloque GeniusZ, `*SIZE` de `PoolSize`, `MUCH_SHORTER_OUTRO_SCREEN`, `mINI::INIFile`). **Ninguna es mía** |
| Regresión `RC`/`H`/`R27` | **no verificada** (requiere compilar): B0/B1 no activan ninguna ruta — hasta que B2/B9 enciendan la ley, `s_viceExtAim` no lo lee nadie |

**Pendiente de autorización:** `ninja …/core/Camera.cpp.o` + `…/core/re3.cpp.o` para confirmar que
compila. Hasta ese momento el trabajo de B0/B1 es **código escrito, no verificado**.

**B13, B14, B15, B17 (el resto del build 1) siguen sin empezar:** B13 necesita `ControllerConfig.h/.cpp`
(s libres) **y** su consumidor en `PlayerPed.cpp` (bloqueado). Se puede hacer la definición del binding,
pero sin el consumidor no es verificable, así que se deja para cuando `PlayerPed.cpp` se libere y poder
cerrar el build 1 entero de una vez.

### 11.4 Build 1 `2026-09-27-aim1` — CERRADO

El jugador confirmó (27/09) que el carril de nado **había terminado** y **autorizó compilar y enlazar
con tag nuevo**. Con eso el bloqueo duro de §11.2 se levanta.

**Verificado antes de empezar:** `PlayerPed.cpp` sin cambios desde 41 min, `VERSION` en
`2026-09-26-swim10`, carril de nado quieto. El build de `aim1` **incluye** lo que el carril de nado
dejó en el árbol (mismo árbol, sin conflictos).

**Hecho en este build (todo el build 1 del plan, B0+B1+B13+B14+B15+B17):**

| Bloque | Fichero | Qué |
|---|---|---|
| **B0** | `src/core/config.h` | `#define VICEEXT_AIM_CLASSICAXIS` con el "qué NO entra" |
| **B0** | `src/core/re3.cpp` | 9 `ReadIniIfExists` + 9 `StoreIni`, sección `ClassicAxis`, claves verbatim |
| **B1** | `src/core/Camera.h` | `struct CAimClassicAxisSettings` + 10 `static s_viceExtAim*` |
| **B1** | `src/core/Camera.cpp` | definiciones con defaults verbatim + reset en `Init` |
| **B1** | `src/core/Camera.cpp` | traza **`AIMCFG`** (una vez, en el primer `Process`, no en `Init`) |
| **B13** | `src/core/ControllerConfig.h` | acción `PED_WALK` al final del enum (no reindexa) |
| **B13** | `src/core/ControllerConfig.cpp` | default `rsLALT`, `SETACTIONNAME`, `case → ACTIONTYPE_1RST3RDPERSON` |
| **B13** | `src/core/re3.cpp` | `"PED_WALK"` al final de `iniControllerActions[]` |
| **B13** | `src/peds/PlayerPed.cpp` | consumidor en `ProcessControl` + traza **`AIMWALK`** por flanco |
| **B14** | `tools/viceext-log-check.py` | bloque `AX` (AX0) + los 3 registros (`main`, resumen, `nuevas`) |
| **B15** | `tools/check-served-build.sh` | 2 marcas (`AIMCFG`, `AIMWALK`) |
| **B17** | `docs/mods/ATTRIBUTION.md` | **las 2 filas falsas corregidas** (`:100` y `:132`) |
| **B16** | `web/lib/index.js` | `VERSION` → `2026-09-27-aim1` |

**Pendiente que se resolvió sobre la marcha:** el enrichment dejó un `PENDIENTE` en B13 sobre cómo
se escribe un par tecla+modificador. Resuelto leyendo el código: `SetControllerKeyAssociatedWithAction`
toma un `RsKeyCodes` y el token `LALT` del INI del mod se resuelve a `rsLALT = 1051`
(`skeleton.h:173`), o sea que **el default es la tecla `rsLALT` tal cual**, no un par `L`+Alt. La acción
sale con su nombre en el menú de controles y es rebindable.

**Verificación ejecutada:**

| Qué | Cómo | Resultado |
|---|---|---|
| Compila | `ninja` de `Camera.cpp.o`, `re3.cpp.o`, `ControllerConfig.cpp.o`, `PlayerPed.cpp.o` | 4/4, solo warnings preexistentes |
| Enlaza | `bash gta_vc_browser/build.sh` | `[235/235] Linking ... reVC.js` |
| Marcas | `bash tools/check-served-build.sh` | **`OK: el motor servido lleva TODAS las marcas (y los datos nuevos)`** — las 2 nuevas `OK` |
| Bloque `AX` | `py_compile` + 5 logs sinteticos | `INCONCLUSIVE` sin `AIMCFG` · `OK` con los 9 verbatim · `FALLO` con `lock=2 fov=0` · `OK` con `AIMWALK spd=0.00` · `FALLO` con `AIMWALK` sin `spd=0.00` |
| Sintaxis shell | `bash -n check-served-build.sh` + `--listar` | OK, las 2 marcas listadas |
| Finales de línea | los 9 ficheros tocados | los que eran CRLF siguen **CRLF puro**; `check-served-build.sh` y `ATTRIBUTION.md` siguen **LF** (son shell/md, no `.cpp`) |
| `git diff --check` | los tocados | limpio |
| `dataTag` | `check-served-build.sh` | `2026-09-26-ve14` **intacto** (este plan no toca datos) |

**Lo que este build NO cambia, a propósito:** la cámara de apuntado sigue siendo `Process_Syphon`
(vanilla). Este build **sólo** pone los 9 ajustes, la tecla de andar y la instrumentación para
medirlos. No se nota nada en juego salvo que pulses **LAlt**, que te pondrás a caminar en vez de correr.

**Criterio PASS del build 1 (§8 recortado a lo que este build puede medir):**
`AIMCFG forceauto=0 lock=1 tri=1 mcx=0.530 mcy=0.400 brazo=0 sensx=1.00 sensy=1.00 fov=1` en el log,
más al menos un `AIMWALK tecla=1051 spd=0.00` al pulsar LAlt y otro con la velocidad de antes al
soltar, y el bloque `AX` en `OK` con los bloques `H`/`R27`/`R5`/`RC` **sin cambio de veredicto**.

**Siguiente:** `aim2` (B2-B7 + B9(a-d) + B11) — la ley de apuntado del mod. `PlayerPed.cpp` ya está libre.

### 11.5 Builds `aim2` (B2-B7 + B9 + B11) y `aim3` (B8, B10, B12) — EN UNA SOLA PASADA

El jugador pidió (27/09) ejecutar TODO el plan en una sola pasada y medir al final, en vez de
build por build. Hecho así, con el riesgo asumido y dicho: una sola pasada concentra el
riesgo (si la ley se rompe, arrastra a HUD, agachado y colisiones sin haber medido por separado).

**Todo el código de B2 a B12 está escrito y compila. Enlazado con `VERSION` =
`2026-09-27-aim2` y `check-served-build.sh` = `OK` con las 13 marcas nuevas.**

#### Lo que se escribió

| Bloque | Fichero | Qué |
|---|---|---|
| B2 | `Camera.h`, `Cam.cpp` | `case MODE_AIMING` **descomentado** (llevaba muerto desde siempre) + declaración de `Process_AimWeapon` y 2 ayudantes |
| B3 | `Cam.cpp` | `Process_AimWeaponFovLerp` (50° en rifles), `Process_AimWeaponCrouchOffset` |
| B3c | `Cam.cpp` | `Process_AvoidCollisions` **EXTRAÍDO** de la ley de coche (no copiado) y la ley lo llama. Los statics de colisiones subidos a ámbito de fichero porque ahora los escriben las dos leyes |
| B3d | `Cam.cpp` | `RightAnalogStickSensitivityX/Y` en los 3 sitios de la ley de a pie (con default 1.0 = **bit a bit idéntico**) |
| B4 | `Cam.cpp` | `Process_AimWeapon` completa, traducción línea a línea de `CamNew.cpp:233-388` |
| B5 | `Cam.cpp` | hombro en espacio de objeto, **sin suavizar** (el mod no lo suaviza) |
| B6 | `Cam.cpp` | callsite del recoil `ViceExtRecoilBegin/Apply` con `source="aim-weapon"`, límites `-PI/+PI`. **Matemática intacta** |
| B7 | `Camera.cpp` | override de la transición (500 ms, 0.1/0.9) al entrar y al salir de la ley. `CamControl` **NO se tocó** |
| B8 | `Camera.cpp` | `Find3rdPersonQuickAimPitch` con la fórmula del mod (`0.01403292f`) + los multiplicadores de retícula desde el ajuste |
| B9 | `PlayerPed.cpp/.h` | `ViceExtProcessPlayerPedControl`: los 5 predicados, el `IsType1stPerson` con early-return, la toma y devolución del modo, la rotación, los **3 `PedIK.MoveLimb`**, C18-3 y C18-4 |
| B10 | `PlayerPed.cpp` | `ViceExtFind3rdPersonMouseTarget` (el objetivo blando de ratón), C8 (cambio de arma bloqueado), C10 en los **4 callsites** de `Fire` |
| B11 | `Hud.cpp` | las 3 funciones del mod: la cruz de ±14 px, la marca de fijado SA/LCS con su ventana de 250 ms, y el triángulo sobre el objetivo blando |
| B12 | `PedFight.cpp` | C18-1 (el `return` propio con su `return` antes del bloque R28) |

**13 trazas nuevas**: `AIMLAW`, `AIMCAM`, `AIMFOV`, `AIMCOL`, `AIMLOCK`, `AIMIK`, `AIHTRI`,
`AIMWPN`, `AIMHUD`, más las 2 de `aim1`.

#### 20 fallos que hubo que corregir al compilar (ningún por lógica, todos de nombres/estructura)

Los que **hubieran roto el arranque** si no se hubieran visto:

1. `MODE_NONE` dentro de `CCamera::Init` no compila: el enum es de `CCam`. Dos sitios.
2. La llave `void` de `Process_AimWeapon` / `Process_Syphon` / `ProcessControl` quedó **partida** por
   la inserción de las funciones nuevas (el `void` se quedó huérfano delante del bloque).
3. `Process_AvoidCollisions` quedó con **dos llaves de cierre** y un `if(0)` de mi primer intento
   (que se descartó antes de escribir). Rehecha limpia.
4. Los statics de colisiones estaban **dentro** de la función de coche y la función extraída no los veía.
5. `m_uiTransitionState` es de `CCamera`, no de `CCam`.
6. Las armas son `m_weapons`/`m_currentWeapon` (vía `GetWeapon()`), no `m_aWeapons`/`m_nCurrentWeapon`.
7. `bIsDucking` y `bCrouchWhenShooting` son **bitfields directos** de `CPed`, no de `m_nPedFlags`.
8. `bIsVisible` es bitfield directo de `CEntity`, no de `m_nFlags` (`Placeable`).
9. `m_bHasLockOnTarget` es de `CPlayerPed`, no de `CPed`.
10. `m_f3rdPersonCHairMultX/Y` son `static` de `CCamera` (había que cualificar).
11. `TheCamera.m_fOrientation` no existe: el yaw es `Cams[ActiveCam].Beta`.
12. `IsTypeMelee` es de `CWeapon` (`GetWeapon()->IsTypeMelee()`), no de `CPed`.
13. `CSprite::CalcScreenCoors` pide `RwV3d*`, no `CVector2D*`, y `Hud.cpp` no incluía `Sprite.h`.
14. `ViceExtCanAim` era `static inline` de `PlayerPed.cpp` → **movido a `PlayerPed.h`** porque `Cam.cpp`
    necesita el MISMO caso de las escopetas (dos copias sería el fallo de RULES 0.6).
15. La sangria real dentro de `CHud::Draw` es de **3 tabuladores**, no de 2 (me costó dos intentos).
16. `s_odWpnWas` / `s_odLockWas2` declarados dentro de un `if` y usados fuera.
17. Un `#endif` y una `}` huérfanos de un intento anterior en `Hud.cpp`.
18. El `print` del verificador sin operador `%` (mostraba `amax=%.1f°` en crudo).
19. `AX2` comparaba la **componente X** del offset con 0.20, pero X = `0.20*cos(rumbo)`: ahora
    comprueba la **magnitud** del vector, que sí debe medir 0.20 para cualquier rumbo.
20. `eWeaponType` del Minigun: el verificador comprueba el 28, y hay que confirmarlo contra la tabla.

**Archivos tocados** (11): `src/core/config.h`, `src/core/Camera.h`, `src/core/Camera.cpp`,
`src/core/Cam.cpp`, `src/core/ControllerConfig.h`, `src/core/ControllerConfig.cpp`,
`src/core/re3.cpp`, `src/peds/PlayerPed.h`, `src/peds/PlayerPed.cpp`, `src/peds/PedFight.cpp`,
`src/renderer/Hud.cpp`. Más `tools/viceext-log-check.py`, `tools/check-served-build.sh`,
`web/lib/index.js`, `docs/mods/ATTRIBUTION.md`.

**Verificado**:
- Los **5** objetos modificados compilan (0 errores, solo warnings preexistentes).
- `build.sh` → `OK`, `check-served-build.sh` → **`OK: el motor servido lleva TODAS las marcas`**.
- Bloque `AX` probado con **3** logs sintéticos: `OK` con la ley bien (modo 5, dist 2.70,
  hombro 0.20, `amax` 18.2°, FOV 50.10) · `FALLO` con los 5 criterios rotos a la vez y el motivo
  exacto de cada uno · `INCONCLUSIVE` sin `AIMCAM`.
- `dataTag` sigue `2026-09-26-ve14` (este plan no toca datos).

**NO verificado (y es lo importante):** el comportamiento en partida. Nada de esto se ha visto
todavía. Los 17 criterios de §8 se miden ahora, de golpe, con la sesión del jugador.

### 11.6 Ronda de corrección tras la sesión del 28/09 — `2026-09-28-aim3`

El jugador jugó (build `crouch29`, que ya llevaba mi código dentro) y confirmó: *«varios bugs
al apuntar y de más», y de ClassicAXIS **vi 0**». El log (2,5 MB, 70 muestras de la ley) y el
bloque `R9`SEXPONEN 4 fallos, **3 de ellos míos**. Todo corregido y verificado.

#### Lo que la medición confirma que SÍ funciona

`AX1` modo 5 y `dist` 2,62–2,82 (el mod: 2,70) · `AX2` hombro 0,20 con `obj=1` y offset real
0,196–0,201 · `AX3` `amax` 38,5° · cruz dibujada 65 veces · objetivo blando adquirido
(`AIHTRI h=1` a 3,4–25,5 m) · `WalkKey` con `spd=0.00` · `AIMCFG` con los 9 verbatim ·
colisiones sin crashes (`nc=0,900` estable) · **sin regresión del recoil** (`multY=0.400` en 91
comprobaciones, `R14 OK`) · **sin regresión del auto-centrado** (3 `camauto2`, 0 con empujón)
· **sin regresión del nado** (curva Serega en banda: deriva 1,10 / crucero 2,32).

#### Los 4 fallos, con su causa medida

| # | Síntoma medido | Causa | Arreglo |
|---|---|---|---|
| **1** | `R9` `desv=90.0` constante (arma 17: 19 de 32 muestras; arma 19: 6 de 24). **Antes `R9` daba `OK`** → regresión mía | Usé `Cams[].Beta` para el rumbo **y** forzë `SetHeading` con la convención equivocada. El motor ya pasa `LimitRadianAngle(-TheCamera.Orientation)` a `SetLookFlag`/`SetAimFlag` (`PlayerPed.cpp:1793`) y **deja que su propia maquinaria gire el cuerpo**; mi `SetHeading` la peleó | `front = LimitRadianAngle(-TheCamera.Orientation)` (el valor del motor) y **se elimina la llamada a `SetHeading`**. El `RotatePlayer` del mod (`Main.cpp:547-574`) es esa maquinaria, que en nuestro árbol ya existe. Con fijado tampoco se sobrepone el rumbo: lo pone el motor vía `m_pPointGunAt` |
| **2** | `AIMHUD marco=0` en **65 de 65** → la marca de fijado nunca se dibujó | Declaré el estado (`s_viceExtAimLockOnUntil`/`LastLockPos`/`LastLockCol`) y **nunca lo escribí**: el mod lo escribía dentro de su `DrawAutoAimTarget`, y al mover el dibujo a `Hud.cpp` el escritor se quedó sin sitio. Código muerto | El escritor va en `PlayerPed.cpp`, en la rama de **"el fijado se queda"** (no en la de soltar, que es lo contrario de lo que pasó), con la posición, el color por salud (SA y LCS) y los 250 ms de `Main.cpp:824-828` |
| **3** | Una sola línea `AIMLAW` y **ninguna** con `aim=0` → el auto-aim no se podía juzgar | El detector de flanco estaba **dentro** del `if (aiming)`, así que sólo podía ver la entrada | El bloque se mueve **fuera** del `if`, y añade `pad=` al final |
| **4** | `aut=1` jugando con ratón → el auto-aim seguía **activo**, lo contrario de §5.2(a) | `ViceExtHasPadInHands` decía "hay mando si el stick derecho devuelve CUALQUIER cosa", y el stick devuelve ruido | Dos condiciones: `CPad::IsAffectedByController && GetMode() == 3` (el único modo paddle real) **y** stick por encima de la zona muerta (40, el mismo umbral de `GetLookAroundLeftRight`) |

Dos trampas del propio motor que costaron un rato y quedan escritas en el código:
`CURMODE` es un macro **local de `Pad.cpp:2327`** (no existe en `PlayerPed.cpp`), y
`#define DETECT_PAD_INPUT_SWITCH` (`config.h:369`) **no tiene valor**, así que va con `#ifdef` y
nunca con `#if`.

#### El fallo del verificador (el más importante de esta ronda)

Mi bloque `AX` dio **`OK`** con el cuerpo a 90 grados, porque solo miraba `dist`, `hombro` y `amax`:
números de la ley, nunca de lo que el jugador ve. **Un verificador que no detecta el fallo más
visible no sirve**, y encima habría que ejecutar el bloque `R9`, que sí lo cazaba.

Arreglo: nuevo criterio **`AX6`**, que mide la alineación del cuerpo con `AIMDIR` y falla con `desv
> 5°`. Comprobado: con el log del 28/09 da **`FALLO` naming el fallo exacto**
(`desv max {19: 90.0, 17: 90.0}`), y con la ley ya arreglada tendrá que dar `OK`.

Además se arregló el `print` del `AX3` que salía con `%.1f°` en crudo, y el `AX2` comparaba
la componente X del offset con 0,20 siendo X = `0.20*cos(rumbo)`: ahora comprueba la **magnitud**.

#### Lo que NO es culpa de ClassicAXIS (queda abierto para otros carriles)

- **`H` nado**: ya fallaba antes de mi cambio (log de `swim9`: 24 salidas, 6,5 m/s). Ahora 17 salidas
  y un pico de **20,98 m/s**. La mediana del avance (2,12 m/s) y las bandas de la curva Serega están
  bien, así que la ley de nado no se tocó; el pico es la salida del agua poco honda, que el
  plan de nado ya daba por rota (`swim18`). **El máximo pasó de 6,5 a 20,98** y conviene mirarlo.
- **`R27` FAIL**: `GunMove_BWD/FWD` fuera de carril, `pies` fuera de raíz, giro inconsistente. Es el
  **carril de agachado** (llegó a `crouch29`; `agachado-calibrado.md` §"carril ClassicAXIS" ya lo
  tenía como pendiente). **Mi `C18-1` toca justo `SetPointGunAt`**, así que hay que coordinarlo.
- **`R17`/`R19` FAIL**: también del carril de agachado.
- **`AX4` FOV 50**: **sin medir, no fallido**. Solo se usaron Colt45 (alcance 30) y chromegun
  (alcance 40); el umbral del mod es `wepMinRange = 70`, y además ambas tienen `CANAIM_WITHARM`, que
  el mod también excluye. Falta un M4/Ruger/M60.
- **`R16`**: no se sacó el lanzacohetes → `SIN DATOS`.
- **`RC`**: `INCONCLUSIVE` por falta de `RECOIL_DIAG_END` (no se cerró la pestaña).

**Conflicto de carril que hay que decir:** `VERSION` estaba en `2026-09-26-swim11` (el carril de nado
la había subido mientras yo arreglaba) y la he puesto a `2026-09-28-aim3`. El `wasm` servido lleva
las dos líneas, pero el otro agente y yo estamos pisando el mismo `index.js`.

## 12. RONDA 2 (28/09) — «no asumas, porta el código del mod» (orden del jugador)

### 12.0 Lo que el jugador corrigió (y por qué importa)

1. **Regla anulada** (`.agents/plans/mods/12-handoff-tandas2.md` §4 punto 2): la
   «regla previa» (¿el motor ya lo hace? → `**YA ESTABA**` → no tocar código»)
   queda **ANULADA**. Motivo, medido: se usó para saltar siete cosas del mod y el
   resultado fue que la mitad de la mecánica no funcionará, con el verificador
   dando `OK` mientras el cuerpo iba 90° desviado. La regla nueva es: **la
   equivalencia se demuestra leyendo los dos lados; si no se demuestra, se porta el
   del mod.** Se siguen evitando duplicados *dentro de nuestro* código (RULES 0.6).
2. **El defecto que el jugador detectó**, textual: con **A/D solo, sin mover el
   ratón**, el cuerpo debe **girar hacia la dirección de marcha y caminar hacia
   ahí**; con **el ratón y el cuerpo quieto**, el cuerpo **se queda quieto y solo
   gira**, permitiendo el **360°**. En el estado anterior el cuerpo **giraba siempre
   con la cámara**. **No estaba implementado.**

### 12.1 Lo que se PORTÓ (esta ronda), con su archivo:línea del mod

| Hook | Mod | Puerto en | Qué hace |
| --- | --- | --- | --- |
| **C1** | `Main.cpp:121-133`, `:1307-1308` | `PlayerPed.cpp` (helper `ViceExtStrafeAiming` + `PlayerControl1stPersonRunAround`) | Sin apuntar → `TYPE_WALKAROUND`: el cuerpo gira hacia donde caminas. Apuntando → `TYPE_STRAFE`. **Y consume `forceRealMoveAnim`** (el 360°) |
| **C1 (2ª mitad)** | `CamNew.cpp`/`Cam.cpp:1988` | `Cam.cpp` (`Process_FollowPedWithMouse`) | **Eliminado** el bloque que en CADA frame obligaba a `m_fRotationCur = m_fRotationDest =` cámara. Era el que pisaba el rumbo del cuerpo |
| **C2** | `Main.cpp:144-152` | `ViceExtStrafeAiming` | `!m_bHasLockOnTarget` respetado: con auto-fijado el cuerpo mira al objetivo, no de costado |
| — | `Main.cpp:125-127` | `ViceExtStrafeAiming` | **Flag muerto encontrado y arreglado**: `s_viceExtAimForceRealMoveAnim` se ponía y se borraba, **nadie lo leía** |
| **C4** | `Main.cpp:113-117` (2 nops) | `Camera.cpp` | Los **dos** `ReqMode = MODE_FIGHT_CAM` apagados → nunca entra en fight cam |
| **C9** | `Main.cpp:387-404` | `PlayerPed.cpp::DoWeaponSmoothSpray` | El mod **sustituye** la función entera: `0.00001f` agachado, `-1.0f` si no. Se anula el `switch` del motor |
| **C11** | `Main.cpp:207-211` | `PlayerPed.cpp::ProcessPlayerWeapon` (2 callsites) | `ClearAimFlag()` si no se está apuntando: el ped no se queda con la pose de apuntado colgada |
| **R1** | `CamNew.cpp:82-86`, `:215-220`, `:411-429` | `Cam.cpp::Process_FollowPedWithMouse` | Los tres faltaban a pie: `duckOffset` en la z del objetivo, **agua** (near clip 0,2 + `Source` solo con la distancia horizontal + z a nivel+0,6) y **esconder peds < 0,5 m** |

### 12.2 Correcciones de la ronda anterior (leídas, no supuestas)

- **`C18-5` YA ESTABA PORTADO** en `PedFight.cpp:220-226` (`blendAmount = 0` +
  `blendDelta = 8`), lo hizo el **carril de agachado (R29)**. Yo lo tenía marcado
  como pendiente **sin haberlo leído**. La regla nueva sirve igual en los dos
  sentidos: no se marca nada ni como hecho ni como pendiente sin leer.
- **`ignoreRotation` es un flag vestigial del mod**: se declara en `Main.cpp:30` y
  **solo se pone a `false`** (`Clear()` en `:580` y `:1225`); nunca a `true`. Así que
  `isAiming && !ignoreRotation` ⇒ `isAiming`, y la traducción queda 1:1.
- **`StoriesAimingCoords`: NO EXISTE en el mod** (0 coincidencias en `Main.cpp` y
  `CamNew.cpp`). No hay nada que portar. Cierra el «pendiente» del plan.
- **`bForceLegsMovements`: NO EXISTE con ese nombre en el mod** (0 coincidencias). Su
  equivalente real es **`forceRealMoveAnim`**, que es lo que se ha conectado en C1.

### 12.3 Lo que NO se pudo portar y por qué (sin inventar)

- **`C5` (sin point-gun cam)**: el mod lo hace con **tres parches de bytes crudos**
  (`Nop(0x472422, 6)`, `Nop(0x4724D5, 9)`, `Nop(0x47254F, 9)`, `Main.cpp:219-222`) y
  **no hay código fuente que|traducir**. En este motor **no existe
  `MODE_POINT_GUN`** (0 coincidencias en `src/core/*.h`), o sea que el modo de
  cámara que el mod desactiva no tiene equivalente con ese nombre. Identificar los
  3 bloques exigiría el binario original de VC desensamblado. **Pendiente de
  binario, no se toca.**
- **`C7` (fix de salto)**: `Set<BYTE>(0x4F0031, 0xEB)` (`Main.cpp:118`) — un solo
  byte que convierte un salto condicional en incondicional, **sin código fuente**. La
  dirección `0x4F002A` es además una de las seis de `playerMovementType`, o sea
  que está en la función de locomoción, pero **qué condición exacta se
  rompe** no se puede leer sin el binario. **Pendiente de binario, no se toca.**

### 12.4 Verificación de esta ronda

- `PlayerPed.cpp`, `Cam.cpp`, `Camera.cpp`, `PedFight.cpp`: **compilan** (los 4
  objetos, con `ninja`).
- Marker nuevo para medir: **añadir** la medición de si el cuerpo se encara al
  andar de lado sin apuntar (la que el jugador acaba de describir).
- **Sin sesión del jugador todavía**: nada de esto está medido en juego.
- Choque de carril (otra vez): `VERSION` la subo a `2026-09-28-aim4`; el `wasm` servido
  seguirá llevando `aim3` hasta que se relinké.
