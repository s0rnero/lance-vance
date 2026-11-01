# 16.0 · Enriquecimiento · texto y luces (subagente TEXTO-LUCES)

> Alcance de este subagente: `src/renderer/Font.cpp`, `src/renderer/Font.h`, `src/renderer/Coronas.cpp`,
> `src/renderer/Particle.cpp`, `src/weapons/Weapon.cpp`, `src/renderer/Credits.cpp`.
> **No se implementa nada aquí**: este documento es propuesta + mediciones.
>
> Especificación: **WFP master de GitHub** (D1). Retail de contraste: `gta-vc.exe` 1.0
> (sha256 `acc6a54681b3c5ce996c956b71be9e4c1315c3b66aea37b896afc913fe28e868`, 3 088 896 B), medido
> byte a byte con escáner propio de patrones (comodín `?` = 1 byte) + capstone 5.0.7.

## 0 · Correcciones al recibo (medido, no supuesto)

| Lo que dice el recibo | Lo que hay (medido) |
|---|---|
| «el port tiene `PrintOutlinedString` declarada y sin usar (`Font.h:186`)» | **Cierto pero engañoso**: `src/renderer/Font.h:184-187` es una declaración **muerta** bajo `#ifdef XBOX_SUBTITLES` (esa macro **no está definida en ningún sitio**: `grep XBOX_SUBTITLES src/` → 0 hits fuera de ese bloque; tampoco está en `build/web/CMakeCache.txt`). **No es la función del master**: firma `PrintOutlinedString(float, float, wchar*, float, bool, CRGBA)` (helper de subtítulos de Xbox) frente a la del master `PrintOutlinedString(float, float, uint32, wchar*, wchar*, float)`. El equivalente real del master en el port es **el bloque de sombra dentro de `CFont::PrintString`** (`Font.cpp:909-927`), porque reVC fusionó la función del retail en `PrintString`. |
| «el 14.0 (SilentPatch) ya tiene su propia capa de sombra/outline con opciones en `src/core/SilentPatchOptions.cpp`» | **Falso**: `src/core/SilentPatchOptions.cpp` (62 líneas, **LF**) sólo expone 12 opciones (`MinimalHUD`, `SlidingMissionTitle`, `SlidingOddJob`, `ShowPropertyBlips`, `DontShrinkRadardisc`, `ScaleScriptSprites`, `ShrinkRadardisc`, `BlipBilinear`, `ScriptSpriteBilinear`, `PickupColours`, `SpeechDelayTimer`, `Units`). **Ninguna de sombras ni de contorno**. En `Font.cpp` no hay ningún `#ifdef VICEEXT_SP_*`. Lo único que el 14.0 aporta al texto es que **el desplazamiento de la sombra escala** (`SCREEN_SCALE_X/Y(dropShadowPosition)`, `Font.cpp:916-922`) → `docs/mods/ATTRIBUTION.md:135` lo marca **YA CUBIERTO**. **⇒ No hay dos capas: hay UNA** (la sombra de 1 desplazamiento). Aplicar el contorno **sustituye** esa capa, no se suma. |
| «C12: el factor 2.0 va en `Coronas.cpp:374-394`» | **Medido**: el factor 2.0 **no** va en las flares (`:418-419`, que ya son del 2.0-B/13.0). Va en el literal `6.0f` de la **línea 374** (ver C12). |
| «C55: `RegisterOne` está en `src/renderer/Coronas.cpp`» | **Falso**: `CBrightLights::RegisterOne` está en `src/renderer/SpecialFX.cpp:984-1005`; en `Coronas.cpp` no hay ninguna llamada. |
| «C27: el equivalente está en `src/renderer/Coronas.cpp`» | **Falso**: `CBrightLights::Render` está en `src/renderer/SpecialFX.cpp:1030-1203`. `Coronas.cpp` no participa. |
| «C33: el fundido está en `src/renderer/Credits.cpp:801`» | **Falso**: `Credits.cpp:801` es `#ifdef CUTSCENE_BORDERS_SWITCH` y `:804-805` dibujan las barras de cinemática. El fundido es `DoFade()` en `src/core/main.cpp:280-359` (rect en `:347`). `Hud.cpp:1513` es la sombra del subtítulo. |
| «C26: el port medido no tiene `m_alphaList` → NO APLICA» | **Falso**: `src/rw/VisibilityPlugins.h:23-26` declara `m_alphaList`, `m_alphaEntityList`, `m_alphaBoatAtomicList`, `m_alphaUnderwaterEntityList`; y `src/rw/VisibilityPlugins.cpp:53-58` **ya multiplica la lista por 3** bajo `ASPECT_RATIO_SCALE` con el mismo comentario del master. → **YA ESTABA** (ver C26). |
| «C27: el patrón del master `C6 44 24 ? ? EB ? ? ? ? ? ? ? DF E0` coincide en el retail 1.0» | **Falso (medido)**: **0 coincidencias** en `.text` del retail 1.0 (con la variante de 15 bytes tampoco: el único candidato, VA `0x571CE9`, cae dentro de la instrucción `mov byte ptr [ecx+0x814072], 0` de `CBrightLights::Init`, no es el opcode que busca el patrón). ⇒ en el retail 1.0 la feature del master es **un no-op**; se porta la **intención** (ver C27). |

---

### C11 Contorno de texto de 16 direcciones (master `Frontend.ixx:171-301` + `Legacy.ixx:995-1000`)

- **Fichero**: `src/renderer/Font.cpp` (bloque de sombra) + `src/renderer/Font.h` (declaración muerta que se retira).
- **Bloque**: `src/renderer/Font.cpp:909-927` (dentro de `CFont::PrintString(float,float,uint32,wchar*,wchar*,float)`, que va de `:899` a `:963`; la variante `MORE_LANGUAGES` de `:855-897` está bajo `#if 0`, es código muerto). `src/renderer/Font.h:184-187`.
- **Qué hace hoy**: una sola sombra difusa de 1 desplazamiento, siempre con `dropColor`, y `Details.bIsShadow = false` al final:
  ```cpp
  909: 	float dropShadowPosition = Details.dropShadowPosition;
  910: 	if (dropShadowPosition != 0.0f && (Details.style == FONT_BANK || Details.style == FONT_STANDARD)) {
  911: 		CRGBA color = Details.color;
  912: 		Details.color = Details.dropColor;
  913: 		Details.dropShadowPosition = 0;
  914: 		Details.bIsShadow = true;
  915: 		if (Details.slant != 0.0f) {
  916: 			Details.slantRefX += SCREEN_SCALE_X(dropShadowPosition);
  ...
  922: 			PrintString(SCREEN_SCALE_X(dropShadowPosition) + x, SCREEN_SCALE_Y(dropShadowPosition) + y, Details.anonymous_25, start, end, spwidth);
  ...
  925: 		Details.dropShadowPosition = dropShadowPosition;
  926: 		Details.bIsShadow = false;
  ```
  El resto del texto del motor (p.ej. `Font.cpp:703-709`, el rect de fondo con `dropShadowPosition`) está dentro de un bloque **comentado** (`/*}else if (IsJapaneseFont()) { ... }*/`, `:701-724`) → **no** es un segundo camino de sombra.
- **Cambio propuesto**: **una sola ruta**, dos comportamientos seleccionados por el valor de la opción: cuando el valor es 0 se deja el código actual tal cual; cuando es > 0, **sustituye** la sombra de 1 desplazamiento por el anillo de 16 direcciones del master (`Frontend.ixx:190-233`), con el alfa compensado `1-(1-a)^(1/16)` y boost ×2, la fuerza 0.25 (valor 1) / 0.5 (valor > 1) y el radio `dropShadowPosition * fuerza` escalado con `SCREEN_SCALE_X/Y` (idéntico al master). Se guarda/restaura `bIsShadow` (el master restaura el valor original; el port hoy fuerza `false`).
  ```cpp
  	float dropShadowPosition = Details.dropShadowPosition;
  	if (dropShadowPosition != 0.0f && (Details.style == FONT_BANK || Details.style == FONT_STANDARD)) {
  		CRGBA color = Details.color;
  		bool8 bIsShadow = Details.bIsShadow;
  		Details.dropShadowPosition = 0;
  #ifdef VICEEXT_WFP_TEXT_OUTLINE
  		if (g_nWFPTextOutline > 0) {
  			static const float fDirs16[16][2] = {
  				{ 1.0000f, 0.0000f}, { 0.9239f, 0.3827f}, { 0.7071f, 0.7071f}, { 0.3827f, 0.9239f},
  				{ 0.0000f, 1.0000f}, { -0.3827f, 0.9239f}, { -0.7071f, 0.7071f}, { -0.9239f, 0.3827f},
  				{ -1.0000f, 0.0000f}, { -0.9239f, -0.3827f}, { -0.7071f, -0.7071f}, { -0.3827f, -0.9239f},
  				{ 0.0000f, -1.0000f}, { 0.3827f, -0.9239f}, { 0.7071f, -0.7071f}, { 0.9239f, -0.3827f}
  			};
  			float fTargetAlpha = (float)color.a / 255.0f;
  			float fPerPassAlpha = 1.0f - Pow(1.0f - fTargetAlpha, 1.0f / 16.0f);
  			uint8 perPassAlpha = (uint8)Clamp(fPerPassAlpha * 255.0f * 2.0f, 0.0f, (float)color.a);
  			float fOutlineStrength = g_nWFPTextOutline > 1 ? 0.5f : 0.25f;
  			Details.bIsShadow = true;
  			for (int i = 0; i < 16; i++) {
  				CRGBA outlineColor = Details.dropColor;
  				outlineColor.a = perPassAlpha;
  				Details.color = outlineColor;
  				float ox = SCREEN_SCALE_X(dropShadowPosition * fDirs16[i][0] * fOutlineStrength);
  				float oy = SCREEN_SCALE_Y(dropShadowPosition * fDirs16[i][1] * fOutlineStrength);
  				if (Details.slant != 0.0f) {
  					Details.slantRefX += ox;
  					Details.slantRefY += oy;
  					PrintString(x + ox, y + oy, Details.anonymous_25, start, end, spwidth);
  					Details.slantRefX -= ox;
  					Details.slantRefY -= oy;
  				} else {
  					PrintString(x + ox, y + oy, Details.anonymous_25, start, end, spwidth);
  				}
  			}
  		} else
  #endif
  		{
  			Details.color = Details.dropColor;
  			Details.bIsShadow = true;
  			if (Details.slant != 0.0f) {
  				Details.slantRefX += SCREEN_SCALE_X(dropShadowPosition);
  				Details.slantRefY += SCREEN_SCALE_Y(dropShadowPosition);
  				PrintString(SCREEN_SCALE_X(dropShadowPosition) + x, SCREEN_SCALE_Y(dropShadowPosition) + y, Details.anonymous_25, start, end, spwidth);
  				Details.slantRefX -= SCREEN_SCALE_X(dropShadowPosition);
  				Details.slantRefY -= SCREEN_SCALE_Y(dropShadowPosition);
  			} else {
  				PrintString(SCREEN_SCALE_X(dropShadowPosition) + x, SCREEN_SCALE_Y(dropShadowPosition) + y, Details.anonymous_25, start, end, spwidth);
  			}
  		}
  		Details.color = color;
  		Details.dropShadowPosition = dropShadowPosition;
  		Details.bIsShadow = bIsShadow;
  	}
  ```
  **Código viejo que se retira** (regla de cero duplicación): `src/renderer/Font.h:184-187`, el bloque
  ```cpp
  #ifdef XBOX_SUBTITLES
  	static void PrintOutlinedString(float x, float y, wchar *str, float outlineStrength, bool fromBottom, CRGBA outlineColor);
  #endif
  ```
  es una **declaración sin definición** bajo una macro que no existe en el árbol ⇒ código muerto que además confunde (el recibo lo señalaba como «la función del master»). Se borra; la implementación nueva **no** se llama `PrintOutlinedString` (vive dentro de `CFont::PrintString`), así que no colisiona.
  **Coordination con la capa del 14.0**: no hay dos capas que coordinar (ver §0). El escalado `SCREEN_SCALE_X/Y(dropShadowPosition)` que el 14.0 ya hizo **se conserva tal cual** en la rama sin contorno; en la rama con contorno el desplazamiento del anillo se escala con las mismas macros, igual que el master. Con la opción a 0 el juego queda **byte a byte** como hoy.
- **Bandera/clave que necesita**: `VICEEXT_WFP_TEXT_OUTLINE` (compila el anillo) + valor runtime `g_nWFPTextOutline` (0/1/2) leído de la clave **`ReplaceTextShadowWithOutline`** del master (`Misc.ixx:31`, `Legacy.ixx:346`, sección `[MISC]` del INI del mod) → en el port, sección `[WidescreenFix/MISC]` (el nombre de sección lo fija T1; D3/supuesto 2). **Apagada de serie** (valor 0 = comportamiento actual).
- **Dependencia**: T1 (contrato de INI) debe exponer `g_nWFPTextOutline` de forma visible desde `Font.cpp` (patrón `src/core/SilentPatchOptions.h`, que declara `extern` y va en **LF**). `Font.cpp` necesitará ese `#include`.
- **Riesgo**: (a) 16 pasadas recursivas por texto → 16 × el coste de `PrintString` en el texto del HUD (el master lo acepta; el texto del HUD son pocas cadenas, pero `CFont::PrintString` se llama miles de veces por frame en el mapa/menú ⇒ medir). (b) El radio del anillo es `dropShadowPosition * fuerza`; con el *clamp* del master (ver item 2) `dropShadowPosition` = 1 → radio 0.25 px (valor 1) / 0.5 px (valor 2) en espacio 640×480: **sub-píxel**, el anillo sale como un halo suave, no como un contorno de 1 px. Es lo que hace el master (fuerza 0.25/0.5 sobre una posición clampada a 1) ⇒ el jugador tiene que ver si le vale; si el contorno le queda weak, el valor 2 (fuerza 0.5) es el que se acerca al retail. (c) El `alpha` por pasada se compensa a 255 → el texto puede quedar **más opaco** que antes.
- **-> verify**: con `ReplaceTextShadowWithOutline=0` el texto se ve **idéntico** a hoy (mismo número de pasadas, misma sombra de 1 px, mismo `bIsShadow`); con `=1` el texto del HUD (`CFont::DrawFonts`) sale con halo en las 16 direcciones y **sin** la sombra de 1 px (comprobación: el texto no tiene doble sombra); con `=2` el halo es el doble de ancho.

---

### Sombras de texto más pequeñas (feature 11 del master, `Legacy.ixx:995-1000` + `:818-823`)

- **Fichero**: `src/renderer/Font.cpp`.
- **Bloque**: `src/renderer/Font.cpp:2113-2117` (el setter). **Medido (el enunciado del encargo decía «sólo admite 0/2»: NO)**: el setter toma un `int16` **sin validar** y lo guarda tal cual; los *callers* usan 0, 1, 2 y 3 (`Font.cpp:370,571`; `Hud.cpp` ×30 con 0/2; `main.cpp:774` con 1; `Game.cpp:174` con **3**; `SceneEdit.cpp:275` con 1; `FrontEndControls.cpp:21,41,61,89` con `m_shadowOffset.x`, un float de widget).
- **Qué hace hoy**:
  ```cpp
  2113: void
  2114: CFont::SetDropShadowPosition(int16 pos)
  2115: {
  2116: 	Details.dropShadowPosition = pos;
  2117: }
  ```
- **Cambio propuesto**: clampar la posición a 1 cuando la opción está activa, en el **único** punto donde entra la posición (el setter), que es exactamente lo que hace el master (`Legacy.ixx:818-823`: `if (Pos > 1 && Pos < 0x77) Pos = 1;`; en el port los valores reales son 0..3, así que `pos > 1` es equivalente). El master instala ese hook **dentro** del `if (ReplaceTextShadowWithOutline)` (`Legacy.ixx:995-1000`), o sea que el clamp es parte del contorno ⇒ se implementan las dos cosas en el mismo setter, sin duplicar el punto de clamp:
  ```cpp
  void
  CFont::SetDropShadowPosition(int16 pos)
  {
  #ifdef VICEEXT_WFP_TEXT_OUTLINE
  	if (g_nWFPTextOutline > 0 && pos > 1)
  		pos = 1;
  #endif
  #ifdef VICEEXT_WFP_SMALLER_TEXT_SHADOWS
  	if (g_bWFPSmallerTextShadows && pos > 1)
  		pos = 1;
  #endif
  	Details.dropShadowPosition = pos;
  }
  ```
  Con `VICEEXT_WFP_TEXT_OUTLINE` apagado **y** `VICEEXT_WFP_SMALLER_TEXT_SHADOWS` apagado, el setter queda **idéntico** al actual (cero cambios de comportamiento con las banderas off, regla del plan).
- **Bandera/clave que necesita**: `VICEEXT_WFP_SMALLER_TEXT_SHADOWS` + `g_bWFPSmallerTextShadows` (0/1) para la feature suelta (el master **no** tiene esta clave: la de la build de 2018 era `SmallerTextShadows`, `docs/mods/ATTRIBUTION.md`/recibo 2018 fila 9) ⇒ **clave propia del port**: `SmallerTextShadows` en `[WidescreenFix/MISC]`. El clamp del contorno va con `VICEEXT_WFP_TEXT_OUTLINE` (misma clave `ReplaceTextShadowWithOutline` del master, valor 0/1/2). **Ambas apagadas de serie.**
- **Dependencia**: T1 (mismos `extern` que C11). Coordinar con el carril del HUD: el clamp afecta también a los widgets del menú (`FrontEndControls.cpp:21,41,61,89`) y a `Game.cpp:174` (posición 3 → 1).
- **Riesgo**: con la opción activa, **todas** las sombras del juego pasan a 1 px, incluidas las del menú y las del HUD, que el mod no tocaba (el master sí, porque el hook es global). Con el contorno activo, además, el radio del anillo queda a 1·fuerza (ver C11). Con la opción apagada, nada cambia.
- **-> verify**: con `SmallerTextShadows=1` la sombra del subtítulo (HUD) y la del menú miden 1 px en vez de 2 (comprobable con el zoom del navegador: el desplazamiento del texto respecto a su fondo); con la opción apagada, el desplazamiento sigue siendo 2 px.

---

### C12 Coronas de vehículo más pequeñas (master `Legacy.ixx:970-974`, clave `SmallerVehicleCorona`)

- **Fichero**: `src/renderer/Coronas.cpp`.
- **Bloque**: `src/renderer/Coronas.cpp:371-386` (el bloque de la textura *streek*). **Medido**: el `2.0f` del master va en el **literal `6.0f` de la línea 374**, que es el factor de estirado de la **corona de tipo `TYPE_STREAK`**, y las coronas `TYPE_STREAK` **sólo las registran los vehículos** (`src/vehicles/Automobile.cpp:3112-3141, 3179-3212, 3362-3375` y `src/vehicles/Bike.cpp:1598-1609, 1643-1648`; `grep TYPE_STREAK src/` → 0 hits fuera de esos dos ficheros) ⇒ es exactamente «el factor de estirado de las luces de vehículo».
  **Medición del retail 1.0** (escáner propio + capstone): el patrón del master `D9 C2 DE CB D9 C2 DE CB D9 05` tiene **1 sola coincidencia** en `.text`, en VA **`0x543A60`**; el operando (match+10) es `0x69590C`, cuyo float **vale 6.0f** en el retail (`00 00 C0 40`); el master lo cambia a `2.0f`. El desensamblado de esa zona (`0x543A48-0x543AA5`) es, literalmente, la fórmula del port:
  ```asm
  00543a48  fld dword ptr [edi + 0x81514c]   ; aCoronas[i].someAngle
  00543a4e  fmul dword ptr [0x695908]        ; * 2/pi  (0.63661974)
  00543a54  fsubr dword ptr [0x695884]       ; 1.0 - x
  00543a5a  fstp st(3)
  00543a5c  fld st(2) / fmulp st(3)  (x3)    ; x^6
  00543a68  fld dword ptr [0x69590c]         ; <<< 6.0f  (el master escribe 2.0f)
  00543a6e  fmul st(3)
  00543a70  fadd dword ptr [0x695870]        ; + 0.5
  00543a7e  fmul dword ptr [0x695910]        ; * 0.06
  00543a84  fsubr dword ptr [0x695914]       ; 0.35 - x
  00543a92  fcomp st(3) / 0x695918 (=0.15)   ; Max(...,0.15)
  ```
  ⇒ correspondencia 1:1 con `Coronas.cpp:373-376`.
- **Qué hace hoy**:
  ```cpp
  371: 				if(aCoronas[i].texture == gpCoronaTexture[8]){
  372: 					// what's this?
  373: 					float f = 1.0f - aCoronas[i].someAngle*2.0f/PI;
  374: 					float wscale = 6.0f*sq(sq(sq(f))) + 0.5f;
  375: 					float hscale = 0.35f - (wscale - 0.5f) * 0.06f;
  376: 					hscale = Max(hscale, 0.15f);
  ```
  (`gpCoronaTexture[8]` = `"streek"`, `Coronas.cpp:69-79`; `TYPE_STREAK` = 8, `Coronas.h:62`.)
- **Cambio propuesto**: sustituir el literal por el valor de la opción (6.0f = retail, 2.0f = master):
  ```cpp
  				if(aCoronas[i].texture == gpCoronaTexture[8]){
  					float f = 1.0f - aCoronas[i].someAngle*2.0f/PI;
  #ifdef VICEEXT_WFP_SMALLER_VEHICLE_CORONA
  					float fStretch = g_bWFPSmallerVehicleCorona ? 2.0f : 6.0f;
  #else
  					float fStretch = 6.0f;
  #endif
  					float wscale = fStretch*sq(sq(sq(f))) + 0.5f;
  					float hscale = 0.35f - (wscale - 0.5f) * 0.06f;
  					hscale = Max(hscale, 0.15f);
  ```
  El comentario preexistente `// what's this?` **desaparece** (línea reescrita; el plan no admite comentarios nuevos y los viejos de una línea tocada no se arrastran). **NO** se toca `Coronas.cpp:418-419` (`SCREEN_STRETCH_X(1.0f)` / `SCREEN_STRETCH_Y(1.0f)`): eso es el escalado de flares **del 2.0-B/13.0** (`docs/mods/ATTRIBUTION.md:139`), no el factor del master.
- **Bandera/clave que necesita**: `VICEEXT_WFP_SMALLER_VEHICLE_CORONA` + `g_bWFPSmallerVehicleCorona` (0/1), clave **`SmallerVehicleCorona`** del master (`Legacy.ixx:342`, sección `[MISC]`) → `[WidescreenFix/MISC]`. **Apagada de serie** (default del master = 0).
- **Dependencia**: T1 (extern). Sin solape con el 15.0: el 15.0 (luces/reflejos) no toca `Coronas.cpp:374`; lo que sí hay es `EnableVehicleCoronaFixes` del 14.0, que es **otra cosa** (la *posición* de la corona de ambulan/firetruk, `docs/mods/ATTRIBUTION.md:189`) ⇒ **cero solape**.
- **Riesgo**: con 2.0f el *streak* de los faros es 3× más estrecho: se ve la línea de luz vertical mucho más fina (es el efecto que busca el mod). El `hscale` depende de `wscale`, así que la altura del streak también cambia (sube de `0.35 - (wscale-0.5)*0.06` a un valor mayor ⇒ el streak se hace **más alto**). Con la opción apagada, nada cambia.
- **-> verify**: de noche, un coche con los faros puestos: con la opción a 1 la línea vertical de luz de los faros es claramente más fina y más corta que con la opción a 0; con la opción a 0 la imagen es idéntica a la de hoy.

---

### C55 Sin «cuadrado» en las luces (master `Legacy.ixx:616-638`, clave `NoLightSquare`)

- **Fichero**: **ninguno de mi territorio**. Destino real medido: `src/renderer/SpecialFX.cpp` (definición de `CBrightLights::RegisterOne`, `:984-1005`) + los **11 call-sites** en vehículos (`src/vehicles/Automobile.cpp:3155, 3157, 3227, 3229, 3298, 3300, 3314, 3316` y `src/vehicles/Bike.cpp:1618, 1657, 1711`). `src/renderer/Coronas.cpp` **no** tiene ninguna llamada a `RegisterOne` (`grep RegisterOne src/renderer/Coronas.cpp` → 0).
- **Bloque**: `src/renderer/SpecialFX.cpp:984-1005` (el `RegisterOne`) — el punto único donde se puede filtrar sin tocar 11 sitios.
- **Qué hace hoy**: el «cuadrado» es el cubo 3D que dibuja `CBrightLights::Render` (`SpecialFX.cpp:1030-1203`) delante de los faros; los vehículos lo piden con `RegisterOne(...)` y el tipo de luz (`BRIGHTLIGHT_FRONT_*` / `BRIGHTLIGHT_REAR_*`) distingue luces de coche de las de tráfico (`BRIGHTLIGHT_TRAFFIC_*`, `TrafficLights.cpp:115-262`) y de las sirenas (`BRIGHTLIGHT_SIREN`).
  El 13.0 (Vice Extended 6.04) **ya** desactiva el cubo en los modelos con luces nuevas: `Automobile.cpp:3154` `... && !bNoBrightHeadLights && !mi->bNewLights` (bit `bNoBrightHeadLights`: `src/entities/Entity.h:81`, `Entity.cpp:70,855,912`, puesto en `src/renderer/Renderer.cpp:701,708`).
- **Cambio propuesto** (carril de vehículos/render, **no yo**): el master anula (NOP) **sólo** las llamadas de coches y motos, no las de semáforos ni sirenas. En el port, el filtro con el tipo de luz reproduce exactamente ese alcance y evita duplicar la condición en 11 sitios:
  ```cpp
  void
  CBrightLights::RegisterOne(CVector pos, CVector up, CVector side, CVector front,
  	uint8 type, uint8 red, uint8 green, uint8 blue)
  {
  #ifdef VICEEXT_WFP_NO_LIGHT_SQUARE
  	if (g_bWFPNoLightSquare && type >= BRIGHTLIGHT_FRONT_LONG && type <= BRIGHTLIGHT_REAR_TALL)
  		return;
  #endif
  	if(NumBrightLights >= NUMBRIGHTLIGHTS)
  		return;
  	...
  ```
  (el enum de tipos está en `src/renderer/SpecialFX.h`; `BRIGHTLIGHT_FRONT_LONG`…`BRIGHTLIGHT_REAR_TALL` son consecutivos y son **exactamente** los tipos que usan coches y motos — verificado en los 11 call-sites). Alternativa equivalente si el carril prefiere no tocar `SpecialFX.cpp`: añadir `&& !g_bWFPNoLightSquare` a las 11 condiciones de `Automobile.cpp`/`Bike.cpp` (más diff, misma semántica).
- **Bandera/clave que necesita**: `VICEEXT_WFP_NO_LIGHT_SQUARE` + `g_bWFPNoLightSquare` (0/1), clave **`NoLightSquare`** del master (`Legacy.ixx:343`, `[MISC]`) → `[WidescreenFix/MISC]`. **Apagada de serie** (default del master = 0).
- **Dependencia**: carril de vehículos (`Automobile.cpp`, `Bike.cpp`) o de render (`SpecialFX.cpp`) — **fuera de mi territorio**; sólo coordino. Con T1 (extern del valor).
- **Riesgo**: **solape con el 13.0**: hoy el cubo ya desaparece en los modelos con luces nuevas y en las entidades con `bNoBrightHeadLights`; C55 lo quita **también** en los modelos viejos ⇒ en un coche sin luces nuevas, el frente deja de tener el "cubo" blanco (el halo pasa a ser sólo la corona). El 14.0 (`EnableVehicleCoronaFixes`) **no** solapa: es la *posición* de la corona de ambulan/firetruk (`docs/mods/ATTRIBUTION.md:189`), no el cubo. Con la opción apagada, nada cambia.
- **-> verify**: de noche, delante de un coche con luces nuevas **y** delante de uno con luces viejas: con la opción a 1 ninguno de los dos tiene el cuadrado blanco; los semáforos y las sirenas **siguen** teniéndolo (no se tocan); con la opción a 0 el comportamiento es el de hoy.

---

### C27 Luces brillantes sólo de lejos (master `Draw.ixx:339-342`)

- **Fichero**: **ninguno de mi territorio**. Destino real medido: `src/renderer/SpecialFX.cpp:1089-1100` (el cálculo del alfa de `CBrightLights::Render`). `src/renderer/Coronas.cpp` no participa.
- **Bloque**: `src/renderer/SpecialFX.cpp:1094-1100`, con las constantes de `:970-973` (`BRIGHTLIGHTS_MAX_DIST 60`, `BRIGHTLIGHTS_FADE_DIST 45`, `CARLIGHTS_MAX_DIST 30`, `CARLIGHTS_FADE_DIST 15`).
- **Qué hace hoy**:
  ```cpp
  1094: 		// fade car lights down to 31 as they come near
  1095: 		if(aBrightLights[i].m_type >= BRIGHTLIGHT_FRONT_LONG && aBrightLights[i].m_type <= BRIGHTLIGHT_REAR_TALL){
  1096: 			if(aBrightLights[i].m_camDist < CARLIGHTS_FADE_DIST)
  1097: 				a = 31;
  1098: 			else if(aBrightLights[i].m_camDist < CARLIGHTS_MAX_DIST)
  1099: 				a = 31 + (255-31)*((aBrightLights[i].m_camDist-CARLIGHTS_FADE_DIST)/(CARLIGHTS_MAX_DIST-CARLIGHTS_FADE_DIST));
  1100: 		}
  ```
  **Medición del master**: el parche del master (`Draw.ixx:340-342`) pone a 0 el inmediato de un `mov byte ptr [esp+X], imm8`; el valor que el retail 1.0 usa para el «31» es ese inmediato. **En el retail 1.0 el patrón del master tiene 0 coincidencias** ⇒ allí la feature del master es un no-op; se porta la **intención** declarada por el propio master («CBrightLights::Render, render only when far away») = el cubo de los coches **no** se dibuja cuando la cámara está cerca (el `31` → `0`).
- **Cambio propuesto** (carril de render, **no yo**): el `31` pasa a ser el valor de la opción:
  ```cpp
  		if(aBrightLights[i].m_type >= BRIGHTLIGHT_FRONT_LONG && aBrightLights[i].m_type <= BRIGHTLIGHT_REAR_TALL){
  #ifdef VICEEXT_WFP_BRIGHT_LIGHTS_FAR
  			float fNearAlpha = g_bWFPBrightLightsFar ? 0.0f : 31.0f;
  #else
  			float fNearAlpha = 31.0f;
  #endif
  			if(aBrightLights[i].m_camDist < CARLIGHTS_FADE_DIST)
  				a = fNearAlpha;
  			else if(aBrightLights[i].m_camDist < CARLIGHTS_MAX_DIST)
  				a = fNearAlpha + (255.0f - fNearAlpha)*((aBrightLights[i].m_camDist-CARLIGHTS_FADE_DIST)/(CARLIGHTS_MAX_DIST-CARLIGHTS_FADE_DIST));
  		}
  ```
- **Bandera/clave que necesita**: `VICEEXT_WFP_BRIGHT_LIGHTS_FAR` + `g_bWFPBrightLightsFar` (0/1). **El master NO tiene clave** para esta feature (es fija en `Draw.ixx:339-342`) ⇒ **clave propia del port**, propuesta `BrightLightsOnlyFar` en `[WidescreenFix/MISC]` (T1 la fija). **Apagada de serie** (regla del plan: nada nuevo encendido por defecto).
- **Dependencia**: carril de render (`SpecialFX.cpp`) — fuera de mi territorio. Con T1 (extern).
- **Riesgo**: el efecto es pequeño (el cubo ya era casi invisible de cerca, alfa 31); con la opción a 1 desaparece del todo por debajo de 15 m y sube hasta 255 a 30 m. Los semáforos y las sirenas **no** cambian (su alfa no pasa por el «31»). Con la opción a 0, nada cambia.
- **-> verify**: de noche, acercándose a un coche con los faros encendidos: con la opción a 1 **no** se ve el cuadrado blanco hasta pasar de ~15 m; con la opción a 0 se ve desde mucho más cerca (como hoy).

---

### C13 Escala de **partículas** en anchas (master `Sprite.ixx:34-44`; el 13.0 ya cubre los sprites de script)

- **Fichero**: `src/renderer/Particle.cpp`.
- **Bloque**: `src/renderer/Particle.cpp:2105-2110` (proyección de la partícula) y `src/renderer/Particle.cpp:2263-2269` (proyección de la estela, `SPEED_TRAIL`).
- **Qué hace hoy**:
  ```cpp
  2105: 				CVector coors;
  2106: 				float w;
  2107: 				float h;
  2108:
  2109: 				if ( CSprite::CalcScreenCoors(particle->m_vecPosition, &coors, &w, &h, true) )
  2110: 				{
  ```
  y
  ```cpp
  2268: 								if ( CSprite::CalcScreenCoors(vecPrevPos, &vecScreenPosition, &fTrailLength, &fRotation, true) )
  2269: 								{
  ```
  **Medido**: la parte del master que reescribe `CalcScreenCoors` (`Sprite.ixx:13-32`) **ya está en el port, idéntica**: `src/renderer/Sprite.cpp:26-52`, con `FIX_SPRITES` definido (`src/core/config.h:459`) y `CDraw::ms_bFixSprites = true` (`src/renderer/Draw.cpp:30`):
  ```cpp
  44: #ifdef FIX_SPRITES
  45: 	*outw = CDraw::ms_bFixSprites ? (fovScale * recip * SCREEN_HEIGHT) : (fovScale * SCREEN_SCALE_AR(recip) * SCREEN_WIDTH);
  46: #else
  47: 	*outw = fovScale * SCREEN_SCALE_AR(recip) * SCREEN_WIDTH;
  48: #endif
  49: 	*outh = fovScale * recip * SCREEN_HEIGHT;
  ```
  ⇒ **lo único que falta es la parte de partículas** (`Sprite.ixx:34-44`, `CalcParticleScreenCoors`): corregir **sólo el ancho** con `SCREEN_SCALE_AR` (= `4/3 / AR`, `src/core/common.h:207`), dejando la proyección y el recorte nativos.
- **Cambio propuesto**: en los **dos** puntos donde la partícula pide su tamaño a la proyección (el master engancha los dos, `Sprite.ixx:57-60`):
  ```cpp
  				if ( CSprite::CalcScreenCoors(particle->m_vecPosition, &coors, &w, &h, true) )
  				{
  #ifdef VICEEXT_WFP_PARTICLE_ASPECT
  					w = SCREEN_SCALE_AR(w);
  #endif
  ```
  ```cpp
  								if ( CSprite::CalcScreenCoors(vecPrevPos, &vecScreenPosition, &fTrailLength, &fRotation, true) )
  								{
  #ifdef VICEEXT_WFP_PARTICLE_ASPECT
  									fTrailLength = SCREEN_SCALE_AR(fTrailLength);
  #endif
  ```
  (en el segundo, `outw` es la longitud de la estela y `outh` la rotación: se corrige el ancho —que es la longitud de la estela— y **no** se toca la rotación, igual que el master).
- **Bandera/clave que necesita**: `VICEEXT_WFP_PARTICLE_ASPECT` (compila la corrección) + valor runtime 0/1 (el master **no** tiene clave) ⇒ **clave propia del port**, propuesta `ParticleAspect` en `[WidescreenFix/MISC]` (T1 la fija). **Apagada de serie**.
- **Dependencia**: T1 (extern). **Sin solape con el 13.0**: `ScriptSpritesScaling` está en `src/renderer/Sprite2d.cpp` (sprites de script) y no toca `Particle.cpp`; el 14.0 (`VICEEXT_SP_HEAT_HAZE_SCALE`) toca `Particle.cpp:985-996` (el heat haze), que es **otro** camino (`CParticle::AddParticle`, `Particle.cpp:981`) y que el master **no** engancha ⇒ no se toca.
- **Riesgo**: con `ms_bFixSprites = true` (default del port) el ancho sale de `fovScale*recip*SCREEN_HEIGHT` (cuadrado) y la corrección lo deja en `0.75×` la altura en 16:9 ⇒ **las partículas se ven más estrechas** que en el retail (que es lo que hace el master, pero el master razona sobre `ms_bFixSprites` en su propio árbol). Hay que verlo: humo de coche, humo de goma, lluvia. La estela (`SPEED_TRAIL`) también se acorta un 25 % en 16:9.
- **-> verify**: con la opción a 1, el humo del motor de un coche quieto se ve **más estrecho** que con la opción a 0, con la misma altura; con la opción a 0, la imagen es idéntica a la de hoy (no hay diferencias en humo, lluvia ni sangre).

---

### C29 Humo de impacto de bala (master `Main.ixx:90-97`)

- **Fichero**: `src/weapons/Weapon.cpp`.
- **Bloque**: `CWeapon::DoBulletImpact` en `src/weapons/Weapon.cpp:1681-1684`; el humo en `:1904-1916` (edificio) y `:1939-1947` (vehículo).
- **Qué hace hoy**: el «humo mal puesto» del retail está **ya desactivado** en el port: ese código está bajo `#ifndef FIX_BUGS` y `FIX_BUGS` **está definido** (`src/core/config.h:414`):
  ```cpp
  1904: #ifndef FIX_BUGS
  1905: 					CVector dist = point->point - (*source);
  1906: 					float distMagnitude = dist.Magnitude();
  1907: 					CVector smokePos = point->point - Max(distMagnitude / 10.0f, 0.2f) * dist / distMagnitude;
  1908: #else
  1909: 				    CVector smokePos = point->point;
  1910: #endif // !FIX_BUGS
  ...
  1916: 					CParticle::AddParticle(PARTICLE_BULLETHIT_SMOKE, smokePos, CVector(0.0f, 0.0f, 0.0f));
  ```
  (ídem en `:1939-1947` para el vehículo). Con `FIX_BUGS` activo el humo **sale en el punto de impacto**, que es el comportamiento que el master consigue con su parche.
- **Cambio propuesto**: **NINGUNO**. Clasificación: **NO APLICA (el mecanismo) + YA ESTABA (el comportamiento)**.
  - **NO APLICA**: lo que hace el master (`Main.ixx:90-97`) es reescribir **un byte de opcode**: `push eax` (0x50) → `push ebp` (0x55) en el retail x86. Medido: **las dos instrucciones empujan 4 bytes**, luego el cambio **no** desalinea la pila (la profundidad es idéntica) — lo que cambia es el **valor** que queda en la pila. Es un truco de *layout* de código del exe x86, sin equivalente en un motor C++ compilado a WASM (`ADR-001`: nada de bytes ni de direcciones del retail en el port).
  - **YA ESTABA**: el efecto observable (el humo del impacto de bala en su sitio) ya es el del port thanks a `FIX_BUGS`.
  - El patrón del master para esta feature (`50 ? ? ? ? ? ? ? 6A ? C7 84 24 …`) es **tan genérico que da 30 037 coincidencias** en el retail 1.0 (medido con el mismo escáner): es la técnica de «escaneo y parcheo» que el plan 16.0 prohíbe explícitamente («el port no copia la técnica de escaneo»).
- **Bandera/clave que necesita**: **ninguna**. No se toca `Weapon.cpp`.
- **Dependencia**: la fila de `docs/mods/ATTRIBUTION.md` para la feature 29 del WFP debe quedar **YA ESTABA** con la referencia `src/weapons/Weapon.cpp:1904-1916` y `:1939-1947` (y `src/core/config.h:414`).
- **Riesgo**: ninguno (no se toca código). El riesgo real es *reintroducir* el humo mal puesto si alguien quita el `#ifndef FIX_BUGS`.
- **-> verify**: disparar a una pared de noche: el humo aparece **en el punto de impacto** (no desplazado hacia el tirador); `grep` del código: no hay ningún byte/patrón del retail en `Weapon.cpp`.

---

### C33 `GetScreenRect` en el fundido (master `Main.ixx:33-40` + `:99-101`)

- **Fichero**: `src/renderer/Credits.cpp` (mi parte: **sin cambios**, ver abajo) + `src/core/main.cpp` (el fundido) + `src/core/Camera.cpp` (el rect).
- **Bloque**: `src/core/main.cpp:280-359` (`DoFade`), rect en `:347-348`; `src/core/Camera.cpp:2273-2290` (`CCamera::GetScreenRect`); `src/renderer/Credits.cpp:800-805` (dibujo de las barras de cinemática) y `:817-820` (`CCredits::AreCreditsDone`).
- **Qué hace hoy**: el fundido se pinta con el rect que devuelve la cámara:
  ```cpp
  347: 		TheCamera.GetScreenRect(rect);
  348: 		CSprite2d::DrawRect(rect, fadeColor);
  ```
  y la cámara devuelve **la banda de contenido** (con el borde fijo del 30 %) siempre que haya widescreen:
  ```cpp
  2278: 	if(m_WideScreenOn
  2279: #ifdef CUTSCENE_BORDERS_SWITCH
  2280: 		&& CMenuManager::m_PrefsCutsceneBorders
  2281: #endif
  2282: 		){
  2283: 		float borderSize = (SCREEN_HEIGHT / 2) * (m_ScreenReductionPercentage / 100.f);
  2284: 		rect.top = borderSize - SCREEN_SCALE_Y(22.f);
  2285: 		rect.bottom = SCREEN_HEIGHT - borderSize - SCREEN_SCALE_Y(14.f);
  2286: 	}else{
  2287: 		rect.top = 0.0f;
  2288: 		rect.bottom = SCREEN_HEIGHT;
  2289: 	}
  ```
  El master (`Main.ixx:34-40`) hace justo lo contrario de lo que sugiere el nombre: su `GetScreenRect` devuelve **la pantalla entera** por defecto y **sólo** el rect de contenido de la cinemática cuando `AreCutsceneBordersVisible()` (es decir, cuando las barras están visibles *ahora*: cinemática con letterbox/pillarbox).
  **Medido**: en el port las barras de widescreen se dibujan **siempre** (`src/core/main.cpp:1831-1836`, en `Render2dStuff`) y también en los créditos (`Credits.cpp:804-805`); `m_ScreenReductionPercentage` vale 30 mientras haya widescreen (`Camera.cpp:2781`).
- **Cambio propuesto** (carril de core — `main.cpp`/`Camera.cpp`, **no yo**): el rect del fundido = rect de contenido **sólo cuando las barras están visibles ahora**; en el resto, pantalla completa:
  ```cpp
  		CRect rect;
  #ifdef VICEEXT_WFP_FADE_SCREEN_RECT
  		if(TheCamera.m_WideScreenOn
  #ifdef CUTSCENE_BORDERS_SWITCH
  			&& CMenuManager::m_PrefsCutsceneBorders
  #endif
  			&& (CCutsceneMgr::IsRunning() || !CCredits::AreCreditsDone()))
  			TheCamera.GetScreenRect(rect);
  		else{
  			rect.left = 0.0f;
  			rect.right = SCREEN_WIDTH;
  			rect.top = 0.0f;
  			rect.bottom = SCREEN_HEIGHT;
  		}
  #else
  		TheCamera.GetScreenRect(rect);
  #endif
  		CSprite2d::DrawRect(rect, fadeColor);
  ```
  **Mi parte (`src/renderer/Credits.cpp`): sin cambios.** El port **ya expone** lo que hace falta: `CCredits::AreCreditsDone()` (`Credits.cpp:817-820`) dice si los créditos están en pantalla, y los créditos son uno de los casos en los que las barras están visibles (`Credits.cpp:804-805`). No hace falta tocar `Credits.cpp`; sí hay que **dejar constancia** en `ATTRIBUTION.md` de que el caso de los créditos entra por `AreCreditsDone()`.
- **Bandera/clave que necesita**: `VICEEXT_WFP_FADE_SCREEN_RECT` (0/1). **El master NO tiene clave** ⇒ **clave propia del port**, propuesta `FadeScreenRect` en `[WidescreenFix/MISC]` (T1 la fija). **Apagada de serie**.
- **Dependencia**: carril de core (`src/core/main.cpp`, `src/core/Camera.cpp`) — fuera de mi territorio; y **C3** (bordes de cinemática adaptativos, `src/core/Camera.cpp:2786-2806`): mientras el borde sea fijo al 30 % y se dibuje siempre, el fundido a pantalla completa vs. a banda es **indistinguible** (las barras ya son negras) ⇒ C33 sólo se ve cuando C3 entre (entonces las barras sólo existen en cinemática y el fundido debe respetarlas). Coordinar el orden con C3.
- **Riesgo**: con la opción a 1 y **sin** C3, el fundido de una carga de área en widescreen se saldría de la banda de contenido y **pintaría encima de las barras** (que se dibujan en `Render2dStuff`, antes que `DoFade` → `main.cpp:1831` vs `:2110`): el negro del fundido se comería la barra, pero como la barra es negra también, **no se nota**; el efecto real sólo aparece con barras adaptativas. Sin embargo, con la opción a 1 y el menú activo (`m_bMenuActive`), el fundido también pasa a pantalla completa: es el cambio pretendido.
- **-> verify**: con la opción a 1, al entrar en una cinemática con bordes y fundirse a negro, **las barras siguen negras** y el fundido se ve sólo dentro del contenido; fuera de cinemática el fundido cubre toda la pantalla. Con la opción a 0, el fundido se comporta como hoy.

---

### C26 Disparo de niebla: `m_alphaList` 150 → 600 (master `VisibilityPlugins.ixx:15-16`)

- **Fichero**: `src/rw/VisibilityPlugins.cpp` (**no** es mi territorio; **no hay cambio**).
- **Bloque**: `src/rw/VisibilityPlugins.cpp:42-71` (`CVisibilityPlugins::Initialise`), en concreto `:45` y `:53-58`.
- **Qué hace hoy** (el recibo decía que el port **no** tenía esa lista: **falso**):
  ```cpp
  45: 	m_alphaList.Init(NUMALPHALIST);
  ...
  53: #ifdef ASPECT_RATIO_SCALE
  54: 	// default 150 is not enough for bigger FOVs
  55: 	m_alphaEntityList.Init(NUMALPHAENTITYLIST * 3);
  56: #else
  57: 	m_alphaEntityList.Init(NUMALPHAENTITYLIST);
  58: #endif // ASPECT_RATIO_SCALE
  ```
  Las listas existen: `src/rw/VisibilityPlugins.h:23-26` (`m_alphaList`, `m_alphaBoatAtomicList`, `m_alphaEntityList`, `m_alphaUnderwaterEntityList`; `+ m_alphaBuildingList` bajo `NEW_RENDERER`), y el llenado/render en `VisibilityPlugins.cpp:118-127, 212`.
- **Cambio propuesto**: **NINGUNO**. Clasificación: **YA ESTABA** (y más generoso que el master): con `ASPECT_RATIO_SCALE` definido (`src/core/config.h:442`) y `VICEEXT_VICE_EXTENDED` definido (`config.h:8`), `NUMALPHAENTITYLIST = 2000` (`config.h:52`) ⇒ la lista de entidades alfa se reserva con **6000** entradas, frente a las **600** (`200*3`) que escribe el master. Además el port ya trae **el mismo comentario** del master («default 150 is not enough for bigger FOVs»), o sea que la idea ya está aplicada.
- **Bandera/clave que necesita**: **ninguna**.
- **Dependencia**: sólo la fila de `docs/mods/ATTRIBUTION.md` (estado **YA ESTABA**, referencia `src/rw/VisibilityPlugins.cpp:53-58`).
- **Riesgo**: ninguno. Ojo al **contraste con el recibo**: la tabla de features del master (`wfp-16-master-rea.md` §2.2 fila 26) dice «A PORTAR (o NO APLICA si el port no tiene lista de alpha)» ⇒ hay que **corregir esa fila a YA ESTABA** en el cierre de T0/T13.
- **-> verify**: `grep -n "NUMALPHAENTITYLIST" src/core/config.h src/rw/VisibilityPlugins.cpp` → la reserva es `NUMALPHAENTITYLIST * 3` bajo `ASPECT_RATIO_SCALE`; con el juego en marcha y el FOV ancho, **no** se ven objetos que desaparezcan al borde (el síntoma que el master arreglaba).

---

## Tabla resumen

| # | Item | Fichero(s) a tocar (líneas) | Bandera `VICEEXT_*` | Clave INI (sección) | Estado |
|---|---|---|---|---|---|
| 1 | **C11** contorno de 16 direcciones | `src/renderer/Font.cpp:909-927` (implementación) · `src/renderer/Font.h:184-187` (retirar la declaración muerta) | `VICEEXT_WFP_TEXT_OUTLINE` | `ReplaceTextShadowWithOutline` 0/1/2 (`[WidescreenFix/MISC]`) | A PORTAR (CONFLICTO CON MOD **resuelto**: sólo hay UNA capa de sombra en el port, el contorno la **sustituye**) |
| 2 | Sombras de texto más pequeñas | `src/renderer/Font.cpp:2113-2117` | `VICEEXT_WFP_SMALLER_TEXT_SHADOWS` (+ el clamp del contorno en `VICEEXT_WFP_TEXT_OUTLINE`) | `SmallerTextShadows` 0/1 (`[WidescreenFix/MISC]`, **clave propia**: el master no la tiene) | A PORTAR |
| 3 | **C12** coronas de vehículo más pequeñas | `src/renderer/Coronas.cpp:371-386` (el `6.0f` de `:374`) | `VICEEXT_WFP_SMALLER_VEHICLE_CORONA` | `SmallerVehicleCorona` 0/1 (`[WidescreenFix/MISC]`) | A PORTAR |
| 4 | **C55** sin «cuadrado» en las luces | `src/renderer/SpecialFX.cpp:984-1005` (**no** en `Coronas.cpp`) · alternativa: `src/vehicles/Automobile.cpp:3155,3157,3227,3229,3298,3300,3314,3316` + `src/vehicles/Bike.cpp:1618,1657,1711` | `VICEEXT_WFP_NO_LIGHT_SQUARE` | `NoLightSquare` 0/1 (`[WidescreenFix/MISC]`) | A PORTAR — **otro carril** |
| 5 | **C27** luces brillantes sólo de lejos | `src/renderer/SpecialFX.cpp:1094-1100` (**no** en `Coronas.cpp`) | `VICEEXT_WFP_BRIGHT_LIGHTS_FAR` | `BrightLightsOnlyFar` 0/1 (`[WidescreenFix/MISC]`, **clave propia**) | A PORTAR — **otro carril**; el patrón del master **no casa** en el retail 1.0 (0 coincidencias) |
| 6 | **C13** escala de partículas | `src/renderer/Particle.cpp:2105-2110` y `:2263-2269` | `VICEEXT_WFP_PARTICLE_ASPECT` | `ParticleAspect` 0/1 (`[WidescreenFix/MISC]`, **clave propia**) | A PORTAR (sólo partículas; la parte de `CalcScreenCoors` **ya está** en `src/renderer/Sprite.cpp:26-52`) |
| 7 | **C29** humo de impacto de bala | `src/weapons/Weapon.cpp` → **ningún cambio** | — | — | **NO APLICA** (parche de opcode x86) + **YA ESTABA** (`FIX_BUGS`, `Weapon.cpp:1904-1916` / `:1939-1947`) |
| 8 | **C33** `GetScreenRect` en el fundido | `src/core/main.cpp:347-348` (**no** `Credits.cpp:801`) · `src/renderer/Credits.cpp` **sin cambios** (ya expone `AreCreditsDone()`, `:817-820`) | `VICEEXT_WFP_FADE_SCREEN_RECT` | `FadeScreenRect` 0/1 (`[WidescreenFix/MISC]`, **clave propia**) | A PORTAR — **carril de core**, y **depende de C3** |
| 9 | **C26** disparo de niebla (`m_alphaList`) | `src/rw/VisibilityPlugins.cpp:53-58` → **ningún cambio** | — | — | **YA ESTABA** (la lista existe y ya es ×3: 6000 vs. las 600 del master) |

### Ficheros con estilo de línea medido (para el ejecutor)

`src/renderer/Font.cpp` **CRLF** · `src/renderer/Font.h` **CRLF** · `src/renderer/Coronas.cpp` **CRLF** ·
`src/renderer/Particle.cpp` **CRLF** · `src/weapons/Weapon.cpp` **CRLF** · `src/renderer/Credits.cpp` **CRLF** ·
`src/renderer/SpecialFX.cpp` **CRLF** · `src/renderer/Sprite.cpp` **CRLF** · `src/renderer/Draw.cpp` **CRLF** ·
`src/core/Camera.cpp` **CRLF** · `src/core/main.cpp` **CRLF** · `src/core/config.h` **CRLF** ·
`src/rw/VisibilityPlugins.cpp` **CRLF** · `src/vehicles/Automobile.cpp` **CRLF** · `src/vehicles/Bike.cpp` **CRLF** ·
`src/core/SilentPatchOptions.cpp` / `.h` **LF** (únicos LF de la lista).

### Notas de estilo para el ejecutor

- Código **sin comentarios** y con **tabuladores** (salvo la cabecera de atribución que el plan 16.0 exige
  —`PORTADO -- WidescreenFixesPack (MIT, …)`— sobre cada bloque portado, según `docs/mods/ATTRIBUTION.md`;
  es la excepción planificada al resto de comentarios).
- Los comentarios preexistentes de una línea que se reescribe **desaparecen** (p.ej. `// what's this?` en
  `Coronas.cpp:372`).
- Editar con script binario en Python y `assert count == 1` por sustitución, conservando el estilo del fichero.
- Ningún cambio en `src/core/SilentPatchOptions.{h,cpp}`: es del carril del 14.0 (y no tiene nada de sombra/contorno).
