---
name: 08-pasada-a1-a2-b6
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 08 — Pasada A1+A2+B6 ejecutada: GXT, los 25 ficheros y catálogo canónico SilentPatch

Todo ejecutado hoy (24/09/2026), solo lectura. Complementa a `05`/`06`/`07`.

---

## A1. GXT decodificados (TKEY/TDAT) servidos vs mod — ✅ RESUELTO

Decodificados los 6 idiomas (parser TKEY/TDAT UTF-16LE propio). Resultado **idéntico en los 6**:

- **0 textos distintos** para claves comunes.
- El servido tiene exactamente **6 claves de más** (los +214 B constantes):

| Clave | Texto |
|---|---|
| `HELLENB` | Hellenbach |
| `MANCHEZ` | Manchez |
| `PREMIER` | Premier |
| `STRTFTR` | Streetfighter |
| `VCPDWTR` | VCPD WinterGreen |
| `WINTERG` | Winter Green |

Son los **nombres de 6 vehículos nuevos** (el mod los referencia en `default.ide` como claves GXT
pero su propio GXT no las trae — nuestro servido sí). Resto: 2.593-2.673 claves por idioma iguales.

**DECISIÓN: conservar los GXT servidos** ✅ (superset del del mod). Coherente con servir el
`main.scm` del mod (sus claves están cubiertas). Nada que hacer.

## A2. Los 25 ficheros «servidos ≠ mod» — caracterizados con DECISIÓN

Convención diff: `+` = mod, `−` = servido.

### Conservar lo SERVIDO ✅
| Fichero | Por qué |
|---|---|
| GXT ×6 | superset (+6 claves), A1 |
| `weapon.dat` | filas de datos idénticas tras normalizar (solo comentarios) |
| `bryx.ide`/`bryx.ipl` | el mod usa **id −1** (feature de modloader: asignación dinámica); nuestro servido adaptó a **6670/6671** fijos ✅ correcto para nuestro loader. ⚠️ Validar que 6670/6671 no chocan con el rango 6660-6699 «ViceEx weapons» del `default.ide` del mod |
| `fronten2.txd` | el servido contiene **todas** las texturas del mod (solo-mod = 0) +3 MB de formato interno (¿mipmaps/compresión) → superset; validar visualmente una vez |
| `frontend_ds2/3/4/x360/xone.txd` | el servido añade `fe_arrows2/3/4` (+`m`) = flechas de menú extra → superset |
| `frontend_nsw.txd` | servido 2,3 MB vs 131 KB del mod → superset |

### Adoptar del MOD 🔀
| Fichero | Qué aporta (verificado) |
|---|---|
| `object.dat` | **descomenta `ak47` y `m16`** + añade `rcgrenade` = física de objetos de las nuevas armas |
| `particle.cfg` | +5 partículas SMALL: `ENGINE_SMOKE2_SMALL`, `ENGINE_STEAM_SMALL`, `ENGINE_SMOKE_SMALL`, `CARFLAME_SMALL`, `CARFLAME_SMOKE_SMALL3` (smoke de vehículos pequeños — bikes nuevas) |
| `occlu.ipl` | el mod **quita 2 cajas** de oclusión (`-955…`, `-901…`) que tapaban contenido |
| `default.ide` | tweaks de vehículos vanilla: stinger/cheetah `250→237`, **police `0→1210`** (grupo de ruedas — ligado a las wheels HD) + los comentarios de rangos (6500-6599 vehicles, 6600-6619 wheels, 6620-6659 vehmods, 6660-6699 weapons). OJO: el mod movió los 6500-6507 a `newVehicles.ide`; **nuestro servido los tiene inline en `default.ide`** ✅ — no duplicar |
| `wheels.DFF`/`wheels.TXD` | **ruedas HD** (1,2 MB cada uno vs 96/24 KB servidos) — detalle de vehículo (X2.7) |
| `default.dat` | +`MODELFILE ViceExtended\MODELS\VEHMODS\{SPOILERS,SKIRTS,SCOOPS,VENTS}.DFF` (kits de tuning; `SUPERCHARGERS` comentado) |
| `gta_vc.dat` | +8 líneas `CDIMAGE …\cdimages\*.img` + mapas `plusroad`, `bryx` — a mergear en el nuestro cuando integremos esos mapas |
| `main.scm` | ya conocido (sustituir + regresión) |

## B6. Catálogo CANÓNICO de fixes — `CHANGELOG-VC.md` (CookiePLMonster, MIT)

Clasificado para nuestro port web (reVC wasm). Leyenda: ✅ = aplicable y se implementa en `src/`;
⚙️ = aplicable y configurable (su INI); ❌ = no aplica en web (Win32/GPU/CD/input nativo).
«Dónde» = punto de nuestro `src/` (ancla exacta la localiza `code_search` al implementar).

### Critical (15)
| Fix | Web | Dónde / nota |
|---|---|---|
| CD check con juego en A:/B: | ❌ | Win32 |
| Ratón se bloquea al salir del menú | ❌ | web canvas |
| Ratón sale de la ventana (multi-monitor) | ❌ | pointer lock |
| Frame limiter más preciso | ✅ parcial | ya hecho (T1/T2 cap+rAF) |
| Mejor en altos FPS (freeze en fadeouts) | ✅ | `core/Timer.cpp`, fadeos `frontend/` — importante: web va a 60+ |
| Dependencia DirectPlay | ❌ | Win32 |
| Crash con DEP | ❌ | Win32 |
| «Cannot find enough video memory» | ❌ | D3D |
| Ruta User Files vía API | ❌ | ya IDBFS |
| Freeze con III/VC/SA a la vez | ❌ | Win32 |
| Crash con textos fuera del GXT (mods) | ✅ | loader GXT: comprobar rango de claves |
| Crash por stingers (pool de objetos) | ✅ | `peds/CopPed.cpp` (stinger) + `core/Pools` |
| Crash por roadblocks (pool lleno) | ✅ | `core/Pools`, script |
| Freeze al crear entidades con pool lleno | ✅ | `CPools::New*` (devolver nil y manejar) |
| Refresh rate por defecto = desktop | ❌ | Win32 |

### Other fixes — aplicables ✅ (mayoría; «dónde» orientativo)
| Fix | Web | Dónde |
|---|---|---|
| Reflejos de calzada mojada | ✅ | `renderer/` (road reflections) |
| Glows bajo pickups (solo PS2) | ✅ | `objects/Pickups.cpp` / `renderer/WeaponEffects` (X-list) |
| Códigos de crimen del dispatch policial | ✅ | `audio/AudioLogic.cpp` |
| Armas melee de cheats reemplazadas al coger pickups | ✅ | `Pools`/`CPickups` + script |
| Paneles de coche tras explosión | ✅ | `vehicles/Automobile.cpp` |
| Constantes métrico-imperiales | ✅ | `core/` (conversores) |
| Pathfinding de coches que persiguen | ✅ | `vehicles/CarCtrl.cpp` |
| Bombas en coches de garaje se guardan | ✅ | `save/GenericGameStorage.cpp` / `Garages` |
| Contadores de car generators | ✅ | `vehicles/CarGen.cpp` |
| Extras en bikes siguen la inclinación | ✅ | `vehicles/Bike.cpp` |
| Latencia de teclado −1 frame | ✅ parcial | `controls/Pad.cpp` |
| Líneas de corona en GPUs no-NVIDIA | ✅ parcial | `renderer/Coronas.cpp` (nuestro GL) |
| Sirena del FBI Washington | ✅ | audio/modelo |
| Taxis sin luces en tráfico | ✅ | `CarCtrl` (¡hermano de X2.3 luces por dummies!) |
| Extra6 en selección aleatoria | ✅ | `CVehicle::SetRandomExtras` |
| Drive-By: sonidos por arma | ✅ | `audio/` (drive-by X-list) |
| Props Malibu/Ocean View/Pole + ver entorno desde interiores | ✅ | **los 8 IPL ya analizados** (`07` §8) |
| Sombras de texto/outline escalan a resolución | ✅ | `renderer/Hud.cpp`, `renderer/Font.cpp` (X2.2) |
| Outline del blip Destino | ✅ | `renderer/Radar.cpp` |
| Créditos escalan | ✅ | `frontend/` |
| Duración de Mission Title/Passed | ✅ | `Hud.cpp` |
| Padding de text boxes | ✅ | `Messages` |
| Offset nombre de arma en Ammu-Nation | ✅ | `frontend/Shop` |
| Blip invertido del Map Legend | ✅ | `Radar.cpp` |
| `FILE_FLAG_NO_BUFFERING` en IMG | ❌ | ya on-demand |
| Resprays gratis heredados a New Game | ✅ | `Garages` |
| Timers dispatch ambulancia/bomberos | ✅ | `GameLogic.cpp` |
| Rain-streak en carretera al cargar save | ✅ | `core/Weather.cpp` (¡clave para nuestro ciclo de saves!) |
| Multiplicadores rango entrar-coche/amenaza al New Game | ✅ | `peds/Ped.cpp` + vars de script |
| Probabilidad de luces de tráfico (PS2) | ✅ | `CarCtrl` |
| Vehículos explotan dos veces | ✅ | `vehicles/Vehicle.cpp`/`Automobile.cpp` |
| Random de script 16-bit (era 15) | ✅ | `control/Script*.cpp` (random) |
| Random de spawn de coches 16-bit | ✅ | `CarGen.cpp` |
| `CPlane::LoadPath`/`CTrain` líneas sin terminar | ✅ | `core/FileLoader.cpp` (paths) |
| Env mapping en extras de vehículo | ✅ | render vehículo (X2.10) |
| Sensibilidad de ratón al New Game | ❌ | web |
| Color del texto de asset money | ✅ | `Hud.cpp` |
| Glow del pickup minigun (rosa→púrpura) | ✅ | `WeaponEffects` |
| Destino del fogonazo (Wesser) | ✅ | `weapons/Weapon.cpp` |
| Shopkeeper «apuntado» con controles Classic | ✅ | `peds/` + `Pad.cpp` |
| LOD obra construcción tras «Demolition Man» | ✅ | script/world |
| Splash del outro 2,5 s | ✅ | `frontend/` |
| Puño a coches con brass knuckles/chainsaw | ✅ | `peds/Ped.cpp` |
| Puño con armas nuevas de VC | ✅ | `Ped.cpp` |
| Sonido de impacto del destornillador | ✅ | audio |
| **Ped Speech Patch** (Sergeanur): peds más parlanchines (PS2) | ✅ | `peds/Ped.cpp` + `audio/` |
| Gas lacrimógeno daña a Tommy/personajes de misión | ✅ | `weapons/` |
| Flares escalan | ✅ | X2.2 |
| SWAT/FBI/Army en roadblocks con arma en mano | ✅ | `peds/` |
| Criminales robandо cancelan objetivo | ✅ | `peds/` |
| Sombras/luces en objetos rotados en X | ✅ | `renderer/Shadows.cpp` |
| Securicars más resistentes | ✅ | `Vehicle.cpp` |
| Blips bilinear en «Blips Only» | ✅ | `Radar.cpp` |
| Extras en barcos (lona del Rio) | ✅ | `vehicles/Boat.cpp` |
| Radar del Tropic animado | ✅ | `Radar.cpp`/`Vehicle.cpp` |
| Elevador trasero del Skimmer | ✅ | controles |
| Stats: nº real de hidden packages | ✅ | `frontend/`/`Stats` |
| Sprites de script con bilinear | ✅ | `2d` render |
| NPCs pueden usar lanzacohetes | ✅ | `peds/Weapon.cpp` |
| Heat haze escala + doble escape + no se desactiva con nombre de zona | ✅ | postfx/weather |
| Gotas de agua/sangre bajo HUD/radar | ✅ | `Weather`/postfx (X2.5) |
| Vainillas: sin expulsión en python/sniper/laser | ✅ | `Weapon.cpp` |
| Corrupción de memoria heladería (Distribution) | ✅ | `objects/` |
| ⚙️ Coronas de sirena (poli/bombero/ambu/Enforcer/Vice Cheetah/FBI) + FBI + taxi + helicóptero | ✅ | **= X2.3 luces por dummies** (`Vehicle.cpp`) |
| ⚙️ Backface culling off (piezas, peds, lista de mapas del INI) | ✅ | `renderer/Renderer.cpp` (DrawBackfaces — listas ya extraídas) |
| ⚙️ Radar: posición horizontal, disco, sombra escalan + disco encogido | ✅ | `Radar.cpp` (X2.4) |
| ⚙️ Sprites/rectángulos de script escalan | ✅ | X2.2 |

### Enhancements (6)
| Fix | Web | Nota |
|---|---|---|
| Resolución desktop por defecto | ❌ | web |
| Censura DE/FR eliminada | ✅ | textos GXT |
| ⚙️ Métrico/imperial por locale | ✅ | |
| ⚙️ Sliding mission titles (beta III) | ✅ | opcional, off |
| ⚙️ «Minimal HUD» (sin usar) | ✅ | opcional, off (el string está en su ASI) |
| ⚙️ Iconos de propiedades comprables en radar/mapa | ✅ | opcional, off (X-list) |

**Recuento**: ~100 fixes — **~12 ❌** (Win32/GPU/input nativo), **~85 ✅ aplicables** (de los cuales
⚙️ ~10 configurables) y 2-3 parciales. SilentPatch queda listo para implementarse en tandas
(críticos de pools → render/radar → audio/peds → opcionales).

## Nuevos items de validación nacidos de esta pasada
1. **Colisión de IDs**: comprobar que `6670/6671` (nuestros `bryx_lights`/`lodbryx_lights`) no
   chocan con el rango 6660-6699 del `default.ide` del mod ni con `newVehicles.ide`.
2. **Formato interno de `fronten2.txd`** servido (+3 MB): mismo contenido que el mod + sobrecarga —
   validar visualmente (¿mipmaps/compresión?) una vez se vea el frontend.
3. El `default.ide` del mod **mueve** los vehículos 6500-6507 a `newVehicles.ide`; el nuestro los
   tiene inline — al mergear datos del mod, **no duplicar** defs.
