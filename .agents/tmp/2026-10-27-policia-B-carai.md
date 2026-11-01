# Worker 2 — Conducción y persecución policial (CarAI / CarCtrl)

Fuentes comparadas:

- OUR: `src/control/CarAI.cpp`, `src/control/CarCtrl.cpp`, `src/control/RoadBlocks.cpp`, `src/core/Streaming.cpp`
- MOD: `gta_vc_browser/tmp/upstream-gta-extended/src/control/CarAI.cpp`, `.../CarCtrl.cpp`
- BASE: `C:\Users\s0rno\OneDrive\Documents\re3\src\control\CarAI.cpp`, `.../CarCtrl.cpp`

Sin ediciones en `src/`. Todo lo que sigue está verificado leyendo el código; lo que no, va marcado como **candidato**.

## 1. Tabla bloque a bloque

| Bloque | OUR | MOD | BASE | Estado |
| --- | --- | --- | --- | --- |
| `FindSwitchDistanceClose/Far` (CarAI.cpp:31-46) | idéntico | idéntico | idéntico | present |
| `BackToCruisingIfNoWantedLevel` (CarAI.cpp:48-58) | idéntico | idéntico | idéntico | present |
| `BackToCruisingIfPlayerLost` (CarAI.cpp:62-73) | idéntico | idéntico | no existe | present |
| `UpdateCarAI`, casos `RAMPLAYER/BLOCKPLAYER` (CarAI.cpp:77-260) | idéntico al mod, incluidos los `IsPlayerLost()` de salida y el `m_nTimeBlocked` de bajarse del coche | — | — | present |
| `FindPoliceCarMissionForWantedLevel` (CarAI.cpp:864-876) | idéntico | idéntico | idéntico | present |
| `FindPoliceBoatMissionForWantedLevel` (CarAI.cpp:878-890) | idéntico | idéntico | idéntico | present |
| `FindPoliceCarSpeedForWantedLevel` (CarAI.cpp:892-908) | idéntico, incluido el 34 con `IsPlayerHides()` | idéntico | idéntico | present |
| `MellowOutChaseSpeed` (CarAI.cpp:910-960) | idéntico | idéntico | idéntico | present |
| `AddPoliceCarOccupants` (CarAI.cpp:651-727) | rama `VICEEXT_POLICE_BIKE` **inventada** (espera al bloque de anim `bikeh` y aplaza conductores) | rama `EX_VCPD_WINTERGREEN` sólo pone conductor, sin espera | no existe | **invented** |
| `COPS_BOAT` al generar (CarCtrl.cpp:511-520 OUR / 410-424 MOD) | sin la rama `IsPlayerHides → MISSION_BLOCKPLAYER_FARAWAY` | la tiene | no existe | **gap (sólo barcos)** |
| `SteerAICarWithPhysics_OnlyMission` (CarCtrl.cpp:2724-2850 / MOD 2511-2637) | idéntico | idéntico | idéntico | present |
| `SteerAICarWithPhysicsFollowPath` (CarCtrl.cpp:3100-3235) | idéntico | idéntico | idéntico | present |
| `SteerAICarWithPhysicsTryingToBlockTarget[_Stop]` (CarCtrl.cpp:3269-3360) | idéntico | idéntico | idéntico | present |
| `PickNextNodeAccordingStrategy` (CarCtrl.cpp:1998-2069 / MOD 1785-1856) | idéntico, incluidos dos defectos latentes (ver §3.3) | idéntico | versión sin `WANTED_PATHS` | present |
| `ChoosePoliceCarModel` (CarCtrl.cpp:1065-1100) | idéntico + rama moto `VICEEXT_POLICE_BIKE` | idéntico + rama `EX_VCPD_WINTERGREEN` | sin moto | present |
| Gate de generación policial (`GenerateOneRandomCar`, CarCtrl.cpp:275-285) | cap del mod **+ escape inventado** `|| NumLawEnforcerCars == 0` | cap propio | cap propio | **invented** |
| Cadencia `LastTimeLawEnforcerCreated +5000/+8000` | idéntico | idéntico | idéntico | present |
| `MaxNumberOfCarsInUse` = 20 con `IMPROVED_TECH_PART` (`IniFile.h:6-10`) | 20 | 20 | 12 sin flag | present |
| `DISTANCE_TO_SPAWN_ROADBLOCK_PEDS` 80/51 (`CarCtrl.cpp:47-51`) | idéntico | idéntico | 51 | present |
| `RemoveDistantCars` + peds de barricada (CarCtrl.cpp:1138-1152) | idéntico (incluye `!IsPlayerHides()`) | idéntico (línea 962) | sin el `!IsPlayerHides()` | present |
| `CRoadBlocks::GenerateRoadBlocks` (RoadBlocks.cpp:130-136) | `if (IsPlayerHides()) return;` | la misma condición | no existe | present |
| Contador `NumLawEnforcerCars` (`UpdateCarCount`, CarCtrl.cpp:3609-3653) | idéntico al base | idéntico | idéntico | present |
| `RecountLawEnforcerCars` (CarCtrl.cpp:3658-3667) | **no existe en el mod ni en la base**; recuenta cada 1000 ms en `GenerateRandomCars` | — | — | **invented** |
| `OdKeepPatrolCarsAlive` (CarCtrl.cpp:146-192, llamada en :239) | **no existe en el mod**: cada fotograma resucita patrullas `STATUS_ABANDONED/SIMPLE`, les pone conductor si están a más de 60 m y les **reasigna misión y velocidad aleatorias** | — | — | **invented** |
| `OdFinitePosition` en `PossiblyRemoveVehicle` (CarCtrl.cpp:1188-1198) | purga inventada de coches con posición no finita | — | — | **invented** |
| Límite de vehículos del streaming (`CStreaming::LoadRequestedModels`, Streaming.cpp:2468) | rechaza cargar un vehículo si `ms_numVehiclesLoaded >= desiredNumVehiclesLoaded` y no hay ranura/vehículo removible | igual | igual | present (heredado) |

## 2. Respuestas concretas

### (a) Por qué aparecen pocos coches de policía

1. **El tope por estrellas es del juego original, no un invento nuestro.** `Wanted.cpp` fija `m_MaximumLawEnforcerVehicles` = 1/2/2/2/3/3 para 1..6 estrellas y `GenerateOneRandomCar` exige `NumLawEnforcerCars < m_MaximumLawEnforcerVehicles`. El mod Extended tiene exactamente el mismo tope (con `IMPROVED_TECH_PART` sólo sube `m_MaxCops` a 5/7/10/10/10, que es el número de policías a pie). Con 4 estrellas el juego base sólo genera **2** patrullas de tráfico; el resto de coches que se ven en viceex salen de **barricadas** (`CRoadBlocks`) y de los coches con peds scriptados.
2. **En nuestro port esas dos plazas quedan ocupadas por coches que no persiguen.** `OdKeepPatrolCarsAlive` resucita cada fotograma cualquier patrulla `STATUS_ABANDONED/SIMPLE` (les pone conductor y `STATUS_PHYSICS`) y `RecountLawEnforcerCars` fuerza el contador al número real de `bIsLawEnforcer && RANDOM_VEHICLE` cada segundo. Esas patrullas están a menos de 160 m (`ONSCREEN_DESPAWN_RANGE` con la feature) y no se borran, así que el contador se queda en 2 y el gate no vuelve a abrir. Ese par de funciones no existe en el mod: es una fila `invented` que hay que retirar.
3. **Las barricadas están apagadas por completo cuando `IsPlayerHides()` es cierto**, y no es un cambio de este slice: OUR `RoadBlocks.cpp:134-136` y `CarCtrl.cpp:1144` son idénticos al mod. Los coches de barricada se crean con `SetClumpAlpha(..., 0)` y sólo se les ponen peds cuando `!IsPlayerHides()`; si esa función devuelve `true` con el jugador perseguido, se ve exactamente lo que describes: barreras, coches invisibles y nadie disparando. La causa raíz está en `CWanted` (worker 1); aquí queda confirmado el efecto.

### (b) Por qué los coches policiales se estampan o se quedan atascados

1. `OdKeepPatrolCarsAlive` **reasigna la misión y la velocidad cada fotograma** a las patrullas que resucita (`m_nCarMission = FindPoliceCarMissionForWantedLevel()`), lo que en la práctica reinicia la persecución continuamente: el coche cambia de estrategia (`RAMPLAYER` ↔ `BLOCKPLAYER`) mientras ya está en una maniobra, y puede acabar empotrado contra una pared sin que `PickNextNodeAccordingStrategy` llegue a sacarlo. El mod no tiene nada equivalente: la transición FARAWAY→CLOSE y el `JoinCarWithRoadSystemGotoCoors` las gobierna sólo `CCarAI::UpdateCarAI`.
2. Las misiones, velocidades, embestidas y bloqueos (`MISSION_RAMPLAYER_CLOSE`, `MISSION_BLOCKPLAYER_CLOSE`, `SteerAICarWithPhysicsTryingToBlockTarget`, `MellowOutChaseSpeed`) están transplantados **bloque a bloque idénticos** al mod, así que la "IA de persecución" en sí no está mutilada; lo que está de más es el ayudante anterior.
3. Dos defectos heredados del mod, idénticos en las tres fuentes, que conviene no tocar en este plan pero sí registrar: en `PickNextNodeAccordingStrategy` el índice `path` se usa (`ThePaths.m_pathNodes[path].GetZ()`) **antes** de comprobar `path != -1`, y `chaseCarPos` se queda sin inicializar si el conductor es un policía con nivel de búsqueda 0 (sólo alcanzable si una patrulla conserva misión `*_FARAWAY` con 0 estrellas). Ambos son `present` respecto al mod.
4. El caso "se bajan del coche en vez de perseguirme" **no es un fallo**: es la rama del mod `m_nTimeBlocked > TIME_COPS_WAIT_TO_EXIT_AFTER_STOPPING && !IsPlayerLost()` que hace bajar a los ocupantes cuando el jugador está parado a menos de 10 m. Está transplantada tal cual.

### (c) Por qué nunca aparece el Cheetah encubierto

La ruta está completa e idéntica al mod: `AreMiamiViceRequired()` (≥3 estrellas) → `CStreaming::StreamVehiclesAndPeds` pide `MI_VICECHEE` y el par de peds VICE del ciclo → `ChoosePoliceCarModel` devuelve `MI_VICECHEE` si `HasModelLoaded(MI_VICECHEE)` y los dos peds → `AddPoliceCarOccupants` monta los dos cop y fija `LastTimeMiamiViceGenerated`. Por tanto **no falta código**: el Cheetah depende de que el modelo esté cargado en el instante en que se llama a `ChoosePoliceCarModel`, y eso compite con tres cosas verificadas:

1. El gate de 2 plazas de §(a): `ChoosePoliceCarModel` sólo se evalúa cuando `GenerateOneRandomCar` decide generar policía, y con el contador clavado en el tope no se evalúa nunca.
2. El presupuesto de vehículos del streaming (`Streaming.cpp:2468`): con `ms_numVehiclesLoaded >= desiredNumVehiclesLoaded` (25 en el build de navegador, `Streaming.cpp:240`) y sin ranura libre, el modelo pedido se descarta y se vuelve a pedir; si el tráfico llena el presupuesto, `MI_VICECHEE` no llega a `LOADED`.
3. El cooldown de 120 s (`LastTimeMiamiViceGenerated + 120000`) y la condición de los peds VICE de `CPopulation::NumMiamiViceCops == 0`: si los dos cop de un Cheetah anterior siguen vivos (`NumMiamiViceCops` se descuenta en `CPed::Remove`, `Ped.cpp:505-506`), el par de peds no se vuelve a pedir y `HasModelLoaded(MI_VICE*)` puede quedar en falso.

**Candidato a verificar en el plan** (no verificado aquí, hace falta log o manifiesto de datos): que `MI_VICECHEE` y los 8 `MI_VICE*` estén realmente en los datos servidos y que lleguen a `LOADED` con 3+ estrellas. Sin esa comprobación no se puede afirmar que el Cheetah esté roto por código.

## 3. Desviaciones que explican los síntomas

1. `OdKeepPatrolCarsAlive` (CarCtrl.cpp:146-192, llamada en :239) — inventada; resucita y rearma patrullas cada fotograma. Explica coches erráticos/atascados y contribuye a congelar el contador.
2. `RecountLawEnforcerCars` (CarCtrl.cpp:3658-3667, llamada en :207-211) — inventada; fija `NumLawEnforcerCars` al número real cada segundo y mantiene el gate cerrado mientras las patrullas resucitadas existan.
3. `OdFinitePosition` en `PossiblyRemoveVehicle` (CarCtrl.cpp:1188-1198) — inventada; en teoría inofensiva, pero es una fila `excluded` sin justificar.
4. Escape `|| NumLawEnforcerCars == 0` en el gate (CarCtrl.cpp:281) — inventado; altera la condición del mod y enmascara el problema del contador en vez de arreglarlo.
5. Falta la rama `IsPlayerHides` de `COPS_BOAT` (CarCtrl.cpp:511-520) — `gap` menor, sólo barcos.
6. `AddPoliceCarOccupants`: la rama de la moto espera al bloque de animación `bikeh` y aplaza la creación de ocupantes (`CarAI.cpp:655-700`); el mod sólo monta conductor. Es una desviación funcional (la moto puede quedarse sin conductor si el bloque no carga).

## 4. Resumen

El estado de persecución del coche policial (misiones, velocidades, FARAWAY→CLOSE, embestidas, bloqueos, bajarse del coche) está transplantado **idéntico** al mod; ahí no hay nada que rehacer. Lo que rompe el comportamiento son cuatro añadidos nuestros en `CarCtrl` que el mod no tiene: un recuento periódico del contador, un "resucitador" de patrullas que rearma su misión cada fotograma, una purga de posiciones no finitas y un escape en el gate de generación. Con el tope original de 2 patrullas a 4 estrellas y esas dos plazas ocupadas por coches resucitados que no llegan a perseguir, no se generan más; y las barricadas —la fuente real de coches de policía alrededor del jugador— quedan apagadas mientras `IsPlayerHides()` sea cierto, que es donde apunta la causa raíz del sistema de estrellas. El Cheetah no tiene código que falte: su aparición depende de que el modelo esté cargado y de que el gate de 2 plazas se abra, y eso hay que verificarlo con el manifiesto de datos y un log, no suponerlo.
