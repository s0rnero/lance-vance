# 16.0 - Enriquecimiento S1-S4 (SkyGfx 3.0b) - subagente de ENRIQUECIMIENTO

> Subagente de **ENRIQUECIMIENTO**. **No se implementa nada**: este documento es la
> *propuesta escrita* de las tandas S1, S2, S3 y S4 del plan
> `.agents/plans/2026-10-20-16.0-widescreen-skygfx.md` (decision D2: entra TODO,
> incluidas las tandas caras S2 y S3).
> Territorio de lectura y propuesta:
> `src/extras/custompipes.{cpp,h}`, `src/extras/custompipes_{gl,d3d9}.cpp`,
> `src/extras/postfx.{cpp,h}`, `src/extras/screendroplets.{cpp,h}`,
> `src/extras/shaders/**`, `vendor/librw/src/gl/shaders/**` (solo lectura) y
> `vendor-librw-local-changes.patch`.
> **Ningun fichero de `src/`, `docs/` o `.agents/` ha sido tocado.**
>
> Fuentes leidas: plan, `.agents/RULES.md`, `.agents/CODING_STANDARDS.md`,
> `.agents/DESIGN.md`, recibo RE `gta_vc_browser/tmp/rea/skygfx-16-rea.md`
> (§2.1, §2.2, §3.5, §3.6, §6.1, §6.3, §7), el fuente del mod
> `gta_vc_browser/tmp/extsrc/skygfx_vc/skygfx_vc-master/src/**` y
> `.../shaders/{vs,ps}/*.hlsl`, y el INI instalado
> `mods/skygfx/SkyGfx/skygfx.ini`.

---

## 0 - Resumen de la tanda

| # | Item | Fichero principal | Bloque medido | Bandera propuesta |
|---|---|---|---|---|
| S1-1 | `envMapSize` configurable | `src/extras/custompipes.cpp` | 50, 57-91, 172-230 | `VICEEXT_SKY_ENVMAP_SIZE` |
| S1-2 | Conmutador de `worldPipe` (menu) | `src/extras/custompipes.cpp` + `custompipes_gl.cpp` | 378-394 / 190-245 | `VICEEXT_SKY_WORLD_PIPE` |
| S1-3 | `disableColourOverlay` | `src/extras/postfx.cpp` | 436-482 | `VICEEXT_SKY_DISABLE_COLOUR_OVERLAY` |
| S1-4 | `trailsBlur` / `trailsMotionBlur` / `currentBlurAlpha` / `currentBlurOffset` | `src/extras/postfx.cpp` | 20-21, 241-276, 296-345 | `VICEEXT_SKY_TRAILS_PARAMS` |
| S1-5 | `neoWaterDrops` modos 2 y 3 | `src/extras/screendroplets.cpp` | 162-168, 369-398 | `VICEEXT_SKY_WATERDROPS_EXTENDED` |
| S2-1 | `texblendSwitch` (PS2/PC/Mobile) | `vendor/librw/src/gl/gl3matfx.cpp` + shaders | 96-150, 196-218 | `VICEEXT_SKY_MATFX_BLEND` |
| S2-2 | `texgenSwitch` (PS2/PC/Mobile) | `vendor/librw/src/gl/gl3matfx.cpp` | 70-93 | `VICEEXT_SKY_MATFX_TEXGEN` |
| S2-3 | `ps2light` (caso PC) | `vendor/librw/src/gl/shaders/` | `default.vert:16-20`, `matfx_env.vert:23-28` | `VICEEXT_SKY_MATFX_PS2LIGHT` |
| S3-1 | `carPipe=2` (pipe de coche Leeds) | `src/extras/custompipes_gl.cpp` | 76-170 | `VICEEXT_SKY_LEEDS_CAR_PIPE` |
| S3-2 | `worldPipe=2` (pipe de mundo Leeds) | `src/extras/custompipes_gl.cpp` | 190-273 | `VICEEXT_SKY_LEEDS_WORLD_PIPE` |
| S4-1 | `seamFix` | `src/extras/postfx.cpp` (+ `src/core/main.cpp`, **fuera**) | 66-202, 436-569 | `VICEEXT_SKY_SEAM_FIX` |
| S4-2 | `ps2Water` (UVs del agua) | `src/renderer/WaterLevel.cpp` (**fuera**) | 2307-2400 | `VICEEXT_SKY_PS2_WATER` |
| S4-3 | `dualPass` | `src/extras/custompipes_gl.cpp` | 127, 240, 338, 453, 495 | `VICEEXT_SKY_DUAL_PASS` |
| S4-4 | Fix de blend del sifon y del punto | `src/renderer/Hud.cpp` (**fuera**) | 588-593 | `VICEEXT_SKY_BLEND_FIX` |
| C | Cotejo final de shaders | `src/extras/shaders/` | - | - |

**Fin de linea de todos los ficheros que se mencionan** (medido byte a byte, 20/10/2026):

| Fichero | Estilo | Fichero | Estilo |
|---|---|---|---|
| `src/extras/custompipes.cpp` | **CRLF** (549) | `src/extras/shaders/*.vert` / `*.frag` | **CRLF** (salvo `grading.frag` = **LF**) |
| `src/extras/custompipes.h` | **CRLF** (145) | `src/extras/shaders/make_glsl.sh` / `makeinc_glsl.sh` | **CRLF** |
| `src/extras/custompipes_gl.cpp` | **CRLF** (738) | `src/extras/shaders/makeinc_hlsl.sh` | **CRLF** |
| `src/extras/custompipes_d3d9.cpp` | **CRLF** (724) | `vendor/librw/src/gl/shaders/matfx_env.vert` | **CRLF** (31) |
| `src/extras/postfx.cpp` | **CRLF** (606) | `vendor/librw/src/gl/shaders/matfx_env.frag` | **CRLF** (34) |
| `src/extras/postfx.h` | **CRLF** (58) | `vendor/librw/src/gl/shaders/default.vert` | **CRLF** (23) |
| `src/extras/screendroplets.cpp` | **CRLF** (827) | `vendor-librw-local-changes.patch` | **LF** (1119) |
| `src/extras/screendroplets.h` | **CRLF** (78) | este documento | **LF** |

Todo fragmento de codigo de este documento va en **tabuladores**, **sin comentarios**
(regla del repo) y con el estilo de fin de linea del fichero donde se aplica.

**Nota de arquitectura (ADR-001 / ADR-003)**: se porta el *comportamiento* del mod, no
sus parches. Ninguna propuesta toca direcciones x86, ni patrones, ni el retail. El
`.asi` y el `rwd3d9.dll` no entran: el renderizador del port es librw GL3 / WebGL2.

**Desviaciones medidas respecto del recibo** (el recibo manda, pero estas lineas han
cambiado desde que se escribio):

| En el recibo | Medido hoy | Donde |
|---|---|---|
| `src/core/main.cpp:1641` = `// seam fixer` | `src/core/main.cpp:1649` | el fichero ha crecido 8 lineas |
| `custompipes.cpp:57-230` = scaffolding env map | `custompipes.cpp:47-230` | el bloque empieza en los globales (`:47`) |
| `custompipes_gl.cpp:99-136` = pipe neo | `custompipes_gl.cpp:76-136` | el callback empieza en `:76` |

---

# S1 - Opciones de SkyGfx

### S1-1 `envMapSize` configurable

- **Fichero**: `src/extras/custompipes.cpp`, `src/extras/custompipes.h`
- **Bloque**:
  - `src/extras/custompipes.cpp:47-55` (globales del env map):
    ```
    47: rw::TexDictionary *neoTxd;
    49: bool bRenderingEnvMap;
    50: int32 EnvMapSize = 128;
    51: rw::Camera *EnvMapCam;
    52: rw::Texture *EnvMapTex;
    53: rw::Texture *EnvMaskTex;
    ```
  - `src/extras/custompipes.cpp:57-91` (`CreateEnvMapCam`): `:60` crea el raster con
    `rw::Raster::create(EnvMapSize, EnvMapSize, 0, rw::Raster::CAMERATEXTURE)` y `:62`
    el z-buffer con el mismo tamano.
  - `src/extras/custompipes.cpp:172-230` (`EnvMapInit` / `EnvMapShutdown`).
  - `src/extras/custompipes.h:78-83` (declaraciones).
- **Que hace hoy**: el tamano del env map es la **constante de compilacion** `128`
  (`custompipes.cpp:50`). No hay ninguna clave de INI, ni mando de menu, ni
  `ReadIniIfExists` para el. El mod lo hace configurable
  (`main.cpp:896-899`: lee `envMapSize` y lo **redondea hacia arriba a potencia de 2**:
  `int n = 1; while(n < config.envMapSize) n *= 2; config.envMapSize = n;`).
- **Cambio propuesto**:
  1. `src/extras/custompipes.h`, tras la linea 79 (`extern int32 EnvMapSize;`),
     anadir la funcion que hace el redondeo (la logica del mod, no el parcheo):
     ```c
     extern int32 EnvMapSize;
     void SetEnvMapSize(int32 size);
     ```
  2. `src/extras/custompipes.cpp`, tras la linea 50:
     ```c
     int32 EnvMapSize = 128;

     void
     SetEnvMapSize(int32 size)
     {
     	int n = 1;
     	while(n < size)
     		n *= 2;
     	if(n < 32)
     		n = 32;
     	if(n > 1024)
     		n = 1024;
     	EnvMapSize = n;
     }
     ```
  3. `src/extras/custompipes.cpp:60` y `:62` **no se tocan**: ya leen `EnvMapSize`.
  4. El valor tiene que estar puesto **antes** de `CustomPipeInit()` (que es quien
     llama a `EnvMapInit()`, `custompipes.cpp:484`). Orden medido del arranque:
     `src/skel/glfw/glfw.cpp:586` -> `CMenuManager::LoadSettings()` ->
     `src/core/Frontend.cpp:3806` -> `LoadINISettings()` (`src/core/re3.cpp:645`) y
     **despues** `src/core/main.cpp:2208` -> `CGame::Initialise` ->
     `src/core/Game.cpp:310` -> `CustomPipes::CustomPipeInit()`. El INI se lee, por
     tanto, **antes** de crear el raster: no hace falta reordenar nada.
  5. `src/core/re3.cpp` (**territorio de otro subagente**): dentro del bloque
     `#ifdef EXTENDED_PIPELINES` de `LoadINISettings()` (around de `re3.cpp:772-778`),
     leer la clave y llamar al setter:
     ```c
     	{
     		int envMapSize = 128;
     		ReadIniIfExists("CustomPipesValues", "envMapSize", &envMapSize);
     		CustomPipes::SetEnvMapSize(envMapSize);
     	}
     ```
     y el mismo bloque en la funcion de guardado (`re3.cpp:928-934`):
     ```c
     	StoreIni("CustomPipesValues", "envMapSize", CustomPipes::EnvMapSize);
     ```
- **Bandera/clave que necesita**: `VICEEXT_SKY_ENVMAP_SIZE` (en `src/core/config.h`,
  al final del bloque de banderas) + clave de INI `CustomPipesValues/envMapSize`
  (mismo nombre que el mod, que lo tenia en `[SkyGfx]`).
- **Dependencia**: ninguna. Es el item mas suelto de S1.
- **Riesgo**:
  - El env map se renderiza **una vez por frame** (`src/core/main.cpp:2061`
    -> `CustomPipes::EnvMapRender()`), asi que subir el tamano a 512/1024 cuadruplica
    o multiplica por 8 el coste de esa pasada en el navegador. Riesgo de rendimiento
    real, no de correctitud.
  - `EnvMapCam` usa `setViewWindow({2.0f, 2.0f})` (`:73-74`), independiente del tamano:
    cambiar el tamano cambia la resolucion del reflejo, no su encuadre.
  - Con la bandera apagada el valor sigue siendo 128 y el raster igual: **cero cambio
    de aspecto**.
- **-> verify**: con `CustomPipesValues/envMapSize=256` en el INI, el log del jugador
  muestra el nuevo tamano y el reflejo del coche sale mas nitido; con la clave
  ausente el tamano sigue siendo 128 y el juego se ve igual que antes.

---

### S1-2 Conmutador de `worldPipe` (el menu que no existe)

- **Fichero**: `src/extras/custompipes.cpp`, `src/extras/custompipes.h`,
  `src/extras/custompipes_gl.cpp`
- **Bloque**:
  - `src/extras/custompipes.cpp:374-394`:
    ```
    378: bool LightmapEnable;
    379: float LightmapMult = 1.0f;
    380: InterpolatedFloat WorldLightmapBlend(1.0f);
    381: rw::ObjPipeline *worldPipe;
    ```
  - `src/extras/custompipes.h:102-109` (`extern bool LightmapEnable;` + las pipes).
  - `src/extras/custompipes_gl.cpp:190-199` (la puerta actual):
    ```
    190: static void
    191: worldRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
    ...
    196: 	if(!LightmapEnable){
    197: 		gl3::defaultRenderCB(atomic, header);
    198: 		return;
    199: 	}
    ```
  - El enganche: `src/modelinfo/SimpleModelInfo.cpp:90-95` (todo modelo simple pasa
    por `AttachWorldPipe`), con `m_wetRoadReflection` -> pipe de gloss y el resto ->
    pipe de mundo.
  - El menu: `src/core/re3.cpp:147` y `:153`
    (`FrontendOptionAddSelect("FED_WLM", ... (int8*)&CustomPipes::LightmapEnable ... "NeoLightMaps")`).
- **Que hace hoy**: el pipe de mundo del port es **uno solo** (el "neo"), y lo que se
  puede apagar es el blending con el lightmap mediante el booleano `LightmapEnable`
  (que por defecto vale `false`, o sea: el look normal). El menu ofrece el interruptor
  on/off "NeoLightMaps", no los tres valores `0 = default, 1 = neo, 2 = leeds` del mod
  (`skygfx.ini:20`, `main.cpp:894`). El mod decide en el callback:
  `pipeswitch.cpp:170-178` (`switch(config.worldPipeSwitch)`).
- **Cambio propuesto**: sustituir el booleano `LightmapEnable` por un conmutador de
  tres valores con la misma semantica del mod (**evita dos interruptores que se
  solapan**, regla de duplicacion del repo).
  1. `src/extras/custompipes.h`, en el bloque de las pipes (tras la linea 88):
     ```c
     enum {
     	WORLDPIPE_DEFAULT,
     	WORLDPIPE_NEO,
     	WORLDPIPE_LEEDS
     };
     extern int32 WorldPipeSwitch;
     ```
  2. `src/extras/custompipes.cpp:378`: sustituir `bool LightmapEnable;` por
     ```c
     int32 WorldPipeSwitch = WORLDPIPE_DEFAULT;
     ```
     (default = `WORLDPIPE_DEFAULT` = el look actual con `LightmapEnable == false`:
     **aspecto identico al de hoy**).
  3. `src/extras/custompipes.h:102`: quitar `extern bool LightmapEnable;` y poner
     `extern int32 WorldPipeSwitch;`.
  4. `src/extras/custompipes_gl.cpp:196-199` (la puerta), que pasa a ser:
     ```c
     	switch(WorldPipeSwitch){
     	case WORLDPIPE_NEO:
     		break;
     	case WORLDPIPE_LEEDS:
     		leedsWorldRenderCB(atomic, header);
     		return;
     	default:
     		gl3::defaultRenderCB(atomic, header);
     		return;
     	}
     ```
  5. `src/core/re3.cpp` (**otro subagente**): el mando del menu pasa a ser un
     `FrontendOptionAddSelect` de 3 valores con los nombres del mod
     (`"FED_MFX"`, `"FED_NEO"`, `"FED_LEEDS"`) apuntando a
     `(int8*)&CustomPipes::WorldPipeSwitch`, con la etiqueta `"World pipe"`; el
     `DebugMenuAddVar` de `re3.cpp:1405` pasa a `WorldPipeSwitch` con el mismo
     rango. El item "NeoLightMaps" del menu **desaparece** (lo sustituye "World pipe").
  6. La rama `WORLDPIPE_LEEDS` llama a `leedsWorldRenderCB`, que se define en S3-2.
     Mientras S3-2 no este, la rama debe caer al `default` (o el item se implementa
     junto con S3-2; **no** dejar un `case` vacio que no compile).
- **Bandera/clave que necesita**: `VICEEXT_SKY_WORLD_PIPE` + clave de INI
  `CustomPipesValues/worldPipe` (0 = default, 1 = neo, 2 = leeds) + mando de menu
  "World pipe".
- **Dependencia**: **S3-2** (el valor 2 necesita el pipe de mundo Leeds). Si S3-2 se
  retrasa, S1-2 se puede cerrar con los dos valores (0 y 1) y el tercero se anade
  despues.
- **Riesgo**:
  - **Cambia un mando ya portado** (el "NeoLightMaps" del menu de graficos): es un
    conflicto de menu con la tanda **T1/W2** del plan (que tambien toca el menu de
    graficos en `src/core/MenuScreensCustom.cpp` y `re3.cpp`). Hay que avisar al
    jugador antes de aplicar (regla de la tabla de conflictos).
  - `LightmapEnable` tiene dos usos fuera de `src/extras/` (`re3.cpp:147/153/1405`).
    Si se retira, esos tres puntos se quedan sin destino: es un cambio de tres lineas
    en un fichero de otro subagente.
  - El pipe de mundo **toca toda la geometria del mundo** (`SimpleModelInfo.cpp:94`):
    un fallo en la rama Leeds afecta a cada frame, no a un objeto.
- **-> verify**: con `CustomPipesValues/worldPipe=0` el mundo se ve como el retail;
  con `=1` se ve como el pipe neo actual (identico a hoy con "NeoLightMaps" on); con
  `=2` se ve el pipe Leeds (tweak de prelight). El log del jugador distingue los tres
  con una linea de traza propia.

---

### S1-3 `disableColourOverlay`

- **Fichero**: `src/extras/postfx.cpp`, `src/extras/postfx.h`
- **Bloque**:
  - `src/extras/postfx.h:8-13` (el enum `POSTFX_OFF / SIMPLE / NORMAL / MOBILE`),
    `:17-20` (`EffectSwitch`, `BlurOn`, `MotionBlurOn`, `Intensity`).
  - `src/extras/postfx.cpp:19` (`int CPostFX::EffectSwitch = POSTFX_NORMAL;`),
    `:436-482` (`CPostFX::Render`, el `switch(EffectSwitch)` de `:466-482`),
    `:366-384` (`NeedBackBuffer`), `:386-409` (`NeedFrontBuffer`).
- **Que hace hoy**: el port **ya tiene** el conmutador equivalente: `EffectSwitch`
  con `POSTFX_OFF` = "no dibujar el overlay de color" (`:467-470`, rama que no
  dibuja nada) y `POSTFX_SIMPLE` = "sin overlay, con el blur de 5 iteraciones"
  (`colourfilterVC.frag`). El mod, en cambio, tiene una bandera *aparte*
  (`disableColourOverlay`, `main.cpp:822-823` y `:537-567`) que **anula el overlay
  de color pero deja el resto de las trails**; el propio INI avisa: *"Disable the
  colour overlay effect when trails are enabled; Trails on/off still switches
  between light values then"* (`skygfx.ini:18`).
  En el mod el corte esta en `CMBlur__MotionBlurRenderVC` (`main.cpp:555-567`): con
  `disableColourOverlay` hace `return` antes de dibujar, asi que **no** se dibuja
  ni el overlay ni el motion blur, pero el valor de luz de las trails sigue
  cambiando.
- **Cambio propuesto**: anadir un conmutor propio que apague **solo** el overlay de
  color, dejando el resto del post-proceso igual, y que no se confunda con
  `EffectSwitch` (que ademas controla el back buffer).
  1. `src/extras/postfx.h`, tras la linea 20 (`static float Intensity;`):
     ```c
     	static bool m_bDisableColourOverlay;
     	static bool ColourOverlayEnabled(void);
     ```
  2. `src/extras/postfx.cpp`, tras la linea 51 (`float CPostFX::m_crOffset = 0.0f;`):
     ```c
     bool CPostFX::m_bDisableColourOverlay = false;

     bool
     CPostFX::ColourOverlayEnabled(void)
     {
     	if(m_bDisableColourOverlay)
     		return false;
     	return EffectSwitch != POSTFX_OFF && EffectSwitch != POSTFX_SIMPLE;
     }
     ```
  3. `src/extras/postfx.cpp:466-482`: la rama del overlay pasa a ser
     ```c
     	}else if(ColourOverlayEnabled()){
     		switch(EffectSwitch){
     		case POSTFX_NORMAL:
     			if(MotionBlurOn){
     				if(!bJustInitialised)
     					RenderOverlayBlur(cam, red, green, blue, blur);
     			}else{
     				RenderOverlayShader(cam, red, green, blue, blur);
     			}
     			break;
     		case POSTFX_MOBILE:
     			RenderOverlayShader(cam, red, green, blue, blur);
     			break;
     		}
     	}
     ```
  4. `src/extras/postfx.cpp:366-384` (`NeedBackBuffer`): con el overlay apagado no
     hace falta copiar la pantalla:
     ```c
     bool
     CPostFX::NeedBackBuffer(void)
     {
     	if(!ColourOverlayEnabled())
     		return false;
     	...
     ```
  5. `src/core/re3.cpp` (**otro subagente**): la clave de INI y el mando de menu
     (`ReadIniIfExists("CustomPipesValues", "disableColourOverlay", &CPostFX::m_bDisableColourOverlay);`
     + `StoreIni` + `DebugMenuAddVarBool8`).
- **Bandera/clave que necesita**: `VICEEXT_SKY_DISABLE_COLOUR_OVERLAY` + clave de INI
  `CustomPipesValues/disableColourOverlay` (mismo nombre que el mod).
- **Dependencia**: ninguna.
- **Riesgo**: bajo. Con la bandera apagada, `ColourOverlayEnabled()` devuelve lo
  mismo que la rama actual, asi que el render es identico. Ojo: `EffectSwitch`
  **sigue** mandando (si vale `POSTFX_OFF`, el overlay ya estaba apagado): la bandera
  nueva solo puede **apagar**, nunca encender.
- **-> verify**: con `disableColourOverlay=1` y `EffectSwitch = POSTFX_NORMAL`, la
  pantalla ya no se tiñe con el color de las trails pero el resto del juego (el
  motion blur por ebriedad, el YCbCr) sigue igual; con la clave ausente el
  resultado es identico al de hoy.

---

### S1-4 `trailsBlur` / `trailsMotionBlur` / `currentBlurAlpha` / `currentBlurOffset`

- **Fichero**: `src/extras/postfx.cpp`, `src/extras/postfx.h`
- **Bloque**:
  - `src/extras/postfx.h:17-20`:
    ```
    17: 	static int EffectSwitch;
    18: 	static bool BlurOn;	// or use CMblur for that?
    19: 	static bool MotionBlurOn;	// or use CMblur for that?
    20: 	static float Intensity;
    ```
  - `src/extras/postfx.cpp:19-21`:
    ```
    19: int CPostFX::EffectSwitch = POSTFX_NORMAL;
    20: bool CPostFX::BlurOn = false;
    21: bool CPostFX::MotionBlurOn = false;
    ```
  - `src/extras/postfx.cpp:241-276` (`RenderOverlayBlur`): el alfa **30** esta
    **duplicado** en 8VertexSetIntRGBA (`:247-254`) y el desplazamiento de 2 px del
    quad esta **fijos** en `Open()` (`:128`, `:137`, `:146`, `:155`).
  - `src/extras/postfx.cpp:296-345` (`RenderOverlayShader`): `blurcolors[3] = 30/255.0f;`
    (`:327`).
- **Que hace hoy**: `BlurOn` y `MotionBlurOn` **existen** pero solo se tocan desde el
  menu de depuracion (`re3.cpp:1380-1381`); no son parametros con nombre ni tienen
  clave de INI. El alfa del blur (30) y el desplazamiento del quad (2 px) estan
  **fijos en el codigo**. El mod los expone como parametros exportados
  (`skygfx.h:217-233`, `main.cpp:775-778`):
  `trailsBlur = 1; trailsMotionBlur = 1; currentBlurAlpha = 39.0f; currentBlurOffset = 2.1f;`
  y los usa en `postfx.cpp:129` (`blurintensity = config.currentBlurAlpha*0.8f;` ->
  **31.2**, que es el 30 del port) y `:135-146` (el desplazamiento de 2.1 px).
  Los valores del port y los del mod **coinciden** (30 ~ 39*0.8; 2.0 ~ 2.1): no hay
  cambio de aspecto, solo falta nombrarlos.
- **Cambio propuesto**:
  1. `src/extras/postfx.h`, tras la linea 20:
     ```c
     	static bool BlurOn;
     	static bool MotionBlurOn;
     	static float Intensity;
     	static float m_fBlurAlpha;
     	static float m_fBlurOffset;
     ```
  2. `src/extras/postfx.cpp:20-21` y tras la linea 294
     (`float CPostFX::Intensity = 1.0f;`):
     ```c
     bool CPostFX::BlurOn = false;
     bool CPostFX::MotionBlurOn = false;
     ```
     ```c
     float CPostFX::m_fBlurAlpha = 30.0f;
     float CPostFX::m_fBlurOffset = 2.0f;
     ```
     (los defaults son **los valores que ya usa el port**: 30 y 2.0; con ellos el
     render no cambia ni un pixel).
  3. `src/extras/postfx.cpp:247-254` (las 8 lineas del alfa): sustituir el `30` por
     `(int32)m_fBlurAlpha`.
  4. `src/extras/postfx.cpp:128-162` (el quad `Vertex2`): los `2.0f` se sustituyen
     por `m_fBlurOffset`:
     ```c
     	RwIm2DVertexSetScreenX(&Vertex2[0], zero + m_fBlurOffset);
     	RwIm2DVertexSetScreenY(&Vertex2[0], zero + m_fBlurOffset);
     ```
     ```c
     	RwIm2DVertexSetScreenX(&Vertex2[1], m_fBlurOffset);
     	RwIm2DVertexSetScreenY(&Vertex2[1], ymax + m_fBlurOffset);
     ```
     ```c
     	RwIm2DVertexSetScreenX(&Vertex2[2], xmax + m_fBlurOffset);
     	RwIm2DVertexSetScreenY(&Vertex2[2], ymax + m_fBlurOffset);
     ```
     ```c
     	RwIm2DVertexSetScreenX(&Vertex2[3], xmax + m_fBlurOffset);
     	RwIm2DVertexSetScreenY(&Vertex2[3], zero + m_fBlurOffset);
     ```
  5. `src/extras/postfx.cpp:327`: `blurcolors[3] = m_fBlurAlpha/255.0f;`
  6. `src/core/re3.cpp` (**otro subagente**): cuatro claves de INI
     (`CustomPipesValues/trailsBlur`, `/trailsMotionBlur`, `/currentBlurAlpha`,
     `/currentBlurOffset`), sus `StoreIni` y cuatro mandos de menu de depuracion.
- **Bandera/clave que necesita**: `VICEEXT_SKY_TRAILS_PARAMS` + las 4 claves de INI
  con el nombre del mod.
- **Dependencia**: ninguna.
- **Riesgo**: muy bajo. Los defaults son los valores que el codigo ya usa, asi que
  con las claves ausentes el render es byte a byte igual. `m_fBlurOffset` se aplica
  al quad `Vertex2`, que **solo** se usa cuando `BlurOn` (`:259` y `:275`): con
  `BlurOn` apagado no se ve ningun cambio.
- **-> verify**: con `currentBlurAlpha=60` el overlay de las trails se ve mas fuerte;
  con `currentBlurOffset=6` el quad del blur se desplaza 6 px. Con las claves ausentes
  la captura es identica a la de antes del cambio.

---

### S1-5 `neoWaterDrops` modos 2 y 3 (Extended / Extended+Blood)

- **Fichero**: `src/extras/screendroplets.cpp`, `src/extras/screendroplets.h`
- **Bloque**:
  - `src/extras/screendroplets.cpp:33-34` (`ms_enabled`, `ms_movingEnabled`),
    `:161-168` (`ScreenDroplets::Process`), `:368-388` (`FillScreenMoving`, que ya
    tiene el parametro `bool isBlood`), `:390-398` (`RegisterSplash`).
  - `src/extras/screendroplets.h:33-34` (los dos bool), `:64-68` (`NewDrop`,
    `SetMoving`, `FillScreen`, `FillScreenMoving`, `RegisterSplash`).
  - El punto de enganche que ya existe en el port:
    `src/objects/ParticleObject.cpp:258-260` -> `ScreenDroplets::RegisterSplash(pobj);`
    (el equivalente al `splashhook` del mod, `neoWaterdrops.cpp:213-224`).
  - El sangrado ya existente: `src/weapons/Weapon.cpp:769-773` ->
    `ScreenDroplets::FillScreenMoving(1.0f, true);` (**fuera** de mi territorio).
- **Que hace hoy**: el port tiene el **modo 1** (Neo): gotas fijas + gotas en
  movimiento por lluvia, hidrantes y salpicaduras. `FillScreenMoving(amount, isBlood)`
  ya sabe pintar gotas **rojas** (`screendroplets.cpp:381-382`), y `Weapon.cpp:771`
  ya las usa cuando el jugador dispara a un ped. El mod anade dos modos mas:
  - **modo 2 (Extended)**: en cada particula creada, si esta a menos de
    `getParticleDistance(tipo)` de la camara, se echan gotas en la pantalla
    (`neoWaterdrops.cpp:338-359`); la sangre se excluye.
  - **modo 3 (Extended + Blood)**: igual, pero la sangre tambien genera gotas
    (rojas) (`neoWaterdrops.cpp:352-356`).
  El punto de enganche del mod es un hook sobre `AddParticle`
  (`neoWaterdrops.cpp:381-392`).
- **Cambio propuesto**: la **logica** va en `screendroplets.cpp` (mi territorio); el
  **gancho** necesita **una linea** en el sistema de particulas (fuera).
  1. `src/extras/screendroplets.h`, tras la linea 34 (`static bool ms_movingEnabled;`):
     ```c
     	static int ms_mode;
     	static void AddExtendedDroplet(int32 particleType, CVector const &pos);
     ```
  2. `src/extras/screendroplets.cpp`, tras la linea 34:
     ```c
     int ScreenDroplets::ms_mode = 1;
     ```
     (default 1 = el modo Neo de hoy: **aspecto identico**).
  3. `src/extras/screendroplets.cpp`, junto a `FillScreenMoving` (tras la linea 388),
     las tablas del mod traducidas a los **nombres** del enum del port
     (`src/renderer/ParticleType.h`), sin IDs hardcodeados:
     ```c
     static float
     GetParticleDistance(int32 type)
     {
     	switch(type){
     	case PARTICLE_WATER_SPARK:
     	case PARTICLE_BLOOD:
     	case PARTICLE_BLOOD_SMALL:
     	case PARTICLE_BLOOD_SPURT:
     	case PARTICLE_RAINDROP:
     	case PARTICLE_RAINDROP_SMALL:
     	case PARTICLE_RAIN_SPLASH:
     	case PARTICLE_RAIN_SPLASH_BIGGROW:
     	case PARTICLE_RAIN_SPLASHUP:
     	case PARTICLE_BLOODDROP:
     	case PARTICLE_RAINDROP_2D:
     		return 5.0f;
     	case PARTICLE_SPLASH:
     	case PARTICLE_WATER_HYDRANT:
     	case PARTICLE_PED_SPLASH:
     		return 10.0f;
     	case PARTICLE_CAR_SPLASH:
     		return 12.0f;
     	case PARTICLE_BOAT_THRUSTJET:
     	case PARTICLE_WATER_CANNON:
     	case PARTICLE_WATER:
     	case PARTICLE_WATERSPRAY:
     	case PARTICLE_WATERDROP:
     		return 20.0f;
     	case PARTICLE_BOAT_SPLASH:
     		return 40.0f;
     	}
     	return 20.0f;
     }

     static bool
     IsParticleBlood(int32 type)
     {
     	switch(type){
     	case PARTICLE_BLOOD:
     	case PARTICLE_BLOOD_SMALL:
     	case PARTICLE_BLOOD_SPURT:
     	case PARTICLE_BLOODDROP:
     		return true;
     	}
     	return false;
     }
     ```
  4. `src/extras/screendroplets.cpp`, la funcion de la gota extendida (el port de
     `AddExtendedDroplets`, `neoWaterdrops.cpp:338-359`):
     ```c
     void
     ScreenDroplets::AddExtendedDroplet(int32 particleType, CVector const &pos)
     {
     	if(ms_mode < 2)
     		return;

     	CVector dist = pos - ms_prevCamPos;
     	float len = dist.Magnitude();
     	if(len < 0.001f || len > GetParticleDistance(particleType))
     		return;

     	bool isBlood = IsParticleBlood(particleType);
     	if(ms_mode < 3 && isBlood)
     		return;

     	FillScreenMoving(1.0f / (len / 2.0f), isBlood);
     }
     ```
     (el `len < 0.001f` evita la division por cero que el mod tiene; la parte de
     "splash" del mod **ya esta** en el port por `RegisterSplash`,
     `ParticleObject.cpp:259`).
  5. `src/extras/screendroplets.cpp:161-168` (`Process`): anadir la llamada
     ```c
     void
     ScreenDroplets::Process(void)
     {
     	ProcessCameraMovement();
     	SprayDrops();
     	ProcessMoving();
     	Fade();
     }
     ```
     (sin cambios: la gota extendida se dispara en el momento de crear la particula,
     no en `Process`; por eso el paso 6 es el que hace falta).
  6. **Fuera de mi territorio** (`src/renderer/Particle.cpp`, funcion
     `CParticle::AddParticle`): **una linea** que llama al gancho:
     ```c
     #ifdef SCREEN_DROPLETS
     	ScreenDroplets::AddExtendedDroplet(type, vecPos);
     #endif
     ```
     (`vecPos` es el `CVector const &vecPos` que ya recibe la funcion).
  7. **Fuera de mi territorio** (`src/weapons/Weapon.cpp:769-773`): el sangrado
     actual debe quedar **condicionado al modo 3**, porque hasta ahora salia siempre:
     ```c
     #ifdef SCREEN_DROPLETS
     	if(ScreenDroplets::ms_mode >= 3)
     		ScreenDroplets::FillScreenMoving(1.0f, true);
     #endif
     ```
  8. `src/core/re3.cpp` (**otro subagente**): la clave
     `CustomPipesValues/neoWaterDrops` (0 = off, 1 = Neo, 2 = Extended,
     3 = Extended + Blood) y el mando de menu con los 4 nombres del mod
     (`"None"`, `"Neo"`, `"Extended"`, `"Extended+Blood"`).
- **Bandera/clave que necesita**: `VICEEXT_SKY_WATERDROPS_EXTENDED` + clave de INI
  `CustomPipesValues/neoWaterDrops`.
- **Dependencia**: el paso 6 (una linea en `src/renderer/Particle.cpp`) y el paso 7
  (dos lineas en `src/weapons/Weapon.cpp`) **no son mio**: hay que coordinarlos con
  quien tenga esos ficheros.
- **Riesgo**:
  - El modo 3 echa gotas **rojas** en cada impacto: con muchas particulas puede
    saturar el array de 2000 gotas (`MAXDROPS`, `screendroplets.h:11`). El mod
    limita con `FillScreenMoving` (`if(ms_numDrops < MAXDROPS && ms_numDropsMoving < MAXDROPSMOVING)`),
    que el port ya tiene (`screendroplets.cpp:376`): el limite esta respetado.
  - El paso 7 cambia el comportamiento actual del sangrado (hoy sale siempre): con
    el modo 1 (default) **dejaria de salir**, salvo que se mantenga el
    `FillScreenMoving(1.0f, true)` actual para el modo 1. Decision a tomar por el
    jugador: la propuesta mas segura es **dejar el paso 7 como esta** (el sangrado al
    disparar es una feature ya portada del 13.0, `VICEEXT_IMPROVED_TECH_PART`) y
    anadir el modo 3 **ademas**. En ese caso el paso 7 no se toca.
  - Con el modo por defecto (1) el comportamiento no cambia en absoluto.
- **-> verify**: con `neoWaterDrops=3`, al saltar una particula de sangre delante de
  la camara aparecen gotas rojas en la lente; con `=2` aparecen gotas claras y
  **no** rojas; con `=1` no aparece ninguna gota nueva (solo las de lluvia, como hoy).

---

# S2 - MatFX PS2/PC/Mobile

> **Territorio de librw**: los cambios de `vendor/librw/**` los escribe otro
> subagente. Aqui se describe **exactamente** el shader, el uniforme y el selector
> que hacen falta, y como hay que reflectirlos en
> `vendor-librw-local-changes.patch`.
>
> **Estado medido del pipeline MatFX en el port** (todo en
> `vendor/librw/src/gl/`):
> - `gl3matfx.cpp:35-59` `matfxDefaultRender` -> usa `defaultShader` (el VS comun).
> - `gl3matfx.cpp:96-150` `matfxEnvRender` -> sube `u_texMatrix`, `u_fxparams`,
>   `u_colorClamp`, `u_envColor` y usa `envShader` / `envShader_noAT` /
>   `envShader_fullLight` / `envShader_fullLight_noAT`.
> - `gl3matfx.cpp:70-93` `uploadEnvMatrix` -> calcula **una sola** matriz de UV de
>   env map: `inverse(LTM del frame del env) * normal2texcoord` (con `normal2texcoord`
>   en `:63-68`).
> - Shaders: `vendor/librw/src/gl/shaders/matfx_env.vert` (31 lineas),
>   `matfx_env.frag` (34), `default.vert` (23).
> - Interruptores ya existentes en librw: `rwplugins.h:163-166`
>   (`envMapFlipU`, `envMapApplyLight`, `envMapUseMatColor`, `envMapColor`).
>   El motor los escribe desde `src/core/re3.cpp:1388-1390` (menu de depuracion).
>   **Ese es el patron a seguir para los 3 selectores nuevos.**
>
> **Como se seleccionaba en el mod** (`matfx.cpp`):
> - `texblendSwitch` -> `config.blendstyle` (0 PS2, 1 PC, 2 Mobile):
>   `_rpMatFXD3D8AtomicMatFXEnvRender_switch_shader` (`matfx.cpp:584-623`) elige
>   entre las tres rutas de fragmento (`ps2_shader:454-520`, `pc_shader:523-551`,
>   `mobile_shader:554-582`), que usan los PS `pcEnvPS` y `mobileEnvPS`.
> - `texgenSwitch` -> `config.texgenstyle`: la **matriz** de UV se calcula en CPU en
>   `ApplyEnvMapTextureMatrix_world` (`matfx.cpp:376-432`), con tres ramas (PS2 /
>   PC / Mobile) y tres matrices (`scalenormal_flipU` en `:109-115`,
>   `scalenormal` en `:99-106`).
> - `ps2light` -> `config.ps2light`: elige el VS del render MatFX
>   (`matfx.cpp:446-447`: `ps2StandardVS` vs `pcStandardVS`; `:472-473` y `:542-543`
>   para las variantes con env map).
> El mod **no toca el shader en tiempo de ejecucion**: compila 3 juegos de shaders y
> elige el puntero. En el port, librw ya compila varias variantes del mismo shader
> con `#define` (`DIRECTIONALS`, `POINTLIGHTS`, `SPOTLIGHTS`, `NO_ALPHATEST`), asi
> que el mismo patron sirve.

### S2-1 `texblendSwitch` (PS2 / PC / Mobile)

- **Fichero**: `vendor/librw/src/gl/shaders/matfx_env.frag` (+ 2 ficheros nuevos),
  `vendor/librw/src/gl/gl3matfx.cpp`, `vendor/librw/src/rwplugins.h`,
  `vendor/librw/src/gl/shaders/Makefile`
- **Bloque**:
  - `vendor/librw/src/gl/shaders/matfx_env.frag:15-33` (el `main`):
    ```
    18: 	vec4 pass1 = v_color;
    19: 	pass1 *= texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
    21: 	vec4 pass2 = v_envColor*shininess*texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y));
    26: 	float fba = max(pass1.a, disableFBA);
    28: 	color.rgb = pass1.rgb*pass1.a + pass2.rgb*fba;
    ```
  - `vendor/librw/src/gl/gl3matfx.cpp:112-117` (los parametros que llegan al shader):
    ```
    112: 	float fxparams[4];
    113: 	fxparams[0] = env->coefficient;
    114: 	fxparams[1] = env->fbAlpha ? 0.0f : 1.0f;
    117: 	setUniform(u_fxparams, fxparams);
    ```
  - `vendor/librw/src/gl/gl3matfx.cpp:196-218` (`matfxOpen`: donde se crean los 4
    `envShader*`).
  - `vendor/librw/src/gl/shaders/Makefile:33-39` (la regla que genera
    `matfx_gl.inc` con `matfx_env.frag` + `matfx_env.vert`).
- **Que hace hoy**: el port tiene **un solo estilo de blend**, que es el **PS2**
  (el `matfx_env.frag` de arriba: pase difuso + pase de env map mezclado con
  `u_colorClamp`/`u_envColor`, `pass1.rgb*pass1.a + pass2.rgb*fba`). Los otros dos
  estilos del mod **no existen**:
  - **PC** (`pcEnvPS.hlsl:12`): `return t0*Color + float4(t1.xyz, 0.0)*EnvCoeff;`
    (el env map se **suma** al difuso).
  - **Mobile** (`mobileEnvPS.hlsl:12`): `return t0*(Color + float4(t1.xyz, 0.0)*EnvCoeff);`
    (el env map **multiplica** al difuso). El mod multiplica el coeficiente por
    **1.45** (`matfx.cpp:569`) y, para los coches (los que no tienen FB alpha),
    lo deja en **0.15** (`matfx.cpp:617`).
  El `texblendSwitch` solo se aplicaba si la clave estaba presente en el INI
  (`main.cpp:836-857`): si no, el juego no se tocaba. Por tanto el **default del
  port debe ser el estilo PS2** (0), que es el actual.
- **Cambio propuesto** (en librw):
  1. **Ficheros nuevos** en `vendor/librw/src/gl/shaders/`:
     - `matfx_env_pc.frag` (el blend PC):
       ```glsl
       uniform sampler2D tex0;
       uniform sampler2D tex1;

       uniform vec4 u_fxparams;

       #define shininess (u_fxparams.x)
       #define disableFBA (u_fxparams.y)

       FSIN vec4 v_color;
       FSIN vec4 v_envColor;
       FSIN vec2 v_tex0;
       FSIN vec2 v_tex1;
       FSIN float v_fog;

       void
       main(void)
       {
       	vec4 pass1 = v_color;
       	pass1 *= texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));

       	vec4 pass2 = v_envColor*shininess*texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y));

       	pass1.rgb = mix(u_fogColor.rgb, pass1.rgb, v_fog);
       	pass2.rgb = mix(vec3(0.0, 0.0, 0.0), pass2.rgb, v_fog);

       	float fba = max(pass1.a, disableFBA);
       	vec4 color;
       	color.rgb = pass1.rgb*pass1.a + pass2.rgb*fba;
       	color.a = pass1.a;

       	DoAlphaTest(color.a);

       	FRAGCOLOR(color);
       }
       ```
       **OJO**: este bloque es el **PS2**; el PC **no** es "otro shader" en el
       port, porque el port ya hace el pase de env map en el mismo fragment shader
       (`pass1` + `pass2`) en vez de en un segundo draw. El PC/Mobile del mod se
       traduzca al port asi:
       - **PC**: `color.rgb = pass1.rgb*pass1.a + pass2.rgb*fba;` -> el env map se
         **suma** en vez de mezclarse por el alpha:
         ```glsl
         	vec4 color;
         	color.rgb = pass1.rgb*pass1.a + pass2.rgb;
         	color.a = pass1.a;
         ```
       - **Mobile**: el env map multiplica al difuso:
         ```glsl
         	vec4 color;
         	color.rgb = pass1.rgb*pass1.a*(1.0 + pass2.rgb*shininess);
         	color.a = pass1.a;
         ```
       Por tanto **bastan dos ficheros nuevos**:
       `vendor/librw/src/gl/shaders/matfx_env_pc.frag` y
       `vendor/librw/src/gl/shaders/matfx_env_mobile.frag`, cada uno una copia de
       `matfx_env.frag` con el `color.rgb` de arriba.
  2. `vendor/librw/src/gl/shaders/Makefile:33-39`: la regla `matfx_gl.inc` tiene que
     incluir los dos ficheros nuevos y exportar dos constantes mas:
     ```make
     matfx_gl.inc: matfx_env.frag matfx_env.vert matfx_env_pc.frag matfx_env_mobile.frag
     	(echo 'const char *matfx_env_vert_src =';\
     	 sed 's/..*/"&\\n"/' matfx_env.vert;\
     	 echo ';';
     	 echo 'const char *matfx_env_frag_src =';\
     	 sed 's/..*/"&\\n"/' matfx_env.frag;\
     	 echo ';';
     	 echo 'const char *matfx_env_pc_frag_src =';\
     	 sed 's/..*/"&\\n"/' matfx_env_pc.frag;\
     	 echo ';';
     	 echo 'const char *matfx_env_mobile_frag_src =';\
     	 sed 's/..*/"&\\n"/' matfx_env_mobile.frag;\
     	 echo ';') >matfx_gl.inc
     ```
     (y hay que **regenerar** `matfx_gl.inc`, que esta versionado).
  3. `vendor/librw/src/rwplugins.h:163-166`: anadir el selector, junto a los otros:
     ```c
     	static int32 envMapBlendStyle;	// 0 = PS2, 1 = PC, 2 = Mobile
     ```
  4. `vendor/librw/src/gl/gl3matfx.cpp`:
     - `matfxOpen` (`:196-218`): crear los 4 shaders de cada estilo (8 en total:
       `envShader`, `envShader_noAT`, `envShader_fullLight`,
       `envShader_fullLight_noAT` para PS2, y los mismos 4 para PC y Mobile).
     - `matfxClose` (`:220-236`): destruirlos.
     - `matfxEnvRender` (`:135-145`): elegir el juego de shaders segun
       `MatFX::envMapBlendStyle`, y **escalar el coeficiente** como hace el mod:
       ```c
       	float coeff = env->coefficient;
       	if(MatFX::envMapBlendStyle == 2)
       		coeff *= 1.45f;
       	fxparams[0] = coeff;
       ```
       (el `env->fbAlpha == 0` -> "coche" del mod: `carMult` 0.25 en VC para el
       estilo PC (`matfx.cpp:588`) y 0.15 para el Mobile (`matfx.cpp:617`) queda
       **fuera**: el port multiplica por `g_nEnvMapStrength` en
       `src/core/main.cpp:2045-2056`, que es el equivalente del control del
       `envMapColor`).
  5. `src/core/re3.cpp` (**otro subagente**): la clave
     `CustomPipesValues/texblendSwitch` (0/1/2) se escribe en
     `rw::MatFX::envMapBlendStyle` (mismo patron que `re3.cpp:1388-1390`).
- **Bandera/clave que necesita**: `VICEEXT_SKY_MATFX_BLEND` + clave de INI
  `CustomPipesValues/texblendSwitch`.
- **Dependencia**: el parche `vendor-librw-local-changes.patch` tiene que incluir
  los 5 hunks (ver S2-4). Y la coordenacion con el 15.0 T2 (especular real del
  retail), que tambien toca `vendor/librw/src/gl/shaders/`.
- **Riesgo**:
  - El cambio de blend afecta a **todos** los materiales con env map de todo el
    juego (los coches, el agua, los cristales). Con el estilo por defecto (0 = PS2)
  no cambia nada; con PC/Mobile cambia el aspecto de la chapa de los coches, que es
  justo lo que se busca, pero **no** hay ninguna captura de referencia del retail
  con la que comparar: el verificador tiene que ser el ojo del jugador.
  - El `matfx_gl.inc` versionado hay que regenerarlo con el `Makefile` de librw
  (que usa `sed`), no a mano.
- **-> verify**: con `texblendSwitch=1` el reflejo de los coches se ve mas "plano"
  (sumado) y con `=2` mas "apagado" (multiplicado); con `=0` o sin clave, la captura
  es identica a la de antes.

### S2-2 `texgenSwitch` (PS2 / PC / Mobile) - la generacion de UV

- **Fichero**: `vendor/librw/src/gl/gl3matfx.cpp`
- **Bloque**: `vendor/librw/src/gl/gl3matfx.cpp:61-93`:
  ```
  63: static RawMatrix normal2texcoord = {
  64: 	{ 0.5f,  0.0f, 0.0f }, 0.0f,
  65: 	{ 0.0f, -0.5f, 0.0f }, 0.0f,
  66: 	{ 0.0f,  0.0f, 1.0f }, 0.0f,
  67: 	{ 0.5f,  0.5f, 0.0f }, 1.0f
  68: };
  ...
  84: 		Matrix::invert(&invMat, frame->getLTM());
  86: 		convMatrix(&invMtx, &invMat);
  87: 		invMtx.pos.set(0.0f, 0.0f, 0.0f);
  88: 		float uscale = fabs(normal2texcoord.right.x);
  89: 		normal2texcoord.right.x = MatFX::envMapFlipU ? -uscale : uscale;
  90: 		RawMatrix::mult(&envMtx, &invMtx, &normal2texcoord);
  92: 	}
  93: 	setUniform(u_texMatrix, &envMtx);
  ```
- **Que hace hoy**: **una sola** generacion de UV: `inverse(LTM del frame) *
  normal2texcoord`, con el signo de U decided por `MatFX::envMapFlipU`
  (`:88-89`). El shader hace `v_tex1 = (u_texMatrix * vec4(Normal, 1.0)).xy;`
  (`matfx_env.vert:21`). Es decir: **la parte "PC"** del mod (que es la que usa
  `envframemat` invertido) y la parte "PS2" coinciden en la estructura, pero el mod
  tiene **tres** ramas distintas (`matfx.cpp:385-423`):
  - **PS2** (`texgenstyle == 0`): `inverse(LTM del frame del env)` con la posicion a
    0 y `flags = 0`, multiplicado por `scalenormal_flipU` (U **negada**:
    `right.x = -0.5`, `pos = (0.5, 0.5, 0)`).
  - **Mobile** (`texgenstyle == 2`): el **LTM de la camara** con `pos = (0.5, 0.5, 0)`
    y `flags = 0`, multiplicado por `scalenormal` (U positiva: `right.x = +0.5`).
  - **PC** (`texgenstyle == 1`): `inverse(LTM del frame) * LTM de la camara`, con la
    fila `right` **negada** y la posicion a 0, multiplicado por `scalenormal`; y
    ademas se aplica la inversa de la camara para llevar la normal a espacio de
    vista (`matfx.cpp:412-422`).
  La diferencia **medible**: si la generacion de UV depende de la camara (PC y
  Mobile) el reflejo "se mueve" con la camara; la PS2 depende solo del frame del env
  map (el reflejo es **estable**).
- **Cambio propuesto** (todo en CPU, **sin tocar los shaders**): `uploadEnvMatrix`
  (`gl3matfx.cpp:70-93`) calcula la matriz segun el estilo. La funcion recibe el
  `Frame *frame` del env map (o la camara, si es null) y el `LTM` de la camara esta
  disponible en `engine->currentCamera->getFrame()->getLTM()`. Propuesta:
  ```c++
  void
  uploadEnvMatrix(Frame *frame)
  {
  	Matrix invMat;
  	if(frame == nil)
  		frame = engine->currentCamera->getFrame();

  	static RawMatrix envMtx;
  	{
  		RawMatrix invMtx;
  		Matrix::invert(&invMat, frame->getLTM());
  		convMatrix(&invMtx, &invMat);
  		invMtx.pos.set(0.0f, 0.0f, 0.0f);

  		float uscale = fabs(normal2texcoord.right.x);
  		normal2texcoord.right.x = MatFX::envMapFlipU ? -uscale : uscale;

  		RawMatrix::mult(&envMtx, &invMtx, &normal2texcoord);
  	}
  	setUniform(u_texMatrix, &envMtx);
  }
  ```
  La variante **exacta** que hay que escribir (las tres ramas del mod
  `matfx.cpp:385-423`, con los mismos signos y las mismas posiciones):
  ```c++
  	switch(MatFX::envMapTexgenStyle){
  	case 1:
  		{
  			RawMatrix invMtx, camMtx;
  			Matrix::invert(&invMat, frame->getLTM());
  			convMatrix(&invMtx, &invMat);
  			invMtx.pos.set(0.0f, 0.0f, 0.0f);
  			convMatrix(&camMtx, &engine->currentCamera->getFrame()->getLTM());
  			camMtx.right.x = -camMtx.right.x;
  			camMtx.right.y = -camMtx.right.y;
  			camMtx.right.z = -camMtx.right.z;
  			camMtx.pos.set(0.0f, 0.0f, 0.0f);
  			RawMatrix::mult(&envMtx, &invMtx, &camMtx);
  			RawMatrix::mult(&envMtx, &envMtx, &normal2texcoord);
  		}
  		break;
  	case 2:
  		{
  			RawMatrix camMtx;
  			convMatrix(&camMtx, &engine->currentCamera->getFrame()->getLTM());
  			camMtx.pos.set(0.5f, 0.5f, 0.0f);
  			RawMatrix::mult(&envMtx, &camMtx, &normal2texcoord);
  		}
  		break;
  	default:
  		{
  			RawMatrix invMtx;
  			Matrix::invert(&invMat, frame->getLTM());
  			convMatrix(&invMtx, &invMat);
  			invMtx.pos.set(0.0f, 0.0f, 0.0f);
  			normal2texcoord.right.x = -fabs(normal2texcoord.right.x);
  			RawMatrix::mult(&envMtx, &invMtx, &normal2texcoord);
  		}
  		break;
  	}
  	setUniform(u_texMatrix, &envMtx);
  ```
  Con `envMapTexgenStyle == 0` (PS2) el resultado es **el mismo que hoy** salvo por
  el signo de U, que es justo lo que hace `envMapFlipU` (que el mod activa siempre
  en el estilo PS2, `matfx.cpp:133-149`: *"for some reason we flip in U as well"*).
  Para no cambiar el aspecto por defecto, el default de `envMapTexgenStyle` tiene
  que ser **0 (PS2)** y el signo de U lo sigue mandando `envMapFlipU` (que hoy esta
  a `false`): es decir, el default del port **no cambia** y el estilo PS2 del mod
  (con U negated) se alcanza con `envMapFlipU` a `true`, que ya es un mando del menu
  (`re3.cpp:1389`). Decision a documentar: **el estilo 0 del port = el estilo PC del
  mod con U positiva**, y el PS2 del mod = el mismo con U negativa. Hay que decirlo
  en el `ATTRIBUTION.md` para que la comparacion sea honesta.
  En `vendor/librw/src/rwplugins.h:163-166` anadir:
  ```c
  	static int32 envMapTexgenStyle;	// 0 = PS2, 1 = PC, 2 = Mobile
  ```
- **Bandera/clave que necesita**: `VICEEXT_SKY_MATFX_TEXGEN` + clave de INI
  `CustomPipesValues/texgenSwitch` (0/1/2).
- **Dependencia**: S2-1 (el mismo fichero de librw, el mismo parche).
- **Riesgo**:
  - **Alto en el sentido visual**: cambiar la generacion de UV cambia el reflejo de
    la chapa de **todo** el juego. Con el default (0) no hay cambio.
  - La matriz `normal2texcoord` es una **variable estatica** que se muta en cada
    llamada (`gl3matfx.cpp:89`): con varias ramas hay que **dejar el signo como
    estaba** al salir, o el frame siguiente hereda el signo del anterior. La
    propuesta lo evita haciendo el signo local a la rama.
  - La conversion `convMatrix` / `RawMatrix::mult` es de librw: hay que comprobar
    que `camMtx` es un `RawMatrix` con la misma disposicion (filas) que la del mod
    (el mod usa `RwMatrixMultiply`, que multiplica en el mismo orden que
    `RawMatrix::mult`).
- **-> verify**: con `texgenSwitch=1` el reflejo del coche cambia al girar la camara
  (el angulo de reflejo se queda "pegado" al objeto); con `=2` el reflejo se mueve
  con la camara; con `=0` o sin clave, la captura es identica a la de antes.

### S2-3 `ps2light` (el caso PC que falta)

- **Fichero**: `vendor/librw/src/gl/shaders/default.vert`,
  `vendor/librw/src/gl/shaders/matfx_env.vert`, `vendor/librw/src/gl/gl3matfx.cpp`,
  `vendor/librw/src/rwplugins.h`
- **Bloque**:
  - `vendor/librw/src/gl/shaders/default.vert:16-20`:
    ```
    16: 	v_color = in_color;
    17: 	v_color.rgb += u_ambLight.rgb*surfAmbient;
    18: 	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
    19: 	v_color = clamp(v_color, 0.0, 1.0);
    20: 	v_color *= u_matColor;
    ```
  - `vendor/librw/src/gl/shaders/matfx_env.vert:23-28` (lo mismo, mas el env map):
    ```
    23: 	v_color = in_color;
    24: 	v_color.rgb += u_ambLight.rgb*surfAmbient;
    25: 	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
    26: 	v_color = clamp(v_color, 0.0, 1.0);
    27: 	v_envColor = max(v_color, u_colorClamp) * u_envColor;
    28: 	v_color *= u_matColor;
    ```
- **Que hace hoy**: el port aplica **siempre** el modelo PS2: el color de material
  multiplica **despues** del `clamp`, y el alfa sale del color de material. El
  caso PC del mod (`pcStandardVS.hlsl:37-41`) es:
  ```
  37: 	float3 c = ambient*surfAmb*matCol.xyz;
  38: 	c += directCol*saturate(dot(N, -directDir))*surfDiff*matCol.xyz;
  40: 	c += lightCol[i]*saturate(dot(N, -lightDir[i]))*surfDiff*matCol.xyz;
  41: 	Out.color = float4(saturate(c), matCol.w);
  ```
  es decir: (a) el color de material multiplica **antes** del `saturate`, (b) el
  alfa es `matCol.w` **suelto**, y (c) el **color de vertice (`in_color`) no se usa**.
  El `ps2light` del mod solo afecta al **pipeline MatFX**
  (`matfx.cpp:446-447`, `:472-473`, `:542-543`), **no** al pipeline normal de RW.
- **Cambio propuesto**:
  1. El VS por defecto de librw (`default.vert`) lo usa **todo** el juego: **no se
     toca**. Lo que hace falta es un **VS propio del pipeline MatFX**, con las dos
     variantes. Ficheros nuevos en `vendor/librw/src/gl/shaders/`:
     - `matfx_default.vert` (el PS2, copia de `default.vert` **sin** la niebla
       duplicada):
       ```glsl
       VSIN(ATTRIB_POS)	vec3 in_pos;

       VSOUT vec4 v_color;
       VSOUT vec2 v_tex0;
       VSOUT float v_fog;

       void
       main(void)
       {
       	vec4 Vertex = u_world * vec4(in_pos, 1.0);
       	gl_Position = u_proj * u_view * Vertex;
       	vec3 Normal = mat3(u_world) * in_normal;

       	v_tex0 = in_tex0;

       	v_color = in_color;
       	v_color.rgb += u_ambLight.rgb*surfAmbient;
       	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
       	v_color = clamp(v_color, 0.0, 1.0);
       	v_color *= u_matColor;

       	v_fog = DoFog(gl_Position.w);
       }
       ```
     - `matfx_default_pc.vert` (el PC):
       ```glsl
       VSIN(ATTRIB_POS)	vec3 in_pos;

       VSOUT vec4 v_color;
       VSOUT vec2 v_tex0;
       VSOUT float v_fog;

       void
       main(void)
       {
       	vec4 Vertex = u_world * vec4(in_pos, 1.0);
       	gl_Position = u_proj * u_view * Vertex;
       	vec3 Normal = mat3(u_world) * in_normal;

       	v_tex0 = in_tex0;

       	vec3 c = u_ambLight.rgb*surfAmbient + DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
       	v_color = vec4(clamp(c*u_matColor.rgb, 0.0, 1.0), u_matColor.a);

       	v_fog = DoFog(gl_Position.w);
       }
       ```
     - El equivalente para el env map: `matfx_env.vert` pasa a tener las dos
       variantes con un `#ifdef`, y se compila dos veces (el patron de
       `gl3matfx.cpp:202-215` ya lo hace con `DIRECTIONALS`/`NO_ALPHATEST`):
       ```glsl
       	v_color = in_color;
       	v_color.rgb += u_ambLight.rgb*surfAmbient;
       	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
       	v_color = clamp(v_color, 0.0, 1.0);
       	v_envColor = max(v_color, u_colorClamp) * u_envColor;
       	v_color *= u_matColor;
       ```
       (PS2, el actual) y, con `#define MATFX_PC_LIGHT`:
       ```glsl
       	vec3 c = u_ambLight.rgb*surfAmbient + DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
       	v_color = vec4(clamp(c*u_matColor.rgb, 0.0, 1.0), u_matColor.a);
       	v_envColor = max(v_color, u_colorClamp) * u_envColor;
       ```
  2. `vendor/librw/src/gl/gl3matfx.cpp`:
     - `matfxDefaultRender` (`:35-59`): usar el shader del MatFX (PS2 o PC) en vez
       del `defaultShader` comun.
     - `matfxEnvRender` (`:135-145`): elegir el VS PS2 o PC segun el mismo flag.
     - `matfxOpen` / `matfxClose`: crear y destruir los shaders nuevos (2 por
       variante de luz x 2 de alpha test x 2 de full light = 8 en total para el env,
       mas 4 para el render plano).
  3. `vendor/librw/src/rwplugins.h:163-166`:
     ```c
     	static bool32 ps2Lighting;	// PS2 style lighting in MatFX
     ```
  4. `src/core/re3.cpp` (**otro subagente**): la clave
     `CustomPipesValues/ps2light` (0 = PC, 1 = PS2) se escribe en
     `rw::MatFX::ps2Lighting` (default **1** = el comportamiento actual).
- **Bandera/clave que necesita**: `VICEEXT_SKY_MATFX_PS2LIGHT` + clave de INI
  `CustomPipesValues/ps2light`.
- **Dependencia**: S2-1 y S2-2 (los tres tocan `gl3matfx.cpp` y los shaders de
  librw: **un solo parche, un solo rebuild**).
- **Riesgo**:
  - **El caso PC descarta el color de vertice (`in_color`)**, que en la geometria
    iluminada es el **prelight** (la luz horneada del mapa): alactivarlo, las
    superficies con lightmap se ven **mas oscuras**. Es el comportamiento del mod
    (su `pcStandardVS` no lee `In.Color`), pero el mod solo lo usa en el pipeline
    MatFX con D3D9.
  - El default (1 = PS2) **no cambia nada**.
  - Tocar `default.vert` (el VS global) seria un error: afectaria a todo el juego.
    Por eso la propuesta es un VS **propio del MatFX**.
- **-> verify**: con `ps2light=0` la chapa de los coches y las superficies con
  env map se ven ligeramente mas planas (el color de material se aplica antes del
  saturado) y las superficies con lightmap pierden el prelight; con `=1` o sin
  clave, la captura es identica a la de antes.

### S2-4 Como se refleja todo esto en `vendor-librw-local-changes.patch`

- **Fichero**: `vendor-librw-local-changes.patch` (raiz del repo)
- **Bloque**: el fichero son **1119 lineas, LF**, con 8 ficheros (`src/clump.cpp`,
  `src/d3d/d3d.cpp`, `src/d3d/d3d8.cpp`, `src/gl/gl3device.cpp`, `src/gl/gl3raster.cpp`,
  `src/gl/shaders/skin.vert`, `src/raster.cpp`, `src/rwobjects.h`, `src/texture.cpp`)
  y ya contiene un hunk de shader (`skin.vert`, lineas 797-807 del parche).
- **Que hace hoy**: el parche es el **registro de los cambios locales del
  submódulo**; ya tiene el formato `diff --git a/... b/...` + `index` + `---`/`+++`
  + `@@` con **un tabulador** de sangria en las lineas de contexto y **`-`/`+`**
  en las cambiadas. Se aplica con `git apply` / `patch -p1` en `vendor/librw`.
- **Cambio propuesto**: anadir **un hunk por fichero tocado**, en el mismo formato,
  y **un hunk de fichero nuevo** para cada shader nuevo:
  ```
  diff --git a/src/gl/shaders/matfx_env_pc.frag b/src/gl/shaders/matfx_env_pc.frag
  new file mode 100644
  index 0000000..0000000
  --- /dev/null
  +++ b/src/gl/shaders/matfx_env_pc.frag
  @@ -0,0 +1,NN @@
  +<linea 1>
  ...
  ```
  Ficheros a anadir al parche (5 ficheros existentes + 3 nuevos):
  | Fichero | Tipo de hunk |
  |---|---|
  | `src/gl/shaders/default.vert` | **no se toca** (es el VS global) |
  | `src/gl/shaders/matfx_env.vert` | modificado (variante `#ifdef MATFX_PC_LIGHT`) |
  | `src/gl/shaders/matfx_env_pc.frag` | **nuevo** |
  | `src/gl/shaders/matfx_env_mobile.frag` | **nuevo** |
  | `src/gl/shaders/matfx_default.vert` | **nuevo** |
  | `src/gl/shaders/matfx_default_pc.vert` | **nuevo** |
  | `src/gl/shaders/Makefile` | modificado (la regla `matfx_gl.inc`) |
  | `src/gl/shaders/matfx_gl.inc` | **regenerado** (versionado) |
  | `src/gl/shaders/default_vs_gl.inc` | **regenerado** si se toca `default.vert` (no) |
  | `src/gl/gl3matfx.cpp` | modificado (los shaders, el selector y la matriz) |
  | `src/rwplugins.h` | modificado (los 3 selectores nuevos) |
- **Bandera/clave que necesita**: ninguna (es el mecanismo, no una feature).
- **Dependencia**: todos los items de S2.
- **Riesgo**: si el parche no se actualiza, el **build de la web** puede seguir
  funcionando (el submodulo esta en el arbol) pero el **repo pierde el registro** de
  los cambios y el proximo `git submodule update` los pierde. Regla del plan:
  "Tocar `vendor/librw` obliga a **actualizar `vendor-librw-local-changes.patch`**".
- **-> verify**: `git apply --check vendor-librw-local-changes.patch` dentro de
  `vendor/librw` sale con 0 (aplica limpio sobre el submodulo), y el build web
  sigue verde.

---

# S3 - Pipes Leeds

> **Como esta el scaffolding en el port** (todo medido):
> - `src/extras/custompipes.cpp:57-91` `CreateEnvMapCam`: camara propia con raster
>   de `EnvMapSize` x `EnvMapSize` (128), near 0.1, far 250, view window 2x2 y la
>   matriz **espejada en X e en Y** (`:79-81`).
> - `src/extras/custompipes.cpp:115-121` `RenderEnvMapScene`:
>   `RenderRoads` + `RenderEverythingBarRoads` + `RenderFadingInEntities`.
> - `src/extras/custompipes.cpp:123-170` `EnvMapRender`: **solo se ejecuta si**
>   `VehiclePipeSwitch == VEHICLEPIPE_NEO` (`:126-127`); renderiza la escena en la
>   camara del env map, la **enmascara** con `CarReflectionMask` (`:153-161`, con
>   `SRCBLEND=BLENDZERO` / `DESTBLEND=BLENDSRCCOLOR`) y devuelve el control.
> - `src/extras/custompipes.cpp:172-230` `EnvMapInit` / `EnvMapShutdown`: crea el
>   quad y busca `CarReflectionMask` en el `neo.txd` (`:175-176`).
> - `src/extras/custompipes_gl.cpp:99-136`: el pipe neo **ya usa `EnvMapTex` como
>   `tex1`** (`:110`) y hace `setTexture(1, nil)` al terminar (`:131`).
> - `src/core/main.cpp:2060-2062`: `CustomPipes::EnvMapRender()` se llama **una vez
>   por frame**, despues de `RenderScene()`.
>
> **Que hace el mod** (el mapa):
> - Pipe de coche Leeds (`leedsCarpipe.cpp`): su **propia** camara y su **propio**
>   env map (`SetupEnvMap`, `:33-54`), que se renderiza en `RenderEnvTex`
>   (`:209-232`) con el cielo + `RenderEverythingBarCarsPeds` (solo edificios,
>   `pipeswitch.cpp:316-332`) + las coronas (`leedsCarpipe.cpp:182-205`). El
>   `CarReflectionMask` **lo usa el pipe neo**, no el de Leeds
>   (`neoCarpipe.cpp:43`, `:168-169`). El material usa la env map como **textura
>   de stage 0** con `D3DTSS_TCI_CAMERASPACENORMAL` (la normal en espacio de
>   camara, `leedsCarpipe.cpp:510`) y el factor `envCoeff * config.leedsEnvMult`
>   (`:493`), con `SRCBLEND=SRCALPHA` y `DESTBLEND=INVSRCALPHA` en el modo VCS
>   (`:484-486`).
>   El VS es `vcsVS.hlsl`: ilumina como el VS del pipe neo pero calcula la UV del
>   env map con la **normal en espacio de vista** (`vcsVS.hlsl:45-47`), no con el
>   vector de reflexion.
> - Pipe de mundo Leeds (`leeds.cpp`): VS propio `leedsWorldVS.hlsl`, que **no**
>   usa la iluminacion dinamica sino el **prelight** (la luz horneada del mapa)
>   con un tweak (`leedsWorldPrelightTweakMult` / `Add`, `leeds.cpp:96`):
>   ```
   35: 	prelight.xyz = saturate(prelight.xyz*tweakMult + float3(1.0,1.0,1.0)*tweakAdd);
   36: 	Out.color = saturate(prelight*float4(ambient, 1.0f) + float4(emissive, 0.0f)*surfEmiss);
   37: 	Out.color.w *= matCol.w;
   ```
>   y el fragmento es el fijo (`D3DTOP_MODULATE2X` en el stage 0, `leeds.cpp:142`).
>
> **Decision de port (medida, no supuesta)**: el mod tiene **dos** env maps (el del
> pipe neo, con mascara, y el del pipe Leeds, sin mascara). El port tiene **uno**
> (con mascara). La propuesta es **reutilizar el env map del port** (`EnvMapTex`,
> ya enmascarado) para el pipe Leeds, que es lo que dice el item 1 del plan
> ("`src/extras/custompipes.cpp` (usar `EnvMapTex` + `EnvMaskTex`)"). Se documenta
> la diferencia en el `ATTRIBUTION.md`.

### S3-1 `carPipe=2` - el pipe de coche "Leeds"

- **Fichero**: `src/extras/custompipes.cpp`, `src/extras/custompipes.h`,
  `src/extras/custompipes_gl.cpp`, `src/extras/shaders/neoLeedsCar.vert`,
  `src/extras/shaders/neoLeedsCar.frag`
- **Bloque**:
  - `src/extras/custompipes.h:85-100` (el enum de los pipes de coche y las
    declaraciones):
    ```
    85: enum {
    86: 	VEHICLEPIPE_MATFX,
    87: 	VEHICLEPIPE_NEO
    88: };
    89: extern int32 VehiclePipeSwitch;
    ```
  - `src/extras/custompipes.cpp:346-371` (el bloque "Neo Vehicle pipe"):
    `:350` `int32 VehiclePipeSwitch = VEHICLEPIPE_MATFX;`
    `:359-371` `AttachVehiclePipe(rw::Atomic*)` / `(rw::Clump*)`.
  - `src/extras/custompipes_gl.cpp:38-180` (el pipe neo de coche):
    `:76-136` `vehicleRenderCB`, `:99-110` (el shader y `setTexture(1, EnvMapTex)`),
    `:112` (`SetRenderState(SRCBLEND, BLENDONE)`), `:138-170`
    `CreateVehiclePipe` / `DestroyVehiclePipe`.
  - `src/extras/custompipes.cpp:126-127` (la puerta que hoy solo deja pasar al neo).
  - El enganche al coche: `src/modelinfo/VehicleModelInfo.cpp:1404` y `:1413`
    (`CustomPipes::AttachVehiclePipe(m_clump)` / `(m_comps[i])`).
- **Que hace hoy**: el enum tiene **2** valores (`MATFX`, `NEO`); el menu
  (`re3.cpp:139` y `:145/:151`) ofrece `"FED_MFX"` / `"FED_NEO"`. El pipe neo
  (`custompipes_gl.cpp:76-136`) hace **un solo draw**: el `neoVehicle.frag` calcula
  el pase difuso y el especular y los suma (`neoVehicle.frag:21-23`). **No hay**
  ningun callback ni shader Leeds.
- **Cambio propuesto**:
  1. `src/extras/custompipes.h`, en el enum de la linea 85:
     ```c
     enum {
     	VEHICLEPIPE_MATFX,
     	VEHICLEPIPE_NEO,
     	VEHICLEPIPE_LEEDS
     };
     ```
     y anadir las declaraciones del pipe nuevo:
     ```c
     extern float LeedsEnvMult;
     extern rw::ObjPipeline *leedsVehiclePipe;
     void CreateLeedsVehiclePipe(void);
     void DestroyLeedsVehiclePipe(void);
     ```
  2. `src/extras/custompipes.cpp:350-357`: anadir
     ```c
     float LeedsEnvMult = 0.4f;
     ```
     (el default del mod **en VC** es 0.4, `main.cpp:655`; el INI documenta 0.3,
     `skygfx.ini:66`: **manda el codigo**).
  3. `src/extras/custompipes.cpp:126-127`: la puerta del env map pasa a ser
     ```c
     	if(VehiclePipeSwitch != VEHICLEPIPE_NEO && VehiclePipeSwitch != VEHICLEPIPE_LEEDS)
     		return;
     ```
  4. `src/extras/custompipes_gl.cpp`: **un callback nuevo**, variante del neo
     (`vehicleRenderCB`, `:76-136`), con estas diferencias medidas:
     - el shader es el de Leeds (iluminacion como el VS del mod `vcsVS`, pero la
       UV del env map sale de la **normal en espacio de vista**, no del vector de
       reflexion);
     - el env map se mezcla con el **coeficiente del material**
       (`MatFX::getEnvCoefficient()`) por `LeedsEnvMult`, sin fresnel ni especular;
     - no hay segundo pase de especular.
     ```c
     static void
     leedsVehicleRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
     {
     	using namespace rw;
     	using namespace rw::gl3;

     	Material *m;

     	rw::uint32 flags = atomic->geometry->flags;
     	setWorldMatrix(atomic->getFrame()->getLTM());
     	lightingCB(atomic);

     	setupVertexInput(header);

     	InstanceData *inst = header->inst;
     	rw::int32 n = header->numMeshes;

     	neoLeedsCarShader->use();

     	setTexture(1, EnvMapTex);

     	SetRenderState(SRCBLEND, BLENDSRCALPHA);
     	SetRenderState(DESTBLEND, BLENDINVSRCALPHA);

     	while(n--){
     		m = inst->material;

     		setMaterial(flags, m->color, m->surfaceProps);

     		setTexture(0, m->texture);

     		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

     		float envProps[4];
     		envProps[0] = 0.0f;
     		MatFX *matfx = MatFX::get(m);
     		if(matfx)
     			envProps[0] = matfx->getEnvCoefficient()*LeedsEnvMult;
     		envProps[1] = 0.0f;
     		envProps[2] = 0.0f;
     		envProps[3] = 0.0f;
     		glUniform4fv(U(u_leedsEnv), 1, envProps);

     		drawInst(header, inst);
     		inst++;
     	}

     	SetRenderState(SRCBLEND, BLENDSRCALPHA);
     	SetRenderState(DESTBLEND, BLENDINVSRCALPHA);

     	setTexture(1, nil);

     	teardownVertexInput(header);
     }
     ```
  5. `src/extras/custompipes_gl.cpp`: `CreateLeedsVehiclePipe` (al lado de
     `CreateVehiclePipe`, `:138-170`):
     ```c
     void
     CreateLeedsVehiclePipe(void)
     {
     	using namespace rw;
     	using namespace rw::gl3;

     	{
     #include "shaders/obj/neoLeedsCar_frag.inc"
     #include "shaders/obj/neoLeedsCar_vert.inc"
     	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoLeedsCar_vert_src, nil };
     	const char *fs[] = { shaderDecl, header_frag_src, neoLeedsCar_frag_src, nil };
     	neoLeedsCarShader = Shader::create(vs, fs);
     	assert(neoLeedsCarShader);
     	}

     	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
     	pipe->instanceCB = rw::gl3::defaultInstanceCB;
     	pipe->uninstanceCB = nil;
     	pipe->renderCB = leedsVehicleRenderCB;
     	leedsVehiclePipe = pipe;
     }
     ```
     y `DestroyLeedsVehiclePipe` al lado de `DestroyVehiclePipe` (`:172-180`).
  6. `src/extras/custompipes.cpp:472-490` (`CustomPipeInit`): anadir
     `CreateLeedsVehiclePipe();` y en `CustomPipeShutdown` (`:492-506`)
     `DestroyLeedsVehiclePipe();`.
  7. `src/extras/custompipes_gl.cpp:566-580` (`CustomPipeRegisterGL`): registrar el
     uniforme nuevo:
     ```c
     	u_leedsEnv = rw::gl3::registerUniform("u_leedsEnv");
     ```
  8. **Los shaders nuevos** (2 ficheros, ver la lista de shaders del final):
     - `src/extras/shaders/neoLeedsCar.vert` (VS): la iluminacion del
       `vcsVS.hlsl` (directa + ambiente + puntuales) y la UV del env map con la
       **normal en espacio de camara** (el equivalente GLSL del
       `D3DTSS_TCI_CAMERASPACENORMAL` del mod, `leedsCarpipe.cpp:510`):
       ```glsl
       uniform vec4 u_leedsEnv;

       VSIN(ATTRIB_POS)	vec3 in_pos;

       VSOUT vec4 v_color;
       VSOUT vec2 v_tex0;
       VSOUT vec2 v_tex1;
       VSOUT float v_fog;

       void
       main(void)
       {
       	vec4 Vertex = u_world * vec4(in_pos, 1.0);
       	gl_Position = u_proj * u_view * Vertex;
       	vec3 Normal = mat3(u_world) * in_normal;

       	v_tex0 = in_tex0;

       	v_color = in_color;
       	v_color.rgb += u_ambLight.rgb*surfAmbient;
       	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
       	v_color = clamp(v_color, 0.0, 1.0);
       	v_color *= u_matColor;

       	vec3 viewNormal = mat3(u_view) * Normal;
       	v_tex1 = viewNormal.xy*0.5 + 0.5;

       	v_fog = DoFog(gl_Position.w);
       }
       ```
       (el `u_view` es el de la cabecera de librw,
       `vendor/librw/src/gl/shaders/header.vert:52`; el `u_eye` **no** hace falta:
       en el `vcsVS.hlsl` el vector `V` se calcula pero no se usa para la UV).
     - `src/extras/shaders/neoLeedsCar.frag` (FS): el difuso mas el env map
       mezclado por el coeficiente:
       ```glsl
       uniform sampler2D tex0;
       uniform sampler2D tex1;

       uniform vec4 u_leedsEnv;

       #define envCoeff (u_leedsEnv.x)

       FSIN vec4 v_color;
       FSIN vec2 v_tex0;
       FSIN vec2 v_tex1;
       FSIN float v_fog;

       void
       main(void)
       {
       	vec4 pass1 = v_color*texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
       	vec4 env = texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y));

       	float f = clamp(envCoeff*env.a, 0.0, 1.0);
       	pass1.rgb = mix(pass1.rgb, env.rgb, f);
       	pass1.rgb = mix(u_fogColor.rgb, pass1.rgb, v_fog);

       	DoAlphaTest(pass1.a);

       	FRAGCOLOR(pass1);
       }
       ```
  9. `src/core/re3.cpp` (**otro subagente**): el menu del pipe de coche pasa a
     3 valores (`"FED_MFX"`, `"FED_NEO"`, `"FED_LEEDS"`), apuntando a
     `CustomPipes::VehiclePipeSwitch`; y las claves
     `CustomPipesValues/carPipe` (0/1/2) y `CustomPipesValues/leedsEnvMult`.
- **Bandera/clave que necesita**: `VICEEXT_SKY_LEEDS_CAR_PIPE` + claves de INI
  `CustomPipesValues/carPipe` y `CustomPipesValues/leedsEnvMult`.
- **Dependencia**: S1-1 (el tamano del env map) es independiente; el shader nuevo
  se compila con `make_glsl.sh` (ver la lista de shaders).
- **Riesgo**:
  - **El env map del port renderiza coches y peds** (`custompipes.cpp:115-121`),
    mientras que el de Leeds del mod renderiza **solo edificios**
    (`pipeswitch.cpp:316-332`). Con el env map del port, el coche se vera **reflejado
    a si mismo** (el "ruido" de las ruedas girando del que habla el recibo, §3.2).
    Es una desviacion medida que hay que documentar.
  - El pipe Leeds **no** lleva el especular del pipe neo: la chapa se vera mas
    mate. Es la intension del mod.
  - `MatFX::getEnvCoefficient()` puede ser 0 en los materiales sin env map: en ese
    caso el env map no se aplica (correcto).
  - La `CarReflectionMask` se aplica al env map del port; el de Leeds del mod **no**
    la lleva. Con el env map del port, el reflejo sale **maskeado** (la mascara
    limita el reflejo a la parte alta de la chapa).
- **-> verify**: con `carPipe=2` el coche refleja el entorno con el factor de
  `leedsEnvMult` (0.4 por defecto) y **sin** el especular del pipe neo; con
  `carPipe=1` el resultado es identico al de hoy; con `carPipe=0` el coche vuelve
  al MatFX normal.

### S3-2 `worldPipe=2` - el pipe de mundo "Leeds"

- **Fichero**: `src/extras/custompipes_gl.cpp`, `src/extras/custompipes.cpp`,
  `src/extras/custompipes.h`, `src/extras/shaders/leedsWorld.vert`,
  `src/extras/shaders/leedsWorld.frag`
- **Bloque**:
  - `src/extras/custompipes_gl.cpp:184-283` (el pipe neo de mundo):
    `:190-245` `worldRenderCB` (con la puerta de `LightmapEnable` en `:196-199`),
    `:247-273` `CreateWorldPipe` / `DestroyWorldPipe` (el shader es
    `default_UV2.vert` + `neoWorldVC.frag`, `:258-265`).
  - `src/extras/custompipes.cpp:374-394` (los conmutadores del mundo:
    `LightmapEnable`, `LightmapMult`, `WorldLightmapBlend`).
  - `src/extras/custompipes.h:102-109`.
- **Que hace hoy**: el pipe de mundo usa **el mismo VS** que el pipeline normal
  (`default_UV2.vert`, que es `default.vert` + `v_tex1 = in_tex1`) y su propio FS
  (`neoWorldVC.frag`) que mezcla la textura difusa con el **lightmap dual**:
  `color = t0*v_color*(1.0 + u_lightMap*(t1-1.0))`. **No hay** el shader de Leeds
  ni el tweak de prelight.
- **Cambio propuesto**:
  1. `src/extras/custompipes.h`: el enum del mundo (creado en S1-2) ya tiene
     `WORLDPIPE_LEEDS`; anadir:
     ```c
     extern float LeedsWorldPrelightTweakMult;
     extern float LeedsWorldPrelightTweakAdd;
     extern rw::ObjPipeline *leedsWorldPipe;
     void CreateLeedsWorldPipe(void);
     void DestroyLeedsWorldPipe(void);
     ```
  2. `src/extras/custompipes.cpp:378-380`: anadir
     ```c
     float LeedsWorldPrelightTweakMult = 1.0f;
     float LeedsWorldPrelightTweakAdd = 0.0f;
     ```
     (defaults del **codigo** del mod, `main.cpp:656-657`; el INI documenta 0.5 /
     0.1, `skygfx.ini:67-68`: **manda el codigo**).
  3. `src/extras/custompipes_gl.cpp`: **un callback nuevo** (el `leedsWorldRenderCB`
     al que llama la rama `WORLDPIPE_LEEDS` de S1-2):
     ```c
     static void
     leedsWorldRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
     {
     	using namespace rw;
     	using namespace rw::gl3;

     	Material *m;

     	rw::uint32 flags = atomic->geometry->flags;
     	setWorldMatrix(atomic->getFrame()->getLTM());
     	lightingCB(atomic);

     	setupVertexInput(header);

     	InstanceData *inst = header->inst;
     	rw::int32 n = header->numMeshes;

     	leedsWorldShader->use();

     	float tweak[4];
     	tweak[0] = LeedsWorldPrelightTweakMult;
     	tweak[1] = LeedsWorldPrelightTweakAdd;
     	tweak[2] = 0.0f;
     	tweak[3] = 0.0f;
     	glUniform4fv(U(u_prelightTweak), 1, tweak);

     	while(n--){
     		m = inst->material;

     		setMaterial(flags, m->color, m->surfaceProps, 0.5f);

     		setTexture(0, m->texture);

     		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

     		drawInst(header, inst);
     		inst++;
     	}

     	teardownVertexInput(header);
     }
     ```
     (el `setMaterial` con el 4º argumento = `u_surfProps.w` = el `surfEmiss` del mod
     (`leeds.cpp:153`: `surfProps[3] = 0.5f`), ver
     `vendor/librw/src/gl/rwgl3.h:188`; el `lightingCB(atomic)` es **imprescindible**:
     es el que sube `u_ambLight`, que en este shader es el "ambient" del mod
     (`leeds.cpp:106-109`: `CTimeCycle__m_fCurrentAmbient*`)).
  4. `src/extras/custompipes_gl.cpp`: `CreateLeedsWorldPipe` (al lado de
     `CreateWorldPipe`, `:247-273`):
     ```c
     void
     CreateLeedsWorldPipe(void)
     {
     	using namespace rw;
     	using namespace rw::gl3;

     	{
     #include "shaders/obj/leedsWorld_frag.inc"
     #include "shaders/obj/leedsWorld_vert.inc"
     	const char *vs[] = { shaderDecl, header_vert_src, leedsWorld_vert_src, nil };
     	const char *fs[] = { shaderDecl, header_frag_src, leedsWorld_frag_src, nil };
     	leedsWorldShader = Shader::create(vs, fs);
     	assert(leedsWorldShader);
     	}

     	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
     	pipe->instanceCB = rw::gl3::defaultInstanceCB;
     	pipe->uninstanceCB = nil;
     	pipe->renderCB = leedsWorldRenderCB;
     	leedsWorldPipe = pipe;
     }
     ```
  5. `src/extras/custompipes.cpp:383-394`: `AttachWorldPipe` tiene que elegir la
     pipe segun el conmutador:
     ```c
     void
     AttachWorldPipe(rw::Atomic *atomic)
     {
     	atomic->pipeline = worldPipe;
     }
     ```
     (una sola pipeline con tres ramas dentro del callback es mas simple y evita
     tocar el enganche de `SimpleModelInfo.cpp:94`).
  6. **Los shaders nuevos**:
     - `src/extras/shaders/leedsWorld.vert` (VS), el port de `leedsWorldVS.hlsl`:
       ```glsl
       uniform vec4 u_prelightTweak;

       #define tweakMult (u_prelightTweak.x)
       #define tweakAdd (u_prelightTweak.y)

       VSIN(ATTRIB_POS)	vec3 in_pos;

       VSOUT vec4 v_color;
       VSOUT vec2 v_tex0;
       VSOUT float v_fog;

       void
       main(void)
       {
       	vec4 Vertex = u_world * vec4(in_pos, 1.0);
       	gl_Position = u_proj * u_view * Vertex;

       	v_tex0 = in_tex0;

       	vec4 prelight = in_color;
       	prelight.rgb = clamp(prelight.rgb*tweakMult + vec3(tweakAdd), 0.0, 1.0);

       	vec3 emissive = u_ambLight.rgb*0.5;
       	v_color = clamp(prelight*vec4(u_ambLight.rgb, 1.0) + vec4(emissive*u_surfProps.w, 0.0), 0.0, 1.0);
       	v_color.a *= u_matColor.a;

       	v_fog = DoFog(gl_Position.w);
       }
       ```
       (el `emissive` del mod es `AmbientSaturated*0.5`, `leeds.cpp:51-53`: se
       aproxima con `u_ambLight.rgb*0.5`, que es lo que el port puede tener a mano
       sin tocar librw).
     - `src/extras/shaders/leedsWorld.frag` (FS), el equivalente GLSL del
       `D3DTOP_MODULATE2X` del stage 0 (`leeds.cpp:142`):
       ```glsl
       uniform sampler2D tex0;

       FSIN vec4 v_color;
       FSIN vec2 v_tex0;
       FSIN float v_fog;

       void
       main(void)
       {
       	vec4 color = v_color*texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
       	color.rgb = min(color.rgb*2.0, vec3(1.0));

       	color.rgb = mix(u_fogColor.rgb, color.rgb, v_fog);
       	DoAlphaTest(color.a);

       	FRAGCOLOR(color);
       }
       ```
  7. `src/core/re3.cpp` (**otro subagente**): las claves
     `CustomPipesValues/worldPipe` (ya de S1-2),
     `CustomPipesValues/leedsWorldPrelightTweakMult` y
     `CustomPipesValues/leedsWorldPrelightTweakAdd`.
- **Bandera/clave que necesita**: `VICEEXT_SKY_LEEDS_WORLD_PIPE` + las 2 claves de
  tweak + `worldPipe`.
- **Dependencia**: **S1-2** (la rama `WORLDPIPE_LEEDS` del conmutador del mundo).
- **Riesgo**:
  - **Este es el shader mas pesado de portar**: sustituye la iluminacion del mundo
    entero. El `prelight` (la luz horneada) pasa a multiplicar el ambiente, lo que
    cambia el brillo de **todas** las superficies del mundo.
  - El `x2` del `MODULATE2X` duplica el brillo: con `clamp` a 1 se satura. Es el
    equivalente del `MODULATE2X` de D3D8 (`C1 = (C2*C3)*2`, saturado).
  - `leedsWorldVS` **no usa la iluminacion dinamica**: con este pipe, las luces
    (faros, explosiones) dejan de illuminate el mundo. Es lo que hace el mod.
  - El `prelight` (`in_color`) en el port es el color de vertice: en la geometria
    **sin** prelight vale 0 y el mundo saldria negro. El pipe de Leeds solo tiene
    sentido en geometria con lightmap (edificios). **El pipe neo de mundo
    (`:190-245`) tiene el mismo problema** y lo resuelve con el flag `LightmapEnable`
    (que el jugador apaga por defecto): con el pipe Leeds hay que comprobar que
    solo se aplica a la geometria con prelight, o la escena se apaga.
- **-> verify**: con `worldPipe=2` y
  `leedsWorldPrelightTweakMult=1.2` / `Add=0.1` el mundo se ve mas claro y con mas
  contraste de luz horneada que con el pipe neo; con `worldPipe=0` o `1` el mundo
  es identico a hoy.

---

# S4 - Sueltos de SkyGfx

### S4-1 `seamFix` (el arreglo de costuras)

- **Fichero**: `src/extras/postfx.cpp` (el helper) + `src/core/main.cpp` (**fuera**,
  solo se describe donde encaja)
- **Bloque**:
  - `src/core/main.cpp:1649`: el comentario `// seam fixer` (dentro de
    `RenderScene`, justo despues de `CRenderer::RenderEverythingBarRoads();`).
  - `src/core/main.cpp:2057-2062`: el punto donde el mod lo llama
    (`RenderScene()` y luego `CustomPipes::EnvMapRender()`), que es el equivalente
    del `RenderScene_hook` del mod (`pipeswitch.cpp:226-245`: `RenderScene(); if(config.seamfix) fixSeams();`).
  - `src/extras/postfx.cpp:16-17` (los dos buffers: `pFrontBuffer` y `pBackBuffer`),
    `:428-433` (`GetBackBuffer`: copia la pantalla actual a `pBackBuffer`),
    `:494-500` (la copia a `pFrontBuffer` al final de `Render`).
- **Que hace hoy**: **nada**. Solo el comentario `// seam fixer` en
  `src/core/main.cpp:1649` (sin implementacion). El mod redibuja el **front buffer**
  desplazado 1 px encima de la escena (`pipeswitch.cpp:88-103` `fixSeams` +
  `:32-86` `makeSeamQuad`): copia la pantalla actual al front buffer
  (`ScreenFX::UpdateFrontBuffer`) y la dibuja con `seamOffX/seamOffY` (1, 1 por
  defecto) encima, con filtrado NEAREST, Z-test on y Z-write off.
- **Cambio propuesto**:
  1. `src/extras/postfx.h`: anadir la declaracion del helper:
     ```c
     	static void RenderSeamFix(RwCamera *cam, int32 offX, int32 offY);
     ```
  2. `src/extras/postfx.cpp`: el helper, que reutiliza `pBackBuffer` (la copia de la
     pantalla actual que ya hace `GetBackBuffer`, `:428-433`) y el quad `Vertex`:
     ```c
     void
     CPostFX::RenderSeamFix(RwCamera *cam, int32 offX, int32 offY)
     {
     	if(!pBackBuffer)
     		return;

     	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
     	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
     	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
     	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
     	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)pBackBuffer);
     	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

     	RwIm2DVertexSetScreenX(&Vertex[0], -0.5f + offX);
     	RwIm2DVertexSetScreenY(&Vertex[0], -0.5f + offY);
     	RwIm2DVertexSetScreenX(&Vertex[1], -0.5f + offX);
     	RwIm2DVertexSetScreenY(&Vertex[1], pBackBuffer->height - 0.5f + offY);
     	RwIm2DVertexSetScreenX(&Vertex[2], pBackBuffer->width - 0.5f + offX);
     	RwIm2DVertexSetScreenY(&Vertex[2], pBackBuffer->height - 0.5f + offY);
     	RwIm2DVertexSetScreenX(&Vertex[3], pBackBuffer->width - 0.5f + offX);
     	RwIm2DVertexSetScreenY(&Vertex[3], -0.5f + offY);

     	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);

     	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
     	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
     	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
     	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
     }
     ```
     (los UV no se tocan: `Vertex` ya los tiene de `Open()`, `:91-125`).
  3. `src/core/main.cpp` (**fuera de mi territorio**): el enganche va **despues** de
     `RenderScene()` (linea **2057**) y **antes** de `CustomPipes::EnvMapRender()`
     (linea **2061**), que es donde el mod lo llama (`pipeswitch.cpp:236-237`):
     ```c
     	RenderScene();
     	tbEndTimer("RenderScene");
     #ifdef VICEEXT_SKY_SEAM_FIX
     	CPostFX::RenderSeamFix(Scene.camera, g_nSeamOffsetX, g_nSeamOffsetY);
     #endif
     ```
     (los offsets, leidos del INI, los pone otro subagente en `re3.cpp`; el
     `// seam fixer` de la linea 1649 se puede borrar o dejar: es un comentario
     muerto de una feature que nunca se implemento).
- **Bandera/clave que necesita**: `VICEEXT_SKY_SEAM_FIX` + claves de INI
  `CustomPipesValues/seamFix` (0/1) y `CustomPipesValues/seamOffsetX` /
  `seamOffsetY` (los dos con el nombre del menu del mod, `main.cpp:729-731`).
- **Dependencia**: la copia de la pantalla actual (`pBackBuffer`) solo existe si
  `CPostFX::NeedBackBuffer()` devuelve true (`postfx.cpp:366-384`), que depende de
  `EffectSwitch` y `MotionBlurOn`. Con el overlay apagado **no hay copia**: el
  helper tendra que forzar la copia (`GetBackBuffer(cam)`) antes de dibujar.
- **Riesgo**:
  - **Es un parche de pantalla completa**: duplica la imagen desplazada 1 px, lo que
    puede verse como un "doble" en las bordes de la pantalla y **puede blarear** la
    imagen (el propio recibo lo dice: "puede blarear"). En un lienzo 16:9 el
    desplazamiento de 1 px es proporcionalmente mas grande.
  - Con la bandera apagada no se toca nada.
  - El `MODULATE2X`/el offset se aplican sobre la pantalla **ya tintada** por el
    post-proceso si el orden es incorrecto: por eso el enganche va justo despues de
    `RenderScene()`.
- **-> verify**: con `seamFix=1` las costuras entre objetos desaparecen y el borde
  de la pantalla queda limpio; el jugador confirma que la imagen no se ve borrosa.
  Con la clave ausente, la imagen es identica a la de antes.

### S4-2 `ps2Water` (los UVs del agua)

- **Fichero**: `src/renderer/WaterLevel.cpp` (**fuera de mi territorio**);
  `src/rw/RwHelper.cpp` y `src/rw/RwPS2AlphaTest.cpp` (referencia, **fuera**)
- **Bloque**:
  - `src/renderer/WaterLevel.cpp:79-83`:
    ```
    79: #ifdef PC_WATER
    80: float fEnvScale               = 0.25f;
    81: #else
    82: float fEnvScale               = 0.5f;
    83: #endif
    ```
    y `:395-404` (la textura del material de la mascara):
    ```
    397: #ifdef PC_WATER
    398: 	RpMaterialSetTexture(maskMaterial, gpWaterEnvBaseTex);
    399: #else
    400: 	RpMaterialSetTexture(maskMaterial, gpWaterTex);
    401: #endif
    ```
    (`src/core/config.h:466` hace `#undef PC_WATER`, luego el port usa **la rama
    PS2**).
  - `src/renderer/WaterLevel.cpp:2307-2532` `CWaterLevel::PreCalcWavyMask`: los UVs
    de la mascara se escriben en `:2366-2374`:
    ```
    2366: maskTexCoords[base+a].v = float(j) / 32 + ((fCamPosY - fY) / 64);
    2371: maskTexCoords[base+a].u = float(i) / 32 + ((fCamPosX - fX) / 64);
    ```
  - `src/rw/RwHelper.cpp:17-21` (`gPS2alphaTest`) y `:91-96` (su uso:
    `rw::SetRenderState(rw::GSALPHATEST, gPS2alphaTest)`).
  - `src/rw/RwPS2AlphaTest.cpp:1`: `#ifndef LIBRW` -> **todo el fichero esta muerto**
    en el port (incluido `_rxD3D8DualPassRenderCallback`, `:17`).
- **Que hace hoy**: **dos de las tres** piezas del `ps2Water` del mod **ya estan**:
  - el *swap* de la textura de la mascara ("sandywater" -> agua clara): la rama
    `#else` de `:399-401` (con `PC_WATER` sin definir) pone `gpWaterTex`;
  - el coeficiente de env map de la mascara: `fEnvScale = 0.5f` (`:82`), que es el
    `maskWaterEnvCoeff = 0.4f` -> "luego cambiado a 0.5" del mod
    (`WaterLevel.cpp:86`).
  **Lo que falta** es la tercera pieza: el **parche de UVs** del mask atomic
  (`WaterLevel.cpp:24-37` del mod).
- **Cambio propuesto** (con una salvedad importante, medida):
  el mod reescribe los UVs del mask atomic con
  `uv[i*33 + j].u = ustart + ustep*i` y `v = vstart + vstep*j`, con
  `ustep = vstep = 2.0f/32.0f` (`WaterLevel.cpp:27-37`), es decir **el doble** de
  la escala del retail. **El layout de vertices no es el mismo**: el mod escribe
  sobre `33*33 = 1089` vertices (uno por punto de la rejilla) y el port sobre
  `33*33*4 = 4356` (cuatro por celda, con los offsets `a/b/c/d` de
  `WaterLevel.cpp:2359-2374`). Por tanto el parche **no es 1:1**:
  - el equivalente en el port es **escalar por 2** el avance por celda: `i/32` ->
    `i/16` y `j/32` -> `j/16`, y el desplazamiento de 0.5 UV de la celda -> 1.0;
  - o, equivalentemente, multiplicar el `offset` de camara por 2.
  La propuesta minima, en `src/renderer/WaterLevel.cpp:2366-2374` (**fuera** de mi
  territorio), con la bandera:
  ```c
  #ifdef VICEEXT_SKY_PS2_WATER
  			maskTexCoords[base+a].v = float(j) / 16 + ((fCamPosY - fY) / 64);
  			maskTexCoords[base+a].u = float(i) / 16 + ((fCamPosX - fX) / 64);
  			maskTexCoords[base+d].v = maskTexCoords[base+a].v + 1.0f;
  			maskTexCoords[base+c].u = maskTexCoords[base+a].u + 1.0f;
  #else
  			maskTexCoords[base+a].v = float(j) / 32 + ((fCamPosY - fY) / 64);
  			maskTexCoords[base+a].u = float(i) / 32 + ((fCamPosX - fX) / 64);
  			maskTexCoords[base+d].v = maskTexCoords[base+a].v + 0.5f;
  			maskTexCoords[base+c].u = maskTexCoords[base+a].u + 0.5f;
  #endif
  ```
  **Aviso al orquestador**: este es el item con mas incertidumbre visual del plan.
  La version *literal* del parche del mod **no es portable** por el layout de
  vertices; lo que se propone es **el equivalente funcional** (el doble de escala
  de UV). Si el jugador quiere el parche exacto, hay que medir el layout del
  mask atomic del retail, que **no** esta verificado aqui.
  El resto del item (`gPS2alphaTest`, el alpha test PS2) **ya esta** en el port
  (`RwHelper.cpp:95` -> `rw::GSALPHATEST`): `src/rw/RwPS2AlphaTest.cpp` es codigo
  muerto (`#ifndef LIBRW`) y **no** hay que tocarlo (no se borra: regla del repo).
- **Bandera/clave que necesita**: `VICEEXT_SKY_PS2_WATER` + clave de INI
  `CustomPipesValues/ps2Water` (0/1).
- **Dependencia**: el `ps2Water` del mod **exige** `texblendSwitch=0` (PS2),
  `skygfx.ini:29`: con el blend Mobile/PC el agua se ve distinta. Si S2-1 esta
  activo, la combinacion hay que probarla.
- **Riesgo**:
  - El agua es la superficie mas visible del juego: un error de escala de UV se ve
    como una textura emborronada o con costuras.
  - El layout de vertices del port **no** es el del mod (medido): la propuesta es
    una equivalencia funcional, no una copia. **Riesgo alto de diferencia visual**:
    requiere una captura A/B con el agua a la vista.
  - Con la bandera apagada, nada cambia (las dos piezas que ya estan no se tocan).
- **-> verify**: con `ps2Water=1` el agua tiene el doble de escala de textura que
  antes (el jugador lo ve en la orilla) y no aparecen costuras; con la clave
  ausente el agua es identica a la de hoy.

### S4-3 `dualPass` (doble pase de transparencia)

- **Fichero**: `src/extras/custompipes_gl.cpp`
- **Bloque**: los 5 puntos de draw de los pipes propios:
  - `custompipes_gl.cpp:127` (`vehicleRenderCB`, el `drawInst(header, inst);`),
  - `:240` (`worldRenderCB`),
  - `:338` (`glossRenderCB`),
  - `:453` (`rimSkinRenderCB`),
  - `:495` (`rimRenderCB`).
  Y el estado de blend que ya se usa en el pipe de gloss:
  `custompipes_gl.cpp:326` (`SetRenderState(ALPHATESTFUNC, ALPHAALWAYS)`) y
  `:345` (`SetRenderState(ALPHATESTFUNC, ALPHAGREATEREQUAL)`).
- **Que hace hoy**: **no hay doble pase**. En librw GL3 el alpha test **no** es
  `glAlphaFunc`: se hace **en el shader** (`vendor/librw/src/gl/gl3device.cpp:1195-1215`,
  con `u_alphaRef` y `DoAlphaTest`), y los render states `rw::ALPHATESTFUNC` /
  `rw::ALPHATESTREF` lo controlan (`gl3device.cpp:893-902`; `ALPHATESTREF` se
  normaliza a 0..1 con `value/255.0f`). El mod, en cambio, intercepta el draw de
  RW (`main.cpp:204-269` `drawDualPass`) y, si hay alpha blending + Z-write,
  dibuja **dos veces**: primero con `ALPHAFUNC >= 128` (Z-write on) y luego con
  `ALPHAFUNC < 128` (Z-write off).
  Nota: `src/rw/RwPS2AlphaTest.cpp:17` tiene un `_rxD3D8DualPassRenderCallback`
  **muerto** (`#ifndef LIBRW`, linea 1): no se puede reutilizar.
- **Cambio propuesto**: un helper en `custompipes_gl.cpp` que usa **los render
  states de librw** (no `glAlphaFunc`), replicando exactamente la logica del mod:
  ```c
  static bool g_bDualPass = false;

  static void
  DrawDualPass(rw::gl3::InstanceDataHeader *header, rw::gl3::InstanceData *inst, bool alphaBlend, bool zwrite)
  {
  	using namespace rw;
  	using namespace rw::gl3;

  	if(!g_bDualPass || !alphaBlend || !zwrite){
  		drawInst(header, inst);
  		return;
  	}

  	SetRenderState(ALPHATESTFUNC, ALPHAGREATEREQUAL);
  	SetRenderState(ALPHATESTREF, 128);
  	drawInst(header, inst);

  	SetRenderState(ALPHATESTFUNC, ALPHALESS);
  	SetRenderState(ALPHATESTREF, 128);
  	SetRenderState(ZWRITEENABLE, FALSE);
  	drawInst(header, inst);

  	SetRenderState(ZWRITEENABLE, TRUE);
  	SetRenderState(ALPHATESTFUNC, ALPHAGREATEREQUAL);
  	SetRenderState(ALPHATESTREF, 3);
  }
  ```
  y sustituir los 5 `drawInst(header, inst);` por
  ```c
  	DrawDualPass(header, inst, inst->vertexAlpha || m->color.alpha != 0xFF, true);
  ```
  (el `zwrite` que se pasa es el del estado actual: los pipes own lo dejan en
  `TRUE` salvo el pase de gloss, que lo apaga en `:325`).
  El `ALPHATESTREF = 3` final restaura el valor por defecto del port
  (`src/rw/RwHelper.cpp:93`).
- **El resto de la geometria transparente**: el port ya enruta **casi toda** la
  geometria del mundo por los pipes propios:
  - el mundo -> `worldRenderCB` (todo modelo simple: `SimpleModelInfo.cpp:94`),
  - las carreteras -> `glossRenderCB` (que encadena `worldRenderCB`),
  - los coches -> `vehicleRenderCB` (`VehicleModelInfo.cpp:1404/1413`),
  - los peds -> `rimRenderCB` / `rimSkinRenderCB` (`PedModelInfo.cpp:86`).
  Lo que **no** pasa por ahi es la pipeline **MatFX de librw**
  (`vendor/librw/src/gl/gl3matfx.cpp:153-182` `matfxRenderCB`, con los materiales
  de bump/env map) y la pipeline por defecto de librw. Para cubrir **todo** hace
  falta **uno de estos dos**:
  1. **un cambio en librw** (envolver `drawInst` en `gl3matfx.cpp` y en el render
     callback por defecto): es el mas fiel al mod (que hookea el draw de RW, no un
     pipeline), y obliga a **actualizar `vendor-librw-local-changes.patch`**;
  2. **un callback propio en el port** que se enganche a los atomics que no pasan
     por `CSimpleModelInfo::SetAtomic` (que es el mismo sitio donde el mod hookea:
     `pipeswitch.cpp:114-130` `CSimpleModelInfo::SetAtomic_hook`).
  **Recomendacion**: la opcion 1, porque el hook del mod es **global** (todo draw
  de RW) y el port solo tiene 4 pipelines. Con la opcion 1, `dualPass` se aplica a
  la geometria transparente de todo el juego.
- **Bandera/clave que necesita**: `VICEEXT_SKY_DUAL_PASS` + clave de INI
  `CustomPipesValues/dualPass` (0/1).
- **Dependencia**: si se elige la opcion 1, depende del parche de librw (S2-4).
- **Riesgo**:
  - **Coste**: cada geometria con alpha se dibuja **dos veces**. En WebGL2, en un
    navegador, con la geometria transparente de una ciudad grande, eso puede
    costarle FPS al jugador. Hay que medirlo con el log (`tbStartTimer`).
  - El `ALPHALESS` con `ALPHATESTREF = 128` cambia el **corte** del alpha test de
    3 a 128 en el segundo pase: los pixeles con alfa entre 3 y 128 pasan al
    segundo pase (correcto, es el objetivo), pero el **primer** pase con
    `>= 128` deja fuera los de alfa entre 3 y 127 (que antes si se dibujaban con
    el alpha test normal). El resultado final es el correcto, pero el **orden** de
    dibujo dentro del frame cambia (Z-write off en el segundo pase).
  - El estado `ALPHATESTFUNC`/`ALPHATESTREF` es **global** de librw: hay que
    **restaurarlo** al final (propuesta: `ALPHAGREATEREQUAL` / `3`, los valores de
    `DefinedState`, `RwHelper.cpp:92-93`).
- **-> verify**: con `dualPass=1` las geometrias con alfa (rejas, vallas, hojas)
  dejan de mostrar "costuras" de alfa; el log del jugador no muestra una caida de
  FPS por encima del 10 % con `tbStartTimer`/`tbEndTimer` de `RenderScene`.

### S4-4 El fix de blend del sifon de auto-aim y del punto del francotirador

- **Fichero**: `src/renderer/Hud.cpp` (**fuera de mi territorio**: solo se describe)
- **Bloque**: `src/renderer/Hud.cpp:586-593` (el punto del francotirador / laser
  dot):
  ```
  586: 						RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
  587: 						RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
  588: 						RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
  589: #ifdef FIX_BUGS
  590: 						RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
  591: #else
  592: 						RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVDESTALPHA);
  593: #endif
  ```
  y `:600-601` el dibujo del punto (`CSprite::RenderOneXLUSprite(...)`).
- **Que hace hoy**: el retail usa el modo de blend **equivocado** para esos dos
  elementos (`push 6` = `rwBLENDINVDESTALPHA` en el enum de RenderWare 3.x:
  `rwBLENDZERO=1, rwBLENDONE=2, rwBLENDSRCALPHA=3, rwBLENDINVSRCALPHA=4,
  rwBLENDDESTALPHA=5, rwBLENDINVDESTALPHA=6`) y el mod lo parchea a
  `rwBLENDINVSRCALPHA` (`main.cpp:928-934`: `push 6` -> `push 4` en `0x5D4EEE`
  (auto-aim) y `0x558024` (punto del francotirador)).
  En el port: el **punto del francotirador** ya esta en `rwBLENDINVSRCALPHA`
  **gracias a `FIX_BUGS`** (`src/core/config.h:414`, que esta activo); sin
  `FIX_BUGS` seria `rwBLENDINVDESTALPHA` (la linea 592).
  El **sifon de auto-aim**: **no existe ningun dibujo en el port**. Medido:
  `rg rwBLENDINVDESTALPHA src/` da 4 coincidencias y ninguna es el sifon
  (`src/weapons/WeaponEffects.cpp:124`, `src/renderer/MBlur.cpp:673`,
  `src/renderer/Hud.cpp:592` y la definicion del enum); y no hay ningun sprite ni
  funcion de "auto-aim target" en el codigo (`bIsAutoAiming` solo se usa en
  `Hud.cpp:305` y `:374` para **no** dibujar la reticula).
- **Cambio propuesto**:
  1. **El punto del francotirador**: **ya esta** (con `FIX_BUGS` activo). La
     linea 592 es la rama muerta. La propuesta es **no tocar nada** (o, si se
     quiere que el fix sea explicito y no dependa de `FIX_BUGS`, cambiar la linea
     592 a `rwBLENDINVSRCALPHA` y borrar el `#ifdef`/`#else`: 2 lineas).
     Decision: **dejarlo como esta** y anotarlo en el `ATTRIBUTION.md` como
     `YA ESTABA (por FIX_BUGS)`.
  2. **El sifon de auto-aim**: **NO APLICA** en el port: no hay el dibujo. No hay
     nada que portar. Se anota en el `ATTRIBUTION.md` como `NO APLICA (el port no
     dibuja el sifon de auto-aim)`.
  3. `src/renderer/Hud.cpp` (**fuera**): si el jugador quiere el fix explicito,
     el cambio es en `:588-593`:
     ```c
     						RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
     						RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
     ```
     (quitando el `#ifdef FIX_BUGS`).
- **Bandera/clave que necesita**: ninguna (feature **fija** del mod: el INI lo dice
  en `skygfx.ini:5-7`: *"Changes that cannot be toggled: Alpha blending for
  Auto-aim syphon and sniper dot in VC is fixed"*). Si se decide explicitar el
  punto del francotirador, `VICEEXT_SKY_BLEND_FIX` (pero **solo** si el jugador
  quiere dejar de depender de `FIX_BUGS`).
- **Dependencia**: ninguna.
- **Riesgo**: nulo (o casi): el fix del punto ya esta; el del sifon no aplica.
- **-> verify**: el punto del francotirador se ve con el alpha correcto (ya se ve
  hoy); la fila del `ATTRIBUTION.md` dice `YA ESTABA` / `NO APLICA` con el motivo
  medido.

---

# Cotejo final de shaders

## Cotejo 1 - Que hay hoy en `src/extras/shaders/`

**15 GLSL** (fuente) + 15 `.inc` generados en `obj/` (**CRLF**, salvo
`grading.frag` = **LF**) + **12 HLSL** (solo build D3D9 nativo, **no** se compila
en la web: `custompipes_d3d9.cpp` esta entero bajo `#ifdef RW_D3D9`, linea 3) con
12 `.cso` + 12 `.inc`.

| GLSL del port | `.inc` | Quien lo usa | Equivalente en SkyGfx 3.0b |
|---|---|---|---|
| `neoVehicle.vert` | `obj/neoVehicle_vert.inc` | pipe de coche neo | `vehiclePass1VS` + `vehiclePass2VS` (fundidos en un solo draw) |
| `neoVehicle.frag` | `obj/neoVehicle_frag.inc` | pipe de coche neo | (mezcla de los 2 pases) |
| `default_UV2.vert` | `obj/default_UV2_vert.inc` | pipe de mundo neo | `ps2StandardVS` / `pcStandardVS` (el caso PS2) |
| `neoWorldVC.frag` | `obj/neoWorldVC_frag.inc` | pipe de mundo neo | `vc_worldPS` |
| `neoGloss.vert` | `obj/neoGloss_vert.inc` | pipe de gloss | `glossVS` |
| `neoGloss.frag` | `obj/neoGloss_frag.inc` | pipe de gloss | `glossPS` |
| `neoRim.vert` | `obj/neoRim_vert.inc` | pipe de rim (peds) | `rimVS` |
| `neoRimSkin.vert` | `obj/neoRimSkin_vert.inc` | pipe de rim (peds con piel) | `rimVS` |
| `simple.frag` | `obj/simple_frag.inc` | fragment comun del rim | (helper) |
| `screenDroplet.frag` | `obj/screenDroplet_frag.inc` | gotas de pantalla | `neoWaterdrops` (PS propio del mod: `screenDroplet_PS.hlsl`) |
| `im2d_UV2.vert` | `obj/im2d_UV2_vert.inc` | gotas de pantalla (2 UVs) | - |
| `im2d.vert` | `obj/im2d_vert.inc` | post-proceso | - |
| `colourfilterVC.frag` | `obj/colourfilterVC_frag.inc` | blur del overlay | `postfx.cpp` del mod |
| `contrast.frag` | `obj/contrast_frag.inc` | trails "mobile" | `contrastPS` |
| `grading.frag` | `obj/grading_frag.inc` | pase YCbCr | `gradingPS` |

**HLSL** (12, solo D3D9): `neoVehicle_VS/_PS`, `default_UV2_VS`, `neoGloss_VS/_PS`,
`neoRim_VS`, `neoRimSkin_VS`, `neoWorldVC_PS`, `screenDroplet_PS`,
`colourfilterVC_PS`, `contrastPS`, `gradingPS`. Cabeceras: `lighting.h`,
`standardConstants.h`.

## Cotejo 2 - Que pide SkyGfx 3.0b (24 HLSL: 16 VS + 8 PS)

| Shader del mod | ¿Esta en el port? | Donde va |
|---|---|---|
| `vehiclePass1VS`, `vehiclePass2VS` | **SI** (fundidos) | `neoVehicle.vert` + `neoVehicle.frag` |
| `rimVS` | **SI** | `neoRim.vert` / `neoRimSkin.vert` |
| `glossVS`, `glossPS` | **SI** | `neoGloss.vert` / `neoGloss.frag` |
| `vc_worldPS` | **SI** | `neoWorldVC.frag` |
| `gradingPS` | **SI** | `grading.frag` |
| `contrastPS` | **SI** | `contrast.frag` |
| `ps2StandardVS` | **SI** (equivalente) | `default_UV2.vert` / `default.vert` de librw |
| `ps2StandardEnvVS` | **SI** (equivalente) | `matfx_env.vert` de librw |
| `ps2StandardEnvOnlyVS` | **SI** (equivalente) | `matfx_env.vert` de librw |
| `nolightVS`, `nolightEnvVS`, `nolightEnvOnlyVS` | **NO** (el port no tiene el camino "sin luz" separado: el `default.vert` de librw siempre mete `in_color` + ambiente) | **nunca**: el port no tiene la variante "nolight" (el mod la usa cuando la geometria no tiene `rpGEOMETRYLIGHT`) |
| `pcStandardVS`, `pcStandardEnvVS`, `pcStandardEnvOnlyVS` | **NO** | S2-3: `matfx_default_pc.vert` + la variante `#ifdef MATFX_PC_LIGHT` de `matfx_env.vert` |
| `pcEnvPS` | **NO** (el port tiene el equivalente en el mismo FS) | S2-1: `matfx_env_pc.frag` |
| `mobileEnvPS` | **NO** (idem) | S2-1: `matfx_env_mobile.frag` |
| `vcsVS` | **NO** | S3-1: `neoLeedsCar.vert` (+ `neoLeedsCar.frag`) |
| `leedsWorldVS` | **NO** | S3-2: `leedsWorld.vert` (+ `leedsWorld.frag`) |
| `worldPS` | **NO APLICA** | es la variante **de GTA III** (`neoWorldpipe.cpp:61-64`, `if(isIII())`); en VC el mod usa `vc_worldPS` |
| `ambientVS` | **NO APLICA** | **codigo muerto en el mod**: `ambientVS.h` no se incluye en ningun `.cpp` (medido: 0 referencias) |
| `curvePS` | **NO APLICA** | el mod lo tiene en un bloque `#if 0` (`main.cpp:402-512`) y no hay ninguna clave de INI para el |

**Resumen del cotejo**: de los 24 HLSL del mod, el port tiene el equivalente de
**13** (7 identicos + 6 equivalentes), **no hace falta** 3 (`worldPS` = III,
`ambientVS` = muerto, `curvePS` = desactivado) y **faltan 8**:
`pcStandardVS`, `pcStandardEnvVS`, `pcStandardEnvOnlyVS` (S2-3),
`pcEnvPS`, `mobileEnvPS` (S2-1), `vcsVS` (S3-1), `leedsWorldVS` (S3-2), mas los
3 shaders "nolight" que **no se portan** (el port no tiene esa rama).

---

# (a) Tabla resumen de items y ficheros

| # | Item | Ficheros que toca (lineas medidas) | Fuera de mi territorio (descripcion) |
|---|---|---|---|
| S1-1 | `envMapSize` | `src/extras/custompipes.cpp:50` (+ helper nuevo), `src/extras/custompipes.h:78-83` | `src/core/re3.cpp:772-778` y `:928-934` (leer/guardar la clave) |
| S1-2 | conmutador de `worldPipe` | `src/extras/custompipes.cpp:378`, `src/extras/custompipes.h:102`, `src/extras/custompipes_gl.cpp:196-199` | `src/core/re3.cpp:139/145/147/151/153/1405` (menu + debug) |
| S1-3 | `disableColourOverlay` | `src/extras/postfx.h:17-20`, `src/extras/postfx.cpp:51`, `:370`, `:466-482` | `src/core/re3.cpp` (clave + mando) |
| S1-4 | `trails*` / `currentBlur*` | `src/extras/postfx.h:17-20`, `src/extras/postfx.cpp:20-21`, `:128-162`, `:247-254`, `:294`, `:327` | `src/core/re3.cpp` (4 claves + 4 mandos) |
| S1-5 | `neoWaterDrops` 2 y 3 | `src/extras/screendroplets.h:33-34`, `src/extras/screendroplets.cpp:34`, `:368-398` (+ helper nuevo) | `src/renderer/Particle.cpp` (`CParticle::AddParticle`, 1 linea), `src/weapons/Weapon.cpp:769-773`, `src/core/re3.cpp` |
| S2-1 | `texblendSwitch` | `vendor/librw/src/gl/shaders/matfx_env_pc.frag` (nuevo), `matfx_env_mobile.frag` (nuevo), `Makefile:33-39`, `matfx_gl.inc` (regenerado), `gl3matfx.cpp:112-117/135-145/196-236`, `rwplugins.h:163-166` | `src/core/re3.cpp` (clave) |
| S2-2 | `texgenSwitch` | `vendor/librw/src/gl/gl3matfx.cpp:61-93` (matriz), `rwplugins.h:163-166` | `src/core/re3.cpp` (clave) |
| S2-3 | `ps2light` | `vendor/librw/src/gl/shaders/matfx_default.vert` (nuevo), `matfx_default_pc.vert` (nuevo), `matfx_env.vert:23-28`, `gl3matfx.cpp:35-59/135-145/196-236`, `rwplugins.h:163-166` | `src/core/re3.cpp` (clave) |
| S2-4 | parche de librw | `vendor-librw-local-changes.patch` (5 hunks de fichero existente + 3 de fichero nuevo) | (lo escribe el subagente de librw) |
| S3-1 | `carPipe=2` | `src/extras/custompipes.h:85-100`, `src/extras/custompipes.cpp:126-127/350-357/472-506`, `src/extras/custompipes_gl.cpp:38-180/566-580`, `src/extras/shaders/neoLeedsCar.{vert,frag}` (nuevos) | `src/core/re3.cpp` (menu de 3 valores + `carPipe` + `leedsEnvMult`) |
| S3-2 | `worldPipe=2` | `src/extras/custompipes.h:102-109`, `src/extras/custompipes.cpp:378-380`, `src/extras/custompipes_gl.cpp:184-283`, `src/extras/shaders/leedsWorld.{vert,frag}` (nuevos) | `src/core/re3.cpp` (`leedsWorldPrelightTweak*`) |
| S4-1 | `seamFix` | `src/extras/postfx.h:43-56`, `src/extras/postfx.cpp:241-345` (helper nuevo) | `src/core/main.cpp:2057-2062` (enganche), `src/core/re3.cpp` (offsets) |
| S4-2 | `ps2Water` | (nada en mi territorio) | `src/renderer/WaterLevel.cpp:2366-2374` (UVs), `src/core/re3.cpp` (clave) |
| S4-3 | `dualPass` | `src/extras/custompipes_gl.cpp:127/240/338/453/495` (+ helper nuevo) | `vendor/librw/src/gl/gl3matfx.cpp` (para cubrir **toda** la geometria transparente) |
| S4-4 | blend sifon/punto | (nada en mi territorio) | `src/renderer/Hud.cpp:586-593` (ya esta con `FIX_BUGS`); el sifon no existe en el port |
| C | cotejo | `src/extras/shaders/` (15 GLSL + 12 HLSL) | - |

# (b) Lista de shaders nuevos

| # | Nombre | Tipo | Donde va | Como se compila | Item |
|---|---|---|---|---|---|
| 1 | `neoLeedsCar.vert` | VS | `src/extras/shaders/` | `sh/make_glsl.sh` (o `sh/makeinc_glsl.sh neoLeedsCar.vert`) -> `obj/neoLeedsCar_vert.inc` (`const char *neoLeedsCar_vert_src`) | S3-1 |
| 2 | `neoLeedsCar.frag` | PS | `src/extras/shaders/` | idem -> `obj/neoLeedsCar_frag.inc` | S3-1 |
| 3 | `leedsWorld.vert` | VS | `src/extras/shaders/` | idem -> `obj/leedsWorld_vert.inc` | S3-2 |
| 4 | `leedsWorld.frag` | PS | `src/extras/shaders/` | idem -> `obj/leedsWorld_frag.inc` | S3-2 |
| 5 | `matfx_env_pc.frag` | PS | `vendor/librw/src/gl/shaders/` | `Makefile` de librw (regla `matfx_gl.inc`, hay que ampliarla) -> `matfx_gl.inc` (`const char *matfx_env_pc_frag_src`) | S2-1 |
| 6 | `matfx_env_mobile.frag` | PS | `vendor/librw/src/gl/shaders/` | idem -> `matfx_gl.inc` | S2-1 |
| 7 | `matfx_default.vert` | VS | `vendor/librw/src/gl/shaders/` | `Makefile` (regla nueva `matfx_default_gl.inc`) -> `matfx_default_gl.inc` | S2-3 |
| 8 | `matfx_default_pc.vert` | VS | `vendor/librw/src/gl/shaders/` | idem -> `matfx_default_gl.inc` | S2-3 |

**Total: 8 shaders nuevos** (4 en `src/extras/shaders/`, 4 en
`vendor/librw/src/gl/shaders/`), de los cuales **2 son de librw** y obligan a
actualizar `vendor-librw-local-changes.patch` (ficheros nuevos: hunk
`new file mode`).

**Shaders que se modifican (no nuevos)**: `vendor/librw/src/gl/shaders/matfx_env.vert`
(la variante `#ifdef MATFX_PC_LIGHT`), `vendor/librw/src/gl/shaders/Makefile` (la
regla `matfx_gl.inc` + una nueva), `vendor/librw/src/gl/shaders/matfx_gl.inc` (a
regenerar) y `vendor/librw/src/gl/shaders/default_vs_gl.inc` (a regenerar **solo**
si se tocara `default.vert`, que **no** se toca).

# (c) Banderas y claves que pido (las escribe otro en `config.h` / `re3.cpp`)

## Banderas `VICEEXT_*` (todas en `src/core/config.h`, al final de su bloque)

| Bandera | Que habilita | Item |
|---|---|---|
| `VICEEXT_SKY_ENVMAP_SIZE` | `CustomPipesValues/envMapSize` configurable | S1-1 |
| `VICEEXT_SKY_WORLD_PIPE` | conmutador `worldPipe` (0/1/2) + menu "World pipe" | S1-2 |
| `VICEEXT_SKY_DISABLE_COLOUR_OVERLAY` | `CustomPipesValues/disableColourOverlay` | S1-3 |
| `VICEEXT_SKY_TRAILS_PARAMS` | `trailsBlur`, `trailsMotionBlur`, `currentBlurAlpha`, `currentBlurOffset` | S1-4 |
| `VICEEXT_SKY_WATERDROPS_EXTENDED` | modos 2 y 3 de `neoWaterDrops` | S1-5 |
| `VICEEXT_SKY_MATFX_BLEND` | `texblendSwitch` (0/1/2) | S2-1 |
| `VICEEXT_SKY_MATFX_TEXGEN` | `texgenSwitch` (0/1/2) | S2-2 |
| `VICEEXT_SKY_MATFX_PS2LIGHT` | `ps2light` (0/1) | S2-3 |
| `VICEEXT_SKY_LEEDS_CAR_PIPE` | `carPipe=2` + `leedsEnvMult` | S3-1 |
| `VICEEXT_SKY_LEEDS_WORLD_PIPE` | `worldPipe=2` + `leedsWorldPrelightTweak*` | S3-2 |
| `VICEEXT_SKY_SEAM_FIX` | `seamFix` + `seamOffsetX/Y` | S4-1 |
| `VICEEXT_SKY_PS2_WATER` | `ps2Water` (UVs del agua) | S4-2 |
| `VICEEXT_SKY_DUAL_PASS` | `dualPass` | S4-3 |
| `VICEEXT_SKY_BLEND_FIX` | (opcional) el fix explicito del punto del francotirador | S4-4 |

## Claves de INI (todas en la seccion `CustomPipesValues` del `reVC.ini`)

| Clave | Tipo | Default | Item | Equivalente del mod |
|---|---|---|---|---|
| `envMapSize` | int | 128 (potencia de 2) | S1-1 | `envMapSize` (`skygfx.ini:41`) |
| `worldPipe` | int | 0 | S1-2 | `worldPipe` (`skygfx.ini:20`) |
| `carPipe` | int | 0 | S3-1 | `carPipe` (`skygfx.ini:19`) |
| `disableColourOverlay` | int | 0 | S1-3 | `disableColourOverlay` (`skygfx.ini:18`) |
| `trailsBlur` | int | 1 | S1-4 | `trailsBlur` (exportado, `skygfx.h:221`) |
| `trailsMotionBlur` | int | 1 | S1-4 | `trailsMotionBlur` (exportado, `skygfx.h:222`) |
| `currentBlurAlpha` | float | 39.0 (el port usa 30) | S1-4 | `currentBlurAlpha` (`skygfx.h:230`) |
| `currentBlurOffset` | float | 2.1 (el port usa 2.0) | S1-4 | `currentBlurOffset` (`skygfx.h:231`) |
| `neoWaterDrops` | int | 1 | S1-5 | `neoWaterDrops` (`skygfx.ini:37`) |
| `texblendSwitch` | int | 0 (PS2) | S2-1 | `texblendSwitch` (`skygfx.ini:24`) |
| `texgenSwitch` | int | 0 (PS2) | S2-2 | `texgenSwitch` (`skygfx.ini:26`) |
| `ps2light` | int | 1 (PS2) | S2-3 | `ps2light` (`skygfx.ini:28`) |
| `leedsEnvMult` | float | 0.4 | S3-1 | `leedsEnvMult` (`skygfx.ini:66`, el codigo usa 0.4) |
| `leedsWorldPrelightTweakMult` | float | 1.0 | S3-2 | `leedsWorldPrelightTweakMult` (`skygfx.ini:67`, el codigo usa 1.0) |
| `leedsWorldPrelightTweakAdd` | float | 0.0 | S3-2 | `leedsWorldPrelightTweakAdd` (`skygfx.ini:68`, el codigo usa 0.0) |
| `seamFix` | int | 0 | S4-1 | `seamFix` (`skygfx.ini:42`) |
| `seamOffsetX` | int | 1 | S4-1 | menu "Seam Offset X" (`main.cpp:730`) |
| `seamOffsetY` | int | 1 | S4-1 | menu "Seam Offset Y" (`main.cpp:731`) |
| `ps2Water` | int | 0 | S4-2 | `ps2Water` (`skygfx.ini:29`) |
| `dualPass` | int | 0 | S4-3 | `dualPass` (`skygfx.ini:30`) |

## Variables que el motor debe escribir en librw (las escribe otro en `re3.cpp`)

| Variable (en `vendor/librw/src/rwplugins.h`, junto a `envMapFlipU`, linea 163) | Tipo | Valores | Item |
|---|---|---|---|
| `rw::MatFX::envMapBlendStyle` | `int32` | 0 = PS2, 1 = PC, 2 = Mobile | S2-1 |
| `rw::MatFX::envMapTexgenStyle` | `int32` | 0 = PS2, 1 = PC, 2 = Mobile | S2-2 |
| `rw::MatFX::ps2Lighting` | `bool32` | 1 = PS2 (default), 0 = PC | S2-3 |

Los tres se escriben desde `src/core/re3.cpp` con el **mismo patron** que los
existentes (`re3.cpp:1388-1390`): `DebugMenuAddVarBool32("Render", "MatFX env map
apply light", &rw::MatFX::envMapApplyLight, nil);`.

## Mandos de menu que se piden (los escribe otro en `re3.cpp`)

| Menu | Tipo | Valores | Item |
|---|---|---|---|
| "World pipe" | `FrontendOptionAddSelect` | default / neo / leeds | S1-2 |
| "Vehicle pipeline" (el de hoy, `re3.cpp:139`) | `FrontendOptionAddSelect` | **3** valores: MFX / Neo / Leeds | S3-1 |
| "Disable colour overlay" | on/off | - | S1-3 |
| "Trails blur" / "Trails motion blur" | on/off | - | S1-4 |
| "Blur alpha" / "Blur offset" | float | 0..255 / 0..16 | S1-4 |
| "Neo water drops" | select | None / Neo / Extended / Extended+Blood | S1-5 |
| "MatFX blend" | select | PS2 / PC / Mobile | S2-1 |
| "MatFX texgen" | select | PS2 / PC / Mobile | S2-2 |
| "MatFX PS2 lighting" | select | PC / PS2 | S2-3 |
| "Leeds car env mult" | float | 0.0..2.0 | S3-1 |
| "Leeds prelight tweak mult" / "add" | float | 0.0..1.0 | S3-2 |
| "Seam fixer" / "Seam offset X" / "Seam offset Y" | on/off, int | -10..10 | S4-1 |
| "PS2 water" | on/off | - | S4-2 |
| "Dual pass" | on/off | - | S4-3 |

# (d) Cotejo final de shaders: que falta y que no

**Lo que NO falta (ya esta en el port, con el nombre equivalente)**:
`vehiclePass1VS`/`vehiclePass2VS` -> `neoVehicle.vert`+`neoVehicle.frag`;
`rimVS` -> `neoRim.vert`/`neoRimSkin.vert`; `glossVS`/`glossPS` ->
`neoGloss.vert`/`neoGloss.frag`; `vc_worldPS` -> `neoWorldVC.frag`;
`gradingPS` -> `grading.frag`; `contrastPS` -> `contrast.frag`;
`ps2StandardVS`/`ps2StandardEnvVS`/`ps2StandardEnvOnlyVS` -> `default_UV2.vert`
(port) y `default.vert`/`matfx_env.vert` (librw).

**Lo que falta y hay que escribir (8 ficheros)**:
`matfx_env_pc.frag`, `matfx_env_mobile.frag` (S2-1);
`matfx_default.vert`, `matfx_default_pc.vert` (S2-3);
`neoLeedsCar.vert`, `neoLeedsCar.frag` (S3-1);
`leedsWorld.vert`, `leedsWorld.frag` (S3-2).

**Lo que falta pero NO se escribe (y por que)**:
- `pcStandardVS`, `pcStandardEnvVS`, `pcStandardEnvOnlyVS`: el port no usa
  "registros de shader" por separado, asi que el caso PC se resuelve con **un
  `#ifdef` en el VS de MatFX** y **dos** ficheros de VS (S2-3), no con tres.
- `nolightVS`, `nolightEnvVS`, `nolightEnvOnlyVS`: el port **no tiene** la rama
  "geometria sin luz" separada (el `default.vert` de librw siempre suma
  `in_color` + ambiente + luces dinamicas). Portar la rama "nolight" exigiria un
  VS mas por cada variante; **no se pide** (el mod la usa solo cuando la
  geometria no tiene `rpGEOMETRYLIGHT`, y en el port esa geometria la dibuja el
  shader por defecto con el prelight).
- `worldPS`: es la variante **de GTA III** (`neoWorldpipe.cpp:61-64`); en VC el mod
  usa `vc_worldPS`, que ya esta.
- `ambientVS`: **codigo muerto en el mod** (0 referencias en `src/`), no hace falta.
- `curvePS`: bloque `#if 0` en el mod (`main.cpp:402-512`), sin clave de INI: no
  hace falta.

**Los HLSL del port (12)**: son el build **D3D9 nativo**, que **no compila** en el
port (`custompipes_d3d9.cpp:3`, `#ifdef RW_D3D9`). Los shaders nuevos se escriben
**solo en GLSL** (regla 4 del plan): los `.cso` de los shaders nuevos **no** hacen
falta y **no** se deben generar.

---

## Nota final de coordinacion

- `src/core/re3.cpp` lo escribe **otro subagente**: de este documento sale la
  lista exacta de variables, claves y mandos que necesita (seccion (c)). **No**
  se ha tocado ese fichero.
- `src/core/main.cpp` (S4-1) y `src/renderer/WaterLevel.cpp` (S4-2),
  `src/renderer/Hud.cpp` (S4-4), `src/renderer/Particle.cpp` y
  `src/weapons/Weapon.cpp` (S1-5) **no** son mio: se describe **que** hay que
  anadir y **donde**, con la linea medida.
- `vendor/librw/**` lo escribe **otro subagente**: aqui queda el shader exacto,
  el uniforme, el selector y el formato del parche.
- **No se ha modificado ningun fichero de `src/`, `docs/` ni `.agents/`.** La unica
  escritura de este subagente es este documento.
