---
name: vice-extended-inclusion
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-19
---

# Plan — Inclusión de Vice Extended en el port web (investigación 2026-09-19)

Documento de decisión: **qué entra, qué no, con qué evidencia, qué impacto tiene y
qué puede salir bien o mal**. Complementa `.agents/plans/vice-extended.md`
(inventario y §5 fuentes abiertas) y no lo sustituye.

Norma vigente desde 2026-09-12: plan antes de tocar código.

## 0. Alcance y exclusiones del jugador

**Fuera de alcance (decisión del jugador, 19/09):**

| Excluido | Qué arrastra técnicamente |
|---|---|
| Taller / garaje de tuning (Transfender) | script propio + `carTweakingTable`/`rimTweakingTable` + ruedas + `MAXWHEELMODELS` + integración SCM. Es el sistema con más dependencias del mod. |
| Modo foto | readback de WebGL (F12 ya da problemas en el port) y el propio mod documenta fotos en negro con antialiasing. |
| GPS | algoritmo de ruta + render del trayecto + opcodes del script del mod. Excluirlo obliga a **stub mudo** si algún día se corre su `main.scm`. |

**Dentro:** datos/NVC-lights del mapa, radar y HUD, peds/jugador, armas nuevas,
vehículos nuevos, animaciones, textos, toggles baratos (`features.ini`) y las
mecánicas de la lista del mod **excepto** las tres de arriba.

## 1. Método y aislamiento (importante)

- **Cero cambios al estado del juego.** Hoy no se ha tocado `streamed/`,
  `bootseed/`, `web/public/manifest.json` ni se ha recompilado nada. Todo el
  análisis vive **fuera del repo**, en `/tmp/ve-audit/` (script
  `img_audit.py`, `txd_audit.py`, más los volcados `*.out`), igual que el
  harness `vc-e2e`. Es de solo lectura sobre el mod y sobre el port.
- Ficheros del repo previstos: **solo documentos** (este plan y `HISTORIAL.md`).
- **Sí dependen del estado del juego** (se pedirá permiso antes de cada uno):
  1. copiar assets a `gta_vc_browser/streamed/` y regenerar
     `web/public/manifest.json` (`tools/gen_manifest.py`) y
     `bootseed/` (`tools/stage_bootseed.py`) → cambia peso de descarga y RAM;
  2. recompilar el wasm con límites nuevos (`config.h`) → cambia el binario;
  3. subir `VERSION` en `gta_vc_browser/web/lib/index.js` → etiqueta de build.
- Cada bloque se probará en **una rama de trabajo del propio port** (overlay)
  y se medirá contra el build actual. Nada de aplicar dos bloques a la vez: si
  algo se rompe, se sabe qué.

## 2. Evidencia medida (no estimada)

### 2.1 IMG y `.dir` del mod: formato e integridad

- Formato leído: entradas de 32 B = `u32 inicio`, `u32 nº sectores`,
  `char[24]` nombre.
- **Convención SECTORES** y encaje exacto con su `.img` (suma × 2048 == tamaño):

  | imagen | entradas | suma×2048 | tamaño `.img` |
  |---|---|---|---|
  | anims | 14 | 1 009 664 | 1 011 712 |
  | generic | 1 | 4 096 | 4 096 |
  | objects | 72 | 15 480 832 | 15 480 832 |
  | peds | 129 | 11 137 024 | 11 137 024 |
  | player | 46 | 4 878 336 | 4 880 384 |
  | radar | 64 | 4 325 376 | 4 325 376 |
  | vehicles | 48 | 14 938 112 | 14 940 160 |
  | weapons | 94 | 3 887 104 | 3 887 104 |

- **Integridad: 0 entradas fuera de rango, 0 solapes, 0 duplicados, 0 de
  tamaño 0.** (El riesgo que apuntaba el plan viejo queda cerrado.)
- **Compatibilidad con el motor:** `CdStreamPosix::CdStreamAddImage` lee ese
  mismo par `u32` como `start`/`sectors`, y `CStreaming::LoadCdDirectory`
  (`Streaming.cpp:446`) usa `direntry.size` como nº de sectores →
  los `.dir` del mod encajan tal cual. Nota: **son .dir no-retail**
  (sectores, no bytes); el build nativo/otras plataformas los leería mal, el
  web no.

### 2.2 Qué trae cada imagen (y qué pisa)

**431 entradas pisan ficheros que ya existen en el port; 37 son nuevas.**

| imagen | entradas | MB | pisan | nuevas | nuevas (lista) |
|---|---|---|---|---|---|
| radar | 64 | 4,1 | 64 | 0 | — (radar00…radar63: 1:1 con el port) |
| peds | 129 | 10,6 | 129 | 0 | — |
| player | 46 | 4,7 | 46 | 0 | — |
| vehicles | 48 | 14,2 | 48 | 0 | — |
| weapons | 94 | 3,7 | 76 | 18 | `beretta`, `desert_eagle`, `gr_launch`, `m16`, `steyr`, `shotgun2`, `rcgrenade`, `grenade2` (+ .txd) |
| objects | 72 | 14,8 | 56 | 16 | `bryx.col`, `bryx_lights.dff`, `plusroad.col`, `wsh_roadswshz1-6.dff`, `lodbryx_lights.dff`, `lodngst2meshdam.dff`, `027agen.dff/.txd`, `lhaitcut.txd`, `miamiland027a.txd` |
| anims | 14 | 1,0 | 11 | 3 | `deagle.ifp`, `steyr.ifp`, `rocket.ifp` |
| generic | 1 | 0,004 | 1 | 0 | `icons4.txd` |

Lección de impacto: **radar/peds/player/vehicles/weapons son sustituciones
masivas**, no añadidos. Copiar una de esas imágenes cambia medio juego; hay que
querer eso bloque a bloque.

### 2.3 Formatos de datos del mod vs lo que parsea el motor

- **`gta_vc.dat` del mod**: CDIMAGE ×8, IDE ×34, IPL ×40, COLFILE ×2,
  SPLASH ×3 → **todas soportadas** (`FileLoader.cpp:75-127`: EXIT, IMAGEPATH,
  TEXDICTION, COLFILE, MODELFILE, HIERFILE, IDE, IPL, SPLASH, CDIMAGE).
  El port hoy **no usa ni una línea `CDIMAGE`** (su `gta3.img` entra por código,
  `Game.cpp:514`).
- **IDE del mod** (35 ficheros): secciones `objs`, `tobj`, `2dfx`, `path`
  (+ `default.ide` con `objs`, `tobj`, `cars`, `peds`, `weap`, `hier`) →
  todas están en `CFileLoader::LoadObjectTypes` (`FileLoader.cpp:596`).
  Efectos `2dfx`: 1075 en el mod vs 1069 en vanilla (misma forma).
- **IPL del mod**: `inst`, `cull`, `pick`, `path` → soportadas.
- **Vehículos nuevos**: `newVehicles.ide` usa secciones `default`, `handling`,
  `handling2`, `carcols` (formato **Maxo's Vehicle Loader**). **Nuestro loader
  no conoce ninguna de esas secciones** → requeriría cargador propio.
- **`ped.ifp`** y los `.ifp` nuevos: los IFP de una IMG se registran solos en
  `LoadCdDirectory` (`CAnimManager::RegisterAnimBlock`), pero los **grupos** de
  animación (p. ej. `steyr`) tienen que existir en el enum/`AssocGroup` → sin
  código, `deagle.ifp`/`steyr.ifp` no se usan.

### 2.4 Armas: el parse no rompe, el nombre sí

- `CWeaponInfo::LoadWeaponData` lee **26 campos** con un `sscanf` por línea; el
  mod añade 2 campos al final → **se ignoran sin romper el parse**.
- Pero `FindWeaponType` (`WeaponInfo.cpp:271`) recorre la tabla de nombres y
  **devuelve `WEAPONTYPE_UNARMED` si no encuentra el nombre** → meter el
  `weapon.dat` del mod sin ampliar la tabla hace que `Beretta`, `Ak47`, `M16`,
  `Steyr`, `Gr_launch`, `Shotgun2`, `Uziold`, `desert_eagle` **se escriban
  encima de Unarmed** (puños con stats de rifle). No crashea: corrompe.
- El enum tiene **37 slots** (`WEAPONTYPE_TOTALWEAPONS = 37`) con ~11 en uso →
  hay índice de sobra; falta la tabla de nombres y los `switch` asociados
  (`IsShotgun`, iconos de HUD, anims, IA).
- El mod asigna a las armas nuevas **IDs de modelo 6661-6669** (y los coches
  6500-6599) → por encima de `MODELINFOSIZE = 6500` de hoy.

### 2.5 Límites del motor vs lo que pide el mod

| Límite | Hoy (port) | El mod necesita | Efecto si no se sube |
|---|---|---|---|
| `MODELINFOSIZE` | 6500 | ~6700 (armas 6661+, coches 6500+) | IDs fuera de rango: sin modelo/colisión, o tabla corrupta |
| `MAX_CDIMAGES` | 8 | 1 (gta3) + 8 del mod = 9 → 12-16 | `ASSERT` en `CdStreamAddImage` |
| `NUMVEHICLES` | 110 | 130 (`limits.ini` del mod) | no caben los coches nuevos |
| `NUMCOLMODELS`/`NUMOBJECTS` | (config.h) | `limits.ini`: 4400/460… | colisiones descartadas silenciosamente |

### 2.6 Audio

`ViceEx.SDT` = **260 B** y `ViceEx.RAW` = **365 KB**: banco **aparte y pequeño**
(no sustituye el `sfx.SDT` del port, que es el parcheado por
`tools/fix_sdt_*`). Integrarlo = extraer muestras y añadirlas a nuestro
pipeline + un ID por arma nueva: trabajo de datos, no un copiar.

### 2.7 Texturas: plataforma, formato y compresión (CERRADO — evidencia dura)

**Método.** Parser propio (`/tmp/ve-audit/txd_audit2.py`, salida `txd2.out`;
nombres en `txd_names.py`; coste en `txd_ram.py`) que replica **byte a byte** los
lectores de librw: `Texture::streamReadNative` (`texture.cpp:481`) despacha por
plataforma a `d3d8::readNativeTexture` (`d3d8.cpp:620`),
`d3d9::readNativeTexture` (`d3d9.cpp:704`) o `gl3::readNativeTexture`
(`gl3raster.cpp:1038`). Solo lectura sobre el mod y sobre el port.

**Los dos layouts nativos NO son iguales** (esto invalidó el primer intento):

| offset | D3D8 (plat 8) | D3D9 (plat 9) |
|---|---|---|
| 0 / 4 | platform / filterAddressing | platform / filterAddressing |
| 8..40 / 40..72 | name[32] / mask[32] | name[32] / mask[32] |
| 72 | `format` (flags de raster) | `format` |
| 76 | **`hasAlpha` (0/1)** | **`d3dformat`** (fourcc si comprimido) |
| 80 / 82 | width u16 / height u16 | width u16 / height u16 |
| 84 / 85 / 86 | depth u8 / numLevels u8 / type u8 | igual |
| 87 | **`compression` = código 0..5** (1=DXT1, 2=DXT2, 3=DXT3, 4=DXT4, 5=DXT5) | **bitfield** (bit3 = comprimido) |

Con la paleta delante de los niveles cuando `format` trae PAL4 (4·32 B) o PAL8
(4·256 B) (`d3d8.cpp:623`). Verificado contra bytes crudos: en `radar00.txd` del
port `type=4` (TEXTURE) y `compression=1`; en `ps3btns.txd` `compression=0`.

**Resultado: 1.381 texturas del mod; plataforma 8 y 9 y nada más.**

| origen | TXD | texturas | plataforma | formatos base | compresión |
|---|---|---|---|---|---|
| radar.img | 64 | 64 | 8 | C565 ×63, C4444 ×1 | **DXT3 ×64** |
| generic.img (icons4) | 1 | 2 | 8 | C565\|MIP | DXT1 ×2 |
| peds.img | 128 | 458 | 8 | C565\|MIP 327, C1555\|MIP 117, C4444\|MIP 14 | DXT1 444, DXT5 14 |
| player.img | 23 | 52 | 8 | C565\|MIP | DXT1 ×52 |
| vehicles.img | 19 | 201 | 8 | C888\|MIP 110, C8888\|MIP 57, C565 13, C4444 9… | DXT1 13, DXT3 9, sin comprimir 179 |
| weapons.img | 47 | 93 | 8 | **C8888\|PAL8 55, C8888\|PAL4 26**, C565 6… | DXT1 6, DXT3 2, sin comprimir 85 |
| objects.img | 18 | 564 | **8 (536) + 9 (28)** | C565 452, C4444 60, **C888\|PAL4/PAL8 ×28**, C8888\|PAL4 12… | DXT1 459, DXT3 58, **DXT2 1**, DXT5 1, sin 45 |
| TXD sueltos (24) | 24 | 623 | 8 (587) + 9 (36) | C8888 508, C565 63, C4444 37… | DXT1 73, DXT3 36, DXT5 1 |
| *(referencia)* port `models/*.txd` | 18 | 370 | 8 | C8888 218, C4444 71, C1555 40, C565 34, **PAL4/PAL8 ×5** | DXT1 75, DXT3 69, DXT5 1 |

Conclusiones medidas:

1. **Plataforma 8 y 9**, exactamente las dos que el port ya lee
   (`texture.cpp:492-496`). Nada de PS2 (4), Xbox (5), WDGL (11) ni GL3 (12).
2. **Formatos base**: C565, C1555, C4444, C888, C8888 y paletizados PAL4/PAL8 —
   todos dentro del enum `Raster::Format` de librw. Ningún LUM8/D16/D24/D32.
3. **Compresión: DXT1, DXT3 y DXT5, más un caso de DXT2** (§2.7-bis).
4. El **port ya ejercita las tres vías vivas** con sus propios ficheros: DXT1/3/5
   (radar, generic, ps3btns, fronten2) y paletizado (las 5 texturas PAL4/PAL8 de
   `frontend_ds2.txd`). No es camino nuevo — nota: el paletizado del port se
   concentra en el TXD de botones de mando, así que conviene probarlo cargando
   armas antes de sustituir `weapons.img` (85 paletizadas).
5. En web sin S3TC, `d3d_to_gl3` (`raster.cpp:503`) devuelve nil y entra el
   fallback por `Image`: decodifica DXT por software (`d3d.cpp:861-878`) y
   despaletiza (`unpalettize()`). Es la vía que quedó verde con el fix de
   `radar*` (F3a/F3b de `freeze-audio-txd-pal8-saves.md`).

#### 2.7-bis — El único caso que NO entra: `helipad_strutT` (DXT2)

`objects.img → 027agen.txd → helipad_strutT` (128×128, 1 nivel, base C4444) es
**DXT2**, y el decodificador por software de librw solo cubre DXT1/DXT3/DXT5:
`d3d.cpp:860-880` hace `switch(natras->format)` con `case D3DFMT_DXT1/DXT3/DXT5`
y `default: image->destroy(); return nil;`. `d3d_to_gl3` (`raster.cpp:513`)
tampoco mapea DXT2/DXT4 (`dxt = 0` → nil), así que falla **con y sin S3TC**.
Consecuencia: esa textura se queda sin raster, el diccionario carga parcial
(el `continue` de `TexRead.cpp:98` salta la textura, no el TXD) y ese objeto se
pinta sin textura. El port **no tiene ninguna DXT2/DXT4 hoy** (0 en sus 12.367
texturas), así que es riesgo nuevo pero acotado a **una textura**.
Arreglo: 2 líneas en `d3d.cpp` (`case D3DFMT_DXT2: setPixelsDXT(3, pix)` y
`case D3DFMT_DXT4: setPixelsDXT(5, pix)`); el bloque es idéntico, solo cambia
que DXT2/DXT4 llevan el alfa premultiplicado.

### 2.8 Nombres de textura: la trampa que queda (no es el formato)

El formato está resuelto; el riesgo restante es **de nombres**: el motor localiza
sprites por nombre y varios TXD del mod **quitaron nombres que el port sí pide**.

| TXD | MOD vs PORT | nombres que el port pide y el MOD no tiene | Qué pasa si se sustituye tal cual |
|---|---|---|---|
| `fronten2.txd` | 13 vs 24 | **11 logos de emisora**: `wildstyle, flash, kchat, fever, vrock, vcpr, espantoso, emotion, wave103, mp3, 810vcn` | El menú los busca en `FRONTEN2.TXD` (`Frontend.cpp:140-170` + `3063` `SetCurrentTxd`) → 11 sprites nil. **El mod los tiene en su propio `models/radio.txd`** (14 texturas, plat 9 / A8R8G8B8) → hay que fusionarlos o cargar `radio.txd` |
| `frontend_ds2/3/4/nsw/x360/xone.txd` | 2 vs 5-6 | **`fe_arrows2`, `fe_arrows3`, `fe_arrows4`** (+ `fe_controllersh` en nsw) | `XINPUT` está definido (`config.h:340`) → `GAMEPAD_MENU` activo → el port pide esos nombres (`Frontend.cpp:165-168`, `6737+`) y en el port existen **solo** ahí (grep: `fe_arrows2` → `frontend_ds2/3/4.txd`) → flechas del menú sin textura |
| `hud.txd` | 57 vs 48 | **ninguno** | Al contrario: **aporta 9** que el port pide y hoy no tiene en ningún TXD: `radar_waypoint` (`Radar.cpp:1100`, waypoint de MAP_ENHANCEMENTS) y `bomb` (`Hud.cpp:179`, icono del arma). `propertyR/business/jump/package/race/rampage/store` no los referencia el código → relleno inofensivo |
| `generic.txd` | 40 vs 35 | ninguno | Añade `Grass_dirtb_64HV`, `dirt64b2`, `scoopsbody`, `vehiclegeneric256` |
| `particle.txd`, `fonts_r.txd`, `nswbtns/ps3btns/x360btns.txd` | iguales | ninguno | Sustitución limpia |
| `radar00..63.txd` | 64 vs 64 | ninguno; **256×256 (MOD) vs 128×128 (PORT)** | Geometría OK: `DrawRadarSection` (`Radar.cpp:778`) usa UV normalizadas (`out.x /= RADAR_TILE_SIZE`, línea 1358) → el tamaño del tile no cambia el mapeo, solo el detalle y la VRAM |

Matiz de mecánica: como los sprites se resuelven con `RwTextureRead` sobre los
diccionarios cargados, un nombre ausente en un TXD sustituido **puede** aparecer
en otro ya cargado; el criterio correcto es "¿existe ese nombre en algún TXD que
el port cargue?" — y para los 11 logos y las 3 flechas la respuesta es **no**.

### 2.9 Coste por bloque (bytes en GPU con el bloque cargado)

`/tmp/ve-audit/txd_ram.py`, mips incluidos. WebGL fuerza RGBA8 para todo lo no
comprimido (`gl3raster.cpp:99` `if(gl3Caps.gles)`; `rasterCreateTexture` solo
acepta C8888/C888/C1555).

| bloque | texturas | sin S3TC (RGBA8) | con S3TC (bloques) | comparación |
|---|---|---|---|---|
| radar HD | 64 | **16,8 MB** | 4,2 MB | port hoy (128², DXT1): 4,2 / 1,0 MB |
| icons4 | 2 | 0,03 MB | ~0 | — |
| peds | 458 | **84,4 MB** | 10,8 MB | sustituye, no añade |
| jugador | 52 | 17,9 MB | 2,2 MB | — |
| vehículos | 201 | 10,5 MB | 9,6 MB | 179 de 201 sin comprimir |
| armas | 93 | 12,2 MB | 11,6 MB | 85 paletizadas → poco comprimibles |
| objetos (mapa) | 564 | 67,3 MB | 10,6 MB | incluye reemplazos vanilla |
| TXD sueltos (HUD/menú) | 623 | 77,7 MB | 33,4 MB | `fronten2.txd` solo: **38,0 MB** sin S3TC vs 3,2 del port (9 mapas de 1024²) |
| *(ref.)* port `models/*.txd` | 370 | 32,0 MB | 13,3 MB | — |
| *(ref.)* port `gta3.img` completo | 11.997 | 1.336 MB | 179,5 MB | techo de lo que ya streamea |

Lectura honesta: **las texturas del mod son mayoritariamente DXT** (radar, peds,
jugador y casi todo objetos), así que **con S3TC cuestan parecido a lo que ya
hay**. El coste se concentra en (a) navegadores **sin S3TC**, donde todo cae a
RGBA8 (peds ×8, objetos ×6) y (b) `fronten2.txd`, que multiplica ×12 la memoria
del mapa del menú.

## 3. Veredicto por bloque

| # | Bloque | Veredicto | Razón real |
|---|---|---|---|
| 1 | Radar HD (`radar00-63`) | **Entra (medido)** | 64 tiles 1:1, DXT3 256²; `DrawRadarSection` usa UV normalizadas (§2.8) → el tamaño no afecta. Coste 16,8 MB sin S3TC / 4,2 con (port: 4,2 / 1,0) |
| 2 | `icons4.txd` (blips) | **Entra** | DXT1 ×2, sin nombres nuevos ni faltantes |
| 3 | Peds (128 TXD / 458 tex) + jugador (23 / 52) | **Entra con prueba** | 0 nombres faltantes; DXT1/DXT5 y C565\|MIP; sin S3TC = 84 MB (peds) → vigilar RAM |
| 4 | Texturas de coche | **Entra con prueba** | 179 de 201 sin comprimir (C888/C8888) → 10,5 MB; 0 nombres faltantes por fichero |
| 5 | HUD / menú / partículas | **Sí `hud.txd` / `generic.txd` / `particle.txd`; NO tal cual `fronten2.txd` ni los `frontend_ds*/x360/xone`** | `hud.txd` **aporta** `radar_waypoint`+`bomb` que el port pide y hoy no tiene en ningún TXD; `fronten2` pierde 11 logos (están en el `radio.txd` del mod) y los `frontend_ds*` pierden `fe_arrows2/3/4` con `GAMEPAD_MENU` activo (§2.8) |
| 5-bis | `radio.txd`, `weaponSights.txd`, `newspapers.txd` | **Solo con su feature** | Decidido: del `radio.txd` solo se **extraen sus 11 logos** para fusionarlos en `fronten2.txd` (el fichero como tal no entra); `weaponSights`/`newspapers` son arte de features no incluidas (peso muerto) |
| 6 | `ped.ifp` / `.ifp` nuevos | **Parcial** | clips nuevos sin grupo en el enum no se usan; los sustituidos cambian el *feel* sin código |
| 7 | GXTs ×7 (incl. español) | **Entra** | formato GXT VC; cadenas de sus armas/garajes quedarán sin uso |
| 8 | Mapa (objects.img + maps + zon/col) | **Entra con trabajo** | IDE/IPL/CDIMAGE soportados y `.dir` consistentes; pide límites + extractor + copiar el bloque entero (los reemplazos de colisión van incluidos) |
| 9 | Armas nuevas (7) | **Código** | nombres → tabla/enum y switches; IDs 6661+; anims; sonidos; IA/tienda |
| 10 | Vehículos nuevos | ✅ **Hecho 19/09** | `tools/import_mvl_vehicles.py` traduce su `newVehicles.ide` (MVL) a `default.ide`/`handling.cfg`/`carcols.dat`; 8 vehículos 6500-6507, `NUMVEHICLES` 130, audio, `MI_VEEXT_*` y grupos de moto en `CBike`. Cheat `CRAZYRIDES` + `tools/vehicles-smoke-test.mjs` (PASS) |
| 11 | Toggles de `features.ini` | ✅ **Paridad 19/09** | Los que el mod trae **encendidos** están portados (`VICEEXT_SWIMMING`, `VICEEXT_RECOIL`, `VICEEXT_NO_CAR_BOUNCE`, `VICEEXT_SPRINT_HEAVY`); los que trae a 0 no se portan y `EnableDistantLights`/`RandomVehicleModsInTraffic` dependen de su renderer / del taller |
| 12 | Mecánicas (nadar, escalar, 1ª persona, autosave, esconderse de la poli, drive-by, IA) | **Parcial** | Nadar sin morir ✅, esprintar con armas pesadas ✅, retroceso ✅, no salir despedido del coche ✅, 1ª persona en modo *peek* ya existía en el port. Quedan: escalar (su ini lo trae **a 0**), autosave, esconderse de la poli, drive-by ampliado e IA |
| 13 | `main.scm` del mod | **Descartado (con medición)** | Ver §8.F: es otro build (1.191.180 B vs 1.269.133 B del de serie, 1.088.070 bytes distintos) y sus opcodes propios no existen en el motor; sin el código nativo del mod no se pueden stubbear con semántica |
| 14 | Taller/tuning, modo foto, GPS | **FUERA** | decisión del jugador |
| 15 | `ViceEx.exe`, `.asi`, `Bonus/x64_beta`, `MVLConverter` | **No entra** | x86 nativo; `MVLConverter` solo como spec |
| 16 | `radio.txd`, `newspapers.txd`, `weaponSights.txd` | **No entrar solos** | arte de features que requieren código → peso muerto |
| 17 | `features.ini`/`limits.ini`/`modloader.ini`, `.data`/`.profiles` | **No entra** | el motor no los lee / bookkeeping del modloader |
| 18 | `ViceEx.SDT`/`.RAW` como banco | **No entra entero** | pipeline de audio propio; solo interesan sus muestras nuevas |

## 4. Impacto (peso, RAM, arranque)

| Bloque | MB nuevos | Dónde va | Efecto en el jugador |
|---|---|---|---|
| Radar HD | ~4,1 (sustituye) | `streamed/models/gta3.img/` | manifiesto igual de grande; nada de RAM extra |
| icons4 + HUD/menú | ~6-7 (sustituye) | `streamed/models/` | `bootseed` (models no está en `FULL_DIRS`; valorar) |
| Peds+jugador | ~15,3 (sustituye) | gta3.img suelta | más peso on-demand; peds más caros de cargar |
| Mapa | ~17,4 + IPL/IDE/COL | nueva(s) IMG + `data/maps` | **+1 CDIMAGE abierta**, `MODELINFOSIZE`↑, riesgo de más RAM y de arranque más lento |
| Armas/vehículos | 3,7 / 14,2 | nueva IMG | +1 CDIMAGE, `NUMVEHICLES`↑ |
| Mecánicas | 0 | código | riesgo de regresión (ped/cámara/guardado), no de peso |

Regla: cada IMG nueva es un `CdStreamAddImage` → sube `MAX_CDIMAGES` y añade
buffers; conviene **una sola IMG agregada** para todo lo del mod en vez de ocho.

## 5. Qué puede salir bien y qué puede salir mal (mecanismo, no intuición)

1. **Radar HD** — bien: sustitución 1:1. Mal: si alguna textura usa un formato
   no soportado, el radar sale con bandas/negro y **hay que revertir**; el port
   ya parcheó `TexRead` para no destruir un diccionario entero por una textura
   (`TexRead.cpp:98`), así que el fallo sería parcial y silencioso.
2. **Peds/jugador** — bien: mismos nombres. Mal: TXD nuevos → peds negros o
   brillantes; jerarquías distintas → brazos raros. Prueba: captura de un ped
   con skin y uno sin.
3. **Mapa** — bien: IDE/IPL/COL/CDIMAGE soportados y `.dir` verificado.
   Mal: (a) IDs > `MODELINFOSIZE` → objetos sin modelo (huecos) o corrupción;
   (b) **copiarlo a medias**: los 56 reemplazos incluyen `haiti.col`,
   `haitin.col`, `docks.txd`, `nbeachw.col`… es decir, colisiones y texturas de
   zonas vanilla; mezclar IPL del mod con colisión vieja = paredes invisibles o
   atravesar suelo; (c) luces NVC del mod (feature de su renderer) no se
   renderizan aquí: se verá distinto, no roto; (d) memoria: más IMG abiertas.
4. **Armas** — bien: `weapon.dat` parsea. Mal: nombres sin registrar →
   **pisar Unarmed** (silencioso); IDs 6661+ → fuera de rango; anims sin grupo →
   postura estática; sin sonidos → disparos mudos; sin IA/script → no aparecen
   en el mundo ni en tiendas.
5. **Vehículos** — mal típico: el IDE MVL no se parsea → no se ven ni se
   conducen; `NUMVEHICLES` corto → coches que no spawnean; ruedas/colores
   extra requieren código (`MAXWHEELMODELS`).
6. **Mecánicas** — riesgo de regresión en `Ped`/`Cam`/`PCSave`: por eso una
   build por mecánica, con sonda y captura, y nada de mezclar.
7. **Script del mod** — `0FA8` desconocido: desync o crash; y ahora además
   llamadas al GPS excluido → stub mudo.

## 6. Plan de pruebas (cómo se demuestra cada cosa)

| Bloque | Preparación | Criterio de éxito | Cómo se mide |
|---|---|---|---|
| Radar/HUD | sustituir TXD en `streamed` + manifiesto | radar legible y sin bandas en 3 zonas | capturas antes/después + sonda WebGL |
| Peds/jugador | ídem | sin peds negros ni skinning roto | capturas (peatón + Tommy a pie y en coche) |
| Mapa | IMG nueva + IDE/IPL/COL + límites | arranque sin `FAIL`, zona bryx/plusroad transitable | log de carga, `odtrace`, RAM vs build previo, capturas aéreas |
| Armas | tabla de nombres + enum + `weapon.dat` + modelos | 7 armas disparan, suenan y salen en tienda | prueba in-game por arma + log |
| Vehículos | cargador MVL + `NUMVEHICLES` | coche spawnea, conduce y colisiona | debug spawn + captura |
| Mecánicas | código por mecánica | comportamiento igual a SA en el caso base | sonda por mecánica + captura |

## 7. Orden recomendado

1. **Datos visibles** (radar HD, icons4, peds/jugador, texturas de coche):
   sustitución pura, reversible, sin límites → primer bloque demostrable.
2. ✅ **Test de formatos TXD CERRADO** (§2.7-2.9): plataforma 8/9, formatos del
   set de librw, DXT1/3/5 y paletizado ya ejercitados por el port. Único hueco:
   **1 textura DXT2** (`helipad_strutT`) → 2 líneas en `d3d.cpp`.
3. **Fusionar los nombres que faltan antes de sustituir por lotes** (§2.8 y
   §8.E, ya decidido): meter los 11 logos del `radio.txd` del mod dentro de
   `fronten2.txd` —hace falta un escritor de TXD, no existe en `tools/`— y
   **no sustituir** los `frontend_ds*/x360/xone` (o reponer sus 3 flechas).
   `hud.txd` / `generic.txd` / `particle.txd` entran tal cual.
4. **Límites + extractor IMG→carpeta suelta** → habilita mapa y agregados.
5. **Mapa** del mod (bryx, plusroad, newGen) como bloque único, con NVC
   documentado como “no se renderiza”.
6. ✅ **Vehículos** (cargador MVL, `tools/import_mvl_vehicles.py`) y **armas**
   (tabla/enum/anims/sonidos, pack 3).
7. **Mecánicas**: hechas las que su `features.ini` trae **encendidas** y son
   locales (nadar sin morir, retroceso, no salir despedido del coche, esprintar
   con armas pesadas) ✅; pendientes las caras (autosave, esconderse de la
   policía, drive-by ampliado, primera persona completa, IA). Escalar está a 0
   en su ini, así que no se porta.
8. ❌ `main.scm` del mod: **descartado con medición** (§8.F) — sin su código
   nativo los opcodes propios no tienen semántica y un stub mudo desincroniza.

## 8. Inventario de exclusiones (respuesta directa a "qué queda fuera y por qué")

### A. Fuera por decisión del jugador (19/09)

| Excluido | Lo que arrastra |
|---|---|
| Taller / garaje de tuning | script propio + `carTweakingTable`/`rimTweakingTable`/`worldTweakingTable` + ruedas + `MAXWHEELMODELS` + integración SCM. El sistema con más dependencias del mod. Sus `.dat` (`neo/`) quedan sin uso |
| Modo foto | readback de WebGL (F12 ya dio problemas) y el propio mod documenta fotos en negro con antialiasing |
| GPS | algoritmo de ruta + render del trayecto + opcodes del script. Excluirlo obliga a **stub mudo** si algún día se corre su `main.scm` |

### B. No entra: imposible en wasm (no es preferencia)

`ViceEx.exe`, `modloader.asi`, `Bonus/x64_beta`, cualquier `.dll` → **x86 nativo**.
`MVLConverter` → herramienta Windows (solo sirve como spec del formato MVL).
`modloader/.data`, `.profiles`, `features.ini`, `limits.ini`, `modloader.ini`
como ficheros → **el motor no los lee**; los knobs equivalentes son defines de
`config.h` y cada uno es un cambio de código.
`ViceEx.SDT` + `ViceEx.RAW` como banco → incompatible con el pipeline web
(muestras sueltas + `fix_sdt_*`); solo interesan sus muestras nuevas.
El **código fuente del mod no existe** (ver §5): todo lo "de código" es
reimplementación nuestra guiada por spec.

### C. Entra, pero no hoy: depende de trabajo propio (con su porqué)

| Pieza | Por qué no entra ya |
|---|---|
| Mapa `objects.img` + `bryx`/`plusroad`/`newGen` | Pide extractor IMG→carpeta suelta, `CDIMAGE` en el `.dat`, subir `MODELINFOSIZE`/`MAX_CDIMAGES` y **copiar el bloque entero** (sus IPL/COL pisan zonas vanilla: a medias = paredes invisibles) |
| ~~7 armas nuevas~~ ✅ 19/09 (pack 3) | Hecho: enum/tablas/`switch`, IDs 6660-6669, `weapons.img`+`anims.img` importadas, grupos `deagle`/`steyr`/`rocket`, proyectil propio del lanzagranadas, cheat `CRAZYTOOLS` y sonda `tools/weapons-smoke-test.mjs`. **Queda**: su banco `ViceEx.{SDT,RAW}` (13 muestras, sin nombres → hay que mapearlas de oído) y la **tienda/misiones**, que dependen de su `main.scm` |
| ~~Vehículos nuevos (ID 6500+)~~ ✅ 19/09 | Hecho con `import_mvl_vehicles.py` (IDE MVL → `default.ide`/`handling.cfg`/`carcols.dat`) + `newVehicles.img` + **`newVehicles.col`** (`COLFILE` en `default.dat`/`gta_vc.dat`, `bootseed.list`): sin ese `.col` el motor **peta con assert** (`CalculateTrianglePlanes: model`) al spawnear los coches — visto en la sonda. Y `CBike` necesita sus IDs (assert `invalid bike model ID`). Su `newVehicles.ide` tiene **8 entradas y las 8 están dentro** (6500-6507); no hay IDs 6508+ en ese fichero, así que el bloque de datos está cerrado. Lo único que queda de vehículos es que **circulen** (§3 fila 10 = importados; el tráfico lo decide `vehClass`/frecuencia) |
| Mecánicas (autosave, esconderse de la poli, drive-by ampliado, IA, 1ª persona completa) | Son código, no dato. Referencias abiertas (SA `gta-reversed`, `plugin-sdk`) son **especificación**, no código pegable. Licencia de este repo = uso como spec. Lo barato y encendido en su ini ya está (§3 filas 11-12) |
| `main.scm` del mod | Ver §8.F: sin su código nativo los opcodes propios no tienen semántica (stub mudo = desync garantizado en los threads que los usen); se queda el de serie, que es el que da misiones funcionales |
| `ped.ifp` | Solo aporta de verdad cuando existan las mecánicas (nadar/escalar/apuntar). Hoy es cambio de *feel* y peso |
| `radio.txd` | **Pasa a obligatorio** si se usa el `fronten2.txd` del mod (§2.8). Como feature (radio nueva) no entra |
| `weaponSights.txd`, `newspapers.txd` | Arte de features que aquí no existen → **peso muerto** si se meten sueltos |
| `frontend_ds5/ps4/ps5/x1btns` (TXD) | Botones de mandos que el port no ofrece; sustituir los que sí usa sí procede |
| `neo/carTweakingTable.dat` etc. | Solo con el taller (excluido) |

### D. Entra con un arreglo pequeño y localizado

- **`helipad_strutT` (DXT2)** → 2 líneas en `d3d.cpp` (§2.7-bis).
- **11 logos de emisora** ausentes del `fronten2.txd` del mod → fusionar con su
  `radio.txd` (o cargar los dos).
- **`fe_arrows2/3/4`** ausentes de sus `frontend_ds*/x360/xone.txd` → recuperar
  esos 3 sprites del port o no sustituir esos 6 ficheros.

### E. Decisiones tomadas (19/09) y dato que sigue faltando

**Decidido:**

1. **Navegador sin S3TC → se acepta y se documenta.** Se meten los bloques del
   mod igual; el plan deja constancia de que sin la extensión
   (`webgl_compressed_texture_s3tc`) todas las texturas caen a RGBA8 y hay más
   presión de RAM. El port ya lo dice al arrancar:
   `[od] webgl s3tc=%d astc=%d (0 = decode por software)`
   (`gl3device.cpp:1936-1941`). Corrección honesta del cuadro §2.9: peds (84 MB)
   y objetos (67 MB) son el **techo si todo el bloque estuviera residente**; en
   uso real se streamea por modelo/TXD (peds ≈ 0,5 MB/TXD, objetos ≈ 3,7 MB/TXD).
   El único caso que se paga **de golpe** es `fronten2.txd`. Y no hay truco
   barato: peds/objetos **ya están en DXT**, no se pueden recomprimir menores.
2. **`fronten2.txd` → fusionar los 11 logos y usar el mapa HD del mod.** Es
   decir: se toman los 9 mapas de 1024² y los botones del mod **más** los 11
   logos de emisora que el mod movió a `models/radio.txd`, todo en **un solo**
   `fronten2.txd` (así el port no necesita cargar `radio.txd`, que no forma
   parte de su set). Coste asumido: 38 MB sin S3TC / 9,5 con (port: 3,2 / 0,75);
   el mapa del menú pasa a HD. Requiere escribir TXD (fusionar texturas de dos
   diccionarios) → herramienta a construir o Magic.TXD a mano.

**Sigue faltando un dato (no bloquea):**

3. **Paletizado poco ejercitado**: el port solo tiene 5 texturas PAL4/PAL8 (en
   `frontend_ds2.txd`, es decir el TXD de botones de mando). `weapons.img` trae
   **85** → conviene una prueba dirigida (arma en mano + captura) antes de
   sustituir ese bloque, y quedarse con la traza `IMG`/`SKIP` de `odtrace.log`.
   ⇒ **19/09: ya está en uso** (pack 3 metió `weapons.img` entera y la sonda
   `weapons-smoke-test.mjs` dispara con las 8 armas nuevas sin un solo
   `RuntimeError`, con 8/8 TXD cargados), así que las 85 paletizadas ya pasan
   por `TexRead` en partida. Se deja la nota por si algún día aparece banding.

### F. Medido el 19/09: `main.scm` y banco de audio (por qué no entran)

**`main.scm` del mod — descartado.** Medición, no intuición:

- Tamaños: **1.191.180 B** (mod) vs **1.269.133 B** (de serie, 12/03/2003).
  `cmp -l` da **1.088.070** bytes distintos: es otro build completo, no un
  parche encima.
- Patrón de opcode: el literal `A8 0F` (`0FA8`) aparece **11 veces** en el de
  serie y **22** en el del mod, y `A9 0F` (`0FA9`) **0** vs **1**. Ojo: contar
  bytes no prueba que sean *opcodes* (pueden caer dentro de datos), así que esa
  cifra es indicio, no prueba. Lo que sí es prueba: `grep 0FA8|4008` en
  `src/control/` = **0 resultados** → el motor no implementa ese comando y el
  mod no publica su semántica (vive en `ViceEx.exe`/sus ASI, que en wasm no
  corren). Sin saber **cuántos argumentos** tiene un opcode desconocido no se
  puede ni saltar el comando: un stub mudo desincroniza la pila del script.
- Coste/beneficio: lo que ese `main.scm` añade (tablones en Ammu-Nation, tienda
  de ropa, periódicos, GPS) depende de código nativo que no tenemos. Se queda el
  `main.scm` de serie: las misiones funcionan y reparten las armas de serie.
  Las armas/vehículos nuevos se prueban con los cheats `CRAZYTOOLS`/`CRAZYRIDES`.

**Banco `ViceEx.{SDT,RAW}` — no entra sin oído.** Medición:

- No es un banco completo: su `ViceEx.SDT` tiene **13 entradas** (20 B/entrada,
  sin tabla de nombres: el formato VC no la tiene). El vanilla tiene 9.941.
- Las 13 son pares de tamaño idéntico (0/1, 3/4, 5/6, 7/8, 9/10) → parejas
  L/R (estéreo), con duraciones de 0,10 s (×2), 0,10 s (×2), 0,32 s, 0,41 s,
  0,51 s (×2), 0,58 s, 1,00 s (×2) y 1,06 s (×2). Encajan con disparos/cortas,
  pero **no hay forma de saber cuál es de qué arma sin escucharlas** ni de
  dónde las llama su código.
- Qué se hizo en su lugar: las 9 armas nuevas **suenan con la muestra de su
  arma equivalente de serie** (documentado en `AudioLogic.cpp`), que es el
  criterio honesto mientras no haya mapeo audible.
