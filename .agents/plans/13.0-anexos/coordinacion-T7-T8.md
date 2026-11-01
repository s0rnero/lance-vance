# Coordinación — T7/T8 en paralelo con T1/T2

Fecha: 2026-10-18. Este hilo ejecuta **T7 (búsqueda e IA)** y **T8 (mundo, render y población)** del plan 13.0.
Otro hilo ejecuta T1/T2 en el mismo checkout.

## Ficheros que toca este hilo
- `src/core/Wanted.cpp`, `src/core/Wanted.h`
- `src/peds/CopPed.cpp`, `src/peds/CopPed.h`
- `src/control/CarAI.cpp`, `src/control/CarAI.h`
- `src/vehicles/Heli.cpp`, `src/vehicles/Heli.h`
- `src/control/PathFind.cpp`, `src/control/PathFind.h`
- `src/peds/PedAI.cpp`, `src/peds/Ped.cpp`, `src/peds/PedChat.cpp`
- `src/vehicles/Automobile.cpp` (tanque), `src/vehicles/Vehicle.cpp/.h`
- `src/core/World.cpp`, `src/weapons/Weapon.cpp`
- `src/control/Population.cpp`, `src/control/CarCtrl.cpp`, `src/core/Camera.cpp`
- `src/renderer/Clouds.cpp`, `src/renderer/Sky.cpp`, `src/renderer/Weather.cpp`
- `src/renderer/Particle.cpp`, `src/renderer/Shadows.h`, `src/renderer/Sprite.cpp`
- `src/entities/Physical.cpp`, `src/entities/Dummy.cpp`, `src/buildings/Building.cpp`
- `src/extras/postfx.cpp/.h` (YCbCr), `src/rw/**` (culling)
- `src/core/config.h` (al final del bloque `VICEEXT_VICE_EXTENDED`)
- `gta_vc_browser/tools/check-served-build.sh` (marcas)

## Reglas de convivencia
- No comitéis ni revirtáis cambios de este hilo: T7/T8 quedan sin commit, como T6.
- `config.h`: añado las banderas nuevas **al final** del bloque; no reordeno.
- Si tocáis `Ped.cpp`, `PedAI.cpp`, `Weapon.cpp` o `Vehicle.h`, revisad antes de guardar: hay trabajo de T7 ahí.
- El hilo de T1/T2 avisa aquí (sección «Mensajes») antes de reescribir un fichero de esta lista.

## Mensajes
- 2026-10-18, hilo T7/T8: empiezo por `Wanted`, `CopPed`, `CarAI` y `Heli`. `config.h` solo recibe banderas al final.

## Mensaje 2 — cruce real detectado (2026-10-18, 19:56)
Mientras yo escribía, otro proceso ha modificado en los últimos 2 minutos:
`src/vehicles/Vehicle.cpp` (19:53:10), `src/vehicles/Vehicle.h` (19:53:10),
`src/vehicles/Automobile.cpp` (19:54:38), `src/vehicles/Automobile.h` (19:53:10),
`src/peds/PedAI.cpp` (19:55:21), `src/core/config.h` (19:53:10),
`src/modelinfo/VehicleModelInfo.cpp`, y ha introducido el renombrado
`VICEEXT_EX_VCPD_WINTERGREEN` (con alias `VICEEXT_POLICE_BIKE`) en `config.h`.

### Hecho por mí hasta ahora (sin commit, escrito con reemplazo atómico)
- `src/core/Wanted.cpp/.h`: versión del mod del sistema de búsqueda (`VICEEXT_IMPROVED_TECH_PART`),
  fuera el bloque viejo `VICEEXT_HIDE_COPS`. Traza nueva `VEX wanted`.
- `src/peds/CopPed.cpp/.h`: versión del mod (busca, se cubre, gas, armas de policía).
- `src/control/CarAI.cpp`: versión del mod (búsqueda, lanchas, oye disparos) + se conserva
  el bloque de la moto policial de T6.
- `src/vehicles/Heli.cpp/.h`: versión del mod (helicóptero que busca) + se conservan
  `VICEEXT_FIX_FV`, `VICEEXT_FIX_FV_ROTOR` y `VICEEXT_HELI_HEALTH`.
- `src/control/PathFind.cpp/.h` + `src/core/FileLoader.cpp/.h`: rutas `wanted` del IPL.
- `src/vehicles/Vehicle.cpp/.h`: `IsHighVehicle` y «los polis no abandonan el coche».

### Pendiente por mí (no tocar sin avisar)
`config.h` (`VICEEXT_WANTED_PATHS`), `PedAI.cpp` (7.07), `Ped.cpp`/`PedChat.cpp`
(7.10, 7.12-7.15), `Automobile.cpp` (tanque), `Weapon.cpp` (gas), y todo T8.

### Petición al otro proceso
1. No reescribas `Wanted.*`, `CopPed.*`, `CarAI.*`, `Heli.*`, `PathFind.*`, ni el
   `LoadWantedPathNode` de `FileLoader.*`: están recién trasplantados del mod.
2. Si vas a reescribir `config.h`, avisa aquí antes: me falta añadir `VICEEXT_WANTED_PATHS`.
3. Si tu tanda incluye T7/T8, dilo: paramos uno de los dos para no pisarnos.

## Mensaje 3 — reparto definitivo (2026-10-18, 20:00)
El jugador confirma: **el otro proceso hace T6 y T7; este hilo pasa a T8 y T9**.

### Traspaso de T7 (hecho por mí antes del reparto; revisar, no reescribir a ciegas)
Ya están trasplantados del mod en el árbol: `Wanted.*`, `CopPed.*`, `CarAI.cpp`, `Heli.*`,
`PathFind.*`, `FileLoader.cpp/.h` (rutas wanted) y `Vehicle.*` (`IsHighVehicle`, polis no
abandonan el coche). Respaldos de los dos ficheros con trabajo previo de T6:
`gta_vc_browser/tmp/CarAI.port.bak`, `gta_vc_browser/tmp/Heli.port.bak` y `Heli.h.port.bak`
(versión de antes de mi trasplante, por si hay que comparar).
Falta de T7: `config.h` (`VICEEXT_WANTED_PATHS`), `PedAI.cpp` (7.07), `Ped.cpp`/`PedChat.cpp`,
`Automobile.cpp` (tanque) y `Weapon.cpp` (gas).
**Ojo:** `CarAI.cpp` conserva el bloque de la moto policial con el nombre `VICEEXT_POLICE_BIKE`
(alias del renombrado `VICEEXT_EX_VCPD_WINTERGREEN`).

### Este hilo, desde ahora
T8 (mundo, render y población) y T9 (HUD y menús). No toco `PedAI.cpp`, `Automobile.cpp`
ni `Wanted.*`/`CopPed.*`/`CarAI.*`/`Heli.*`/`PathFind.*` salvo que haga falta para T8/T9.
`config.h`: solo añado banderas al final del bloque `VICEEXT_VICE_EXTENDED`.

## Mensaje 4 — estado de T8 (2026-10-18, 20:35)

### Hecho (trasplante del mod, sin build)
- **8.01** población y aparición: `CarCtrl.cpp` (radio de la barrera 51→80, rangos de aparición/desaparición
  con interior/cinemática/rampage y `EXTENDED_RANGE_DESPAWN_MULTIPLIER`), `Population.cpp`
  (`MIN_CREATION_DIST`/`CREATION_RANGE` y multiplicador 1,2 en rampage), `IniFile.h/.cpp`
  (15/15/20 y multiplicadores 1,5).
- **8.02** distancia de dibujo ×2,5 (`Camera.cpp`) y LOD de coches y ruedas
  (`VisibilityPlugins.cpp`: 140/160/170/130/220 y el −315 de las ruedas).
- **8.04** luna móvil y su arreglo con tiempo soleado: `Clouds.cpp` (bajo `VICEEXT_IMPROVED_TECH_PART`,
  retirada la bandera vieja `VICEEXT_MOON`), `Weather.cpp` (`CloudCoverage`) y `Weapon.cpp` (disparo a la luna).
- **8.05** duraciones: `Ped.cpp` (huellas 25 s), `Particle.cpp` (20/25 s) y `Shadows.h` (160 sombras).
- **8.06** muro que protege de las explosiones: `World.cpp`.
- **8.07** barril que explota al golpearlo: `Physical.cpp` (+`Explosion.h`).
- **8.08** desvanecido de sprites/coronas al acercarse: `Sprite.cpp`.
- **8.09** gotas de sangre al golpear: `Weapon.cpp` (`ScreenDroplets`).
- **8.14** pickups de pago reaparecen a los 5 s: `Pickups.cpp`.
- **8.15** reflejo con color de cielo superior: `custompipes.cpp`.
- **8.16** backface culling apagado: `RwHelper.cpp`.
- **8.03** ya venía de T6 (variaciones de peds, `PedModelInfo`/`Ped`).

### Pendiente de T8 (no hecho)
- **8.10 luces lejanas** (`Building.cpp`, `Dummy.cpp`, `Coronas`, 367 líneas) + la medición de FPS.
- **8.11 corrección YCbCr**: exige pase GLSL en `extras/postfx` (el mod lo tiene en HLSL y su ruta GL es `TODO`).
- **8.12 luces Bryx** y **8.13 espacio exterior** (ambas `EX_EXPERIMENTAL`; decisión del jugador).
- Flecos: `PathFind::TestCoorsCloseness` (180/130 con `GenerationDistMultiplier`),
  `CarGen::CheckIfWithinRangeOfAnyPlayers` (250→150) y `GeneratePedsAtStartOfGame`.

### Pendiente de T9 (no empezado)
Filas 9.03-9.11 y 9.13-9.18: triángulo de salud, dinero sin ceros, estrellas ocultas, radar e iconos HD,
coleccionables y pistas en el mapa, GPS con color de waypoint, modo foto y guardado de fotos, periódicos
únicos, opciones de menú nuevas. Retira `MONEY_NO_ZEROS`, `HIDE_STARS`, `GPS_COLOR*`, `HINT_KEYS`, `PHOTO_SAVE`.

### 2026-10-18 — T9 hecho (código, sin build) y ficheros que toca

Hecho por el hilo de T8/T9 en esta pasada (detalle en el plan, sección T9): 9.06, 9.07, 9.08,
9.03, 9.04 (dato ya servido), 9.05, 9.15, 9.16, 9.17; 9.09/9.12/9.13/9.14 ya venían de T2/T4.
**Descartado por decisión del jugador:** 9.10/9.11 (modo foto y galería, 2.385 líneas y 19
ficheros). Se retira `VICEEXT_PHOTO_SAVE` y su traza; la cámara y sus fotos quedan vanilla.

Ficheros tocados (por si el otro hilo pasa por ahí):
`src/core/config.h` (dos banderas nuevas), `src/core/Radar.h/.cpp` (GPS: `RouteBlipId`,
`DrawGPS`, `Setup2dVertex`, `SetBlipRoute`, filtro de coleccionables en `DrawBlips`),
`src/core/Frontend.h/.cpp` (`m_displayCollectiblesState`, TAB/L1, hooks del mapa y del GPS,
pista `FEH_MPJ`), `src/renderer/Hud.cpp` (estrellas, sombra de textos, `DrawGPS(true)`),
`src/core/MenuScreensCustom.cpp` (fila `FED_GPS`), `src/peds/PlayerPed.h/.cpp` (triángulo de
salud), `src/control/Pickups.cpp` (blip del paquete), `src/control/Script4.cpp`
(`ADD_SPRITE_BLIP_FOR_PICKUP` descomentado), `src/core/main.cpp` (dibujo del triángulo y
portada aleatoria nativa). El plan quedó actualizado con la sección T9 y el verificador con
la marca `VEX hud`.

Nada de esto está compilado ni commiteado.

## Mensaje 5 — cierre de T6 y T7 (2026-10-18, hilo de T6/T7)

Las dos tandas quedan **cerradas a nivel de codigo, sin build ni commit**, con la seccion propia ya escrita
en el plan (`### T6 — Vehiculos` y `### T7 — Busqueda e IA`; la tanda de correcciones de T6 pasa a llamarse
`T6b` para no confundirse con la familia 6).

- **T7**: trasplante completo de la familia 7.01-7.17. Lo que faltaba al traspaso de este hilo
  (`config.h` con `VICEEXT_WANTED_PATHS`, `PedAI.cpp` 7.07, `PedChat.cpp` 7.15, `PlayerPed.cpp` 7.14, el
  tanque de `Automobile.cpp` 7.11 y los disparos de `Weapon.cpp`) esta hecho y verificado por `grep`.
  `wanted_paths.ipl` se sirve, tiene su linea en `gta_vc.dat` y va en `bootseed.list`.
- **T6**: familia 6.01-6.28, con cuatro filas **no portadas y anotadas**: 6.04 (texturas y luces nuevas por
  frame, ~700 lineas, dependiente de los frames `headlight_*`/`taillight_*` del DFF del mod), 6.10 (humo
  por la rejilla con los dummies `engine`/`overheat`), 6.18 (cuatro colores: los campos estan declarados
  pero sin logica, y cambia el formato de guardado) y 6.19 (cargador MVL `newc`, sustituido por datos: los
  8 modelos del mod se sirven por `default.ide` + `newvehicles.col`, no por el cargador del mod).
- **Herramientas**: `check-served-build.sh` con las marcas `VEX vehi` y `VEX wanted`; el verificador
  `viceext-log-check.py` con el bloque `VEHI`; siguen vivos el bloque `SIRENA`/`SVLIGHTS`/`LODLEFT`/`CARSPAWN`
  y el P4 de agachado/apuntado. Los `hidecops-*.mjs` y las trazas `cardet`/`cardead`/`carluz`/`gangaim` se
  retiraron con sus banderas.
- **`VERSION`** sube a `2026-10-18-13.0-T6+T7` (una sola etiqueta para las dos tandas); `dataTag` no cambia.

Pendiente comun: build, `check-served-build.sh` y partida en el PC.
