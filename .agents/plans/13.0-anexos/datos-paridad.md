# Vice Extended 2510: paridad de DATOS y SCRIPT con lo servido (auditoría 14, 2026-10-14)

> Auditoría de **solo lectura**, por evidencia (tamaño + sha256 de cada fichero y de
> cada entrada de IMG), no por nombre. No se ha tocado el proyecto ni la instalación,
> ni se ha ejecutado ningún script del proyecto: solo `sha256sum`/`cmp`/`find` y dos
> lectores propios en Python (solo biblioteca estándar, `python -I`) que viven fuera del
> repo (ver §8).

## Fuentes

| Papel | Ruta |
|---|---|
| Instalación viva (verdad del mod 2510) | `C:\Users\s0rno\Desktop\Gamez\(g)old\Grand Theft Auto Vice City` (`ViceExtended\`, `modloader\`, raíz) |
| Paquete del mod | `mods\extended\GameFiles\` (200 ficheros) |
| Base vanilla de referencia | la **raíz** de la instalación (`data\`, `models\`, `anim\`, `TEXT\`, `txd\`): el paquete no trae ningún dato de raíz (solo `ViceEx.exe`, `modloader.asi`, `modloader\`, `ViceExtended\`), así que la raíz es el repack vanilla |
| Servido on-demand | `gta_vc_browser\streamed\` (manifiesto `web\public\manifest.json`, 2026-10-14 18:01) |
| Servido precargado | `gta_vc_browser\web\public\build\reVC.data` (161.251.529 B, 1.935 ficheros, enlazado 2026-10-14 18:21) = `gta_vc_browser\bootseed\` |
| Fuente del mod (3.0, jul-2024) | `gta_vc_browser\tmp\upstream-gta-extended` |

**Instalación = paquete, medido**: los 200 ficheros del paquete tienen el mismo sha256 en la
instalación (incluido `ViceEx.exe` `4df2726e0365`). Solo existen en la instalación:
`ViceExtended\audio\ViceEx.RAW` (fuera de git por `*.RAW`), `ViceExtended\userfiles\*` y
`ViceEx.ini` (los escribe el juego).

## Cómo resuelve el port las rutas (leído en `ondemand.js`, `stage_bootseed.py`, `gen_manifest.py`, `build_streamed.py`)

- El motor pide una ruta; `OD.ensure` la normaliza (`\`→`/`, minúsculas) y mira primero
  **MEMFS** (lo precargado de `reVC.data`, que gana) y después el manifiesto de
  `streamed/` (clave en minúsculas → ruta real).
- `bootseed/` = `bootseed.list` (1.935 líneas: 1.721 entradas de `models/gta3.img/`, `gta3.dir`,
  `.col`, IDE/IPL…) + los directorios enteros `TEXT`, `txd`, `data`, `neo`, `skins` +
  `Audio/sound.cache`, copiados **desde `streamed/`**. Coherencia medida: **0 diferencias**
  entre `streamed/`, `bootseed/` y `reVC.data` en los 1.935 ficheros precargados.
- El overlay del mod se **aplana**: `ViceExtended/X` → `X` (única excepción:
  `ViceExtended/features.ini` se queda con su ruta); `modloader/ViceExtended/{gta_vc.dat, *.zon,
  object.dat, occlu.ipl}` → `data/`; `modloader/ViceExtended/maps/**` → `data/maps/**`;
  `newVehicles/newVehicles.col` → `models/coll/newvehicles.col`.
- Los IMG del mod (`cdimages/*.img` y `newVehicles.img`) **no** se sirven como IMG: sus entradas
  van como ficheros sueltos dentro de la carpeta `models/gta3.img/` (6.085 ficheros) con su fila en
  `models/gta3.dir` (6.096 entradas = 6.043 vanilla + **53 nuevas del mod**, exactamente las 3+16+18+16
  de la tabla de IMG). `gta3.dir` es idéntico en `streamed/`, `bootseed/` y `reVC.data`
  (`fa294ccbba16`).
- El banco de sonido propio (`ViceEx.SDT/RAW`) se **transforma**: 13 entradas añadidas a
  `Audio/sfx.SDT` + 13 mp3 sueltos en `Audio/sfx/`.

## 0. Resumen

Recuento sobre los **205 ficheros** que el mod pone en la instalación (`ViceExtended\` 63,
`modloader\` 139 = 103 de `modloader\ViceExtended\` + 36 del propio modloader, y en la raíz
`ViceEx.exe`, `ViceEx.ini`, `modloader.asi`):

| Categoría | IDENTICO | DISTINTO | AUSENTE | NO-APLICA |
|---|---|---|---|---|
| Modelos IMG (`cdimages/*.img/.dir`, 8 pares) | 16 (484/484 entradas bit a bit) | 0 | 0 | 0 |
| newVehicles (MVL) | 3 (`.col`, `.img`, `.dir`) | 1 (`.ide`, transformado e integrado) | 0 | 0 |
| Modelos sueltos (`generic.txd`, `particle.txd`, `wheels.*`, `newspapers.txd`) | 2 | 2 (`wheels.DFF/TXD` = vanilla) | 1 (`newspapers.txd`) | 0 |
| Tuning (`vehmods/*.dff`) | 0 | 0 | 0 | 4 |
| Animaciones (`ped.ifp`) | 0 | 1 (superconjunto) | 0 | 0 |
| Texto (GXT) | 0 | 6 (superconjuntos) | 0 | 1 (ucraniano) |
| Audio (`ViceEx.SDT/RAW`) | 0 | 2 (transformados, cubiertos) | 0 | 0 |
| Datos de mundo/juego (IDE/IPL/DAT/COL/ZON/CFG) | 87 | 4 (equivalentes) | 1 (`ViceEx.dat`) | 0 |
| HUD/menú/fuentes/botones | 8 | 7 | 4 | 1 (`fonts_u`) |
| Script (`main.scm`, `freeroam_miami.scm`) | 2 | 0 | 0 | 0 |
| Configuración (`features.ini`, `gamecontrollerdb.txt`, `limits.ini`, `ViceEx.ini`) | 2 | 0 | 1 (`limits.ini`) | 1 (`ViceEx.ini`) |
| Neo (4) | 4 | 0 | 0 | 0 |
| userfiles (partida, foto) | 0 | 0 | 0 | 2 |
| Motor/ASI (`ViceEx.exe`, `modloader.asi`) | 0 | 0 | 0 | 2 |
| Basura/doc (`desktop.ini` ×3, `Changes.txt`) | 0 | 0 | 0 | 4 |
| modloader como sistema (`.data/`, ini, log) | 0 | 0 | 0 | 36 |
| **Total (205)** | **124** | **23** | **7** | **51** |

Lectura rápida:

- **Datos del mod: completos en lo que importa para jugar.** Las 484 entradas de los 9 IMG del
  mod están en `models/gta3.img/` **bit a bit** y con los mismos sectores en `gta3.dir`
  (radar HD, peds, armas nuevas, vehículos, mundo); los 87 ficheros de mundo/juego del mod que el
  port sirve son idénticos (incluidos los 12 IPL/IDE donde el mod cambia el vanilla, `object.dat`,
  `occlu.ipl`, `bryx`, `plusroad`, `newGen`, `wanted_paths`); `hud.txd`, `radio.txd`,
  `particle.txd`, `generic.txd`, `weaponSights.txd`, `features.ini`, `particle.cfg`,
  `newWeapons.col`, `main.scm` y `freeroam_miami.scm` son idénticos.
- **Los 23 DISTINTO son superconjuntos o transformaciones**, salvo tres pequeñas regresiones
  cosméticas: `wheels.DFF/TXD` (el servido es el vanilla; las ruedas HD y las llantas 6600/6601
  del mod no llegan) y los `frontend_*` de mando (versión 3.0, no 2510).
- **AUSENTE que importa**: solo `txd/newspapers.txd` (feature 2506 «periódicos únicos tras
  ciertas misiones», cosmética). Los otros 6 ausentes no hacen falta (`limits.ini`, `ViceEx.dat`)
  o son iconos de mando PS4/PS5/XOne.
- **Script**: el `main.scm` servido **es el del mod 2510 bit a bit** en las tres copias. El
  muro para jugar no es de datos (ver §5 y §6).

## 1-2. Inventario y estado, fichero a fichero

Columnas: **Servido efectivo** = la copia que recibe el motor (`precarga` = `reVC.data`, que
gana a `streamed/`; `streamed` = solo on-demand), con su tamaño y sha256 (12 primeros
caracteres). **Vanilla (raíz)** = el mismo fichero en la raíz de la instalación (`=servido` si
lo servido es el vanilla). Estado: IDENTICO (mismo sha256), DISTINTO (existe pero difiere),
AUSENTE (sin equivalente servido), NO-APLICA (excluido o no es dato).
#### modelos (IMG) (16)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `modloader/ViceExtended/cdimages/anims.dir` | 448 | `0f138ca4754b` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/anims.img` | 1.011.712 | `d7803c6eb380` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/generic.dir` | 32 | `5c116525e677` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/generic.img` | 4.096 | `d727cc1fdd7b` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/objects.dir` | 2.304 | `09c4c08c86b1` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/objects.img` | 15.480.832 | `027ca4d02737` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/peds.dir` | 4.128 | `212e77589f99` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/peds.img` | 11.137.024 | `133a8bae44b9` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/player.dir` | 1.472 | `897b2856a88c` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/player.img` | 4.880.384 | `bc78e90d78b3` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/radar.dir` | 2.048 | `a088612c034e` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/radar.img` | 4.325.376 | `56ce671cd0e7` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/vehicles.dir` | 1.536 | `2e4bcf164aaa` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/vehicles.img` | 14.940.160 | `f0d711cffe2d` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/weapons.dir` | 3.008 | `c7881649d355` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `modloader/ViceExtended/cdimages/weapons.img` | 3.887.104 | `0ea1a67d3bf7` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |

#### newVehicles (MVL) (4)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/newVehicles/newVehicles.col` | 5.404 | `48f417725a60` | `models/coll/newvehicles.col` 5.404 B `48f417725a60` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/newVehicles/newVehicles.dir` | 512 | `4ff785d17d01` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |
| `ViceExtended/newVehicles/newVehicles.ide` | 4.543 | `e0f0856050d1` | - | - | **DISTINTO** | transformado: formato MVL convertido por `import_mvl_vehicles.py`; los 8 vehiculos (6500-6507) estan en `data/default.ide`, `handling.cfg` (8 lineas + 4 `!` de moto) y `carcols.dat`, con dff/txd en `gta3.img/`. Unica desviacion: hellenbach usa rueda 253 (`wheel_classic`) en vez de 6600 (`wheel_classic2`, que vive en el wheels.DFF del mod, no servido) |
| `ViceExtended/newVehicles/newVehicles.img` | 5.689.344 | `323f292ffb72` | - | - | **IDENTICO** | por entradas: todas fundidas bit a bit en `models/gta3.img/` (ver tabla IMG) |

#### modelos (sueltos) (5)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/models/generic.txd` | 1.549.364 | `da3e3a100225` | `models/generic.txd` 1.549.364 B `da3e3a100225` (precarga) | 262.568 B `8f28f214bdc9` | **IDENTICO** |  |
| `ViceExtended/models/generic/wheels.DFF` | 1.221.640 | `6688b7bc25e2` | `models/generic/wheels.DFF` 96.067 B `f8869c8e8f56` (precarga) | 96.067 B `f8869c8e8f56` =servido | **DISTINTO** | servido = VANILLA (96.067 B). El del mod es el juego de ruedas HD (55 marcos: anade wheel_classic2/saloon2/saloon3/lighttruck2/offroad2). No es solo taller: lo usan las llantas 6600/6601 del default.ide del mod (cosmetico) |
| `ViceExtended/models/generic/wheels.TXD` | 1.200.584 | `fe5e7f8289f5` | `models/generic/wheels.TXD` 23.976 B `964b3f91c47f` (precarga) | 23.976 B `964b3f91c47f` =servido | **DISTINTO** | servido = VANILLA (23.976 B); el del mod trae 16 texturas HD (cosmetico, va con wheels.DFF) |
| `ViceExtended/models/particle.txd` | 2.668.072 | `dc2a40771019` | `models/particle.txd` 2.668.072 B `dc2a40771019` (precarga) | 454.696 B `a66a90c1dbcd` | **IDENTICO** |  |
| `ViceExtended/txd/newspapers.txd` | 924.080 | `de7c216c642c` | - | - | **AUSENTE** | IMPORTA (cosmetico): 22 texturas `newspaper_*` de la feature 2506 "periodicos unicos tras ciertas misiones"; en ViceEx.exe la ruta va junto a `CRubbish::Render` (periodicos que vuelan por la calle) |

#### tuning (vehmods) (4)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/models/vehmods/scoops.dff` | 13.372 | `07ac4e4b29c8` | - | - | **NO-APLICA** | tuning excluido |
| `ViceExtended/models/vehmods/skirts.dff` | 5.744 | `936ce8965246` | - | - | **NO-APLICA** | tuning excluido |
| `ViceExtended/models/vehmods/spoilers.dff` | 124.276 | `bfef553f6e49` | - | - | **NO-APLICA** | tuning excluido |
| `ViceExtended/models/vehmods/vents.dff` | 57.832 | `793d9026e135` | - | - | **NO-APLICA** | tuning excluido |

#### animaciones (1)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/anim/ped.ifp` | 2.486.356 | `59929c978b4a` | `anim/ped.ifp` 2.499.960 B `14dc5608964d` (precarga) | 2.201.152 B `a200cf5085f1` | **DISTINTO** | 272 clips del mod presentes: 270 identicos byte a byte, 2 retocados (`Crouch_Roll_L/R`), +2 del port (`GunCrouchFwd/Bwd`) |

#### texto (7)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/TEXT/american.gxt` | 429.370 | `a8b2e8cc8410` | `TEXT/american.gxt` 429.622 B `4256de92bc00` (precarga) | 421.910 B `5a779f819502` | **DISTINTO** | superconjunto: 4.857/4.857 claves del mod con el mismo texto + 7 del port (`FEM_1PV` y 6 nombres cortos de los vehiculos MVL) |
| `ViceExtended/TEXT/french.gxt` | 475.056 | `035986b834df` | `TEXT/french.gxt` 475.318 B `d9c90687d1a2` (precarga) | 467.752 B `794c1b20ab77` | **DISTINTO** | superconjunto: todas las claves del mod iguales + las mismas 7 del port |
| `ViceExtended/TEXT/german.gxt` | 467.230 | `6a75546ff0f6` | `TEXT/german.gxt` 467.482 B `375f83658153` (precarga) | 459.466 B `97e5b2b21363` | **DISTINTO** | superconjunto: todas las claves del mod iguales + las mismas 7 del port |
| `ViceExtended/TEXT/italian.gxt` | 464.066 | `011fed3f88d0` | `TEXT/italian.gxt` 464.320 B `dc13416c488c` (precarga) | 456.762 B `7300bdfe42b5` | **DISTINTO** | superconjunto: todas las claves del mod iguales + las mismas 7 del port |
| `ViceExtended/TEXT/russian.gxt` | 453.366 | `b2d7ae825a26` | `TEXT/russian.gxt` 453.626 B `9e44201efb21` (precarga) | - | **DISTINTO** | superconjunto: todas las claves del mod iguales + las mismas 7 del port |
| `ViceExtended/TEXT/spanish.gxt` | 460.330 | `6d9f94502c75` | `TEXT/spanish.gxt` 460.588 B `4b7c0b0ec1f8` (precarga) | 468.412 B `24304c1fdd71` | **DISTINTO** | superconjunto: 4.846/4.846 claves del mod iguales + 7 del port (`FEM_1PV`=PRIMERA PERSONA, nombres MVL) |
| `ViceExtended/TEXT/ukrainian.gxt` | 425.564 | `312c57fb920d` | - | - | **NO-APLICA** | idioma excluido (ucraniano) |

#### audio (2)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/audio/ViceEx.RAW` | 364.850 | `ec4addbbb169` | - | - | **DISTINTO** | transformado: las 13 muestras viven como `Audio/sfx/9941.mp3`..`9953.mp3` (13/13 presentes). El RAW no esta en git ni en el paquete (`*.RAW` en .gitignore): la unica copia es la de la instalacion |
| `ViceExtended/audio/ViceEx.SDT` | 260 | `abd463273b55` | - | - | **DISTINTO** | transformado: sus 13 entradas van al final de `Audio/sfx.SDT` servido (ids 9941-9953, 9.954 entradas); 8 con tamano/Hz/bucle identicos, 5 remuestreadas (36000/33000->32000 Hz, 22000->22050 Hz) con la misma duracion. No esta en bootseed ni en reVC.data (on-demand) |

#### datos de mundo/juego (92)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `modloader/ViceExtended/gta_vc.dat` | 3.226 | `5a84b60e9fb3` | `data/gta_vc.dat` 2.933 B `3a7b260e4386` (precarga) | 2.626 B `580acea78b2c` | **DISTINTO** | equivalente: mismas lineas IDE/IPL (incluidas plusroad/bryx/newgen/wanted_paths); el servido quita el bloque `CDIMAGE MODELS\cdimages\*.img` (fundidos en gta3.img) y anade `COLFILE 0 MODELS\COLL\NEWVEHICLES.COL` |
| `modloader/ViceExtended/info.zon` | 10.650 | `0dbd54a08a59` | `data/info.zon` 10.650 B `0dbd54a08a59` (precarga) | 10.650 B `0dbd54a08a59` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/map.zon` | 2.189 | `721ad89772cc` | `data/map.zon` 2.189 B `721ad89772cc` (precarga) | 2.189 B `721ad89772cc` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/airport/airport.ide` | 16.534 | `349b065a7d35` | `data/maps/airport/airport.ide` 16.534 B `349b065a7d35` (precarga) | 16.534 B `349b065a7d35` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/airport/airport.ipl` | 38.137 | `f2ea82fb0e44` | `data/maps/airport/airport.ipl` 38.137 B `f2ea82fb0e44` (precarga) | 38.137 B `f2ea82fb0e44` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/airportN/airportN.ide` | 4.767 | `02f52d18a47e` | `data/maps/airportN/airportN.ide` 4.767 B `02f52d18a47e` (precarga) | 4.767 B `02f52d18a47e` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/airportN/airportN.ipl` | 21.202 | `6518be29a7f9` | `data/maps/airportN/airportN.ipl` 21.202 B `6518be29a7f9` (precarga) | 21.202 B `6518be29a7f9` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bank/bank.ide` | 7.792 | `a39f4b008f6c` | `data/maps/bank/bank.ide` 7.792 B `a39f4b008f6c` (precarga) | 7.792 B `a39f4b008f6c` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bank/bank.ipl` | 5.041 | `b47b269c60ca` | `data/maps/bank/bank.ipl` 5.041 B `b47b269c60ca` (precarga) | 5.041 B `b47b269c60ca` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bar/bar.ide` | 44 | `b245d6e14ddc` | `data/maps/bar/bar.ide` 44 B `b245d6e14ddc` (precarga) | 44 B `b245d6e14ddc` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bridge/bridge.ide` | 2.072 | `adbb66408375` | `data/maps/bridge/bridge.ide` 2.072 B `adbb66408375` (precarga) | 2.072 B `adbb66408375` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bridge/bridge.ipl` | 4.891 | `b32c3d05914e` | `data/maps/bridge/bridge.ipl` 4.891 B `b32c3d05914e` (precarga) | 4.891 B `b32c3d05914e` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bryx/bryx.ide` | 111 | `b82b774a8958` | `data/maps/bryx/bryx.ide` 111 B `b82b774a8958` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/bryx/bryx.ipl` | 172 | `b9b2c1c424f6` | `data/maps/bryx/bryx.ipl` 172 B `b9b2c1c424f6` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/cisland/cisland.ide` | 9.189 | `b1cdcebf58ca` | `data/maps/cisland/cisland.ide` 9.189 B `b1cdcebf58ca` (precarga) | 9.189 B `b1cdcebf58ca` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/cisland/cisland.ipl` | 25.904 | `f43194becca9` | `data/maps/cisland/cisland.ipl` 25.904 B `f43194becca9` (precarga) | 25.904 B `f43194becca9` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/club/club.ide` | 9.440 | `18ed4b95e3b6` | `data/maps/club/club.ide` 9.440 B `18ed4b95e3b6` (precarga) | 9.440 B `18ed4b95e3b6` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/club/CLUB.ipl` | 6.848 | `d6ce3a31d78a` | `data/maps/club/CLUB.ipl` 6.848 B `d6ce3a31d78a` (precarga) | 7.024 B `28122d3e93f1` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/concerth/concerth.ide` | 8.267 | `253f3777bc12` | `data/maps/concerth/concerth.ide` 8.267 B `253f3777bc12` (precarga) | 8.267 B `253f3777bc12` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/concerth/concerth.ipl` | 1.841 | `96fa0a110fc8` | `data/maps/concerth/concerth.ipl` 1.841 B `96fa0a110fc8` (precarga) | 1.841 B `96fa0a110fc8` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/cull.ipl` | 57.117 | `66ad5fc5108a` | `data/maps/cull.ipl` 57.117 B `66ad5fc5108a` (precarga) | 57.117 B `66ad5fc5108a` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/docks/docks.ide` | 9.361 | `cac3379ee49f` | `data/maps/docks/docks.ide` 9.361 B `cac3379ee49f` (precarga) | 9.361 B `cac3379ee49f` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/docks/docks.ipl` | 30.648 | `4a84dd4e9c89` | `data/maps/docks/docks.ipl` 30.648 B `4a84dd4e9c89` (precarga) | 30.554 B `f8ce3b51b760` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/downtown/downtown.ide` | 23.049 | `05f54c2a5866` | `data/maps/downtown/downtown.ide` 23.049 B `05f54c2a5866` (precarga) | 23.049 B `05f54c2a5866` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/downtown/downtown.ipl` | 55.338 | `f6a3f0eed5cf` | `data/maps/downtown/downtown.ipl` 55.338 B `f6a3f0eed5cf` (precarga) | 55.338 B `f6a3f0eed5cf` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/downtows/downtows.ide` | 6.986 | `ccbda23d4911` | `data/maps/downtows/downtows.ide` 6.986 B `ccbda23d4911` (precarga) | 6.986 B `ccbda23d4911` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/downtows/downtows.ipl` | 26.691 | `8fc8b8f30b72` | `data/maps/downtows/downtows.ipl` 26.691 B `8fc8b8f30b72` (precarga) | 26.691 B `8fc8b8f30b72` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/generic.ide` | 22.127 | `34171d2eb927` | `data/maps/generic.ide` 22.127 B `34171d2eb927` (precarga) | 22.127 B `34171d2eb927` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/golf/golf.ide` | 5.715 | `5dd15fd6f2fa` | `data/maps/golf/golf.ide` 5.715 B `5dd15fd6f2fa` (precarga) | 5.715 B `5dd15fd6f2fa` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/golf/golf.ipl` | 17.564 | `744a0323c479` | `data/maps/golf/golf.ipl` 17.564 B `744a0323c479` (precarga) | 17.564 B `744a0323c479` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/haiti/haiti.ide` | 5.964 | `d79a117e8e52` | `data/maps/haiti/haiti.ide` 5.964 B `d79a117e8e52` (precarga) | 5.964 B `d79a117e8e52` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/haiti/haiti.ipl` | 39.303 | `e9385ad92320` | `data/maps/haiti/haiti.ipl` 39.303 B `e9385ad92320` (precarga) | 39.211 B `42f045fc058d` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/haitiN/haitiN.ide` | 5.945 | `731438e7580b` | `data/maps/haitiN/haitiN.ide` 5.945 B `731438e7580b` (precarga) | 5.941 B `d080cf155ac7` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/haitiN/haitin.ipl` | 26.689 | `bbb76a30051d` | `data/maps/haitiN/haitin.ipl` 26.689 B `bbb76a30051d` (precarga) | 26.682 B `cb41d0e2f489` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/hotel/hotel.ide` | 4.761 | `b3a7b569481c` | `data/maps/hotel/hotel.ide` 4.761 B `b3a7b569481c` (precarga) | 4.761 B `b3a7b569481c` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/hotel/hotel.ipl` | 9.035 | `6a08cdaf956e` | `data/maps/hotel/hotel.ipl` 9.035 B `6a08cdaf956e` (precarga) | 9.035 B `77040ae4d1e8` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/islandsf/islandsf.ide` | 4.004 | `b792bafdf881` | `data/maps/islandsf/islandsf.ide` 4.004 B `b792bafdf881` (precarga) | 4.004 B `b792bafdf881` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/islandsf/islandsf.ipl` | 10.380 | `0158872bbef2` | `data/maps/islandsf/islandsf.ipl` 10.380 B `0158872bbef2` (precarga) | 10.380 B `0158872bbef2` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/lawyers/lawyers.ide` | 1.098 | `1eaf1f3b9107` | `data/maps/lawyers/lawyers.ide` 1.098 B `1eaf1f3b9107` (precarga) | 1.098 B `1eaf1f3b9107` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/lawyers/lawyers.ipl` | 1.438 | `b196f2fea07a` | `data/maps/lawyers/lawyers.ipl` 1.438 B `b196f2fea07a` (precarga) | 1.438 B `b196f2fea07a` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/littleha/littleha.ide` | 15.573 | `bf84206d2986` | `data/maps/littleha/littleha.ide` 15.573 B `bf84206d2986` (precarga) | 15.573 B `bf84206d2986` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/littleha/littleha.ipl` | 68.907 | `b54c19e7784f` | `data/maps/littleha/littleha.ipl` 68.907 B `b54c19e7784f` (precarga) | 68.907 B `b54c19e7784f` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/mall/mall.ide` | 10.024 | `637ef6691d28` | `data/maps/mall/mall.ide` 10.024 B `637ef6691d28` (precarga) | 10.024 B `637ef6691d28` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/mall/mall.ipl` | 24.048 | `b0901bb83d4c` | `data/maps/mall/mall.ipl` 24.048 B `b0901bb83d4c` (precarga) | 24.048 B `b0901bb83d4c` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/mansion/mansion.ide` | 9.684 | `8c47789ea91a` | `data/maps/mansion/mansion.ide` 9.684 B `8c47789ea91a` (precarga) | 9.684 B `8c47789ea91a` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/mansion/mansion.ipl` | 15.315 | `31db6f94a84d` | `data/maps/mansion/mansion.ipl` 15.315 B `31db6f94a84d` (precarga) | 15.239 B `fa9c36cbdf88` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map0.dat` | 691 | `c23577aed503` | `data/maps/map0.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map1.dat` | 691 | `c23577aed503` | `data/maps/map1.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map2.dat` | 691 | `c23577aed503` | `data/maps/map2.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map3.dat` | 691 | `c23577aed503` | `data/maps/map3.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map4.dat` | 691 | `c23577aed503` | `data/maps/map4.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map5.dat` | 691 | `c23577aed503` | `data/maps/map5.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map6.dat` | 691 | `c23577aed503` | `data/maps/map6.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/map7.dat` | 691 | `c23577aed503` | `data/maps/map7.dat` 691 B `c23577aed503` (precarga) | 691 B `c23577aed503` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/nbeach/nbeach.ide` | 5.659 | `2d5f7d0b5f12` | `data/maps/nbeach/nbeach.ide` 5.659 B `2d5f7d0b5f12` (precarga) | 5.659 B `2d5f7d0b5f12` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/nbeach/nbeach.ipl` | 43.289 | `a565e0246d35` | `data/maps/nbeach/nbeach.ipl` 43.289 B `a565e0246d35` (precarga) | 43.289 B `a565e0246d35` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/nbeachbt/nbeachbt.ide` | 10.364 | `c34553fae3f0` | `data/maps/nbeachbt/nbeachbt.ide` 10.364 B `c34553fae3f0` (precarga) | 10.371 B `eadfa58adf51` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/nbeachbt/nbeachbt.ipl` | 75.000 | `46e68d45e3bd` | `data/maps/nbeachbt/nbeachbt.ipl` 75.000 B `46e68d45e3bd` (precarga) | 74.906 B `4d8893d5d875` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/nbeachw/nbeachw.ide` | 9.508 | `045e0ef4576b` | `data/maps/nbeachw/nbeachw.ide` 9.508 B `045e0ef4576b` (precarga) | 9.458 B `348054d962b6` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/nbeachw/nbeachw.ipl` | 41.365 | `a7cc5eb3e8cb` | `data/maps/nbeachw/nbeachw.ipl` 41.365 B `a7cc5eb3e8cb` (precarga) | 41.365 B `a7cc5eb3e8cb` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/newGen/newgen.col` | 14.148 | `3f2c050711ff` | `data/maps/newGen/newgen.col` 14.148 B `3f2c050711ff` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/newGen/newgen.IDE` | 95 | `f08d316ebac4` | `data/maps/newGen/newgen.IDE` 95 B `f08d316ebac4` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/newGen/newgen.IPL` | 106 | `749e99b939aa` | `data/maps/newGen/newgen.IPL` 106 B `749e99b939aa` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/oceandn/oceandN.ide` | 20.652 | `2e87615c632b` | `data/maps/oceandn/oceandN.ide` 20.652 B `2e87615c632b` (precarga) | 20.652 B `2e87615c632b` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/oceandn/oceandN.ipl` | 77.730 | `cf07c3a757b3` | `data/maps/oceandn/oceandN.ipl` 77.730 B `cf07c3a757b3` (precarga) | 77.730 B `cf07c3a757b3` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/oceandrv/oceandrv.ide` | 13.429 | `b1d194d91a9f` | `data/maps/oceandrv/oceandrv.ide` 13.429 B `b1d194d91a9f` (precarga) | 13.429 B `b1d194d91a9f` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/oceandrv/oceandrv.ipl` | 50.666 | `cc3406d23b2d` | `data/maps/oceandrv/oceandrv.ipl` 50.666 B `cc3406d23b2d` (precarga) | 50.666 B `cc3406d23b2d` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/paths.ipl` | 1.171.556 | `4ed810384d17` | `data/maps/paths.ipl` 1.171.556 B `4ed810384d17` (precarga) | 1.171.556 B `4ed810384d17` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/plusroad/plusroad.IDE` | 305 | `759c75f95915` | `data/maps/plusroad/plusroad.IDE` 305 B `759c75f95915` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/plusroad/plusroad.ipl` | 1.771 | `ce21740e3b34` | `data/maps/plusroad/plusroad.ipl` 1.771 B `ce21740e3b34` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/stadint/stadint.ide` | 10.478 | `0a9dd6073393` | `data/maps/stadint/stadint.ide` 10.478 B `0a9dd6073393` (precarga) | 10.478 B `0a9dd6073393` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/stadint/stadint.ipl` | 7.357 | `a58fde5b8d0f` | `data/maps/stadint/stadint.ipl` 7.357 B `a58fde5b8d0f` (precarga) | 7.357 B `a58fde5b8d0f` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/starisl/starisl.ide` | 4.104 | `6b4e617b25e0` | `data/maps/starisl/starisl.ide` 4.104 B `6b4e617b25e0` (precarga) | 4.104 B `6b4e617b25e0` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/starisl/starisl.ipl` | 39.410 | `f1cf04b63817` | `data/maps/starisl/starisl.ipl` 39.410 B `f1cf04b63817` (precarga) | 39.410 B `f1cf04b63817` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/stripclb/stripclb.ide` | 5.207 | `978745cc51a8` | `data/maps/stripclb/stripclb.ide` 5.207 B `978745cc51a8` (precarga) | 5.207 B `978745cc51a8` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/stripclb/stripclb.ipl` | 1.188 | `b5b77974272b` | `data/maps/stripclb/stripclb.ipl` 1.188 B `b5b77974272b` (precarga) | 1.188 B `b5b77974272b` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/wanted_paths.ipl` | 22.428 | `f7306421eab7` | `data/maps/wanted_paths.ipl` 22.428 B `f7306421eab7` (precarga) | - | **IDENTICO** |  |
| `modloader/ViceExtended/maps/washintn/washintn.ide` | 18.503 | `32d9facb4b3e` | `data/maps/washintn/washintn.ide` 18.503 B `32d9facb4b3e` (precarga) | 17.812 B `7da9826735e8` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/washintn/washintn.ipl` | 83.794 | `a4c048c1a7e0` | `data/maps/washintn/washintn.ipl` 83.794 B `a4c048c1a7e0` (precarga) | 83.794 B `a4c048c1a7e0` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/washints/washints.ide` | 21.669 | `e27a27f9f6d4` | `data/maps/washints/washints.ide` 21.669 B `e27a27f9f6d4` (precarga) | 21.669 B `e27a27f9f6d4` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/washints/washints.ipl` | 68.206 | `23e452a73a1f` | `data/maps/washints/washints.ipl` 68.206 B `23e452a73a1f` (precarga) | 68.124 B `2f56f4abc228` | **IDENTICO** |  |
| `modloader/ViceExtended/maps/yacht/yacht.ide` | 2.413 | `206d0f75d2ac` | `data/maps/yacht/yacht.ide` 2.413 B `206d0f75d2ac` (precarga) | 2.413 B `206d0f75d2ac` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/maps/yacht/yacht.ipl` | 85 | `4351545987af` | `data/maps/yacht/yacht.ipl` 85 B `4351545987af` (precarga) | 85 B `4351545987af` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/navig.zon` | 773 | `68102a71fdcc` | `data/navig.zon` 773 B `68102a71fdcc` (precarga) | 773 B `68102a71fdcc` =servido | **IDENTICO** |  |
| `modloader/ViceExtended/object.dat` | 26.779 | `22b718c7796c` | `data/object.dat` 26.779 B `22b718c7796c` (precarga) | 26.708 B `da56e01fa4b0` | **IDENTICO** |  |
| `modloader/ViceExtended/occlu.ipl` | 21.945 | `e46192296f2e` | `data/occlu.ipl` 21.945 B `e46192296f2e` (precarga) | 22.073 B `f2cff324ee36` | **IDENTICO** |  |
| `ViceExtended/data/default.dat` | 990 | `4e42aa17a307` | `data/default.dat` 606 B `c826dc557b48` (precarga) | 530 B `74e43f88c5ad` | **DISTINTO** | rutas `ViceExtended\...` aplanadas a la raiz; quita `IDE newVehicles.IDE` (integrado en default.ide), los 4 `MODELFILE VEHMODS\*.DFF` (tuning) y usa `WHEELS.DFF/TXD` vanilla; `NEWWEAPONS.COL` y `NEWVEHICLES.COL` presentes |
| `ViceExtended/data/default.ide` | 22.874 | `98d613dafae8` | `data/default.ide` 21.976 B `5247c16d1094` (precarga) | 21.221 B `e02b25143a28` | **DISTINTO** | servido = el del mod + 8 vehiculos MVL, menos: 4 comentarios de cabecera, llantas 6600/6601 (`wheel_classic2`, `wheel_saloon2`), 24 piezas `vehmods` 6620-6657 (tuning) y rueda 237 (`wheel_rim`) -> 250 (`wheel_sport`) en stinger/cheetah/vicechee. Las 10 armas nuevas 6660-6669 estan |
| `ViceExtended/data/particle.cfg` | 18.363 | `49130d2521a1` | `data/particle.cfg` 18.363 B `49130d2521a1` (precarga) | 17.559 B `421e1df9cf7c` | **IDENTICO** |  |
| `ViceExtended/data/ViceEx.dat` | 48 | `08baf20f63dc` | - | - | **AUSENTE** | manifiesto de carga de su motor (`CDIMAGE ViceExtended\newVehicles\newVehicles.img`); efecto replicado: las 16 entradas de newVehicles.img estan fundidas en `models/gta3.img/` -> no hace falta |
| `ViceExtended/data/weapon.dat` | 8.363 | `0f59451f8bf4` | `data/weapon.dat` 8.261 B `fcd389413616` (precarga) | 6.908 B `a96fe002cbaa` | **DISTINTO** | contenido identico: solo cambian los fin de linea (CRLF->LF) |
| `ViceExtended/models/coll/newWeapons.col` | 224 | `a5b85b2b6cd5` | `models/coll/newWeapons.col` 224 B `a5b85b2b6cd5` (precarga) | - | **IDENTICO** |  |

#### HUD/menu/fuentes (20)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/models/fonts_r.txd` | 524.584 | `f3e26d230735` | `models/fonts_r.txd` 524.584 B `f3e26d230735` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/models/fonts_u.txd` | 524.584 | `75723aa272ce` | - | - | **NO-APLICA** | fuente del ucraniano (idioma excluido) |
| `ViceExtended/models/fronten2.txd` | 9.504.424 | `2ed8cfedd19b` | `models/fronten2.txd` 12.389.416 B `a9ed1d1fc857` (precarga) | 748.584 B `1ee528b092dc` | **DISTINTO** | NO TOCAR (radio): sus 13 texturas (mapa del menu + flechas) son identicas; el servido anade 11 logos de emisora |
| `ViceExtended/models/frontend_ds2.txd` | 133.448 | `49f3c7dc8412` | `models/frontend_ds2.txd` 329.976 B `a31fa6cfc154` (precarga) | - | **DISTINTO** | el servido es el de `gamefiles/` (fuente 3.0), no el de 2510 (cosmetico, mando) |
| `ViceExtended/models/frontend_ds3.txd` | 327.976 | `0f370aab9128` | `models/frontend_ds3.txd` 524.968 B `44a48411df67` (precarga) | - | **DISTINTO** | idem (gamefiles 3.0; cosmetico, mando) |
| `ViceExtended/models/frontend_ds4.txd` | 327.976 | `85ec09943d27` | `models/frontend_ds4.txd` 524.968 B `7fd8e784af07` (precarga) | - | **DISTINTO** | idem (gamefiles 3.0; cosmetico, mando) |
| `ViceExtended/models/frontend_ds5.txd` | 327.976 | `c28ac979a57a` | - | - | **AUSENTE** | frontend de mando DualSense (cosmetico, solo con mando) |
| `ViceExtended/models/frontend_nsw.txd` | 131.368 | `95447fab3c59` | `models/frontend_nsw.txd` 2.360.104 B `4b75d4962bdc` (precarga) | - | **DISTINTO** | idem (gamefiles 3.0; cosmetico, mando) |
| `ViceExtended/models/frontend_x360.txd` | 327.976 | `58a851848016` | `models/frontend_x360.txd` 524.968 B `8c8e3f9179c6` (precarga) | - | **DISTINTO** | idem (gamefiles 3.0; cosmetico, mando) |
| `ViceExtended/models/frontend_xone.txd` | 327.976 | `0520282025c6` | `models/frontend_xone.txd` 524.968 B `54d5e8edc94d` (precarga) | - | **DISTINTO** | idem (gamefiles 3.0; cosmetico, mando) |
| `ViceExtended/models/hud.txd` | 6.332.968 | `31b25c325d68` | `models/hud.txd` 6.332.968 B `31b25c325d68` (precarga) | 107.176 B `51a73732cb6e` | **IDENTICO** |  |
| `ViceExtended/models/nswbtns.txd` | 528.424 | `9cb2dbca5111` | `models/nswbtns.txd` 528.424 B `9cb2dbca5111` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/models/pcbtns.txd` | 2.597.740 | `d87ebaf6a77f` | `models/pcbtns.txd` 2.597.740 B `d87ebaf6a77f` (streamed) | - | **IDENTICO** |  |
| `ViceExtended/models/ps3btns.txd` | 528.424 | `348f41637445` | `models/ps3btns.txd` 528.424 B `348f41637445` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/models/ps4btns.txd` | 528.424 | `fbe233dbe34c` | - | - | **AUSENTE** | iconos de mando PS4 (cosmetico, solo con mando) |
| `ViceExtended/models/ps5btns.txd` | 528.424 | `c4a388d6f21d` | - | - | **AUSENTE** | iconos de mando PS5 (cosmetico, solo con mando) |
| `ViceExtended/models/radio.txd` | 3.147.304 | `66ae4df35ee1` | `models/radio.txd` 3.147.304 B `66ae4df35ee1` (streamed) | - | **IDENTICO** |  |
| `ViceExtended/models/weaponSights.txd` | 164.776 | `cc5c55188368` | `models/weaponsights.txd` 164.776 B `cc5c55188368` (streamed) | - | **IDENTICO** |  |
| `ViceExtended/models/x1btns.txd` | 528.424 | `1b5f1d10d468` | - | - | **AUSENTE** | iconos de mando Xbox One (cosmetico, solo con mando) |
| `ViceExtended/models/x360btns.txd` | 528.424 | `8049a0baedf2` | `models/x360btns.txd` 528.424 B `8049a0baedf2` (precarga) | - | **IDENTICO** |  |

#### script (2)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/data/freeroam_miami.scm` | 38.197 | `3bf53e996439` | `data/freeroam_miami.scm` 38.197 B `3bf53e996439` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/data/main.scm` | 1.191.180 | `41a932cc593e` | `data/main.scm` 1.191.180 B `41a932cc593e` (precarga) | 1.269.133 B `0fb3fd45b6a6` | **IDENTICO** |  |

#### configuracion (4)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceEx.ini` | 3.438 | `ce7a40d2fd0e` | - | - | **NO-APLICA** | ajustes del jugador escritos por ViceEx.exe (el port usa su propio reVC.ini/config) |
| `ViceExtended/features.ini` | 567 | `bf884c129adb` | `ViceExtended/features.ini` 567 B `bf884c129adb` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/gamecontrollerdb.txt` | 253.094 | `84accbc51810` | `gamecontrollerdb.txt` 253.094 B `84accbc51810` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/limits.ini` | 250 | `20929a1db13b` | - | - | **AUSENTE** | limites en codigo del port (P4.10 `limits 140/130` PASS en log real, plan 12.0 ve85e) -> no hace falta |

#### neo (4)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/neo/carTweakingTable.dat` | 7.491 | `f21ed5c96aff` | `neo/carTweakingTable.dat` 7.491 B `f21ed5c96aff` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/neo/neo.txd` | 28.968 | `4cc2bb778c19` | `neo/neo.txd` 28.968 B `4cc2bb778c19` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/neo/rimTweakingTable.dat` | 9.004 | `0ff2890704e2` | `neo/rimTweakingTable.dat` 9.004 B `0ff2890704e2` (precarga) | - | **IDENTICO** |  |
| `ViceExtended/neo/worldTweakingTable.dat` | 1.243 | `046b48131ba5` | `neo/worldTweakingTable.dat` 1.243 B `046b48131ba5` (precarga) | - | **IDENTICO** |  |

#### userfiles (2)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `ViceExtended/userfiles/Gallery/photo_00000.png` | 374 | `f3ad37190e93` | - | - | **NO-APLICA** | foto de galeria (salida del juego, no dato) |
| `ViceExtended/userfiles/GTAVCsf1.b` | 201.828 | `af058d2200fc` | - | - | **NO-APLICA** | partida del jugador escrita por ViceEx.exe (no es dato del mod); NO carga en el port: ver seccion 3 |

#### motor/asi (2)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `modloader.asi` | 652.288 | `b4fae2aa1d21` | - | - | **NO-APLICA** | binario Windows (el motor del port es reVC WASM) |
| `ViceEx.exe` | 3.822.080 | `4df2726e0365` | - | - | **NO-APLICA** | binario Windows (el motor del port es reVC WASM) |

#### basura/doc (4)

| Fichero del mod (instalacion) | Tamano | sha256 | Servido efectivo | Vanilla (raiz) | Estado | Nota |
|---|---|---|---|---|---|---|
| `modloader/ViceExtended/Changes.txt` | 560 | `340673155028` | - | - | **NO-APLICA** | changelog de modloader (documentacion) |
| `ViceExtended/audio/desktop.ini` | 46 | `27991cd3e289` | - | - | **NO-APLICA** | metadato de Windows |
| `ViceExtended/models/desktop.ini` | 48 | `da10b419af93` | - | - | **NO-APLICA** | metadato de Windows |
| `ViceExtended/models/generic/desktop.ini` | 48 | `da10b419af93` | - | - | **NO-APLICA** | metadato de Windows |

#### modloader como sistema (36 ficheros)

`modloader/.data/Command Line Arguments.md`, `modloader/.data/config.ini`, `modloader/.data/config.ini.0`, `modloader/.data/Leia-me.md`, `modloader/.data/LICENSE`, `modloader/.data/licenses/boost.txt`, `modloader/.data/licenses/cereal.txt`, `modloader/.data/licenses/tinympl.txt`, `modloader/.data/licenses/utf8-cpp.txt`, `modloader/.data/modloader.ini.0`, `modloader/.data/plugins.ini.0`, `modloader/.data/plugins/gta3/std.asi.dll`, `modloader/.data/plugins/gta3/std.asi.md`, `modloader/.data/plugins/gta3/std.bank.dll`, `modloader/.data/plugins/gta3/std.bank.md`, `modloader/.data/plugins/gta3/std.data.dll`, `modloader/.data/plugins/gta3/std.data.md`, `modloader/.data/plugins/gta3/std.fx.dll`, `modloader/.data/plugins/gta3/std.fx.md`, `modloader/.data/plugins/gta3/std.movies.dll`, `modloader/.data/plugins/gta3/std.movies.md`, `modloader/.data/plugins/gta3/std.scm.dll`, `modloader/.data/plugins/gta3/std.scm.md`, `modloader/.data/plugins/gta3/std.sprites.dll`, `modloader/.data/plugins/gta3/std.sprites.md`, `modloader/.data/plugins/gta3/std.stream.dll`, `modloader/.data/plugins/gta3/std.stream.md`, `modloader/.data/plugins/gta3/std.text.dll`, `modloader/.data/plugins/gta3/std.text.md`, `modloader/.data/plugins/gta3/std.tracks.dll`, `modloader/.data/plugins/gta3/std.tracks.md`, `modloader/.data/plugins/plugins.ini`, `modloader/.data/Profiles.md`, `modloader/.data/Readme.md`, `modloader/modloader.ini`, `modloader/modloader.log`

## 3. IMG del mod, entrada a entrada

| IMG del mod | Entradas | Nuevas (no estan en el gta3.img vanilla) | Reemplazan vanilla | IDENTICO en `models/gta3.img/` | Sectores en `gta3.dir` servido = mod | Precargadas en reVC.data (= streamed) |
|---|---|---|---|---|---|---|
| `modloader/ViceExtended/cdimages/anims.img` | 14 | 3 | 11 | 14/14 | 14/14 | 7 |
| `modloader/ViceExtended/cdimages/generic.img` | 1 | 0 | 1 | 1/1 | 1/1 | 1 |
| `modloader/ViceExtended/cdimages/objects.img` | 72 | 16 | 56 | 72/72 | 72/72 | 12 |
| `modloader/ViceExtended/cdimages/peds.img` | 129 | 0 | 129 | 129/129 | 129/129 | 46 |
| `modloader/ViceExtended/cdimages/player.img` | 46 | 0 | 46 | 46/46 | 46/46 | 4 |
| `modloader/ViceExtended/cdimages/radar.img` | 64 | 0 | 64 | 64/64 | 64/64 | 20 |
| `modloader/ViceExtended/cdimages/vehicles.img` | 48 | 0 | 48 | 48/48 | 48/48 | 26 |
| `modloader/ViceExtended/cdimages/weapons.img` | 94 | 18 | 76 | 94/94 | 94/94 | 14 |
| `ViceExtended/newVehicles/newVehicles.img` | 16 | 16 | 0 | 16/16 | 16/16 | 0 |

Nuevas por IMG:
- `anims.img`: `rocket.ifp`, `deagle.ifp`, `steyr.ifp`
- `objects.img`: `wsh_roadswshz2.dff`, `wsh_roadswshz6.dff`, `wsh_roadswshz4.dff`, `wsh_roadswshz5.dff`, `wsh_roadswshz3.dff`, `wsh_roadswshz1.dff`, `wsh_roadswshz1.txd`, `bryx_lights.dff`, `lodbryx_lights.dff`, `plusroad.col`, `bryx.col`, `lodngst2meshdam.dff`, `027agen.dff`, `027agen.txd`, `lhaitcut.txd`, `miamiland027a.txd`
- `weapons.img`: `desert_eagle.dff`, `desert_eagle.txd`, `m16.dff`, `m16.txd`, `rcgrenade.dff`, `rcgrenade.txd`, `shotgun2.dff`, `shotgun2.txd`, `uziold.dff`, `uziold.txd`, `beretta.dff`, `beretta.txd`, `steyr.dff`, `steyr.txd`, `gr_launch.dff`, `gr_launch.txd`, `grenade2.dff`, `grenade2.txd`
- `newVehicles.img`: `hellenbach.dff`, `hellenbach.txd`, `manchez.dff`, `manchez.txd`, `peren2.dff`, `peren2.txd`, `premier.dff`, `premier.txd`, `streetfi.dff`, `streetfi.txd`, `trash2.dff`, `trash2.txd`, `polwintergreen.dff`, `polwintergreen.txd`, `wintergreen.dff`, `wintergreen.txd`

**IMG: veredicto.** Las **484 entradas** de los 9 IMG del mod están en
`streamed/models/gta3.img/` con el **mismo sha256 exacto** (sin necesidad de recortar el relleno
de sector) y con los **mismos sectores** en el `gta3.dir` servido; 130 de ellas van además
precargadas en `reVC.data` y son idénticas a las de `streamed/`. No hay nombres repetidos entre
IMG del mod (el orden de carga de modloader no influye). La parte nueva (53 entradas) cuadra con
el delta del `gta3.dir` servido frente al vanilla (6.096 − 6.043).

## 4. Detalle de los DISTINTO (evidencia, más allá del hash)

| Fichero | Qué difiere, medido |
|---|---|
| 6 GXT (`american`, `spanish`, `french`, `german`, `italian`, `russian`) | Lector de tablas GXT de VC (TKEY/TDAT): **0 claves del mod que falten y 0 textos cambiados**; el servido añade 7 claves en `MAIN`: `FEM_1PV` («FIRST PERSON» / «PRIMERA PERSONA»…) y `HELLENB`, `MANCHEZ`, `PREMIER`, `STRTFTR`, `VCPDWTR`, `WINTERG` (nombres cortos de los vehículos MVL). El delta de ~250 B es exactamente eso |
| `anim/ped.ifp` | Lector ANPK: mod 272 clips, servido 274, vanilla 234. Los 272 del mod están; comparando el bloque `DGAN` de cada uno: **270 idénticos**, 2 distintos (`Crouch_Roll_L`, `Crouch_Roll_R`); el servido añade `GunCrouchFwd` y `GunCrouchBwd`. Los 38 clips que el mod añade al vanilla (`CLIMB_*`, `Crouch_*`, `Swim_*`, `GunMove_*`, `*_DB`, `sprint_*`, `CAR_die_*`) están todos |
| `data/default.ide` | `diff` mod↔servido: el servido no trae los 4 comentarios de cabecera, las llantas `6600 wheel_classic2` y `6601 wheel_saloon2`, las 24 piezas de tuning 6620-6657, y pone rueda **250** (`wheel_sport`, vanilla) donde el mod pone **237** (`wheel_rim`) en `stinger`, `cheetah`, `vicechee`; añade los 8 vehículos MVL 6500-6507. Las 10 armas 6660-6669 (`rcgrenade`, `ak47`, `m16`, `shotgun2`, `uziold`, `desert_eagle`, `beretta`, `steyr`, `gr_launch`, `grenade2`) y los cambios de clase de anim de `sniper/laser/rocketla/camera` son los del mod |
| `data/default.dat` | Rutas `ViceExtended\…` aplanadas; sin `IDE newVehicles.IDE` (integrado en `default.ide`), sin los 4 `MODELFILE VEHMODS\*.DFF` (tuning) y con `WHEELS.DFF/TXD` vanilla. `COLFILE NEWWEAPONS.COL` y `NEWVEHICLES.COL` presentes |
| `data/gta_vc.dat` | Comparado sin mayúsculas/espacios: idéntico salvo el bloque `CDIMAGE MODELS\cdimages\*.img` (8 líneas + 1 comentada) que en el port no existe porque esas IMG van fundidas, el orden de 4 líneas (`plusroad`, `bryx`, `newGen`, `wanted_paths` presentes en ambos) y el `COLFILE NEWVEHICLES.COL` añadido |
| `data/weapon.dat` | Normalizando fin de línea: **idéntico** (103 líneas); los 102 B son CRLF→LF |
| `models/generic/wheels.DFF` / `.TXD` | Lo servido es el **vanilla** (96.067 / 23.976 B). El del mod (1.221.640 / 1.200.584 B) es el juego de ruedas HD: 55 marcos frente a 39 (añade `wheel_classic2`, `wheel_saloon2`, `wheel_saloon3`, `wheel_lighttruck2`, `wheel_offroad2`) y 16 texturas. Lo usan las llantas 6600/6601 del `default.ide` del mod (y `hellenbach`, que en el MVL pide la 6600 y en el port lleva la 253); `limits.ini` sube `MAXWHEELMODELS` a 12 por esto. No es solo taller |
| `models/fronten2.txd` | **NO TOCAR (radio)**. Sus 13 texturas (`mapTop/Mid/Bot01-03`, `upON/upOff/downON/downOFF`) son idénticas byte a byte en el servido; el servido añade 11 logos de emisora (`wildstyle`, `flash`, `kchat`, `fever`, `vrock`, `vcpr`, `espantoso`, `emotion`, `wave103`, `810vcn`, `mp3`) que el mod pasó a `radio.txd` |
| `frontend_ds2/ds3/ds4/nsw/x360/xone.txd` | Lo servido es la versión de `gta_vc_browser/gamefiles/` (fuente 3.0), no la de 2510 (más pequeña). Iconos de mando, cosméticos |
| `ViceEx.SDT` → `Audio/sfx.SDT` | Cola de 13 entradas del `sfx.SDT` servido (9.954 entradas; el vanilla tiene 9.941): índices 5-12 con tamaño/Hz/bucle **idénticos**; 0-4 remuestreados con la misma duración (0-1: 7.328 B @36000 → 6.514 B @32000; 2: 21.158 @33000 → 20.518 @32000; 3-4: 22.528 @22000 → 22.580 @22050). `sfx.SDT` no va en `bootseed/` ni en `reVC.data` |
| `ViceEx.RAW` → `Audio/sfx/9941-9953.mp3` | 13/13 mp3 presentes (2.205…13.837 B). No hay copia del RAW fuera de la instalación |
| `newVehicles.ide` (MVL) | Los 8 (`streetfi`, `peren2`, `trash2`, `hellenbach`, `premier`, `manchez`, `wintergreen`, `polwintergreen`) tienen línea en `default.ide`, en `handling.cfg` (8 + 4 líneas `!` de moto) y en `carcols.dat`, y su `.dff`/`.txd` en `gta3.img/` |

## 5. Lo que importa para jugar (por orden)

1. **Blips nuevos del mod (código, no dato).** `hud.txd` servido = el del mod (57 texturas;
   las 9 que no tiene el vanilla: `propertyR`, `bomb`, `race`, `package`, `jump`, `rampage`,
   `store`, `business`, `radar_waypoint`). El port carga 41 sprites (`RADAR_SPRITE_COUNT`, el
   enum acaba en `RADAR_SPRITE_WAYPOINT`, `src/core/Radar.h:97`); la fuente del mod añade 7
   (`HOUSE_FOR_SALE`, `BOMB_SHOP`, `RACE`, `STORE` tras `IMPROVED_TECH_PART`; `PACKAGE`, `STUNT`,
   `RAMPAGE` tras `EX_DISPLAYED_COLLECTIBLES`) y cambia la textura de `PropertySprite` a
   `business` (`upstream-gta-extended/src/core/Radar.h:99-109`, `Radar.cpp:1426-1491`). Un blip de
   su script con sprite ≥ 41 indexa `RadarSprites[]` fuera de rango: es la caída
   `CSprite2d::Draw ← CRadar::DrawRadarSprite ← CRadar::DrawCoordBlip` de `web/odtrace.log`
   (plan 12.0, tanda `ve85e`). **El dato ya está; falta el código.**
2. **Cargar la partida del mod**: causa medida en §6.4 (12 B de más en las «simple vars» del
   save, antes del bloque de scripts). No es dato servido.
3. **`txd/newspapers.txd` AUSENTE** (924.080 B, 22 texturas `newspaper_intro`, `_lawyer4`,
   `_general1-5`, `_serg1-3`, `_baron2-5`, `_prot1/3`, `_finale`, `_bankj4`, `_count2`, `_porn2`,
   `_cuban4`, `_peace`). Es la feature 2506 «Unique newspapers after certain missions»
   (`ChangesEN.txt`). En `ViceEx.exe` la ruta `ViceExtended/txd/newspapers.txd` y esos nombres van
   pegados a `CRubbish::Render` / `newspaper01_64` (cadenas en los offsets de fichero 0x2F22B0-0x2F2350): son los
   periódicos que vuelan por la calle. Cosmético; sin él el port pinta el periódico vanilla.
   Hipótesis, **no medida**: `0x0FA8` (20 usos; escribe un valor y un instante de caducidad en
   dos globales) podría ser «poner periódico N durante un tiempo»; se confirmaría con un xref
   de `0xCF8474` desde `CRubbish::Render`.
4. **`0x0FA4/0FA5/0FA7` no fijan el resultado del `IF`** (ver §6.5): 1 sitio en su `main.scm`
   (`0FA7` ×1).
5. **Ruedas HD** (`wheels.DFF/TXD` vanilla en lo servido): cosmético.
6. **Iconos de mando** (`ps4btns`, `ps5btns`, `x1btns`, `frontend_ds5` ausentes; `frontend_*`
   en versión 3.0): solo se ven jugando con mando; con teclado manda `pcbtns.txd` (IDENTICO).

Lo que **no** falta: radar HD (64/64 `radarNN.txd`), peds (129/129), jugador/ropa (46/46),
vehículos (48/48 + 16 MVL), armas (94/94, con las 18 nuevas), animaciones de armas nuevas
(`rocket/deagle/steyr.ifp`), mundo 2510 (IPL/IDE, `object.dat`, `occlu.ipl`, `bryx`,
`plusroad`, `newGen`, carteles de Ammu-Nation), `particle.cfg`, `newWeapons.col`, GXT, `features.ini`.

## 6. SCRIPT

### 6.1 Qué `main.scm` se sirve hoy: **el del mod 2510, bit a bit**

| Copia | Tamaño | sha256 |
|---|---|---|
| Instalación `ViceExtended\data\main.scm` (2510) | 1.191.180 | `41a932cc593e9e63a12148deca5c3082e89c828351977605dcb0dc0658e33fac` |
| Paquete `mods\extended\GameFiles\ViceExtended\data\main.scm` | 1.191.180 | idéntico |
| `streamed/data/main.scm` | 1.191.180 | idéntico |
| `bootseed/data/main.scm` | 1.191.180 | idéntico |
| `reVC.data` (`/data/main.scm`, bytes 34.427.483-35.618.663; es la copia que lee el motor) | 1.191.180 | idéntico |
| Vanilla (raíz de la instalación) | 1.269.133 | `0fb3fd45b6a6…` |

`freeroam_miami.scm` (38.197 B, `3bf53e996439`) también es idéntico en instalación, `streamed/` y
`reVC.data`. **`script.img`**: no existe en VC (es de San Andreas); no hay ninguno en la instalación,
el paquete ni lo servido. La copia de respaldo del vanilla que cita el plan
(`gta_vc_browser/tmp/viceext-13/vanilla/main.scm`) ya no está en disco.

### 6.2 Estructura medida (cabecera SCM)

| | Mod 2510 | Vanilla |
|---|---|---|
| Espacio de globales (fin del segmento 1) | 34.624 B | 34.336 B (+288 B = 72 globales más) |
| Objetos declarados | 214 | 204 (+10: `beretta`, `shotgun2`, `uziold`, `ak47`, `desert_eagle`, `steyr`, `m16`, `gr_launch`, `lodngst2meshdam`, `lodngst2mesh`) |
| `MainScriptSize` | 206.049 | 204.596 |
| Misión más grande | 30.863 | 32.408 |
| Misiones | 97 (0 exclusivas) | 97 |

Los **214 nombres de objeto** del script del mod resuelven contra los IDE servidos (`data/default.ide`
+ `data/maps/**/*.ide`): **0 sin definición**. Las claves GXT que use también están (los GXT servidos
son superconjunto de los del mod).

### 6.3 Opcodes no vanilla que usa su `main.scm` (medición existente, no rehecha)

Fuentes: tabla «Familia 13» del plan
`.agents/plans/2026-10-14-12.0-mod-completo-extended.md` (líneas 368-434: aridad y semántica medidas en
`ViceEx.exe`, dispatcher `ProcessCommands4000To4099` VA `0x4946C0`, tabla `0x494A70`) y recuento de
usos de `.agents/plans/mods/10-plan-pasada-siguiente.md` §E21 (líneas 471-480, walk con
`tools/cleo_disasm.py`; el plan 12.0 avisa de que ese recuento **no se ha re-verificado** porque falta
`vc.json`).

| Opcode | Aridad | Semántica (plan 12.0) | Usos en su `main.scm` (mods/10) | En el port (`src/control/Script8.cpp`, tras `VICEEXT_MOD_SCRIPT`) |
|---|---|---|---|---|
| `0x0FA0` | 2 | skin/TXD del ped por nombre (`"%s"`/`"%s_%i"`) | 45 | consume parámetros; guarda variante por ranura (sin efecto visual) |
| `0x0FA1` | 1 → 1 | lee un byte de `CPed` `[+0x668]` | 2 | devuelve la variante guardada |
| `0x0FA2` | 2 | selecciona el ped actual (valida generación del handle) | **91** | réplica (índice en un global propio) |
| `0x0FA3` | 0 | `DoAutoSave` | 0 | marca autosave del port |
| `0x0FA4` | 0 | `sub_48B120(0)` | 0 | ver §6.5 |
| `0x0FA5` | 0 | `sub_48B120(byte[0x93763D] == 0)` | 0 | ver §6.5 |
| `0x0FA6` | 0 | `byte[0xD19CAA] = byte[0x93763D]` | 46 | réplica sobre globales propios (`0x93763D` sin escritor conocido) |
| `0x0FA7` | 0 | `sub_48B120(byte[0xD19CAA])` | 1 | ver §6.5 |
| `0x0FA8` | 1 | `sub_5E1070(param)`: dos globales (valor + tiempo) | 20 | consume y traza; **sin destino** (hipótesis en §5.3) |
| `0x0FA9` | 2 | rumbo del vehículo en grados | 1 | `SetHeading(DEGTORAD(grados))` |

**7 tipos usados, 206 usos.** Ningún otro opcode fuera del rango vanilla aparece en el recuento citado.

### 6.4 El fallo `CRunningScript::Load` al cargar la partida del mod

Lo que dice el plan 12.0 (sesión de noche, líneas 710-716; `ve85c`, 1032-1040; `ve85e`, 1061):
abrir `ViceExtended/userfiles/GTAVCsf1.b` revienta en `CRunningScript::Load` (`memory access out of
bounds`) **igual con el `main.scm` del mod que con el vanilla** → «el save lo escribió su
`ViceEx.exe` y su bloque de script no encaja con este cargador». Salida usada: partida nueva
(funciona: `initstep 0..18`, el script del mod arranca su intro `INT_A`).

**Medido hoy (lectura del save, 201.828 B, sha256 `af058d2200fc…`; la copia del arnés
`gta_vc_browser/tmp/vc-save/GTAVCsf1.b` es el mismo fichero):**

- El **bloque de scripts en sí encaja** con el script servido y con el formato del port: varspace
  **34.624** B (= globales del `main.scm` del mod), `script_data_size` 584 (= la fórmula del port),
  `MainScriptSize` 206.049, misión mayor 30.863, 97 misiones, **54 scripts vivos × 136 B**
  (formato `COMPATIBLE_SAVES`; nombres `package`, `autosav`, `cl_st`, `wrdrb`…, `main`), y el tamaño
  del bloque cuadra al byte (42.564 = 34.624 + 584 + 12 + 54×136).
- Lo que **no** encaja está **antes**: el save trae las «simple vars» con **12 B de más**. El port
  espera el tamaño del bloque de scripts en `work_buff+0xE4` (`SIZE_OF_SIMPLEVARS 0xE4`,
  `src/save/GenericGameStorage.cpp:51`) y la cabecera `SCR\0` en `+0xE8`; en el save el tamaño
  (42.572) está en `+0xF0` y `SCR\0` en `+0xF4` (fichero 248). En `+0xE4` hay
  `08 31 30 30` (el byte `viceExtendedVersion` = 8 con relleno) y en `+0xE8` `00 2E 39 36`
  (`bSaveAnywhere` = 0 con relleno), más 4 B a cero en `+0xEC`.
- La fuente del mod lo confirma a medias: con `VICE_EXTENDED` define `SIZE_OF_SIMPLEVARS 0xEC` y lee
  `CGame::viceExtendedVersion` + `bSaveAnywhere` tras `RadioStationPosition`
  (`upstream-gta-extended/src/save/GenericGameStorage.cpp:48-52` y la lectura de `GenericLoad`);
  el binario 2510 escribe 4 B más que esa fuente (0xF0). Con todo desplazado 12 B, el port lee
  basura como tamaño/varspace/`runningScripts` y la caída aparece en `StartNewScript(0)->Load`.
  Eso explica que falle igual con cualquier `main.scm`.
- **No revisado**: los 23 bloques siguientes (peds, garajes, vehículos, pickups, player info…) pueden
  tener más diferencias del mod; la lista de bloques es la misma en ambas fuentes.

### 6.5 Observación de coherencia: `sub_48B120` es `CRunningScript::UpdateCompareFlag`

Sin decodificar nada nuevo: la medición del plan dice que `sub_48B120` actúa sobre el propio
`CRunningScript` (`ecx`) con `[+0x79]` flags, `[+0x80]` contador y `[+0x82]` modo, AND con contador
1..8 y OR con 0x15..0x1C. En el `CRunningScript` del port (`src/control/Script.h:280-296`,
`MAX_STACK_DEPTH=6`, `NUM_LOCAL_VARS=16`, `NUM_TIMERS=2`) esos desplazamientos son exactamente
`m_bCondResult` (0x79), `m_nAndOrState` (0x80) y `m_bNotFlag` (0x82), y 1..8 / 21..28 son
`ANDS_1..8` / `ORS_1..8` del mismo fichero. Es decir, `0FA4/0FA5/0FA7` son **opcodes de
condición** (fijan el resultado del `IF`). El port los replica con la misma lógica pero sobre
variables estáticas propias (`ViceExtModScriptApplyFlag` sobre `s_odModScriptFlags`,
`Script8.cpp`), así que **no tocan `m_bCondResult`**: el `IF` que los use se decide con el resultado
anterior. Arreglo directo: `UpdateCompareFlag(false)`, `UpdateCompareFlag(byte == 0)` y
`UpdateCompareFlag(guardado)`. Impacto: 1 sitio (`0FA7`).

## 7. Excluidos y NO TOCAR

| Tema | Ficheros del mod | Estado |
|---|---|---|
| Taller de tuning (excluido) | `models/vehmods/{spoilers,skirts,scoops,vents}.dff`; filas 6620-6657 del `default.ide`; `RandomVehicleModsInTraffic` | NO-APLICA. Ojo: `neo/carTweakingTable.dat` y `rimTweakingTable.dat` se sirven igual (IDENTICO) |
| Idiomas (ucraniano) | `TEXT/ukrainian.gxt`, `models/fonts_u.txd` | NO-APLICA (`russian.gxt` y `fonts_r.txd` sí se sirven, idénticos/superconjunto) |
| CLEO | ninguno: la instalación no trae carpeta CLEO ni el paquete la incluye | — |
| modloader como sistema | 36 ficheros (`modloader/.data/**`, `modloader.ini`, `modloader.log`) + `modloader.asi` | NO-APLICA. Sus **datos** (`modloader/ViceExtended/**`) sí están auditados arriba |
| NO TOCAR: radio | `models/radio.txd` (IDENTICO), `models/fronten2.txd` (DISTINTO solo por 11 logos de emisora), iconos `R*` de `hud.txd` (IDENTICO) | marcados, sin cambios |
| NO TOCAR: nado | clips `Swim_*` del `ped.ifp` (presentes), `EnableSwimming` del `features.ini` (servido), sonido de agua de 2510 (una de las 13 muestras de `ViceEx.RAW`, no identificada) | marcados, sin cambios |

## 8. Diferencias con la auditoría anterior y cómo repetir esta

- `tools/viceext-parity-check.py` (último resultado citado en el plan 12.0, `ve85`:
  `identicos=105 distintos=20 ausentes=38`) compara contra el **paquete** a nivel de fichero y mapea
  `cdimages/x.img` → `models/x.img`, que no existe: cuenta esos 16 ficheros como AUSENTE «cubierto»
  **sin mirar dentro**. Aquí se han comparado las 484 entradas una a una (todas idénticas) y se han
  medido los superconjuntos (GXT, `ped.ifp`) que allí son «pendiente/cubierto». Los recuentos no son
  comparables 1:1 (aquí entra la instalación, con RAW, userfiles, modloader y `ViceEx.ini`).
- Ningún dato del mod servido está desincronizado entre `streamed/`, `bootseed/` y `reVC.data`.
- Lectores usados (temporales, fuera del repo, solo biblioteca estándar, `python -I`):
  `C:\Users\s0rno\AppData\Local\Temp\claude\C--Users-s0rno-OneDrive-Documents-lance-vance\afc592ea-92c1-4c71-800e-e3b349b4a550\scratchpad\audit\audit.py`
  (hash de ficheros, mapeo, IMG por entradas, SDT, MVL, SCM, coherencia de copias; salida
  `out\audit.json`), `deep.py` (claves GXT, clips IFP, texturas TXD; salida `out\deep.json`) y
  `tables.py` (tablas de este documento), en esa misma carpeta.
