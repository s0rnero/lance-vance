# Análisis A — wanted / estrellas / persecución

Worker 1 de 3. Solo lectura: no se ha tocado `src/` ni `mods/`.

## Fuentes comparadas

| Etiqueta | Ruta |
| --- | --- |
| OUR | `src/core/Wanted.cpp`, `src/core/Wanted.h`, `src/peds/PedFight.cpp`, `src/renderer/Hud.cpp`, `src/control/CarCtrl.cpp`, `src/control/CarAI.cpp`, `src/peds/CopPed.cpp`, `src/control/PathFind.cpp` |
| MOD | `gta_vc_browser/tmp/upstream-gta-extended/src/...` (mismos ficheros, bloques `IMPROVED_TECH_PART` / `WANTED_PATHS`) |
| BASE | `C:\Users\s0rno\OneDrive\Documents\re3\src\core\Wanted.cpp` (sin mods) |
| DATOS | `mods/extended/GameFiles/modloader/ViceExtended/maps/wanted_paths.ipl`, `gta_vc_browser/bootseed.list` |

## 1. Veredicto corto

El núcleo de wanted/persecución **está trasplantado fielmente**: las 17 ramas
`VICEEXT_IMPROVED_TECH_PART` de `Wanted.cpp` y las 14 de `CarAI.cpp` son el mismo código que
el mod, línea a línea. No falta lógica de persecución ni de registro de cops.

Lo que sí difiere son cuatro cosas concretas:

1. **HUD**: falta el bloque `WANTED_PATHS` que pinta las estrellas en color "flash" cuando
   `IsPlayerHides()`. Sin él no hay aviso visual de que se están perdiendo: desaparecen de golpe.
2. **PedFight**: falta el hook del mod que marca "me han visto" cuando un cop te embiste.
3. **Invenciones del port en `CarCtrl.cpp`** (`RecountLawEnforcerCars`, `OdKeepPatrolCarsAlive`,
   escape `NumLawEnforcerCars == 0`): no existen en el mod ni en el base y cambian la cadencia real
   de patrullas.
4. **Dependencia de datos**: el mod se apoya en `data/maps/wanted_paths.ipl` (secciones `wanted`).
   Está listado en `bootseed.list:1731`, pero hay que confirmar en ejecución que se sirve; si no
   llega, `m_bMovesToWantedPathNode` nunca se activa y los coches vuelven al `goto` del base.

Bug del propio mod, portado tal cual: `CarCtrl.cpp:2011-2013` indexa `m_pathNodes[path]` **antes**
de comprobar `path != -1` (lectura fuera de rango cuando el nodo más cercano está a más de 50 m).

## 2. Tabla bloque a bloque (ADR-021)

| Bloque | OUR | MOD | BASE | Estado |
| --- | --- | --- | --- | --- |
| `Initialise` campos `m_nLastTimeSeenPlayer`, `m_vecLastSeenPosPlayer`, `m_nTimeDelayBeforeBeingSeen`, `m_nTimeAutomaticUpdatedPosPlayer`, `m_bSearchPlayerRandomly` | 55-61 | 43-48 | no existe | `present` |
| `AreMiamiViceRequired` (≥3), `AreSwatRequired` (=4), `AreFbiRequired` (=5), `AreArmyRequired` (=6) | 71-88 | 65-82 | idéntico | `present` |
| `NumOfHelisRequired` → 2 helicópteros con 5-6 estrellas | 99-104 | 94-99 | 1 | `translated` |
| `SetWantedLevel`: fija `m_vecLastSeenPosPlayer` si level>0 | 118-121 | 112-115 | no | `present` |
| `ReportCrimeNow`: refresca `m_nLastTimeSeenPlayer` | 284 | 256 | no | `present` |
| `UpdateWantedLevel`: `m_MaxCops` 5/7/10/10 con `VICEEXT_IMPROVED_TECH_PART` | 388-411 | 360-381 | 3/4/6/8 | `translated` |
| `IsPlayerLost` = `now - lastSeen > 25000 + 5000*nivel` | 438-442 | 411-415 | no | `present` |
| `IsPlayerHides` = `now - lastSeen > (en coche ? 3000 : 500)` | 444-448 | 417-421 | no | `present` |
| `WorkOutPolicePresence`: radio 90 en coche, cono `angleOfSight < -0.25`, LOS a la cabeza, `TIME_DELAY_BEFORE_BEING_SEEN`, `m_vLastSeenPlayerVehicle`, `m_bMovesToLastPlayerPosition` | 450-660 | 423-633 | solo radio 18 | `translated` |
| `WorkOutPolicePresence` helicóptero: LOS desde `z+0.75`, refresco de lastSeen | 560-640 | 540-610 | no | `present` |
| `Update`: `if (m_nWantedLevel > 0 && IsPlayerLost()) SetWantedLevel(0)` | 663-665 | 636-638 | no | `present` |
| `Update`: recálculo aleatorio de `m_vecLastSeenPosPlayer` ±50 m | 678-686 | 647-655 | no | `present` |
| `Update`: llamada a `WorkOutPolicePresence(coors, 150.0f)` | 688 | 657 | no | `present` |
| `Update`: radio 70 (vs 18 del base) cuando nivel ≤1 | 718-722 | 693-697 | 18 | `translated` |
| `Update`: reparación del array `m_pCops` y `m_CurrentCops = currCopNum` | 725-758 | 660-693 | idéntico | `present` |
| `ResetPolicePursuit` (`m_CurrentCops = 0`) | 768-786 | 706-721 | idéntico | `present` |
| **Port-only**: `if (!IsPlayerOnAMission() && MaximumWantedLevel < 6) { MaximumWantedLevel = 6; nMaximumWantedLevel = 7200; }` | 671-676 | no existe | no existe | `excluded` (workaround de un opcode de misión, ajeno a la policía) |
| `CarAI::BackToCruisingIfPlayerLost` | 61-70 | 58-67 | no | `present` |
| `CarAI::UpdateCarAI` rama `VICEEXT_WANTED_PATHS` (`m_bMovesToWantedPathNode`, `FindClosestWantedPathsToCoors`, `TellCarToSeekWantedPaths`, velocidad 15/30) | 78-105 | 74-101 | no | `present` (depende de datos, ver §4) |
| `CarAI::FindPoliceCarMissionForWantedLevel` (BLOCK/ RAM por estrellas) | 864-878 | 837-851 | idéntico | `present` |
| `CarAI::FindPoliceCarSpeedForWantedLevel` (34 si `IsPlayerHides`) | 892-897 | 865-870 | no | `present` |
| `CarAI::TellCarToSeekWantedPaths` / `TellOccupantsToLeaveCarImmediately` | 808-862 | 770-826 | no | `present` |
| `CopPed`: alta en `m_pCops` y baja al morir/abandonar | 258-271, 313-317 | 261-274, 314-318 | idéntico | `present` |
| `CarCtrl::GenerateOneRandomCar`: gate `nivel>1 && NumLawEnforcerCars < m_MaximumLawEnforcerVehicles && m_CurrentCops < m_MaxCops`, cadencia 5 s (≥3) / 8 s (≥2) | 275-284 | 169-173 | idéntico | `present` |
| `CarCtrl::UpdateCarCount` (+/− `NumLawEnforcerCars`) | 3604-3655 | 3390-3428 | idéntico | `present` |
| `CarCtrl::PossiblyRemoveVehicle` / `RemoveDistantCars` con `!IsPlayerHides()` para peds de barricada | 1143-1146 | 959-966 | parcial | `present` |
| `CarCtrl::PickNextNodeAccordingStrategy` rama `WANTED_PATHS` | 2002-2025 | 1789-1812 | no | `translated` + bug `m_pathNodes[-1]` (índice calculado en la línea anterior a `path != -1`) |
| **Port-only**: `OdKeepPatrolCarsAlive()` (revive coches sin conductor, les pone misión) y su llamada con nivel>1 | 157-195, 237 | no existe | no existe | `excluded` |
| **Port-only**: `RecountLawEnforcerCars()` (recuento a 1 Hz, filtra `VehicleCreatedBy == RANDOM_VEHICLE`) + llamada + volcado `COPST/COPVEH` | 3658-3669, 209-236 | no existe | no existe | `excluded` |
| **Port-only**: escape `|| NumLawEnforcerCars == 0` en el gate de generación | 281 | no existe | no existe | `excluded` |
| `Hud.cpp`: estrella sólida y parpadeo 2 s tras cambio de nivel (`m_nLastWantedLevelChange`) | 986-992 | 833-840 | idéntico | `present` |
| `Hud.cpp`: **estrella en `WANTED_COLOR_FLASH` cuando `IsPlayerHides()`** | no existe | 841-849 | no existe | **`gap`** |
| `PedFight.cpp`: cop embiste al jugador → `m_nLastTimeSeenPlayer = now` | no existe | 3841-3844 | no existe | **`gap`** |

## 3. Respuestas a las preguntas de la ficha

**¿Se registran y desregistran los cops igual?** Sí. `m_pCops[10]`, alta en
`CopPed.cpp:313-317`, baja en `CopPed.cpp:258-271` (muerte/abandono), y `m_CurrentCops` se
reconcilia cada segundo en `Wanted.cpp:725-735` (OUR) / `660-670` (MOD). No hay rama OUR distinta.

**¿Puede quedarse `m_CurrentCops` clavado en el máximo cerrando la generación?** Por lógica, no:
el recuento se repara solo contra el array. Solo se quedaría lleno si los cops no se desregistran
nunca (no mueren, no abandonan). El port ya emite `VEX wanted ... cops=%d max=%d` (`Wanted.cpp:693-704`)
y `COPST lvl/lne/cap/cops/max/rand/traf` (`CarCtrl.cpp:212-220`): es la medición que falta, no un gap de código.

**Sitios que refrescan `m_nLastTimeSeenPlayer`.** OUR: 56 (init), 284 (`ReportCrimeNow`), 544 y 618
(`WorkOutPolicePresence`), 640 (flanco de reset), 669 (nivel 0). MOD: 44, 256, 514, 585, 607, 627 y
**`PedFight.cpp:3841-3844`**, que en OUR no existe. Es el único sitio del mod que falta.

**¿Pérdida de estrellas igual que el mod?** Sí. El mod no decae caos con nivel >1: al esconderse,
`IsPlayerLost()` (25 s + 5 s por estrella) dispara `SetWantedLevel(0)` y el nivel cae de golpe. La
diferencia que percibe el jugador está en el HUD, no en `Wanted.cpp`: el mod pinta las estrellas en
`WANTED_COLOR_FLASH` mientras `IsPlayerHides()` (500 ms a pie, 3 s en coche) antes de perderlas.

## 4. Desviaciones que explican los síntomas

**(a) Apenas salen patrullas / el cheetah no aparece.**
- El tope de "coches law-enforcer aleatorios" es del juego base y del mod: 2 con 4 estrellas, 3 con
  5-6 (`Wanted.cpp:400-422`). Lo que en viceex da sensación de 10 coches son las barricadas
  (`m_RoadblockDensity` 18/24/30) y los coches que llegan por sus rutas de búsqueda, no el generador.
- En OUR el gate se abre de más con `NumLawEnforcerCars == 0` y `RecountLawEnforcerCars` cierra el
  contador a 1 Hz: entra un coche, el recuento lo cuenta y el gate se cierra 5-8 s. Resultado: goteo
  de 1-2 coches en vez de una presencia sostenida. Es una invención, no paridad.
- `OdKeepPatrolCarsAlive()` reutiliza coches vacíos en vez de dejar que el motor los sustituya, y
  reescribe misión/velocidad de coches que el mod dejaría gestionar a `CarAI`.
- El cheetah (`MI_VICECHEE`) exige `HasModelLoaded(MI_VICECHEE)` y dos modelos `VICE` residentes
  (`CarCtrl.cpp:1065-1095`). En escritorio están casi siempre residentes; en el port el streaming de
  vehículos va limitado (`Streaming.cpp:1889-1898`, ventana de 350 frames/500 ms y
  `ms_numVehiclesLoaded <= desiredNumVehiclesLoaded`), así que puede devolver `MI_POLICE` siempre.
  Requiere medición (`HasModelLoaded(MI_VICECHEE)` con 3+ estrellas), no es un gap de lógica.

**(b) Las estrellas desaparecen de golpe.**
- `Hud.cpp:986-992` no tiene la rama `WANTED_PATHS` del mod (`Hud.cpp:841-849`): nunca se pinta el
  color flash mientras `IsPlayerHides()`. Al caer el nivel, la fila pasa de sólida a nada.
- `PedFight.cpp` sin el hook del mod: embestir/rozar a un cop no refresca "me han visto", así que el
  cronómetro de pérdida sigue corriendo aunque tengas un cop encima.

**(c) Los cops no conducen / se estrellan / se bajan.**
- La lógica es idéntica al mod, así que la diferencia está en sus entradas. La principal es la ruta de
  búsqueda: sin nodos `wanted` cargados, `FindClosestWantedPathsToCoors` devuelve todo a cero,
  `m_bMovesToWantedPathNode` no se activa y además se salta el `else if` que refresca
  `m_nCruiseSpeed` (`CarAI.cpp:100-108`) mientras `IsPlayerHides()`. El coche persigue por el sistema
  de calles del base, con `JoinCarWithRoadSystemGotoCoors`, y acaba empotrado.
- `TellOccupantsToLeaveCarImmediately` (`CarAI.cpp:831-862`) se dispara al llegar a un nodo cero o a
  menos de 10 m del jugador con el coche parado: es la vía por la que "se bajan en vez de perseguir".
- El mod tiene un fallo de índice (`CarCtrl.cpp:2011-2013`, `m_pathNodes[path]` con `path == -1`) que
  en el port se copió igual; leer 40 bytes antes del array puede devolver basura y alterar la decisión
  de perseguir/abandonar.

## 5. Lo que NO está roto (no tocar)

- Valores de estrellas, caos y `m_MaxCops` (5/7/10/10/10) con `VICEEXT_IMPROVED_TECH_PART`: iguales al mod.
- `m_MaximumLawEnforcerVehicles` 1/2/2/2/3/3 y cadencia 5 s/8 s: iguales al mod.
- `MaxNumberOfCarsInUse` = `DEFAULT_MAX_NUMBER_OF_CARS(20.0) × CarNumberMultiplier(1.5)` = 30, y
  `PedNumberMultiplier` 1.5: iguales al mod (`IniFile.h:4-6`, `IniFile.cpp:10-12`).
- Misiones de policía (BLOCK/RAM por estrellas), velocidades, `MellowOutChaseSpeed`, salida del
  coche con `!IsPlayerLost()`: iguales al mod.
- Barricadas y su gating por `IsPlayerHides()`: presentes.

## 6. Verificación pendiente (solo lectura de logs / ejecución del jugador)

1. `data/maps/wanted_paths.ipl` servido y parseado: contar líneas `wanted` en el fichero y confirmar
   que existe en la carpeta de datos que sirve el port (el `bootseed.list` ya lo lista).
2. En una partida con 3+ estrellas, una traza de `HasModelLoaded(MI_VICECHEE)` y de
   `pVehicle->AutoPilot.m_bMovesToWantedPathNode` en un coche de policía.
3. `VEX wanted` + `COPST` durante una persecución de 2 minutos: si `cops` llega a `max` y no baja,
   el problema es desregistro de cops en ejecución, no el gate de generación.
