---
name: vice-extended-pendientes
status: EXECUTED
type: maintenance
domain: mods
owner_rules: .agents
created: 2026-09-20
---

# Vice Extended · qué falta para darlo por terminado

Fuente de verdad: el propio mod, no el recuerdo.
- `vice-extended-october-2025-update_1788102643_908113/ChangesEN.txt` (cambios de
  v1.0 a 2510) y `Credits.txt`.
- `Grand Theft Auto Vice City/ViceExtended/features.ini` (18 knobs, los que el
  mod trae puestos son los que "hacen" el mod) y `limits.ini`.
- El binario (`ViceEx.exe`) no tiene fuente pública: donde no hay dato, se dice.

Leyenda: **HECHO** (con build) · **PENDIENTE** · **DATOS** (sólo assets, ya
metidos) · **FUERA** (excluido por decisión del jugador o imposible en wasm).

## 1. `features.ini`: los que el mod trae ENCENDIDO (=1)

| Toggle | Estado | Nota |
|---|---|---|
| `PlayerDoesntBounceAwayFromMovingCar` | HECHO | `VICEEXT_NO_CAR_BOUNCE` |
| `RecoilWhenFiring` | HECHO | `VICEEXT_RECOIL` (v2, 12ª partida) |
| `EnableSwimming` | HECHO | `VICEEXT_SWIMMING` + cámara (R14-1) |
| `RocketLauncherThirdPersonAiming` | **HECHO `ve29`** | R16: el lanzacohetes apunta en 3ª persona y se puede andar apuntando |
| `EnableDistantLights` | PENDIENTE | X6. Falta **medir** qué son en el mod (PC nativo, con el mismo sitio a la vista) antes de tocar el render |
| `RandomVehicleModsInTraffic` | FUERA (de momento) | depende del taller de tuning (excluido) y de MVL; sustituto razonable por decidir con el jugador |

Los demás (13) están a 0 en su `features.ini`, así que **no** forman parte del
mod tal como se instala. Dos se portaron igualmente porque el jugador los pidió:
`RemoveMoneyZerosInTheHud` (`VICEEXT_MONEY_NO_ZEROS`) y `EnableClimbing`
(`VICEEXT_CLIMB`).

## 2. Lo que el mod trae siempre puesto y AÚN NO está

Ordenado por coste. Ninguno lleva toggle en el mod: si no está, no es el mod.

| # | Qué (ChangesEN) | Dónde | Coste |
|---|---|---|---|
| 1 | **Iconos de emisora al cambiar** (2.6) — las 9 texturas `R*` ya están en `hud.txd` | `Hud.cpp` + `MusicManager` | bajo |
| 2 | **Sonido del agua al nadar** (2510) | `AudioLogic`/`PlayerPed` | bajo |
| 3 | **Teclas de pista con `Shift`** (2510) — toca D7 (`KEYICONS`) | `Messages`/`Hud` | bajo |
| 4 | **Trucos nuevos**: `BIGHEADS`, `RCROCKET`, `AEZAKMI`, `IAMINVINCIBLE`, `AIRWAYS`, `BIGSMOKE` (v1.0-v2.5) | `Pad.cpp` (compartido: añadir al final) | bajo |
| 5 | **Crash del Micro-UZI** (2510) | `Weapon.cpp` | bajo |
| 6 | **Triángulo de salud** sobre la cabeza (3.0) — no hay textura en `hud.txd`: se dibuja por código | `Hud.cpp` | bajo-medio |
| 7 | **Animaciones de muerte dentro del coche** + conductores muertos con comportamiento aleatorio (3.0) | `Automobile`/`Ped` | medio |
| 8 | **Variaciones de peds** (2.6) | `.ide`/`Population` | medio |
| 9 | **ALT para caminar** y **sentarse/moverse sentado** (2.0/1.0) | `Pad.cpp` + `PlayerPed` | bajo |
| 10 | **Sensibilidad de apuntado** y **precisión de la IA** (2506/v1.0) | `ControllerConfig`/`PedAI` | bajo |
| 11 | **Detalle de vehículo** (2.0/2.5/2506): rueda que se cae, puertas que golpean, giro aleatorio al aparecer aparcado, rotación de ruedas al salir, intermitentes | `Automobile`/`Bike` | medio |
| 12 | **Luces del coche** (2.5): textura, haz separado, humo y fuego del capó con el capó cerrado | `Automobile`/renderer | medio |
| 13 | **Luna móvil** (1.5, posición según el sol) | `Clouds.cpp` | bajo-medio |
| 14 | **Blips e hints del mapa** (2.5/3.0/2506): farmacias, Bomb Shop, estadio, casas en venta, coleccionables | datos + `Radar`/`FrontEnd` | medio |
| 15 | **Paneles de Ammu-Nation/ferreterías y edificios de Little Haiti** (2510) | DATOS (`.ipl/.ide` del mod) | medio (re-empaquetar) |
| 16 | **Colores de vértice de noche** (2506, soporte d3d9) | renderer | medio |
| 17 | **Radio de aparición mayor** (1.0) + `limits.ini` (`NUMPEDS=140`, `NUMVEHICLES=130`) | `CarCtrl`/límites | bajo-medio |
| 18 | **Pulido vanilla+** (v1.0): SWAT con gas, dos helicópteros a 5-6★, daño al Maverick por arma, cañón de agua, conductores que reaccionan, muro que protege, partículas más duraderas | varios | alto |
| 19 | **Sueltos por datos** (2.5/2.6) que dependen de su `main.scm`: garajes, aparcados nuevos, blips, modelo de Lance | datos | N/A (script) |

## 3. Lo que ya está (para no volver a medirlo)

- **Datos**: radar HD, `icons4.txd`, peds/jugador/coches/armas del mod, `ped.ifp`
  (272 clips), `.gxt` (incl. español), coches nuevos 6500-6507, armas nuevas
  (Beretta/AK/M16/AUG/Deagle/Shotgun2/Micro-UZI/Gr_launch + `deagle.ifp`,
  `steyr.ifp`, `rocket.ifp`), mapas `bryx`/`plusroad`/`newGen`, `weapon.dat`,
  sonidos de las armas nuevas (`sfx.SDT` + 13 mp3), texturas de emisora.
- **Mecánicas**: nadar (R7), agachado + andar agachado + pose al desagacharse
  (R15), escalada (E1), apuntado con ratón y pose por arma (R6/R9/R13), apuntado
  con escopetas (C7), caminar apuntando (H4), retroceso (R14-3), 1ª persona (R5),
  drive-by (P2), sprint con armas pesadas, autoguardado y guardar en cualquier
  sitio (C2), recarga a mano (C3.1), depósito de gasolina (C3.3), luces que se
  rompen (C3.5), esconderse de la policía (P1), luces de servicio (R11/ve24),
  miras por arma (D6), iconos de tecla en los avisos (D7), autocentrado de cámara
  de coche (R14-2), `CRAZYTOOLS` y sus cheats de armas/coches, lanzacohetes en 3ª
  persona (R16).
- **Fuera** (decisión del jugador o imposible en wasm): taller de tuning, modo
  foto, GPS, ropa/tienda, periódicos únicos, ucraniano, CLEO Redux, `main.scm`
  del mod (`ViceEx.exe`/`.asi`), su banco de audio entero.

## 4. Orden propuesto para cerrarlo

1. **Tanda corta (una build)**: filas 1-6 (iconos de emisora, sonido del agua,
   teclas con Shift, trucos nuevos, crash del Micro-UZI, triángulo de salud).
2. **Tanda media (una build por bloque)**: filas 7-13 (peds, ALT/sentarse,
   sensibilidad/IA, detalles de vehículo, luces, luna móvil).
3. **Tanda de datos (una build + re-descarga)**: filas 14-15 (blips/hints,
   paneles y Little Haiti) — sube `dataTag`.
4. **Tanda alta**: fila 17 (límites/radio de aparición) y fila 18 (pulido
   vanilla+), que es donde el mod se separa más del motor base.
5. **Con el jugador delante**: `EnableDistantLights` (medir en el mod nativo) y
   `RandomVehicleModsInTraffic` (decidir sustituto sin taller de tuning).
