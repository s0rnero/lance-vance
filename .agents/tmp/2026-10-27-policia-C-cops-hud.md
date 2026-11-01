# Análisis C — Policía a pie / salida del coche / HUD de estrellas

> Análisis de solo lectura. No se editó código. Fuentes comparadas:
> - OUR: `src/peds/CopPed.cpp`, `src/peds/PedAI.cpp`, `src/peds/PedFight.cpp`, `src/control/CarAI.cpp`, `src/control/CarCtrl.cpp`, `src/vehicles/Vehicle.cpp`, `src/renderer/Hud.cpp`, `src/core/Wanted.cpp`.
> - MOD: `gta_vc_browser/tmp/upstream-gta-extended/src/...` (bloques `IMPROVED_TECH_PART`, `WANTED_PATHS`).
> - BASE: `C:\Users\s0rno\OneDrive\Documents\re3\src\...`.

## 1. Policía a pie y salida del coche

### 1.1 Tabla bloque a bloque

| Bloque | OUR | MOD | Estado |
| --- | --- | --- | --- |
| `CCarAI::UpdateCarAI` — rama `m_bMovesToWantedPathNode` (buscar al jugador por wanted paths) | `src/control/CarAI.cpp:76-105` | `CarAI.cpp:64-93` | `present` (traducido, mismos umbrales: `IsPlayerHides()`, nodo a 50 m, 25 m, velocidades 15/30) |
| `CCarAI::UpdateCarAI` — MISSION_RAMPLAYER_FARAWAY/CLOSE, `m_nTimeBlocked`, salida del coche | `src/control/CarAI.cpp:143-200` | `CarAI.cpp:136-199` | `present` |
| `CCarAI::UpdateCarAI` — MISSION_BLOCKPLAYER_FARAWAY/CLOSE, salida del coche | `src/control/CarAI.cpp:202-260` | `CarAI.cpp:218-259` | `present` |
| Constante de espera antes de bajar | `src/control/CarCtrl.h:9` = 2500 | `CarCtrl.h:9` = 2500 | `present` |
| `CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget_Stop` (bajar a pie al bloquear) | `src/control/CarCtrl.cpp:3284-3330` | `CarCtrl.cpp:3088-3115` | `present` |
| `CVehicle::InflictDamage` — un cop no abandona el coche al recibir daño | `src/vehicles/Vehicle.cpp:1326` (`pDriver->m_nPedType != PEDTYPE_COP`) | `Vehicle.cpp:1347-1351` | `present` |
| `CCopPed::ProcessControl` — `CopAI()` también cuando `IsPlayerLost()` | `src/peds/CopPed.cpp:804-810` | `CopPed.cpp:784-786` | `present` |
| `CCopPed` — estado `m_bLookingForPlayer` / `m_bMovesToLastPlayerPosition` | `src/peds/CopPed.cpp:1056-1075` | `CopPed.cpp:1041-1075` | `present` |
| `CPedAI` — condiciones `IsPlayerHides()` en ataque/persecución/`PED_DRAG_FROM_CAR` | `src/peds/PedAI.cpp:6658, 6725, 6826, 6989` | `PedAI.cpp:6646, 6714, 6813, 6975` | `present` |
| **`PedFight` — el jugador choca con un cop: se marca "visto"** | **ausente** (0 referencias a `m_pWanted` en todo `src/peds/PedFight.cpp`) | `PedFight.cpp:3840-3843` (`if (IsPlayer() && collideWith->m_nPedType == PEDTYPE_COP) m_pWanted->m_nLastTimeSeenPlayer = now;`) | **`gap`** |

### 1.2 Qué significa el `gap`

`CWanted::IsPlayerHides()` es literalmente
`now - m_nLastTimeSeenPlayer > (InVehicle ? 3000 : 500)` (`src/core/Wanted.cpp:445-448`).
`m_nLastTimeSeenPlayer` sólo se refresca cuando un policía o un helicóptero tiene línea de visión
(`WorkOutPolicePresence`, `Wanted.cpp:514/585`) y, en el mod, además **cuando el jugador se
cruza con un policía** (`PedFight.cpp:3840`). Nuestro port no tiene esa segunda fuente.

Consecuencia medible en el código, no en el juego:
- `IsPlayerHides()` se queda en `true` en cuanto pasan 500 ms sin que un cop tenga LOS directa,
  aunque el jugador esté disparando o chocando contra ellos.
- Con `IsPlayerHides()` cierto, `CCarAI::UpdateCarAI` mete los coches en modo búsqueda
  (`m_bMovesToWantedPathNode`) en vez de `RAMPLAYER/BLOCKPLAYER` (`CarAI.cpp:82-99`).
- Con `IsPlayerHides()` cierto, `CWanted::Update` no refresca `m_vecLastSeenPosPlayer`
  (`Wanted.cpp:660-668`: `if (!IsPlayerHides()) m_vecLastSeenPosPlayer = ...`), así que el punto
  de búsqueda se queda donde te vieron por última vez.
- Tras `25 s + 5 s/estrella` sin ser visto, `IsPlayerLost()` es cierto y `CWanted::Update`
  hace `SetWantedLevel(0)` de golpe (`Wanted.cpp:656-657`).

### 1.3 ¿Se bajan del coche "en vez de perseguir"?

No hay ninguna regla propia que lo provoque; es la regla del juego base, idéntica en mod y port:

- `MISSION_BLOCKPLAYER_CLOSE` / `MISSION_RAMPLAYER_CLOSE`: si la velocidad del vehículo del
  jugador es `< 0.04–0.05`, `m_nTimeBlocked += dt`; cuando supera `2500 ms` **y** el coche policía
  está a **< 10 m**, `TellOccupantsToLeaveCar` (`CarAI.cpp:225-238`, `CarAI.cpp:171-181`).
- `SteerAICarWithPhysicsTryingToBlockTarget_Stop` hace lo mismo en el camino de "bloquear"
  (`CarCtrl.cpp:3310-3322`).

Es decir: sólo se bajan cuando **ya han llegado** y el jugador está parado. Que el jugador los vea
"bajarse en vez de perseguir" significa que llegaron mal (bloqueo/ram fallido) y se detuvieron al
lado; la selección de misión y la aproximación viven en `CarAI/CarCtrl`
(`FindPoliceCarMissionForWantedLevel`, `JoinCarWithRoadSystemGotoCoors`), no en `CopPed`.
Ese punto pertenece al análisis B.

## 2. HUD de estrellas

### 2.1 Tabla bloque a bloque

| Bloque | OUR | MOD | Estado |
| --- | --- | --- | --- |
| `CHud::DrawWantedLevel` (6 estrellas, `m_LastWanted`, `DrawFadeState(HUD_WANTED_FADING, …)`) | `src/renderer/Hud.cpp:950-1000` | `Hud.cpp:806-870` | `present` |
| Parpadeo 2 s tras cambio de nivel: `now > m_nLastWantedLevelChange + 2000 \|\| FRAMECOUNTER & 4` | `Hud.cpp:981-984` | `Hud.cpp:831-832` | `present` |
| Estrella "perdiéndose": `m_nMinWantedLevel > i && FRAMECOUNTER & 4` con `WANTED_COLOR_FLASH` | `Hud.cpp:993-995` | `Hud.cpp:857-859` | `present` |
| Estrella vacía `NOTWANTED_COLOR` | `Hud.cpp:996-999` | `Hud.cpp:862-864` | `present` |
| **Cambio de color cuando los cops te pierden de vista** (`#ifdef WANTED_PATHS` → `IsPlayerHides()` dibuja con `WANTED_COLOR_FLASH`) | **ausente**: 0 apariciones de `IsPlayerHides` en `src/renderer/` | `Hud.cpp:839-846` | **`gap`** |
| Máquina de fade `CHud::DrawFadeState` | `Hud.cpp:2482-2560` | `Hud.cpp:2269-2350` | `present`, pero **inerte en ambos**: `HUD_AUTO_FADE` no está definido en ninguno de los dos `config.h`, así que `m_WantedState = FADE_DISABLED` y `alpha` es siempre 255 |
| Colores | `WANTED_COLOR(97,194,247,255)`, `WANTED_COLOR_FLASH(62,141,181,255)` (`Hud.cpp:118-119`) | idénticos (`Hud.cpp:63-64`) | `present` |

### 2.2 Qué significa el `gap`

`WANTED_PATHS` sí está activo en nuestro port (`src/core/config.h:641`
`#define VICEEXT_WANTED_PATHS`), pero la rama de HUD que ese define habilita no se trasplantó.
En el mod, mientras `IsPlayerHides()` es cierto (los cops han dejado de verte), la estrella se
pinta con `WANTED_COLOR_FLASH` `(62,141,181)`, que sobre el HUD se lee como una estrella más
apagada/transparente; al volver a verte vuelve a `WANTED_COLOR` `(97,194,247)`.
En nuestro port la estrella mantiene el color brillante y, cuando `IsPlayerLost()` se cumple,
el nivel pasa a 0 de una sola vez (`Wanted.cpp:656-657`): el jugador ve "desaparecer de la nada",
que es exactamente el síntoma reportado.

Nota: la "transparencia" **no** puede venir del fade por alfa, porque `HUD_AUTO_FADE` está
desactivado en el mod y en el port por igual. Viene del cambio de color de esta rama.

### 2.3 Pérdida de estrellas (contexto, no HUD)

`CWanted::Update` es idéntico en mod y port:
- con `m_nWantedLevel > 1` el caos **no** decae (`Wanted.cpp:653-655`);
- la pérdida real es `IsPlayerLost()` → `SetWantedLevel(0)` en un salto (`Wanted.cpp:656-657`).
Por tanto el mod tampoco baja estrella a estrella: lo que cambia la percepción es el color de la
estrella (gap 2.1) y el parpadeo de 2 s tras el cambio.

## 3. Desviaciones que explican los síntomas

1. **Estrellas que desaparecen sin transición** → falta la rama `WANTED_PATHS` de
   `IsPlayerHides()` en el HUD (`Hud.cpp` del mod 839-846). Añadirla es el único cambio de HUD que
   hace falta; el resto del bloque ya está trasplantado.
2. **Cops que se bajan a sacarte en vez de perseguir** → no hay desviación en la regla de salida
   (idéntica al base/mod, 2500 ms + <10 m + objetivo parado). Lo que hace que lleguen y se paren
   es la aproximación/bloqueo (análisis B). Agravante propio de este análisis: sin el hook de
   `PedFight`, los cops te consideran "oculto" enseguida y pasan de perseguir a buscar.
3. **Persecución que se apaga sola** → mismo hook ausente: `IsPlayerHides()` cierto ⇒ misiones de
   búsqueda (`m_bMovesToWantedPathNode`) y `IsPlayerLost()` a los `25 s + 5 s/estrella` ⇒ nivel a 0.

## 4. Filas `excluded` (invenciones del port, no del mod)

- `src/core/Wanted.cpp:646-651`: override `if (!IsPlayerOnAMission() && MaximumWantedLevel < 6) { MaximumWantedLevel = 6; nMaximumWantedLevel = 7200; }` — no existe en el mod. Afecta al techo de estrellas y debería revisarse en el plan, no darse por trasplantado.
- `src/core/Wanted.cpp:678-699`: bloque de trazas `VEX wanted …` bajo `__EMSCRIPTEN__` — instrumentación del port, inocua para el comportamiento.

## 5. Lo que no se verificó

- Nada en ejecución: sin navegador (prohibido por reglas) y sin logs de esta sesión.
- No se revisó `FindPoliceCarMissionForWantedLevel` ni el pathfinding de aproximación (análisis B).
- No se comprobó si los datos de *wanted paths* están presentes en los assets servidos; si no lo
  estuvieran, `FindClosestWantedPathsToCoors` devuelve todo a cero y la rama de búsqueda queda
  inerte (habría que confirmarlo en el análisis B, que toca `PathFind`).
