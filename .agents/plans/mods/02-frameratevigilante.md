---
name: 02-frameratevigilante
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-21
---

# Plan · FramerateVigilante (bugs de FPS altos)

Origen: `GTAmodding/FramerateVigilante` · **MIT** © 2023 GTA modding (Junior_Djjr)
Código: `FramerateVigilante/FramerateVigilante.cpp` (496 líneas) · requiere
`plugin-sdk` + `injector` (hookeo de binaria).

## Qué es y qué nos sirve

Es una lista de **sitios del motor que suponen 50 fps** (la base de VC: el
`CTimer` de la trilogía está calibrado a 50 Hz, con el «mágico» 30/50 de las
animaciones). Lo arregla parcheando instrucciones por dirección con envoltorios
de ensamblador (`asm_fmul`, `asm_fdiv`, …) y multiplicando por
`CTimer::ms_fTimeStep`. Sus direcciones no nos valen (compilamos desde fuente),
pero su **lista es el mapa del peligro**:

1. **Constantes por-frame** que nadie escaló: se multiplican por `ms_fTimeStep`
   sin cambiar la intención.
2. **Temporizadores que asumen 30/50 fps**: `GetTimeSinceLastFrame`,
   `NewFrameRender`, `AutoPilotTimerFix_VC` (autopiloto).
3. **Entradas que se rompen a FPS altos**: alternar **bocina/sirena** y el
   **giro de rueda en raíles** (`CarWheelOnRailsSpinFix`).
4. **Rotores**: `rotorFinalSpeed` (velocidad de rotor de helicópteros) — parche
   comentado en su fuente, pero el problema es real y está en nuestro código.
5. Extras: `_fpsLimit`/`autoLimitFPS` según la frecuencia del monitor y (en SA)
   quitar el retardo de 14 ms por frame. **No aplican** a web.

## Hallazgos ya localizados en nuestro motor (con línea)

Búsqueda hecha sobre el árbol actual; son candidatos confirmados por lectura:

| Qué | Dónde | Por qué falla a FPS altos |
|---|---|---|
| **Rotor del helicóptero** | `src/vehicles/Heli.cpp:573` → `m_fRotorRotation += 3.14f/6.5f;` | suma **por frame**; a 120 fps gira 2,4× más rápido que a 50 |
| **Rueda en raíles** (tren/vías) | `src/control/CarCtrl.cpp:1159` `CCarCtrl::UpdateCarOnRails` (+ `src/vehicles/Automobile.cpp:467`, `src/vehicles/Bike.cpp:325`) | el giro de rueda se acumula por frame |
| **Bocina/sirena (toggle)** | `src/vehicles/Automobile.cpp:1378` (`m_bSirenOrAlarm = !m_bSirenOrAlarm`) y `:1587` (`CTimer::GetFrameCounter()&7`) | el rebote/parpadeo de la entrada y el «&7» dependen del **número de frame**, no del tiempo |
| **Autopiloto** | `src/control/CarAI.cpp` (timers del autopiloto) | los timers de la trilogía se cuentan en frames |

Patrón a barrer en todo el motor (receta concreta, no a ojo):

```
grep -rn "GetFrameCounter()\s*&\|GetFrameCounter()\s*%" src     # parpadeos por frame
grep -rn "+= 3\.14f\|+= [0-9.]*f;\s*//\|/6\.5f" src             # acumuladores por frame
grep -rn "m_nTimer\s*-=\|--m_nTimer\|\bm_nTimer--" src          # cuentas atrás en frames
```

Cada acierto se **mide** antes de tocar: traza con el valor por segundo a 50 y a
120 fps. Si a 120 el valor por segundo es ~2,4× el de 50, es frame-dependiente.

## Cómo se arregla en nuestro motor (dos formas, elegir por caso)

1. **Escalar**: `+= VELOCIDAD * CTimer::GetTimeStep()` (o
   `GetTimeStepInSeconds()` si la unidad es por segundo). Es lo que hace el
   propio re3 en casi todo (`m_fCurrentStamina - CTimer::GetTimeStep()`).
2. **Pasar a tiempo**: cuando el contador es «cada N frames» (parpadeo, `&7`),
   convertirlo a milisegundos (`CTimer::GetTimeInMilliseconds()` con intervalo),
   que además hace que el efecto se vea **igual** a 35 y a 120 fps.

## Plan de prueba (esto es lo que nadie ha hecho todavía)

1. Jugar con **límite de FPS apagado** (el tope duro del puerto es 120) unos
   minutos: ruedas, rotor de helicóptero, sirena (encendido y parpadeo) y giro en
   vías.
2. Trazas nuevas, una línea por segundo: `FPSDEP rotor=`, `FPSDEP siren=`,
   `FPSDEP rails=`, con el valor por segundo. Se comparan dos partidas (35 y 120).
3. PASS: el valor por segundo **no cambia** entre las dos partidas.
4. Bloque nuevo en `tools/viceext-log-check.py` (`F`) que compare las dos cifras
   del propio log y diga PASS/FAIL sin que nadie mire a ojo.

## Lo que NO se toma

- Direcciones (`0x005AF238`, `0x006C4EFE`, …), `injector`, `plugin-sdk`,
  `EntryPoint`, `IniReader` (ya tenemos `features.ini` y `VICEEXT_*`).
- Límite de FPS por frecuencia del monitor y el retardo de 14 ms: son de
  Windows/D3D; aquí manda el tope de `skel/glfw`.

## Encargo sugerido (agente A, dueño de vehículos y `CarCtrl`/`CarAI`)

Para cada uno de los 4 hallazgos: medir (traza) → arreglar con `GetTimeStep` o
tiempo → compilar sólo sus objetos con ninja (sin enlazar) → anotar en
`.agents/HISTORIAL.md` con la columna `ATTRIBUTION.md`. Nada de tocar
`Hud.cpp`/`Font.cpp` (son del agente B) ni `gta_vc_browser/**` (sección 1).
