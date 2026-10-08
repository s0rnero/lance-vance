---
name: vice-extended
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-19
---

# Plan — Vice Extended en reVC-web (`vice-extended-…_1788102643_908113`)

> Decisión usuario: natación y Transfender quedan últimos (no prioritarios).
> Vía elegida: empírica (overlay + boot + log del primer fallo), sin más análisis estático.

## 1. Lo investigado (cerrado)

- Inventario total: 198 ficheros. `GameFiles/ViceExtended/` (58 files, 46.7 MB: `data/main.scm` 1.19 MB, `newVehicles/` 8 coches IDs 6500–6507 + img 5.5 MB/16 entradas, GXTs ×7 incl. español, TXDs frontend/HUD, `ped.ifp` 2.43 MB vs 2.20 vanilla, `ViceEx.RAW/.SDT` 356 KB, tablas `neo/`, `features.ini`, `limits.ini`) + `GameFiles/modloader/ViceExtended/` (8 `cdimages`, mapas `bryx/plusroad/newGen`, `gta_vc.dat` propio) + `ViceEx.exe`/`modloader.asi` (código x86, inportable).
- `SourceCode/` = solo MVLConverter (conversor XML→IDE, inútil en runtime). `Bonus/x64_beta/` = exe 64-bit, no fuente. **No hay fuente del mod.**
- Límites: `config.h` reVC ya trae los del `limits.ini` salvo `NUMVEHICLES 110→130`. Falta: `MODELINFOSIZE 6500→~6660` (IDs 6500+ hoy = fuera de rango), `MAX_CDIMAGES 8→~16` (vanilla 3 + mod 9).
- Handling de coches nuevos va inline en el IDE (no toca tabla global) ✔. Tablas `neo/` las entiende reVC ✔.
- Gate del script: `main.scm` mod usa opcode custom **`0FA8`** (offset 45810, tras GXT `HELP61`; ~4 params chicos). reVC despacha 0–1499 → opcode desconocido = desync/crash. Lista completa pendiente (se obtiene por boot, no por Sanny).
- Nadie lo portó a reVC antes. `Photosounder/ViceCity` descartado (es física ped-vehículo, nada del mod). `CLEO Redux` no corre en wasm (inyección DLL), pero `sannybuilder/library` (dir `vc/`) documenta opcodes y `CLEO-Redux` declara soporte ViceEx (puente de comandos custom = referencia de semántica).
- Bases open por mecánica: `niltwill/vc-cleo-scripts` (~6 toggles 1:1), `plugin-sdk/examples/{GPS,UniversalTurnlights}`, `ThirteenAG/Project2DFX`, `aap/skygfx` (gran parte ya en reVC), `GTAmodding/VCSPC` (MIT, mecánicas de vehículo), fork `Cowboy-69/librw` (dif pendiente: posible NVC/YCbCr).

## 2. Alcance

**Entra**: datos completos (mapa, coches, armas-objeto, textos, HUD, partículas, anims, audio-banco), script del mod con stubs, toggles chicos (Grupo 0+1), GPS/foto/primera persona/drive-by (Grupo 2).
**Fuera**: `ViceEx.exe/.asi` y lo que solo vive ahí sin spec abierta. Natación y Transfender, últimos por decisión.

## 3. Fases

### Fase 0 — Overlay datos + script vanilla (valida pipeline)
1. `streamed/` += contenido `modloader/ViceExtended/` y `ViceExtended/` (mod gana; respetar `modloader.ini` default prio 50).
2. `.dat` fusionado: `CDIMAGE` nuevas (8 cdimages + `newVehicles.img` sueltos con su `.dir` propio; NO fusionar sectores entre imágenes) + IDE/IPL/COL del mod.
3. Recompilar con `MODELINFOSIZE~6660`, `NUMVEHICLES 130`, `MAX_CDIMAGES 16`.
4. Boot con `main.scm` **vanilla**. Aceptación: mundo extendo carga, 0 `FAIL`, coche nuevo spawneable por índice (debug o script test).

### Fase 1 — Gate del script (1 boot)
1. Boot con `main.scm` del mod. Leer primer `Comm/desconocido` del log (`gamelog` + `odtrace.log`).
2. Clasificar cada custom: stub degradable (GPS mudo, autosave vanilla, hints) vs lógica dura.
3. Implementar stubs con tamaño exacto de params (referencia: `sannybuilder/library` + puente ViceEx de `CLEO-Redux`).
4. Aceptación: intro jugable con su script; misiones vanilla-mod pasan sin abort.

### Fase 2 — Toggles y HUD (victorias rápidas)
Portar desde `niltwill` (no-burn, regen, no-bounce, tanque, tráfico) + `UniversalTurnlights` + HUD (estrellas, ceros, waypoint, icons radio, sights, triángulo salud) + `ViceEx.RAW` como banco + cheats (CRAZYTOOLS…). Aceptación: cada toggle conmutable, sin regresión de FPS/RAM en heartbeat.

### Fase 3 — Mecánicas medianas
GPS (render+script), photo mode (cuidado: readback WebGL, F12 ya sufre), primera persona, drive-by, moverse-apuntando, esconderse-polis, IA (cover, cuchilla, gasolinera). Una por build, con captura.

### Fase 4 (aparcada) — Natación, Transfender completo.

## 4-bis. Triage de datos (19/09): qué entra YA, qué pide código, qué no

Comprobado contra el motor de este repo (no contra el .exe del mod):

- `FileLoader.cpp:123` **ya parsea `CDIMAGE <ruta>`** en cualquier `.dat` → se
  pueden registrar IMG extra sin tocar C++. En web, `CdStreamPosix.cpp` admite
  además una IMG servida como **carpeta suelta + `.dir`** (así vive
  `models/gta3.img/`, 6032 ficheros).
- Límites actuales: `MAX_CDIMAGES = 8` (vanilla 3 + mod 9 → 12–16),
  `NUMVEHICLES = 110` (mod 130), `MODELINFOSIZE = 6500` (IDs 6500+ del mod
  fuera de rango → ~6660). El `gta_vc.dat` del port **no tiene** líneas
  `CDIMAGE`: hoy `gta3.img` entra por código (`Game.cpp:514`).
- `.dir` del mod leídos con el formato real (`offset, size, name[24]`): son
  consistentes y sus nombres son vanilla + nuevos → mapeo fiable.
- El port tiene **exactamente 64 tiles** `radar00..radar63.txd` sueltos y el
  mod trae **los mismos 64 nombres** en `cdimages/radar.img` → sustitución 1:1.

### A. Dato puro, sin código — «Pack 1: toque moderno»

| Qué | Origen en el mod | Cómo | Riesgo |
|---|---|---|---|
| Radar HD (Absolute Radar Remake) | `cdimages/radar.img` (64 × `radarNN.txd`, 4.3 MB) | extraer sobre `streamed/models/gta3.img/radarNN.txd` | nulo (solo texturas) |
| Blips / iconos HD | `cdimages/generic.img` = `icons4.txd` | copiar a `streamed/models/` | bajo |
| HUD, menú, partículas | `models/hud.txd`, `fronten2.txd`, `generic.txd`, `particle.txd` | pisar por nombre | medio: `hud.txd` trae iconos de armas que aquí no existen (inofensivo); probar por lotes |
| Animaciones de peds | `anim/ped.ifp` (2.43 MB vs 2.20) | pisar `streamed/anim/ped.ifp` | medio: clips nuevos (`deagle/steyr`) sin código = solo peso; los modificados cambian el *feel* |
| Textos (incl. español) | `TEXT/*.gxt` ×7 | pisar | bajo-medio (GXT con cadenas de sus armas/garajes) |
| Modelos de peds / jugador | `cdimages/peds.img` (129), `player.img` (46) | extraer a suelta o `CDIMAGE` | bajo (mismos nombres) |
| Texturas de coches | `cdimages/vehicles.img` (48; `infernus.txd`, `taxi.txd`…) | pisar solo los que casen por nombre | bajo |
| Tablas `neo/` | `neo/*.dat` + `neo.txd` | **ya están** (idénticas byte a byte) | — |
| `radio.txd` (iconos de emisora) y `newspapers.txd` | `models/`, `txd/` | **NO** por sí solos: son arte de features que necesitan código (icono de radio, periódicos por misión) → peso muerto | — |

Pipeline obligatorio en cada tanda: copiar a `streamed/` → `gen_manifest.py` →
`bootseed.list`/`stage_bootseed.py` si hace falta en arranque → rebuild (`.data`).
Antes de dar por bueno un TXD del mod, mirar `freeze-audio-txd-pal8-saves.md`
(el port ya tuvo artefactos con TXD pal8/DXT).

### B. Dato + herramienta (Pack 2: mapa)

- `cdimages/objects.img` (72): `bryx_lights.dff`, `plusroad.col`, `deagl.dff`,
  `mall_ammu`/`wash_ammu`/`wash_hardwares` (rótulos Ammu-Nation/ferreterías),
  `wsh*paynspray.dff` (persianas de garajes), `lithawaste1`, `miamiland027a`,
  `haiti.col`/`haitin.col`, LODs (`lodbryx_lights`, `lodngst2meshdam`).
- `modloader/ViceExtended/maps/` (nuevos: `bryx`, `newGen`, `plusroad`,
  `wanted_paths.ipl`) + `map.zon`/`info.zon`/`navig.zon`/`occlu.ipl`/`cull.ipl`
  + su `gta_vc.dat`.
- Requiere: **extractor IMG → carpeta suelta** (no existe en `tools/`),
  fusionar IDE/IPL en el `gta_vc.dat` del port, subir `MODELINFOSIZE` y
  `MAX_CDIMAGES`, y meter sus `.col`. Los IPL del mod retocan áreas vanilla:
  no se pueden copiar «a medias» sin mirar el par IDE↔IPL↔COL.

### C. Solo con código (los archivos no bastan)

- **Armas nuevas** (`weapons.img` 94 entradas: `ak47`, `m16`, `beretta`,
  `steyr`=AUG, `gr_launch`=lanzagranadas, `desert_eagle` + `deagle.ifp`/
  `steyr.ifp` + `weapon.dat` 8.3 KB): slots `WEAPONTYPE` nuevos, HUD, sonidos,
  script. Los DFF sueltos sin IDE solo engordan el paquete.
- **Coche(s) nuevo(s)** `newVehicles` (IDs 6500–6507, IDE de formato extendido
  propio + MVLConverter) y tuning/Transfender.
- **Mecánicas**: nadar, escalar, GPS, modo foto, 1ª persona, autosave,
  drive-by, esconderse de la poli (Fase 2/3/4 de arriba).
- **`main.scm` del mod**: opcode custom `0FA8` → sin stubs, desync/crash.

### D. No entra

- `ViceEx.exe`, `modloader.asi`, `Bonus/x64_beta/`, `*.dll` (x86 nativo) y
  `SourceCode/MVLConverter` (herramienta Windows; solo serviría como spec).
- `GameFiles/modloader/.data|.profiles` + `modloader.ini` (bookkeeping del
  modloader).
- `features.ini` / `limits.ini` como ficheros: este motor no los lee (los
  knobs equivalentes son defines de `config.h`). `gamecontrollerdb.txt` ya
  está en el port.
- `audio/ViceEx.SDT|.RAW`: banco incompatible con el pipeline web (muestras
  sueltas + `sfx.SDT` ya retocado por `fix_sdt_*`). Solo interesan sus sonidos
  nuevos de armas, y eso es trabajo aparte.

**Primer movimiento propuesto (Pack 1, reversible)**: radar HD + `icons4.txd`
+ peds/jugador + texturas de coche que casen, en un solo overlay con
`gen_manifest` + `.data`, medido con la sonda de arranque/RAM. Nada del mod se
commitea (terceros, igual que los assets de Rockstar).

## 4. Riesgos y notas
- `.dir` regenerados no-retail vistos en el proyecto: verificar que los del mod sean retail-consistentes (sizes+bytes vs imagen) antes de fiar el mapping.
- `ped.ifp` mod sustituye al vanilla vía `anims.img` del mod (no como suelto).
- Ucraniano necesita glifos (baja prioridad; español incluido ✔).
- Nada del mod se commitea (terceros, como assets Rockstar): overlay local + docs.

## 5. Fuentes abiertas (búsqueda 19/09): qué hay y por dónde se reconstruye

### 5.1 Lo que NO existe (comprobado)

- **Vice Exended no tiene fuente pública.** El GitHub del autor principal
  (`Cowboy-69`) tiene 8 repos y ninguno es el mod: `librw` (fork MIT),
  `modloader` (fork MIT), `FileSystemRedux` (plugin de CLEO Redux, AGPL-3.0),
  `LibertyToolBox`, `CameraOnim`, `ZeroToolbox`, `CarGeneratorWPL`,
  `DeadGalaxy`. Búsquedas en libertycity / gtabuilder / gtaforums / Reddit:
  solo el `.exe` y los datos.
- `x87/gta-extended` **no es el mod**: su README es el de reVC (III `master`,
  VC `miami`) — es el mismo linaje que este repo.
- `strings ViceEx.exe` solo da nombres de ficheros **vanilla de reVC**
  (`Cam.cpp`, `Ped.cpp`, `VehicleModelInfo.cpp`, `WeaponInfo.cpp`,
  `custompipes_d3d9.cpp`…): el mod se hizo **parcheando ficheros existentes**,
  sin módulos nuevos reconocibles. Tampoco publica PDB.

### 5.2 La vía sólida que sí existe

`ViceEx.exe` **es un build de reVC** (3.7 MB, 32-bit, carga `modloader.asi`).
Como nuestro código es el mismo de partida, es viable **RE diferencial con
Ghidra**: compilar nuestro reVC con las mismas flags/compilador, emparejar
funciones (strings, orden de tabla vtables, firmas) y lo que no exista en
nuestro binario **es el añadido del mod** → semántica exacta función a función.
Coste alto por función, pero es la única vía de “portar el mod tal cual”.
Empezar por lo barato: el lector de `limits.ini`/`features.ini` (los knobs
están en texto) y `WeaponInfo` (armas nuevas).

### 5.3 Referencias abiertas por mecánica

| Mecánica | Referencia | Qué aporta | Cómo se puede usar |
|---|---|---|---|
| Límites, streaming, spawn, cámara 1ª persona de vehículo | `Photosounder/ViceCity` (fork de reVC, marcas `// rouz edit`) | subir límites, no descargar objetos, spawn de coches, arreglos de la cámara de vehículo, salir del coche a cualquier velocidad | **Mismo linaje reVC → fusionable** con pinzas (fork con cambios “broma”) |
| Nadar / escalar | `gta-reversed/gta-reversed` (SA 1.0, 50-60 %) y el SA de re3 | algoritmo de natación y escalada (CPlayerPed/CCam/tareas) | SA ≠ VC: **spec/algoritmo**, hay que adaptar a la API de VC |
| Armas y vehículos add-on sin reemplazar | **Maxo's Vehicle Loader** (`maxorator`, fuente pública) | formato extendido que usa `newVehicles.ide` del mod + carga de modelos nuevos | MVL parchea el exe retail: **reimplementar en `FileLoader`/`Streaming`** |
| Stats de armas/coches nuevas | `weapon.dat` (8.3 KB) y `newVehicles.ide` del mod | daño, anims, handling, carcols… ya en texto plano | **Dato directo** (sin código no hacen nada) |
| Luces lejanas / coronas de LOD | `ThirteenAG/III.VC.SA.IV.Project2DFX` | comportamiento, parámetros `ini`/`dat` | ASI: **spec**, no código |
| GPS (ruta en minimapa) | `DK22Pac/plugin-sdk` `examples/GPS`, `janglapuk/SAMP-GPS` | algoritmo de ruta + render | ASI sobre offsets retail: **spec** |
| Toggles pequeños (cinturones, intermitentes…) | `plugin-sdk/examples/UniversalTurnlights`, `niltwill/vc-cleo-scripts` | lógica de cada toggle | **spec** |
| PostFX PS2 / YCbCr | `aap/skygfx_vc`, `aap/skygfx` | corrección YCbCr, postfx | parte **ya está** en el librw del repo |
| Semántica de opcodes del script del mod | `cleolibrary/CLEO-Redux`, `cleolibrary/opcodes-restoration-project`, `sannybuilder/library` | lista y comportamiento de opcodes custom (el `0FA8` del mod) | CLEO Redux no corre en wasm (ASI), pero **sus fuentes documentan comandos** |
| Overlay de mods | `thelink2012/modloader` (MIT) | modelo de prioridades/overlay | ya lo cubre nuestro `streamed/` + manifiesto |

**Licencia (importante)**: este repo **no tiene licencia** (“no estamos en
posición de licenciar este código; solo uso educativo/documentación/modding”).
Consecuencia: **no pegar código ajeno GPL/MIT**; usar esos repos como
*especificación* y escribir el código nosotros en estilo reVC. Los forks del
mismo linaje reVC son el material más limpio.

### 5.4 Orden recomendado (cada paso: una build + sonda de arranque/RAM)

> **Investigación completa y decisiones en
> `.agents/plans/vice-extended-inclusion.md`** (evidencia medida: `.dir`,
> conteos de reemplazos/nuevos, secciones IDE/IPL, límites, armas, audio).
> **Fuera de alcance por decisión del jugador (19/09)**: taller/garaje de
> tuning, **modo foto** y **GPS**.

1. **Datos (Pack 1)**: radar HD, `icons4.txd`, peds/jugador, texturas de
   coche. Cero C++, efecto visible inmediato.
2. **RE de los knobs**: `features.ini`/`limits.ini` → localizar su lector en
   `ViceEx.exe` y portar los toggles baratos (no arder al volcar, regenerar
   vida, no rebotar del coche, color del waypoint, luces lejanas, intermitentes,
   quitar retroceso). Es lo que más “moderniza” por línea de código.
3. **Llaves de sistemas**: `MODELINFOSIZE`/`NUMVEHICLES`/`MAX_CDIMAGES` +
   extractor IMG→carpeta suelta + líneas `CDIMAGE` → mapa del mod
   (`bryx`/`plusroad`/`newGen`) y `newVehicles` (formato MVL).
4. **Armas nuevas (7)**: slots `WEAPONTYPE` + `weapon.dat` del mod +
   `deagle.ifp`/`steyr.ifp` + iconos HUD + sonidos del banco del mod.
5. **Mecánicas grandes**, una por build: nadar, escalar, GPS, 1ª persona,
   drive-by, modo foto, autosave / esconderse de la policía.
