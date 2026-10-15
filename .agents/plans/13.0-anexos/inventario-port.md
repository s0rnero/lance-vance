# Inventario del delta del port Vice Extended (viceex-14) frente a reVC `a16fcd8d`

> Fecha: 2026-10-14. Sólo lectura: no se ha modificado nada del repo salvo este fichero.
> Base vanilla: `gta_vc_browser/tmp/revc-a16fcd8d/src` · Port: `src/` (rama `miami`, árbol de trabajo actual).
> Objetivo: saber qué hay que **quitar/sustituir** (mod viejo de los agentes) y qué hay que **proteger** (NADO, RADIO e infraestructura web).

## 0. Método

- Delta fichero a fichero con `diff -U0 --strip-trailing-cr <base>/<f> src/<f>`, para los 121 ficheros modificados y los 6 nuevos (`core/PedArbiter.*`, `core/ViceExtFeatures.*`, `skel/ondemand.*`). Resultado: **20 195 líneas añadidas** y **2 013 líneas vanilla borradas**.
- Para cada línea añadida se calculó la pila de preprocesador real (`#if/#ifdef/#ifndef/#elif/#else/#endif`) en el fichero nuevo:
  - bajo `VICEEXT_SWIM*` → NADO;
  - bajo `VICEEXT_RADIO_ICONS` → RADIO;
  - bajo cualquier otro `VICEEXT_*` → MOD-VIEJO, con su bandera (la más interna).
- El resto (sin guarda, sólo `__EMSCRIPTEN__` u otras macros vanilla) y todos los borrados se revisaron a mano, bloque a bloque, en 9 grupos de ficheros. También se reclasificaron las trazas que viven dentro de los bloques `VICEEXT_*`/SWIM/RADIO y el código de nado o radio que estaba bajo otras guardas.
- Validación: no queda ninguna línea añadida ni borrada sin clasificar.
- Categorías, **excluyentes** (cada línea cuenta una sola vez):

| Categoría | Qué es | Destino |
|---|---|---|
| INFRA-WEB | Port web que no es del mod: Emscripten/Asyncify, glfw web, OpenAL/sampman, on-demand, CdStreamPosix, GXT web, bucle principal, IDBFS, arreglos de crash del port | se queda |
| NADO | Natación y lo que sólo existe para ella | se protege |
| RADIO | Iconos/logo de emisora, `radio.txd`, retune web | se protege |
| MOD-VIEJO | Código del mod tras `#ifdef VICEEXT_*` | se sustituye |
| SIN-GUARDA | Código del mod viejo **fuera de todo `VICEEXT_*`**: altera el vanilla siempre. Incluye lo que sólo está tras `__EMSCRIPTEN__` pero es lógica del mod, y los borrados de vanilla hechos por el mod | se sustituye o se revierte |
| DIAGNOSTICO | Trazas e instrumentación de medida (`ODTRACES/ODTRACEI`, `P4 kind=…`, contadores, macros `VICEEXT_P4_EMIT`, statics `s_od*` que sólo leen trazas) | se quita, salvo lo que el tooling necesita (§7.3) |

- Convenciones:
  - Las líneas son números del fichero **nuevo** (`src/`). `oN` es un número de línea de la **base vanilla** (línea borrada).
  - Los comentarios y blancos cuentan en la categoría del código que documentan.

## 1. Totales por categoría

| Categoría | Líneas añadidas | de ellas código (sin comentario/blanco) | Líneas vanilla borradas |
|---|---:|---:|---:|
| INFRA-WEB | 3337 | 2606 | 831 |
| NADO | 719 | 583 | 14 |
| RADIO | 94 | 91 | 4 |
| MOD-VIEJO | 6208 | 4294 | 59 |
| SIN-GUARDA | 3867 | 2350 | 1097 |
| DIAGNOSTICO | 5970 | 5066 | 8 |
| **Total** | **20195** | **14990** | **2013** |

Subdivisiones útiles:

| Subdivisión | Líneas |
|---|---:|
| DIAGNOSTICO dentro de bloques VICEEXT_* (mod viejo) | 2714 |
| DIAGNOSTICO dentro de bloques de NADO | 257 |
| DIAGNOSTICO dentro de bloques de RADIO | 29 |
| DIAGNOSTICO sin guarda VICEEXT (sólo __EMSCRIPTEN__ o nada) | 2970 |
| NADO bajo otra bandera VICEEXT_* | 64 |
| NADO sin guarda VICEEXT | 196 |
| NADO tras VICEEXT_SWIMMING | 459 |
| RADIO fuera de VICEEXT_RADIO_ICONS | 29 |
| RADIO tras VICEEXT_RADIO_ICONS | 65 |

Notas para leer los totales:

- **SIN-GUARDA, borradas (1 097):** 817 son de `core/Cam.cpp`.
  - 574 vacían `Process_FollowCar_SA` (sólo se ejecuta con `FREE_CAM`/`bFreeCam`).
  - 131 son pura sangría perdida en `ProcessSpecialHeightRoutines` (con `diff -w` desaparecen).
  - 49 son el bloque de tilt/roll reindentado, que reaparece igual en 2848-2904.
- **INFRA-WEB, borradas (831):** glfw.cpp −385 (main troceado para `emscripten_set_main_loop`), Game.cpp −168 (`CGame::Initialise` troceado), Text.cpp −105 (GXT robusto). Además, 104 blancos en MenuScreensCustom.cpp y comentarios re-codificados de Font.cpp, colaterales y sin efecto.
- **DIAGNOSTICO (5 970 añadidas, el 30 % del delta):** 2 714 líneas viven dentro de bloques `VICEEXT_*` y desaparecerán con ellos. 2 970 están fuera de todo `VICEEXT_*`, y entre ellas hay trazas que el arnés **necesita** (FPHASE, FPSLOG, LOADSCR, `WR load-req`: §7.3).
- **MOD-VIEJO (6 208):** ya sin las trazas. Las banderas que más pesan son CROUCH (1 303), AIM_CLASSICAXIS (710), RECOIL (462) y DISTANT_LIGHTS (382).

---

## 2. NADO: lista cerrada y dependencias

### 2.1 Banderas y constantes

| Elemento | Dónde | Valor |
|---|---|---|
| `#define VICEEXT_SWIMMING` | core/config.h:390 | (definida) |
| `VICEEXT_SWIM_SEREGA_ENTRY` | peds/PlayerPed.cpp:3558 | 0.022f (velocidad de flotar al entrar) |
| `VICEEXT_SWIM_SEREGA_CRUISE` | :3559 | 0.025f (flotar tras el timeout) |
| `VICEEXT_SWIM_SEREGA_SWIM` | :3560 | 0.05f (braza) |
| `VICEEXT_SWIM_SEREGA_SPRINT` | :3561 | 0.12f (crol con esprint) |
| `VICEEXT_SWIM_SEREGA_TIMEOUT` | :3562 | 300 (ticks) |
| `VICEEXT_SWIM_STROKE_SLACK` | :3563 | 0.65f |
| `VICEEXT_SWIM_CRAWL_ROOT_M` | :3564 | 2.505f |
| `VICEEXT_SWIM_BREAST_ROOT_M` | :3565 | 3.011f |
| `VICEEXT_SWIM_CHEST_M` | :3566 | 1.45f |
| `VICEEXT_SWIM_HIP_M` | :3567 | 1.10f |
| Clips | animation/AnimationId.h:292-295 (sin guarda) | `ANIM_STD_SWIM_TREAD/CRAWL/BREAST/JUMPOUT` |
| Grupo de anim | animation/AnimManager.h:79-81, AnimManager.cpp:935-970 y 1115-1117 (sin guarda) | `ASSOCGRP_PLAYERSWIM` = `"playerswim"` sobre el bloque `"ped"` (del ped.ifp del mod) |
| Sonidos | audio/soundlist.h:206-211, AudioLogic.cpp:4533-4561 (sin guarda) | `SOUND_NADO_BRAZO_A/B/LS_A/LS_B` → `SFX_FOOTSTEP_WATER_1..4` (muestras vanilla) |

### 2.2 Bloques: rangos exactos (calculados tras la clasificación)

| Fichero | Líneas NADO (+) | Rangos (líneas nuevas) | Borradas vanilla (oN) | Traza interna (DIAG, opcional) |
|---|---:|---|---|---|
| peds/PlayerPed.cpp | 391 | 5, 9-10, 34, 36, 38, 133-136, 158-163, 319-325, 1100-1105, 1326-1329, 1519-1522, 2667-2670, 3556-3575, 3590-3643, 3647-3655, 3659-3663, 3667-3720, 3731, 3745-3756, 3760-3768, 3772-3807, 3815-3895, 3902-3926, 3940-3943, 4039-4047, 4130-4137, 6556-6564, 6581-6586, 6611-6617, 6626 | 1826-1828 | 173: 3576-3589, 3644-3646, 3656-3658, 3664-3666, 3721-3744, 3757-3759, 3769-3771, 3808-3814, 3896-3901, 3927-3939, 3944-4038 |
| core/PedArbiter.cpp | 99 | 1-3, 5-46, 54-56, 65-83, 92-123 | — | 24: 4, 47-53, 57-64, 84-91 |
| core/Cam.cpp | 88 | 1180-1241, 1255-1258, 1785-1794, 1812-1813, 6235-6242, 6260-6261 | — | 47: 1242-1254, 1795-1811, 6243-6259 |
| animation/AnimManager.cpp | 39 | 935-970, 1115-1117 | — | — |
| audio/AudioLogic.cpp | 29 | 4533-4561 | — | — |
| core/PedArbiter.h | 25 | 1-25 | — | — |
| peds/Ped.cpp | 22 | 10, 1983-1986, 1988-1989, 2013-2019, 2026-2027, 2029-2030, 2903, 3437-3438, 3446 | 1658, 1682-1683, 1690, 1692, 2565, 3099-3100, 3108 | — |
| animation/AnimationId.h | 9 | 287-295 | — | — |
| audio/soundlist.h | 6 | 206-211 | 206-207 | — |
| peds/PlayerPed.h | 4 | 123-126 | — | — |
| animation/AnimManager.h | 3 | 79-81 | — | — |
| renderer/Hud.cpp | 2 | 28, 455 | — | 13: 425-437 |
| peds/Ped.h | 1 | 1221 | — | — |
| core/config.h | 1 | 390 | — | — |

Qué hace cada pieza:

- **peds/PlayerPed.cpp:3556-4047.** Bloque principal: `CPlayerPed::ViceExtSwimControl` y `ViceExtIsSwimming`, junto con `ViceExtSwimClipSpeed`, `ViceExtSwimWake` (estela) y `ViceExtSwimWeaponFists`.
  - Puerta: profundidad > pecho para entrar y > cadera para mantenerse.
  - Al entrar reclama los carriles `PEDLANE_NADO` (MOVIMIENTO/POSTURA/APUNTAR/ARMA).
  - Mientras nada: flotación en Z, rumbo por la cámara, velocidad por estado (flotar, braza o crol), clip a ritmo por metros de raíz, sonido y salpicadura en cada brazada.
- **Ganchos en PlayerPed.cpp:**
  - :319-325 `bDrownsInWater=false`;
  - :1100-1105 bloqueo del cambio de arma;
  - :2667-2670 `ClearPointGunAt`;
  - :6556-6564, 6581-6586 y 6611-6626 en `ProcessControl` (sin control a pie ni `ProcessPlayerWeapon` mientras nada);
  - :158-163 dentro de `ViceExtIsAiming` (CROUCH), :1326-1329 y :1519-1522 dentro de ClassicAXIS, :4130-4137 dentro de CLIMB. Coordinan con features viejas (§2.3).
- **peds/PlayerPed.h:123-126.** Declaraciones.
- **core/Cam.cpp:**
  - 1180-1258 `IsTargetInWater`: el jugador vivo en el agua no pasa a `MODE_PLAYER_FALLEN_WATER`;
  - 1785-1813 `Process_FollowPedWithMouse`: el objetivo sube a nivel+0,5;
  - 6235-6261: lo mismo en `Process_FollowPed_Rotation` (FREE_CAM).
  - Los tres están en funciones vanilla, **no** dentro de las leyes reescritas.
- **peds/Ped.cpp, casi todo sin guarda:**
  - :1983-2030 `ProcessBuoyancy`: sin impulso de flotación, sin ahogo y sin el freno de entrada mientras el carril NADO tiene el movimiento;
  - :2903 `ProcessControl`: no está "en el aire" nadando;
  - :3437-3446 y Ped.h:1221: `particleProduceFootSplash` deja de ser static y recibe un `CRGBA` (lo usa la brazada);
  - :10 include.
- **core/PedArbiter.h/.cpp (148 líneas).** El árbitro entero (124 de NADO y 24 de traza `PEDCLAIM`). Ver §4.1.
- **renderer/Hud.cpp:455 (+ include :28), sin guarda.** Oculta la cruz si el carril NADO tiene `APUNTAR`. Va pegada a la línea 454 (ClassicAXIS): al quitar ClassicAXIS hay que conservar la 455.

### 2.3 Dependencias del NADO hacia código MOD-VIEJO / SIN-GUARDA (qué se rompe al quitar lo viejo)

| Dependencia | Dónde | Tipo | Qué hacer |
|---|---|---|---|
| **PedArbiter** (`ViceExtPedClaim/Release/Owns`, `PEDLANE_NADO`, `PEDCAP_*`) + `#define VICEEXT_PEDARBITER` (config.h:472, MOD-VIEJO) | PlayerPed.cpp:1101, 2668, 3704-3707, 3773-3776; Ped.cpp:1985, 2903; Hud.cpp:455; Cam.cpp (sólo consultas de leyes viejas) | dura: es la columna vertebral. Sin la bandera los stubs devuelven MOTOR o false: el nado mueve, pero se pierden la flotación de Ped.cpp, el bloqueo de arma y de apuntado y la cruz oculta | **Conservar** PedArbiter.* y la bandera. `PEDLANE_LEY` (1ª persona vieja) y `PEDLANE_AGACHADO_MOD` (sin uso) se pueden quitar |
| `ViceExtPedResetAll()` en `CGame::InitialiseWhenRestarting` | core/Game.cpp:989 (+ includes :2-3), **sin guarda**, contado como SIN-GUARDA | dura (reset de los carriles en cada carga) | **Conservar** |
| **ViceExtFeatures** (`ViceExtFeatures::On("EnableSwimming", true)`) + `#define VICEEXT_FEATURES_INI` (config.h:492) | core/PedArbiter.cpp:41 | dura de compilación: sin el .cpp no enlaza. Ojo: `ViceExtSwimControl` ignora el resultado de `Claim`, así que con `EnableSwimming=0` el jugador sigue nadando y sólo se apagan las puertas por carril | Sustituir la línea 41 por `true` (o la bandera), o conservar un lector mínimo de esa clave. La radio no usa ViceExtFeatures |
| Clips `swim_*` del **ped.ifp del mod** + límites `NUMANIMATIONS=512` y `NUMANIMBLOCKS=40` (config.h:72-79, **sin guarda**) | AnimationId.h, AnimManager.*, config.h | dura de datos: con 450 animaciones el ped.ifp del mod (272 clips) no cabe y el nado se queda sin clips en silencio (AnimManager.cpp:1475-1493 salta el .ifp) | Conservar el ped.ifp del mod y un `NUMANIMATIONS` con margen |
| Orden del enum `AssocGroupId` | AnimManager.h:72-78: `ASSOCGRP_DEAGLE/STEYR/ROCKET` (armas del mod, SIN-GUARDA) van **antes** de `ASSOCGRP_PLAYERSWIM`; la tabla AnimManager.cpp:1111-1117 sigue el mismo orden | compilación y datos: si se quitan las armas del mod sin quitar enum y filas a la vez, "playerswim" queda desalineado | Quitar enum y filas juntos. CROUCH/CLIMB van detrás y no afectan |
| Firma `particleProduceFootSplash(..., CRGBA)` | Ped.cpp:3437-3446, Ped.h:1221 (también la usa `CPed::PlayFootSteps`, Ped.cpp:3619-3626, SIN-GUARDA: salpicadura de charco con color) | compilación | Si se restaura Ped.cpp a vanilla, mantener esta firma |
| Sonidos `SOUND_NADO_BRAZO_*` | soundlist.h:206-211, AudioLogic.cpp:4533-4561 (sin guarda, ya contados como NADO) | compilación | Conservar |
| Clamp de agua de la cámara ClassicAXIS | core/Cam.cpp:1959-1973 (bajo `VICEEXT_AIM_CLASSICAXIS`) | **blanda**: impide que la cámara se meta bajo el agua mientras se nada | Conservarlo o reimplementarlo como parte del nado |
| `CanStrafeOrMouseControl` con cámara de ratón | peds/Ped.cpp:4254-4268 (SIN-GUARDA, `FREE_CAM`) | blanda: según su comentario, hace que el nado vaya hacia donde mira la cámara con ratón | Revisar al reimplementar la cámara |
| Coordinación con features viejas | PlayerPed.cpp:158-163 (en `ViceExtIsAiming`, CROUCH), 1326-1329 y 1519-1522 (ClassicAXIS), 3593-3595, 3755-3761 y 4130-4137 (CLIMB); y las consultas inversas del código viejo al carril NADO (Cam.cpp:652-656, 4292-4298; PlayerPed.cpp:5128, 5328, 6047; Script6.cpp:1008) | coordinación: se van con el código viejo sin romper el nado | La feature nueva de apuntado, agachado o escalada tiene que **respetar** el carril NADO (nadando no se apunta, no se agacha y no se escala) |
| Traza SWIM2 (sólo web) | PlayerPed.cpp:4019-4026 usa `ViceExtBlendWeight` (AIM_WALK), `odCrouched` (CROUCH/AIM_WALK), `ViceExtIsAiming` (CROUCH) y `CCamera::s_viceExtAimLawActive` (Camera.h, SIN-GUARDA) | compilación web | Al quitar AIM_WALK, CROUCH o ClassicAXIS, adaptar o quitar esta traza |
| Sonda `PEDAT` del arnés | PlayerPed.cpp:5684-5797, **bajo `VICEEXT_CROUCH`** (DIAGNOSTICO) | tooling: el escenario `nado` (`navegarAlAgua`: campos `mar`, `dist`) y el escenario `carga` la necesitan | Sacarla del bloque CROUCH (a NADO o INFRA) **antes** de borrar el agachado |

Lo que el nado usa del vanilla puro (sin riesgo): `CWaterLevel::GetWaterLevel`, `CTimeCycle`, `CParticle`/`CParticleObject`, `SOUND_SPLASH`, `bDrownsInWater`, `bIsInWater` y `m_vecLastAboveWaterCamPosition`. `renderer/WaterLevel.cpp` **no** es del nado: es un arreglo on-demand (INFRA-WEB) para cuando falta `waterpro.dat`.

### 2.4 Trazas del nado (DIAGNOSTICO en:NADO, 257 líneas, opcionales)

- **Etiquetas:** `SWIMNAT`, `VICEEXT swim enter/exit/move/punos/arma-devuelta/trepa-off`, `SWIM2 move/exit`, `SWIMIDLE`, `SWIMFASE`, `SWIMEF`, `SWIMFOV`, `SWIMERR` (PlayerPed.cpp); `SWIMCAM` (Cam.cpp); `SWIMHUD` (Hud.cpp:425-437); `PEDCLAIM` (PedArbiter.cpp).
- **Las consume el tooling:** `check-served-build.sh` tiene 19 marcas de nado más `camz=`. Los bloques H, R14, R17 y R19 de `viceext-log-check.py` y el escenario `nado` de `harness/esquemas.mjs` exigen `SWIM2` y `SWIMCAM` (§9).

### 2.5 Avisos (no tocar ahora, sólo saberlo)

- `ViceExtSwimControl` ignora lo que devuelve `ViceExtPedClaim`.
- `s_odSwimMedido` nunca se actualiza (el filtro de la fuerza de la salpicadura está roto). `s_odEstela` no se usa.
- Los 4 IDs `ANIM_STD_SWIM_*` son los primeros del bloque añadido a AnimationId.h: quitar CLIMB/CROUCH (que van detrás) no los desplaza.

---

## 3. RADIO: lista cerrada y dependencias

### 3.1 Piezas

| Fichero | Líneas RADIO (+) | Rangos (líneas nuevas) | Borradas vanilla (oN) | Traza interna (DIAG, opcional) |
|---|---:|---|---|---|
| audio/MusicManager.cpp | 93 | 10-12, 47-51, 574, 576-578, 580, 595, 597-599, 601, 614, 616-618, 620, 700-704, 1379-1409, 1423-1425, 1458-1475, 1479, 1493, 1512, 1519-1528 | 1312, 1345, 1367, 1374 | 29: 575, 596, 615, 1410-1422, 1480-1492 |
| core/config.h | 1 | 473 | — | — |

| Pieza | fichero:líneas | Guarda | Qué hace |
|---|---|---|---|
| Bandera | core/config.h:473 | — | `#define VICEEXT_RADIO_ICONS` |
| Include | audio/MusicManager.cpp:10-12 | RADIO_ICONS | `TxdStore.h` |
| Carga de logos | MusicManager.cpp:1379-1409, 1423-1424 (en `DisplayRadioStationName`) | RADIO_ICONS | Tablas `odRadioNames[11]` (wildstyle, flash, kchat, fever, vrock, vcpr, espantoso, emotion, wave103, mp3, none) y sus máscaras `…A`. Carga perezosa: `FindTxdSlot("radio")`, si no existe `AddTxdSlot` + `LoadTxd("MODELS/RADIO.TXD")`, y después `SetTexture` ×11 |
| Línea vanilla desplazada | MusicManager.cpp:1425 (o1312) | sin guarda | `wchar* string = nil;` |
| Emisora → icono | MusicManager.cpp:1458-1468 | RADIO_ICONS (+ `RADIO_OFF_TEXT` vanilla) | `odStation` 0..8, USERTRACK=9 (mp3), RADIO_OFF=10 (none) |
| Altura del texto | MusicManager.cpp:1469-1475 | RADIO_ICONS / `#else` | `odTextY` = 68 con icono, 22 sin él |
| Texto y sombra | MusicManager.cpp:1512, 1519 (sustituyen o1367, o1374) | **sin guarda** (compila con el `#else`) | El nombre de la emisora se pinta en `odTextY` |
| Dibujo del icono | MusicManager.cpp:1520-1528 | RADIO_ICONS | Sprite de 64×56 sobre el texto |
| Retune web | MusicManager.cpp:47-51, 574-580, 595-601, 614-620, 700-704 | `__EMSCRIPTEN__` | El cambio de emisora se mide en ms reales (600 ms) para que el lag no acumule vueltas del dial. No es del mod, pero es parte de "cambiar de emisora" |
| Datos | `gta_vc_browser/streamed/models/radio.txd` (de `mods/extended/GameFiles/ViceExtended/models/radio.txd`) y la entrada `models/radio.txd` de `web/public/manifest.json`. **No** está en `bootseed.list` | — | 11 logos y 11 máscaras. Fichero **del mod** |

INFRA-WEB relacionada (se queda):
- MusicManager.cpp:140-141, 189-193, 263-265 y 1263: sin división por longitud 0.
- oal/stream.cpp:1281-1308: un `.adf` puede ser MP3 plano.
- glfw.cpp:1697-1707: sin auto-repeat de teclas, que provocaba "taps fantasma de radio".
- `web/ondemand.js:305-357, 1101-1135`: precarga de emisoras.

Trazas de radio (DIAGNOSTICO, 29 líneas internas más ~16 alrededor):
- `RADIOLOGOS` (1410-1422);
- `P4 kind=radio2` (1480-1492);
- `RETUNE` (26-38, 575, 596, 615, 1213-1215);
- printf `[web] radio lens` (196-203).

### 3.2 Dependencias de la RADIO

1. **`TXDSTORESIZE = 1500`** (config.h:27, límite del mod **sin guarda**; vanilla 1385). `AddTxdSlot("radio")` necesita un slot libre. Si se quitan los TXD del mod y se vuelve a 1385, comprobar que sobra hueco. Si el límite queda justo, los iconos desaparecen sin error.
2. **`radio.txd` es un fichero del mod.** Tiene que seguir en `streamed/models/` y en el manifiesto aunque se limpien los demás assets de Vice Extended.
3. **Fragilidad propia, no dependencia del código viejo.** `odRadioInit = true` se marca aunque `LoadTxd` falle. Si la capa on-demand aplaza el fichero en partida, no hay iconos en toda la sesión.
4. **Tooling:**
   - `check-served-build.sh:165` busca `P4 kind=radio2 schema=1` en el wasm;
   - `viceext-log-check.py` P4.11 (:2973, :3017, :4001-4025) lee `emisora`/`icono` y da FALLO si `icono=-1`;
   - `web/lib/index.js:208` prioriza `RETUNE`/`RADIOTRACK`.

   Si se desmonta el andamiaje P4, hay que mantener esa línea con el mismo formato o cambiar el tooling a la vez.
5. **No depende** de PedArbiter, ViceExtFeatures, `CCamera::s_viceExt*`, Hud.cpp, Font.cpp ni de ninguna otra `VICEEXT_*`. El dibujo usa sólo API vanilla (CTxdStore, CSprite2d, CFont). La llamada a `DisplayRadioStationName()` en main.cpp:1824 es vanilla.
6. Relacionado pero **no** es RADIO:
   - Cam.cpp:2693-2697 (SIN-GUARDA): el botón de radio del mando ya no recentra la cámara del coche.
   - ControllerConfig.cpp:299-302: 'R' = `PED_RELOAD`, la misma tecla que `VEHICLE_CHANGE_RADIO_STATION`.

   Quitarlos no afecta a los iconos.

---

## 4. Ficheros nuevos de los agentes: quién los usa

### 4.1 `core/PedArbiter.h/.cpp` (árbitro de carriles)

- **API:** `ViceExtPedOwner/Owns/Claim/Release/OnRelease/ResetAll`.
- **Capacidades:** `PEDCAP_MOVIMIENTO/POSTURA/APUNTAR/CAMARA/ARMA`.
- **Carriles:** `PEDLANE_MOTOR`, `PEDLANE_NADO`, `PEDLANE_AGACHADO_MOD` (sin uso), `PEDLANE_LEY` (1ª persona).
- **Lógica:** bajo `VICEEXT_PEDARBITER`. La cabecera va sin guarda. `Claim(NADO)` consulta `ViceExtFeatures::On("EnableSwimming")`.

| Usuario | Guarda | Para qué | ¿Nado? |
|---|---|---|---|
| PlayerPed.cpp:3773-3776 / 3704-3707 | SWIMMING | claim/release al entrar y salir del agua | **NADO** |
| PlayerPed.cpp:1101, 2668 | SWIMMING | arma y puños nadando | **NADO** |
| Ped.cpp:1985 (SWIMMING), 2903 (sin guarda) | — | flotación, ahogo y caída | **NADO** |
| Hud.cpp:455 | sin guarda | cruz oculta nadando | **NADO** |
| Game.cpp:989 | sin guarda | `ViceExtPedResetAll()` en cada carga | infra del árbitro (lo necesita el NADO) |
| PlayerPed.cpp:159 (CROUCH+SWIM), 1327 (CLASSICAXIS+SWIM), 5128, 5328 (CROUCH), 6047 (sin guarda) | — | las features viejas se apagan nadando | viejo → nado (se va con lo viejo) |
| Cam.cpp:653-654 (PEDARBITER), 4292-4293 (sin guarda) | — | cámara de hombro y FOV ClassicAXIS se apagan nadando | viejo → nado |
| Script6.cpp:24, 1008 | sin guarda | `IS_PLAYER_TARGETTING_CHAR` ignora el apuntado nadando (+ arreglo SilentPatch 1610) | viejo → nado (SIN-GUARDA) |
| Camera.cpp:36, 974, 1460-1477, 2650 | FIRST_PERSON | carril `PEDLANE_LEY` sobre `PEDCAP_CAMARA` | MOD-VIEJO (1ª persona) |
| **RADIO** | — | ninguna referencia | no |

### 4.2 `core/ViceExtFeatures.h/.cpp` (lector de `ViceExtended/features.ini`)

- **API:** `Load()`, `On(clave, portDefault)`, `Waypoint(canal, def)` y `WasRead()` (sin uso).
- **Claves:** 17. 7 tienen override del jugador fijado a 1: EnableClimbing, RemoveMoneyZerosInTheHud, VehiclesDontCatchFireWhenTurningOver, HealthRegenerationUpToHalf, WantedStarsHideOnScreenWhenThereIsNoSearch, MilitaryFiringFromTankAtPlayer y StandardCarsUseTurnSignals. **EnableSwimming no tiene override.**
- **Compilación:** la cabecera va **sin guarda**, pero el .cpp sólo se compila con `VICEEXT_FEATURES_INI`. Todos los llamadores sin guarda dejan de enlazar si se quita.

| Usuario | Clave | Guarda real |
|---|---|---|
| Game.cpp:191 | `Load()` | FEATURES_INI |
| **PedArbiter.cpp:41** | **EnableSwimming** | PEDARBITER → **NADO** |
| Radar.cpp:1108-1110 | WaypointColorRGB | sólo MAP_ENHANCEMENTS (sin VICEEXT) |
| World.cpp:1948 | EnableDistantLights | sin guarda |
| PedFight.cpp:3890; PlayerPed.cpp:3305 | PlayerDoesntBounceAwayFromMovingCar | sin guarda |
| PlayerPed.cpp:2719 | RocketLauncherThirdPersonAiming | sin guarda |
| PlayerPed.cpp:3292 | EnableClimbing | CLIMB |
| PlayerPed.cpp:6323 | HealthRegenerationUpToHalf | sin guarda |
| Hud.cpp:816 / 1066 | RemoveMoneyZerosInTheHud / WantedStarsHide… | sin guarda |
| SpecialFX.cpp:393, 451 | DisableBulletTraces | sin guarda |
| Automobile.cpp:1842, 1856 / 3913 / 4770 | StandardCarsUseTurnSignals / MilitaryFiringFromTank… / VehiclesDontCatchFire… | TURN_SIGNALS / sin guarda / sin guarda |
| Weapon.cpp:299 | RecoilWhenFiring | RECOIL |
| **RADIO** | — | ninguna |

---

## 5. MOD-VIEJO por bandera

### 5.1 Qué hace cada bandera de `config.h`

Todas están **definidas (activas)**. Su valor y el nº de línea están en la tabla 5.2. "Sin bloques propios" significa que la bandera existe pero la lógica va **sin guarda** (normalmente por `ViceExtFeatures::On`).

| Bandera (config.h) | Qué hace | Observación |
|---|---|---|
| `VICEEXT_LIMITS` (:51), `VICEEXT_SPAWN_RADIUS_MULT 1.5f` (:52) | NUMVEHICLES 110→130; radio de spawn/despawn de peds y coches ×1,5 | Los otros límites del mod (MAX_CDIMAGES, MODELINFOSIZE…) están SIN-GUARDA |
| `VICEEXT_SWIMMING` (:390) | NADO | se protege |
| `VICEEXT_RECOIL` (:391) | Retroceso por arma con cola de patadas que consume la cámara (Weapon.cpp, Cam.cpp) | ~40 % de su código es traza |
| `VICEEXT_NO_CAR_BOUNCE` (:392) | — | sin bloques propios: PedFight.cpp:3883-3894 y PlayerPed.cpp:3300-3319 van SIN-GUARDA |
| `VICEEXT_SPRINT_HEAVY` (:393) | `CanSprintWithCurrentWeapon()`=true | El cambio RUNFAST→`sprint_*` de AnimManager.cpp va SIN-GUARDA |
| `VICEEXT_POLICE_BIKE_LIGHTS` (:394) | Sirena y coronas de la moto policial 6507 | Bike.cpp, Vehicle.cpp, Pad.cpp |
| `VICEEXT_HIDE_COPS` (:395) | Esconderse de la policía (`CWanted::UpdateHiding`, CopPed) | `WantedCopSeesPlayer` y las trazas WANTED van SIN guarda (no compila sin la bandera) |
| `VICEEXT_FIRST_PERSON` (:396) | Conmutador de 1ª persona con V, IK de cabeza, INI `[FirstPerson]`, opción de menú | Usa el carril LEY de PedArbiter |
| `VICEEXT_DRIVEBY_WIDE` (:397) | Pistolas en drive-by, cadencia por arma | Reescritura de los 3 `DoDriveByShootings` SIN-GUARDA |
| `VICEEXT_WEAPON_SIGHTS` (:398) | Miras por arma (`weaponSights.txd`, columna 27 de weapon.dat) | Hud.cpp, WeaponInfo |
| `VICEEXT_HINT_KEYS` (:399) | Iconos de tecla en los avisos (`pcbtns.txd`, `~K<vk>~`) | Font, Messages, ControllerConfig |
| `VICEEXT_AUTOSAVE` (:400) | Autoguardado al pasar una misión (ranura 9) | Las ranuras hasta `SAVESLOT_9`, `SLOT_COUNT` 9 y el menú sin ranura 8 van SIN-GUARDA |
| `VICEEXT_SAVE_ANYWHERE` (:401) | Guardar desde la pausa | — |
| `VICEEXT_MANUAL_RELOAD` (:402) | Recarga con R | La acción `PED_RELOAD` va SIN-GUARDA en ControllerConfig |
| `VICEEXT_GAS_TANK` (:403) | Disparo al dummy `petrolcap` → explota | Weapon.cpp |
| `VICEEXT_SHOTGUN_AIM` (:404) | Las escopetas apuntan | Macros neutras `ViceExtCanAim/AimWithArm` sin guarda |
| `VICEEXT_CROUCH` (:405), `VICEEXT_CROUCH_CAM_DROP 0.55f` (:424) | Agachado propio (PED_DUCK) con clips del mod, rueda, pose y cámara baja | La sonda PEDAT del arnés vive aquí. `ViceExtCrouchShooting` se llama SIN guarda desde PedFight |
| `VICEEXT_CLIMB` (:425) | Escalada con salto (clips `CLIMB_*`) | IDs y grupo de anim SIN-GUARDA |
| `VICEEXT_AIM_WALK` (:427) | Apuntando sólo se camina | — |
| `VICEEXT_ROCKET_3RD_PERSON` (:429) | Sólo el predicado | El uso real (PlayerPed.cpp:2713-2724) va SIN-GUARDA |
| `VICEEXT_AIM_CLASSICAXIS` (:447) | Ley de apuntado de ClassicAXIS (lo guardado) | El grueso va SIN-GUARDA: `Process_AimWeapon*`, `ViceExtProcessPlayerPedControl`, Camera init/transition, cruz del HUD |
| `VICEEXT_BREAKABLE_LIGHTS` (:448) | Disparo a un faro → luz rota | — |
| `VICEEXT_TURN_SIGNALS` (:449) | Intermitentes | El comentario dice "APAGADO" pero está definida. `vc-test` la compila apagada |
| `VICEEXT_BIKE_EXIT_SIDE` (:450) | Salir de la moto por el lado del giro | PedAI.cpp |
| `VICEEXT_AIM_BLADE 0.70f` (:451) | Ángulo de la pala de apuntado | Usada como valor: PlayerPed.cpp:1525 (ClassicAXIS) y 6193 (SIN-GUARDA) |
| `VICEEXT_FIX_SILENTPATCH` (:453) | Sirena FBICAR, sin casquillo en Python/Sniper/Laser, código muerto en VehicleModelInfo | Muchos arreglos SilentPatch van SIN-GUARDA (Coronas, Shadows, MBlur, Sprite2d, outro, loadbar). La rama `#else` de AudioLogic borró el `if(FBIRANCH)` vanilla |
| `VICEEXT_FIX_WFP` (:454) | Bandas negras para texturas fullscreen que no son 4:3 | Sprite2d |
| `VICEEXT_FIX_FV` (:455), `VICEEXT_FIX_FV_ROTOR` (:456) | Fases por tiempo en vez de FrameCounter | **Trampa:** borran vanilla sin `#else` (RC Raider/Goblin, heli IA, rotor) |
| `VICEEXT_POLICE_BIKE` (:457) | La moto policial aparece con nivel de búsqueda y lleva policía | — |
| `VICEEXT_NO_WHEEL_PIERCE` (:458) | El disparo en la rueda no atraviesa | World.cpp |
| `VICEEXT_SKIP_PHONE_CALL` (:459) | Saltar la llamada de misión con ENTER | Le quita ENTER a `VEHICLE_ENTER_EXIT` (SIN-GUARDA) |
| `VICEEXT_MOD_SCRIPT` (:460) | Opcodes 0x0FA0-0x0FA9 del main.scm del mod | Script8.cpp |
| `VICEEXT_PARTICLE_NAMES` (:461) | `particle.cfg` resuelto por nombre | Los 5 tipos nuevos de ParticleType.h van SIN-GUARDA |
| `VICEEXT_LOD_EXTRA 20.0f` (:466) | +20 m de LOD | Usada como valor **sin `#ifdef`** (SimpleModelInfo.cpp) |
| `VICEEXT_MONEY_NO_ZEROS` (:470) | — | sin bloques propios: Hud.cpp:813-820 va SIN-GUARDA por features.ini |
| `VICEEXT_PEDARBITER` (:472) | Árbitro de carriles | **dependencia del NADO: conservar** |
| `VICEEXT_RADIO_ICONS` (:473) | RADIO | se protege |
| `VICEEXT_EXTRA_CHEATS` (:474) | Trucos del mod (COWBOY69, AEZAKMI, BIGSMOKE…) | — |
| `VICEEXT_CAR_DEATH` (:475) | Los ocupantes NPC mueren | Automobile.cpp |
| `VICEEXT_CAR_LIGHTS` (:476) | Sólo traza | — |
| `VICEEXT_ALT_SIT` (:477) | Andar con LALT (`CPad::GetPedWalk`) | La acción `PED_WALK` va SIN-GUARDA |
| `VICEEXT_GANG_AIM` (:482) | Precisión de peds y policías | — |
| `VICEEXT_MOON` (:483) | Luna móvil | Clouds.cpp |
| `VICEEXT_MAPDATA` (:486) | Sólo traza (`P4 kind=worlddata`) | — |
| `VICEEXT_POLISH` (:487) | Explosiones sin línea de visión no dañan, partículas de sangre y polvo, conductores huyen | — |
| `VICEEXT_WHEEL_DETACH` (:488) | La rueda reventada se desprende | — |
| `VICEEXT_FEATURES_INI` (:492) | Lector de features.ini | **El NADO depende de 1 clave** (§4.2) |
| `VICEEXT_NO_FLIP_BURN` (:493), `VICEEXT_HEALTH_REGEN` (:494), `VICEEXT_HIDE_STARS` (:495), `VICEEXT_NO_JUMP_FROM_CAR` (:496), `VICEEXT_TANK_FIRE` (:498) | — | sin bloques propios: lógica SIN-GUARDA por `ViceExtFeatures::On` (Automobile.cpp:4767-4779, PlayerPed.cpp:6317-6342, Hud.cpp:1063-1081, PlayerPed.cpp:3300-3319, Automobile.cpp:3910-3942) |
| `VICEEXT_STD_TURN_SIGNALS` (:497) | Intermitentes en los coches de serie | — |
| `VICEEXT_GPS_COLOR` (:499), `_R/_G/_B` 255/0/255 (:500-502) | Sólo la traza va guardada | El color se aplica SIN guarda en Radar.cpp:1106-1128 (el features.ini servido trae 180,24,24) |
| `VICEEXT_CAR_ROTATE_WHEEL` (:505) | El volante gira | — |
| `VICEEXT_CAR_WHEEL_KEEP` (:506) | Las ruedas conservan el giro al salir | — |
| `VICEEXT_CAR_FOUR_COLORS` (:507) | Sólo campos sin uso | — |
| `VICEEXT_GAS_STATION_BLOW` (:508) | Choque con un surtidor → explosión | — |
| `VICEEXT_PROJECTILE_BLOW` (:509) | Disparar a un proyectil lo hace explotar | — |
| `VICEEXT_DISTANT_LIGHTS` (:510) | Luces lejanas (Building/Dummy) | La llamada en World.cpp va SIN guarda (no compila sin la bandera) |
| `VICEEXT_COP_COVER` (:513) | El policía se agacha antes de cubrirse | — |
| `VICEEXT_HELI_HEALTH` (:514) | El heli policial cae con 400 de daño | — |
| `VICEEXT_AIM_SENSITIVITY` (:515) | Factores de sensibilidad (INI `[Controller]`) | Llamadas sin guarda en Cam.cpp (neutras: devuelven 1,0) |
| `VICEEXT_CHEAT_EFFECTS` (:516) | BIGHEADS, RCROCKET y AIRWAYS con efecto | — |
| `VICEEXT_PHOTO_SAVE` (:517) | Sólo una traza en Weapon.cpp | No hay funcionalidad en `src/` |

Otras macros `VICEEXT_*` definidas fuera de config.h:

- **PlayerPed.cpp:**
  - `VICEEXT_ENGINE_DUCK_KEY()` (:79-81) y `VICEEXT_RELOAD_NO()` (:3442-3450);
  - `VICEEXT_CLIMB_MIN/MAX/MIN_MS/MAX_MS/REACH` (:4065-4069);
  - `VICEEXT_CROUCH_*` y `VICEEXT_WALK_*` (:4298-5104);
  - `VICEEXT_P4_EMIT` (:1568, traza).
- **Otros ficheros:**
  - Ped.cpp: `VICEEXT_DRIVEBY_REV` (:5163, traza);
  - Wanted.cpp: `VICEEXT_HIDE_GRACE_MS/STAR_MS/SWEEP_MS/SEEN_MS/REV` (:42-80);
  - Pad.cpp: `VICEEXT_CHEATCMP` (:1831);
  - Camera.cpp: `VICEEXT_1P_TOGGLE_JUSTDOWN` (:261);
  - main.cpp: `VICEEXT_LOADBAR_BORDER_X/Y` (:677-681, SilentPatch, sin guarda);
  - Script8.cpp: `VICEEXT_MODSCRIPT_SLOTS` (:633) y `VICEEXT_MODSCRIPT_P4_EMIT` (:643, traza);
  - Automobile.cpp: `VICEEXT_CAR_ROTATE_WHEEL_NOMBRE/GAIN` (:2064-2065) y `VICEEXT_P4_EMIT` (:2002, traza);
  - Weapon.cpp: `VICEEXT_GAS_TANK_RADIUS_DUMMY/BOX` (:1431-1432) y `VICEEXT_LIGHT_HIT_RADIUS` (:1542);
  - ParticleMgr.cpp: `VICEEXT_P4_EMIT` (:133, traza).

### 5.2 Sitios por bandera (líneas MOD-VIEJO, ya sin trazas ni NADO/RADIO)

| Bandera | Valor (config.h:línea) | Líneas MOD-VIEJO (de ellas en rama `#else`/`#ifndef`) | Sitios `fichero:líneas` (rangos de líneas nuevas, huecos ≤5 fusionados) |
|---|---|---:|---|
| VICEEXT_CROUCH | `(definida)` (:405) | 1303 (3) | peds/PlayerPed.cpp:47-54, 72-83, 137-157, 164-190, 585-597, 1055-1058, 1232-1235, 1396-1422, 1486-1487, 1510-1517, 2211-2218, 2678, 2686, 4227-4837, 4857-4892, 4912-4953, 4978-4979, 5074-5159, 5169-5309, 5325-5343, 5353-5375, 5404-5423, 5441-5467, 5534, 5662-5683, 5798, 6565-6570, 6606-6609, 6791-6798; peds/Ped.cpp:1506-1515, 1589-1652, 1660-1684, 1702, 1717, 1745-1748; core/Cam.cpp:1766-1784, 6224-6234; peds/PlayerPed.h:145-154; core/config.h:405 |
| VICEEXT_AIM_CLASSICAXIS | `(definida)` (:447) | 710 (72) | peds/PlayerPed.cpp:43-46, 306-308, 558-560, 900-919, 944-959, 966, 975-976, 1298-1395, 1423-1425, 1433-1509, 1518-1566, 2184-2194, 2219-2233, 2259-2261, 2540-2552, 2845-2859, 2898-2907, 6384-6386; renderer/Hud.cpp:465-491, 646-712, 758-786; core/Cam.cpp:1917-1973, 2006-2036, 2079-2086, 2096, 4544-4554; peds/Ped.cpp:1536-1551, 1579-1584, 1647-1659, 1703-1716, 1758-1761, 9732-9739, 9823-9839, 9858-9869; core/re3.cpp:511-526, 670-683; core/Camera.cpp:1549, 1558-1574; peds/PedFight.cpp:191-206; core/config.h:434-447; peds/PlayerPed.h:128-138 |
| VICEEXT_RECOIL | `(definida)` (:391) | 462 | weapons/Weapon.cpp:124-167, 174-197, 248-329, 339-344, 351-354, 365-388, 418-419, 465-551, 563, 868-874; core/Cam.cpp:84-121, 131-157, 184, 197, 237-248, 257-262, 1377-1379, 1719-1724, 1743-1745, 1857-1859, 1879-1885, 3259-3261, 3300-3302, 3324-3328, 3390-3392, 3421-3423, 3442-3446, 3578-3580, 3638-3640, 3647-3651, 3774-3776, 3872-3874, 3894-3898, 4021-4023, 4499-4509, 4646-4648, 4707-4717, 6217-6219, 6318-6320, 6345-6348, 6372-6376; weapons/Weapon.h:37-50, 105-109; core/config.h:391 |
| VICEEXT_DISTANT_LIGHTS | `(definida)` (:510) | 382 | buildings/Building.cpp:32-220; entities/Dummy.cpp:62-245; buildings/Building.h:16-18; core/config.h:510-512; entities/Dummy.h:14-16 |
| VICEEXT_HINT_KEYS | `(definida)` (:399) | 350 | renderer/Font.cpp:12-15, 310-317, 478-490, 514-583, 780-793, 1596-1615, 1703-1710, 1763-1768, 1894-1899; core/ControllerConfig.cpp:2851-2965; text/Messages.cpp:13-16, 516-533, 571-610; renderer/Font.h:147-162; core/ControllerConfig.h:230-236; renderer/Hud.cpp:2201-2208; core/config.h:399 |
| VICEEXT_FEATURES_INI | `(definida)` (:492) | 262 | core/ViceExtFeatures.cpp:1-213, 245-284; core/Game.cpp:187-192; core/config.h:489-492 |
| VICEEXT_HIDE_COPS | `(definida)` (:395) | 214 | core/Wanted.cpp:38-58, 81, 255-262, 272, 648-805; peds/CopPed.cpp:475-503; core/Wanted.h:41-59; core/config.h:395 |
| VICEEXT_FIRST_PERSON | `(definida)` (:396) | 173 (5) | core/Camera.cpp:229-266, 971-976, 1405-1442, 1450-1485, 1506-1507, 2646-2652; core/Cam.cpp:3791-3817, 3825; core/MenuScreensCustom.cpp:52-57, 425; core/ControllerConfig.cpp:702-706; core/re3.cpp:544-548; core/config.h:396 |
| VICEEXT_WEAPON_SIGHTS | `(definida)` (:398) | 170 (17) | renderer/Hud.cpp:35-37, 229-315, 358-372, 510-515, 523-528, 538-543, 551-556, 2176-2191, 2199-2200, 2471-2475; weapons/WeaponInfo.cpp:175-177, 214-227, 253-257, 288-290; weapons/WeaponInfo.h:65-69; core/config.h:398 |
| VICEEXT_CLIMB | `(definida)` (:425) | 166 | peds/PlayerPed.cpp:3287-3295, 4048-4129, 4138-4216, 4224-4226; peds/PlayerPed.h:155-157; core/config.h:425-426 |
| VICEEXT_MOD_SCRIPT | `(definida)` (:460) | 160 | control/Script8.cpp:31-34, 623-720, 727-773; control/ScriptCommands.h:1477-1491; control/Script.cpp:1095-1100; control/Script.h:343-345; core/config.h:460 |
| VICEEXT_PARTICLE_NAMES | `(definida)` (:461) | 152 (3) | renderer/ParticleMgr.cpp:13-135, 156, 213-247, 254; core/config.h:461 |
| VICEEXT_FIX_FV | `(definida)` (:455) | 111 (24) | control/CarCtrl.cpp:115-126, 149-153, 213, 235, 264, 293, 1053-1062, 2787-2799; vehicles/Automobile.cpp:114-116, 301-308, 1467-1477, 1656, 2262-2266, 2568; vehicles/Bike.cpp:105-107, 230-237, 1404-1408, 1837; vehicles/Heli.cpp:71-73, 130-137, 325; core/World.cpp:2163-2171; vehicles/Automobile.h:80-82; vehicles/Bike.h:91-93; vehicles/Heli.h:64-66; core/config.h:455 |
| VICEEXT_GAS_TANK | `(definida)` (:403) | 108 | weapons/Weapon.cpp:1404-1476, 1490-1513, 1522-1525, 1593-1594, 1603, 2730, 3450-3456; core/config.h:403 |
| VICEEXT_STD_TURN_SIGNALS | `(definida)` (:497) | 101 | vehicles/Automobile.cpp:1790-1911, 1923-1924; core/config.h:497 |
| VICEEXT_AUTOSAVE | `(definida)` (:400) | 90 (2) | core/Frontend.cpp:1143-1156, 4889-4895, 4903-4908; control/Script.cpp:21, 71-75, 1608-1622, 1636; core/MenuScreensCustom.cpp:457-468; control/Script3.cpp:2336-2342; control/Script.h:621-626; save/GenericGameStorage.h:38-43; control/Script8.cpp:721-725; save/GenericGameStorage.cpp:55-59; core/config.h:400 |
| VICEEXT_MANUAL_RELOAD | `(definida)` (:402) | 90 | peds/PlayerPed.cpp:3416, 3427-3436, 3452-3469, 3480-3527, 3536, 3553-3555, 6467-6478, 6511; peds/PlayerPed.h:120-122; core/config.h:402 |
| VICEEXT_AIM_WALK | `(definida)` (:427) | 89 | peds/PlayerPed.cpp:55-71, 817-855, 2202-2209, 3195-3207, 3246-3255, 3276-3278; core/config.h:427-428; peds/PlayerPed.h:127, 140 |
| VICEEXT_SHOTGUN_AIM | `(definida)` (:404) | 83 (9) | peds/PlayerPed.cpp:84-132, 2701, 2830, 2872, 2918; peds/PlayerPed.h:9-26; core/Cam.cpp:665-675; core/config.h:404 |
| VICEEXT_BREAKABLE_LIGHTS | `(definida)` (:448) | 80 | weapons/Weapon.cpp:1526-1568, 1577-1602, 2317-2320, 2726-2729, 3451-3454; core/config.h:448 |
| VICEEXT_EXTRA_CHEATS | `(definida)` (:474) | 78 | core/Pad.cpp:805, 818-849, 1830-1888; core/config.h:474 |
| VICEEXT_POLICE_BIKE_LIGHTS | `(definida)` (:394) | 76 | vehicles/Bike.cpp:1116-1135, 1310-1317, 1349-1381; core/Pad.cpp:434-441; vehicles/Vehicle.cpp:1654-1659; core/config.h:394 |
| VICEEXT_SAVE_ANYWHERE | `(definida)` (:401) | 67 | save/GenericGameStorage.cpp:43-45, 60-86; core/Frontend.cpp:4854-4863, 4910-4919, 4930-4931, 4939; core/MenuScreensCustom.cpp:640-647; save/GenericGameStorage.h:33-37; core/config.h:401 |
| VICEEXT_AIM_SENSITIVITY | `(definida)` (:515) | 60 (12) | core/Camera.cpp:153-203; core/re3.cpp:527-543, 658-669; core/config.h:515 |
| VICEEXT_CAR_ROTATE_WHEEL | `(definida)` (:505) | 48 | vehicles/Automobile.cpp:2050-2082, 2094-2095, 3344-3346; vehicles/Automobile.h:144-150; core/config.h:503-505 |
| VICEEXT_ROCKET_3RD_PERSON | `(definida)` (:429) | 46 (18) | peds/PlayerPed.cpp:2620-2640, 2662-2663; weapons/Weapon.cpp:2876-2894; core/config.h:429-433 |
| VICEEXT_FIX_SILENTPATCH | `(definida)` (:453) | 44 | modelinfo/VehicleModelInfo.cpp:1175-1200; weapons/Weapon.cpp:1963-1972; audio/AudioLogic.cpp:2896-2906; core/config.h:452-453 |
| VICEEXT_ALT_SIT | `(definida)` (:477) | 44 | peds/PlayerPed.cpp:6282-6302, 6315-6316; core/Pad.cpp:2623-2636; core/config.h:477-481; core/Pad.h:225-227 |
| VICEEXT_POLISH | `(definida)` (:487) | 40 (2) | control/CarAI.cpp:15-17, 29-32, 65-80; renderer/Particle.cpp:884-889, 930-938, 947-952; core/World.cpp:2215-2220; core/config.h:487 |
| VICEEXT_CHEAT_EFFECTS | `(definida)` (:516) | 38 (3) | vehicles/Automobile.cpp:1539-1557; peds/Ped.cpp:69-71, 94-95, 5438-5448; core/Pad.cpp:1865-1866, 1875-1876; core/config.h:516 |
| VICEEXT_WHEEL_DETACH | `(definida)` (:488) | 37 | vehicles/Automobile.cpp:4931-4962, 4990-5000; core/config.h:488 |
| VICEEXT_DRIVEBY_WIDE | `(definida)` (:397) | 36 | weapons/Weapon.cpp:945-952, 969-981; peds/Ped.cpp:5190-5196, 5207-5213; core/config.h:397 |
| VICEEXT_SKIP_PHONE_CALL | `(definida)` (:459) | 27 | control/Script.cpp:896-903, 915-917; core/ControllerConfig.cpp:314-316, 549-551, 1978-1980; core/ControllerConfig.h:71-73; core/re3.cpp:335-337; core/config.h:459 |
| VICEEXT_FIX_WFP | `(definida)` (:454) | 26 | renderer/Sprite2d.cpp:95-119; core/config.h:454 |
| VICEEXT_LIMITS | `(definida)` (:51) | 24 (2) | control/CarCtrl.cpp:310-312, 612-623, 1130-1133; core/config.h:51-63; peds/Population.cpp:433-435 |
| VICEEXT_GAS_STATION_BLOW | `(definida)` (:508) | 24 | vehicles/Automobile.cpp:4782-4805; core/config.h:508 |
| VICEEXT_GANG_AIM | `(definida)` (:482) | 23 (10) | peds/CopPed.cpp:35-39, 47-51, 60-64, 90-97, 110; peds/Ped.cpp:380-384; core/config.h:482 |
| VICEEXT_MOON | `(definida)` (:483) | 21 (2) | renderer/Clouds.cpp:139-154, 172-178; core/config.h:483-485 |
| VICEEXT_POLICE_BIKE | `(definida)` (:457) | 19 | control/CarCtrl.cpp:967-973; control/CarAI.cpp:571-573, 583-584; peds/Population.cpp:842-844; vehicles/Vehicle.cpp:1632-1634; core/config.h:457 |
| VICEEXT_CROUCH_CAM_DROP | `0.55f` (:424) | 19 | core/config.h:406-424 |
| VICEEXT_CAR_DEATH | `(definida)` (:475) | 18 | vehicles/Automobile.cpp:4968-4989; core/config.h:475 |
| VICEEXT_PROJECTILE_BLOW | `(definida)` (:509) | 18 | weapons/Weapon.cpp:4038-4059; core/config.h:509 |
| VICEEXT_SPRINT_HEAVY | `(definida)` (:393) | 17 (3) | peds/PlayerPed.cpp:977-988, 2286, 3275-3279; core/config.h:393; peds/PlayerPed.h:108 |
| VICEEXT_CAR_WHEEL_KEEP | `(definida)` (:506) | 14 (3) | vehicles/Automobile.cpp:81-91, 565, 581, 601; core/config.h:506 |
| VICEEXT_TURN_SIGNALS | `(definida)` (:449) | 9 | vehicles/Automobile.h:138-143; vehicles/Automobile.cpp:1789, 1925; core/config.h:449 |
| VICEEXT_FIX_FV_ROTOR | `(definida)` (:456) | 9 | vehicles/Heli.cpp:585-592; core/config.h:456 |
| VICEEXT_COP_COVER | `(definida)` (:513) | 9 | peds/PedAI.cpp:5658-5660, 5673, 5761-5763, 5776; core/config.h:513 |
| VICEEXT_PEDARBITER | `(definida)` (:472) | 7 | core/Cam.cpp:652-656; core/config.h:471-472 |
| VICEEXT_NO_WHEEL_PIERCE | `(definida)` (:458) | 7 (3) | core/World.cpp:381-386; core/config.h:458 |
| VICEEXT_HELI_HEALTH | `(definida)` (:514) | 7 (3) | vehicles/Heli.cpp:1012-1018; core/config.h:514 |
| config-defines | (no está en config.h) | 5 | core/config.h:386-389, 518 |
| VICEEXT_BIKE_EXIT_SIDE | `(definida)` (:450) | 5 | peds/PedAI.cpp:4187-4190; core/config.h:450 |
| VICEEXT_LOD_EXTRA | `20.0f` (:466) | 5 | core/config.h:462-466 |
| VICEEXT_CAR_FOUR_COLORS | `(definida)` (:507) | 5 | vehicles/Vehicle.h:182-185; core/config.h:507 |
| VICEEXT_MONEY_NO_ZEROS | `(definida)` (:470) | 4 | core/config.h:467-470 |
| VICEEXT_GPS_COLOR | `(definida)` (:499) | 3 | core/Radar.cpp:1136, 1146; core/config.h:499 |
| VICEEXT_NO_CAR_BOUNCE | `(definida)` (:392) | 1 | core/config.h:392 |
| VICEEXT_AIM_BLADE | `0.70f` (:451) | 1 | core/config.h:451 |
| VICEEXT_CAR_LIGHTS | `(definida)` (:476) | 1 | core/config.h:476 |
| VICEEXT_MAPDATA | `(definida)` (:486) | 1 | core/config.h:486 |
| VICEEXT_NO_FLIP_BURN | `(definida)` (:493) | 1 | core/config.h:493 |
| VICEEXT_HEALTH_REGEN | `(definida)` (:494) | 1 | core/config.h:494 |
| VICEEXT_HIDE_STARS | `(definida)` (:495) | 1 | core/config.h:495 |
| VICEEXT_NO_JUMP_FROM_CAR | `(definida)` (:496) | 1 | core/config.h:496 |
| VICEEXT_TANK_FIRE | `(definida)` (:498) | 1 | core/config.h:498 |
| VICEEXT_GPS_COLOR_R | `255` (:500) | 1 | core/config.h:500 |
| VICEEXT_GPS_COLOR_G | `0` (:501) | 1 | core/config.h:501 |
| VICEEXT_GPS_COLOR_B | `255` (:502) | 1 | core/config.h:502 |
| VICEEXT_PHOTO_SAVE | `(definida)` (:517) | 1 | core/config.h:517 |
| VICEEXT_SPAWN_RADIUS_MULT | `1.5f` (:52) | 0 | (sin bloques MOD-VIEJO propios: ver notas) |
| VICEEXT_SWIMMING | `(definida)` (:390) | 0 | (sin bloques MOD-VIEJO propios: ver notas) |
| VICEEXT_RADIO_ICONS | `(definida)` (:473) | 0 | (sin bloques MOD-VIEJO propios: ver notas) |

Nota: "config-defines" son líneas de `config.h` del bloque de defines del mod agrupadas en una sola fila. Las banderas con 0 o 1 línea sólo tienen su `#define`.

---

## 6. SIN-GUARDA: cambios del mod viejo que alteran el vanilla siempre

Total: **3 867 líneas añadidas + 1 097 borradas de vanilla**. La lista completa, fila a fila, está en §6.3.

### 6.1 Los más importantes (alta importancia)

| # | Sitio | Qué altera |
|---|---|---|
| 1 | core/Cam.cpp:4141-4641 (`Process_AvoidCollisions`, `Process_AimWeaponFovLerp`, `Process_AimWeaponCrouchOffset`, `Process_AimWeapon`, miembros nuevos de CCam declarados en Camera.h:227-245) + :423-425/−o193 (`case MODE_AIMING` en `CCam::Process`) + :627-695 y 1419/1814/6262 (hombro de 0,2 m en todos los follows a pie) | Ley de apuntado ClassicAXIS completa, sin guarda (~511 líneas) |
| 2 | core/Cam.cpp:2349-2956 / −o1795-1903 (`Process_Cam_On_A_String` reescrita) + −o4782-5355 (`Process_FollowCar_SA` vaciada, FREE_CAM) | Cámara de coche "BARRIDO 1". Deja de llamar a `Cam_On_A_String_Unobscured`, `WorkOutCamHeight`, `RotCamIfInFrontCar`, `FixCamWhenObscuredByVehicle` y `AvoidTheGeometry`. El eje vertical del stick se anula en coche |
| 3 | core/Cam.cpp:1850-1853 / −o1398-1399; :1886-1895 / −o1422-1423 | Stick a pie de 1/14 a 1/20 (más lento). Clamp vertical de +45° a +60°/−89,5° |
| 4 | peds/PlayerPed.cpp:5799-6278 + 6343-6347 (`ViceExtProcessPlayerPedControl` y sus predicados) | Cada frame, antes de `CPed::ProcessControl`: `TakeControl(MODE_AIMING)`, IK, fijado de objetivo, objetivo blando de ratón… Hay un **`if` colgante** en 6183-6186 que cambia el comportamiento según la plataforma |
| 5 | peds/PlayerPed.cpp:3135-3136 / −o1500-1502 | Quita para siempre el bloqueo del stick al fijar objetivo (`MovementDisabledBecauseOfTargeting`) |
| 6 | peds/PlayerPed.cpp:6317-6342 | Regeneración de vida hasta la mitad (features.ini, activa por defecto) |
| 7 | peds/PlayerPed.cpp:3300-3319 / −o1566; peds/PedFight.cpp:3883-3894 | El jugador ya no esquiva coches ni sale despedido por el capó |
| 8 | peds/PlayerPed.cpp:2713-2724 | El lanzacohetes ya no entra en modo francotirador |
| 9 | peds/PlayerPed.cpp:2823-2827 / −o1351 | Se quita `bFreeCam` de la rotación de apuntado con ratón |
| 10 | peds/PedFight.cpp:207-248 / −o182-193; 442, 558, 580, 638, 742-745, 863, 1095 | Se elimina la pose vanilla `crouchfire` para **todos** los peds. Se llama a `ViceExtCrouchShooting()`, que sólo existe con `VICEEXT_CROUCH`: sin la bandera no enlaza |
| 11 | peds/PedFight.cpp:957-1021 | `CPed::Attack` fuerza `MODE_FOLLOWPED` al disparar con la ley ClassicAXIS |
| 12 | core/Camera.cpp:4497-4518 / −o3988-3992; :424-448; :2994-3011 | `Find3rdPersonQuickAimPitch` reemplazado (cambia el resultado siempre); Init y StartTransition de ClassicAXIS |
| 13 | core/config.h:12-44, 72-79 | Límites del mod: `MAX_CDIMAGES` 12, `MODELINFOSIZE` 6700, `TXDSTORESIZE` 1500, `COLSTORESIZE` 48, `WEAPONMODELSIZE` 47, `VEHICLEMODELSIZE` 130, `NUMANIMBLOCKS` 40, `NUMANIMATIONS` 512. **El NADO y la RADIO necesitan margen en NUMANIMATIONS y TXDSTORESIZE** |
| 14 | weapons/WeaponType.h:53-79 + WeaponInfo.*, WeaponEffects, Weapon.cpp, ProjectileInfo, Vehicle.cpp, Pickups.*, Darkel, ModelIndices.h | 9 armas nuevas del mod (48..56), `WEAPONTYPE_TOTALALLTYPES` e `IsWeaponType`: redimensionan todas las tablas por arma |
| 15 | modelinfo/ModelIndices.h:484-495; vehicles/HandlingMgr.h:116-152, 303-308; HandlingMgr.cpp; Bike.cpp:85-99; audio/AudioLogic.cpp:868-1058 | Vehículos del mod 6500-6507 (handling, motos fuera del rango contiguo, tabla de audio `MAX_CARS+8`) |
| 16 | audio/AudioSamples.h:10120-10137; AudioLogic.cpp:4565-5025; sampman_oal.cpp:124-333, 1980-2001 | 13 muestras `SFX_VICEEX_*`: cambian `TOTAL_AUDIO_SAMPLES` y obligan a servir el SDT rehecho. Además, sonidos de las armas del mod |
| 17 | vehicles/Automobile.cpp:1928-2762; modelinfo/VehicleModelInfo.cpp:199-271 | Luces de sirena por dummies `servicelights_*` (SVLIGHTS). `CreateInstance` clona ahora el subárbol completo de cada extra |
| 18 | vehicles/Automobile.cpp:3910-3942 y 4767-4779 | El tanque de la IA dispara al jugador y el coche volcado no arde (features.ini, activos por defecto) |
| 19 | modelinfo/SimpleModelInfo.cpp:118-141 | +20 m de LOD con `VICEEXT_LOD_EXTRA` usado como valor (sin el define, no compila) |
| 20 | core/World.cpp:1945-1994 | `ProcessDistantLights` sobre todos los dummies y edificios **cada frame** (sin `VICEEXT_DISTANT_LIGHTS`, no compila) |
| 21 | control/Script6.cpp:1007-1009 | El opcode `IS_PLAYER_TARGETTING_CHAR` exige cámara de apuntado (SilentPatch 1610) y consulta el carril NADO |
| 22 | core/ControllerConfig.cpp:199, 290-313; ControllerConfig.h:62, 67-70 | Entrar/salir pasa a `F` (sin ENTER). Se quita la V de cambiar cámara. Nuevas acciones `PED_TOGGLE_1RST_PERSON`/`PED_RELOAD`/`PED_WALK` en huecos del enum (cambia el formato de los controles guardados) |
| 23 | core/Frontend.cpp (1081, 1126-1160, 1459, 1539, 4822, 5150-5153, 5588-5590), Frontend.h:173, MenuScreensCustom.cpp:473-479, 526-527, save/GenericGameStorage.h:6-8, .cpp:89-93 | Ranura 9 de autoguardado: `SLOT_COUNT` 9. La ranura 8 desaparece de los menús |
| 24 | renderer/Hud.cpp:813-820, 1063-1081; core/Radar.cpp:1106-1128 | Dinero sin ceros, estrellas ocultas y color del waypoint, leídos de features.ini sin `#ifdef` |
| 25 | renderer/Hud.cpp:391, 408-418, 454, 492-521 / −o253-291; −o1720 | Cruz con la ley ClassicAXIS y para las armas del mod. Caja reducida para todos. **Borrado accidental de `m_pLastZoneName = nil;`** en `CHud::Initialise` |
| 26 | animation/AnimManager.cpp:733-769 / −o708, o722, o736; :628-631; AnimationId.h:296-329; AnimManager.h:72-86 | Hueco RUNFAST pasa a `sprint_*` (clips sólo del ped.ifp del mod), buddy de 2 a 4 clips, IDs y grupos de CLIMB/CROUCH y grupos deagle/steyr/rocket |
| 27 | core/Game.cpp:989 | `ViceExtPedResetAll()` sin guarda (lo necesita el NADO: conservar) |
| 28 | core/FileLoader.cpp:1203-1217 | Una instancia IPL con id −1 se resuelve por nombre (`bryx.ipl` del mod) |
| 29 | core/Wanted.cpp:33-37, 102-158 | `WantedCopSeesPlayer` sin guarda, que depende de `WANTED_SIGHT_COS` (bajo HIDE_COPS) |
| 30 | renderer/ParticleType.h:88-102, ParticleMgr.h:136-137, Particle.cpp:674-688 | 5 tipos de partícula nuevos (`MAX_PARTICLES` 83→88). Nadie los emite |

Arreglos "genéricos" sin guarda que **conviene conservar o decidir aparte** (no son del mod en sí):
- Arreglos SilentPatch (contados como SIN-GUARDA):
  - Coronas.cpp:418-419 (flares);
  - Shadows.cpp:2003-2005;
  - MBlur.cpp:570, 573;
  - Sprite2d.cpp (escalado de sprites de script) con Hud.cpp:1601-1614 y 1944-1952;
  - Frontend.cpp:5917-5957 (outro de 2,5 s), :2478-2498 y :941-942;
  - main.cpp:661-683 (loadbar);
  - `MUCH_SHORTER_OUTRO_SCREEN` comentado en config.h:535-537.
- Stinger.cpp:39 y 100.
- CarAI.cpp:216 y 276 (`GetLogicalFrameCounter`).
- El shader neoRimSkin (`.w` en vez de `.z` para la niebla).
- ParticleMgr.cpp:189-193 (quita el `\r` del CFG).
- Cam.cpp:3940-3945 (arreglo de crash de la 1ª persona).

### 6.2 Trampas de compilación y de comportamiento al quitar lo viejo

**Código sin guarda que depende de definiciones guardadas.** Si se apaga la bandera sin quitarlo, no compila o no enlaza:
- PedFight → `ViceExtCrouchShooting` (CROUCH).
- World.cpp → `ProcessDistantLights` (DISTANT_LIGHTS).
- Wanted.cpp → `WANTED_SIGHT_COS` (HIDE_COPS).
- SimpleModelInfo → `VICEEXT_LOD_EXTRA`.
- Radar → `VICEEXT_GPS_COLOR_*`.
- Todos los usuarios de `ViceExtFeatures::On` (FEATURES_INI).
- Cam.cpp → `CCamera::ViceExtAimSens*`.
- PlayerPed.cpp:
  - `ViceExtProcessPlayerPedControl` → miembros declarados sólo bajo AIM_WALK+CLASSICAXIS;
  - `s_odStickBlocked`, `ViceExtStrafeAiming` y `ViceExtMoveHeading` → CLASSICAXIS.
- Cam.cpp:1953-1957 (guardado) → `Process_AimWeaponCrouchOffset` (sin guarda).

**Borrados de vanilla sin `#else`.** Quitar la bandera sin restaurar el vanilla rompe el juego:
- `VICEEXT_FIX_FV`: Automobile.cpp:1470-1477 (o1419-1424, el RC no despega) y CarCtrl.cpp:2797 (o2647, el heli IA no acelera el rotor).
- `VICEEXT_FIX_FV_ROTOR`: Heli.cpp:590-592 (o573, el rotor no gira).
- `VICEEXT_FIX_SILENTPATCH`: AudioLogic.cpp o2861 (sirena del FBI Rancher).

**Macros neutras fuera de `#ifdef`.** Con la bandera apagada expanden a lo vanilla, pero al limpiar hay que devolver el texto vanilla:
- `VICEEXT_ENGINE_DUCK_KEY()`: PlayerPed.cpp:1055, 1058, 1232, 1235, 2678, 2686.
- `ViceExtCanAim()`: 2701, 2872, 2918.
- `ViceExtAimWithArm()`: 2830.
- `CanSprintWithCurrentWeapon()`: 977-983, 2286, 3275, 3279.
- Ganchos `odPhase`/`odSpawnPhase`/`odDespawnK` de FIX_FV/LIMITS en Automobile, Bike, Heli y CarCtrl.

**Problemas propios del port web:**
- Streaming.cpp llama a `odBlockingPush/Pop` sin guarda, pero sólo incluye `ondemand.h` bajo `__EMSCRIPTEN__`: el nativo no compila.
- `VICEEXT_P4_EMIT` se usa fuera de `__EMSCRIPTEN__` (PlayerPed.cpp:5316-5324).
- Las trazas DRIVEBY de Ped.cpp:5105-5179 usan `EM_ASM_INT` sin guarda web.

### 6.3 Lista completa de filas SIN-GUARDA (4964 líneas del diff = 3 867 añadidas + 1 097 borradas)

Columnas: `fichero:líneas` (oN = vanilla borrada) · ± · nº de líneas reales del diff en el rango · feature · descripción.

| Fichero:líneas | ± | Nº | Bandera/feature | Descripción |
|---|---|---:|---|---|
| peds/PlayerPed.cpp:13 | + | 1 | AIM_CLASSICAXIS (include) | Sprite.h para CSprite::CalcScreenCoors del fijado ClassicAXIS |
| peds/PlayerPed.cpp:32 | + | 1 | MANUAL_RELOAD/ALT_SIT (include) | ControllerConfig.h (PED_RELOAD/PED_WALK) |
| peds/PlayerPed.cpp:35 | + | 1 | armas mod (include) | WeaponInfo.h |
| peds/PlayerPed.cpp:39 | + | 1 | FEATURES_INI (include) | ViceExtFeatures.h: features.ini en tiempo de ejecucion |
| peds/PlayerPed.cpp:1073-1084 | + | 11 | AIM_CLASSICAXIS C10 | PlayerControlSniper: fuerza MODE_FOLLOWPED alrededor de Fire si s_viceExtAimLawActive |
| peds/PlayerPed.cpp:1109-1115 | + | 7 | AIM_CLASSICAXIS C8 | ProcessWeaponSwitch: bloquea cambio de arma con la ley de apuntado activa |
| peds/PlayerPed.cpp:1130 | + | 1 | AIM_CLASSICAXIS C8 | blanco |
| peds/PlayerPed.cpp:1252-1263 | + | 12 | AIM_CLASSICAXIS C10 | PlayerControlM16: fuerza MODE_FOLLOWPED alrededor de Fire |
| peds/PlayerPed.cpp:2713-2724 | + | 10 | ROCKET_3RD_PERSON (features.ini, defecto 1) | lanzacohetes no entra en modo francotirador si RocketLauncherThirdPersonAiming |
| peds/PlayerPed.cpp:2749 | + | 1 | ROCKET_3RD_PERSON (features.ini, defecto 1) | cierre del if (!odRocket3p) |
| peds/PlayerPed.cpp:2823-2827 | + | 5 | FREE_CAM sin bFreeCam | rotacion de apuntado ya no exige CCamera::bFreeCam |
| peds/PlayerPed.cpp:3135-3136 | + | 2 | AIM_CLASSICAXIS (stick) | PlayerControlZelda: s_odStickBlocked sustituye al bloqueo vanilla del stick |
| peds/PlayerPed.cpp:3213-3215 | + | 3 | AIM_CLASSICAXIS (rumbo) | Zelda: ViceExtStrafeAiming/ViceExtMoveHeading deciden m_fRotationDest |
| peds/PlayerPed.cpp:3300-3306 | + | 7 | NO_CAR_BOUNCE (features.ini, defecto 1) | PlayerDoesntBounceAwayFromMovingCar: anula la evasion/salto ante coche |
| peds/PlayerPed.cpp:3318-3319 | + | 2 | NO_CAR_BOUNCE (features.ini, defecto 1) |  |
| peds/PlayerPed.cpp:5799-6278 | + | 403 | AIM_CLASSICAXIS (ProcessPlayerPedControl sin guarda) | predicados IsAbleToAim/1stPerson/TwoHanded, Find3rdPersonMouseTarget, RotatePlayer, ViceExtProcessPlayerPedControl (toma de camara MODE_AIMING, fijado, IK) |
| peds/PlayerPed.cpp:6317-6342 | + | 15 | HEALTH_REGEN (features.ini, defecto 1) | regenera vida hasta la mitad parado; VICEEXT_HEALTH_REGEN no se usa |
| peds/PlayerPed.cpp:6343-6347 | + | 5 | AIM_CLASSICAXIS (llamadas) | ProcessControl llama siempre ViceExtProcessPlayerPedControl + ViceExtFind3rdPersonMouseTarget |
| peds/PlayerPed.cpp:o840 | - | 1 | AIM_CLASSICAXIS C10 | Fire reubicado entre cambio de modo de camara |
| peds/PlayerPed.cpp:o1351 | - | 1 | FREE_CAM sin bFreeCam | condicion vanilla && CCamera::bFreeCam quitada |
| peds/PlayerPed.cpp:o1500-1502 | - | 3 | AIM_CLASSICAXIS (stick) | vanilla: con MovementDisabledBecauseOfTargeting el stick se ponia a 0; quitado siempre |
| peds/PlayerPed.cpp:o1566 | - | 1 | NO_CAR_BOUNCE (features.ini) | condicion de evasion reescrita |
| peds/PlayerPed.h:27-54 | + | 28 | AIM_CLASSICAXIS (helpers inline) | ViceExtAimHeavy / ViceExtAimLongGun (los usan Cam.cpp y ClassicAXIS) |
| core/Cam.cpp:11 | + | 1 | PEDARBITER(sin guarda) | #include PedArbiter.h sin guarda; lo exige ViceExtPedOwns() sin guarda de Process_AimWeaponFovLerp |
| core/Cam.cpp:61-67 | + | 7 | CAR_LAW/AIM_CLASSICAXIS | statics de fichero de la ley de colisiones; s_odLosHit lo usa la logica (s_odDistObs) |
| core/Cam.cpp:74 | + | 1 | CAR_LAW/AIM_CLASSICAXIS | s_odSphApp (lo usa la logica: s_odDistObs) |
| core/Cam.cpp:77-83 | + | 7 | AIM_CLASSICAXIS/CAR_LAW | s_odHidePeds/s_odHideCount (esconder peds en apuntado) y s_odDistObs (restaura distancia en la ley de coche) |
| core/Cam.cpp:423-425 | + | 3 | AIM_CLASSICAXIS(sin guarda) | CCam::Process: descomenta case MODE_AIMING y lo conecta a Process_AimWeapon (ley ClassicAXIS) siempre |
| core/Cam.cpp:627-651 | + | 25 | AIM_CLASSICAXIS(sin guarda) B4 | hombro al apuntar: s_odAimStoriesShoulder, ViceExtAimShoulder, ViceExtAimingOverShoulder (cabecera) |
| core/Cam.cpp:657-664 | + | 8 | AIM_CLASSICAXIS(sin guarda) B4 | ViceExtAimingOverShoulder: puerta GetTarget + CANAIM |
| core/Cam.cpp:676-695 | + | 20 | AIM_CLASSICAXIS(sin guarda) B4 | ViceExtAimShoulderSmoothed (WellBufferMe del hombro 0.2 m) |
| core/Cam.cpp:713-843 | + | 129 | COSMETICO-indent | ProcessSpecialHeightRoutines desindentado (solo espacios: diff -w vacio; sin cambio funcional) |
| core/Cam.cpp:1419-1420 | + | 2 | AIM_CLASSICAXIS(sin guarda) B4 | Process_FollowPed: TargetCoors += hombro suavizado |
| core/Cam.cpp:1495 | + | 1 | CAST(sin guarda) | Process_FollowPed: CPed* -> CPlayerPed* (sin efecto funcional) |
| core/Cam.cpp:1814-1819 | + | 6 | AIM_CLASSICAXIS(sin guarda) B4 | FollowPedWithMouse: TargetCoors += hombro suavizado |
| core/Cam.cpp:1822-1823 | + | 2 | RECOIL/AIM(sin guarda) | UseMouse/LookUpDown sacadas del bloque (las usa el recoil y el clamp) |
| core/Cam.cpp:1837 | + | 1 | RECOIL/AIM(sin guarda) | declaracion LookLeftRight sin LookUpDown (hoist) |
| core/Cam.cpp:1847-1848 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) | raton FollowPedWithMouse * ViceExtAimSensMouseX/Y (1.0 si la bandera esta apagada) |
| core/Cam.cpp:1850-1853 | + | 4 | AIM_CLASSICAXIS(sin guarda) B8 | stick FollowPedWithMouse 1/14 -> 1/20 (y 0.6/14 -> 0.6/20) * ViceExtAimSensPad: cambia la velocidad vanilla siempre |
| core/Cam.cpp:1886-1895 | + | 10 | AIM_CLASSICAXIS(sin guarda) B8 | clamp vertical +45/-89.5 -> +60/-89.5 (y +-50 apuntando) |
| core/Cam.cpp:2349-2414 | + | 66 | CAR_LAW(sin guarda) | banner BARRIDO 1: describe la ley de coche nueva y los bloques vanilla barridos |
| core/Cam.cpp:2418-2717 | + | 283 | CAR_LAW(sin guarda) | Process_Cam_On_A_String reescrito: esferica CamNew, Beta como estado, raton/palo, detector de mirada, retorno pasivo 1,5 s |
| core/Cam.cpp:2764-2779 | + | 16 | CAR_LAW(sin guarda) | transicion: libera el pin de m_bUseTransitionBeta |
| core/Cam.cpp:2781-2790 | + | 10 | CAR_LAW(sin guarda) | hold de entrada 5 frames (linea 2787 lleva ademas una llamada BETASNAP) + centrar detras |
| core/Cam.cpp:2792-2797 | + | 6 | CAR_LAW(sin guarda) | centrar detras |
| core/Cam.cpp:2799-2811 | + | 13 | CAR_LAW(sin guarda) | centrar delante + colocacion Source/Front |
| core/Cam.cpp:2819-2821 | + | 3 | CAR_LAW(sin guarda) | Firetruck: dist -> dist2 (renombrado por choque de nombres) |
| core/Cam.cpp:2824-2833 | + | 10 | CAR_LAW(sin guarda) | suelo de agua + suelo RC en lugar de Cam_On_A_String_Unobscured/WorkOutCamHeight/RotCamIfInFrontCar/FixCamWhenObscuredByVehicle |
| core/Cam.cpp:2836-2843 | + | 8 | CAR_LAW(sin guarda) | Process_AvoidCollisions(TargetCoors,length,false) en lugar de AvoidTheGeometry |
| core/Cam.cpp:2848-2904 | + | 55 | CAR_LAW(sin guarda) retenido | bloque tilt heli / roll retenido verbatim, solo reindentado dentro de { } |
| core/Cam.cpp:2956 | + | 1 | CAR_LAW(sin guarda) | blanco |
| core/Cam.cpp:3312-3313 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) | Process_Rocket: raton * ViceExtAimSensMouseX/Y |
| core/Cam.cpp:3425-3426 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) | Process_M16_1stPerson: raton * ViceExtAimSensMouseX/Y |
| core/Cam.cpp:3884-3885 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) | Process_1rstPersonPedOnPC: raton * ViceExtAimSensMouseX/Y |
| core/Cam.cpp:3940-3945 | + | 6 | FIRST_PERSON(sin guarda) | Process_1rstPersonPedOnPC: CamTargetEntity en vez de TheCamera.pTargetEntity (crash con la 1a persona del mod) |
| core/Cam.cpp:4033-4034 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) | Process_Sniper: raton * ViceExtAimSensMouseX/Y |
| core/Cam.cpp:4141-4498 | + | 349 | AIM_CLASSICAXIS(sin guarda) | CCam::Process_AvoidCollisions (LOS+5 esferas, esconder peds), constantes AIM_*, Process_AimWeaponFovLerp, Process_AimWeaponCrouchOffset, Process_AimWeapon (1a parte) |
| core/Cam.cpp:4292-4298 | + | 7 | AIM_CLASSICAXIS(sin guarda) hook PEDLANE_NADO | FovLerp: si el carril NADO tiene APUNTAR/CAMARA, FOV por defecto (usa PedArbiter sin guarda) |
| core/Cam.cpp:4510-4543 | + | 34 | AIM_CLASSICAXIS(sin guarda) | Process_AimWeapon: offsets, clamp +-50, detras/delante, angulo anterior |
| core/Cam.cpp:4555-4569 | + | 15 | AIM_CLASSICAXIS(sin guarda) | Process_AimWeapon: vectores, Process_AvoidCollisions(...,true), GetVectorsReadyForRW |
| core/Cam.cpp:4637-4641 | + | 5 | AIM_CLASSICAXIS(sin guarda) | cierre Process_AimWeapon |
| core/Cam.cpp:6214 | + | 1 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | Process_FollowPed_Rotation: BaseOffset 0.75 -> 0.4 |
| core/Cam.cpp:6262-6263 | + | 2 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | FollowPed_Rotation: TargetCoors += hombro suavizado |
| core/Cam.cpp:6287 | + | 1 | COSMETICO [FREE_CAM] | ResetStatics = false; |
| core/Cam.cpp:6309-6313 | + | 5 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | comentario: se habilita el raton en FollowPed_Rotation |
| core/Cam.cpp:6325 | + | 1 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | descomenta la rama del raton (vanilla la tenia en /* */) |
| core/Cam.cpp:6328 | + | 1 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | cierre de la rama del raton descomentada |
| core/Cam.cpp:6330-6331 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) [FREE_CAM] | raton * ViceExtAimSensMouseX/Y |
| core/Cam.cpp:6333-6334 | + | 2 | AIM_SENSITIVITY(llamada sin guarda) [FREE_CAM] | stick * ViceExtAimSensPadX/Y |
| core/Cam.cpp:6349-6354 | + | 6 | AIM_CLASSICAXIS(sin guarda) B8 [FREE_CAM] | clamp vertical +45 -> +60 (+-50 apuntando) |
| core/Cam.cpp:6478-6485 | + | 8 | CAR_LAW(sin guarda) [FREE_CAM] | Process_FollowCar_SA: comentario de barrido |
| core/Cam.cpp:6487-6490 | + | 4 | CAR_LAW(sin guarda) [FREE_CAM] | Process_FollowCar_SA delega en Process_Cam_On_A_String (torreta retenida) |
| core/Cam.cpp:o193 | - | 1 | AIM_CLASSICAXIS(sin guarda) | borra el comentario //case MODE_AIMING (pasa a case activo) |
| core/Cam.cpp:o409-539 | - | 129 | COSMETICO-indent | ProcessSpecialHeightRoutines: lineas re-anadidas sin sangria (solo espacios) |
| core/Cam.cpp:o1107 | - | 1 | CAST(sin guarda) | CPed *ped (pasa a CPlayerPed*) |
| core/Cam.cpp:o1382-1385 | - | 2 | RECOIL/AIM(sin guarda) | UseMouse/LookUpDown movidas fuera del bloque |
| core/Cam.cpp:o1395-1396 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) | formula de raton vanilla FollowPedWithMouse |
| core/Cam.cpp:o1398-1399 | - | 2 | AIM_CLASSICAXIS(sin guarda) B8 | formula de stick vanilla 1/14 FollowPedWithMouse |
| core/Cam.cpp:o1422-1423 | - | 2 | AIM_CLASSICAXIS(sin guarda) B8 | clamp vanilla +45/-89.5 FollowPedWithMouse |
| core/Cam.cpp:o1795-1847 | - | 28 | CAR_LAW(sin guarda) | Process_Cam_On_A_String vanilla: reset de tilt, Beta re-derivada, Cam_On_A_String_Unobscured, WorkOutCamHeight, RotCamIfInFrontCar, FixCamWhenObscuredByVehicle, AvoidTheGeometry |
| core/Cam.cpp:o1852-1903 | - | 49 | CAR_LAW(sin guarda) retenido | tilt/roll vanilla: reaparece verbatim en 2848-2904 solo reindentado |
| core/Cam.cpp:o2256-2257 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) | raton vanilla Process_Rocket |
| core/Cam.cpp:o2358-2359 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) | raton vanilla Process_M16_1stPerson |
| core/Cam.cpp:o2544 | - | 1 | COSMETICO | blanco borrado en Process_1stPerson |
| core/Cam.cpp:o2561 | - | 1 | COSMETICO | blanco borrado en Process_1stPerson |
| core/Cam.cpp:o2769-2770 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) | raton vanilla Process_1rstPersonPedOnPC |
| core/Cam.cpp:o2820-2823 | - | 4 | FIRST_PERSON(sin guarda) | TheCamera.pTargetEntity vanilla (sustituido por CamTargetEntity) |
| core/Cam.cpp:o2908-2909 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) | raton vanilla Process_Sniper |
| core/Cam.cpp:o4578 | - | 1 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | BaseOffset 0.75 vanilla |
| core/Cam.cpp:o4608-4610 | - | 3 | COSMETICO [FREE_CAM] | ResetStatics/llave/blanco fusionados |
| core/Cam.cpp:o4636-4647 | - | 6 | AIM_CLASSICAXIS(sin guarda) B4 [FREE_CAM] | quita el /* */ que desactivaba el raton en FollowPed_Rotation |
| core/Cam.cpp:o4649-4650 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) [FREE_CAM] | raton vanilla FollowPed_Rotation |
| core/Cam.cpp:o4652-4653 | - | 2 | AIM_SENSITIVITY(llamada sin guarda) [FREE_CAM] | stick vanilla FollowPed_Rotation |
| core/Cam.cpp:o4664 | - | 1 | AIM_CLASSICAXIS(sin guarda) B8 [FREE_CAM] | clamp vanilla +45 FollowPed_Rotation |
| core/Cam.cpp:o4782-5355 | - | 572 | CAR_LAW(sin guarda) [FREE_CAM] | Process_FollowCar_SA (LCS/SA) vaciado: 571 lineas vanilla borradas (CARCAM_SET, historial, raton, colisiones LCS, suelo agua/RC) |
| peds/Ped.cpp:9 | + | 1 | include | include PlayerPed.h para las llamadas ViceExt* (agachado/apuntado) desde CPed |
| peds/Ped.cpp:58-59 | + | 2 | VICEEXT_CHEAT_EFFECTS | definicion estaticos CPed::bViceExtBigHeads/bViceExtRcRocket fuera de guarda |
| peds/Ped.cpp:1497-1505 | + | 9 | VICEEXT_CROUCH | CalculateNewVelocity: odCrouchMove/odAimMove saltan el corte vanilla de control de raton |
| peds/Ped.cpp:1510-1511 | + | 2 | VICEEXT_CROUCH | CalculateNewVelocity: odCrouchMove/odAimMove saltan el corte vanilla de control de raton |
| peds/Ped.cpp:1516-1519 | + | 4 | VICEEXT_CROUCH | CalculateNewVelocity: odCrouchMove/odAimMove saltan el corte vanilla de control de raton |
| peds/Ped.cpp:o1450-1451 | - | 2 | VICEEXT_CROUCH | condicion vanilla del corte reescrita |
| peds/Ped.cpp:o1468 | - | 1 | VICEEXT_AIM_CLASSICAXIS | comentario vanilla borrado al insertar el else de ClassicAXIS |
| peds/Ped.cpp:1757 | + | 1 | VICEEXT_AIM_CLASSICAXIS | bool odLegsBodyOwned (piernas con la ley de rumbo) |
| peds/Ped.cpp:1800-1808 | + | 9 | VICEEXT_AIM_CLASSICAXIS | rama nueva: si la ley ClassicAXIS es duenna del rumbo, bDontAcceptIKLookAts por angulo de piernas |
| peds/Ped.cpp:o1482 | - | 1 | VICEEXT_AIM_CLASSICAXIS | if vanilla convertido en else-if |
| peds/Ped.cpp:3619-3626 | + | 8 | POLISH(sin guarda) | PlayFootSteps: salpicadura de charco coloreada por timecycle y mayor (0.22/6 vs 0.15/4) |
| peds/Ped.cpp:o3281 | - | 1 | POLISH(sin guarda) | llamada vanilla de salpicadura |
| peds/Ped.cpp:4254-4268 | + | 15 | FREE_CAM/raton(sin guarda) | CanStrafeOrMouseControl: bFreeCam solo corta si NO hay camara de raton 3a persona |
| peds/Ped.cpp:o3909 | - | 1 | FREE_CAM/raton(sin guarda) | condicion vanilla bFreeCam |
| peds/Ped.cpp:4988 | + | 1 | armas-mod | GiveWeapon: IsWeaponType() admite IDs de armas del mod |
| peds/Ped.cpp:o4629 | - | 1 | armas-mod | limite vanilla WEAPONTYPE_TOTALWEAPONS |
| peds/Ped.cpp:5059 | + | 1 | armas-mod | GrantAmmo: IsWeaponType() |
| peds/Ped.cpp:o4700 | - | 1 | armas-mod | limite vanilla |
| peds/Ped.cpp:5079 | + | 1 | armas-mod | SetAmmo: IsWeaponType() |
| peds/Ped.cpp:o4720 | - | 1 | armas-mod | limite vanilla |
| peds/Ped.cpp:5180-5189 | + | 10 | VICEEXT_DRIVEBY_WIDE | CanDoDriveByWithCurrentWeapon / KeepsWeaponInHandWhileDriving (andamiaje sin guarda; equivalen a vanilla sin la bandera) |
| peds/Ped.cpp:5197-5206 | + | 10 | VICEEXT_DRIVEBY_WIDE | CanDoDriveByWithCurrentWeapon / KeepsWeaponInHandWhileDriving (andamiaje sin guarda; equivalen a vanilla sin la bandera) |
| peds/Ped.cpp:5214-5216 | + | 3 | VICEEXT_DRIVEBY_WIDE | CanDoDriveByWithCurrentWeapon / KeepsWeaponInHandWhileDriving (andamiaje sin guarda; equivalen a vanilla sin la bandera) |
| peds/Ped.cpp:5220-5221 | + | 2 | VICEEXT_DRIVEBY_WIDE | RemoveWeaponWhenEnteringVehicle: keepsWeapon |
| peds/Ped.cpp:5223-5228 | + | 6 | VICEEXT_DRIVEBY_WIDE | rama: pistola en la mano gana al cambio a SMG |
| peds/Ped.cpp:5230 | + | 1 | VICEEXT_DRIVEBY_WIDE | if vanilla convertido en else-if |
| peds/Ped.cpp:o4749 | - | 1 | VICEEXT_DRIVEBY_WIDE | if vanilla de cambio a SMG |
| peds/Ped.cpp:5235 | + | 1 | VICEEXT_DRIVEBY_WIDE | else-if keepsWeapon: no esconde el modelo |
| peds/Ped.cpp:5250-5252 | + | 3 | VICEEXT_DRIVEBY_WIDE | ReplaceWeaponWhenExitingVehicle: keepsWeapon |
| peds/Ped.cpp:5264 | + | 1 | VICEEXT_DRIVEBY_WIDE | else-if keepsWeapon: no vuelve a poner el modelo |
| peds/Ped.h:1005-1012 | + | 8 | VICEEXT_DRIVEBY_WIDE | declaraciones CanDoDriveByWithCurrentWeapon/KeepsWeaponInHandWhileDriving |
| peds/Ped.h:1071-1081 | + | 11 | VICEEXT_CROUCH | declaracion CPed::ViceExtCrouchShooting (impl. en PlayerPed.cpp) |
| peds/Ped.h:1180-1182 | + | 3 | VICEEXT_CHEAT_EFFECTS | estaticos bViceExtBigHeads/bViceExtRcRocket |
| peds/PedFight.cpp:21 | + | 1 | VICEEXT_NO_CAR_BOUNCE | include ViceExtFeatures.h (lo usa KillPedWithCar sin guarda) |
| peds/PedFight.cpp:184-190 | + | 7 | VICEEXT_CROUCH | comentario R28 (cambio sin guarda) |
| peds/PedFight.cpp:o182-185 | - | 4 | VICEEXT_CROUCH | SetPointGunAt: rama vanilla de crouchfire borrada |
| peds/PedFight.cpp:207-222 | + | 16 | VICEEXT_CROUCH | SetPointGunAt: agachado del port no pone clip de arma (return) y siempre ANIM_WEAPON_FIRE |
| peds/PedFight.cpp:225 | + | 1 | VICEEXT_CROUCH | SetPointGunAt: siempre AddAnimation ANIM_WEAPON_FIRE |
| peds/PedFight.cpp:o189-193 | - | 5 | VICEEXT_CROUCH | rama vanilla BlendAnimation crouchfire borrada |
| peds/PedFight.cpp:238-248 | + | 11 | VICEEXT_CROUCH | PointGunAt: agachado del port solo ajusta IK y sale |
| peds/PedFight.cpp:442-443 | + | 2 | VICEEXT_CROUCH | SetAttack: ViceExtCrouchShooting() en vez de bCrouchWhenShooting&&bIsDucking |
| peds/PedFight.cpp:o399 | - | 1 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:558 | + | 1 | VICEEXT_CROUCH | FinishedAttackCB: ViceExtCrouchShooting() |
| peds/PedFight.cpp:o514 | - | 1 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:580 | + | 1 | VICEEXT_CROUCH | FinishedAttackCB: ViceExtCrouchShooting() && GetCrouchFireAnim |
| peds/PedFight.cpp:o536 | - | 1 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:638 | + | 1 | VICEEXT_CROUCH | FinishedReloadCB: ViceExtCrouchShooting() |
| peds/PedFight.cpp:o594 | - | 1 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:742-745 | + | 4 | VICEEXT_CROUCH | Attack: ViceExtCrouchShooting() |
| peds/PedFight.cpp:o686-687 | - | 2 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:863 | + | 1 | VICEEXT_CROUCH | Attack: ViceExtCrouchShooting() |
| peds/PedFight.cpp:o781 | - | 1 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:957-967 | + | 10 | VICEEXT_AIM_CLASSICAXIS | Attack: fuerza MODE_FOLLOWPED durante el disparo si s_viceExtAimLawActive (C10) |
| peds/PedFight.cpp:1011-1021 | + | 10 | VICEEXT_AIM_CLASSICAXIS | Attack: idem segundo callsite |
| peds/PedFight.cpp:1095 | + | 1 | VICEEXT_CROUCH | Attack: recarga agachada con ViceExtCrouchShooting() |
| peds/PedFight.cpp:o980 | - | 1 | VICEEXT_CROUCH | condicion vanilla |
| peds/PedFight.cpp:2960-2967 | + | 8 | armas-mod | InflictDamage: casos de muerte para armas del mod (BERETTA, DESERT_EAGLE, SHOTGUN2, UZIOLD, AK47, M16, STEYR) |
| peds/PedFight.cpp:3883-3894 | + | 12 | VICEEXT_NO_CAR_BOUNCE | KillPedWithCar: jugador pegado al suelo via ViceExtFeatures::On() SIN #ifdef |
| peds/CopPed.cpp:208 | + | 1 | cosmetico | dos lineas vanilla fundidas en una (sin efecto) |
| peds/CopPed.cpp:o170-171 | - | 2 | cosmetico | lineas vanilla fundidas |
| peds/Population.cpp:432 | + | 1 | VICEEXT_LIMITS | PedCreationDistMultiplier: variable intermedia para el multiplicador (equivale sin bandera) |
| peds/Population.cpp:436 | + | 1 | VICEEXT_LIMITS | PedCreationDistMultiplier: variable intermedia para el multiplicador (equivale sin bandera) |
| peds/Population.cpp:o416 | - | 1 | VICEEXT_LIMITS | return vanilla |
| core/Camera.h:227-245 | + | 19 | VICEEXT_AIM_CLASSICAXIS | CCam: declaraciones Process_AimWeapon/FovLerp/CrouchOffset/Process_AvoidCollisions |
| core/Camera.h:344-361 | + | 18 | VICEEXT_AIM_CLASSICAXIS | struct CAimClassicAxisSettings |
| core/Camera.h:362-379 | + | 18 | VICEEXT_AIM_SENSITIVITY | struct CAimSensSettings |
| core/Camera.h:594-605 | + | 12 | VICEEXT_AIM_CLASSICAXIS | CCamera: s_viceExtAim, s_viceExtAimLawActive |
| core/Camera.h:606-614 | + | 9 | VICEEXT_AIM_SENSITIVITY | CCamera: s_viceExtSens + ViceExtAimSens*() |
| core/Camera.h:615-624 | + | 10 | VICEEXT_AIM_CLASSICAXIS | CCamera: estado de la ley (switch speed, prev mode/angulos, lock-on) |
| core/Camera.h:625-629 | + | 5 | VICEEXT_FIRST_PERSON | CCamera: s_viceExt1P* (FirstPerson.cfg) y m_bViceExt1stPersonView (definido solo con la bandera) |
| core/Camera.h:630-634 | + | 5 | VICEEXT_AIM_CLASSICAXIS | CCamera: forceRealMoveAnim, view dir/pending/mode, ViceExtAimBetaFromFollowBeta |
| core/Camera.cpp:36 | + | 1 | VICEEXT_FIRST_PERSON | include PedArbiter.h (carril LEY de la 1a persona) |
| core/Camera.cpp:86-101 | + | 16 | VICEEXT_AIM_CLASSICAXIS | definicion s_viceExtAim (defaults del INI) y s_viceExtAimLawActive |
| core/Camera.cpp:102-117 | + | 16 | VICEEXT_AIM_SENSITIVITY | definicion s_viceExtSens (defaults ViceEx.ini) |
| core/Camera.cpp:142-152 | + | 11 | VICEEXT_AIM_SENSITIVITY | ViceExtAimSensFactor y cuerpos de ViceExtAimSens* (devuelven 1.0 sin bandera) |
| core/Camera.cpp:161-164 | + | 4 | VICEEXT_AIM_SENSITIVITY | ViceExtAimSensFactor y cuerpos de ViceExtAimSens* (devuelven 1.0 sin bandera) |
| core/Camera.cpp:174-178 | + | 5 | VICEEXT_AIM_SENSITIVITY | ViceExtAimSensFactor y cuerpos de ViceExtAimSens* (devuelven 1.0 sin bandera) |
| core/Camera.cpp:184-188 | + | 5 | VICEEXT_AIM_SENSITIVITY | ViceExtAimSensFactor y cuerpos de ViceExtAimSens* (devuelven 1.0 sin bandera) |
| core/Camera.cpp:194-198 | + | 5 | VICEEXT_AIM_SENSITIVITY | ViceExtAimSensFactor y cuerpos de ViceExtAimSens* (devuelven 1.0 sin bandera) |
| core/Camera.cpp:204 | + | 1 | VICEEXT_AIM_SENSITIVITY | ViceExtAimSensFactor y cuerpos de ViceExtAimSens* (devuelven 1.0 sin bandera) |
| core/Camera.cpp:205-225 | + | 21 | VICEEXT_AIM_CLASSICAXIS | estaticos de estado de la ley + ViceExtAimBetaFromFollowBeta |
| core/Camera.cpp:424-448 | + | 25 | VICEEXT_AIM_CLASSICAXIS | Init: m_f3rdPersonCHairMultX/Y desde s_viceExtAim (re3.cpp los puede cambiar por INI) + reset del estado de la ley |
| core/Camera.cpp:o244-245 | - | 2 | VICEEXT_AIM_CLASSICAXIS | constantes vanilla 0.53/0.4 |
| core/Camera.cpp:453-467 | + | 15 | VICEEXT_FIRST_PERSON | near-clip dual / FirstPerson.cfg (estaticos s_viceExt1P*, macros) |
| core/Camera.cpp:540-542 | + | 3 | VICEEXT_FIRST_PERSON | Process: near-clip segun coche/pie (ambos DEFAULT_NEAR salvo re3.cpp) |
| core/Camera.cpp:o297 | - | 1 | VICEEXT_FIRST_PERSON | RwCameraSetNearClipPlane vanilla |
| core/Camera.cpp:2994-3011 | + | 18 | VICEEXT_AIM_CLASSICAXIS | StartTransition: transicion de 500 ms al entrar/salir de MODE_AIMING (C6) |
| core/Camera.cpp:4481-4482 | + | 2 | cosmetico | solo sangria (Find3rdPersonCamTargetVector) |
| core/Camera.cpp:o3972-3973 | - | 2 | cosmetico | sangria vanilla |
| core/Camera.cpp:4497-4518 | + | 22 | VICEEXT_AIM_CLASSICAXIS | Find3rdPersonQuickAimPitch reemplazado por la formula ClassicAXIS (cambia el resultado SIEMPRE) |
| core/Camera.cpp:o3988-3992 | - | 5 | VICEEXT_AIM_CLASSICAXIS | formula vanilla del pitch |
| core/main.cpp:661-683 | + | 23 | SilentPatch-loadbar | cabecera + macros VICEEXT_LOADBAR_BORDER_X/Y (SilentPatch :723) sin #ifdef VICEEXT |
| core/main.cpp:o688 | - | 1 | SilentPatch-loadbar | contorno de la barra de carga con margen fijo 1px (vanilla) sustituido |
| core/main.cpp:746-749 | + | 4 | SilentPatch-loadbar | contorno de barra de carga con margen escalado |
| core/Streaming.cpp:1773-1776 | + | 4 | armas-mod(MI_GR_GRENADE) | proyectil del lanzagranadas del mod residente (RequestModel MI_GR_GRENADE) |
| core/FileLoader.cpp:1203-1217 | + | 15 | mapa-mod(bryx.ipl) | inst con id -1 se resuelve por nombre (IPL del mod bryx.ipl) |
| core/re3.cpp:o325 | - | 1 | PED_WALK(ClassicAXIS WalkKey) | lista de nombres de acciones de control |
| core/re3.cpp:333-334 | + | 2 | PED_WALK(ClassicAXIS WalkKey) | accion PED_WALK anadida al ini de controles sin guarda |
| core/re3.cpp:338 | + | 1 | PED_WALK(ClassicAXIS WalkKey) | cierre del array (comentario ClassicAXIS C17) |
| core/re3.cpp:1063-1073 | + | 11 | debugmenu-coches-mod | DEBUGMENU: spawn de coches ViceEx (MI_VEEXT_*) |
| core/config.h:o12 | - | 1 | limites-mod | MAX_CDIMAGES=8 vanilla |
| core/config.h:12-15 | + | 4 | limites-mod | MAX_CDIMAGES=12 (8 IMG del mod) |
| core/config.h:o15-17 | - | 3 | limites-mod | MODELINFOSIZE/TXDSTORESIZE/COLSTORESIZE vanilla |
| core/config.h:18-32 | + | 15 | limites-mod | MODELINFOSIZE=6700, TXDSTORESIZE=1500, COLSTORESIZE=48 |
| core/config.h:o24-26 | - | 2 | limites-mod | WEAPONMODELSIZE/VEHICLEMODELSIZE vanilla |
| core/config.h:39-44 | + | 5 | limites-mod | WEAPONMODELSIZE=47, VEHICLEMODELSIZE=130 |
| core/config.h:o46-47 | - | 2 | limites-mod | NUMANIMBLOCKS/NUMANIMATIONS vanilla |
| core/config.h:72-79 | + | 8 | limites-mod | NUMANIMBLOCKS=40, NUMANIMATIONS=512 (anims nuevas del ped.ifp del mod, incl. swim_*) |
| core/config.h:o370 | - | 1 | SilentPatch-outro | MUCH_SHORTER_OUTRO_SCREEN desactivado |
| core/config.h:535-537 | + | 3 | SilentPatch-outro | MUCH_SHORTER_OUTRO_SCREEN comentado (outro de SilentPatch) |
| core/Game.cpp:2-3 | + | 2 | includes-mod | include PedArbiter.h y ViceExtFeatures.h sin guarda |
| core/Game.cpp:989 | + | 1 | VICEEXT_PEDARBITER(sin guarda) | ViceExtPedResetAll() en InitialiseWhenRestarting sin #ifdef |
| save/GenericGameStorage.cpp:89-93 | + | 5 | VICEEXT_AUTOSAVE (sin guarda, bajo MISSION_REPLAY) | PAUSE_SAVE_SLOT = SLOT_COUNT-1 (ranura 9 = autosave) |
| save/GenericGameStorage.cpp:o51 | - | 1 | VICEEXT_AUTOSAVE (sin guarda) | borra PAUSE_SAVE_SLOT = SLOT_COUNT vanilla |
| save/GenericGameStorage.h:6-8 | + | 3 | VICEEXT_AUTOSAVE (sin guarda) | SLOT_COUNT 8 -> 9 (ranura 9 = autosave/quicksave) |
| save/GenericGameStorage.h:o6 | - | 1 | VICEEXT_AUTOSAVE (sin guarda) | SLOT_COUNT (8) vanilla borrado |
| save/GenericGameStorage.h:72-75 | + | 4 | VICEEXT_AUTOSAVE (sin guarda, bajo MISSION_REPLAY) | extern const int PAUSE_SAVE_SLOT (lo usan Frontend.cpp:5590 y Script.cpp:1630) |
| renderer/Coronas.cpp:418-419 | + | 2 | VICEEXT_FIX_SILENTPATCH (sin guarda; SP :1931) | flares escalados con SCREEN_STRETCH_X/Y |
| renderer/Coronas.cpp:o382-383 | - | 2 | VICEEXT_FIX_SILENTPATCH (sin guarda; SP :1931) | tamano de flare vanilla sin stretch |
| animation/AnimManager.cpp:628-631 | + | 4 | MOD-ARMAS anims (sin guarda) | buddy_reload/buddy_crouchreload anadidos al grupo buddy vanilla |
| animation/AnimManager.cpp:667-686 | + | 20 | MOD-ARMAS anims (sin guarda) | aDeagle/aSteyr/aRocketAnimations (anims.img del mod) |
| animation/AnimManager.cpp:733-737 | + | 5 | VICEEXT_SPRINT_HEAVY (sin guarda) | sprint_rocket en el hueco RUNFAST |
| animation/AnimManager.cpp:751-753 | + | 3 | VICEEXT_SPRINT_HEAVY (sin guarda) | sprint_armed en el hueco RUNFAST |
| animation/AnimManager.cpp:767-769 | + | 3 | VICEEXT_SPRINT_HEAVY (sin guarda) | sprint_csaw en el hueco RUNFAST |
| animation/AnimManager.cpp:o708-736 | - | 3 | VICEEXT_SPRINT_HEAVY (sin guarda) | run_rocket/run_armed/run_csaw del hueco RUNFAST |
| animation/AnimManager.cpp:971-1024 | + | 54 | VICEEXT_CROUCH (sin guarda) | aCrouchAnimations + aCrouchAnimDescs (incl. GunMove_*) |
| animation/AnimManager.cpp:1025-1045 | + | 21 | VICEEXT_CLIMB (sin guarda) | aClimbAnimations + aClimbAnimDescs |
| animation/AnimManager.cpp:1111-1114 | + | 4 | MOD-ARMAS anims (sin guarda) | grupos deagle/steyr/rocket en la tabla de grupos |
| animation/AnimManager.cpp:1118 | + | 1 | VICEEXT_CROUCH (sin guarda) | grupo playercrouch |
| animation/AnimManager.cpp:1119 | + | 1 | VICEEXT_CLIMB (sin guarda) | grupo playerclimb |
| animation/AnimManager.h:72-78 | + | 7 | MOD-ARMAS anims (sin guarda) | ASSOCGRP_DEAGLE/STEYR/ROCKET (preceden a PLAYERSWIM) |
| animation/AnimManager.h:82 | + | 1 | VICEEXT_CROUCH (sin guarda) | ASSOCGRP_PLAYERCROUCH |
| animation/AnimManager.h:83-86 | + | 4 | VICEEXT_CLIMB (sin guarda) | ASSOCGRP_PLAYERCLIMB |
| animation/AnimationId.h:296-306 | + | 11 | VICEEXT_CLIMB (sin guarda) | ANIM_STD_CLIMB_* (7) |
| animation/AnimationId.h:307-329 | + | 23 | VICEEXT_CROUCH (sin guarda) | ANIM_STD_CROUCH_* (5) + CROUCH_AIM* (4, GunMove_*) |
| audio/sampman_oal.cpp:124-138 | + | 15 | OdSfxReserve (armas del mod) | comentario R14 + odSfxProtected[]: reserva de muestras de disparo del mod |
| audio/sampman_oal.cpp:213-215 | + | 3 | OdSfxReserve (armas del mod) | OdSfxEvict no libera las reservadas |
| audio/sampman_oal.cpp:234-243 | + | 10 | OdSfxReserve (armas del mod) | OdSfxReserve() |
| audio/sampman_oal.cpp:329-333 | + | 5 | OdSfxReserve (armas del mod) | reservada = decode inmediato |
| audio/sampman_oal.cpp:1848-1863 | + | 13 | ReserveSample (armas del mod) | cSampleManager::ReserveSample (OAL) |
| audio/sampman_oal.cpp:1980-1994 | + | 14 | SFX_VICEEX banco del mod | InitialiseChannel acepta SFX_VICEEX_00..12 (>SAMPLEBANK_MAX) por la via SFX_BANK_0 |
| audio/sampman_oal.cpp:2001 | + | 1 | SFX_VICEEX banco del mod | condicion // odViceExSample en la via on-demand |
| audio/sampman.h:220-222 | + | 3 | ReserveSample (armas del mod) | declaracion ReserveSample |
| audio/AudioSamples.h:10120-10137 | + | 18 | SFX_VICEEX banco del mod | 13 muestras SFX_VICEEX_00..12 (ids 9941..9953) |
| audio/AudioLogic.cpp:868-874 | + | 7 | coches del mod (audio) | NUM_VEEXT_VEHICLE_SOUNDS=8, tabla ampliada |
| audio/AudioLogic.cpp:o865 | - | 1 | coches del mod (audio) | cabecera vanilla de la tabla |
| audio/AudioLogic.cpp:984-1010 | + | 27 | coches del mod (audio) | 8 filas 6500-6507 + VehicleAudioIndex() |
| audio/AudioLogic.cpp:o975 | - | 1 | coches del mod (audio) | cierre vanilla de la tabla |
| audio/AudioLogic.cpp:1058 | + | 1 | coches del mod (audio) | ProcessVehicle usa VehicleAudioIndex |
| audio/AudioLogic.cpp:o1023 | - | 1 | coches del mod (audio) | indice vanilla |
| audio/AudioLogic.cpp:2553-2557 | + | 3 | indentacion | solo indentacion cambiada (inocuo) |
| audio/AudioLogic.cpp:o2518-2522 | - | 3 | indentacion | lineas vanilla reindentadas |
| audio/AudioLogic.cpp:o2861 | - | 1 | VICEEXT_FIX_SILENTPATCH | if(FBIRANCH) vanilla borrado: sin la bandera se pierde la sirena aguda del Rancher |
| audio/AudioLogic.cpp:4565-4586 | + | 15 | armas del mod (audio) | casos SHOTGUN2/UZIOLD/AK47/M16/STEYR en colas de eco (lineas fusionadas) |
| audio/AudioLogic.cpp:o4473-4492 | - | 13 | armas del mod (audio) | casos vanilla reescritos |
| audio/AudioLogic.cpp:4651-4707 | + | 45 | armas del mod (audio) | disparo DEAGLE/SHOTGUN2/STEYR/GL con SFX_VICEEX_* + ReserveSample |
| audio/AudioLogic.cpp:4729-5025 | + | 12 | armas del mod (audio) | casos BERETTA/AK47/M16/UZIOLD/DEAGLE/STEYR/SHOTGUN2/GL en switches de arma |
| weapons/Weapon.h:57-58 | + | 2 | VICEEXT_DRIVEBY_WIDE | declaración GetDriveByShotDelay sin guarda |
| weapons/Weapon.h:93 | + | 1 | ARMAS_MOD | IsShotgun incluye WEAPONTYPE_SHOTGUN2 |
| weapons/Weapon.h:o77 | - | 1 | ARMAS_MOD | IsShotgun vanilla sustituido |
| weapons/Weapon.cpp:2 | + | 1 | VICEEXT_FEATURES_INI | include ViceExtFeatures.h sin guarda |
| weapons/Weapon.cpp:660 | + | 1 | ARMAS_MOD | case SHOTGUN2 en Fire |
| weapons/Weapon.cpp:690-696 | + | 7 | ARMAS_MOD | cases armas mod hitscan en Fire |
| weapons/Weapon.cpp:710 | + | 1 | ARMAS_MOD | case GRENADE_LAUNCHER en Fire |
| weapons/Weapon.cpp:731 | + | 1 | ARMAS_MOD | case GRENADE_LAUNCHER_GRENADE en Fire |
| weapons/Weapon.cpp:832-839 | + | 8 | ARMAS_MOD | cases armas mod (fogonazo/estadística) |
| weapons/Weapon.cpp:850-851 | + | 2 | ARMAS_MOD | cases lanzagranadas |
| weapons/Weapon.cpp:932-944 | + | 13 | VICEEXT_DRIVEBY_WIDE | GetDriveByShotDelay sin guarda (devuelve 70 sin la bandera) |
| weapons/Weapon.cpp:953-955 | + | 3 | VICEEXT_DRIVEBY_WIDE | cierre GetDriveByShotDelay |
| weapons/Weapon.cpp:1868-1871 | + | 4 | ARMAS_MOD | cases rifles mod (fogonazo grande) |
| weapons/Weapon.cpp:2862-2867 | + | 6 | ARMAS_MOD | lanzagranadas dispara GRENADE_LAUNCHER_GRENADE |
| weapons/Weapon.cpp:2870-2872 | + | 3 | ARMAS_MOD | projectileType ROCKET sólo para lanzacohetes |
| weapons/Weapon.cpp:o2085 | - | 1 | ARMAS_MOD | condición vanilla sustituida |
| weapons/Weapon.cpp:o2088 | - | 1 | ARMAS_MOD | asignación vanilla sustituida |
| weapons/Weapon.cpp:3768 | + | 1 | ARMAS_MOD | IsWeaponType en Update |
| weapons/Weapon.cpp:o2962 | - | 1 | ARMAS_MOD | chequeo < TOTALWEAPONS sustituido |
| weapons/Weapon.cpp:3798-3804 | + | 7 | ARMAS_MOD | sonido de recarga ASSOCGRP_DEAGLE/STEYR |
| weapons/Weapon.cpp:3951-3953 | + | 3 | ARMAS_MOD | IsType2Handed con AK47/M16/STEYR |
| weapons/Weapon.cpp:o3138 | - | 1 | ARMAS_MOD | IsType2Handed vanilla sustituido |
| weapons/WeaponInfo.cpp:12 | + | 1 | ARMAS_MOD | tabla a WEAPONTYPE_TOTALALLTYPES |
| weapons/WeaponInfo.cpp:o12 | - | 1 | ARMAS_MOD | tabla vanilla |
| weapons/WeaponInfo.cpp:50-62 | + | 13 | ARMAS_MOD | tiempos de recarga armas mod |
| weapons/WeaponInfo.cpp:o50 | - | 1 | ARMAS_MOD | fin de tabla vanilla |
| weapons/WeaponInfo.cpp:66 | + | 1 | ARMAS_MOD | ms_aMaxAmmoForWeapon TOTALALLTYPES |
| weapons/WeaponInfo.cpp:o54 | - | 1 | ARMAS_MOD | tabla vanilla |
| weapons/WeaponInfo.cpp:69 | + | 1 | ARMAS_MOD | valores ampliados |
| weapons/WeaponInfo.cpp:o57 | - | 1 | ARMAS_MOD | valores vanilla |
| weapons/WeaponInfo.cpp:72-73 | + | 2 | ARMAS_MOD | aWeaponInfo/ms_aWeaponNames TOTALALLTYPES |
| weapons/WeaponInfo.cpp:o60-61 | - | 2 | ARMAS_MOD | declaraciones vanilla |
| weapons/WeaponInfo.cpp:112-125 | + | 14 | ARMAS_MOD | nombres armas mod (weapon.dat) |
| weapons/WeaponInfo.cpp:138 | + | 1 | ARMAS_MOD | Initialise recorre TOTALALLTYPES |
| weapons/WeaponInfo.cpp:o112 | - | 1 | ARMAS_MOD | bucle vanilla |
| weapons/WeaponInfo.cpp:315 | + | 1 | ARMAS_MOD | FindWeaponType recorre TOTALALLTYPES |
| weapons/WeaponInfo.cpp:o270 | - | 1 | ARMAS_MOD | bucle vanilla |
| weapons/WeaponInfo.h:34-36 | + | 3 | ARMAS_MOD | ms_aWeaponNames TOTALALLTYPES |
| weapons/WeaponInfo.h:38-39 | + | 2 | ARMAS_MOD | tablas TOTALALLTYPES |
| weapons/WeaponInfo.h:o34 | - | 1 | ARMAS_MOD | declaración vanilla |
| weapons/WeaponInfo.h:o36-37 | - | 2 | ARMAS_MOD | declaraciones vanilla |
| weapons/WeaponType.h:53-79 | + | 26 | ARMAS_MOD | enum armas 48..56 + TOTALALLTYPES + IsWeaponType() |
| weapons/WeaponType.h:o53 | - | 1 | ARMAS_MOD | fin enum vanilla |
| weapons/WeaponEffects.cpp:71-73 | + | 3 | ARMAS_MOD | aCrossHairSize TOTALALLTYPES |
| weapons/WeaponEffects.cpp:85-89 | + | 5 | ARMAS_MOD | tamaños de mira armas mod |
| weapons/WeaponEffects.cpp:o71 | - | 1 | ARMAS_MOD | declaración vanilla |
| weapons/WeaponEffects.cpp:o83 | - | 1 | ARMAS_MOD | fin tabla vanilla |
| weapons/ProjectileInfo.cpp:104-126 | + | 23 | ARMAS_MOD | AddProjectile: granada del lanzagranadas |
| weapons/ProjectileInfo.cpp:202 | + | 1 | ARMAS_MOD | reformateo break/case |
| weapons/ProjectileInfo.cpp:204-210 | + | 7 | ARMAS_MOD | proyectil MI_GR_GRENADE |
| weapons/ProjectileInfo.cpp:248 | + | 1 | ARMAS_MOD | RemoveProjectile case lanzagranadas |
| weapons/ProjectileInfo.cpp:273 | + | 1 | ARMAS_MOD | RemoveNotAdd case lanzagranadas |
| weapons/ProjectileInfo.cpp:339 | + | 1 | ARMAS_MOD | Update trata granada como cohete (impacto) |
| weapons/ProjectileInfo.cpp:348-349 | + | 2 | ARMAS_MOD | aceleración 1.07 sólo cohete |
| weapons/ProjectileInfo.cpp:390 | + | 1 | ARMAS_MOD | IsProjectileInRange incluye granada mod |
| weapons/ProjectileInfo.cpp:o179-180 | - | 2 | ARMAS_MOD | break/case vanilla reformateado |
| weapons/ProjectileInfo.cpp:o182-183 | - | 2 | ARMAS_MOD | reindent vanilla |
| weapons/ProjectileInfo.cpp:o310 | - | 1 | ARMAS_MOD | condición vanilla |
| weapons/ProjectileInfo.cpp:o319 | - | 1 | ARMAS_MOD | aceleración vanilla |
| weapons/ProjectileInfo.cpp:o360 | - | 1 | ARMAS_MOD | condición vanilla |
| vehicles/Automobile.cpp:3 | + | 1 | VICEEXT_FEATURES_INI | include ViceExtFeatures.h sin guarda |
| vehicles/Automobile.cpp:o1525 | - | 1 | COSMETICO | línea en blanco borrada |
| vehicles/Automobile.cpp:1928-1972 | + | 45 | SVLIGHTS (R11) | luces de servicio por dummies: comentario + caché OdServiceCache |
| vehicles/Automobile.cpp:1973-1975 | + | 3 | SVLIGHTS (R11) | s_odSvcRetry (lógica sólo en web) |
| vehicles/Automobile.cpp:1976 | + | 1 | SVLIGHTS (R11) | blanco |
| vehicles/Automobile.cpp:2096-2117 | + | 22 | SVLIGHTS (R11) | ViceExtServiceLightWorldPos (camino cacheado) |
| vehicles/Automobile.cpp:2142-2146 | + | 5 | SVLIGHTS (R11) | ViceExtServiceLightWorldPos |
| vehicles/Automobile.cpp:2147-2157 | + | 11 | SVLIGHTS (R11) | reintento cada 5 s (sólo __EMSCRIPTEN__, es lógica) |
| vehicles/Automobile.cpp:2158-2221 | + | 64 | SVLIGHTS (R11) | búsqueda de dummies servicelights_* y caché |
| vehicles/Automobile.cpp:2226-2229 | + | 4 | SVLIGHTS (R11) | rama sin dummies |
| vehicles/Automobile.cpp:2241-2243 | + | 3 | SVLIGHTS (R11) | return |
| vehicles/Automobile.cpp:2623-2628 | + | 6 | SVLIGHTS (R11) | tamaño/alcance de corona variables |
| vehicles/Automobile.cpp:2636-2649 | + | 14 | SVLIGHTS (R11) | MI_FIRETRUCK luces por dummies |
| vehicles/Automobile.cpp:2656-2667 | + | 12 | SVLIGHTS (R11) | MI_AMBULAN luces por dummies |
| vehicles/Automobile.cpp:2674-2688 | + | 15 | SVLIGHTS (R11) | MI_POLICE luces por dummies |
| vehicles/Automobile.cpp:2733-2738 | + | 6 | SVLIGHTS (R11) | sólo pasa a mundo las fijas |
| vehicles/Automobile.cpp:o2145-2146 | - | 2 | SVLIGHTS (R11) | transformación vanilla |
| vehicles/Automobile.cpp:2748 | + | 1 | SVLIGHTS (R11) | corona con tamaño/alcance variable |
| vehicles/Automobile.cpp:o2156 | - | 1 | SVLIGHTS (R11) | vanilla 0.4/50 |
| vehicles/Automobile.cpp:2756 | + | 1 | SVLIGHTS (R11) | corona variable |
| vehicles/Automobile.cpp:o2164 | - | 1 | SVLIGHTS (R11) | vanilla |
| vehicles/Automobile.cpp:2762 | + | 1 | SVLIGHTS (R11) | UpdateCoronaCoors alcance variable |
| vehicles/Automobile.cpp:o2170 | - | 1 | SVLIGHTS (R11) | vanilla |
| vehicles/Automobile.cpp:2769-2777 | + | 9 | SIRENA_FBICAR (P5) | case MI_FBICAR en coronas de sirena |
| vehicles/Automobile.cpp:2781-2784 | + | 4 | SIRENA_FBICAR (P5) | posición luz FBIRANCH/FBICAR; se quita el chequeo de cámara detrás |
| vehicles/Automobile.cpp:o2180-2182 | - | 3 | SIRENA_FBICAR (P5) | posición y condición vanilla |
| vehicles/Automobile.cpp:2836-2837 | + | 2 | RESTO_NEUTRO | forceLightsOff=false (resto sin efecto) |
| vehicles/Automobile.cpp:o2234 | - | 1 | RESTO_NEUTRO | vanilla |
| vehicles/Automobile.cpp:3704-3705 | + | 2 | RESTO_NEUTRO | exitVehicleHeld (refactor sin efecto) |
| vehicles/Automobile.cpp:o3098 | - | 1 | RESTO_NEUTRO | vanilla |
| vehicles/Automobile.cpp:3910-3933 | + | 24 | VICEEXT_TANK_FIRE (no usa la bandera) | tanque de la IA dispara al jugador (features.ini, defecto ON) |
| vehicles/Automobile.cpp:3935-3942 | + | 7 | VICEEXT_TANK_FIRE (no usa la bandera) | explosión + camino vanilla |
| vehicles/Automobile.cpp:o3303 | - | 1 | VICEEXT_TANK_FIRE (no usa la bandera) | condición vanilla |
| vehicles/Automobile.cpp:4498-4503 | + | 6 | VICEEXT_DRIVEBY_WIDE (P2/P4) | fireHeld adelantado |
| vehicles/Automobile.cpp:4519-4529 | + | 11 | VICEEXT_DRIVEBY_WIDE (P2/P4) | puertas reordenadas + CanDoDriveByWithCurrentWeapon |
| vehicles/Automobile.cpp:o3859-3866 | - | 8 | VICEEXT_DRIVEBY_WIDE (P2/P4) | puertas vanilla (slot 5) |
| vehicles/Automobile.cpp:4556 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | usa fireHeld |
| vehicles/Automobile.cpp:4560-4563 | + | 4 | VICEEXT_DRIVEBY_WIDE (P2/P4) | cadencia por arma (GetDriveByShotDelay) |
| vehicles/Automobile.cpp:4567 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | m_nTimer + shotDelay |
| vehicles/Automobile.cpp:o3908-3910 | - | 3 | VICEEXT_DRIVEBY_WIDE (P2/P4) | disparo vanilla 70 ms fijos |
| vehicles/Automobile.cpp:4767-4770 | + | 4 | VICEEXT_NO_FLIP_BURN (no usa la bandera) | features.ini VehiclesDontCatchFireWhenTurningOver (defecto ON) |
| vehicles/Automobile.cpp:4774-4779 | + | 6 | VICEEXT_NO_FLIP_BURN (no usa la bandera) | camino vanilla en el else |
| vehicles/Automobile.cpp:o4110-4113 | - | 4 | VICEEXT_NO_FLIP_BURN (no usa la bandera) | daño por vuelco vanilla |
| vehicles/Bike.cpp:85-99 | + | 13 | MOTOS_MOD | grupo de animación de las 4 motos del mod |
| vehicles/Bike.cpp:1456-1457 | + | 2 | RESTO_NEUTRO | forceLightsOff=false |
| vehicles/Bike.cpp:o1333 | - | 1 | RESTO_NEUTRO | vanilla |
| vehicles/Bike.cpp:1941-1942 | + | 2 | RESTO_NEUTRO | exitVehicleHeld |
| vehicles/Bike.cpp:o1817 | - | 1 | RESTO_NEUTRO | vanilla |
| vehicles/Bike.cpp:2138-2141 | + | 4 | VICEEXT_DRIVEBY_WIDE (P2/P4) | fireHeld adelantado |
| vehicles/Bike.cpp:2157-2167 | + | 11 | VICEEXT_DRIVEBY_WIDE (P2/P4) | puertas + CanDoDriveByWithCurrentWeapon |
| vehicles/Bike.cpp:o2012-2019 | - | 8 | VICEEXT_DRIVEBY_WIDE (P2/P4) | puertas vanilla |
| vehicles/Bike.cpp:2169 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | usa fireHeld |
| vehicles/Bike.cpp:o2036 | - | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | vanilla |
| vehicles/Bike.cpp:2203 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | usa fireHeld |
| vehicles/Bike.cpp:2206-2208 | + | 3 | VICEEXT_DRIVEBY_WIDE (P2/P4) | cadencia por arma |
| vehicles/Bike.cpp:2210 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | m_nTimer + shotDelay |
| vehicles/Bike.cpp:o2070-2072 | - | 3 | VICEEXT_DRIVEBY_WIDE (P2/P4) | disparo vanilla 70 ms |
| vehicles/Boat.cpp:1418-1421 | + | 4 | VICEEXT_DRIVEBY_WIDE (P2/P4) | fireHeld adelantado |
| vehicles/Boat.cpp:1437-1447 | + | 11 | VICEEXT_DRIVEBY_WIDE (P2/P4) | puertas + CanDoDriveByWithCurrentWeapon |
| vehicles/Boat.cpp:o1417-1424 | - | 8 | VICEEXT_DRIVEBY_WIDE (P2/P4) | puertas vanilla |
| vehicles/Boat.cpp:1467 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | usa fireHeld |
| vehicles/Boat.cpp:1469-1471 | + | 3 | VICEEXT_DRIVEBY_WIDE (P2/P4) | cadencia por arma |
| vehicles/Boat.cpp:1473 | + | 1 | VICEEXT_DRIVEBY_WIDE (P2/P4) | m_nTimer + shotDelay |
| vehicles/Boat.cpp:o1459-1461 | - | 3 | VICEEXT_DRIVEBY_WIDE (P2/P4) | disparo vanilla 70 ms |
| vehicles/Vehicle.h:342-344 | + | 3 | FindDummyFrame | API CVehicle::FindDummyFrame sin guarda |
| vehicles/Vehicle.cpp:35 | + | 1 | FindDummyFrame | include NodeName.h |
| vehicles/Vehicle.cpp:1176-1183 | + | 8 | ARMAS_MOD | InflictDamage cases armas mod |
| vehicles/Vehicle.cpp:1216 | + | 1 | ARMAS_MOD | case BERETTA |
| vehicles/Vehicle.cpp:1220 | + | 1 | ARMAS_MOD | case DESERT_EAGLE |
| vehicles/Vehicle.cpp:1228 | + | 1 | ARMAS_MOD | case SHOTGUN2 |
| vehicles/Vehicle.cpp:1236 | + | 1 | ARMAS_MOD | case UZIOLD |
| vehicles/Vehicle.cpp:1241-1243 | + | 3 | ARMAS_MOD | cases AK47/M16/STEYR |
| vehicles/Vehicle.cpp:1450-1479 | + | 30 | FindDummyFrame | búsqueda recursiva de frame por nombre |
| vehicles/HandlingMgr.cpp:118-127 | + | 10 | COCHES_MOD | nombres de handling de los 8 vehículos del mod |
| vehicles/HandlingMgr.cpp:o118 | - | 1 | COCHES_MOD | fin de lista vanilla |
| vehicles/HandlingMgr.cpp:392 | + | 1 | COCHES_MOD | IsBikeHandling |
| vehicles/HandlingMgr.cpp:o383 | - | 1 | COCHES_MOD | rango vanilla de motos |
| vehicles/HandlingMgr.h:116-127 | + | 12 | COCHES_MOD | enum HANDLING_* del mod |
| vehicles/HandlingMgr.h:130-131 | + | 2 | COCHES_MOD | NUMBIKEHANDLINGS ampliado |
| vehicles/HandlingMgr.h:o118 | - | 1 | COCHES_MOD | NUMBIKEHANDLINGS vanilla |
| vehicles/HandlingMgr.h:136-152 | + | 17 | COCHES_MOD | BikeHandlingSlot/IsBikeHandling |
| vehicles/HandlingMgr.h:303-308 | + | 6 | COCHES_MOD | GetBikePointer por slot |
| vehicles/HandlingMgr.h:o273 | - | 1 | COCHES_MOD | GetBikePointer vanilla |
| modelinfo/VehicleModelInfo.cpp:199-250 | + | 52 | SVLIGHTS (R11b) | clonado de subárbol completo de extras |
| modelinfo/VehicleModelInfo.cpp:o203 | - | 1 | SVLIGHTS (R11b) | variable vanilla |
| modelinfo/VehicleModelInfo.cpp:264 | + | 1 | SVLIGHTS (R11b) | ViceExtCloneComponent comp1 |
| modelinfo/VehicleModelInfo.cpp:o213-219 | - | 7 | SVLIGHTS (R11b) | clon vanilla de atomic |
| modelinfo/VehicleModelInfo.cpp:271 | + | 1 | SVLIGHTS (R11b) | ViceExtCloneComponent comp2 |
| modelinfo/VehicleModelInfo.cpp:o226-232 | - | 7 | SVLIGHTS (R11b) | clon vanilla de atomic |
| modelinfo/ModelIndices.h:469-495 | + | 27 | ARMAS_MOD/COCHES_MOD | MI_* armas 6660-6669 y vehículos 6500-6507 |
| modelinfo/SimpleModelInfo.cpp:118-123 | + | 6 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | +20 m de LOD |
| modelinfo/SimpleModelInfo.cpp:o118 | - | 1 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | vanilla |
| modelinfo/SimpleModelInfo.cpp:129 | + | 1 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | GetNearDistance +20 m |
| modelinfo/SimpleModelInfo.cpp:o124 | - | 1 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | vanilla |
| modelinfo/SimpleModelInfo.cpp:136 | + | 1 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | comentario |
| modelinfo/SimpleModelInfo.cpp:141 | + | 1 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | GetLargestLodDistance +20 m |
| modelinfo/SimpleModelInfo.cpp:o135 | - | 1 | VICEEXT_LOD_EXTRA (valor usado sin #ifdef) | vanilla |
| objects/Stinger.cpp:39 | + | 1 | STINGER_FIXES (mod 2.0 :2309) | memset del objeto en el ctor |
| objects/Stinger.cpp:o39 | - | 1 | STINGER_FIXES (mod 2.0 :2309) | ctor vanilla |
| objects/Stinger.cpp:100 | + | 1 | STINGER_FIXES (mod 2.0 :2309) | Remove reinicia m_nSpikeState |
| control/CarCtrl.cpp:930-960 | + | 31 | TRAFICO_PONDERADO (D4, sólo __EMSCRIPTEN__) | ChooseCarModelToLoad pondera por m_frequency |
| control/CarAI.cpp:216 | + | 1 | FPS_LOGICO | GetLogicalFrameCounter en vez de GetFrameCounter |
| control/CarAI.cpp:o192 | - | 1 | FPS_LOGICO | vanilla |
| control/CarAI.cpp:276 | + | 1 | FPS_LOGICO | GetLogicalFrameCounter |
| control/CarAI.cpp:o252 | - | 1 | FPS_LOGICO | vanilla |
| control/Pickups.cpp:61-71 | + | 11 | ARMAS_MOD | AmmoForWeapon(_OnStreet) TOTALALLTYPES |
| control/Pickups.cpp:o61-62 | - | 2 | ARMAS_MOD | vanilla |
| control/Pickups.cpp:73 | + | 1 | ARMAS_MOD | CostOfWeapon TOTALALLTYPES |
| control/Pickups.cpp:o64-67 | - | 4 | ARMAS_MOD | vanilla |
| control/Pickups.cpp:75-81 | + | 7 | ARMAS_MOD | precios armas mod |
| control/Pickups.cpp:o69-70 | - | 2 | ARMAS_MOD | vanilla |
| control/Pickups.cpp:86 | + | 1 | ARMAS_MOD | aPickupColors dimensionado |
| control/Pickups.cpp:o75 | - | 1 | ARMAS_MOD | vanilla |
| control/Pickups.cpp:130-146 | + | 17 | ARMAS_MOD | colores armas mod |
| control/Pickups.cpp:o119 | - | 1 | ARMAS_MOD | vanilla |
| control/Pickups.cpp:397 | + | 1 | ARMAS_MOD | IsWeaponType |
| control/Pickups.cpp:o370 | - | 1 | ARMAS_MOD | vanilla |
| control/Pickups.cpp:400 | + | 1 | ARMAS_MOD | IsWeaponType |
| control/Pickups.cpp:o373 | - | 1 | ARMAS_MOD | vanilla |
| control/Pickups.h:122-124 | + | 3 | ARMAS_MOD | externs TOTALALLTYPES |
| control/Pickups.h:o122-124 | - | 3 | ARMAS_MOD | vanilla |
| control/Darkel.cpp:322 | + | 1 | ARMAS_MOD | IsWeaponType |
| control/Darkel.cpp:o322 | - | 1 | ARMAS_MOD | vanilla |
| control/Darkel.cpp:409 | + | 1 | ARMAS_MOD | IsWeaponType |
| control/Darkel.cpp:o409 | - | 1 | ARMAS_MOD | vanilla |
| control/Darkel.cpp:412 | + | 1 | ARMAS_MOD | IsWeaponType |
| control/Darkel.cpp:o412 | - | 1 | ARMAS_MOD | vanilla |
| control/Darkel.cpp:424 | + | 1 | ARMAS_MOD | IsWeaponType |
| control/Darkel.cpp:o424 | - | 1 | ARMAS_MOD | vanilla |
| renderer/Hud.cpp:2 | + | 1 | FEATURES_INI | include ViceExtFeatures.h (dinero y estrellas leidos de features.ini sin guarda) |
| renderer/Hud.cpp:391 | + | 1 | AIM_CLASSICAXIS | odAimLaw = CCamera::s_viceExtAimLawActive (activa la cruz con la ley de apuntado) |
| renderer/Hud.cpp:408-418 | + | 10 | WEAPON_SIGHTS / armas del mod | rango de armas con cruz PC ampliado (COLT45..ROCKETLAUNCHER + BERETTA..GRENADE_LAUNCHER del enum del mod) |
| renderer/Hud.cpp:o253-255 | - | 2 | WEAPON_SIGHTS / armas del mod | condicion vanilla de armas con cruz PC sustituida |
| renderer/Hud.cpp:454 | + | 1 | AIM_CLASSICAXIS | la condicion de pintar cruz incluye odAimLaw |
| renderer/Hud.cpp:o262 | - | 1 | AIM_CLASSICAXIS + NADO | condicion vanilla (DrawCrossHair // DrawCrossHairPC) sustituida por 454-455 |
| renderer/Hud.cpp:492-494 | + | 3 | AIM_CLASSICAXIS | cruz 3a persona tambien con ley de apuntado + ViceExtIsAiming() |
| renderer/Hud.cpp:o272 | - | 1 | AIM_CLASSICAXIS | condicion vanilla Using3rdPersonMouseCam sustituida |
| renderer/Hud.cpp:500-508 | + | 8 | WEAPON_SIGHTS | caja de la cruz 3a persona reducida 0.6 -> 0.42 |
| renderer/Hud.cpp:o279-282 | - | 4 | WEAPON_SIGHTS | tamano vanilla 0.6 |
| renderer/Hud.cpp:518-521 | + | 4 | WEAPON_SIGHTS | caja reducida 0.4 -> 0.26 |
| renderer/Hud.cpp:o288-291 | - | 4 | WEAPON_SIGHTS | tamano vanilla 0.4 |
| renderer/Hud.cpp:813-820 | + | 8 | MONEY_NO_ZEROS (FEATURES_INI) | dinero sin ceros via ViceExtFeatures::On, sin #ifdef VICEEXT_MONEY_NO_ZEROS |
| renderer/Hud.cpp:o423 | - | 1 | MONEY_NO_ZEROS | sprintf vanilla $%08d sustituido |
| renderer/Hud.cpp:941 | + | 1 | FIX_SILENTPATCH F3 (bajo FIX_BUGS) | VERTEXALPHAENABLE antes del icono de arma |
| renderer/Hud.cpp:1063-1068 | + | 6 | HIDE_STARS (FEATURES_INI) | estrellas ocultas sin busqueda via ViceExtFeatures, sin #ifdef VICEEXT_HIDE_STARS |
| renderer/Hud.cpp:1080-1081 | + | 2 | HIDE_STARS (FEATURES_INI) | cierre y condicion && !odHideStars |
| renderer/Hud.cpp:o633 | - | 1 | HIDE_STARS | condicion vanilla de estrellas sustituida |
| renderer/Hud.cpp:1601-1614 | + | 4 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | SetScriptSpriteScale alrededor de los sprites de script (Draw) |
| renderer/Hud.cpp:1944-1952 | + | 4 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | idem en DrawAfterFade |
| renderer/Hud.cpp:o1720 | - | 1 | WEAPON_SIGHTS (colateral) | borrado m_pLastZoneName = nil; de CHud::Initialise al insertar el bloque de miras |
| core/Frontend.h:173 | + | 1 | AUTOSAVE | SAVESLOT_9 anadido al enum sin guarda |
| core/Frontend.cpp:941-942 | + | 2 | mods-fix MouseSensNewGame (agente B, lote mods) | reset de aceleracion del raton al empezar partida |
| core/Frontend.cpp:1081 | + | 1 | AUTOSAVE | rango de ranuras hasta SAVESLOT_9 |
| core/Frontend.cpp:o1066 | - | 1 | AUTOSAVE | rango vanilla hasta SAVESLOT_8 |
| core/Frontend.cpp:1126-1139 | + | 11 | AUTOSAVE | nombre/fecha por numero de ranura (no por posicion), hasta SAVESLOT_9 |
| core/Frontend.cpp:o1111-1122 | - | 6 | AUTOSAVE | lectura vanilla por indice i |
| core/Frontend.cpp:1157-1160 | + | 4 | AUTOSAVE | FEM_SL%d por odSlot (rama no-autosave) |
| core/Frontend.cpp:1459 | + | 1 | AUTOSAVE | rango hasta SAVESLOT_9 |
| core/Frontend.cpp:o1421 | - | 1 | AUTOSAVE | rango vanilla |
| core/Frontend.cpp:1539 | + | 1 | AUTOSAVE | rango hasta SAVESLOT_9 |
| core/Frontend.cpp:o1501 | - | 1 | AUTOSAVE | rango vanilla |
| core/Frontend.cpp:2478-2498 | + | 14 | FIX_SILENTPATCH :2384 OutroSplashFix | clamp del alfa del fade del menu (Min(+20,255)) |
| core/Frontend.cpp:o2447 | - | 1 | FIX_SILENTPATCH :2384 OutroSplashFix | incremento vanilla sin clamp |
| core/Frontend.cpp:4822 | + | 1 | AUTOSAVE | rango hasta SAVESLOT_9 |
| core/Frontend.cpp:o4765 | - | 1 | AUTOSAVE | rango vanilla |
| core/Frontend.cpp:5150-5153 | + | 2 | AUTOSAVE (bajo FIX_BUGS) | rango hasta SAVESLOT_9 |
| core/Frontend.cpp:o5022-5025 | - | 2 | AUTOSAVE | rango vanilla |
| core/Frontend.cpp:5588-5590 | + | 3 | AUTOSAVE / MISSION_REPLAY | ranura del quicksave de mision = PAUSE_SAVE_SLOT (fichero 9) en vez de SLOT_COUNT |
| core/Frontend.cpp:o5460 | - | 1 | AUTOSAVE / MISSION_REPLAY | m_nCurrSaveSlot = SLOT_COUNT vanilla |
| core/Frontend.cpp:5917-5957 | + | 17 | FIX_SILENTPATCH :4086 outro 2,5 s | tick 10->32 ms, cuenta 150->75 (y MUCH_SHORTER_OUTRO_SCREEN desactivado en config.h) |
| core/Frontend.cpp:o5769-5793 | - | 3 | FIX_SILENTPATCH :4086 outro 2,5 s | constantes vanilla |
| core/MenuScreensCustom.cpp:o503-505 | - | 3 | AUTOSAVE | filas FEM_SL6/7/8 de la carga: SL8 desaparece (SL6+SL7 quedan en una linea) |
| core/MenuScreensCustom.cpp:o559 | - | 1 | AUTOSAVE | fila FEM_SL8 de guardar eliminada |
| core/MenuScreensCustom.cpp:473-479 | + | 7 | AUTOSAVE | lista de carga sin SL8 (SL6+SL7 en una linea) + comentario |
| core/MenuScreensCustom.cpp:526-527 | + | 2 | AUTOSAVE | lista de guardar sin SL8 (comentario) |
| core/Radar.cpp:22 | + | 1 | GPS_COLOR (FEATURES_INI) | include ViceExtFeatures.h |
| core/Radar.cpp:1106-1128 | + | 7 | GPS_COLOR (FEATURES_INI) | color del waypoint desde features.ini / VICEEXT_GPS_COLOR_* (vanilla 255,72,77), bajo MAP_ENHANCEMENTS |
| core/Radar.cpp:o1103-1133 | - | 8 | GPS_COLOR | #define/#undef WAYPOINT_* vanilla y su uso |
| core/Radar.cpp:1683 | + | 1 | mods-fix blip DOWN (agente B) | triangulo de la leyenda abajo corregido |
| core/Radar.cpp:o1670 | - | 1 | mods-fix blip DOWN | poligono vanilla |
| renderer/Sprite2d.cpp:16-21 | + | 6 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | estado y setter SetScriptSpriteScale |
| renderer/Sprite2d.cpp:92-94 | + | 3 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | escala 640x448 en CSprite2d::Draw |
| renderer/Sprite2d.cpp:120 | + | 1 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | SetVertices con el rect escalado |
| renderer/Sprite2d.cpp:o86 | - | 1 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | SetVertices vanilla |
| renderer/Sprite2d.cpp:366-369 | + | 4 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | escala en DrawRect |
| renderer/Sprite2d.cpp:o332 | - | 1 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | SetVertices vanilla |
| renderer/Sprite2d.h:24 | + | 1 | FIX_SILENTPATCH :1999 ScriptSpritesScaling | declaracion SetScriptSpriteScale |
| core/Pad.cpp:296-321 | + | 26 | EXTRA_CHEATS (CRAZYTOOLS) | WeaponCheat4 definida sin guarda (solo se llama bajo VICEEXT_EXTRA_CHEATS) |
| core/Pad.cpp:351-366 | + | 16 | EXTRA_CHEATS (CRAZYTOOLS) | resto de WeaponCheat4 |
| core/ControllerConfig.h:62 | + | 1 | FIRST_PERSON | PED_TOGGLE_1RST_PERSON ocupa el hueco _CONTROLLERACTION_36 |
| core/ControllerConfig.h:o62 | - | 1 | FIRST_PERSON | _CONTROLLERACTION_36 vanilla |
| core/ControllerConfig.h:67-70 | + | 4 | MANUAL_RELOAD / ALT_SIT (WalkKey) | PED_RELOAD sustituye UNKNOWN_ACTION; PED_WALK nueva accion |
| core/ControllerConfig.h:o67 | - | 1 | MANUAL_RELOAD | UNKNOWN_ACTION vanilla |
| core/ControllerConfig.h:192-202 | + | 11 | FIRST_PERSON / CROUCH / SKIP_PHONE_CALL | declaracion ViceExtActionKeyJustDown |
| core/ControllerConfig.cpp:199 | + | 1 | SKIP_PHONE_CALL | VEHICLE_ENTER_EXIT por defecto F en teclado (ENTER queda libre) |
| core/ControllerConfig.cpp:o199-200 | - | 2 | SKIP_PHONE_CALL | ENTER teclado + F extra vanilla |
| core/ControllerConfig.cpp:290-313 | + | 24 | FIRST_PERSON / MANUAL_RELOAD / ALT_SIT | V quitada de cambiar camara; defaults V (1a persona), R (recarga), LALT (andar) |
| core/ControllerConfig.cpp:o291 | - | 1 | FIRST_PERSON | V extra de CAMERA_CHANGE_VIEW_ALL_SITUATIONS |
| core/ControllerConfig.cpp:546-548 | + | 3 | FIRST_PERSON / MANUAL_RELOAD / ALT_SIT | SETACTIONNAME de las 3 acciones nuevas |
| core/ControllerConfig.cpp:1216-1233 | + | 18 | FIRST_PERSON / CROUCH / SKIP_PHONE_CALL | definicion ViceExtActionKeyJustDown (tecla con respaldo) |
| core/ControllerConfig.cpp:2030-2032 | + | 3 | FIRST_PERSON / MANUAL_RELOAD / ALT_SIT | GetActionType de las 3 acciones nuevas |
| core/ViceExtFeatures.h:1-37 | + | 37 | FEATURES_INI | API declarada sin guarda (las definiciones solo existen con VICEEXT_FEATURES_INI) |
| core/Wanted.cpp:33-37 | + | 5 | VICEEXT_HIDE_COPS | WANTED_SIGHT_RADIUS 40 m sin guarda (lo usan la mecanica y la traza) |
| core/Wanted.cpp:102-158 | + | 57 | VICEEXT_HIDE_COPS | WantedCopSeesPlayer sin guarda (usa WANTED_SIGHT_COS, definido SOLO bajo la guarda) |
| core/World.cpp:2-3 | + | 2 | VICEEXT_DISTANT_LIGHTS | includes ViceExtFeatures.h/Building.h |
| core/World.cpp:9 | + | 1 | VICEEXT_DISTANT_LIGHTS | include Dummy.h |
| core/World.cpp:374-376 | + | 3 | VICEEXT_NO_WHEEL_PIERCE | refactor chassisHit (equivalente sin la bandera) |
| core/World.cpp:o370-371 | - | 2 | VICEEXT_NO_WHEEL_PIERCE | if vanilla de ProcessLineOfSight sustituido por chassisHit |
| core/World.cpp:o376 | - | 1 | VICEEXT_NO_WHEEL_PIERCE | condicion de rueda vanilla movida a wheelWins |
| core/World.cpp:387 | + | 1 | VICEEXT_NO_WHEEL_PIERCE | if(wheelWins) |
| core/World.cpp:1945-1949 | + | 5 | VICEEXT_DISTANT_LIGHTS | CWorld::Process llama ProcessDistantLights SIN guarda (features.ini EnableDistantLights) |
| core/World.cpp:1951-1956 | + | 6 | VICEEXT_DISTANT_LIGHTS | bucle pool de dummies |
| core/World.cpp:1960-1968 | + | 9 | VICEEXT_DISTANT_LIGHTS | ProcessDistantLights dummies + bucle edificios |
| core/World.cpp:1972-1973 | + | 2 | VICEEXT_DISTANT_LIGHTS | ProcessDistantLights edificios |
| core/World.cpp:1993-1994 | + | 2 | VICEEXT_DISTANT_LIGHTS | cierre del bloque |
| core/World.cpp:2172-2174 | + | 2 | VICEEXT_FIX_FV | refactor odPhase en el chequeo de caidos (equivalente sin la bandera) |
| core/World.cpp:o2094-2096 | - | 2 | VICEEXT_FIX_FV | GetFrameCounter()&7 vanilla sustituido por odPhase |
| control/Script.cpp:13 | + | 1 | VICEEXT_SKIP_PHONE_CALL | include ControllerConfig.h |
| control/Script.cpp:o1552-1555 | - | 4 | VICEEXT_AUTOSAVE | borrado del autosave vanilla muerto (if 0) y pPlayerInfo |
| control/Script6.cpp:24 | + | 1 | PedArbiter (NADO) | include PedArbiter.h para bSwimAim |
| control/Script6.cpp:1007-1009 | + | 3 | SilentPatch 1610 + PEDLANE_NADO | IS_PLAYER_TARGETTING_CHAR exige camara de apuntado y no estar apuntando nadando |
| control/Script6.cpp:o1005 | - | 1 | SilentPatch 1610 + PEDLANE_NADO | condicion vanilla sustituida |
| renderer/ParticleMgr.cpp:189-193 | + | 5 | VICEEXT_PARTICLE_NAMES | quita CR final de linea en particle.cfg (inocuo con LF) |
| renderer/ParticleMgr.h:136-137 | + | 2 | VICEEXT_PARTICLE_NAMES | VALIDATE_SIZE 0x2FFC->0x32E0 (88 tipos) |
| renderer/ParticleMgr.h:o136 | - | 1 | VICEEXT_PARTICLE_NAMES | VALIDATE_SIZE vanilla |
| renderer/Particle.cpp:674-688 | + | 15 | VICEEXT_PARTICLE_NAMES | raster de los 5 tipos de particula del mod |
| renderer/ParticleType.h:88-102 | + | 13 | VICEEXT_PARTICLE_NAMES | 5 tipos nuevos en tParticleType, PARTICLE_LAST y PARTICLE_VICEEXT_FIRST |
| renderer/ParticleType.h:o88 | - | 1 | VICEEXT_PARTICLE_NAMES | blanco |
| renderer/ParticleType.h:o91 | - | 1 | VICEEXT_PARTICLE_NAMES | PARTICLE_LAST vanilla |
| renderer/SpecialFX.cpp:4 | + | 1 | VICEEXT_FEATURES_INI | include ViceExtFeatures.h |
| renderer/SpecialFX.cpp:391-395 | + | 5 | VICEEXT_FEATURES_INI | DisableBulletTraces en AddTrace(posiciones) |
| renderer/SpecialFX.cpp:449-453 | + | 5 | VICEEXT_FEATURES_INI | DisableBulletTraces en AddTrace(arma) |
| renderer/SpecialFX.cpp:481-482 | + | 2 | armas del mod | DESERT_EAGLE/SHOTGUN2 en trazas de bala gruesas |
| renderer/SpecialFX.cpp:492-494 | + | 3 | armas del mod | AK47/M16/STEYR en trazas de bala largas |
| buildings/Building.cpp:6-13 | + | 8 | VICEEXT_DISTANT_LIGHTS | includes de ProcessDistantLights |
| entities/Dummy.cpp:6-13 | + | 8 | VICEEXT_DISTANT_LIGHTS | includes de ProcessDistantLights |
| entities/Entity.cpp:418 | + | 1 | espacios | tabulador quitado en CEntity::Render (sin efecto) |
| entities/Entity.cpp:o418 | - | 1 | espacios | linea vanilla con tabulador |
| renderer/Shadows.cpp:2003-2005 | + | 3 | SilentPatch 1967 | CastShadowEntityXY usa tambien GetUp() |
| renderer/Shadows.cpp:o2003-2005 | - | 3 | SilentPatch 1967 | proyeccion vanilla sin eje Up |
| renderer/MBlur.cpp:o569 | - | 1 | SilentPatch 2196 | TODO vanilla borrado |
| renderer/MBlur.cpp:570 | + | 1 | SilentPatch 2196 | zona segura radar escalada con SCREEN_STRETCH |
| renderer/MBlur.cpp:o571 | - | 1 | SilentPatch 2196 | zona radar vanilla |
| renderer/MBlur.cpp:573 | + | 1 | SilentPatch 2196 | zona segura HUD superior escalada |
| renderer/MBlur.cpp:o574 | - | 1 | SilentPatch 2196 | zona HUD vanilla |
| extras/shaders/neoRimSkin.vert:42 | + | 1 | shader niebla | DoFog(gl_Position.w) como el resto de shaders neo |
| extras/shaders/neoRimSkin.vert:o42 | - | 1 | shader niebla | DoFog(gl_Position.z) vanilla |
| extras/shaders/obj/neoRimSkin_vert.inc:43 | + | 1 | shader niebla | idem en el .inc generado |
| extras/shaders/obj/neoRimSkin_vert.inc:o43 | - | 1 | shader niebla | idem vanilla |

---

## 7. DIAGNOSTICO

### 7.1 Volumen y dónde

- **5 970 líneas añadidas** (8 borradas).
- **Llamadas en `src/`:** 269 `ODTRACES/ODTRACEI` y 38 `VICEEXT_P4_EMIT`/`VICEEXT_MODSCRIPT_P4_EMIT`.
- **56 esquemas `P4 kind=`:** identity, overflow, modscript, ini, inifile, firewait, uzi, partcfg, worlddata, pose_*, move_*, input_*, hud2, gangaim, cardet, cardead, carluz, wheelrot, turners, stand_end, sight, shift, service, roll_*, radio2, polbike, particulas, partials, muro, moon, limits, legs, jump, jump_end, hud_cross, hint2, fire, distlights, crouchspeed, chofer, cheat2, cam_*, armsight, aimsens, aim…
- **Transporte:** `ODTRACES` (skel/ondemand.h:8-9) → `window.__odq` → `web/lib/index.js` → `/odtrace` → `web/odtrace.log`.

Por fichero:

| Fichero | Líneas DIAGNOSTICO | Llamadas ODTRACE*/P4_EMIT añadidas | Rangos |
|---|---:|---:|---|
| peds/PlayerPed.cpp | 2025 | 75 | 19, 33, 37, 191-283, 786-798, 1116-1129, 1426-1432, 1528-1530, 1567-2183, 2234-2258, 2641-2661, 2698-2700, 2721-2722, 2796-2815, 2989-3124, 3137-3179, 3220-3243, 3307-3317, 3417-3426, 3437-3451, 3470-3482, 3488, 3492, 3496, 3500, 3514-3516, 3528-3552, 3576-3589, 3644-3646, 3656-3658, 3664-3666, 3721-3744, 3757-3759, 3769-3771, 3808-3814, 3896-3901, 3927-3939, 3944-4038, 4158-4160, 4177-4179, 4190-4192, 4200-4202, 4217-4223, 4838-4856, 4893-4911, 4918-4922, 4954-5073, 5144-5146, 5153-5156, 5160-5168, 5174-5176, 5187-5188, 5203-5205, 5310-5324, 5344-5352, 5376-5403, 5424-5440, 5468-5661, 5684-5797, 6049-6067, 6083-6085, 6097-6099, 6123-6126, 6158-6167, 6173-6186, 6219-6240, 6267-6269, 6299, 6303-6314, 6331-6341, 6387-6417, 6479-6510, 6618-6625 |
| core/Cam.cpp | 348 | 15 | 26-51, 58, 68-76, 104-107, 122-130, 158-236, 249-256, 555, 565, 571, 1242-1254, 1795-1811, 2718-2763, 2780, 2791, 2798, 2906-2953, 4185-4186, 4570-4636, 6243-6259 |
| peds/Ped.cpp | 310 | 12 | 11, 72-93, 385-395, 1552-1578, 1685-1701, 1718-1744, 1762-1799, 5105-5179, 5222, 5229, 5234-5236, 5240-5244, 5253, 5260-5272, 5445, 5849-5885, 5919-5923, 9740-9766, 9798-9802 |
| weapons/Weapon.cpp | 278 | 9 | 24, 168-176, 183-186, 198-247, 306, 330-338, 345-350, 355-366, 373-375, 383-464, 486-487, 523-526, 552-561, 606-616, 918-925, 984-999, 1477-1489, 1514-1521, 1569-1576, 1610-1617, 1981-1988, 3139-3145, 4051-4055 |
| vehicles/Automobile.cpp | 237 | 11 | 6, 29, 58-80, 84, 1555, 1815-1817, 1830-1832, 1836-1838, 1850-1852, 1864-1867, 1871-1873, 1881-1883, 1892, 1904, 1912-1922, 1977-2049, 2083-2093, 2118-2141, 2222-2225, 2230-2240, 2598-2612, 3934, 4518, 4557-4559, 4564-4566, 4771-4773, 4799, 4954-4955, 4963-4967, 4983-4987, 4992-4996, 5052-5061 |
| core/Pad.cpp | 209 | 4 | 15-16, 46-50, 322-350, 367-433, 442-498, 806-817, 1529-1538, 1814-1829, 1835, 1840, 1845, 1850, 1855, 1860, 1867-1869, 1877-1879, 1885 |
| renderer/Hud.cpp | 205 | 13 | 21-24, 34, 316-357, 425-453, 713-757, 804-810, 881-912, 1069-1079, 2192-2198, 2205-2207, 2366-2389 |
| core/Streaming.cpp | 192 | 12 | 369-381, 389-398, 649-660, 679-702, 714-724, 739-749, 767-770, 1696-1717, 1914-1939, 1949-1962, 2419-2437, 2722-2725, 2749, 2778-2795, 3450-3453 |
| core/Wanted.cpp | 192 | 10 | 14, 19-32, 59-101, 159-242, 263-271, 606-613, 719-723, 760-763, 776-778, 787-790, 822-824, 866-873, 918-925 |
| core/Camera.cpp | 191 | 7 | 37, 118-141, 168-169, 480-504, 1265-1370, 1443-1449, 1486-1505, 4143-4148 |
| core/main.cpp | 191 | 7 | 3-5, 795-912, 951-957, 1004-1010, 1035-1048, 1072, 1081, 1885-1887, 1903-1905, 1909-1911, 1916-1920, 1926-1928, 1935-1940, 1971-1974, 2023-2025, 2063-2066, 2073-2077, 2085-2087 |
| audio/sampman_oal.cpp | 142 | 9 | 145-149, 167, 174-186, 268, 306, 311, 322-323, 345-353, 363, 435-455, 469-487, 1471-1473, 1855-1857, 2049-2054, 2058-2070, 2247-2256, 2421-2426, 2433-2435, 2441-2452, 2617-2631 |
| core/CdStreamPosix.cpp | 101 | 5 | 26, 123-199, 238-239, 254-261, 279-290, 854-856 |
| text/Messages.cpp | 85 | 5 | 14-21, 28-57, 233-235, 318-320, 342-344, 534-570, 814-816 |
| control/CarCtrl.cpp | 75 | 4 | 31-33, 658-672, 786-844 |
| core/Frontend.cpp | 67 | 7 | 469-477, 4836-4846, 4896-4902, 4920-4938, 5606-5618, 5736-5745 |
| skel/glfw/glfw.cpp | 67 | 8 | 30-32, 2246, 2359, 2367, 2451, 2457, 2470-2475, 2521-2531, 2616-2657 |
| audio/MusicManager.cpp | 64 | 4 | 26-38, 196-203, 575, 596, 615, 1149-1159, 1213-1215, 1410-1422, 1480-1492 |
| core/Game.cpp | 62 | 7 | 5, 409-413, 737-753, 1070, 1128-1130, 1141-1142, 1190-1206, 1260-1269, 1293, 1306-1309, 1314, 1319, 1364, 1379, 1653 |
| animation/CutsceneMgr.cpp | 59 | 6 | 196-204, 215-226, 268-277, 282-296, 364-370, 475-481 |
| renderer/Font.cpp | 54 | 1 | 10-11, 951-998, 1011-1014 |
| renderer/Coronas.cpp | 53 | 2 | 20, 93-119, 279-283, 350-351, 364, 437-453 |
| peds/CopPed.cpp | 52 | 4 | 22, 98-109, 176-182, 265-291, 486-490 |
| peds/PedFight.cpp | 50 | 4 | 29, 729-740, 773-784, 848-859, 918-930 |
| core/World.cpp | 48 | 4 | 29, 388-395, 1950, 1957-1959, 1969-1992, 2207-2209, 2217, 2322-2330 |
| text/Text.cpp | 48 | 4 | 2, 58-64, 147-161, 337-360, 465-467 |
| renderer/Renderer.cpp | 44 | 1 | 3, 838-874, 917-922 |
| vehicles/Bike.cpp | 37 | 1 | 40-41, 1318-1348, 2156, 2204-2205, 2209 |
| audio/AudioLogic.cpp | 35 | 2 | 5-7, 2932-2951, 4694-4705 |
| core/ViceExtFeatures.cpp | 32 | 2 | 5, 214-244 |
| rw/TexRead.cpp | 31 | 3 | 9-11, 101-110, 156-164, 192-200 |
| control/Script.cpp | 29 | 3 | 24-26, 904-914, 1030-1031, 1623-1635 |
| renderer/ParticleMgr.cpp | 27 | 3 | 6-7, 133, 158-166, 217, 223-225, 233-235, 240-241, 431-437 |
| save/GenericGameStorage.cpp | 27 | 4 | 20-22, 351-354, 358-365, 369-370, 592-601 |
| peds/PedAI.cpp | 26 | 3 | 2, 4905, 5661-5672, 5764-5775 |
| save/PCSave.cpp | 26 | 2 | 22-27, 57-64, 89-100 |
| fakerw/fake.cpp | 24 | 2 | 13-15, 311-332 |
| core/PedArbiter.cpp | 24 | 3 | 4, 47-53, 57-64, 84-91 |
| animation/AnimManager.cpp | 19 | 1 | 2, 1399-1402, 1429-1435, 1440-1446 |
| renderer/Clouds.cpp | 18 | 2 | 17, 155-171 |
| audio/AudioManager.cpp | 16 | 2 | 14-16, 82-84, 1384-1388, 1406-1410 |
| peds/Population.cpp | 16 | 1 | 31-33, 366-378 |
| control/CarAI.cpp | 15 | 2 | 10, 72-76, 574-582 |
| control/Script8.cpp | 15 | 11 | 32-33, 643, 682, 694, 706, 717, 726, 730, 734, 738, 742, 750, 761, 767 |
| renderer/Particle.cpp | 12 | 1 | 24-26, 887, 939-946 |
| core/Radar.cpp | 11 | 1 | 25-26, 1137-1145 |
| core/ZoneCull.cpp | 11 | 0 | 173-183 |
| vehicles/Heli.cpp | 10 | 1 | 30, 1022-1030 |
| control/Script6.cpp | 8 | 1 | 21, 1032-1038 |
| skel/ondemand.h | 8 | 0 | 1-4, 8-9, 27-28 |
| core/Wanted.h | 7 | 0 | 8-14 |
| core/ZoneCull.h | 7 | 0 | 54-60 |
| peds/Ped.h | 6 | 0 | 1013-1018 |
| peds/PlayerPed.h | 5 | 0 | 139-144 |
| weapons/Weapon.h | 5 | 0 | 42-44, 48-49 |
| core/FileLoader.cpp | 4 | 1 | 3, 53-55 |
| renderer/WaterLevel.cpp | 3 | 1 | 21, 135-136 |
| vehicles/Boat.cpp | 3 | 0 | 1436, 1468, 1472 |
| core/main.h | 2 | 0 | 62-63 |
| buildings/Building.cpp | 1 | 0 | 14 |
| entities/Dummy.cpp | 1 | 0 | 14 |

### 7.2 Etiquetas principales por zona

| Zona | Etiquetas |
|---|---|
| PlayerPed.cpp (2 030) | andamiaje P4 1567-2183 (617 líneas: identity, move_*, input_*, aim, pose_*, cam_*), AIMDIR/armsight, CROUCH2, PEDAT/WINFO, partials, reload, R25, stand_end, firewait, AX7, ODRUMBO, AIMLOCK/AIMIK/AIMLAW, R3P, fire, climb, roll_*, crouchspeed, AIMWPN, sprint, AIMWALK, ini; nado: SWIM* (173) |
| Cam.cpp (348) | BETASNAP, camauto2/CAMB2/CAMB2b/CAMB3/CAMB3b/CAMSA/CAMV2, AIMCAM/AIMVEC/AIMFOV/AIMCOL, RECOIL_* (98), statics `s_odLos*`/`s_odSph*`, SWIMCAM (47) |
| Peds/Camera.cpp (675) | CAM1P/1p bind/key/set, DRIVEBY, P4 move_apply/firewait/jump/legs/aimsens/ini/gangaim/limits, WANTEDCOP, AIMCFG, PEDCLAIM, ODCROUCHDIR |
| Infra web (627) | FPHASE (158), ODSHORT/ODSRECOVER (97), CAR*/TXDIN/FAIL (152), WEBHB (42), FPSLOG, webload, ODMARK, LBAR/LOADSCR, loadtick, worlddata |
| Save/texto/anim/render (334) | CUT, CORONA/CORONAREND, TXTMISS/TXTGXTFAIL/MSGTABLE, LODLEFT, SKIP tex, save, hackRead, IFPFILE, WATER |
| Audio (257) | ODSFX*/SFXLEN/CHINIT/STREAM preload/RADIOTRACK/SFXDATA, SIRENA, VICEEX sfx, RETUNE, RADIOLOGOS, radio2, LOOPORPHAN/LOOPKILL |
| Vehículos/armas (660) | RECOIL_* (~190 en Weapon.cpp, `window.__odRecoilEmit`, `OD_RECOIL_SCHEMA_1`), uzi, ini, turners, service/SVLIGHTS, wheelrot, cardet/cardead/carluz, chofer, polbike, BSIREN, CARSPAWN/CARZONE/CARRATE, DRIVEBY, gastank, luces rota |
| HUD/entrada (663) | SCRTXT/SCRTXT3, SIGHT/SIGHTS, SWIMHUD, hud_cross, AIMHUD/AIHTRI/hud2, HUDICON, KEYICONS/HINTKEY/hint2/shift, FEMENU, WR newgame-req/load-req, WLOAD, HINTTEST, cheat2, inifile; **cheats de prueba CRAZYPISTOL/CRAZYRIDES/CRAZYHINT/CRAZYCOP** |
| Mundo/script (376) | WANTED*/WANTEDHIDE*/WANTEDCHANGE/PURGE/SUSPEND, SHOT rueda, distlights, muro, CALL skip, SCMLOOP, autosave, TGTGUN, modscript, partcfg, particulas, moon, worlddata (todo `VICEEXT_MAPDATA` es traza) |

### 7.3 Trazas que NO hay que quitar a ciegas

- **Las necesita el arnés (INFRA de verificación):**
  - `FPHASE` y `FPSLOG` (main.cpp:795-957, Streaming.cpp:389-398);
  - `WR load-req` (Frontend.cpp:5606-5618);
  - `LOADSCR` (main.cpp:1004-1081);
  - `JS build=` (web/ondemand.js:378).
- **Transporte:** `ODTRACES/ODTRACEI` y `odBlockingPush/Pop` de `skel/ondemand.*`. `odBlockingPush` es INFRA-WEB: evita aplazar ficheros durante `LoadAllRequestedModels`.
- **Del nado y la radio** (si se quieren mantener los tests): `SWIM2`, `SWIMCAM`, `PEDCLAIM`, `PEDAT` (**hoy bajo VICEEXT_CROUCH**) y `P4 kind=radio2`.

### 7.4 Trazas que se ejecutan siempre o cambian el juego (retirar primero)

- Wanted.cpp: `WANTED*` sin guarda. Cada segundo lanza un `ProcessLineOfSight` por cada policía perseguidor, incluso sin HIDE_COPS.
- Script6.cpp:1032-1038: `TGTGUN` escribe una línea en cada ejecución del opcode.
- Pad.cpp:367-498, 1529-1538, 1814-1829: los cheats de prueba **CRAZYPISTOL** (da la Beretta), **CRAZYRIDES** (crea los 8 coches del mod), **CRAZYHINT** y **CRAZYCOP** funcionan en cualquier build. `WeaponCheat4`/CRAZYTOOLS (Pad.cpp:296-366) es SIN-GUARDA.
- Coronas.cpp: contadores sin guarda.
- fake.cpp:311-319: `hackRead` tiene un `return` temprano anti-nil ("quitar con el fix raíz").

---

## 8. INFRA-WEB (se queda): resumen por subsistema

Total: **3 337 líneas añadidas + 831 borradas**. Filas completas en el Anexo C.

| Subsistema | Sitios principales |
|---|---|
| Bucle principal y glfw web | glfw.cpp:2011-2681, 2897-2910 (main troceado + `emscripten_set_main_loop`, −o2081-2466); arranque con splash; `window.__vcState/__vcInGame/__vcFrame`; tope de fps 35/120 (**también afecta al nativo GL3**); canvas y modos de vídeo; joystick; filtro de auto-repeat 1697-1708 |
| On-demand | skel/ondemand.cpp:1-129, ondemand.h (`__wrap_fopen/open/stat/access`, `od_fetch` con Asyncify, `odBlockingPush/Pop`) |
| CdStream | CdStreamPosix.cpp: modo monohilo sin semáforos (320-542); imágenes como directorio suelto con `.dir` (111-122, 200-300, 770-847, 887-894) |
| Streaming web | Streaming.cpp: tráfico sin `nreq<5` y con reloj real, enfriamiento de re-solicitud, reintentos acotados, `gWebLoadBudget`, `LoadSceneStep` troceado; Streaming.h:212-225 |
| Carga troceada y pantalla de carga | Game.cpp:397-1259 (`InitialiseStep`, `ShutDownForRestartStep`, `InitialiseRestartStep`), main.cpp (`WebDrawLoadScreen`, `InitialiseGameStep`), main.h, Game.h |
| IDBFS y guardado | glfw.cpp:185-191, re3.cpp:197-205, skeleton.cpp:221-237, PCSave.cpp, GenericGameStorage.cpp (flush, slot y carga diferida; **borra `LoadSceneCollision` o476 sin guarda**), Frontend.cpp:3342-3347 |
| GXT robusto | Text.cpp:26-497 (−o28-419), Text.h (sin guarda: también nativo) |
| Audio web | sampman_oal.cpp (SFX on-demand mp3, caché de 96 MB, OpenAL sin EFX), oal/*, AudioManager.cpp (watchdog de loops huérfanos), audio de misión bloqueante, MusicManager con longitud 0, config.h:615-627 (sin `PS2_AUDIO_PATHS` ni `MULTITHREADED_AUDIO`) |
| Cinemáticas | CutsceneMgr.h:3-20, 49; CutsceneMgr.cpp:190, 230-258, 671 |
| Arreglos de crash | TexRead.cpp, AnimBlendAssocGroup.cpp:182-196, AnimManager.cpp:1475-1493 y 1638-1654, WaterLevel.cpp:127-139, PedAI.cpp:4889-4911, Stinger.cpp:124-202 (nil-checks), re3.cpp:1323-1338, Script.cpp:1019-1039 (vigilante SCM) |
| Build | CMakeLists.txt:55-68; config.h:667-675 (`USE_UNNAMED_SEM`, sin `USE_TXD_CDIMAGE`) |

---

## 9. Tooling de verificación que depende de las trazas viejas

| Fichero | Qué consume | Qué se rompe al quitar lo viejo (preservando nado y radio) |
|---|---|---|
| `gta_vc_browser/tools/check-served-build.sh` (161 marcas, `MARCAS=(` :25-185) | `grep -a` de cadenas literales en el wasm/js servido. Reparto: 132 MOD-VIEJO, 19 NADO (+`camz=`), 4 dependencias del nado en código viejo (`PEDCLAIM cap=`, `ViceExtPedOwns` → PedArbiter; `PEDAT x=`, `dist=` → CROUCH), 1 RADIO (`P4 kind=radio2 schema=1`, :165), 2 INFRA (`LOADSCR begin`, `ODSTA`), 2 andamiaje P4 (`identity`, `overflow`). Además: hashes de los datos del mod (:192-200), `COLFILE` (:311) y `ve13` (:275) | ~134 `FALTA` y exit 1. Hay que podar `MARCAS` hasta dejar NADO, RADIO, INFRA y lo que se reimplemente |
| `gta_vc_browser/tools/viceext-log-check.py` (4 496 líneas) | Bloques D2 (TXTMISS), D4 (CAR*, ODSHORT), D5 (IFPFILE: 272/274 clips del ped.ifp del mod, **dependencia del nado**), D6 (SIGHT*), D7 (KEYICONS, HINTKEY, shift), D8 (VICEEX sfx, ODSFXMISS, CHINIT), R2 (FPHASE, ODSTA, STREAM preload), R5 (1p), R9/AX (AIM*), R11 (SVLIGHTS), LB, SR, L2, L6 (LOADSCR), **H/R14/R17/R19** (CROUCH2 + **SWIM2/SWIMCAM/SWIMEF/SWIMFOV/SWIMHUD/SWIMIDLE/PEDCLAIM/PEDAT**), RC (RECOIL_*), R12/R15/R16/R27, `--plan-4` P4.1-P4.16 (move/pose/input/roll/cam/…; **P4.11 radio/radio2**; cheat2; cardet; ini/inifile; worlddata; identity/overflow) | `main()` sólo da exit 0 con D2, D4, D5, D6, D7 y D8 en OK. D6, D7 y D8 son del mod viejo. H/R14/R17/R19 mezclan nado y agachado: conviene partirlos |
| `vc-test.mjs` (193 líneas) | Ejecuta `check-served-build.sh`; compila `VICEEXT_TURN_SIGNALS` apagado (:71-83); lee `JS build=` (:90) y las líneas de resumen OK/FALLO del verificador (:96-99); exige las `marcas` de cada escenario (:138-139) | El paso de la bandera apagada pierde sentido si la bandera desaparece |
| `tools/harness/vc-harness.mjs` (586 líneas) | `check-served-build` (`FALTA`), `VERSION`/`window.__vcVersion`, `#gamelog state N`, **`WR load-req`** (Frontend.cpp:5612), **`FPHASE`** (main.cpp:902), `JS build=` | Nada, si se conservan FPHASE, `WR load-req` y LOADSCR |
| `tools/harness/esquemas.mjs` (1 093 líneas) | `navegarAlAgua` (PEDAT `mar`/`dist`, `VICEEXT swim enter`); `carga` (marca PEDAT, FPHASE, FPSLOG); `agachado` (CROUCH2, CROUCHPOSE, rueda, CRAZYHINT/CRAZYPISTOL); **`nado`** (marcas SWIM2 y SWIMCAM: `velo`, `dz`, `hondo`, `modo`, `camdist`, `SWIMCAM objetivo/no-fallen-water`); `armas` (WINFO, WLOAD, HINTTEST, CRAZYTOOLS); `agachado-mira` | Sin PEDAT (bajo CROUCH) caen `carga` y `nado`. `agachado`, `agachado-mira` y `armas` abortan |
| Otros (fuera del encargo) | `tools/crouch-swim-smoke-test.mjs`, `driveby-log-check.mjs`, `hidecops-log-check.mjs`, `hidecops-smoke-test.mjs` (lee `WANTED*`), `test_controls_axis.py` (genera `P4 kind=inifile`) | — |
| Capa web | `web/lib/index.js:208` (ODPRI: loadtick, FPSLOG, FPHASE, ODSFXDEFER, RETUNE, RADIOTRACK…), :505-522 (sobre `RECOIL` con `OD_RECOIL_SCHEMA_1`, que sólo sirve al recoil viejo) | El sobre RECOIL queda inerte |

El detalle marca a marca (línea del tooling → emisor en `src/` → categoría) está en el Anexo F.

---

## 10. Totales por fichero

| Fichero | + | − | INFRA-WEB | NADO | RADIO | MOD-VIEJO | SIN-GUARDA | DIAG | − de SIN-GUARDA | − de INFRA-WEB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| peds/PlayerPed.cpp | 4827 | 22 | 0 | 391 | 0 | 1923 | 488 | 2025 | 6 | 0 |
| core/Cam.cpp | 1952 | 817 | 0 | 88 | 0 | 359 | 1157 | 348 | 817 | 0 |
| weapons/Weapon.cpp | 852 | 15 | 0 | 0 | 0 | 513 | 61 | 278 | 4 | 0 |
| skel/glfw/glfw.cpp | 836 | 385 | 769 | 0 | 0 | 0 | 0 | 67 | 0 | 385 |
| vehicles/Automobile.cpp | 805 | 35 | 0 | 0 | 0 | 272 | 296 | 237 | 27 | 0 |
| audio/sampman_oal.cpp | 651 | 13 | 448 | 0 | 0 | 0 | 61 | 142 | 0 | 13 |
| peds/Ped.cpp | 640 | 19 | 0 | 22 | 0 | 217 | 91 | 310 | 10 | 0 |
| core/Game.cpp | 600 | 168 | 529 | 0 | 0 | 6 | 3 | 62 | 0 | 168 |
| core/Camera.cpp | 536 | 10 | 0 | 0 | 0 | 175 | 170 | 191 | 10 | 0 |
| renderer/Hud.cpp | 532 | 15 | 0 | 2 | 0 | 272 | 53 | 205 | 15 | 0 |
| core/Streaming.cpp | 441 | 9 | 245 | 0 | 0 | 0 | 4 | 192 | 0 | 9 |
| core/main.cpp | 435 | 5 | 217 | 0 | 0 | 0 | 27 | 191 | 1 | 4 |
| core/Wanted.cpp | 424 | 1 | 0 | 0 | 0 | 170 | 62 | 192 | 0 | 0 |
| core/Pad.cpp | 354 | 0 | 0 | 0 | 0 | 103 | 42 | 209 | 0 | 0 |
| core/CdStreamPosix.cpp | 301 | 0 | 200 | 0 | 0 | 0 | 0 | 101 | 0 | 0 |
| core/ViceExtFeatures.cpp | 284 | 0 | 0 | 0 | 0 | 252 | 0 | 32 | 0 | 0 |
| renderer/Font.cpp | 221 | 23 | 20 | 0 | 0 | 147 | 0 | 54 | 0 | 20 |
| animation/AnimManager.cpp | 210 | 3 | 36 | 39 | 0 | 0 | 116 | 19 | 3 | 0 |
| audio/AudioLogic.cpp | 199 | 22 | 16 | 29 | 0 | 9 | 110 | 35 | 20 | 2 |
| buildings/Building.cpp | 198 | 0 | 0 | 0 | 0 | 189 | 8 | 1 | 0 | 0 |
| entities/Dummy.cpp | 193 | 0 | 0 | 0 | 0 | 184 | 8 | 1 | 0 | 0 |
| core/config.h | 190 | 9 | 14 | 1 | 1 | 139 | 35 | 0 | 9 | 0 |
| core/Frontend.cpp | 184 | 20 | 10 | 0 | 0 | 50 | 57 | 67 | 17 | 0 |
| renderer/ParticleMgr.cpp | 183 | 0 | 0 | 0 | 0 | 151 | 5 | 27 | 0 | 0 |
| text/Text.cpp | 182 | 105 | 134 | 0 | 0 | 0 | 0 | 48 | 0 | 105 |
| core/ControllerConfig.cpp | 178 | 3 | 0 | 0 | 0 | 129 | 49 | 0 | 3 | 0 |
| audio/MusicManager.cpp | 165 | 11 | 8 | 0 | 93 | 0 | 0 | 64 | 0 | 7 |
| control/CarCtrl.cpp | 164 | 12 | 0 | 0 | 0 | 58 | 31 | 75 | 0 | 0 |
| control/Script8.cpp | 155 | 0 | 0 | 0 | 0 | 140 | 0 | 15 | 0 | 0 |
| vehicles/Bike.cpp | 153 | 15 | 0 | 0 | 0 | 78 | 38 | 37 | 14 | 0 |
| peds/PedFight.cpp | 152 | 17 | 0 | 0 | 0 | 15 | 87 | 50 | 17 | 0 |
| text/Messages.cpp | 144 | 8 | 0 | 0 | 0 | 59 | 0 | 85 | 0 | 0 |
| skel/ondemand.cpp | 129 | 0 | 129 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| core/PedArbiter.cpp | 123 | 0 | 0 | 99 | 0 | 0 | 0 | 24 | 0 | 0 |
| save/GenericGameStorage.cpp | 118 | 5 | 51 | 0 | 0 | 35 | 5 | 27 | 1 | 4 |
| core/re3.cpp | 103 | 2 | 22 | 0 | 0 | 67 | 14 | 0 | 1 | 1 |
| core/World.cpp | 101 | 5 | 0 | 0 | 0 | 20 | 33 | 48 | 5 | 0 |
| core/Camera.h | 96 | 0 | 0 | 0 | 0 | 0 | 96 | 0 | 0 | 0 |
| peds/CopPed.cpp | 95 | 2 | 0 | 0 | 0 | 42 | 1 | 52 | 2 | 0 |
| control/Script.cpp | 86 | 5 | 17 | 0 | 0 | 39 | 1 | 29 | 4 | 0 |
| audio/AudioManager.cpp | 85 | 0 | 69 | 0 | 0 | 0 | 0 | 16 | 0 | 0 |
| peds/PlayerPed.h | 85 | 0 | 0 | 4 | 0 | 48 | 28 | 5 | 0 | 0 |
| modelinfo/VehicleModelInfo.cpp | 77 | 15 | 0 | 0 | 0 | 23 | 54 | 0 | 15 | 0 |
| animation/CutsceneMgr.cpp | 67 | 4 | 8 | 0 | 0 | 0 | 0 | 59 | 0 | 4 |
| audio/oal/oal_utils.h | 62 | 0 | 62 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| audio/oal/stream.cpp | 60 | 14 | 60 | 0 | 0 | 0 | 0 | 0 | 0 | 14 |
| peds/PedAI.cpp | 59 | 0 | 21 | 0 | 0 | 12 | 0 | 26 | 0 | 0 |
| renderer/Coronas.cpp | 55 | 2 | 0 | 0 | 0 | 0 | 2 | 53 | 2 | 0 |
| vehicles/Vehicle.cpp | 55 | 0 | 0 | 0 | 0 | 9 | 46 | 0 | 0 | 0 |
| weapons/WeaponInfo.cpp | 54 | 9 | 0 | 0 | 0 | 20 | 34 | 0 | 8 | 0 |
| rw/TexRead.cpp | 48 | 0 | 17 | 0 | 0 | 0 | 0 | 31 | 0 | 0 |
| save/PCSave.cpp | 46 | 1 | 20 | 0 | 0 | 0 | 0 | 26 | 0 | 1 |
| renderer/Renderer.cpp | 44 | 1 | 0 | 0 | 0 | 0 | 0 | 44 | 0 | 0 |
| animation/AnimationId.h | 43 | 0 | 0 | 9 | 0 | 0 | 34 | 0 | 0 | 0 |
| renderer/Particle.cpp | 43 | 1 | 0 | 0 | 0 | 16 | 15 | 12 | 0 | 0 |
| control/CarAI.cpp | 40 | 2 | 0 | 0 | 0 | 23 | 2 | 15 | 2 | 0 |
| control/Pickups.cpp | 39 | 12 | 0 | 0 | 0 | 0 | 39 | 0 | 12 | 0 |
| renderer/Sprite2d.cpp | 39 | 2 | 0 | 0 | 0 | 25 | 14 | 0 | 2 | 0 |
| vehicles/HandlingMgr.h | 37 | 2 | 0 | 0 | 0 | 0 | 37 | 0 | 2 | 0 |
| weapons/ProjectileInfo.cpp | 37 | 7 | 0 | 0 | 0 | 0 | 37 | 0 | 7 | 0 |
| core/ViceExtFeatures.h | 37 | 0 | 0 | 0 | 0 | 0 | 37 | 0 | 0 | 0 |
| renderer/Clouds.cpp | 36 | 0 | 0 | 0 | 0 | 18 | 0 | 18 | 0 | 0 |
| vehicles/Heli.cpp | 36 | 3 | 0 | 0 | 0 | 26 | 0 | 10 | 0 | 0 |
| core/MenuScreensCustom.cpp | 35 | 92 | 0 | 0 | 0 | 26 | 9 | 0 | 4 | 88 |
| skel/skeleton.cpp | 34 | 0 | 34 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| skel/ondemand.h | 31 | 0 | 23 | 0 | 0 | 0 | 0 | 8 | 0 | 0 |
| peds/Ped.h | 29 | 0 | 0 | 1 | 0 | 0 | 22 | 6 | 0 | 0 |
| modelinfo/ModelIndices.h | 27 | 0 | 0 | 0 | 0 | 0 | 27 | 0 | 0 | 0 |
| core/ControllerConfig.h | 26 | 2 | 0 | 0 | 0 | 10 | 16 | 0 | 2 | 0 |
| core/Wanted.h | 26 | 0 | 0 | 0 | 0 | 19 | 0 | 7 | 0 | 0 |
| fakerw/fake.cpp | 26 | 1 | 2 | 0 | 0 | 0 | 0 | 24 | 0 | 1 |
| weapons/WeaponType.h | 26 | 1 | 0 | 0 | 0 | 0 | 26 | 0 | 1 | 0 |
| core/PedArbiter.h | 25 | 0 | 0 | 25 | 0 | 0 | 0 | 0 | 0 | 0 |
| peds/Population.cpp | 24 | 1 | 0 | 0 | 0 | 6 | 2 | 16 | 1 | 0 |
| save/GenericGameStorage.h | 23 | 1 | 5 | 0 | 0 | 11 | 7 | 0 | 1 | 0 |
| vehicles/Boat.cpp | 23 | 11 | 0 | 0 | 0 | 0 | 20 | 3 | 11 | 0 |
| core/Radar.cpp | 22 | 9 | 0 | 0 | 0 | 2 | 9 | 11 | 9 | 0 |
| weapons/Weapon.h | 22 | 1 | 0 | 0 | 0 | 14 | 3 | 5 | 1 | 0 |
| core/main.h | 21 | 0 | 19 | 0 | 0 | 0 | 0 | 2 | 0 | 0 |
| animation/CutsceneMgr.h | 19 | 1 | 19 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| core/FileLoader.cpp | 19 | 0 | 0 | 0 | 0 | 0 | 15 | 4 | 0 | 0 |
| audio/AudioSamples.h | 18 | 0 | 0 | 0 | 0 | 0 | 18 | 0 | 0 | 0 |
| renderer/Font.h | 16 | 0 | 0 | 0 | 0 | 16 | 0 | 0 | 0 | 0 |
| renderer/SpecialFX.cpp | 16 | 0 | 0 | 0 | 0 | 0 | 16 | 0 | 0 | 0 |
| vehicles/Automobile.h | 16 | 0 | 0 | 0 | 0 | 16 | 0 | 0 | 0 | 0 |
| animation/AnimBlendAssocGroup.cpp | 15 | 1 | 15 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| animation/AnimManager.h | 15 | 0 | 0 | 3 | 0 | 0 | 12 | 0 | 0 | 0 |
| control/ScriptCommands.h | 15 | 0 | 0 | 0 | 0 | 15 | 0 | 0 | 0 | 0 |
| CMakeLists.txt | 14 | 0 | 14 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| core/Streaming.h | 14 | 0 | 14 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| renderer/WaterLevel.cpp | 14 | 0 | 11 | 0 | 0 | 0 | 0 | 3 | 0 | 0 |
| renderer/ParticleType.h | 13 | 2 | 0 | 0 | 0 | 0 | 13 | 0 | 2 | 0 |
| control/Script6.cpp | 12 | 1 | 0 | 0 | 0 | 0 | 4 | 8 | 1 | 0 |
| audio/oal/oal_utils.cpp | 11 | 0 | 11 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| core/ZoneCull.cpp | 11 | 0 | 0 | 0 | 0 | 0 | 0 | 11 | 0 | 0 |
| vehicles/HandlingMgr.cpp | 11 | 2 | 0 | 0 | 0 | 0 | 11 | 0 | 2 | 0 |
| audio/sampman_null.cpp | 10 | 0 | 10 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| weapons/WeaponInfo.h | 10 | 3 | 0 | 0 | 0 | 5 | 5 | 0 | 3 | 0 |
| audio/sampman_miles.cpp | 9 | 0 | 9 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| control/Script.h | 9 | 0 | 0 | 0 | 0 | 9 | 0 | 0 | 0 | 0 |
| modelinfo/SimpleModelInfo.cpp | 9 | 3 | 0 | 0 | 0 | 0 | 9 | 0 | 3 | 0 |
| audio/oal/channel.cpp | 8 | 0 | 8 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| core/Game.h | 8 | 0 | 8 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| objects/Stinger.cpp | 8 | 1 | 6 | 0 | 0 | 0 | 2 | 0 | 1 | 0 |
| weapons/WeaponEffects.cpp | 8 | 2 | 0 | 0 | 0 | 0 | 8 | 0 | 2 | 0 |
| control/Script3.cpp | 7 | 0 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 0 |
| core/ZoneCull.h | 7 | 0 | 0 | 0 | 0 | 0 | 0 | 7 | 0 | 0 |
| vehicles/Vehicle.h | 7 | 0 | 0 | 0 | 0 | 4 | 3 | 0 | 0 | 0 |
| audio/soundlist.h | 6 | 2 | 0 | 6 | 0 | 0 | 0 | 0 | 0 | 0 |
| audio/sampman.h | 4 | 0 | 1 | 0 | 0 | 0 | 3 | 0 | 0 | 0 |
| control/Darkel.cpp | 4 | 4 | 0 | 0 | 0 | 0 | 4 | 0 | 4 | 0 |
| audio/oal/channel.h | 3 | 0 | 3 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| buildings/Building.h | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 0 | 0 | 0 |
| control/Pickups.h | 3 | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 3 | 0 |
| core/Pad.h | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 0 | 0 | 0 |
| entities/Dummy.h | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 0 | 0 | 0 |
| renderer/Shadows.cpp | 3 | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 3 | 0 |
| text/Text.h | 3 | 3 | 3 | 0 | 0 | 0 | 0 | 0 | 0 | 3 |
| vehicles/Bike.h | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 0 | 0 | 0 |
| vehicles/Heli.h | 3 | 0 | 0 | 0 | 0 | 3 | 0 | 0 | 0 | 0 |
| renderer/MBlur.cpp | 2 | 3 | 0 | 0 | 0 | 0 | 2 | 0 | 3 | 0 |
| renderer/ParticleMgr.h | 2 | 1 | 0 | 0 | 0 | 0 | 2 | 0 | 1 | 0 |
| core/Frontend.h | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |
| entities/Entity.cpp | 1 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 1 | 0 |
| extras/shaders/neoRimSkin.vert | 1 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 1 | 0 |
| extras/shaders/obj/neoRimSkin_vert.inc | 1 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 1 | 0 |
| renderer/Sprite2d.h | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |

---

# Anexos

## Anexo A. Filas DIAGNOSTICO (5978 líneas del diff: 5 970 añadidas + 8 borradas)

| Fichero:líneas | ± | Nº | Bandera/feature | Descripción |
|---|---|---:|---|---|
| peds/PlayerPed.cpp:19 | + | 1 | WINFO en:WEB | include AnimBlendAssocGroup.h solo para la traza WINFO |
| peds/PlayerPed.cpp:33 | + | 1 | ODTRACES include | ondemand.h para ODTRACES |
| peds/PlayerPed.cpp:37 | + | 1 | P4/SWIMFOV en:MOD | Draw.h: CDraw solo lo usan trazas P4 y SWIMFOV |
| peds/PlayerPed.cpp:191-283 | + | 93 | WINFO en:WEB | ViceExtClipName + ViceExtWeaponInfoTrace (traza WINFO por arma, arnes) |
| peds/PlayerPed.cpp:786-798 | + | 13 | VICEEXT sprint en:WEB | traza de esprint con arma pesada en SetRealMoveAnim |
| peds/PlayerPed.cpp:1116-1129 | + | 14 | AIMWPN en:WEB | traza de bloqueo de cambio de arma |
| peds/PlayerPed.cpp:1426-1432 | + | 7 | P4 pose en:MOD | estado/decl del muestreador P4 pose_exit |
| peds/PlayerPed.cpp:1528-1530 | + | 3 | P4 pose_exit en:MOD | llamada al muestreador |
| peds/PlayerPed.cpp:1567-2183 | + | 617 | P4 identity/move/input/aim/pose/cam en:MOD | VICEEXT_P4_EMIT + ViceExtP4Identity, ViceExtMoveTick (spans move_begin/end), pose_*, cam_* (EM_ASM a window.__odq) |
| peds/PlayerPed.cpp:2234-2258 | + | 25 | AX7 en:MOD | traza AX7 1 Hz en PlayerControl1stPersonRunAround |
| peds/PlayerPed.cpp:2641-2661 | + | 21 | R3P en:MOD | ViceExtRocket3pTrace |
| peds/PlayerPed.cpp:2698-2700 | + | 3 | WINFO en:WEB | llamada a ViceExtWeaponInfoTrace |
| peds/PlayerPed.cpp:2721-2722 | + | 2 | R3P en:SIN-GUARDA | llamada a ViceExtRocket3pTrace |
| peds/PlayerPed.cpp:2796-2815 | + | 20 | P4 kind=fire en:WEB | traza de disparo |
| peds/PlayerPed.cpp:2989-3119 | + | 131 | AIMDIR/armsight en:MOD | ViceExtBlendWeight + ViceExtAimDirTrace (todo el bloque VICEEXT_AIM_WALK es medicion) |
| peds/PlayerPed.cpp:3120-3124 | + | 5 | P4 partial en:WEB | forward de ViceExtP4PartialWatch |
| peds/PlayerPed.cpp:3137-3179 | + | 42 | P4 stand_end en:WEB | medidor de tramo de marcha |
| peds/PlayerPed.cpp:3220-3243 | + | 24 | ODRUMBO en:WEB | traza de rumbo 1 Hz |
| peds/PlayerPed.cpp:3307-3317 | + | 11 | P4 kind=ini opt=10 en:WEB |  |
| peds/PlayerPed.cpp:3417-3426 | + | 10 | VICEEXT reload en:MOD | estado s_viceExtManualReload/StartClip/StartType (solo lo leen las trazas) |
| peds/PlayerPed.cpp:3437-3451 | + | 15 | VICEEXT reload en:MOD | macro VICEEXT_RELOAD_NO |
| peds/PlayerPed.cpp:3470-3479 | + | 10 | VICEEXT reload en:MOD | reload key |
| peds/PlayerPed.cpp:3482 | + | 1 | VICEEXT reload en:MOD | reload no |
| peds/PlayerPed.cpp:3488 | + | 1 | VICEEXT reload en:MOD | reload no |
| peds/PlayerPed.cpp:3492 | + | 1 | VICEEXT reload en:MOD | reload no |
| peds/PlayerPed.cpp:3496 | + | 1 | VICEEXT reload en:MOD | reload no |
| peds/PlayerPed.cpp:3500 | + | 1 | VICEEXT reload en:MOD | reload no |
| peds/PlayerPed.cpp:3514-3516 | + | 3 | VICEEXT reload en:MOD | marca de recarga para la traza |
| peds/PlayerPed.cpp:3528-3535 | + | 8 | VICEEXT reload en:MOD | reload anim |
| peds/PlayerPed.cpp:3537-3552 | + | 16 | VICEEXT reload en:MOD | reload anim/start |
| peds/PlayerPed.cpp:3576-3589 | + | 14 | SWIMNAT en:NADO | traza del nado |
| peds/PlayerPed.cpp:3644-3646 | + | 3 | SWIMFOV (estado) en:NADO | traza del nado |
| peds/PlayerPed.cpp:3656-3658 | + | 3 | VICEEXT swim punos en:NADO | traza del nado |
| peds/PlayerPed.cpp:3664-3666 | + | 3 | VICEEXT swim arma-devuelta en:NADO | traza del nado |
| peds/PlayerPed.cpp:3721-3730 | + | 10 | VICEEXT swim exit / SWIM2 exit en:NADO | traza del nado |
| peds/PlayerPed.cpp:3732-3744 | + | 13 | SWIMIDLE en:NADO | traza del nado |
| peds/PlayerPed.cpp:3757-3759 | + | 3 | VICEEXT swim trepa-off en:NADO | traza del nado |
| peds/PlayerPed.cpp:3769-3771 | + | 3 | SWIMFOV (reset) en:NADO | traza del nado |
| peds/PlayerPed.cpp:3808-3814 | + | 7 | VICEEXT swim enter en:NADO | traza del nado |
| peds/PlayerPed.cpp:3896-3901 | + | 6 | SWIMFASE en:NADO | traza del nado |
| peds/PlayerPed.cpp:3927-3939 | + | 13 | SWIMEF en:NADO | traza del nado |
| peds/PlayerPed.cpp:3944-4038 | + | 95 | VICEEXT swim move / SWIMFOV / SWIM2 move / SWIMERR en:NADO | traza del nado |
| peds/PlayerPed.cpp:4158-4160 | + | 3 | VICEEXT climb en:MOD | traza de escalada |
| peds/PlayerPed.cpp:4177-4179 | + | 3 | VICEEXT climb en:MOD | traza de escalada |
| peds/PlayerPed.cpp:4190-4192 | + | 3 | VICEEXT climb en:MOD | traza de escalada |
| peds/PlayerPed.cpp:4200-4202 | + | 3 | VICEEXT climb en:MOD | traza de escalada |
| peds/PlayerPed.cpp:4217-4223 | + | 7 | VICEEXT climb en:MOD | traza de escalada |
| peds/PlayerPed.cpp:4838-4856 | + | 19 | P4 roll_end/roll_cancel en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:4893-4911 | + | 19 | P4 roll_start (ViceExtCrouchRollStarted) en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:4918-4922 | + | 5 | P4 pose_begin en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:4954-4977 | + | 24 | VICEEXT crouch pose off en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:4980-5073 | + | 94 | P4 partial / CROUCH pose (ViceExtP4PartialWatch, ViceExtCrouchPoseTrace) en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5144-5146 | + | 3 | crouch repetido en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5153-5156 | + | 4 | P4 pose_begin en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5160-5168 | + | 9 | VICEEXT crouch on/off en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5174-5176 | + | 3 | VICEEXT crouch off en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5187-5188 | + | 2 | llamada ViceExtCrouchPoseTrace en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5203-5205 | + | 3 | P4 pose_enter en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5310-5324 | + | 15 | P4 roll_step en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5344-5352 | + | 9 | VICEEXT crouch roll en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5376-5403 | + | 28 | motivo rueda R27 en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5424-5440 | + | 17 | P4 crouchspeed en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5468-5533 | + | 66 | medidor tramo agachado R25 en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5535-5661 | + | 127 | CROUCH2 1 Hz en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:5684-5797 | + | 114 | ViceExtWeaponInfoOf + ViceExtPedAtTrace (PEDAT/sonda de agua del arnes) en:MOD | traza del agachado / arnes |
| peds/PlayerPed.cpp:6049-6067 | + | 19 | AIMLAW en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6083-6085 | + | 3 | P4 cam_begin en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6097-6099 | + | 3 | AIMLOCK (estado) en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6123-6126 | + | 4 | AIMLOCK en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6158-6167 | + | 10 | AIMLOCK en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6173-6181 | + | 9 | AIMLOCK en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6183-6186 | + | 4 | AIMLOCK (estado; if colgante) en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6219-6240 | + | 22 | AIMIK en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6267-6269 | + | 3 | P4 cam_begin en:SIN-GUARDA | traza ClassicAXIS |
| peds/PlayerPed.cpp:6299 | + | 1 | AIMWALK en:MOD | odWalk solo lo usa la traza |
| peds/PlayerPed.cpp:6303-6314 | + | 12 | AIMWALK en:MOD | traza por flanco de la tecla andar |
| peds/PlayerPed.cpp:6331-6341 | + | 11 | P4 kind=ini opt=8 en:WEB |  |
| peds/PlayerPed.cpp:6387-6417 | + | 30 | P4 kind=firewait + MoveTick/CamTick en:WEB |  |
| peds/PlayerPed.cpp:6479-6510 | + | 32 | VICEEXT reload done/fail en:MOD |  |
| peds/PlayerPed.cpp:6618-6621 | + | 4 | AIMDIR en:MOD | llamada ViceExtAimDirTrace |
| peds/PlayerPed.cpp:6622-6625 | + | 4 | PEDAT en:WEB | llamada ViceExtPedAtTrace |
| peds/PlayerPed.h:139 | + | 1 | AIMDIR en:MOD | declaracion ViceExtAimDirTrace |
| peds/PlayerPed.h:141-144 | + | 4 | PEDAT/WINFO en:WEB | declaraciones ViceExtPedAtTrace/ViceExtWeaponInfoOf (arnes) |
| core/Cam.cpp:26-51 | + | 26 | BETASNAP en:WEB | include ondemand.h + helper s_odBetaSnap (traza 1 linea por motivo); stub vacio fuera de web |
| core/Cam.cpp:58 | + | 1 | ODTRACES en:WEB | segundo #include ondemand.h (solo para ODTRACES del autocentrado) |
| core/Cam.cpp:68-73 | + | 6 | CAMB2b/CAMB3b/AIMCOL en:MOD(sin guarda) | statics solo-traza s_odLosD s_odLosPed s_odSphHit s_odSphModel s_odSphPed s_odSphOwn |
| core/Cam.cpp:75-76 | + | 2 | CAMB2b/AIMCOL en:MOD(sin guarda) | statics solo-traza s_odSphD s_odSphNear |
| core/Cam.cpp:104-107 | + | 4 | RECOIL_QUEUE_OVERFLOW en:MOD | traza de desbordamiento de la cola de recoil |
| core/Cam.cpp:122-130 | + | 9 | RECOIL_RESET en:MOD | traza de reset del residual |
| core/Cam.cpp:158-183 | + | 26 | RECOIL_CONTROL en:MOD | acumulador para ViceExtRecoilRecordControl (solo telemetria; sale si la traza esta apagada) |
| core/Cam.cpp:185-196 | + | 12 | RECOIL_CAMERA en:MOD | traza de cambio de modo + s_lastResetReason (solo lo lee RECOIL_PERSIST) |
| core/Cam.cpp:198-236 | + | 39 | RECOIL_PERSIST en:MOD | bloque de traza de persistencia del residual (triggerDown solo para la traza) |
| core/Cam.cpp:249-256 | + | 8 | RECOIL_APPLY en:MOD | traza por solicitud aplicada |
| core/Cam.cpp:555 | + | 1 | BETASNAP en:VANILLA | s_odBetaSnap("mirar-atras") dentro de CCam::Process |
| core/Cam.cpp:565 | + | 1 | BETASNAP en:VANILLA | s_odBetaSnap("snap-lados") |
| core/Cam.cpp:571 | + | 1 | BETASNAP en:VANILLA | s_odBetaSnap("snap-lados") |
| core/Cam.cpp:1242-1254 | + | 13 | SWIMCAM en:NADO | traza unica no-fallen-water (dentro de VICEEXT_SWIMMING + __EMSCRIPTEN__) |
| core/Cam.cpp:1795-1811 | + | 17 | SWIMCAM en:NADO | traza 1/s del objetivo de camara nadando (FollowPedWithMouse) |
| core/Cam.cpp:2718-2763 | + | 46 | camauto2/CAMB2/CAMB2b en:MOD(sin guarda) | trazas de la ley de coche (bajo __EMSCRIPTEN__) |
| core/Cam.cpp:2780 | + | 1 | BETASNAP en:MOD(sin guarda) | s_odBetaSnap("pin-liberado") |
| core/Cam.cpp:2791 | + | 1 | BETASNAP en:MOD(sin guarda) | s_odBetaSnap("centrar-detras") |
| core/Cam.cpp:2798 | + | 1 | BETASNAP en:MOD(sin guarda) | s_odBetaSnap("centrar-delante") |
| core/Cam.cpp:2906-2953 | + | 47 | CAMB3/CAMB3b/CAMSA/CAMV2 en:MOD(sin guarda) | rafaga de trazas tras soltar el raton + CAMSA/CAMV2 1/s |
| core/Cam.cpp:4185-4186 | + | 2 | CAMB2b/AIMCOL en:MOD(sin guarda) | s_odLosD/s_odLosPed (solo traza) en Process_AvoidCollisions |
| core/Cam.cpp:4570-4636 | + | 67 | AIMCAM/AIMVEC/AIMFOV/AIMCOL en:MOD(sin guarda) | trazas 1 Hz de la ley de apuntado (bajo __EMSCRIPTEN__) |
| core/Cam.cpp:6243-6259 | + | 17 | SWIMCAM en:NADO | traza 1/s del objetivo de camara nadando (FollowPed_Rotation) |
| peds/Ped.cpp:11 | + | 1 | include ondemand.h en:MOD | include para ODTRACES |
| peds/Ped.cpp:72-93 | + | 22 | P4 kind=ini en:MOD | ViceExtCheatEmit (traza de trucos) |
| peds/Ped.cpp:385-395 | + | 11 | P4 kind=gangaim en:WEB | traza de precision de ped (constructor) |
| peds/Ped.cpp:1552-1578 | + | 27 | P4 kind=move_apply en:MOD | firma/contador y traza de la ley de movimiento |
| peds/Ped.cpp:1685-1701 | + | 17 | ODCROUCHDIR en:MOD | traza rumbo agachado |
| peds/Ped.cpp:1718-1744 | + | 27 | P4 kind=move_apply en:MOD | traza de movimiento agachado |
| peds/Ped.cpp:1762-1799 | + | 38 | P4 kind=legs en:WEB | traza de piernas/clips |
| peds/Ped.cpp:5105-5179 | + | 75 | DRIVEBY state/shot/enter/exit en:MOD | DriveByTraceTag (EM_ASM_INT sin guarda) + DriveByTraceState/Shot/VehicleWeapon + VICEEXT_DRIVEBY_REV |
| peds/Ped.cpp:5222 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5229 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5234 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5236 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5240 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5242-5244 | + | 3 | DRIVEBY en:MOD | DriveByTraceVehicleWeapon(enter) |
| peds/Ped.cpp:5253 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5260-5262 | + | 3 | DRIVEBY en:MOD | outcome / else solo para la traza |
| peds/Ped.cpp:5265 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5268 | + | 1 | DRIVEBY en:MOD | outcome |
| peds/Ped.cpp:5270-5272 | + | 3 | DRIVEBY en:MOD | DriveByTraceVehicleWeapon(exit) |
| peds/Ped.cpp:5445 | + | 1 | P4 kind=ini en:MOD | ViceExtCheatEmit (cabezones) |
| peds/Ped.cpp:5849-5885 | + | 37 | P4 kind=jump_end en:MOD | estado s_odJump* + ViceExtP4JumpEnd |
| peds/Ped.cpp:5919-5923 | + | 5 | P4 kind=jump_end en:MOD | llamada en SetLanding |
| peds/Ped.cpp:9740-9766 | + | 27 | P4 kind=jump en:MOD | traza de salto |
| peds/Ped.cpp:9798-9802 | + | 5 | P4 kind=jump_end en:MOD | llamada en FinishLaunchCB |
| peds/Ped.h:1013-1018 | + | 6 | DRIVEBY en:MOD | declaraciones DriveByTrace* |
| peds/PedAI.cpp:2 | + | 1 | include ondemand.h en:WEB | include para ODTRACES |
| peds/PedAI.cpp:4905 | + | 1 | ODANIMFAIL en:WEB | aviso de bloques sin cargar |
| peds/PedAI.cpp:5661-5672 | + | 12 | P4 kind=ini opt=16 en:MOD | traza cop cover |
| peds/PedAI.cpp:5764-5775 | + | 12 | P4 kind=ini opt=16 en:MOD | traza cop cover en vehiculo |
| peds/PedFight.cpp:29 | + | 1 | include ondemand.h en:WEB | include para ODTRACES |
| peds/PedFight.cpp:729-740 | + | 12 | P4 kind=firewait en:WEB | traza |
| peds/PedFight.cpp:773-784 | + | 12 | P4 kind=firewait en:WEB | traza |
| peds/PedFight.cpp:848-859 | + | 12 | P4 kind=firewait en:WEB | traza |
| peds/PedFight.cpp:918-930 | + | 13 | P4 kind=firewait en:WEB | traza |
| peds/CopPed.cpp:22 | + | 1 | include ondemand.h en:MOD | include para ODTRACES |
| peds/CopPed.cpp:98-109 | + | 12 | P4 kind=gangaim en:MOD | traza precision de policia |
| peds/CopPed.cpp:176-182 | + | 7 | WANTEDCOP arrest-begin (sin guarda) | traza |
| peds/CopPed.cpp:265-291 | + | 27 | WANTEDCOP join (sin guarda) | traza con rafaga |
| peds/CopPed.cpp:486-490 | + | 5 | WANTEDCOP search-last-known en:MOD | traza |
| peds/Population.cpp:31-33 | + | 3 | include ondemand.h en:WEB | include para ODTRACES |
| peds/Population.cpp:366-378 | + | 13 | P4 kind=limits en:MOD | traza de limites |
| core/PedArbiter.cpp:4 | + | 1 | include ondemand.h en:NADO | include para PEDCLAIM |
| core/PedArbiter.cpp:47-53 | + | 7 | PEDCLAIM en:NADO | traza claim rechazado |
| core/PedArbiter.cpp:57-64 | + | 8 | PEDCLAIM en:NADO | traza claim ok |
| core/PedArbiter.cpp:84-91 | + | 8 | PEDCLAIM en:NADO | traza release |
| core/Camera.cpp:37 | + | 1 | include ondemand.h en:MOD | include para ODTRACES |
| core/Camera.cpp:118-141 | + | 24 | P4 kind=aimsens en:WEB | ViceExtP4AimSens |
| core/Camera.cpp:168-169 | + | 2 | P4 kind=aimsens en:MOD | llamada a ViceExtP4AimSens |
| core/Camera.cpp:480-504 | + | 25 | AIMCFG en:MOD | traza de ajustes ClassicAXIS (todo el bloque V es traza) |
| core/Camera.cpp:1265-1370 | + | 106 | CAM1P / VICEEXT 1p bind en:MOD | traza del peek/conmutador 1a persona (incluye las lineas V que solo alimentan la traza) |
| core/Camera.cpp:1443-1449 | + | 7 | VICEEXT 1p key en:MOD | traza |
| core/Camera.cpp:1486-1505 | + | 20 | VICEEXT 1p set en:MOD | traza |
| core/Camera.cpp:4143-4148 | + | 6 | [od] splines printf en:WEB | printf de carga de splines |
| skel/glfw/glfw.cpp:30-32 | + | 3 | include-ondemand(ODTRACES) | include ondemand.h solo para ODTRACES |
| skel/glfw/glfw.cpp:2246 | + | 1 | loadtick | ODTRACES loadtick Y |
| skel/glfw/glfw.cpp:2359 | + | 1 | loadtick | ODTRACES loadtick B splash |
| skel/glfw/glfw.cpp:2367 | + | 1 | loadtick | ODTRACES loadtick B init |
| skel/glfw/glfw.cpp:2451 | + | 1 | loadtick | ODTRACES loadtick A shutdown sub |
| skel/glfw/glfw.cpp:2457 | + | 1 | loadtick | ODTRACES loadtick A shutdown fin |
| skel/glfw/glfw.cpp:2470-2475 | + | 6 | loadtick | ODTRACES loadtick firstdraw |
| skel/glfw/glfw.cpp:2521-2531 | + | 11 | WR wipe | traza WR wipe (ODTRACES + printf [want]) |
| skel/glfw/glfw.cpp:2616-2657 | + | 42 | WEBHB/censo OD | latido cada 120 ticks: printf [web] tick + censo OD + ODTRACES WEBHB |
| skel/ondemand.h:1-4 | + | 4 | ODTRACE | comentario de la traza web (window.__odq) |
| skel/ondemand.h:8-9 | + | 2 | ODTRACE | macros ODTRACEI/ODTRACES (cola window.__odq) |
| skel/ondemand.h:27-28 | + | 2 | ODTRACE | stubs nativos ODTRACEI/ODTRACES |
| core/main.h:62-63 | + | 2 | FPSLOG strm | gWebStrmMs (medida de streaming para FPSLOG) |
| core/main.cpp:3-5 | + | 3 | include-ondemand(ODTRACES) | include ondemand.h solo para ODTRACES |
| core/main.cpp:795-912 | + | 118 | FPHASE | cronometraje por fases del frame (WebPhaseBegin/Mark/NoRender/End) + traza FPHASE |
| core/main.cpp:951-957 | + | 7 | FPSLOG | traza FPSLOG avg/maxdelta/heap/strm |
| core/main.cpp:1004-1010 | + | 7 | LOADSCR | traza LOADSCR clamp |
| core/main.cpp:1035-1048 | + | 14 | LBAR | traza LBAR w/h/borde |
| core/main.cpp:1072 | + | 1 | LOADSCR | ODTRACES LOADSCR begin |
| core/main.cpp:1081 | + | 1 | LOADSCR | ODTRACES LOADSCR end |
| core/main.cpp:1885-2063 | + | 30 | FPHASE | llamadas WebPhaseBegin/Mark/NoRender + traza WR demo-timeout + llaves del if(arg==nil) |
| core/main.cpp:2065-2087 | + | 10 | FPHASE | WebPhaseMark(4/5)/WebPhaseEnd |
| core/CdStreamPosix.cpp:26 | + | 1 | include-ondemand(ODTRACES) | include ondemand.h solo para ODTRACES |
| core/CdStreamPosix.cpp:123-199 | + | 77 | ODSHORT/ODSRECOVER | anillo de fallos de apertura solo para trazas ODSHORT/ODSRECOVER |
| core/CdStreamPosix.cpp:238-239 | + | 2 | loosemiss | printf [od-trace] + ODTRACEI loosemiss |
| core/CdStreamPosix.cpp:254-256 | + | 3 | ODSHORT | OdNoteFailed |
| core/CdStreamPosix.cpp:259-261 | + | 3 | ODSRECOVER | OdNoteOpened |
| core/CdStreamPosix.cpp:279-290 | + | 12 | ODSHORT | traza ODSHORT short |
| core/CdStreamPosix.cpp:854-856 | + | 3 | addimage | traza addimage LOOSE/ARCHIVE |
| core/Streaming.cpp:369-381 | + | 13 | CARBLOCK | traza CARBLOCK |
| core/Streaming.cpp:389-398 | + | 9 | FPSLOG strm | medida de ms de streaming (gWebStrmMs) |
| core/Streaming.cpp:649-660 | + | 12 | FAIL model | traza FAIL model |
| core/Streaming.cpp:679-702 | + | 24 | TXDIN | traza TXDIN (checksum de cabecera) |
| core/Streaming.cpp:714-724 | + | 11 | FAIL txd | traza FAIL txd |
| core/Streaming.cpp:739-749 | + | 11 | FAIL col | traza FAIL col |
| core/Streaming.cpp:767-770 | + | 4 | traza-retirada | comentario de rastro por .ifp retirado |
| core/Streaming.cpp:1696-1717 | + | 22 | CARLOAD | traza CARLOAD |
| core/Streaming.cpp:1914-1939 | + | 26 | CARPED/CARPOOL | trazas CARPED/CARPOOL |
| core/Streaming.cpp:1949-1962 | + | 14 | CARFULL | traza CARFULL |
| core/Streaming.cpp:2419-2437 | + | 19 | CARFAIL | traza CARFAIL |
| core/Streaming.cpp:2722-2725 | + | 4 | ODLOAD big | cronometro por fichero |
| core/Streaming.cpp:2749 | + | 1 | CDRETRY | ODTRACES CDRETRY agotado |
| core/Streaming.cpp:2778-2795 | + | 18 | ODLOAD big | traza ODLOAD big |
| core/Streaming.cpp:3450-3453 | + | 4 | FPSLOG strm | gWebStrmMs |
| core/FileLoader.cpp:3 | + | 1 | include-ondemand(ODTRACES) | include ondemand.h solo para ODTRACES |
| core/FileLoader.cpp:53-55 | + | 3 | loadlevel | traza loadlevel |
| core/Game.cpp:5 | + | 1 | include-ondemand(ODTRACES) | include ondemand.h solo para ODTRACES |
| core/Game.cpp:409-413 | + | 5 | initstep | traza initstep/heap |
| core/Game.cpp:737-753 | + | 17 | P4 kind=worlddata (VICEEXT_MAPDATA) | traza P4 worlddata (zonas de oclusion del mod) bajo VICEEXT_MAPDATA |
| core/Game.cpp:1070 | + | 1 | webload | s_loadT0 (duracion de carga) |
| core/Game.cpp:1128-1130 | + | 3 | webload | ODTRACES/printf GenericLoad empieza + s_loadT0 |
| core/Game.cpp:1141-1142 | + | 2 | webload | ODTRACES/printf GenericLoad FAIL |
| core/Game.cpp:1190-1206 | + | 17 | webload | ODTRACES/printf GenericLoad OK + total + loaded pos |
| core/Game.cpp:1260-1653 | + | 16 | ODMARK | macro ODMARK no-op y sus marcas en CGame::Process/DrasticTidyUpMemory |
| save/GenericGameStorage.cpp:20-22 | + | 3 | save-trazas | include ondemand.h solo para ODTRACES de carga/guardado |
| save/GenericGameStorage.cpp:351-354 | + | 4 | save GenericLoad FAIL | ODTRACES+printf fallo de tamano |
| save/GenericGameStorage.cpp:358-365 | + | 8 | save GenericLoad abre | ODTRACES+printf apertura |
| save/GenericGameStorage.cpp:369-370 | + | 2 | save GenericLoad FAIL | ODTRACES+printf cuerpo incompleto |
| save/GenericGameStorage.cpp:592-601 | + | 10 | save CheckSlotDataValid | ODTRACES+printf DIAG F2-load |
| save/PCSave.cpp:22-27 | + | 6 | save-trazas | includes Lists/PlayerInfo/PlayerPed + ondemand solo para la traza save slot pos |
| save/PCSave.cpp:57-64 | + | 8 | save DeleteSlot | ODTRACES+printf |
| save/PCSave.cpp:89-100 | + | 12 | save slot pos | ODTRACES+printf posicion al guardar |
| rw/TexRead.cpp:9-11 | + | 3 | SKIP tex | include ondemand.h para trazas SKIP |
| rw/TexRead.cpp:101-110 | + | 10 | SKIP tex | traza acotada dictpos |
| rw/TexRead.cpp:156-164 | + | 9 | SKIP1 tex | traza acotada |
| rw/TexRead.cpp:192-200 | + | 9 | SKIP2 tex | traza acotada |
| fakerw/fake.cpp:13-15 | + | 3 | hackRead | include ondemand.h para trazas |
| fakerw/fake.cpp:311-319 | + | 9 | hackRead tex NIL | DIAG F3a (incluye return temprano si tex nil; quitar con el fix raiz) |
| fakerw/fake.cpp:321-332 | + | 12 | hackRead convert NIL | traza acotada |
| renderer/Renderer.cpp:3 | + | 1 | LODLEFT | include ondemand.h |
| renderer/Renderer.cpp:838-874 | + | 37 | LODLEFT | OdLodLeftDiag (D10 LOD pegado) |
| renderer/Renderer.cpp:917-922 | + | 6 | LODLEFT | llamada a OdLodLeftDiag si dist<60 |
| renderer/Renderer.cpp:o879 | - | 1 | LODLEFT | linea en blanco sustituida por el bloque de traza |
| renderer/Coronas.cpp:20 | + | 1 | CORONA | include ondemand.h |
| renderer/Coronas.cpp:93-119 | + | 27 | CORONA tex | volcado unico de texturas de corona |
| renderer/Coronas.cpp:279-283 | + | 5 | CORONAREND | contadores solo para la traza |
| renderer/Coronas.cpp:350-351 | + | 2 | CORONAREND | contador sinTextura |
| renderer/Coronas.cpp:364 | + | 1 | CORONAREND | contador dibujadas |
| renderer/Coronas.cpp:437-453 | + | 17 | CORONAREND | traza por segundo de coronas dibujadas |
| renderer/WaterLevel.cpp:21 | + | 1 | WATER | include ondemand.h |
| renderer/WaterLevel.cpp:135-136 | + | 2 | WATER no-waterpro | printf [od-trace] + ODTRACES |
| text/Text.cpp:2 | + | 1 | TXTMISS | include ondemand.h |
| text/Text.cpp:58-64 | + | 7 | TXTGXTFAIL | ODTRACES en OdTextFail |
| text/Text.cpp:147-161 | + | 15 | TXTMISS | traza de clave ausente |
| text/Text.cpp:337-351 | + | 14 | MSGTABLE | traza por tabla de mision cargada |
| text/Text.cpp:353-360 | + | 8 | TXTGXTSHORT | traza lectura corta |
| text/Text.cpp:465-467 | + | 3 | TXTMISS | comentario sobre la traza movida a CText::Get |
| animation/AnimManager.cpp:2 | + | 1 | IFPFILE | include ondemand.h |
| animation/AnimManager.cpp:1399-1402 | + | 4 | IFPFILE | comentario de traza retirada |
| animation/AnimManager.cpp:1429-1446 | + | 14 | IFPFILE | printf ifp-file + ODTRACES clips por fichero |
| animation/CutsceneMgr.cpp:196-204 | + | 9 | CUT begin | traza |
| animation/CutsceneMgr.cpp:215-226 | + | 11 | CUT cutsimg-FALLO | traza si cuts.img no abre (sustituye al assert en web) |
| animation/CutsceneMgr.cpp:268-277 | + | 10 | CUT audio | traza |
| animation/CutsceneMgr.cpp:282-296 | + | 15 | CUT loaded | traza + printf [od] cutscene |
| animation/CutsceneMgr.cpp:364-370 | + | 7 | CUT start | traza |
| animation/CutsceneMgr.cpp:475-481 | + | 7 | CUT end | traza |
| audio/MusicManager.cpp:26-38 | + | 13 | RETUNE (en:WEB) | RetuneTrace(): traza RETUNE de cada tap/consumo del dial + include ondemand.h |
| audio/MusicManager.cpp:196-203 | + | 8 | printf [web] radio lens ms (en:WEB) | printf de las 9 longitudes de emisora para tests headless |
| audio/MusicManager.cpp:575 | + | 1 | RETUNE (en:RADIO) | RetuneTrace(tapF9) |
| audio/MusicManager.cpp:596 | + | 1 | RETUNE (en:RADIO) | RetuneTrace(tap) |
| audio/MusicManager.cpp:615 | + | 1 | RETUNE (en:RADIO) | RetuneTrace(tapWheel) |
| audio/MusicManager.cpp:1149-1159 | + | 11 | CUTMUS (en:WEB) | traza del audio de cinematica |
| audio/MusicManager.cpp:1213-1215 | + | 3 | RETUNE (en:WEB) | RetuneTrace(consume) |
| audio/MusicManager.cpp:1410-1422 | + | 13 | RADIOLOGOS (en:RADIO) | cuenta logos/mascaras cargados de radio.txd |
| audio/MusicManager.cpp:1480-1492 | + | 13 | P4 kind=radio2 (en:RADIO) | traza P4 radio2 emisora/icono/mascara |
| audio/sampman_oal.cpp:145-149 | + | 5 | ODSFXSTAT contadores (en:WEB) | contadores hit/miss/ms/evict |
| audio/sampman_oal.cpp:167 | + | 1 | ODSFXSTAT contadores (en:WEB) | contadores defer/pump/maxframe/defertrace |
| audio/sampman_oal.cpp:174-186 | + | 13 | ODSFXSTAT (en:WEB) | OdSfxStat(): traza periodica |
| audio/sampman_oal.cpp:268 | + | 1 | ODSFXSTAT (en:WEB) | maximo ms por frame (estadistica) |
| audio/sampman_oal.cpp:306 | + | 1 | ODSFXSTAT (en:WEB) | odSfxPumped++ |
| audio/sampman_oal.cpp:311 | + | 1 | ODSFXSTAT (en:WEB) | OdSfxStat(pump) |
| audio/sampman_oal.cpp:322-323 | + | 2 | ODSFXSTAT (en:WEB) | odSfxHit++ / OdSfxStat(hit) |
| audio/sampman_oal.cpp:345-353 | + | 9 | ODSFXDEFER (en:WEB) | contador + traza de diferidos |
| audio/sampman_oal.cpp:363 | + | 1 | ODSFXSTAT (en:WEB) | odSfxMiss++ |
| audio/sampman_oal.cpp:435-455 | + | 21 | SFXLEN (en:WEB) | traza longitud decode vs SDT + printf [audio] |
| audio/sampman_oal.cpp:469-471 | + | 3 | ODSFXMISS (en:WEB) | comentario + contador nmiss |
| audio/sampman_oal.cpp:473 | + | 1 | ODSFXSTAT (en:WEB) | odSfxMs += |
| audio/sampman_oal.cpp:476-487 | + | 12 | ODSFXMISS (en:WEB) | traza por muestra decodificada + OdSfxStat(miss) |
| audio/sampman_oal.cpp:1471-1473 | + | 3 | printf [web] sampman init ok (en:WEB) | marca de arranque en consola |
| audio/sampman_oal.cpp:1855-1857 | + | 3 | ODSFXPROT (en:SIN-GUARDA) | traza de reserva |
| audio/sampman_oal.cpp:2049-2054 | + | 6 | CHINIT (en:WEB) | t0 para medir arranque de canal |
| audio/sampman_oal.cpp:2058-2070 | + | 13 | CHINIT (en:WEB) | traza canal/muestra/ms |
| audio/sampman_oal.cpp:2247-2256 | + | 10 | STREAM preload (en:WEB) | traza de apertura de stream |
| audio/sampman_oal.cpp:2421-2452 | + | 21 | RADIOTRACK (en:WEB) | mide setup/seek de emisora (3 bloques) |
| audio/sampman_oal.cpp:2617-2631 | + | 15 | SFXDATA (en:WEB) | identidad FNV de la SDT (odtrace + printf) |
| audio/AudioLogic.cpp:5-7 | + | 3 | ondemand.h (en:WEB) | include solo para ODTRACES |
| audio/AudioLogic.cpp:2932-2951 | + | 20 | SIRENA (en:WEB) | traza de sirenas |
| audio/AudioLogic.cpp:4694-4705 | + | 12 | VICEEX sfx arma= (en:SIN-GUARDA) | traza muestra por arma |
| audio/AudioManager.cpp:14-16 | + | 3 | ondemand.h (en:WEB) | include solo para ODTRACES |
| audio/AudioManager.cpp:82-84 | + | 3 | printf [web] audiomanager init ok (en:WEB) | marca de arranque |
| audio/AudioManager.cpp:1384-1388 | + | 5 | LOOPORPHAN (en:WEB) | traza |
| audio/AudioManager.cpp:1406-1410 | + | 5 | LOOPKILL (en:WEB) | traza |
| weapons/Weapon.h:42-44 | + | 3 | RECOIL_* en:MOD | declaraciones FireAttempt/TraceEnabled/Trace (sólo trazas de recoil) |
| weapons/Weapon.h:48-49 | + | 2 | RECOIL_CONTROL en:MOD | declaración RecordControl (traza) |
| weapons/Weapon.cpp:24 | + | 1 | ODTRACES | include ondemand.h para trazas |
| weapons/Weapon.cpp:168-173 | + | 6 | RECOIL_FIRE_WAIT en:MOD | estado de ventana/munición sólo para traza |
| weapons/Weapon.cpp:175-176 | + | 2 | RECOIL_RELOAD/READY en:MOD | estado sólo para traza |
| weapons/Weapon.cpp:183-186 | + | 4 | RECOIL_CLASS en:MOD | traza en ResolvePendingTap |
| weapons/Weapon.cpp:198-247 | + | 50 | RECOIL_SHOT en:MOD | ViceExtRecoilTypeName/InputSource sólo para trazas |
| weapons/Weapon.cpp:306 | + | 1 | RECOIL3 en:MOD | contador de disparos sólo para traza |
| weapons/Weapon.cpp:330-338 | + | 9 | RECOIL_READY en:MOD | arranque de sesión de traza |
| weapons/Weapon.cpp:345-350 | + | 6 | RECOIL_WEAPON_CHANGE en:MOD | traza cambio de arma |
| weapons/Weapon.cpp:355-364 | + | 10 | RECOIL_RELOAD en:MOD | traza flanco de recarga |
| weapons/Weapon.cpp:366 | + | 1 | RECOIL_INPUT en:MOD | buffer de traza |
| weapons/Weapon.cpp:373-375 | + | 3 | RECOIL_INPUT en:MOD | traza flanco press |
| weapons/Weapon.cpp:383-384 | + | 2 | RECOIL_INPUT en:MOD | traza flanco release |
| weapons/Weapon.cpp:386 | + | 1 | RECOIL_INPUT en:MOD | emisión traza |
| weapons/Weapon.cpp:389-417 | + | 29 | RECOIL_FIRE_WAIT/RECOIL3 en:MOD | ventana de 1 s y miracheck sólo traza |
| weapons/Weapon.cpp:420-464 | + | 45 | RECOIL_* en:MOD | FireAttempt/TraceEnabled/Trace(window.__odRecoilEmit)/RecordControl |
| weapons/Weapon.cpp:486-487 | + | 2 | RECOIL_FIRE_WAIT/RECOIL3 en:MOD | contadores de traza en Kick |
| weapons/Weapon.cpp:523-526 | + | 4 | RECOIL_CLASS en:MOD | traza burst confirmado |
| weapons/Weapon.cpp:552-561 | + | 10 | RECOIL_SHOT en:MOD | traza de cada disparo |
| weapons/Weapon.cpp:606-612 | + | 7 | P4 kind=uzi en:WEB | traza UZIOLD entrada Fire (sólo __EMSCRIPTEN__) |
| weapons/Weapon.cpp:613-616 | + | 4 | RECOIL_FIRE_WAIT en:MOD | llamada FireAttempt (sólo alimenta traza) |
| weapons/Weapon.cpp:918-925 | + | 8 | P4 kind=uzi en:WEB | traza UZIOLD salida Fire |
| weapons/Weapon.cpp:984-999 | + | 16 | RECOIL_SHOT en:MOD | traza drive-by FireFromCar |
| weapons/Weapon.cpp:1477-1489 | + | 13 | VICEEXT gastank reserva en:MOD | traza modelo sin dummy petrolcap |
| weapons/Weapon.cpp:1514-1521 | + | 8 | VICEEXT gastank hit en:MOD | traza explosión depósito |
| weapons/Weapon.cpp:1569-1576 | + | 8 | VICEEXT luces rota en:MOD | traza faro roto |
| weapons/Weapon.cpp:1610-1617 | + | 8 | P4 kind=uzi en:WEB | traza UZIOLD entrada FireInstantHit |
| weapons/Weapon.cpp:1981-1988 | + | 8 | P4 kind=uzi en:WEB | traza UZIOLD salida FireInstantHit |
| weapons/Weapon.cpp:3139-3145 | + | 7 | P4 kind=ini opt=19 en:MOD | bloque VICEEXT_PHOTO_SAVE = sólo traza |
| weapons/Weapon.cpp:4051-4055 | + | 5 | P4 kind=ini opt=14 en:MOD | traza proyectil reventado |
| vehicles/Automobile.cpp:6 | + | 1 | P4 kind=wheelrot | include Pools.h sólo para traza |
| vehicles/Automobile.cpp:29 | + | 1 | ODTRACES | include ondemand.h |
| vehicles/Automobile.cpp:58-80 | + | 23 | P4 kind=ini en:WEB | ViceExtP4IniEmit (emisor de trazas) |
| vehicles/Automobile.cpp:84 | + | 1 | P4 kind=ini opt=2 en:MOD | traza en ViceExtWheelKeepSteer |
| vehicles/Automobile.cpp:1555 | + | 1 | P4 kind=ini opt=18 en:MOD | traza RC cohete |
| vehicles/Automobile.cpp:1815-1817 | + | 3 | P4 kind=turners en:MOD | prototipo emisor |
| vehicles/Automobile.cpp:1830-1832 | + | 3 | P4 kind=turners en:MOD | traza |
| vehicles/Automobile.cpp:1836-1838 | + | 3 | P4 kind=turners en:MOD | traza |
| vehicles/Automobile.cpp:1850-1852 | + | 3 | P4 kind=turners en:MOD | traza |
| vehicles/Automobile.cpp:1864 | + | 1 | P4 kind=turners en:MOD | odMotivo sólo traza |
| vehicles/Automobile.cpp:1867 | + | 1 | P4 kind=turners en:MOD | odMotivo sólo traza |
| vehicles/Automobile.cpp:1871-1873 | + | 3 | P4 kind=turners en:MOD | traza |
| vehicles/Automobile.cpp:1881-1883 | + | 3 | P4 kind=turners en:MOD | traza |
| vehicles/Automobile.cpp:1892 | + | 1 | P4 kind=turners en:MOD | odAnchorErr sólo traza |
| vehicles/Automobile.cpp:1904 | + | 1 | P4 kind=turners en:MOD | odAnchorErr sólo traza |
| vehicles/Automobile.cpp:1912-1922 | + | 11 | P4 kind=turners en:MOD | traza acotada |
| vehicles/Automobile.cpp:1977-2049 | + | 73 | P4 kind=service/turners en:WEB | ViceExtP4ServiceColdLocal/ServiceEmit/TurnersEmit + macro VICEEXT_P4_EMIT |
| vehicles/Automobile.cpp:2083-2093 | + | 11 | P4 kind=wheelrot en:MOD | traza volante |
| vehicles/Automobile.cpp:2118-2141 | + | 24 | SVLIGHTS/P4 kind=service en:WEB | trazas camino caliente |
| vehicles/Automobile.cpp:2222-2225 | + | 4 | P4 kind=service en:WEB | traza camino frío |
| vehicles/Automobile.cpp:2230-2240 | + | 11 | SVLIGHTS en:WEB | traza |
| vehicles/Automobile.cpp:2598-2612 | + | 15 | SVLIGHTS/P4 kind=service en:WEB | llamada a ServiceLightWorldPos sólo para trazar |
| vehicles/Automobile.cpp:3934 | + | 1 | P4 kind=ini opt=7 en:SIN-GUARDA | traza disparo tanque |
| vehicles/Automobile.cpp:4518 | + | 1 | DRIVEBY state en:SIN-GUARDA | DriveByTraceState |
| vehicles/Automobile.cpp:4557-4559 | + | 3 | DRIVEBY shot en:SIN-GUARDA | comentario de la traza |
| vehicles/Automobile.cpp:4564-4566 | + | 3 | DRIVEBY shot en:SIN-GUARDA | DriveByTraceShot |
| vehicles/Automobile.cpp:4771-4773 | + | 3 | P4 kind=ini opt=5 en:SIN-GUARDA | traza |
| vehicles/Automobile.cpp:4799 | + | 1 | P4 kind=ini opt=4 en:MOD | traza gasolinera |
| vehicles/Automobile.cpp:4954-4955 | + | 2 | P4 kind=cardet en:MOD | buffer traza |
| vehicles/Automobile.cpp:4963-4967 | + | 5 | P4 kind=cardet en:MOD | traza rueda desprendida |
| vehicles/Automobile.cpp:4983-4987 | + | 5 | P4 kind=cardead en:MOD | traza ocupantes muertos |
| vehicles/Automobile.cpp:4992-4996 | + | 5 | P4 kind=cardet en:MOD | traza rueda reventada |
| vehicles/Automobile.cpp:5052-5061 | + | 10 | P4 kind=carluz en:MOD | bloque VICEEXT_CAR_LIGHTS = sólo traza |
| vehicles/Bike.cpp:40-41 | + | 2 | BSIREN | includes Sprite.h/ondemand.h para traza |
| vehicles/Bike.cpp:1318-1348 | + | 31 | BSIREN en:MOD | traza sirena moto policial |
| vehicles/Bike.cpp:2156 | + | 1 | DRIVEBY state en:SIN-GUARDA | DriveByTraceState |
| vehicles/Bike.cpp:2204-2205 | + | 2 | DRIVEBY shot en:SIN-GUARDA | comentario traza |
| vehicles/Bike.cpp:2209 | + | 1 | DRIVEBY shot en:SIN-GUARDA | DriveByTraceShot |
| vehicles/Boat.cpp:1436 | + | 1 | DRIVEBY state en:SIN-GUARDA | DriveByTraceState |
| vehicles/Boat.cpp:1468 | + | 1 | DRIVEBY shot en:SIN-GUARDA | comentario traza |
| vehicles/Boat.cpp:1472 | + | 1 | DRIVEBY shot en:SIN-GUARDA | DriveByTraceShot |
| vehicles/Heli.cpp:30 | + | 1 | ODTRACES | include ondemand.h |
| vehicles/Heli.cpp:1022-1030 | + | 9 | P4 kind=ini opt=13 en:MOD | traza helicóptero derribado |
| control/CarCtrl.cpp:31-33 | + | 3 | ODTRACES | include ondemand.h |
| control/CarCtrl.cpp:658-672 | + | 15 | CARSPAWN en:WEB | traza coche generado |
| control/CarCtrl.cpp:786-791 | + | 5 | CARRATE en:WEB | refactor neutro de ChooseCarRating para poder trazar |
| control/CarCtrl.cpp:o748-749 | - | 2 | CARRATE en:WEB | return vanilla |
| control/CarCtrl.cpp:793-844 | + | 52 | CARZONE/CARRATE/CARRATECNT en:WEB | trazas de mezcla de tráfico |
| control/CarCtrl.cpp:o751 | - | 1 | CARRATE en:WEB | return vanilla |
| control/CarAI.cpp:10 | + | 1 | ODTRACES | include ondemand.h |
| control/CarAI.cpp:72-76 | + | 5 | P4 kind=chofer en:MOD | traza |
| control/CarAI.cpp:574-582 | + | 9 | P4 kind=polbike en:MOD | traza |
| renderer/Hud.cpp:21-24 | + | 4 | SCRTXT en:WEB | include ondemand.h/stdio para la traza SCRTXT |
| renderer/Hud.cpp:34 | + | 1 | ondemand en:MOD | include ondemand.h para trazas D4/D6/D7 |
| renderer/Hud.cpp:316-357 | + | 42 | SIGHT / P4 kind=sight en:MOD | trazas de la mira por arma dentro de ViceExtDrawSight |
| renderer/Hud.cpp:425-437 | + | 13 | SWIMHUD en:NADO | traza 1 Hz de cruz suprimida nadando (consulta ViceExtPedOwns) |
| renderer/Hud.cpp:438-453 | + | 16 | P4 kind=hud_cross en:WEB | traza de estado de la cruz |
| renderer/Hud.cpp:713-757 | + | 45 | AIMHUD / AIHTRI / P4 kind=hud2 en:MOD | trazas del HUD de apuntado ClassicAXIS |
| renderer/Hud.cpp:804-810 | + | 7 | P4 kind=hud2 en:WEB | traza de dinero cada frame |
| renderer/Hud.cpp:881-912 | + | 32 | HUDICON en:WEB | traza del icono de arma (TXD/textura) |
| renderer/Hud.cpp:1069-1079 | + | 11 | P4 kind=ini en:WEB | traza de hide stars |
| renderer/Hud.cpp:2192-2198 | + | 7 | SIGHTS en:MOD | traza carga TXD de miras |
| renderer/Hud.cpp:2205-2207 | + | 3 | KEYICONS en:MOD | traza carga pcbtns.txd |
| renderer/Hud.cpp:2366-2389 | + | 24 | SCRTXT en:WEB | traza de mensajes de ayuda |
| renderer/Font.cpp:10-11 | + | 2 | SCRTXT3 | include ondemand.h/string.h para la traza (sin guarda) |
| renderer/Font.cpp:951-1014 | + | 52 | SCRTXT3 en:WEB | caza-todo de textos en la mitad inferior + llamada en PrintString |
| core/Frontend.cpp:469-477 | + | 9 | FEMENU | traza de cambio de pantalla (sin guarda; ODTRACES vacio fuera de web) |
| core/Frontend.cpp:4836-4846 | + | 11 | WR newgame-req en:WEB | traza de peticion de partida nueva |
| core/Frontend.cpp:4896-4902 | + | 7 | VICEEXT save-anywhere en:MOD | traza rechazo ranura autosave |
| core/Frontend.cpp:4920-4929 | + | 10 | VICEEXT save-anywhere en:MOD | traza rechazo |
| core/Frontend.cpp:4932-4938 | + | 7 | VICEEXT save-anywhere en:MOD | traza aceptado |
| core/Frontend.cpp:5606-5618 | + | 13 | WR load-req en:WEB | traza de peticion de carga + reindentacion de 3 lineas vanilla (sin efecto) |
| core/Frontend.cpp:o5476-5478 | - | 3 | WR load-req | reindentacion (sin efecto) |
| core/Frontend.cpp:5736-5745 | + | 10 | FEMENU | traza apertura/cierre de menu (sin guarda) |
| text/Messages.cpp:17-57 | + | 35 | SCRTXT en:WEB | includes + ViceExtTraceText |
| text/Messages.cpp:233-235 | + | 3 | SCRTXT en:WEB | llamada traza brief |
| text/Messages.cpp:318-320 | + | 3 | SCRTXT en:WEB | llamada traza big |
| text/Messages.cpp:342-344 | + | 3 | SCRTXT en:WEB | llamada traza bigQ |
| text/Messages.cpp:534-570 | + | 37 | HINTKEY / P4 kind=hint2 / kind=shift en:MOD | trazas de los iconos de tecla |
| text/Messages.cpp:814-816 | + | 3 | SCRTXT en:WEB | llamada traza brief+str |
| core/Radar.cpp:25-26 | + | 2 | P4 kind=ini | include ondemand.h/Timer.h para la traza |
| core/Radar.cpp:1137-1145 | + | 9 | P4 kind=ini en:MOD | traza color GPS |
| core/Pad.cpp:15-16 | + | 2 | CRAZYHINT / WLOAD | includes ondemand.h y TxdStore.h para trazas |
| core/Pad.cpp:46-50 | + | 5 | cheat-prueba CRAZYRIDES / WLOAD | includes de vehiculos y PlayerPed.h (ViceExtWeaponInfoOf) |
| core/Pad.cpp:322-350 | + | 29 | WLOAD en:WEB | traza de carga de las 8 armas del mod |
| core/Pad.cpp:367-433 | + | 67 | cheat-prueba CRAZYPISTOL / CRAZYRIDES (sin guarda) | WeaponCheat5 + SpawnViceExtendedVehicle |
| core/Pad.cpp:442-498 | + | 57 | cheat-prueba CRAZYHINT / CRAZYRIDES (sin guarda) | HintCheat + VehicleCheat4 |
| core/Pad.cpp:806-817 | + | 12 | P4 kind=cheat2 en:MOD | ViceExtCheatTrace |
| core/Pad.cpp:1529-1538 | + | 10 | cheat-prueba CRAZYRIDES / CRAZYHINT (sin guarda) | activacion en AddToPCCheatString |
| core/Pad.cpp:1814-1829 | + | 16 | cheat-prueba CRAZYPISTOL / CRAZYCOP (sin guarda) | activacion en AddToPCCheatString (CRAZYCOP pone 3 estrellas) |
| core/Pad.cpp:1835 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1840 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1845 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1850 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1855 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1860 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1867 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1869 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1877 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1879 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/Pad.cpp:1885 | + | 1 | P4 kind=cheat2 en:MOD | llamada ViceExtCheatTrace |
| core/ViceExtFeatures.cpp:5 | + | 1 | P4 kind=inifile | include ondemand.h para la traza |
| core/ViceExtFeatures.cpp:214-244 | + | 31 | P4 kind=inifile en:MOD | traza de las 17 claves + resumen |
| text/Messages.cpp:14 | + | 1 | HINTKEY en:MOD | include ondemand.h para la traza HINTKEY |
| core/Wanted.cpp:14 | + | 1 | include ondemand.h en:WEB | include solo para ODTRACES |
| core/Wanted.cpp:19-32 | + | 14 | WANTED | comentario de la instrumentacion de linea base P1 |
| core/Wanted.cpp:59-80 | + | 22 | WANTEDHIDEINIT en:MOD | VICEEXT_HIDE_REV y su historial: solo lo imprime la traza |
| core/Wanted.cpp:82-101 | + | 20 | WANTED tag en:WEB | WantedTraceTag (window.__vcWantedTag via EM_ASM) |
| core/Wanted.cpp:159-242 | + | 84 | WANTED | WantedTraceUnseenFor/Hiding/State (traza 1/s, incluye sus ifdef VICEEXT_HIDE_COPS internos) |
| core/Wanted.cpp:263-271 | + | 9 | WANTEDHIDEINIT en:MOD | linea de revision por sesion |
| core/Wanted.cpp:606-613 | + | 7 | WANTEDCHANGE | if reescrito con llaves para trazar el cambio de nivel |
| core/Wanted.cpp:o363 | - | 1 | WANTEDCHANGE | if vanilla sustituido por la version con llaves de la traza |
| core/Wanted.cpp:719-723 | + | 5 | WANTEDHIDE end en:MOD | traza fin de busqueda |
| core/Wanted.cpp:760-763 | + | 4 | WANTEDHIDE seen en:MOD | traza avistamiento |
| core/Wanted.cpp:776-778 | + | 3 | WANTEDHIDE start en:MOD | traza inicio escondido |
| core/Wanted.cpp:787-790 | + | 4 | WANTEDHIDE drop en:MOD | traza bajada de estrella |
| core/Wanted.cpp:822-824 | + | 3 | WANTED | llamada WantedTraceState en Update |
| core/Wanted.cpp:866-873 | + | 8 | WANTEDPURGE | traza en ResetPolicePursuit |
| core/Wanted.cpp:918-925 | + | 8 | WANTEDSUSPEND | traza en Suspend |
| core/Wanted.h:8-14 | + | 7 | WANTED tag | declaracion WantedTraceTag |
| core/World.cpp:29 | + | 1 | include ondemand.h | include solo para ODTRACES |
| core/World.cpp:388-395 | + | 8 | SHOT rueda en:WEB | traza impacto en rueda |
| core/World.cpp:1950 | + | 1 | P4 kind=distlights | contadores estaticos de la traza |
| core/World.cpp:1957-1959 | + | 3 | P4 kind=distlights | contadores |
| core/World.cpp:1969-1971 | + | 3 | P4 kind=distlights | contadores |
| core/World.cpp:1974-1992 | + | 19 | P4 kind=distlights | traza 1/s + reset de contadores |
| core/World.cpp:2207-2209 | + | 3 | P4 kind=muro en:MOD | contador odSaved (solo traza) |
| core/World.cpp:2217 | + | 1 | P4 kind=muro en:MOD | odSaved++ |
| core/World.cpp:2322-2330 | + | 9 | P4 kind=muro en:MOD | traza de explosiones frenadas por muro |
| control/Script.cpp:24-26 | + | 3 | include ondemand.h en:MOD | include para ODTRACES |
| control/Script.cpp:904-914 | + | 11 | CALL skip en:MOD | traza de saltar llamada |
| control/Script.cpp:1030-1031 | + | 2 | SCMLOOP en:WEB | printf + ODTRACES del corte |
| control/Script.cpp:1623-1635 | + | 13 | VICEEXT autosave en:MOD | traza del autosave |
| control/Script6.cpp:21 | + | 1 | include ondemand.h | include para ODTRACES |
| control/Script6.cpp:1032-1038 | + | 7 | TGTGUN | traza por llamada del opcode |
| control/Script8.cpp:32-33 | + | 2 | P4 kind=modscript en:MOD | includes Timer.h/ondemand.h (solo los usa la traza) |
| control/Script8.cpp:643 | + | 1 | P4 kind=modscript en:MOD | macro VICEEXT_MODSCRIPT_P4_EMIT |
| control/Script8.cpp:682 | + | 1 | P4 kind=modscript en:MOD | buffer odt |
| control/Script8.cpp:694 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA0 |
| control/Script8.cpp:706 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA1 |
| control/Script8.cpp:717 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA2 |
| control/Script8.cpp:726 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA3 |
| control/Script8.cpp:730 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA4 |
| control/Script8.cpp:734 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA5 |
| control/Script8.cpp:738 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA6 |
| control/Script8.cpp:742 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA7 |
| control/Script8.cpp:750 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA8 |
| control/Script8.cpp:761 | + | 1 | P4 kind=modscript en:MOD | emit 0x0FA9 |
| control/Script8.cpp:767 | + | 1 | P4 kind=modscript en:MOD | emit opcode desconocido |
| renderer/ParticleMgr.cpp:6-7 | + | 2 | P4 kind=partcfg | includes Timer.h/ondemand.h para la traza |
| renderer/ParticleMgr.cpp:133 | + | 1 | P4 kind=partcfg en:MOD | macro VICEEXT_P4_EMIT |
| renderer/ParticleMgr.cpp:158-166 | + | 9 | P4 kind=partcfg en:MOD | contadores de carga (else del ifndef) |
| renderer/ParticleMgr.cpp:217 | + | 1 | P4 kind=partcfg en:MOD | contador |
| renderer/ParticleMgr.cpp:223 | + | 1 | P4 kind=partcfg en:MOD | contador |
| renderer/ParticleMgr.cpp:225 | + | 1 | P4 kind=partcfg en:MOD | emit desconocido |
| renderer/ParticleMgr.cpp:233-235 | + | 3 | P4 kind=partcfg en:MOD | contadores |
| renderer/ParticleMgr.cpp:240-241 | + | 2 | P4 kind=partcfg en:MOD | emit nombre_largo |
| renderer/ParticleMgr.cpp:431-437 | + | 7 | P4 kind=partcfg en:MOD | resumen de carga |
| renderer/Particle.cpp:24-26 | + | 3 | P4 kind=particulas en:MOD | include ondemand.h |
| renderer/Particle.cpp:887 | + | 1 | P4 kind=particulas en:MOD | odPolishTraced |
| renderer/Particle.cpp:939-946 | + | 8 | P4 kind=particulas en:MOD | traza una vez de vida escalada |
| renderer/Clouds.cpp:17 | + | 1 | include ondemand.h | include para ODTRACES |
| renderer/Clouds.cpp:155-171 | + | 17 | P4 kind=moon en:MOD | traza 1/s de la luna |
| buildings/Building.cpp:14 | + | 1 | include ondemand.h | include sin uso (no hay ODTRACES) |
| entities/Dummy.cpp:14 | + | 1 | include ondemand.h | include sin uso (no hay ODTRACES) |
| core/ZoneCull.cpp:173-183 | + | 11 | P4 kind=worlddata en:MOD | ViceExtNumZones: solo lo usa la traza de CGame::Initialise |
| core/ZoneCull.h:54-60 | + | 7 | P4 kind=worlddata en:MOD | declaracion ViceExtNumZones |

## Anexo B. Filas NADO y RADIO revisadas a mano (además de lo autoclasificado por `VICEEXT_SWIM*` / `VICEEXT_RADIO_ICONS`)

### NADO

| Fichero:líneas | ± | Nº | Bandera/feature | Descripción |
|---|---|---:|---|---|
| peds/PlayerPed.cpp:5 | + | 1 | NADO dep PedArbiter (include sin guarda) | include PedArbiter.h: carriles PEDLANE_NADO que usa el nado (tambien CROUCH y ClassicAXIS) |
| peds/PlayerPed.cpp:9-10 | + | 2 | NADO (include sin guarda) | soundlist.h (SOUND_NADO_BRAZO_*) y Timecycle.h (color salpicadura nado) |
| peds/PlayerPed.cpp:34 | + | 1 | NADO (include sin guarda) | WaterLevel.h (CWaterLevel::GetWaterLevel del nado; tb CLIMB y sonda) |
| peds/PlayerPed.cpp:36 | + | 1 | NADO (include sin guarda) | ParticleObject.h (POBJECT_PED_WATER_SPLASH) |
| peds/PlayerPed.cpp:38 | + | 1 | NADO (include sin guarda) | Particle.h (salpicadura por brazada) |
| peds/PlayerPed.cpp:136 | + | 1 | VICEEXT_SWIMMING | linea en blanco tras s_odSwimSlot |
| peds/PlayerPed.cpp:3556 | + | 1 | VICEEXT_SWIMMING | blanco antes del bloque de nado |
| peds/PlayerPed.cpp:6611 | + | 1 | NADO (reindentado neutro) | ProcessPlayerWeapon reindentado para meter la puerta del nado |
| peds/PlayerPed.cpp:6617 | + | 1 | NADO (reindentado neutro) |  |
| peds/PlayerPed.cpp:6626 | + | 1 | NADO (reindentado neutro) |  |
| peds/PlayerPed.cpp:o1826-1828 | - | 3 | NADO (reindentado neutro) | mismas lineas reindentadas |
| core/Cam.cpp:1180-1199 | + | 20 | VICEEXT_SWIMMING(comentario sin guarda) | comentario de IsTargetInWater: nadar no es caer al agua |
| peds/Ped.cpp:10 | + | 1 | VICEEXT_SWIMMING(sin guarda) | include PedArbiter.h: flotacion/ahogo del nado (ViceExtPedOwns PEDLANE_NADO) |
| peds/Ped.cpp:1983 | + | 1 | VICEEXT_SWIMMING(sin guarda) | ProcessBuoyancy: odSwimmingPlayer anula impulso de flotacion, bloque de ahogo (bIsDrowning+dano) y freno/salpicadura de entrada (puerta sin guarda) |
| peds/Ped.cpp:1988-1989 | + | 2 | VICEEXT_SWIMMING(sin guarda) | ProcessBuoyancy: odSwimmingPlayer anula impulso de flotacion, bloque de ahogo (bIsDrowning+dano) y freno/salpicadura de entrada (puerta sin guarda) |
| peds/Ped.cpp:2017-2030 | + | 7 | VICEEXT_SWIMMING(sin guarda) | ProcessBuoyancy: odSwimmingPlayer anula impulso de flotacion, bloque de ahogo (bIsDrowning+dano) y freno/salpicadura de entrada (puerta sin guarda) |
| peds/Ped.cpp:o1658-1692 | - | 5 | VICEEXT_SWIMMING(sin guarda) | ProcessBuoyancy vanilla reescrito para el nado |
| peds/Ped.cpp:2903 | + | 1 | VICEEXT_SWIMMING(sin guarda) | ProcessControl: no CheckIfInTheAir mientras PEDLANE_NADO tiene el movimiento |
| peds/Ped.cpp:o2565 | - | 1 | VICEEXT_SWIMMING(sin guarda) | condicion vanilla de caida reescrita |
| peds/Ped.cpp:3437-3446 | + | 3 | VICEEXT_SWIMMING(sin guarda) | particleProduceFootSplash deja de ser static y recibe color (lo usa la brazada, PlayerPed.cpp:3921) |
| peds/Ped.cpp:o3099-3108 | - | 3 | VICEEXT_SWIMMING(sin guarda) | firma/color vanilla de particleProduceFootSplash |
| peds/Ped.h:1221 | + | 1 | VICEEXT_SWIMMING(sin guarda) | declaracion particleProduceFootSplash(color) que usa el nado |
| core/PedArbiter.h:1-25 | + | 25 | VICEEXT_PEDARBITER(sin guarda) | API del arbitro (carriles MOTOR/NADO/AGACHADO_MOD/LEY, capacidades) |
| core/PedArbiter.cpp:1-3 | + | 3 | VICEEXT_PEDARBITER | includes (ViceExtFeatures para EnableSwimming) |
| core/PedArbiter.cpp:5-46 | + | 42 | VICEEXT_PEDARBITER | estado y Owner/Owns/Claim (EnableSwimming apaga el carril NADO) |
| core/PedArbiter.cpp:54-56 | + | 3 | VICEEXT_PEDARBITER | claim |
| core/PedArbiter.cpp:65-83 | + | 19 | VICEEXT_PEDARBITER | callback de release + Release |
| core/PedArbiter.cpp:92-123 | + | 32 | VICEEXT_PEDARBITER | callback, OnRelease, ResetAll |
| core/config.h:390 | + | 1 | VICEEXT_SWIMMING | #define de bandera en config.h (bloque features.ini del mod) |
| animation/AnimManager.cpp:935-970 | + | 36 | NADO sin guarda | aSwimAnimations + aSwimAnimDescs (swim_tread/crawl/breast/jumpout) |
| animation/AnimManager.cpp:1115-1117 | + | 3 | NADO sin guarda | grupo playerswim (bloque ped) |
| animation/AnimManager.h:79-81 | + | 3 | NADO sin guarda | ASSOCGRP_PLAYERSWIM |
| animation/AnimationId.h:287-295 | + | 9 | NADO sin guarda | ANIM_STD_SWIM_TREAD/CRAWL/BREAST/JUMPOUT (tras ANIM_STRIP_G) |
| audio/soundlist.h:206-211 | + | 6 | NADO (sin guarda) SOUND_NADO_BRAZO_* | 4 eventos de brazada + TOTAL/NO_SOUND=208 |
| audio/soundlist.h:o206-207 | - | 2 | NADO (sin guarda) | TOTAL=204/NO_SOUND=205 vanilla |
| audio/AudioLogic.cpp:4533-4561 | + | 29 | NADO (sin guarda) SOUND_NADO_BRAZO_* | one-shot de brazada -> SFX_FOOTSTEP_WATER_1..4 |
| renderer/Hud.cpp:28 | + | 1 | PEDARBITER (sin guarda) | include PedArbiter.h: lo usa el ocultado de la cruz nadando (l.455) |
| renderer/Hud.cpp:455 | + | 1 | PEDARBITER (sin guarda) | no pintar cruz si el carril NADO posee PEDCAP_APUNTAR |

### RADIO

| Fichero:líneas | ± | Nº | Bandera/feature | Descripción |
|---|---|---:|---|---|
| core/config.h:473 | + | 1 | VICEEXT_RADIO_ICONS | #define de bandera en config.h (bloque features.ini del mod) |
| audio/MusicManager.cpp:47-51 | + | 5 | __EMSCRIPTEN__ retune | gRetuneStartMs: ventana de retune en ms reales (web, no mod) |
| audio/MusicManager.cpp:574-580 | + | 5 | __EMSCRIPTEN__ retune | tap F9: arranque de ventana de retune en ms + estatica (web) |
| audio/MusicManager.cpp:595-601 | + | 5 | __EMSCRIPTEN__ retune | tap cambio de emisora: ventana en ms (web) |
| audio/MusicManager.cpp:614-620 | + | 5 | __EMSCRIPTEN__ retune | rueda raton: ventana en ms (web) |
| audio/MusicManager.cpp:700-704 | + | 5 | __EMSCRIPTEN__ retune | expira la ventana de retune por tiempo real (600 ms) |
| audio/MusicManager.cpp:1425 | + | 1 | VICEEXT_RADIO_ICONS (sin guarda) | linea vanilla wchar* string desplazada tras el bloque de iconos |
| audio/MusicManager.cpp:o1312 | - | 1 | VICEEXT_RADIO_ICONS (sin guarda) | misma linea vanilla, movida |
| audio/MusicManager.cpp:o1345 | - | 1 | VICEEXT_RADIO_ICONS (sin guarda) | linea en blanco sustituida por el bloque odStation |
| audio/MusicManager.cpp:1512 | + | 1 | VICEEXT_RADIO_ICONS (sin guarda) | sombra del nombre en odTextY (baja el texto si hay icono) |
| audio/MusicManager.cpp:o1367 | - | 1 | VICEEXT_RADIO_ICONS (sin guarda) | vanilla Y=22 fija |
| audio/MusicManager.cpp:1519 | + | 1 | VICEEXT_RADIO_ICONS (sin guarda) | nombre de emisora en odTextY |
| audio/MusicManager.cpp:o1374 | - | 1 | VICEEXT_RADIO_ICONS (sin guarda) | vanilla Y=22 fija |

## Anexo C. Filas INFRA-WEB (4168 líneas del diff: 3 337 añadidas + 831 borradas)

| Fichero:líneas | ± | Nº | Bandera/feature | Descripción |
|---|---|---:|---|---|
| peds/PedAI.cpp:4889-4911 | + | 21 | streaming web (gWebLoadBudget) | SetAnimOffsetForEnterOrExitVehicle: carga sin tope y salida segura si faltan bloques de anim (arreglo de crash OOB del port) |
| skel/glfw/glfw.cpp:50-67 | + | 10 | glfw-web | includes para EmscriptenTick (CutsceneMgr/Draw/Camera) y emscripten.h |
| skel/glfw/glfw.cpp:185-191 | + | 6 | idbfs | _psGetUserFilesFolder absoluta /userfiles (IDBFS) |
| skel/glfw/glfw.cpp:o261 | - | 1 | glfw-web | CLOCK_MONOTONIC_RAW sustituido por rama Emscripten |
| skel/glfw/glfw.cpp:280-285 | + | 6 | glfw-web | reloj CLOCK_MONOTONIC en Emscripten (arreglo de CTimer) |
| skel/glfw/glfw.cpp:431-450 | + | 20 | glfw-web | log [web] + tamano de canvas desde el layout |
| skel/glfw/glfw.cpp:464-530 | + | 9 | glfw-web | logs [web] de psInitialize |
| skel/glfw/glfw.cpp:o560 | - | 1 | glfw-web | sysinfo (reindentado dentro de #else) |
| skel/glfw/glfw.cpp:612-647 | + | 12 | glfw-web | stub de memoria fisica (sin sysinfo) + log |
| skel/glfw/glfw.cpp:719-732 | + | 7 | glfw-web | lista de modos de video en ventana |
| skel/glfw/glfw.cpp:905-1011 | + | 38 | glfw-web | seleccion de modo de video del canvas + logs setvideomode |
| skel/glfw/glfw.cpp:1065-1125 | + | 11 | glfw-web | joystick/gamepad sin API de mappings en GLFW web |
| skel/glfw/glfw.cpp:1232-1240 | + | 9 | glfw-web | redimensionar canvas al cambiar modo |
| skel/glfw/glfw.cpp:1697-1708 | + | 12 | glfw-web | filtro de auto-repeat de teclas (taps fantasma) |
| skel/glfw/glfw.cpp:2011-2681 | + | 607 | mainloop | main() troceado en OuterSetup/InnerFrame/AfterInner/FinalCleanup + EmscriptenTick (emscripten_set_main_loop), carga diferida, tope de fps |
| skel/glfw/glfw.cpp:2777-2788 | + | 6 | glfw-web | logs [web] RWINITIALIZE |
| skel/glfw/glfw.cpp:2897-2910 | + | 10 | mainloop | main(): emscripten_set_main_loop / bucle nativo con las piezas extraidas |
| skel/glfw/glfw.cpp:o2081-2466 | - | 383 | mainloop | bucle while() de main() movido a OuterSetup/InnerFrame/AfterInner/FinalCleanup |
| skel/glfw/glfw.cpp:2951-2957 | + | 6 | glfw-web | CapturePad sin glfwGetGamepadState |
| skel/skeleton.cpp:8-237 | + | 16 | idbfs | salir: sync de userfiles + recarga de pagina |
| skel/skeleton.cpp:332-397 | + | 18 | glfw-web | logs [web] rwinit |
| skel/ondemand.cpp:1-129 | + | 129 | ondemand | capa on-demand: __wrap_fopen/open/stat/access + od_fetch (Asyncify) + odBlockingPush/Pop |
| skel/ondemand.h:5-7 | + | 3 | ondemand | pragma + include emscripten |
| skel/ondemand.h:10-26 | + | 17 | ondemand | odBlockingPush/Pop (carga bloqueante: no aplazar ficheros) |
| skel/ondemand.h:29-31 | + | 3 | ondemand | stubs nativos odBlockingPush/Pop |
| core/main.h:45-61 | + | 16 | carga-web | InitialiseGameStep, WebDrawLoadScreen/Begin/End, gWebLoadFrac, gWebBootInitPending |
| core/main.h:64-66 | + | 3 | fps-web | WebDrawFps (contador por API de pagina) |
| core/main.cpp:116-128 | + | 13 | carga-web | gWebLoadScreenActive |
| core/main.cpp:579-586 | + | 8 | carga-web | LoadSplash fija splash1 durante carga web |
| core/main.cpp:700-710 | + | 11 | carga-web | LoadingScreen delega en WebDrawLoadScreen |
| core/main.cpp:790-794 | + | 5 | carga-web | gWebLoadFrac/gWebBootInitPending |
| core/main.cpp:913-950 | + | 38 | fps-web | WebDrawFps (contador en esquina por API de pagina) |
| core/main.cpp:958-1003 | + | 46 | carga-web | WebDrawFps (pintado) + WebDrawLoadScreen (barra monotona) |
| core/main.cpp:1011-1034 | + | 24 | carga-web | WebDrawLoadScreen (splash + geometria; usa macros SilentPatch-loadbar) |
| core/main.cpp:1049-1071 | + | 23 | carga-web | WebDrawLoadScreen (barra) + WebBeginLoadScreen |
| core/main.cpp:1073-1080 | + | 8 | carga-web | WebEndLoadScreen |
| core/main.cpp:1082-1084 | + | 3 | carga-web | cierre bloque web |
| core/main.cpp:2064 | + | 1 | fps-web | WebDrawFps() en el frame |
| core/main.cpp:2117-2160 | + | 33 | carga-web | WebDrawFps en menu + precarga splash1 + InitialiseGameStep (carga progresiva) |
| core/main.cpp:2400-2404 | + | 4 | whitespace | reindentado en rama GTA_PS2 (no compila en web) |
| core/main.cpp:o1970-1974 | - | 4 | whitespace | reindentado en rama GTA_PS2 (no compila en web) |
| core/CdStreamPosix.cpp:14 | + | 1 | cdstream | include errno.h |
| core/CdStreamPosix.cpp:111-122 | + | 12 | cdstream | estructuras de imagen suelta (gOdLoose/gOdIsLoose, dimensionadas por MAX_CDIMAGES) |
| core/CdStreamPosix.cpp:200-237 | + | 38 | cdstream | OdDoRead: lectura sincrona archivo/suelto |
| core/CdStreamPosix.cpp:240-253 | + | 14 | cdstream | OdDoRead (cont.) |
| core/CdStreamPosix.cpp:257-258 | + | 2 | cdstream | OdDoRead (cont.) |
| core/CdStreamPosix.cpp:262-278 | + | 17 | cdstream | OdDoRead (cont.) |
| core/CdStreamPosix.cpp:291-300 | + | 10 | cdstream | OdDoRead (fin) |
| core/CdStreamPosix.cpp:320-542 | + | 24 | cdstream | modo monohilo: sin hilos/semaforos, CdStreamRead en linea, tamano de imagen suelta |
| core/CdStreamPosix.cpp:770-847 | + | 74 | cdstream | AddImage: verificacion de fichero + preferencia por directorio suelto (.dir) |
| core/CdStreamPosix.cpp:887-894 | + | 8 | cdstream | liberar entradas sueltas al cerrar |
| core/Streaming.h:212-225 | + | 14 | carga-web | LoadSceneStep/ResetSteps/CountPendingRequests/gWebLoadBudget |
| core/Streaming.cpp:2-4 | + | 3 | ondemand | include ondemand.h (odBlockingPush/Pop) |
| core/Streaming.cpp:o343-348 | - | 3 | streaming-web | puerta del cargador de trafico refactorizada |
| core/Streaming.cpp:346-368 | + | 20 | streaming-web | puerta de trafico sin nreq<5/prioridad en web (D4) |
| core/Streaming.cpp:382-387 | + | 3 | streaming-web | rama nativa de la puerta |
| core/Streaming.cpp:553-570 | + | 18 | streaming-web | enfriamiento de re-solicitud (OdCoolingDown) |
| core/Streaming.cpp:602-604 | + | 3 | streaming-web | OdCoolingDown |
| core/Streaming.cpp:662-664 | + | 3 | streaming-web | OdCoolingDown |
| core/Streaming.cpp:726-728 | + | 3 | streaming-web | OdCoolingDown |
| core/Streaming.cpp:751-753 | + | 3 | streaming-web | OdCoolingDown |
| core/Streaming.cpp:o1716-1718 | - | 2 | streaming-web | temporizador de carga de vehiculos |
| core/Streaming.cpp:1879-1893 | + | 14 | streaming-web | temporizador de trafico en tiempo real (D4) |
| core/Streaming.cpp:1943-1945 | + | 3 | streaming-web | marca de ultima carga |
| core/Streaming.cpp:o2416-2419 | - | 2 | streaming-web | reintento sin tope sustituido |
| core/Streaming.cpp:2653-2659 | + | 5 | streaming-web | reintento acotado (25) en LoadAllRequestedModels |
| core/Streaming.cpp:2700-2704 | + | 5 | ondemand | odBlockingPush (carga bloqueante) |
| core/Streaming.cpp:o2466-2477 | - | 2 | streaming-web | bucle de LoadAll con presupuesto / do-while corregido |
| core/Streaming.cpp:2711-2720 | + | 10 | carga-web | presupuesto de ficheros por llamada (gWebLoadBudget) |
| core/Streaming.cpp:2735-2761 | + | 24 | streaming-web | do/while de CdStreamSync corregido (D15, pantalla negra) con tope |
| core/Streaming.cpp:2805 | + | 1 | ondemand | odBlockingPop |
| core/Streaming.cpp:3442-3449 | + | 8 | carga-web | gWebLoadBudget |
| core/Streaming.cpp:3454-3572 | + | 119 | carga-web | LoadSceneStep troceado + CountPendingRequests |
| core/Game.h:62-74 | + | 8 | carga-web | InitialiseResetSteps/InitialiseStep + pasos de restart/shutdown troceados |
| CMakeLists.txt:55-68 | + | 14 | build-web | stamp de bootseed como LINK_DEPENDS |
| core/re3.cpp:o197 | - | 1 | idbfs | ruta de reVC.ini |
| core/re3.cpp:197-205 | + | 9 | idbfs | reVC.ini en /userfiles (IDBFS) |
| core/re3.cpp:1323-1338 | + | 13 | crash-port | asserts acotados a 30 en web |
| core/config.h:615-627 | + | 7 | audio-web | PS2_AUDIO_PATHS off y sin MULTITHREADED_AUDIO en web |
| core/config.h:667-675 | + | 7 | glfw-web | USE_UNNAMED_SEM y sin USE_TXD_CDIMAGE en web |
| core/Game.cpp:o3 | - | 1 | carga-web | linea en blanco |
| core/Game.cpp:6-24 | + | 7 | carga-web | includes del cargador troceado (crossplatform, Lists, PlayerInfo, emscripten heap, ColStore) |
| core/Game.cpp:108-120 | + | 13 | carga-web | WEB_LOAD_PROGRESS/WEB_LOAD_OVERLAY + NUM_INIT_STEPS |
| core/Game.cpp:383-385 | + | 3 | glfw-web | log [web] once-afterRW |
| core/Game.cpp:o366-587 | - | 166 | carga-web | CGame::Initialise monolitico troceado en InitialiseStep (19 pasos) |
| core/Game.cpp:397-736 | + | 279 | carga-web | InitialiseResetSteps/InitialiseStep (switch por pasos, nativo y web) + Initialise envolvente |
| core/Game.cpp:934-986 | + | 53 | carga-web | ShutDownForRestartStep troceado |
| core/Game.cpp:o837 | - | 1 | whitespace | espacio final eliminado |
| core/Game.cpp:1057 | + | 1 | whitespace | espacio final eliminado |
| core/Game.cpp:1064-1259 | + | 173 | carga-web | InitialiseRestartStep: carga de partida troceada (parse, drenado colision, escena) |
| save/GenericGameStorage.cpp:344-350 | + | 7 | save/IDBFS | re-deriva LoadFileName del slot actual antes de leer |
| save/GenericGameStorage.cpp:356 | + | 1 | save/IDBFS | llave de cierre del if reestructurado |
| save/GenericGameStorage.cpp:o302-305 | - | 2 | save/IDBFS | lecturas ahora comprobadas |
| save/GenericGameStorage.cpp:366-376 | + | 9 | save/IDBFS | cuerpo incompleto -> CloseFile + return false (antes seguia con basura) |
| save/GenericGameStorage.cpp:546-558 | + | 13 | save/carga diferida web | gWebDeferSceneLoad: carga por pasos |
| save/GenericGameStorage.cpp:o476 | - | 1 | save/carga diferida web (SIN guarda: tambien nativo) | quita CStreaming::LoadSceneCollision del tail de carga |
| save/GenericGameStorage.cpp:568-579 | + | 12 | save/carga diferida web | DoGameSpecificStuffAfterSucessLoadDeferred + flags |
| save/GenericGameStorage.cpp:584-591 | + | 8 | save/IDBFS | LoadFileName fijado al slot (antes vacio en la ruta PC) |
| save/GenericGameStorage.cpp:o489 | - | 1 | save/IDBFS | if reescrito con slotOk |
| save/GenericGameStorage.cpp:602 | + | 1 | save/IDBFS | if (slotOk) |
| save/GenericGameStorage.h:19-23 | + | 5 | save/carga diferida web | externs gWebDefer* |
| save/PCSave.cpp:4-11 | + | 8 | save/IDBFS | WEB_FLUSH_USERFILES (OD.syncUserfiles) |
| save/PCSave.cpp:36-42 | + | 6 | save/IDBFS | DefaultPCSaveFileName con / y /userfiles |
| save/PCSave.cpp:56 | + | 1 | save/IDBFS | resultado de DeleteFile guardado |
| save/PCSave.cpp:o36 | - | 1 | save/IDBFS | DeleteFile sin resultado |
| save/PCSave.cpp:66-70 | + | 4 | save/IDBFS | DeleteSlot devuelve delok==0 |
| save/PCSave.cpp:88 | + | 1 | save/IDBFS | flush IDBFS tras guardar |
| rw/TexRead.cpp:97-116 | + | 7 | texturas/crash | saltar textura fallida sin destruir el diccionario (via small) |
| rw/TexRead.cpp:153-170 | + | 6 | texturas/crash | saltar textura fallida (via 1) |
| rw/TexRead.cpp:191-206 | + | 4 | texturas/crash | saltar textura fallida (via 2) |
| fakerw/fake.cpp:804-805 | + | 2 | wasm | comentario: cast exacto por call_indirect de wasm |
| fakerw/fake.cpp:o780 | - | 1 | wasm | comentario illegal cast sustituido |
| renderer/WaterLevel.cpp:127-139 | + | 11 | on-demand/crash | sin waterpro.dat: break en vez de bucle infinito |
| text/Text.cpp:26-57 | + | 32 | GXT/crash | OdTextFileName (default) + OdTextFail (printf) |
| text/Text.cpp:65-114 | + | 32 | GXT/crash | CText::Load robusto (apertura, EOF, chunk completo) |
| text/Text.cpp:241-336 | + | 49 | GXT/crash | ReadChunkHeader devuelve bytes; LoadMissionText robusto (cuelgue pantalla negra) |
| text/Text.cpp:352 | + | 1 | GXT/crash | else |
| text/Text.cpp:361-395 | + | 12 | GXT/crash | OdTextFail + CKeyArray::Load devuelve bytes |
| text/Text.cpp:475-497 | + | 8 | GXT/crash | CData::Load devuelve bytes |
| text/Text.cpp:o28-419 | - | 105 | GXT/crash | switch de idioma duplicado y bucles de chunks sin comprobar (sustituidos) |
| text/Text.h:29-95 | + | 3 | GXT/crash | Load/ReadChunkHeader devuelven size_t |
| text/Text.h:o29-95 | - | 3 | GXT/crash | firmas void vanilla |
| animation/AnimManager.cpp:1475-1493 | + | 19 | on-demand/crash | comprobacion de tabla de bloques/anims llena antes de consumir hueco |
| animation/AnimManager.cpp:1638-1654 | + | 17 | on-demand/crash | RemoveLastAnimFile no libera rangos invalidos (doble free) |
| animation/AnimBlendAssocGroup.cpp:182-196 | + | 15 | anim/crash | clip sin jerarquia: aviso y asociacion marcada en vez de crash |
| animation/AnimBlendAssocGroup.cpp:o182 | - | 1 | anim/crash | Init directo vanilla |
| animation/CutsceneMgr.cpp:190 | + | 1 | on-demand cinematica | vcCutscenePublishLoading |
| animation/CutsceneMgr.cpp:230-258 | + | 6 | on-demand/crash | comprobaciones stream/file nulos |
| animation/CutsceneMgr.cpp:o209-235 | - | 4 | on-demand/crash | usos sin comprobar de stream/file |
| animation/CutsceneMgr.cpp:671 | + | 1 | on-demand cinematica | vcCutscenePublishLoading |
| animation/CutsceneMgr.h:3-20 | + | 18 | on-demand cinematica | vcCutscenePublishLoading (window.__vcInGame=false) |
| animation/CutsceneMgr.h:49 | + | 1 | on-demand cinematica | StartCutsceneProcessing avisa a la pagina |
| animation/CutsceneMgr.h:o31 | - | 1 | on-demand cinematica | StartCutsceneProcessing vanilla |
| audio/MusicManager.cpp:140-141 | + | 2 | radio sin ficheros | ResetMusicAfterReload: evita modulo por longitud 0 |
| audio/MusicManager.cpp:o119 | - | 1 | radio sin ficheros | linea vanilla sustituida (modulo por 0) |
| audio/MusicManager.cpp:189-193 | + | 2 | radio sin ficheros | SetStartingTrackPositions: salta pistas de longitud 0 (+ espacio) |
| audio/MusicManager.cpp:o167-171 | - | 2 | radio sin ficheros | lineas vanilla sustituidas |
| audio/MusicManager.cpp:263-265 | + | 3 | radio sin ficheros | SetRadioChannelByScript: longitud 0 -> -1 (y reindentado) |
| audio/MusicManager.cpp:o233-235 | - | 3 | radio sin ficheros | lineas vanilla sustituidas |
| audio/MusicManager.cpp:1263 | + | 1 | radio sin ficheros | GetTrackStartPos: longitud 0 |
| audio/MusicManager.cpp:o1196 | - | 1 | radio sin ficheros | linea vanilla sustituida |
| audio/sampman_oal.cpp:4 | + | 1 | ondemand | include ondemand.h (odBlockingPush/Pop + trazas) |
| audio/sampman_oal.cpp:90-123 | + | 12 | SFX on-demand | blancos + cache SFX mp3 por muestra (mpg123), tablas odSfx* |
| audio/sampman_oal.cpp:139-212 | + | 55 | SFX on-demand | tope de cache, presupuesto por frame, cola, OdSfxInUse/OdSfxEvict |
| audio/sampman_oal.cpp:216-233 | + | 18 | SFX on-demand | fin evict + prototipo |
| audio/sampman_oal.cpp:244-328 | + | 80 | SFX on-demand | cola de diferidos, OdSfxPump, OdSfxDecode (inicio) |
| audio/sampman_oal.cpp:334-490 | + | 110 | SFX on-demand | cola/decode mpg123 gapless, mono, ajuste a nSize, registro en cache |
| audio/sampman_oal.cpp:547-620 | + | 16 | OpenAL Web Audio | proveedor unico Web Audio (sin enumeracion ALC/EFX) |
| audio/sampman_oal.cpp:1358-1370 | + | 8 | SFX on-demand | libera el banco monolitico SFX_BANK_0 (+ espacios) |
| audio/sampman_oal.cpp:o962-968 | - | 2 | SFX on-demand | espacios vanilla |
| audio/sampman_oal.cpp:1547-1555 | + | 9 | SFX on-demand | Terminate libera la cache PCM |
| audio/sampman_oal.cpp:1661-1672 | + | 7 | SFX on-demand | LoadSampleBank sin carga masiva |
| audio/sampman_oal.cpp:o1249 | - | 1 | SFX on-demand | espacio vanilla |
| audio/sampman_oal.cpp:1708-1720 | + | 8 | SFX on-demand | LoadMissionAudio desde mp3 por muestra |
| audio/sampman_oal.cpp:o1288 | - | 1 | SFX on-demand | espacio vanilla |
| audio/sampman_oal.cpp:1805-1818 | + | 10 | SFX on-demand | LoadPedComment desde mp3 por muestra |
| audio/sampman_oal.cpp:o1380 | - | 1 | SFX on-demand | espacio vanilla |
| audio/sampman_oal.cpp:1979 | + | 1 | espacios | espacio |
| audio/sampman_oal.cpp:o1527 | - | 1 | espacios | espacio vanilla |
| audio/sampman_oal.cpp:1998-2000 | + | 3 | SFX on-demand | PCM por muestra en InitialiseChannel |
| audio/sampman_oal.cpp:o1532 | - | 1 | espacios | espacio vanilla |
| audio/sampman_oal.cpp:2002-2007 | + | 6 | SFX on-demand | OdSfxDecode + direccion PCM |
| audio/sampman_oal.cpp:2048 | + | 1 | espacios | espacio |
| audio/sampman_oal.cpp:o1573 | - | 1 | espacios | espacio vanilla |
| audio/sampman_oal.cpp:o1700 | - | 1 | espacios | espacio vanilla |
| audio/sampman_oal.cpp:2207-2246 | + | 32 | audio de mision bloqueante | PreloadStreamedFile: odBlockingPush/Pop + segundo intento para WAV de mision |
| audio/sampman_oal.cpp:2564-2574 | + | 11 | audio de mision bloqueante | IsStreamedFileOpened() |
| audio/sampman_oal.cpp:2578-2580 | + | 3 | SFX on-demand | OdSfxPump en Service |
| audio/sampman_oal.cpp:2604-2694 | + | 57 | SFX on-demand / Asyncify | SDT por read() directo, sin sfx.RAW, tamano derivado de la tabla |
| audio/sampman_oal.cpp:o2040-2046 | - | 4 | SFX on-demand | fseek/ftell vanilla sobre sfx.RAW sustituidos |
| audio/sampman_null.cpp:330-339 | + | 10 | audio de mision bloqueante | IsStreamedFileOpened (backend nulo) |
| audio/sampman_miles.cpp:2530-2538 | + | 9 | audio de mision bloqueante | IsStreamedFileOpened (MSS) |
| audio/sampman.h:254 | + | 1 | audio de mision bloqueante | declaracion IsStreamedFileOpened |
| audio/oal/channel.cpp:187-194 | + | 8 | SFX on-demand | CChannel::UsesData (para no liberar PCM en uso) |
| audio/oal/channel.h:7-45 | + | 3 | OpenAL web | sin efx.h en web + UsesData |
| audio/oal/oal_utils.cpp:52-190 | + | 11 | OpenAL web | EFX no-op en web |
| audio/oal/oal_utils.h:5-67 | + | 62 | OpenAL web | declaraciones EFX minimas para compilar en web |
| audio/oal/stream.cpp:1281-1308 | + | 28 | streams radio .adf/mp3 | .adf acepta MP3 plano (sniff sync/ID3) o ADF XOR |
| audio/oal/stream.cpp:o1281-1282 | - | 2 | streams radio .adf/mp3 | apertura vanilla ADF |
| audio/oal/stream.cpp:1733-1766 | + | 32 | arreglo freeze stream | reindentado + do/while acotado (desync de sources estereo) |
| audio/oal/stream.cpp:o1707-1720 | - | 12 | arreglo freeze stream | bloque vanilla reindentado |
| audio/AudioLogic.cpp:10278-10297 | + | 16 | audio de mision bloqueante | ProcessMissionAudioSlot: LOADED solo si el fichero abrio + reintento |
| audio/AudioLogic.cpp:o10116-10120 | - | 2 | audio de mision bloqueante | lineas vanilla sustituidas |
| audio/AudioManager.cpp:206-218 | + | 13 | arreglo loops huerfanos | DestroyEntity para loops del emisor |
| audio/AudioManager.cpp:1361-1426 | + | 56 | watchdog loops | corta loops huerfanos/inaudibles (bucle del Malibu) |
| objects/Stinger.cpp:124-126 | + | 3 | crash-fix nil-check (no-mod) | CheckForBurstTyres sin pSpikes |
| objects/Stinger.cpp:165-166 | + | 2 | crash-fix nil-check (no-mod) | spike nulo |
| objects/Stinger.cpp:202 | + | 1 | crash-fix nil-check (no-mod) | asociación THROW_UNDER nula |
| renderer/Font.cpp:80-145 | + | 10 | colateral-codificacion | comentarios re-codificados (byte Latin-1 -> U+FFFD), sin efecto |
| renderer/Font.cpp:o74-139 | - | 10 | colateral-codificacion | idem (version vanilla de esos comentarios) |
| renderer/Font.cpp:200-249 | + | 10 | colateral-codificacion | idem en el bloque MORE_LANGUAGES |
| renderer/Font.cpp:o194-243 | - | 10 | colateral-codificacion | idem |
| core/Frontend.cpp:7-10 | + | 4 | emscripten/ondemand | includes (EM_ASM del sync IDBFS; ondemand tambien da las trazas) |
| core/Frontend.cpp:3342-3347 | + | 6 | IDBFS | sync de ajustes a IDBFS en SaveSettings |
| core/MenuScreensCustom.cpp:o33-502 | - | 48 | colateral-blancos | lineas en blanco eliminadas, sin efecto |
| core/MenuScreensCustom.cpp:o506-558 | - | 7 | colateral-blancos | lineas en blanco eliminadas, sin efecto |
| core/MenuScreensCustom.cpp:o560-817 | - | 33 | colateral-blancos | lineas en blanco eliminadas, sin efecto |
| control/Script.cpp:1019-1029 | + | 11 | SCM watchdog | tope de 200000 comandos por guion y frame (anti-cuelgue de la pestana) |
| control/Script.cpp:1032-1039 | + | 6 | SCM watchdog | resto del tope (else vanilla) |

## Anexo D. Filas MOD-VIEJO revisadas a mano (reasignaciones de bandera, ganchos neutros, defines de config.h; el resto de MOD-VIEJO se asigna por su `#ifdef`, ver §5.2)

| Fichero:líneas | ± | Nº | Bandera/feature | Descripción |
|---|---|---:|---|---|
| peds/PlayerPed.cpp:46 | + | 1 | VICEEXT_AIM_CLASSICAXIS | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:54 | + | 1 | VICEEXT_CROUCH | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:61 | + | 1 | VICEEXT_AIM_WALK | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:71 | + | 1 | VICEEXT_AIM_WALK | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:83 | + | 1 | VICEEXT_CROUCH | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:102 | + | 1 | VICEEXT_SHOTGUN_AIM | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:132 | + | 1 | VICEEXT_SHOTGUN_AIM | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:190 | + | 1 | VICEEXT_CROUCH | linea en blanco entre bloques guardados |
| peds/PlayerPed.cpp:976 | + | 1 | VICEEXT_AIM_CLASSICAXIS | artefacto del diff: llave de cierre de DoWeaponSmoothSpray |
| peds/PlayerPed.cpp:977-983 | + | 7 | VICEEXT_SPRINT_HEAVY (envoltorio) | CanSprintWithCurrentWeapon(): con la bandera apagada devuelve lo vanilla |
| peds/PlayerPed.cpp:1055-1058 | + | 2 | VICEEXT_CROUCH (macro VICEEXT_ENGINE_DUCK_KEY) | PlayerControlSniper: tecla duck via macro (vanilla con bandera off) |
| peds/PlayerPed.cpp:1232-1235 | + | 2 | VICEEXT_CROUCH (macro VICEEXT_ENGINE_DUCK_KEY) | PlayerControlM16: tecla duck via macro |
| peds/PlayerPed.cpp:2194 | + | 1 | VICEEXT_AIM_CLASSICAXIS | blanco |
| peds/PlayerPed.cpp:2286 | + | 1 | VICEEXT_SPRINT_HEAVY (envoltorio) | 1stPersonRunAround: CanSprintWithCurrentWeapon() |
| peds/PlayerPed.cpp:2663 | + | 1 | VICEEXT_ROCKET_3RD_PERSON | blanco |
| peds/PlayerPed.cpp:2678-2686 | + | 2 | VICEEXT_CROUCH (macro VICEEXT_ENGINE_DUCK_KEY) | ProcessPlayerWeapon: tecla duck via macro |
| peds/PlayerPed.cpp:2701 | + | 1 | VICEEXT_SHOTGUN_AIM (macro ViceExtCanAim) | precision 95 si el arma apunta (escopetas) |
| peds/PlayerPed.cpp:2830 | + | 1 | VICEEXT_SHOTGUN_AIM (macro ViceExtAimWithArm) | CAN_AIM_WITH_ARM incluye escopetas/lanzagranadas |
| peds/PlayerPed.cpp:2872 | + | 1 | VICEEXT_SHOTGUN_AIM (macro ViceExtCanAim) |  |
| peds/PlayerPed.cpp:2918 | + | 1 | VICEEXT_SHOTGUN_AIM (macro ViceExtCanAim) |  |
| peds/PlayerPed.cpp:3207 | + | 1 | VICEEXT_AIM_WALK | blanco |
| peds/PlayerPed.cpp:3255 | + | 1 | VICEEXT_AIM_WALK | blanco |
| peds/PlayerPed.cpp:3275 | + | 1 | VICEEXT_SPRINT_HEAVY (envoltorio) | Zelda: CanSprintWithCurrentWeapon() |
| peds/PlayerPed.cpp:3279 | + | 1 | VICEEXT_SPRINT_HEAVY (envoltorio) | cierre de la condicion |
| peds/PlayerPed.cpp:3295 | + | 1 | VICEEXT_CLIMB | blanco |
| peds/PlayerPed.cpp:4048 | + | 1 | VICEEXT_CLIMB | blanco |
| peds/PlayerPed.cpp:4227 | + | 1 | VICEEXT_CROUCH | blanco |
| peds/PlayerPed.cpp:6282-6296 | + | 15 | VICEEXT_ALT_SIT | comentario del bloque WalkKey |
| peds/PlayerPed.cpp:6570 | + | 1 | VICEEXT_CROUCH | blanco |
| peds/PlayerPed.cpp:6791 | + | 1 | VICEEXT_CROUCH | PlayIdleAnimations: condicion partida para insertar odCrouched |
| peds/PlayerPed.cpp:6798 | + | 1 | VICEEXT_CROUCH |  |
| peds/PlayerPed.cpp:o682 | - | 1 | VICEEXT_CROUCH (macro) | DuckJustDown -> VICEEXT_ENGINE_DUCK_KEY |
| peds/PlayerPed.cpp:o685 | - | 1 | VICEEXT_CROUCH (macro) |  |
| peds/PlayerPed.cpp:o820 | - | 1 | VICEEXT_CROUCH (macro) |  |
| peds/PlayerPed.cpp:o823 | - | 1 | VICEEXT_CROUCH (macro) |  |
| peds/PlayerPed.cpp:o908 | - | 1 | VICEEXT_SPRINT_HEAVY (envoltorio) |  |
| peds/PlayerPed.cpp:o1242 | - | 1 | VICEEXT_CROUCH (macro) |  |
| peds/PlayerPed.cpp:o1250 | - | 1 | VICEEXT_CROUCH (macro) |  |
| peds/PlayerPed.cpp:o1262 | - | 1 | VICEEXT_SHOTGUN_AIM (macro) |  |
| peds/PlayerPed.cpp:o1354 | - | 1 | VICEEXT_SHOTGUN_AIM (macro) |  |
| peds/PlayerPed.cpp:o1382 | - | 1 | VICEEXT_SHOTGUN_AIM (macro) |  |
| peds/PlayerPed.cpp:o1419 | - | 1 | VICEEXT_SHOTGUN_AIM (macro) |  |
| peds/PlayerPed.cpp:o1554 | - | 1 | VICEEXT_SPRINT_HEAVY (envoltorio) |  |
| peds/PlayerPed.cpp:o1993 | - | 1 | VICEEXT_CROUCH | condicion partida |
| peds/PlayerPed.h:9-10 | + | 2 | VICEEXT_SHOTGUN_AIM | comentario del bloque ViceExtCanAim |
| peds/PlayerPed.h:108 | + | 1 | VICEEXT_SPRINT_HEAVY (envoltorio) | declaracion CanSprintWithCurrentWeapon |
| core/Cam.cpp:261-262 | + | 2 | VICEEXT_RECOIL | blancos tras el bloque de recoil |
| core/Cam.cpp:1931-1952 | + | 22 | VICEEXT_AIM_CLASSICAXIS | comentario del bloque duckOffset/clamp de agua de la ley a pie |
| core/Cam.cpp:1959-1973 | + | 15 | VICEEXT_AIM_CLASSICAXIS (dep. blanda NADO) | clamp ClassicAXIS: con bTouchingWater la camara no baja de nivel+0,6 y near clip 0,2 (afecta a la camara de nado) |
| core/Cam.cpp:2006-2010 | + | 5 | VICEEXT_AIM_CLASSICAXIS | comentario del bloque de esconder peds de la ley a pie |
| peds/Ped.cpp:95 | + | 1 | VICEEXT_CHEAT_EFFECTS | blanco tras bloque |
| peds/Ped.cpp:1748 | + | 1 | VICEEXT_CROUCH | blanco tras bloque |
| peds/Ped.cpp:5448 | + | 1 | VICEEXT_CHEAT_EFFECTS | blanco tras bloque |
| peds/Ped.cpp:9831 | + | 1 | VICEEXT_AIM_CLASSICAXIS | blanco tras bloque |
| peds/Ped.cpp:9858 | + | 1 | VICEEXT_AIM_CLASSICAXIS | blanco antes de bloque |
| peds/PedFight.cpp:191-199 | + | 9 | VICEEXT_AIM_CLASSICAXIS | comentario del bloque C18-1 |
| peds/CopPed.cpp:475 | + | 1 | VICEEXT_HIDE_COPS | blanco antes de bloque |
| core/Camera.cpp:229-230 | + | 2 | VICEEXT_FIRST_PERSON | particion del bloque IMPROVED_CAMERA para meter el macro de 1a persona (sin efecto) |
| core/Camera.cpp:265-266 | + | 2 | VICEEXT_FIRST_PERSON | particion del bloque IMPROVED_CAMERA (sin efecto) |
| core/Camera.cpp:1507 | + | 1 | VICEEXT_FIRST_PERSON | blanco tras bloque |
| core/config.h:51 | + | 1 | VICEEXT_LIMITS | #define VICEEXT_LIMITS (grupo limites-mod) |
| core/config.h:52-53 | + | 2 | VICEEXT_LIMITS | #define VICEEXT_SPAWN_RADIUS_MULT 1.5f (solo se usa bajo VICEEXT_LIMITS) |
| core/config.h:386-389 | + | 4 | config-defines | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:391 | + | 1 | VICEEXT_RECOIL | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:392 | + | 1 | VICEEXT_NO_CAR_BOUNCE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:393 | + | 1 | VICEEXT_SPRINT_HEAVY | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:394 | + | 1 | VICEEXT_POLICE_BIKE_LIGHTS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:395 | + | 1 | VICEEXT_HIDE_COPS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:396 | + | 1 | VICEEXT_FIRST_PERSON | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:397 | + | 1 | VICEEXT_DRIVEBY_WIDE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:398 | + | 1 | VICEEXT_WEAPON_SIGHTS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:399 | + | 1 | VICEEXT_HINT_KEYS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:400 | + | 1 | VICEEXT_AUTOSAVE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:401 | + | 1 | VICEEXT_SAVE_ANYWHERE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:402 | + | 1 | VICEEXT_MANUAL_RELOAD | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:403 | + | 1 | VICEEXT_GAS_TANK | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:404 | + | 1 | VICEEXT_SHOTGUN_AIM | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:405 | + | 1 | VICEEXT_CROUCH | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:406-424 | + | 19 | VICEEXT_CROUCH_CAM_DROP | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:425-426 | + | 2 | VICEEXT_CLIMB | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:427-428 | + | 2 | VICEEXT_AIM_WALK | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:429-433 | + | 5 | VICEEXT_ROCKET_3RD_PERSON | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:434-447 | + | 14 | VICEEXT_AIM_CLASSICAXIS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:448 | + | 1 | VICEEXT_BREAKABLE_LIGHTS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:449 | + | 1 | VICEEXT_TURN_SIGNALS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:450 | + | 1 | VICEEXT_BIKE_EXIT_SIDE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:451 | + | 1 | VICEEXT_AIM_BLADE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:452-453 | + | 2 | VICEEXT_FIX_SILENTPATCH | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:454 | + | 1 | VICEEXT_FIX_WFP | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:455 | + | 1 | VICEEXT_FIX_FV | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:456 | + | 1 | VICEEXT_FIX_FV_ROTOR | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:457 | + | 1 | VICEEXT_POLICE_BIKE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:458 | + | 1 | VICEEXT_NO_WHEEL_PIERCE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:459 | + | 1 | VICEEXT_SKIP_PHONE_CALL | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:460 | + | 1 | VICEEXT_MOD_SCRIPT | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:461 | + | 1 | VICEEXT_PARTICLE_NAMES | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:462-466 | + | 5 | VICEEXT_LOD_EXTRA | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:467-470 | + | 4 | VICEEXT_MONEY_NO_ZEROS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:471-472 | + | 2 | VICEEXT_PEDARBITER | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:474 | + | 1 | VICEEXT_EXTRA_CHEATS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:475 | + | 1 | VICEEXT_CAR_DEATH | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:476 | + | 1 | VICEEXT_CAR_LIGHTS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:477-481 | + | 5 | VICEEXT_ALT_SIT | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:482 | + | 1 | VICEEXT_GANG_AIM | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:483-485 | + | 3 | VICEEXT_MOON | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:486 | + | 1 | VICEEXT_MAPDATA | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:487 | + | 1 | VICEEXT_POLISH | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:488 | + | 1 | VICEEXT_WHEEL_DETACH | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:489-492 | + | 4 | VICEEXT_FEATURES_INI | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:493 | + | 1 | VICEEXT_NO_FLIP_BURN | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:494 | + | 1 | VICEEXT_HEALTH_REGEN | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:495 | + | 1 | VICEEXT_HIDE_STARS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:496 | + | 1 | VICEEXT_NO_JUMP_FROM_CAR | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:497 | + | 1 | VICEEXT_STD_TURN_SIGNALS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:498 | + | 1 | VICEEXT_TANK_FIRE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:499 | + | 1 | VICEEXT_GPS_COLOR | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:500 | + | 1 | VICEEXT_GPS_COLOR_R | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:501 | + | 1 | VICEEXT_GPS_COLOR_G | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:502 | + | 1 | VICEEXT_GPS_COLOR_B | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:503-505 | + | 3 | VICEEXT_CAR_ROTATE_WHEEL | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:506 | + | 1 | VICEEXT_CAR_WHEEL_KEEP | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:507 | + | 1 | VICEEXT_CAR_FOUR_COLORS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:508 | + | 1 | VICEEXT_GAS_STATION_BLOW | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:509 | + | 1 | VICEEXT_PROJECTILE_BLOW | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:510-512 | + | 3 | VICEEXT_DISTANT_LIGHTS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:513 | + | 1 | VICEEXT_COP_COVER | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:514 | + | 1 | VICEEXT_HELI_HEALTH | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:515 | + | 1 | VICEEXT_AIM_SENSITIVITY | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:516 | + | 1 | VICEEXT_CHEAT_EFFECTS | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:517 | + | 1 | VICEEXT_PHOTO_SAVE | #define de bandera en config.h (bloque features.ini del mod) |
| core/config.h:518 | + | 1 | config-defines | #define de bandera en config.h (bloque features.ini del mod) |
| save/GenericGameStorage.cpp:60 | + | 1 | VICEEXT_SAVE_ANYWHERE | blanco entre bloques AUTOSAVE/SAVE_ANYWHERE |
| save/GenericGameStorage.cpp:86 | + | 1 | VICEEXT_SAVE_ANYWHERE | blanco tras ViceExtCanSaveAnywhere |
| audio/AudioLogic.cpp:2896-2901 | + | 6 | VICEEXT_FIX_SILENTPATCH | comentario de atribucion SilentPatch (sirena FBI) |
| weapons/Weapon.h:105 | + | 1 | VICEEXT_RECOIL | blanco antes del bloque recoil |
| weapons/Weapon.cpp:127-128 | + | 2 | VICEEXT_RECOIL | llave/blanco estructural del bloque recoil |
| weapons/Weapon.cpp:874 | + | 1 | VICEEXT_RECOIL | blanco |
| weapons/Weapon.cpp:1525 | + | 1 | VICEEXT_GAS_TANK | blanco |
| weapons/Weapon.cpp:1582 | + | 1 | VICEEXT_BREAKABLE_LIGHTS | blanco |
| weapons/Weapon.cpp:1603 | + | 1 | VICEEXT_GAS_TANK | blanco |
| weapons/Weapon.cpp:1963-1972 | + | 10 | VICEEXT_FIX_SILENTPATCH | casquillos Python/Sniper/Laser + reindent de AddGunshell |
| weapons/Weapon.cpp:o1211 | - | 1 | VICEEXT_FIX_SILENTPATCH | AddGunshell vanilla reemitido con otra sangría |
| weapons/Weapon.cpp:2730 | + | 1 | VICEEXT_GAS_TANK | línea vanilla reemitida tras el gancho |
| weapons/Weapon.cpp:o1953 | - | 1 | VICEEXT_GAS_TANK | línea vanilla reemitida |
| weapons/Weapon.cpp:o2092-2100 | - | 9 | VICEEXT_ROCKET_3RD_PERSON | chequeo de cámara 1ª persona movido a #ifndef |
| weapons/Weapon.cpp:3450 | + | 1 | VICEEXT_GAS_TANK | llave del gancho en FireInstantHitFromCar |
| weapons/Weapon.cpp:3456 | + | 1 | VICEEXT_GAS_TANK | llave del gancho |
| weapons/WeaponInfo.cpp:253 | + | 1 | VICEEXT_WEAPON_SIGHTS | sscanf partido para columna 27 (neutro) |
| weapons/WeaponInfo.cpp:257 | + | 1 | VICEEXT_WEAPON_SIGHTS | cierre sscanf |
| weapons/WeaponInfo.cpp:o215 | - | 1 | VICEEXT_WEAPON_SIGHTS | cierre sscanf vanilla |
| vehicles/Automobile.cpp:90-91 | + | 2 | VICEEXT_CAR_WHEEL_KEEP | blancos |
| vehicles/Automobile.cpp:565 | + | 1 | VICEEXT_CAR_WHEEL_KEEP | gancho sin #ifdef, neutro por stub #else (devuelve 0) |
| vehicles/Automobile.cpp:o517 | - | 1 | VICEEXT_CAR_WHEEL_KEEP | m_fSteerAngle = 0 vanilla |
| vehicles/Automobile.cpp:581 | + | 1 | VICEEXT_CAR_WHEEL_KEEP | gancho neutro |
| vehicles/Automobile.cpp:o533 | - | 1 | VICEEXT_CAR_WHEEL_KEEP | vanilla |
| vehicles/Automobile.cpp:601 | + | 1 | VICEEXT_CAR_WHEEL_KEEP | gancho neutro |
| vehicles/Automobile.cpp:o553 | - | 1 | VICEEXT_CAR_WHEEL_KEEP | vanilla |
| vehicles/Automobile.cpp:1467-1469 | + | 3 | VICEEXT_FIX_FV | comentario port FramerateVigilante |
| vehicles/Automobile.cpp:o1419 | - | 1 | VICEEXT_FIX_FV | comentario vanilla; el spool del rotor RC queda SIN #else |
| vehicles/Automobile.cpp:o1422 | - | 1 | VICEEXT_FIX_FV | spool vanilla eliminado (sin #else) |
| vehicles/Automobile.cpp:o1424 | - | 1 | VICEEXT_FIX_FV | spool vanilla eliminado (sin #else) |
| vehicles/Automobile.cpp:1539 | + | 1 | VICEEXT_CHEAT_EFFECTS | else-if estructural (rama #else false) |
| vehicles/Automobile.cpp:1545 | + | 1 | VICEEXT_CHEAT_EFFECTS | estructural |
| vehicles/Automobile.cpp:1656 | + | 1 | VICEEXT_FIX_FV | odPhase en vez de FrameCounter (neutro con #else) |
| vehicles/Automobile.cpp:o1586 | - | 1 | VICEEXT_FIX_FV | vanilla |
| vehicles/Automobile.cpp:1789 | + | 1 | VICEEXT_TURN_SIGNALS | blanco |
| vehicles/Automobile.cpp:1925 | + | 1 | VICEEXT_TURN_SIGNALS | blanco |
| vehicles/Automobile.cpp:2050 | + | 1 | VICEEXT_CAR_ROTATE_WHEEL | blanco |
| vehicles/Automobile.cpp:2568 | + | 1 | VICEEXT_FIX_FV | odPhase (neutro con #else) |
| vehicles/Automobile.cpp:o2042 | - | 1 | VICEEXT_FIX_FV | vanilla |
| vehicles/Automobile.cpp:4805 | + | 1 | VICEEXT_GAS_STATION_BLOW | blanco |
| vehicles/Bike.cpp:1310 | + | 1 | VICEEXT_POLICE_BIKE_LIGHTS | blanco |
| vehicles/Bike.cpp:1837 | + | 1 | VICEEXT_FIX_FV | odPhase (neutro con #else) |
| vehicles/Bike.cpp:o1713 | - | 1 | VICEEXT_FIX_FV | vanilla |
| vehicles/Heli.cpp:325 | + | 1 | VICEEXT_FIX_FV | odPhase (neutro con #else) |
| vehicles/Heli.cpp:o313 | - | 1 | VICEEXT_FIX_FV | vanilla |
| vehicles/Heli.cpp:585-589 | + | 5 | VICEEXT_FIX_FV_ROTOR | comentario port FramerateVigilante |
| vehicles/Heli.cpp:o573 | - | 1 | VICEEXT_FIX_FV_ROTOR | rotor vanilla borrado (bloque sin #else) |
| vehicles/Heli.cpp:1018 | + | 1 | VICEEXT_HELI_HEALTH | usa odHeliDamageLimit (neutro con #else 700) |
| vehicles/Heli.cpp:o994 | - | 1 | VICEEXT_HELI_HEALTH | vanilla 700 |
| control/CarCtrl.cpp:118 | + | 1 | VICEEXT_FIX_FV | blanco |
| control/CarCtrl.cpp:213 | + | 1 | VICEEXT_FIX_FV | odSpawnPhase (neutro con #else) |
| control/CarCtrl.cpp:o196 | - | 1 | VICEEXT_FIX_FV | vanilla |
| control/CarCtrl.cpp:235 | + | 1 | VICEEXT_FIX_FV | odSpawnPhase |
| control/CarCtrl.cpp:o218 | - | 1 | VICEEXT_FIX_FV | vanilla |
| control/CarCtrl.cpp:264 | + | 1 | VICEEXT_FIX_FV | odSpawnPhase |
| control/CarCtrl.cpp:o247 | - | 1 | VICEEXT_FIX_FV | vanilla |
| control/CarCtrl.cpp:293 | + | 1 | VICEEXT_FIX_FV | odSpawnPhase |
| control/CarCtrl.cpp:o276 | - | 1 | VICEEXT_FIX_FV | vanilla |
| control/CarCtrl.cpp:612 | + | 1 | VICEEXT_LIMITS | odDespawnK (neutro sin la bandera) |
| control/CarCtrl.cpp:617 | + | 1 | VICEEXT_LIMITS | usa odDespawnK |
| control/CarCtrl.cpp:o593 | - | 1 | VICEEXT_LIMITS | vanilla |
| control/CarCtrl.cpp:623 | + | 1 | VICEEXT_LIMITS | usa odDespawnK |
| control/CarCtrl.cpp:o599 | - | 1 | VICEEXT_LIMITS | vanilla |
| control/CarCtrl.cpp:1062 | + | 1 | VICEEXT_FIX_FV | odPhase (neutro) |
| control/CarCtrl.cpp:o922 | - | 1 | VICEEXT_FIX_FV | vanilla |
| control/CarCtrl.cpp:1133 | + | 1 | VICEEXT_LIMITS | línea vanilla reemitida |
| control/CarCtrl.cpp:o990 | - | 1 | VICEEXT_LIMITS | vanilla |
| control/CarCtrl.cpp:2787-2793 | + | 7 | VICEEXT_FIX_FV | comentario port FramerateVigilante |
| control/CarCtrl.cpp:o2647 | - | 1 | VICEEXT_FIX_FV | spool heli IA vanilla borrado (sin #else) |
| control/CarAI.cpp:32 | + | 1 | VICEEXT_POLISH | blanco |
| renderer/Hud.cpp:372 | + | 1 | VICEEXT_WEAPON_SIGHTS | blanco tras el bloque |
| renderer/Hud.cpp:786 | + | 1 | VICEEXT_AIM_CLASSICAXIS | blanco tras el bloque |
| renderer/Hud.cpp:2471 | + | 1 | VICEEXT_WEAPON_SIGHTS | blanco |
| renderer/Font.cpp:317 | + | 1 | VICEEXT_HINT_KEYS | blanco |
| renderer/Font.cpp:583 | + | 1 | VICEEXT_HINT_KEYS | blanco |
| renderer/Font.cpp:1703 | + | 1 | VICEEXT_HINT_KEYS | case K partido para insertar el bloque (equivalente sin bandera) |
| renderer/Font.cpp:1710 | + | 1 | VICEEXT_HINT_KEYS | resto del case K vanilla |
| renderer/Font.cpp:o1522 | - | 1 | VICEEXT_HINT_KEYS | case K vanilla (re-partido) |
| renderer/Font.cpp:1763 | + | 1 | VICEEXT_HINT_KEYS | idem |
| renderer/Font.cpp:1768 | + | 1 | VICEEXT_HINT_KEYS | idem |
| renderer/Font.cpp:o1575 | - | 1 | VICEEXT_HINT_KEYS | idem |
| renderer/Font.cpp:1894 | + | 1 | VICEEXT_HINT_KEYS | idem |
| renderer/Font.cpp:1899 | + | 1 | VICEEXT_HINT_KEYS | idem |
| renderer/Font.cpp:o1701 | - | 1 | VICEEXT_HINT_KEYS | idem |
| renderer/Font.h:162 | + | 1 | VICEEXT_HINT_KEYS | blanco |
| core/MenuScreensCustom.cpp:57 | + | 1 | VICEEXT_FIRST_PERSON | blanco tras el macro |
| core/MenuScreensCustom.cpp:425 | + | 1 | VICEEXT_FIRST_PERSON | uso de FIRST_PERSON_TOGGLE (vacio sin bandera) |
| text/Messages.cpp:516-517 | + | 2 | VICEEXT_HINT_KEYS | reindentacion de codigo vanilla para insertar el bloque (sin efecto) |
| text/Messages.cpp:o468-476 | - | 8 | VICEEXT_HINT_KEYS | reindentacion (sin efecto) |
| text/Messages.cpp:603-610 | + | 8 | VICEEXT_HINT_KEYS | reindentacion de codigo vanilla (sin efecto) |
| core/Pad.cpp:849 | + | 1 | VICEEXT_EXTRA_CHEATS | blanco |
| core/Pad.cpp:2623 | + | 1 | VICEEXT_ALT_SIT | blanco |
| core/ControllerConfig.cpp:702-706 | + | 5 | VICEEXT_FIRST_PERSON | comentario (sin codigo) |
| core/ControllerConfig.cpp:2851 | + | 1 | VICEEXT_HINT_KEYS | blanco |
| core/ViceExtFeatures.cpp:1-4 | + | 4 | VICEEXT_FEATURES_INI | includes (el cuerpo va bajo #ifdef VICEEXT_FEATURES_INI) |
| core/ViceExtFeatures.cpp:6 | + | 1 | VICEEXT_FEATURES_INI | blanco |
| core/Wanted.cpp:799 | + | 1 | VICEEXT_HIDE_COPS | blanco tras el bloque |
| core/Wanted.h:41 | + | 1 | VICEEXT_HIDE_COPS | blanco antes de los campos nuevos |
| control/Script.cpp:o20 | - | 1 | VICEEXT_AUTOSAVE | ifdef MISSION_REPLAY ampliado a // VICEEXT_AUTOSAVE |
| control/Script.h:621-622 | + | 2 | VICEEXT_AUTOSAVE | desalineacion del diff (endif/blanco) |
| control/Script8.cpp:623 | + | 1 | VICEEXT_MOD_SCRIPT | blanco |
| renderer/ParticleMgr.cpp:135 | + | 1 | VICEEXT_PARTICLE_NAMES | blanco |
| renderer/Particle.cpp:889 | + | 1 | VICEEXT_POLISH | blanco |
| renderer/Particle.cpp:930 | + | 1 | VICEEXT_POLISH | blanco |
| renderer/Particle.cpp:o906 | - | 1 | VICEEXT_POLISH | linea en blanco (tab) sustituida |
| buildings/Building.cpp:32 | + | 1 | VICEEXT_DISTANT_LIGHTS | blanco |
| entities/Dummy.cpp:62 | + | 1 | VICEEXT_DISTANT_LIGHTS | blanco |

## Anexo E. Notas detalladas por grupo de ficheros

### E.1 NADO en PlayerPed.cpp/.h (inventario cerrado)

Bandera `VICEEXT_SWIMMING` (config.h:390, sin valor). En estos dos ficheros el nado son **395 líneas funcionales** (391 en .cpp y 4 en .h) más **173 líneas de traza** dentro de sus bloques (DIAGNOSTICO en:NADO). Las trazas se pueden quitar sin afectar al nado, salvo lo que consuma el arnés (ver el apartado 5).

#### 1. Bloques (números de línea de src/)

| rango | función / sitio | guarda real | qué hace |
|---|---|---|---|
| PlayerPed.h:123-126 | declaración en `CPlayerPed` | VICEEXT_SWIMMING | `bool ViceExtSwimControl(CPad*)`; `static bool ViceExtIsSwimming(void)` |
| .cpp:5 | include | **sin guarda** | `PedArbiter.h` (carriles `PEDLANE_NADO`) |
| .cpp:9, 10, 34, 36, 38 | includes | **sin guarda** | soundlist.h (`SOUND_NADO_BRAZO_*`), Timecycle.h, WaterLevel.h, ParticleObject.h, Particle.h |
| .cpp:133-136 | estático | VICEEXT_SWIMMING | `static int32 s_odSwimSlot`: ranura de arma guardada mientras se nada |
| .cpp:158-163 | dentro de `CPlayerPed::ViceExtIsAiming` | VICEEXT_CROUCH + SWIMMING | nadando no se apunta (`s_odAimWhy="nado"`) |
| .cpp:319-325 | constructor `CPlayerPed` | VICEEXT_SWIMMING | `bDrownsInWater = false` (el jugador no se ahoga) |
| .cpp:1100-1105 | `ProcessWeaponSwitch` | VICEEXT_SWIMMING | si NADO posee `PEDCAP_ARMA`: fija `m_nSelectedWepSlot = s_odSwimSlot` y salta a `switchDetectDone` |
| .cpp:1326-1329 | dentro de `ViceExtIsAimingNow` | AIM_CLASSICAXIS + SWIMMING | ClassicAXIS no apunta nadando |
| .cpp:1519-1522 | dentro de `CPlayerPed::ViceExtPrepareMove` | AIM_CLASSICAXIS + SWIMMING | nadando, `s_odMove.prepared = false` (la ley de movimiento ClassicAXIS no mueve) |
| .cpp:2667-2670 | `ProcessPlayerWeapon` (al principio) | VICEEXT_SWIMMING | si NADO posee `ARMA`, `ClearPointGunAt()` |
| .cpp:3556-4047 | bloque principal | VICEEXT_SWIMMING | constantes, `ViceExtSwimClipSpeed`, `odSwimActive`, `ViceExtSwimWake` (estela), estáticos, `ViceExtSwimWeaponFists` (puños: quita y devuelve el arma), **`CPlayerPed::ViceExtSwimControl`** y **`CPlayerPed::ViceExtIsSwimming`** |
| .cpp:3593-3595, 3755-3761 | dentro del bloque | CLIMB + SWIMMING | al entrar al agua aborta la trepada (`ViceExtClimbAbort`) |
| .cpp:4130-4137 | dentro de `ViceExtClimbControl` | CLIMB + SWIMMING | nadando no se escala |
| .cpp:6556-6564 | `ProcessControl` | VICEEXT_SWIMMING | `odSwimming = ViceExtSwimControl(padUsed)`; si estaba en `PED_FALL/JUMP`, pasa a `PED_IDLE` |
| .cpp:6581-6586 | `ProcessControl` (switch) | VICEEXT_SWIMMING | nadando no se reparte el control a pie (ni Sniper, ni 1stPerson, ni Zelda) |
| .cpp:6611-6617, 6626 (+ vanilla o1826-1828) | `ProcessControl` | VICEEXT_SWIMMING (reindentado sin guarda, neutro) | nadando no se llama a `ProcessPlayerWeapon` |

##### Qué hace `ViceExtSwimControl` (3670-4040)

1. **Puerta (`odGuard`).** Exige pad, ni vehículo, ni muerte, ni replay, y agua por `CWaterLevel::GetWaterLevel`. Para entrar hace falta `bIsInWater` y profundidad > `CHEST_M - FEET_OFFSET`; para mantenerse basta profundidad > `HIP_M - FEET_OFFSET`.
2. **Salida.** Devuelve el arma, libera los cuatro `PEDCAP` del carril NADO, pone la velocidad a 0, `odSwimActive = false` y desvanece los clips de nado.
3. **Entrada.** Aborta la trepada y reclama `PEDCAP_MOVIMIENTO/POSTURA/APUNTAR/ARMA` en `PEDLANE_NADO`. Desvanece los 7 clips de salto/caída. Salpicadura de entrada: `SOUND_SPLASH`, `POBJECT_PED_WATER_SPLASH` y 4× `PARTICLE_RAIN_SPLASH_BIGGROW`, con el color de `CTimeCycle`.
4. **Cada frame.**
   - `bIsInTheAir = false`, `bIsStanding = false`, quita el `bIsDucking` y aplica los puños.
   - Rumbo = mando relativo a la cámara (`m_fRotationCur = m_fRotationDest`).
   - Velocidad XY fija según el estado: 1 = flotar (ENTRY y, tras TIMEOUT ticks, CRUISE); 2 = braza (SWIM); 3 = crol con esprint (SPRINT).
   - Flotación en Z hacia `nivel - (CHEST_M - FEET_OFFSET)`.
5. **Clip.** `CAnimManager::BlendAnimation(ASSOCGRP_PLAYERSWIM, swim_tread|swim_breast|swim_crawl, 4.0)`. El ritmo se calcula con `ViceExtSwimClipSpeed`: metros de raíz del clip / `totalLength`, × `STROKE_SLACK`.
6. **Brazada.** En cada vuelta del clip suena `SOUND_NADO_BRAZO_{A,B,LS_A,LS_B}` alternando. Además: salpicadura de pie (`particleProduceFootSplash` con color), `POBJECT_PED_WATER_SPLASH` si la fuerza es > 0,7, y estela (`ViceExtSwimWake`: `PARTICLE_CAR_SPLASH` + `PARTICLE_BOAT_SPLASH`).

##### Constantes (PlayerPed.cpp:3558-3567)

| constante | valor |
|---|---|
| `VICEEXT_SWIM_SEREGA_ENTRY` | 0.022f (velocidad de flotar al entrar) |
| `VICEEXT_SWIM_SEREGA_CRUISE` | 0.025f (flotar tras el TIMEOUT) |
| `VICEEXT_SWIM_SEREGA_SWIM` | 0.05f (braza) |
| `VICEEXT_SWIM_SEREGA_SPRINT` | 0.12f (crol con esprint) |
| `VICEEXT_SWIM_SEREGA_TIMEOUT` | 300 (ticks quieto) |
| `VICEEXT_SWIM_STROKE_SLACK` | 0.65f |
| `VICEEXT_SWIM_CRAWL_ROOT_M` | 2.505f (m de raíz de swim_crawl) |
| `VICEEXT_SWIM_BREAST_ROOT_M` | 3.011f (m de raíz de swim_breast) |
| `VICEEXT_SWIM_CHEST_M` | 1.45f |
| `VICEEXT_SWIM_HIP_M` | 1.10f |

##### Estado añadido

- **Estáticos de fichero:** `s_odSwimSlot`, `odSwimActive`, `s_odSwimVisualHidden`, `s_odSwimLastPos`, `s_odSwimLastPosOk`, `s_odSwimMedido`, `s_odSwimPrevTime`, `s_odNadoBrazo`.
- **Solo traza:** `s_odPrevFase`, `s_odFovNext/Prev/Paso`.
- **Sin uso:** `s_odEstela`.
- **Estáticos locales:** `s_odSwimState`, `s_odSwimTicks`, `odFallAnims[7]`, `odSwimAnims[4]`, y `s_odUltima` en `ViceExtSwimWake`.
- **Miembros nuevos de `CPlayerPed`:** ninguno. Se usa el campo vanilla `bDrownsInWater`.

##### Clips usados

`ANIM_STD_SWIM_TREAD`, `ANIM_STD_SWIM_CRAWL`, `ANIM_STD_SWIM_BREAST` y `ANIM_STD_SWIM_JUMPOUT` (los nombres `swim_tread/crawl/breast/jumpout`).

- Están definidos **sin guarda** en `animation/AnimationId.h:292-295`, `AnimManager.h:81` (`ASSOCGRP_PLAYERSWIM`) y `AnimManager.cpp:941-944, 963-969, 1117`. La última es el grupo `"playerswim"` del bloque `"ped"`, con `MI_COP` como modelo.
- Salen del **ped.ifp del mod** (`ped` en gta3.img del port).
- Necesitan los límites subidos, también **sin guarda**, en config.h: `NUMANIMATIONS = 512` (el comentario cuenta "38 nuevas del ped.ifp del mod") y `NUMANIMBLOCKS = 40`.

#### 2. Dependencias del NADO hacia código del mod viejo o de los agentes

| dependencia | dónde | tipo | qué se rompe si se quita |
|---|---|---|---|
| **PedArbiter** (`core/PedArbiter.cpp/.h`, bandera `VICEEXT_PEDARBITER`): `ViceExtPedClaim`/`Release`/`Owns`, `PEDLANE_NADO`, `PEDCAP_*` | PlayerPed.cpp:159, 1101, 1327, 2668, 3704-3707, 3773-3776 (y 6047 en la ley ClassicAXIS sin guarda). Fuera: Cam.cpp:653-654, 4292-4293; Ped.cpp:1985, 2903 (flotación); Hud.cpp:427, 455 (retícula); Script6.cpp:1008; Game.cpp:989 (`ViceExtPedResetAll`) | compilación y función | Sin los ficheros no compila. Sin `VICEEXT_PEDARBITER` compila con stubs: `Owns` siempre es falso y `Claim` falla. El nado sigue moviendo, pero desaparecen el bloqueo de arma (1101), el bloqueo de apuntado (2668), la flotación de Ped.cpp, la retícula oculta y la cámara de nado. |
| **ViceExtFeatures** (`core/ViceExtFeatures.cpp/.h`) | PedArbiter.cpp:41: `if (l == PEDLANE_NADO && !ViceExtFeatures::On("EnableSwimming", true)) return false;`; la entrada está en ViceExtFeatures.cpp:35 | indirecta, por PedArbiter | Sin ella no compila PedArbiter. Ojo: `ViceExtSwimControl` **ignora** el resultado de `Claim`. Con `EnableSwimming=0` el nado sigue moviendo al jugador; solo se apagan las puertas que van por carril. |
| `VICEEXT_CLIMB`: `ViceExtClimbAbort`, `ViceExtClimbControl` | 3593-3595, 3755-3761, 4130-4137 | coordinación, guardada por `#ifdef VICEEXT_CLIMB` | Nada: compila sin CLIMB. |
| `VICEEXT_CROUCH`: `ViceExtIsAiming` contiene la regla del nado | 158-163 | coordinación | Al quitar CROUCH desaparece "no apuntar nadando" de esa API. Cam, Hud y la ley ClassicAXIS lo comprueban por su cuenta con `PEDLANE_NADO`. |
| `VICEEXT_AIM_CLASSICAXIS`: `ViceExtIsAimingNow`, `ViceExtPrepareMove` | 1326-1329, 1519-1522; 6047 (sin guarda) | coordinación | Al reimplementar el apuntado o el movimiento hay que mantener que nadando ni se apunta ni se mueve por esa ley. |
| **Traza SWIM2** (solo web) usa `ViceExtBlendWeight` (bloque AIM_WALK, 2989-3119), `odCrouched` (CROUCH\|\|AIM_WALK, l.59) y `ViceExtIsAiming` (CROUCH) | 4019, 4024, 4026 | compilación (solo `__EMSCRIPTEN__`) | Quitar AIM_WALK o CROUCH sin quitar ni adaptar SWIM2 rompe la build web. |
| `CCamera::s_viceExtAimLawActive` (Camera.h, sin guarda, ClassicAXIS) | solo en las trazas SWIMFOV/SWIM2 (3990, 4019) | compilación de la traza | Igual que la fila anterior. |
| Cámara: `CPlayerPed::ViceExtIsSwimming()` leído desde Cam.cpp:1790, 6238 ("H2: la cámara va a la superficie") | fuera de PlayerPed | uso inverso | Los clasifica el grupo de Cam.cpp. |
| Audio: `SOUND_NADO_BRAZO_A/B/LS_A/LS_B` (soundlist.h:206-209, sin guarda), traducidos en AudioLogic.cpp:4533-4548 a `SFX_FOOTSTEP_WATER_1..4` (vanilla) | 3906-3907, 3926 | compilación | Sin soundlist.h/AudioLogic.cpp no compila o no suena. Encaja con el tooling `add_viceex_sfx.py`. |
| `particleProduceFootSplash(..., CRGBA color)`: firma cambiada en Ped.cpp:3438 y exportada en Ped.h:1221 (vanilla: 4 parámetros, estática) | 3921 | compilación | Si se restaura Ped.cpp/Ped.h a vanilla, el nado no compila. |
| Anims y límites (AnimationId.h, AnimManager.*, config.h `NUMANIMATIONS/NUMANIMBLOCKS`, ped.ifp del mod) | 3675-3683, 3859, 3571-3572 | compilación y datos | Sin el grupo `playerswim` o sin el ped.ifp del mod no hay clip (`BlendAnimation` con un id inexistente). |

**Vanilla puro que usa el nado (no depende de nada viejo):** `CWaterLevel`, `CTimeCycle`, `CParticle`/`CParticleObject` (`POBJECT_PED_WATER_SPLASH`, `PARTICLE_CAR/BOAT_SPLASH`, `RAIN_SPLASH_BIGGROW`), `SOUND_SPLASH`, `FEET_OFFSET`, `bDrownsInWater`, `bIsInWater`, `CTimer::GetTimeStepInSeconds`.

#### 3. Nado fuera de las etiquetas S en estos ficheros

- Includes sin guarda: 5, 9, 10, 34, 36, 38.
- Reindentado neutro de 6611/6617/6626.
- 6047 (ClassicAXIS sin guarda) y 5128/5328 (CROUCH) **consultan** el carril NADO. Son código viejo que se retira, no nado.
- 5700-5797 (PEDAT, CROUCH+EM): la "sonda de agua" del arnés, que navega hasta el mar para los tests del nado. Es DIAGNOSTICO, pero **el arnés del nado depende de ella**. Si se quita CROUCH, desaparece la sonda que usan los tests de nado.

#### 4. Sorpresas del nado (no tocar, solo saberlo)

- `s_odSwimMedido` nunca se actualiza tras el reset (3868 lo lee, nadie lo escribe). El "medido" suavizado es solo la fracción instantánea: el filtro está roto. Afecta a la fuerza de la salpicadura y de la estela, no al movimiento.
- `s_odEstela` está declarado y reseteado, pero no se usa.
- El resultado de `ViceExtPedClaim` se ignora (ver ViceExtFeatures en el apartado 2).

#### 5. Trazas del nado (DIAGNOSTICO en:NADO, 173 líneas)

Las etiquetas que emiten, por si las consume el tooling (`viceext-log-check.py`, harness): `SWIMNAT`, `VICEEXT swim punos`, `VICEEXT swim arma-devuelta`, `VICEEXT swim exit`, `SWIM2 exit`, `SWIMIDLE`, `VICEEXT swim trepa-off`, `VICEEXT swim enter`, `SWIMFASE`, `SWIMEF`, `VICEEXT swim move`, `SWIMFOV`, `SWIM2 move`, `SWIMERR`.

Rangos: 3576-3589, 3644-3646, 3656-3658, 3664-3666, 3721-3730, 3732-3744, 3757-3759, 3769-3771, 3808-3814, 3896-3901, 3927-3939, 3944-4038.

### E.2 RADIO (inventario cerrado, todo el repo)

Lo que hay que proteger son las emisoras: el icono/logo de la emisora al cambiar, `radio.txd` y la parte de MusicManager que los dibuja.
Hice grep de `VICEEXT_RADIO_ICONS`, `radio.txd`/`RADIO.TXD`, `"radio"`, `odRadio`, `kind=radio`, `RADIOLOGOS`, `RETUNE` y `RADIOTRACK` en `src/` y en `gta_vc_browser/` (sin contar node_modules ni tmp).
Fuera de `src/audio/MusicManager.cpp` y `src/core/config.h` no hay **ningún** código C++ de iconos de radio: ni en Hud.cpp, ni en Font.cpp, ni en Sprite2d, Frontend, Game, Streaming, FileLoader o TxdStore.

#### 1. Piezas en src/

| # | fichero:líneas | guarda | qué hace |
|---|---|---|---|
| R1 | src/core/config.h:473 | ninguna (en el bloque `VANILLA_DEFINES`, junto al resto de defines del mod) | `#define VICEEXT_RADIO_ICONS` |
| R2 | src/audio/MusicManager.cpp:10-12 | VICEEXT_RADIO_ICONS | `#include "TxdStore.h"` |
| R3 | src/audio/MusicManager.cpp:1379-1409, 1423-1424 (dentro de `cMusicManager::DisplayRadioStationName`) | VICEEXT_RADIO_ICONS | Tablas `odRadioNames[11]` = wildstyle, flash, kchat, fever, vrock, vcpr, espantoso, emotion, wave103, mp3, none, y `odRadioMasks[11]` (mismo nombre con sufijo `A`); `static CSprite2d odRadioSprites[11]`, `odRadioMaskOk[11]`, `odRadioInit`, `int odStation`. Hace una única carga perezosa: `CTxdStore::FindTxdSlot("radio")` y, si no existe, `AddTxdSlot("radio")` + `LoadTxd(slot,"MODELS/RADIO.TXD")` + `AddRef` + `PopCurrentTxd`. Después `SetTexture(name, mask)` para las 11 y comprueba la máscara con `RwTexDictionaryFindNamedTexture` |
| R4 | src/audio/MusicManager.cpp:1410-1422 | VICEEXT_RADIO_ICONS + __EMSCRIPTEN__ | DIAGNOSTICO: traza `RADIOLOGOS cargadas=N/11 mascaras=N/11 txd=radio` |
| R5 | src/audio/MusicManager.cpp:1425 (o1312 movida) | sin guarda | La línea vanilla `wchar* string = nil;` quedó desplazada tras el bloque R3 |
| R6 | src/audio/MusicManager.cpp:1458-1468 | VICEEXT_RADIO_ICONS (+ `RADIO_OFF_TEXT`, define vanilla de reVC, activo en config.h:522) | Asigna emisora → icono: `odStation = track - WILDSTYLE` (0..8), USERTRACK=9 (mp3) y RADIO_OFF=10 (none) |
| R7 | src/audio/MusicManager.cpp:1469-1475 | VICEEXT_RADIO_ICONS / `#else` | `odTextY` vale 68 si hay icono y 22 si no. La rama `#else` deja `odTextY = 22` (vanilla) |
| R8 | src/audio/MusicManager.cpp:1479-1493 | VICEEXT_RADIO_ICONS + __EMSCRIPTEN__ | DIAGNOSTICO: `P4 kind=radio2 schema=1 ... emisora=%d icono=%d mascara=%d` |
| R9 | src/audio/MusicManager.cpp:1512, 1519 (sustituyen o1367, o1374) | **sin guarda** | Sombra y texto del nombre de emisora en `odTextY` en vez de `SCREEN_SCALE_Y(22)`. Compila sin la bandera gracias al `#else` de R7 |
| R10 | src/audio/MusicManager.cpp:1520-1528 | VICEEXT_RADIO_ICONS | Dibuja el icono de 64x56 (escalado) centrado encima del texto: `odRadioSprites[odStation].Draw(CRect, CRGBA(255,255,255,255), uvs)` con la sobrecarga de 10 floats de CSprite2d, que no se ha modificado |
| R11 | src/audio/MusicManager.cpp:47-51, 574-580, 595-601, 614-620, 700-704 | __EMSCRIPTEN__ (no es del mod) | Retune web: la ventana de cambio de emisora se mide en ms reales (`gRetuneStartMs`, caduca a los 600 ms) y fuerza la estática (`RadioStaticTimer - 800`), para que el lag no acumule vueltas del dial. Lo he clasificado RADIO porque es parte del "cambiar de emisora" en web |
| R12 | src/audio/MusicManager.cpp:26-38, 575, 596, 615, 1213-1215 | __EMSCRIPTEN__ | DIAGNOSTICO `RETUNE` (máximo 60 líneas) |

Total en MusicManager: 93 líneas RADIO (91 de ellas tras `VICEEXT_RADIO_ICONS`, menos 26 de traza, más 28 de R5/R9/R11), 4 borradas RADIO, y 45 líneas DIAG relacionadas con la radio (R4, R8, R12 y el printf `radio lens`).

##### Constantes y texturas
- TXD: `MODELS/RADIO.TXD`, en el slot de TxdStore llamado `"radio"`. Tiene 11 texturas con sus 11 máscaras: `wildstyle(A) flash(A) kchat(A) fever(A) vrock(A) vcpr(A) espantoso(A) emotion(A) wave103(A) mp3(A) none(A)`.
- Geometría: icono de 64 x 56 (SCREEN_SCALE), 8 px por encima del texto; texto en Y=68 con icono y Y=22 sin icono.
- Retune web: 20 ticks o 600 ms reales; estática a -800 ms.

#### 2. Piezas fuera de src/ (datos y web)

| pieza | dónde | qué hace |
|---|---|---|
| radio.txd (3.147.304 bytes) | `mods/extended/GameFiles/ViceExtended/models/radio.txd`, copiado tal cual a `gta_vc_browser/streamed/models/radio.txd` | Fuente de los 11 logos. Es un fichero **del mod** |
| manifiesto | `gta_vc_browser/web/public/manifest.json` → `"models/radio.txd"` (lo genera `tools/gen_manifest.py`, que recorre `streamed/` con las claves en minúsculas) | Hace que el fopen de `MODELS/RADIO.TXD` lo sirva la capa on-demand (`web/ondemand.js`, que normaliza a minúsculas en la l.231) |
| no está en `bootseed.list` | — | radio.txd **no** se precarga: se trae al mostrarse por primera vez el nombre de una emisora |
| `tools/txd_merge.py` | gta_vc_browser/tools | Alternativa que no se usa en el código actual: meter los 11 logos dentro de `fronten2.txd`. El código vigente carga `radio.txd` directamente |
| Prefetch de emisoras | `web/ondemand.js:305-357, 1101-1135` (`OD.radios`, `noteStation`, `prefetchLastStation`, fast-fail de los `.adf`) | Audio de las emisoras en web (infraestructura web, no del mod): precarga la siguiente emisora del dial y nunca bloquea por un .adf |
| Radio ligera | `tools/convert_radio.py`, `patches/NOTES.md` §G/§I, y en src `audio/oal/stream.cpp:1281-1308` (un `.adf` puede ser MP3 plano) | Formato de los .adf servidos (INFRA-WEB) |
| Tecla sin repetición | `src/skel/glfw/glfw.cpp:1697-1707` (INFRA-WEB, de otro grupo) | Evita los "taps fantasma de radio" que provoca el auto-repeat de Emscripten |
| Robustez a longitud 0 | `src/audio/MusicManager.cpp:140-141, 189-193, 263-265, 1263` (INFRA-WEB) | La radio no divide por cero si falta un .adf |

#### 3. Dependencias hacia MOD-VIEJO / SIN-GUARDA (qué se rompería)

1. **`TXDSTORESIZE = 1500`** (src/core/config.h:27, cambio de límites del mod **sin guarda**; vanilla 1385). `AddTxdSlot("radio")` necesita un slot libre. Hoy sobran porque el límite se subió para los TXD de las IMG del mod. Al quitar las IMG del mod y volver a 1385 con el gta3.dir vanilla, debería sobrar al menos un slot. Pero si se reimplementa el mod con sus TXD y el límite se queda justo, `AddTxdSlot` devuelve -1 y **los iconos desaparecen sin error** (el código no lo comprueba, sólo `odSlot >= 0`). Hay que conservar el margen.
2. **radio.txd es un fichero del mod**: tiene que seguir en `streamed/models/` y en el manifiesto aunque se limpie el resto de assets de Vice Extended.
3. **Carga on-demand sin reintento**: `odRadioInit = true` se fija aunque `LoadTxd` falle. Si la capa on-demand aplaza el fichero (lo hace en partida, `wkFastFail` en ondemand.js:708), los iconos no salen en toda la sesión. Hoy funciona porque el fichero suele estar ya en IDB/MEMFS. Esto es fragilidad propia de RADIO, no una dependencia de código viejo, pero conviene saberlo antes de tocar bootseed/manifiesto.
4. **R9 sin guarda** (MusicManager.cpp:1512, 1519): depende de `odTextY`, que existe en las dos ramas de R7. No depende de nada más.
5. **Tooling que exige trazas de radio** (si se quitan R4/R8 sin tocar estas herramientas, fallarán):
   - `gta_vc_browser/tools/check-served-build.sh:165` busca la cadena `P4 kind=radio2 schema=1` dentro del **wasm**.
   - `gta_vc_browser/tools/viceext-log-check.py:2973, 3017, 4001-4025` (P4.11): consume los eventos `radio`/`radio2` con los campos `emisora` e `icono`. Da FALLO si una emisora 0..8 sale con `icono=-1` ("textura ausente en radio.txd").
   - `gta_vc_browser/web/lib/index.js:208` (ODPRI): prioriza `RETUNE` y `RADIOTRACK` en el envío de trazas.
   - `patches/NOTES.md:147`: verificación headless con `[web] radio lens ms:` (printf de MusicManager.cpp:196-203).
6. **No hay dependencia** de PedArbiter, ViceExtFeatures, CCamera::s_viceExt*, Hud.cpp, Font.cpp ni de ningún otro `VICEEXT_*`. El dibujado usa sólo API vanilla (CTxdStore, CSprite2d::SetTexture/Draw, CFont, SCREEN_SCALE_*) y librw (`RwTexDictionaryFindNamedTexture`). La llamada a `MusicManager.DisplayRadioStationName()` está en `src/core/main.cpp:1824` (Render2dStuff) y es vanilla, fuera del delta.
7. Relacionado pero **no** RADIO (no se protege): `src/core/Cam.cpp:2693-2697` (SIN-GUARDA, grupo cámara). El botón de radio del mando (LeftShoulder1 = ForceCameraBehindPlayer) ya no recentra la cámara del coche. Quitarlo devuelve el comportamiento vanilla y no afecta a los iconos. `src/core/ControllerConfig.cpp:299-302` (SIN-GUARDA) asigna 'R' a `PED_RELOAD`, que comparte tecla con `VEHICLE_CHANGE_RADIO_STATION` (OPTIONAL_EXTRA vanilla). Quitarlo no afecta a la radio.

#### 4. Lista cerrada para proteger

- `src/core/config.h:473` `#define VICEEXT_RADIO_ICONS`
- `src/audio/MusicManager.cpp`: 10-12, 47-51, 574-580, 595-601, 614-620, 700-704, 1379-1425, 1458-1475, 1479-1493, 1512, 1519-1528 (las trazas 26-38, 575, 596, 615, 1213-1215, 1410-1422 y 1480-1492 son opcionales; si se quitan, hay que actualizar el tooling del punto 3.5)
- `src/audio/MusicManager.cpp` INFRA-WEB de radio: 140-141, 189-193, 263-265, 1263; `src/audio/oal/stream.cpp:1281-1308`; `src/skel/glfw/glfw.cpp:1697-1707`
- Datos: `gta_vc_browser/streamed/models/radio.txd` (y su origen en `mods/extended/GameFiles/ViceExtended/models/radio.txd`) y la entrada `models/radio.txd` en `web/public/manifest.json`
- Límite: margen de `TXDSTORESIZE` para el slot `"radio"`

### E.3 PedArbiter: qué es y quién lo usa

#### Qué es
Fichero nuevo de los agentes: `src/core/PedArbiter.h` (25 líneas, sin guarda) y `src/core/PedArbiter.cpp`
(123 líneas; la lógica bajo `#ifdef VICEEXT_PEDARBITER`, trazas `PEDCLAIM` bajo `__EMSCRIPTEN__`).
Árbitro de "quién manda" sobre el ped del jugador, por capacidad:

- Capacidades `ePedCap`: `PEDCAP_MOVIMIENTO, PEDCAP_POSTURA, PEDCAP_APUNTAR, PEDCAP_CAMARA, PEDCAP_ARMA`.
- Carriles `ePedLane`: `PEDLANE_MOTOR` (vanilla, dueño por defecto), `PEDLANE_NADO`, `PEDLANE_AGACHADO_MOD` (no lo usa nadie), `PEDLANE_LEY` (1ª persona).
- API: `ViceExtPedOwner(cap)`, `ViceExtPedOwns(lane, cap)`, `ViceExtPedClaim(lane, cap)` (sólo si el dueño es MOTOR o ya es él; dispara el callback de release del anterior), `ViceExtPedRelease(lane, cap)`, `ViceExtPedOnRelease(lane, fn, ud)` (nadie lo registra), `ViceExtPedResetAll()`.
- Estado: `ms_aOwner[NUM_PEDCAPS]`, `ms_apfnRelease/ms_audRelease[NUM_PEDLANES]` (estáticos, sólo con la bandera).
- **`ViceExtPedClaim(PEDLANE_NADO, …)` consulta `ViceExtFeatures::On("EnableSwimming", true)`** (PedArbiter.cpp:41): el `features.ini` puede apagar el nado entero. Incluye `ViceExtFeatures.h`.
- Sin `VICEEXT_PEDARBITER`: `Owner` devuelve siempre MOTOR, `Claim` devuelve false → el nado nunca se adueña de nada.
- `ViceExtFeatures::On` sólo existe con `VICEEXT_FEATURES_INI` (ViceExtFeatures.cpp:7-284): con PEDARBITER y sin FEATURES_INI no enlaza.

#### Quién lo usa (todo src/)

| sitio | guarda | uso | dueño funcional |
|---|---|---|---|
| peds/PlayerPed.cpp:5 | sin guarda | include | — |
| peds/PlayerPed.cpp:3773-3776 | VICEEXT_SWIMMING | `ViceExtPedClaim(PEDLANE_NADO, MOVIMIENTO/POSTURA/APUNTAR/ARMA)` al entrar a nadar | **NADO** |
| peds/PlayerPed.cpp:3704-3707 | VICEEXT_SWIMMING | `ViceExtPedRelease(PEDLANE_NADO, …)` al salir | **NADO** |
| peds/PlayerPed.cpp:1101, 2668 | VICEEXT_SWIMMING | `Owns(NADO, ARMA)` (armas/puños al nadar) | **NADO** |
| peds/PlayerPed.cpp:159 | VICEEXT_CROUCH + VICEEXT_SWIMMING | `Owns(NADO, APUNTAR)` | NADO ↔ agachado |
| peds/PlayerPed.cpp:1327 | VICEEXT_AIM_CLASSICAXIS + VICEEXT_SWIMMING | `Owns(NADO, APUNTAR)` | NADO ↔ ClassicAXIS |
| peds/PlayerPed.cpp:5128, 5328 | VICEEXT_CROUCH | `Owns(NADO, POSTURA)` → no agacharse nadando | MOD-VIEJO (agachado) consulta al nado |
| peds/PlayerPed.cpp:6047 | sin guarda | `!Owns(NADO, APUNTAR)` en la decisión de apuntado ClassicAXIS | MOD-VIEJO (ClassicAXIS) consulta al nado |
| peds/Ped.cpp:10 | sin guarda | include | NADO |
| peds/Ped.cpp:1985 | VICEEXT_SWIMMING | `ProcessBuoyancy`: `Owns(NADO, MOVIMIENTO)` → sin flotación/ahogo | **NADO** |
| peds/Ped.cpp:2903 | sin guarda | `ProcessControl`: no "en el aire" mientras nada | **NADO** |
| core/Cam.cpp:11 | sin guarda | include | — |
| core/Cam.cpp:653-654 | VICEEXT_PEDARBITER | `ViceExtAimingOverShoulder`: sin cámara de hombro si el nado apunta/tiene la cámara | MOD-VIEJO (cámara apuntado) consulta al nado |
| core/Cam.cpp:4292-4293 | sin guarda | `Process_AimWeaponFovLerp` (ClassicAXIS): FOV por defecto si el nado apunta/tiene cámara | MOD-VIEJO (ClassicAXIS) consulta al nado |
| core/Camera.cpp:36 | sin guarda | include | — |
| core/Camera.cpp:974, 1460, 1470, 1472, 1477, 2650 | VICEEXT_FIRST_PERSON | carril `PEDLANE_LEY` sobre `PEDCAP_CAMARA` (claim/release del conmutador de 1ª persona; la puerta exige cámara libre = MOTOR o LEY) | MOD-VIEJO (1ª persona) |
| core/Game.cpp:2, 989 | sin guarda | `ViceExtPedResetAll()` en `CGame::InitialiseWhenRestarting` | infraestructura del árbitro (lo necesita el NADO) |
| control/Script6.cpp:24, 1008 | sin guarda | `bSwimAim = Owns(NADO, APUNTAR)` desactiva el chequeo "PC" de apuntar-a-ped del opcode mientras se apunta nadando | NADO (sin guarda) |
| renderer/Hud.cpp:28 | sin guarda | include | — |
| renderer/Hud.cpp:427 | `__EMSCRIPTEN__` | traza `SWIMHUD` | DIAGNOSTICO del nado |
| renderer/Hud.cpp:455 | sin guarda | no se dibuja la cruz si el nado tiene `APUNTAR` | NADO (sin guarda) |

#### ¿Lo usa el NADO?
Sí, es su columna vertebral: el nado se "adueña" de MOVIMIENTO/POSTURA/APUNTAR/ARMA (PlayerPed.cpp:3773-3776)
y todo lo demás pregunta `ViceExtPedOwns(PEDLANE_NADO, …)`: flotación y ahogo (Ped.cpp), caída (Ped.cpp:2903),
cruz del HUD (Hud.cpp:455), opcode de script (Script6.cpp:1008), y el apagado de las leyes de apuntado/cámara
del mod mientras se nada (Cam.cpp:653, 4292; PlayerPed.cpp:159, 1327, 5128, 5328, 6047). Además es el
punto donde `features.ini` `EnableSwimming` apaga el nado (vía ViceExtFeatures).

Para proteger el nado hay que conservar: `PedArbiter.h/.cpp` con `#define VICEEXT_PEDARBITER`,
`ViceExtFeatures::On` (o sustituir la línea 41 de PedArbiter.cpp por un simple `true`/la bandera),
`ViceExtPedResetAll()` en Game.cpp:989, y los consumidores NADO de la tabla. Los consumidores
MOD-VIEJO (1ª persona con `PEDLANE_LEY`, ClassicAXIS, agachado, cámara de hombro) pueden irse con su
código; `PEDLANE_LEY` y `PEDLANE_AGACHADO_MOD` quedarían sin uso.

#### ¿Lo usa la RADIO?
No. Ninguna referencia en audio/, MusicManager ni en el dibujo de iconos de emisora.

#### Qué partes de Camera.cpp / Camera.h usa el nado
- Código funcional del nado: **ninguna** de los miembros/estáticos añadidos `s_viceExt*`, `m_bViceExt1stPersonView`,
  `ViceExtAimSens*`, `Process_AimWeapon*`. El nado usa la cámara vanilla (`TheCamera.Orientation`,
  `TheCamera.GetPosition()`, `Cams[ActiveCam].Mode/Beta/Alpha`) y, en Cam.cpp, sus propios bloques
  `VICEEXT_SWIMMING` con `ViceExtIsSwimming()` (Cam.cpp:1202, 1790, 6238; los clasifica el grupo de Cam.cpp).
- Sólo las **trazas** del nado leen `CCamera::s_viceExtAimLawActive` (PlayerPed.cpp:3990 `SWIMFOV`, 4019 línea de estado SWIM).
- Camera.cpp no tiene ningún bloque de nado; su único enlace con el árbitro es el carril LEY de la 1ª persona.
- Acoplamiento inverso (no del nado): ClassicAXIS consulta al carril NADO en Cam.cpp:4292 (`Process_AimWeaponFovLerp`,
  declarado en Camera.h:239) y Cam.cpp:653.

### E.4 ViceExtFeatures: qué es y quién lo usa

#### Qué es

Ficheros nuevos de los agentes: `src/core/ViceExtFeatures.h` (37 líneas, **sin guarda**) y `src/core/ViceExtFeatures.cpp` (284 líneas, cuerpo bajo `#ifdef VICEEXT_FEATURES_INI`; las líneas 1-6 son includes sin guarda).

Es un port del lector de `features.ini` de Vice Extended (`x87/gta-extended`, `src/core/re3.cpp:284`, mINI). Pertenece a la "familia 11".

**API** (namespace `ViceExtFeatures`):
- `void Load()`
  - Lee **una vez** `ViceExtended/features.ini` desde la raíz del juego, con `CFileMgr::LoadFile` y un buffer de 4 KB.
  - Parsea la sección `[Features]` con formato `Clave = valor`, sin distinguir mayúsculas.
  - Traza `P4 kind=inifile` (líneas 214-244, sólo en web): una línea por clave más un resumen `case=1`.
  - Se llama desde `CGame::InitialiseOnceBeforeRW` (`core/Game.cpp:191`, bajo VICEEXT_FEATURES_INI). Si no, la primera llamada a `On()`/`Waypoint()` lo carga sola.
- `bool On(const char *key, bool portDefault)`, por orden de prioridad:
  1. El override del jugador (`kOverrides`) manda y no se puede mover desde el fichero.
  2. Si no hay override, vale el valor del fichero.
  3. Si la clave no está o no hay fichero, vale `portDefault`.
- `int Waypoint(int channel, int portDefault)`: canal R, G o B de `WaypointColorRGB` = r, g, b. Sin la clave devuelve `portDefault`. El default interno de su lector es 180,24,24, pero sólo se usa si la clave existe.
- `bool WasRead()`: **nadie la usa** en src/.

**Las 17 claves (`kKeys`)**. `impl=1` en 14; `impl=0` en CameraShakeInVehicleAtHighSpeed, RandomVehicleModsInTraffic y MirrorModeByDefault, que se leen pero nadie las consulta:

VehiclesDontCatchFireWhenTurningOver, HealthRegenerationUpToHalf, WantedStarsHideOnScreenWhenThereIsNoSearch, RemoveMoneyZerosInTheHud, PlayerDoesntBounceAwayFromMovingCar, StandardCarsUseTurnSignals, CameraShakeInVehicleAtHighSpeed, MilitaryFiringFromTankAtPlayer, DisableBulletTraces, WaypointColorRGB, EnableDistantLights, RecoilWhenFiring, EnableClimbing, **EnableSwimming**, RocketLauncherThirdPersonAiming, RandomVehicleModsInTraffic, MirrorModeByDefault.

**Overrides del jugador (`kOverrides`, todos a 1)**: EnableClimbing, RemoveMoneyZerosInTheHud, VehiclesDontCatchFireWhenTurningOver, HealthRegenerationUpToHalf, WantedStarsHideOnScreenWhenThereIsNoSearch, MilitaryFiringFromTankAtPlayer, StandardCarsUseTurnSignals. **EnableSwimming NO tiene override**: el fichero manda.

**Fichero servido**: `gta_vc_browser/streamed/ViceExtended/features.ini`, idéntico en `bootseed/` y en `mods/extended/GameFiles/`. Valores relevantes:

- `EnableSwimming = 1`
- `WaypointColorRGB = 180, 24, 24`
- `RecoilWhenFiring = 1`
- `EnableDistantLights = 1`
- `RocketLauncherThirdPersonAiming = 1`
- `PlayerDoesntBounceAwayFromMovingCar = 1`
- `DisableBulletTraces = 0`
- El resto de claves están a 0, aunque varias quedan anuladas por los overrides.

**Riesgo estructural.** La cabecera declara la API sin guarda, pero las definiciones sólo existen con `VICEEXT_FEATURES_INI`. Todos los llamadores sin guarda (tabla de abajo) dejan de **enlazar** si se apaga la bandera o se borra el .cpp. Quitar ViceExtFeatures obliga a tocar todos esos sitios.

#### Quién lo usa (todo src/)

| fichero:línea | Uso | Clave | Guarda real |
|---|---|---|---|
| core/Game.cpp:3 | `#include` | — | sin guarda |
| core/Game.cpp:191 | `ViceExtFeatures::Load()` | — | VICEEXT_FEATURES_INI |
| core/PedArbiter.cpp:3 | `#include` | — | sin guarda |
| **core/PedArbiter.cpp:41** | `ViceExtPedClaim`: `if (l == PEDLANE_NADO && !On("EnableSwimming", true)) return false;` | **EnableSwimming** | VICEEXT_PEDARBITER |
| core/Radar.cpp:22 | `#include` | — | sin guarda |
| core/Radar.cpp:1108-1110 | color del waypoint | WaypointColorRGB (`Waypoint(0..2, VICEEXT_GPS_COLOR_*)`) | sólo MAP_ENHANCEMENTS (sin VICEEXT) |
| core/World.cpp:2 | `#include` | — | sin guarda |
| core/World.cpp:1948 | luces lejanas | EnableDistantLights | sin guarda |
| peds/PedFight.cpp:21 | `#include` | — | sin guarda |
| peds/PedFight.cpp:3890 | no rebotar del coche | PlayerDoesntBounceAwayFromMovingCar | sin guarda |
| peds/PlayerPed.cpp:39 | `#include` | — | sin guarda |
| peds/PlayerPed.cpp:2719 | lanzacohetes en 3ª persona | RocketLauncherThirdPersonAiming | sin guarda |
| peds/PlayerPed.cpp:3292 | escalada | EnableClimbing | VICEEXT_CLIMB |
| peds/PlayerPed.cpp:3305 | no rebotar del coche | PlayerDoesntBounceAwayFromMovingCar | sin guarda |
| peds/PlayerPed.cpp:6323 | regeneración de salud | HealthRegenerationUpToHalf | sin guarda |
| renderer/Hud.cpp:2 | `#include` | — | sin guarda |
| renderer/Hud.cpp:816 | dinero sin ceros | RemoveMoneyZerosInTheHud | sin guarda |
| renderer/Hud.cpp:1066 | estrellas ocultas | WantedStarsHideOnScreenWhenThereIsNoSearch | sin guarda |
| renderer/SpecialFX.cpp:4 | `#include` | — | sin guarda |
| renderer/SpecialFX.cpp:393, 451 | trazas de bala | DisableBulletTraces (default false) | sin guarda |
| vehicles/Automobile.cpp:3 | `#include` | — | sin guarda |
| vehicles/Automobile.cpp:1842, 1856 | intermitentes | StandardCarsUseTurnSignals | `defined(VICEEXT_TURN_SIGNALS) \|\| defined(VICEEXT_STD_TURN_SIGNALS)` |
| vehicles/Automobile.cpp:3913 | tanque dispara al jugador | MilitaryFiringFromTankAtPlayer | sin guarda |
| vehicles/Automobile.cpp:4770 | no arder al volcar | VehiclesDontCatchFireWhenTurningOver | sin guarda |
| weapons/Weapon.cpp:2 | `#include` | — | sin guarda |
| weapons/Weapon.cpp:299 | retroceso | RecoilWhenFiring | VICEEXT_RECOIL |

Tooling que consume su traza:
- `gta_vc_browser/tools/check-served-build.sh:173`: marca `P4 kind=inifile schema=1`. Las líneas 172 (`P4 kind=ini`) y 196 (hash de `ViceExtended/features.ini`) también tocan features.ini.
- `tools/viceext-log-check.py:4060-4082`: bloques P4.14 `kind=ini` y familia 11 `kind=inifile`.
- `tools/test_controls_axis.py:1098-1134`: genera líneas `P4 kind=inifile`.

#### ¿Lo usa el NADO?

**Sí, indirectamente y en un solo punto.**

- **Dónde:** `core/PedArbiter.cpp:41`, dentro de `ViceExtPedClaim`, bajo VICEEXT_PEDARBITER.
- **Qué hace:** con `EnableSwimming` falso, el carril `PEDLANE_NADO` no puede reclamar ninguna capacidad, y todo el nado queda inactivo (vuelve al comportamiento vanilla).
- **Clave:** `EnableSwimming`, default del port `true`, sin override. El fichero servido la trae a 1.
- **Depende de:** `ViceExtFeatures::On()`, `Load()` y `Parse()`, es decir, del .cpp entero bajo `VICEEXT_FEATURES_INI`, y de la bandera VICEEXT_FEATURES_INI en config.h. Si se borra ViceExtFeatures, `PedArbiter.cpp:3/41` no compila o no enlaza.
- **Opciones al limpiar:** sustituir la línea por `true` constante, o conservar un lector mínimo de `EnableSwimming`.

#### ¿Lo usa la RADIO?

**No.** `audio/MusicManager.cpp` no incluye ni llama a ViceExtFeatures, y ninguna de las 17 claves es de radio.

### E.5 Grupo PlayerPed.cpp/.h

Base: `diff -U0 --strip-trailing-cr` frente a reVC a16fcd8d. Filas en `rows_g1_playerped.tsv` (163 filas, `check.py` sin huecos).

#### Totales (líneas añadidas; borradas aparte)

| fichero | DIAGNOSTICO | MOD-VIEJO | NADO | SIN-GUARDA | INFRA-WEB | total + | borradas |
|---|---:|---:|---:|---:|---:|---:|---:|
| peds/PlayerPed.cpp | 2025 | 1923 | 391 | 488 | 0 | 4827 | 22 (13 MOD-VIEJO, 6 SIN-GUARDA, 3 NADO) |
| peds/PlayerPed.h | 5 | 48 | 4 | 28 | 0 | 85 | 0 |
| **suma** | **2030** | **1971** | **395** | **516** | **0** | **4912** | **22** |

Dónde están las 2030 de DIAGNOSTICO: 1502 dentro de bloques MOD-VIEJO, 275 tras `__EMSCRIPTEN__` solo, 173 dentro del NADO y 79 dentro del código SIN-GUARDA de ClassicAXIS.
Llamadas de traza: 55 `ODTRACES/ODTRACEI` y 22 `VICEEXT_P4_EMIT`, más un `EM_ASM` (P4 identity).

No hay INFRA-WEB en estos ficheros. Todo lo que solo está tras `__EMSCRIPTEN__` es traza del mod o del arnés.

#### (a) SIN-GUARDA: lógica del mod viejo que se compila siempre

| # | sitio | qué altera del vanilla | importancia |
|---|---|---|---|
| 1 | PlayerPed.cpp:5799-6278 (función `ViceExtProcessPlayerPedControl`, predicados `ViceExtIsAbleToAim`/`IsType1stPerson`/`IsTypeTwoHanded`/`IsWeaponPossiblyCompatible`/`IsTypeMelee`, `ViceExtHasPadInHands`, `ViceExtFind3rdPersonMouseTarget`, `ViceExtRotatePlayer`) + llamadas en 6343-6347 (`ProcessControl`) | Cada frame, antes de `CPed::ProcessControl`, corre la ley de apuntado de ClassicAXIS. Hace `TakeControl(MODE_AIMING)` y devuelve el modo anterior al soltar (con `m_bLookingAtPlayer`), pone `SetLookFlag/SetAimFlag/SetLookTimer`, toca `m_fRotationDest` y `m_fFPSMoveHeading`, mueve el IK con tres `MoveLimb`, gestiona el fijado y la marca de fijado (`CCamera::s_viceExtAimLast*`), hace `ClearPointGunAt/ClearWeaponTarget` al soltar y `SetDuck` si se suelta agachado, y elige el objetivo blando de ratón (`ReactToPointGun`, `Say`). Los miembros solo se declaran bajo `VICEEXT_AIM_WALK && VICEEXT_AIM_CLASSICAXIS` (PlayerPed.h:127-138): sin esas banderas no compila. | **ALTA** |
| 2 | PlayerPed.cpp:3135-3136 + borradas vanilla o1500-1502 | Se quita para siempre el `if (MovementDisabledBecauseOfTargeting()) { upDown = leftRight = 0; }` de `PlayerControlZelda`. Ahora solo se guarda `s_odStickBlocked`, una variable definida bajo `VICEEXT_AIM_CLASSICAXIS` (línea 1435). | **ALTA** |
| 3 | PlayerPed.cpp:6317-6342 | Regenera vida (+0,15 por frame) hasta la mitad de la vida máxima cuando el jugador está quieto, si `ViceExtFeatures::On("HealthRegenerationUpToHalf", true)`. La bandera `VICEEXT_HEALTH_REGEN` de config.h no se usa aquí. | **ALTA** |
| 4 | PlayerPed.cpp:3300-3306, 3318-3319 + borrada o1566 | El jugador ya no esquiva ni salta ante un coche en marcha (`PlayerDoesntBounceAwayFromMovingCar`, por defecto 1). `VICEEXT_NO_CAR_BOUNCE` y `VICEEXT_NO_JUMP_FROM_CAR` no se usan aquí. | **ALTA** |
| 5 | PlayerPed.cpp:2713-2724, 2749 | El lanzacohetes ya no entra en el modo francotirador ni en 1ª persona (`RocketLauncherThirdPersonAiming`, por defecto 1). `VICEEXT_ROCKET_3RD_PERSON` solo guarda el predicado y la traza. | **ALTA** |
| 6 | PlayerPed.cpp:2823-2827 + borrada o1351 (dentro de `#ifdef FREE_CAM`) | Quita `&& CCamera::bFreeCam` de la rotación de apuntado con ratón. Ahora rota siempre que haya `m_bUseMouse3rdPerson`. | MEDIA-ALTA |
| 7 | PlayerPed.cpp:1073-1084 y 1252-1263 + borrada o840 | ClassicAXIS C10: en `PlayerControlSniper`/`PlayerControlM16` el modo de cámara se fuerza a `MODE_FOLLOWPED` alrededor de `Fire`. Solo actúa si `CCamera::s_viceExtAimLawActive`. | MEDIA |
| 8 | PlayerPed.cpp:1109-1115, 1130 | ClassicAXIS C8: con la ley de apuntado activa, `bDontAllowWeaponChange = true`. | MEDIA |
| 9 | PlayerPed.cpp:3213-3215 | En `PlayerControlZelda`, `m_fRotationDest` lo deciden `ViceExtStrafeAiming`/`ViceExtMoveHeading` (definidas bajo CLASSICAXIS). | MEDIA |
| 10 | PlayerPed.h:27-54 | `ViceExtAimHeavy` y `ViceExtAimLongGun` en línea (los usan Cam.cpp:4317/4619 y PlayerPed.cpp:1524/6193). | BAJA |
| 11 | PlayerPed.cpp:13, 32, 35, 39 | Includes: Sprite.h, ControllerConfig.h, WeaponInfo.h y ViceExtFeatures.h. | BAJA |

Cambios MOD-VIEJO "neutros por macro". Están fuera de `#ifdef`, pero con la bandera apagada expanden a lo vanilla. Al quitar lo viejo hay que devolver el texto vanilla en estos sitios:

- `VICEEXT_ENGINE_DUCK_KEY()`: 1055, 1058, 1232, 1235, 2678, 2686 (vanilla o682/685/820/823/1242/1250).
- `ViceExtCanAim()`: 2701, 2872, 2918 (o1262/1382/1419).
- `ViceExtAimWithArm()`: 2830 (o1354).
- `CanSprintWithCurrentWeapon()`: 977-983, 2286, 3275, 3279, PlayerPed.h:108 (o908/1554).
- `PlayIdleAnimations`: 6791/6798 (o1993).

#### (b) DIAGNOSTICO por etiqueta

| líneas | etiqueta | rangos |
|---:|---|---|
| 617 | P4 identity / move_begin / move_end / input_edge / input_hold_end / aim / pose_* / cam_* (`VICEEXT_P4_EMIT`, `ViceExtMoveTick`, `ViceExtP4Cam*`) | 1567-2183 |
| 131 | AIMDIR + P4 armsight (todo `ViceExtAimDirTrace`, bloque AIM_WALK) | 2989-3119 (+ llamada 6618-6621, decl. h:139) |
| 127 | CROUCH2 1 Hz | 5535-5661 |
| 114 | PEDAT/R19 `ViceExtPedAtTrace` (sonda de agua del arnés) + `ViceExtWeaponInfoOf` | 5684-5797 (+ llamada 6622-6625, decl. h:141-144) |
| 97 | WINFO (`ViceExtClipName`, `ViceExtWeaponInfoTrace`) | 19, 191-283, 2698-2700 |
| 95 | VICEEXT swim move / SWIMFOV / SWIM2 move / SWIMERR | 3944-4038 |
| 94 | P4 partials + pose del agachado | 4980-5073 |
| 67+32 | VICEEXT reload key/no/anim/start + done/fail | 3417-3426, 3437-3451, 3470-3479, 3482, 3488, 3492, 3496, 3500, 3514-3516, 3528-3535, 3537-3552; 6479-6510 |
| 66 | medidor del tramo agachado R25 | 5468-5533 |
| 42 | P4 stand_end | 3137-3179 |
| 30 | P4 firewait + MoveTick/CamTick | 6387-6417 |
| 28 | motivo de la rueda R27 | 5376-5403 |
| 25 | AX7 | 2234-2258 |
| 24 | ODRUMBO | 3220-3243 |
| 24 | VICEEXT crouch pose off | 4954-4977 |
| 23+22+19+6+7 | AIMLOCK / AIMIK / AIMLAW / P4 cam_begin (dentro del ClassicAXIS SIN-GUARDA) | 6049-6067, 6083-6085, 6097-6099, 6123-6126, 6158-6167, 6173-6181, 6183-6186, 6219-6240, 6267-6269 |
| 21+2 | R3P | 2641-2661, 2721-2722 |
| 20 | P4 fire | 2796-2815 |
| 19 | VICEEXT climb | 4158-4160, 4177-4179, 4190-4192, 4200-4202, 4217-4223 |
| 19+19+15+17+9+3+3 | P4 roll_end/roll_cancel, roll_start, roll_step, crouchspeed, pose_begin, pose_enter, pose_exit | 4838-4856, 4893-4911, 5310-5324, 5424-5440, 4918-4922, 5153-5156, 5203-5205, 1426-1432, 1528-1530 |
| 14 | AIMWPN | 1116-1129 |
| 13 | VICEEXT sprint | 786-798 |
| 13 | AIMWALK | 6299, 6303-6314 |
| 11+11 | P4 kind=ini opt=10 / opt=8 | 3307-3317, 6331-6341 |
| 14+13+13+10+7+6+3·5 | SWIMNAT, SWIMIDLE, SWIMEF, swim exit / SWIM2 exit, swim enter, SWIMFASE, punos / arma-devuelta / trepa-off / estado SWIMFOV | 3576-3589, 3732-3744, 3927-3939, 3721-3730, 3808-3814, 3896-3901, 3656-3658, 3664-3666, 3757-3759, 3644-3646, 3769-3771 |
| resto | VICEEXT crouch on/off/roll/repetido, llamada a `ViceExtCrouchPoseTrace`, includes de traza (33, 37) | 5144-5176, 5187-5188, 5344-5352 |

Esquemas `P4 kind=` emitidos aquí (25): identity, overflow, move_begin, move_end, input_edge, input_hold_end, aim, pose_begin, pose_end, pose_exit, partials, cam_begin, cam_end, cam_cancel, cam_first, armsight, stand_end, fire, firewait, ini, roll_start, roll_step, roll_end, roll_cancel, crouchspeed.

#### (c) Qué hace cada bandera VICEEXT_* en estos ficheros (MOD-VIEJO, 1971 líneas)

| bandera | líneas | qué hace aquí |
|---|---:|---|
| `VICEEXT_CROUCH` | 1131 | Agachado propio con la tecla PED_DUCK (C): estado `odCrouched`, clips `ASSOCGRP_PLAYERCROUCH`, tope de velocidad (`ViceExtCrouchLimitSpeed`), ritmo del clip, pose de apuntar agachado, rueda lateral (`s_odRoll*`), `CPed::ViceExtCrouchShooting` y la API única `ViceExtIsAiming`. Anula la tecla duck del motor (`VICEEXT_ENGINE_DUCK_KEY`). En `SetRealMoveAnim`, si se está agachado, devuelve el clip propio. |
| `VICEEXT_AIM_CLASSICAXIS` | 337 (+17 en ramas `#else`) | Estado de movimiento `s_odMove` (`ViceExtPrepareMove`/`GetMove`/`MoveHeading`/`StickHeading`/`StickBlocked`/`MoveWalkaround`), `ViceExtIsAimingNow`, `DoWeaponSmoothSpray` sustituido (-1 / 0,00001), rumbo de marcha en `1stPersonRunAround`, `clearAimFlag` en `ProcessPlayerWeapon`, `ViceExtMoveReset` (destructor/SetInitialState) y `ViceExtStrafeAiming`. |
| `VICEEXT_CLIMB` | 164 | Escalada con la tecla de salto: detección del borde por rayos, clips `CLIMB_*` (`ASSOCGRP_PLAYERCLIMB`); además depende de `ViceExtFeatures::On("EnableClimbing")`. |
| `VICEEXT_AIM_WALK` | 92 | Apuntando solo se camina: tope de `padMoveInGameUnit`/`m_fMoveSpeed` a 1,0, sin esprint; `odAimWalkActive/Uncapped` para el clip de andar en `SetRealMoveAnim` (817-855). |
| `VICEEXT_MANUAL_RELOAD` | 89 | Recarga con PED_RELOAD/'R' (`ViceExtTryManualReload`) y su llamada en `ProcessControl`. |
| `VICEEXT_SHOTGUN_AIM` | 58 (+9 `#else`) | `ViceExtCanAim` (escopetas + SHOTGUN2 apuntan) y `ViceExtAimWithArm` (pose con brazo para escopetas y lanzagranadas). |
| `VICEEXT_ROCKET_3RD_PERSON` | 23 | Solo el predicado `ViceExtRocket3rdPerson` (el uso real está SIN-GUARDA). |
| `VICEEXT_ALT_SIT` | 22 | Con la tecla de andar (`CPad::GetPedWalk`), `m_fMoveSpeed = 0`. |
| `VICEEXT_SPRINT_HEAVY` | 11+5 | `CanSprintWithCurrentWeapon()` devuelve true. |
| `VICEEXT_WEAPON_SIGHTS` | (dentro de WINFO) | Solo imprime `m_nSight` en la traza WINFO. |
| `VICEEXT_SWIMMING` | NADO | Ver `nado_playerped.md`. |

#### (d) INFRA-WEB

Ninguna línea.

#### (e) Sorpresas

1. **`if` colgante en PlayerPed.cpp:6183-6186.** Es `if (m_bHasLockOnTarget && m_pPointGunAt)` seguido de `#ifdef __EMSCRIPTEN__ s_odLockWas2 = true; #endif`. En nativo el `if` se queda con la sentencia siguiente (6192, `if (!bIsDucking ...) m_fRotationDest = ...`), así que el comportamiento cambia según la plataforma.
2. **`VICEEXT_P4_EMIT` fuera de `__EMSCRIPTEN__`.** Se usa en 5316-5324 (roll_step) solo bajo `VICEEXT_CROUCH`, pero el macro solo existe en web. En nativo con CROUCH no compila.
3. **Las banderas no son opcionales de verdad.** Hay código sin guarda que llama a funciones definidas bajo banderas, y viceversa:
   - `s_odStickBlocked`, `ViceExtStrafeAiming` y `ViceExtMoveHeading` (CLASSICAXIS), y miembros declarados solo bajo AIM_WALK+CLASSICAXIS.
   - `ViceExtPedAtTrace`/`ViceExtWeaponInfoOf`: declarados solo bajo `__EMSCRIPTEN__`, definidos bajo CROUCH+EM y llamados en 6624 y en Pad.cpp:347.
4. **Banderas de config.h que aquí no se usan.** `VICEEXT_HEALTH_REGEN`, `VICEEXT_NO_CAR_BOUNCE` y `VICEEXT_NO_JUMP_FROM_CAR` no se miran: las funciones van por `ViceExtFeatures::On(clave, true)`, con valor por defecto 1.
5. **La ley ClassicAXIS consulta el nado.** En 6047 exige `!ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)`. Al reimplementar el apuntado hay que conservar "nadando no se apunta".
6. **`#ifdef __EMSCRIPTEN__` duplicado y anidado** en 1116-1117.
7. **Trazas que dependen de banderas viejas.** La traza SWIM2 del nado usa `ViceExtBlendWeight` (bloque AIM_WALK), `odCrouched` (CROUCH||AIM_WALK) y `ViceExtIsAiming` (CROUCH). Si se quitan esas banderas sin quitar la traza, la build web se rompe.

### E.6 Grupo Cam.cpp

Delta: +1952 / −817 (base reVC a16fcd8d). Totales según `check.py rows_g2_cam.tsv`:

| Categoría | + líneas | − líneas |
|---|---:|---:|
| SIN-GUARDA | 1157 | 817 |
| MOD-VIEJO (bajo VICEEXT_*) | 359 | 0 |
| DIAGNOSTICO | 348 | 0 |
| NADO | 88 | 0 |
| RADIO | 0 | 0 |
| INFRA-WEB | 0 | 0 |

Desglose de SIN-GUARDA (+ / −): ley de apuntado ClassicAXIS 495+16 (FREE_CAM) / 5+8; ley de coche "BARRIDO 1" 479+12 (FREE_CAM) / 77+572 (FREE_CAM); cosmético (solo espacios o blancos) 130 / 134; llamadas de sensibilidad de apuntado 14 / 14; arreglo de 1ª persona 6 / 4; varios 5 / 3.
En Cam.cpp **no hay INFRA-WEB**: los 180 bloques `__EMSCRIPTEN__` son todos trazas.

#### (a) SIN-GUARDA — cambios del mod viejo que alteran vanilla siempre

| Sitio (src/core/Cam.cpp) | Función | Qué altera | Imp. |
|---|---|---|---|
| +423-425 / −o193 | `CCam::Process` | Descomenta `case MODE_AIMING` y lo conecta a `Process_AimWeapon` (ley ClassicAXIS). Se activa cuando PlayerPed.cpp:6082 hace `TakeControl(MODE_AIMING)`. | ALTA |
| +4141-4255 | **`CCam::Process_AvoidCollisions`** (miembro NUEVO) | LOS + 5 esferas de CamNew; esconde peds (<0,5 m) en apuntado; escribe `s_odDistObs`. Lo usan la ley de coche y la de apuntado. | ALTA |
| +4257-4353 | **`Process_AimWeaponFovLerp`**, **`Process_AimWeaponCrouchOffset`** (NUEVOS), constantes `AIM_*` | FOV 70→50 al apuntar; bajada del objetivo agachado. FovLerp llama a `ViceExtPedOwns(PEDLANE_NADO,…)` **sin guarda** (4292-4298). | ALTA |
| +4355-4641 | **`CCam::Process_AimWeapon`** (NUEVO, menos el bloque RECOIL 4499-4509 y CLASSICAXIS 4544-4554) | La ley de apuntado completa del mod: hombro 0,2, LOS, lock-on con horShift/verShift, ratón −2,5/4,0, clamp ±50. Usa `CCamera::s_viceExtAim*`, `s_viceExtAimLawActive`, `s_viceExtAimSwitchSpeed`, `s_viceExtAimPrevHor/Ver` (Camera.h/.cpp). | ALTA |
| +627-695 (sin 652-656 ni 665-675) | `ViceExtAimShoulder`, `ViceExtAimingOverShoulder`, `ViceExtAimShoulderSmoothed` | Desplaza el objetivo 0,2 m al hombro cuando se apunta. | ALTA |
| +1419-1420, +1814-1819, +6262-6263 | `Process_FollowPed`, `Process_FollowPedWithMouse`, `Process_FollowPed_Rotation` | `TargetCoors += hombro suavizado` en los follows a pie. | ALTA |
| +1850-1853 / −o1398-1399 | `Process_FollowPedWithMouse` | El stick vanilla pasa de `1/14` a `1/20` (ClassicAXIS B8): **la cámara a pie con mando es más lenta** siempre. | ALTA |
| +1886-1895 / −o1422-1423 | `Process_FollowPedWithMouse` | Clamp vertical +45° → +60°/−89,5° (±50° apuntando). | MEDIA |
| +2349-2717, 2764-2811, 2819-2843, 2956 / −o1795-1847 | **`Process_Cam_On_A_String` reescrita** (cámara de coche) | Esférica CamNew + Beta como estado + ratón/palo + detector de mirada + retorno pasivo de 1,5 s + hold de 5 frames al entrar. Deja de llamar a `Cam_On_A_String_Unobscured`, `WorkOutCamHeight`, `RotCamIfInFrontCar`, `FixCamWhenObscuredByVehicle` y `AvoidTheGeometry`: esas funciones vanilla siguen en el fichero, pero el string-cam ya no las llama. El eje vertical del palo derecho se anula en coche. | ALTA |
| +2848-2904 / −o1852-1903 | `Process_Cam_On_A_String` | Tilt del heli / roll del coche: código vanilla que reaparece igual, solo reindentado dentro de `{ }`; los 49 borrados son ese movimiento. | BAJA |
| +6478-6490 / −o4782-5355 (574) | **`Process_FollowCar_SA` vaciada** [FREE_CAM] | Se borran 571 líneas de la cámara LCS/SA de reVC y la función delega en `Process_Cam_On_A_String`; se conserva la torreta Rhino/Firetruck. Solo corre con `bFreeCam`. | MEDIA |
| +6214 / −o4578 [FREE_CAM] | `Process_FollowPed_Rotation` | `BaseOffset` 0,75 → 0,4. | BAJA |
| +6309-6334 / −o4636-4653 [FREE_CAM] | `Process_FollowPed_Rotation` | Quita el `/* */` vanilla que desactivaba el ratón y le aplica la sensibilidad. | BAJA |
| +6349-6354 / −o4664 [FREE_CAM] | `Process_FollowPed_Rotation` | Clamp +45° → +60° (±50° apuntando). | BAJA |
| +1847-1848, 3312-3313, 3425-3426, 3884-3885, 4033-4034, 6330-6334 / − los pares vanilla | FollowPedWithMouse, Rocket, M16_1stPerson, 1rstPersonPedOnPC, Sniper, FollowPed_Rotation | El ratón y el palo se multiplican por `CCamera::ViceExtAimSens*()`. La función devuelve 1,0 con `VICEEXT_AIM_SENSITIVITY` apagada (Camera.cpp:163-200), así que no cambia nada; aun así la llamada está sin guarda y necesita el símbolo. | BAJA |
| +1822-1823, 1837 / −o1382, o1385 | `Process_FollowPedWithMouse` | `UseMouse`/`LookUpDown` se declaran fuera del bloque para que los use el recoil. | BAJA |
| +3940-3945 / −o2820-2823 | `Process_1rstPersonPedOnPC` | Usa `CamTargetEntity` en vez de `TheCamera.pTargetEntity` (crash con la 1ª persona del mod). Inocuo; se puede quedar. | BAJA |
| +11 | cabecera | `#include "PedArbiter.h"` sin guarda: hace falta por 4292. | BAJA |
| +713-843 / −o409-539 | `ProcessSpecialHeightRoutines` | **Solo cosmético**: se perdió la sangría (con `diff -w` no queda nada). Conviene restaurar el vanilla. | BAJA |
| +1495 / −o1107 | `Process_FollowPed` | `CPed*` → `CPlayerPed*`, sin efecto. | BAJA |
| −o2544, o2561, +6287 / −o4608-4610 | 1stPerson, FollowPed_Rotation | Blancos borrados o líneas fusionadas (cosmético). | BAJA |

Funciones vanilla de CCam que quedaron reescritas siempre:
- `Process_Cam_On_A_String`: cuerpo completo sustituido.
- `Process_FollowCar_SA`: vaciada.
- `Process_FollowPedWithMouse`: stick, clamp y hombro.
- `Process_FollowPed` y `Process_FollowPed_Rotation`: hombro, clamp y ratón.
- `CCam::Process`: MODE_AIMING activado.
- Process_Rocket, M16_1stPerson, 1rstPersonPedOnPC y Sniper: solo el multiplicador de sensibilidad, que vale 1,0.

Miembros nuevos (declarados en Camera.h, grupo g3): `Process_AimWeapon`, `Process_AimWeaponFovLerp`, `Process_AimWeaponCrouchOffset`, `Process_AvoidCollisions`. `Process_1rstPerson*` solo recibe el recoil (MOD-VIEJO) y el arreglo de crash.

Dependencia inversa: los bloques **guardados** de `VICEEXT_AIM_CLASSICAXIS` (1953-1957) llaman a `Process_AimWeaponCrouchOffset`, que está sin guarda. Quitar el bloque sin guarda sin quitar los guardados rompe la compilación.

#### (b) DIAGNOSTICO — 348 líneas

| Etiqueta | Líneas | Rangos | Contexto |
|---|---:|---|---|
| BETASNAP (helper + include ondemand.h + llamadas) | 33 | 26-51, 58, 555, 565, 571, 2780, 2791, 2798 (y una llamada embebida en 2787, clasificada SIN-GUARDA) | helper bajo `__EMSCRIPTEN__` con stub vacío; llamadas sin guarda |
| camauto2 / CAMB2 / CAMB2b | 46 | 2718-2763 | ley de coche, bajo `__EMSCRIPTEN__` |
| CAMB3 / CAMB3b / CAMSA / CAMV2 | 47 | 2906-2953 | ley de coche, bajo `__EMSCRIPTEN__` |
| AIMCAM / AIMVEC / AIMFOV / AIMCOL | 67 | 4570-4636 | ley de apuntado, bajo `__EMSCRIPTEN__` |
| statics solo-traza (`s_odLosD`, `s_odLosPed`, `s_odSphHit/Model/Ped/Own/D/Near`) | 10 | 68-73, 75-76, 4185-4186 | sin guarda. `s_odLosHit`, `s_odSphApp`, `s_odDistObs` y `s_odHidePeds` sí los usa la lógica (SIN-GUARDA). |
| RECOIL_QUEUE_OVERFLOW / RESET / CONTROL / CAMERA / PERSIST / APPLY | 98 | 104-107, 122-130, 158-183, 185-196, 198-236, 249-256 | dentro de `VICEEXT_RECOIL`; van por `CWeapon::ViceExtRecoilTrace/RecordControl` (Weapon.cpp) |
| SWIMCAM | 47 | 1242-1254, 1795-1811, 6243-6259 | **dentro de NADO** (`VICEEXT_SWIMMING` + `__EMSCRIPTEN__`) |

Ninguna traza P4 `kind=` vive en Cam.cpp.

#### (c) Banderas VICEEXT_* en Cam.cpp (MOD-VIEJO, 359 líneas)

- `VICEEXT_RECOIL` (186 sin contar las 98 de traza): cola y residual del retroceso (84-260: `ViceExtRecoilAlphaAdd`, `CWeapon::ViceExtRecoilBegin/Apply`), con llamadas `Begin/Apply` en FollowPed, FollowPedWithMouse, Rocket, M16, 1stPerson, 1rstPersonPedOnPC, AimWeapon, Syphon y FollowPed_Rotation.
- `VICEEXT_AIM_CLASSICAXIS` (90): vista pendiente al tomar o soltar la ley (1917-1927, 4544-4554); duckOffset a pie (1953-1957); clamp de agua (1959-1973); esconder peds a pie (2011-2036). Además `#ifndef` (2079-2096, 9 líneas) que quita el bloque vanilla que forzaba el rumbo del ped a la cámara.
- `VICEEXT_CROUCH` (30): baja el objetivo `VICEEXT_CROUCH_CAM_DROP*blend` en FollowPedWithMouse (1766-1784) y FollowPed_Rotation (6224-6234).
- `VICEEXT_FIRST_PERSON` (26+2): posición de la cabeza por IK en `Process_1rstPersonPedOnPC` (3791-3825).
- `VICEEXT_SHOTGUN_AIM` (11): las escopetas cuentan como "apuntar por encima del hombro" (665-675).
- `VICEEXT_PEDARBITER` (5): el hombro no se aplica si el carril NADO tiene APUNTAR o CAMARA (652-656).

#### (d) INFRA-WEB

Ninguna. Todo lo que está bajo `__EMSCRIPTEN__` en Cam.cpp es traza; el `#include "ondemand.h"` (26 y 58) solo está por `ODTRACES`.

#### (e) Cámara del NADO — lista cerrada en Cam.cpp

| Rango | Función | Qué hace | Guarda |
|---|---|---|---|
| 1180-1199 | `CCam::IsTargetInWater` | Comentario (sin guarda). | — |
| 1200-1258 | `CCam::IsTargetInWater` | El jugador vivo en el agua nunca entra en `MODE_PLAYER_FALLEN_WATER`: si `!DyingOrDead()` actualiza `m_vecLastAboveWaterCamPosition = Source` y devuelve false. Traza SWIMCAM en 1242-1254. | `VICEEXT_SWIMMING` |
| 1785-1813 | `Process_FollowPedWithMouse` | Nadando, `TargetCoors.z` sube como mínimo a nivel+0,5 (`CWaterLevel::GetWaterLevel`). Traza SWIMCAM en 1795-1811. | `VICEEXT_SWIMMING` |
| 6235-6261 | `Process_FollowPed_Rotation` [FREE_CAM] | Lo mismo para la cámara libre. Traza en 6243-6259. | `VICEEXT_SWIMMING` |

Total del nado en Cam.cpp: 135 líneas, de las que 88 son funcionales o comentario (NADO) y 47 son traza SWIMCAM.

Dependencias de la cámara de nado:
- **Duras:**
  - `CPlayerPed::ViceExtIsSwimming()` (PlayerPed.h:125, PlayerPed.cpp:4043, bajo `VICEEXT_SWIMMING`).
  - `CWaterLevel::GetWaterLevel` (vanilla).
  - `CPed::DyingOrDead`, `m_vecLastAboveWaterCamPosition` (vanilla).
  - Para la traza: `ondemand.h` (`ODTRACES`) y `CTimer`.
  - Los tres bloques están dentro de funciones vanilla (`IsTargetInWater`, `Process_FollowPedWithMouse`, `Process_FollowPed_Rotation`), **no** dentro de la ley ClassicAXIS ni de la de coche. Quitar esas leyes, la ley de coche o el recoil no los rompe.
  - Ojo con la posición: en FollowPedWithMouse y FollowPed_Rotation el bloque va justo después del de `VICEEXT_CROUCH` y antes de la línea sin guarda del hombro. Al limpiar hay que dejarlo en ese punto (antes del cálculo de Source).
- **Blanda (comportamiento):** el clamp de agua de ClassicAXIS (1959-1973, bajo `VICEEXT_AIM_CLASSICAXIS`): con `bTouchingWater`, Source no baja de nivel+0,6 y el near clip va a 0,2. Es lo que impide que la cámara se meta bajo el agua mientras se nada. Si se quita ClassicAXIS desaparece. **Hay que conservarlo o reimplementarlo como parte del nado.**
- **Ganchos de carril NADO en código viejo** (desaparecen con él; la ley nueva tiene que respetar el carril):
  - 652-656: hombro, bajo `VICEEXT_PEDARBITER`.
  - 4292-4298: FovLerp, sin guarda.

  Los dos piden "con NADO dueño de APUNTAR o CAMARA no se apunta". El nado no los necesita para funcionar, pero sin ellos la ley de apuntado vieja podría activarse nadando.
- `PedArbiter.h` se incluye sin guarda (línea 11) por el uso de 4292. Si se quita la ley de apuntado vieja, Cam.cpp ya no necesita PedArbiter salvo el bloque guardado 652-656.

### E.7 Grupo peds (resto), PedArbiter, Camera

Ficheros: peds/Ped.cpp, peds/Ped.h, peds/PedAI.cpp, peds/PedFight.cpp, peds/CopPed.cpp,
peds/Population.cpp, core/PedArbiter.cpp/.h (nuevos), core/Camera.cpp, core/Camera.h.
Filas: `rows_g3_peds_camera.tsv` (171 filas, validado con check.py: 0 sin clasificar).

#### Recuento (líneas añadidas / borradas de vanilla)

| fichero | INFRA-WEB | NADO | MOD-VIEJO | SIN-GUARDA | DIAGNOSTICO | borradas |
|---|---|---|---|---|---|---|
| peds/Ped.cpp | 0 | 22 | 217 | 91 | 310 | 19 (10 SIN-GUARDA, 9 NADO) |
| peds/Ped.h | 0 | 1 | 0 | 22 | 6 | 0 |
| peds/PedAI.cpp | 21 | 0 | 12 | 0 | 26 | 0 |
| peds/PedFight.cpp | 0 | 0 | 15 | 87 | 50 | 17 SIN-GUARDA |
| peds/CopPed.cpp | 0 | 0 | 42 | 1 | 52 | 2 SIN-GUARDA |
| peds/Population.cpp | 0 | 0 | 6 | 2 | 16 | 1 SIN-GUARDA |
| core/PedArbiter.h | 0 | 25 | 0 | 0 | 0 | — |
| core/PedArbiter.cpp | 0 | 99 | 0 | 0 | 24 | — |
| core/Camera.cpp | 0 | 0 | 175 | 170 | 191 | 10 SIN-GUARDA |
| core/Camera.h | 0 | 0 | 0 | 96 | 0 | 0 |
| **total (1779 + / 49 -)** | **21** | **147** | **467** | **469** | **675** | **40 SIN-GUARDA + 9 NADO** |

RADIO: nada en estos ficheros.

#### (a) SIN-GUARDA (código del mod viejo fuera de todo `#ifdef VICEEXT_*`)

##### Alta (cambian la conducta vanilla siempre o rompen el enlace si se quita la bandera)
| sitio | qué altera |
|---|---|
| peds/PedFight.cpp:207-222, :225 (borra vanilla o182-185, o189-193) | `CPed::SetPointGunAt`: se elimina la rama vanilla de pose `crouchfire` (`bCrouchWhenShooting && bIsDucking`). Ahora SIEMPRE `ANIM_WEAPON_FIRE`, para TODOS los peds (policías agachados de `DuckAndCover` incluidos) + `return` si el jugador está agachado con el agachado del port. |
| peds/PedFight.cpp:442-443, 558, 580, 638, 742-745, 863, 1095 (borra o399, o514, o536, o594, o686-687, o781, o980) | `bIsDucking`/`bCrouchWhenShooting` sustituidos por `ViceExtCrouchShooting()`. Cambia semántica vanilla en 558 y 1095 (vanilla sólo pedía `bIsDucking`; ahora exige también `bCrouchWhenShooting`). **`CPed::ViceExtCrouchShooting` y `CPlayerPed::ViceExtIsCrouched` sólo se definen bajo `VICEEXT_CROUCH` (PlayerPed.cpp:5083/5099)**: sin esa bandera PedFight.cpp no enlaza. |
| peds/PedFight.cpp:238-248 | `CPed::PointGunAt`: con el agachado del port sólo ajusta IK y sale (salta la lógica vanilla de pose). |
| peds/PedFight.cpp:957-967, 1011-1021 | `CPed::Attack`: si `CCamera::s_viceExtAimLawActive`, fuerza `MODE_FOLLOWPED` durante el disparo y luego restaura (ClassicAXIS C10). |
| peds/PedFight.cpp:3883-3894 (+ include :21) | `KillPedWithCar`: `ViceExtFeatures::On("PlayerDoesntBounceAwayFromMovingCar", true)` **sin `#ifdef VICEEXT_NO_CAR_BOUNCE`** → el jugador ya no sale despedido por el capó, siempre. `ViceExtFeatures::On` sólo existe con `VICEEXT_FEATURES_INI`. |
| peds/PedFight.cpp:2960-2967 | `InflictDamage`: casos de muerte para armas del mod (BERETTA, DESERT_EAGLE, SHOTGUN2, UZIOLD, AK47, M16, STEYR) — dependen de `WeaponType.h` ampliado. |
| core/Camera.cpp:4497-4518 (borra o3988-3992) | `Find3rdPersonQuickAimPitch` reemplazado por la fórmula ClassicAXIS (factor doble, /aspect, usa `Alpha` de la cámara): **cambia el resultado siempre**. |
| core/Camera.cpp:424-448 (borra o244-245) | `CCamera::Init`: `m_f3rdPersonCHairMultX/Y` salen de `s_viceExtAim` (mismos 0.53/0.4 por defecto, pero `re3.cpp` los puede cambiar por INI `[ClassicAxis]`) + reset de 12 estáticos de la ley. |
| core/Camera.cpp:2994-3011 | `StartTransition`: transición de 500 ms al entrar/salir de `MODE_AIMING`/`MODE_FOLLOWPED` si `s_viceExtAimSwitchSpeed` (lo pone PlayerPed.cpp sin guarda). |
| peds/Ped.cpp:4254-4268 (borra o3909; bajo `FREE_CAM` vanilla) | `CanStrafeOrMouseControl`: `bFreeCam` sólo corta si NO hay cámara de ratón 3ª persona. Afecta a Cam.cpp:2087/5380 (rumbo del ped = rumbo de cámara) y `ProcessAnimGroups`; el comentario dice que arregló "el nado que no va hacia donde se mira" → **dependencia blanda del NADO con ratón**. |

##### Media
| sitio | qué altera |
|---|---|
| peds/Ped.cpp:1497-1519 (borra o1450-1451) | `CalculateNewVelocity`: el corte vanilla de control de ratón se envuelve con `odCrouchMove/odAimMove` (falsos sin `VICEEXT_CROUCH` → equivalente a vanilla). |
| peds/Ped.cpp:1757, 1800-1808 (borra o1482) | `CalculateNewVelocity`: rama nueva "piernas" (`bDontAcceptIKLookAts`) si la ley ClassicAXIS es dueña del rumbo (sólo se activa con la bandera). |
| peds/Ped.cpp:4988, 5059, 5079 (borra o4629, o4700, o4720) | `GiveWeapon/GrantAmmo/SetAmmo`: `IsWeaponType()` en vez de `< WEAPONTYPE_TOTALWEAPONS` (armas del mod con IDs nuevos). |
| peds/Ped.cpp:5180-5216, 5220-5264 (borra o4749) + Ped.h:1005-1012 | drive-by: `CanDoDriveByWithCurrentWeapon`/`KeepsWeaponInHandWhileDriving` y reescritura de `RemoveWeaponWhenEnteringVehicle`/`ReplaceWeaponWhenExitingVehicle` (equivalen a vanilla sin `VICEEXT_DRIVEBY_WIDE`, pero la estructura vanilla está cambiada). Los llaman Automobile/Bike/Boat sin guarda. |
| peds/Ped.cpp:3619-3626 (borra o3281) | `PlayFootSteps`: salpicadura de charco con color de timecycle y mayor (0.22/6 en vez de 0.15/4/gris). Hace falta pasar un color porque el NADO cambió la firma de `particleProduceFootSplash`. |
| core/Camera.cpp:86-225 | definición de `s_viceExtAim`, `s_viceExtAimLawActive`, `s_viceExtSens`, `ViceExtAimSensFactor`, cuerpos de `ViceExtAimSensMouseX/Y/PadX/Y` (devuelven 1.0 sin `VICEEXT_AIM_SENSITIVITY`; Cam.cpp los llama en 23 sitios sin guarda), 12 estáticos de la ley, `ViceExtAimBetaFromFollowBeta`. |
| core/Camera.cpp:453-467, 540-542 (borra o297) | near-clip dual pie/coche (GeniusZ 1P; ambos `DEFAULT_NEAR` salvo que `re3.cpp` lea `FirstPerson.cfg` bajo `VICEEXT_FIRST_PERSON`). |
| core/Camera.h:227-245, 344-379, 594-634 (96 líneas) | ver tabla de usos abajo. |

##### Baja
| sitio | qué altera |
|---|---|
| peds/Ped.cpp:9 | `#include "PlayerPed.h"` |
| peds/Ped.cpp:58-59, Ped.h:1180-1182 | estáticos `bViceExtBigHeads/bViceExtRcRocket` (los usan Pad.cpp, Automobile.cpp; bandera `VICEEXT_CHEAT_EFFECTS`). |
| peds/Ped.h:1071-1081 | declaración `ViceExtCrouchShooting` (definida sólo con `VICEEXT_CROUCH`). |
| peds/Ped.cpp:1468 (borrado) | comentario vanilla. |
| peds/Population.cpp:432-436 (borra o416) | variable intermedia del multiplicador de spawn (equivale sin `VICEEXT_LIMITS`). |
| peds/CopPed.cpp:208 (borra o170-171) | cosmético: dos líneas vanilla fundidas en una. |
| core/Camera.cpp:4481-4482 (borra o3972-3973) | cosmético: sangría. |
| core/Camera.cpp:36 | `#include "PedArbiter.h"` (carril LEY de la 1ª persona). |

##### Camera.h — los 96 líneas sin guarda: quién usa cada miembro
| miembro (Camera.h) | usuarios |
|---|---|
| `Process_AimWeapon`, `Process_AimWeaponFovLerp`, `Process_AimWeaponCrouchOffset` (227-240) | Cam.cpp (ley ClassicAXIS, casi todo sin guarda). `Process_AimWeaponFovLerp` (Cam.cpp:4292) contiene una consulta al carril NADO (resetea FOV si el nado apunta). |
| `Process_AvoidCollisions` (241-245) | Cam.cpp: ley ClassicAXIS **y la ley de coche** (Cam.cpp:62, 2010, 2354, 2606, 2837...). |
| `CAimClassicAxisSettings` / `s_viceExtAim` | Camera.cpp, re3.cpp (INI, bajo `VICEEXT_AIM_CLASSICAXIS`), Cam.cpp (3, sin guarda), PlayerPed.cpp (3, sin guarda), Hud.cpp (7, CLASSICAXIS). |
| `s_viceExtAimLawActive` | PedFight.cpp (6, sin guarda), PlayerPed.cpp (17: 13 sin guarda, 2 CROUCH, **2 dentro de VICEEXT_SWIMMING pero sólo en trazas SWIMFOV/SWIM — PlayerPed.cpp:3990, 4019**), Hud.cpp (5), Cam.cpp (1). |
| `CAimSensSettings` / `s_viceExtSens` / `ViceExtAimSens*()` | Camera.cpp, re3.cpp (INI `[Controller]`, `VICEEXT_AIM_SENSITIVITY`); Cam.cpp llama `ViceExtAimSens*` en 23 sitios sin guarda. |
| `s_viceExtAimSwitchSpeed/PrevCamMode/PrevHor/PrevVer/MouseTarget/LockOnUntil/LastLockPos/LastLockCol/ForceRealMoveAnim/ViewDir/ViewPending/ViewMode`, `ViceExtAimBetaFromFollowBeta` | PlayerPed.cpp (mayoría sin guarda), Cam.cpp, Hud.cpp (CLASSICAXIS). |
| `s_viceExt1PNearClipOnFoot/InCar/MouseSens/VehicleOffset` | Camera.cpp (near-clip), re3.cpp (`VICEEXT_FIRST_PERSON`). `s_viceExt1PVehicleOffset`/`MouseSens` no los lee nadie más. |
| `m_bViceExt1stPersonView` | Camera.cpp y MenuScreensCustom.cpp, todo bajo `VICEEXT_FIRST_PERSON` (declarado sin guarda, definido sólo con la bandera). |

**¿Los usa el nado?** El código funcional del nado NO usa ningún miembro `s_viceExt*` de CCamera:
sólo las trazas de nado (`SWIMFOV`, línea de estado `SWIM` en PlayerPed.cpp:3984-3994 y 4014-4022)
leen `CCamera::s_viceExtAimLawActive`. Al revés sí hay acoplamiento: la ley ClassicAXIS (Cam.cpp
`ViceExtAimingOverShoulder` :653 bajo `VICEEXT_PEDARBITER`, `Process_AimWeaponFovLerp` :4292 sin
guarda; PlayerPed.cpp:6047 sin guarda; Hud.cpp:455 sin guarda) pregunta al árbitro por el carril NADO
para apagarse mientras se nada. Al quitar ClassicAXIS esas consultas desaparecen con ella sin romper
el nado.

#### (b) DIAGNOSTICO (675 líneas en estos ficheros)

| etiqueta | líneas | sitios |
|---|---|---|
| CAM1P / `VICEEXT 1p bind` / `1p key` / `1p set` | 133 | Camera.cpp:1265-1370 (sin guarda, con líneas V que sólo alimentan la traza), 1443-1449, 1486-1505 |
| DRIVEBY state/shot/enter/exit | 98 | Ped.cpp:5105-5179 (`DriveByTraceTag` con `EM_ASM_INT` **sin guarda `__EMSCRIPTEN__`**), 5222-5272 (outcome), Ped.h:1013-1018 |
| P4 kind=move_apply | 54 | Ped.cpp:1552-1578, 1718-1744 |
| P4 kind=firewait | 49 | PedFight.cpp:729-740, 773-784, 848-859, 918-930 |
| P4 kind=jump / jump_end | 74 | Ped.cpp:5849-5885, 5919-5923, 9740-9766, 9798-9802 |
| P4 kind=legs | 38 | Ped.cpp:1762-1799 |
| WANTEDCOP arrest-begin / join / search-last-known | 39 | CopPed.cpp:176-182, 265-291 (sin guarda), 486-490 (HIDE_COPS) |
| P4 kind=aimsens | 26 | Camera.cpp:118-141, 168-169 |
| AIMCFG | 25 | Camera.cpp:480-504 |
| P4 kind=ini (trucos y cop cover) | 47 | Ped.cpp:72-93, 5445; PedAI.cpp:5661-5672, 5764-5775 |
| P4 kind=gangaim | 23 | Ped.cpp:385-395, CopPed.cpp:98-109 |
| PEDCLAIM (dentro del árbitro del nado) | 23 | PedArbiter.cpp:47-53, 57-64, 84-91 |
| ODCROUCHDIR | 17 | Ped.cpp:1685-1701 |
| P4 kind=limits | 13 | Population.cpp:366-378 |
| `[od] splines` printf (web) | 6 | Camera.cpp:4143-4148 |
| ODANIMFAIL (web) | 1 | PedAI.cpp:4905 |
| `#include "ondemand.h"` sólo para trazas | 9 | Ped.cpp:11, PedAI.cpp:2, PedFight.cpp:29, CopPed.cpp:22, Population.cpp:31-33, PedArbiter.cpp:4, Camera.cpp:37 |

`WantedTraceTag()` (CopPed) vive en otro grupo (Wanted.*).

#### (c) Banderas VICEEXT_* en estos ficheros

| bandera | qué hace aquí |
|---|---|
| VICEEXT_SWIMMING | Ped.cpp `ProcessBuoyancy`: frena la caída del jugador en el agua (S 1984-1986, 2013-2016); el resto del nado aquí va SIN guarda (ver NADO). |
| VICEEXT_PEDARBITER | PedArbiter.cpp: estado y lógica real del árbitro (sin ella `Claim` devuelve false y `Owner` siempre MOTOR). |
| VICEEXT_CROUCH | Ped.cpp `CalculateNewVelocity`: velocidad/rumbo del agachado a pie desde el mando (1506-1515, 1589-1747). |
| VICEEXT_AIM_CLASSICAXIS | Ped.cpp: movimiento con la ley (`ViceExtGetMove`, 1536-1584), salto con rumbo del mando (9732-9739, 9823-9869); PedFight.cpp: C18-1 (apuntar agachado sin CROUCHFIRE → RestorePreviousState); Camera.cpp: anula la cámara de pelea (`#ifndef`, 1549-1574). |
| VICEEXT_AIM_SENSITIVITY | Camera.cpp: factores aim/look de ratón/pad (153-203). |
| VICEEXT_FIRST_PERSON | Camera.cpp: conmutador 1ª persona con V (231-264, 971-976, 1405-1506, 2646-2652), usa carril LEY del árbitro. |
| VICEEXT_CHEAT_EFFECTS | Ped.cpp: escala de cabeza BIGHEADS en `PreRender` (69-94, 5438-5447). |
| VICEEXT_GANG_AIM | Ped.cpp/CopPed.cpp: precisión de peds/policías (73, 84, 80, 78, 86). |
| VICEEXT_DRIVEBY_WIDE | Ped.cpp: pistola válida en drive-by y se queda en la mano (5190-5196, 5207-5213). |
| VICEEXT_HIDE_COPS | CopPed.cpp `CopAI`: si te escondes, los policías a pie van a la última posición conocida (476-503). |
| VICEEXT_COP_COVER | PedAI.cpp `DuckAndCover`: el policía se agacha antes de cubrirse (5658-5673, 5761-5776). |
| VICEEXT_BIKE_EXIT_SIDE | PedAI.cpp `SetExitCar`: salir de la moto por la derecha según el giro (4187-4190). |
| VICEEXT_LIMITS (+VICEEXT_SPAWN_RADIUS_MULT) | Population.cpp: radio de creación de peds ×1.5 (433-435). |
| VICEEXT_POLICE_BIKE | Population.cpp `AddPedInCar`: `MI_VEEXT_POLWINTERG` lleva policía (842-844). |

#### (d) INFRA-WEB
| sitio | qué es |
|---|---|
| peds/PedAI.cpp:4889-4911 | `SetAnimOffsetForEnterOrExitVehicle`: `LoadAllRequestedModels` con `CStreaming::gWebLoadBudget = 0` y salida segura si faltan bloques de anim (arreglo del OOB del init del port). Contiene 1 línea de traza (ODANIMFAIL, 4905). Depende de `CStreaming::gWebLoadBudget` (Streaming, otro grupo). |

#### (e) NADO en estos ficheros

| sitio | guarda | qué hace |
|---|---|---|
| peds/Ped.cpp:10 | sin guarda | `#include "PedArbiter.h"` |
| peds/Ped.cpp:1983-2030 (borra o1658, o1682-1683, o1690, o1692) | S 1984-1986, 2013-2016; resto **sin guarda** | `ProcessBuoyancy`: `odSwimmingPlayer = IsPlayer() && ViceExtPedOwns(PEDLANE_NADO, PEDCAP_MOVIMIENTO)` anula el impulso de flotación (ApplyMoveForce), el bloque de ahogo (bIsDrowning + daño WEAPONTYPE_DROWNING) y el de freno/salpicadura de entrada; amortigua la caída vertical (S). |
| peds/Ped.cpp:2903 (borra o2565) | sin guarda | `ProcessControl`: no entra en "en el aire" (`CheckIfInTheAir`) mientras el carril NADO tiene el movimiento. |
| peds/Ped.cpp:3437-3446 (borra o3099-3100, o3108) + Ped.h:1221 | sin guarda | `particleProduceFootSplash` pasa de `static` a extern y recibe `CRGBA color`; lo usa la brazada del nado (PlayerPed.cpp:3921, código de nado sin guarda). |
| core/PedArbiter.h / .cpp (148 líneas, 24 de traza PEDCLAIM) | `VICEEXT_PEDARBITER` | carril `PEDLANE_NADO` (ver deps_pedarbiter.md). |

Dependencias del NADO de estos ficheros hacia código MOD-VIEJO:
- **PedArbiter** (`VICEEXT_PEDARBITER`) → sin él `ViceExtPedClaim` devuelve false y todas las consultas de nado quedan en falso.
- **ViceExtFeatures** (`VICEEXT_FEATURES_INI`): `ViceExtPedClaim(PEDLANE_NADO, …)` llama `ViceExtFeatures::On("EnableSwimming", true)`; `On` sólo se compila con `VICEEXT_FEATURES_INI` → si se quita esa bandera con PEDARBITER puesto, no enlaza.
- Los claims/releases los hace PlayerPed.cpp:3704-3707/3773-3776 (VICEEXT_SWIMMING) y el reset `ViceExtPedResetAll()` está en Game.cpp:989 (sin guarda, `CGame::InitialiseWhenRestarting`).
- Blanda: Ped.cpp:4254-4268 (`CanStrafeOrMouseControl` con cámara de ratón, SIN-GUARDA) — según su comentario es parte de que el nado vaya hacia donde mira la cámara con ratón.
- Las trazas del nado (PlayerPed.cpp:3990, 4019) leen `CCamera::s_viceExtAimLawActive` (Camera.h sin guarda / ClassicAXIS): si se quita ClassicAXIS hay que retocar esas dos trazas.

### E.8 Grupo infraestructura web (glfw, main, CdStream, Streaming, Game, config.h, re3)

Ficheros: skel/glfw/glfw.cpp, skel/skeleton.cpp, skel/ondemand.cpp, skel/ondemand.h, core/main.cpp, core/main.h, core/CdStreamPosix.cpp, core/Streaming.cpp, core/Streaming.h, core/Game.cpp, core/Game.h, core/FileLoader.cpp, core/config.h, core/re3.cpp, CMakeLists.txt.
Filas: `rows_g4a_web_core.tsv` (validado con check.py: 0 sin clasificar).

#### Recuento del grupo (líneas añadidas / borradas)

| Categoría | + | − |
|---|---:|---:|
| INFRA-WEB | 2237 | 567 |
| DIAGNOSTICO | 627 | 0 |
| MOD-VIEJO | 212 | 0 |
| SIN-GUARDA | 98 | 11 |
| NADO | 1 | 0 |
| RADIO | 1 | 0 |
| **Total** | **3176** | **578** |

Por fichero (+): glfw.cpp INFRA 769 / DIAG 67 (−385 INFRA); Game.cpp INFRA 529 / DIAG 62 / MOD 6 / SIN 3 (−168 INFRA); Streaming.cpp INFRA 245 / DIAG 192 / SIN 4 (−9 INFRA); main.cpp INFRA 217 / DIAG 191 / SIN 27 (−4 INFRA, −1 SIN); CdStreamPosix INFRA 200 / DIAG 101; config.h MOD 139 / SIN 35 / INFRA 14 / NADO 1 / RADIO 1 (−9 SIN); re3.cpp MOD 67 / INFRA 22 / SIN 14 (−1 INFRA, −1 SIN); ondemand.cpp INFRA 129; ondemand.h INFRA 23 / DIAG 8; skeleton.cpp INFRA 34; main.h INFRA 19 / DIAG 2; FileLoader SIN 15 / DIAG 4; CMakeLists INFRA 14; Streaming.h INFRA 14; Game.h INFRA 8.

#### (a) SIN-GUARDA (código del mod viejo sin `#ifdef VICEEXT_*`)

| Importancia | Sitio | Qué altera |
|---|---|---|
| ALTA | core/config.h:12-15 (−o12) | `MAX_CDIMAGES` 8→12 (las 8 IMG propias de Vice Extended). Dimensiona también `gOdLoose[]`/`gOdIsLoose[]` de CdStreamPosix. |
| ALTA | core/config.h:18-32 (−o15-17) | `MODELINFOSIZE` 6500→6700, `TXDSTORESIZE` 1385→1500, `COLSTORESIZE` 31→48 (ids y TXD/COL del mod). |
| ALTA | core/config.h:39-44 (−o24, o26) | `WEAPONMODELSIZE` 37→47, `VEHICLEMODELSIZE` 110→130. |
| ALTA | core/config.h:72-79 (−o46-47) | `NUMANIMBLOCKS` 35→40, `NUMANIMATIONS` 450→512 (38 clips nuevos del ped.ifp del mod, incluidos los `swim_*`). |
| ALTA | core/Game.cpp:989 | `ViceExtPedResetAll();` (PedArbiter) llamado en `InitialiseWhenRestarting` sin guarda: el árbitro se ejecuta siempre en cada carga/reinicio aunque se quite `VICEEXT_PEDARBITER`. |
| MEDIA | core/FileLoader.cpp:1203-1217 | `LoadObjectInstance`: un `inst` con id −1 se resuelve por nombre (lo necesita el `maps/bryx/bryx.ipl` del mod; sin él, crash `null function`). Es inocuo con el mundo vanilla. |
| MEDIA | core/re3.cpp:333-334, 338 (−o325) | Añade la acción `"PED_WALK"` (ClassicAXIS C17 `WalkKey`) al array de nombres de acciones del INI de controles; depende del enum `PED_WALK` de ControllerConfig.h (también sin guarda, otro grupo). |
| MEDIA | core/Streaming.cpp:1773-1776 | `LoadInitialWeapons`: `RequestModel(MI_GR_GRENADE, DONT_REMOVE)` — proyectil del lanzagranadas del mod residente (MI_GR_GRENADE lo añade ModelIndices.h). |
| BAJA | core/main.cpp:661-683, 746-749 (−o688) | SilentPatch :723 (contorno de la barra de carga escalado). Macros `VICEEXT_LOADBAR_BORDER_X/Y` con prefijo VICEEXT pero sin guarda; también las usa `WebDrawLoadScreen` (main.cpp:1033-1034, INFRA-WEB). |
| BAJA | core/config.h:535-537 (−o370) | `MUCH_SHORTER_OUTRO_SCREEN` comentado (outro de SilentPatch ≈2,5 s). |
| BAJA | core/re3.cpp:1063-1073 | DEBUGMENU: entrada "Spawn ViceEx Car" (MI_VEEXT_FIRST/LAST_VEHICLE). Solo compila con DEBUGMENU. |
| BAJA | core/Game.cpp:2-3 | `#include "PedArbiter.h"` / `"ViceExtFeatures.h"` sin guarda (necesarios por la línea 989 y por el bloque VICEEXT_FEATURES_INI). |

Nota: `#define VICEEXT_LIMITS` (config.h:51) y `VICEEXT_SPAWN_RADIUS_MULT 1.5f` (:52) se han dejado como MOD-VIEJO `VICEEXT_LIMITS` (es una guarda real); forman parte del grupo «límites del mod»: `NUMVEHICLES` 110→130 (config.h:57-63) y el multiplicador de radio de spawn/despawn (lo usan CarCtrl.cpp:310/613/1130 y Population.cpp:366/433, todos bajo `#ifdef VICEEXT_LIMITS`).

#### (b) DIAGNOSTICO

**ondemand.h**: `ODTRACES(msg)`/`ODTRACEI(tag,v)` (ondemand.h:8-9, stubs nativos :27-28) encolan texto en `window.__odq`; `web/lib/index.js` lo manda por lotes a `OD.cfg.odtraceUrl` (odtrace.log). Es diagnóstico puro. `odBlockingPush/Pop` (ondemand.h:10-26, ondemand.cpp:33-46) **es INFRA-WEB**: publica `window.__vcODBlock` para que la capa on-demand NO aplace ficheros mientras el motor está en una carga bloqueante (lo usa `CStreaming::LoadAllRequestedModels`, Streaming.cpp:2703/2805). Quitarlo rompe cinemáticas y cargas (escena vacía).

Etiquetas de traza (líneas del grupo, 627 en total):

| Etiqueta | Líneas | Sitios |
|---|---:|---|
| FPHASE (cronometraje por fases del frame) | 158 | main.cpp:795-912 (WebPhase*), llamadas en main.cpp:1885-2087 (salvo 2064) |
| ODSHORT / ODSRECOVER / loosemiss | 97 | CdStreamPosix.cpp:123-199, 238-239, 254-256, 259-261, 279-290 |
| WEBHB + censo OD (latido cada 120 ticks) | 42 | glfw.cpp:2616-2657 |
| CARPED/CARPOOL, CARLOAD, CARFAIL, CARFULL, CARBLOCK (D4 tráfico) | 94 | Streaming.cpp:1914-1939, 1696-1717, 2419-2437, 1949-1962, 369-381 |
| TXDIN, FAIL model/txd/col | 58 | Streaming.cpp:679-702, 649-660, 714-724, 739-749 |
| ODLOAD big, CDRETRY | 23 | Streaming.cpp:2722-2725, 2778-2795, 2749 |
| FPSLOG / FPSLOG strm (gWebStrmMs) | 22 | main.cpp:951-957, main.h:62-63, Streaming.cpp:389-398, 3450-3453 |
| webload (carga de partida) | 23 | Game.cpp:1070, 1128-1130, 1141-1142, 1190-1206 |
| P4 kind=worlddata (bajo VICEEXT_MAPDATA) | 17 | Game.cpp:737-753 — único P4 de este grupo; es TODO el contenido de `VICEEXT_MAPDATA` aquí |
| ODMARK (macro no-op) | 16 | Game.cpp:1260-1266 + marcas en 1269…1653 |
| LBAR, LOADSCR | 23 | main.cpp:1035-1048, 1004-1010, 1072, 1081 |
| loadtick, WR wipe | 22 | glfw.cpp:2246, 2359, 2367, 2451, 2457, 2470-2475, 2521-2531 |
| initstep, loadlevel, addimage, traza retirada | 15 | Game.cpp:409-413, FileLoader.cpp:53-55, CdStreamPosix.cpp:854-856, Streaming.cpp:767-770 |
| includes de ondemand.h solo para trazas + macros | 17 | glfw.cpp:30-32, main.cpp:3-5, CdStreamPosix.cpp:26, FileLoader.cpp:3, Game.cpp:5, ondemand.h:1-4/8-9/27-28 |

Los `printf("[web] …")` de arranque (glfw.cpp, skeleton.cpp, Game.cpp:383-385) se han dejado como INFRA-WEB (log de consola del port, no instrumentación de los agentes).

#### (c) Banderas VICEEXT_* en estos ficheros

- **config.h:386-518**: definición de TODAS las banderas del bloque features.ini (71 filas en el TSV, una por bandera). Con valor: `VICEEXT_SPAWN_RADIUS_MULT 1.5f` (:52), `VICEEXT_CROUCH_CAM_DROP 0.55f` (:424), `VICEEXT_AIM_BLADE 0.70f` (:451), `VICEEXT_LOD_EXTRA 20.0f` (:466), `VICEEXT_GPS_COLOR_R/G/B 255/0/255` (:500-502). Sin valor (activas): LIMITS(:51), SWIMMING(:390, NADO), RECOIL, NO_CAR_BOUNCE, SPRINT_HEAVY, POLICE_BIKE_LIGHTS, HIDE_COPS, FIRST_PERSON, DRIVEBY_WIDE, WEAPON_SIGHTS, HINT_KEYS, AUTOSAVE, SAVE_ANYWHERE, MANUAL_RELOAD, GAS_TANK, SHOTGUN_AIM, CROUCH, CLIMB, AIM_WALK, ROCKET_3RD_PERSON, AIM_CLASSICAXIS, BREAKABLE_LIGHTS, TURN_SIGNALS (¡el comentario dice «APAGADO por defecto» pero está definido!), BIKE_EXIT_SIDE, FIX_SILENTPATCH, FIX_WFP, FIX_FV, FIX_FV_ROTOR, POLICE_BIKE, NO_WHEEL_PIERCE, SKIP_PHONE_CALL, MOD_SCRIPT, PARTICLE_NAMES, MONEY_NO_ZEROS, PEDARBITER, RADIO_ICONS(:473, RADIO), EXTRA_CHEATS, CAR_DEATH, CAR_LIGHTS, ALT_SIT, GANG_AIM, MOON, MAPDATA, POLISH, WHEEL_DETACH, FEATURES_INI, NO_FLIP_BURN, HEALTH_REGEN, HIDE_STARS, NO_JUMP_FROM_CAR, STD_TURN_SIGNALS, TANK_FIRE, GPS_COLOR, CAR_ROTATE_WHEEL, CAR_WHEEL_KEEP, CAR_FOUR_COLORS, GAS_STATION_BLOW, PROJECTILE_BLOW, DISTANT_LIGHTS, COP_COVER, HELI_HEALTH, AIM_SENSITIVITY, CHEAT_EFFECTS, PHOTO_SAVE.
- **VICEEXT_LIMITS** (config.h:51-63): NUMVEHICLES 110→130 (NUMPEDS igual, 140) y habilita el multiplicador de radio de spawn.
- **VICEEXT_FEATURES_INI** (Game.cpp:187-192): `ViceExtFeatures::Load()` en `InitialiseOnceBeforeRW` (lee el features.ini del mod una vez).
- **VICEEXT_MAPDATA** (Game.cpp:737-753): solo la traza P4 worlddata (cuenta zonas de oclusión con `CCullZones::ViceExtNumZones()`); reclasificado a DIAGNOSTICO.
- **VICEEXT_SKIP_PHONE_CALL** (re3.cpp:335-337): nombre de acción `"SKIP_PHONE_CALL"` en el INI de controles.
- **VICEEXT_AIM_CLASSICAXIS** (re3.cpp:511-526, 670-683): lee/escribe la sección `[ClassicAxis]` del INI en `CCamera::s_viceExtAim`.
- **VICEEXT_AIM_SENSITIVITY** (re3.cpp:527-543, 658-669): lee/escribe `[Controller]` (8 claves de sensibilidad) en `CCamera::s_viceExtSens`.
- **VICEEXT_FIRST_PERSON** (re3.cpp:544-548): lee `[FirstPerson]` (near clip a pie/en coche, sensibilidad de ratón).

#### (d) INFRA-WEB por subsistema

- **Bucle principal / glfw web** (glfw.cpp): `main()` troceado en OuterSetup/InnerShouldRun/InnerFrame/AfterInner/FinalCleanup + `EmscriptenTick` con `emscripten_set_main_loop` (glfw.cpp:2011-2681, 2897-2910; −o2081-2466); arranque diferido de carga con splash (2238-2253, 2340-2396, 2439-2500); publica `window.__vcState/__vcInGame/__vcFrame`; tope de fps 35/120 sustituye al limitador vanilla (2307-2318, afecta también al nativo RW_GL3); reloj CLOCK_MONOTONIC (280-285); canvas/modos de vídeo (431-450, 719-732, 905-1011, 1232-1240); joystick sin mappings (1065-1125, 2951-2957); filtro de auto-repeat (1697-1708); logs `[web]`.
- **On-demand** (ondemand.cpp:1-129, ondemand.h): `__wrap_fopen/open/stat/access` + `od_fetch` (EM_ASYNC_JS, Asyncify) + blindaje de directorios; `odBlockingPush/Pop`.
- **CdStream** (CdStreamPosix.cpp): modo monohilo sin semáforos (320-542), imágenes como directorio suelto con `.dir` (111-122, 200-300, 770-847, 887-894).
- **Streaming web** (Streaming.cpp): cargador de tráfico sin la puerta `nreq<5` y con temporizador en tiempo real (346-387, 1879-1893, 1943-1945); enfriamiento de re-solicitud (553-570 y usos); reintentos acotados y do/while de CdStreamSync corregido (2653-2659, 2735-2761); presupuesto de ficheros por llamada `gWebLoadBudget` (2711-2720, 3442-3449); `LoadSceneStep` troceado (3454-3572); Streaming.h:212-225.
- **Carga troceada / pantalla de carga** (Game.cpp, main.cpp, main.h, Game.h): `CGame::Initialise` en 19 pasos (`InitialiseStep`, Game.cpp:397-736, −o366-587; también lo usa el nativo), `ShutDownForRestartStep` (934-986), `InitialiseRestartStep` (1064-1259), `WEB_LOAD_PROGRESS/OVERLAY` (108-120); `WebDrawLoadScreen/Begin/End`, `gWebLoadScreenActive`, splash fijo, `InitialiseGameStep` (main.cpp:116-128, 579-586, 700-710, 790-794, 958-1084, 2117-2160).
- **Contador FPS** por API de página (`__vcShowFps`): main.cpp:913-950, 958-976, 2064, 2118.
- **IDBFS / userfiles**: glfw.cpp:185-191 (/userfiles), re3.cpp:197-205 (reVC.ini), skeleton.cpp:221-237 (salir = sync + reload).
- **Audio web** (config.h:615-627): sin PS2_AUDIO_PATHS y sin MULTITHREADED_AUDIO; **config web** (667-675): USE_UNNAMED_SEM, sin USE_TXD_CDIMAGE.
- **Arreglos de crash**: asserts acotados a 30 (re3.cpp:1323-1338).
- **Build**: CMakeLists.txt:55-68 (stamp de bootseed como LINK_DEPENDS).
- Whitespace sin efecto: main.cpp:2400-2404 (rama GTA_PS2), Game.cpp:1057.
- Observación: Streaming.cpp incluye ondemand.h solo bajo `__EMSCRIPTEN__` pero llama `odBlockingPush/Pop` sin guarda (2703/2805): el nativo no compilaría.

#### (e) Carga de assets del mod de la que dependen NADO / RADIO

- **NADO**: los clips `swim_tread/crawl/breast/jumpout` vienen del ped.ifp del mod (grupo `playerswim` en AnimManager.cpp:940-969/1117, AnimationId.h:292-295 — otro grupo). Necesitan `NUMANIMATIONS=512` (config.h:79; el comentario cuenta "38 nuevas del ped.ifp del mod"): **si se revierte el límite a 450, el ped.ifp del mod no cabe y el nado se queda sin clips**. `NUMANIMBLOCKS=40` es por deagle/steyr/rocket (no es del nado). La bandera `VICEEXT_SWIMMING` se define en config.h:390.
- **RADIO**: `MusicManager.cpp:1391-1394` crea un slot TXD `"radio"` y carga `MODELS/RADIO.TXD` (`AddTxdSlot`). Con el gta3.dir del port (1389 TXD tras los packs 3/4 del mod) hace falta `TXDSTORESIZE` > 1389 (config.h:27, ahora 1500); si se quitan las IMG/TXD del mod y se vuelve a 1385 hay que comprobar que queda hueco para el slot `radio`. La bandera `VICEEXT_RADIO_ICONS` se define en config.h:473.
- Las IMG del mod se registran por datos (`IMG` en gta_vc.dat → FileLoader.cpp:125 `CdStreamAddImage`): requieren `MAX_CDIMAGES=12` (config.h:15) y el backend suelto de CdStreamPosix. El mundo del mod (bryx.ipl) requiere el arreglo de FileLoader.cpp:1203-1217. `MODELINFOSIZE/WEAPONMODELSIZE/VEHICLEMODELSIZE/COLSTORESIZE` son para coches/armas/colisiones del mod (no los usan nado ni radio).

### E.9 Grupo guardado, texto, animación, render

Ficheros: save/GenericGameStorage.cpp/.h, save/PCSave.cpp, rw/TexRead.cpp, fakerw/fake.cpp,
text/Text.cpp/.h, renderer/Renderer.cpp, renderer/Coronas.cpp, renderer/WaterLevel.cpp,
animation/AnimManager.cpp/.h, animation/AnimBlendAssocGroup.cpp, animation/AnimationId.h,
animation/CutsceneMgr.cpp/.h. TSV: `rows_g4b_misc_infra.tsv` (check.py sin huecos).

Totales del grupo (líneas añadidas / borradas):

| Categoría | + | - |
|---|---:|---:|
| INFRA-WEB | 321 | 120 |
| DIAGNOSTICO | 334 | 1 |
| SIN-GUARDA | 176 | 7 |
| NADO | 51 | 0 |
| MOD-VIEJO | 46 | 0 |
| RADIO | 0 | 0 |
| **Total** | **928** | **128** |

Por fichero (+): AnimManager.cpp SG116/NADO39/WEB36/DIAG19 (-3 SG); AnimationId.h SG34/NADO9;
AnimManager.h SG12/NADO3; Text.cpp WEB134/DIAG48 (-105 WEB); CutsceneMgr.cpp DIAG59/WEB8;
Coronas.cpp DIAG53/SG2; Renderer.cpp DIAG44; GenericGameStorage.cpp WEB51/MOD35/DIAG27/SG5;
PCSave.cpp DIAG26/WEB20; TexRead.cpp DIAG31/WEB17; fake.cpp DIAG24/WEB2; CutsceneMgr.h WEB19;
AnimBlendAssocGroup.cpp WEB15; WaterLevel.cpp WEB11/DIAG3; GenericGameStorage.h MOD11/SG7/WEB5; Text.h WEB3.

#### (a) SIN-GUARDA (mod viejo fuera de cualquier `#ifdef VICEEXT_*`)

| Importancia | Sitio | Qué altera del vanilla | Feature |
|---|---|---|---|
| ALTA | animation/AnimationId.h:296-306 | 7 IDs `ANIM_STD_CLIMB_*` añadidos al enum | VICEEXT_CLIMB |
| ALTA | animation/AnimationId.h:307-329 | 9 IDs `ANIM_STD_CROUCH_*` (5 Crouch + 4 GunMove) | VICEEXT_CROUCH |
| ALTA | animation/AnimManager.h:72-78 | `ASSOCGRP_DEAGLE/STEYR/ROCKET` (están ANTES de `ASSOCGRP_PLAYERSWIM`) | armas del mod (anims.img) |
| ALTA | animation/AnimManager.h:82, 83-86 | `ASSOCGRP_PLAYERCROUCH`, `ASSOCGRP_PLAYERCLIMB` | VICEEXT_CROUCH / VICEEXT_CLIMB |
| ALTA | animation/AnimManager.cpp:733-737, 751-753, 767-769 (borra o708, o722, o736) | Hueco `ANIM_STD_RUNFAST` de los grupos rocket/armed/chainsaw pasa de `run_*` a `sprint_*` (clips que sólo trae el ped.ifp del mod). Cambia el vanilla SIEMPRE, aunque se quite VICEEXT_SPRINT_HEAVY; con el ped.ifp de serie el clip no existe | VICEEXT_SPRINT_HEAVY |
| ALTA | animation/AnimManager.cpp:628-631 | `aBuddyAnimations` pasa de 2 a 4 clips (`buddy_reload`, `buddy_crouchreload`): el grupo vanilla "buddy" crece (usa `aWeaponAnimDescs`) | armas del mod |
| MEDIA | animation/AnimManager.cpp:667-686, 1111-1114 | arrays `aDeagle/aSteyr/aRocketAnimations` + 3 filas en la tabla de grupos (mismo orden que el enum) | armas del mod |
| MEDIA | animation/AnimManager.cpp:971-1024, 1118 | `aCrouchAnimations` + `aCrouchAnimDescs` + grupo "playercrouch" | VICEEXT_CROUCH |
| MEDIA | animation/AnimManager.cpp:1025-1045, 1119 | `aClimbAnimations` + `aClimbAnimDescs` + grupo "playerclimb" | VICEEXT_CLIMB |
| MEDIA | save/GenericGameStorage.h:6-8 (borra o6) | `SLOT_COUNT` 8 -> 9 (ranura 9 = autosave/quicksave, dimensiona las matrices de ranuras y los bucles de PCSave.cpp:140/145) | VICEEXT_AUTOSAVE |
| MEDIA | save/GenericGameStorage.cpp:89-93 (borra o51) | `PAUSE_SAVE_SLOT = SLOT_COUNT - 1` (bajo MISSION_REPLAY, macro vanilla activa) | VICEEXT_AUTOSAVE |
| BAJA | save/GenericGameStorage.h:72-75 | `extern const int PAUSE_SAVE_SLOT` (lo usan Frontend.cpp:5590 y Script.cpp:1630) | VICEEXT_AUTOSAVE |
| BAJA | renderer/Coronas.cpp:418-419 (borra o382-383) | flares de corona escalados con `SCREEN_STRETCH_X/Y` (SilentPatch :1931, plan 2.0 agente B) | VICEEXT_FIX_SILENTPATCH |

#### (b) DIAGNOSTICO (334 líneas + 1 borrada)

Todas tras `#ifdef __EMSCRIPTEN__` salvo los `#include "ondemand.h"` sueltos y los contadores de Coronas.

| Etiqueta(s) | Sitio | Líneas |
|---|---|---:|
| `CUT begin/cutsimg-FALLO/audio/loaded/start/end` + printf `[od] cutscene` | animation/CutsceneMgr.cpp:196-204, 215-226, 268-277, 282-296, 364-370, 475-481 | 59 |
| `CORONA tex[...]`, `CORONAREND` (+ contadores `coronasDibujadas/SinTextura` sin guarda) | renderer/Coronas.cpp:20, 93-119, 279-283, 350-351, 364, 437-453 | 53 |
| `TXTMISS`, `TXTGXTFAIL`, `MSGTABLE`, `TXTGXTSHORT` | text/Text.cpp:2, 58-64, 147-161, 337-351, 353-360, 465-467 | 48 |
| `LODLEFT` (OdLodLeftDiag, D10) | renderer/Renderer.cpp:3, 838-874, 917-922 (-o879) | 44 |
| `SKIP/SKIP1/SKIP2 tex dictpos` | rw/TexRead.cpp:9-11, 101-110, 156-164, 192-200 | 31 |
| save: `GenericLoad FAIL/abre`, `CheckSlotDataValid`, `DeleteSlot`, `save slot pos` (+ printf `[save]`) | save/GenericGameStorage.cpp:20-22, 351-354, 358-365, 369-370, 592-601; save/PCSave.cpp:22-27, 57-64, 89-100 | 53 |
| `hackRead tex NIL`, `hackRead convert NIL` (DIAG F3a; el primero lleva un `return` temprano anti-nil, "quitar con el fix raíz") | fakerw/fake.cpp:13-15, 311-319, 321-332 | 24 |
| `IFPFILE` + printf `[od] ifp-file` | animation/AnimManager.cpp:2, 1399-1402, 1429-1446 | 19 |
| `WATER no-waterpro` | renderer/WaterLevel.cpp:21, 135-136 | 3 |

Ninguna es `P4 kind=`. Los `#include "ondemand.h"` de estos ficheros (salvo CutsceneMgr.h) existen sólo para las trazas.

#### (c) VICEEXT_* presentes en estos ficheros

| Bandera | Sitio | Qué hace |
|---|---|---|
| VICEEXT_SAVE_ANYWHERE | save/GenericGameStorage.cpp:43-45 (include Wanted.h), 61-85; .h:33-37 | `ViceExtCanSaveAnywhere()`: sin misión, sin nivel de búsqueda y parado (<0,1) para guardar desde la pausa |
| VICEEXT_AUTOSAVE | save/GenericGameStorage.cpp:55-59; .h:38-43 | `bool ViceExtAutosaveInProgress`: marca de "autosave escribiendo" que deja pisar la ranura 9 (consumida en Frontend.cpp) |

Las demás features de estos ficheros (CLIMB, CROUCH, SPRINT_HEAVY, armas del mod, autosave por ranura) van SIN guarda (ver a).

#### (d) INFRA-WEB por subsistema (321 + / 120 -)

- GXT/texto robusto (D18/D19, cuelgue de pantalla negra): text/Text.cpp:26-57, 65-114, 241-336, 352, 361-395, 475-497 (borra o28-o419, 105 líneas); text/Text.h:29, 48, 95. Sin guarda (también nativo): `OdTextFileName` con default, salida de bucle en EOF y chunk incompleto, Load/ReadChunkHeader devuelven bytes.
- Guardado IDBFS: save/PCSave.cpp:4-11 (`WEB_FLUSH_USERFILES`), 36-42, 56, 66-70, 88; save/GenericGameStorage.cpp:344-350, 356, 366-376, 584-591, 602.
- Carga diferida de partida: save/GenericGameStorage.cpp:546-558, 568-579; .h:19-23. OJO: borra `CStreaming::LoadSceneCollision` (o476) SIN guarda: afecta también al nativo.
- Cinemáticas on-demand: animation/CutsceneMgr.h:3-20, 49 (`vcCutscenePublishLoading` -> `window.__vcInGame=false`); CutsceneMgr.cpp:190, 671, y 230-258 (comprobación stream/file nulos, sin guarda).
- Arreglos de crash: rw/TexRead.cpp:97-116, 153-170, 191-206 (saltar textura fallida); animation/AnimBlendAssocGroup.cpp:182-196 (clip sin jerarquía); animation/AnimManager.cpp:1475-1493 (tabla de anims/bloques llena), 1638-1654 (RemoveLastAnimFile sin doble free); renderer/WaterLevel.cpp:127-139 (sin `waterpro.dat` no hay bucle infinito).
- wasm: fakerw/fake.cpp:804-805 (sólo comentario del cast exacto por `call_indirect`).

#### (e) Animación: IDs/grupos/bloques añadidos y dependencias

NADO (sin guarda, se protege; 51 líneas):
- animation/AnimationId.h:287-295: `ANIM_STD_SWIM_TREAD`, `ANIM_STD_SWIM_CRAWL`, `ANIM_STD_SWIM_BREAST`, `ANIM_STD_SWIM_JUMPOUT`. Van justo detrás de `ANIM_STRIP_G` y son los PRIMEROS del bloque añadido: quitar CLIMB/CROUCH (detrás) no los desplaza.
- animation/AnimManager.h:79-81: `ASSOCGRP_PLAYERSWIM`.
- animation/AnimManager.cpp:935-945 `aSwimAnimations` ("swim_tread", "swim_crawl", "swim_breast", "swim_jumpout"); 946-970 `aSwimAnimDescs` (TREAD REPEAT; CRAWL/BREAST REPEAT|MOVEMENT|HAS_TRANSLATION; JUMPOUT FADEOUTWHENDONE|PARTIAL); 1115-1117 fila `{ "playerswim", "ped", MI_COP, awc(aSwimAnimations), aSwimAnimDescs }`.

Resto del mod (SIN-GUARDA, se sustituye): CLIMB_* x7 (AnimationId.h:296-306; AnimManager.cpp:1025-1045, 1119; AnimManager.h:83-86), CROUCH_* x5 + CROUCH_AIM* x4 (GunMove_*) (AnimationId.h:307-329; AnimManager.cpp:971-1024, 1118; AnimManager.h:82), grupos deagle/steyr/rocket (AnimManager.h:72-78; AnimManager.cpp:667-686, 1111-1114), buddy_reload/buddy_crouchreload (AnimManager.cpp:628-631), sprint_rocket/sprint_armed/sprint_csaw en RUNFAST (AnimManager.cpp:733-737, 751-753, 767-769).

Dependencias del NADO hacia código viejo / otros grupos:
1. Orden del enum `AssocGroupId`: `ASSOCGRP_PLAYERSWIM` va detrás de `ASSOCGRP_DEAGLE/STEYR/ROCKET` (viejo) y la tabla de grupos de AnimManager.cpp tiene que seguir ese mismo orden ("mismo orden que AssocGroupId", 1111). Si se quitan las armas del mod hay que quitar enum (AnimManager.h:72-77) y filas (AnimManager.cpp:1112-1114) a la vez; si no, "playerswim" queda desalineado. PLAYERCROUCH/PLAYERCLIMB van detrás y se pueden quitar sin afectar al nado (también enum y fila a la vez).
2. Regla de grupo `id - firstAnimId`: los 4 IDs SWIM son consecutivos y no dependen de CLIMB/CROUCH.
3. Datos: los clips salen del `ped.ifp` DEL MOD (272 clips frente a 234 de serie), servido en lugar del vanilla. El nado necesita ese ped.ifp y los límites de config.h (`NUMANIMATIONS = 512`, `NUMANIMBLOCKS = 40`, de otro grupo). Sólo con el ped.ifp del mod (410+38 = 448) cabría en 450, pero los bloques deagle/steyr/rocket lo suben a ~486.
4. Robustez web relacionada (INFRA-WEB, se queda): AnimBlendAssocGroup.cpp:182-196 (si falta un clip swim no peta en web) y AnimManager.cpp:1475-1493 (si la tabla se llena, salta el .ifp con un printf: bajar los límites rompería el nado en silencio).
5. renderer/WaterLevel.cpp NO es del nado: es un arreglo on-demand (falta `waterpro.dat` -> `break`). El nado usa la API vanilla `CWaterLevel::GetWaterLevel`, sin dependencia de código viejo.
6. RADIO: nada en estos ficheros.

### E.10 Grupo audio

Recuento (líneas añadidas, tras aplicar `rows_g5_audio_radio.tsv`):
INFRA-WEB 705 · DIAGNOSTICO 257 · SIN-GUARDA 192 · RADIO 93 · NADO 35 · MOD-VIEJO 9.
Borradas: INFRA-WEB 36 · SIN-GUARDA 20 · RADIO 4 · NADO 2.

Por fichero (+): sampman_oal 448 INFRA / 142 DIAG / 61 SIN-GUARDA · AudioLogic 110 SIN-GUARDA / 35 DIAG / 29 NADO / 16 INFRA / 9 MOD ·
MusicManager 93 RADIO / 64 DIAG / 8 INFRA · AudioManager 69 INFRA / 16 DIAG · oal/* 144 INFRA · AudioSamples.h 18 SIN-GUARDA ·
soundlist.h 6 NADO · sampman.h 3 SIN-GUARDA + 1 INFRA · sampman_null/miles 19 INFRA.

#### (a) SIN-GUARDA (mod viejo sin `#ifdef VICEEXT_*`)

| fichero:líneas | qué altera | importancia |
|---|---|---|
| audio/AudioLogic.cpp:868-874, 984-1010 (borra o865, o975) | Tabla `aVehicleSettings` ampliada a `MAX_CARS + 8` con 8 filas para los coches del mod 6500-6507 y `VehicleAudioIndex()` | ALTA (cambia el tamaño/índice de la tabla de audio de vehículos) |
| audio/AudioLogic.cpp:1058 (borra o1023) | `ProcessVehicle` indexa con `VehicleAudioIndex(modelIndex)` en vez de `modelIndex - MI_FIRST_VEHICLE` (depende de `MI_VEEXT_FIRST_VEHICLE` de ModelIndices.h) | ALTA |
| audio/AudioLogic.cpp:4565-4586 (borra o4473-4492) | Casos de armas del mod (SHOTGUN2, UZIOLD, AK47, M16, STEYR) en las colas de eco; además fusiona líneas (`switch (weaponType) {\t\t\t\tcase ...`) | MEDIA (necesita los WEAPONTYPE_* nuevos de WeaponType.h) |
| audio/AudioLogic.cpp:4651-4707 | Disparo DEAGLE/SHOTGUN2/STEYR/GRENADE_LAUNCHER con muestras `SFX_VICEEX_03/05/09/11` + `SampleManager.ReserveSample` | MEDIA |
| audio/AudioLogic.cpp:4729, 4883-4884, 4903, 4996-4997, 5008-5011, 5019, 5025 | `case WEAPONTYPE_*` del mod añadidos a switches de arma vanilla | MEDIA (no compila sin los enums del mod) |
| audio/AudioLogic.cpp:2553-2557 (o2518-2522) | Sólo reindentación de 3 líneas (inocuo) | BAJA |
| audio/AudioLogic.cpp:o2861 | Rama `#else` de `VICEEXT_FIX_SILENTPATCH` incompleta: el `if (params.m_nIndex == FBIRANCH)` vanilla se borró; sin la bandera la sirena aguda del FBI Rancher desaparece | MEDIA (latente: la bandera está puesta) |
| audio/AudioSamples.h:10120-10137 | 13 muestras `SFX_VICEEX_00..12` (ids 9941..9953) añadidas al enum; exige el SDT/RAW reconstruido con `tools/add_viceex_sfx.py` | ALTA (cambia `TOTAL_AUDIO_SAMPLES`, y con él el formato del SDT que se lee) |
| audio/sampman.h:220-222 + sampman_oal.cpp:1848-1863 | `cSampleManager::ReserveSample`. Sólo está implementado en OAL: sampman_null y sampman_miles no lo tienen, así que un build MSS o nulo no enlazaría con la llamada de AudioLogic.cpp:4675 | BAJA (latente) |
| audio/sampman_oal.cpp:124-138, 213-215, 234-243, 329-333 | `odSfxProtected[]`/`OdSfxReserve`: reserva de muestras de disparo del mod (decode inmediato, nunca se recicla) | BAJA (mecanismo genérico motivado por el mod) |
| audio/sampman_oal.cpp:1980-1994, 2001 | `InitialiseChannel` acepta `SFX_VICEEX_*` (> `SAMPLEBANK_MAX`) por la vía on-demand de `SFX_BANK_0` (`odViceExSample`) | MEDIA (sólo tras `__EMSCRIPTEN__`) |

#### (b) DIAGNOSTICO

| etiqueta | fichero:líneas | líneas aprox. |
|---|---|---|
| RETUNE | MusicManager.cpp:26-38, 575, 596, 615, 1213-1215 | 19 |
| `[web] radio lens ms` (printf) | MusicManager.cpp:196-203 | 8 |
| CUTMUS | MusicManager.cpp:1149-1159 | 11 |
| RADIOLOGOS (dentro de RADIO) | MusicManager.cpp:1410-1422 | 13 |
| `P4 kind=radio2` (dentro de RADIO; lo exige check-served-build.sh y viceext-log-check.py P4.11) | MusicManager.cpp:1480-1492 | 13 |
| ODSFXSTAT/ODSFXDEFER/ODSFXMISS/SFXLEN/ODSFXPROT/CHINIT/STREAM preload/RADIOTRACK/SFXDATA | sampman_oal.cpp:145-149, 167, 174-186, 268, 306, 311, 322-323, 345-353, 363, 435-455, 469-471, 473, 476-487, 1855-1857, 2049-2054, 2058-2070, 2247-2256, 2421-2452, 2617-2631 | 139 |
| `[web] sampman init ok` (printf) | sampman_oal.cpp:1471-1473 | 3 |
| SIRENA | AudioLogic.cpp:2932-2951 | 20 |
| `VICEEX sfx arma=` | AudioLogic.cpp:4694-4705 | 12 |
| includes de ondemand.h sólo para trazas | AudioLogic.cpp:5-7, AudioManager.cpp:14-16 | 6 |
| `[web] audiomanager init ok` (printf) | AudioManager.cpp:82-84 | 3 |
| LOOPORPHAN / LOOPKILL | AudioManager.cpp:1384-1388, 1406-1410 | 10 |

`web/lib/index.js:208` (ODPRI) prioriza RETUNE, RADIOTRACK, ODSFXDEFER, ODSFXSTAT en el envío de trazas.

#### (c) Banderas VICEEXT_* en estos ficheros

- `VICEEXT_RADIO_ICONS` (MusicManager.cpp:10-12, 1379-1424, 1458-1475, 1479-1493, 1520-1528): iconos de emisora desde `MODELS/RADIO.TXD`. Es RADIO, ver radio.md.
- `VICEEXT_FIX_SILENTPATCH` (AudioLogic.cpp:2896-2906): la sirena del FBI Washington (FBICAR) suena aguda como la del FBI Rancher. La rama `#else` está rota (ver o2861).
- No hay más VICEEXT_* en audio/. Las armas y coches del mod, el banco SFX_VICEEX y el nado entran sin guarda.

#### (d) INFRA-WEB por subsistema

- **SFX on-demand (mp3 por muestra, mpg123, caché con tope de 96 MB, presupuesto por frame, cola de diferidos)**: sampman_oal.cpp:90-123, 139-490 (sin las trazas ni la reserva), 1358-1370, 1547-1555, 1661-1672, 1708-1720, 1805-1818, 1998-2007, 2578-2580, 2604-2694; oal/channel.cpp:187-194 + channel.h:45 (`UsesData`).
- **OpenAL Web Audio / sin EFX**: sampman_oal.cpp:547-620; oal/oal_utils.cpp:52-190; oal/oal_utils.h:5-67; oal/channel.h:7-9.
- **Streams**: oal/stream.cpp:1281-1308 (`.adf` acepta MP3 plano: es la "radio ligera" del port), 1733-1766 (do/while acotado: arreglo de un freeze por desincronización de las fuentes estéreo).
- **Audio de misión bloqueante**: sampman_oal.cpp:2207-2246, 2564-2574; sampman.h:254; sampman_null.cpp:330-339; sampman_miles.cpp:2530-2538; AudioLogic.cpp:10278-10297.
- **Radio sin ficheros (longitud 0 → sin división por cero)**: MusicManager.cpp:140-141, 189-193, 263-265, 1263.
- **Loops huérfanos**: AudioManager.cpp:206-218, 1361-1426 (watchdog del "bucle del Malibú").

#### (e) Dependencias NADO/RADIO encontradas

- NADO (audio, sin guarda): soundlist.h:206-211 (`SOUND_NADO_BRAZO_A/B/LS_A/LS_B`, TOTAL=208, NO_SOUND=208; en vanilla NO_SOUND era TOTAL+1, pero es inocuo porque AudioManager.cpp:269 filtra con `< SOUND_TOTAL_SOUNDS`) y AudioLogic.cpp:4533-4561 (one-shot de brazada → `SFX_FOOTSTEP_WATER_1..4`, muestras vanilla). Quien lo emite es PlayerPed.cpp:3906-3907 (`s_odNadoBrazo`). No depende de ningún código MOD-VIEJO de audio; sólo del enum de soundlist.h.
- RADIO: ver radio.md. El retune web (gRetuneStartMs) lo he clasificado como RADIO, aunque no es del mod.
- Ojo, `SFX_VICEEX_12` es la muestra de "nadar" del mod, pero no se usa (comentario en AudioLogic.cpp:4655): el nado no depende del banco SFX_VICEEX.

### E.11 Grupo armas, vehículos, modelinfo, tráfico

27 ficheros. Recuento tras check.py (líneas añadidas / borradas):
DIAGNOSTICO 660 (+3 borradas), MOD-VIEJO 1045 (+33 borradas), SIN-GUARDA 796 (+122 borradas), INFRA-WEB 6, NADO 0, RADIO 0.

Por fichero (añadidas): Weapon.cpp MOD 513 / DIAG 278 / SG 61; Automobile.cpp SG 296 / MOD 272 / DIAG 237;
Bike.cpp MOD 78 / SG 38 / DIAG 37; CarCtrl.cpp DIAG 75 / MOD 58 / SG 31; VehicleModelInfo.cpp SG 54 / MOD 23;
Vehicle.cpp SG 46 / MOD 9; Pickups.cpp SG 39; HandlingMgr.h SG 37; ProjectileInfo.cpp SG 37; WeaponInfo.cpp SG 34 / MOD 20;
ModelIndices.h SG 27; WeaponType.h SG 26; Heli.cpp MOD 26 / DIAG 10; CarAI.cpp MOD 23 / DIAG 15 / SG 2; Boat.cpp SG 20 / DIAG 3; resto < 15.

#### (a) SIN-GUARDA (código del mod viejo sin `#ifdef VICEEXT_*`)

| Importancia | Bloque | Sitios (fichero:líneas nuevas; `oN` = vanilla borrada) | Qué altera del vanilla |
|---|---|---|---|
| ALTA | Armas nuevas del mod (ARMAS_MOD): enum 48..56 + `WEAPONTYPE_TOTALALLTYPES` + `IsWeaponType()` | WeaponType.h:53-79 (o53); WeaponInfo.h:34-39 (o34,o36-37); WeaponInfo.cpp:12, 50-62, 66, 69, 72-73, 112-125, 138, 315 (o12,o50,o54,o57,o60-61,o112,o270); WeaponEffects.cpp:71-73, 85-89 (o71,o83); Weapon.h:93 (o77); Weapon.cpp:660, 690-696, 710, 731, 832-839, 850-851, 1868-1871, 2862-2867, 2870-2872 (o2085,o2088), 3768 (o2962), 3798-3804, 3951-3953 (o3138); ProjectileInfo.cpp:104-126, 202-210, 248, 273, 339, 348-349, 390 (o179-183,o310,o319,o360); Vehicle.cpp:1176-1183, 1216, 1220, 1228, 1236, 1241-1243; Pickups.cpp:61-81, 86, 130-146, 397, 400 (o61-70,o75,o119,o370,o373); Pickups.h:122-124 (o122-124); Darkel.cpp:322, 409, 412, 424 (o322,o409,o412,o424); ModelIndices.h:469-482 | Redimensiona TODAS las tablas por tipo de arma (aWeaponInfo, nombres, munición, recarga, mira, precios, colores de pickup), cambia el enum (TOTALALLTYPES), añade granada de lanzagranadas con física propia (gravedad, sin aceleración 1.07), IsShotgun/IsType2Handed/daño a vehículo para 9 armas nuevas. Base de datos de todo lo demás (recoil, sights...). |
| ALTA | Vehículos nuevos del mod (COCHES_MOD/MOTOS_MOD) | ModelIndices.h:484-495; HandlingMgr.h:116-152, 303-308 (o118,o273); HandlingMgr.cpp:118-127, 392 (o118,o383); Bike.cpp:85-99 | IDs 6500-6507, 8 handlings nuevos (4 motos fuera del rango contiguo → `BikeHandlingSlot`, `GetBikePointer` cambia de indexado), grupo de anim de las motos. |
| ALTA | SVLIGHTS (R11/R11b): luces de servicio de policía/ambulancia/bomberos por dummies `servicelights_*` | Automobile.cpp:1928-1976, 2096-2117, 2142-2221, 2226-2229, 2241-2243, 2623-2628, 2636-2649, 2656-2667, 2674-2688, 2733-2738, 2748, 2756, 2762 (o2145-2146,o2156,o2164,o2170); VehicleModelInfo.cpp:199-250, 264, 271 (o203,o213-219,o226-232) | Las coronas de sirena de MI_FIRETRUCK/AMBULAN/POLICE se colocan desde la esfera del atomic del dummy, con corona 0,55/60 m en vez de 0,4/50; `CreateInstance` clona ahora el SUBÁRBOL completo de cada extra (antes sólo el atomic). La retry de 5 s (2147-2157, 1973-1975) sólo existe en web. |
| ALTA | Tanque de la IA dispara al jugador (bandera `VICEEXT_TANK_FIRE` definida pero NO usada) | Automobile.cpp:3910-3933, 3935-3942 (o3303) | `TankControl` deja de salir si el tanque no es del jugador: apunta y lanza `EXPLOSION_TANK_GRENADE` cada 2 s. Decidido en runtime por `ViceExtFeatures::On("MilitaryFiringFromTankAtPlayer", true)` (defecto ON). |
| ALTA | Coche volcado no arde (bandera `VICEEXT_NO_FLIP_BURN` definida pero NO usada) | Automobile.cpp:4767-4770, 4774-4779 (o4110-4113) | Con la clave runtime `VehiclesDontCatchFireWhenTurningOver` (defecto ON) el coche volcado ni pierde salud ni arde: cambia el vanilla por defecto. |
| ALTA | LOD +20 m (`VICEEXT_LOD_EXTRA` usado como valor, sin `#ifdef`) | SimpleModelInfo.cpp:118-123, 129, 136, 141 (o118,o124,o135) | Suma 20 m a GetLodDistance/GetNearDistance/GetLargestLodDistance. Si se quita el define de config.h, NO COMPILA. |
| MEDIA-ALTA | Drive-by ampliado y cadencia por arma (`VICEEXT_DRIVEBY_WIDE`, P2/P4) | Automobile.cpp:4498-4503, 4519-4529, 4556, 4560-4563, 4567 (o3859-3866,o3908-3910); Bike.cpp:2138-2141, 2157-2167, 2169, 2203, 2206-2208, 2210 (o2012-2019,o2036,o2070-2072); Boat.cpp:1418-1421, 1437-1447, 1467, 1469-1471, 1473 (o1417-1424,o1459-1461); Weapon.h:57-58; Weapon.cpp:932-944, 953-955 | Reescribe los tres `DoDriveByShootings`: la puerta de slot 5 pasa a `CPed::CanDoDriveByWithCurrentWeapon()` (Ped.cpp, otro grupo; vanilla sin la bandera), la cadencia a `CWeapon::GetDriveByShotDelay()` (70 ms sin la bandera) y se reordenan las puertas para poder trazar. Neutro sin la bandera salvo el orden. |
| MEDIA | Sirena de FBICAR/FBIRANCH (P5) | Automobile.cpp:2769-2777, 2781-2784 (o2180-2182) | Añade `case MI_FBICAR`, mueve la luz de FBIRANCH y QUITA la condición vanilla "sólo visible si la cámara está detrás". |
| MEDIA | Tráfico ponderado por frecuencia (D4) sólo en web | CarCtrl.cpp:930-960 | `ChooseCarModelToLoad` sortea el modelo a cargar por `m_frequency` (vanilla: uniforme). Bajo `__EMSCRIPTEN__` pero es lógica del mod. |
| MEDIA | `CVehicle::FindDummyFrame` (API pública) | Vehicle.h:342-344; Vehicle.cpp:35, 1450-1479 | Lo usan GAS_TANK, BREAKABLE_LIGHTS, TURN_SIGNALS, CAR_ROTATE_WHEEL y SVLIGHTS. Inocuo por sí solo. |
| MEDIA | FPS lógico en IA de coches | CarAI.cpp:216, 276 (o192,o252) | `GetFrameCounter()&7` → `GetLogicalFrameCounter()&7` en el cambio de pathfinding lejano (estilo FramerateVigilante, sin guarda). |
| MEDIA | StingerFixes del mod 2.0 (`:2309`) | Stinger.cpp:39 (o39), 100 | `memset` del objeto en el ctor y `m_nSpikeState = NONE` en Remove. Arreglos sanos. |
| BAJA | Restos neutros | Automobile.cpp:2836-2837 (o2234) y Bike.cpp:1456-1457 (o1333) `forceLightsOff=false`; Automobile.cpp:3704-3705 (o3098) y Bike.cpp:1941-1942 (o1817) `exitVehicleHeld`; Automobile.cpp o1525 (blanco borrado) | Sin efecto; restos de features retiradas. |
| BAJA | Includes sin guarda | Weapon.cpp:2, Automobile.cpp:3 (`ViceExtFeatures.h`) | Dependencia de `ViceExtFeatures` (fichero nuevo de agentes). |

Ganchos sin `#ifdef` pero NEUTROS (clasificados MOD-VIEJO porque su stub `#else` devuelve el vanilla): Automobile.cpp:565/581/601 (`ViceExtWheelKeepSteer`, o517/o533/o553), 1539/1545 (else-if de CHEAT_EFFECTS), 1656/2568 + Bike.cpp:1837 + Heli.cpp:325 + CarCtrl.cpp:213/235/264/293/1062 (`odPhase`/`odSpawnPhase` de FIX_FV), CarCtrl.cpp:612/617/623/1133 (`odDespawnK` de LIMITS), Heli.cpp:1018 (`odHeliDamageLimit`), WeaponInfo.cpp:253/257 (sscanf partido para WEAPON_SIGHTS).

**Trampas para la limpieza (borrados de vanilla SIN `#else`)** — quitar la bandera sin restaurar rompe el vanilla:
- `VICEEXT_FIX_FV`: Automobile.cpp:1470-1477 envuelve el spool del rotor de los RC (o1419 comentario, o1422, o1424 borrados) → sin la bandera el RC Raider/Goblin NO despega. CarCtrl.cpp:2797-2799 (o2647) → sin la bandera el heli IA no acelera el rotor.
- `VICEEXT_FIX_FV_ROTOR`: Heli.cpp:590-592 (o573) → sin la bandera el rotor del heli no gira.
- `VICEEXT_ROCKET_3RD_PERSON`: Weapon.cpp:2876-2894 conserva el chequeo vanilla en `#ifndef` (o2092-2100), OK.
- Automobile.h declara `ViceExtProcessTurnSignals` sólo con `VICEEXT_TURN_SIGNALS`, pero Automobile.cpp lo define/llama con `TURN_SIGNALS || STD_TURN_SIGNALS`.
- `VICEEXT_CAR_FOUR_COLORS` sólo declara `m_currentColour3/4` en Vehicle.h:182-185 (nadie los usa).
- `VICEEXT_FIX_SILENTPATCH` en VehicleModelInfo.cpp:1175-1200 es código muerto (`if(false && ...)`).

#### (b) DIAGNOSTICO

| Etiqueta | Sitios | Contexto |
|---|---|---|
| RECOIL_READY/INPUT/SHOT/CLASS/RELOAD/WEAPON_CHANGE/FIRE_WAIT/CONTROL, RECOIL3 (emite por `window.__odRecoilEmit`, sesión `window.__odRecoilSession`, marca `OD_RECOIL_SCHEMA_1`) | Weapon.cpp:168-173, 175-176, 183-186, 198-247, 306, 330-338, 345-350, 355-364, 366, 373-375, 383-384, 386, 389-417, 420-464, 486-487, 523-526, 552-561, 613-616, 984-999; Weapon.h:42-44, 48-49 | en:MOD (VICEEXT_RECOIL). ~190 líneas: el recoil viejo es ~40 % traza. |
| P4 kind=uzi | Weapon.cpp:606-612, 918-925, 1610-1617, 1981-1988 | en:WEB (sólo `__EMSCRIPTEN__`) |
| P4 kind=ini opt=2/4/5/7/13/14/18/19 (emisor `ViceExtP4IniEmit`) | Automobile.cpp:58-80, 84, 1555, 3934, 4771-4773, 4799; Heli.cpp:1022-1030; Weapon.cpp:3139-3145 (bloque PHOTO_SAVE entero), 4051-4055 | mixto MOD / SIN-GUARDA |
| P4 kind=turners | Automobile.cpp:1815-1817, 1830-1832, 1836-1838, 1850-1852, 1864, 1867, 1871-1873, 1881-1883, 1892, 1904, 1912-1922, 2027-2049 | en:MOD (TURN_SIGNALS) |
| P4 kind=service / SVLIGHTS (+ macro `VICEEXT_P4_EMIT`) | Automobile.cpp:1977-2026, 2118-2141, 2222-2225, 2230-2240, 2598-2612 | en:WEB/SIN-GUARDA |
| P4 kind=wheelrot | Automobile.cpp:6 (include Pools.h), 2083-2093 | en:MOD |
| P4 kind=cardet / cardead / carluz / overflow | Automobile.cpp:4954-4955, 4963-4967, 4983-4987, 4992-4996, 5052-5061 (bloque VICEEXT_CAR_LIGHTS = sólo traza) | en:MOD |
| VICEEXT gastank reserva/hit, VICEEXT luces rota | Weapon.cpp:1477-1489, 1514-1521, 1569-1576 | en:MOD |
| DRIVEBY state/shot (`CPed::DriveByTraceState/Shot`) | Automobile.cpp:4518, 4557-4559, 4564-4566; Bike.cpp:2156, 2204-2205, 2209; Boat.cpp:1436, 1468, 1472 | en:SIN-GUARDA |
| BSIREN | Bike.cpp:40-41, 1318-1348 | en:MOD |
| CARSPAWN, CARZONE, CARRATE, CARRATECNT | CarCtrl.cpp:658-672, 786-791, 793-844 (o748-749, o751: refactor neutro de ChooseCarRating sólo para trazar) | en:WEB |
| P4 kind=chofer, P4 kind=polbike | CarAI.cpp:72-76, 574-582 | en:MOD |
| includes `ondemand.h` | Weapon.cpp:24, Automobile.cpp:29, Heli.cpp:30, CarCtrl.cpp:31-33, CarAI.cpp:10, Bike.cpp:41 | — |

#### (c) Banderas VICEEXT_* en estos ficheros

| Bandera | Qué hace aquí |
|---|---|
| VICEEXT_RECOIL | Weapon.cpp:124-563, 613-616, 868-873, 984-999; Weapon.h:37-50, 106-109: perfil de patada por arma, clasificación tap/burst SMG, cola de patadas que consume Cam.cpp (`ViceExtRecoilAlphaAdd`); apagable con features.ini `RecoilWhenFiring`. |
| VICEEXT_GAS_TANK | Weapon.cpp:1404-1524 (+ganchos 1583-1602, 2317-2320, 2726-2729, 3451-3454): disparo al dummy `petrolcap` (o caja de colisión) → `BlowUpCar`. |
| VICEEXT_BREAKABLE_LIGHTS | Weapon.cpp:1526-1581: disparo a `headlight_*`/`taillight_*` → luz rota. |
| VICEEXT_DRIVEBY_WIDE | Weapon.cpp:945-952 (cadencia por arma), 969-981 (sonido de pistola desde vehículo). |
| VICEEXT_ROCKET_3RD_PERSON | Weapon.cpp:2876-2894: el lanzacohetes dispara sin modo de cámara 1ª persona. |
| VICEEXT_PHOTO_SAVE | Weapon.cpp:3139-3145: sólo traza (la funcionalidad está en otro grupo). |
| VICEEXT_PROJECTILE_BLOW | Weapon.cpp:4038-4059: disparar a un proyectil lo hace explotar. |
| VICEEXT_FIX_SILENTPATCH | Weapon.cpp:1969-1971 (sin casquillo Python/Sniper/Laser); VehicleModelInfo.cpp:1175-1200 (código muerto). |
| VICEEXT_WEAPON_SIGHTS | WeaponInfo.cpp:175-290, WeaponInfo.h:65-69: columna 27 de weapon.dat → `m_nSight`. |
| VICEEXT_FIX_FV / VICEEXT_FIX_FV_ROTOR | Automobile.*, Bike.*, Heli.*, CarCtrl.cpp: fases por tiempo (`m_fOdPhase`, `ms_odSpawnPhase`) en vez de FrameCounter; spool/rotor por timestep (ver trampas). |
| VICEEXT_CAR_WHEEL_KEEP | Automobile.cpp:81-89: las ruedas conservan el giro al salir. |
| VICEEXT_CHEAT_EFFECTS | Automobile.cpp:1540-1557: RC Raider/Goblin disparan cohetes con el truco RCROCKET. |
| VICEEXT_TURN_SIGNALS / VICEEXT_STD_TURN_SIGNALS | Automobile.cpp:1790-1924, Automobile.h:138-143: intermitentes por coronas en dummies `indicator*`. |
| VICEEXT_CAR_ROTATE_WHEEL | Automobile.cpp:2051-2095, 3344-3346, Automobile.h:144-150: volante `steeringwheel` gira. |
| VICEEXT_GAS_STATION_BLOW | Automobile.cpp:4782-4804: choque fuerte con surtidor → explosión. |
| VICEEXT_WHEEL_DETACH / VICEEXT_CAR_DEATH | Automobile.cpp:4931-5000: rueda reventada se desprende; ocupantes NPC mueren. |
| VICEEXT_CAR_LIGHTS | Automobile.cpp:5052-5061: sólo traza. |
| VICEEXT_POLICE_BIKE_LIGHTS / VICEEXT_POLICE_BIKE | Bike.cpp:1116-1135, 1311-1381; Vehicle.cpp:1632-1634, 1654-1659; CarCtrl.cpp:967-973; CarAI.cpp:571-584: moto policial 6507 (sirena, coronas, aparición con nivel de búsqueda, ocupante). |
| VICEEXT_HELI_HEALTH | Heli.cpp:1012-1016: el heli policial cae con 400 de daño (vanilla 700). |
| VICEEXT_LIMITS (+ VICEEXT_SPAWN_RADIUS_MULT) | CarCtrl.cpp:310-312, 613-615, 1130-1132: radios de spawn/despawn ×1,5. |
| VICEEXT_POLISH | CarAI.cpp:15-31, 65-80: conductores huyen del coche ante disparos a <15 m. |
| VICEEXT_CAR_FOUR_COLORS | Vehicle.h:182-185: campos sin uso. |
| Definidas en config.h pero no usadas aquí como `#ifdef` | VICEEXT_TANK_FIRE, VICEEXT_NO_FLIP_BURN (la lógica va sin guarda, por ViceExtFeatures), VICEEXT_LOD_EXTRA (usado como valor). |

#### (d) INFRA-WEB

Sólo Stinger.cpp:124-126, 165-166, 202 (6 líneas): nil-checks de `pSpikes[]` y de la asociación `ANIM_STD_THROW_UNDER`. El plan del agente C (`.agents/plans/2026-10-03-2.0-agente-C-logica-ia-agachado.md:390-395`) dice que NO son del mod: son arreglos de crash del port. En estos 27 ficheros no hay más infraestructura web; todo lo que va bajo `__EMSCRIPTEN__` es traza o lógica del mod (CarCtrl.cpp:930-960, Automobile.cpp:1973-1975/2147-2157).

#### (e) Relación con NADO / RADIO

- NADO: ninguna línea de estos ficheros pertenece al nado ni la usa. `bIsInWater` sólo aparece en la línea vanilla reemitida Automobile.cpp:4775 (o4110) y en el spool de FIX_FV (vanilla). Boat.cpp sólo recibe la reescritura del drive-by.
- RADIO: ninguna relación (no se toca la radio de vehículos aquí).
- Dependencias hacia fuera del grupo: `CPed::CanDoDriveByWithCurrentWeapon/DriveByTraceState/DriveByTraceShot` (Ped.cpp:5122-5210, Ped.h:1006-1017), `ViceExtFeatures::On` (RecoilWhenFiring, StandardCarsUseTurnSignals, MilitaryFiringFromTankAtPlayer, VehiclesDontCatchFireWhenTurningOver), `CPed::bViceExtRcRocket` (CHEAT_EFFECTS), `ViceExtRecoilAlphaAdd` lo consume Cam.cpp, `MI_GR_GRENADE` se precarga en `CStreaming::LoadInitialWeapons` (Streaming.cpp), `ASSOCGRP_DEAGLE/STEYR` vienen de animación (AnimManager/AnimationId.h).

### E.12 Grupo HUD, menús, entrada, ViceExtFeatures

Ficheros: renderer/Hud.cpp, renderer/Font.cpp/.h, core/Frontend.cpp/.h, core/MenuScreensCustom.cpp,
text/Messages.cpp, core/Radar.cpp, renderer/Sprite2d.cpp/.h, core/Pad.cpp/.h, core/ControllerConfig.cpp/.h,
core/ViceExtFeatures.cpp/.h (nuevos).

Recuento con `check.py` (líneas añadidas / borradas):
- Añadidas: MOD-VIEJO 1091 · DIAGNOSTICO 663 · SIN-GUARDA 288 · INFRA-WEB 30 · NADO 2 · RADIO 0.
- Borradas: SIN-GUARDA 52 · INFRA-WEB 108 (todas colaterales sin efecto: blancos y codificación) · MOD-VIEJO 11 (reindentación) · DIAGNOSTICO 3 (reindentación).

#### (a) SIN-GUARDA: cambios del mod viejo que alteran el vanilla siempre

| Importancia | fichero:líneas (nuevas / `oN` vanilla) | Qué altera | Bandera o feature |
|---|---|---|---|
| ALTA | core/ControllerConfig.cpp:199 (o199-200) | `VEHICLE_ENTER_EXIT` pasa a tener `F` como tecla principal; se le quita ENTER (queda libre para SKIP_PHONE_CALL) | SKIP_PHONE_CALL |
| ALTA | core/ControllerConfig.cpp:290-313 (o291) | Se quita la `V` de "cambiar cámara". Nuevos defaults: V = 1ª persona, R = recarga, LALT = andar | FIRST_PERSON / MANUAL_RELOAD / ALT_SIT |
| ALTA | core/ControllerConfig.h:62 (o62), 67-70 (o67) | El enum de acciones cambia: `_CONTROLLERACTION_36`→`PED_TOGGLE_1RST_PERSON`, `UNKNOWN_ACTION`→`PED_RELOAD`, y se añade `PED_WALK` al final (altera el formato de la config de controles guardada) | FIRST_PERSON / MANUAL_RELOAD / ALT_SIT |
| MEDIA | core/ControllerConfig.cpp:546-548, 2030-2032 | Nombres y tipo (a pie) de las 3 acciones nuevas | idem |
| MEDIA | core/ControllerConfig.h:192-202 + .cpp:1216-1233 | `ViceExtActionKeyJustDown` (tecla con respaldo). La usan Camera.cpp:262 (1ª persona), PlayerPed.cpp:4248 (agachado, bajo VICEEXT_CROUCH) y Script.cpp:899 (SKIP_PHONE_CALL) | FIRST_PERSON / CROUCH / SKIP_PHONE_CALL |
| ALTA | core/Radar.cpp:1106-1128 (o1103-1133), 22 | Color del waypoint: sale de `ViceExtFeatures::Waypoint()` con default `VICEEXT_GPS_COLOR_*` (255,0,255); el `features.ini` servido manda 180,24,24 (vanilla 255,72,77). Está bajo MAP_ENHANCEMENTS y no bajo VICEEXT_GPS_COLOR. Sin config.h no compila | GPS_COLOR (FEATURES_INI) |
| ALTA | renderer/Hud.cpp:813-820 (o423), 2 | Dinero sin ceros vía `ViceExtFeatures::On("RemoveMoneyZerosInTheHud")`. La bandera VICEEXT_MONEY_NO_ZEROS existe pero no se usa aquí. Override del jugador = 1 | MONEY_NO_ZEROS |
| ALTA | renderer/Hud.cpp:1063-1068, 1080-1081 (o633) | Estrellas ocultas sin búsqueda vía `ViceExtFeatures::On(...)`. La bandera VICEEXT_HIDE_STARS no se usa aquí. Override = 1 | HIDE_STARS |
| ALTA | renderer/Hud.cpp:391, 454, 492-494 (o262, o272) | La cruz se pinta también con la ley de apuntado ClassicAXIS (`CCamera::s_viceExtAimLawActive`, `ViceExtIsAiming()`). Depende de miembros que el mod añade a CCamera/CPed | AIM_CLASSICAXIS |
| MEDIA | renderer/Hud.cpp:408-418 (o253, o255) | Rango de armas con cruz PC ampliado a COLT45..ROCKETLAUNCHER + BERETTA..GRENADE_LAUNCHER (enum del mod) | armas del mod / WEAPON_SIGHTS |
| MEDIA | renderer/Hud.cpp:500-508, 518-521 (o279-282, o288-291) | Caja de la cruz de 3ª persona reducida (0.6→0.42 y 0.4→0.26) para todos, haya mira del mod o no | WEAPON_SIGHTS |
| MEDIA | renderer/Hud.cpp:o1720 | Borrado accidental de `m_pLastZoneName = nil;` en `CHud::Initialise` | WEAPON_SIGHTS (colateral) |
| BAJA | renderer/Hud.cpp:941 (bajo FIX_BUGS) | `VERTEXALPHAENABLE` antes del icono del arma (SilentPatch F3) | FIX_SILENTPATCH |
| MEDIA | renderer/Sprite2d.cpp:16-21, 92-94, 120 (o86), 366-369 (o332); Sprite2d.h:24; Hud.cpp:1601-1614, 1944-1952 | Escalado 640x448 de los sprites/rects de script (SilentPatch :1999 ScriptSpritesScaling). Sólo está activo entre `SetScriptSpriteScale(true/false)` | FIX_SILENTPATCH |
| MEDIA | core/Frontend.cpp:5917-5957 (o5769, o5776, o5793) | Outro de 2,5 s: tick 10→32 ms y cuenta 150→75 (va junto a MUCH_SHORTER_OUTRO_SCREEN comentado en config.h) | FIX_SILENTPATCH :4086 |
| BAJA | core/Frontend.cpp:2478-2498 (o2447) | Clamp del alfa del fade del menú (OutroSplashFix) | FIX_SILENTPATCH :2384 |
| MEDIA | core/Frontend.cpp:941-942 | Reset de la aceleración del ratón al empezar partida (`MouseSensNewGame`, lote de mods del agente B) | mods-fix |
| BAJA | core/Radar.cpp:1683 (o1670) | Corrección del triángulo "abajo" de la leyenda del radar | mods-fix (agente B) |
| ALTA | core/Frontend.cpp:1081, 1126-1139, 1157-1160, 1459, 1539, 4822, 5150-5153 (o1066, o1111-1122, o1421, o1501, o4765, o5022-5025); Frontend.h:173 | Las ranuras de guardado llegan a `SAVESLOT_9` y el nombre/fecha de cada fila se calcula por número de ranura | AUTOSAVE |
| ALTA | core/MenuScreensCustom.cpp:473-479 (o503-505), 526-527 (o559) | La ranura 8 desaparece de los menús de cargar y guardar. En carga, SL6 y SL7 quedan en una sola línea | AUTOSAVE |
| MEDIA | core/Frontend.cpp:5588-5590 (o5460) | El quicksave de misión usa `PAUSE_SAVE_SLOT` (fichero 9) en vez de `SLOT_COUNT` | AUTOSAVE / MISSION_REPLAY |
| BAJA | core/Pad.cpp:296-321, 351-366 | `WeaponCheat4` (CRAZYTOOLS) definida sin guarda. Sólo se activa bajo VICEEXT_EXTRA_CHEATS, pero compila siempre y referencia `MI_BERETTA`… y `WEAPONTYPE_*` del mod | EXTRA_CHEATS |
| BAJA | core/ViceExtFeatures.h:1-37 | API declarada sin guarda; las definiciones sólo existen con VICEEXT_FEATURES_INI. Quien la llama sin guarda (Hud, Radar, World, PedFight, PlayerPed, SpecialFX, Automobile) no enlaza sin la bandera | FEATURES_INI |

Activos sin guarda pero clasificados como DIAGNOSTICO (instrumentación de los agentes; atención, cambian el juego):
- core/Pad.cpp:367-433, 442-498, 1529-1538, 1814-1829: cheats de prueba **CRAZYPISTOL** (da la Beretta), **CRAZYRIDES** (crea los 8 vehículos del mod), **CRAZYHINT** (aviso con iconos de tecla) y **CRAZYCOP** (pone 3 estrellas). Funcionan en cualquier build.
- Las trazas `FEMENU` (Frontend.cpp:469-477, 5736-5745) y `SCRTXT3` (Font.cpp:10-11) van sin guarda, pero ODTRACES es `((void)0)` fuera de web.

#### (b) DIAGNOSTICO: etiquetas y rangos

| Etiqueta | Sitios | Contexto |
|---|---|---|
| SCRTXT (textos en pantalla) | Hud.cpp:21-24, 2366-2389; Messages.cpp:17-57, 233-235, 318-320, 342-344, 814-816 | WEB |
| SCRTXT3 (caza-todo de textos abajo) | Font.cpp:10-11, 951-1014 | WEB |
| SIGHT / `P4 kind=sight` / SIGHTS | Hud.cpp:316-357, 2192-2198 | MOD (WEAPON_SIGHTS) |
| SWIMHUD | Hud.cpp:425-437 | NADO (consulta `ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)`) |
| `P4 kind=hud_cross` | Hud.cpp:438-453 | WEB |
| AIMHUD / AIHTRI / `P4 kind=hud2` | Hud.cpp:713-757 | MOD (AIM_CLASSICAXIS) |
| `P4 kind=hud2` dinero (cada frame) | Hud.cpp:804-810 | WEB |
| HUDICON | Hud.cpp:881-912 | WEB |
| `P4 kind=ini` (opt=9 estrellas, opt=11 GPS) | Hud.cpp:1069-1079; Radar.cpp:25-26, 1137-1145 | WEB / MOD |
| KEYICONS | Hud.cpp:2205-2207 | MOD (HINT_KEYS) |
| HINTKEY / `P4 kind=hint2` / `kind=shift` / `kind=overflow` | Messages.cpp:14, 534-570 | MOD (HINT_KEYS) |
| FEMENU | Frontend.cpp:469-477, 5736-5745 | sin guarda |
| WR newgame-req / WR load-req (+printf `[want]`) | Frontend.cpp:4836-4846, 5606-5618 (o5476-5478 reindentados) | WEB (port: guardado) |
| "VICEEXT save-anywhere rechazado/aceptado" | Frontend.cpp:4896-4902, 4920-4929, 4932-4938 | MOD (AUTOSAVE / SAVE_ANYWHERE) |
| WLOAD (+printf `[wload]`, llama `ViceExtWeaponInfoOf`) | Pad.cpp:15-16, 46-50, 322-350 | WEB |
| HINTTEST | Pad.cpp:459-461 | WEB |
| `P4 kind=cheat2` (ViceExtCheatTrace) | Pad.cpp:806-817 + llamadas 1835…1885 | MOD (EXTRA_CHEATS) |
| Cheats de prueba CRAZYPISTOL / CRAZYRIDES / CRAZYHINT / CRAZYCOP | Pad.cpp:367-433, 442-498, 1529-1538, 1814-1829 | sin guarda |
| `P4 kind=inifile` | ViceExtFeatures.cpp:5, 214-244 | MOD (FEATURES_INI) |

#### (c) Banderas VICEEXT_* en estos ficheros

| Bandera | Líneas | Sitios | Qué hace aquí |
|---|---|---|---|
| VICEEXT_WEAPON_SIGHTS | 191 | Hud.cpp:35-37, 229-371, 510-556, 2176-2200, 2472-2475 | Miras por arma: `weaponSights.txd` + columna 27 de weapon.dat. La mira se pinta al apuntar |
| VICEEXT_AIM_CLASSICAXIS | 167 | Hud.cpp:465-491, 646-785 | Cruz de 3ª persona con la ley de apuntado, marca de fijado coloreada por salud y triángulo sobre el objetivo de ratón. La rama `#else` conserva un marco de 4 rects |
| VICEEXT_HINT_KEYS | 370 | Font.cpp/.h, Messages.cpp:13-16, 518-602, ControllerConfig.cpp:2852-2965, .h:230-236, Hud.cpp:2201-2208 | `pcbtns.txd` y marca `~K<vk>~`: el aviso pinta el icono de la tecla en vez de su nombre. Incluye la tabla rs→VK |
| VICEEXT_FEATURES_INI | 278 | ViceExtFeatures.cpp:7-284 | Lector de `ViceExtended/features.ini` (ver deps_viceextfeatures.md) |
| VICEEXT_EXTRA_CHEATS (+VICEEXT_CHEAT_EFFECTS anidada) | 103 | Pad.cpp:805-848, 1830-1888 | Trucos COWBOY69, IAMINVINCIBLE, AIRWAYS, BIGSMOKE, AEZAKMI, PHOTOGRAPHER, RCROCKET, BIGHEADS y CRAZYTOOLS. Con CHEAT_EFFECTS, BIGHEADS y RCROCKET tienen efecto real |
| VICEEXT_SAVE_ANYWHERE | 48 | Frontend.cpp:4854-4863, 4910-4939; MenuScreensCustom.cpp:640-647 | "Guardar" en el menú de pausa, con las condiciones del mod |
| VICEEXT_AUTOSAVE | 45 | Frontend.cpp:1143-1156, 4889-4908; MenuScreensCustom.cpp:457-468 | Fila AUTOGUARDADO (ranura 9) la primera de la lista de carga; prohíbe guardar a mano en la 9 |
| VICEEXT_FIX_WFP | 25 | Sprite2d.cpp:95-119 | Bandas negras para texturas fullscreen no 4:3 (WFP) |
| VICEEXT_ALT_SIT | 16 | Pad.cpp:2624-2636; Pad.h:225-227 | `CPad::GetPedWalk()`: tecla de la acción PED_WALK |
| VICEEXT_SKIP_PHONE_CALL | 12 | ControllerConfig.cpp:314-316, 549-551, 1978-1980; .h:71-73 | Acción SKIP_PHONE_CALL (ENTER) |
| VICEEXT_GPS_COLOR | 11 | Radar.cpp:1136-1146 | Sólo la traza. La lógica del color va sin guarda |
| VICEEXT_POLICE_BIKE_LIGHTS | 8 | Pad.cpp:434-441 | La moto policial que crea CRAZYRIDES nace con la sirena puesta |
| VICEEXT_FIRST_PERSON | 5 (+57, 425) | MenuScreensCustom.cpp:52-57, 425 | Opción "FEM_1PV" en el menú Display (`CCamera::m_bViceExt1stPersonView`) |

#### (d) INFRA-WEB

| fichero:líneas | Qué |
|---|---|
| core/Frontend.cpp:7-10 | includes `emscripten.h` + `ondemand.h` |
| core/Frontend.cpp:3342-3347 | `OD.syncUserfiles('settings')`: persiste los ajustes en IDBFS en SaveSettings |
| renderer/Font.cpp:80-145, 200-249 (o74-139, o194-243) | Colateral: 20 comentarios re-codificados (byte Latin-1 → U+FFFD), sin efecto |
| core/MenuScreensCustom.cpp:o33-o817 (104 líneas) | Colateral: líneas en blanco eliminadas, sin efecto |

Del port web real sólo hay en este grupo el sync de ajustes a IDBFS y sus includes.

#### (e) RADIO y NADO en estos ficheros

- **RADIO: nada.** Los iconos de emisora y `RADIO.TXD` viven en `audio/MusicManager.cpp` (bajo VICEEXT_RADIO_ICONS). El selector de radio del menú (`Frontend.cpp` PrintRadioSelector) es vanilla, sin cambios. Hud.cpp no dibuja iconos de radio. Los cambios de Sprite2d no la afectan: el escalado de script sólo actúa entre `SetScriptSpriteScale(true/false)` y WFP sólo con rects de pantalla completa.
- **NADO: 2 líneas sin guarda + 1 traza.**
  - `renderer/Hud.cpp:455`: `&& !ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)` oculta la cruz mientras el carril NADO posee el apuntado. Sin `#ifdef`.
  - `renderer/Hud.cpp:28`: `#include "PedArbiter.h"`, que es lo que la habilita.
  - Depende de `core/PedArbiter.*`. `ViceExtPedOwns()` compila sin VICEEXT_PEDARBITER: devuelve MOTOR y la condición queda en verdadero.
  - La línea 455 va pegada a la 454 (`|| odAimLaw`, ClassicAXIS). Al quitar AIM_CLASSICAXIS hay que conservar la 455.
  - Traza SWIMHUD: Hud.cpp:425-437.
- **Teclas del nado: ninguna** en Pad/ControllerConfig. El nado no usa PED_WALK, PED_RELOAD, PED_TOGGLE_1RST_PERSON ni `ViceExtActionKeyJustDown`. Los usos de PED_WALK y PED_DUCK en PlayerPed.cpp están bajo AIM_CLASSICAXIS, ALT_SIT y CROUCH.
- **Dependencia indirecta del NADO hacia FEATURES_INI.** `PedArbiter.cpp:41` (bajo VICEEXT_PEDARBITER) niega el carril NADO si `ViceExtFeatures::On("EnableSwimming", true)` es falso. Ver deps_viceextfeatures.md.

### E.13 Grupo mundo, script, partículas, edificios

Ficheros: core/Wanted.cpp/.h, core/World.cpp, control/Script.cpp/.h, Script3.cpp, Script6.cpp,
Script8.cpp, ScriptCommands.h, renderer/ParticleMgr.cpp/.h, Particle.cpp, ParticleType.h,
Clouds.cpp, SpecialFX.cpp, buildings/Building.cpp/.h, entities/Dummy.cpp/.h, Entity.cpp,
core/ZoneCull.cpp/.h, renderer/Shadows.cpp, MBlur.cpp, extras/shaders/neoRimSkin.vert(+.inc).

Filas: `rows_g7b_world_script.tsv` (validado con check.py: 0 sin clasificar).

#### Totales del grupo (líneas añadidas / borradas)

| Categoría | + | − |
|---|---:|---:|
| MOD-VIEJO (bajo VICEEXT_*) | 983 | 2 |
| DIAGNOSTICO | 376 | 1 |
| SIN-GUARDA | 175 | 22 |
| INFRA-WEB | 17 | 0 |
| NADO | 0 | 0 |
| RADIO | 0 | 0 |
| **Total** | **1551** | **25** |

Por fichero (+): Building.cpp 189 MOD/8 SG/1 DIAG · Dummy.cpp 184/8/1 · Wanted.cpp 170 MOD/62 SG/192 DIAG ·
Script8.cpp 140 MOD/15 DIAG · ParticleMgr.cpp 151 MOD/5 SG/27 DIAG · World.cpp 20 MOD/33 SG/48 DIAG ·
Script.cpp 39 MOD/1 SG/29 DIAG/17 WEB · resto < 40 líneas.

#### (a) SIN-GUARDA (código del mod viejo fuera de cualquier `#ifdef VICEEXT_*`)

| fichero:líneas | Qué altera del vanilla | Importancia |
|---|---|---|
| core/World.cpp:1945-1994 | `CWorld::Process` recorre CADA FRAME los pools enteros de dummies y edificios llamando a `ProcessDistantLights()` (luces lejanas, coronas hasta 530 m). Sólo lo gobierna `ViceExtFeatures::On("EnableDistantLights", true)` (por defecto ON). Además **rompe la compilación sin `VICEEXT_DISTANT_LIGHTS`** (el método sólo se declara bajo la bandera en Building.h/Dummy.h). | ALTA |
| control/Script6.cpp:1007-1009 (−1005), :24 | `COMMAND_IS_PLAYER_TARGETTING_CHAR`: la heurística PC de "apuntando" exige cámara de apuntado (`Using1stPersonWeaponMode`/`Using3rdPersonMouseCam`) — arreglo SilentPatch 1610 — y además `!ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)` (depende de PedArbiter y del carril del nado). Cambia lo que ven los guiones de misión. | ALTA |
| core/Wanted.cpp:33-37, 102-158 | `WANTED_SIGHT_RADIUS` y `WantedCopSeesPlayer()` (LOS + cono de 75°) sin guarda. Usa `WANTED_SIGHT_COS`, que sólo existe bajo `VICEEXT_HIDE_COPS` → **sin la bandera no compila**. Sin efecto en el juego salvo vía la mecánica y la traza (que sí se ejecuta siempre, ver DIAGNOSTICO). | MEDIA |
| renderer/ParticleType.h:88-102 (−88, −91) | 5 tipos nuevos en `tParticleType` (ENGINE_SMOKE2_SMALL, ENGINE_STEAM_SMALL, ENGINE_SMOKE_SMALL, CARFLAME_SMALL, CARFLAME_SMOKE_SMALL32), `MAX_PARTICLES` 83→88, `PARTICLE_LAST` cambia, `PARTICLE_VICEEXT_FIRST`. Agranda `cParticleSystemMgr`. Nadie emite esos tipos (sólo se cargan y se les da textura). | MEDIA |
| renderer/ParticleMgr.h:136-137 (−136) | `VALIDATE_SIZE(cParticleSystemMgr)` 0x2FFC→0x32E0 por los 88 tipos. | MEDIA (va con lo anterior) |
| renderer/Particle.cpp:674-688 | `CParticle::Initialise`: raster de los 5 tipos del mod (gpCloudRaster4/gpSmoke2Raster/gpFlame1Raster). | BAJA |
| renderer/ParticleMgr.cpp:189-193 | Quita el `\r` final de cada línea de PARTICLE.CFG (para el fichero CRLF del mod). Inocuo con LF; candidato a conservar. | BAJA |
| renderer/SpecialFX.cpp:4, 391-395, 449-453 | `CBulletTraces::AddTrace` (las dos sobrecargas) sale si `ViceExtFeatures::On("DisableBulletTraces", false)`. Por defecto OFF = vanilla. | MEDIA (lee features.ini en cada disparo) |
| renderer/SpecialFX.cpp:481-482, 492-494 | Armas del mod (DESERT_EAGLE, SHOTGUN2, AK47, M16, STEYR) en el `switch` de grosor/duración de trazas de bala. Dependen de `WeaponType.h` (otro grupo). | BAJA |
| core/World.cpp:374-376, 387 (−370-371, −376) | Refactor de `ProcessLineOfSightSectorList` (variables `chassisHit`/`wheelWins`) para `VICEEXT_NO_WHEEL_PIERCE`; equivalente sin la bandera. | BAJA |
| core/World.cpp:2172-2174 (−2094, −2096) | Refactor del chequeo de entidades caídas (`odPhase` en vez de `GetFrameCounter()&7`) para `VICEEXT_FIX_FV`; equivalente sin la bandera. | BAJA |
| core/World.cpp:2-3, 9 | includes ViceExtFeatures.h, Building.h, Dummy.h (luces lejanas). | BAJA |
| control/Script.cpp:13 | include ControllerConfig.h (para `VICEEXT_SKIP_PHONE_CALL`). | BAJA |
| control/Script.cpp:−1552..1555 | Borra el autosave vanilla muerto (`#if 0`) y `pPlayerInfo` en `COMMAND_TERMINATE_THIS_SCRIPT`; su sitio lo ocupa el bloque `VICEEXT_AUTOSAVE`. Sin efecto (código muerto). | BAJA |
| buildings/Building.cpp:6-13, entities/Dummy.cpp:6-13 | includes de `ProcessDistantLights` sin guarda. | BAJA |
| renderer/Shadows.cpp:2003-2005 (−2003-2005) | Arreglo SilentPatch 1967: `CastShadowEntityXY` proyecta también con `GetUp()` (sombras de entidades inclinadas). Cambia el render vanilla. | MEDIA |
| renderer/MBlur.cpp:570, 573 (−569, −571, −574) | Arreglo SilentPatch 2196: zonas seguras de `AddRenderFx` escaladas con `SCREEN_STRETCH_*` (radar y HUD superior). | BAJA |
| extras/shaders/neoRimSkin.vert:42, obj/neoRimSkin_vert.inc:43 | `DoFog(gl_Position.w)` en vez de `.z`, igual que neoRim/neoGloss/neoVehicle. Es un arreglo de render genérico (no del mod): **candidato a conservar como INFRA**. | BAJA |
| entities/Entity.cpp:418 (−418) | Sólo se quitó un tabulador. Sin efecto. | NULA |

#### (b) DIAGNOSTICO (376 líneas + 1 borrada)

| Etiqueta | fichero:líneas | Líneas | Nota |
|---|---|---:|---|
| `WANTED tag=… ` (1/s), `WantedTraceTag` (`window.__vcWantedTag`) | Wanted.cpp:14, 19-32, 82-101, 159-242, 822-824; Wanted.h:8-14 | ~132 | **Sin guarda: corre siempre**, incluso sin VICEEXT_HIDE_COPS; cada segundo lanza un `ProcessLineOfSight` por perseguidor. La consume `tools/hidecops-smoke-test.mjs`. |
| `WANTEDHIDEINIT` (rev/constantes) | Wanted.cpp:59-80, 263-271 | 31 | dentro de VICEEXT_HIDE_COPS; `VICEEXT_HIDE_REV` sólo existe para esta traza |
| `WANTEDHIDE end/seen/start/drop` | Wanted.cpp:719-723, 760-763, 776-778, 787-790 | 16 | dentro de VICEEXT_HIDE_COPS |
| `WANTEDCHANGE` | Wanted.cpp:606-613 (−363) | 8 | sin guarda |
| `WANTEDPURGE`, `WANTEDSUSPEND` | Wanted.cpp:866-873, 918-925 | 16 | sin guarda |
| `SHOT rueda` | World.cpp:388-395 | 8 | `__EMSCRIPTEN__` |
| `P4 kind=distlights` (+overflow) | World.cpp:1950, 1957-1959, 1969-1971, 1974-1992 | 26 | contadores sin guarda + traza `__EMSCRIPTEN__` |
| `P4 kind=muro` | World.cpp:2207-2209, 2217, 2322-2330 | 13 | dentro de VICEEXT_POLISH |
| `CALL skip` | Script.cpp:904-914 | 11 | dentro de VICEEXT_SKIP_PHONE_CALL |
| `SCMLOOP` | Script.cpp:1030-1031 | 2 | aviso del vigilante SCM (INFRA-WEB) |
| `VICEEXT autosave` | Script.cpp:24-26, 1623-1635 | 16 | dentro de VICEEXT_AUTOSAVE |
| `TGTGUN` | Script6.cpp:21, 1032-1038 | 8 | **sin guarda, una línea por cada ejecución del opcode** (spam) |
| `P4 kind=modscript op=0x0FA0..0x0FA9` (+overflow) | Script8.cpp:32-33, 643, 682, 694, 706, 717, 726, 730, 734, 738, 742, 750, 761, 767 | 15 | dentro de VICEEXT_MOD_SCRIPT, una línea por opcode ejecutado |
| `P4 kind=partcfg` | ParticleMgr.cpp:6-7, 133, 158-166, 217, 223, 225, 233-235, 240-241, 431-437 | 27 | dentro de VICEEXT_PARTICLE_NAMES (contadores + emits) |
| `P4 kind=particulas` | Particle.cpp:24-26, 887, 939-946 | 12 | dentro de VICEEXT_POLISH |
| `P4 kind=moon` | Clouds.cpp:17, 155-171 | 18 | dentro de VICEEXT_MOON |
| `P4 kind=worlddata` (soporte) | ZoneCull.cpp:173-183, ZoneCull.h:54-60 | 18 | **todo `VICEEXT_MAPDATA` aquí es sólo diagnóstico**: `CCullZones::ViceExtNumZones()` sólo lo llama la traza de `CGame::Initialise` (Game.cpp:737-752) |
| include ondemand.h sin uso | Building.cpp:14, Dummy.cpp:14, World.cpp:29 | 3 | |

#### (c) Banderas VICEEXT_* de estos ficheros (sitios MOD-VIEJO)

| Bandera | Qué hace aquí | Sitios (+) | Líneas |
|---|---|---|---:|
| VICEEXT_DISTANT_LIGHTS | `CBuilding/CDummy::ProcessDistantLights` (Project2DFX portado de su exe): coronas de luces 2dfx lejanas hasta el far clip; la llamada está en World.cpp **sin guarda** | Building.cpp:32-220, Building.h:16-18, Dummy.cpp:62-245, Dummy.h:14-16 | 379 |
| VICEEXT_HIDE_COPS | "Changed wanted system": esconderse; campos nuevos al final de `CWanted` (m_vecLastKnownPos, m_nLastSeenTime, m_nHiddenSince, m_nLastStarDrop, m_bHiding), `AnyCopSeesPlayer`, `UpdateHiding` (5 s gracia + 1 estrella/15 s sin ser visto, también nivel 1), llamada cada frame en `Update` | Wanted.cpp:38-58, 81, 255-262, 272, 648-799 (menos trazas), 803-805; Wanted.h:41-59 | 189 |
| VICEEXT_MOD_SCRIPT | Opcodes propios del main.scm del mod 4000-4009 (0x0FA0-0x0FA9): SET/GET_PED_SKIN, SELECT_PED, AUTOSAVE, SCRIPT_FLAG_OFF/INVERT/SAVE/RESTORE, SCRIPT_VALUE, VEHICLE_HEADING_DEG; despacho en `ProcessOneCommand`, enum en ScriptCommands.h, estado en estáticos `s_odModScript*` | Script.cpp:1095-1100, Script.h:343-345, Script8.cpp:31-773, ScriptCommands.h:1477-1491 | 164 |
| VICEEXT_PARTICLE_NAMES | particle.cfg resuelto por NOMBRE (tabla `odParticleNames` con los 88 tipos) en vez de posicional; entrada comodín para nombres desconocidos; copia de nombre acotada | ParticleMgr.cpp:13-135, 156, 213-254 | 151 |
| VICEEXT_AUTOSAVE | Autosave al terminar misión (ranura 9): `ViceExtAutosavePending` lo pone REGISTER_MISSION_PASSED (Script3.cpp) y el opcode 0x0FA3, lo consume TERMINATE_THIS_SCRIPT | Script.cpp:21, 71-75, 1608-1622, 1636; Script.h:621-626; Script3.cpp:2336-2342; Script8.cpp:721-725 | ~40 |
| VICEEXT_POLISH | Explosiones: entidades tras un muro (sin LOS) no reciben daño; partículas: sangre ×1,5 y polvo ×2 de vida | World.cpp:2215-2220; Particle.cpp:884-889, 930-952 | 21 |
| VICEEXT_MOON | Luna móvil en `-sunDir*150` con brillo/fade propios (su Clouds.cpp) | Clouds.cpp:139-154, 172-178 | 18 |
| VICEEXT_SKIP_PHONE_CALL | Saltar llamada de teléfono de misión con tecla (`SKIP_PHONE_CALL`, Enter): FailCurrentMission=2 y limpia el audio de misión | Script.cpp:896-917 | 11 |
| VICEEXT_FIX_FV | Chequeo de caídos al vacío con fase por tiempo (`odFallenPhase` += timestep) en vez de por frame (independiente de FPS) | World.cpp:2163-2171 | 9 |
| VICEEXT_NO_WHEEL_PIERCE | El disparo en la rueda gana si el impacto en chasis está a ≤0,5 m (no atraviesa la rueda) | World.cpp:381-386 | 6 |
| VICEEXT_MAPDATA | Sólo diagnóstico aquí (ver DIAGNOSTICO) | ZoneCull.cpp/.h | 0 MOD (18 DIAG) |

Banderas que **no compilan apagadas** por código sin guarda que depende de ellas: `VICEEXT_DISTANT_LIGHTS`
(World.cpp:1960/1972 llaman a un método declarado sólo bajo la bandera) y `VICEEXT_HIDE_COPS`
(Wanted.cpp:133 usa `WANTED_SIGHT_COS`, definido en :58 bajo la bandera).

#### (d) INFRA-WEB

| fichero:líneas | Qué es |
|---|---|
| control/Script.cpp:1019-1029, 1032-1039 (traza en 1030-1031) | Vigilante del intérprete SCM bajo `__EMSCRIPTEN__`: si un guion ejecuta >200000 comandos seguidos sin ceder, se corta y duerme 500 ms (evita que la pestaña muera en un bucle infinito). Comportamiento distinto del vanilla sólo en el caso patológico. Pudo motivarlo un opcode del mod no implementado, pero es protección genérica del port. |

#### (e) Relación con NADO / RADIO y dependencias

- **NADO**: el único punto es `control/Script6.cpp:24, 1008-1009` — `IS_PLAYER_TARGETTING_CHAR` consulta
  `ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)` (PedArbiter). Es dependencia del código viejo HACIA el nado
  (no al revés): quitarlo no rompe el nado; si se quita PedArbiter, hay que quitar también esta línea. La
  traza `TGTGUN` imprime `nado=`.
- Ninguna partícula/opcode de este grupo la usa el nado (no hay salpicaduras ni tipos de agua nuevos; los 5
  tipos de partícula nuevos son humo/llama de motor y no los emite nadie).
- **RADIO**: nada en este grupo.
- Dependencias hacia fuera del grupo: `ViceExtFeatures` (claves `EnableDistantLights` en World.cpp:1948,
  `DisableBulletTraces` en SpecialFX.cpp:393/451); `ControlsManager.ViceExtActionKeyJustDown` y la acción
  `SKIP_PHONE_CALL` (ControllerConfig, otro grupo); `ViceExtAutosaveInProgress` (GenericGameStorage, otro
  grupo); armas `WEAPONTYPE_DESERT_EAGLE/SHOTGUN2/AK47/M16/STEYR` (WeaponType.h, otro grupo);
  `CWanted::IsHiding/GetLastKnownPos` los usa CopPed.cpp (otro grupo); la traza `P4 kind=worlddata` vive en
  Game.cpp (otro grupo) y necesita `CCullZones::ViceExtNumZones`.

## Anexo F. Tooling: detalle marca a marca

Solo lectura: no se ejecutó nada. Las rutas de tooling son relativas a `gta_vc_browser/tools/` y las de emisor a la raíz del repo. "Guarda del emisor" es la pila de preprocesador de la línea emisora en `src/` (calculada con el diff anotado). `+EM` significa que además está dentro de `#ifdef __EMSCRIPTEN__`.

### 0. Quién transporta las trazas (INFRA-WEB, se queda)

| Pieza | Dónde | Qué hace |
|---|---|---|
| `ODTRACES(msg)` / `ODTRACEI(tag,v)` | `src/skel/ondemand.h:8-9` (en nativo son no-op, `:27-28`) | `EM_ASM` que hace push de la línea en `window.__odq` |
| Cola, prioridad y envío | `web/lib/index.js:208` (`ODPRI`: loadtick, FPSLOG, FPHASE, ODSFXDEFER, RADIOTRACK…), `:271-303` (POST async a `traceUrl`), `:499-590` (flush, tope de 30000 líneas, RECOIL tiene prioridad) | Manda `window.__odq` por lotes |
| Sobre del recoil | `web/lib/index.js:505-522` (`RECOIL sessionId=… wasm=OD_RECOIL_SCHEMA_1 …`, `RECOIL_DIAG`, `RECOIL_DIAG_END`) | Envuelve las líneas `RECOIL_*` del motor |
| Errores y latido | `web/lib/index.js:602-625` (`JSERR`, `JSERR_REJ`, `ENGERR`), `:770` (`FPSLOG frames=… maxdelta=`) | Trazas de la capa web |
| `OD.trace` | `web/ondemand.js:365-378` (prefijo `JS `; emite `JS build=<VERSION> data=<dataTag>`), `:805` `ODSTA`, `:828` `ODWRITE`; destino `odtraceUrl: '/odtrace'` (`:142`) | Trazas del pre-js/on-demand |
| Escritura a disco | `web/lib/vite.js:27-53` (`/odtrace[/<nombre>]` → `web/odtrace[-<nombre>].log`, `.prev.log`), `:139-155` (rota el fichero cuando llega `JS build=`) | Middleware de Vite que escribe el log |

Si se borran todas las llamadas `ODTRACES` del mod viejo, el transporte sigue funcionando: solo se queda sin líneas del mod. `ondemand.h` (ODTRACES y `odBlockingPush/Pop`) es INFRA-WEB y hay que conservarlo.

### 1. `check-served-build.sh` (351 líneas)

**Propósito:** comprueba que el `.wasm`/`.js` servido (`web/public/build/reVC.{wasm,js}`) lleva las "marcas" de lo implementado. Hace `grep -a` de cadenas literales (formatos de `snprintf`, nombres de función que quedan en la sección de nombres del wasm, ids de clip) y comprueba los datos del mod servidos. Sale con exit 1 si falta cualquier marca. Lo llaman `harness/vc-harness.mjs:191-203` (`motorServido`) y `vc-test.mjs --estatico`.

Otras comprobaciones, además de la tabla de marcas:
- `:238-242` dataTag de `reVC.js`, `:244-272` `VERSION` de `web/lib/index.js` (en disco y servido por `localhost:2077`). Son INFRA-WEB.
- `:275` la cadena `ve13` en `reVC.js` (muestras de audio del mod recodificadas). Mezcla INFRA-WEB y MOD-VIEJO (sonidos del mod).
- `:192-200` DATOS servidos con tamaño y sha256: `data/particle.cfg` (fam. 3/5), `data/main.scm` (fam. 13), `models/coll/newWeapons.col` (13), `ViceExtended/features.ini` (11), `data/object.dat`, `data/occlu.ipl`, `data/maps/bryx/bryx.{ide,ipl}` (fam. 7). Todos son MOD-VIEJO (datos del mod).
- `:310-315` la línea `COLFILE … NEWWEAPONS.COL` en `streamed/data/default.dat`. Es MOD-VIEJO.

Recuento de las 161 marcas de `MARCAS=(` (líneas 25-185): **MOD-VIEJO 132**, **NADO 19**, NADO + agachado 1 (`camz=`), dependencias del NADO que hoy viven en código viejo 4 (`PEDCLAIM cap=`, `ViceExtPedOwns` → PedArbiter; `PEDAT x=…`, `dist=%.0f` → bajo `VICEEXT_CROUCH`), **RADIO 1** (`P4 kind=radio2`), INFRA-WEB 2 (`LOADSCR begin`, `ODSTA`) y andamiaje P4 2 (`P4 kind=identity`, `P4 kind=overflow`).

Por guarda del emisor: 37 marcas salen de código que solo está tras `__EMSCRIPTEN__` (sin ningún `VICEEXT_*`) y 12 de código SIN GUARDA. Entre las SIN GUARDA están `SVLIGHTS`, `CAM1P`, `servicelightson`, `ViceExtPedOwns`, `SOUND_NADO_BRAZO_LS_B`, `ViceExtWeaponInfoOf`, `Crouch_Roll_L`, `GunCrouchFwd`, `GunMove_FWD`, `ViceExtCrouchShooting`, `ViceExtAimSensMouseX` y `MouseAimSensX`.

#### Tabla de marcas

| línea | marca | bloque (texto del script) | emisor (src, primeras) | guarda del emisor | categoría |
|---|---|---|---|---|---|
| 25 | `VICEEX sfx arma` | D8  sonidos: qué muestra pide cada arma | src/audio/AudioLogic.cpp:4700 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 26 | `odViceExSample` | D8b sonidos del mod admitidos por el motor | src/audio/sampman_oal.cpp:1990, src/audio/sampman_oal.cpp:1991, src/audio/sampman_oal.cpp:2001 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 27 | `SWIM2 move` | R7  nado: avance real medido | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 28 | `CROUCH2` | R6  agachado: peso del clip | src/peds/PlayerPed.cpp:4673, src/peds/PlayerPed.cpp:5388, src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH | MOD-VIEJO |
| 29 | `SVLIGHTS` | R11 luces de servicio por dummies | src/vehicles/Automobile.cpp:29, src/vehicles/Automobile.cpp:2135, src/vehicles/Automobile.cpp:2233 | SIN GUARDA | MOD-VIEJO |
| 30 | `extras=` | R11 luces de servicio: variante y extras elegidos | src/vehicles/Automobile.cpp:2135, src/vehicles/Automobile.cpp:2233 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 31 | `SIRENA tipo=` | SR  sirena audible: muestra que encola el motor | src/audio/AudioLogic.cpp:2944 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 32 | `LBAR w=` | LB  contorno de la barra de carga escalado (SilentPatch 723) | src/core/main.cpp:1044 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 33 | `RECOIL_APPLY shotSeq=` | R8  retroceso que mueve la CÁMARA (esquema RC: era RECOIL2) | src/core/Cam.cpp:251 | VICEEXT_RECOIL | MOD-VIEJO |
| 34 | `AIMDIR` | R9  apuntado: desviación cámara-ped | src/peds/PlayerPed.cpp:3111 | VICEEXT_AIM_WALK +EM | MOD-VIEJO |
| 35 | `CAM1P` | R5  primera persona: puerta y tecla | src/core/Camera.cpp:1352 | SIN GUARDA | MOD-VIEJO |
| 36 | `LOADSCR begin` | L6  carga de partida: una portada y una barra (sección 1) | src/core/main.cpp:1072 | __EMSCRIPTEN__ (sin VICEEXT) | INFRA-WEB |
| 37 | `camz=` | H1/H2 cámara medida agachado y nadando | src/peds/PlayerPed.cpp:4011, src/peds/PlayerPed.cpp:5624, src/peds/PlayerPed.cpp:5783 | VICEEXT_SWIMMING | NADO + MOD-VIEJO (agachado) |
| 38 | `servicelightson` | §6  nombres de luces de servicio del mod | src/vehicles/Automobile.cpp:2193 | SIN GUARDA | MOD-VIEJO |
| 39 | `reload anim` | R12 recarga a mano: animación del clip del arma | src/peds/PlayerPed.cpp:3531, src/peds/PlayerPed.cpp:3540 | VICEEXT_MANUAL_RELOAD +EM | MOD-VIEJO |
| 40 | `repetido=` | R12 agachado: antirrebote del conmutador | src/peds/PlayerPed.cpp:5163 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 41 | `pedido=` | R12 autocentrado de coche: retorno pedido vs pasivo | src/core/Cam.cpp:2725, src/peds/PlayerPed.cpp:3933 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 42 | `WINFO arma=` | R13 ficha por arma: flags, canaim/witharm, reload y nombres de clip | src/peds/PlayerPed.cpp:266 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 43 | `clips=` | R13 los NOMBRES de clip que resuelve el grupo del arma | src/animation/AnimManager.cpp:1443, src/peds/PlayerPed.cpp:266, src/peds/PlayerPed.cpp:4974 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 44 | `1p bind tecla=` | R13 la tecla de 1ª persona que tiene guardada el navegador | src/core/Camera.cpp:1323 | VICEEXT_FIRST_PERSON +EM | MOD-VIEJO |
| 45 | `por=` | R13 motivo por el que se pinta la mira (SIGHT) | src/renderer/Hud.cpp:328 | VICEEXT_WEAPON_SIGHTS +EM | MOD-VIEJO |
| 46 | `manoH=` | R13b hacia dónde apunta la mano que lleva el arma | src/peds/PlayerPed.cpp:3111 | VICEEXT_AIM_WALK +EM | MOD-VIEJO |
| 47 | `SWIMCAM` | R14 nado: objetivo de cámara en la superficie | src/core/Cam.cpp:1250, src/core/Cam.cpp:1806, src/core/Cam.cpp:6254 | VICEEXT_SWIMMING | NADO |
| 48 | `teclas=` | R14 autocentrado: teclas (empujón) vs ratón (suave) | src/core/Cam.cpp:2725 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 49 | `RECOIL3 miracheck` | R14 retroceso: la mira NO se mueve | src/weapons/Weapon.cpp:412 | VICEEXT_RECOIL +EM | MOD-VIEJO |
| 50 | `ODSFXPROT` | R14 sonidos de arma del mod reservados (sin cola ni reciclado) | src/audio/sampman_oal.cpp:1856 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 51 | `CROUCHPOSE` | R15 desagacharse: la pose vuelve a la de pie | src/peds/PlayerPed.cpp:5067 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 52 | `R3P arma=` | R16 lanzacohetes apuntando en 3ª persona (Vice Extended) | src/peds/PlayerPed.cpp:2654 | VICEEXT_ROCKET_3RD_PERSON +EM | MOD-VIEJO |
| 53 | `crouch move spd=` | R17 agachado: clip de MOVIMIENTO (la raíz avanza) y avance medido | src/peds/PlayerPed.cpp:5610 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 54 | `CROUCH2 h=%.2f avance=` | R17 agachado: traza de avance real + distancia de cámara | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 55 | `swim move spd=` | R17 nado: velocidad y cadencia dictadas por el clip del mod | src/peds/PlayerPed.cpp:3971 | VICEEXT_SWIMMING | NADO |
| 56 | `SWIMCAM no-fallen-water` | R17 cámara: el jugador vivo en el agua no usa la cámara de ahogado | src/core/Cam.cpp:1250 | VICEEXT_SWIMMING | NADO |
| 57 | `ViceExtSwimClipSpeed` | R17 nado: m/s natural del clip (braza/crol) desde hierarchy->totalLeng | src/peds/PlayerPed.cpp:3569, src/peds/PlayerPed.cpp:3876, src/peds/PlayerPed.cpp:3909 | VICEEXT_SWIMMING | NADO |
| 58 | `ahogando=%d` | R18 cámara de nado: el motor marca ahogado y la cámara sigue siendo la | src/core/Cam.cpp:1250, src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 59 | `velo=%.2f dt=` | R18 velocidad de MUNDO (m/s de simulación) además del avance de reloj | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 60 | `hondo=` | R30 nado Extended: pin al pecho con puerta por profundidad (1.3) | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 61 | `glide=` | R31 nado Extended: la pose de caída no se cuela al nada (1.4) | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 62 | `pie=%d col=%d` | R32 nado Extended: quién manda en el eje vertical y quién no (1.6) | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 63 | `PEDCLAIM cap=` | R33 arbitro de capacidades: un dueño por pieza, y la traza de cada cam | src/core/PedArbiter.cpp:50, src/core/PedArbiter.cpp:60, src/core/PedArbiter.cpp:87 | VICEEXT_PEDARBITER +EM | MOD-VIEJO (PedArbiter, dep. NADO) |
| 64 | `odcrouch=%d` | R34 nado Extended: el agachado que se ve es el del mod, no el del moto | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 65 | `SWIMHUD cruz=` | R34 nado Extended: delata si la puerta del HUD deja pasar la retícula  | src/renderer/Hud.cpp:433 | __EMSCRIPTEN__ (sin VICEEXT) | NADO |
| 66 | `ViceExtPedOwns` | R35 arbitro: las consultas son de PROPIEDAD, no de permiso (1.8) | src/control/Script6.cpp:1008, src/core/Cam.cpp:653, src/core/Cam.cpp:654 | SIN GUARDA | MOD-VIEJO (PedArbiter, dep. NADO) |
| 67 | `SWIMEF braz=1 splash=1 medido=` | R37 nado: splash por brazada, el avance MEDIDO en m/s y el id de sonid | src/peds/PlayerPed.cpp:3933 | VICEEXT_SWIMMING | NADO |
| 68 | `SWIMFOV fov=%.2f aim=%d aplica=%d modo=%d camdist=%.2f paso=%d` | R37 nado: el FOV real, el boton de apuntar DE VERDAD y la distancia de | src/peds/PlayerPed.cpp:3988 | VICEEXT_SWIMMING | NADO |
| 69 | `SOUND_NADO_BRAZO_LS_B` | R37 nado: los cuatro sonidos de brazo, del banco del propio Vice City  | src/audio/AudioLogic.cpp:4536, src/audio/soundlist.h:209, src/peds/PlayerPed.cpp:3906 | SIN GUARDA | NADO |
| 70 | `SWIMNAT clip=%d totalLength=` | R36 nado: el totalLength REAL de cada clip y su naturalidad, que la 1. | src/peds/PlayerPed.cpp:3584 | VICEEXT_SWIMMING | NADO |
| 71 | `SWIMERR esperada=` | R30 nado Serega: traza de error solo ante desvio | src/peds/PlayerPed.cpp:4032 | VICEEXT_SWIMMING | NADO |
| 72 | `aim=%d law=` | R30 nado Serega: axis-compat (ley de apuntado apagada nadando) | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 73 | `PEDAT x=%.1f y=%.1f z=%.1f` | R19 posición, velocidad real y camz del ped (sondas y navegación) | src/peds/PlayerPed.cpp:5783 | VICEEXT_CROUCH +EM | MOD-VIEJO bajo VICEEXT_CROUCH (dep. arnés carga/nado) |
| 74 | `medida=1x` | R19b la velocidad del agachado se mide una sola vez por frame | src/peds/PlayerPed.cpp:5610 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 75 | `dist=%.0f` | R19c la sonda mide la distancia al agua (navegación hasta el mar) | src/peds/PlayerPed.cpp:5783 | VICEEXT_CROUCH +EM | MOD-VIEJO bajo VICEEXT_CROUCH (dep. arnés carga/nado) |
| 76 | `WLOAD arma=` | ve37 arnés: qué modelo y diccionario del mod deja cargado el truco de  | src/core/Pad.cpp:342 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 77 | `ViceExtWeaponInfoOf` | ve37 arnés: la ficha WINFO de un arma concreta (medir las 8 sin teclad | src/core/Pad.cpp:50, src/core/Pad.cpp:347, src/peds/PlayerPed.cpp:5694 | SIN GUARDA | MOD-VIEJO |
| 78 | `txdCargado=` | ve37 arnés: el diccionario del arma está en memoria, no sólo mapeado | src/core/Pad.cpp:342 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 79 | `nom=%s` | R20 agachado: NOMBRE del clip que pide el motor (8 direcciones medible | src/peds/Ped.cpp:1789, src/peds/PlayerPed.cpp:5610, src/peds/PlayerPed.cpp:5624 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 80 | `ang=%.1f` | R20 agachado: ángulo del mando en el sistema del ped | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 81 | `strafe=%d` | R20 agachado: modo de control (ratón en 3ª persona = clips de costado) | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 82 | `Crouch_Roll_L` | R20 agachado: los clips de costado del mod, registrados | src/animation/AnimationId.h:319, src/animation/AnimManager.cpp:975 | SIN GUARDA | MOD-VIEJO |
| 83 | `ViceExtCrouchRate` | R20 agachado: ritmo de los clips (palanca en caliente) | src/peds/PlayerPed.cpp:4617, src/peds/PlayerPed.cpp:5423 | VICEEXT_CROUCH | MOD-VIEJO |
| 84 | `veloc=%.2f rueda=` | R22 agachado: la velocidad OBJETIVO (1,13 m/s desde R28) y el estado d | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 85 | `moved=%.2f mvec=` | R22 agachado: la velocidad DEL MOTOR (m_moved) se publica para medir s | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 86 | `VICEEXT crouch roll lado=` | R22 agachado: cada rueda se anuncia como evento (se cuenta cuántas hac | src/peds/PlayerPed.cpp:5347 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 87 | `pesomira=%.2f` | R21 apuntar agachado: peso de la pose WEAPON_crouch del mod | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 88 | `mira=%d gat=%d pesomira=` | R21 apuntar agachado: el apuntado y su pose se publican en CROUCH2 | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 89 | `ViceExtCrouchAimPose` | R21 apuntar agachado: la pose del mod se mezcla sin bIsDucking | src/peds/PlayerPed.cpp:1436, src/peds/PlayerPed.cpp:3044, src/peds/PlayerPed.cpp:4515 | VICEEXT_AIM_CLASSICAXIS | MOD-VIEJO |
| 90 | `ViceExtCrouchMoveSpeed` | R22 agachado: el desplazamiento usa la velocidad objetivo (1,13 m/s de | src/peds/Ped.cpp:1637, src/peds/PlayerPed.cpp:1507, src/peds/PlayerPed.cpp:5525 | VICEEXT_CROUCH | MOD-VIEJO |
| 91 | `ViceExtCrouchRateFor` | R22 agachado: el ritmo del clip se calcula de la velocidad (pies sin p | src/peds/PlayerPed.cpp:4617, src/peds/PlayerPed.cpp:5423 | VICEEXT_CROUCH | MOD-VIEJO |
| 92 | `giro=%d hdgr=` | R26 agachado: motivo del giro a los lados + headingRate | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 93 | `cal=1 clipr=` | R27 agachado calibrado: clips del sa-crouch servidos y raíz declarada  | src/peds/PlayerPed.cpp:5610, src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 94 | `GunCrouchFwd` | R27 agachado: el motor pide los dos clips del sa-crouch servidos | src/animation/AnimationId.h:317, src/animation/AnimManager.cpp:973 | SIN GUARDA | MOD-VIEJO |
| 95 | `VICEEXT crouch roll skip motivo=` | R27 rueda: por qué NO salió (sin-mira/arma/disparo/bloqueada/activa) | src/peds/PlayerPed.cpp:5397 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 96 | `motivo=ok` | R27 agachado: cada rueda sale por su evento con motivo | src/peds/PlayerPed.cpp:5347 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 97 | `entrada=%.0f` | R27 agachado: ms desde el cambio de clip (ventana del blend 10) | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 98 | `P4 kind=roll_end schema=1` | P4  plan4: fin de rueda con maxWeight y rateIntegral | src/peds/PlayerPed.cpp:4843 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 99 | `GunMove_FWD` | R29 movimiento apuntando: los cuatro GunMove_* del mod en el grupo de  | src/animation/AnimationId.h:326, src/animation/AnimManager.cpp:982, src/peds/PlayerPed.cpp:4577 | SIN GUARDA | MOD-VIEJO |
| 100 | `gat=` | R29 CROUCH2: gatillo para separar pose de apuntado y disparo (costura  | src/peds/PlayerPed.cpp:2808, src/peds/PlayerPed.cpp:5624 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 101 | `ViceExtCrouchShooting` | R28 agachado: el motor elige sus clips de agachado (pose de apuntar y  | src/peds/Ped.h:1080, src/peds/PedFight.cpp:443, src/peds/PedFight.cpp:558 | SIN GUARDA | MOD-VIEJO |
| 102 | `ViceExtCrouchOtherPartials` | R28 agachado: presupuesto de parciales (la pose del mod cede ante el c | src/peds/PlayerPed.cpp:4683, src/peds/PlayerPed.cpp:4771 | VICEEXT_CROUCH | MOD-VIEJO |
| 103 | `ViceExtCrouchRollClearPartials` | R28 rueda: mientras rueda se retiran las parciales (si no, el clip de  | src/peds/PlayerPed.cpp:4723, src/peds/PlayerPed.cpp:4764 | VICEEXT_CROUCH | MOD-VIEJO |
| 104 | `otros=%.2f pose=%s pesoarma=` | R28 agachado: presupuesto, clip de la pose y parcial del arma en CROUC | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 105 | `nomarma=%s` | R28 agachado: NOMBRE de la parcial del arma (*_crouchfire = disparo ag | src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 106 | `ViceExtCrouchSideHeading` | R26 agachado: el cuerpo gira al avance a los lados | src/peds/Ped.cpp:1660, src/peds/PlayerPed.cpp:2215, src/peds/PlayerPed.cpp:4438 | VICEEXT_CROUCH | MOD-VIEJO |
| 107 | `ODSTA` | R2  capa on-demand: emisora y precargas | gta_vc_browser/web/ondemand.js:805 | capa web (JS) | INFRA-WEB |
| 108 | `sphOwn=%d dSph=` | plan camara-coche-sin-lucha: la esfera de oclusion ya no choca con el  | src/core/Cam.cpp:2758, src/core/Cam.cpp:2932 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 109 | `sph=%d sphM=%d` | plan camara-coche-sin-lucha: CAMB2b/CAMB3b publican el impacto de las  | src/core/Cam.cpp:2758, src/core/Cam.cpp:2932, src/core/Cam.cpp:4630 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 110 | `base=%.2f obs=%d` | plan camara-coche-sin-lucha ve73: la distancia vuelve a maxDist si no  | src/core/Cam.cpp:2747 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 111 | `RECOIL_SHOT shotSeq=` | RC  recoil nuevo: cada disparo exitoso emite su diagnóstico (una línea | src/weapons/Weapon.cpp:553, src/weapons/Weapon.cpp:991 | VICEEXT_RECOIL | MOD-VIEJO |
| 112 | `RECOIL_APPLY shotSeq=` | RC  recoil nuevo: cada patada se aplica una vez a la cámara activa | src/core/Cam.cpp:251 | VICEEXT_RECOIL | MOD-VIEJO |
| 113 | `RECOIL_INPUT edge=` | RC  recoil nuevo: flancos press/release del gatillo con fuente | src/weapons/Weapon.cpp:374, src/weapons/Weapon.cpp:383 | VICEEXT_RECOIL | MOD-VIEJO |
| 114 | `RECOIL_CLASS shotSeq=` | RC  recoil nuevo: clasificación tap/ráfaga del SMG | src/weapons/Weapon.cpp:184, src/weapons/Weapon.cpp:524 | VICEEXT_RECOIL | MOD-VIEJO |
| 115 | `RECOIL_PERSIST reason=` | RC  recoil nuevo: persistencia sin retorno automático | src/core/Cam.cpp:222 | VICEEXT_RECOIL | MOD-VIEJO |
| 116 | `follow-ped-passive` | RC  recoil nuevo: FollowPed drena la cola (no retiene patadas) | src/core/Cam.cpp:1722 | VICEEXT_RECOIL | MOD-VIEJO |
| 117 | `OD_RECOIL_SCHEMA_1` | RC  recoil nuevo: esquema de telemetría acotada por sesión | src/weapons/Weapon.cpp:335, gta_vc_browser/web/lib/index.js:508, gta_vc_browser/web/lib/index.js:515 | VICEEXT_RECOIL | MOD-VIEJO |
| 118 | `serega=` | nado Serega: curva deriva/crucero/sprint+timeout en SWIM2 | src/peds/PlayerPed.cpp:4011 | VICEEXT_SWIMMING | NADO |
| 119 | `AIMCFG forceauto=` | AX  ClassicAXIS: los 9 ajustes de la seccion [ClassicAxis] leidos (una | src/core/Camera.cpp:494 | VICEEXT_AIM_CLASSICAXIS | MOD-VIEJO |
| 120 | `AIMWALK tecla=` | AX  ClassicAXIS: WalkKey, la tecla de andar (LALT) pone la velocidad a | src/peds/PlayerPed.cpp:6311 | VICEEXT_ALT_SIT +EM | MOD-VIEJO |
| 121 | `AIMLAW aim=` | AX  ClassicAXIS: flanco de isAiming con el modo de camara y el auto-ai | src/peds/PlayerPed.cpp:6060 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 122 | `AIMCAM m=` | AX  ClassicAXIS: la ley de apuntado: modo, dist 2.70, altura y hombro  | src/core/Cam.cpp:4582 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 123 | `AIMFOV fov=` | AX  ClassicAXIS: FOV 50 al apuntar por CLASE de arma (pesada: alcance  | src/core/Cam.cpp:4622 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 124 | `AIMCOL los=` | AX  ClassicAXIS: colisiones de la ley (LOS + 5 esferas, near-clip re-l | src/core/Cam.cpp:4630 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 125 | `AIMLOCK hx=` | AX  ClassicAXIS: lock-on, la ventana de 250 ms y por que se suelta | src/peds/PlayerPed.cpp:6124, src/peds/PlayerPed.cpp:6163, src/peds/PlayerPed.cpp:6177 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 126 | `AIMIK pitch=` | AX  ClassicAXIS: el brazo: pitch, torso, cabeza y brazo inferior | src/peds/PlayerPed.cpp:6232 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 127 | `AIHTRI h=` | AX  ClassicAXIS: triangulo sobre el objetivo blando de raton | src/renderer/Hud.cpp:749 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 128 | `AIMWPN block=` | AX  ClassicAXIS: cambio de arma bloqueado con el apuntado activo | src/peds/PlayerPed.cpp:1124 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 129 | `AIMHUD cruz=` | AX  ClassicAXIS: cruz del mod, marca de fijado por tipo y triangulo | src/renderer/Hud.cpp:730 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 130 | `AX7 tipo=` | AX  ClassicAXIS: el cuerpo sigue la marcha (WALKAROUND) y no la camara | src/peds/PlayerPed.cpp:2253 | VICEEXT_AIM_CLASSICAXIS | MOD-VIEJO |
| 131 | `P4 kind=identity schema=1` | P4  plan4: identidad version/dataTag (cadena EM_ASM del .js) | src/peds/PlayerPed.cpp:1581 | VICEEXT_AIM_CLASSICAXIS +EM | DIAGNOSTICO (andamiaje P4) |
| 132 | `P4 kind=move_begin schema=1` | P4  plan4: movimiento (policy, base y vector pedido) | src/peds/PlayerPed.cpp:1832 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 133 | `P4 kind=pose_begin schema=1` | P4  plan4: postura, los 9 IDs de PLAYERCROUCH | src/peds/PlayerPed.cpp:1935 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 134 | `P4 kind=input_edge schema=1` | P4  plan4: flancos de rueda (neutral/rearm/elegible) | src/peds/PlayerPed.cpp:1763, src/peds/PlayerPed.cpp:1774 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 135 | `P4 kind=roll_start schema=1` | P4  plan4: rueda por flanco y clip | src/peds/PlayerPed.cpp:4903 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 136 | `P4 kind=cam_begin schema=1` | P4  plan4: camara, vista presentada y transferencia | src/peds/PlayerPed.cpp:2127 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 137 | `P4 kind=service schema=1` | P4  plan4: ancla de luces de servicio por instancia | src/vehicles/Automobile.cpp:2017 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 138 | `P4 kind=overflow` | P4  plan4: buffer de traza desbordado (no PASS) | src/control/Script8.cpp:643, src/core/Camera.cpp:133, src/core/Pad.cpp:812 | VICEEXT_MOD_SCRIPT | DIAGNOSTICO (andamiaje P4) |
| 139 | `P4 kind=move_end schema=1` | P4  plan4: terminal de movimiento (speedTarget/speedReal/dirErrMax) | src/peds/PlayerPed.cpp:1642 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 140 | `P4 kind=sight schema=1` | P4  plan42: mira del mod por clase de arma al apuntar | src/renderer/Hud.cpp:350 | VICEEXT_WEAPON_SIGHTS +EM | MOD-VIEJO |
| 141 | `P4 kind=crouchspeed schema=1` | P4  plan42: objetivo y cadencia del agachado | src/peds/PlayerPed.cpp:5434 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 142 | `P4 kind=move_apply schema=1` | P4  plan42: que magnitud se aplico al movimiento | src/peds/Ped.cpp:1571, src/peds/Ped.cpp:1735 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 143 | `P4 kind=pose_exit schema=1` | P4  plan42: asociaciones del grupo de agachado tras desagacharse | src/peds/PlayerPed.cpp:1697 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 144 | `P4 kind=aim schema=1` | P4  plan43: por que se apunta o se deja de apuntar (why/sprint/jump/po | src/peds/PlayerPed.cpp:1754 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 145 | `P4 kind=jump schema=1` | P4  plan43: direccion del salto frente a la marcha (lr/ud/rumbo/dir) | src/peds/Ped.cpp:9758 | VICEEXT_AIM_CLASSICAXIS,VICEEXT_AIM_WALK +EM | MOD-VIEJO |
| 146 | `P4 kind=fire schema=1` | P4  plan43: puerta del gatillo sin apuntar (aim/gat/arma/hip) | src/peds/PlayerPed.cpp:2808 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 147 | `P4 kind=firewait schema=1` | P4  plan71-4c: espera previa al fuego con asociacion congelada o estad | src/peds/PedFight.cpp:736, src/peds/PedFight.cpp:780, src/peds/PedFight.cpp:855 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 148 | `P4 kind=armsight schema=1` | P4  plan43: pose de brazos aplicada al apuntar (pose/errDeg) | src/peds/PlayerPed.cpp:3065 | VICEEXT_AIM_WALK,VICEEXT_CROUCH +EM | MOD-VIEJO |
| 149 | `P4 kind=roll_step schema=1` | P4  plan50: avance MEDIDO de la rueda frame a frame (E1) | src/peds/PlayerPed.cpp:5320 | VICEEXT_CROUCH | MOD-VIEJO |
| 150 | `P4 kind=jump_end schema=1` | P4  plan50: salto con vector real y dirErr (E5) | src/peds/Ped.cpp:5877 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 151 | `P4 kind=stand_end schema=1` | P4  plan50: tramo de pie medido, apuntando y sin apuntar (E3) | src/peds/PlayerPed.cpp:3170 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 152 | `P4 kind=partials schema=1` | P4  plan50: parciales del arma al dejar de apuntar (E2) | src/peds/PlayerPed.cpp:5032 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 153 | `posenom=%s posepeso=%.2f aim=%d` | P4  plan50: NOMBRE del clip de las piernas y de la pose (E6) | src/peds/Ped.cpp:1789 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 154 | `pose=%s posepeso=%.2f errDeg=%.4f canon=%.4f px=%.1f` | P4  plan50: armsight continuo con peso, canon y pixeles (E4) | src/peds/PlayerPed.cpp:3065 | VICEEXT_AIM_WALK,VICEEXT_CROUCH +EM | MOD-VIEJO |
| 155 | `piesMps=%.2f` | P4  plan44: pies AL SUELO (ritmo del clip x su raiz) junto al avance r | src/peds/PlayerPed.cpp:5523, src/peds/PlayerPed.cpp:5624 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 156 | `pesosum=%.2f` | P4  plan44: suma de pesos de las animaciones de movimiento al desagach | src/peds/PlayerPed.cpp:1697 | VICEEXT_AIM_CLASSICAXIS +EM | MOD-VIEJO |
| 157 | `canon=%.4f` | P4  plan44: angulo entre el canon del arma y la direccion de la reticu | src/peds/PlayerPed.cpp:3065 | VICEEXT_AIM_WALK,VICEEXT_CROUCH +EM | MOD-VIEJO |
| 158 | `det=%s` | P4  plan44: detalle de la retirada de la pose (pesos y blendDelta por  | src/peds/PlayerPed.cpp:4974 | VICEEXT_CROUCH +EM | MOD-VIEJO |
| 159 | `ViceExtCrouchBlend` | P4  plan44: un solo reloj para agacharse y desagacharse (altura y pose | src/core/Cam.cpp:1780, src/core/Cam.cpp:6230, src/peds/PlayerPed.cpp:5106 | VICEEXT_CROUCH | MOD-VIEJO |
| 160 | `P4 kind=legs schema=1` | P4  plan41: piernas sin doble angulo (bodyYaw/legYaw/legMis/skip) | src/peds/Ped.cpp:1789 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 161 | `P4 kind=hud_cross schema=1` | P4  plan41: reticula con la ley de apuntado (draw/pc/law/lock) | src/renderer/Hud.cpp:446 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 162 | `P4 kind=turners schema=1` | P4  plan41: intermitentes (motivo/steer/anchorErr) | src/vehicles/Automobile.cpp:2043 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 163 | `P4 kind=polbike schema=1` | P4  plan41: moto policial con un solo ocupante (occ/passengers) | src/control/CarAI.cpp:577 | VICEEXT_POLICE_BIKE +EM | MOD-VIEJO |
| 164 | `P4 kind=limits schema=1` | P4  plan9.0: limites del mod (numpeds/numvehicles/mult) | src/peds/Population.cpp:373 | VICEEXT_LIMITS +EM | MOD-VIEJO |
| 165 | `P4 kind=radio2 schema=1` | P4  plan9.1(1)+10.0-C: icono de emisora al cambiar (emisora/icono) | src/audio/MusicManager.cpp:1489 | VICEEXT_RADIO_ICONS +EM | RADIO |
| 166 | `P4 kind=shift schema=1` | P4 plan9.1(3): Shift+tecla en avisos (dos iconos) | src/text/Messages.cpp:563 | VICEEXT_HINT_KEYS | MOD-VIEJO |
| 167 | `P4 kind=cheat2 schema=1` | P4  plan9.1(4)+10.0-C: trucos nuevos del mod (cheat/efecto) | src/core/Pad.cpp:809 | VICEEXT_EXTRA_CHEATS | MOD-VIEJO |
| 168 | `P4 kind=uzi schema=1` | P9.1(5): flanco de disparo Micro-UZI (medida previa al fix) | src/weapons/Weapon.cpp:609, src/weapons/Weapon.cpp:921, src/weapons/Weapon.cpp:1613 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 169 | `P4 kind=cardet schema=1` | P4  plan10.0-B: rueda reventada/desprendida (paso/rueda/impulso) | src/vehicles/Automobile.cpp:4963, src/vehicles/Automobile.cpp:4992 | VICEEXT_WHEEL_DETACH | MOD-VIEJO |
| 170 | `P4 kind=cardead schema=1` | P4  plan10.0-B: muerte NPC en choque duro (muertos/impulso) | src/vehicles/Automobile.cpp:4983 | VICEEXT_CAR_DEATH,VICEEXT_WHEEL_DETACH | MOD-VIEJO |
| 171 | `P4 kind=carluz schema=1` | P4  plan10.0-B: faro roto por impacto (luz/impulso) | src/vehicles/Automobile.cpp:5055 | VICEEXT_CAR_LIGHTS | MOD-VIEJO |
| 172 | `P4 kind=ini schema=1` | P4  plan9.2: opciones del features.ini del mod (opt/flag/a/val) | src/core/Radar.cpp:1140, src/peds/Ped.cpp:81, src/peds/PedAI.cpp:5668 | VICEEXT_GPS_COLOR +EM | MOD-VIEJO |
| 173 | `P4 kind=inifile schema=1` | 11  familia 11: su features.ini leido al arrancar (clave/ini/aplica/or | src/core/ViceExtFeatures.cpp:230, src/core/ViceExtFeatures.cpp:238 | VICEEXT_FEATURES_INI +EM | MOD-VIEJO |
| 174 | `P4 kind=worlddata schema=1` | 7   familia 7: mundo del mod ingerido (zonas de oclusion del occlu.ipl | src/core/Game.cpp:747 | VICEEXT_MAPDATA +EM | MOD-VIEJO |
| 175 | `P4 kind=modscript schema=1` | 13  script del mod: los opcodes propios 0x0FA0-0x0FA9 en el interprete | src/control/Script8.cpp:694, src/control/Script8.cpp:706, src/control/Script8.cpp:717 | VICEEXT_MOD_SCRIPT | MOD-VIEJO |
| 176 | `ViceExtRotateSteeringWheel` | plan12.0: volante que gira (v2.5) - la pieza del mod sigue la direccio | src/vehicles/Automobile.cpp:2068, src/vehicles/Automobile.cpp:3345, src/vehicles/Automobile.h:149 | VICEEXT_CAR_ROTATE_WHEEL | MOD-VIEJO |
| 177 | `steeringwheel` | plan12.0: volante que gira (v2.5): el dummy que traen 18 modelos del g | src/vehicles/Automobile.cpp:2064 | VICEEXT_CAR_ROTATE_WHEEL | MOD-VIEJO |
| 178 | `P4 kind=wheelrot schema=1` | P4  plan12.0: volante que gira (v2.5): angulo aplicado vs direccion | src/vehicles/Automobile.cpp:2090 | VICEEXT_CAR_ROTATE_WHEEL +EM | MOD-VIEJO |
| 179 | `ViceExtAimSensMouseX` | plan12.0: sensibilidad de apuntado (2506) - par de mirar vs par de apu | src/core/Cam.cpp:1847, src/core/Cam.cpp:2559, src/core/Cam.cpp:3312 | SIN GUARDA | MOD-VIEJO |
| 180 | `MouseAimSensX` | plan12.0: sensibilidad de apuntado (2506): las ocho claves de su [Cont | src/core/Camera.h:374, src/core/re3.cpp:536, src/core/re3.cpp:662 | SIN GUARDA | MOD-VIEJO |
| 181 | `P4 kind=aimsens schema=1` | P4  plan12.0: sensibilidad de apuntado (2506): factor aim/look aplicad | src/core/Camera.cpp:129 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 182 | `P4 kind=moon schema=1` | P4  plan12.0: luna movil (v1.5): el disco sale del rumbo del sol | src/renderer/Clouds.cpp:162 | VICEEXT_MOON +EM | MOD-VIEJO |
| 183 | `P4 kind=distlights schema=1` | P4  plan12.0: luces lejanas (v2.6): edificios y dummies con efectos 2D | src/core/World.cpp:1980 | __EMSCRIPTEN__ (sin VICEEXT) | MOD-VIEJO |
| 184 | `P4 kind=gangaim schema=1` | P4  plan12.0: punteria de la IA (v1.0/2506): peds 73 y polis del mod | src/peds/CopPed.cpp:105, src/peds/Ped.cpp:391 | VICEEXT_GANG_AIM +EM | MOD-VIEJO |
| 185 | `P4 kind=partcfg schema=1` | 3/5 particulas: particle.cfg con el tipo resuelto por NOMBRE | src/renderer/ParticleMgr.cpp:225, src/renderer/ParticleMgr.cpp:241, src/renderer/ParticleMgr.cpp:436 | VICEEXT_PARTICLE_NAMES | MOD-VIEJO |

### 2. `viceext-log-check.py` (4496 líneas)

**Propósito:** verificador que solo lee el log `web/odtrace.log` (o el que se le pase). Tiene un bloque por mecánica y da OK / PARCIAL / SIN DATOS / FALLO (`main()` en `:4403`). Con `--plan-4` evalúa los eventos `P4 kind=… schema=1` con su esquema estricto (`P4_KINDS` en `:2964`, `P4_REQ` en `:2982`, `p4_main` en `:4357`). `main()` solo da exit 0 si D2, D4, D5, D6, D7 y D8 están en OK (`:4471`). Los bloques R5…AX solo cuentan como fallo si salen FALLO con datos (`:4483`); si no hay trazas salen "SIN DATOS". Lo invoca `vc-test.mjs:96-99`, que solo filtra las líneas del resumen (`^\s{3}\S+\s+(OK|FALLO|SIN DATOS|PARCIAL)`).

| Bloque (def) | Etiquetas que parsea | Emisor en src | Categoría |
|---|---|---|---|
| D2 `bloque_d2` :144 | `TXTMISS key=`, nombres de coche del mod (`HELLENB`, `STRTFTR`, …) | `src/text/Text.cpp:156` | MOD-VIEJO (textos/coches del mod) y DIAG |
| D4 `bloque_d4` :163 | `CARPED`, `CARPOOL`, `CARBLOCK`, `CARSPAWN model=`, `CARLOAD model=`, `CARFAIL`, `CARZONE`, `CARRATE`, `ODSHORT open-fail`, `ODSRECOVER` | `src/core/Streaming.cpp:1922,1931,375,1706,2429`; `src/control/CarCtrl.cpp:667,804,831`; `src/core/CdStreamPosix.cpp:139,140,173` | Mezcla: tráfico y streaming web = INFRA-WEB/DIAG; los ids de coche del mod (`VEH_MOD 6500-6507`, `:96`) = MOD-VIEJO |
| D5 `bloque_d5` :298 | `IFPFILE <ruta> clips=` (espera 272/274 clips del `ped.ifp` del mod) | `src/animation/AnimManager.cpp:1443` | MOD-VIEJO (carga de anims del mod). El NADO necesita esos clips: es dependencia |
| D6 `bloque_d6` :321 | `SIGHTS cargadas=`, `SIGHT arma=` | `src/renderer/Hud.cpp:2195,328` | MOD-VIEJO (`VICEEXT_WEAPON_SIGHTS`) |
| D7 `bloque_d7` :345 | `KEYICONS`, `HINTKEY accion=`, `P4 kind=shift` | `src/renderer/Hud.cpp:2206`, `src/text/Messages.cpp:549,563` | MOD-VIEJO (`VICEEXT_HINT_KEYS`) |
| J1 `bloque_j1` :375 | `JSERR`, `JSERR_REJ`, `ENGERR`; `LODLEFT model=` | `web/lib/index.js:610-625`; `src/renderer/Renderer.cpp:867` | INFRA-WEB (errores); LODLEFT = MOD-VIEJO/DIAG (`VICEEXT_LOD_EXTRA`, D10) |
| R2 `bloque_r2` :408 | `FPHASE`, `ODSTA`, `ODWRITE`, `STREAM preload`, MEMFS | `src/core/main.cpp:902`; `web/ondemand.js:805,828`; `src/audio/sampman_oal.cpp:2252` | INFRA-WEB (streaming de emisora `.adf` y tirones; no son los iconos de radio) |
| R3 `bloque_r3` :469 / R3b `bloque_r3b` :738 | `SCRTXT`, `SCRTXT3 x=` | `src/renderer/Hud.cpp:2384`, `src/text/Messages.cpp:52`, `src/renderer/Font.cpp:994` | DIAGNOSTICO (caza-textos en pantalla) |
| D8 `bloque_d8` :503 | `VICEEX sfx arma=`, `ODSFXMISS sfx=`, `CHINIT … sfx=` | `src/audio/AudioLogic.cpp:4700`, `src/audio/sampman_oal.cpp:480,2065` | MOD-VIEJO (sonidos de las armas del mod), sobre infra de sampman |
| R5 `bloque_r5` :548 | `VICEEXT 1p key`, `1p set puerta= … tog=` | `src/core/Camera.cpp:1323,1445,1492` | MOD-VIEJO (`VICEEXT_FIRST_PERSON`) |
| R9 `bloque_r9` :571 | `AIMDIR arma=` | `src/peds/PlayerPed.cpp:3111` | MOD-VIEJO (apuntado) |
| R11 `bloque_r11` :605 | `SVLIGHTS model=` (`var=`, `extras=`) | `src/vehicles/Automobile.cpp:2135,2233` | MOD-VIEJO (luces de servicio) |
| LB `bloque_lbar` :656 | `LBAR w=` | `src/core/main.cpp:1044` (solo `__EMSCRIPTEN__`) | MOD-VIEJO (SilentPatch `VICEEXT_LOADBAR_BORDER_*`) |
| SR `bloque_sirena` :691 | `SIRENA tipo=` | `src/audio/AudioLogic.cpp:2944` (solo `__EMSCRIPTEN__`) | MOD-VIEJO (sirena de la moto policial) |
| L2 `bloque_icon` :715 | `HUDICON arma=` | `src/renderer/Hud.cpp:906` | MOD-VIEJO (iconos de las armas del mod) |
| L6 `bloque_l6` :1025 | `LOADSCR begin/clamp/end` | `src/core/main.cpp:1072,1008,1081` | INFRA-WEB (pantalla de carga web) |
| **H** `bloque_h` :763 | `CROUCH2 h=`, `PEDAT camz=`, **`SWIM2 move`**, **`SWIM2 exit`**, **`SWIMIDLE`** | `PlayerPed.cpp:5624` (agachado), `:5783` (PEDAT, bajo `VICEEXT_CROUCH`), `:4011,3727,3739` (`VICEEXT_SWIMMING`) | **NADO** (H2) + MOD-VIEJO (H1, agachado) |
| R12 `bloque_r12` :972 | `VICEEXT reload …`, `VICEEXT crouch …` (`repetido=`) | `PlayerPed.cpp:3444-3540,5163` | MOD-VIEJO |
| R13 `bloque_r13` :895 | `WINFO arma=`, `SIGHT arma=` | `PlayerPed.cpp:266` (solo `__EMSCRIPTEN__`), `Hud.cpp:328` | MOD-VIEJO (armas del mod) |
| **R14** `bloque_r14` :1058 | `RECOIL3 miracheck`, **`SWIMCAM objetivo=`**, **`VICEEXT swim`**, `ODSFXPROT sfx=`, `ODSFXDEFER` | `Weapon.cpp:412`; `Cam.cpp:1806,6254` (`VICEEXT_SWIMMING`); `PlayerPed.cpp:3657…`; `sampman_oal.cpp:1856,349` | **NADO** (cámara de nado) + MOD-VIEJO (recoil, sfx) |
| AX `bloque_ax` :1131 | `AIMCFG`, `AIMWALK`, `AIMLAW`, `AIMCAM`, `AIMFOV`, `AIMCOL`, `AIMVEC`, `AIMDIR`, `AX7 tipo=` (AX0-AX8 son subapartados del propio verificador) | `Camera.cpp:494`; `PlayerPed.cpp:6311,6060,2253,3111`; `Cam.cpp:4582,4622,4630,4607` | MOD-VIEJO (`VICEEXT_AIM_CLASSICAXIS`) |
| R16 `bloque_r16` :1438 | `R3P arma=` | `PlayerPed.cpp:2654` | MOD-VIEJO (`VICEEXT_ROCKET_3RD_PERSON`) |
| R15 `bloque_r15` :1482 | `CROUCHPOSE desde=`, `VICEEXT crouch` | `PlayerPed.cpp:5067` | MOD-VIEJO |
| **R17** `bloque_r17` :1543 | `CROUCH2`, `VICEEXT crouch`; **`SWIM2 move`** (`camdist=`, `modo=`, `velo=`, `hondo=`, `glide=`, `pie=`/`col=`), **`SWIMCAM no-fallen-water`**, **`SWIMEF`**, **`SWIMFOV fov=`**, **`SWIMHUD cruz=`**, **`PEDCLAIM cap= carril= de= a= ok=`** | `PlayerPed.cpp:5624`; `PlayerPed.cpp:4011,3933,3988`; `Cam.cpp:1250`; `Hud.cpp:433` (solo `__EMSCRIPTEN__`); `src/core/PedArbiter.cpp:50,60,87` | **NADO** + MOD-VIEJO (agachado). PEDCLAIM pertenece al árbitro, del que depende el nado |
| **R19** `bloque_r19` :1977 | `CROUCH2 h=`, `PEDAT`, **`SWIM2 move`** | `PlayerPed.cpp:5624,5783,4011` | **NADO** + MOD-VIEJO |
| RC `bloque_recoil` :2109 | `RECOIL_SHOT`, `RECOIL_APPLY`, `RECOIL_INPUT`, `RECOIL_CLASS`, `RECOIL_PERSIST`, `RECOIL_CONTROL`, `RECOIL_RESET`, `RECOIL_WEAPON_CHANGE`, `RECOIL_QUEUE_OVERFLOW`, `RECOIL_DIAG(_END)`, `RECOIL2`/`RECOIL3`, sobre `RECOIL session` | `Weapon.cpp:553,374,184,460,347`; `Cam.cpp:251,222,127,105`; `web/lib/index.js:505-522` | MOD-VIEJO (`VICEEXT_RECOIL`) |
| R27 `bloque_r27` :2268 | `CROUCH2 h=`/`con`/`sin`, `CROUCHMOVE fin`, `VICEEXT crouch`, `WEAPON_crouch` | `PlayerPed.cpp:5523,5624,4974-5397` | MOD-VIEJO |
| B `bloque_build` :2681 | `JS build=` y `VERSION` de `web/lib/index.js` | `web/ondemand.js:378` | INFRA-WEB |
| S13 `bloque_modscript` :2688 | `P4 kind=modscript` | `src/control/Script8.cpp:694-726` | MOD-VIEJO (`VICEEXT_MOD_SCRIPT`) |
| P35 `bloque_partcfg` :2724 | `P4 kind=partcfg` | `src/renderer/ParticleMgr.cpp:225,241,436` | MOD-VIEJO (`VICEEXT_PARTICLE_NAMES`) |
| VW :2758 / AW :2791 / LM :2829 / LL :2869 / GA :2927 | `P4 kind=wheelrot` / `aimsens` / `moon` / `distlights` / `gangaim` | `Automobile.cpp:2090`; `Camera.cpp:129`; `Clouds.cpp:162`; `World.cpp:1980`; `CopPed.cpp:105`, `Ped.cpp:391` | MOD-VIEJO |
| AS `bloque_altsit` :2898 | `AIMWALK tecla=` | `PlayerPed.cpp:6311` | MOD-VIEJO (`VICEEXT_ALT_SIT`/`AIM_WALK`) |
| UZI `bloque_uzi` :3930 | `P4 kind=uzi`, `WINFO arma=` | `Weapon.cpp:609,921,1613,1984` | MOD-VIEJO |
| `--plan-4` P4.1-P4.9 (`p4_1` :3279 … `p4_9` :3839) | `move_*`, `pose_*`, `input_*`, `roll_*`, `cam_*`, `service`, `pose_exit`, `armsight`, `fire`, `legs`, … | `PlayerPed.cpp` (`:1581` identity, `:1642`, `:1697`, `:1754-1832`, `:1935-2170`, `:3065`, `:4843-4903`), `Ped.cpp:1571,1735,1789`, `Automobile.cpp:2017` | MOD-VIEJO y DIAG (andamiaje P4 de agachado/apuntado/cámara) |
| P4.10 `p4_10` :3979 | `kind=limits` | `src/peds/Population.cpp:373` | MOD-VIEJO (`VICEEXT_LIMITS`) |
| **P4.11** `p4_11` :4001 | **`kind=radio` / `kind=radio2`** (`emisora=`, `icono=`; FALLO si `icono=-1` con emisora 0..8, "textura ausente en radio.txd") | `src/audio/MusicManager.cpp:1489` (`VICEEXT_RADIO_ICONS`) | **RADIO** |
| P4.12 `p4_12` :4028 | `kind=cheat2` (AEZAKMI, AIRWAYS, BIGHEADS, BIGSMOKE, IAMINVINCIBLE, RCROCKET) | `src/core/Pad.cpp:809` | MOD-VIEJO (`VICEEXT_EXTRA_CHEATS`) |
| P4.13 `p4_13` :4284 | `kind=cardet/cardead/carluz` | `Automobile.cpp:4963,4983,5055` | MOD-VIEJO |
| P4.14 `p4_14` :4059 / P4.15 `p4_15` :4143 | `kind=ini` / `kind=inifile` | `Radar.cpp:1140`, `Ped.cpp:81`, `PedAI.cpp:5668…` / `src/core/ViceExtFeatures.cpp:230,238` | MOD-VIEJO (features.ini) |
| P4.16 `p4_16` :4260 | `kind=worlddata` | `src/core/Game.cpp:747` | MOD-VIEJO (`VICEEXT_MAPDATA`) |
| `p4_identidad` :4333, `p4_main` :4357 | `kind=identity`, `kind=overflow` | `PlayerPed.cpp:1581` (bajo `VICEEXT_AIM_CLASSICAXIS +EM`), macros `VICEEXT_P4_EMIT` en 16 sitios | DIAGNOSTICO (andamiaje P4) |

### 3. `vc-test.mjs` (193 líneas)

**Propósito:** lanzador de pruebas. `--estatico` hace tres cosas: llama a `motorServido()`, que ejecuta `check-served-build.sh` (`:56-62`); compila los defines apagados (`:64-84`, hoy solo `VICEEXT_TURN_SIGNALS` → `src/vehicles/Automobile.cpp` mediante `check-define-build.sh`, buscando `^//#define VICEEXT_TURN_SIGNALS` en `config.h`); y pasa `viceext-log-check.py` sobre el último log (`:86-131`). Lo que consume: las marcas (vía check-served-build); `JS build=` del log (`:90`); y las líneas de resumen OK/FALLO/SIN DATOS/PARCIAL del verificador (`:96-99`). Sin argumentos ejecuta los escenarios de `harness/esquemas.mjs` en una sola sesión y, antes, exige las `marcas` de cada escenario (`:138-139`).

| Lo que consume | Línea | Emisor | Categoría |
|---|---|---|---|
| salida de `check-served-build.sh` (`OK: …`, `FALTA`) | 56-62 | (tabla §1) | mixta |
| `VICEEXT_TURN_SIGNALS` apagado en config.h | 71-83 | `src/core/config.h` | MOD-VIEJO |
| `JS build=` | 90 | `web/ondemand.js:378` | INFRA-WEB |
| resumen de `viceext-log-check.py` | 96-99 | (tabla §2) | mixta |
| `marcas` de los escenarios | 138-139 | (§5) | mixta |

### 4. `harness/vc-harness.mjs` (586 líneas)

**Propósito:** librería del arnés. Abre un Chrome con puppeteer, siembra la partida en IDBFS, recorre el menú, espera a estar en partida y deja leer la traza de la sesión (`web/odtrace-<nombre>.log`, vía `&trace=<nombre>`, `:283,299`).

| Lo que consume | Línea | Emisor | Categoría |
|---|---|---|---|
| `check-served-build.sh` → `FALTA (\S+)`, `dataTag servido:` | 191-203, 233-241 (la sesión aborta si falta una marca de las `marcas` pedidas) | §1 | mixta |
| `VERSION = '…'` de `web/lib/index.js`; `window.__vcVersion` | 200-201, 453 | `web/lib/index.js:636` | INFRA-WEB |
| `#gamelog` → `state N` (7 = menú, 8/9 = carga/partida) | 334-348, 411, 428 | `web/lib/index.js:344` (elemento `#gamelog`) | INFRA-WEB |
| `WR load-req` (el menú pidió cargar la partida) | 409 | `src/core/Frontend.cpp:5612` (solo `__EMSCRIPTEN__`) | INFRA-WEB/DIAG (hay que conservarla) |
| `FPHASE` (el motor está pintando frames) | 444 | `src/core/main.cpp:902` (solo `__EMSCRIPTEN__`) | INFRA-WEB/DIAG (hay que conservarla) |
| `JS build=` | 458 | `web/ondemand.js:378` | INFRA-WEB |
| `Traza.lineas(tag)` y `campo(l, nombre)`: genéricos, sin etiquetas fijas | 97-158 | — | — |

### 5. `harness/esquemas.mjs` (1093 líneas): escenarios

| Escenario (línea) | `marcas:` obligatorias | Trazas y campos que lee | Emisor | Categoría |
|---|---|---|---|---|
| helper `navegarAlAgua` :42-104 | — | `PEDAT` (`x`, `y`, **`mar`** = azimut al agua, **`dist`** = distancia al agua), `VICEEXT swim enter` | `PlayerPed.cpp:5783` (**bajo `VICEEXT_CROUCH`**); `PlayerPed.cpp:3811` (`VICEEXT_SWIMMING`) | NADO (el arnés), pero con dependencia de código viejo de agachado |
| `carga` :113 | `PEDAT` | `FPHASE`, `PEDAT` (`ms`, `ts`), `FPSLOG` (`avg`, `maxdelta`), versión `\d{4}-\d\d-\d\d-ve\d+` | `main.cpp:902,953`, `web/lib/index.js:770`, `PlayerPed.cpp:5783` | INFRA-WEB. Ojo: exige PEDAT, que está bajo `VICEEXT_CROUCH` |
| `agachado` :146 | `CROUCH2`, `medida=1x`, `CROUCHPOSE`, `pesomira=%.2f`, `veloc=%.2f`, `rueda=%d`, `moved=%.2f`, `ViceExtCrouchMoveSpeed`, `Crouch_Roll_L` | `CROUCH2` (`nom`, `avance`, `velo`, `rueda`, `mira`, `moved`, `camz`, …), `PEDAT`, `CROUCHPOSE`, `VICEEXT crouch roll lado=`, `VICEEXT crouch pose off clips=`, `crouch pose off`; trucos `CRAZYHINT` y `CRAZYPISTOL` (:472-473) | `PlayerPed.cpp:5624,5067,5347,4974`; trucos en `src/core/Pad.cpp` | MOD-VIEJO |
| **`nado`** :739 | **`SWIM2`, `SWIMCAM`** | `SWIM2 … move` (`velo`, `dz`, `hondo`, `modo`, `camdist`), `SWIMCAM objetivo=… nivel=`, `SWIMCAM no-fallen-water`, `SWIMCAM … modo=23`; usa `navegarAlAgua` (PEDAT) | `PlayerPed.cpp:4011`; `Cam.cpp:1250,1806,6254` | **NADO** |
| `armas` :818 | `WINFO arma=`, `WLOAD arma=` | `HINTTEST mostrado=1`, `WLOAD` (`arma`, `cargado`, `txd`, `txdCargado`), `WINFO` (`arma`, `clips=`), `FPHASE`; trucos `CRAZYHINT` y `CRAZYTOOLS` (:828,835) | `src/core/Pad.cpp:460,342` (solo `__EMSCRIPTEN__`), `PlayerPed.cpp:266` | MOD-VIEJO (armas del mod y trucos de diagnóstico) |
| `agachado-mira` :937 | `pesomira=%.2f`, `moved=%.2f`, `VICEEXT crouch roll lado=`, `Crouch_Roll_L` | `CROUCH2`, `PEDAT`, `VICEEXT crouch roll lado=` | `PlayerPed.cpp` | MOD-VIEJO |

Fuera del encargo, pero también consumen trazas viejas: `tools/crouch-swim-smoke-test.mjs`, `tools/driveby-log-check.mjs` y `tools/hidecops-log-check.mjs` (los tres buscan `JS build=`).

### 6. Qué se rompe al quitar el código y las trazas viejas (conservando nado y radio)

1. **check-served-build.sh da FALLO casi seguro.** 132 de las 161 marcas son MOD-VIEJO y saldrían `FALTA`. También las 2 de andamiaje P4 si se quitan las macros `VICEEXT_P4_EMIT`. Los DATOS (`:192-200`) y el `COLFILE` (`:311`) siguen pasando mientras se sirvan los ficheros del mod. Hay que podar `MARCAS` hasta dejar NADO (19 + `camz=`), RADIO (`P4 kind=radio2`), INFRA-WEB (`LOADSCR begin`, `ODSTA`) y lo que se reimplemente. Si se quita el andamiaje P4, la traza de radio (`P4 kind=radio2 …` en `MusicManager.cpp:1489`) tiene que conservar ese formato o hay que cambiar la marca `:165` y `p4_11`.
2. **Árbitro (PedArbiter).** Las marcas `PEDCLAIM cap=` (`:63`) y `ViceExtPedOwns` (`:66`), y el apartado PEDCLAIM de R17, salen de `src/core/PedArbiter.cpp` y de llamadas SIN GUARDA (`Script6.cpp:1008`, `Cam.cpp:653-654`, …). Si el nado sigue usando el árbitro, hay que conservarlo; si no, se quitan esas marcas.
3. **PEDAT está bajo `VICEEXT_CROUCH`** (`PlayerPed.cpp:5783`). Al quitar el agachado viejo se rompen el escenario `carga` (lo exige como marca) y `navegarAlAgua` (usa `mar`/`dist`), y por tanto el escenario `nado`, además de H1/R19 del verificador y las marcas `:73` y `:75`. Hay que sacar PEDAT del bloque de agachado (a NADO o a INFRA-WEB) antes de borrar.
4. **`camz=` (`:37`)** la emiten a la vez SWIM2 (nado) y CROUCH2/PEDAT. Seguirá presente mientras quede SWIM2.
5. **Bloques mixtos del verificador:** H, R14, R17 y R19 mezclan agachado y nado. Sin el agachado, la parte de agachado sale "SIN DATOS" (no tumba el exit), pero R17 y H leen `CROUCH2` y `PEDAT` como contexto. Conviene partirlos en un bloque "nado" propio.
6. **`main()` del verificador** exige OK en D2, D4, D5, D6, D7 y D8. D6 (miras), D7 (iconos de tecla) y D8 (sfx de armas) son MOD-VIEJO: si se quitan, el verificador acaba con exit 1. `vc-test` no mira el exit code, solo las líneas con FALLO, así que no suspende por "SIN DATOS". D5 (`IFPFILE` 272/274 clips) mide la carga del `ped.ifp` del mod, del que dependen los clips de nado.
7. **INFRA-WEB que el arnés necesita y que vive en ficheros con mucho código viejo:** `FPHASE`/`FPSLOG` (`main.cpp:902,953`), `WR load-req` (`Frontend.cpp:5612`), `LOADSCR` (`main.cpp`) y `JS build=` (ondemand.js). Hay que conservarlas al limpiar `main.cpp` y `Frontend.cpp`.
8. **Escenario `armas` y trucos de diagnóstico:** `CRAZYHINT` → `HINTTEST`, `CRAZYTOOLS` y `WLOAD` en `Pad.cpp` son MOD-VIEJO solo tras `__EMSCRIPTEN__`. Al quitarlos, el escenario `armas` aborta por las marcas `WINFO arma=`/`WLOAD arma=`. Los escenarios `agachado` y `agachado-mira` abortan entero.
9. **`vc-test --estatico`** compila `VICEEXT_TURN_SIGNALS` apagado contra `Automobile.cpp` (`:71`). Si la bandera desaparece de `config.h`, el paso queda en "ya está encendido" o tiene que quitarse.
10. **Sobre RECOIL en la capa web** (`web/lib/index.js:505-522`, `OD_RECOIL_SCHEMA_1`). Es código web que solo sirve al recoil viejo (MOD-VIEJO dentro de INFRA-WEB). Si no se toca, queda inerte.
