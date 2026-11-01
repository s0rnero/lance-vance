# WFP para GTA VC — Recibo de ingeniería inversa (subagente 16)

> Mod: **WidescreenFixesPack for GTA VC** (ThirteenAG), build instalada del **27/09/2018**.
> Objetivo: inventario clasificado de **TODAS** las features del binario instalado, con evidencia medida,
> para alimentar un plan de port a reVC (GTA VC → WebAssembly). **No se implementa nada en el motor.**
>
> Regla de oro de este recibo: **el binario instalado manda**; el fuente público (master) es sólo el mapa.
> Cuando discrepan, se documenta la discrepancia. Toda afirmación sin medida directa va marcada `supuesto`.

---

## 0. Veredicto en una línea

El ASI instalado **es la build "legacy" del WFP**: el mismo código que hoy vive en
`source/GTAVC.WidescreenFix/Legacy.ixx` del master (96 de sus 106 patrones coinciden **exactamente**),
compilado en 2018 y con **cuatro features que el master ya no tiene** (`CarSpeedDependantFOV`,
`FixVehicleLights`, `SmallerTextShadows`, `FOVControl`) y **dos que el master sí tiene** y el binario no
(`SmallerVehicleCorona`, `NoLightSquare`). Del lado del port, **ninguna** de las 20 claves de INI existe
hoy en `src/`: el port tiene su propia capa de escalado (`SCREEN_STRETCH_*`/`SCREEN_SCALE_*`) y su propia
gestión de aspecto/FOV, pero no la granularidad del mod.

---

## 1. Ficha del artefacto

### 1.1 El ASI (lo que hay que portar)

| Campo | Valor | Cómo se midió |
|---|---|---|
| Ruta | `mods/widescreen/modloader/Widescreen Fix/GTAVC.WidescreenFix.asi` | — |
| sha256 | `83d5113e19a7b2bbf2217c58fb3ccd41b2a27f38638b956d469790c555aad33b` | `hashlib` + REA `open_binary` (coinciden) |
| Tamaño | **320 512 B** (313,0 KiB) | cabecera PE |
| Formato | PE32, x86 (machine `0x014C`), DLL, subsystem 2 (GUI), DLL chars `0x2102` | cabecera PE |
| Image base | **`0x10000000`** | cabecera PE |
| Entry point | RVA **`0x00014E1D`** → VA `0x10014E1D` | cabecera PE |
| Timestamp PE | **1538063614 → 2018-09-27 22:33:34Z** | cabecera PE |
| Secciones | `.text` `0x1000`/vsize `0x33598`; `.rdata` `0x35000`/`0x154BA`; `.data` `0x4B000`/`0x2A80`; `.rsrc` `0x4E000`/`0x5A8`; `.reloc` `0x4F000`/`0x31D4` | cabecera PE |
| Exports | DLL name `GTAVC.WidescreenFix.asi`; **1 export: `InitializeASI`** (RVA `0x00012580`, VA `0x10012580`) | tabla de exports leída del fichero |
| VERSIONINFO | `CompanyName=ThirteenAG`, `FileDescription=https://thirteenag.github.io/wfp`, `FileVersion=1.0.0.0`, `ProductName=GTAVC.WidescreenFix`, `LegalCopyright=MIT License`, `OriginalFilename=GTAVC.WidescreenFix.asi`, `UpdateUrl=https://github.com/ThirteenAG/WidescreenFixesPack` | recurso `.rsrc` (visto por REA `list_strings`) |
| Versión declarada | **27/09/2018** | `mods/widescreen/Readme (or die).txt` ("Version: 27/09/18", Author: ThirteenAG) y `Leiame (ou morra).txt` |
| Modloader | mixMods SA-Modloader; el ASI se carga por `scripts/global.ini` (`[GlobalSets] LoadPlugins=1`, `LoadFromScriptsOnly=0`, `UseD3D8to9=1`) | `mods/widescreen/scripts/global.ini` |

### 1.2 El INI instalado

`mods/widescreen/modloader/Widescreen Fix/GTAVC.WidescreenFix.ini` — 20 claves, **todas bajo `[MAIN]`**
(el INI no tiene más secciones). Contenido literal (claves y valores por defecto tal como se instalan):

```
ResX = 0 | ResY = 0 | ForceAspectRatio = auto | FrontendAspectRatio = auto
HudWidthScale = 0.0 | HudHeightScale = 0.0 | RadarWidthScale = 0.0 | SubtitlesScale = 0.0
SmallerTextShadows = 1 | FixVehicleLights = 1 | RestoreCutsceneFOV = 1 | CarSpeedDependantFOV = 20.0
DontTouchFOV = 0 | FOVControl = 1 | HideAABug = 1 | SmartCutsceneBorders = 1
IVRadarScaling = 1 | ReplaceTextShadowWithOutline = 2 | ForceMultisamplingLevel = 0
AllowAltTabbingWithoutPausing = 1
```

### 1.3 Lo que el binario lee de verdad (medido sobre el `.rdata` del ASI)

Escaneo ASCII/UTF-16 propio del `.rdata` (offset de fichero `0x33A00`, vaddr `0x35000`) y contraste con
las referencias en `.text` (se busca el `imm32` de la VA de la cadena). Resultado:

* **Sección INI leída: sólo `[MAIN]`.** En el binario **no existe** la cadena `"MISC"` ni `"GRAPHICS"`
  (búsqueda ASCII en todo el fichero: 0 aciertos). El master, en cambio, lee muchas claves de `"MISC"`
  (`Legacy.ixx:339-346`) y de `"GRAPHICS"` (`MSAA.ixx:44-45`). **Discrepancia 1**: el binario instalado
  es *todo* `[MAIN]`.
* **Las 20 claves del INI están presentes** en `.rdata` con referencia en código (1 uso cada una):

| Clave | offset `.rdata` | VA | ref en `.text` | tipo de lectura (decompilado) | default en el código |
|---|---|---|---|---|---|
| `ResX` | `0x34848` | `0x10035E48` | `0x10010356` | entero | `-1` |
| `ResY` | `0x34850` | `0x10035E50` | `0x10010389` | entero | `-1` |
| `HudWidthScale` | `0x34B8C` | `0x1003618C` | `0x10009D47` | float | `0.0` → **`0.8`** (`0x3F4CCCCD`) |
| `HudHeightScale` | `0x34B9C` | `0x1003619C` | `0x10009DAA` | float | `0.0` → **`0.8`** (`0x3F4CCCCD`) |
| `RadarWidthScale` | `0x34BBC` | `0x100361BC` | `0x1000A14F` | float | `0.0` → **`0.9`** (`0x3F666666`) |
| `SubtitlesScale` | `0x34C14` | `0x10036214` | `0x1000A4B8` | float | `0.0` → **`0.8`** (`0x3F4CCCCD`) |
| `IVRadarScaling` | `0x34BAC` | `0x100361AC` | `0x1000A10F` | entero | `0` |
| `RestoreCutsceneFOV` | `0x34CA4` | `0x100362A4` | `0x1000A6FF` | entero | `1` |
| `CarSpeedDependantFOV` | `0x34CB8` | `0x100362B8` | `0x1000A73F` | **float** | `0.0` |
| `DontTouchFOV` | `0x34CD0` | `0x100362D0` | `0x1000A786` | entero | `0` |
| `ForceAspectRatio` | `0x34CE8` | `0x100362E8` | `0x1000A7CA` | string | `"auto"` (`0x100362E0`) |
| `FOVControl` | `0x34CFC` | `0x100362FC` | `0x1000A994` | entero | `1` |
| `HideAABug` | `0x34D1C` | `0x1003631C` | `0x1000AA5F` | entero | `0` |
| `SmartCutsceneBorders` | `0x34D44` | `0x10036344` | `0x1000AB20` | entero | `1` |
| `ReplaceTextShadowWithOutline` | `0x34D5C` | `0x1003635C` | `0x1000ABD2` | entero | `0` |
| `ForceMultisamplingLevel` | `0x34EA8` | `0x100364A8` | `0x1000AFE5`, `0x1000B085` | string/entero | `""` / `0`; literal `"max"` en `0x100364C0` |
| `FixVehicleLights` | `0x34EF8` | `0x100364F8` | `0x1000B178` | entero | `1` |
| `SmallerTextShadows` | `0x350D4` | `0x100366D4` | `0x1000B780` | entero | `1` |
| `AllowAltTabbingWithoutPausing` | `0x350E8` | `0x100366E8` | `0x1000B82C` | entero | `0` |
| `FrontendAspectRatio` | `0x35134` | `0x10036734` | `0x1000BCA8` | string | `"auto"` |
| `FED_WIS` | `0x3522C` | `0x1003682C` | `0x10009B50` | literal | + `L"BORDERS"` (UTF-16, `0x35234`) |

* **No existen** en el binario: `SmallerVehicleCorona`, `NoLightSquare`, `VCSCamShake`, `HQRadarDisc`
  (0 coincidencias ASCII en todo el fichero). `SmallerVehicleCorona` y `NoLightSquare` **sí** existen en el
  master (`Legacy.ixx:342-343`) → **features POSTERIORES al binario instalado**.
* Constantes float del `.rdata` confirmadas byte a byte: `0x100368EC = 0.58f`, `0x100368F0 = 0.8f`,
  `0x100368F4 = 1.2f`, `0x100368F8 = 1.22f` (los multiplicadores de subtítulos y el alto del HUD).

### 1.4 El juego retail (destino de los parches)

| Campo | Valor |
|---|---|
| Ruta | `C:\Users\s0rno\Desktop\Gamez\(g)old\Grand Theft Auto Vice City\gta-vc.exe` |
| sha256 | `acc6a54681b3c5ce996c956b71be9e4c1315c3b66aea37b896afc913fe28e868` (verificado) |
| Tamaño | 3 088 896 B |
| PE | PE32 x86, **image base `0x400000`**, entry RVA `0x00268560`, timestamp 1347250503 = 2012-09-10 |
| Secciones | `.text` vaddr `0x1000` raddr `0xC00`; `.rdata` `0x27E000`/`0x27D000`; `.data` `0x282000`; `.bss` `0x2F5000`; `.edata` `0x610000` |
| Exports | 4, todos de CDAPFN (`?CDAPFN0506_...`) — el retail **no exporta símbolos de gameplay** |

> Convención del recibo: `RVA` = offset virtual desde el image base; `VA` = `0x400000 + RVA`.
> Offset de fichero de `.text` = `RVA - 0x1000 + 0xC00`.

---

## 2. Mapa fuente público vs binario instalado

### 2.1 Método (medido)

1. Se extrajeron los **106 patrones** que el binario lleva en su `.rdata` (fichero ya existente
   `gta_vc_browser/tmp/extsrc/wfp_binary_patterns.txt`, uno por línea, `?` = comodín).
2. Se extrajeron los **356 literales de patrón** del master (`source/GTAVC.WidescreenFix/*.ixx` +
   `dllmain.cpp`), normalizados a mayúsculas y con `??`→`?`.
3. Comparación **exacta token a token** y, además, comprobación de **subcadena** (un patrón del binario
   puede estar embebido en uno más largo del master).
4. Para los que no salen: búsqueda del equivalente más cercano en el master por contexto, y **escaneo del
   retail** con el algoritmo del propio mod (máscara `?`) para obtener offset/RVA/VA reales.
5. Para el retail: proyecto Ghidra headless ya existente
   (`gta_vc_browser/tmp/rea/ghidra_vanilla`, programa `gta-vc.exe`) + script propio `R2WhoFile` para
   resolver la función que contiene cada sitio.

### 2.2 Resultado medido

| Categoría | Nº | Observación |
|---|---|---|
| Patrones del binario con **equivalente exacto** en el master | **96 / 106** | todos en `Legacy.ixx` salvo 3 (`dllmain.cpp:124`, `Misc.ixx:44` y `Misc.ixx:47`) |
| Patrones del binario con **variante de 1 byte** (ModRM) | **1 / 106** | P42 (§2.3) |
| Patrones del binario **sin equivalente de ningún tipo** | **9 / 106** | P40, P76, P88, P89, P91, P93, P94, P95, P97 (§2.3) |
| Patrones del master que el binario **no** tiene | **244** (241 fuera de `Legacy.ixx`) | §2.4 |

> **Discrepancia con el enunciado (95/11 y 109).** Medido aquí: **96 exactos + 1 variante de 1 byte +
> 9 sin equivalente = 106**, y **244 patrones sólo en el master**. La diferencia con 95/11 y 109 es de
> método (el enunciado parece contar "equivalente" en sentido más estricto y comparar contra un subconjunto
> de ficheros del master). Con las cifras medidas, el binario instalado es **casi idéntico** al camino
> `Legacy.ixx` del master, lo que confirma que esta build es la "legacy" y no la reescritura modular.

### 2.3 Los 10 patrones del binario sin coincidencia exacta — qué feature representa cada uno

| # | Patrón (texto del binario) | Retail medido (offset/RVA/VA) | Función retail (Ghidra) | Feature del binario | Equivalente más cercano en el master | Veredicto |
|---|---|---|---|---|---|---|
| P40 | `0F B6 05 ? ? ? ? 6B C0 2E 8B 0C C5 ? ? ? ? 85 C9 74 10` | `0x000BBE00` / `0x000BC200` / **`0x4BC200`** | `FUN_004BC200` (40 B) | **Hook de la pantalla de carga de interior** (tabla de anchos de glifo: `movzx eax, byte[0xA10B03]; imul eax,0x2E; mov ecx,[eax*8+…]`). El ASI guarda la dirección de esa función (`DAT_1004CBDC = *match`) para reescribir la métrica de texto | `InteriorLoading.ixx:634` contiene `6B C0 ?` y `8B 04 C5 ? ? ? ?` (misma aritmética, 46 glifos) | **POSTERIOR**: en el master esta feature vive en el módulo nuevo `InteriorLoading.ixx`; la build legacy del master **no** la tiene |
| P42 | `53 83 EC 10 8B 5D 24 18 8A 13` | **0 coincidencias en el retail 1.0** | — (patrón roto) | **Dirección de la función `SetColor` del motor**, necesaria para el contorno del texto (feature 18). Con `8B 5D` (EBP) no encuentra nada; con `8B 5C` (ESP) hay 2: `0x54FF40` y `0x57D590` | `Legacy.ixx:432` → `53 83 EC 10 8B 5C 24 18 8A 13` (**ESP**) | **Discrepancia**: el binario apunta al prólogo de otro build (Steam 1.1). En el retail 1.0 el puntero queda sin resolver → feature 18 degradada |
| P76 | `DE D9 DE D9 EB E9` | `0x001762A0` / `0x001766A0` / **`0x5766A0`** | borde de `FUN_00576210` (1168 B) | **FOVControl**: escribe el float **`1.0f` (`0x3F800000`)** en `match+6` = **`0x5766A6`** (constante de escala del FOV). Es el único patrón **sin equivalente en el master** que sí usa una clave del INI | — (el master declara `bool bFOVControl;` en `Legacy.ixx:155` pero **nunca lo lee ni lo usa**) | **PROPIO de esta build**. El comentario del INI dice "writing float to 0x576686": la dirección medida es `0x5766A6` (+0x20), el comentario está **desactualizado** |
| P88 | `D8 05 ? ? ? ? DE C1 D9 5C 24 28` | `0x000C179E` / `0x000C1B9E` / **`0x4C1B9E`** | `FUN_004C1A40` (820 B) | **IVRadarScaling**: escala del radar (sitio A). En el binario escribe `70.0f` (`0x428E0000`) | `Legacy.ixx:414-415` deriva `CRadarPattern` del operando de `MenuPattern6` en vez de usar un patrón propio | **VARIANTE 2018**: el master unifica en un patrón derivado |
| P89 | `83 EC 50 DD D9 D9 05 ? ? ? ? D8 C9` | `0x000C25B1` / `0x000C29B1` / **`0x4C29B1`** | `FUN_004C29A0` (877 B) | **IVRadarScaling**: escala del radar (sitio B, el `MenuPattern6` de `0x4C29AB` es su hermano) | ídem | **VARIANTE 2018** |
| P91 | `D9 05 ? ? ? ? D8 CB DA 2C 24 DE C1` | `0x000C17D4` / `0x000C1BD4` / **`0x4C1BD4`** | `FUN_004C1A40` | **IVRadarScaling**: escala del radar (sitio C) | ídem | **VARIANTE 2018** |
| P93 | `C7 84 24 80 04 00 00 00 00 08 42` | `0x0015A56F` / `0x0015A96F` / **`0x55A96F`** | `FUN_00557340` (18850 B) | **IVRadarScaling**: constante `34.0f` (`0x42080000`) en `[esp+0x480]` | `Legacy.ixx:983` escribe `76+5 = 81.0f` en un patrón equivalente | **VARIANTE 2018** (valor distinto) |
| P94 | `C7 84 24 A0 04 00 00 00 00 08 42` | `0x0015A6AD` / `0x0015AAAD` / **`0x55AAAD`** | `FUN_00557340` | **IVRadarScaling**: constante `34.0f` en `[esp+0x4A0]` | ídem | **VARIANTE 2018** |
| P95 | `D8 05 ? ? ? ? D8 05 ? ? ? ? D9 9C 24` | `0x0015A5CC` y `0x0015A705` / `0x0015A9CC` y `0x0015AB05` / **`0x55A9CC`**, **`0x55AB05`** | `FUN_00557340` | **IVRadarScaling**: ancho del anillo del radar (`FADD [0x697AA4]`) | `Legacy.ixx:987` usa el patrón largo `D8 0D … 98 04 00 00` (P96) para lo mismo | **VARIANTE 2018** |
| P97 | `D9 05 ? ? ? ? D8 CA DA 6C 24 74 51` | `0x0015A6D0` / `0x0015AAD0` / **`0x55AAD0`** | `FUN_00557340` | **IVRadarScaling**: alto del anillo del radar. El binario escribe **`108.5f` (`0x42D90000`)**; el master escribe `81.0f` | `Legacy.ixx:991` usa `D9 05 ? ? ? ? D8 CA D8 C1` (P98) con `81.0f` | **VARIANTE 2018** (valor distinto) |

**Conclusión de §2.3**: de los 10, **7 (P88, P89, P91, P93, P94, P95, P97) son una versión más agresiva
del `IVRadarScaling`** (10 sitios de parche en vez de 4), **1 (P76) es el `FOVControl`**, **1 (P40) es el
hook de carga de interior** (feature que el master sí tiene, pero en el módulo nuevo) y **1 (P42) es un
patrón roto** en el retail 1.0.

### 2.4 Los 244 patrones del master que el binario NO tiene → features POSTERIORES

Reparto medido por fichero del master (patrones únicos que no aparecen en el binario):

| Fichero master | Nº | Qué feature es POSTERIOR (no portable desde esta build) |
|---|---|---|
| `Frontend.ixx` | 66 | Reescritura del frontend: `ResXInvRefs`/`ResYInvRefs` por clase (`eCHud`, `eCRadar`, `eCMenuManager`, `eCLoadingScreen`, `eCParticle`, `eCSpecialFX`, `eCReplay`…), `ReplaceTextShadowWithOutline` por dibujo (`outlineStrength` 0.25/0.5, 4 u 8 vecinos), `SubtitlesScale` con clamp 0.25–4.0 |
| `InteriorLoading.ixx` | 28 | Reescritura de la pantalla de carga de interior (la feature P40 del binario, ya madura) |
| `Loading.ixx` | 26 | Saltar la pantalla de carga al cambiar de isla/interior (`gbNoIslandLoading`, `gbNoInteriorLoading`) |
| `Sprite2d.ixx` | 19 | `IsFullscreen`, `ComputeContentRect`/`CoverRect`, barras pillar/letterbox, fundidos limpios, `Draw` fullscreen vs UI |
| `Hud.ixx` | 12 | Anclaje del HUD a los bordes en pantalla ancha |
| `Main.ixx` | 11 | Redimensionado de la ventana al cambiar de resolución |
| `TransparentMenu.ixx` | 11 | Menús translúcidos (DX8/DX9) |
| `Menu.ixx` | 10 | Menús a pantalla completa |
| `dllmain.cpp` | 9 | Nuevo gancho de arranque con `safetyhook` (`onInitEvent`, `onGameInitEvent`, `onGameProcessEvent`, `onBeforeReset`, `onEndScene`, `onShutdownEvent`) frente al `InitializeASI` de una sola llamada de esta build |
| `Draw.ixx` | 8 | Reescritura del dibujado 2D (`CDraw`) |
| `MenuConstraint.ixx` | 8 | Restricción de resolución del menú |
| `Radardisc.ixx` | 7 | Disco del radar procedural de alta calidad (`HQRadarDisc`) |
| `MSAA.ixx` | 5 | `ForceMultisamplingLevel` y `HideAABug` por API **D3D8** (`IDirect3DDevice8::Clear` con `D3DRECT`) — el legacy hacía el `HideAABug` con `CSprite2d::DrawRect` |
| `Misc.ixx` | 4 | `CarSpeedDependantFOV` **reescrito** (hook a `Process_Cam_On_A_String`, FOV por velocidad del coche con `pow(0.98, dt)` y tope `+30`), `VCSCamShake`, sombra de subtítulos |
| `Timer.ixx` | 3 | Escala temporal del HUD |
| `PostFX.ixx` | 3 | Post-proceso |
| `Sprite.ixx` | 3 | Escalado de sprites |
| `CutsceneMgr.ixx` | 2 | Barras del modo cinemática según aspecto |
| `Skeleton.ixx` | 2 | Parcheos de esqueleto |
| `Camera.ixx` | 1 | `FOVManager` (multiplicadores de FOV por hash: `SetFOVMultiplier`/`RemoveFOVMultiplier`) |
| `Movie.ixx` | 1 | Ruta de movie separate + restauración de foco |
| `Vehicle.ixx` | 1 | Luces de vehículo |
| `VisibilityPlugins.ixx` | 1 | Plugins de visibilidad |
| `Legacy.ixx` | 3 | Los 3 son **comentados en el master**: `Legacy.ixx:363` (`3D 80 02 00 00`), `Legacy.ixx:939` (textbox) y `Legacy.ixx:432` (P42, la variante `8B 5C`) |

**Traducción operativa**: de todo lo que el master sabe hacer, **el binario instalado sólo trae la
generación "legacy"**. Todo lo de los módulos nuevos (`Frontend`, `Sprite2d`, `Hud`, `Menu`,
`TransparentMenu`, `Loading`, `InteriorLoading`, `Radardisc`, `MSAA`, `Camera/FOVManager`) es
**posterior** y, por tanto, **no se puede portar "desde esta build"**: se puede portar **desde el master**,
que es fuente pública independiente. Este recibo las marca como **fuera de alcance de la build instalada**.

---

## 3. Inventario de TODAS las features del binario instalado

32 features. Las 20 primeras tienen clave de INI; las 12 restantes son comportamiento **sin clave**
(siempre activo o condicionado por otras features).

### Resumen

| # | Feature | Clave INI | Estado en el port |
|---|---|---|---|
| 1 | Forzar resolución horizontal (`ResX`) | `ResX` | YA ESTABA (equivalente) |
| 2 | Forzar resolución vertical (`ResY`) | `ResY` | YA ESTABA (equivalente) |
| 3 | Aspecto de_ratio forzado | `ForceAspectRatio` | PARCIAL |
| 4 | Aspecto del frontend | `FrontendAspectRatio` | YA ESTABA (equivalente) |
| 5 | Escala horizontal del HUD | `HudWidthScale` (0.8) | PARCIAL |
| 6 | Escala vertical del HUD | `HudHeightScale` (0.8) | PARCIAL |
| 7 | Escala del radar | `RadarWidthScale` (0.9) | PARCIAL |
| 8 | Escala de subtítulos | `SubtitlesScale` (0.8) | A PORTAR |
| 9 | Sombras de texto más pequeñas | `SmallerTextShadows` | A PORTAR |
| 10 | Luces de vehículo más pequeñas | `FixVehicleLights` | A PORTAR |
| 11 | FOV original en cinemáticas | `RestoreCutsceneFOV` | YA ESTABA |
| 12 | FOV dependiente de la velocidad | `CarSpeedDependantFOV` | A PORTAR |
| 13 | No tocar el FOV | `DontTouchFOV` | PARCIAL |
| 14 | Control de FOV (float 1.0 en la escala) | `FOVControl` | A PORTAR |
| 15 | Ocultar el bug de AA de 1 px | `HideAABug` (0/1/2) | NO APLICA |
| 16 | Bordes de cinemática inteligentes | `SmartCutsceneBorders` | PARCIAL |
| 17 | Escala de radar al estilo GTA IV | `IVRadarScaling` | A PORTAR |
| 18 | Contorno en vez de sombra de texto | `ReplaceTextShadowWithOutline` (0/1/2) | PARCIAL |
| 19 | Forzar nivel de multisampling | `ForceMultisamplingLevel` | NO APLICA |
| 20 | Alt-Tab sin pausar | `AllowAltTabbingWithoutPausing` | NO APLICA |
| 21 | Escalado de todos los textos de menú | (sin clave) | YA ESTABA (equivalente) |
| 22 | Escalado del HUD (todos los elementos) | (sin clave) | YA ESTABA (equivalente) |
| 23 | Fijar la retícula (posición y escala) | (sin clave) | PARCIAL |
| 24 | Bordes de cinemática (motor del ajuste) | (sin clave) | PARCIAL |
| 25 | FOV de vehículos de emergencia | (sin clave) | PARCIAL |
| 26 | Cálculo de aspecto propio | (sin clave) | YA ESTABA (equivalente) |
| 27 | FOV propio (widescreen) | (sin clave) | YA ESTABA (equivalente) |
| 28 | Arreglo de ratón "exclusive" | (sin clave) | NO APLICA |
| 29 | Gancho de estado de carga (cambios tardíos) | (sin clave) | PARCIAL |
| 30 | Renombrar "WIDESCREEN" → "BORDERS" | (sin clave) | YA ESTABA (equivalente) |
| 31 | Sprites 2D con aspecto de frontend | (sin clave) | PARCIAL |
| 32 | Tamaño de corona de luz (corona de vehículo) | (sin clave) | YA ESTABA |

### Detalle por feature

**1 · Forzar resolución horizontal** — clave `ResX` (int, default `-1`).
Patrón: `E8 ? ? ? ? 85 C0 74 0A B8 01 00 00 00 E9 C1 00 00 00` (P7) y `83 3D ? ? ? ? 00 59 59 74 2B 81 3C 24` (P9),
`C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 80 02 00 00` (P10), `85 DB 75 0D E8 ? ? ? ? 39 05 ? ? ? ? 7C A4` (P11),
`A3 ? ? ? ? E9 84 00 00 00` (P12), `8D 45 60 83 C4 0C 6A 01 50 53 E8 ? ? ? ? 8D 85 5B 01 00 00` (P8).
Fuente público equivalente: `Legacy.ixx:367-372` y `:473-498` (`OverwriteResolution`).
Retail medido: P7 → `0x602F00` (FUN `00602EC0`); P8 → `0x4900A5` (FUN `0048FE86`); P9/P11/P12 → `0x600E90`/`0x600ECB`/`0x600E72`
(FUN `00600DC0` = `RsSelectDevice`); P10 → `0x602D10` y `0x602D24` (FUN `00602D10`).
Qué hace: escribe `ResX` en los campos de modo de vídeo del retail (`0x600E7B+3` y `0x602D3A+6`/`0x602D4E+6`),
convierte el `cmp`/`jl` de validación en `jmp` y anula la rama que forzaba 640×480.
Estado: **YA ESTABA (equivalente)** — el port no necesita "forzar" nada: el canvas del navegador *es* la
resolución de render. Evidencia: `src/core/re3.cpp:672-673` (`[VideoMode] Width/Height`),
`src/skel/glfw/glfw.cpp:912-913` (`m_nPrefsWidth/Height` → tamaño de ventana).

**2 · Forzar resolución vertical** — clave `ResY` (int, default `-1`). Mismos patrones que #1.
Retail: idénticos sitios. Qué hace: idem #1 en Y (`0x600E84+4`, `0x602D44+6`, `0x602D58+6`).
Estado: **YA ESTABA (equivalente)** (misma evidencia que #1).

**3 · Aspecto de ratio forzado** — clave `ForceAspectRatio` (string, default `"auto"`; formato `W:H`).
Patrón: `80 3D ? ? ? ? 00 C7 05 ? ? ? ? 00 00 00 00 74 21` (P53).
Fuente: `Legacy.ixx:500-504` (`FixAspectRatio`) + `:900-906` (parseo de `W:H`).
Retail medido: `0x54A290` (FUN `0054A290`, 90 B) — es la función de cálculo de aspecto del retail, a la que
el ASI **salta** (`MakeJMP`) para imposing su propio valor; y `0x4A5EC1` (FUN `004A5DA0`) donde vive la
variable de aspecto (`PUSH [0x0094DD40]`).
Qué hace: sustituye el cálculo de aspecto del juego (`MaxW/MaxH` o 4:3 fijo) por `W/H` del INI, y de ahí
sale `fWideScreenWidthScaleDown = (1/640) / (aspecto/(4/3))`, que es el factor que se inyecta en ~100
puntos de escala del juego.
Estado: **PARCIAL** — el port ya tiene su propio aspecto (`AR_AUTO/4:3/5:4/16:10/16:9/21:9`,
`src/renderer/Draw.cpp:35-52`, `src/core/Frontend.cpp:1415-1436`), pero **no** admite un `W:H` arbitrario
como el mod. Fichero destino: `src/renderer/Draw.cpp:35` (`FindAspectRatio`).

**4 · Aspecto del frontend** — clave `FrontendAspectRatio` (string, default `"auto"`).
Patrón: `E8 ? ? ? ? 8B 0B 83 C4 14 85 C9` (P100) + `A1 ? ? ? ? 83 EC 08 83 38 00` (P101).
Fuente: `Legacy.ixx:1060-1078` (`Fix2DSprites`).
Retail medido: `0x57867C` y `0x578740` (FUN `00578660` / `00578730`) — las dos llamadas a `SetVertices`
del dibujado 2D; `0x649BF0` (FUN `00649BF0`) = `RwRenderStateSet`.
Qué hace: intercepta el quad *fullscreen* del dibujado 2D, lo recorta a la relación de aspecto del
frontend (del INI, o la del propio raster si es cuadrada: 4:3) y pinta las barras laterales negras.
Estado: **YA ESTABA (equivalente)** — el port recorta y pinta barras en `CSprite2d::Draw` con textura
fullscreen bajo `VICEEXT_FIX_WFP`: `src/renderer/Sprite2d.cpp:115-139` (content-rect por aspecto del raster
+ `DrawRect` de barras). La **variante "FrontendAspectRatio = W:H"** (elegir el aspecto del raster a
fuerza) no está: el port usa el aspecto real del raster (`Sprite2d.cpp:123-127`).

**5 · Escala horizontal del HUD** — clave `HudWidthScale` (float, default efectivo **0.8**).
Patrón: `D8 0D ? ? ? ? D8 ? ? ? ? ? D9 1C 24 E8` (P27, `DrawHudHorScalePattern`).
Fuente: `Legacy.ixx:334` (lectura), `:829-836` (aplicación), `:1091` (`fCustomWideScreenWidthScaleDown`).
Retail medido: P27 → 51 coincidencias; la que usa el mod es la **9ª** (índice 8) → **`0x55678E`**
(FUN `00556700`).
Qué hace: multiplica el factor de escala del HUD por `HudWidthScale`; las exclusiones medidas en el
binario son **exactamente** las del master: índices `>3` y `≠ 20, 66, 67, 15, 16, 17` (los 15/16/17 son
la retícula). **Aviso**: con 51 coincidencias del patrón largo en el retail 1.0, los índices 66 y 67
(los dos slots del radar) **no existen** → en retail 1.0 esas dos escrituras simplemente no ocurren
(la lista de exclusiones viene de un build con más coincidencias, p.ej. Steam 1.1). Ver R13.
Estado: **PARCIAL** — el port escala el HUD con `SCREEN_SCALE_X/Y` (`src/core/common.h:195-217`), pero
**no** tiene un factor multiplicador configurable tipo "0.8× el HUD". Fichero destino: `src/renderer/Hud.cpp`.

**6 · Escala vertical del HUD** — clave `HudHeightScale` (float, default efectivo **0.8**; el master legacy
usa `1.0714285`, el binario instalado usa **0.8** — discrepancia medida: `DAT_1004CB40 = 0x3F4CCCCD`).
Patrón: `D8 0D ? ? ? ? D8 8D ? ? ? ? D8 0D` (P28, `DrawHudVerScalePattern`).
Retail medido: P28 → 1 coincidencia → **`0x556772`** (FUN `00557340`); exclusiones medidas: índices `>1` y
`≠ 18, 57, 58, 13, 14, 15`.
Estado: **PARCIAL** — mismo motivo que #5.

**7 · Escala del radar** — clave `RadarWidthScale` (float, default efectivo **0.9**).
Patrón: `D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24` (P25, `CDarkelDrawMessagesPattern`) y
`D8 0D ? ? ? ? DD DA D9 05 ? ? ? ? D8 CA` (P71) / `D8 0D ? ? ? ? DD DA D9 C0 D8 CA` (P72) (marcador del jugador).
Fuente: `Legacy.ixx:336`, `:867-886`, `:1094-1097`.
Retail medido: P25 → 105 coincidencias; P71 3ª → `0x4C431A` (FUN `004C4220`); P72 2ª → `0x4C549D` (FUN `004C5190`).
Qué hace: escala del disco/blips del radar con `fWideScreenWidthScaleDown * RadarWidthScale` y mueve el
marcador "you are here" a `94 * RadarWidthScale`.
Estado: **PARCIAL** — el port escala la posición con `SCREEN_SCALE_X` (`src/core/Radar.cpp:1577-1581`) y
dibuja el disco con `SCREEN_SCALE_X(RADAR_WIDTH)` (`src/renderer/Hud.cpp:1370-1372`), pero **no** tiene el
multiplicador `0.9`. Fichero destino: `src/core/Radar.cpp` / `src/core/Radar.h:181-182`.

**8 · Escala de subtítulos** — clave `SubtitlesScale` (float, default efectivo **0.8**).
Patrones: `D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59` (P73), `D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50` (P74),
`D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? A1 ? ? ? ? 59` (P75).
Fuente: `Legacy.ixx:337`, `:888-898`.
Retail medido: P73 7ª → `0x556A22` (FUN `00556700`); P74 23ª → `0x55B030` (FUN `00557340`);
P75 1ª → `0x55AF7E` (FUN `00557340`).
Qué hace: escribe `1.2 * SubtitlesScale`, `1.22 * SubtitlesScale` y `0.58 * SubtitlesScale` en las tres
constantes de escala del texto de subtítulos (constantes confirmadas byte a byte en el `.rdata` del ASI:
`1.2f`, `1.22f`, `0.58f`).
Estado: **A PORTAR** — el port dibuja subtítulos en `src/renderer/Hud.cpp:1500-1530` pero sin factor de
tamaño configurable. Fichero destino: `src/renderer/Hud.cpp:1500`.

**9 · Sombras de texto más pequeñas** — clave `SmallerTextShadows` (int, default **1**).
Patrón: `8B 44 24 04 66 A3 ? ? ? ? C3` (P44).
Fuente: en el master esta clave **no existe**; el master engancha el mismo patrón **sólo** para
`ReplaceTextShadowWithOutline` (`Legacy.ixx:995-1000`).
Retail medido: P44 → `0x54FF40` y `0x57D590` (FUN `0054FF40` / `0057D590`); es el *setter* de la posición
de la sombra (`mov ax, [arg]; mov [addr], ax`).
Qué hace (medido en el decompilado del ASI): si `SmallerTextShadows != 0` **o** `ReplaceTextShadowWithOutline != 0`,
salta al patrón P44 y lo reescribe con un `SetDropShadowPosition` propio que **fuerza la posición 1** en
lugar de 2 (la sombra grande) o 0 (sin sombra).
Estado: **A PORTAR** — el port ya tiene `CFont::SetDropShadowPosition` (`src/renderer/Font.cpp:370`, `:571`;
declaración en `src/renderer/Font.h`) y llama a 2/0 según el contexto, pero no tiene el "1" (sombra pequeña)
ni una opción para ello. Fichero destino: `src/renderer/Font.cpp` + `src/core/SilentPatchOptions.cpp`.

**10 · Luces de vehículo más pequeñas** — clave `FixVehicleLights` (int, default **1**).
Patrón: `D9 C2 DE CB D9 C2 DE CB D9 05` (P87).
Fuente: en el master esta clave **no existe**; el master la llama `SmallerVehicleCorona`
(`Legacy.ixx:342`, `:970-974`) y usa **el mismo patrón P87** escribiendo **2.0f**.
Retail medido: `0x543A60` (FUN `00543520`, 3075 B).
Qué hace (medido en el decompilado del ASI): escribe el float **`2.0f` (`0x40000000`)** en el operando del
patrón (match+10 = `0x543A6A`), que es el factor de estirado de las luces/coronas de los vehículos.
Estado: **A PORTAR** — el port dibuja las flares de corona con `spritew * size * wscale` (`src/renderer/Coronas.cpp:379-394`),
pero **no** aplica el factor 2.0 a las luces de vehículo. Fichero destino: `src/renderer/Coronas.cpp`.

**11 · FOV original en cinemáticas** — clave `RestoreCutsceneFOV` (int, default **1**).
Patrón de contexto: `80 3D ? ? ? ? ? 74 04 83 C4 38 C3` (P41, variable `bIsInCutscene`).
Fuente: `Legacy.ixx:206-209` (`if (*bIsInCutscene && bRestoreCutsceneFOV) → AdjustFOV(factor, aspecto, min(aspecto,16/9))`).
Retail medido: `0x4A9683` (FUN `004A9680`); la variable de FOV del juego está en `0x46DDA3` (FUN `0046C58C`,
`FDIV float ptr [0x00696658]`).
Qué hace: en cutscene, el FOV no se estira con el aspecto (se queda en el 4:3 original, limitado a 16:9).
Estado: **YA ESTABA** — `src/renderer/Draw.cpp:97-106` (`CDraw::SetFOV`): `if (!CCutsceneMgr::IsRunning()) ms_fScaledFOV = ConvertFOV(fov); else ms_fScaledFOV = fov;`
(es decir: en cinemática el FOV **no** se convierte).

**12 · FOV dependiente de la velocidad** — clave `CarSpeedDependantFOV` (float, default `0.0`).
Patrón: **ninguno** (no usa patrón de bytes: es un valor float leído y aplicado en runtime).
Fuente: en el master esta feature **sí** existe pero reescrita (`Misc.ixx:32`, `:60-89`: hook a
`Process_Cam_On_A_String`, `FOV += (velocidadFrontal - 0.4) * dt`, relajación `pow(0.98, dt)`, tope `+30°`).
Retail medido: la clave se lee en el ASI en `0x1000A73F` y se guarda en `DAT_1004CB84` (float);
**no hay patrón de parche asociado** (el bloque de FOV de esta build no engancha la cámara: medido — tras la
lectura no se construye ningún patrón hasta `P76`).
Qué hace (medido): el valor se usa como factor de FOV dependiente de la velocidad; **cuál es exactamente
la fórmula en esta build no se ha medido** (el bloque que lo consume no está en las funciones
descompiladas del ASI en esta sesión) → marcado como **supuesto**, apoyándose en el comentario del INI
("FOV will be increased at high speeds. The lower the value, the higher FOV will be. Set to 0.0 to disable")
y en la reescritura equivalente del master.
Estado: **A PORTAR** — el port tiene `Cams[ActiveCam].FOV` y `m_fFovSpeedOverOneFrame` (`src/core/Cam.cpp:966`),
pero **no** aplica un FOV por velocidad de coche. Fichero destino: `src/core/Cam.cpp` / `src/core/Camera.cpp`.

**13 · No tocar el FOV** — clave `DontTouchFOV` (int, default `0`).
Patrón: ninguno propio; se combina con `P54` (`D9 44 24 04 D9 1D ? ? ? ? C3`, retail `0x54A300`, FUN `0054A300`),
que es el retorno del ajuste de FOV del retail al que el ASI salta.
Fuente: `Legacy.ixx:204-205`.
Retail medido: `0x54A300`.
Qué hace: si está activo, el FOV se escribe **tal cual** (sin `AdjustFOV` por aspecto); si no, se estira con
el aspecto y, en cutscene, se recorta a 16:9.
Estado: **PARCIAL** — el port convierte el FOV siempre (`Draw.cpp:99-104`); no hay un interruptor para
dejar el FOV intacto. Fichero destino: `src/renderer/Draw.cpp:97`.

**14 · Control de FOV (escritura de float)** — clave `FOVControl` (int, default **1**).
Patrón: `DE D9 DE D9 EB E9` (P76).
Fuente: **no existe en el master** (`Legacy.ixx:155` declara `bool bFOVControl;` pero nunca lo lee).
Retail medido: `0x5766A0`; la escritura va a `match+6` = **`0x5766A6`** (bytes del retail:
`0x5766A0: DE D9 DE D9 EB E9 00 00 00 00 …`; el `eb e9` de `0x5766A2` salta hacia atrás a `0x57668F`).
Qué hace (medido): escribe el float **`1.0f` (`0x3F800000`)** en la constante de escala del FOV → deja el FOV
en su valor neutro (sin estirar). El comentario del INI ("writing float to 0x576686") **no** coincide con la
dirección medida (`0x5766A6`, +0x20): comentario desactualizado.
Estado: **A PORTAR** — el port ya tiene un interruptor equivalente (`DontTouchFOV` no; pero `Draw.cpp:99-104`
tiene la rama "cutscene vs normal"). Lo que falta es el control granular del valor de FOV.
Fichero destino: `src/renderer/Draw.cpp:97`.

**15 · Ocultar el bug de antialiasing de 1 px** — clave `HideAABug` (int, default `0`; el INI instalado pone `1`).
Patrón: `E8 ? ? ? ? A1 ? ? ? ? 50 E8` (P77), 16º uso.
Fuente: `Legacy.ixx:908-912` + `:263-272` (`Hide1pxAABug`).
Retail medido: P77 → 38 coincidencias; la que usa el mod es la **16ª** → `0x4A6D71` (FUN `004A69F0`,
`CALL 0x57FAC0`).
Qué hace: dibuja un rectángulo negro de 1 px arriba y otro de 1 px a la izquierda (con `CSprite2d::DrawRect`,
`CRect(0,-5,W,0.5)` y `CRect(-5,-1,0.5,H)`); con valor **2** añade los otros dos lados (4 líneas simétricas).
Es un parche para el **MSAA de D3D8**, que dejaba 1 px sin cubrir en el borde del backbuffer.
Estado: **NO APLICA** — el bug es del multisampling de Direct3D 8; el port renderiza con WebGL2/librw, sin
MSAA de D3D8 ni borde de backbuffer. En WebGL2 el MSAA se puede hacer con `renderbufferStorageMultisample`
pero el artefacto específico (el borde de D3D8) no existe. Además, en el master esta feature **cambió de
técnica** (`MSAA.ixx:73-105`: `IDirect3DDevice8::Clear` con `D3DRECT` en vez de `CSprite2d::DrawRect`).

**16 · Bordes de cinemática inteligentes** — clave `SmartCutsceneBorders` (int, default **1**).
Patrón: `74 ? B9 ? ? ? ? E8` (P6), usos 15º y 22º.
Fuente: `Legacy.ixx:274-303` (`GetScreenRect` + `CCamera::DrawBordersForWideScreen`) y `:914-918`.
Retail medido: P6 15º → `0x4A6207` (FUN `004A61B0`); 22º → `0x54A23C` (FUN `005447B7`, 23248 B).
Qué hace: en vez de un borde fijo, calcula el rectángulo útil con
`dScreenHeightWeWannaCut = (-9/16)*aspecto + 1` y
`dBorderProportionsFix = (-144643/50000)*aspecto² + (807321/100000)*aspecto - (551143/100000)`,
y pinta letterbox (si el aspecto es menor que 16:9) o pillarbox (si es mayor).
Estado: **PARCIAL** — el port tiene el **interruptor** (`CUTSCENE_BORDERS_SWITCH`, `src/core/config.h:696`;
`CMenuManager::m_PrefsCutsceneBorders`, `src/core/Frontend.cpp:125`; usado en `src/core/Camera.cpp:2279-2280`,
`src/core/main.cpp:1831-1836`, `src/renderer/Credits.cpp:801-802`, `src/renderer/Hud.cpp:1513-1514`),
pero la geometría es **fija al 30 %** (`Camera.cpp:2790-2792`), no adaptativa al aspecto.
Fichero destino: `src/core/Camera.cpp:2786`.

**17 · Escala de radar al estilo GTA IV** — clave `IVRadarScaling` (int, default `0`; el INI instalado pone `1`).
Patrones: **10** — P88, P89, P91, P93, P94, P95, P97 (sólo en esta build) + P90, P92, P96, P98 (comunes).
Fuente: `Legacy.ixx:976-993` (sólo 4 en el master).
Retail medido: `0x4C1B9E` (P88), `0x4C29B1` (P89), `0x4C1BD4` (P91), `0x55A96F` (P93), `0x55AAAD` (P94),
`0x55A9CC`/`0x55AB05` (P95), `0x55AAD0` (P97), `0x4C1B84` (P90), `0x4C1BB8` (P92), `0x55A9C6` (P96),
`0x55A9DF` (P98).
Qué hace: pone el disco del radar a **ancho 88.5 (94−5.5)** y **alto 81 (76+5)**, y el anillo a
88.5×81 (en esta build, con 2 valores extra: 34.0f en dos constantes y **108.5f** en el alto del anillo).
Estado: **A PORTAR** — el port tiene `RADAR_WIDTH`/`RADAR_HEIGHT` a 94×76 (`src/core/Radar.h:181-182`, o
82×82 con `CDraw::ms_bFixRadar`, `src/renderer/Draw.cpp:27`); **no** tiene la variante 88.5×81.
Fichero destino: `src/core/Radar.h:181-182` + `src/core/Radar.cpp:1577-1581`.

**18 · Contorno en vez de sombra de texto** — clave `ReplaceTextShadowWithOutline` (int, default `0`; el INI
instalado pone `2`).
Patrones: `D8 44 24 38 D9 1C 24 E8 ? ? ? ? 83 C4 18` (P78), `66 A3 ? ? ? ? A0 ? ? ? ? D9 05` (P79),
`6A 32 6A 64 6A 64 6A 64 E8` (P80), `8D 4C 24 0C 68 FF 00 00 00 6A 00 6A 00 6A 00` (P81),
`E8 ? ? ? ? 83 C4 0C E8 ? ? ? ? 83 C4 18` (P82), `E8 ? ? ? ? 8B 85 08 01 00 00 83 C4 0C 3D FF 00 00 00` (P83),
`66 8B 2D ? ? ? ? 0F BF C5` (P49) y la cadena de "compatibilidad con SilentPatch"
(P50 `89 44 24 0C D8 05 …`, P51 `0F BF C5 D9 1C 24 …`, P52 `FF 74 24 34 …`).
Fuente: `Legacy.ixx:441-464` (`SilentPatchCompatibility`), `:758-808` (`PrintStringHook`), `:920-955`, `:995-1000`.
Retail medido: P78 → `0x55186C` (FUN `005516E0`); P79 → `0x551C62` (FUN `00551A50`); P80 → `0x4A35C3` (FUN `004A214E`);
P81 3ª → `0x5FA1C0` (FUN `005F9F00`); P82 → `0x5FA2AA`; P83 → `0x49E32F` (FUN `0049DF61`); P49 → `0x551711` (FUN `005516E0`).
Qué hace: sustituye la sombra difusa por un **contorno**: redibuja el texto 8 veces (o 4 con valor 1) a
±1 px, luego a ±2 px, con el alfa de la sombra a la mitad (`originalColorA / 2`), y anula las sombras del
cursor, de la radio y del título de menú.
Estado: **PARCIAL** — el port **declara** `CFont::PrintOutlinedString(float,float,wchar*,float,bool,CRGBA)`
(`src/renderer/Font.h:186`) pero **no** la usa en ninguna parte (grep: 1 sola coincidencia, la declaración);
el dibujo de texto con sombra está en `src/renderer/Font.cpp:918-922` (`PrintString` con `dropShadowPosition`).
Fichero destino: `src/renderer/Font.cpp`.

**19 · Forzar nivel de multisampling** — clave `ForceMultisamplingLevel` (string: `max` o número; default `""`/`0`).
Patrones: `A1 ? ? ? ? 3B C3` (P85) y `C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 01 00 00 00` (P86).
Fuente: `Legacy.ixx:959-968`.
Retail medido: P85 → `0x65C371` (FUN `0065C010`, `MOV EAX,[0x006DDE10]`) y `0x66562A`;
P86 3ª → `0x65BAF6`.
Qué hace: sobrescribe la variable "nivel de multisampling seleccionado" del motor con el valor del INI, o
con el máximo que soporte la tarjeta si el INI dice `max`.
Estado: **NO APLICA** — depende de `IDirect3DDevice8` y de `EnumDisplayModes`/`D3DERR_NOTAVAILABLE` del
Direct3D 8 (el master lo resuelve con `IDirect3DDevice8*` y `MaxMultisamplingLevels`, `MSAA.ixx:12-34`).
En WebGL2 el MSAA existe pero con otro modelo (multisampled framebuffer, no "niveles del driver"), y el
port no expone MSAA en ninguna parte (grep de `MSAA`/`multisample` en `src/`: 0 resultados). Decidirlo
requiere un rediseño, no un port.

**20 · Alt-Tab sin pausar** — clave `AllowAltTabbingWithoutPausing` (int, default `0`; el INI instalado pone `1`).
Patrón: `A0 ? ? ? ? B9 ? ? ? ? A2 ? ? ? ? A0 ? ? ? ?` (P99).
Fuente: `Legacy.ixx:1002-1006`.
Retail medido: `0x4A4FF0` (FUN `004A4FF0`, 52 B, `MOV AL,[0x008696A5]`).
Qué hace: **parchea la función a `ret`** (`mov byte [addr], 0xC3`), anulando la rutina que pausa el juego al
perder el foco.
Estado: **NO APLICA** — el "focus loss" es del navegador; la pausa la controla el bucle de la página, no un
`WM_ACTIVATE` de Windows. Además, en un port web la pestaña en segundo plano **sí** se throttlea; la
equivalente honesta sería un `visibilitychange` en el host web, que no forma parte de este mod.

### Features sin clave de INI (siempre activas o condicionadas por otras)

**21 · Escalado de todos los textos de menú** — (sin clave).
Patrones: P13 (`89 84 24 88 00 00 00 DB 84 24 88 00 00 00 D8 0D …`), P14 (`89 44 24 48 DB 44 24 48 D8 0D`),
P15 (`89 44 24 50 …`), P16 (`89 14 24 DB 04 24 D8 0D`), P17 (`89 44 24 18 …`), P18 (`D8 0D ? ? ? ? 83 EC 50 DD D9`),
P19 (`C6 83 40 01 00 00 01 C7 43 40 00 00 22 43 C7 43 44`), P20 (`89 04 24 …`), P21 (`89 44 24 08 …`),
P22 (`89 44 24 10 …`), P23 (`D8 05 ? ? ? ? DD DA D9 46 44`), P24 (`89 ? 24 20 …`).
Fuente: `Legacy.ixx:541-596` (`FixMenu`).
Retail medido (los que usa el mod, con su índice): P13 2º → `0x49E466`; P14 (índices 0, 1 y 3) → `0x443C8F`,
`0x490731`, `0x490DE4` (FUN `0049067B`); P15 36º → `0x492DBB`; P16 4º → `0x495896`; P17 5º y 11º → `0x49B443`,
`0x49B6C4`; P18 → `0x4C29AB`; P19 → `0x4A3D03`; P20 10º → `0x48F4A5`; P21 21º → `0x49A3E5`; P22 12º → `0x4987AE`;
P23 → `0x497AD1`; P24 18º y 22º → `0x4A3624`, `0x4A374A`.
Qué hace: reescribe **12 constantes float** de escala del menú, cada una divided por `(aspecto/(4/3))`:
texto del menú `0.0009375000373`, opciones `0.00062499999` y `0.00039062501`, barras `0.0046875002`,
cabeceras de skins `0.0014062499`, logo de VC `0.2453125`, leyenda del mapa `0.001015625` y
`0.000859375`, posición del mapa `(640*(aspecto/(4/3)))/2`, texto inferior del mapa `0.00046875002`,
lista de guardado/carga `0.00075000001`, texto de "cargando" `0.00065625005`, cursor `0.0546875` y
`0.0703125`.
Estado: **YA ESTABA (equivalente)** — el port escala todo el frontend con `SCREEN_STRETCH_X/Y`
(`src/core/Frontend.cpp:2926-2934` y 128 usos más), que es el mismo objetivo.

**22 · Escalado del HUD (todos los elementos)** — (sin clave).
Patrones: P25, P26, P27, P28, P29, P30, P31, P64, P65, P66, P67, P102.
Fuente: `Legacy.ixx:659-719` (`FixHUD`).
Retail medido: P25 → 105 coincidencias; la 9ª (índice 8) → **`0x42F2F5`**, y el patrón de mensajes de
`CDarkel` se deriva de la 2ª → `0x42A087`; P26 → 26 coincidencias, 1ª → `0x42A070`; P27 9ª → `0x55678E`;
P28 → `0x556772`; P29 → `0x560C82` y `0x560E03` (FUN `005608E0`); P30 19ª → `0x573F70` (FUN `00573F40`) y
22ª → `0x605E01` (dibujo de escena editable); P31 → `0x61DF59` (FUN `0061DE90`); P64/P65 → `0x4A6437`/
`0x4A648B` (bordes del visor de tirador); P66 → `0x5FA17C` (texto de la radio); P67 3ª → `0x620C26`
(FUN `00620BC0`, replay); P102 → `0x5FA17C`.
Qué hace: inyecta `fWideScreenWidthScaleDown` (y su versión vertical) en ~100 punteros de escala repartidos
por HUD, partículas, FX 2D, replay, mapa y texto de radio.
Estado: **YA ESTABA (equivalente)** — el port escala el HUD con `SCREEN_SCALE_*`
(`src/core/common.h:195-217`; 136 usos en `Hud.cpp`, 29 en `Radar.cpp`, 12 en `Font.cpp`).

**23 · Fijar la retícula (posición y escala)** — (sin clave).
Patrones: `D9 05 ? ? ? ? D8 25 ? ? ? ? 69 C0 CC 01 00 00` (P68), `D8 0D ? ? ? ? D9 5C 24 24` (P69),
`D8 0D ? ? ? ? D8 AC 24 AC 00 00 00 DD DB D9 C2 D8 C8 DE C1 D9 FA` (P70).
Fuente: `Legacy.ixx:721-740` (`FixCrosshair`).
Retail medido: P68 2ª → `0x46F925` (FUN `0046F890`); P69 8ª → `0x557618` (FUN `00557340`);
P70 → `0x606B03` (FUN `00606710`).
Qué hace: escribe `fCrosshairPosFactor = ((0.53-0.5)/(aspecto/(16/9))) + 0.5` en la posición horizontal de
la retícula y `fCrosshairHeightScaleDown = fWideScreenWidthScaleDown * aspecto` en la escala vertical (índices
13-15 del patrón de escala vertical).
Estado: **PARCIAL** — el port tiene el factor de la retícula en la cámara
(`CCamera::m_f3rdPersonCHairMultX/Y`, `src/core/Camera.cpp:283-284`) y la cruz se dibuja en
`src/renderer/Hud.cpp:370`, pero **no** aplica el ajuste por aspecto de la posición vertical/horizontal.
Fichero destino: `src/renderer/Hud.cpp` + `src/core/Camera.cpp`.

**24 · Bordes de cinemática (motor del ajuste)** — (sin clave).
Patrones: `D8 0D ? ? ? ? D8 0D ? ? ? ? DE E9 D9 5A 04` (P62), `D9 5A 0C 83 C4 08 5B C2 04 00` (P63).
Fuente: `Legacy.ixx:648-657` (`FixBorders`).
Retail medido: `0x46FDA9` y `0x46FE09` (FUN `0046FD5D`).
Qué hace: anula 12 + 3 bytes de la rutina de bordes original del retail (que dibujaba barras fijas) y la
sustituye por el cálculo adaptativo de la feature 16.
Estado: **PARCIAL** — mismo destino que #16 (`src/core/Camera.cpp:2786`).

**25 · FOV de vehículos de emergencia** — (sin clave).
Patrón: `D9 9B F4 00 00 00 D9 83 F0 00 00 00` (P3) + `E8 ? ? ? ? D9 EE D9 EE D9 44 24 38 D8 A3 7C 01 00 00 DD D9` (P55).
Fuente: `Legacy.ixx:200-202` (`fEmergencyVehiclesFix = 70.0f / fFactor`) y `:529-538`.
Retail medido: P3 → `0x46DEB7` (FUN `0046C58C`, `FSTP float ptr [EBX+0xF4]`); P55 → `0x480456` (FUN `0048FE86`).
Qué hace: la cámara de vehículo de emergencia usaba un FOV fijo (45°) que se veía demasiado cerrado en
widescreen; el mod lo recalcula a `70 / factor` y, además, salta la rutina `CCamera::AvoidTheObstacles`
cuando el aspecto ≥ 2.0.
Estado: **PARCIAL** — el port tiene `CCamera::m_fFOV_Wide_Screen` (`src/core/Camera.cpp:2779`,
`src/core/Camera.h:466`) y el salto a `AvoidTheObstacles` no está portado (no se ha medido equivalente en
`src/`). Fichero destino: `src/core/Camera.cpp` / `src/core/Cam.cpp`.

**26 · Cálculo de aspecto propio** — (sin clave).
Patrón: P53 (`80 3D ? ? ? ? 00 C7 05 ? ? ? ? 00 00 00 00 74 21`).
Fuente: `Legacy.ixx:500-504`.
Retail medido: `0x54A290` (FUN `0054A290`); la variable de aspecto del retail: `0x4A5EC1` (FUN `004A5DA0`).
Qué hace: salta al cálculo de aspecto propio (`MaxW/MaxH` o el `W:H` del INI) en vez del del juego.
Estado: **YA ESTABA (equivalente)** — `src/renderer/Draw.cpp:56-76` (`CDraw::CalculateAspectRatio`) + `:35-52`
(`FindAspectRatio`), con `ASPECT_RATIO_SCALE` (`src/core/config.h:442`).

**27 · FOV propio (widescreen)** — (sin clave).
Patrones: P54 (`D9 44 24 04 D9 1D ? ? ? ? C3`) y P55 (`E8 ? ? ? ? D9 EE D9 EE D9 44 24 38 D8 A3 7C 01 00 00 DD D9`).
Fuente: `Legacy.ixx:524-539` (`FixFOV`).
Retail medido: `0x54A300` (FUN `0054A300`, 11 B) y `0x480456` (FUN `0048FE86`).
Qué hace: sustituye el ajuste de FOV del retail por `AdjustFOV(factor, aspecto[, min(aspecto,16/9)])`.
Estado: **YA ESTABA (equivalente)** — `src/renderer/Draw.cpp:83-106` (`CDraw::ConvertFOV` + `CDraw::SetFOV`).

**28 · Arreglo de ratón "exclusive"** — (sin clave).
Patrón: `8B 0D ? ? ? ? 83 EC 10 8D 04 24 51 50` (P2).
Fuente: `Legacy.ixx:1107` (`injector::WriteMemory<uint32_t>(…, 0xC3C030)`).
Retail medido: `0x601770` (FUN `00601770`, 42 B, `MOV ECX,[0x00783EEC]`).
Qué hace: salta la rutina que pedía el ratón en modo exclusivo (necesario en D3D8 con `CreateDevice`).
Estado: **NO APLICA** — el navegador captura el ratón con Pointer Lock; no hay modo exclusivo de Windows.

**29 · Gancho de estado de carga (cambios tardíos)** — (sin clave).
Patrón: `C7 05 ? ? ? ? 09 00 00 00` (P1).
Fuente: `Legacy.ixx:1119-1154` (struct `LoadState`).
Retail medido: `0x60043D` (FUN `005FFAD0`, `MOV [0x009B5F10],0x9`).
Qué hace: cuando el juego pasa a estado de carga (< 9), aplica los cambios "tardíos": reescala el texto de
la radio y del replay, y renombra el texto del menú (feature 30). También es el punto donde se llama a
`SilentPatchCompatibility()`.
Estado: **PARCIAL** — el port ya tiene equivalentes sueltos (subtítulos en `Hud.cpp:1500`, radio/replay en
`Hud.cpp:1507,1516,1846,1849`), pero **no** hay un punto único de re-aplicado al cambiar de escena.
Fichero destino: `src/core/Game.cpp` / `src/core/Streaming.cpp`.

**30 · Renombrar "WIDESCREEN" → "BORDERS"** — (sin clave; cadena literal `"FED_WIS"` + `L"BORDERS"`).
Patrón: `E8 ? ? ? ? DB 05 ? ? ? ? 50 89 C3 D8 0D` (P104).
Fuente: `Legacy.ixx:1140-1147`.
Retail medido: `0x42A525` (FUN `00429FE0`, 1627 B, `CALL 0x00584F50` = el getter de `TheText`).
Qué hace: en el menú, sustituye la etiqueta de la opción "WIDESCREEN" (clave de texto `FED_WIS`) por
`L"BORDERS"` (8 caracteres wide, copiados con `wcscpy_s`), de modo que la opción pasa a llamarse
"BORDERS" (bordes) en vez de "WIDESCREEN" (panorámico).
Estado: **YA ESTABA (equivalente)** — el port tiene sus propias etiquetas de menú
(`src/core/MenuScreensCustom.cpp`, `src/core/MenuScreens.cpp`) y no usa la nomenclatura `FED_WIS` del retail.

**31 · Sprites 2D con aspecto de frontend** — (sin clave).
Patrones: P100, P101 (compartidos con la feature 4) y `8B 35 ? ? ? ? 6A 00 55 6A 00 6A 00 52 50` (P32, `RsGlobal`).
Fuente: `Legacy.ixx:1009-1078`.
Retail medido: `0x57867C`/`0x578740` (FUN `00578660`/`00578730`), `0x649BF0` (FUN `00649BF0`),
`0x5FFC6E` (FUN `005FFAD0`, `MOV ESI,[0x009B48E0]` = `RsGlobal`).
Qué hace: recorta los quads fullscreen del dibujado 2D al aspecto del frontend y pinta barras laterales.
Estado: **PARCIAL** — el port ya recorta en `CSprite2d::Draw` (`src/renderer/Sprite2d.cpp:115-139`), pero **no**
tiene el concepto de "aspecto del frontend" configurable ni el ajuste `FrontendAspectRatio` (#4).

**32 · Tamaño de corona de luz** — (sin clave; siempre activo).
Patrón: `D8 0E D9 1E D9 05 ? ? ? ? D8 35 ? ? ? ? D8 0B D9 1B` (P56) y
`FF 35 ? ? ? ? 50 8D 84 24 24 05 00` (P57), `D9 83 FC 03 00 00 D8 1D ? ? ? ? DF E0 F6 C4 45` (P58),
`D9 EE D9 EE 83 EC 20 8B 44 24` (P59).
Fuente: `Legacy.ixx:611-639` (`FixCoronas`).
Retail medido: P56 → `0x577999` (FUN `005778D0`, 241 B); P57 → `0x589BE0`; P58 → `0x60AD9C`;
P59 → `0x571840` (FUN `00571840`, `FLDZ`).
Qué hace: cambia el operando `0x0E` por `0x0B` en la instrucción que escala las coronas (menos sangrado de
la corona), y con `NoLightSquare` anula las llamadas a `CBrightLights::RegisterOne` (el "cuadrado" de la luz
de los coches). En esta build `NoLightSquare` **no** existe (0 apariciones de la cadena).
Estado: **YA ESTABA** — el port dibuja las coronas con `spritew * size * wscale` (`src/renderer/Coronas.cpp:379-394`)
con `FIX_SPRITES` (`src/core/config.h:459`).

---

## 4. Ficheros del mod que NO aportan nada al port

| Fichero | Qué es | Clasificación y motivo técnico |
|---|---|---|
| `mods/widescreen/d3d8.dll` | 2 120 192 B, sha256 `0e066b700cf8a0cbdf3a7c27efa9b56bd465c595673776a2c12d198d0a7047bf`, PE32 x86, timestamp 2018-09-27 16:02:36Z. Exporta **`Direct3DCreate8`** (y `Direct3DCreate9`, `Direct3D9EnableMaximizedWindowedModeShim`, 1084 exports en total) e **importa `d3d9.dll`**: es un envoltorio que emula la API **Direct3D 8** sobre **Direct3D 9** | **NO APLICA** — Triple: (a) es una DLL de Win32 que se carga por el resolvedor de imports del proceso (el port no tiene proceso de Windows ni `d3d8.dll`); (b) su API es D3D8/D3D9, y el port renderiza con WebGL2 vía librw (`vendor/librw`), sin COM ni DirectX; (c) su raison d'être (MSAA de D3D8, `Present` con ventana, `Reset` de dispositivo) no tiene equivalente en un contexto WebGL. **Además** su único usuario aquí es `scripts/global.ini` (`UseD3D8to9=1`), que es configuración del modloader |
| `mods/widescreen/scripts/global.ini` | `[GlobalSets] LoadPlugins=1 / LoadFromScriptsOnly=0 / UseD3D8to9=1` — configuración del **SA-Modloader** | **NO APLICA** — no hay modloader: el port no carga ASI ni plugins; el único "plugin" es la capa `VICEEXT_*` compilada dentro del motor (`src/core/config.h`) |
| `mods/widescreen/Readme (or die).txt` y `Leiame (ou morra).txt` | Documentación del mod (versión 27/09/18, autor, instrucciones de instalación, nota de `CarSpeedDependantFOV`) | **NO APLICA como código**; su valor es documental: fija la versión y da la semántica de las claves (se ha usado en §3) |
| `mods/widescreen/modloader/Widescreen Fix/GTAVC.WidescreenFix.ini` | El INI de configuración | **A PORTAR como contrato** (nombres de clave y rangos), **NO APLICA** como fichero: el port tiene su propio INI (`src/core/re3.cpp:672` lee `[VideoMode]`, `src/core/SilentPatchOptions.cpp:42` lee `[SilentPatch]`). **Discrepancia medida**: el binario lee todo de `[MAIN]`; el master lee de `[MAIN]` **y** `[MISC]`/`[GRAPHICS]` — si se replica el contrato del binario hay que usar `[MAIN]` |
| `textures/` y `resources/` del master (packaging) | Assets de arte (iconos de mando, luces de vehículo, `radardisc`) | **NO APLICA** — el port no copia assets; si hace falta el disco del radar, se dibuja por código (y el `hud.txd` servido ya trae `radardisc`, según `docs/mods/ATTRIBUTION.md:322`) |

---

## 5. Lista final por categoría

> **Cómo leer §5 respecto a §3.** El inventario (§3) clasifica **32 features**:
> **6 A PORTAR** · **10 YA ESTABA** · **12 PARCIAL** · **4 NO APLICA**.
> Las listas de §5 son **de acciones**, no de features: §5.1 descompone las 6 features "A PORTAR" y las
> 12 "PARCIAL" en acciones concretas con fichero destino (17 acciones), §5.2 agrupa las 9 features
> "YA ESTABA" equivalentes, §5.3 las 4 "NO APLICA" más los 2 ficheros que no aportan nada, y §5.4 las
> decisiones que quedan abiertas y **no** son trabajo de código.

### 5.1 A PORTAR

| # | Item | Fichero destino del port | Razón |
|---|---|---|---|
| 1 | Escala de subtítulos (`SubtitlesScale`) | `src/renderer/Hud.cpp:1500` (`CRadar`… `DrawSubtitles`) | el port dibuja subtítulos sin factor de tamaño configurable; el mod escribe `1.2/1.22/0.58 × SubtitlesScale` en 3 constantes del retail (`0x556A22`, `0x55B030`, `0x55AF7E`) |
| 2 | Sombras de texto más pequeñas (`SmallerTextShadows`) | `src/renderer/Font.cpp` + `src/core/SilentPatchOptions.cpp` | el port tiene `SetDropShadowPosition` con valores 0/1/2 pero **no** la variante "sombra pequeña" (posición 1) como opción; el mod la engancha al mismo setter (`0x54FF40`/`0x57D590`) |
| 3 | Luces de vehículo más pequeñas (`FixVehicleLights`) | `src/renderer/Coronas.cpp:379-394` | el port escala las flares con `wscale/hscale` pero no aplica el factor **2.0** a las luces de vehículo (sitio retail `0x543A60`) |
| 4 | FOV dependiente de la velocidad (`CarSpeedDependantFOV`) | `src/core/Cam.cpp` / `src/core/Camera.cpp` | feature presente en el binario (float, default 20.0) y ausente del port; el master la reescribe con hook a `Process_Cam_On_A_String` |
| 5 | Control de FOV (`FOVControl`) | `src/renderer/Draw.cpp:97` | el binario escribe `1.0f` en la constante de escala del FOV del retail (`0x5766A6`); el port no tiene ese interruptor |
| 6 | Escala de radar al estilo GTA IV (`IVRadarScaling`) | `src/core/Radar.h:181-182` + `src/core/Radar.cpp:1577-1581` | el port tiene 94×76 (o 82×82 con `FixRadar`); el mod pone 88.5×81 en **10** sitios del retail |
| 7 | Escalado de HUD configurable (`HudWidthScale`/`HudHeightScale`) | `src/renderer/Hud.cpp` | el port escala con `SCREEN_SCALE_*` pero sin multiplicador de usuario (0.8 por defecto en el mod) |
| 8 | Escalado de radar configurable (`RadarWidthScale`) | `src/core/Radar.cpp` / `src/core/Radar.h:181` | ídem, factor 0.9 |
| 9 | `W:H` arbitrario en el aspecto (`ForceAspectRatio`) | `src/renderer/Draw.cpp:35` (`FindAspectRatio`) | el port ofrece una lista cerrada de aspectos; el mod acepta `W:H` libre |
| 10 | `FrontendAspectRatio` explícito | `src/renderer/Sprite2d.cpp:115-139` | el port deduce el aspecto del raster; el mod permite fijarlo |
| 11 | Bordes de cinemática adaptativos al aspecto | `src/core/Camera.cpp:2786-2804` | el port usa un 30 % fijo; el mod calcula letterbox/pillarbox con la fórmula de `GetScreenRect` |
| 12 | Contorno de texto (`ReplaceTextShadowWithOutline`) | `src/renderer/Font.cpp` (`CFont::PrintOutlinedString`, declarado en `Font.h:186` y **sin uso**) | la función existe declarada pero nadie la llama |
| 13 | Retícula escalada por aspecto | `src/renderer/Hud.cpp` + `src/core/Camera.cpp:283-284` | el port no aplica `fCrosshairPosFactor` ni `fCrosshairHeightScaleDown` |
| 14 | FOV de vehículos de emergencia | `src/core/Camera.cpp` | el mod recalcula `70/factor` (sitio `0x46DEB7`) |
| 15 | Salto de `CCamera::AvoidTheObstacles` con aspecto ≥ 2 | `src/core/Camera.cpp` | el mod lo salta con `ret 10h` (patrón P55, `0x480456`); no medido equivalente en `src/` |
| 16 | Re-aplicado de escalas al cambiar de escena | `src/core/Game.cpp` / `src/core/Streaming.cpp` | el mod lo hace en el hook de estado de carga (`0x60043D`); el port lo hace a mano en 4 sitios (`Hud.cpp:1481,1492,1829,1834`) |
| 17 | Contracto de INI del WFP (nombres de clave) | `src/core/re3.cpp:672` (zona `[VideoMode]`/`[Draw]`) o sección nueva | para que un usuario del port pueda escribir las mismas 20 claves |

### 5.2 YA ESTABA

| # | Item | Evidencia en el port |
|---|---|---|
| 1 | `ResX`/`ResY` (equivalente) | `src/core/re3.cpp:672-673`, `src/skel/glfw/glfw.cpp:912-913` |
| 2 | `FrontendAspectRatio` (equivalente) | `src/renderer/Sprite2d.cpp:115-139` |
| 3 | `RestoreCutsceneFOV` | `src/renderer/Draw.cpp:97-106` |
| 4 | Cálculo de aspecto propio | `src/renderer/Draw.cpp:35-52`, `:56-76`, `src/core/config.h:442` |
| 5 | FOV propio (widescreen) | `src/renderer/Draw.cpp:83-106`, `src/core/common.h:191-192` |
| 6 | Escalado de textos de menú | `src/core/Frontend.cpp:2926-2934` + `SCREEN_STRETCH_*` (`src/core/common.h:195-208`) |
| 7 | Escalado del HUD completo | `src/core/common.h:195-217` (136 usos en `Hud.cpp`) |
| 8 | Renombrar "WIDESCREEN" → "BORDERS" | `src/core/MenuScreensCustom.cpp`, `src/core/MenuScreens.cpp` (etiquetas propias) |
| 9 | Tamaño de corona de luz | `src/renderer/Coronas.cpp:379-394`, `src/core/config.h:459` (`FIX_SPRITES`) |

### 5.3 NO APLICA

| # | Item | Motivo técnico |
|---|---|---|
| 1 | `HideAABug` | parche del bug de MSAA de **Direct3D 8** (1 px sin cubrir del backbuffer); el port usa WebGL2/librw, sin ese bug |
| 2 | `ForceMultisamplingLevel` | depende de `IDirect3DDevice8` y de los niveles de MSAA del driver D3D8/9; WebGL2 tiene otro modelo (multisampled framebuffer) |
| 3 | `AllowAltTabbingWithoutPausing` | parchea el `WM_ACTIVATE` de Windows (→ `ret`); en web el evento es `visibilitychange` y el throttling lo hace el navegador |
| 4 | Arreglo de ratón "exclusive" | modo exclusivo del ratón de Win32; en web es Pointer Lock |
| 5 | `mods/widescreen/d3d8.dll` | envoltorio D3D8→D3D9 nativo; el port no tiene proceso Win32 ni DirectX |
| 6 | `mods/widescreen/scripts/global.ini` | configuración del modloader (SA-Modloader); no hay carga de ASI |

### 5.4 PENDIENTE (decisión de diseño, no falta de código)

| # | Item | Por qué queda pendiente |
|---|---|---|
| 1 | Pantalla de carga de interior (P40) | el master la trae madura en `InteriorLoading.ixx`; la build instalada sólo tiene el patrón. Decidir si se porta desde el master (fuera del alcance "desde esta build") |
| 2 | Saltar la pantalla de carga al cambiar de isla/interior | en el master es `Loading.ixx` (26 patrones); `docs/mods/ATTRIBUTION.md:321` ya lo dejó **PENDIENTE con motivo** (romper `LoadScene` rompe el on-demand: necesita el diseño D15) |
| 3 | Menús translúcidos (`TransparentMenu.ixx`) | son 11 patrones del master, **posteriores** a esta build; en el port no hay concept of menú translúcido con D3D8/D3D9 |
| 4 | Disco del radar procedural (`Radardisc.ixx`) | `docs/mods/ATTRIBUTION.md:322` ya lo cerró como **NO APLICA** (el `hud.txd` servido ya trae `radardisc`); reabrir sólo si a ojo se ve borroso |
| 5 | Multisampling en WebGL2 | no es un "port" sino un rediseño (FB multisample + resolve). Decidir si entra en el plan |

---

## 6. Riesgos

| # | Riesgo | Feature(s) afectada(s) | Detalle |
|---|---|---|---|
| R1 | **Depende de la API de Windows** | `HideAABug`, `ForceMultisamplingLevel`, `AllowAltTabbingWithoutPausing`, ratón exclusive, y todo el mecanismo (ASI loader, `VirtualProtect`, inyecciones de código) | El mod es un ASI de Win32: parchea el `.text` del retail con `VirtualProtect` + `WriteMemory` + `MakeJMP/MakeCALL`. En un port web **no hay retail que parchear**: el equivalente es código propio en el motor. Todo lo que en el mod es "reescribir un float del juego" es aquí "cambiar una constante del motor" — trabajo de refactor, no de parcheo |
| R2 | **Depende de MSAA / D3D9** | `HideAABug`, `ForceMultisamplingLevel` | El bug de 1 px es del multisampling de D3D8; el "forzar nivel de MSAA" escribe en la variable del driver que sólo existe con `IDirect3DDevice8`. En WebGL2 el MSAA se puede hacer, pero **no** es el mismo modelo ni el mismo bug |
| R3 | **Depende de la ventana nativa / del sistema** | `ResX`, `ResY`, `AllowAltTabbingWithoutPausing`, `d3d8.dll` | En web la "resolución" es el canvas (y cambia con el redimensionado del navegador); el foco/pausa los controla la pestaña. Replicar estas features exige tocar `src/skel/glfw` o el host web, no el motor |
| R4 | **El patrón P42 no resuelve en el retail 1.0** | `ReplaceTextShadowWithOutline` (y `SmallerTextShadows`) | Medido: `53 83 EC 10 8B 5D 24 18 8A 13` → **0 coincidencias** en `gta-vc.exe` 1.0; con la variante `8B 5C` (ESP) hay 2 (`0x54FF40`, `0x57D590`). El binario instalado apunta al prólogo de otro build (Steam 1.1). Si el port tomara el patrón del binario, el puntero `SetColor` quedaría nulo → la feature 18 se degrade |
| R5 | **El comentario del INI sobre `FOVControl` es incorrecto** | `FOVControl` | El comentario dice "writing float to 0x576686"; la dirección medida es **`0x5766A6`** (+0x20). Quien porte el valor debe usar la medida, no el comentario |
| R6 | **Los valores por defecto del INI no son los del código** | `HudHeightScale`, `HudWidthScale`, `RadarWidthScale`, `SubtitlesScale` | El INI instalado pone `0.0` en las cuatro, y el código sustituye `0.0` por su default: **0.8 / 0.8 / 0.9 / 0.8** (medido en el decompilado). El master legacy usa `1.0 / 1.0714285 / 1.0 / 1.0` — **discrepancia**; el binario manda |
| R7 | **`CarSpeedDependantFOV` sin fórmula medida** | #12 | Se sabe que se lee como float (default 0.0) y que el master la reescribe con otra técnica; **la fórmula exacta de esta build no se ha medido**. Si se porta, hay que decidir la fórmula (la del master es la mejor referencia, pero es POSTERIOR) |
| R8 | **`IVRadarScaling` tiene 10 sitios, no 4** | #17 | La build 2018 parchea 10 constantes del radar (y una con valor distinto: 108.5 en vez de 81). Portar "lo que dice el master" (4 sitios, 81.0) **no** es portar esta build. Decidir el juego de valores |
| R9 | **La sección del INI cambia entre build y master** | las 20 claves | El binario lee **sólo `[MAIN]`**; el master lee de `[MAIN]`, `[MISC]` y `[GRAPHICS]`. Si se replica el contrato del binario, el INI del port debe ser `[MAIN]`-único |
| R10 | **El ASI se carga con un modloader + un wrapper D3D8** | todo el conjunto | El ASI instalado es un `.asi` (no `.dll`): lo carga el SA-Modloader vía `scripts/global.ini`, y el juego necesita el `d3d8.dll` envolvente. Nada de eso existe en el port: el mod **no es una unidad portable**, es un conjunto de parches sobre un ejecutable Windows |
| R11 | **La fuente pública es POSTERIOR a la build** | features 1-32 | 244 de los patrones del master no existen en el binario. Si el plan dice "portar el WFP", hay que decidir explícitamente si el objetivo es *esta build* (lo que se inventaría aquí) o *el master* (más trabajo y más riesgo, pero mejor terminado). Este recibo cubre **esta build** |
| R12 | **Interacción con SilentPatch** | `ReplaceTextShadowWithOutline`, `SmallerTextShadows` | El mod incluye un bloque de "compatibilidad con SilentPatch" (P49-P52) que **reescribe el render de texto** de otra forma. El port ya trae su propia capa SilentPatch (`src/core/SilentPatchOptions.cpp`, `src/renderer/Font.cpp`): aplicar ambos a la vez puede duplicar el contorno |
| R13 | **Los índices de la lista de exclusión no caben en el retail 1.0** | `HudWidthScale`, `HudHeightScale` | Medido: el patrón largo P27 tiene **51** coincidencias en `gta-vc.exe` 1.0, pero el código excluye los índices **66** y **67** (los dos slots horizontales del radar). Esos índices sólo existen en un build con más coincidencias (Steam 1.1). En retail 1.0 esas dos escrituras no se hacen. Además el master **deriva** un patrón corto (`D8 0D <operando>`) a partir de P27/P25/P30, y ese patrón corto tiene **más** coincidencias que el largo: quien replique la técnica del master verá un conjunto de sitios distinto del de esta build |
| R14 | **El "patrón largo" y el "patrón derivado" no son lo mismo** | features 5, 6, 7, 22 | Esta build lleva los patrones **largos** (17-23 bytes) en su `.rdata`; el master lleva los **cortos derivados**. Las cuentas de sitios (y por tanto los índices que se tocan) no coinciden. Al portar hay que elegir: el conjunto de esta build (más completo para el HUD) o el del master (más simple, menos fragile) |

---

## 7. Anexo — reproducibilidad

| Qué | Cómo |
|---|---|
| Hash / PE / strings del ASI | `hashlib` + parser PE propio sobre el fichero; contrastado con REA `open_binary` (sha256 idéntico) y `binary_overview` (image base `0x10000000`, 7 segmentos, 1474 procedimientos, 772 cadenas) |
| Patrones del binario | ya extraídos en `gta_vc_browser/tmp/extsrc/wfp_binary_patterns.txt` (106 líneas) y **verificados**: cada uno aparece como cadena ASCII en `.rdata` del ASI con una referencia `imm32` en `.text` |
| Comparación con el master | extracción de literales de patrón de `source/GTAVC.WidescreenFix/*` con normalización de `?`/`??`; comparación exacta y por subcadena |
| Sitios en el retail | escaneo propio con máscara `?` sobre `gta-vc.exe` (mismo algoritmo que el mod) → offset de fichero, RVA y VA; función containment con el proyecto Ghidra `gta_vc_browser/tmp/rea/ghidra_vanilla` + script propio `R2WhoFile` |
| Semántica de las claves INI | decompilación del ASI con Ghidra headless (proyecto propio en temporal), función `FUN_10009CB0` (7467 B) = `ReadSettings` + `ApplyIniOptions` de esta build |
| Estado del port | `Select-String` sobre `src/**/*.{cpp,h}` con línea; todas las líneas citadas en este documento están medidas |

> Nota de método: la sesión REA (servidor compartido) re-enlaza el documento abierto entre llamadas
> (se comprobó: un `read_bytes` devolvió bytes de otro `.asi`). Por eso **todo lo que aparece en este
> recibo como "medido en el binario" se obtuvo con scripts propios sobre el fichero** (o con Ghidra
> headless en un proceso separado), no con el estado en caché de REA.
