# 16.0 · Enriquecimiento T1 (contrato de INI + menú + banderas) y W7 (menús)

> Subagente de **enriquecimiento** del plan `2026-10-20-16.0-widescreen-skygfx.md`.
> **No se implementa nada**: este documento es el único fichero escrito.
> Decisiones del jugador aplicadas: **D1** (el master del WFP es la especificación),
> **D3** (sección propia `[WidescreenFix]` con los nombres de clave del mod),
> **D4** (fórmula del FOV ya medida), y los **13 conflictos ya aprobados**.
>
> Todos los `fichero:línea` de este documento están **medidos sobre el árbol actual**
> (no copiados del recibo). Los finales de línea de cada fichero se detectan al final
> (§3).

---

## 0 · Resumen de lo que se propone

| # | Item | Veredicto | Fichero principal | Bandera |
|---|---|---|---|---|
| 1 | **T1-1** · Contrato de INI: las 32 claves del master en `[WidescreenFix]` | **A PORTAR** (contrato) | `src/core/re3.cpp` + módulo nuevo `src/core/WidescreenFixOptions.{h,cpp}` | `VICEEXT_WSP_OPTIONS` |
| 2 | **T1-2** · Interruptores del menú de Gráficos | **A PORTAR** (página nueva) | `src/core/MenuScreensCustom.cpp` (+ `src/core/Frontend.h` para la página) | `VICEEXT_WSP_OPTIONS` |
| 3 | **T1-3** · Banderas `VICEEXT_WSP_*` | **A PORTAR** (31 banderas, un bloque nuevo) | `src/core/config.h` | las 31 |
| 4 | **C7** · Menú en lienzo 16:9 + mapa 448 + ratón re-mapeado (C7+C9+C17) | **CONFLICTO REAL, aprobado** | `src/core/Frontend.cpp` | `VICEEXT_WSP_MENU_CANVAS` |
| 5 | **C8** · Escalas del frontend por clase + texto de script centrado (C8+C10) | **CONFLICTO DUPLICADO** → no se porta | `src/core/Frontend.cpp`, `src/renderer/Hud.cpp:1466,1820` | — |
| 6 | **C39** · Arreglos de Brief / Stats / Leyenda | **CONFLICTO REAL, aprobado** | `src/core/Frontend.cpp` + `src/core/Frontend.h` | `VICEEXT_WSP_MENU_CONSTANTS` |
| 7 | **C14** · Sin pantalla de carga entre islas / interior (features 45 y 46) | **NO APLICA / PENDIENTE con motivo (D15)** | `src/core/Streaming.cpp` (sin tocar) | — (sin bandera) |
| 8 | **F41** · Anclaje del mapa / radio del carousel (feature 41) | **PARCIAL**: el port ya re-ancla; falta conservar la fracción | `src/core/Frontend.cpp` | `VICEEXT_WSP_MAP_ANCHOR` |

Cifras de este item: **8 items**, **32 claves de INI**, **31 banderas** `VICEEXT_WSP_*`,
**15 opciones de menú** en una página nueva, **19 claves GXT** nuevas.

---

### 1 · T1-1 · Contrato de INI: las 32 claves del master en `[WidescreenFix]`

- **Fichero**: `src/core/re3.cpp` (lector) + `src/core/WidescreenFixOptions.h` / `.cpp` (**nuevos**, módulo de opciones al estilo `src/core/SilentPatchOptions.{h,cpp}`)
- **Bloque**:
  - `src/core/re3.cpp:645-661` (`LoadINISettings`, hasta `ReadSilentPatchOptions();`)
  - `src/core/re3.cpp:800-825` (bloque genérico de CFO: carga de las opciones de menú)
  - `src/core/re3.cpp:955-970` (bloque genérico de CFO: guardado de las opciones de menú)
  - `src/core/re3.cpp:352-467` (`ReadIniIfExists` / `StoreIni`: 5 sobrecargas + la de `char*`)
  - `src/core/SilentPatchOptions.h:1-20` (precedente: 14.0)
  - `src/core/SilentPatchOptions.cpp:34-62` (precedente: el lector)
- **Qué hace hoy**:
  - `src/core/re3.cpp:199-207`: el INI del port es `/userfiles/reVC.ini` (web) / `reVC.ini` (escritorio), leído con `mINI`; `src/core/re3.cpp:196` define `MINI_CASE_SENSITIVE` → **las secciones y las claves se buscan con mayúsculas y minúsculas exactas** (`src/extras/ini.h:182-194`), o sea que los nombres del mod (`HudWidthScale`, `IVRadarScaling`, `NoCutsceneBorderAnimation`…) se pueden usar tal cual.
  - `src/core/re3.cpp:661`: `ReadSilentPatchOptions();` — el precedente del 14.0: las opciones del mod se leen de su propia sección `[SilentPatch]` con los nombres de clave del mod (`src/core/SilentPatchOptions.cpp:38-49`).
  - No hay ninguna clave del WFP: las 32 del master no existen en el port (`gta_vc_browser/tmp/rea/wfp-16-rea.md:18`).
- **Cambio propuesto**:
  1. **Módulo nuevo** `src/core/WidescreenFixOptions.{h,cpp}` (LF, como `SilentPatchOptions.{h,cpp}`) con **una variable canónica por clave** (32) y un lector `ReadWidescreenFixOptions()` que lee **las 32 claves de `[WidescreenFix]`** con los **nombres de clave del mod** y con los **defaults reales del código del mod** (tabla §9).
  2. `src/core/re3.cpp:661` → una línea más: `ReadWidescreenFixOptions();` y una traza `VEX wsp …` (como la de `VEX sp opts` de `:664-668`), para que el log del jugador vea los valores leídos.
  3. **Regla de precedencia** (una sola, documentada): *si la clave del WFP está en el INI, manda sobre la clave propia del port para la misma feature; si no está, manda la del port*. Solo hay **una** colisión real: `SmartCutsceneBorders` ↔ `Display/CutsceneBorders` (`src/core/MenuScreensCustom.cpp:47`).
  4. **Defaults**: el *default del port* es **neutro** (1.0 en las escalas, 0 en los interruptores) para que nada cambie por defecto; el *default del mod* (medido en su código) se documenta en la tabla §4 y se aplica **solo si el usuario escribe la clave**. Única divergencia consciente: `HudHeightScale = 0.0` significa 1.0714285 en el mod (`Legacy.ixx:335`) y **1.0** en el port (regla del jugador: nada que cambie el HUD por defecto). Si el usuario escribe `1.0714285`, el port lo respeta.
  5. Las claves **sin equivalente en el port** (`ResX`, `ResY`, `ForceAspectRatio`, `FrontendAspectRatio`, `MenuFrameLimit`, `ForceMultisamplingLevel`, `HideAABug`, `SMAA`, `ConsoleGamma`, `TransparentMenu`, `AllowAltTabbingWithoutPausing`, `HQRadarDisc`, `NoIslandLoading`, `NoInteriorLoading`) **se leen igual** (contrato) y **no hacen nada**: son NO APLICA o PENDIENTE (§9 lo dice clave por clave). Se leen para que un INI del mod copiado tal cual no dé error y para que la fila de `ATTRIBUTION.md` pueda decir "leída, sin destino".
- **Bandera/clave que necesita**: `VICEEXT_WSP_OPTIONS` (bloque WSP nuevo de `config.h`, §10). Las 32 claves van a la sección `[WidescreenFix]` del `reVC.ini` del port.
- **Dependencia**: `src/core/re3.cpp` (único punto de lectura del INI), `src/core/SilentPatchOptions.h` (precedente), `src/extras/ini.h` (mINI, `MINI_CASE_SENSITIVE`). Las **claves de SkyGfx no entran aquí**: las de SkyGfx se quedan donde ya están (`Rendering/…`, `CustomPipesValues/…`) y son del subagente de `src/extras/`; **este item solo escribe las 32 del WFP** (ver §9.1, "SkyGfx: reservado al integrador").
- **Riesgo**:
  - El bloque genérico de CFO (`src/core/re3.cpp:809-822`) **vuelve a leer** las 15 claves que tengan opción de menú, en la **misma variable** y con el **mismo valor** → es inocuo, pero es una lectura redundante. Para que no haya dos fuentes de verdad, el lector del WFP se llama **antes** del bloque CFO y su valor es el que queda (mismo valor, misma variable).
  - `CCFOSelect`/`CCFOSlider` guardan `int8`/`float` (`src/core/Frontend.h:501-543`): **las variables canónicas de las 15 claves con menú tienen que ser `int8` o `float`**, nunca `int32` ni `bool` (si no, el CFO machaca 4 bytes con 1).
  - Las 2 claves de tipo *string* del mod (`HudAspectRatioConstraint`, `MenuAspectRatioConstraint`) no caben en un `CCFOSelect` (que escribe `int8`): el lector **normaliza** el texto del mod (`Auto`/`16:9`/`21:9` → 0/1/2) a un `int8` y acepta también el número que escribe el menú. Documentado en §2.1.
- -> verify: `reVC.ini` con `[WidescreenFix]` y las 32 claves escritas → el log del jugador trae una línea `VEX wsp …` con los 32 valores leídos, y con el INI vacío (sin la sección) la línea trae los defaults neutros y **el juego se ve igual que antes** (comparación de capturas del jugador antes/después, sin `dataTag` ni `VERSION`).

### 1.1 · Contenido del módulo nuevo (propuesta, sin comentarios)

`src/core/WidescreenFixOptions.h` (nuevo, **LF**):

```cpp
#pragma once

extern int32 g_nWSPOptionResX;
extern int32 g_nWSPOptionResY;
extern char g_szWSPOptionForceAspectRatio[32];
extern char g_szWSPOptionFrontendAspectRatio[32];
extern int8 g_nWSPOptionDontTouchFOV;
extern int8 g_nWSPOptionRestoreCutsceneFOV;
extern float g_fWSPOptionHudWidthScale;
extern float g_fWSPOptionHudHeightScale;
extern float g_fWSPOptionRadarWidthScale;
extern float g_fWSPOptionSubtitlesScale;
extern float g_fWSPOptionRadarWidth;
extern float g_fWSPOptionRadarHeight;
extern int8 g_nWSPOptionHudAspectRatio;
extern int8 g_nWSPOptionMenuAspectRatio;
extern int8 g_nWSPOptionNoCutsceneBorderAnimation;
extern int32 g_nWSPOptionMenuFrameLimit;
extern int8 g_nWSPOptionSmartCutsceneBorders;
extern int8 g_nWSPOptionAllowAltTabbingWithoutPausing;
extern int32 g_nWSPOptionHideAABug;
extern int8 g_nWSPOptionSmallerVehicleCorona;
extern int8 g_nWSPOptionNoLightSquare;
extern char g_szWSPOptionForceMultisamplingLevel[32];
extern int8 g_nWSPOptionIVRadarScaling;
extern int8 g_nWSPOptionReplaceTextShadowWithOutline;
extern int8 g_nWSPOptionCarSpeedDependantFOV;
extern int8 g_nWSPOptionVCSCamShake;
extern int8 g_nWSPOptionNoIslandLoading;
extern int8 g_nWSPOptionNoInteriorLoading;
extern int8 g_nWSPOptionHQRadarDisc;
extern int8 g_nWSPOptionSMAA;
extern int8 g_nWSPOptionTransparentMenu;
extern int8 g_nWSPOptionConsoleGamma;

bool ReadIniIfExists(const char *cat, const char *key, bool *out);
bool ReadIniIfExists(const char *cat, const char *key, int32 *out);
bool ReadIniIfExists(const char *cat, const char *key, int8 *out);
bool ReadIniIfExists(const char *cat, const char *key, float *out);
bool ReadIniIfExists(const char *cat, const char *key, char *out, int size);

void ReadWidescreenFixOptions();
```

`src/core/WidescreenFixOptions.cpp` (nuevo, **LF**):

```cpp
#include "common.h"
#include "WidescreenFixOptions.h"

int32 g_nWSPOptionResX = 0;
int32 g_nWSPOptionResY = 0;
char g_szWSPOptionForceAspectRatio[32] = "auto";
char g_szWSPOptionFrontendAspectRatio[32] = "auto";
int8 g_nWSPOptionDontTouchFOV = 0;
int8 g_nWSPOptionRestoreCutsceneFOV = 1;
float g_fWSPOptionHudWidthScale = 1.0f;
float g_fWSPOptionHudHeightScale = 1.0f;
float g_fWSPOptionRadarWidthScale = 1.0f;
float g_fWSPOptionSubtitlesScale = 1.0f;
float g_fWSPOptionRadarWidth = 94.0f;
float g_fWSPOptionRadarHeight = 76.0f;
int8 g_nWSPOptionHudAspectRatio = 0;
int8 g_nWSPOptionMenuAspectRatio = 0;
int8 g_nWSPOptionNoCutsceneBorderAnimation = 0;
int32 g_nWSPOptionMenuFrameLimit = 0;
int8 g_nWSPOptionSmartCutsceneBorders = 0;
int8 g_nWSPOptionAllowAltTabbingWithoutPausing = 0;
int32 g_nWSPOptionHideAABug = 0;
int8 g_nWSPOptionSmallerVehicleCorona = 0;
int8 g_nWSPOptionNoLightSquare = 0;
char g_szWSPOptionForceMultisamplingLevel[32] = "";
int8 g_nWSPOptionIVRadarScaling = 0;
int8 g_nWSPOptionReplaceTextShadowWithOutline = 0;
int8 g_nWSPOptionCarSpeedDependantFOV = 0;
int8 g_nWSPOptionVCSCamShake = 0;
int8 g_nWSPOptionNoIslandLoading = 0;
int8 g_nWSPOptionNoInteriorLoading = 0;
int8 g_nWSPOptionHQRadarDisc = 0;
int8 g_nWSPOptionSMAA = 0;
int8 g_nWSPOptionTransparentMenu = 0;
int8 g_nWSPOptionConsoleGamma = 0;

static int8
WspParseAspectConstraint(const char *value)
{
	if (!value[0])
		return 0;
	if (value[0] >= '0' && value[0] <= '9')
		return (int8)atoi(value);
	if (value[0] == 'A' || value[0] == 'a')
		return 0;
	if (value[0] == '1' && value[1] == '6')
		return 1;
	if (value[0] == '2' && value[1] == '1')
		return 2;
	return 0;
}

void
ReadWidescreenFixOptions(void)
{
#ifdef VICEEXT_WSP_OPTIONS
	ReadIniIfExists("WidescreenFix", "ResX", &g_nWSPOptionResX);
	ReadIniIfExists("WidescreenFix", "ResY", &g_nWSPOptionResY);
	ReadIniIfExists("WidescreenFix", "ForceAspectRatio", g_szWSPOptionForceAspectRatio, 32);
	ReadIniIfExists("WidescreenFix", "FrontendAspectRatio", g_szWSPOptionFrontendAspectRatio, 32);
	ReadIniIfExists("WidescreenFix", "DontTouchFOV", &g_nWSPOptionDontTouchFOV);
	ReadIniIfExists("WidescreenFix", "RestoreCutsceneFOV", &g_nWSPOptionRestoreCutsceneFOV);
	ReadIniIfExists("WidescreenFix", "HudWidthScale", &g_fWSPOptionHudWidthScale);
	ReadIniIfExists("WidescreenFix", "HudHeightScale", &g_fWSPOptionHudHeightScale);
	ReadIniIfExists("WidescreenFix", "RadarWidthScale", &g_fWSPOptionRadarWidthScale);
	ReadIniIfExists("WidescreenFix", "SubtitlesScale", &g_fWSPOptionSubtitlesScale);
	ReadIniIfExists("WidescreenFix", "RadarWidth", &g_fWSPOptionRadarWidth);
	ReadIniIfExists("WidescreenFix", "RadarHeight", &g_fWSPOptionRadarHeight);
	ReadIniIfExists("WidescreenFix", "NoCutsceneBorderAnimation", &g_nWSPOptionNoCutsceneBorderAnimation);
	ReadIniIfExists("WidescreenFix", "MenuFrameLimit", &g_nWSPOptionMenuFrameLimit);
	ReadIniIfExists("WidescreenFix", "SmartCutsceneBorders", &g_nWSPOptionSmartCutsceneBorders);
	ReadIniIfExists("WidescreenFix", "AllowAltTabbingWithoutPausing", &g_nWSPOptionAllowAltTabbingWithoutPausing);
	ReadIniIfExists("WidescreenFix", "HideAABug", &g_nWSPOptionHideAABug);
	ReadIniIfExists("WidescreenFix", "SmallerVehicleCorona", &g_nWSPOptionSmallerVehicleCorona);
	ReadIniIfExists("WidescreenFix", "NoLightSquare", &g_nWSPOptionNoLightSquare);
	ReadIniIfExists("WidescreenFix", "ForceMultisamplingLevel", g_szWSPOptionForceMultisamplingLevel, 32);
	ReadIniIfExists("WidescreenFix", "IVRadarScaling", &g_nWSPOptionIVRadarScaling);
	ReadIniIfExists("WidescreenFix", "ReplaceTextShadowWithOutline", &g_nWSPOptionReplaceTextShadowWithOutline);
	ReadIniIfExists("WidescreenFix", "CarSpeedDependantFOV", &g_nWSPOptionCarSpeedDependantFOV);
	ReadIniIfExists("WidescreenFix", "VCSCamShake", &g_nWSPOptionVCSCamShake);
	ReadIniIfExists("WidescreenFix", "NoIslandLoading", &g_nWSPOptionNoIslandLoading);
	ReadIniIfExists("WidescreenFix", "NoInteriorLoading", &g_nWSPOptionNoInteriorLoading);
	ReadIniIfExists("WidescreenFix", "HQRadarDisc", &g_nWSPOptionHQRadarDisc);
	ReadIniIfExists("WidescreenFix", "SMAA", &g_nWSPOptionSMAA);
	ReadIniIfExists("WidescreenFix", "TransparentMenu", &g_nWSPOptionTransparentMenu);
	ReadIniIfExists("WidescreenFix", "ConsoleGamma", &g_nWSPOptionConsoleGamma);

	if (g_fWSPOptionHudWidthScale == 0.0f)
		g_fWSPOptionHudWidthScale = 1.0f;
	if (g_fWSPOptionHudHeightScale == 0.0f)
		g_fWSPOptionHudHeightScale = 1.0f;
	if (g_fWSPOptionRadarWidthScale == 0.0f)
		g_fWSPOptionRadarWidthScale = 1.0f;
	if (g_fWSPOptionSubtitlesScale == 0.0f)
		g_fWSPOptionSubtitlesScale = 1.0f;

	char constraint[32];
	if (ReadIniIfExists("WidescreenFix", "HudAspectRatioConstraint", constraint, 32))
		g_nWSPOptionHudAspectRatio = WspParseAspectConstraint(constraint);
	if (ReadIniIfExists("WidescreenFix", "MenuAspectRatioConstraint", constraint, 32))
		g_nWSPOptionMenuAspectRatio = WspParseAspectConstraint(constraint);
#endif
}
```

`src/core/re3.cpp` (dentro de `LoadINISettings`, tras la línea 661):

```cpp
	ReadSilentPatchOptions();
	ReadWidescreenFixOptions();
```

y la traza (junto a la de `VEX sp opts`, `src/core/re3.cpp:662-669`):

```cpp
#ifdef __EMSCRIPTEN__
	char wspOpts[224];
	snprintf(wspOpts, sizeof wspOpts, "VEX wsp hud=%.3f,%.3f radar=%.3f,%.2f,%.2f sub=%.3f outline=%d fov=%d,%.2f shake=%d corona=%d,%d menu=%d,%d",
		g_fWSPOptionHudWidthScale, g_fWSPOptionHudHeightScale, g_fWSPOptionRadarWidthScale,
		g_fWSPOptionRadarWidth, g_fWSPOptionRadarHeight, g_fWSPOptionSubtitlesScale,
		g_nWSPOptionReplaceTextShadowWithOutline, g_nWSPOptionCarSpeedDependantFOV,
		g_fWSPOptionHudHeightScale, g_nWSPOptionVCSCamShake, g_nWSPOptionSmallerVehicleCorona,
		g_nWSPOptionNoLightSquare, g_nWSPOptionMenuAspectRatio, g_nWSPOptionHudAspectRatio);
	ODTRACES(wspOpts);
#endif
```

> Cabecera de atribución obligatoria sobre cada bloque portado (según el plan y `docs/mods/ATTRIBUTION.md`): `PORTADO -- WidescreenFixesPack (MIT, (c) ThirteenAG)`. Es la única excepción al rule de "sin comentarios" y la pone el integrador, no este documento.

---

### 2 · T1-2 · Interruptores del menú de Gráficos

- **Fichero**: `src/core/MenuScreensCustom.cpp` (página nueva + macros) y `src/core/Frontend.h` (**+1 entrada en el enum `MENUPAGES`**, fuera de mi territorio: lo pide el integrador)
- **Bloque**:
  - `src/core/MenuScreensCustom.cpp:768-792` (página `MENUPAGE_GRAPHICS_SETTINGS`)
  - `src/core/MenuScreensCustom.cpp:36-101` (macros de las opciones: `VIDEOMODE_SELECTOR`, `MULTISAMPLING_SELECTOR`, `CUTSCENE_BORDERS_TOGGLE`, `DUALPASS_SELECTOR`, `ISLAND_LOADING_SELECTOR`, `POSTFX_SELECTORS`)
  - `src/core/MenuScreensCustom.cpp:100-101` (`filterNames`, `off_on`), `:161` (`islandLoadingOpts`)
  - `src/core/MenuScreensCustom.cpp:102-132` (`RestoreDefGraphics`)
  - `src/core/Frontend.h:427-434` (`NUM_MENUROWS = 18`), `:225-233` (páginas propias), `:238-247` (`MENUPAGE_OUTRO` … `MENUPAGES`)
  - `src/extras/frontendoption.h:83-84` (`FrontendOptionAddSelect` / `AddDynamic`) y `src/core/Frontend.h:501-543` (`CCFOSelect`, `CCFOSlider`)
- **Qué hace hoy**:
  - La página de Gráficos (`MENUPAGE_GRAPHICS_SETTINGS`) tiene **16 de las 18 filas** ocupadas (`src/core/MenuScreensCustom.cpp:772-790` + las 4 que inserta `src/core/re3.cpp:144-148`): `FED_RES`, `FED_WIS`, `FEM_SCF`, `FEM_VSC`, `FEM_FRM`, `FED_AAS`, `FEM_ISL`, `FEM_2PR`, `FED_CLF`, `FED_MBL` + `FED_VPL`/`FED_PRM`/`FED_WLM`/`FED_RGL` + `FET_DEF` + `FEDS_TB`.
  - La página de Display (`src/core/MenuScreensCustom.cpp:434-455`) tiene **13 de 18**.
  - `NUM_MENUROWS = 18` (`src/core/Frontend.h:430`) y `MENUPAGES` **no tiene huecos libres**: el enum termina en `MENUPAGES` (`src/core/Frontend.h:238-247`) y `aScreens[]` tiene una entrada por cada valor (`src/core/MenuScreensCustom.cpp:366-845`); `FrontendScreenAdd` **no se llama en el port** (solo está declarado, `src/extras/frontendoption.cpp:159`) y su `assert(id < MENUPAGES)` (`src/extras/frontendoption.cpp:54`) no dejaría añadir una página: `GetLastMenuScreen()` devolvería `MENUPAGE_OUTRO` (34) y `RegisterNewScreen` escribiría **encima** de `MENUPAGE_GAMEPAD_SETTINGS`.
  - Los textos de las opciones son **claves GXT** (`src/core/Frontend.cpp:1318`: `leftText = TheText.Get(aScreens[...].m_EntryName)`); una clave que no exista sale como `"<CLAVE> missing"` (`src/text/Text.cpp:465-468`).
- **Cambio propuesto**:
  1. **Página nueva** `MENUPAGE_WIDEFIX_SETTINGS` ("WIDESCREEN FIX"), con **15 opciones + `FET_DEF` + `FEDS_TB` = 17 filas ≤ 18**. Se entra desde la página de Gráficos con una fila `MENUACTION_CHANGEMENU` (como la que lleva a Display, `src/core/MenuScreensCustom.cpp:636`).
  2. **Todas apagadas de serie**: cada opción apunta a la variable canónica del §1, cuyo default es el neutro (1.0 / 0). Ninguna opción del WFP cambia nada con el INI vacío.
  3. **Los textos**: clave GXT nueva por opción (tabla §2.1). El GXT servido **no tiene** claves para "HUD scale", "radar scale", "subtítulos", "FOV", "contorno de texto", "lienzo 16:9" (medido: ninguna de las 2499 claves de `gamefiles/TEXT/american.gxt` dice eso; `gta_vc_browser/tools/gxt_inspect.py claves …`).
  4. `RestoreDefGraphics` (`src/core/MenuScreensCustom.cpp:102-132`) tiene que **restaurar también las 15 variables del WFP** a sus defaults neutros.
  5. Los **nombres de clave en el INI** de cada opción son los **del mod** y la sección es `[WidescreenFix]` (D3) → el `CCFOSelect`/`CCFOSlider` guarda con `saveCat="WidescreenFix"`, `save="<clave del mod>"` (`src/core/Frontend.h:512`, `:535`), y el bloque genérico de guardado (`src/core/re3.cpp:955-970`) los escribe solos.
- **Bandera/clave que necesita**: `VICEEXT_WSP_OPTIONS` (todas); cada opción usa además la bandera de su feature (§5) en el punto donde el valor se consume.
- **Dependencia**:
  - **19 claves GXT nuevas** en los **6 GXT servidos** (`gta_vc_browser/streamed/TEXT/{american,french,german,italian,russian,spanish}.gxt`) y su copia al bootseed (`gta_vc_browser/tools/stage_bootseed.py`). Herramienta: `python gta_vc_browser/tools/gxt_inspect.py add <fichero> CLAVE=valor …` (el propio tool **no sobrescribe** una clave existente: verificado que las 19 candidatas están libres en los 6 idiomas).
  - **Esto cambia los datos servidos** → **`dataTag` sube** (`gta_vc_browser/web/ondemand.js:130`, hoy `2026-10-16-13.0-T5`). El plan dice en T13 "dataTag **sin** subir (no cambian los datos)": **el plan y este item se contradicen** → **decisión del jugador** (o se aceptan las 15 opciones con etiquetas nuevas, o se recurre a etiquetas del GXT viejo, o se deja la página sin texto nuevo).
  - `src/core/Frontend.h`: **+1 valor en el enum `MENUPAGES`** y la entrada correspondiente en `aScreens[]` (el orden del array **tiene que seguir al enum**, `src/core/MenuScreensCustom.cpp:366-845`).
- **Riesgo**:
  - La página de Gráficos **no cabe**: con 16 filas usadas quedan 2. Por eso la página nueva es obligatoria (o el reparto Display+Gráficos, que llena las dos páginas: Display 13+5=18 y Gráficos 16+2=18, sin margen para el futuro).
  - `CCFOSlider` escribe con `%f` (`src/core/re3.cpp:457-462`) → el valor sale como `0.800000`; el lector lo acepta (`strtof`, `src/core/re3.cpp:412`).
  - `CCFOSelect` guarda `int8` (`src/core/re3.cpp:450-455`): los valores 0/1/2 caben; el `ReplaceTextShadowWithOutline` del mod también es 0/1/2 → cabe.
  - La página nueva necesita su propia clave de título: `FET_WSF` ("WIDESCREEN FIX").
- -> verify: con el INI vacío, la página "WIDESCREEN FIX" aparece en Gráficos con las 15 opciones **todas en OFF / 1.00 / AUTO** y el juego se ve **igual que antes**; con `HudWidthScale = 0.8` en `[WidescreenFix]` el HUD se encoge (2.0-B/W2) y al arrancar de nuevo el valor persiste (se lee del INI).

### 2.1 · Las 15 opciones de la página nueva (propuesta)

| # | Etiqueta (GXT) | Texto | Tipo | Clave WFP | Variable canónica | Default | Feature |
|---|---|---|---|---|---|---|---|
| 1 | `FEM_HWS` | HUD WIDTH SCALE | slider 0.50–1.50 | `HudWidthScale` | `g_fWSPOptionHudWidthScale` | 1.00 | W2 (C1) |
| 2 | `FEM_HHS` | HUD HEIGHT SCALE | slider 0.50–1.50 | `HudHeightScale` | `g_fWSPOptionHudHeightScale` | 1.00 | W2 (C1) |
| 3 | `FEM_RWS` | RADAR SCALE | slider 0.50–1.50 | `RadarWidthScale` | `g_fWSPOptionRadarWidthScale` | 1.00 | W2 |
| 4 | `FEM_RSZ` | RADAR SIZE | select `FEM_NRM`/`FEM_IVR` | `IVRadarScaling` | `g_nWSPOptionIVRadarScaling` | 0 (NORMAL) | W2 (C2) |
| 5 | `FEM_SBS` | SUBTITLE SCALE | slider 0.25–4.00 | `SubtitlesScale` | `g_fWSPOptionSubtitlesScale` | 1.00 | W3 (C18) |
| 6 | `FEM_OUT` | TEXT OUTLINE | select `FEM_OFF`/`FEM_4D4`/`FEM_16D` | `ReplaceTextShadowWithOutline` | `g_nWSPOptionReplaceTextShadowWithOutline` | 0 (OFF) | W3 (C11) |
| 7 | `FEM_STS` | SMALL TEXT SHADOW | select `FEM_OFF`/`FEM_ON` | `SmallerTextShadows` (**no está en las 32 del master**: es de la build de 2018) | `g_nWSPOptionSmallerTextShadows` (a declarar solo si entra) | 0 (OFF) | W3 |
| 8 | `FEM_SFD` | SPEED FOV | select `FEM_OFF`/`FEM_ON` | `CarSpeedDependantFOV` | `g_nWSPOptionCarSpeedDependantFOV` | 0 (OFF) | W1 (C15) |
| 9 | `FEM_DTF` | DONT TOUCH FOV | select `FEM_OFF`/`FEM_ON` | `DontTouchFOV` | `g_nWSPOptionDontTouchFOV` | 0 (OFF) | W1 |
| 10 | `FEM_CSB` (**reutilizada**) | CUTSCENE BORDERS | select `FEM_OFF`/`FEM_ON` | `SmartCutsceneBorders` | `g_nWSPOptionSmartCutsceneBorders` | 0 (OFF) | W4 (C3) |
| 11 | `FEM_VCS` | VCS CAMERA SHAKE | select `FEM_OFF`/`FEM_ON` | `VCSCamShake` | `g_nWSPOptionVCSCamShake` | 0 (OFF) | W6 |
| 12 | `FEM_SCL` | SMALLER CAR LIGHTS | select `FEM_OFF`/`FEM_ON` | `SmallerVehicleCorona` | `g_nWSPOptionSmallerVehicleCorona` | 0 (OFF) | W5 (C12) |
| 13 | `FEM_NLS` | NO LIGHT SQUARE | select `FEM_OFF`/`FEM_ON` | `NoLightSquare` | `g_nWSPOptionNoLightSquare` | 0 (OFF) | W5 |
| 14 | `FEM_MC9` | MENU 16:9 CANVAS | select `FEM_AUT`/`FEM_169`/`FEM_219` | `MenuAspectRatioConstraint` | `g_nWSPOptionMenuAspectRatio` | 0 (AUTO) | W7 (C7) |
| 15 | `FEM_HAC` | HUD ASPECT | select `FEM_AUT`/`FEM_169`/`FEM_219` | `HudAspectRatioConstraint` | `g_nWSPOptionHudAspectRatio` | 0 (AUTO) | W2 (C6) |

Textos de la derecha que hay que **añadir** al GXT: `FEM_4D4` ("4 DIR"), `FEM_16D` ("16 DIR"), `FEM_IVR` ("GTA IV"), `FEM_169` ("16:9"), `FEM_219` ("21:9"). Título de página: `FET_WSF` ("WIDESCREEN FIX"). Etiquetas nuevas: `FEM_HWS`, `FEM_HHS`, `FEM_RWS`, `FEM_RSZ`, `FEM_SBS`, `FEM_OUT`, `FEM_STS`, `FEM_SFD`, `FEM_DTF`, `FEM_VCS`, `FEM_SCL`, `FEM_NLS`, `FEM_MC9`, `FEM_HAC` → **19 claves GXT** en total.

> `SmallerTextShadows` no existe en el master (es de la build de 2018): el master aplica el mismo comportamiento con `ReplaceTextShadowWithOutline` (`Legacy.ixx:997-1000` engancha `SetDropShadowPosition`). Como el master **no** tiene la clave, **no entra en las 32**; se deja como una opción del 14.0/2.0-B si ya existe, o no se expone. La fila 7 de la tabla es, por tanto, **opcional** y usa una clave del port (no del WFP).

Fragmento de la página (a insertar en `src/core/MenuScreensCustom.cpp`, junto a las demás pantallas):

```cpp
#ifdef VICEEXT_WSP_OPTIONS
const char *wspOutlineTexts[] = { "FEM_OFF", "FEM_4D4", "FEM_16D" };
const char *wspRadarSizeTexts[] = { "FEM_NRM", "FEM_IVR" };
const char *wspRatioTexts[] = { "FEM_AUT", "FEM_169", "FEM_219" };
#define WSP_SLIDER(var, key, gxt, lo, hi) MENUACTION_CFO_SLIDER, gxt, { new CCFOSlider(var, "WidescreenFix", key, lo, hi) }, 0, 0, MENUALIGN_LEFT,
#define WSP_SELECT(var, key, gxt, texts) MENUACTION_CFO_SELECT, gxt, { new CCFOSelect(var, "WidescreenFix", key, texts, ARRAY_SIZE(texts), false) }, 0, 0, MENUALIGN_LEFT,
#endif
```

```cpp
#ifdef VICEEXT_WSP_OPTIONS
	// MENUPAGE_WIDEFIX_SETTINGS
	{ "FET_WSF", MENUPAGE_GRAPHICS_SETTINGS, new CCustomScreenLayout({40, 78, 25, true, true}), nil,
		WSP_SLIDER(&g_fWSPOptionHudWidthScale, "HudWidthScale", "FEM_HWS", 0.5f, 1.5f)
		WSP_SLIDER(&g_fWSPOptionHudHeightScale, "HudHeightScale", "FEM_HHS", 0.5f, 1.5f)
		WSP_SLIDER(&g_fWSPOptionRadarWidthScale, "RadarWidthScale", "FEM_RWS", 0.5f, 1.5f)
		WSP_SELECT(&g_nWSPOptionIVRadarScaling, "IVRadarScaling", "FEM_RSZ", wspRadarSizeTexts)
		WSP_SLIDER(&g_fWSPOptionSubtitlesScale, "SubtitlesScale", "FEM_SBS", 0.25f, 4.0f)
		WSP_SELECT(&g_nWSPOptionReplaceTextShadowWithOutline, "ReplaceTextShadowWithOutline", "FEM_OUT", wspOutlineTexts)
		WSP_SELECT(&g_nWSPOptionCarSpeedDependantFOV, "CarSpeedDependantFOV", "FEM_SFD", off_on)
		WSP_SELECT(&g_nWSPOptionDontTouchFOV, "DontTouchFOV", "FEM_DTF", off_on)
		WSP_SELECT(&g_nWSPOptionSmartCutsceneBorders, "SmartCutsceneBorders", "FEM_CSB", off_on)
		WSP_SELECT(&g_nWSPOptionVCSCamShake, "VCSCamShake", "FEM_VCS", off_on)
		WSP_SELECT(&g_nWSPOptionSmallerVehicleCorona, "SmallerVehicleCorona", "FEM_SCL", off_on)
		WSP_SELECT(&g_nWSPOptionNoLightSquare, "NoLightSquare", "FEM_NLS", off_on)
		WSP_SELECT(&g_nWSPOptionMenuAspectRatio, "MenuAspectRatioConstraint", "FEM_MC9", wspRatioTexts)
		WSP_SELECT(&g_nWSPOptionHudAspectRatio, "HudAspectRatioConstraint", "FEM_HAC", wspRatioTexts)
		MENUACTION_CFO_DYNAMIC, "FET_DEF", { new CCFODynamic(nil, nil, nil, nil, RestoreDefWidescreenFix) }, 320, 0, MENUALIGN_CENTER,
		MENUACTION_GOBACK, "FEDS_TB", {nil, SAVESLOT_NONE, MENUPAGE_NONE}, 320, 0, MENUALIGN_CENTER,
	},
#endif
```

y en la página de Gráficos, una fila que abra la nueva:

```cpp
		MENUACTION_CHANGEMENU, "FET_WSF", {nil, SAVESLOT_NONE, MENUPAGE_WIDEFIX_SETTINGS}, 0, 0, MENUALIGN_CENTER,
```

---

### 3 · T1-3 · Banderas `VICEEXT_WSP_*` (escritura única de `src/core/config.h`)

- **Fichero**: `src/core/config.h` (**única escritura**; los demás subagentes solo piden banderas)
- **Bloque**: bloque nuevo **WidescreenFixesPack (plan 16.0)**, insertado **después de la línea 556** (`#define VICEEXT_SP_OPTIONS`, fin del bloque de SilentPatch) y **antes de la línea 558** (comentario de "v2.5 Remove zeros"). Nada se reordena: es un bloque nuevo al final de su sitio.
- **Qué hace hoy**: `src/core/config.h:515-556` es el bloque de la familia SilentPatch (`VICEEXT_SP_*`), el precedente de cómo se declara una familia de banderas de un mod; `src/core/config.h:488-505` y `:567-575` son los bloques de Vice Extended; `src/core/config.h:668-703` el de "Hud, frontend and radar".
- **Cambio propuesto**: 31 banderas `VICEEXT_WSP_*`, una por feature del WFP, **todas definidas** (como la familia `VICEEXT_SP_*`): el *efecto* de cada una lo decide **el valor de su clave de INI**, cuyo default es neutro → con el INI vacío el juego se comporta **igual que antes** (que es la condición del plan: "con la bandera apagada el juego se comporta igual"; aquí la "bandera apagada" es la clave del INI en su valor neutro, igual que en el 14.0).
- **Bandera/clave que necesita**: es el item que **escribe** las banderas; no necesita ninguna.
- **Dependencia**: ninguna (es un fichero de defines). Los subagentes de W1–W7 y S1–S4 **leen** estas banderas, no las escriben.
- **Riesgo**:
  - Una bandera por feature **y no una por tanda**: el plan dice "una bandera `VICEEXT_*`" por fix; con 31 banderas se puede apagar un fix suelto (requisito del jugador: "nada que cambie el HUD por defecto").
  - **No hay bandera para las 9 features NO APLICA / PENDIENTE** (§4 lo dice clave por clave): `NoIslandLoading`, `NoInteriorLoading`, `HQRadarDisc`, `ForceMultisamplingLevel`, `HideAABug`, `SMAA`, `TransparentMenu`, `ConsoleGamma`, `AllowAltTabbingWithoutPausing`, `MenuFrameLimit`, `ResX`/`ResY`.
  - `src/core/config.h` es **CRLF** (medido): el bloque nuevo se escribe con `\r\n` (regla de `.agents/CODING_STANDARDS.md` §6).
- -> verify: `grep -c "VICEEXT_WSP_" src/core/config.h` = 31 y el bloque está **después** de `VICEEXT_SP_OPTIONS` y **antes** del comentario de "v2.5"; el build sigue verde y `check-served-build.sh` no se queja de defines nuevos (no hay que tocar el verificador hasta T13).

Fragmento del bloque (sin comentarios dentro; las líneas `//` de una línea son el estilo del repo para explicar **qué hace** la bandera):

```cpp
// 16.0 WidescreenFixesPack (master de GitHub, MIT, (c) ThirteenAG): una bandera
// por feature del mod. Todas OFF por defecto: el efecto lo decide la clave de
// su INI, en la seccion [WidescreenFix] (D3).
#define VICEEXT_WSP_OPTIONS				// el contrato [WidescreenFix]: las 32 claves del master y sus mandos en el menu
#define VICEEXT_WSP_HUD_SCALE				// multiplicador de escala del HUD en X e Y (HudWidthScale / HudHeightScale)
#define VICEEXT_WSP_RADAR_SCALE			// factor de escala del radar y tamano configurable (RadarWidthScale / RadarWidth / RadarHeight / IVRadarScaling)
#define VICEEXT_WSP_HUD_ASPECT			// el HUD se ancla a la izquierda o a la derecha segun el ancho (HudAspectRatioConstraint)
#define VICEEXT_WSP_TEXT_OUTLINE			// contorno de texto de 16 direcciones en vez de la sombra difusa (ReplaceTextShadowWithOutline)
#define VICEEXT_WSP_SMALL_TEXT_SHADOW		// sombra de texto de 1 px en vez de 2 (SmallerTextShadows)
#define VICEEXT_WSP_SUBTITLES			// escala, anchura y posicion de los subtitulos, con la reserva del radar (SubtitlesScale)
#define VICEEXT_WSP_FOV_SPEED				// el FOV crece con la velocidad del coche en la camara VC (CarSpeedDependantFOV)
#define VICEEXT_WSP_FOV_DONT_TOUCH			// el FOV no se estira por el aspecto (DontTouchFOV)
#define VICEEXT_WSP_FOV_CUTSCENE			// el FOV de cinematica sigue el zoom del borde (RestoreCutsceneFOV / NoCutsceneBorderAnimation)
#define VICEEXT_WSP_FOV_MULTIPLIERS		// API de multiplicadores de FOV por hash (SetFOVMultiplier / RemoveFOVMultiplier / GetCurrentFOV)
#define VICEEXT_WSP_FOV_EMERGENCY			// el FOV de los vehiculos de emergencia se recalcula a 70/factor
#define VICEEXT_WSP_CAMERA_WIDE			// la camara no colisiona con la geometria en pantallas de 2:1 o mas
#define VICEEXT_WSP_CUTSCENE_BORDERS		// los bordes de cinematica siguen el aspecto en vez del 30 % fijo (SmartCutsceneBorders)
#define VICEEXT_WSP_ASPECT_RATIO			// aspecto forzado libre W:H en vez de la lista cerrada del port (ForceAspectRatio)
#define VICEEXT_WSP_FRONTEND_ASPECT		// el frontend se recorta al aspecto pedido con barras laterales (FrontendAspectRatio)
#define VICEEXT_WSP_SMALL_CORONA			// las luces de los vehiculos se dibujan mas pequenas (SmallerVehicleCorona)
#define VICEEXT_WSP_NO_LIGHT_SQUARE		// las luces de los vehiculos no dibujan el cuadrado (NoLightSquare)
#define VICEEXT_WSP_BRIGHT_LIGHTS_FAR		// las luces brillantes solo se dibujan cuando la camara esta lejos
#define VICEEXT_WSP_PARTICLES				// las particulas escalan con el ancho de la pantalla
#define VICEEXT_WSP_CAM_SHAKE_VCS			// la camara tiembla con la velocidad al estilo VCS (VCSCamShake)
#define VICEEXT_WSP_TRAFFIC_DISTANCE		// el trafico se genera mas lejos en pantallas anchas
#define VICEEXT_WSP_BULLET_SMOKE			// el impacto de bala deja humo
#define VICEEXT_WSP_FADE_RECT				// el fundido usa el rectangulo de contenido y no la pantalla entera
#define VICEEXT_WSP_MAP_ANCHOR				// el mapa del menu y la radio del carrusel conservan su posicion al cambiar de resolucion
#define VICEEXT_WSP_HUD_ANCHOR				// el HUD se ancla a los bordes en vez de a la derecha
#define VICEEXT_WSP_CROSSHAIR				// la reticula escala con el aspecto
#define VICEEXT_WSP_MENU_CANVAS			// el menu se dibuja en un lienzo 16:9 centrado con el raton re-mapeado
#define VICEEXT_WSP_MENU_SCALES			// las escalas del frontend se calculan por clase y no por puntero
#define VICEEXT_WSP_SCRIPT_CANVAS			// los textos de script se dibujan en el lienzo centrado
#define VICEEXT_WSP_MENU_CONSTANTS			// las posiciones del briefing, de las estadisticas y de la leyenda se corrigen al ancho
```

> Cabecera de atribución obligatoria sobre cada bloque portado (según el plan y `docs/mods/ATTRIBUTION.md`): `PORTADO -- WidescreenFixesPack (MIT, (c) ThirteenAG)`. Es la única excepción al rule de "sin comentarios" y la pone el integrador, no este documento.


---

### 4 · C7 · Menú en lienzo 16:9 + mapa de 448 px + ratón re-mapeado (C7+C9+C17)

- **Fichero**: `src/core/Frontend.cpp` (y `src/core/Frontend.h` para las dos variables nuevas)
- **Bloque**:
  - `src/core/Frontend.cpp:183-194` (las tres macros de horizontal del menú: `MENU_X_RIGHT_ALIGNED`, `MENU_X_LEFT_ALIGNED`, `MENU_X`, `MENU_Y`)
  - `src/core/Frontend.cpp:2903-2962` (`CMenuManager::DrawBackground`: fondo + bordes)
  - `src/core/Frontend.cpp:6821-6873` (`CMenuManager::PrintMap`: el mapa del menú)
  - `src/core/Frontend.cpp:563-565` y `:642-644` (inicialización de `m_fMapSize` / `m_fMapCenterX` / `m_fMapCenterY`)
  - `src/core/Frontend.cpp:5058-5066` (lectura y recorte de la posición del ratón)
  - `src/core/Frontend.cpp:627-644` (`CMenuManager::Initialise`, donde se recalcula la geometría del menú)
- **Qué hace hoy**:
  - El port usa **el raster completo** para el menú: el fondo se dibuja a `CRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT)` (`src/core/Frontend.cpp:2921`, `:2943`) y los bordes con `SCREEN_STRETCH_*` sobre el ancho completo (`:2926-2961`).
  - El contenido del menú (opciones, mapa, leyenda) se coloca en una **caja 4:3 centrada**: `MENU_X_LEFT_ALIGNED(x) = SCALE_AND_CENTER_X(x)` (`src/core/Frontend.cpp:187`), y `SCALE_AND_CENTER_X` (`src/core/common.h:208`) = `(SCREEN_WIDTH - SCREEN_SCALE_X(640))/2 + SCREEN_SCALE_X(x)`.
  - Medido: con `PROPER_SCALING` (`src/core/config.h:443`) y `DEFAULT_SCREEN_HEIGHT = 480` (`src/core/common.h:154-161`), `SCREEN_SCALE_X(a)` y `SCREEN_SCALE_Y(a)` son **el mismo factor** (`SCREEN_HEIGHT/480`): el contenido **no se deforma**, solo se centra. Esa es la diferencia con el mod, que mete el menú en un lienzo 16:9 y reparte el contenido por todo ese ancho (`Draw.ixx:111-222`, `MenuConstraint.ixx:243-260`).
  - El ratón se recorta al raster completo: `if (m_nMousePosX > SCREEN_WIDTH) m_nMousePosX = SCREEN_WIDTH;` (`src/core/Frontend.cpp:5063-5064`).
  - El mapa: `m_fMapSize = MENU_Y(162.0f)` y `m_fMapCenterX = MENU_X_LEFT_ALIGNED(320.0f)` (`src/core/Frontend.cpp:563-565`) → **el mapa del port ya no se deforma**; el mod lo que hace es darle un lienzo propio de 448 px de alto (`Frontend.ixx:493-600`, `Height = 448.0f`).
  - El menú a pantalla completa **ya está verificado** por el 2.0-B: `docs/mods/ATTRIBUTION.md:320` ("Menus a pantalla completa … `src/core/Frontend.cpp:2346-2373` … **YA ESTABA**: verificado, sin cambios. PASS = menu cubre el viewport"). *(La línea citada ahí (`2346-2373`) está desfasada respecto al árbol actual: hoy el bloque es `2903-2962`.)*
- **Cambio propuesto** (todo detrás de `VICEEXT_WSP_MENU_CANVAS`, apagado por defecto):
  1. **Dos variables nuevas** en `CMenuManager` (`src/core/Frontend.h`): `float m_fMenuCanvasOffset;` y `float m_fMenuCanvasWidth;`.
  2. **Cálculo del lienzo** (nuevo `CMenuManager::UpdateMenuCanvas()`, llamado desde `CMenuManager::Initialise` —`src/core/Frontend.cpp:627-644`— y tras cada cambio de modo de vídeo): con la opción en AUTO, `m_fMenuCanvasWidth = SCREEN_WIDTH` y `m_fMenuCanvasOffset = 0` (**idéntico a hoy**); con 16:9 o 21:9, `m_fMenuCanvasWidth = min(SCREEN_HEIGHT * ratio, SCREEN_WIDTH)` (mínimo `SCREEN_HEIGHT * 4/3`, como el mod: `Draw.ixx:137-140`) y `m_fMenuCanvasOffset = (SCREEN_WIDTH - m_fMenuCanvasWidth) * 0.5f`.
  3. **Los helpers** (`src/core/Frontend.cpp:183-194`) pasan a usar el lienzo: `MENU_X_LEFT_ALIGNED(x)` = `m_fMenuCanvasOffset + x * factor` con el factor **uniforme** del port (`SCREEN_HEIGHT/480`) y el contenido repartido por el ancho del lienzo (el factor X interno del lienzo hay que **calibrarlo contra el retail**, porque el mod usa su propia escala aspect-aware `INV_SCREEN_WIDTH(ar) = 1/(480*ar)`, `Frontend.ixx:18`). Con el lienzo en AUTO, los helpers **deben dar exactamente los mismos números que hoy** (verificación por `assert`/comparación en el primer frame).
  4. **Ratón re-mapeado (C17)**: en `src/core/Frontend.cpp:5060-5064`, `m_nMousePosX = m_nMouseTempPosX - m_fMenuCanvasOffset` y el recorte pasa a `[0, m_fMenuCanvasWidth]`; con AUTO, offset 0 y ancho = `SCREEN_WIDTH` → **el mismo recorte de hoy**.
  5. **Fondo y bordes** (`src/core/Frontend.cpp:2903-2962`): el sprite de fondo y los cuatro bordes se dibujan **dentro del lienzo** (`m_fMenuCanvasOffset … m_fMenuCanvasOffset + m_fMenuCanvasWidth`) y los márgenes laterales se pintan de negro con `CSprite2d::DrawRect`; con AUTO, el rectángulo es el de hoy.
  6. **Mapa (C9)**: `m_fMapSize` / `m_fMapCenterX` / `m_fMapCenterY` (`src/core/Frontend.cpp:563-565`, `:642-644`) se calculan **con el lienzo** (centro = `m_fMenuCanvasOffset + m_fMenuCanvasWidth/2`, tamaño = `MENU_Y(162)` como hoy). El mapa del port **ya no se deforma** (escala uniforme), así que el cambio real es **dónde** se coloca, no su forma: con el lienzo 16:9 el mapa se sale del 4:3 y usa el ancho.
  7. **Ratón en el mapa**: `mapCrosshair` (`src/core/Frontend.cpp:6929-6934`) usa `m_nMousePosX`, que ya viene re-mapeado.
- **Bandera/clave que necesita**: `VICEEXT_WSP_MENU_CANVAS`; clave de INI `MenuAspectRatioConstraint` (valores 0=AUTO, 1=16:9, 2=21:9).
- **Dependencia**:
  - `src/core/Frontend.h` (+2 variables, +1 método) — fuera de mi territorio, lo pide el integrador.
  - La página nueva del §2 (para que el jugador pueda encenderlo) y la opción 14.
  - `src/core/common.h:208` (`SCALE_AND_CENTER_X`) y `src/core/common.h:195-208` (las macros de escalado) — **no se tocan**: el lienzo se implementa en los helpers del menú, no en las macros globales.
  - `docs/mods/ATTRIBUTION.md:320` (la fila del menú a pantalla completa hay que actualizarla: pasa de "YA ESTABA" a "con la opción de lienzo 16:9" cuando se aplique).
- **Riesgo**:
  - **Es el cambio que más se ve**: con el lienzo 16:9 el menú deja de ocupar la pantalla completa (lo que el 2.0-B verificó como correcto) y aparece margen a los lados. **Por eso la opción tiene que estar apagada de serie** y el PASS es del jugador con el menú delante.
  - La calibración del factor X **dentro** del lienzo es lo único que no se puede deducir del código del port: el mod reescribe punteros de escala del retail (`Frontend.ixx:667-743`), cosa que el port **no** hace (`ADR-001`: el port aplica el comportamiento a su propio código, no copia la técnica de escaneo). Si el factor sale mal, la opción queda con el texto descolocado → el criterio de verificación es numérico (que las etiquetas caigan dentro del lienzo), no de gusto.
  - El ratón re-mapeado tiene que ir **de la mano** del lienzo: si el ratón se re-mapea y el lienzo no, el puntero se va a la esquina; y al revés.
  - `CMenuManager::Initialise` (`src/core/Frontend.cpp:627`) no es el único punto donde cambia la geometría: `LoadAllTextures` (`:3654-3660`) y `ChangeRadioStation` (`:6959-6973`) también dependen de `MENU_X*`; con el lienzo, el recálculo del offset tiene que ocurrir **antes** de dibujar.
- -> verify: con la opción en AUTO el menú se ve **exactamente igual que hoy** (comparación de capturas del jugador); con 16:9, el menú queda dentro de un lienzo con márgenes laterales iguales, el ratón se mueve solo dentro del lienzo (al llegar al borde del lienzo se para y no se sale), y el mapa se dibuja centrado en el lienzo sin deformarse. Criterio numérico del log: el ancho del lienzo y su offset salen en la traza `VEX wsp …` y coinciden con `min(SCREEN_HEIGHT*16/9, SCREEN_WIDTH)`.

---

### 5 · C8 · Escalas del frontend por clase + texto de script centrado (C8+C10)

- **Fichero**: `src/core/Frontend.cpp` (escalas) y `src/renderer/Hud.cpp` (texto de script)
- **Bloque**:
  - `src/core/Frontend.cpp:183-194` (los helpers de escalado del menú)
  - `src/core/Frontend.cpp` en general: **113** usos de `SCREEN_SCALE_*`, **68** de `SCREEN_STRETCH_*`, **10** de `SCALE_AND_CENTER_X`, **372** de `MENU_X_LEFT_ALIGNED`, **67** de `MENU_X_RIGHT_ALIGNED` (contados con `rg` sobre el fichero actual)
  - `src/core/common.h:195-208` (`SCREEN_STRETCH_*`, `SCREEN_SCALE_*`, `SCALE_AND_CENTER_X`)
  - `src/renderer/Hud.cpp:1466` y `src/renderer/Hud.cpp:1820` (los dos pases del texto de script)
  - master: `Frontend.ixx:667-743` (24 punteros de escala) y `Frontend.ixx:423-489` (`ScriptDraw`)
- **Qué hace hoy**:
  - El port escala el frontend **con macros**, no con punteros: `MENU_X(x) = SCREEN_SCALE_X(x)` y `MENU_Y(y) = SCREEN_SCALE_Y(y)` (`src/core/Frontend.cpp:188-189`), que salen de `SCREEN_STRETCH_*` + el factor de aspecto (`src/core/common.h:201-208`).
  - El texto de script **ya está en un lienzo centrado**: `CFont::PrintString(SCREEN_WIDTH - SCALE_AND_CENTER_X(DEFAULT_SCREEN_WIDTH - line.m_fAtX), …)` (`src/renderer/Hud.cpp:1466` y `:1820`), o sea, el mismo `SCALE_AND_CENTER_X` que usa el menú.
- **Cambio propuesto**: **ninguno**. Se clasifica como **CONFLICTO DUPLICADO**:
  - **C8**: la intención del master (que el frontend escale con el ancho) ya está; la técnica (24 punteros `ResXInvRefs`/`ResYInvRefs` reescritos) es **x86 y no portable** (`ADR-001`): el port la resuelve con macros, que además son **más baratas** (se evaluan en tiempo de compilación). Portar el master encima = **doble escalado**.
  - **C10**: idéntico. El master mueve los textos de script a un lienzo 4:3 centrado (`Frontend.ixx:429-445`, `GetOffsetX`), y el port **ya lo hace** con `SCALE_AND_CENTER_X` en los **dos** pases (`Hud.cpp:1466` y `:1820`). Implementar los dos = **doble desplazamiento**.
  - **Lo único que el master aporta aquí** son las **6 constantes de Brief/Stats/Leyenda** → es el item siguiente (C39) y va con su propia bandera.
- **Bandera/clave que necesita**: **ninguna** para C8/C10. La de C39 es `VICEEXT_WSP_MENU_CONSTANTS`.
- **Dependencia**: `src/core/Frontend.cpp` y `src/renderer/Hud.cpp` **no se tocan** en este item.
- **Riesgo**: **aplicar C8 o C10 sobre el port rompería el menú** (doble escala / doble desplazamiento). Por eso la fila de `ATTRIBUTION.md` de estas dos features debe quedar como *"CONFLICTO DUPLICADO — el port ya lo resuelve con macros; no se porta"* (no como "PENDIENTE"), y el plan (tanda W7) debe **sacarlas** de la lista de W7.
- -> verify: el log del jugador y una captura antes/después **no cambian** al aplicar W7 (porque C8/C10 no se tocan); la fila de `ATTRIBUTION.md` de las features 37 y 38 dice "CONFLICTO DUPLICADO" con el `fichero:línea` del port (`src/core/Frontend.cpp:188-189` y `src/renderer/Hud.cpp:1466,1820`).

---

### 6 · C39 · Arreglos de Brief / Stats / Leyenda

- **Fichero**: `src/core/Frontend.cpp` (los usos) y `src/core/Frontend.h` (las constantes)
- **Bloque** (medido):
  | Constante del master (`Frontend.ixx:745-771`) | Valor | Equivalente en el port | Fichero:línea |
  |---|---|---|---|
  | `fBriefWrapMult` | 0.875 (560/640) | `CFont::SetWrapx(MENU_X_RIGHT_ALIGNED(80.0f))` | `src/core/Frontend.cpp:3991` |
  | `fBriefXMult` (sitio 1) | 0.15625 (100/640) | `BRIEFS_LINE_X` = 100.0f | `src/core/Frontend.h:97` (usado en `src/core/Frontend.cpp:4009` y `:4010`) |
  | `fBriefXMult` (sitio 2) | 0.15625 | `MENU_X_LEFT_ALIGNED(BRIEFS_LINE_X)` | `src/core/Frontend.cpp:4009-4010` |
  | `fStatsXMult` | 0.171875 (110/640) | `STATS_ROW_LEFT_MARGIN` = 110.0f | `src/core/Frontend.h:84` (usado en `src/core/Frontend.cpp:4101`) |
  | `fStatsWrapMult` | 0.82343751 (530/640) | `MENU_X_RIGHT_ALIGNED(STATS_ROW_RIGHT_MARGIN)`, `STATS_ROW_RIGHT_MARGIN` = 113.0f | `src/core/Frontend.h:85`, `src/core/Frontend.cpp:4103` |
  | `fLegendLeft` | 0.25 (160/640) | `MENU_X_LEFT_ALIGNED(160.0f)` | `src/core/Frontend.cpp:6914` |
  | `fLegendRight` | 0.171875 (350/640 en el recálculo) | `MENU_X_LEFT_ALIGNED(350.0f)` | `src/core/Frontend.cpp:6920` |
- **Qué hace hoy**:
  - El briefing se imprime con `CFont::SetWrapx(MENU_X_RIGHT_ALIGNED(80.0f))` (`src/core/Frontend.cpp:3991`) y las líneas en `MENU_X_LEFT_ALIGNED(BRIEFS_LINE_X)` (`:4009-4010`).
  - Las estadísticas en `MENU_X_LEFT_ALIGNED(STATS_ROW_LEFT_MARGIN)` (`:4101`) y `MENU_X_RIGHT_ALIGNED(STATS_ROW_RIGHT_MARGIN)` (`:4103`).
  - La leyenda del mapa en `MENU_X_LEFT_ALIGNED(160.0f)` (`:6914`) y `MENU_X_LEFT_ALIGNED(350.0f)` (`:6920`).
  - Las 4 coordenadas **no son píxeles absolutos**: `MENU_X_LEFT_ALIGNED` = `SCALE_AND_CENTER_X` (`src/core/Frontend.cpp:187`), o sea que **ya están en la caja 4:3 centrada** (`src/core/common.h:208`).
- **Cambio propuesto**: detrás de `VICEEXT_WSP_MENU_CONSTANTS` (apagado de serie), **las 4 coordenadas pasan a ser función del ancho de la caja** con la misma fórmula que el master (`Frontend.ixx:804-813`):
  ```
  boxW  = min(ancho, alto * 4/3)
  off   = (ancho - boxW) / 2
  x_brief  = off + boxW * (100/640)
  x_briefW = off + boxW * (560/640)
  x_stats  = off + boxW * (110/640)
  x_statsW = off + boxW * (530/640)
  x_legL   = off + boxW * (160/640)
  x_legR   = off + boxW * (350/640)
  ```
  En el port, `boxW`/`off` **ya** son los de `SCALE_AND_CENTER_X`; lo que falta es poder **desviar** cada coordenada de su valor fijo a la fracción del master, y eso es lo que hace la bandera. Con la bandera **encendida** y el mismo ancho, los números **coinciden** con los de hoy (el puerto ya está en la caja 4:3) → el cambio real aparece **cuando el lienzo del menú (C7) está activo** (la caja pasa a ser 16:9 y estas 6 constantes tienen que seguir la caja, no el 4:3 fijo).
  → **Por eso C39 y C7 van juntos**: con C7 apagado, C39 es un no-op; con C7 encendido, C39 es lo que mantiene el briefing, las estadísticas y la leyenda dentro de la caja.
- **Bandera/clave que necesita**: `VICEEXT_WSP_MENU_CONSTANTS`. **Sin clave de INI**: es un ajuste interno que acompaña al lienzo del menú (el master tampoco lo tiene: son 6 constantes fijas recalculadas en cada cambio de resolución, `Frontend.ixx:773-816`).
- **Dependencia**: `src/core/Frontend.cpp` (4 puntos) + `src/core/Frontend.h` (las constantes de `BRIEFS_*` / `STATS_*`); el item C7 (el offset del lienzo).
- **Riesgo**:
  - Si se aplica C39 **sin** C7, las 6 constantes cambian de sitio en el briefing/estadísticas/leyenda sin motivo (el puerto ya las coloca en la caja 4:3) → **el briefing se mueve**. Por eso C39 **no** se aplica solo: se aplica **junto a** C7, o no se aplica.
  - Los valores del master (560/640, 530/640) **no** coinciden con los del port (80 px desde la derecha, 113 px): el master reescribe las constantes *nativas* del retail, el port usa las suyas. La conversión se hace con las fracciones de arriba, no copiando los números.
- -> verify: con C7 apagado y C39 encendido, el briefing, las estadísticas y la leyenda se ven **exactamente igual** que con C39 apagado (mismas coordenadas). Con C7 encendido, el briefing y la leyenda quedan **dentro** de la caja 16:9, sin salirse a los márgenes. Criterio: las coordenadas de las 6 constantes salen en la traza y coinciden con `off + boxW * fracción` con `boxW` de la caja activa.

---

### 7 · C14 · Sin pantalla de carga entre islas / interior (features 45 y 46)

- **Fichero**: `src/core/Streaming.cpp` (**sin tocar**)
- **Bloque**:
  - `src/core/Streaming.cpp:3425-3467` (`CStreaming::LoadScene`: purga, `RequestBigBuildings`, `RemoveIslandsNotUsed`, `LoadAllRequestedModels`, `InstanceBigBuildings`)
  - `src/core/Streaming.cpp:3496-3503` (`CStreaming::LoadSceneResetSteps`) y `:3505-3599` (`CStreaming::LoadSceneStep`: la escena de carga **troceada por ticks del navegador**)
  - `src/core/Streaming.cpp:2711-2830` (`CStreaming::LoadAllRequestedModels`: el bucle que **trae los ficheros**)
  - `src/core/Streaming.cpp:2726-2729` (`odBlockingPush()`: el motor **se bloquea** mientras pide los ficheros)
  - master: `Loading.ixx:291-387` (26 patrones: precarga de edificios grandes y **salta `LoadScene`**) e `InteriorLoading.ixx:527-737` (28 patrones: `CStreaming::LoadScene` reescrito, `RemoveBuildingsNotInArea`, el cambio de interior sin fundido)
- **Qué hace hoy**:
  - El port **carga por on-demand**: `CStreaming::LoadAllRequestedModels` (`src/core/Streaming.cpp:2713`) pide los ficheros que faltan y **bloquea el motor** con `odBlockingPush()` (`:2729`) porque en el navegador la página no puede "aplazar" un fichero a mitad de una cinemática (`:2726-2728`); con la condición de reintentos ya arreglada por D15 (`:2764-2777`).
  - El 2.0-B lo dejó **PENDIENTE con motivo** (`docs/mods/ATTRIBUTION.md:321`): "saltar `LoadScene` rompe el on-demand (los datos llegan por ahí); necesita garantía de residencia = diseño D15. No se inventa".
  - El port ya tiene el equivalente funcional del "no island loading" **del retail**: `NO_ISLAND_LOADING` (`src/core/config.h:695`) con las opciones `ISLAND_LOADING_LOW/MEDIUM/HIGH` (`src/core/MenuScreensCustom.cpp:161`, `src/core/Streaming.cpp:1436`, `:2154-2156`).
- **Cambio propuesto**: **ninguno**. Se clasifica como **NO APLICA / PENDIENTE con motivo (D15)**, y el motivo **sigue en pie**:
  1. El mod **no** salta la carga: **precarga todos los edificios de la isla** y luego **salta `LoadScene`** (`Loading.ixx:295-330`: `RequestBigBuildings` reescrito, `LoadScene` con `E990` = NOP, `RemoveIslandsNotUsed` con NOP, `LoadBigBuildingsWhenNeeded` con `RET`). Eso exige que **todos** los datos de la isla estén **residentes**.
  2. En el port los datos **no** están residentes: vienen por la capa on-demand (`gta_vc_browser/web/ondemand.js`, `bootseed/` + `streamed/` con caché IDB). Precargar una isla entera es **GBs por red**, y saltarse `LoadScene` dejaría **modelos sin pedir**: la capa on-demand no los traería nunca y la escena saldría **vacía** (el mismo motivo que el reintento de D15, `src/core/Streaming.cpp:2764-2777`).
  3. Saltar la pantalla de carga **además** quitaría el feedback de "está cargando" que el jugador ya ve, con la escena troceada por ticks (`src/core/Streaming.cpp:3505-3599`) que es justo lo que hace hoy la carga de forma **progresiva**.
  4. `NoIslandLoading` y `NoInteriorLoading` **se leen igual** en `[WidescreenFix]` (contrato, §4) y quedan **sin bandera**: no hay código de motor detrás.
- **Bandera/clave que necesita**: **ninguna**. Las claves `NoIslandLoading` y `NoInteriorLoading` se leen (contrato) y no hacen nada.
- **Dependencia**: `docs/mods/ATTRIBUTION.md:321` (la fila se queda como PENDIENTE con motivo, ahora con la cita de `Streaming.cpp:2711-2830` y `:3505-3599`).
- **Riesgo**: si alguien lo aplica "por el mismo nombre" (copiando el nombre de la feature), rompe el on-demand: la partida carga con el mundo **vacío**. Por eso **no hay bandera** y la fila del inventario dice PENDIENTE.
- -> verify: `grep -n "NoIslandLoading\|NoInteriorLoading" src/**` → solo el lector del §1 (contrato) y la fila del inventario; **ningún** `LoadScene`/`LoadSceneStep`/`LoadAllRequestedModels` tocado (diff del fichero `src/core/Streaming.cpp` **vacío** en la tanda).

---

### 8 · F41 · Anclaje del mapa / radio del carousel (feature 41)

- **Fichero**: `src/core/Frontend.cpp`
- **Bloque**:
  - `src/core/Frontend.cpp:3653-3660` (`CMenuManager::LoadAllTextures`: `m_LeftMostRadioX = MENU_X_LEFT_ALIGNED(MENURADIO_ICON_FIRST_X - MENURADIO_ICON_SIZE)`)
  - `src/core/Frontend.cpp:4573-4671` (dibujo del carrusel de la radio)
  - `src/core/Frontend.cpp:4675-4697` (la animación de scroll: compara con `MENU_X_LEFT_ALIGNED(MENURADIO_ICON_FIRST_X - MENURADIO_ICON_SIZE)` y mueve `MENU_X(6.f)` por tick)
  - `src/core/Frontend.cpp:6959-6973` (`CMenuManager::ChangeRadioStation`: pone `m_LeftMostRadioX` en uno de los dos extremos)
  - `src/core/Frontend.cpp:6735` (`m_bSpritesLoaded = false` en `UnloadTextures`) y `src/skel/glfw/glfw.cpp:744-758` (`_psSelectScreenVM` → `UnloadTextures` → `_psSetVideoMode` → `LoadAllTextures`)
  - `src/core/Frontend.h:731` (`int32 m_LeftMostRadioX`)
  - master: `Frontend.ixx:773-816` (`UpdateMenuLayout`: guarda `radioFraction = x / radioWidth` y la reaplica tras el cambio de resolución)
- **Qué hace hoy**:
  - El carrusel de la radio está en **píxeles de pantalla**: `m_LeftMostRadioX` es un `int32` (`src/core/Frontend.h:731`).
  - Al cambiar de resolución, `_psSelectScreenVM` (`src/skel/glfw/glfw.cpp:744-758`) **descarga y vuelve a cargar** las texturas del frontend → `LoadAllTextures` **re-ancla** el carrusel a la nueva escala (`src/core/Frontend.cpp:3660`). O sea: **el port ya re-ancla el carrusel** y no hay "deriva" al cambiar de resolución.
  - Lo que **no** hay: si el cambio de resolución ocurre **en mitad** de un scroll, la fracción recorrida se pierde (el carrusel vuelve al extremo).
  - El mapa (`m_fMapSize` / `m_fMapCenterX` / `m_fMapCenterY`, `src/core/Frontend.cpp:563-565` y `:642-644`) **también** se recalcula en `Initialise`, pero el zoom/pan que haya hecho el jugador (`src/core/Frontend.cpp:4166-4318`) **no** se conserva.
- **Cambio propuesto**: **aplica parcialmente**. Detrás de `VICEEXT_WSP_MAP_ANCHOR` (apagado de serie), **conservar la fracción de scroll** en vez de volver al extremo:
  ```cpp
  // en LoadAllTextures, antes de la línea 3660
  static int32 radioWidth = 0;
  static int32 radioPosition = 0;
  static float radioFraction = 0.0f;
  int32 left = MENU_X_LEFT_ALIGNED(MENURADIO_ICON_FIRST_X - MENURADIO_ICON_SIZE);
  int32 right = MENU_X_LEFT_ALIGNED(MENURADIO_ICON_FIRST_X);
  if (radioWidth == 0 || m_LeftMostRadioX != radioPosition)
      radioFraction = (right > left) ? float(m_LeftMostRadioX - left) / float(right - left) : 0.0f;
  radioWidth = right - left;
  radioPosition = left + int32(radioFraction * float(radioWidth));
  m_LeftMostRadioX = radioPosition;
  ```
  (el master hace lo mismo: `Frontend.ixx:781-789`, con `radioFraction = x / radioWidth`).
  El **mapa** (el zoom y el pan) **no** se toca: el port ya lo recalcula al reiniciar el frontend y el master tampoco conserva el zoom del mapa, solo el ancla.
- **Bandera/clave que necesita**: `VICEEXT_WSP_MAP_ANCHOR`. **Sin clave de INI** (el master tampoco la tiene: es una feature sin clave).
- **Dependencia**: `src/core/Frontend.cpp:3653-3660`; nada más.
- **Riesgo**: bajo. El carrusel solo se mueve si el cambio de resolución cae en mitad del scroll (el cambio de resolución en el navegador **re-inicia el canvas** y recarga las texturas, `src/skel/glfw/glfw.cpp:1236-1243`). Con la bandera apagada, el comportamiento es **exactamente** el de hoy.
- -> verify: con la bandera encendida, cambiar de resolución **mientras** la radio se desplaza deja el carrusel en el **mismo punto del recorrido** (no en el extremo); con la bandera apagada, el carrusel vuelve al extremo como hoy. Criterio: la fracción (0..1) se puede leer de la traza.

---

## 9 · (a) Las 32 claves del master del WFP — contrato `[WidescreenFix]`

Sección del master = en qué sección lo lee **el mod** (`Legacy.ixx`, `Misc.ixx`, `MSAA.ixx`, `PostFX.ixx`, `TransparentMenu.ixx`, `Radardisc.ixx`, `InteriorLoading.ixx`, `Loading.ixx`, `Menu.ixx`, `MenuConstraint.ixx`, `Draw.ixx`, `Frontend.ixx`).
Sección del port = **siempre `[WidescreenFix]`** (D3).
"Default del mod" = **el del código del mod** (medido), no el del comentario del INI.
"Default del port" = el neutro, para que nada cambie por defecto.

| # | Clave | Sección master | Tipo | Default del mod (código) | Default del port | Variable del port | Destino en el port | Menú |
|---|---|---|---|---|---|---|---|---|
| 1 | `ResX` | MAIN | int | `-1` (`Legacy.ixx:327`) | 0 | `g_nWSPOptionResX` | **NO APLICA**: el canvas del navegador manda | no |
| 2 | `ResY` | MAIN | int | `-1` (`Legacy.ixx:328`) | 0 | `g_nWSPOptionResY` | **NO APLICA** (idem) | no |
| 3 | `ForceAspectRatio` | MAIN | string | `"auto"` (`Legacy.ixx:329`) | `"auto"` | `g_szWSPOptionForceAspectRatio` | W4: `W:H` libre (hoy la lista es cerrada, `src/renderer/Draw.cpp:35-52`) | no (string) |
| 4 | `FrontendAspectRatio` | MAIN | string | `"auto"` (`Legacy.ixx:330`) | `"auto"` | `g_szWSPOptionFrontendAspectRatio` | W4: recorte del frontend (hoy `src/renderer/Sprite2d.cpp:115-139`) | no (string) |
| 5 | `DontTouchFOV` | MAIN | int | `0` (`Legacy.ixx:331`) | 0 | `g_nWSPOptionDontTouchFOV` | W1: el FOV no se estira (`src/renderer/Draw.cpp:96-106`) | sí |
| 6 | `RestoreCutsceneFOV` | MAIN | int | `1` (`Legacy.ixx:332`) | 1 | `g_nWSPOptionRestoreCutsceneFOV` | **YA ESTABA**: el port no convierte el FOV en cinemática (`src/renderer/Draw.cpp:100-103`) | no |
| 7 | `HudWidthScale` | MAIN | float | `0.0` → **`1.0`** (`Legacy.ixx:334`) | 1.0 | `g_fWSPOptionHudWidthScale` | W2 (C1): multiplicador X del HUD | sí (slider) |
| 8 | `HudHeightScale` | MAIN | float | `0.0` → **`1.0714285`** (`Legacy.ixx:335`) | **1.0** (neutro) | `g_fWSPOptionHudHeightScale` | W2 (C1): multiplicador Y del HUD | sí (slider) |
| 9 | `RadarWidthScale` | MAIN | float | `0.0` → **`1.0`** (`Legacy.ixx:336`) | 1.0 | `g_fWSPOptionRadarWidthScale` | W2: factor de escala del radar (`src/core/Radar.cpp:1577-1581`) | sí (slider) |
| 10 | `SubtitlesScale` | MAIN | float | `0.0` → **`1.0`** (`Legacy.ixx:337`) | 1.0 | `g_fWSPOptionSubtitlesScale` | W3 (C18): escala de los subtítulos (`src/renderer/Hud.cpp:1500-1550`) | sí (slider) |
| 11 | `RadarWidth` | MAIN | float | `94.0` (`Frontend.ixx:644`) | 94.0 | `g_fWSPOptionRadarWidth` | W2 (C2): ancho del disco (hoy 82×82 o 94×76, `src/core/Radar.h:181-182`) | no |
| 12 | `RadarHeight` | MAIN | float | `76.0` (`Frontend.ixx:645`) | 76.0 | `g_fWSPOptionRadarHeight` | W2 (C2): alto del disco | no |
| 13 | `HudAspectRatioConstraint` | MAIN | string | `""` (`Draw.ixx:328`) | `""` → 0 | `g_nWSPOptionHudAspectRatio` | W2 (C6): ancla del HUD (`Auto`/`16:9`/`21:9` → 0/1/2) | sí (select) |
| 14 | `MenuAspectRatioConstraint` | MAIN | string | `"Auto"` (`MenuConstraint.ixx:252`) | `"Auto"` → 0 | `g_nWSPOptionMenuAspectRatio` | W7 (C7): lienzo del menú | sí (select) |
| 15 | `NoCutsceneBorderAnimation` | MAIN | int | `0` (`Frontend.ixx:647`) | 0 | `g_nWSPOptionNoCutsceneBorderAnimation` | W4 (C3): bordes de cinemática animados | sí |
| 16 | `MenuFrameLimit` | MAIN | int | `60` (`Menu.ixx:567`) | 0 | `g_nWSPOptionMenuFrameLimit` | **NO APLICA**: el frame rate lo da el navegador | no |
| 17 | `SmartCutsceneBorders` | MISC | int | `1` (`Legacy.ixx:339`) | 0 (el interruptor del port manda: `Display/CutsceneBorders`) | `g_nWSPOptionSmartCutsceneBorders` | W4 (C3): bordes adaptativos; **colisión** con `src/core/MenuScreensCustom.cpp:47` | sí |
| 18 | `AllowAltTabbingWithoutPausing` | MISC | int | `0` (`Legacy.ixx:340`) | 0 | `g_nWSPOptionAllowAltTabbingWithoutPausing` | **NO APLICA**: en web manda `visibilitychange` | no |
| 19 | `HideAABug` | MISC (y GRAPHICS) | int | `0` (`Legacy.ixx:341`) / `1` (`MSAA.ixx:45`, recortado 0..2) | 0 | `g_nWSPOptionHideAABug` | **NO APLICA**: bug de MSAA de Direct3D 8 | no |
| 20 | `SmallerVehicleCorona` | MISC | int | `0` (`Legacy.ixx:342`) | 0 | `g_nWSPOptionSmallerVehicleCorona` | W5 (C12): luces de vehículo más pequeñas (`src/renderer/Coronas.cpp:374-394`) | sí |
| 21 | `NoLightSquare` | MISC | int | `0` (`Legacy.ixx:343`) | 0 | `g_nWSPOptionNoLightSquare` | W5: sin el cuadrado de la luz | sí |
| 22 | `ForceMultisamplingLevel` | MISC (y GRAPHICS) | string / int | `""` (`Legacy.ixx:344`) / `0` (`Legacy.ixx:354`) | `""` | `g_szWSPOptionForceMultisamplingLevel` | **NO APLICA**: `IDirect3DDevice8` | no |
| 23 | `IVRadarScaling` | MISC | int | `0` (`Legacy.ixx:345`) | 0 | `g_nWSPOptionIVRadarScaling` | W2 (C2): radar al estilo GTA IV (88,5×81) | sí (select) |
| 24 | `ReplaceTextShadowWithOutline` | MISC | int | `0` (`Legacy.ixx:346`, `Misc.ixx:31`) | 0 | `g_nWSPOptionReplaceTextShadowWithOutline` | W3 (C11): contorno de 16 direcciones (`src/renderer/Font.h:186` está declarada y sin usar) | sí (select 0/1/2) |
| 25 | `CarSpeedDependantFOV` | MISC | int | `1` (`Misc.ixx:32`) | 0 | `g_nWSPOptionCarSpeedDependantFOV` | W1 (C15): FOV por velocidad en la cámara VC (`src/core/Cam.cpp:2123`); **el master lo lee como int 0/1, no como float** | sí |
| 26 | `VCSCamShake` | MISC | int | `0` (`Misc.ixx:33`) | 0 | `g_nWSPOptionVCSCamShake` | W6: sacudida de cámara VCS | sí |
| 27 | `NoInteriorLoading` | MISC | int | `0` (`InteriorLoading.ixx:527`) | 0 | `g_nWSPOptionNoInteriorLoading` | **PENDIENTE (D15)**: ver item 7 | no |
| 28 | `NoIslandLoading` | MISC | int | `1` (`Loading.ixx:291`) | 0 | `g_nWSPOptionNoIslandLoading` | **PENDIENTE (D15)**: ver item 7 | no |
| 29 | `HQRadarDisc` | MISC | int | `0` (`Radardisc.ixx:65`) | 0 | `g_nWSPOptionHQRadarDisc` | **NO APLICA**: el `hud.txd` servido ya trae `radardisc` (decidido el 04/10) | no |
| 30 | `SMAA` | GRAPHICS | int | `0` (`PostFX.ixx:36`; el INI del mod trae 1) | 0 | `g_nWSPOptionSMAA` | **NO APLICA**: requiere `IDirect3DDevice9` | no |
| 31 | `TransparentMenu` | GRAPHICS | int | `0` (`TransparentMenu.ixx:46`) | 0 | `g_nWSPOptionTransparentMenu` | **NO APLICA**: D3D8/D3D9 | no |
| 32 | `ConsoleGamma` | GRAPHICS | int | `0` (`PostFX.ixx:35`) | 0 | `g_nWSPOptionConsoleGamma` | **NO APLICA**: requiere `IDirect3DDevice9` | no |

Reparto: **MAIN 16**, **MISC 13**, **GRAPHICS 5** (con `HideAABug` y `ForceMultisamplingLevel` leídos en dos secciones por el master: 32 claves distintas, 34 lecturas).
En el port: **las 32 en `[WidescreenFix]`** (D3).

### 9.1 · Claves de SkyGfx — **reservadas al integrador / al subagente de `src/extras/`**

Este item **no** escribe ninguna clave de SkyGfx. Lo que el integrador tiene que tener en cuenta (medido en `gta_vc_browser/tmp/rea/skygfx-16-rea.md` §2.1):

- SkyGfx expone **46 claves** (no 41) y el port **ya** lee 9 de ellas en sus propias secciones: `Rendering/YCbCrCorrection`, `Rendering/lumaScale`, `Rendering/lumaOffset`, `Rendering/CbScale`, `Rendering/CbOffset`, `Rendering/CrScale`, `Rendering/CrOffset` (`src/core/re3.cpp:764-770`), `Rendering/BackfaceCulling` (`:779`) y `CustomPipesValues/NeoVehicleShininess` … `CustomPipesValues/GlossMult` (`:773-777`).
- Las **nuevas** de SkyGfx que el subagente de `src/extras/` tenga que añadir (S1–S4 del plan) van **en sus secciones** (`Rendering/…`, `CustomPipesValues/…`), **nunca** en `[WidescreenFix]`: `envMapSize`, `worldPipe`, `texblendSwitch`, `texgenSwitch`, `ps2light`, `seamFix`, `dualPass`, `disableColourOverlay`, `trailsSwitch`, `trailsBlur`, `trailsMotionBlur`, `currentBlurAlpha`, `currentBlurOffset`, `neoWaterDrops` (modos 2/3).
- **Petición explícita al integrador**: que las claves de SkyGfx **no** se mezclen con las del WFP en el mismo lector, y que la sección `[WidescreenFix]` **solo** contenga las 32 claves de la tabla §9. Si el subagente de SkyGfx necesita una variable compartida, que la declare en su módulo (`src/extras/`) y no la lea del lector del WFP.
- **Riesgo**: si las claves de SkyGfx se meten en `[WidescreenFix]`, el INI del WFP y el del SkyGfx se mezclan y el "contrato" deja de ser verificable contra el INI del mod.

---

## 10 · (b) Lista final de banderas `VICEEXT_WSP_*` y dónde va cada una

Bloque: **uno nuevo**, "WidescreenFixesPack (plan 16.0)", en `src/core/config.h`, **insertado después de la línea 556** (`#define VICEEXT_SP_OPTIONS`, fin del bloque de SilentPatch) y **antes de la línea 558** (el comentario de "v2.5 Remove zeros in the money in the HUD"). Nada se reordena. Fichero **CRLF**.

| # | Bandera | Bloque de `config.h` | Qué hace (una línea) | Clave INI | Tanda |
|---|---|---|---|---|---|
| 1 | `VICEEXT_WSP_OPTIONS` | WSP nuevo (tras 556) | el contrato `[WidescreenFix]`: las 32 claves del master y sus mandos en el menú | — (todas) | T1 |
| 2 | `VICEEXT_WSP_HUD_SCALE` | WSP nuevo | multiplicador de escala del HUD en X e Y | `HudWidthScale`, `HudHeightScale` | W2 |
| 3 | `VICEEXT_WSP_RADAR_SCALE` | WSP nuevo | factor de escala del radar y tamaño configurable | `RadarWidthScale`, `RadarWidth`, `RadarHeight`, `IVRadarScaling` | W2 |
| 4 | `VICEEXT_WSP_HUD_ASPECT` | WSP nuevo | el HUD se ancla a la izquierda o a la derecha según el ancho | `HudAspectRatioConstraint` | W2 |
| 5 | `VICEEXT_WSP_TEXT_OUTLINE` | WSP nuevo | contorno de texto de 16 direcciones en vez de la sombra difusa | `ReplaceTextShadowWithOutline` | W3 |
| 6 | `VICEEXT_WSP_SMALL_TEXT_SHADOW` | WSP nuevo | sombra de texto de 1 px en vez de 2 | `SmallerTextShadows` (build 2018) | W3 |
| 7 | `VICEEXT_WSP_SUBTITLES` | WSP nuevo | escala, anchura y posición de los subtítulos, con la reserva del radar | `SubtitlesScale` | W3 |
| 8 | `VICEEXT_WSP_FOV_SPEED` | WSP nuevo | el FOV crece con la velocidad del coche en la cámara VC | `CarSpeedDependantFOV` | W1 |
| 9 | `VICEEXT_WSP_FOV_DONT_TOUCH` | WSP nuevo | el FOV no se estira por el aspecto | `DontTouchFOV` | W1 |
| 10 | `VICEEXT_WSP_FOV_CUTSCENE` | WSP nuevo | el FOV de cinemática sigue el zoom del borde | `RestoreCutsceneFOV`, `NoCutsceneBorderAnimation` | W1/W4 |
| 11 | `VICEEXT_WSP_FOV_MULTIPLIERS` | WSP nuevo | API de multiplicadores de FOV por hash | — | W1 |
| 12 | `VICEEXT_WSP_FOV_EMERGENCY` | WSP nuevo | el FOV de los vehículos de emergencia se recalcula a 70/factor | — | W1 |
| 13 | `VICEEXT_WSP_CAMERA_WIDE` | WSP nuevo | la cámara no colisiona con la geometría en pantallas de 2:1 o más | — | W1 |
| 14 | `VICEEXT_WSP_CUTSCENE_BORDERS` | WSP nuevo | los bordes de cinemática siguen el aspecto en vez del 30 % fijo | `SmartCutsceneBorders` | W4 |
| 15 | `VICEEXT_WSP_ASPECT_RATIO` | WSP nuevo | aspecto forzado libre `W:H` | `ForceAspectRatio` | W4 |
| 16 | `VICEEXT_WSP_FRONTEND_ASPECT` | WSP nuevo | el frontend se recorta al aspecto pedido con barras laterales | `FrontendAspectRatio` | W4 |
| 17 | `VICEEXT_WSP_SMALL_CORONA` | WSP nuevo | las luces de los vehículos se dibujan más pequeñas | `SmallerVehicleCorona` | W5 |
| 18 | `VICEEXT_WSP_NO_LIGHT_SQUARE` | WSP nuevo | las luces de los vehículos no dibujan el cuadrado | `NoLightSquare` | W5 |
| 19 | `VICEEXT_WSP_BRIGHT_LIGHTS_FAR` | WSP nuevo | las luces brillantes solo se dibujan cuando la cámara está lejos | — | W5 |
| 20 | `VICEEXT_WSP_PARTICLES` | WSP nuevo | las partículas escalan con el ancho de la pantalla | — | W6 |
| 21 | `VICEEXT_WSP_CAM_SHAKE_VCS` | WSP nuevo | la cámara tiembla con la velocidad al estilo VCS | `VCSCamShake` | W6 |
| 22 | `VICEEXT_WSP_TRAFFIC_DISTANCE` | WSP nuevo | el tráfico se genera más lejos en pantallas anchas | — | W6 |
| 23 | `VICEEXT_WSP_BULLET_SMOKE` | WSP nuevo | el impacto de bala deja humo | — | W6 |
| 24 | `VICEEXT_WSP_FADE_RECT` | WSP nuevo | el fundido usa el rectángulo de contenido y no la pantalla entera | — | W6 |
| 25 | `VICEEXT_WSP_MAP_ANCHOR` | WSP nuevo | el mapa del menú y la radio del carrusel conservan su posición al cambiar de resolución | — | W6 |
| 26 | `VICEEXT_WSP_HUD_ANCHOR` | WSP nuevo | el HUD se ancla a los bordes en vez de a la derecha | `HudAspectRatioConstraint` | W2 |
| 27 | `VICEEXT_WSP_CROSSHAIR` | WSP nuevo | la retícula escala con el aspecto | — | W2 |
| 28 | `VICEEXT_WSP_MENU_CANVAS` | WSP nuevo | el menú se dibuja en un lienzo 16:9 centrado con el ratón re-mapeado | `MenuAspectRatioConstraint` | W7 |
| 29 | `VICEEXT_WSP_MENU_SCALES` | WSP nuevo | las escalas del frontend se calculan por clase y no por puntero | — | W7 (C8, **no se aplica**) |
| 30 | `VICEEXT_WSP_SCRIPT_CANVAS` | WSP nuevo | los textos de script se dibujan en el lienzo centrado | — | W7 (C10, **no se aplica**) |
| 31 | `VICEEXT_WSP_MENU_CONSTANTS` | WSP nuevo | las posiciones del briefing, de las estadísticas y de la leyenda se corrigen al ancho | — | W7 (C39) |

**Sin bandera** (NO APLICA / PENDIENTE, leídas y nada más): `ResX`, `ResY`, `MenuFrameLimit`, `ForceMultisamplingLevel`, `HideAABug`, `SMAA`, `ConsoleGamma`, `TransparentMenu`, `AllowAltTabbingWithoutPausing`, `NoIslandLoading`, `NoInteriorLoading`, `HQRadarDisc`. **12 claves sin bandera.**

> Nota: las banderas 29 y 30 (`VICEEXT_WSP_MENU_SCALES`, `VICEEXT_WSP_SCRIPT_CANVAS`) **se declaran pero no se usan** (C8 y C10 son CONFLICTO DUPLICADO). Declararlas sin uso es ruido; la alternativa es **no declararlas** y que la fila de `ATTRIBUTION.md` diga directamente "CONFLICTO DUPLICADO — el port ya lo resuelve". **Recomendación: no declararlas** (29 banderas en vez de 31). La lista de arriba las deja por si el jugador prefiere el rastro explícito.

---

## 11 · (c) Resumen de items y ficheros

| Item | Fichero a tocar | Ficheros de apoyo | Bloque medido | Bandera |
|---|---|---|---|---|
| T1-1 · Contrato de INI | `src/core/re3.cpp` | `src/core/WidescreenFixOptions.h/.cpp` (**nuevos**), `src/core/SilentPatchOptions.h` (precedente) | `re3.cpp:645-661`, `:800-825`, `:955-970` | `VICEEXT_WSP_OPTIONS` |
| T1-2 · Menú | `src/core/MenuScreensCustom.cpp` | `src/core/Frontend.h` (**+1 página**), `gta_vc_browser/streamed/TEXT/*.gxt` (**19 claves**), `gta_vc_browser/bootseed/TEXT/*.gxt` (copia), `gta_vc_browser/web/ondemand.js` (**dataTag**) | `MenuScreensCustom.cpp:768-792`, `:36-101`, `:102-132`; `Frontend.h:427-434`, `:225-247` | `VICEEXT_WSP_OPTIONS` |
| T1-3 · Banderas | `src/core/config.h` | — | tras la línea 556 | las 29–31 de §10 |
| C7 · Lienzo del menú | `src/core/Frontend.cpp` | `src/core/Frontend.h` (+2 variables, +1 método) | `Frontend.cpp:183-194`, `:2903-2962`, `:6821-6873`, `:5058-5066`, `:627-644` | `VICEEXT_WSP_MENU_CANVAS` |
| C8 · Escalas del frontend | **no se toca** | `src/renderer/Hud.cpp:1466,1820` (ya lo hacen) | `Frontend.cpp:183-194`; `Hud.cpp:1466,1820` | — (CONFLICTO DUPLICADO) |
| C39 · Brief/Stats/Leyenda | `src/core/Frontend.cpp` | `src/core/Frontend.h` (constantes) | `Frontend.cpp:3991`, `:4009-4010`, `:4101`, `:4103`, `:6914`, `:6920`; `Frontend.h:84-97` | `VICEEXT_WSP_MENU_CONSTANTS` |
| C14 · Carga sin pantalla | **no se toca** | `docs/mods/ATTRIBUTION.md:321` | `Streaming.cpp:3425-3467`, `:3496-3599`, `:2711-2830` | — (PENDIENTE D15) |
| F41 · Carrusel de la radio | `src/core/Frontend.cpp` | `src/skel/glfw/glfw.cpp:744-758` | `Frontend.cpp:3653-3660`, `:4675-4697`, `:6959-6973` | `VICEEXT_WSP_MAP_ANCHOR` |

---

## 12 · CRLF/LF detectados (por fichero, medido byte a byte)

| Fichero | Estilo | Nº de saltos |
|---|---|---|
| `src/core/re3.cpp` | **CRLF** | 1700 |
| `src/core/config.h` | **CRLF** | 837 |
| `src/core/MenuScreensCustom.cpp` | **CRLF** | 847 |
| `src/core/Frontend.cpp` | **CRLF** | 7956 |
| `src/core/Streaming.cpp` | **CRLF** | 3712 |
| `src/core/Frontend.h` | **CRLF** | 980 |
| `src/core/common.h` | **CRLF** | 443 |
| `src/core/SilentPatchOptions.h` | **LF** | 20 |
| `src/core/SilentPatchOptions.cpp` | **LF** | 62 |
| `src/extras/frontendoption.h` | **CRLF** | 88 |
| `src/extras/frontendoption.cpp` | **CRLF** | 170 |
| `src/core/Radar.h` / `Radar.cpp` | CRLF | 327 / 2194 |
| `src/renderer/Hud.cpp` | **CRLF** | 2536 |
| `src/renderer/Draw.cpp` | **CRLF** | 112 |
| `src/core/Camera.cpp` / `Cam.cpp` | CRLF | 4373 / 6748 |
| `docs/mods/ATTRIBUTION.md` | **MIXED** (516 CRLF + 2 LF) | 518 |
| `gta_vc_browser/tmp/rea/wfp-16-master-rea.md` | **CRLF** | 708 |
| `gta_vc_browser/tmp/rea/wfp-16-rea.md` | **LF** | 732 |
| master `Frontend.ixx` / `Draw.ixx` / `Menu.ixx` / `MenuConstraint.ixx` / `Legacy.ixx` / `Misc.ixx` / `Loading.ixx` / `InteriorLoading.ixx` | **LF** | 1148 / 345 / 659 / 266 / 1167 / 97 / 389 / 751 |
| master `GTAVC.WidescreenFix.ini` | **LF** | 35 |
| **nuevo** `src/core/WidescreenFixOptions.h/.cpp` | **LF** (como `SilentPatchOptions.*`) | — |
| **nuevo** `gta_vc_browser/tmp/rea/16-enrich-menus-ini.md` | **LF** | — |

Regla que se hereda de `.agents/CODING_STANDARDS.md` §6: **editar `.cpp/.h` con script binario en Python y `assert count == 1`**, generando los patrones con el estilo del fichero destino; los GXT con `gta_vc_browser/tools/gxt_inspect.py` (que **no** sobrescribe claves existentes).

---

## 13 · Decisiones que este item deja abiertas para el jugador

1. **19 claves GXT nuevas** (6 idiomas) → **los datos servidos cambian** → el `dataTag` (`gta_vc_browser/web/ondemand.js:130`) **sube**, y el plan (T13) dice "dataTag sin subir". Sin esto, la página nueva no tiene etiquetas (salen como `"FEM_HWS missing"`).
2. **Página de menú nueva**: exige **+1 entrada en el enum `MENUPAGES`** (`src/core/Frontend.h:238-247`) y la entrada correspondiente en `aScreens[]` (`src/core/MenuScreensCustom.cpp:366-845`). No hay hueco (`FrontendScreenAdd` no se usa y su `assert` no valdría).
3. **`HudHeightScale`**: el default del mod es `1.0714285` (código) y el neutro del port `1.0`. Se propone 1.0 (nada cambia por defecto) y que el `1.0714285` se aplique solo si el usuario lo escribe.
4. **`ReplaceTextShadowWithOutline`** (0/1/2) entra en la página del WFP **o** se resuelve con la capa del 14.0 (`src/core/SilentPatchOptions.cpp`): el plan avisa (tabla de conflictos, C11) de que hay que decidir **una sola** de las dos capas.
5. **`VICEEXT_WSP_MENU_SCALES` y `VICEEXT_WSP_SCRIPT_CANVAS`**: se declaran sin usar (rastro) o no se declaran (recomendación: no).
6. **C8 y C10 salen de la tanda W7** (son duplicados): el plan los tenía como "CONFLICTO DUPLICADO" y la lista de W7 los incluye; **no se aplican**.
