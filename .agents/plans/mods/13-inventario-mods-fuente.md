---
name: 13-inventario-mods-fuente
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 13 · Inventario total de `mods/` + fuentes portables para rehacer agachado/nado/apuntado/recoil ADAPTANDO (no reinventando)

> Fecha: 24/09/2026. Origen: encargo directo del jugador — el agachado, nado,
> apuntado y recoil actuales se crearon desde cero y funcionan mal; hay que
> leer TODO `mods/`, comparar cómo funciona allí vs aquí, y copiar/adaptar
> código funcional en vez de adivinar. 3 subagentes en paralelo (2 explore
> solo-lectura + 1 minería web) + verificación propia. Cero ficheros del juego
> tocados en esta pasada: solo este documento.
>
> Nota de cifra: el jugador habló de "50%"; la cifra vigente acordada (E22) es
> **≈30% o menos**. Este documento es la ruta para subirla con las mecánicas
> vitales que faltan.

## 0. Respuesta a "¿puedes llamar a subagentes?"

**Sí.** Demostrado en esta pasada: 3 a la vez (inventario nado/agachado,
inventario primera-persona/cámaras, minería GitHub), cada uno con su tabla de
ficheros y orden de solo-lectura. Reglas que se usan: un bloque = un dueño de
ficheros (índice `00` §4), nada de commits, el enlace final lo hace un solo
agente, y todo veredicto queda en planes + `ATTRIBUTION.md`.

## 1. Qué hay en `mods/` (13 entradas) y su cobertura real

| Entrada | Qué es | Cobertura en planes |
|---|---|---|
| `extended/` | Vice Extended (el mod objetivo, sin fuente: `ViceEx.exe`) | Total (packs 1-4 + mecánicas). PERO sus 4 mecánicas se reimplementaron desde cero → §2 |
| `silent patch/` | SilentPatch (fuente MIT + `.asi` + `data/maps` + ini) | Total (`01`, `08` B6, tandas) |
| `widescreen/` | WidescreenFixesPack (MIT, `.ixx` + ini) | Total (`03`, `10` B7) |
| `framerate/` | FramerateVigilante (**solo `.asi` + `.ini`, sin fuente local**) | Parcial: plan `02` con anclas; fuente real hallada en GitHub (§4.6) |
| `skygfx/` | SkyGfx (solo binarios `d3d8.dll`/`rwd3d9.dll` + carpeta) | Parcial: spec B9 desde `aap/skygfx_vc` (sin LICENSE: reimplementar) |
| `Classic AXIS/` | ClassicAxisVC.asi (2022) + ini 898 B | **NUEVO en este doc**: ini completo + defaults (§3.3) |
| `1510564741_ca-1/` | ClassicAXIS v1.6 (2017) + `addon/ClassicAXIS/anim/movements.img` | Parcial (B8): este doc corrige el malentendido "piernas" (§3.3) |
| `climbing/` | Climbing (¡CON `source_code/`!) | Hecho (E1). Modelo a seguir: fuente real → port directo |
| `ginput/` | GInput (cerrado, solo spec ini/docs/api) | Spec (`04` §9, `06`): reimplementar sobre Gamepad API |
| `sa-crouch-movement_1774634386_626056/` | SA Crouch (CLEO + anim + `VC.CustomAnimsData`) | Parcial (C12): **NUEVO aquí** el `.dat` con flags 88/212 y la secuencia de entrada (§3.2) |
| `1498977446_107784/` | Nado de Serega (CLEO + `ped.ifp` 236) | **CERO cobertura antes. NUEVO aquí** (§3.1) |
| `1487678468_firstperson/` | 1ª persona de GeniusZ (`.asi`, cp1251) | **CERO cobertura antes. NUEVO aquí** (§3.3) |
| `SACarCam.asi` | Shim de cámara de coche (solo `GetGInputInterface`) | **CERO cobertura antes. Veredicto: nada que portar** (§3.3) |

## 2. La crítica es correcta: mapa honesto de qué SÍ es portable para las 4 mecánicas

El `ViceEx.exe` **no tiene fuente** (solo minería de strings/dispatch, ya hecha
en `11` §6): "portear el mod" para agachado/nado/apuntado/recoil NO puede ser
copiar su código. Lo portable de verdad es:

| Mecánica nuestra (estado) | Fuente portable real | Tipo |
|---|---|---|
| Agachado (R20c/R21/R26, 8-dir por código) | `sa-crouch` CLEO (secuencia + flags + filtros) + clips del mod (ya servidos) | Spec CLEO + dato |
| Nado (ViceExtSwimControl) | Serega `swim.cs` (curva 0.022/0.025/0.1 + escalas 0.004/0.05/0.12 + timeout 300) + `Stories Style Swimming` (estados) + `ThirteenAG/III.VC.SA.CLEOScripts` (MIT) | Spec + MIT |
| Apuntado (AIMDIR/desv, mira) | ClassicAXIS (`CameraCrosshairMult 0.53/0.4`, hombro, `LockOnTargetType=1`, sensibilidades `*FOV/80`) + jborza (fórmula `padHeading-Orientation`, ojo IK+0.19) | Spec (sin LICENSE: reimplementar) |
| Recoil (`multY` + offset cámara) | `WeaponRecoilAuto` (mira sube por precisión del arma, crosshair vuelve) + `Manual Driveby` (fire-rate y sonido por `weapon.dat`, ya lo cumplimos en P2/P4) | Spec (sin LICENSE) |
| FV (timestep) | **`GTAmodding/FramerateVigilante` — MIT confirmada** | **Portar con atribución** |

## 3. Hallazgos por mod (de los subagentes)

### 3.1 Nado de Serega — curva de velocidad copiable, clips NO

- Contenido: `anim/ped.ifp` 236 anims (RECORTADO: sin `Swim_*`, `DUCK_low` con
  raíz +5.1 m = bug que catapulta, `Drown` truncado) + `CLEO/swim.cs` 2288 B
  (158 instrucciones) + readmes sin valor técnico.
- Mecanismo: loop `WAIT 0` con guarda `PLAYING + NOT_IN_CAR + IN_WATER`;
  flotación escribiendo `0.022/0.025/0.1` + velocidad del stick escalada
  `*0.004 (arranque) / *0.05 (crucero) / *0.12 (sprint) * frametime` + timeout
  300 ticks; anims por ids internos 156/142/149/148 (= tread/crawl/breast/
  jumpout); botones 14=nadar, 8=cancelar, 16=salir. Todo con
  `READ/WRITE_MEMORY + CALL_FUNCTION 0x405640` (inportable a WASM).
- Extraer → `src/peds/PlayerPed.cpp` (`ViceExtSwimControl/Move/ClipSpeed`):
  la **curva** (deriva 0.022 parado, crucero 0.05, sprint 0.12, timeout 300) con
  `m_vecMoveSpeed` en vez de pokes; guarda triple; botones → `CPad`
  (Sprint/Jump); ids → enum `AnimationId` (`Swim_Tread/Crawl/Breast/jumpout`
  del `ped.ifp` 272 servido, que es MEJOR base que su ifp 236).
- NO copiar: offsets `ped+120/76/136/20`, `CALL 0x405640`, `0x975424`, su
  `ped.ifp` entero, botones CLEO crudos.

### 3.2 SA Crouch — secuencia de entrada + calibración, clips sueltos

- Contenido: `ped.ifp` 240 + `CrouchMovement(forClassicAxis).cs` 2052 B (284
  instr, `SET_CHAR_CROUCH 04EB` + `PLAY_ANIMATION 0673`) + `VC.CustomAnimsData`
  (`.asi` Win32 inútil + `.dat` 616 líneas: `%man ped 1 179` con
  `GunCrouchFwd/Bwd 173-174 flag 88`, `Crouch_Roll_L/R 175-176`,
  `GunMove_L/R 177-178 flag 212`; 88=loop/move, 212=one-shot).
- Mecanismo: test de flag agachado (`ped+336>>4&1`) + sticks (umbral ±16) +
  6 bloques por dirección/arma → `SET_CHAR_CROUCH 0` un frame →
  `PLAY blend 10` → `WAIT 500` → `SET 1` (+ mantenimiento blend 30 parado);
  filtro de armas `>=17 sin 28-33`; watchdog de salida; distinción
  Classic/Standard por `GET_CONTROLLER_MODE`. Clips con raíz medida ±2.74 m /
  0.731 s = 3.75 m/s; rolls 0.931 s.
- Extraer → `PlayerPed.cpp` (`ViceExtCrouchControl/Anim/RateFor/TargetSpeed`,
  `Ped.cpp` R20c): **calibrar a 2.74 m/0.731 s** (no a ojo); secuencia de
  entrada sin parpadeo; deadzone ±16; filtro de armas; flags 88/212 si se
  importan clips sueltos (`GunCrouchFwd/Bwd` están en `tmp/a4/`).
- NO copiar: `.asi`/`.dat 179`, `ped.ifp` 240 entero (pierde `Swim_*` y los
  `Crouch_*` stock de 24 seqs), offsets, botones crudos, ids mágicos 173-178.

### 3.3 Primera persona (GeniusZ), SACarCam, ClassicAXIS — lo que faltaba

- **GeniusZ** (cp1251: toggle vista con **V**, menú **Alt+B**, req. 1.0 US):
  cámara posicional con **offsets XYZ por ped y por vehicle-id + near-clip
  separado a pie/en coche + FOV y sensibilidad configurables** (+ flag link a
  sensibilidad del juego); contempla `WEAPON_crouch`; sin head-bob.
  Extraer → `Camera.cpp`/`Cam.cpp` (`MODE_1STPERSON_RUNABOUT`): nudge de ojo
  por modelo (no clipar techo), near-clip dual, FOV configurable, menú de
  calibración en vez de tecla fija. NO copiar: hooks Win32, bloqueo 1.0 US,
  defaults de `FirstPerson.cfg` (no viene).
- **SACarCam**: shim `GetGInputInterface` sin matemática propia.
  **Veredicto: nada que portar** (`Process_FollowCar_SA` ya lo cubre).
- **ClassicAXIS**: ini nuevo completo (`ForceAutoAim`, `LockOnTargetType=1`,
  `ShowTriangleForMouseRecruit`, `WalkKey=LALT`, `CameraCrosshairMultX/Y=
  0.53/0.4`, `StoriesAimingCoords/PointingArm=false`, sensibilidades 1.0) e
  ini viejo (`bForceLegsMovements=0`, `bMoveCameraOnVehicle=1`,
  `bEnableFirstPersonMode=1`). **Corrección**: `movements.img` NO trae
  `walk_left/right/back` (son 34 clips de arma: `*_crouchfire/crouchreload` =
  bloque C7); la spec B8 que los cite como "piernas" debe corregirse.
  Extraer → `Camera.cpp` (hombro 0.2/0.55, dist 2.7, clamp ±50°,
  `CrosshairMult`, `LockOnTargetType`), `Hud.cpp` (triángulo de salud),
  `Pad.cpp` (sensibilidades `*FOV/80`, `WalkKey`). Recoil no está aquí
  (está en `features.ini` del Extended). NO copiar: `bForceLegsMovements=1`
  (pide addon + rompe vanilla; H4 ya lo suple), modos IV, acoplo GInput,
  copia literal (sin LICENSE).

## 4. Minería GitHub (licencias + qué extraer)

> **Actualización 27/09/2026:** la licencia **ya no bloquea** (`00-INDICE.md` §2):
> se toma lo que haga falta de cada fuente, con o sin LICENSE; la atribución es
> cortesía. La columna «Licencia» se conserva como inventario histórico.

| Fuente | Licencia | Qué extraer |
|---|---|---|
| `GTAmodding/FramerateVigilante` (+ `JuniorDjjr/CLEOPlus`) | **MIT** | `*CTimer::ms_fTimeStep` para todo lo frame-dependiente (`Timer.cpp`, nado, vehículos). **Portar con atribución** |
| `ThirteenAG/III.VC.SA.CLEOScripts` (+ Torno270 swim, crédito permisivo) | **MIT** / crédito | Plantilla `swim.traits` + estados con stamina/oxígeno → `PlayerPed.cpp`, `Stamina.cpp` |
| `WeaponRecoilAuto` / `WeaponRecoilRE` (HzanRsxa2959), `Bullet Spread/Recoil Fix` (jenksta), `Manual Driveby` (SpitFire), `Manual Driveby (VC)` (BirbsLeHecker) | Todas **solo spec** (cerradas/sin LICENSE) | Recoil = mira sube por precisión del arma + crosshair vuelve (NO sacudir cámara); spread por `accuracy+anim+stance` también con ratón; drive-by = fire-rate y sonido por `weapon.dat` (esto último YA lo cumplimos en P2/P4) |
| `SA Crouch Movement` (lomonosov) | **Prohibitiva explícita** | **Solo spec** (y con permiso para más): roll + 8-dir + fix de heavy; sin fuente `.txt` hallada |
| `Manual Aiming v1.5`, GeniusZ/BoPoH, jborza (código caído) | Solo spec | RMB-aim + correr apuntando + recarga R (YA tenemos C3.1/R); fórmula `padHeading-Orientation`; ojo IK+0.19 (YA lo hacemos en R5) |
| `sannybuilder/library` (`vc/vc.json`), `cleolibrary/III.VC.CLEO`, `NewOpcodes`, `CLEO-Redux`, `plugin-sdk` (DK22Pac, zlib) | Docs/zlib | CLEO-lite: implementar `0A8C/0A8D/0AA5/0605/04ED/SET_CHAR_CROUCH` como **wrappers a funciones nativas, jamás poke WASM** |
| `gennariarmando/classic-axis`, `aap/skygfx_vc` | **Sin LICENSE** | Solo reimplementación con atribución; SkyGfx no aporta nada a estas 4 mecánicas |

## 5. CLEO-lite enriquecido (respuesta a "quiero saber más")

Los `.cs` ya desensamblados lo confirman: `swim.cs` = memory-hack total
(`WRITE×4 + CALL + READ×4` a direcciones x86 1.0), `CrouchMovement` =
parcial (`READ×4` + `SET_CHAR_CROUCH` + `PLAY_ANIMATION`, retorno limpio).
Arquitectura (`06` §2, vigente): `cleo_disasm.py` (HECHO) → `gen_cleo_ops.py`
(code-gen desde `vc.json`, 1689 comandos, 0 colisiones) → `ScriptCleo.cpp`
(solo cuerpos delegando en mecánicas nativas) → loader en `Script6.cpp`.
**Añadido de esta pasada**: con `ThirteenAG/III.VC.SA.CLEOScripts` (MIT) hay
plantilla legal para `swim.traits`; con `NewOpcodes`/`CLEO-Redux` hay puente
recomendado en vez de memoria cruda. Lo que NUNCA entra: `0A8C`-style a
direcciones x86, `.cs` binarios, `MemoryModule`/DLLs, natives Redux.

## 6. Plan de re-trabajo: las 4 mecánicas adaptando (orden propuesto)

1. **Recoil** (más barato): mantener `multY` (bala+mira) + offset de cámara;
   añadir subida **por precisión del arma** (`weapon.dat`) con retorno
   (spec `WeaponRecoilAuto`); spread con ratón (spec jenksta). Ficheros:
   `Weapon.cpp`, `Cam.cpp`. PASS de oído + `RECOIL2/3`.
2. **Nado**: injertar la **curva Serega** (deriva/crucero/sprint + timeout) en
   `ViceExtSwimControl/Move` con `m_vecMoveSpeed`; clips del `ped.ifp` 272
   (no tocar); estados estilo VCS (spec `Stories Style`) + plantilla MIT.
   PASS: `SWIM2 avance` + ojo.
3. **Agachado**: calibrar a **2.74 m/0.731 s** + secuencia de entrada del CLEO
   + deadzone ±16 + filtro de armas; `GunCrouchFwd/Bwd` sueltos si hace falta
   (`tmp/a4/`). PASS: `CROUCHMOVE` + ojo (las piernas lentas se deciden con él).
4. **Apuntado**: `CrosshairMult 0.53/0.4` + hombro conmutable + `LockOnTargetType`
   + sensibilidades `*FOV/80` (spec ClassicAXIS/B8) + near-clip dual y offsets
   por vehículo (spec GeniusZ). PASS: `AIMDIR desv` + capturas.
5. **FV**: portar lo MIT de `FramerateVigilante` (`*ms_fTimeStep`) donde falte.

## 7. Qué evitar (global)

Binarios (`.asi/.cs`/DLLs), direcciones x86 y `hook::pattern`, memoria cruda
en WASM, `ped.ifp` completos ajenos (236/240: pierden `Swim_*/Crouch_*` y
traen bugs como el `DUCK_low` de 5.1 m), copiar literal (la licencia ya no bloquea),
`movements.img` como "piernas", `bForceLegsMovements=1`, defaults inventados
de inis ausentes, y re-empaquetar datos sin subir `dataTag`.

*Informes completos de los subagentes disponibles en el hilo; este MD es el
consolidado para relectura. Sin commits; `VERSION`/`dataTag` no se tocan en
una pasada de solo-lectura.*
