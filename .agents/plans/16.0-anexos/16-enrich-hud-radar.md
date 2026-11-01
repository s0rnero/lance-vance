# Enriquecimiento HUD / radar / subtítulos — plan 16.0 (WFP master)

> Subagente de **ENRIQUECIMIENTO** (no implementa nada). Territorio:
> `src/renderer/Hud.cpp`, `src/core/Radar.cpp`, `src/core/Radar.h`,
> `src/core/common.h` (solo el bloque de macros `SCREEN_*`, líneas 194-218).
> Todo lo de `src/core/Camera.cpp`, `src/core/Frontend.cpp`, `src/renderer/Draw.cpp`,
> `src/core/re3.cpp`, `src/core/config.h` y `src/core/Camera.h` es **lectura** o
> *propuesta de contrato* para otro carril.
>
> Fuentes: `gta_vc_browser/tmp/rea/wfp-16-master-rea.md` (tabla de conflictos),
> `gta_vc_browser/tmp/rea/wfp-16-rea.md` (build instalada) y el mapa del mod
> `gta_vc_browser/tmp/extsrc/wfp/WidescreenFixesPack-master/source/GTAVC.WidescreenFix/`
> (`Frontend.ixx`, `Hud.ixx`, `Legacy.ixx`, `Sprite2d.ixx`, `Draw.ixx`, `common.h`)
> + `includes/stdafx.cpp` (las fórmulas del master) y
> `data/GTAVC.WidescreenFix/scripts/GTAVC.WidescreenFix.ini` (el INI que trae el master).

---

## 0 · Hechos medidos que corrigen al plan y al recibo

| # | Hecho | Dónde está medido | Consecuencia |
|---|---|---|---|
| M1 | El master tiene **dos caminos mutuamente excluyentes**: si existe `GTAVC.WidescreenFixLegacy.ini` usa `InitLegacy()` (path `Legacy.ixx`) y **sale**; si no, usa el camino modular (`Frontend.ixx`, `Hud.ixx`, …). | `dllmain.cpp:9-17` | Los 87/74 slots de `Frontend.ixx:1021-1037` y los de `Legacy.ixx:829-845` son **el mismo comportamiento por dos caminos**, no dos features. |
| M2 | El **INI que trae el master** pone `HudWidthScale = 0.8`, `HudHeightScale = 0.8` ("Original value is 1.0"), `RadarWidth = 97.5`, `RadarHeight = 89.0`, `SubtitlesScale = 1.0`, `HudAspectRatioConstraint = Auto`, `MenuAspectRatioConstraint = 16:9`. | `data/GTAVC.WidescreenFix/scripts/GTAVC.WidescreenFix.ini:1-10` | El "0,8" de C1 y el "97,5×89" de C2 son **valores de INI**, no constantes de código. El código sin INI usa 1.0 / 1.0714285 / 94 / 76. |
| M3 | El master escribe el ancho del radar como `RadarWidth * ((640/480)/(640/448))` = `RadarWidth * 0,9333` y el alto tal cual (`RadarHeight`). | `Frontend.ixx:1103-1111` | Con el INI del master: ancho **91,0** (no 97,5) y alto **89,0**, en el espacio 640×448 del retail. El 97,5 es el valor "pre-escala". |
| M4 | `fWidescreenHudOffset` vale **0** si no hay `HudAspectRatioConstraint`: `CalculateWidescreenOffset(W,H,W,H,0,true)` = `((W - H*(W/H))/2)` = 0. | `Draw.ixx:64`, `includes/stdafx.cpp:79-87` | **Los 12 hooks de `Hud.ixx` no mueven nada** con el INI del master (`Auto`). C6 (feature 22) y C18 (feature 18) son **la misma feature partida en dos**. |
| M5 | El texto del HUD se dibuja **en lote** también en el port: `CFont::PrintString` acumula y `CFont::DrawFonts()` → `RenderFontBuffer()` vacía. | `src/renderer/Font.cpp:1942-1946`, `1024` | El "flush antes de cambiar de lado" del master es **portable tal cual**. |
| M6 | `SCREEN_SCALE_X(a)` = `a * (W/640) * (4/3) / SCREEN_ASPECT_RATIO`; `SCREEN_SCALE_Y(a)` = `a * H/480` (con `PROPER_SCALING` y `ms_bProperScaling=true`, `ScaleY` es la identidad). | `src/core/common.h:195-217`, `src/renderer/Draw.cpp:108-113` | El base del port **es exactamente** el base del master (`INV_SCREEN_WIDTH` = `(1/640)*(4/3)/AR`). El multiplicador de usuario va **encima**, una sola vez. |
| M7 | El HUD del port está **anclado a la derecha** (30 usos de `SCREEN_SCALE_FROM_RIGHT`) y el **radar a la izquierda** (`RADAR_LEFT`/`RADAR_BOTTOM` en `Radar.h:172-173`). | `src/renderer/Hud.cpp` (30 `FROM_RIGHT`), `src/core/Radar.h:171-173` | La estructura es la **misma** que la del retail que el master reordena: grupo izquierdo (radar + mensaje de ayuda) y grupo derecho (todo lo demás). El mapeo de C6 es 1:1. |
| M8 | La rama `TheCamera.m_WideScreenOn` del bloque de subtítulos (1526) **está viva**: el bloque de subtítulos (1499) está fuera de `if (m_Wants_To_Draw_Hud && !TheCamera.m_WideScreenOn)` (284-1407). | `src/renderer/Hud.cpp:1419-1551` | En modo teatro (widescreen) **no hay radar en pantalla** (todo el HUD está en 284-1407) → la reserva de radar solo importa en la rama 4:3. |
| M9 | La barra de cinemática del port es **fija al 30 %**: `borderSize = (H/2)*0,30`, barra = `0,15H + SCREEN_SCALE_Y(14)` = exactamente la `vanillaBar` del master. | `src/core/Camera.cpp:2786-2796`, `Frontend.ixx:129-135` | La "subida del subtítulo" (C43) da **0** hoy; solo deja de dar 0 cuando entre C3 (bordes adaptativos). |
| M10 | Final de línea de los 4 ficheros del territorio: **CRLF puro** (Hud.cpp 2536 CRLF / 0 LF; Radar.cpp 2194/0; Radar.h 327/0; common.h 443/0). Sangría con tabuladores (Hud.cpp 6827 tabuladores / 78 líneas con espacio). | medición byte a byte | Editar con script binario en Python, `assert count == 1`, y escribir en **CRLF**. |

### Correcciones al enunciado (medidas, no supuestos)

1. **C1 no es "0,8 fijo"**: 0,8 es el valor del INI del master; el default del código es 1.0 (X) / 1.0714285 (Y). El factor es **una clave por eje**, no un número.
2. **C2 no es "97,5×89"**: el master escribe **91,0 × 89,0** con su INI (M3). Y el "88,5×81" es del path *legacy* (`IVRadarScaling`), no del master modular.
3. **C6 y C18 son la misma cosa** (M4): sin `HudAspectRatioConstraint` no hay desplazamiento.
4. El doble escalado del que avisa el enunciado **no ocurre** si el factor se aplica *encima* de `SCREEN_SCALE_X` (M6): el factor de aspecto aparece **una sola vez** (dentro de `SCREEN_SCALE_X`) y el 0,8 es un multiplicador plano adicional. El doble escalado real ocurre si el factor se **dobla dentro** de `SCREEN_SCALE_X`/`SCREEN_STRETCH_X` (afectaría también al radar, al menú y a los sprites) o si se aplica en la macro **y** en el punto de llamada. La propuesta de abajo lo evita por construcción: la base no se toca y el factor vive en macros **propias del HUD**.

---

### C1 · Multiplicador de escala del HUD (X e Y)

- **Fichero**: `src/core/common.h` (macros `SCREEN_*`) + `src/renderer/Hud.cpp`
- **Bloque**:
  - `src/core/common.h:194-218` (bloque `SCREEN_STRETCH_*` / `SCREEN_SCALE_*`).
  - `src/renderer/Hud.cpp`: 225 apariciones de `SCREEN_SCALE` (88 `SCREEN_SCALE_X`, 98 `SCREEN_SCALE_Y`, 30 `SCREEN_SCALE_FROM_RIGHT`, 9 `SCREEN_SCALE_FROM_BOTTOM`); los grupos de HUD que **sí** deben llevar el factor son ~45 de ellos (dinero 666, icono 745/769/778, munición 805, vida 869/873, armadura 896/900, buscado 954-964/1002/1012, zona 1098/1110, vehículo 1198/1209, reloj 1243, temporizador 1286-1352, mensajes grandes 1704-1978).
  - **Quedan fuera** (el master los excluye: índices 15-17/20/66/67 y 13-15/18/57/58): retícula (378-559), **radar** (1361-1406), texto de script (1416-1498, 1775-1822) y subtítulos (1499-1551, que tienen su propia escala).
- **Qué hace hoy**: no hay multiplicador de usuario. Todo el HUD se dimensiona con `SCREEN_SCALE_X/Y`, que ya lleva el factor de aspecto: `src/core/common.h:201` (`#define SCREEN_SCALE_X(a) SCREEN_SCALE_AR(SCREEN_STRETCH_X(a))`) y `207` (`SCREEN_SCALE_AR(a) = (a) * DEFAULT_ASPECT_RATIO / SCREEN_ASPECT_RATIO`).
- **Cambio propuesto**:
  1. `src/core/common.h`, dentro del bloque `SCREEN_*` (después de la línea 204): un juego de macros **propias del HUD** que componen el factor de usuario **encima** de la base, sin tocar `SCREEN_SCALE_X/Y` (así el factor de aspecto se aplica una sola vez y el radar/menú/sprites no se ven afectados):

```
#ifdef VICEEXT_WFP_HUD_SCALE
extern float g_fWfpHudWidthScale;
extern float g_fWfpHudHeightScale;
void SetWfpHudScale(float fWidthScale, float fHeightScale);
#define HUD_SCALE_X(a)            (SCREEN_SCALE_X(a) * g_fWfpHudWidthScale)
#define HUD_SCALE_Y(a)            (SCREEN_SCALE_Y(a) * g_fWfpHudHeightScale)
#define HUD_SCALE_FROM_RIGHT(a)   (SCREEN_WIDTH - HUD_SCALE_X(a))
#define HUD_SCALE_FROM_BOTTOM(a)  (SCREEN_HEIGHT - HUD_SCALE_Y(a))
#else
#define HUD_SCALE_X(a)            SCREEN_SCALE_X(a)
#define HUD_SCALE_Y(a)            SCREEN_SCALE_Y(a)
#define HUD_SCALE_FROM_RIGHT(a)   SCREEN_SCALE_FROM_RIGHT(a)
#define HUD_SCALE_FROM_BOTTOM(a)  SCREEN_SCALE_FROM_BOTTOM(a)
#endif
```

  2. `src/renderer/Hud.cpp`, cabecera del fichero (después de los `#include`, ~línea 40) y cuerpo de `CHud::Draw()`: definición + setter (los valores por defecto son **1,0 = identidad**, o sea "con la bandera encendida y sin INI el juego se ve igual"):

```
#ifdef VICEEXT_WFP_HUD_SCALE
float g_fWfpHudWidthScale = 1.0f;
float g_fWfpHudHeightScale = 1.0f;

void SetWfpHudScale(float fWidthScale, float fHeightScale)
{
	g_fWfpHudWidthScale = fWidthScale;
	g_fWfpHudHeightScale = fHeightScale;
}
#endif
```

  3. `src/renderer/Hud.cpp`: sustituir `SCREEN_SCALE_X` → `HUD_SCALE_X`, `SCREEN_SCALE_Y` → `HUD_SCALE_Y`, `SCREEN_SCALE_FROM_RIGHT` → `HUD_SCALE_FROM_RIGHT`, `SCREEN_SCALE_FROM_BOTTOM` → `HUD_SCALE_FROM_BOTTOM` **solo** en los ~45 sitios de los grupos de HUD listados arriba. El bloque del radar (1361-1406) y el de la retícula (378-559) **no** se tocan.
  4. Valores: `HudWidthScale` y `HudHeightScale` se leen de `[WidescreenFix]`, se limitan a `[0.25, 4.0]` (como el master: `Frontend.ixx:1002-1003`) y el **0,0 significa "sin cambio"** → 1,0 (el master modular trata 0,0 como "no aplicar": `Frontend.ixx:1000`).
- **Bandera/clave que necesita**: `VICEEXT_WFP_HUD_SCALE` (nueva, al final de su bloque en `src/core/config.h`) + claves `[WidescreenFix] HudWidthScale` y `[WidescreenFix] HudHeightScale` (float; default del código 1,0; el INI del mod trae 0,8 / 0,8).
- **Dependencia**: la capa de INI (T1, `src/core/re3.cpp`) llama a `SetWfpHudScale()`; `src/core/common.h` es visible desde allí (`common.h` incluye `config.h` y `Draw.h`).
- **Riesgo**: (a) si se cambia `SCREEN_SCALE_X/Y` en vez de las macros `HUD_SCALE_*`, se encoge **todo** el juego (menú, radar, sprites) y el disco del radar deja de cuadrar con los blips; (b) el intercambio de ~45 sitios es mecánico pero hay que revisar los 4 bloques excluidos; (c) al ser un factor sobre la *distancia al borde*, el HUD se encoge hacia su esquina (como en el master: el puntero de escala del retail multiplica la distancia al borde).
- **-> verify**: con `HudWidthScale = 0.8` en `[WidescreenFix]`, la X del dinero (`Hud.cpp:666`) pasa de `SCREEN_WIDTH - SCREEN_SCALE_X(110)` a `SCREEN_WIDTH - 0.8*SCREEN_SCALE_X(110)` (se comprueba en captura: el texto se acerca 20 % al borde derecho y ocupa un 20 % menos); con `1.0` no cambia ni un píxel; el disco del radar mantiene su tamaño exacto (no se toca 1370-1372).

---

### C2 · Tamaño del radar (97,5 / 89 del INI → 91,0 × 89 medidos)

- **Fichero**: `src/core/Radar.h`, `src/core/Radar.cpp`
- **Bloque**:
  - `src/core/Radar.h:171-195` (bloque "Values for screen space": `RADAR_LEFT` 172, `RADAR_BOTTOM` 173, y las macros `RADAR_WIDTH`/`RADAR_HEIGHT` en 181-182 con `FIX_RADAR`, o 193-194 sin él).
  - `src/core/Radar.cpp:1569-1583` (`CRadar::TransformRadarPointToScreenSpace`): 1577/1579 usan `SCREEN_SCALE_X(RADAR_WIDTH)`, 1581 usa `SCREEN_SCALE_Y(RADAR_HEIGHT)` y `SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM + RADAR_HEIGHT)`.
  - `src/core/Radar.cpp:886-935` (`DrawRadarMask`) y `937+` (`DrawRadarSection`) derivan **todo** de `TransformRadarPointToScreenSpace`.
  - `src/renderer/Hud.cpp:1361-1406` (el disco del radar): 1370 `CRect(0,0,SCREEN_SCALE_X(RADAR_WIDTH), SCREEN_SCALE_Y(RADAR_HEIGHT))`, 1372 la traslación.
  - `src/core/Radar.cpp:1104-1123` (`CRadar::Initialise`): punto de arranque para el valor por defecto.
- **Qué hace hoy**: el tamaño es una **constante de compilación** dependiente de `CDraw::ms_bFixRadar` (`src/renderer/Draw.cpp:27`, `true` por defecto): **82×82** con `FIX_RADAR` (que está activo, `src/core/config.h:670`), **94×76** sin él. El master escribe en 3+3 slots del retail `RadarWidth * 0,9333` y `RadarHeight` (`Frontend.ixx:1056-1111`); con su INI eso es **91,0 × 89,0**. El path legacy escribe 88,5×81 (`Legacy.ixx:976-993`).
- **Cambio propuesto**: convertir `RADAR_WIDTH`/`RADAR_HEIGHT` en **valores de runtime**, con el valor por defecto = el del port (82×82 / 94×76 según `ms_bFixRadar`) y la posibilidad de fijarlos por INI con los nombres del mod (`RadarWidth`, `RadarHeight`).
  - `src/core/Radar.h:175-195`: anteponer la rama nueva y conservar tal cual las dos existentes:

```
#ifdef VICEEXT_WFP_RADAR_SIZE
extern float g_fWfpRadarWidth;
extern float g_fWfpRadarHeight;
void SetWfpRadarSize(float fWidth, float fHeight);
#define RADAR_WIDTH  (g_fWfpRadarWidth)
#define RADAR_HEIGHT (g_fWfpRadarHeight)
#elif defined(FIX_RADAR)
#define RADAR_WIDTH ((CDraw::ms_bFixRadar)  ? (82.0f) : (94.0f))
#define RADAR_HEIGHT ((CDraw::ms_bFixRadar) ? (82.0f) : (76.0f))
#else
#define RADAR_WIDTH (94.0f)
#define RADAR_HEIGHT (76.0f)
#endif
```

  - `src/core/Radar.cpp`, junto a `CRadar::Initialise()` (1104-1123): valores por defecto + setter:

```
#ifdef VICEEXT_WFP_RADAR_SIZE
float g_fWfpRadarWidth = 94.0f;
float g_fWfpRadarHeight = 76.0f;

void SetWfpRadarSize(float fWidth, float fHeight)
{
	g_fWfpRadarWidth = fWidth;
	g_fWfpRadarHeight = fHeight;
}
#endif
```

  - y dentro de `CRadar::Initialise()`, antes de `m_radarRange = 350.0f;` (1120), el valor por defecto del port:

```
#ifdef VICEEXT_WFP_RADAR_SIZE
	g_fWfpRadarWidth = CDraw::ms_bFixRadar ? 82.0f : 94.0f;
	g_fWfpRadarHeight = CDraw::ms_bFixRadar ? 82.0f : 76.0f;
#endif
```

  - Conversión de unidades (M3): el valor del INI está en el espacio del master (640×480 para el ancho, pre-escalado por 448/480). Si se quiere reproducir **exactamente** el master, el ancho que se escribe es `RadarWidth * 448.0f/480.0f` (97,5 → 91,0) y el alto `RadarHeight` (89,0). Si se quiere el tamaño *visual* del master en el port (que usa base 480 en Y), el alto equivalente es `89,0 * 480/448 = 95,36`. **Decisión del jugador**: (i) "valor del INI tal cual" (simple, pero el ancho queda 8 % más ancho que en el master) o (ii) "idéntico al master" (multiplicar el ancho por 448/480 y el alto por 480/448).
  - **Nota**: `RadarWidthScale` (feature 8 del master, factor 0,9) **no tiene equivalente**: en el port el marcador "you are here" está en el **centro** del radar (`src/core/Radar.cpp:1638-1681`, se dibuja en `x,y` = centro), no a 94 px del borde, así que no hay nada que mover.
- **Bandera/clave que necesita**: `VICEEXT_WFP_RADAR_SIZE` (nueva en `src/core/config.h`) + `[WidescreenFix] RadarWidth` y `[WidescreenFix] RadarHeight` (float; default del código = el del port; el INI del mod trae 97,5 / 89,0).
- **Dependencia**: ninguna interna (Radar.h lo incluye `Hud.cpp:14`). El INI (T1) llama a `SetWfpRadarSize()`. La reserva de radar de los subtítulos (C42) debe leer ya el valor final → se resuelve con la misma variable.
- **Riesgo**: el disco del radar (1370) es una textura **cuadrada** estirada al rect → con 91×89 sale ovalada (ya lo está hoy con 82×82/94×76: es el mismo comportamiento, pero 91×89 acentúa la diferencia). El radar grande puede **tapar el marcador "you are here"** y la esquina inferior izquierda del HUD. El 13.0 ya trae "HD radar" (disco de 256 px), así que el disco grande del mod puede no ser deseable → es un item de CONFLICTO REAL, necesita visto bueno.
- **-> verify**: con `RadarWidth`/`RadarHeight` = 97,5/89,0 el rect del disco (`Hud.cpp:1370`) mide `0,9333*97,5` × `89,0` en espacio 640 y la máscara del radar (`DrawRadarMask`) sigue coincidiendo con el disco (los blips caen dentro del círculo); con los valores por defecto (82×82) la captura es idéntica píxel a píxel a la de hoy.

---

### C6 + C18 · HUD anclado a los bordes (feature 22) y restricción de aspecto del HUD (feature 18)

> **Decisión (medida, M4): son la MISMA feature.** La feature 18 (`HudAspectRatioConstraint`, `Draw.ixx:64-75`) calcula **la magnitud** del desplazamiento (`fWidescreenHudOffset`); la feature 22 (`Hud.ixx`, 12 hooks) decide **a qué lado** se empuja cada grupo de HUD. Con `Auto` (el valor del INI del master) la magnitud es **0** y los 12 hooks no mueven nada. Lo que entra al port es, por tanto, **una sola feature**: "anclar el HUD a un lienzo con aspecto `valor` centrado".
> Lo que **no** entra: los 12 hooks como hooks (el port no parchea direcciones); se porta el **comportamiento** (reordenar por grupos), que es lo que exige `ADR-001`.

- **Fichero**: `src/renderer/Hud.cpp` (+ el valor del aspecto, que vive en el carril de FOV/cámara)
- **Bloque**:
  - Master: `Hud.ixx:33-118` (12 hooks), `Sprite2d.ixx:75-96` (`g_wantsToMoveHudLeft/Right`, `GetHudOffset()`, `ScaleRect()`), `Draw.ixx:26` + `:57-78` (`fWidescreenHudOffset`, `SetAspectRatio`), `includes/stdafx.cpp:79-87` (`CalculateWidescreenOffset`) y `:145-154` (`ClampHudAspectRatio`).
  - Port: `src/renderer/Hud.cpp:271-1407` (`CHud::Draw`), `1361-1406` (radar, grupo izquierdo), `1553-1667` (mensaje de ayuda, grupo izquierdo), `618-1359` (dinero/munición/vida/armadura/buscado/zona/vehículo/reloj/temporizador, grupo derecho), `1668-1985` (mensajes grandes y título de misión, grupo derecho), `378-559` (retícula, centrada), `1416-1498` (texto de script, centrado), `1499-1551` (subtítulos, centrados).
- **Qué hace hoy**: el HUD del port está **anclado a la derecha** con `SCREEN_SCALE_FROM_RIGHT` (30 sitios: 666, 745, 769, 778, 805, 869, 873, 896, 900, 954, 959, 964, 1002, 1012, 1098, 1110, 1198, 1209, 1243, 1286, 1293, 1326, 1331, 1352, 1749, 1950, 1975, 1978…) y el radar a la **izquierda** (`RADAR_LEFT`/`RADAR_BOTTOM`, `Radar.h:172-173`). No hay ningún desplazamiento por aspecto: el HUD queda pegado a los bordes de la pantalla sea cual sea el aspecto.
- **Cambio propuesto** (todo dentro de `src/renderer/Hud.cpp`):
  1. **Magnitud** (equivalente a `Draw.ixx:64-75`): el desplazamiento del HUD es la diferencia entre el ancho real y el ancho del lienzo con aspecto `valor`:

```
static float s_fWfpHudShift = 0.0f;

static void WfpHudUpdateShift(float fConstraint)
{
	if (fConstraint <= 0.0f) {
		s_fWfpHudShift = 0.0f;
		return;
	}
	fConstraint = Min(fConstraint, CDraw::GetAspectRatio());
	fConstraint = Max(fConstraint, 4.0f / 3.0f);
	fConstraint = Min(fConstraint, 32.0f / 9.0f);
	s_fWfpHudShift = (SCREEN_WIDTH - SCREEN_HEIGHT * fConstraint) * 0.5f;
}
```

  2. **Estado por grupo** (equivalente a los 12 hooks): una variable de desplazamiento X y un conmutador de "lado", más el **vaciado del lote de fuentes** en cada cambio (el port tiene el mismo lote diferido que el retail, M5):

```
static float s_fWfpHudOffsetX = 0.0f;

static void WfpHudGroup(int iSide)
{
	s_fWfpHudOffsetX = s_fWfpHudShift * (float)iSide;
	CFont::DrawFonts();
}
```

  con `iSide`: `+1` = grupo **izquierdo** (se empuja a la **derecha**), `-1` = grupo **derecho** (se empuja a la **izquierda**), `0` = grupo centrado (sin desplazamiento). La magnitud del master es `(H*valor - W)/2 ≤ 0` y `GetHudOffset()` devuelve `+offset` para "move right" (desplaza a la izquierda) y `-offset` para "move left" (desplaza a la derecha) → los signos de arriba son los del master.
  3. **Aplicación**: las macros del HUD (las de C1) pasan a sumar el desplazamiento del grupo:

```
#define HUD_SCALE_X(a)            (SCREEN_SCALE_X(a) * g_fWfpHudWidthScale + s_fWfpHudOffsetX)
#define HUD_SCALE_FROM_RIGHT(a)   (SCREEN_WIDTH - HUD_SCALE_X(a))
```

  (el desplazamiento se suma a la X ya calculada; para el grupo derecho, `s_fWfpHudOffsetX` es negativo → el HUD se separa del borde derecho, igual que en el master).
  4. **Llamadas** dentro de `CHud::Draw()`: `WfpHudGroup(-1)` antes del bloque del radar (1361) y del mensaje de ayuda (1553); `WfpHudGroup(+1)` al principio del bloque del HUD (618) y antes de los mensajes grandes / título de misión (1668 / 1930, dentro de `DrawAfterFade`); `WfpHudGroup(0)` antes de la retícula (378), del texto de script (1416) y de los subtítulos (1499).
  5. **Quién calcula el valor**: `HudAspectRatioConstraint` (string: `Auto` | `16:9` | `21:9` | `W:H` | `WxH`) lo parsea la capa de INI (T1) con la misma gramática del master (`includes/stdafx.cpp:89-143`, `ParseWidescreenHudOffset`) y entrega un float de aspecto (0 = Auto = sin desplazamiento). La geometría del lienzo la recalcula el HUD cuando cambia el aspecto (el mismo evento que usa `CDraw::SetAspectRatio`, `Draw.ixx:77`).
- **Bandera/clave que necesita**: `VICEEXT_WFP_HUD_ANCHOR` (nueva en `src/core/config.h`) + clave `[WidescreenFix] HudAspectRatioConstraint` (string; default `Auto` = sin cambio).
- **Dependencia**: (a) el **aspecto real** (`CDraw::GetAspectRatio()` / `SCREEN_WIDTH`/`SCREEN_HEIGHT`) viene del carril de FOV/cámara (`src/renderer/Draw.cpp`); (b) la reordenación de los grupos del HUD **toca las mismas ~45 líneas** que C1 → **C1 y C6 se deben hacer en la misma pasada** (o C6 después de C1, sobre las macros `HUD_SCALE_*` ya puestas); (c) el texto del HUD se dibuja en lote (M5) → los `CFont::DrawFonts()` de corte son necesarios, si no el texto ya encolado se dibujaría con el desplazamiento viejo.
- **Riesgo**: (a) **es el cambio más visible del plan**: mueve el HUD entero de golpe; (b) el grupo derecho y el izquierdo se desplazan en sentidos opuestos → si el desplazamiento es mayor que la separación entre grupos, el radar y el texto se solapan (con 16:9 en 21:9 el desplazamiento es `(W - H*16/9)/2`, que a 2560×1080 son 160 px: el radar (82 px) cabe, pero a 3440×1440 son 240 px); (c) tocar el lote de fuentes en medio del dibujado puede interactuar con la capa de texto del 14.0 (`src/core/SilentPatchOptions.cpp`); (d) el desplazamiento NO debe aplicarse a la retícula ni al texto de script (centrados en su propio lienzo) → si se aplicara, la retícula se descentraría.
- **-> verify**: con `HudAspectRatioConstraint = 16:9` en una pantalla 21:9, el radar (bottom-izquierda) se separa del borde izquierdo exactamente `(W - H*16/9)/2` píxeles y el dinero/las barras (arriba-derecha) se separan del borde derecho la misma distancia; con `Auto` la captura es idéntica a la actual; la retícula sigue en `SCREEN_WIDTH/2` (no se mueve).

---

### C18 + C42 + C43 + C44 · Subtítulos: escala, reserva de radar y posición

- **Fichero**: `src/renderer/Hud.cpp`
- **Bloque**: `src/renderer/Hud.cpp:1499-1551` (bloque `DrawSubtitles`), en concreto:
  - 1526-1533 rama `TheCamera.m_WideScreenOn`: 1530 `SetCentreSize(SCREEN_WIDTH - SCREEN_SCALE_X(60.0f))`, 1531 `SetScale(SCREEN_SCALE_X(0.58f), SCREEN_SCALE_Y(1.2f))`, 1532 `PrintString(SCREEN_WIDTH / 2.f, SCREEN_SCALE_FROM_BOTTOM(80.0f), m_Message)`.
  - 1534-1549 rama 4:3: 1542 `SetScale(SCREEN_SCALE_X(0.58f), SCREEN_SCALE_Y(1.22f))`, 1544 `radarBulge = SCREEN_SCALE_X(140.0f) + SCREEN_SCALE_X(8.0f)`, 1545 `rectWidth`, 1546 `SetCentreSize(rectWidth)`, 1548 `PrintString(rectWidth / 2.0f + radarBulge, SCREEN_SCALE_FROM_BOTTOM(105.f + 2.0f), m_Message)`.
  - Master: `Frontend.ixx:102-138` (namespace `Subtitles`: `RadarReserve` 104, `SetCentreSize` 107-116, `PrintStringSubtitles` 120-138), `Frontend.ixx:1113-1132` (escala de las 3 constantes), `Frontend.ixx:1139` (cálculo de `RadarReserve`), `Frontend.ixx:1144-1145` (hook de posición).
- **Qué hace hoy**: el subtítulo se dibuja a **tamaño fijo** (0,58 en X y 1,2/1,22 en Y, ya escalados por `SCREEN_SCALE_*`). La rama 4:3 **ya reserva el radar** con un `radarBulge` **duro de 140+8 = 148** (línea 1544) y centra el texto en el espacio libre a la derecha del radar (1545-1548). La rama "teatro" (1526) **no** reserva nada, pero ahí no hay radar en pantalla (M8). La posición vertical es fija (`SCREEN_SCALE_FROM_BOTTOM(80)` / `(105+2)`) y **no** sube con la barra de cinemática.
- **Cambio propuesto**:
  1. **C44 — escala por `SubtitlesScale`** (equivalente a `Frontend.ixx:1113-1132` y `Legacy.ixx:888-898`: el master multiplica por el factor las 3 constantes del retail 1,2 / 1,22 / 0,58; en el port son exactamente las 3 de 1531 y 1542):

```
#ifdef VICEEXT_WFP_SUBTITLES
float g_fWfpSubtitlesScale = 1.0f;
#endif
```
```
	CFont::SetScale(SCREEN_SCALE_X(0.58f * g_fWfpSubtitlesScale), SCREEN_SCALE_Y(1.2f * g_fWfpSubtitlesScale));
```
```
	CFont::SetScale(SCREEN_SCALE_X(0.58f * g_fWfpSubtitlesScale), SCREEN_SCALE_Y(1.22f * g_fWfpSubtitlesScale));
```

  2. **C42 — la reserva del radar sigue al tamaño real del radar** (`Frontend.ixx:1139`: `RadarReserve = max(140, 40 + RadarWidth + 6) + 8`); hoy está fija en 148:

```
	float fRadarReserve = Max(140.0f, 40.0f + RADAR_WIDTH + 6.0f) + 8.0f;
	float radarBulge = SCREEN_SCALE_X(fRadarReserve);
```
  Con el radar del port (82) sale 148 → **idéntico a hoy**; con el radar del master (91) sale 151,5 → el subtítulo se aparta un poco más del radar.
  3. **C43 — el subtítulo sube lo que la barra de cinemática se ha encogido** (`Frontend.ixx:119-138`): el master mueve el texto `-(vanillaBar - barraActual)` cuando la barra actual es **menor** que la de 4:3, con `vanillaBar = SCREEN_HEIGHT*0.15 + SCREEN_SCALE_Y(14)`. En el port la barra es fija al 30 % (`src/core/Camera.cpp:2790-2792`), o sea `barraActual = 0,15H + SCREEN_SCALE_Y(14) = vanillaBar` → **el término es 0 hoy** (M9). El código debe existir igualmente para cuando entre C3 (bordes adaptativos):

```
	float fVanillaBar = SCREEN_HEIGHT * 0.15f + SCREEN_SCALE_Y(14.0f);
	float fCurrentBar = (SCREEN_HEIGHT / 2.0f) * (TheCamera.m_ScreenReductionPercentage / 100.0f) + SCREEN_SCALE_Y(14.0f);
	float fSubtitleY = SCREEN_SCALE_FROM_BOTTOM(80.0f);
	if (fCurrentBar < fVanillaBar)
		fSubtitleY += fVanillaBar - fCurrentBar;
```
  (el `SCREEN_SCALE_FROM_BOTTOM(80.0f)` es el de la rama 1526-1533; el de la rama 4:3 es `SCREEN_SCALE_FROM_BOTTOM(105.f + 2.0f)`).
  4. **C18 — el ancho del subtítulo dentro del lienzo del HUD**: el master limita `SetCentreSize` a `hudWidth - 2*SCREEN_SCALE_X(RadarReserve)`, con `hudWidth = SCREEN_WIDTH + 2*min(fWidescreenHudOffset, 0)`. En el port esto solo tiene sentido si C6 está activo (el desplazamiento del HUD): el `rectWidth` de 1545 debe pasar a usar el ancho del lienzo del HUD en vez del ancho de la pantalla.
- **Bandera/clave que necesita**: `VICEEXT_WFP_SUBTITLES` (nueva en `src/core/config.h`) + clave `[WidescreenFix] SubtitlesScale` (float; default del código 1,0 = sin cambio; el INI del mod trae 1,0). El texto de la reserva del radar (40/6/8/140) es interno, **no** necesita clave.
- **Dependencia**: (a) `RADAR_WIDTH` de C2 (si C2 no entra, la reserva sigue valiendo 148 y el cambio es un no-op); (b) `TheCamera.m_ScreenReductionPercentage` (carril de cámara) para C43; cuando entre C3 el cálculo debe usar el **tamaño real** de la barra, no el 30 % → hace falta un accessor del carril de cámara; (c) C18 depende de C6 (el `hudWidth`).
- **Riesgo**: (a) el `SetScale` con factor < 1 reduce el ancho del texto y el `SetCentreSize` se queda como está → el subtítulo se ve más pequeño pero con el mismo ancho de envoltura (el master también los multiplica por separado: `SubtitlesWidth *= f`, `SubtitlesHeight *= f`); (b) tocar `SetCentreSize` sin tocar `PrintString` descentra el texto (el texto se centra sobre `xstart`, no sobre `centreSize`); (c) con `SubtitlesScale` > 1 el texto puede invadir el radar → por eso la reserva y la escala se tocan juntas.
- **-> verify**: con `SubtitlesScale = 0.8` la altura del subtítulo es el 80 % de la actual (se mide comparando la altura del glifo con el texto de referencia) y el texto **no** invade el disco del radar; con `1.0` la captura es idéntica; en una cinemática con borde adaptativo (C3) el subtítulo queda **dentro** de la barra negra (su base está a `80 px` del borde inferior de la pantalla y por encima de la barra).

---

### F53 · Retícula escalada por aspecto (feature 53 del master)

- **Fichero**: `src/renderer/Hud.cpp` (el equivalente **no** requiere tocar `src/core/Camera.cpp`)
- **Bloque**:
  - Master: `Legacy.ixx:721-740` (`FixCrosshair`: escribe `fCrosshairHeightScaleDown` en los slots verticales 13-15 y `fCrosshairPosFactor` en 3 sitios) y `Legacy.ixx:1087-1088` (los valores): `fCrosshairPosFactor = ((0.53 - 0.5) / (AR / (16/9))) + 0.5` y `fCrosshairHeightScaleDown = fWideScreenWidthScaleDown * AR` (= `(1/640)*(4/3)` = 1/480, constante).
  - Port: `src/renderer/Hud.cpp:378-379` (posición de la retícula de 3ª persona) y `428-431`, `455-457`, `464-466`, `489-491`, `502-504`, `510-512`, `519-521` (el rect: `SCREEN_SCALE_X` en X, `SCREEN_SCALE_Y` en Y). `src/core/Camera.cpp:246-250` fija `m_f3rdPersonCHairMultX = 0.53` (0.51 con `VICEEXT_FIRING_AND_AIMING`) y `m_f3rdPersonCHairMultY = 0.4` (0.49). El "rango de armas" de la cruz está en `Hud.cpp:313-343`.
- **Qué hace hoy**: la retícula se coloca en `SCREEN_WIDTH * m_f3rdPersonCHairMultX` (378) y `SCREEN_HEIGHT * m_f3rdPersonCHairMultY` (379) → su separación del centro **crece** con la pantalla (0,03·W). Su **tamaño** ya usa el base del master: X = `SCREEN_SCALE_X` (con el factor de aspecto) e Y = `SCREEN_SCALE_Y` (base 1/480) → **la parte de la escala vertical del master ya está implementada** (M6). Lo que falta es **el factor de posición**.
- **Cambio propuesto** (equivalente del `fCrosshairPosFactor`, aplicado en el lado del HUD, sin tocar `Camera.cpp`):

```
#ifdef VICEEXT_WFP_CROSSHAIR_ASPECT
	float fAspect = CDraw::GetAspectRatio();
	float fPosFactor = ((TheCamera.m_f3rdPersonCHairMultX - 0.5f) / (fAspect / (16.0f / 9.0f))) + 0.5f;
	float f3rdX = SCREEN_WIDTH * fPosFactor;
#else
	float f3rdX = SCREEN_WIDTH * TheCamera.m_f3rdPersonCHairMultX;
#endif
	float f3rdY = SCREEN_HEIGHT * TheCamera.m_f3rdPersonCHairMultY;
```
  Con `AR = 16/9` el factor da 0,53 (**idéntico a hoy**); con 21:9 da 0,52286 (la cruz se acerca un 0,7 % al centro, como en el master); con 4:3 da 0,54 (el master también se separa un 1 % del valor de fábrica).
  La **altura** de la cruz no se toca: el port ya usa el base 1/480 = `fCrosshairHeightScaleDown` del master.
- **Bandera/clave que necesita**: `VICEEXT_WFP_CROSSHAIR_ASPECT` (nueva en `src/core/config.h`); **ninguna clave de INI** (el master no tiene clave para esta feature: `Legacy.ixx` la aplica siempre).
- **Dependencia**: `CDraw::GetAspectRatio()` (carril de FOV/cámara: declarado en `src/renderer/Draw.h:56`, calculado en `src/renderer/Draw.cpp:55-76`); `m_f3rdPersonCHairMultX` es `static` en `CCamera` (`src/core/Camera.h:494`) y **el carril de cámara NO tiene que cambiar nada** — el valor de fábrica sigue siendo la entrada y el HUD aplica la corrección.
- **Riesgo**: (a) con `VICEEXT_FIRING_AND_AIMING` el factor de fábrica es 0,51, no 0,53 → la fórmula del master (que asume 0,53) desplazaría la cruz un 0,4 % hacia el centro de más; se puede usar el valor de fábrica tal cual (como arriba) o fijar 0,53 como constante del master (decisión del jugador); (b) `GetAspectRatio()` devuelve el aspecto **teatral** (×0,9375) cuando `m_WideScreenOn` → la cruz se mueve también en ese modo (en el master la retícula no se dibuja en ese modo: el port tampoco, el HUD está en 284-1407).
- **-> verify**: en 21:9 la X de la cruz es `SCREEN_WIDTH * 0,52286` (se mide con un trace o con la captura) y en 16:9 es exactamente `SCREEN_WIDTH * 0,53` como hoy; el tamaño de la cruz no cambia (0.53 → 0.52286 solo mueve el centro, no el rect).

---

### C8 (+C10) · Escalas del frontend por clase — **CONFLICTO DUPLICADO**: no se propone cambio

- **Fichero**: `src/core/Frontend.cpp` / `src/core/Frontend.h` (**fuera de mi territorio**: solo se localizan las constantes, no se proponen cambios)
- **Bloque**:
  - Master: `Frontend.ixx:667-743` (24 punteros de escala, uno por clase: `eCDarkel`, `eCGarages`, `eCMenuManager`, `eLoadingScreen`, `eCRadar`, `eCHud`, `eCParticle`, `eCSpecialFX`, `ecMusicManager`, `eCSceneEdit`, `eConvertingTexturesScreen`, `eCReplay` → `ResXInvRefs`/`ResYInvRefs` = inversos de resolución, recalculados en `:773-816` y `:791-799`) y `Frontend.ixx:745-771` (las 6 constantes de Brief/Stats/Leyenda).
  - Port: `src/core/Frontend.cpp` tiene **114** apariciones de `SCREEN_SCALE` y **68** de `SCREEN_STRETCH`, más **665** usos de su propia capa `MENU_X()/MENU_Y()` (`Frontend.cpp:183-194`), que se apoya en `SCREEN_SCALE_X/Y` y `SCALE_AND_CENTER_X`.
- **Qué hace hoy**: el port escala el frontend con la **misma intención** que el master (encajar el menú en el ancho), pero con macros en vez de punteros a float: `MENU_X(x) = SCREEN_SCALE_X(x)`, `MENU_Y(y) = SCREEN_SCALE_Y(y)`, `MENU_X_LEFT_ALIGNED(x) = SCALE_AND_CENTER_X(x)`, `MENU_X_RIGHT_ALIGNED(x) = SCALE_AND_CENTER_X(640-x)`. El master, en cambio, sustituye cada puntero de escala por `INV_SCREEN_WIDTH(aspect)`/`1/altura`.
- **Veredicto**: **CONFLICTO DUPLICADO** (como ya dice el recibo, `wfp-16-master-rea.md:506`): la intención es la misma y la implementación es equivalente; aplicar las dos cosas = doble desplazamiento. **Lo que el master sí aporta** son las **6 constantes de Brief/Stats/Leyenda**, que en el port **coinciden** con las constantes del propio port (medido):

| Constante del master | Valor | Equivalente en el port | Valor del port | ¿Coincide? |
|---|---|---|---|---|
| `fBriefXMult` (`Frontend.ixx:750`) | 0,15625 (= 100/640) | `BRIEFS_LINE_X` (`src/core/Frontend.h:97`, usado en `Frontend.cpp:4009-4010` con `MENU_X_LEFT_ALIGNED`) | 100,0f | **sí** |
| `fBriefWrapMult` (`Frontend.ixx:746`) | 0,875 (= 560/640) | el brief no fija ancho de envoltura (usa el del texto) | — | sin equivalente |
| `fStatsXMult` (`Frontend.ixx:758`) | 0,171875 (= 110/640) | `STATS_ROW_LEFT_MARGIN` (`Frontend.h:84`, `Frontend.cpp:4101`) | 110,0f | **sí** |
| `fStatsWrapMult` (`Frontend.ixx:759`) | 0,82343751 (= (640−113)/640) | `STATS_ROW_RIGHT_MARGIN` (`Frontend.h:85`, `Frontend.cpp:4103`) | 113,0f | **sí** |
| `fLegendLeft` (`Frontend.ixx:765`) | 0,25 (= 160/640) | la leyenda del mapa sigue el lienzo del mapa (`FrontEndMenuManager.m_fMapCenterX/m_fMapSize`), no hay constante fija | — | sin equivalente |
| `fLegendRight` (`Frontend.ixx:769`) | 0,171875 (= 110/640) | ídem | — | sin equivalente |

- **Cambio propuesto**: **ninguno**. Se documenta como `CONFLICTO DUPLICADO` + "la corrección de las 6 constantes ya está implícita en las constantes del port, que coinciden con las del master".
- **Bandera/clave que necesita**: ninguna.
- **Dependencia**: si el jugador quiere el menú en lienzo 16:9 (C7/C9/C17), ese trabajo es de otro carril (`src/core/Frontend.cpp`, `MenuScreensCustom.cpp`).
- **Riesgo**: si alguien aplica los 24 punteros del master **además** de las macros del port, el menú se escala dos veces (doble desplazamiento y doble escala).
- **-> verify**: con el plan tal cual (sin tocar `Frontend.cpp`), el menú sigue escalándose con `MENU_X/MENU_Y` y no hay doble escala; la tabla de arriba (6/6 constantes) queda como evidencia en `docs/mods/ATTRIBUTION.md`.

---

## Resumen

| Item | Ficheros tocados (los míos) | Banderas pedidas | Claves de INI pedidas (`[WidescreenFix]`) | Depende de otro carril |
|---|---|---|---|---|
| C1 · escala del HUD (X/Y) | `src/core/common.h` (194-218), `src/renderer/Hud.cpp` (~45 sitios de los grupos de HUD) | `VICEEXT_WFP_HUD_SCALE` | `HudWidthScale`, `HudHeightScale` | T1 (INI) |
| C2 · tamaño del radar | `src/core/Radar.h` (171-195), `src/core/Radar.cpp` (1104-1123) | `VICEEXT_WFP_RADAR_SIZE` | `RadarWidth`, `RadarHeight` | T1 (INI); decisión jugador (unidad 448/480) |
| C6+C18 · HUD anclado / lienzo con aspecto | `src/renderer/Hud.cpp` (271-1407, 1499-1551, 1668-1985) + `src/core/common.h` (macros) | `VICEEXT_WFP_HUD_ANCHOR` | `HudAspectRatioConstraint` | FOV/cámara (aspecto); T1 (INI); **se solapa con C1** |
| C18+42+43+44 · subtítulos | `src/renderer/Hud.cpp` (1499-1551) | `VICEEXT_WFP_SUBTITLES` | `SubtitlesScale` | C2 (RADAR_WIDTH), cámara (barra de cinemática, C3) |
| F53 · retícula por aspecto | `src/renderer/Hud.cpp` (378-379) | `VICEEXT_WFP_CROSSHAIR_ASPECT` | — | FOV/cámara (`GetAspectRatio`) |
| C8 · escalas del frontend | **ninguno** (CONFLICTO DUPLICADO) | — | — | — |

**Ficheros con línea final (CRLF puro, tabuladores)**: `src/core/common.h` (194-218), `src/renderer/Hud.cpp` (271-1407, 1499-1551, 378-379), `src/core/Radar.h` (171-195), `src/core/Radar.cpp` (1104-1123, 1569-1583).

**Banderas nuevas pedidas** (todas al final de su bloque en `src/core/config.h`, y todas **apagadas de serie**): `VICEEXT_WFP_HUD_SCALE`, `VICEEXT_WFP_RADAR_SIZE`, `VICEEXT_WFP_HUD_ANCHOR`, `VICEEXT_WFP_SUBTITLES`, `VICEEXT_WFP_CROSSHAIR_ASPECT`.

**Claves de INI pedidas** (sección `[WidescreenFix]`, nombres del mod, leídas por la capa T1): `HudWidthScale`, `HudHeightScale`, `RadarWidth`, `RadarHeight`, `SubtitlesScale`, `HudAspectRatioConstraint`. Los defaults del **código** deben ser identidad (1,0 / tamaño actual del radar / 1,0 / Auto) para que con la bandera encendida y sin INI el juego se vea igual; los valores del mod (0,8 / 0,8 / 97,5 / 89,0 / 1,0 / Auto) van en el INI, no en el código.

**Cabecera de atribución**: cada bloque portado necesita la cabecera `PORTADO -- WidescreenFixesPack (MIT, …)` que exige el plan; en los fragmentos de arriba se ha omitido para no meter comentarios en el código (se añade al escribir).
