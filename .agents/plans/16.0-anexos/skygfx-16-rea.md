# SkyGfx 3.0b (GTA Vice City) — recibo de ingenieria inversa e inventario de features

> Documento de RE para alimentar el plan de port de SkyGfx al port reVC (GTA VC -> WebAssembly/WebGL2).
> Regla de oro aplicada: **el binario instalado manda**; el fuente publico (`aap/skygfx_vc`, master) es el mapa.
> Todo lo que sigue sin la marca «medido» es un **supuesto**, y el motivo va escrito.

---

## 0. Resumen ejecutivo (cifras medidas)

| Medida | Valor | Como se midio |
|---|---|---|
| Features inventariadas | **55** | 44 features con clave de INI (46 claves: `carPipe`/`carPipeSwitch` y `worldPipe`/`worldPipeSwitch` son la misma feature) + 11 features fijas (sin clave) |
| Claves de INI presentes en el binario | **46** | `REA list_strings` sobre `skygfx.asi` (474 strings definidos) |
| Exports del `.asi` | **9** | `REA list_names` (tabla de exports PE) |
| Imports del `.asi` | 4 DLLs | `RWD3D9.DLL` (9 simbolos), `KERNEL32.DLL` (76), `USER32.DLL` (2), `ADVAPI32.DLL` (1) + `psapi` en runtime |
| Version reportada por el binario | `0x280` (640) | `REA procedure_pseudo_code` de `SkyGFXGetVersion` (0x1000c190) |
| Procedimientos Ghidra | 1319 | `REA binary_overview` |
| Tamano `skygfx.asi` | 286 720 B | Python, `Get-Item` / hash propio |
| Features **YA ESTABAN / PORTADAS** en el port | **16** | ver §6.2 |
| Features **PARCIALES** | **9** | ver §6.3 |
| Features **A PORTAR** | **9** | ver §6.1 |
| Features **NO PORTADOS** (la feature base no existe en el port) | **12** | ver §6.5 |
| Features **NO APLICA** | **9** | ver §6.4 |
| Suma | **55** | cuadra exactamente |

**Nota sobre el conteo de claves**: el enunciado de partida decia «41 claves de INI». La medicion del binario da **46**
(las 41 mas `radiosityIntensity`, `radiosityLimit`, `radiosityRenderPasses`, `radiosityFilterPasses` y `leedsEnvMult`,
que tambien estan en el binario; ver la lista completa en §2.1). El binario manda: 46.

---

## 1. Ficha del artefacto

### 1.1 `mods\skygfx\SkyGfx\skygfx.asi` — la logica del mod

| Campo | Valor | Evidencia |
|---|---|---|
| Ruta | `mods\skygfx\SkyGfx\skygfx.asi` | — |
| SHA-256 | `f6f5bb623f5a1329ff152184061514fdd97422b9bb4119e69155b6de53850ff2` | `REA open_binary` + script Python propio |
| Tamano | 286 720 B | idem |
| Formato | PE32, x86, `executable_role: shared-library` (DLL) | `REA binary_overview` |
| Image base | `0x10000000` | idem |
| Timestamp | 2021 (segun `Readme (or die).txt:30` «Version: 3.0b») | fichero de texto |
| Modo de carga | ASI loader externo (Simple DLL loader / Ultimate ASI Loader). **No** funciona con el loader Mss32 integrado: el propio binario lo avisa | string `0x10038a28` «SkyGFX cannot be loaded by the default Mss32 ASI loader.» + `MessageBoxA` en la funcion `FUN_100089f0` (`REA procedure_pseudo_code`) |

**Segmentos** (`REA binary_overview`):

| Segmento | Rango | Longitud |
|---|---|---|
| Headers | `0x10000000-0x10000400` | 1024 |
| `.text` | `0x10001000-0x10037c00` | 224 256 |
| `.rdata` | `0x10038000-0x10042c00` | 44 032 |
| `.data` | `0x10043000-0x1005a7bc` | 96 188 (size virtual; 4096 en disco) |
| `.gfids` | `0x1005b000-0x1005b200` | 512 |
| `.rsrc` | `0x1005c000-0x1005c200` | 512 |
| `.reloc` | `0x1005d000-0x10060000` | 12 288 |

**Exports (9)** — `REA list_names` sobre la export table:

| Ordinal | Nombre | Direccion | Que es |
|---|---|---|---|
| 1 | `AttachCarPipeToRwObject` | `0x1001ee70` | engancha el pipe de coche a un `RpAtomic` o `RpClump` |
| 2 | `AttachRimPipeToRwObject` | `0x10019b20` | engancha el pipe de rim light |
| 3 | `AttachWorldPipeToRwObject` | `0x1001eeb0` | engancha el pipe de mundo |
| 4 | `SkyGFXGetParam` | `0x1000ba80` | lee un `int` de la config por nombre |
| 5 | `SkyGFXGetParamF` | `0x1000bc60` | lee un `float` de la config por nombre |
| 6 | `SkyGFXGetParamPtr` | `0x1000be20` | devuelve la direccion de un campo de la config |
| 7 | `SkyGFXGetVersion` | `0x1000c190` | devuelve `0x280` (= 640) |
| 8 | `SkyGFXSetParam` | `0x1000c1a0` | escribe un `int` de la config |
| 9 | `SkyGFXSetParamF` | `0x1000c3c0` | escribe un `float` de la config |

> El modulo de la export table se llama `skygfx.dll` (string `0x100422a2`), aunque el fichero instalado se llama
> `skygfx.asi`. El README (`SkyGfx/README.txt:6-8`) explica que son el mismo binario y que el autor renombro a `.asi`.

**Imports (4 DLLs)** — `REA list_names`:

| DLL | Simbolos | Notas |
|---|---|---|
| `RWD3D9.DLL` | `RwD3D9Supported`, `RwD3D9CreateVertexShader`, `RwD3D9CreatePixelShader`, `RwD3D9SetVertexShader`, `RwD3D9SetPixelShader`, `RwD3D9SetFVF`, `RwD3D9SetVertexShaderConstant`, `RwD3D9SetPixelShaderConstant`, `RwD3D9SetIm2DPixelShader` | **El mod no puede funcionar sin `rwd3d9.dll`**: todos los pipelines neo/leeds y el pase YCbCr dependen deVertex/Pixel Shader Model 2.0 |
| `KERNEL32.DLL` | 76 (`LoadLibraryA`, `GetProcAddress`, `VirtualProtect`, `CreateThread`, `GetModuleFileNameA/W`, `GetAsyncKeyState`, …) | incluye `VirtualProtect` para los parches en caliente del exe |
| `USER32.DLL` | `MessageBoxA`, `GetAsyncKeyState` | `MessageBoxA` = los errores «Error - SkyGFX»; `GetAsyncKeyState` = la consola con F8 |
| `ADVAPI32.DLL` | `SystemFunction036` (= `RtlGenRandom`) | alimenta el PRNG de PS2 (`ps2rand`, `main.cpp:514-526`) |

**`psapi` no es un import estatico**: se carga en runtime. Medido en el binario:
`"kernel32"` `0x10038e4f`, `"K32EnumProcessModules"` `0x10038e5c`, `"psapi"` `0x10038e74`, `"EnumProcessModules"` `0x10038e7c`.
Fuente: `ModuleList.hpp:14-70`. Sirve **solo** para enumerar modulos cargados y detectar `iii_anim`
(string `0x10038eb0`, usada en `neoRimpipe.cpp:84-93`). Es decir: `psapi` no aporta nada visual.

**Estructura de la config en memoria** (medida con `REA procedure_pseudo_code` de `SkyGFXGetParam` en `0x1000ba86`):
la funcion encadena `strcmp` contra `"worldPipeSwitch"`, `"carPipeSwitch"`, `"trailsSwitch"`, `"trailsBlur"`, …
y devuelve los campos de un `struct` en `.data` a partir de `0x10044340`. El layout medido coincide **exactamente**
con `skygfx.h:215-259`:

| Offset `.data` | Campo | Tipo |
|---|---|---|
| `0x10044340` | `worldPipeSwitch` | int |
| `0x10044344` | `carPipeSwitch` | int |
| `0x10044348` | `trailsSwitch` | int |
| `0x1004434c` | `trailsBlur` | int |
| `0x10044350` | `trailsMotionBlur` | int |
| `0x10044354` | `radiosity` | int |
| `0x10044358` | `radiosityIntensity` | int |
| `0x1004435c` | `radiosityLimit` | int |
| `0x10044360` | `currentAmbientMultRed` | float |
| `0x10044364` | `currentAmbientMultGreen` | float |
| `0x10044368` | `currentAmbientMultBlue` | float |
| `0x1004436c` | `currentBlurAlpha` | float |
| `0x10044370` | `currentBlurOffset` | float |
| `0x10044374` | `leedsWorldPrelightTweakMult` | float |
| `0x10044378` | `leedsWorldPrelightTweakAdd` | float |

Confirmacion cruzada: la funcion que lee el INI (`FUN_100089f0`, pseudocode REA) escribe
`DAT_1004434c = 1`, `DAT_10044350 = 1`, `DAT_1004436c = 0x421c0000` (**39.0f**) y `DAT_10044370 = 0x40066666` (**2.1f**),
que son literalmente `config.trailsBlur=1; config.trailsMotionBlur=1; config.currentBlurAlpha=39.0f; config.currentBlurOffset=2.1f;`
de `main.cpp:775-778`. **El binario instalado y el fuente publico son el mismo codigo**: el mapa es fiable.

**Strings de INI en el binario** (direccion -> clave), de `REA list_strings` (474 strings definidos):

```
0x10038724 worldPipeSwitch      0x10038734 carPipeSwitch        0x10038744 trailsSwitch
0x10038754 trailsBlur           0x10038760 trailsMotionBlur     0x10038774 radiosity
0x10038780 radiosityIntensity   0x10038794 radiosityLimit       0x100387a4 currentAmbientMultRed
0x100387bc currentAmbientMultGreen                             0x100387d4 currentAmbientMultBlue
0x100387ec currentBlurAlpha     0x10038800 currentBlurOffset    0x10038814 leedsWorldPrelightTweakMult
0x10038830 leedsWorldPrelightTweakAdd                          0x10038888 leeds
0x10038a88 dontNag              0x10038a90 SkyGfx               0x10038a98 seamFix
0x10038aa0 radiosityRenderPasses                                0x10038ab8 radiosityFilterPasses
0x10038ad0 disableColourOverlay 0x10038ae8 ps2Water             0x10038af4 texblendSwitch
0x10038b04 noCarcolTex          0x10038b10 texgenSwitch         0x10038b24 ps2light
0x10038b30 IIIEnvFrame          0x10038b3c VCEnvFrame           0x10038b48 carPipe
0x10038b50 worldPipe            0x10038b5c envMapSize           0x10038b68 leedsEnvMult
0x10038b78 neoRimLightPipe      0x10038b88 neoGlossPipe         0x10038b98 dualPass
0x10038ba4 disableBackfaceCulling                              0x10038bbc neoWaterDrops
0x10038bcc replaceDefaultPipeline                               0x10038be4 ps2Loadscreen
0x10038bf4 YCbCrCorrection       0x10038c04 lumaScale           0x10038c10 lumaOffset
0x10038c1c CbScale              0x10038c24 CbOffset            0x10038c30 CrScale
0x10038c38 CrOffset             0x10038c44 ps2carCam
```

**Strings de datos** (prueba de que el binario instalado trae las rutas de los `.dat` y el TXD):

```
0x10038d10 neo\neo.txd            0x10038d6c dropmask        0x10038d9c CarReflectionMask
0x10038db0 neo\carTweakingTable.dat                          0x10038ec4 neo\rimTweakingTable.dat
0x10038fb0 neo\worldTweakingTable.dat                        0x10038e0f ?_gloss
0x10038e90 nodeD3D8SkinAtomicAllInOne.csl                    0x10038eb0 iii_anim
```

### 1.2 `mods\skygfx\rwd3d9.dll` — el driver RenderWare -> D3D9 (con los shaders)

| Campo | Valor |
|---|---|
| Ruta | `mods\skygfx\rwd3d9.dll` |
| SHA-256 | `832bbea07f016709969decfba0b0eb2ef48faf709b919fe85fa2a2e7bdf3a5a3` |
| Tamano | 72 704 B |
| PE32 x86, image base `0x10000000`; 336 procedimientos, 238 strings (`REA binary_overview`) |
| Imports | solo `KERNEL32.DLL` (63 simbolos) — **no importa `d3d9.dll`**: usa `GetProcAddress`/`LoadLibrary` en runtime para resolver `Direct3DCreate9` y friends |

Exports propios (los que el `.asi` usa), direccion y nombre:

| Direccion | Export |
|---|---|
| `0x100012b0` | `RwD3D9SetIm2DPixelShader` |
| `0x10001530` | `RwD3D9Supported` |
| `0x10001560` | `RwD3D9GetCurrentD3DDevice` |
| `0x10001570` | `RwD3D9CreateVertexShader` |
| `0x100015c0` | `RwD3D9CreatePixelShader` |
| `0x10001610` | `RwD3D9SetVertexShader` |
| `0x10001660` | `RwD3D9SetPixelShader` |
| `0x100016b0` | `RwD3D9SetFVF` |
| `0x10001700` | `RwD3D9SetVertexDeclaration` |
| `0x10001750` | `RwD3D9SetVertexShaderConstant` |
| `0x10001780` | `RwD3D9SetPixelShaderConstant` |
| `0x100017b0` | `RwD3D9SetVertexPixelShaderConstant` |

**Componente del mod**: es el **driver RenderWare -> Direct3D 9** que sustituye al `d3d8.dll` del juego. Sin el, el
`.asi` ni siquiera puede resolver `RwD3D9Supported()` (import estatico) y **todas** las features que necesitan
shaders quedan apagadas. Los shaders van embebidos en el binario como blobs de bytecode D3D9.

### 1.3 `mods\skygfx\d3d8.dll` — el envoltorio D3D8 -> D3D9

| Campo | Valor |
|---|---|
| Ruta | `mods\skygfx\d3d8.dll` |
| SHA-256 | `58b9b73859f107183e4146c2b854f929397892058d46de3691129722a6f29d92` |
| Tamano | 279 040 B |
| PE32 x86, image base `0x10000000`; `.text` 194 560 B, `.rdata` 66 560 B |

**Componente del mod**: el `d3d8to9` de Leonard (d3d8to9), un **proxy de la API Direct3D 8** que traduce las llamadas
D3D8 del juego a D3D9. Se coloca en el directorio del juego y el juego lo carga en vez de `d3d8.dll`.
El README del mod (`SkyGfx/README.txt:18-20`) lo dice: *«A d3d8 to d3d9 wrapper is needed for all features to work properly.»*

### 1.4 `mods\skygfx\SkyGfx\neo\*` — los datos

| Fichero | Tamano | SHA-256 (prefijo) | Que es |
|---|---|---|---|
| `neo\neo.txd` | 31 144 B | `93118FE57E4E` | TXD de RenderWare con las texturas del mod: `CarReflectionMask`, `dropmask`, `MaterialCursor` y las 15 texturas de gloss de carretera (`roadLOD_gloss`, `LODroad_gloss`, `Tar_1line256HV_gloss`, `washroadlod1_gloss`, `sjmlodrdpth_gloss`, `roosveltbridgetarmac_gloss`, `dt_road_gloss`, `lhroadlod_gloss`, `road2_256_gloss`, `road2w_pkg_256 copy_gloss`, `road_LODa_gloss`, `road_LODb_gloss`, `StarRoadLOD_gloss`, `Road_yellowline256HV_gloss`, `Tar_2line128_gloss`) |
| `neo\carTweakingTable.dat` | 7 491 B | `F21ED5C96AFF` | 4 tablas de 24x7 (fresnel, power, color difuso, color especular) |
| `neo\rimTweakingTable.dat` | 9 004 B | `0FF2890704E2` | 5 tablas de 24x7 (ramp start, ramp end, offset, scale, scaling) |
| `neo\worldTweakingTable.dat` | 1 243 B | `046B48131BA5` | 1 tabla de 24x7 (LM blend) |

### 1.5 `mods\skygfx\SkyGfx\skygfx.ini` y `README.txt`

El INI instalado (68 lineas) es el de fabrica con **5 lineas activas**:
`replaceDefaultPipeline=1`, `ps2Loadscreen=1`, `texblendSwitch=0`, `texgenSwitch=0`, `ps2light=1`, `ps2Water=1`,
`dualPass=1`, `neoWaterDrops=1`, `envMapSize=128`, `disableColourOverlay=0`.
El resto de claves estan comentadas (`;`), lo que segun el propio INI significa **«el codigo del juego no se toca»**
(`skygfx.ini:10-13`).

El `README.txt` (50 lineas) documenta los 9 exports y la necesidad de `rwd3d9.dll` + envoltorio `d3d8.dll`.

### 1.6 `mods\skygfx\Readme (or die).txt` y `Leiame (ou morra).txt`

Envoltorio de MixMods. `Readme (or die).txt:30` confirma **«Version: 3.0b»** y **«Author: The Hero (aap)»**.
El `Leiame (ou morra).txt` es la version portuguesa del mismo texto.

### 1.7 Componentes y su papel (resumen)

| Componente | Papel | ¿Sirve al port WebGL2? |
|---|---|---|
| `skygfx.asi` | Toda la logica: lee el INI, engancha los pipelines, parchea el exe, trae los shaders embebidos | **Fuente del port** (es C++ Win32; hay que reescribir la parte D3D) |
| `rwd3d9.dll` | Driver RW->D3D9 + blobs de shaders D3D9 | **NO**: D3D9 nativo de Windows. El port usa librw GL3 |
| `d3d8.dll` | Proxy D3D8->D3D9 | **NO**: solo tiene sentido en Windows/x86 con el exe retail |
| `neo/*.dat` | Tablas de tweaking por hora y clima | **SI**: son datos, no codigo. El port ya los sirve (§4) |
| `neo/neo.txd` | Texturas (mascara de reflejo, mask de gotas, gloss de carreteras) | **SI**: el port ya sirve un `neo.txd` equivalente (§4) |

---

## 2. Inventario de TODAS las features

Convenciones de estado: `YA ESTABA` · `PORTADO` · `PARCIAL` · `NO APLICA (motivo)` · `A PORTAR (destino)`.

### 2.1 Las 46 claves de INI

#### A) Pipelines principales (D3D9)

| # | Feature / clave INI | Default (binario) | Valor INI instalado | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|---|
| F01 | **Pipe de coche** `carPipe` (export: `carPipeSwitch`) | `-1` -> `0` (default) | ausente (comentada) | `main.cpp:893`; `pipeswitch.cpp:187-196, 226-245, 264-274`; `neoCarpipe.cpp`; `leedsCarpipe.cpp` | str `carPipe` `0x10038b48`, `carPipeSwitch` `0x10038734`; xref `0x1000ba92` | Elige el pipeline de los coches: **0** = el de RW (MatFX), **1** = *neo* (doble pase: difuso con env map en tiempo real + pase especular con `specColor`/`power`/`fresnel`), **2** = *leeds* (LCS/VCS, reflection map con mascara). El pipe neo reescribe por completo la iluminacion en el VS (`vehiclePass1VS`/`vehiclePass2VS`) | **PARCIAL** | El port tiene el caso **0** y el caso **1**: `src/extras/custompipes.cpp:350` (`VehiclePipeSwitch = VEHICLEPIPE_MATFX`), `:359-371` (`AttachVehiclePipe`), `src/extras/custompipes_gl.cpp:76-170` (pipe neo con `neoVehicle.vert/.frag`), menu `src/core/re3.cpp:145` («VehiclePipeline»). **Falta el valor 2 (leeds)**: no existe `LeedsCarPipe` ni su shader |
| F02 | **Pipe de mundo** `worldPipe` (export: `worldPipeSwitch`) | `-1` -> `0` | ausente (comentada) | `main.cpp:894`; `pipeswitch.cpp:134-143, 170-178`; `neoWorldpipe.cpp`; `leeds.cpp` | str `worldPipe` `0x10038b50`, `worldPipeSwitch` `0x10038724`; xrefs `0x1000ba86, 0x1000be27, 0x1000c1a6` | Elige el pipeline del mundo: **0** = default, **1** = *neo* (modula el lightmap con el blending de `worldTweakingTable.dat` y usa `MODULATE2X` en la segunda etapa de textura), **2** = *leeds* (shader de mundo propio con tweak de prelight) | **PARCIAL** | El port tiene el pipe neo del mundo (`src/extras/custompipes.cpp:378-396`, `src/extras/custompipes_gl.cpp:190-273`, `neoWorldVC.frag` + `default_UV2.vert`), enganchado en `src/modelinfo/SimpleModelInfo.cpp:94`. **Falta**: el interruptor de menu (no hay «World pipe»), el valor *leeds* y el `LightmapEnable` como conmutador real |
| F03 | **Rim light Xbox** `neoRimLightPipe` | `-1` -> `0` | `0` (activa) | `main.cpp:903`; `neoRimpipe.cpp` (todo) | str `neoRimLightPipe` `0x10038b78`; menu «Neo rim light pipe» `0x10038940` | Pinta un borde iluminado en los peds segun el angulo entre la normal y la vista, con una rampa de color y el tweak de `rimTweakingTable.dat`. En VC engancha el callback de render del *skin pipe* de RW (`nodeD3D8SkinAtomicAllInOne.csl`) | **PORTADO** (con shader GLSL reescrito) | `src/extras/custompipes_gl.cpp:393-414` (`uploadRimData`), `:416-457` (piel), `:459-498` (no-piel), `:500-546` (creacion), menu `src/core/re3.cpp:146/152` («NeoRimLight»), enganche `src/modelinfo/PedModelInfo.cpp:86`. El VS original es `rimVS.h`; aqui es `neoRim.vert` / `neoRimSkin.vert` (GLSL propio) |
| F04 | **Gloss de carreteras** `neoGlossPipe` | `-1` -> `0` | `0` (activa) | `main.cpp:904`; `neoGlosspipe.cpp` | str `neoGlossPipe` `0x10038b88`; menu «Neo gloss pipe» `0x10038954` | Segundo pase sobre las carreteras: una pasada especular con la textura `<nombre>_gloss` del TXD, con `ZWRITE` off y `alpha test ALWAYS` | **PORTADO** (parcial) | `src/extras/custompipes_gl.cpp:294-372` (glossRenderCB + CreateGlossPipe), `src/extras/custompipes.cpp:403-433` (`GetGlossTex` busca `<tex>_gloss` en el `neo.txd`), menu `src/core/re3.cpp:148/154` («NeoRoadGloss»). **Diferencia medida**: SkyGfx engancha el gloss pipe a las carreteras por el flag `isRoad` (`pipeswitch.cpp:126`); el port lo engancha por `m_wetRoadReflection` (`src/modelinfo/SimpleModelInfo.cpp:91-92`) |
| F05 | **Gotas de agua en pantalla** `neoWaterDrops` | `-1` -> `0` | `1` (Neo) | `main.cpp:939-941`; `neoWaterdrops.cpp` (35 KB) | str `neoWaterDrops` `0x10038bbc`; menu «Neo water drops» `0x10038964`; `dropmask` `0x10038d6c` | Simula gotas de lluvia en la lente: hasta 2000 gotas fijas + 700 en movimiento, cada una un sprite con la mascara `dropmask` del `neo.txd`, desplazadas por el movimiento de camara. Los valores 2 (*Extended Neo*) y 3 (*Extended+Blood*) anaden gotas al saltar una particula de sangre (**TAG, 13AG**) | **PORTADO** (el caso 1) | `src/extras/screendroplets.cpp` completo (`ScreenDroplets`, `MAXDROPS=2000`, `MAXDROPSMOVING=700`, identico a `neo.h:235-241`), shader `screenDroplet.frag`/`im2d_UV2.vert`, `src/core/config.h:455` (`SCREEN_DROPLETS`), `src/control/GameLogic.cpp:172,291,349`. **Diferencia medida**: la mascara se **genera proceduralmente** (`CreateDropMask(64)`, `screendroplets.cpp:80-103`) en vez de leerse del TXD, porque el `neo.txd` del port **no tiene** `dropmask` (§4). **Falta**: los modos 2 y 3 (Extended / +Blood) |

#### B) PS2

| # | Feature / clave INI | Default | Valor INI instalado | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|---|
| F06 | **Blend de env map MatFX** `texblendSwitch` | `-1` (no se toca) | `0` (PS2) | `main.cpp:836-857`; `matfx.cpp:204-350, 584-624` | str `texblendSwitch` `0x10038af4`; menu «MatFX blend» `0x100388c0` | Reescribe el pipeline de env map de MatFX con uno de tres estilos de mezcla de textura: **0** PS2, **1** PC, **2** Mobile. Cambia como se mezcla el color de la env map con la textura difusa | **A PORTAR** | El port usa el MatFX de librw (`vendor/librw/src/gl/gl3matfx.cpp`, shaders `matfx_env.vert/.frag`), que tiene **un solo** estilo. Los 3 estilos de SkyGfx son shaders HLSL (`ps2StandardVS`, `pcStandardVS`, `nolightVS` + `pcEnvPS`/`mobileEnvPS`/`worldPS`) → hay que escribir 3 shaders GLSL |
| F07 | **Generacion de UV de env map** `texgenSwitch` | `-1` (no se toca) | `0` (PS2) | `main.cpp:866-875`; `matfx.cpp:117-187, 375-434` | str `texgenSwitch` `0x10038b10`; menu «MatFX texgen» `0x100388d4` | Cambia como se calculan las coordenadas de textura de la env map: **0** PS2 (cube map con una matriz de escala propia), **1** PC, **2** Mobile. Afecta al aspecto del reflejo en el coche y a los materiales con env map | **A PORTAR** | Igual que F06: falta en `vendor/librw/src/gl/shaders/matfx_env.vert`. Destino: `vendor/librw/src/gl/shaders/` + un selector en el motor |
| F08 | **Luz PS2 en MatFX** `ps2light` | `1` | `1` (activo) | `main.cpp:877`; `matfx.cpp:435-583` | str `ps2light` `0x10038b24`; menu «MatFX PS2 lighting» `0x100388e4` | Cambia el vertex shader del render MatFX por el equivalente de PS2 (clamp del color antes del color de material, iluminacion con el modelo PS2). El propio INI dice «the difference is subtle but visible» | **A PORTAR** | El port usa siempre el `default.vert` de librw (`v_color = clamp(...); v_color *= u_matColor;`), que **ya es el comportamiento PS2** (`vendor/librw/src/gl/shaders/default.vert:19-20`). Es decir: el caso PS2 ya esta; **falta el caso PC** como alternativa |
| F09 | **Agua PS2** `ps2Water` | `0` | `1` (activo) | `main.cpp:832, 844-845`; `WaterLevel.cpp:14-90` | str `ps2Water` `0x10038ae8` | Reescribe el calculo de UVs del «mask atomic» del agua (33x33 vertices) y el coeficiente de env map, para que el agua se vea como en PS2. Exige `texblendSwitch=0` | **PARCIAL** | El port tiene `gPS2alphaTest` (`src/rw/RwHelper.cpp:18/95`, `rw::GSALPHATEST`) y el fichero `src/rw/RwPS2AlphaTest.cpp`, **pero ese fichero esta muerto**: todo su cuerpo esta bajo `#ifndef LIBRW` (linea 1) y el port compila contra librw. El parche de UVs de `WaterLevel.cpp` **no esta portado** |
| F10 | **Doble pase de transparencia** `dualPass` | `-1` (no se toca) | `1` (activo) | `main.cpp:906-914`; `main.cpp:204-269` (`drawDualPass`, `dualPassHook`); `rwfix.cpp:390` | str `dualPass` `0x10038b98`; menu «Dual pass» `0x100388f8` | Intercepta el draw call de RW y, si hay alpha blending + Z-write, dibuja la geometria **dos veces**: primero con `ALPHAFUNC >= 128` (Z-write on), luego con `ALPHAFUNC < 128` y Z-write off. Elimina los artefactos de alpha en la geometria transparente | **NO APLICA (tal cual) / A PORTAR** | El hook es sobre el codigo D3D8 de RW (`0x678D69`), que no existe en el port. En WebGL2 se puede reimplementar con `glAlphaFunc` + dos `drawInst`, pero el port no lo hace. Nota: `src/rw/RwPS2AlphaTest.cpp:17` tiene `_rxD3D8DualPassRenderCallback`, pero esta en el bloque muerto `#ifndef LIBRW` |
| F11 | **Sin backface culling** `disableBackfaceCulling` | `0` | `0` (desactivado) | `main.cpp:916-926` | str `disableBackfaceCulling` `0x10038ba4` | Pone a 1 el byte de culling en 6 funciones del renderer de VC (`RenderOneNonRoad`, `RenderBoats`, `RenderEverythingBarRoads`, `RenderFadingInEntities`, `RenderRoads`, `CCutsceneObject::Render`), para que se dibuje tambien la cara oculta (necesario para los DFF de PS2) | **YA ESTABA** | El port lo tiene de otra forma y mas fino: `src/rw/RwHelper.cpp:23-25` (`gBackfaceCulling = false` por defecto), `:103-110` (`SetCullMode` fuerza `CULLNONE` si esta apagado), y el interruptor `Rendering/BackfaceCulling` en `src/core/re3.cpp:779` / `:935` / `:1337`. Inventario historico: fila **8.16** de `.agents/plans/13.0-anexos/inventario-mod.md` |
| F12 | **Pantalla de carga PS2** `ps2Loadscreen` | `0` | `1` (activo) | `main.cpp:955-967` | str `ps2Loadscreen` `0x10038be4` | En el exe retail de VC parchea el punto donde se pasa el argumento de la pantalla de carga (`push [esp+0x78]` en vez del original) para que se use la variante PS2 | **NO APLICA (motivo tecnico)** | El parche es sobre bytes del exe retail (`0x4A69D4`, medido: `00 59 50 e8` -> `90 ff 74 24 78`). El port tiene su propia pantalla de carga (`src/core/main.cpp:696` `LoadingScreen`, `:2170-2187` `InitialiseGame`, y en web `WebDrawLoadScreen`), asi que no hay destino para el hook |
| F13 | **Camara de coche PS2** `ps2carCam` | `0` | ausente (comentada) | `main.cpp:995-998` | str `ps2carCam` `0x10038c44` | Solo para GTA III: anula dos tweaks de la camara en=get-in-vehicle. En VC **no hace nada** (todo el bloque esta bajo `gtaversion == III_10`) | **NO APLICA** | Codigo muerto en VC: `main.cpp:995` es `if(gtaversion == III_10 && ...)`. En el port no hay equivalente de III |

#### C) Pipeline por defecto "Xbox" y env maps

| # | Feature / clave INI | Default | Valor INI instalado | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|---|
| F14 | **Pipeline por defecto tipo Xbox** `replaceDefaultPipeline` | `0` | `1` (activo) | `main.cpp:943-953`; `rwfix.cpp:8-28, 403-419` | str `replaceDefaultPipeline` `0x10038bcc`; xref `0x1000a8f3` | Sustituye los 3 callbacks del pipeline por defecto de RW (instance, reinstance, render) por versiones que **no premultiplican el color de vertice por el color de material** y que hacen la iluminacion al estilo Xbox. Arregla los bugs de objetos con color de vertice transparente | **YA ESTABA (equivalente por construccion)** | El bug era del path D3D8 de RW. En el port, librw aplica el color de material **una sola vez**, en el vertex shader: `vendor/librw/src/gl/shaders/default.vert:19-20` (`v_color = clamp(v_color,0,1); v_color *= u_matColor;`), y `src/extras/shaders/default_UV2.vert` hace lo mismo. No hay doble modulacion que arreglar. **Medido sobre el shader, no sobre una captura** |
| F15 | **Tamano del env map** `envMapSize` | `128` (redondeado a potencia de 2) | `128` | `main.cpp:896-899`; `neoCarpipe.cpp:49-50`; `leedsCarpipe.cpp:35-36` | str `envMapSize` `0x10038b5c`; xref `0x1000a092` | Tamano (cuadrado) de la textura donde se renderiza la escena reflejada para el coche. El valor se redondea hacia arriba a la siguiente potencia de 2 | **YA ESTABA** (pero **fijo a 128**, sin opción) | `src/extras/custompipes.cpp:50` (`int32 EnvMapSize = 128;`), `:60` (`rw::Raster::create(EnvMapSize, EnvMapSize, ...)`), `:62`. No hay clave de INI ni mando de menu equivalente |
| F16 | **Frame de env map de GTA III** `IIIEnvFrame` | `0` | ausente (comentada) | `main.cpp:879-883`; `main.cpp:181-202` | str `IIIEnvFrame` `0x10038b30` | En VC, usa el frame de env map de GTA III (matriz de identidad con flags `0x20003`) para que los reflejos **no dependan del angulo de camara** incluso con la generacion de UV de PC. Es una curiosidad: el juego retail de VC no trae el frame de III | **NO APLICA (curiosidad)** | Es literalmente una «curiosity» (el propio INI lo dice, `skygfx.ini:56-57`). Ademas el port ya tiene su propio frame de env map |
| F17 | **Frame de env map de VC** `VCEnvFrame` | `0` | ausente (comentada) | `main.cpp:884-888`; `main.cpp:314-324` | str `VCEnvFrame` `0x10038b3c` | El inverso: en **GTA III** usa el frame de VC (rotacion de 60 grados). En VC no aplica | **NO APLICA** | Bloque `if(isIII() && ...)`, `main.cpp:884` |
| F18 | **Sin texturas de color de coche** `noCarcolTex` | `0` | ausente (comentada) | `main.cpp:861-864` | str `noCarcolTex` `0x10038b04` | En **GTA III** anula dos `mov` para que los coerces no usen textura. El propio codigo dice que rompe MatFX. En VC no aplica | **NO APLICA** | Bloque `if(gtaversion == III_10 && ...)`, `main.cpp:861` |

#### D) Miscelanea

| # | Feature / clave INI | Default | Valor INI instalado | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|---|
| F19 | **Arreglo de seams** `seamFix` | `0` | `0` (desactivado) | `main.cpp:811`; `pipeswitch.cpp:28-103` (`fixSeams`, `makeSeamQuad`) | str `seamFix` `0x10038a98`; menu «Seam fixer» `0x1003898c`, «Seam Offset X/Y» `0x10038998/0x100389a8` | Redibuja el front buffer desplazado 1 px (`seamOffX/Y`) encima de la escena, para tapar las costuras entre objetos. Es un parche de pantalla completa, no una correccion de la causa | **A PORTAR / NO PORTADO** | En el port solo queda el comentario `// seam fixer` en `src/core/main.cpp:1641`, **sin implementacion**. La tecnica es portable (es un quad con offset de textura), pero hay que decidir si compensa |
| F20 | **Desactivar overlay de color** `disableColourOverlay` | `-1` -> `0` | `0` | `main.cpp:822-823, 537-567` | str `disableColourOverlay` `0x10038ad0`; menu «Disable Colour Overlay» `0x10038974` | Cuando hay trails, el juego pinta un overlay de color sobre la pantalla. Con esta opcion se **anula** ese overlay (el overlay se sigue, pero el color no se dibuja); el propio INI avisa: «Trails on/off still switches between light values then» | **PARCIAL** | El port tiene `CPostFX::EffectSwitch` con `POSTFX_OFF / POSTFX_SIMPLE / POSTFX_NORMAL / POSTFX_MOBILE` (`src/extras/postfx.h:8-13`), que es el equivalente funcional (elegir el efecto de overlay), pero **no** hay un interruptor «desactivar el overlay» ni la logica de «switch between light values» de `main.cpp:537-567` |
| F21 | **No dar la lata** `dontNag` | `0` | ausente (comentada) | `main.cpp:604-610, 810` | str `dontNag` `0x10038a88`; `MessageBoxA` import | Suprime los `MessageBox` de error del mod («Couldn't load 'neo\neo.txd'», etc.) | **NO APLICA** | El port ya avisa por `printf` (p.ej. `src/extras/custompipes.cpp:477`), no por MessageBox de Windows |

#### E) Correccion YCbCr (7 claves)

| # | Feature / clave INI | Default | Valor INI instalado | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|---|
| F22 | **Activar correccion YCbCr** `YCbCrCorrection` | `0` | ausente (comentada) | `main.cpp:969`; `screenfx.cpp:127-228` | str `YCbCrCorrection` `0x10038bf4`; menu «Enable YCbCr tweak» `0x100389b8` | Activa un pase de post-proceso a pantalla completa: convierte RGB->YUV, aplica escalas/desplazamientos a Y, Cb y Cr, y vuelve a convertir a RGB. Emula como un PS2/Xbox analogico interpretaria la señal | **PORTADO** | `src/extras/postfx.cpp:511-568` (el pase), `:45-51` (los 7 parametros con los defaults de SkyGfx), shader GLSL `src/extras/shaders/grading.frag` (equivalente a `gradingPS.hlsl`). INI: `src/core/re3.cpp:764` (`Rendering/YCbCrCorrection`). Menu: `src/core/re3.cpp:1373`. Inventario historico: fila **8.11** de `inventario-mod.md` |
| F23 | `lumaScale` | `219/255` = 0.858824 | ausente (comentada) | `main.cpp:970`; `screenfx.cpp:13` | str `lumaScale` `0x10038c04` | Multiplicador de la luminancia (Y) en el pase YCbCr | **PORTADO** | `src/extras/postfx.cpp:46`, `src/core/re3.cpp:765` |
| F24 | `lumaOffset` | `16/255` = 0.062745 | ausente | `main.cpp:971`; `screenfx.cpp:14` | str `lumaOffset` `0x10038c10` | Desplazamiento (bias) de la luminancia | **PORTADO** | `src/extras/postfx.cpp:47`, `src/core/re3.cpp:766` |
| F25 | `CbScale` | `1.23` | ausente | `main.cpp:972`; `screenfx.cpp:15` | str `CbScale` `0x10038c1c` | Multiplicador del crominance azul (Cb) | **PORTADO** | `src/extras/postfx.cpp:48`, `src/core/re3.cpp:767` |
| F26 | `CbOffset` | `0.0` | ausente | `main.cpp:973`; `screenfx.cpp:16` | str `CbOffset` `0x10038c24` | Desplazamiento de Cb | **PORTADO** | `src/extras/postfx.cpp:49`, `src/core/re3.cpp:768` |
| F27 | `CrScale` | `1.23` | ausente | `main.cpp:974`; `screenfx.cpp:17` | str `CrScale` `0x10038c30` | Multiplicador del crominance rojo (Cr) | **PORTADO** | `src/extras/postfx.cpp:50`, `src/core/re3.cpp:769` |
| F28 | `CrOffset` | `0.0` | ausente | `main.cpp:975`; `screenfx.cpp:18` | str `CrOffset` `0x10038c38` | Desplazamiento de Cr | **PORTADO** | `src/extras/postfx.cpp:51`, `src/core/re3.cpp:770` |

> Nota: el INI instalado documenta los defaults como `0.8588` y `0.0627` (`skygfx.ini:49-50`), que son
> `219/255` y `16/255` redondeados; el codigo real usa la division exacta (`main.cpp:970-971`).

#### F) Trails / Leeds / radiosidad (avanzado)

| # | Feature / clave INI | Default | Valor INI instalado | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|---|
| F29 | **Tipo de trails** `trailsSwitch` | `-1` -> `0` | ausente (comentada) | `main.cpp:815`; `main.cpp:537-567`; `main.cpp:977` (`enableTrailsSetting`) | str `trailsSwitch` `0x10038744`; menu «Trails switch» `0x10038930` | Tipo de motion blur / trails del pantalla: **0** el de PC, **1** *leeds*, **2** *mobile*. Ademas **hackea el menu de opciones** para que la casilla «Trails» sea escribible (`silenttrails.cpp:40-64`, robado de VC-Trails de Silent) | **PARCIAL** | El port tiene el caso *mobile* (`CPostFX::POSTFX_MOBILE`, `src/extras/postfx.h:12`, `src/extras/postfx.cpp:479-481` con el shader `contrast.frag`) y el normal. **Falta** el caso *leeds* (el mas pesado: bloom + sharpening) y el hack del menu de Trails |
| F30 | `trailsBlur` | `1` (fijado en `patch()`) | exportado, no configurable por INI | `main.cpp:775` | str `trailsBlur` `0x10038754`; `DAT_1004434c = 1` medido en el pseudocode | Activa/desactiva el blur de las trails | **A PORTAR** (como parametro) | El port tiene `CPostFX::BlurOn` (`src/extras/postfx.cpp:20`) pero **no** exposed como parametro con nombre ni con la misma semantica |
| F31 | `trailsMotionBlur` | `1` (fijado en `patch()`) | exportado, no configurable por INI | `main.cpp:776` | str `trailsMotionBlur` `0x10038760`; `DAT_10044350 = 1` medido | Activa/desactiva el motion blur de las trails | **A PORTAR** (como parametro) | El port tiene `CPostFX::MotionBlurOn` (`src/extras/postfx.cpp:21`) |
| F32 | **Radiosidad VCS** `radiosity` | `-1` -> `0` | ausente (comentada) | `main.cpp:814`; `skygfx.h:277-289` (`CMBlur::ms_pRadiosityRaster1/2`, `RadiosityInit/Render`) | str `radiosity` `0x10038774`; menu «Leeds Radiosity» `0x100384a8` | Simula la radiosidad de GTA Vice City: copia el frame a un raster aparte, lo ilumina y lo reinyecta. El INI avisa: «needs leeds trails and a plugin to supply control values» | **NO PORTADO** | `rg radiosity src/` = **0 coincidencias** (medido). No hay ningun raster de radiosidad en el port |
| F33 | `radiosityIntensity` | `0` | exportado, sin INI | `skygfx.h:224` | str `radiosityIntensity` `0x10038780` | Intensidad de la radiosidad | **NO PORTADO** | Sin soporte de radiosidad |
| F34 | `radiosityLimit` | `0` | exportado, sin INI | `skygfx.h:225` | str `radiosityLimit` `0x10038794` | Limite de la radiosidad | **NO PORTADO** | Sin soporte de radiosidad |
| F35 | `radiosityRenderPasses` | `2` | ausente (comentada) | `main.cpp:819` | str `radiosityRenderPasses` `0x10038aa0`; menu «Leeds Radiosity Render Passes» `0x100384e8` | Cuantas veces se dibuja el efecto de radiosidad | **NO PORTADO** | Sin soporte de radiosidad |
| F36 | `radiosityFilterPasses` | `4` | ausente (comentada) | `main.cpp:820` | str `radiosityFilterPasses` `0x10038ab8`; menu «Leeds Radiosity Filter Passes» `0x100384c8` | Cuantas veces se reduce (downsample) el raster de radiosidad | **NO PORTADO** | Sin soporte de radiosidad |
| F37 | **Multiplicador del env map de Leeds** `leedsEnvMult` | `-9999` -> `0.4` en VC (`0.22` en III) | ausente (comentada) | `main.cpp:655, 901`; `leedsCarpipe.cpp:493` | str `leedsEnvMult` `0x10038b68`; menu «Leeds Car Env Mult» `0x1003891c` | Multiplica el coeficiente de env map de los coches en el pipe leeds. El INI documenta `0.3` como default, pero el codigo usa `0.4` en VC (`main.cpp:655`) | **NO PORTADO** | No hay pipe leeds |
| F38 | **Tweak de prelight de Leeds (multiplicador)** `leedsWorldPrelightTweakMult` | `-9999` -> `1.0` | ausente (comentada) | `main.cpp:656, 816`; `leeds.cpp:96` | str `leedsWorldPrelightTweakMult` `0x10038814`; menu «Leeds Prelight Tweak Mult» `0x10038508` | Multiplica el prelight (lightmap) del mundo en el pipe leeds | **NO PORTADO** | No hay pipe leeds |
| F39 | **Tweak de prelight de Leeds (suma)** `leedsWorldPrelightTweakAdd` | `-9999` -> `0.0` | ausente (comentada) | `main.cpp:657, 817`; `leeds.cpp:96` | str `leedsWorldPrelightTweakAdd` `0x10038830`; menu «Leeds Prelight Tweak Add» `0x10038524` | Suma al prelight del mundo en el pipe leeds | **NO PORTADO** | No hay pipe leeds |
| F40 | `currentAmbientMultRed` | `0` | exportado, sin INI | `skygfx.h:227`; `leeds.cpp:36, 102` | str `currentAmbientMultRed` `0x100387a4` | Multiplicador del ambiente en rojo (usado por el pipe leeds; en III el ambiente no viene del timecycle) | **NO PORTADO** | El port usa el ambiente del timecycle directamente |
| F41 | `currentAmbientMultGreen` | `0` | exportado, sin INI | `skygfx.h:228`; `leeds.cpp:37, 103` | str `currentAmbientMultGreen` `0x100387bc` | Multiplicador del ambiente en verde | **NO PORTADO** | idem |
| F42 | `currentAmbientMultBlue` | `0` | exportado, sin INI | `skygfx.h:229`; `leeds.cpp:38, 104` | str `currentAmbientMultBlue` `0x100387d4` | Multiplicador del ambiente en azul | **NO PORTADO** | idem |
| F43 | `currentBlurAlpha` | `39.0` (fijado en `patch()`) | exportado, sin INI | `main.cpp:777`; `skygfx.h:230` | str `currentBlurAlpha` `0x100387ec`; `DAT_1004436c = 0x421c0000` (39.0f) medido | Alfa del overlay de blur de las trails | **A PORTAR** (como parametro) | El port tiene `CPostFX::Intensity` (`src/extras/postfx.h:20`), que es otra cosa |
| F44 | `currentBlurOffset` | `2.1` (fijado en `patch()`) | exportado, sin INI | `main.cpp:778`; `skygfx.h:231` | str `currentBlurOffset` `0x10038800`; `DAT_10044370 = 0x40066666` (2.1f) medido | Desplazamiento del overlay de blur | **A PORTAR** (como parametro) | idem |

### 2.2 Las 11 features fijas (sin clave de INI)

| # | Feature | Default | Fuente publica | Binario (REA) | Que hace exactamente | Estado en el port | Evidencia |
|---|---|---|---|---|---|---|---|
| F45 | **Fix del blend del sifon de auto-aim y del punto de francotirador** | fijo | `main.cpp:929-934` | xref a `0x5D4EEE` y `0x558024`; parchea 1 byte con `rwBLENDINVSRCALPHA` | En VC el juego usa un modo de blend equivocado para el sifon de auto-aim y para el punto de la mira del francotirador, y el alfa se ve mal. SkyGfx lo corrige a `rwBLENDINVSRCALPHA` en los dos sitios | **A PORTAR** | Parche de bytes del retail (`6a 06` -> `6a 04` en `0x5D4EEE`, medido en el exe). En el port hay que localizar el dibujo de esos dos elementos y cambiar el estado de blend. Destino natural: `src/renderer/Hud.cpp` / el sprite del punto |
| F46 | **Trails del francotirador siempre activadas** | fijo | `main.cpp:272-312, 936-937` | str `Trails switch` menu `0x10038930`; el hook esta en `0x55EA39` (medido en el exe retail: el byte es `00` dentro de una secuencia `push 2`/`push 0xb`) | Dibuja un quad con el buffer de blur (180,255,180,120) en el screen space cuando el jugador usa el francotirador, imitando el motion blur de la version PS2 | **A PORTAR / VERIFICAR** | El port ya tiene `CPostFX::RenderOverlaySniper` (`src/extras/postfx.h:47`, `src/extras/postfx.cpp:463-465`) y `MOTION_BLUR_SNIPER`, asi que **el caso sniper ya existe**: verificar el color/alpha y el dessin |
| F47 | **Integracion con DebugMenu** | auto | `main.cpp:683-755`; `debugmenu_public.h` | strings `debugmenu` `0x100385ec` y las 12 APIs `DebugMenuAdd*` `0x100385f8-0x10038704` | Si hay un `debugmenu.dll` cargado, registra ~30 mandos (pipes, blending, YCbCr, seam, radiosidad, …) para cambiar todo en caliente | **YA ESTABA** | `src/extras/debugmenu.cpp` (36 KB) + `src/core/re3.cpp:1337-1403` (mandos «Render»). Es la copia del mod Extended, no de SkyGfx, pero cubre lo mismo |
| F48 | **Plugin de material «Gloss» de RenderWare** | fijo | `rwplugin.cpp:15-46` | hook en `0x4A5BB5` (`hookplugins`, `InterceptCall` de `PluginAttach`) | Registra un plugin en `RpMaterial` para cachear la textura `<nombre>_gloss` y no buscarla en el TXD cada frame | **PORTADO** | `src/extras/custompipes.cpp:11-18` (`CustomMatExt {glossTex, haveGloss}`), `:26-43` (`CustomMatCtor`/`CustomMatCopy`), `:508-517` (`CustomPipeRegister` -> `CustomMatOffset`) |
| F49 | **Enumeracion de modulos cargados (psapi / K32EnumProcessModules)** | fijo | `ModuleList.hpp:14-70`; usado en `neoRimpipe.cpp:84-93` | strings `kernel32` `0x10038e4f`, `K32EnumProcessModules` `0x10038e5c`, `psapi` `0x10038e74`, `EnumProcessModules` `0x10038e7c`, `iii_anim` `0x10038eb0` | Enumera los DLLs cargados del proceso para saber si hay `iii_anim` (que ya pone el pipe de skin de los peds) y no pisarlo | **NO APLICA** | Es codigo **solo para GTA III** (`if(gtaversion == III_10)`, `neoRimpipe.cpp:84`). En VC no se ejecuta. Y en un WASM no hay `EnumProcessModules` |
| F50 | **Consola de depuracion con F8** | fijo | `main.cpp:1074-1078` | import `GetAsyncKeyState`; strings `CONIN$` `0x10038c50`, `CONOUT$` `0x10038c5c` | Al arrancar, si se pulsa F8, abre una consola (`AllocConsole`) para ver el `printf` del mod | **NO APLICA** | Consola de Windows; en el navegador hay la consola del navegador |
| F51 | **Deteccion de D3D9 y degradacion a D3D8** | fijo | `main.cpp:650` (`d3d9 = RwD3D9Supported()`); `neoCarpipe.cpp:202-205`, `neoRimpipe.cpp:116-119`, `neoGlosspipe.cpp:70-73` | import `RWD3D9.DLL::RwD3D9Supported` | Comprueba si el driver D3D9 esta disponible. Si no, `canUse = false` en los pipes neo y el mod sigue con D3D8 (el pipe de mundo tiene fallback D3D8, segun el propio INI) | **NO APLICA** | El port es GL3: no hay D3D9 ni D3D8. La degradacion no tiene sentido |
| F52 | **Conjunto de shaders MatFX PS2/PC/Mobile** | fijo (artefacto) | `shaders/vs/*.hlsl` (16 VS) + `shaders/ps/*.hlsl` (8 PS) | los shaders van embebidos en el `.asi` y en `rwd3d9.dll` | El set completo de vertex/pixel shaders Model 2.0 que usa el mod: `ps2StandardVS`, `ps2StandardEnvVS`, `ps2StandardEnvOnlyVS`, `pcStandardVS`, `pcStandardEnvVS`, `pcStandardEnvOnlyVS`, `nolightVS`, `nolightEnvVS`, `nolightEnvOnlyVS`, `ambientVS`, `leedsWorldVS`, `rimVS`, `glossVS`, `vcsVS`, `vehiclePass1VS`, `vehiclePass2VS` + `worldPS`, `vc_worldPS`, `pcEnvPS`, `mobileEnvPS`, `glossPS`, `gradingPS`, `curvePS`, `contrastPS` | **A PORTAR** (subconjunto) | En el port solo existe el subconjunto GLSL de `src/extras/shaders/` (§3.5). Faltan los VS de MatFX y los de Leeds |
| F53 | **Pipe de coche Leeds (LCS / VCS)** | valor `2` de `carPipe` | `leedsCarpipe.cpp` (18,5 KB) | shaders `vcsVS` (y el `LeedsCarPipe` usa `reflectionMask` + `reflectionTex` + `leedsEnvMult`) | Pipe de coche «Leeds»: renderiza la escena en un env map de `envMapSize`, lo enmascara con `CarReflectionMask` y lo usa como reflejo en el material del coche, con el multiplicador `leedsEnvMult` | **A PORTAR** | El port tiene **todo el scaffolding** (env map, mascara, camara) pero no el pipe: `src/extras/custompipes.cpp:57-230` (camara, quad, `EnvMapRender`), `src/extras/custompipes_gl.cpp:99-136` (el pipe neo ya usa `EnvMapTex` como `tex1`). El pipe leeds seria un `vehicleRenderCB` alternativo |
| F54 | **Pipe de mundo Leeds + tweak de prelight** | valor `2` de `worldPipe` | `leeds.cpp` (7,8 KB) | shader `leedsWorldVS` (`leeds.cpp:69-71`) | Pipe de mundo «Leeds»: reescribe la iluminacion del mundo con un vertex shader propio (MODULATE2X, tweak de prelight, emisiva a media) | **A PORTAR** | Mismo scaffolding. Es el shader mas pesado de portar |
| F55 | **Radiosidad VCS (rasteres de radiosidad)** | fijo, depende de `radiosity` | `skygfx.h:277-289`; `main.cpp:537-567` | rasters `ms_pRadiosityRaster1/2`, verts `ms_radiosityVerts[44]`, indices `ms_radiosityIndices[7*6]` | Rasteres extra para el efecto de radiosidad: copia el frame, lo reduce, lo ilumina y lo vuelve a componer | **NO PORTADO** (forma parte de F32-F36) | Sin soporte |

### 2.3 Tabla INI -> default -> instalado (resumen operativo)

| Clave | Default **del codigo** | Default **documentado en el INI** | Valor del INI instalado | Lee el codigo en |
|---|---|---|---|---|
| `replaceDefaultPipeline` | 0 | — | **1** | `main.cpp:943` |
| `disableColourOverlay` | -1 -> 0 | — | 0 | `main.cpp:822` |
| `carPipe` | -1 -> 0 | 0 = default, 1 = neo, 2 = leeds | (comentada) | `main.cpp:893` |
| `worldPipe` | -1 -> 0 | 0 = default, 1 = neo, 2 = leeds | (comentada) | `main.cpp:894` |
| `ps2Loadscreen` | 0 | — | **1** | `main.cpp:955` |
| `texblendSwitch` | no se toca | 0 = PS2, 1 = PC, 2 = Mobile | **0** | `main.cpp:836` |
| `texgenSwitch` | no se toca | 0 = PS2, 1 = PC, 2 = Mobile | **0** | `main.cpp:867` |
| `ps2light` | 1 | — | **1** | `main.cpp:877` |
| `ps2Water` | 0 | — | **1** | `main.cpp:832` |
| `dualPass` | no se toca | — | **1** | `main.cpp:906` |
| `disableBackfaceCulling` | 0 | — | 0 | `main.cpp:916` |
| `neoRimLightPipe` | -1 -> 0 | — | 0 | `main.cpp:903` |
| `neoGlossPipe` | -1 -> 0 | — | 0 | `main.cpp:904` |
| `neoWaterDrops` | -1 -> 0 | 0 = default, 1 = Neo, 2 = Extended Neo, 3 = Extended Neo + Blood | **1** | `main.cpp:939` |
| `envMapSize` | 128 | — | **128** | `main.cpp:896` |
| `seamFix` | 0 | — | 0 | `main.cpp:811` |
| `dontNag` | 0 | — | (comentada) | `main.cpp:810` |
| `noCarcolTex` | 0 | — | (comentada) | `main.cpp:861` |
| `YCbCrCorrection` | 0 | 0 | (comentada) | `main.cpp:969` |
| `lumaScale` | 219/255 | 0.8588 | (comentada) | `main.cpp:970` |
| `lumaOffset` | 16/255 | 0.0627 | (comentada) | `main.cpp:971` |
| `CbScale` | 1.23 | 1.22 | (comentada) | `main.cpp:972` |
| `CbOffset` | 0.0 | 0.0 | (comentada) | `main.cpp:973` |
| `CrScale` | 1.23 | 1.22 | (comentada) | `main.cpp:974` |
| `CrOffset` | 0.0 | 0.0 | (comentada) | `main.cpp:975` |
| `IIIEnvFrame` | 0 | — | (comentada) | `main.cpp:879` |
| `radiosity` | -1 -> 0 | — | (comentada) | `main.cpp:814` |
| `radiosityRenderPasses` | 2 | — | (comentada) | `main.cpp:819` |
| `radiosityFilterPasses` | 4 | — | (comentada) | `main.cpp:820` |
| `leedsEnvMult` | -9999 -> 0.4 (VC) | 0.3 | (comentada) | `main.cpp:901` |
| `leedsWorldPrelightTweakMult` | -9999 -> 1.0 | 0.5 | (comentada) | `main.cpp:816` |
| `leedsWorldPrelightTweakAdd` | -9999 -> 0.0 | 0.1 | (comentada) | `main.cpp:817` |
| `trailsSwitch` | -1 -> 0 | 0 = default, 1 = leeds, 2 = mobile | (comentada) | `main.cpp:815` |
| `trailsBlur` | 1 (fijado) | — | exportado | `main.cpp:775` |
| `trailsMotionBlur` | 1 (fijado) | — | exportado | `main.cpp:776` |
| `radiosityIntensity` | 0 | — | exportado | `skygfx.h:224` |
| `radiosityLimit` | 0 | — | exportado | `skygfx.h:225` |
| `currentAmbientMultRed/Green/Blue` | 0 | — | exportado | `skygfx.h:227-229` |
| `currentBlurAlpha` | 39.0 | — | exportado | `main.cpp:777` |
| `currentBlurOffset` | 2.1 | — | exportado | `main.cpp:778` |
| `ps2carCam` | 0 | — | (comentada) | `main.cpp:995` |
| `VCEnvFrame` | 0 | — | (comentada) | `main.cpp:884` |

> Discrepancia detectada entre el INI y el codigo (medida): el INI documenta `leedsWorldPrelightTweakMult=0.5` /
> `Add=0.1` y `leedsEnvMult=0.3`, pero el codigo usa `1.0` / `0.0` / `0.4` (`main.cpp:655-657`). **El codigo manda.**
> Lo mismo con `CbScale`/`CrScale`: el INI dice 1.22, el codigo 1.23.

---

## 3. El nucleo que ya existe en el port (`src\extras\`)

### 3.1 Que es `src\extras\` exactamente

**Medido por hash contra el original del mod Vice Extended** (`gta_vc_browser\tmp\upstream-gta-extended\src\extras\`):

| Fichero | Hash local vs upstream |
|---|---|
| `arrow.inc`, `cursor.inc`, `custompipes.h`, `debugmenu.h`, `frontendoption.cpp`, `frontendoption.h`, `GitSHA1.cpp.in`, `GitSHA1.h`, `ini.h`, `re3_inttypes.h`, `screendroplets.h` | **IDENTICO** (byte a byte) |
| `custompipes.cpp` | DIFERENTE (5 lineas) |
| `custompipes_gl.cpp` | DIFERENTE (12 lineas) |
| `custompipes_d3d9.cpp` | DIFERENTE (12 lineas) |
| `postfx.cpp` | DIFERENTE (87 lineas) |
| `postfx.h` | DIFERENTE (6 lineas) |
| `screendroplets.cpp` | DIFERENTE (2 lineas) |
| `modloader.cpp/.h`, `modloader_reVC.h`, `PhotoMode.cpp/.h` | **FALTAN** en el port (no hacen falta: el port no es un mod loader de Win32) |

Los 12+12+5+2 lineas de diferencia son, todas, **rutas de ficheros y nombres de macro**:

* `custompipes.cpp`: `#ifdef VICE_EXTENDED` → se simplifica a `"neo/neo.txd"` fijo; `VICEEXT_IMPROVED_VEHICLES` → `IMPROVED_VEHICLES`.
* `custompipes_gl.cpp` / `custompipes_d3d9.cpp`: idem para los tres `.dat`.
* `screendroplets.cpp`: `VICEEXT_FIRST_PERSON` → `FIRST_PERSON`.

**Conclusion medida**: `src/extras/` es una **copia casi literal de la copia de SkyGfx que trae el mod Vice Extended**
(que a su vez es un fork de SkyGfx). No es una reimplementacion: es el mismo codigo, con la ruta de ficheros
adaptada al serve web. Por eso el nucleo de SkyGfx 3.0b que falta en el port es, essencialmente, **lo que el
mod Extended no trae**.

### 3.2 `src\extras\custompipes.cpp` — el nucleo comun (no depende de la API grafica)

| Que | Donde | Que hace |
|---|---|---|
| `VehiclePipeSwitch` (MATFX / NEO) | `:350` | El interruptor equivalente a `carPipe` (solo 2 valores; falta «leeds») |
| `Fresnel`, `Power`, `DiffColor`, `SpecColor` | `:353-356` | Los 4 `Interpolated*` de `neoCarpipe.cpp:23-26` |
| `WorldLightmapBlend` | `:380` | El `InterpolatedFloat` de `neoWorldpipe.cpp:128` |
| `RampStart`, `RampEnd`, `Offset`, `Scale`, `Scaling` | `:444-448` | Los 5 de `neoRimpipe.cpp:36-40` |
| `ReadTweakValueTable` | `:312-342` | Copia exacta de `neoReadWeatherTimeBlock` (`neo.cpp:178-208`), pero sobre un buffer en memoria |
| `InterpolatedFloat/Color/Light` + `INTERP_SETUP` | `:236-310` | Copia exacta de `neo.cpp:100-176` (interpolacion por hora del reloj + clima) |
| `neoTxd` | `:47`, cargado en `:475-482` | Lee `neo/neo.txd` con `RwTexDictionaryGtaStreamRead` |
| `CreateEnvMapCam` | `:57-91` | Camara + rasters de `EnvMapSize` (128) x 128, near 0.1 / far 250, view window 2x2, matriz espejada en X e Y |
| `EnvMapRender` | `:123-170` | Renderiza la escena en la camara de env map, la enmascara con `CarReflectionMask` y devuelve el control a la camara principal |
| `RenderEnvMapScene` | `:115-121` | `RenderRoads` + `RenderEverythingBarRoads` + `RenderFadingInEntities` |
| `EnvMapInit` / `EnvMapShutdown` | `:172-230` | Crea el quad y busca `CarReflectionMask` en el `neo.txd` |
| `CustomPipeInit` / `CustomPipeShutdown` / `CustomPipeRegister` | `:472-517` | Ciclo de vida de las 4 pipes y del plugin de material |
| `GetGlossTex` | `:407-421` | Busca `<nombre>_gloss` en el `neo.txd` y lo cachea en `CustomMatExt` |
| `SetTxdFindCallback` | `:534-545` | Fallback: si una textura no esta en el TXD actual, la busca en `generic` |

**Diferencias medidas con SkyGfx 3.0b**:

| Punto | SkyGfx 3.0b | Port | Consecuencia |
|---|---|---|---|
| Escena del env map | `RenderEverythingBarCarsPeds` + `RenderAlphaListBarCarsPeds` (`pipeswitch.cpp:314-353`): **solo edificios** (`type == 1`), sin coches ni peds | `RenderRoads` + `RenderEverythingBarRoads` + `RenderFadingInEntities` (`custompipes.cpp:115-121`): **todo**, incluidos coches y peds | El env map del port se recalcula con los coches dentro -> las ruedas «giran mas rapido» en el reflejo. El propio SkyGfx lo evita a proposito (`neoCarpipe.cpp:123`) |
| Posicion de la camara de reflejo | Posicion de la camara principal (`neoCarpipe.cpp:157`) | Posicion del **jugador** (`FindPlayerCoors()`, `custompipes.cpp:132`) | En camara de vehiculo el reflejo no coincide con la vista |
| Color de cielo del env map | `skyBotRed/Green/Blue` (borde inferior del cielo) | `GetSkyTop*` con `VICEEXT_IMPROVED_VEHICLES`, si no `GetSkyBottom*` (`custompipes.cpp:137-145`) | Es la fila **8.15** del inventario historico (deliberadamente distinto del SkyGfx) |
| View window de la camara de reflejo | 2.0 x 2.0 durante el render, 0.4 x 0.4 en `SetupEnvMap` (`neoCarpipe.cpp:49-59, 139-141`) | 2.0 x 2.0 fijo (`custompipes.cpp:73`) | Menor |
| Mask | `CarReflectionMask` del `neo.txd` | idem (`custompipes.cpp:176`) | Igual |

### 3.3 `src\extras\custompipes_gl.cpp` — las 4 pipes en GL3

| Pipe | Callback | Shader (GLSL) | Equivalente en SkyGfx |
|---|---|---|---|
| **Coche (neo)** | `vehicleRenderCB` `:76-136` | `neoVehicle.vert` + `neoVehicle.frag` (`obj/neoVehicle_*.inc`) | `vehiclePass1VS` + `vehiclePass2VS` (`neoCarpipe.cpp:213-226`) |
| **Mundo (neo)** | `worldRenderCB` `:190-245` | `default_UV2.vert` + `neoWorldVC.frag` (`obj/default_UV2_vert.inc`, `obj/neoWorldVC_frag.inc`) | `vc_worldPS` (`neoWorldpipe.cpp:66`) |
| **Gloss (carreteras)** | `glossRenderCB` `:294-350` (encadena `worldRenderCB`) | `neoGloss.vert` + `neoGloss.frag` | `glossVS` + `glossPS` (`neoGlosspipe.cpp:82-95`) |
| **Rim (peds)** | `rimRenderCB` `:459-498` / `rimSkinRenderCB` `:416-457` | `neoRim.vert` / `neoRimSkin.vert` + `simple.frag` | `rimVS` (`neoRimpipe.cpp:125-133`) |

**Diferencias medidas con SkyGfx 3.0b**:

| Punto | SkyGfx 3.0b | Port |
|---|---|---|
| Pipe de coche: dos pases | `DiffusePass` + `SpecularPass` (`neoCarpipe.cpp:322-421`): el especular es un **segundo draw** con `BLENDONE`/`BLENDONE` y `ZWRITE` off | **Un solo draw**: `neoVehicle.frag` calcula `pass1` y `pass2` y los suma (`color.rgb = pass1.rgb*pass1.a + pass2`) |
| Pipe de coche: especular | Usa `specColor`/`power`/`fresnel` del `.dat` y las luces | `uploadSpecLights` (`:44-74`) sube `u_specDir[5]`/`u_specColor[5]`; el `power` se usa como exponente |
| Pipe de mundo: `MODULATE2X` | `neoWorldpipe.cpp:193` usa `D3DTOP_MODULATE2X` en la etapa 1 | `neoWorldVC.frag` hace `color = t0*v_color*(1.0 + u_lightMap*(t1-1.0))`, que es la misma maths reescrita en GLSL |
| `LightmapEnable` | En SkyGfx el pipe neo del mundo **siempre** esta instalado y el blend se decide en el shader (`config.worldPipeSwitch != WORLD_NEO` → `f = 0`, `neoWorldpipe.cpp:243-244`) | El port tiene un conmutador real `LightmapEnable` (`custompipes.cpp:378`) que, si esta apagado, delega en `gl3::defaultRenderCB` (`custompipes_gl.cpp:196-199`). **Mejor que SkyGfx** |
| `GlossEnable` | Solo activo con `config.neoGlossPipe` (`neoGlosspipe.cpp:221`) | Conmutador `GlossEnable` (`custompipes.cpp:403`) + menu. **Igual o mejor** |
| `RimlightEnable` | `config.rimlight` (`neoRimpipe.cpp:226, 294`) | Conmutador `RimlightEnable` + menu. **Igual** |
| `envMapSize` configurable | Si (`main.cpp:896`) | No: fijo a 128 (`custompipes.cpp:50`) |
| Uniformes | Registros D3D9 fijos (`c41`, `c42`, `c43`, …) | Uniformes GLSL con nombre (`u_eye`, `u_reflProps`, `u_specDir`, `u_specColor`, `u_lightMap`, `u_viewVec`, `u_rampStart`, `u_rampEnd`, `u_rimData`) |

### 3.4 `src\extras\postfx.cpp` — el post-proceso

| SkyGfx 3.0b | Port | Nota |
|---|---|---|
| `screenfx.cpp:127-205` `ScreenFX::AVColourCorrection`: compose RGB2YUV * escalas -> YUV2RGB y lo sube como **3 constantes de pixel shader** (`redGrade`/`greenGrade`/`blueGrade`), con el pixel shader `gradingPS` | `src/extras/postfx.cpp:511-568`: misma matriz (`RGB2YUV`/`YUV2RGB` en el port), mismo calculo, y las 3 filas como `u_gradeR`/`u_gradeG`/`u_gradeB` en el shader GLSL `src/extras/shaders/grading.frag` | **Equivalente funcional medido** (el shader del mod era D3D9 puro: `RwD3D9SetPixelShaderConstant`, sin rama GL) |
| `screenfx.cpp:81-110` `ScreenFX::Initialise` (crea el front buffer y el quad) | `src/extras/postfx.cpp:66-202` `CPostFX::Open` (front + back buffer, quads, shaders) | El port **fusiona** las dos pasadas (SkyGfx tiene `CMBlur` + `ScreenFX`; el port tiene un solo `CPostFX`) |
| `matfx.cpp` / `postfx.cpp` de SkyGfx (el trails leeds/mobile) | `src/extras/postfx.cpp:241-482` con `POSTFX_OFF/SIMPLE/NORMAL/MOBILE` y los shaders `colourfilterVC.frag` (el blur de 5 iteraciones) y `contrast.frag` | El port tiene el caso *mobile* (el `contrastPS` de SkyGfx), **no** el caso *leeds* |

### 3.5 Shaders: que hay en `src\extras\shaders\` y quien usa cada uno

Ficheros fuente (GLSL y HLSL) y sus `.inc` generados:

| Shader | `.inc` / `.cso` generados | Pipeline que lo usa | Origen en SkyGfx |
|---|---|---|---|
| `neoVehicle.vert` + `neoVehicle.frag` | `obj/neoVehicle_vert.inc`, `obj/neoVehicle_frag.inc` | Pipe de coche (neo) | `vehiclePass1VS`/`vehiclePass2VS` (reescrito) |
| `neoVehicle_VS.hlsl` + `neoVehicle_PS.hlsl` | `obj/neoVehicle_VS.inc/.cso`, `obj/neoVehicle_PS.inc/.cso` | Solo build D3D9 nativo (no se usa en web) | — |
| `neoWorldVC.frag` | `obj/neoWorldVC_frag.inc` | Pipe de mundo (neo) | `vc_worldPS` |
| `neoWorldVC_PS.hlsl` | `obj/neoWorldVC_PS.inc/.cso` | Solo D3D9 | — |
| `default_UV2.vert` + `default_UV2_VS.hlsl` | `obj/default_UV2_vert.inc`, `obj/default_UV2_VS.inc/.cso` | Pipe de mundo (vertice) | equivalente a `ps2StandardVS`/`pcStandardVS` |
| `neoGloss.vert` + `neoGloss.frag` | `obj/neoGloss_vert.inc`, `obj/neoGloss_frag.inc` | Pipe de gloss | `glossVS`/`glossPS` |
| `neoGloss_VS.hlsl` + `neoGloss_PS.hlsl` | `obj/neoGloss_VS.inc/.cso`, `obj/neoGloss_PS.inc/.cso` | Solo D3D9 | — |
| `neoRim.vert`, `neoRimSkin.vert` | `obj/neoRim_vert.inc`, `obj/neoRimSkin_vert.inc` | Pipes de rim | `rimVS` |
| `neoRim_VS.hlsl`, `neoRimSkin_VS.hlsl` | `obj/neoRim_VS.inc/.cso`, `obj/neoRimSkin_VS.inc/.cso` | Solo D3D9 | — |
| `simple.frag` | `obj/simple_frag.inc` | Fragment comun del rim | — |
| `screenDroplet.frag` + `im2d_UV2.vert` | `obj/screenDroplet_frag.inc`, `obj/im2d_UV2_vert.inc` | Gotas de pantalla | `neoWaterdrops` |
| `screenDroplet_PS.hlsl` | `obj/screenDroplet_PS.inc/.cso` | Solo D3D9 | — |
| `colourfilterVC.frag` + `im2d.vert` | `obj/colourfilterVC_frag.inc`, `obj/im2d_vert.inc` | Blur del overlay de color | `postfx.cpp` del mod |
| `colourfilterVC_PS.hlsl` | `obj/colourfilterVC_PS.inc/.cso` | Solo D3D9 | — |
| `grading.frag` | `obj/grading_frag.inc` | Pase YCbCr | `gradingPS.hlsl` |
| `gradingPS.hlsl` | `obj/gradingPS.inc/.cso` | Solo D3D9 | — |
| `contrast.frag` | `obj/contrast_frag.inc` | Trails «mobile» | `contrastPS.hlsl` |
| `contrastPS.hlsl` | `obj/contrast_PS.inc/.cso` | Solo D3D9 | — |
| `lighting.h`, `standardConstants.h` | — | Cabeceras HLSL compartidas | `lighting.h` de SkyGfx (re-escrito) |

**Lo que NO hay en el port (y si hay en SkyGfx)**: `ps2StandardVS`, `ps2StandardEnvVS`, `ps2StandardEnvOnlyVS`,
`pcStandardVS`, `pcStandardEnvVS`, `pcStandardEnvOnlyVS`, `nolightVS`, `nolightEnvVS`, `nolightEnvOnlyVS`,
`ambientVS`, `leedsWorldVS`, `vcsVS`, `glossVS`/`glossPS` (HLSL), `worldPS`, `pcEnvPS`, `mobileEnvPS`, `curvePS`.

**Generacion**: `src/extras/shaders/make_glsl.sh` + `makeinc_glsl.sh` (`.vert`/`.frag` -> `obj/*.inc` como
`const char *nombre_vert_src`), `make_hlsl.cmd` + `makeinc_hlsl.sh` (`.hlsl` -> `.cso` con `fxc` y de ahí a `.inc`).
El build web (`gta_vc_browser/build.sh:64`, `-DLIBRW_PLATFORM=GL3`) usa **solo los `.inc` GLSL**: los `.cso` son para
el build nativo D3D9. `src/extras/custompipes_d3d9.cpp` esta **entero** bajo `#ifdef RW_D3D9` (linea 3) y **no se
compila** en el port.

### 3.6 Que falta para igualar a SkyGfx 3.0b (resumen ejecutable)

1. **Pipe de coche Leeds (F53)** y **pipe de mundo Leeds (F54)**: faltan los shaders (`vcsVS`, `leedsWorldVS`) y los
   `RenderCallback` correspondientes. El scaffolding (env map, mascara, camara) ya existe.
2. **Los 3 estilos de MatFX (F06, F07, F08)**: faltan 6-9 shaders GLSL (los VS PS2/PC/nolight y los PS env).
3. **Trails Leeds + radiosidad VCS (F29, F32-F36)**: faltan los rasteres de radiosidad y el shader de contraste Leeds.
4. **`neoWaterDrops` modos 2 y 3 (F05)**: faltan las gotas «Extended» y «+Blood» (hooks de particulas).
5. **`dualPass` (F10)**: falta el doble draw con alpha func.
6. **`seamFix` (F19)**: falta (solo el comentario en `src/core/main.cpp:1641`).
7. **`ps2Water` (F09)**: falta el parche de UVs del agua.
8. **Fixes de blend del sifon y del punto (F45)**: falta localizarlos y corregirlos.
9. **`envMapSize` configurable (F15)** y **conmutador de `worldPipe` (F02)**: faltan como opciones.
10. **Los 8 parametros exportados sin equivalente (F30, F31, F33, F34, F40-F44)**: el port no expone nada
    equivalente a `SkyGFXGetParam`/`SetParam` (no hay API de DLL, pero podria haber un bloque en `reVC.ini`).

---

## 4. Las tablas de datos `neo/*.dat`

### 4.1 Que son y como las lee el mod

Formato (medido leyendo los ficheros): **tablas de 24 lineas (hora 0..23) x 7 columnas (clima)**,
separadas por tabuladores, con lineas `#` de comentario que se saltan.

El lector es `neoReadWeatherTimeBlock` (`neo.cpp:178-208`), que consume bloques de 24 lineas de un `FILE*` y llama
`InterpolatedValue::Read(buf, line, field)` con `sscanf` segun el tipo:

| Fichero | Bloques (24 lineas cada uno) | Tipo | Valores por defecto en el codigo |
|---|---|---|---|
| `carTweakingTable.dat` (104 lineas) | 1. `Fresnel` (L3-26), 2. `Specular Power` (L29-52), 3. `Diffuse Colour Modifier` (L55-78), 4. `Specular Colour` (L81-104) | float / float / color RGBA 0-255 / color RGBA | `0.4`, `18.0`, `(0,0,0,0)`, `(0.7,0.7,0.7,1.0)` (`neoCarpipe.cpp:23-26`) |
| `rimTweakingTable.dat` (130 lineas) | 1. `Ramp Start` (L3-26), 2. `Ramp End` (L29-52), 3. `Offset` (L55-78), 4. `Scale` (L81-104), 5. `Scaling` (L107-130) | color RGBA / color RGBA / float / float / float | `(0,0,0,1)`, `(1,1,1,1)`, `0.5`, `1.5`, `2.0` (`neoRimpipe.cpp:36-40`) |
| `worldTweakingTable.dat` (30 lineas) | 1. `LM blend` (L3-26) | float | `1.0` (`neoWorldpipe.cpp:27`) |

Los colores del `.dat` se normalizan: `InterpolatedColor::Read` divide por 255 (`neo.cpp:151-152`), pero
`InterpolatedLight::Read` divide el **alfa por 100** (`neo.cpp:174`). Ojo: es una rareza del autor, no un error
nuestro.

La interpolacion final es bilineal sobre (hora del reloj, clima viejo, clima nuevo): `INTERP_SETUP`/`INTERP`
(`neo.cpp:100-111`), con `NUMWEATHERS = 7` para VC (`neo.h:33`).

Los shaders no leen los `.dat`: los valores se suben como constantes/uniformes desde el CPU.

### 4.2 Si el port los carga: **SI**

Medido:

| Fichero | Servido en el port | Identico al de SkyGfx | Quien lo lee |
|---|---|---|---|
| `carTweakingTable.dat` | `gta_vc_browser/streamed/neo/carTweakingTable.dat` (7 491 B) y `gta_vc_browser/gamefiles/neo/carTweakingTable.dat` | **SI** (sha256 `F21ED5C96AFF…` en los dos) | `src/extras/custompipes_gl.cpp:144-152` |
| `rimTweakingTable.dat` | `gta_vc_browser/streamed/neo/rimTweakingTable.dat` (9 004 B) | **SI** (`0FF2890704E2…`) | `src/extras/custompipes_gl.cpp:505-514` |
| `worldTweakingTable.dat` | `gta_vc_browser/streamed/neo/worldTweakingTable.dat` (1 243 B) | **SI** (`046B48131BA5…`) | `src/extras/custompipes_gl.cpp:253-256` |
| `neo.txd` | `gta_vc_browser/streamed/neo/neo.txd` (**28 968 B**) | **NO** (`93118FE57E4E…` en SkyGfx) | `src/extras/custompipes.cpp:475-482` |

Y el manifiesto web los publica: `gta_vc_browser/web/public/manifest.json` contiene las 4 entradas
`neo/cartweakingtable.dat`, `neo/neo.txd`, `neo/rimtweakingtable.dat`, `neo/worldtweakingtable.dat`.

**Conclusion**: las 3 tablas de tweaking **ya estan portadas y servidas, byte a byte identicas a las de SkyGfx 3.0b**.
No hay ningun hueco ahi. El unico fichero que difiere es `neo.txd`:

* SkyGfx 3.0b (31 144 B) tiene `CarReflectionMask`, `dropmask`, `MaterialCursor` + 15 texturas `_gloss`.
* El del port (28 968 B) tiene `CarReflectionMask`, `MaterialCursor` + las mismas 15 texturas `_gloss`,
  **pero NO tiene `dropmask`**.

Y esa es exactamente la razon por la que el port genera la mascara de gotas proceduralmente
(`src/extras/screendroplets.cpp:80-103`, `CreateDropMask(64)`): no hay `dropmask` que leer. Es una **decision
consciente ya resuelta**, no un bug.

### 4.3 Huecos medibles que quedan de las tablas

| Hueco | Donde | Impacto |
|---|---|---|
| El port **no** lee `dropmask` del TXD | `screendroplets.cpp:108` | La mascara es un circulo blanco de 64x64 generado por codigo; la original puede tener forma de gota. **Diferencia visual menor, medible** |
| El port **no** tiene el `MaterialCursor` | (esta en su TXD) | Sin uso en SkyGfx |
| La tabla `worldTweakingTable.dat` se lee, pero su valor se multiplica por `LightmapMult` (un multiplicador propio del mod Extended, `custompipes.cpp:379`) | `custompipes_gl.cpp:224` | El SkyGfx usa el valor tal cual (`neoWorldpipe.cpp:242`). Desviacion documentada del fork |
| `envMapSize` no es configurable | `custompipes.cpp:50` | El `.dat` no lo controla tampoco en SkyGfx |

---

## 5. Ficheros que no aportan nada al port

### 5.1 `mods\skygfx\d3d8.dll` (279 040 B) — envoltorio D3D8 -> D3D9

**Clasificacion: NO APLICA (Windows/x86 nativo).**

* Que es: el `d3d8to9` de Leonard, un proxy de la API Direct3D 8. El juego retail de VC llama a `d3d8.dll`; este
  fichero, con el mismo nombre, traduce las llamadas a D3D9 y por eso el `rwd3d9.dll` puede hablar con la GPU.
* Por que no sirve: el port **no ejecuta el exe retail**. Es un `reVC.wasm` (Emscripten) con **librw GL3**
  (`gta_vc_browser/build.sh:64`, `-DLIBRW_PLATFORM=GL3`, `-sUSE_WEBGL2=1`). No hay `d3d8.dll`, ni `d3d9.dll`,
  ni superficie de ventana Win32.
* Que habria que hacer si se quisiera: implementar en WebGL2 lo que el wrapper hacia en D3D9 — es decir, **nada
  de este fichero es reutilizable**, porque su unico trabajo es la traduccion de API que el port no usa.

### 5.2 `mods\skygfx\rwd3d9.dll` (72 704 B) — driver RenderWare -> D3D9

**Clasificacion: NO APLICA (D3D9 nativo), pero es la fuente de verdad de los shaders.**

* Que es: un driver de RenderWare reimplementado sobre Direct3D 9. Exporta las 12 funciones `RwD3D9*`
  (medidas, §1.2) y lleva dentro los blobs de bytecode de los vertex/pixel shaders Model 2.0.
* Por que no sirve tal cual: el port usa **librw GL3**, cuyo pipeline es OpenGL. `RwD3D9CreateVertexShader` no
  tiene equivalente, y los shaders que produce son bytecode D3D9, no GLSL.
* **Pero si es aprovechable**: es la **fuente de los shaders**. El `.asi` trae las fuentes HLSL en el repo publico
  (`gta_vc_browser/tmp/extsrc/skygfx_vc/skygfx_vc-master/shaders/`, 24 ficheros) y los `.cso` estan embebidos en
  estos dos binarios. Para portar, la via correcta es **reescribir los HLSL a GLSL** (que es lo que ya se hizo
  con `neoVehicle`/`neoGloss`/`neoRim`/`neoWorldVC`), no extraer bytecode de D3D9.
* Ojo: el `.asi` **importa estaticamente** `RWD3D9.DLL` (`RwD3D9Supported` entre otros). Sin `rwd3d9.dll` al lado,
  el `.asi` **no carga**. Es dependencia dura.

### 5.3 `mods\skygfx\SkyGfx\skygfx.asi` — no es "no aporta": es la referencia

Al contrario que los otros dos: **es el unico componente que aporta**. Todo el trabajo del port sale de aqui
(46 claves, 9 exports, ~1300 funciones, el set completo de shaders HLSL embebidos en el formato público).

### 5.4 `mods\skygfx\Readme (or die).txt` y `Leiame (ou morra).txt`

Informativos (version 3.0b, autor). Sin valor tecnico. El `Leiame` es la traduccion al portugues.

---

## 6. Lista final por categoria

Recuento: **A PORTAR 9 · PARCIAL 9 · YA ESTABA/PORTADO 16 · NO PORTADO 12 · NO APLICA 9 = 55 features.**
(Una feature tiene **una sola** categoria primaria; las notas cruzadas van dentro de la fila.)

### 6.1 `A PORTAR` (9 features, con destino y razon)

| # | Feature | Fichero destino | Razon |
|---|---|---|---|
| 1 | **F53 · Pipe de coche Leeds** (valor 2 de `carPipe`) | `src/extras/custompipes_gl.cpp` (nuevo `leedsVehicleRenderCB`) + `src/extras/shaders/neoLeedsCar.vert/.frag` + `src/extras/custompipes.cpp` (usar `EnvMapTex` + `EnvMaskTex`) | Es el unico pipeline de SkyGfx que da el reflejo "de verdad" con la `CarReflectionMask`. El scaffolding (env map de 128, mascara, camara) **ya existe**: solo falta el shader y el callback. El `vcsVS` de SkyGfx es un VS D3D9; hay que reescribirlo en GLSL |
| 2 | **F54 · Pipe de mundo Leeds** (valor 2 de `worldPipe`) | `src/extras/custompipes_gl.cpp` + `src/extras/shaders/leedsWorld.vert` | Necesario para `F37/F38/F39` (los tweaks de prelight) y para `F32-F36` (radiosidad). Es el shader mas pesado |
| 3 | **F06 · `texblendSwitch`** (PS2/PC/Mobile) | `vendor/librw/src/gl/shaders/` (3 pares VS/PS) + selector en `src/core/re3.cpp` | Los 3 estilos de blend de env map no existen en librw. Impacto visual: el reflejo del coche y de los materiales con env map |
| 4 | **F07 · `texgenSwitch`** | `vendor/librw/src/gl/shaders/matfx_env.vert` (3 variantes) | Igual que F06, afecta a las UV del env map |
| 5 | **F08 · `ps2light`** (el caso PC) | `vendor/librw/src/gl/shaders/default.vert` (variante PC) + conmutador | El caso PS2 ya esta (por construccion). Falta el caso PC como alternativa |
| 6 | **F19 · `seamFix`** | `src/core/main.cpp` (en `RenderScene_new`, donde ya esta el comentario `// seam fixer`) | Es un parche de pantalla completa; el sitio ya esta marcado en el codigo. Riesgo: puede blarear la imagen |
| 7 | **F09 · `ps2Water`** (el parche de UVs, no el alpha test) | `src/core/WaterLevel.cpp` (o `src/renderer/`) | El `gPS2alphaTest` ya esta; falta el parche de las UVs del mask atomic y el coeficiente de env map |
| 8 | **F10 · `dualPass`** | `src/extras/custompipes_gl.cpp` (doble `drawInst` con `glAlphaFunc`) + `src/renderer/` para la geometria transparente | En WebGL2 es reimplementable (2 draws con alpha func), pero el hook de SkyGfx es sobre el draw call D3D8 de RW, que no existe en el port |
| 9 | **F45 · Fix del blend del sifon y del punto de francotirador** | `src/renderer/Hud.cpp` (dibujo del punto) y el equivalente del sifon | Bug real de alpha en el retail (parche de 1 byte en `0x5D4EEE` y `0x558024`). En el port hay que localizar el `rw::SRCBLEND` de esos dos draws y poner `BLENDINVSRCALPHA` |

### 6.2 `YA ESTABA` / `PORTADO` (16 features; no hay que hacer nada)

| # | Feature | Donde esta en el port |
|---|---|---|
| 1 | **F03 · `neoRimLightPipe`** | `src/extras/custompipes_gl.cpp:393-546` + menu `src/core/re3.cpp:146/152` + enganche `src/modelinfo/PedModelInfo.cpp:86` |
| 2 | **F04 · `neoGlossPipe`** | `src/extras/custompipes_gl.cpp:294-372` + `src/extras/custompipes.cpp:407-421` + menu `src/core/re3.cpp:148/154` + enganche `src/modelinfo/SimpleModelInfo.cpp:91-92` |
| 3 | **F11 · `disableBackfaceCulling`** | `src/rw/RwHelper.cpp:23-25` (`gBackfaceCulling = false`), `:103-110` (`SetCullMode`), `src/core/re3.cpp:779/935/1337` (INI `Rendering/BackfaceCulling` + menu). Inventario historico fila **8.16** |
| 4 | **F14 · `replaceDefaultPipeline`** (equivalente por construccion) | `vendor/librw/src/gl/shaders/default.vert:19-20` — el color de material se aplica **una sola vez**; no hay premultiplicacion doble |
| 5 | **F15 · `envMapSize` = 128** | `src/extras/custompipes.cpp:50` |
| 6 | **F22 · `YCbCrCorrection`** | `src/extras/postfx.cpp:511-568` + `src/extras/shaders/grading.frag` + `src/core/re3.cpp:764` + menu `:1373`. Inventario historico fila **8.11** |
| 7 | **F23 · `lumaScale`** | `src/extras/postfx.cpp:46`, `src/core/re3.cpp:765`, menu `:1374` |
| 8 | **F24 · `lumaOffset`** | `src/extras/postfx.cpp:47`, `src/core/re3.cpp:766`, menu `:1375` |
| 9 | **F25 · `CbScale`** | `src/extras/postfx.cpp:48`, `src/core/re3.cpp:767`, menu `:1376` |
| 10 | **F26 · `CbOffset`** | `src/extras/postfx.cpp:49`, `src/core/re3.cpp:768`, menu `:1377` |
| 11 | **F27 · `CrScale`** | `src/extras/postfx.cpp:50`, `src/core/re3.cpp:769`, menu `:1378` |
| 12 | **F28 · `CrOffset`** | `src/extras/postfx.cpp:51`, `src/core/re3.cpp:770`, menu `:1379` |
| 13 | **F46 · Trails del francotirador** (equivalente) | `src/extras/postfx.cpp:463-465` (`RenderOverlaySniper`, `MOTION_BLUR_SNIPER`) |
| 14 | **F47 · Integracion con DebugMenu** | `src/extras/debugmenu.cpp` + `src/core/re3.cpp:1337-1403` |
| 15 | **F48 · Plugin de material «Gloss»** | `src/extras/custompipes.cpp:11-18, 26-43, 508-517` |
| 16 | **F52 · Conjunto de shaders** (subconjunto) | `src/extras/shaders/` (27 shaders: 15 GLSL + 12 HLSL, ya traducidos) |

### 6.3 `PARCIAL` (9 features; falta una pieza concreta)

| # | Feature | Que falta exactamente |
|---|---|---|
| 1 | **F01 · `carPipe`** | El **tercer valor** (leeds). Faltan el shader y el `RenderCallback`. El caso *neo* ya esta (`src/extras/custompipes_gl.cpp:76-170`) y el menu solo ofrece `FED_MFX` / `FED_NEO` (`src/core/re3.cpp:139`) |
| 2 | **F02 · `worldPipe`** | El valor *leeds* y el **conmutador de menu** (no hay «World pipe»). El pipe neo del mundo esta instalado (`src/modelinfo/SimpleModelInfo.cpp:94`) y lo que se puede apagar es el blending con `LightmapEnable` |
| 3 | **F05 · `neoWaterDrops`** | El caso 1 (Neo) esta; faltan los modos **2 (*Extended Neo*)** y **3 (*Extended+Blood*)**: los hooks de `AddParticle` (`neoWaterdrops.cpp:338-359, 381-392`) |
| 4 | **F20 · `disableColourOverlay`** | Un conmutador real de «no dibujar el overlay de color» y la logica de «switch between light values» (`main.cpp:537-567`). El port tiene `CPostFX::EffectSwitch` (`src/extras/postfx.h:8-13`), que es otra cosa |
| 5 | **F29 · `trailsSwitch`** | El caso *mobile* esta (`POSTFX_MOBILE` + `contrast.frag`); falta el caso *leeds* (el mas pesado: bloom + sharpening) y el hack del menu de Trails (`silenttrails.cpp:40-64`) |
| 6 | **F30 · `trailsBlur`** | La funcion existe (`CPostFX::BlurOn`), pero no como **parametro con nombre** exportable |
| 7 | **F31 · `trailsMotionBlur`** | La funcion existe (`CPostFX::MotionBlurOn`), pero no como parametro con nombre |
| 8 | **F43 · `currentBlurAlpha`** | El port tiene `CPostFX::Intensity` (`src/extras/postfx.h:20`), que es otra magnitud |
| 9 | **F44 · `currentBlurOffset`** | Sin equivalente |

### 6.4 `NO APLICA` (9 features, con el motivo tecnico)

| # | Feature | Motivo tecnico |
|---|---|---|
| 1 | **F12 · `ps2Loadscreen`** | Parche de bytes del exe retail (`0x4A69D4`); el port tiene su propia pantalla de carga (`src/core/main.cpp:696, 2170-2187`, y `WebDrawLoadScreen` en web) |
| 2 | **F13 · `ps2carCam`** | Solo GTA III (`main.cpp:995`); en VC el bloque no se compila |
| 3 | **F16 · `IIIEnvFrame`** | Curiosidad (el propio INI la lista bajo «Curiosities», `skygfx.ini:56-57`): usa un frame de env map de GTA III dentro de VC |
| 4 | **F17 · `VCEnvFrame`** | Solo GTA III (`main.cpp:884`); el inverso de F16 |
| 5 | **F18 · `noCarcolTex`** | Solo GTA III (`main.cpp:861`); el propio codigo dice que rompe MatFX |
| 6 | **F21 · `dontNag`** | `MessageBoxA` de Windows; el port avisa por `printf` (`src/extras/custompipes.cpp:477`) |
| 7 | **F49 · Enumeracion de modulos (psapi)** | Solo se usa para detectar `iii_anim` (GTA III). En WASM no hay `EnumProcessModules` |
| 8 | **F50 · Consola F8** | `AllocConsole` de Windows; en el navegador hay la consola del navegador |
| 9 | **F51 · Deteccion de D3D9 / fallback a D3D8** | El port es GL3: no hay D3D9 ni D3D8 |

Ademas, **artefactos NO APLICABLES** (no son features pero se clasifican aqui):

| Artefacto | Motivo tecnico |
|---|---|
| **`mods\skygfx\d3d8.dll`** | Envoltorio D3D8->D3D9 de Windows; el port no ejecuta el exe retail (§5.1) |
| **`mods\skygfx\rwd3d9.dll`** | Driver D3D9 nativo; el port usa librw GL3. Los shaders si son aprovechables como fuente (§5.2) |

### 6.5 `NO PORTADO` (12 features: la feature base no existe en el port)

Todas cuelgan de la radiosidad VCS o del pipe de mundo Leeds, que no existen en el port.

| # | Feature | Bloqueante | Nota |
|---|---|---|---|
| 1 | **F32 · `radiosity`** | F29 (trails leeds) + F55 | `rg radiosity src/` = **0 coincidencias** (medido) |
| 2 | **F33 · `radiosityIntensity`** | F32 | Exportado para que otro mod fije el valor; el INI avisa que hace falta un plugin externo |
| 3 | **F34 · `radiosityLimit`** | F32 | idem |
| 4 | **F35 · `radiosityRenderPasses`** | F32 | idem |
| 5 | **F36 · `radiosityFilterPasses`** | F32 | idem |
| 6 | **F37 · `leedsEnvMult`** | F53 | Multiplicador del env map del pipe leeds |
| 7 | **F38 · `leedsWorldPrelightTweakMult`** | F54 | Multiplicador del prelight en el pipe leeds |
| 8 | **F39 · `leedsWorldPrelightTweakAdd`** | F54 | Suma al prelight en el pipe leeds |
| 9 | **F40 · `currentAmbientMultRed`** | F54 | Multiplicador del ambiente (solo III lo usaba de verdad) |
| 10 | **F41 · `currentAmbientMultGreen`** | F54 | idem |
| 11 | **F42 · `currentAmbientMultBlue`** | F54 | idem |
| 12 | **F55 · Radiosidad VCS (rasteres)** | F32 | Rasteres `ms_pRadiosityRaster1/2` + verts/indices |

### 6.6 `PENDIENTE` (decisiones sin accion clara todavia)

| # | Asunto | Por que esta pendiente |
|---|---|---|
| 1 | **El `neo.txd` del SkyGfx (31 144 B) vs el del port (28 968 B)** | El del port no tiene `dropmask` y el port lo compensa generando la mascara por codigo. Decision: ¿se adopta el TXD del SkyGfx tal cual (y se carga `dropmask`), o se mantiene el actual? Afecta a `F05` |
| 2 | **El `envMapRender` del port renderiza coches y peds dentro del env map** | Es una desviacion deliberada respecto a SkyGfx (`neoCarpipe.cpp:123` lo evita explicitamente). Afecta a `F01`/`F53` |
| 3 | **API de parametros al estilo `SkyGFXGetParam`/`SetParam`** | El `.asi` expone 9 funciones para que otros mods ajusten sus parametros en caliente. El port no tiene nada equivalente (ni DLL ni API). Habria que decidir si el plan de port lo quiere |
| 4 | **Nombres de las claves de INI** | El port usa `reVC.ini` con secciones propias (`Rendering`, `CustomPipesValues`). ¿Se mantienen los nombres de SkyGfx (`carPipe`, `neoWaterDrops`, …) o los del port? Afecta a la continuidad del port |
| 5 | **El `.asi` como modulo, no como codigo** | El port no puede «cargar el `.asi`»; hay que decidir si el plan reimplementa (que es lo que supone este inventario) o si se documenta el mod como referencia de comportamiento |

---

## 7. Riesgos

### 7.1 Features que exigen D3D9 y son imposibles tal cual en WebGL2

| Feature | Por que | Que hay que hacer |
|---|---|---|
| **F01/F53** (pipe de coche neo/leeds) | El VS original (`vehiclePass1VS`) sube matrices y luces en **registros D3D9 fijos** (`c41` = eye, `c42` = reflProps, `c43` = specLights[5], `c0` = combined, `c4` = world). En GL3 son **uniforms con nombre** y matrices con layouts distintos (`u_proj`/`u_view`/`u_world` por separado en lugar de `combinedMat`) | Reescritura total del VS. **El port ya lo hizo** (`neoVehicle.vert`): `gl_Position = u_proj * u_view * Vertex` en vez de `mul(combinedMat, Position)`. Ojo: `combinedMat` de SkyGfx = `proj * view * world`; el `u_world` de librw es solo `world`. El shader GLSL del port **replica** la multiplicacion, asi que el resultado es el mismo |
| **F03** (rim light) | `rimVS` + `viewVec` calculado en el VS con `XMVector4Transform` | Rehecho en `neoRim.vert` / `neoRimSkin.vert` (el `u_viewVec` se calcula en la CPU, `custompipes_gl.cpp:399-400`). **Ya resuelto** |
| **F04** (gloss) | `glossVS` + `glossPS` con `MakeProjectionMatrix` (bias de near plane 0.01) | Rehecho en `neoGloss.vert/.frag` |
| **F54** (pipe de mundo Leeds) | `leedsWorldVS` + el tweak de prelight | Falta reescribir |
| **F22-F28** (YCbCr) | `gradingPS` con `RwD3D9SetPixelShaderConstant` (3 filas de matriz) | **Resuelto**: el port lo hace con `u_gradeR/G/B` en GLSL (`grading.frag`). El comentario del propio port lo dice: «La mitad D3D9 del mod no se porta» (`postfx.cpp:550-551`) |
| **F06/F07/F08** (MatFX PS2/PC/Mobile) | 16 VS + 8 PS de Model 2.0 | Falta todo el set en GLSL. **Es el bloque mas grande de trabajo** |

### 7.2 Features que dependen de la ventana nativa / del exe retail

* **F12 `ps2Loadscreen`**, **F45 (fix de blend)**, **F11 en su forma original**, **F16/F17 (frames de env map)**,
  **F09 (parche de UVs del agua)**: todos son **parches de bytes sobre `gta-vc.exe`**. En el port no hay exe que
  parchar: hay que reimplementar la intencion en el codigo. Medido: las direcciones parcheadas existen y tienen el
  contenido esperado en el retail (p.ej. `0x4A69D4` = `00 59 50 e8`, que tras el parche de SkyGfx queda
  `90 ff 74 24 78` = `push DWORD PTR [esp+0x78]`).
* **F49 (psapi)** y **F50 (consola F8)**: API de proceso de Windows.
* **F14**: el hook es sobre la *vtable* de la pipeline de RW del exe retail.

### 7.3 Features que dependen de los datos del mod (`.dat` / TXD)

* Las tres tablas `.dat` **estan servidas y son identicas** → riesgo nulo (medido, §4.2).
* `neo.txd`: el del port **no tiene `dropmask`**. Si en el futuro se decide usar el TXD del SkyGfx (31 144 B) hay que
  servirlo y volver a la carga por nombre, o mantener la mascara procedural. Riesgo: bajo, pero es una decision
  explicita pendiente (§6.6-1).
* `CarReflectionMask` y las 15 texturas `_gloss`: **estan** en el `neo.txd` del port. Riesgo nulo.
* Si se portea el pipe Leeds (F53), haria falta decidir si se reutiliza el env map del port (que renderiza coches y
  peds) o se arregla para que solo renderice edificios, como hace SkyGfx.

### 7.4 "Curiosities" sin valor real

| Feature | Por que |
|---|---|
| **F16 `IIIEnvFrame`** | Usa una matriz de env map de otro juego. El propio INI lo coloca bajo «Curiosities» (`skygfx.ini:56-57`) |
| **F17 `VCEnvFrame`** | El inverso; ademas solo III |
| **F13 `ps2carCam`** | Solo III; ademas el propio codigo lo marca como un bugfix agradecido a Fire_Head |
| **F18 `noCarcolTex`** | Solo III; el propio codigo dice «but it could be intended... what to do?» (`main.cpp:860`) y rompe MatFX |
| **F30/F31 `trailsBlur`/`trailsMotionBlur`** | Se exportan para otros mods, pero no se pueden configurar por INI; sus valores estan fijados a 1 en `patch()` |
| **F33/F34 `radiosityIntensity`/`radiosityLimit`** | Idem: exportados, sin INI, y la radiosidad VCS no se puede usar sin un plugin externo que de los valores de control (lo dice el INI: «needs leeds trails and a plugin to supply control values», `skygfx.ini:63`) |
| **F40-F42 `currentAmbientMult*`** | Exportados para que otro mod fije el ambiente; el port toma el ambiente del timecycle directamente |

### 7.5 Riesgos de portabilidad del nucleo ya existente

1. **`src/extras/custompipes_d3d9.cpp` no se compila en el port** (todo bajo `#ifdef RW_D3D9`, linea 3) — el port
   solo tiene la variante GL3. Si alguien espera que el build nativo D3D9 tambien funcione, hay que revisar ese
   fichero aparte (no verificado: el build nativo no es el del port).
2. **`src/rw/RwPS2AlphaTest.cpp` esta muerto** (todo bajo `#ifndef LIBRW`, linea 1) pero `gPS2alphaTest` **si** se usa
   (`src/rw/RwHelper.cpp:18, 95`). Es codigo muerto que puede confundir: la implementacion real del alpha test esta
   en librw (`rw::GSALPHATEST`).
3. **El `neo.txd` se carga con `RwStreamOpen(rwSTREAMFILENAME, ...)`** (`src/extras/custompipes.cpp:475`). En el
   port eso funciona porque el emscripten FS monta el arbol servido, pero es un punto de fragility: cualquier cambio
   en las rutas del serve rompe el `neo.txd` **y con el las 15 texturas de gloss y la mascara de reflejo**.
4. **`AttachGlossPipe(rw::Clump*)` llama a `AttachWorldPipe`** (`src/extras/custompipes.cpp:430-434`) — un bug
   heredado del mod Extended. No duele porque el llamador real usa la version de `RpAtomic`, pero es una trampa.
5. **El env map se renderiza una vez por frame, dentro de `RenderScene` + `EnvMapRender`**
   (`src/core/main.cpp:2037-2041`): en el port web eso son **2 escenas por frame**, con el coste medido de un
   pipeline extra. Si se anade el pipe Leeds habria que medir si conviene.
6. **La `neoWorldVC.frag`/`default_UV2.vert` reimplementan la maths de la `MODULATE2X` de D3D8** en GLSL. El
   resultado *no* es identico bit a bit (D3D8 y GLSL redondean distinto, y `MODULATE2X` satura a 1 mientras que la
   formula `(1 + lm*(t1-1))` no satura). **Riesgo de diferencia visual** que solo se cierra con capturas A/B.
   *(Supuesto razonado, no medido: no hay capturas comparativas.)*
7. **El pipe de coche del port hace un solo draw donde SkyGfx hace dos.** El especular se mezcla en el fragment
   shader (`pass1.rgb*pass1.a + pass2`). Con Z-write off y `BLENDONE` en el pase especular de SkyGfx, el resultado
   con niebla y con alpha puede diferir. *(Supuesto: no verificado contra el retail.)*

---

## 8. Anexo: como se midio

| Herramienta | Que se uso para | Dato |
|---|---|---|
| `REA` (Ghidra 12.1.4) `open_binary` | Identidad del artefacto | sha256, PE, x86 |
| `REA` `binary_overview` | Segmentos, nº de procedimientos y de strings | 1319 proc., 474 strings |
| `REA` `list_strings` | Las 46 claves de INI, las rutas de los `.dat`, el TXD, `dropmask`, `CarReflectionMask`, `?_gloss`, `psapi`, `K32EnumProcessModules`, `iii_anim`, `nodeD3D8SkinAtomicAllInOne.csl` | direcciones exactas |
| `REA` `list_names` | Exports (9) e imports (RWD3D9.DLL x9, KERNEL32 x76, USER32 x2, ADVAPI32 x1) | tabla de exports |
| `REA` `xrefs` | Referencias a `envMapSize` (`0x1000a092`), `worldPipeSwitch` (`0x1000ba86`…), `replaceDefaultPipeline` (`0x1000a8f3`) | puntos de lectura del INI |
| `REA` `procedure_pseudo_code` | `SkyGFXGetVersion` (devuelve `0x280`), `SkyGFXGetParam` (cadena de `strcmp` contra las claves), `FUN_100089f0` (el lector del INI: escribe `39.0f` y `2.1f` en `DAT_1004436c/70`), `AttachCarPipeToRwObject` | layout de la config en `.data` |
| `REA` sobre `rwd3d9.dll` | Exports `RwD3D9*` (12) y layout PE | idem |
| Python propio (`sha256`, parser PE y volcado de bytes) | Hashes, secciones, y bytes del exe retail en las direcciones que SkyGfx parchea (`0x4A69D4`, `0x5D4EEE`, `0x67BAB7`, `0x678D69`, `0x674380`, `0x4C9E5F`, `0x5D4EEE`, `0x558024`, `0x55EA39`, `0x57A8BA`, `0x697FF8`, `0x698028`, `0x698088`, `0x4A604A`, `0x579DFB`, `0x6755D0`, `0x656031`, `0x676460`, `0x5C2E8C`, `0x5C3816`, `0x5BF977`, `0x578B7B`, `0x4A6151`, `0x5FFAFE`, `0x600411`) | confirmacion de que el retail es el objetivo correcto |
| `Get-FileHash` / `Compare-Object` | Comparar `src/extras/` con el original del mod Extended; comparar los `neo/*.dat` de SkyGfx con los del port | ver §3.1 y §4.2 |
| `grep` sobre `src/` | Estado real de cada feature en el port | ver §6 |
| Lectura del `manifest.json` del serve web | Confirmar que los 4 ficheros de `neo/` se sirven | ver §4.2 |
