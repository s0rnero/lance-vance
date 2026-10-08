---
name: 05-hallazgos-4a-partida
status: EXECUTED
type: research
domain: gameplay
owner_rules: .agents
created: 2026-09-20
---

# Hallazgos de la 4ª partida (20/09) — reparto y plan

Este fichero lo leen las **3 secciones**. Cada una corrige lo suyo y, si algo de
la lista no es suyo, lo dice aquí (o en su sección "Estado") en vez de tocarlo.

Método vigente: **nada de sondas de Chrome** (el jugador lo pidió: le saturan la
CPU). Todo se dictamina leyendo la traza de una partida real
(`gta_vc_browser/web/odtrace.log`) + el código.

## Evidencia leída

- `web/odtrace.log`, sesión **18:42:35 → 18:47:31Z** (5.524 líneas), build
  `2026-09-20-ve12`, datos `ve10`. Carga menú → `GTAVCsf4.b` → `GTAVCsf2.b`.
- La traza **se corta en medio de un frame normal** (últimos `FPHASE`,
  `CORONAREND` y `JS ASYNC` completos, sin línea de error): eso es un **cuelgue
  duro**, no un cierre ordenado.
- `cut=0` en toda la sesión (no llegó a entrar en cinemática ahí) y `WANTED lvl`
  máximo **1** (tampoco las 3 estrellas). Los dos cuelgues que reporta el
  jugador son de otra tanda: **antes no dejaban motivo**, desde hoy sí (J1).
- `JS ODWK … peticiones=1565 aplazados=1385 entregados=1565 escritos=1565 red=58
  err=0`: el 88 % de los ficheros on-demand se **aplazan** (fast-fail) y se
  entregan después; 0 errores de red. Es el camino de siempre, no un fallo.

## J0. Dato nuevo y gordo: las animaciones de las mecánicas YA se sirven

Parseado `gta_vc_browser/streamed/anim/ped.ifp` (el del mod: 272 clips, es el que
va dentro del `.data` servido desde D5). Contiene, literalmente:

| Mecánica | Clips disponibles en el `ped.ifp` servido |
|---|---|
| Nadar | `Swim_Breast`, `Swim_Crawl`, `Swim_Tread`, `Swim_jumpout`, `Drown` |
| Agachado | `Crouch_Idle`, `Crouch_Forward`, `Crouch_Backward`, `Crouch_Roll_L`, `Crouch_Roll_R`, `DUCK_down`, `DUCK_low`, `WEAPON_crouch` |
| Correr armado | `sprint_armed`, `sprint_rocket`, `sprint_csaw`, `sprint_civi`, `sprint_panic` |
| Escalada | `CLIMB_idle`, `CLIMB_jump`, `CLIMB_jump_B`, `CLIMB_jump2fall`, `CLIMB_Pull`, `CLIMB_Stand`, `CLIMB_Stand_finish` |

No hay que traer arte nuevo: hay que **asociar** esos clips. Los grupos de
animación están **hardcodeados** en `src/animation/AnimManager.cpp`
(`enum` en `src/animation/animmanager.h`, tabla de nombres —ahí está `"DUCK_down"`,
línea ~456— y `aStdAnimDescs`); el grupo nuevo (`Swim`/`Crouch`/`CLIMB`) se añade
ahí con sus `ASSOC_*`. Los nombres son los de **San Andreas**, así que los layouts
de SA (`Swim_*`, `Crouch_*`, `CLIMB_*`) sirven de referencia directa.

## J1. Los cuelgues ya dejan motivo en la traza (hecho, sección 1)

`web/lib/index.js` sólo mandaba el motivo del fallo a **la consola de la página**
(`[FATAL] …`), que se pierde. Ahora el texto se encola en la misma cola de trazas
(`JSERR`, `JSERR_REJ`, `ENGERR`) y se envía **de forma síncrona** antes de que la
página muera; el latido `FPSLOG` (1/s) que falte después marca el instante exacto.

- `JSERR <stack>`: excepción no capturada (incluye el `RuntimeError: Aborted(...)`
  de emscripten).
- `JSERR_REJ <motivo>`: promesa rechazada sin capturar.
- `ENGERR <texto>`: cualquier `console.error` con `Aborted|RuntimeError|Out of
  memory|Cannot enlarge memory|unreachable`.

**Para las secciones 2 y 3:** cuando el jugador diga "se crasheó tal cosa", la
respuesta está en `odtrace.log` en la última línea antes del corte. No hace falta
sonda.

## J2. Reparto (lo que él pide → quién)

| # | Reporte del jugador | Evidencia en el log / código | Dueño | Qué hay que hacer |
|---|---|---|---|---|
| 1 | Ninguna cinemática carga y el juego **crashea** | `cut=0` toda la sesión; los actores de cinemática (`vice1`, `vice2`, `vicechee`, `chopper`) se piden a las 18:46:29 y **se aplazan** (`ODSHORT open-fail … vice1.txd`) | 3 (CutsceneMgr) + 1 (capa on-demand) | Comprobar que la carga de cinemática va por el camino **bloqueante** (`odBlockingPush/Pop` + `LoadAllRequestedModels`): si pide por el camino normal, la capa on-demand **aplaza** y la escena sale vacía. El motivo del cuelgue lo dirá J1. |
| 2 | Cuelgue con **3 estrellas** | `WANTED` llega sólo a `lvl=1` en esta traza | 2 (P1) | Reproducir y mirar la última línea antes del corte (J1). |
| 3 | Texturas de **LOD** que no se actualizan de cerca (calles, muros, Malibú…) | No hay `FAIL model/txd` ni `TEXA … useA=0`; `[texconv] clamp 9->7` sólo recorta **mips pequeños** (nivel 0 intacto) ⇒ no es conversión de textura | 1 (D10) | `LODLEFT` nuevo (ver abajo) decide entre "el modelo real no cargó" y "su alfa no llegó a 255". |
| 4 | No hay **cámara en 1ª persona** con `V` | 7 líneas `CAM1P … tog=0` ⇒ el conmutador **no se disparó ni una vez** en toda la sesión | 3 (C1) | Ver nota C1 abajo: la tecla SÍ está bindeada (`'V'`) y el diagnóstico nuevo (`VICEEXT 1p key …`) **no estaba en el build que jugó** (`ve12`, 13:26): hay que recompilar y volver a probar antes de tocar el binding. |
| 5 | La **retícula** se ve con el arma en la mano, no sólo al apuntar | HUD de serie: con cámara de ratón (`Using3rdPersonMouseCam`) pinta la cruz **siempre** (`DrawCrossHairPC`) | **1 (D6b, hecho)** | La mira se dibuja sólo al apuntar (`PED_LOCK_TARGET`, botón derecho o Supr). |
| 6 | No se puede **caminar agachado** | — | 3 | Clips `Crouch_*`/`DUCK_*` ya servidos (J0); `CPad::DuckJustDown()` existe. |
| 7 | No se puede **correr con armas de 2 manos** | `VICEEXT_SPRINT_HEAVY` está definido pero no basta | 3 | Clips `sprint_armed/rocket/csaw` ya servidos (J0); referencia: mods "Sprinting With Two Handed Weapons" (dato: `ped.ifp`+`weapon.dat`) y su versión ASI. |
| 8 | No se puede **apuntar con la escopeta** | — | 3 | Revisar la columna de flags de `CWeaponInfo` para los tipos de escopeta del `weapon.dat` del mod (45 armas). |
| 9 | No se puede **nadar**: Tommy cae al vacío, sin animaciones | `VICEEXT_SWIMMING` hoy sólo evita el ahogo; **no hay** estado `PED_SWIM` en `src/` | 3 | Clips `Swim_*` + `Drown` ya servidos (J0) + `CWaterLevel::GetWaterLevel` (ya usado por la cámara). Referencia: mod "Swimming in GTA VC with new animation". |
| 10 | El **depósito de gasolina** arde en vez de explotar al primer tiro | `VICEEXT gastank sin-dummy model=207/204/197/165/159/6504…` | 3 (C3.3) + dato de 1 | Sólo **25 de ~100** `.dff` de vehículos traen el dummy `petrolcap` (los 8 del mod sí: `hellenbach`, `manchez`, `polwintergreen`, `streetfi`, `peren2`, `trash2`, `premier`, `wintergreen`; de serie casi ninguno: p. ej. `bobcat` no). ⇒ hace falta **posición de reserva** (zona del tapón calculada del caja/ejes) para que explote igual. Sección 1 puede añadir dummies a los `.dff` de serie, pero es binario: preferimos el respaldo en código. |
| 11 | **Slot extra** en cargar partida; el autoguardado debería ir primero, con su nombre, y **no** ser reescribible por un guardado normal | `CheckSlotDataValid slot=3 file=/userfiles/GTAVCsf4.b` (autoguardado escribe la ranura 4) y `slot=1 … GTAVCsf2.b` | 3 (C2) | Lista de ranuras + escritura protegida. |
| 12 | **Autocentrado de cámara** sólo en vehículos, nunca a pie | Ya hay trazas `C1b-2` en `src/core/Cam.cpp` | 3 (C1) | A pie, no recentrar. |
| 13 | Disparar **con pistola desde el coche** sale con balas/animación de **SMG** | `DRIVEBY enter … wep=24 slot=5 … outcome=switch-smg` (con pistola, `wep=17`) | 2 (P2) | No cambiar de arma: usar el arma en mano (una mano) y sus balas/sonidos. Referencia: mod "Manual Driveby" (alternar pistolas/SMG con sonidos correctos por arma). |
| 14 | **`R` no recarga** | `VICEEXT_MANUAL_RELOAD` definido; el bloque C3.1 quedó a medias (rompió el build el 19/09) | 3 (C3.1) | Comprobar el estado actual. Referencia: mod "Manual Aiming v1.5" (recarga con `R`). |
| 15 | **Sirena del coche de policía** que no se ve (imagen) | `VICEEXT_POLICE_BIKE_LIGHTS` es de la moto (6507), no del coche | 2 | Vice Extended: "Service lights for service cars". Confirmar el modelo de la foto y sus coronas. |
| 16 | El log se comía a sí mismo (`CARRATE` ×2.391 líneas) | — | **1 (D4c, hecho)** | Throttle: primeras 30 + cambios de clase + latido cada 512. |

### Nota C1 (cámara en 1ª persona) — qué se puede afirmar y qué no

- `src/core/ControllerConfig.cpp:298` ya bindea `PED_TOGGLE_1RST_PERSON` a la
  tecla **`V`**, y `Camera.cpp:1165` traza `VICEEXT 1p key …` en cada pulsación
  (lo añadió la sección 3 como C1b) — pero esa línea **no está en el wasm
  servido** (`grep -a "1p key" reVC.wasm` sobre el de las 13:26 = 0 coincidencias),
  así que el build que jugó el jugador no podía decir si la tecla llegaba.
- Lo único sólido: en la traza `tog=0` en las 7 muestras y **ninguna línea
  `VICEEXT 1p`** ⇒ en esa partida el conmutador nunca se encendió (y por tanto
  V no hizo nada, coincidiendo con el reporte).
- Siguiente paso de la sección 3: recompilar con C1b y volver a jugar; la traza
  dirá si la tecla llega (`VICEEXT 1p key …`) o si la puerta (`IsPedInControl`,
  `WideScreen`, `m_bLookingAtPlayer`, `MODE_*`) la bloquea.

## J3. Investigación de comunidad (para no inventar)

Lo que hay hecho y probado por otros, y que sirve de referencia directa:

- **Manual Aiming v1.5** (22,6 K descargas, 38 valoraciones): apuntar con el
  **botón derecho**, correr/moverse mientras se apunta, **recargar con `R`**,
  zoom para lanzacohetes y francotirador. Es exactamente los puntos 5/8/14.
- **Manual Driveby**: animaciones propias **por vehículo** (coche, moto, barco),
  **alternar pistolas/SMG** y **sonidos correctos por arma**. Es el punto 13.
- **Sprinting With Two Handed Weapons [VC]** (8,6 K descargas): lo hace **por
  dato** (`ped.ifp` + `weapon.dat`) — la vía más barata si el `weapon.dat` del
  mod ya trae las entradas. Y **Sprint With Two-Handed Weapons (ASI)**: misma
  mecánica sin animaciones nuevas (vía código).
- **First Person View for VC** (GeniusZ, 50 K+): conmutador con la tecla **`V`**
  y menú de ajustes propios. Es el punto 4.
- **Swimming in GTA VC with new animation** y **Climbing [reVC]** (mod ya hecho
  *sobre reVC*, configurado por `reVC.ini`): nado y escalada; la escalada es justo
  el bloque opcional E1 (`features.ini: EnableClimbing=0`).
- Y lo que ya tenemos: **reVC upstream** (nuestra base) + el `features.ini` y
  `ChangesEN.txt` del propio mod (ahí está el "qué" de cada mecánica).

Regla de estilo que se deduce de todo esto: **primero dato/animación ya
servida, después código nuevo, y siempre detrás de su `VICEEXT_*`.**

## J4. Lo que hace la sección 1 en este bloque

1. **D6b** — la mira del arma sólo se dibuja apuntando (`src/renderer/Hud.cpp`).
2. **J1** — captura de motivo de cuelgue (`web/lib/index.js`).
3. **D10** — traza `LODLEFT model=… dist=… relacion=… rwobj=… alpha=…`
   (`src/renderer/Renderer.cpp`, en el punto exacto donde el LOD se sigue
   dibujando de cerca). Una línea por modelo (40 máx.) y sólo si el LOD está a
   menos de 60 m. Con eso el punto 3 se resuelve con dato en la próxima partida.
4. **D4c** — `CARRATE` deja de inundar la traza (`src/control/CarCtrl.cpp`).
5. Verificación: `python gta_vc_browser/tools/viceext-log-check.py --desde-marca`
   (se le añadirá la lectura de `LODLEFT` y de `JSERR/ENGERR`).

## J5. Cómo se valida

1. `Ctrl+Shift+R` en la pestaña (los cambios de web/lib entran al recargar; los
   de C++ necesitan el build de la sección 1, etiqueta `ve13`).
2. El jugador hace lo de siempre: conducir, apuntar, cinemática, 3 estrellas.
3. `python gta_vc_browser/tools/viceext-log-check.py --desde-marca` dicta
   D2/D4/D5/D6/D7 + `LODLEFT` + `JSERR/ENGERR`.
