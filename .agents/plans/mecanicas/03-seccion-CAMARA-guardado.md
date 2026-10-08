---
name: 03-seccion-CAMARA-guardado
status: EXECUTED
type: feature
domain: gameplay
owner_rules: .agents
created: 2026-09-19
---

# Sección 3 — Cámara, guardado y movimiento (para **aux 2**)

Dos bloques de mecánica + un cajón de mecánicas pequeñas. Contexto general y
reglas: `00-INDICE.md` (léelo antes; ahí están el arnés, la propiedad de
ficheros y los avisos de `node`/`chrome`/`dataTag`).

Fuente: changelog del mod (v1.0 → v2510) y su `features.ini`. Donde el mod no
da detalle, el criterio es "lo más parecido a SA que no rompa lo de serie", y
siempre detrás de un `#define VICEEXT_*`.

---

## C1 — Primera persona completa (v1.5: "First-person view")

Hoy el port ya tiene una primera persona **parcial**: `m_bFirstPersonBeingUsed`
en `Camera.cpp` (~líneas 1010-1045) entra en `CCam::MODE_1STPERSON` cuando el
jugador **mira alrededor** y sale al moverse, al apuntar o a los 2,85 s sin
input; también la usa el coche (`Camera.cpp:565`) y el modo arma
(`MODE_M16_1STPERSON`, `TheCamera.SetNewPlayerWeaponMode` en `PlayerPed.cpp`
~1290). O sea: es un *peek*, no un modo de vista.

**Punto de partida medido:**

- `src/core/Camera.cpp`: `m_bFirstPersonBeingUsed`, `m_uiFirstPersonCamLastInputTime`,
  `PedZoomIndicator`, `ReqMode = CCam::MODE_1STPERSON`, `PLAYERCONTROL_CAMERA`.
- `src/core/Cam.cpp`: `Process_1stPerson` (`Cam.cpp:221`), `MODE_1STPERSON_RUNABOUT` (285).
- `src/peds/PlayerPed.cpp`: `PlayerControl1stPersonRunAround` (~883) — **ya existe**
  el control en primera persona; `PlayerControlSniper`/`PlayerControlM16` (~684/822).
- `src/core/ControllerConfig.h`: acciones `PED_1RST_PERSON_LOOK_LEFT/RIGHT/UP/DOWN`
  (ya hay bindings) y **`_CONTROLLERACTION_36` marcada como "Unused"** → hueco
  libre para una acción nueva (p. ej. `PED_TOGGLE_1RST_PERSON`) sin reordenar el enum.
- `src/weapons/WeaponInfo.h`: flag `WEAPONFLAG_1_PERSON` (`WEAPONFLAG_1ST_PERSON`).

**Tareas (en orden):**

1. **Medir** qué hace hoy el modo *peek* en partida (capturas + `odtrace`): desde
   qué teclas entra/sale y qué se ve (¿manos? ¿armas? ¿HUD?). Anotar en "Estado".
2. Contrato (`#define VICEEXT_FIRST_PERSON`): un **conmutador de vista** en pie
   (tecla nueva o una existente libre) que:
   - mantenga `MODE_1STPERSON` mientras esté activo (no solo 2,85 s),
   - permita caminar/correr dentro del modo (`PlayerControl1stPersonRunAround`
     ya está: comprobar que responde a WASD),
   - mantenga el arma en primera persona (usar el camino de
     `SetNewPlayerWeaponMode`/`WEAPONFLAG_1ST_PERSON` para que el arma se vea),
   - vuelva a 3ª persona al apuntar con mira o al subir a un coche (como SA).
3. Si hace falta una tecla nueva, usar el hueco `_CONTROLLERACTION_36`, con su
   binding por defecto en la tabla de `src/core/Pad.cpp` (añadir **al final** del
   bloque correspondiente, comentario `// Sección 3:`) y su etiqueta en el menú
   de controles si el PC_MENU lista acciones (mirar cómo están las demás).
4. Sonda `tools/firstperson-smoke-test.mjs`: cargar partida, activar el modo,
   caminar, apuntar y disparar; capturas comparando 1ª vs 3ª persona y
   comprobar 0 asserts.

**Verificación (PASS):** el modo se mantiene al caminar (no se cae solo),
dispara en 1ª persona con arma visible y volver al menú/partida no rompe la
cámara (capturas).

**Riesgos:** la cámara la comparten coche, cutscenes y apuntado. **No** tocar
`Cam.cpp:Process_1stPerson` más de lo necesario ni las rutas de cutscene. Si el
modo se queda pegado al morir/cargar, forzar salida en `CCamera::Restore` /
`CPlayerPed::SetInitialState`.

---

## C2 — Autosave tras misión y "guardar en cualquier sitio" (v2.5)

Su changelog: "Autosave after completing a mission" y "Saving anywhere. You must
not be on a mission, not have a search level and not move".

**Punto de partida medido:**

- `src/control/Script.cpp:1553`: **ya existe el hook** del autosave, apagado:
  ```c
  #if 0 // makeing autosave is pointless and is a bit buggy
      SaveGameForPause(SAVE_TYPE_QUICKSAVE);
  ```
  Está justo en el camino de "misión pasada" (`CStats::LastMissionPassedName`).
- `src/save/GenericGameStorage.cpp:1295`: `SaveGameForPause(int type)` con sus
  guardas: `WaitForSave`, `gGameState == GS_PLAYING_GAME`,
  `CTheScripts::bAlreadyRunningAMissionScript` (bloquea si hay misión en curso,
  salvo tipos especiales) y `SAVE_TYPE_QUICKSAVE` en `GenericGameStorage.h:64`.
- Los tipos de save ya incluyen `SAVE_TYPE_QUICKSAVE[_FOR_SCRIPT]` (los usan
  `Script6/8` y el debug). En el web build el guardado va al perfil
  (IDBFS) — ver `web/lib` y `extract-save-from-profile.mjs`.

**Tareas (en orden):**

1. **Medir** cómo se guarda hoy desde el menú de pausa en el web build (¿existe
   la opción de guardar? ¿usa `SaveGameForPause`?) y qué ranura usa el
   quicksave. Anotar.
2. Contrato (`#define VICEEXT_AUTOSAVE`): quitar el `#if 0` y llamar al
   autosave **cuando la misión se da por pasada** (no en cada `SAVE` de script),
   con las guardas que ya tiene la función. Documentar en `HISTORIAL` qué ranura
   se pisa (el jugador debe saberlo).
3. Guardar en cualquier sitio: en vanilla VC PC el menú de pausa ya permite
   guardar; si el port lo restringe, relajarlo a "no en misión, sin búsqueda y
   parado" (las tres condiciones del mod) usando las guardas existentes.
4. Sonda: extender `slot0-load-test.mjs` o crear `tools/autosave-smoke-test.mjs`
   que **guarde** desde el menú, recargue la página y compruebe que la partida
   guardada existe y carga (ahí está el valor real: que sobreviva a un reload).

**Verificación (PASS):** tras guardar, recargar y cargar la ranura devuelve la
partida (captura + sin `RuntimeError`); el autosave escribe una vez por misión
(contar en `odtrace.log`/consola).

**Riesgos:** escribir un save corrupto es lo peor que puede pasar en este
bloque → **probar siempre en la ranura de test y con copia** (el save de prueba
está en `tools/testdata/GTAVCsf1.b`). No tocar el formato de save.

---

## C3 — Cajón de mecánicas pequeñas (una por build si se puede)

Cada una es corta y visible; van detrás de su propio define. Prioridad
recomendada (de más visible a menos):

1. **Recarga manual con tecla** (v2.5: "Reloading a weapon on the key"):
   `CWeapon::Reload()` ya existe (`Weapon.cpp:2944`) y el estado
   `WEAPONSTATE_RELOADING` con `m_nTimer` está en `CWeapon::Update`. Falta una
   tecla (hueco `_CONTROLLERACTION_36` o una libre) que dispare la recarga si
   el cargador no está lleno. Ojo con el sonido (`SOUND_WEAPON_RELOAD`) y con no
   recargar en medio de un *burst*.
2. **Movimiento sentado / "seated movement"** (v1.0): poder andar/sentarse con
   animación concreta; en VC el equivalente son los estados `PED_...` + anims
   `ANIM_STD_...`. Medir antes si el port ya permite moverse en interiores.
3. **Depósito de gasolina** (v2.5: "Gas tank. When shot, the car explodes") y
   **luces rompibles** (v2.5): puntos de daño de coche específicos
   (`CVehicle::VehicleDamage`/`CAutomobile::VehicleDamage`, `eCarNodes`), con
   `BurstTyre`/`BlowUpCar` como referencia. Empezar por el depósito.
4. **Intermitentes** (v2.5: "Turners, which are used by NPCs and can also be
   used by the player"): el mod tiene toggle aparte
   (`StandardCarsUseTurnSignals=0`) → implementar como define **apagado por
   defecto**, para paridad con su ini; enciende la luz del intermitente en
   `CAutomobile` (material `lights`/`lightson`, ver TXD de los coches) cuando
   el jugador gire con el volante.

**Verificación por mecánica:** captura + tecla + 0 asserts. Para el depósito:
disparar al depósito y ver la explosión; para la recarga: contador de munición
que sube y animación/sonido de recarga.

**Riesgos:** son cambios en `CVehicle`/`Automobile` (compartidos con la sección
2 por `DoDriveByShootings`, pero ahí solo tocan los tres `DoDriveByShootings`:
**no editar otras funciones de `Automobile.cpp`** sin avisar).

---

## Estado (lo rellena aux 2)

- [x] C1 medido (comportamiento del *peek*) → **19/09, build `2026-09-19-ve8` + traza `CAM1P`** (ver abajo)
- [x] C1 implementado → **20/09, `VICEEXT_FIRST_PERSON` + tecla B** (`PED_TOGGLE_1RST_PERSON`)
- [x] C1 sonda PASS → **`tools/firstperson-smoke-test.mjs`, 7/7, 0 errores, 38 capturas**
- [x] C2 medido (guardado desde menú/ranuras) → **20/09, ver abajo**
- [x] C2 implementado → **`VICEEXT_AUTOSAVE` + `VICEEXT_SAVE_ANYWHERE`** (compila y enlaza; trazas puestas)
- [ ] C2 verificado en partida real (el usuario juega y se lee la traza; sin sonda Chrome)
- [x] C3.1 recarga manual → **`VICEEXT_MANUAL_RELOAD`, tecla R** (compila; trazas puestas)
- [x] C3.2 movimiento sentado → **medido y descartado por decisión del jugador (20/09)**: ver abajo (el mod no da detalle y el port no tiene estado sentado del jugador)
- [x] C3.3 depósito (petrolcap) → **`VICEEXT_GAS_TANK`** (compila; trazas puestas)
- [x] C3.5 luces rompibles → **`VICEEXT_BREAKABLE_LIGHTS`** (20/09; los dos mecánicos de bala comparten un único punto de entrada)
- [x] C3.4 intermitentes → **`VICEEXT_TURN_SIGNALS`, apagado por defecto**, jugador **y NPC cercanos** (compila con el define puesto: `tools/tmp-syntax-turners.sh`; en el build normal no entra)
- [x] Verificación desde el log → **`tools/seccion3-log-check.mjs`** (lee `odtrace.log` y dice qué bloque ha dejado traza; sin Chrome)
- [ ] Trazas de partida real: **C1/C2/C3.1/C3.3/C3.5 pendientes de ver en el log**; la partida del 20/09 sólo dio una línea de recarga **automática** (ver "Lección": era un falso positivo)

Código vivo en el build **`2026-09-20-ve12`** (`reVC.wasm` de 20/09 13:20).

---

### C1 — medición (19/09/2026)

**Conclusión: hoy el *peek* no se puede usar en el port.** Con los controles por
defecto (Standard, ratón) la puerta de 1ª persona **no se abre nunca**, y
no es porque las teclas fallen: es la condición `!Cams[0].Using3rdPersonMouseCam()`.

Evidencia: sonda nueva `tools/firstperson-smoke-test.mjs` (carga el slot 0 y
pulsa las teclas de mirar y el ratón) + traza `CAM1P` de la instrumentación de
`CCamera::CamControl` (sólo emite al cambiar el estado, no por frame): 20 líneas,
25 capturas, 0 errores de página.

| Pregunta del plan | Medición |
|---|---|
| ¿Con qué controles arranca? | `ctrl=0 mouse3d=1` → **Standard (ratón)** y `Using3rdPersonMouseCam()=1` |
| ¿Entra en 1ª persona? | **No.** `fp=0` en las 20 líneas, `mode=4` (`MODE_FOLLOWPED`) siempre |
| ¿Desde qué teclas? | 4 y 6 del numérico **sí llegan al pad** (`stick=-128,0` y `stick=128,0` en `NewState.RightStickX`); 8 y 5 llegan como tecla (`num=4`/`num=8`) pero **no mueven el stick** |
| ¿Cuánto dura / qué se ve? | **No medible hoy**: nunca entra (queda para el bloque de implementación) |
| ¿Sale al moverse/apuntar? | No comprobable (nunca entra) |

De dónde sale cada cosa (código, líneas al empezar el bloque):

- `Camera.cpp` ~1015-1050: el bloque que pone `m_bFirstPersonBeingUsed = true`
  está detrás de `!Cams[0].Using3rdPersonMouseCam()`; con controles de ratón
  (`CCamera::m_bUseMouse3rdPerson = true` + `Cams[0].Mode == MODE_FOLLOWPED`, o
  sea `ctrl=0`) esa condición es falsa → el bloque no corre y
  `m_bFirstPersonBeingUsed` se fuerza a `false` en el `else`.
- `ControllerConfig.cpp` ~900-950: las teclas de mirar del numérico
  (`PED_1RST_PERSON_LOOK_*`) escriben `RightStickX` (±128) — es lo que se ve en
  la traza — pero **arriba/abajo sólo se aplican con
  `m_ControlMethod == CONTROL_CLASSIC`**: en Standard ni mueven el stick
  (medido: `num=4`/`num=8` con `stick=0,0`).
- Salidas del modo (`Camera.cpp` ~1032-1040): caminar
  (`GetPedWalkLeftRight/UpDown`), botones de acción, `TargetJustDown` o **2,85 s
  sin mirar**. Además, con `!IsPedInControl()` o `m_fMoveSpeed > 0` se apaga.
- La 1ª persona que **sí** existe hoy es la del arma (`MODE_M16_1STPERSON`,
  sniper, lanzacohetes, y `MODE_1STPERSON` en coche por `CamZoom1stPerson`), que
  no pasa por esta puerta.

Consecuencia para el contrato: el conmutador de C1 tiene que **levantar la
puerta del ratón**; una tecla sola no basta (hoy la tecla llega y no pasa nada).

### C1 — implementación (20/09/2026, madrugada)

**Contrato:** `VICEEXT_FIRST_PERSON` (`src/core/config.h`, al final del bloque
Vice Extended) + acción **`PED_TOGGLE_1RST_PERSON`** en el hueco
`_CONTROLLERACTION_36` (mismo índice, así que no se reordena el enum; antes
estaba marcado "Unused"), con binding por defecto **B** en `KEYBOARD`
(`ControllerConfig.cpp`) y su nombre en el menú de controles. Al ser una acción
de verdad, el jugador puede rebindarla.

Qué hace: mientras está encendida pide `ReqMode = MODE_1STPERSON_RUNABOUT` (la
1ª persona **de PC**: `Cam::Process_1rstPersonPedOnPC`, cámara en la cabeza,
ratón y punto de mira) en vez del `MODE_1STPERSON` del *peek* (palo derecho y
2,85 s de caducidad). No caduca por tiempo ni al caminar; se cierra al apuntar
con mira (`TargetJustDown`), al subir a un coche (rama de vehículo de
`CamControl`), al perder el control del ped y en `CCamera::Restore()`.

Dos arreglos de fondo en `Cam::Process_1rstPersonPedOnPC` (código que en este
port **no activaba nadie**):

1. La cabeza se saca del **IK** (`m_pedIK.GetComponentPosition(…, PED_HEAD)`),
   como en `Process_M16_1stPerson`. La vía original
   (`TransformToNode` + transformar la matriz del hueso con
   `RpHAnimHierarchyGetMatrixArray`/`RpHAnimIDGetIndex` y escalarla a cero)
   devolvía una posición inválida → la cámara se iba fuera del mundo →
   `memory access out of bounds` en `CWorld::ProcessLineOfSightSectorList`
   (llamado desde `cAudioManager::UpdateReflections`, que lanza rayos desde
   `TheCamera.GetPosition()`) a los ~5 s de entrar en el modo.
2. `TheCamera.pTargetEntity` → `CamTargetEntity` al forzar el rumbo del ped
   (el primero es nil si el objetivo cambió a mitad de frame).

Y un detalle que **no** se copia del *peek*: `DisablePlayerControls |=
PLAYERCONTROL_CAMERA`. `CPad::ArePlayerControlsDisabled()` es
`DisablePlayerControls != 0`, y con cualquier bit puesto
`GetPedWalkUpDown/LeftRight` y `TargetJustDown` devuelven 0/false → el jugador
no andaba y apuntar no sacaba del modo.

Evidencia: sonda `tools/firstperson-smoke-test.mjs` **PASS 7/7** (tecla →
`tog=1`/`mode=41`; 12 líneas andando con `spd=21…168` y la cámara moviéndose;
ninguna caída del modo; `tog=0`/`mode=4` al apuntar; 0 errores de página) y 38
capturas en `%TEMP%\vc-firstperson`. Detalle completo y avisos del arnés en
`.agents/HISTORIAL.md` (20/09, madrugada).

### C2 — medición (20/09/2026)

**Cómo guarda hoy el port (antes de tocar nada):**

| Pregunta | Medición |
|---|---|
| ¿Se puede guardar desde el menú de pausa? | **No.** `MENUPAGE_PAUSE_MENU` (`MenuScreensCustom.cpp` ~line 692) no tiene ninguna acción de guardado; sólo REANUDAR/OPCIONES/MAPA/ESTADÍSTICAS. |
| ¿De dónde se guarda entonces? | De la **zona de guardado** del script: opcode `ACTIVATE_SAVE_MENU` → `m_bActivateSaveMenu` → `m_OnlySaveMenu = true` + `m_nCurrScreen = MENUPAGE_CHOOSE_SAVE_SLOT` (`Frontend.cpp` ~5738). |
| ¿Qué ranuras hay? | Ocho visibles, `SAVESLOT_1..8` en la pantalla de carga y de guardado. `m_nCurrSaveSlot = saveSlot - SAVESLOT_1` (0..7) → ficheros `GTAVCsf1..8.b`. |
| ¿Y el quicksave? | `PAUSE_SAVE_SLOT`, que era `SLOT_COUNT` (=8) → **fichero 9** (`GTAVCsf9.b`). Lo usan `SaveGameForPause()` (síncrono: `SaveSlot(PAUSE_SAVE_SLOT)` + `PopulateSlotInfo`, sin pasar por el menú) y el reintento de misión móvil. La ranura 8 quedaba **fuera** de las matrices (`SlotFileName[8]`, `Slots[8]`): el fichero se escribía pero el menú no lo listaba. |
| ¿Los ficheros? | IDBFS `/userfiles` (persistido por la capa web). El menú llama a `MakeValidSaveName(slot)` → `%s<slot+1>.b` = `GTAVCsf<slot+1>.b`. |
| ¿Existe rótulo para una 9ª ranura? | Sí: `FEM_SL9` está en **todos** los GXT del port (`tools/gxt_inspect.py check streamed/text/*.gxt FEM_SL9` → presente). |

### C2 — implementación (20/09/2026)

**Ranura 9 = autosave, y visible.** `SLOT_COUNT` 8 → **9** (`GenericGameStorage.h`),
`PAUSE_SAVE_SLOT` sigue valiendo **8** (`SLOT_COUNT - 1`, el mismo número que antes,
para no cambiar el fichero que ya usaba el quicksave móvil), así que el fichero
sigue siendo `GTAVCsf9.b` pero ahora entra en `SlotFileName[]`/`Slots[]` y el
menú de carga lo lista con `FEM_SL9` como una ranura más (los `<= SAVESLOT_8` de
`Frontend.cpp` pasan a `<= SAVESLOT_9`). Las 8 ranuras del jugador no cambian.

**Autosave (`VICEEXT_AUTOSAVE`).** El gancho ya existía apagado (`Script.cpp:1553`,
`#if 0 // makeing autosave is pointless...`). Se sustituye por: marca en
`COMMAND_REGISTER_MISSION_PASSED` (`Script3.cpp` → `ViceExtAutosavePending`) y
escritura en `COMMAND_TERMINATE_THIS_SCRIPT` (`Script.cpp`), que es cuando la
misión ya está dada por superada y `bAlreadyRunningAMissionScript` está limpio
(si no, `SaveGameForPause` la rechaza). Es **una escritura por misión**, no por
cada `SAVE` de script. Traza: `VICEEXT autosave ok=… slot=8 file=GTAVCsf9.b mision=…`.

**Guardar en cualquier sitio (`VICEEXT_SAVE_ANYWHERE`).** Entrada nueva en el
menú de pausa (`FET_SG` → `MENUPAGE_CHOOSE_SAVE_SLOT`) y guarda de las tres
condiciones del mod justo antes de pisar la ranura (`Frontend.cpp`,
`MENUACTION_SAVEGAME` → `ViceExtCanSaveAnywhere()` en `GenericGameStorage.cpp`):
**sin misión** (`bAlreadyRunningAMissionScript`), **sin nivel de búsqueda**
(`m_pWanted->GetWantedLevel() == 0`) y **parado** (velocidad del ped o de su
vehículo ≤ 0,1 m/s). Si no se cumplen, no se escribe nada y sale el aviso
`FES_SAV` (presente en los GXT). La cancelación de esa pantalla vuelve al menú de
pausa cuando NO venimos de una zona de guardado (si venimos, se comporta como
siempre: cierra el front-end). Trazas: `VICEEXT save-anywhere aceptado opt=… zona=…`
y `… rechazado opt=… slot=…`.

**Riesgo asumido:** nada de esto toca el formato del save; el autosave escribe por
el mismo camino que el quicksave de misión que el port ya usaba.

### C3.1 — recarga manual (20/09/2026)

`VICEEXT_MANUAL_RELOAD` + acción **`PED_RELOAD`** (hueco `UNKNOWN_ACTION`, mismo
índice) con tecla por defecto **R** (`ControllerConfig.cpp`, rebindable en el menú
de controles). `CPlayerPed::ViceExtTryManualReload()` reutiliza el camino de la
recarga automática (`m_eWeaponState = WEAPONSTATE_RELOADING` + `m_nTimer`), así
que la animación y el sonido los pone el motor; sólo actúa a pie, con el control
del jugador, con el cargador no lleno y el arma con cargador
(`m_nAmountofAmmunition > 1`, munición total > 0), y respeta `m_bFastReload`.
Se llama desde `CPlayerPed::ProcessControl`. Trazas: `VICEEXT reload start …`,
`… reload no …` (por qué no recargó) y `… reload done clip=…/… total=…`.

### C3.2 — movimiento sentado: medido, no implementado (20/09/2026)

El mod da **una sola línea** (`ChangesEN.txt:171` "Seated movement",
`ChangesRU.txt:172` "Передвижение в состоянии сидя"), sin detalle, y aparece en la
lista de **v1.0** junto a cosas de jugador (nadar, escalar, apuntado) y a una de
NPC (escalar "including for NPCs"), así que no se puede deducir a qué se refiere
exactamente. Lo que hay en el port, comprobado en el código:

- **El jugador no tiene estado "sentado"**: `PED_SIT` existe en el enum
  (`src/peds/Ped.h:325`) pero **nadie lo escribe** (0 usos en todo `src/`); el
  jugador sólo está "sentado" en un vehículo (`PED_DRIVING` + `bInVehicle`), y
to moverse ahí es conducir.
- **Los NPC sí**: los attractors de asiento (`OBJECTIVE_GOTO_SEAT_ON_FOOT` →
  `WAITSTATE_SIT_DOWN` → `SIT_IDLE` → `SIT_UP`, `PedAI.cpp:1803` y
  `Ped.cpp:8810-8860`). Un ped sentado se levanta **sólo** por temporizador
  (25-30 s), por amenaza (arma/explosión/ped muerto) o si huye de un vehículo:
  durante `SIT_IDLE` no responde a movimiento.

Conclusión: implementarlo "a ciegas" sería inventar una mecánica. Se preguntó al
jugador (que es quien juega al mod) el **20/09**: decisión suya = **dejarlo
pendiente**. No se implementa; si alguna vez hace falta, el punto de entrada sería
el attractor de banco (`WAITSTATE_SIT_*`) o una mecánica de asiento nueva para el
jugador (no existe hoy).

### C3.3 — depósito de gasolina (20/09/2026)

`VICEEXT_GAS_TANK`. El mod marca el depósito con un dummy `petrolcap`
(`AdaptingVehiclesEN.txt`) y sus modelos adaptados lo traen. `CVehicle::FindDummyFrame()`
(búsqueda recursiva por nombre en el clump) + comprobación de que el punto de
impacto cae a ≤ 0,6 m del dummy. Lo importante del camino elegido: **no se pone
fuego a mano**; se deja la salud del vehículo en el umbral del motor
(`250.0f`, `DAMAGE_HEALTH_TO_CATCH_FIRE` en `Vehicle.cpp`) para que **el daño de
la propia bala** lo cruce acto seguido en `InflictDamage`, que es quien pone
`ENGINE_STATUS_ON_FIRE` y `m_pSetOnFireEntity` → humo, llamas y explosión a los
~5 s como cualquier coche incendiado. Enganchado en `DoBulletImpact`,
`FireShotgun` y `FireFromCar`. Trazas: `VICEEXT gastank hit …` y
`… gastank sin-dummy model=…` (modelo sin dummy, una vez por modelo).
**No hechas** (fuera del alcance de mi bloque): las "luces rompibles" y el punto 7
experimental del mod (faros que se rompen al disparo).

### C3.4 — intermitentes (20/09/2026)

`VICEEXT_TURN_SIGNALS`, **apagado por defecto** a propósito (su `features.ini`
trae `StandardCarsUseTurnSignals=0`). `CAutomobile::ViceExtProcessTurnSignals()`
(único bloque mío en `Automobile.cpp`, al final de `ProcessControl`, marcado con
comentario de propiedad) enciende coronas naranjas parpadeantes (~1,5 Hz) en los
objetos/dummies `indicator_*`/`indicators_*` del modelo mientras gira el volante
(`m_fSteerInput`) más de 0,25 — nombres sacados de `AdaptingVehiclesEN.txt`
(6.5), no inventados.

Los **NPC también** (la otra mitad de la frase del changelog): cualquier coche
con conductor que esté a menos de 40 m, con presupuesto de **2 coches por
fotograma**, porque la piscina de coronas es de 56 para todo el juego
(`CCoronas`) y encender todo el tráfico a la vez la revienta.

Como el define está apagado, el bloque **no entra en el build normal**. Se
comprueba con `tools/check-define-build.sh VICEEXT_TURN_SIGNALS
src/vehicles/Automobile.cpp` (reutiliza la línea de compilación real de
`ninja -t commands` y le añade el define; sirve para cualquier bloque apagado de
las tres secciones). **Corrección
encontrada por esa vía (20/09):** el bloque usaba `ODTRACES` sin
`#include "ondemand.h"` — con el define apagado no se notaba y habría petado al
encenderlo. Ya está el include (guardado por el define).

### C3.5 — luces que se rompen al disparo (20/09/2026)

`VICEEXT_BREAKABLE_LIGHTS`. Es el punto 5 del mod ("Car lights can break on
impact") y su experimental 7) ("Vehicle lights can break when shot at").

Ancla en el motor: los modelos adaptados traen los faros como objetos con nombre
propio (`headlight_l/r`, `taillight_l/r`; `AdaptingVehiclesEN.txt` 6.1/6.2 —
comprobado que **están** en los `.dff` de `streamed/models/gta3.img`: 25 modelos
con `headlight_l`), y el port **ya sabe dibujar una luz rota**:
`CAutomobile::Render` decide el resplandor con `Damage.GetLightStatus(VEHLIGHT_*)`
(`Automobile.cpp` ~2350-2530). Lo que faltaba era el camino del disparo: en
chapa, `DamageManager` ya rompe la luz por componente con daño fuerte
(`COMPGROUP_PANEL` → `SetLightStatus(..., 1)`), así que el "break on impact" es
de serie y lo que se añade es el tiro.

Implementación: el impacto de bala que cae a ≤ 0,5 m del objeto del faro pone
esa luz en `LIGHT_STATUS_BROKEN` (y no repite la traza si ya estaba rota).
**Lo que no se copia del mod:** él cambia la textura del faro a la de apagado y
enseña el hueco del modelo; aquí no hay intercambio de material por objeto, así
que el resultado visible es que ese faro deja de alumbrar.

**Refactor de paso:** los dos mecánicos de bala (depósito y luces) comparten un
único punto de entrada, `ViceExtBulletHitVehicle()`, y los tres caminos del arma
(`DoBulletImpact`, `FireShotgun`, `FireFromCar`) lo llaman igual. Antes había
tres `#ifdef` sueltos por mecánica; así la siguiente no obliga a tocar tres
sitios.

### Verificación desde el log (20/09/2026)

`tools/seccion3-log-check.mjs` (node, **no** abre Chrome): lee `odtrace.log` (y
`odtrace.prev.log` con `--prev`) y da el veredicto de cada bloque con las líneas
de prueba y, si falta, la acción exacta que hay que hacer en partida. Es la vía
para cerrar C2/C3 sin sondas: se juega una vez y luego se pasa el verificador.

Protocolo mínimo en partida (una pulsación por bloque):

| Bloque | Acción | Línea que debe salir |
|---|---|---|
| C1 | tecla **B**, andar, apuntar, tecla **B** | `CAM1P … tog=1 mode=41` y luego `tog=0 mode=4` |
| C2a | superar una misión | `VICEEXT autosave ok=1 slot=8 file=GTAVCsf9.b` |
| C2b | Esc → GUARDAR PARTIDA → ranura | `VICEEXT save-anywhere aceptado` (o `rechazado` si te mueves/te buscan) |
| C3.1 | disparar unas balas y **R** | `VICEEXT reload start` + `VICEEXT reload done manual=1` |
| C3.3 | disparar al tapón del depósito | `VICEEXT gastank hit model=… hp=…` |
| C3.5 | disparar a un faro | `VICEEXT luces rota luz=headlight_l …` |
| C3.4 | (define apagado) | `VICEEXT turners quien=…` sólo si se enciende |

### Avisos del arnés (sirven a las 3 secciones)

- **(importante) El numérico no llega al motor con `page.keyboard`**: con CDP el
  navegador lo entrega como dígito de la fila superior (lo cazó la traza:
  `num=0` con la tecla pulsada) y el motor nunca ve `NUM4`. Hay que despachar
  `Input.dispatchKeyEvent` con `location: 3`, como hace ya
  `firstperson-smoke-test.mjs`. Afecta a cualquier sonda que use el numérico:
  p. ej. el ciclo de armas de `weapons-smoke-test.mjs` (`NumpadDecimal`) — esa
  parte probablemente nunca hizo nada.
- **Escape no llega al motor** (ni con CDP): no se puede abrir el menú de pausa
  desde una sonda, así que no se puede conmutar el método de control por menú.
  La fase 2 de la sonda (intento de CLASSIC) queda sin efecto y así se reporta.
  Para medir el modo con CLASSIC habrá que ir por otra vía (p. ej. dejar
  `gta_vc.set` en `/userfiles`, que hoy no existe: sólo hay `reVC.ini`).
- Sí llegan y se miden: letras (`keys=1` con W, `speed>0` = el jugador anda) y
  flechas (`keys=4`/`keys=8`).
- Nota sobre la traza: los campos `kstick=` salen siempre `0,0` porque
  `CPad::Update` limpia `PCTempKeyState` dentro del frame; el campo bueno para el
  *stick* de mirar es `stick=` (`NewState`). Se dejan por si hacen falta al
depurar el modo nuevo.
- La sonda deja 25 capturas en `%TEMP%\vc-firstperson` (comparables 3ª persona vs
  lo que salga cuando el modo se pueda activar).
- (20/09, al verificar C1) **No fiarse de `odtrace.log` para "¿cargó mi
  pestaña?"**: con otras sondas escribiendo en el mismo fichero, un `FPHASE`
  ajeno da por buena una carga que no ocurrió. El heartbeat propio de la página
  (`#gamelog`, `state 9` = `GS_PLAYING_GAME`) sí es de fiar.
- (20/09) Un perfil de Chrome que conserve un `reVC.js` viejo aborta el motor
  (`TypeError: _asyncify_start_unwind is not a function`) al juntarse con un
  `reVC.wasm` nuevo, aunque el servidor mande `no-store`. En las sondas:
  `page.setCacheEnabled(false)`.
- (20/09) En el arnés headless el **botón derecho del ratón no tiene binding**
  (`CMousePointerStateHelper::GetMouseSetUp()` exige cursor fuera de (0,0) al
  inicializar el motor): para apuntar, `Supr` (binding de teclado de
  `PED_LOCK_TARGET`) da la misma señal (`RightShoulder1`).
- (20/09) **Lección de la partida real: una traza no vale como prueba si no
distingue al autor de la acción.** La del 20/09 sólo dejó un
  `VICEEXT reload done clip=17/17` y ningún `start`: era la recarga
  **automática** al vaciar el cargador (el jugador no pulsó R; `tog=1` tampoco
  aparece en toda la sesión). O sea: el falso positivo no venía del código sino
  de instrumentarlo sin marcar el origen. Ahora `reload done` exige la marca
  `s_viceExtManualReload` e imprime `manual=1`; **regla para las mecánicas que
  quedan: si el motor tiene un camino propio que produce el mismo efecto, la
  traza tiene que decir de dónde vino**.
- (20/09) **El log rota solo** (`rotado: sesion nueva` + `odtrace.prev.log`) y
  con él se va la evidencia: el verificador mira los dos ficheros, pero para
  cerrar un bloque conviene pasarlo justo después de la partida.
- (20/09) Ruido propio retirado: `CAM1P` escribía en cada cambio de velocidad
  (~2.589 líneas en 10 min de partida normal) y enterraba las trazas de las
  otras dos secciones (el log lo comparten las tres). Ahora sólo escribe al
  conmutar o **mientras** el modo de 1ª persona está encendido.

---

# PLAN v2 — 20/09 (tarde): la 3ª partida real del jugador

El jugador jugó ~5 min con el build `ve12` (18:42:35Z–18:47:31Z en
`gta_vc_browser/web/odtrace.log`) y trajo 13 quejas. Esta sección es el plan de
lo que **yo** (sección 3) hago con ellas, con la evidencia del log por delante y
la investigación de fuera (mods de la comunidad) por detrás. Regla de la casa:
**una queja = una línea de log** o se queda sin tocar.

## 0. La partida, leída

| Traza | Qué dice |
|---|---|
| `JS build=2026-09-20-ve12` · `IFPFILE ANIM\PED.IFP clips=272 total=272` | El build y **el `ped.ifp` del mod** (272 clips) son los del jugador. |
| `CAM1P … tog=0 togkey=0` (7 líneas, ninguna con `tog=1`) | **C1 nunca se encendió** en toda la partida. |
| **0 líneas** `VICEEXT reload` | C3.1 no llegó ni al camino de éxito ni al de rechazo. |
| `VICEEXT luces rota model=156 luz=headlight_l/headlight_r/taillight_l` | **C3.5 funciona** en partida real. |
| `VICEEXT gastank hit model=159 hp=250→225→…→25` (10 impactos seguidos) | C3.3 se dispara, pero **el coche arde** y no explota: el jugador disparó 10 veces al mismo punto. |
| `VICEEXT gastank sin-dummy model=207/204/197/165` | El dummy `petrolcap` no está en todos los modelos: normal, va por modelo. |
| `ODSHORT open-fail models/gta3.img/BFOBE/WMYLG/WFYLG/{wfybe,wfyjg,wmobe,wmyjg}.{dff,txd}` + `CARFAIL status=254` ×8, y **el log se corta seco** a las 18:47:31 | Los skins de ped del mod no existen en el `.img` → el motor pide, falla y **la página muere ahí**. |
| `ODSHORT open-fail` también de `pickupsave.dff`, `washpaynspray.dff`, `washbuild194/195.dff`, `wshchurchwal.dff`, `od_walkway2.dff`, `rafneonsign.txd`, `rafelneonsgn.dff`, `undermallneon.dff`, `washdeconeon1.dff`, `wshneon.dff`, `wshbuildneon9.dff`, `vice1/vice2/vicechee/chopper .{dff,txd}` (27 fallos) | Modelos/carteles del mod que el IDE pide y el `gta3.img` no tiene. |
| `[texconv] clamp 9->7 niveles` / `9->4 niveles` (30 líneas) | Las TXD se recortan de mipmaps al convertir. |
| `WEBHB … cut=0` en **todas** las líneas de la sesión | Ninguna cinemática llegó a arrancar (`cut` = 1 significa cinemática). |
| `SIGHT arma=19 mira=4` / `arma=17 mira=2` | La mira/sight es de la sección 1 (bloque D6, `renderer/Hud.cpp`). |

## 1. Reparto de las 13 quejas

**Mías (sección 3, este plan):**

1. no hay 1ª persona al pulsar **V** → §C1b
2. la retícula se ve **siempre** con el arma en la mano → §C1c
3. **no puedo agacharme** ni andar agachado → §C5
4. no puedo **correr con armas de 2 manos** → §C6
5. no puedo **apuntar con la escopeta** → §C7
6. **no puedo nadar** (Tommy cae al vacío) → §C4
7. el coche al que disparo el depósito **arde en vez de explotar** → §C3.3b
8. la ranura de autoguardado se ve **de más** y debería ir **primero**, llamarse
autoguardado y **no poder pisarse** con un guardado normal → §C2b
9. falta el **autocentrado de cámara** (solo en vehículo) → §C1b-2
10. **R** no recarga → §C3.1b

**NO mías (con la evidencia lista para el dueño):**

| Queja | Dueño | Evidencia / pista |
|---|---|---|
| texturas que no pasan de LOD bajo (calles, muros, Malibú) | sección 1 | `[texconv] clamp 9->7/9->4 niveles` + 27 `open-fail` de TXD del mod |
| el juego se cierra con 3 estrellas y con cinemáticas | sección 1 (datos) | la sesión muere justo tras 8 `CARFAIL status=254` de skins del mod; `cut=0` en toda la sesión |
| no cargan cinemáticas | sección 1 | `WEBHB … cut=0` siempre |
| balas de SMG al disparar la pistola desde el coche | sección 2 (P2) | su `Help.txt`: *"to switch between pistol and submachine gun in a vehicle, press the change weapon keys while aiming"*; `FireInstantHitFromCar` usa `GetInfo()` del arma → el camino del drive-by es de `DoDriveByShootings` |
| la retícula/sight siempre visible | sección 1 (D6) | `SIGHT arma=… mira=…` en `renderer/Hud.cpp` |

## 2. Investigación (lo que ya está resuelto por la comunidad)

Todo lo de abajo son mods **públicos y usados**, con la misma pega resuelta; se
copian ideas (no código): el criterio es que quede como Vice Extended.

- **Nadar** — *GTA Vice City (Stories Style Swimming)* (ModDB, 2022, 11.6k
descargas) y *The ability to swim in GTA Vice City*: CLEO + **`ped.ifp`
reemplazado** con las animaciones VCS/SA. Además "you can only climb out of the
water" y "if your health is less than 5 you drown". **En este port no hay que
añadir dato**: el `ped.ifp` del mod ya está dentro (272 clips) y trae
`swim_crawl`, `swim_breast`, `swim_tread`, `swim_jumpout` (comprobado leyendo el
binario servido). Lo que falta es el **estado** de natación.
- **Esprintar con armas de 2 manos** — *Sprinting With Two Handed Weapons*,
*Heavy Weapon Running* (Nexus), *Sprint With All Weapons + Swimming*: en el mod
original se resuelve con las animaciones `sprint_armed` / `sprint_rocket` /
`sprint_csaw`, que **también están** en el `ped.ifp` del mod. La tabla del port
las ignora: `aPlayer2ArmedAnimations` usa `run_armed` **en el hueco del
esprint**.
- **Primera persona** — *Ultimate First Person Mod* (SA) y *FPS Mod* (GameBanana)
coinciden en las tres piezas: cámara en la cabeza con mirada de ratón,
**retícula solo al apuntar** y **"vehicle autocenter"** (la cámara vuelve sola
detrás del coche; a pie no, que es justo lo que pide el jugador).
- **Agachado** — en VCS/LCS y en todos los mods de *crouch* para VC se usa
`duck_down`/`duck_low` **+** `crouch_idle`/`crouch_forward`/`crouch_backward`
(el mod trae las cinco) y el *keyboard* se ata a una acción, que es justo lo que
hoy no llega.
- **Recarga a mano** — *Reload Mod (Fixed)*, *Weapon Reload Mod*, *Reload Mod by
Junior_Djjr*: todos coinciden en `R` + "rellena el cargador desde la munición
total al terminar la animación" y en no recargar si el cargador está lleno. Es
lo que ya hace C3.1: el problema es que **la tecla no llega**, no el contrato.
- **Depósito que explota** — el propio changelog del mod: *"2) Gas tank. When
shot, the car explodes"*. Ningún mod de la comunidad lo hace arder primero: la
paridad es explosión **inmediata**.

## 3. Los bloques nuevos (contrato, ancla y riesgo)

### C1b — la 1ª persona se enciende con **V** (y V deja de cambiar de cámara)

- **Por qué**: el jugador pulsa **V** (tecla de «cambiar cámara» en VC); el
  conmutador está en **B**. Se cambia el default a **V** y se libera V de
  `CAMERA_CHANGE_VIEW_ALL_SITUATIONS` (que se queda en `rsHOME`, su binding
  principal). Sigue siendo rebindable en el menú de controles.
- **Traza nueva**: se registra **cada vez que la tecla del conmutador se ve
  pulsada**, aunque el modo no pueda aplicarse (fuera de control, detenido,
  cinemática). Así se distingue «la tecla no llega» de «llega y no aplica».

### C1b-2 — autocentrado de cámara **solo en vehículo**

- **Contrato**: conduciendo, si el jugador no toca la mirada (ratón/palo derecho)
  durante ~1,5 s y no está apuntando, la cámara vuelve poco a poco detrás del
  coche (1,5 rad/s). **A pie no se toca nada** (hoy ya está bien y el jugador
  dice que nunca hizo falta).
- **Ancla**: rama de vehículo de `CCamera::CamControl` / `CCam::Process_FollowCar_SA`
  (`Cams[ActiveCam].m_fHorizontalAngle`), dentro de `#ifdef VICEEXT_FIRST_PERSON`
  (es la misma familia: «cámara moderna» del mod v1.0).
- **Riesgo**: si se aplica también con la cámara de mira dentro del coche, marea.
  Solo se aplica cuando **no** hay apuntado ni modo 1ª persona.

### C1c — la retícula, solo al apuntar  *(NO es mía)*

Va a la **sección 1**: `renderer/Hud.cpp` (bloque D6) pinta el sight siempre que
hay arma en la mano (`SIGHT arma=… mira=…`). Lo que pide el jugador (retícula
solo con el botón de apuntar pulsado) es la condición que D6 ya conoce
(`mira=`), así que es un cambio de una condición, no de diseño.

### C2b — la ranura del autoguardado: **primera, con nombre y protegida**

- **Qué**: `MENUPAGE_CHOOSE_LOAD_SLOT` lista las 8 ranuras y luego la 9ª (hoy,
  además, sale «de más» al final). Se hace:
  1. **primera** de la lista (y las 8 normales pasan a 2ª..9ª posición);
  2. etiqueta propia **«autoguardado»** (no `FEM_SL9`, que es el texto de
     «no hay archivo»): se usa una clave GXT existente si la hay y, si no, se
     escribe el literal en la entrada del menú (sin tocar datos de la sección 1);
  3. **no reescribible**: en `MENUPAGE_CHOOSE_SAVE_SLOT` la ranura 9 no existe
     (ya es así), y en `MENUACTION_SAVEGAME` se rechaza explícitamente guardar
     en `SAVESLOT_9` si no viene del autosave (`ViceExtAutosavePending`). La
     pantalla de **borrado** tampoco la lista.
- **Riesgo**: el menú usa `m_aEntries[0]` con coordenadas reales y las demás a
  `0,0`; al mover la entrada hay que llevarse las coordenadas con ella.

### C3.1b — por qué no recarga: la traza tiene que contestar

- **Lo que se sabe**: 0 líneas `VICEEXT reload` en 5 min de partida → o la tecla
  no llega, o el ped estaba en un estado que no está en la lista blanca, o el
  arma estaba en un estado que devuelve `false` **sin traza** (hay 4 salidas
  silenciosas).
- **Se arregla**:
  1. **traza en todas las salidas** (con motivo), y una línea
     `reload key` rate-limited (1/s) en cuanto la tecla se ve;
  2. lista de estados del ped más amplia (todo lo que no sea vehículo, muerte,
     caída o cinemática) y arma en cualquier estado salvo `RELOADING`;
  3. **no depender solo del teclado**: la tecla se lee igual que la del C1
     (misma función) y, además, se acepta el botón de pad equivalente si está
     puesto.
- **Verificación**: `reload key` + `reload no motivo=…` **o** `reload start`.

### C3.3b — el depósito **explota**, no arde

- **Contrato**: impacto a ≤ 0,6 m del dummy `petrolcap` → `BlowUpCar(shooter)`
  (explosión de serie: onda, fuego, destrozo). Se deja de bajar la vida al
  umbral del motor.
- **Riesgo**: `BlowUpCar` con `shooter` nil o ya explotado; se comprueba
  `m_nStatus != STATUS_WRECKED` y que no esté ya en llamas antes de llamar.

### C4 — **nadar** (`VICEEXT_SWIMMING`, el toggle del mod ya encendido)

- **Qué tiene que pasar**: al meterse en agua honda, Tommy **flota en la
  superficie** (no cae al vacío), se mueve nadando con la mirada, **no se
  ahoga** (eso ya estaba), y sale a la orilla con `swim_jumpout`.
- **Dato ya dentro** (no hay que tocar `gta_vc_browser/`): clips
  `swim_crawl`, `swim_breast`, `swim_tread`, `swim_jumpout` en el `ped.ifp` del
  mod (verificado: 272 clips servidos).
- **Cómo** (en `src/animation/` + `src/peds/PlayerPed.cpp`):
  1. `AnimationId.h`: 4 ids nuevos **al final** (no se reordena nada);
     `AnimManager.cpp`: grupo nuevo `playerswim` con su `aSwimAnimations` /
     `aSwimAnimDescs` al final de las tablas, para no romper el paralelismo
     `animNames[j] ↔ animDescs[j]` de los grupos existentes;
  2. `CPlayerPed::ProcessControl`: si `CWaterLevel::GetWaterLevel` dice que hay
     agua y el ped está por debajo de la superficie, entra en estado de nado:
     sube `m_vecMoveSpeed.z` hacia la superficie, la mantiene a `level - 0.5`,
     amortigua la caída y **anula el `PED_FALL`**;
  3. movimiento: velocidad horizontal desde el walk del pad **relativa a la
     cámara** (como la 1ª persona), `swim_breast` al avanzar / `swim_tread`
     quieto / `swim_crawl` esprintando;
  4. salida: agua somera o colisión con tierra → `swim_jumpout` y devolver el
     control normal.
- **Traza**: `VICEEXT swim enter/exit/move` (rate-limited) para poder cerrarlo
  desde el log sin sondas.
- **Riesgo**: es el bloque más caro. Va detrás del define existente
  (`VICEEXT_SWIMMING`), y **solo actúa con agua** (si no hay agua no se ejecuta
  ni una línea), así que no puede romper el movimiento en tierra.

### C5 — **agachado** (`VICEEXT_CROUCH`)

- **Lo que falta de verdad**: `PED_DUCK` está atado a **C** y el motor tiene
  todo el camino de agachado (`SetDuck`/`ClearDuck`, `bIsDucking`,
  `duck_down`/`duck_low`/`weapon_crouch`), pero `CPad::DuckJustDown()` **solo lee
  el botón de pad** (`NewState.LeftShock`): la tecla del teclado nunca toca ese
  bit, así que el jugador no se agacha.
- **Se arregla**: helper propio en `PlayerPed.cpp` (`ViceExtDuckJustDown`) que
  acepta el pad **o** la tecla de `PED_DUCK` (misma función que C1/C3.1), usado
  en los 4 sitios donde el jugador decide agacharse. `Pad.cpp` no se toca.
- **Caminar agachado**: el hueco del esprint de los grupos del jugador pasa a
  los clips del mod cuando toca (§C6) y el movimiento agachado usa
  `crouch_idle`/`crouch_forward`/`crouch_backward` (que están en el `ped.ifp`).

### C6 — **esprintar con armas de 2 manos** (`VICEEXT_SPRINT_HEAVY`, ya encendido)

- **El fallo real**: el motor ya deja esprintar (mi helper de C3 `CanSprintWithCurrentWeapon`
  devuelve `true`), pero la **tabla de animaciones** pone el **mismo** clip en el
  hueco de correr y de esprintar, así que el esprint no se ve:
  `aPlayer2ArmedAnimations` → `run_armed` (debe ser `sprint_armed`),
  `aPlayerWithRocketAnimations` → `run_rocket` (`sprint_rocket`),
  `aPlayerChainsawAnimations` → `run_csaw` (`sprint_csaw`).
- Los tres clips existen en el `ped.ifp` del mod (comprobado). Es un cambio de
  tres cadenas en `AnimManager.cpp`.

### C7 — **apuntar con la escopeta**

- **Ancla**: `WEAPONFLAG_CANAIM` (`PlayerPed.cpp` decide con ese flag si la
  recámara permite apuntar). La escopeta no lo lleva. La paridad con el mod
  (v1.5 «Changed aiming animations») pide que sí: se añade el flag a las
  escopetas, detrás de `VICEEXT_WEAPON_SIGHTS` (es la misma familia: su columna
  27 de `weapon.dat`).
- **Riesgo**: con la escopeta apuntando, la animación de apuntar es la de
  disparo; si no existe `SHOTGUN_aim`, se usa la de fuego (no se inventan
  clips).

## 4. Orden de implementación (un build, al final de todo)

1. C1b (tecla V) + C1c (aviso a sección 1) — 5 líneas.
2. C3.1b (traza + guardas) — trazas primero, para que la próxima partida diga la
   verdad.
3. C3.3b (explosión) — media hora.
4. C2b (ranura) — menú + guarda.
5. C6 (tres cadenas) + C7 (un flag).
6. C5 (agachado) + C4 (nadar) — los dos que tocan `AnimationId.h`/`AnimManager.cpp`
   (van juntos para no compilar dos veces).
7. Build **uno solo** al terminar, `seccion3-log-check.mjs` extendido para
   C4/C5/C6 y aviso en `HISTORIAL.md`.

## 5. Qué debe verse en la partida de verificación

| Bloque | Acción | Traza que lo cierra |
|---|---|---|
| C1b | **V** a pie | `CAM1P … tog=1 mode=41` (y `togkey=1` en la pulsación) |
| C1b-2 | conducir sin tocar el ratón | `CAM1P … auto=1` (nueva) |
| C2b | Esc → CARGAR PARTIDA | la 9ª sale **primero**, rotulada autoguardado |
| C3.1b | disparar y **R** | `VICEEXT reload key` + `reload start` + `reload done manual=1` |
| C3.3b | tiro al depósito | `VICEEXT gastank hit … explota=1` |
| C4 | tirarse al agua | `VICEEXT swim enter` / `swim move spd=…` / `swim exit` |
| C5 | **C** a pie | `VICEEXT crouch on/off` |
| C6 | esprintar con rifle | `VICEEXT sprint grupo=… pesada=1` |

# PLAN v3 — 20/09 (tarde-noche): la 4ª partida, la investigación de fuera y el cierre

## 0. El dato que ordena todo: esa partida se jugó con el build **ANTERIOR**

La 4ª partida es la sesión `18:42:35Z → 18:47:31Z` de `web/odtrace.log`, y su
primera línea lo dice: `JS build=2026-09-20-ve12 data=2026-09-20-ve10`. Es decir,
**el jugador probó `ve12` (enlace de las 13:20), que es el build de ANTES de todo
el PLAN v2**. Mi código de v2 se enlazó a las **14:20 (`ve13`)** y se ha
**reenlazado a las 14:41 (`ve14`, este turno)**; entre la partida y el primer
enlace de v2 no pasó ni una sesión de juego.

Consecuencia práctica, queja por queja (las mías):

| Queja del jugador | ¿Estaba el código en `ve12`? | ¿En qué build está ya? |
|---|---|---|
| no hay 1ª persona con **V** | No (el conmutador estaba en **B** y V cambiaba de cámara) | `ve13`→`ve14`, C1b |
| **R** no recarga y no dice nada | Sí pero con 4 salidas **silenciosas** y la lista de estados estrecha | `ve13`→`ve14`, C3.1b (traza en cada salida) |
| el depósito **arde** y no explota | Sí (dejaba la salud en 250 para que el motor lo incendiara) | `ve13`→`ve14`, C3.3b (explosión inmediata) |
| **slot extra** sin nombre, mal colocado y reescribible | Parcial (salía al final y sin rótulo) | `ve13`→`ve14`, C2b |
| no hay **autocentrado** de cámara en coche | No | `ve13`→`ve14`, C1b-2 |
| no puedo **agacharme** | No | `ve13`→`ve14`, C5 |
| no puedo **correr con armas de 2 manos** | No | `ve13`→`ve14`, C6 |
| no puedo **apuntar con la escopeta** | No | `ve13`→`ve14`, C7 |
| no puedo **nadar** | No | `ve13`→`ve14`, C4 |

Lo que **sí** cabe atribuir a esa sesión de mi sección: C3.5 (luces) funcionó
(`VICEEXT luces rota model=156 luz=taillight_l/headlight_l/headlight_r`) y C3.3
se disparaba (12 líneas `gastank hit`), confirmando que el gancho de bala llega en
partida real. Lo demás estaba sin escribir.

## 1. Lo que dejó la traza de esa partida (mi sección)

| Traza | Lectura |
|---|---|
| `CAM1P … tog=0 togkey=0` ×7, y **0 líneas** `VICEEXT 1p key` | El conmutador no se encendió ni una vez y la traza de la tecla (C1b) **no estaba** en ese wasm: no se puede saber si V llegó. Se recompila y se vuelve a mirar (ahora sí: `VICEEXT 1p key` sale con **cada** pulsación). |
| **0 líneas** `VICEEXT reload` | Ni éxito ni rechazo: el bloque salía por 4 sitios sin dejar rastro. Arreglado en C3.1b: `reload key` (la tecla llegó) + `reload no motivo=…` (por qué no) + `reload start`/`reload done manual=1`. |
| `VICEEXT gastank hit model=159 hp=250→225→…→25` (10 tiros seguidos) | El jugador disparó 10 veces al mismo depósito y el coche no explotaba: era el contrato viejo (arder → explotar a los ~5 s). Ahora el primer tiro lo vuela. |
| `VICEEXT gastank sin-dummy model=207/204/197/165/6504` | Sólo **25 de ~100** `.dff` traen `petrolcap`. El jugador lo vio como "le disparo al depósito de este coche y no pasa nada": de ahí el **C3.3c** de este turno (respaldo por caja de colisión). |
| `ODSHORT open-fail models/gta3.img/{BFOBE,WMYLG,WFYLG,…}.{dff,txd}` + `CARFAIL status=254` ×8 y el log **se corta en seco** | NO es mío: son skins/modelos del mod que el IDE pide y el `.img` no tiene (sección 1). Es el instante del cierre, así que es el candidato número uno del "se crashea". |
| `[texconv] clamp 9->7/9->4 niveles` (30 líneas) | NO es mío (sección 1, D10/LOD): recorta mips pequeños, no el nivel 0. |

## 2. Investigación de fuera (mods públicos, con lo que se queda cada bloque)

Regla: **primero dato/animación ya servida, después código nuevo, siempre detrás
de su `VICEEXT_*`**. Nada de copiar código de mods: se copian **decisiones**.

- **Nadar.** *GTA Vice City (Stories Style Swimming)* (ModDB, 2022, 11,6 K
descargas) y *Swimming in GTA VC with New Animation* (libertycity, 17,2 K) y
*The ability to swim in GTA Vice City* (Serega 2012, 5,6 K). De sus listas de
**fallos conocidos** salen dos decisiones de mi implementación:
  1. *"It will not work to wet your feet on the shore, since upon contact with
     water the swimming mode immediately turns on"* → aquí el estado de nado
     exige **agua por encima de la cintura** (`VICEEXT_SWIM_DEEP_MARGIN = 0,90 m`),
     así que en la orilla se anda normal.
  2. *"If you just enter the water, the swimming animation will be, but there
     will be no forward movement"* → el movimiento **no** lo pone el clip: lo
     pone el pad (rumbo de cámara + stick) y el clip sólo cambia
     (`swim_tread`/`swim_breast`/`swim_crawl`), igual que en la 1ª persona.
  Y su requisito de dato ("`ped.ifp` con Drown/FALL/breaststroke/crawl") aquí ya
  está cubierto: el `ped.ifp` **del mod** (272 clips) trae los cuatro y se sirve
  desde D5, así que no se toca `gta_vc_browser/`.
- **Agachado.** *SA Crouch Movement* (libertycity, 27/03/2026, 2,2 K): agacharse y
  **moverse agachado** (y rodar) "como en San Andreas", con `Crouch_*` del
  `ped.ifp`. Nuestra versión hace el agachado + andar agachado (los tres clips
  `crouch_idle/forward/backward` que el mod ya sirve, superpuestos como animación
  PARCIAL para no reimplementar el movimiento). **La voltereta (`Crouch_Roll_L/R`,
  que también viene en el `ped.ifp`) queda fuera**: necesita su propia entrada y
  máquina de estado, y la queja era andar agachado. Anotado como futuro.
- **Esprintar con arma de 2 manos.** *Sprinting With Two Handed Weapons [VC]*
  (8,6 K) y *Sprint With All Weapons + Swimming* (libertycity, 08/2026, 9,2 K;
en sus comentarios confirman que **Vice Extended** lo incorporó). Ambas resuelven
lo mismo que la nuestra: el clip del hueco de esprint del grupo del jugador
(`sprint_armed`/`sprint_rocket`/`sprint_csaw`, también en el `ped.ifp` servido).
- **Recarga a mano.** Familia *Reload Mod* / *Manual Aiming v1.5*: **R**,
  rellenar desde la munición total al final de la animación y **no recargar si el
  cargador está lleno**. Es exactamente el contrato que ya tenía C3.1; el fallo
  era de instrumentación/estado, no de contrato.
- **1ª persona.** *First Person View for VC* (50 K+, conmutador con **V**) y
  *Ultimate First Person Mod* (SA): las tres piezas son cámara en la cabeza con
  mirada de ratón, **retícula sólo al apuntar** y **autocentrado sólo en
  vehículos**. Las tres están: C1/C1b (V), C1c (sección 1, D6b) y C1b-2.
- **Depósito.** El changelog del propio mod (v2.5) dice literalmente *"2) Gas
  tank. When shot, the car explodes"*: **explosión**, no incendio. Ningún mod de
  la comunidad lo hace arder primero.
- **COCHE DE POLICÍA / balas de pistola en drive-by (NO míos).** El `Help.txt`
  del mod: *"to switch between pistol and submachine gun in a vehicle, press the
  change weapon keys while aiming"*, y *Manual Driveby* (9,2 K) para "sonidos
  correctos por arma". Reparto: sección 2 (P2).

## 3. Lo que he hecho en este turno (además de lo ya implementado)

1. **C3.3c — respaldo del depósito por caja de colisión.** `Weapon.cpp`:
   `ViceExtGasTankPoint()` devuelve el punto del depósito por el dummy
   `petrolcap` **o**, si el modelo no lo trae, por la caja de colisión del
   vehículo (el eje local Y es el morro en este motor → el depósito es
   `y = min.y + 6 % del largo`, `x = 0`, `z = min.z + 30 % de la altura`, radio
   0,75 m). Traza nueva `VICEEXT gastank reserva model=N` (una por modelo) y
   `gastank hit … reserva=1` cuando se ha usado el respaldo. Así el `bobcat` y
   compañía también explotan, que era la queja ("**algunos** carros…").
2. **Herramienta `tools/seccion3-log-check.mjs` ampliada a 10 bloques**: C1
   (+`VICEEXT 1p key`), C1b-2 (`camauto`), C2a, C2b, C3.1 (con el aviso de que
   un `reload done` sin `manual=` es de un build viejo y **no vale**), C3.3
   (ahora cuenta `reserva=1` y avisa de las líneas del contrato viejo), C3.5, C4
   (nado), C5 (agachado) y C6 (esprint). Sigue sin lanzar navegador: se juega una
   vez y luego `node gta_vc_browser/tools/seccion3-log-check.mjs`.
3. **Un solo build, `ve14` (14:41)**, con todo dentro (comprobado con
   `grep -a` sobre el wasm: `gastank reserva`, `1p key`, `camauto`, `swim move`,
   `crouch %s arma`, `sprint grupo`, `playerswim`, `playercrouch`,
   `sprint_armed`, `reload key` y el rótulo `AUTOGUARDADO` en UTF-16).
   `dataTag` sigue `ve10` (no hay datos nuevos).

## 4. Lo que NO es mío (con la evidencia ya reunida para su sección)

| Queja | Dueño | Evidencia |
|---|---|---|
| cinemáticas que no cargan + cuelgue | 3 (¿CutsceneMgr?) **+ 1 (capa on-demand)** | `WEBHB … cut=0` toda la sesión; los actores de cinemática (`vice1/vice2/vicechee/chopper`) se piden y **se aplazan** (`ODSHORT open-fail`). Ver J2/§1. |
| cuelgue con 3 estrellas | 2 (P1) | En esta traza `WANTED` sólo llega a `lvl=1`. |
| texturas de LOD que no suben de nivel | 1 (D10) | `[texconv] clamp 9->7` sólo recorta mips; falta `LODLEFT` del build nuevo. |
| retícula visible con el arma en la mano | 1 (D6b, hecho) | `renderer/Hud.cpp`. |
| balas de SMG al disparar la pistola desde el coche | 2 (P2) | `Ped.cpp:4876 outcome="switch-smg"`. |
| sirena del coche de policía (la foto) | 2 | su `VICEEXT_POLICE_BIKE_LIGHTS` es la moto (6507). |
| el cuelgue en sí | 1 (J1) | `JSERR`/`ENGERR` ya vuelcan el motivo en `odtrace.log`: la última línea antes del corte lo dice. |

## 5. Protocolo mínimo para cerrar los 10 bloques (una sola partida)

1. **Ctrl+Shift+R** (el wasm servido es el de **14:41**, etiqueta **ve14**).
2. **V** a pie y andar (C1/C1b) → apuntar y salir.
3. Conducir recto ~2 s sin tocar el ratón (C1b-2).
4. Esc → **GUARDAR PARTIDA** (C2b) y superar una misión (C2a).
5. Disparar unas balas y **R** (C3.1).
6. Un tiro al tapón de un coche **con** dummy y otro a uno **sin** dummy (C3.3/C3.3c).
7. Un tiro a un faro (C3.5).
8. Tirarse al mar y nadar (C4); **C** a pie sin arma (C5); **Shift** con rifle (C6).
9. `node gta_vc_browser/tools/seccion3-log-check.mjs` → dice qué ha dejado traza
   y qué no, y qué acción falta.

---

# PLAN v4 — 21/09 (7ª partida: "todas mentiras") — build `ve17`

Lo que el jugador probó y lo que era, en corto (detalle completo en
`.agents/HISTORIAL.md`, entrada 08:40):

- **C1b-3 — la V no llegaba al motor.** No era el binding: es que la **config de
  controles guardada en el navegador** carga por encima de los valores por defecto
  y deja las acciones NUEVAS sin tecla (`rsNULL` = 1056; su log lo enseña en la
  recarga: `reload key tecla=1056`). Corregido con
  `CControllerConfigManager::ViceExtActionKeyJustDown(acción, respaldo)`, usado por
  V (1ª persona) y por C (agachado). **Regla nueva:** cualquier acción que añadamos
  se lee por esta vía, no con la acción a pelo.
- **C1b-2 — el retorno estaba en una cámara que no se juega.** Se aplicaba sólo en
  `Process_FollowCar_SA` (necesita `bFreeCam`, que nace apagado); la de serie es
  `Process_Cam_On_A_String`. Ahora está en las dos, y el detector de "estoy
  mirando" pasa de `delta != 0` (con el ratón apoyado daba SIEMPRE 1:
  `camauto auto=0 mirando=1`) a una **ventana de ~1 s con suma con signo**. Traza
  nueva `camauto2`.
- **C2b-2 — la lista de CARGAR vuelve a 8 filas**: autoguardado + ranuras 1..7
  (rótulo AUTOGUARDADO). `GTAVCsf8.b` deja de ser accesible desde el menú; el menú
  de guardar lista las mismas 1..7. Si algún día se quiere recuperar la 8ª, hay
  que volver a 9 filas o mover la del autoguardado a otra pantalla.
- **C3.3d — precisión del depósito**: 0,22 m al dummy; respaldo por CUADRO local
  (0,30 × 0,45 × 0,30) y **nada** de respaldo en motos.
- **C4b — el nado reconoce la caída al agua** (`PED_FALL`/`PED_JUMP` no cumplen
  `IsPedInControl()`, y era justo el momento de nadar). Traza `swim no` con motivo.
- **C5b — el agachado ya no es el de disparo del motor**: estado propio, movimiento
  propio (0,5 m/s) con los clips del mod, y las tres ramas de `SetDuck` del jugador
  apagadas detrás de `VICEEXT_ENGINE_DUCK_KEY()`.

**Verificación pendiente (una partida):** V a pie y andar; C y andar agachado;
mar; disparo al tapón; y en coche soltar el ratón ~2 s. Con esas cinco cosas el
log dice el resto sin sondas.

