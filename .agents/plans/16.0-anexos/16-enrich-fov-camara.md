# 16.0 · Enriquecimiento W1 (FOV y cámara) — WFP master

> Subagente de **ENRIQUECIMIENTO**. **No se implementa nada**: este documento es la
> *propuesta escrita* de la tanda W1 (FOV) + la parte de FOV de W4.
> Territorio: `src/core/Cam.cpp`, `src/core/Camera.cpp`, `src/renderer/Draw.cpp`
> (lectura y propuesta). Ningún fichero de `src/` ha sido tocado.
>
> Fuentes leídas: plan `.agents/plans/2026-10-20-16.0-widescreen-skygfx.md`,
> `.agents/RULES.md`, `.agents/CODING_STANDARDS.md`, `.agents/DESIGN.md`,
> recibo RE del master `gta_vc_browser/tmp/rea/wfp-16-master-rea.md` (§1 y §3),
> recibo RE de la build instalada `gta_vc_browser/tmp/rea/wfp-16-rea.md` (§25),
> y el fuente del mod `gta_vc_browser/tmp/extsrc/wfp/WidescreenFixesPack-master/source/GTAVC.WidescreenFix/`
> (`Misc.ixx`, `Draw.ixx`, `Main.ixx`, `Legacy.ixx`, `Sprite2d.ixx`, `common.h`).

## 0 · Resumen de la tanda

| # | Item | Fichero principal | Bloque medido | Bandera propuesta |
|---|---|---|---|---|
| 1 | **C15** FOV por velocidad en la cámara VC nativa | `src/core/Cam.cpp` | 2122-2137 | `VICEEXT_WFP_CAR_SPEED_FOV` |
| 2 | **C16** API de multiplicadores de FOV | `src/renderer/Draw.cpp` | 8-16, 96-106 | `VICEEXT_WFP_FOV_MULTIPLIERS` |
| 3 | FOV de vehículos de emergencia | `src/core/Cam.cpp` | 5123, 5333 | `VICEEXT_WFP_EMERGENCY_FOV` |
| 4 | Salto de la colisión de cámara con AR ≥ 2 | `src/core/Camera.cpp` | 2158-2160 | `VICEEXT_WFP_SKIP_CAMERA_COLLISION` |
| 5 | **C5** FOV inverso en `AvoidTheGeometry` | `src/core/Camera.cpp` | 2207-2208 | `VICEEXT_WFP_INVERSE_FOV_COLLISION` |
| 6 | `DontTouchFOV` | `src/renderer/Draw.cpp` | 96-106 | `VICEEXT_WFP_DONT_TOUCH_FOV` |
| 7 | **C4** FOV de cinemática ligado al zoom del borde | `src/renderer/Draw.cpp` | 96-106 (+ 78-94) | `VICEEXT_WFP_CUTSCENE_FOV` |
| 8 | **C30** FOV de la cámara de rueda | `src/core/Cam.cpp` | 3974-3977 | `VICEEXT_WFP_WHEELCAM_FOV` |

**Fin de línea de todos los ficheros de este documento** (medido byte a byte, 20/10/2026):
`src/core/Cam.cpp` **CRLF** (6748 CRLF / 0 LF) · `src/core/Camera.cpp` **CRLF** (4373/0) ·
`src/renderer/Draw.cpp` **CRLF** (112/0) · `src/renderer/Draw.h` **CRLF** (70/0) ·
`src/core/Camera.h` **CRLF** (674/0) · `src/core/config.h` **CRLF** (837/0).
Todo fragmento de código de este documento va en **tabuladores**, **sin comentarios**
(regla del repo) y con el estilo de fin de línea del fichero donde se aplica.

**Nota de arquitectura (ADR-001 / ADR-003)**: se porta el *comportamiento* del master, no
sus parches. Ninguna de las propuestas toca direcciones x86, ni patrones, ni el retail.

---

### C15 FOV dependiente de la velocidad en la cámara VC nativa

- **Fichero**: `src/core/Cam.cpp`
- **Bloque**: `CCam::Process_Cam_On_A_String`, líneas **2122-2137** (cabeza de la función;
  el `FOV = DefaultFOV` está en la línea **2131** y el bloque `ResetStatics` en **2133-2137**).
  La función completa llega hasta la línea 2208 (`TheCamera.AvoidTheGeometry(...)`).
- **Qué hace hoy**:
  - `src/core/Cam.cpp:2131` → `FOV = DefaultFOV;` **sin condición**: el FOV de la cámara
    nativa de VC se resets a 70° **cada frame**, no hay término de velocidad.
  - `src/core/Cam.cpp:2133-2137` → el bloque `if(ResetStatics){...}` sólo limpia
    `AlphaSpeed`, `m_fTilt` y `m_fTiltSpeed`.
  - El master (`Misc.ixx:60-89`) hookea **exactamente** esta función: neutraliza la
    instrucción del retail que escribía `cam->FOV = 70.0f` (`Misc.ixx:62-63`, patrón
    `C7 83 ? ? ? ? ? ? ? ? 80 7B`, retail `0x47FF22`, medido en el recibo §4.5) y la
    sustituye por la fórmula completa.
  - El **port ya tiene esa misma fórmula** en la cámara SA (`Process_FollowCar_SA`,
    `src/core/Cam.cpp:6046-6070`: mismo umbral `0.4f`, misma relajación
    `Pow(0.98f, CTimer::GetTimeStep())`, mismo tope `DefaultFOV + 30.0f`). Se copia
  **tal cual**, sin tocar la SA.
- **Cambio propuesto**: sustituir las líneas 2128-2137 por el bloque siguiente
  (la rama `#else` es el comportamiento actual, byte a byte). Con la bandera apagada
  el juego queda exactamente igual.

```cpp
	// unused
	// ((CVehicle*)CamTargetEntity)->GetVehicleAppearance();

#ifdef VICEEXT_WFP_CAR_SPEED_FOV
	if(ResetStatics){
		FOV = DefaultFOV;
		AlphaSpeed = 0.0f;
		m_fTilt = 0.0f;
		m_fTiltSpeed = 0.0;
	}else{
		float forwardSpeed = DotProduct(((CVehicle*)CamTargetEntity)->GetForward(), ((CVehicle*)CamTargetEntity)->m_vecMoveSpeed);
		if(forwardSpeed > 0.4f)
			FOV += (forwardSpeed - 0.4f) * CTimer::GetTimeStep();
		if(FOV > DefaultFOV)
			FOV = Pow(0.98f, CTimer::GetTimeStep()) * (FOV - DefaultFOV) + DefaultFOV;
		FOV = Clamp(FOV, DefaultFOV, DefaultFOV + 30.0f);
	}
#else
	FOV = DefaultFOV;

	if(ResetStatics){
		AlphaSpeed = 0.0f;
		m_fTilt = 0.0f;
		m_fTiltSpeed = 0.0;
	}
#endif
```

  Notas de implementation:
  - `m_vecMoveSpeed` es `CPhysical::m_vecMoveSpeed` (`src/entities/Physical.h`), en
    unidades de juego (m/s ÷ 50). **El mismo campo y el mismo umbral que usa la cámara
    SA** (`Cam.cpp:6051-6052`), así que el comportamiento es idéntico en las dos cámaras.
  - `CamTargetEntity` ya está comprobado como vehículo en `Cam.cpp:2125-2126`
    (`if(!CamTargetEntity->IsVehicle()) return;`), por eso el `((CVehicle*)...)` es seguro.
  - **No se duplica el término en la SA**: `Process_Cam_On_A_String` y
    `Process_FollowCar_SA` son ramas excluyentes del mismo `switch(Mode)`
    (`src/core/Cam.cpp:298-305`: `MODE_CAM_ON_A_STRING` → `Process_FollowCar_SA` sólo con
    el cheat `FREE_CAM`, si no `Process_Cam_On_A_String`). El término de velocidad se
    aplica **una sola vez** en cualquier momento.
  - El master no restringe a coche/moto (usa `CVehicle*` tal cual); la cámara SA del port
    sí restringe a `isCar || isBike` (`Cam.cpp:6049`). Decisión pendiente del jugador:
    aquí se copia el master (cualquier vehículo). Si el jugador lo prefiere, se añade
    `if(vehicle is car or bike)` alrededor del bloque de velocidad.
  - Efecto lateral medido: el FOV entra en `TheCamera.AvoidTheGeometry(..., FOV)`
    (`Cam.cpp:2178-2180`), o sea que un FOV más ancho ensancha **la esfera de colisión**
    de la cámara (igual que ya pasa en la cámara SA). Es el mismo efecto que en el mod.
- **Bandera/clave que necesita**: `VICEEXT_WFP_CAR_SPEED_FOV` (binaria).
  Clave del mod: **`CarSpeedDependantFOV`** (`Misc.ixx:32`, leída de la sección `[MISC]`
  del INI del WFP, `0|1`, default 1 en el master).
  En el port: sección `[WidescreenFix]` del `reVC.ini` (la fija T1; si T1 replica las
  tres secciones del mod, esta es `[MISC]`). Lectura propuesta en `src/core/re3.cpp`
  (bloque `ReadIniIfExists`, patrón de `re3.cpp:725`):
  `ReadIniIfExists("WidescreenFix", "CarSpeedDependantFOV", &bWfpCarSpeedDependantFOV);`
  (el `#define` es el valor de reserva: con la bandera apagada, el juego va como ahora).
- **Dependencia**: T1 (contrato de INI `[WidescreenFix]` + interruptor del menú) y
  `src/core/config.h` (bandera). El bloque de la SA (`Cam.cpp:6046-6070`) **no se toca**.
- **Riesgo**: medio. Cambia el FOV de la cámara de vehículo (la de "cámara sobre un
  cable", la de siempre conduciendo) → se nota. Interactúa con la colisión de cámara
  (esfera más ancha) y con el `m_fFovSpeedOverOneFrame` de la interpolación
  (`Cam.cpp:966`, `Camera.cpp:2714`). Con la bandera apagada, cambio cero.
- **-> verify**: con `VICEEXT_WFP_CAR_SPEED_FOV` apagada, el log del jugador y el
  behaviour son idénticos a antes; encendida, el FOV de `Cams[ActiveCam].FOV` crece al
  acelerar y decae al soltar (tope 100°) — comprobable con una traza `FOV` por frame
  (o leyendo `CCam::FOV` en el log del jugador) y con la cámara SA **sin** cambio
  (el mismo `CCam::FOV` no debe oscilar en la cámara SA).

---

### C16 API de multiplicadores de FOV

- **Fichero**: `src/renderer/Draw.cpp` (+ `src/renderer/Draw.h` para las declaraciones)
- **Bloque**: `src/renderer/Draw.cpp` **8-16** (los `static` de `CDraw`) y **96-106**
  (`CDraw::SetFOV`). Declaraciones: `src/renderer/Draw.h` **53-66**.
- **Qué hace hoy**:
  - `src/renderer/Draw.cpp:96-106` → `CDraw::SetFOV(float fov)` **no tiene ningún
    multiplicador**: o convierte por aspecto (`ConvertFOV(fov)`, línea 101) o, en
    cinemática, deja el FOV tal cual (línea 103). `ms_fFOV = fov` (línea 105).
  - El master tiene **dos** capas: la nueva (`Draw.ixx:259-294`, `FOVManager`) y la
    legacy (`Legacy.ixx:211-214` + `:217-236`, `FOVMods` como `std::map<void*,float>`).
    Ambas se aplican en `SetFOV`: la nueva multiplica el FOV **escalado**
    (`Draw.ixx:301`: `ms_fScaledFOV = ConvertFOV(fov) * FOVManager::GetCurrentMultiplier();`)
    y la legacy multiplica el FOV **de pantalla** (`Legacy.ixx:211-214`).
    Semántica medida del master: producto de todos los multiplicadores, valor
    **acotado a [0.5, 2.0]** (`Draw.ixx:285`) y `<= 0` → 1.0 (`Legacy.ixx:224-225`).
    `GetCurrentFOV` devuelve el FOV **sin** conversión de aspecto × el multiplicador
    (`Draw.ixx:275-279`).
- **Cambio propuesto**: en `src/renderer/Draw.cpp`, registro de multiplicadores con
  tabla fija (el repo no usa `std::map`/`<unordered_map>` en `src/`: medido, 0 usos), y
  aplicación del producto en `SetFOV`. Código propuesto (a insertar entre las líneas 94
  y 96 de `Draw.cpp`, y el cambio de la línea 101):

```cpp
static const int FOV_MULTIPLIER_SLOTS = 8;

struct FOVMultiplierSlot
{
	uint32 key;
	float value;
};

static FOVMultiplierSlot ms_odFOVMultipliers[FOV_MULTIPLIER_SLOTS];
static int ms_odFOVMultiplierCount = 0;

static float GetFOVMultiplier()
{
	float total = 1.0f;
	for(int i = 0; i < ms_odFOVMultiplierCount; i++)
		total *= ms_odFOVMultipliers[i].value;
	return total;
}

void
CDraw::SetFOVMultiplier(uint32 key, float value)
{
	if(value <= 0.0f)
		value = 1.0f;
	if(value < 0.5f)
		value = 0.5f;
	if(value > 2.0f)
		value = 2.0f;
	for(int i = 0; i < ms_odFOVMultiplierCount; i++){
		if(ms_odFOVMultipliers[i].key == key){
			ms_odFOVMultipliers[i].value = value;
			return;
		}
	}
	if(ms_odFOVMultiplierCount >= FOV_MULTIPLIER_SLOTS)
		return;
	ms_odFOVMultipliers[ms_odFOVMultiplierCount].key = key;
	ms_odFOVMultipliers[ms_odFOVMultiplierCount].value = value;
	ms_odFOVMultiplierCount++;
}

void
CDraw::RemoveFOVMultiplier(uint32 key)
{
	for(int i = 0; i < ms_odFOVMultiplierCount; i++){
		if(ms_odFOVMultipliers[i].key == key){
			ms_odFOVMultipliers[i] = ms_odFOVMultipliers[ms_odFOVMultiplierCount - 1];
			ms_odFOVMultiplierCount--;
			return;
		}
	}
}

float
CDraw::GetCurrentFOV()
{
	return CDraw::GetFOV() * GetFOVMultiplier();
}
```

  Y la línea 101 de `SetFOV` pasa a ser:

```cpp
		ms_fScaledFOV = ConvertFOV(fov) * GetFOVMultiplier();
```

  Declaraciones a añadir en `src/renderer/Draw.h` (junto a `SetFOV`, líneas 53-56):

```cpp
	static void SetFOVMultiplier(uint32 key, float value);
	static void RemoveFOVMultiplier(uint32 key);
	static float GetCurrentFOV(void);
```

  Diferencias asumidas frente al master (y por qué): el master indexa por `void*`
  (un puntero-hash de otro mod); en el port la clave es un `uint32` (hash o id numérico)
  porque no hay DLL que la llame. El producto, el acotado [0.5, 2.0] y el `GetCurrentFOV`
  son los del master. El master aplica el multiplicador **sólo** en la rama de juego
  (no en cinemática) — el port hace lo mismo.
- **Bandera/clave que necesita**: `VICEEXT_WFP_FOV_MULTIPLIERS`. **Sin clave de INI**
  (el master no la tiene: es una API aditiva, feature #25 del master, "SIN CONFLICTO").
  La bandera existe para que la API **no exista** cuando el jugador no la quiera.
- **Dependencia**: `src/renderer/Draw.h` (declaraciones) — **fuera del territorio de este
  subagente**, hay que pedirlo explícitamente. T1 (si la opción va al menú).
  Convive con C4 y `DontTouchFOV` en **la misma función** (`SetFOV`): los tres items se
  aplican juntos en W1 (el recibo lo avisa: C16 "toca el mismo `SetFOV` que C4").
- **Riesgo**: bajo. Con la bandera apagada, `GetFOVMultiplier()` devuelve 1.0 y el
  resultado es idéntico. Con la bandera encendida y sin ningún multiplicador
  registrado, también (producto vacío = 1.0).
- **-> verify**: con la bandera encendida y sin multiplicadores, `CDraw::GetScaledFOV()`
  es idéntico al valor sin bandera; tras `SetFOVMultiplier(k, 1.5)` el FOV escalado
  crezca un 50 % y tras `RemoveFOVMultiplier(k)` vuelva al valor original.

---

### FOV de vehículos de emergencia

- **Fichero**: `src/core/Cam.cpp` (destino principal) y `src/core/Camera.cpp` (destino
  alternativo, ver "Riesgo")
- **Bloque**:
  - `src/core/Cam.cpp:5117-5124` → `CCam::ProcessArrestCamOne`, con
    **`src/core/Cam.cpp:5123`: `FOV = 45.0f;`** (FOV fijo).
  - `src/core/Cam.cpp:5311-5334` → `CCam::ProcessArrestCamTwo`, con
    **`src/core/Cam.cpp:5333`: `FOV = 60.0f;`** (FOV fijo).
  - `src/core/Camera.cpp:2769-2783` → `CCamera::ProcessWideScreenOn`, con
    **`src/core/Camera.cpp:2779`: `m_fFOV_Wide_Screen = 0.3f*Cams[ActiveCam].FOV;`**.
    Medido: `m_fFOV_Wide_Screen` **se escribe y no se lee nunca** en el port
    (grep de `src/`: sólo `Camera.cpp:174` (init), `:2776` y `:2779`) → **código muerto**.
- **Qué hace hoy**:
  - Las cámaras cinematográficas con FOV fijo (arresto: 45° y 60°) mandan su valor tal
    cual a `Cams[ActiveCam].FOV` → `CDraw::SetFOV` (`src/core/Camera.cpp:608`) →
    `ConvertFOV` (estirado por aspecto). En 16:9 el 45° del retail sale a ≈60° de
    hFOV. **No hay ninguna re-calculación por el factor del FOV de pantalla.**
  - El master (`Legacy.ixx:200-202`) calcula, **dentro de `CDraw::SetFOV`**,
    `fEmergencyVehiclesFix = 70.0f / fFactor;` (donde `fFactor` es el FOV que el juego
    acaba de pedir) y lo inyecta en el retail en `0x46DEB7` (`Legacy.ixx:532-538`,
    patrón `D9 9B F4 00 00 00 D9 83 F0 00 00 00`, dentro de FUN `0046C58C`), **sustituyendo
    el factor** con el que el retail multiplica su FOV fijo. Efecto neto medido en el
    recibo de la build instalada (feature 25): *"la cámara usaba un FOV fijo (45°) que se
    veía demasiado cerrado en widescreen; el mod lo recalcula a `70/factor`"* → el FOV
    final pasa a ser **70°** (el del juego) en vez de 45°.
- **Qué falta exactamente**: al port le falta **el punto de inyección del factor**. El
  port no tiene ni el factor ni un campo multiplicador: los FOV fijos (45°/60°) se
  usan tal cual. Lo que hay que aportar es el *resultado* observable del master:
  **la cámara de emergencia usa el FOV estándar del juego (70°) en vez del FOV fijo**.
- **Cambio propuesto** (en `src/core/Cam.cpp`, `ProcessArrestCamOne` y
  `ProcessArrestCamTwo`):

```cpp
#ifdef VICEEXT_WFP_EMERGENCY_FOV
	FOV = DefaultFOV;
#else
	FOV = 45.0f;
#endif
```

  y

```cpp
#ifdef VICEEXT_WFP_EMERGENCY_FOV
	FOV = DefaultFOV;
#else
	FOV = 60.0f;
#endif
```

  Equivalente exacto del master: el `70/factor` del master produce siempre 70 cuando la
  cámara pide su FOV fijo (`45 * 70/45 = 70`, `60 * 70/60 = 70`), luego
  `FOV = DefaultFOV` **es** la fórmula del master escrita en el port.
  Si la RE de 5 minutos confirmase que el `[ebx+0xF4]` del retail es
  `CCamera::m_fFOV_Wide_Screen` (la hipótesis del recibo 16-A), la alternativa sería
  cambiar `src/core/Camera.cpp:2779` por
  `m_fFOV_Wide_Screen = DefaultFOV / CDraw::GetFOV();` — **pero ese campo no se lee
  en el port**, así que el cambio sería invisible: por eso la opción preferida es la de
  las cámaras de FOV fijo.
- **Bandera/clave que necesita**: `VICEEXT_WFP_EMERGENCY_FOV`. **Sin clave de INI**
  (el master no tiene clave para esta feature: `Legacy.ixx:200-202` y `:529-538`).
- **Dependencia**: **RE previa recomendada** (abrir el retail 1.0 en Ghidra en
  `0x46DEB7` / FUN `0046C58C` y ver qué campo es `[ebx+0xF4]` y qué FOV fijo se
  multiplica): sin ella, la propuesta se apoya en la lectura del recibo de la build
  instalada (feature 25), que es coherente pero no está verificada símbolo a símbolo en
  el port. T1 (sin clave: no hace falta).
- **Riesgo**: medio y **acotado a la escena de arresto** (una cinemática del juego).
  Con la bandera encendida, el FOV pasa de 45°/60° a 70°: la cinemática se ve más
  abierta. Si el campo real del retail no fuera el de las cámaras de arresto, el
  cambio sería inocuo pero no haría nada (por eso la RE previa).
- **-> verify**: con la bandera encendida, en la cinemática de arresto el FOV de
  `Cams[ActiveCam].FOV` vale 70 en vez de 45/60 (trazable en el log del jugador);
  con la bandera apagada, 45/60 como antes.

---

### Salto de la colisión de cámara con AR ≥ 2

- **Fichero**: `src/core/Camera.cpp`
- **Bloque**: `CCamera::AvoidTheGeometry`, líneas **2158-2259**; la entrada de la
  función está en **2158-2161** (firma) y el primer dato en **2161** (`float Beta`).
- **Qué hace hoy**:
  - **El port NO tiene `CCamera::AvoidTheObstacles`** (medido: grep de `AvoidTheObstacles`
    en `src/` → 0 resultados; sólo hay `AvoidTheGeometry`). El nombre del plan/recibo
    es un desvío: la rutina que el master salta es `CCamera::AvoidTheGeometry`.
  - `src/core/Camera.cpp:2158-2259` → la función hace (a) el recorte de la cámara por
    línea de visión (`CWorld::ProcessLineOfSight`, líneas 2182-2193), (b) el ajuste del
    near clip (2199-2201) y (c) la **esfera de colisión** contra el mundo
    (`CWorld::TestSphereAgainstWorld`, línea 2211) con `m_vecClearGeometryVec` (2228).
  - El master (`Legacy.ixx:506-522`) sustituye **la llamada** a la rutina del retail
    (`0x480456`, `Legacy.ixx:529-530`) por un gancho que, si el aspecto **≥ 2.0**,
    hace `ret 10h` (no entra a la rutina) y si no, salta al original. Es decir:
    **con AR ≥ 2 la cámara no choca con la geometría**.
- **Cambio propuesto**: el equivalente en el port es **no entrar** en la función cuando
  el aspecto es ≥ 2. Se hace al principio de `CCamera::AvoidTheGeometry` (equivale al
  `ret` del master, porque la función no tiene efectos previos que conservar):

```cpp
void
CCamera::AvoidTheGeometry(const CVector &Source, const CVector &TargetPos, CVector &NewSource, float FOV)
{
#ifdef VICEEXT_WFP_SKIP_CAMERA_COLLISION
	if(CDraw::GetAspectRatio() >= 2.0f)
		return;
#endif
	float Beta = 0.0f;
	float Alpha = 0.0f;
```

  Diferencia asumida frente al master y su justificación: el master salta **una**
  llamada (la del retail `0x480456`, dentro de FUN `0048FE86`); en el port la decisión
  se aplica a **todas** las llamadas (17 sitios: `Cam.cpp:722, 758, 795, 867, 1522,
  1917, 2178, 2180, 3495, 3549, 3650, 3767, 4205, 5004, 5228, 5296, 5305`), porque el
  punto de retail no se ha mapeado a un símbolo del port. El criterio del master
  (**aspecto ≥ 2.0**) se conserva literal.
  Consecuencia medida: en 16:9 (1.777) **no** se salta nada; sólo en 21:9 (2.33) y
  ultrapanorámicas (≥ 2.0) se desactiva la colisión de cámara.
- **Bandera/clave que necesita**: `VICEEXT_WFP_SKIP_CAMERA_COLLISION`. **Sin clave de
  INI** (el master no da clave a esta feature: `Legacy.ixx:506-522`, `:529-530`).
- **Dependencia**: `src/core/Camera.cpp` ya usa `CDraw::CalculateAspectRatio()`
  (línea 2208) y `CDraw` está incluido en el fichero → **sin dependencias de cabecera**.
  Convive con C5 (el FOV inverso), que toca la misma función (línea 2207).
- **Riesgo**: medio-alto *conceptual*, bajo en la práctica: sólo afecta a pantallas con
  AR ≥ 2 (21:9 o más). Con la cámara sin colisión, la cámara puede entrar en paredes y
  edificios. En 16:9 (el caso del jugador) el cambio es **cero**.
  Ojo: el FOV se usa en esta función (`ViewPlaneHeight`, línea 2207), así que el salto
  también deja de usar el FOV (misma semántica que el master).
- **-> verify**: con la bandera encendida y el navegador en 21:9, la cámara atraviesa
  las paredes (no hay corrección de `m_vecClearGeometryVec`); con el navegador en 16:9
  el comportamiento es idéntico al actual (traza de `m_vecClearGeometryVec` = 0 en
  ambos casos si no hay colisión).

---

### C5 FOV inverso en `AvoidTheGeometry`

- **Fichero**: `src/core/Camera.cpp` (uso) + `src/renderer/Draw.cpp` (la función nueva)
- **Bloque**: `src/core/Camera.cpp:2204-2209` (dentro de `AvoidTheGeometry`, 2158-2259):

```cpp
	static float fClearGeomAmount;
	static float fClearGeomAmountSpeed;
	float Near = RwCameraGetNearClipPlane(Scene.camera);
	float ViewPlaneHeight = Tan(DEGTORAD(FOV) / 2.0f);
	float ViewPlaneWidth = ViewPlaneHeight * CDraw::CalculateAspectRatio() * fAvoidTweakFOV;
```

  El parámetro `FOV` se usa **sólo** en la línea 2207 dentro de toda la función
  (medido: 2158-2259, una sola lectura de `FOV`).
- **Qué hace hoy**: el port pasa el FOV de la cámara tal cual y calcula la esfera de
  colisión como `tan(FOV/2) · aspecto · 1.15` (`fAvoidTweakFOV = 1.15f`, línea 2155).
  El master (`Main.ixx:27-31` + `:111-113`) hookea la **función** del retail
  (`CCamera::AvoidTheGeometry`, 0x473AA4) y **convierte el FOV a la inversa antes de
  la colisión**:
  `ConvertFOVInverse(fov) = 2·atan(tan(fov/2)·(4/3)/aspecto)` (`Draw.ixx:249-257`).
- **Cambio propuesto**:
  1. **Añadir** en `src/renderer/Draw.cpp` (junto a `ConvertFOV`, líneas 78-94) la
     inversa del master, con el mismo nombre y la misma fórmula:

```cpp
float
CDraw::ConvertFOVInverse(float fov)
{
	float ar1 = DEFAULT_ASPECT_RATIO;
	float ar2 = GetAspectRatio();
	float rad = DEGTORAD(fov);
	return RADTODEG(2.0f * Atan(tan(rad * 0.5f) * (ar1 / ar2)));
}
```

  2. **Usarla** en `src/core/Camera.cpp:2207`:

```cpp
	float Near = RwCameraGetNearClipPlane(Scene.camera);
#ifdef VICEEXT_WFP_INVERSE_FOV_COLLISION
	float ViewPlaneHeight = Tan(DEGTORAD(CDraw::ConvertFOVInverse(FOV)) / 2.0f);
#else
	float ViewPlaneHeight = Tan(DEGTORAD(FOV) / 2.0f);
#endif
	float ViewPlaneWidth = ViewPlaneHeight * CDraw::CalculateAspectRatio() * fAvoidTweakFOV;
```

  Por qué así y no al revés: el master convierte el **argumento** de la función; en el
  port el FOV sólo se usa en la línea 2207, así que convertir ahí es equivalente y
  deja el resto de la función intacto (menos superficie de cambio).
  **Cifras medidas** (FOV 70°, AR 16/9, `fAvoidTweakFOV` 1.15) — el radio de la esfera
  de colisión:
  - port actual: `tan(35°)·1.7778·1.15` = **1.431** (unidades por unidad de distancia);
  - con C5: `tan(27.68°)·1.7778·1.15` = **1.074**;
  - semi-extensión horizontal realmente visible: `tan(43.03°)` = **0.934**.
  Es decir: hoy la esfera es un **53 %** más ancha que la imagen; con C5 queda un **15 %**
  más ancha. En la práctica **la cámara se acerca más a las paredes** en pantalla ancha
  (deja de "huir" de ellas).
- **Bandera/clave que necesita**: `VICEEXT_WFP_INVERSE_FOV_COLLISION`. **Sin clave de
  INI** (el master no da clave a esta feature: `Main.ixx:27-31`, `:111-113`).
- **Dependencia**: `src/renderer/Draw.h` — hace falta declarar `ConvertFOVInverse`
  (junto a `ConvertFOV`, líneas 62-64). **Fuera del territorio de este subagente**:
  hay que pedirlo. `Camera.cpp` ya incluye `Draw.h` (usa `CDraw::CalculateAspectRatio`).
  Convive con el item anterior (misma función, líneas 2158-2209).
- **Riesgo**: **alto en la sensación de cámara** (lo avisa el recibo, C5: *"es de las
  que el jugador nota"*). Cambia la distancia a la que la cámara se separa de las
  paredes en 16:9 y en cualquier aspecto ≥ 1.0. Con la bandera apagada, cero cambio.
  Nota: `CDraw::GetAspectRatio()` devuelve el valor cacheado (`ms_fAspectRatio`), que se
  refresca en `CCamera::CalculateDerivedValues` (`Camera.cpp:4261`) → **un frame de
  retraso** al cambiar de resolución (irrelevante).
- **-> verify**: con la bandera encendida, en 16:9, `m_vecClearGeometryVec` (la
  corrección que aparta la cámara de la pared) es **menor** que con la bandera apagada
  en la misma escena (mismoWall); con la bandera apagada, los valores son idénticos a
  los de hoy.

---

### `DontTouchFOV`

- **Fichero**: `src/renderer/Draw.cpp`
- **Bloque**: `src/renderer/Draw.cpp:96-106` (`CDraw::SetFOV`), con la conversión en
  **99-104** (`#ifdef ASPECT_RATIO_SCALE` … `ms_fScaledFOV = fov;`).
- **Qué hace hoy**:
  - `src/renderer/Draw.cpp:99-104` → **el port convierte siempre el FOV por aspecto**
    cuando no está en cinemática (`ms_fScaledFOV = ConvertFOV(fov);`, línea 101).
    En cinemática deja el FOV tal cual (línea 103) — eso es el `RestoreCutsceneFOV` del
    mod, que el recibo marca como **YA ESTABA** (`Draw.cpp:100-103`).
  - El master (`Legacy.ixx:204-205`, clave leída en `:331`) hace:

```cpp
    if (bDontTouchFOV)
        *pfScreenFieldOfView = fFactor;
    else if (*bIsInCutscene == true && bRestoreCutsceneFOV)
        *pfScreenFieldOfView = AdjustFOV(fFactor, *pfScreenAspectRatio, std::min(*pfScreenAspectRatio, 16.0f / 9.0f));
    else
        *pfScreenFieldOfView = AdjustFOV(fFactor, *pfScreenAspectRatio);
```

  es decir: `DontTouchFOV` **gana** a todo lo demás (incluido el FOV de cinemática) y
  deja el FOV **sin estirar**.
- **Cambio propuesto**: un interruptor estático en `CDraw` + una rama nueva al principio
  de `SetFOV`. En `src/renderer/Draw.cpp`, junto a los otros `static` (líneas 8-16):

```cpp
bool CDraw::ms_bDontTouchFOV = false;
```

  y `SetFOV` (líneas 96-106) pasa a ser:

```cpp
void
CDraw::SetFOV(float fov)
{
#ifdef ASPECT_RATIO_SCALE
	if(ms_bDontTouchFOV)
		ms_fScaledFOV = fov;
	else if(!CCutsceneMgr::IsRunning())
		ms_fScaledFOV = ConvertFOV(fov) * GetFOVMultiplier();
	else
		ms_fScaledFOV = ConvertFOVforCutscene(fov);
#endif
	ms_fFOV = fov;
}
```

  (la rama de cinemática cambia de `ms_fScaledFOV = fov;` a `ConvertFOVforCutscene(fov)`
  → es el item de **C4** de más abajo; con el zoom a 1.0 devuelve `fov`, o sea que
  **no** cambia el comportamiento actual).
  Declaración a añadir en `src/renderer/Draw.h` (junto a `ms_bFixSprites`, líneas 44-46):

```cpp
	static bool ms_bDontTouchFOV;
```

  y el valor se carga desde el INI en `src/core/re3.cpp` (T1) con el patrón de
  `re3.cpp:725`:
  `ReadIniIfExists("WidescreenFix", "DontTouchFOV", &CDraw::ms_bDontTouchFOV);`
  (`ReadIniIfExists` tiene sobrecarga para `bool*`: `re3.cpp:374`).
- **Bandera/clave que necesita**: `VICEEXT_WFP_DONT_TOUCH_FOV` (binaria) +
  clave de INI **`DontTouchFOV`** (`Legacy.ixx:331`, sección **`[MAIN]`** del INI del
  WFP, `0|1`, default 0 en el master). En el port: `[WidescreenFix]` del `reVC.ini`
  (o `[MAIN]` si T1 replica las secciones del mod).
- **Dependencia**: `src/renderer/Draw.h` (el `static bool` público) — fuera del
  territorio de este subagente. T1 (lectura del INI + interruptor del menú).
  Comparte `SetFOV` con C16 y C4 → **los tres se aplican en la misma edición**.
- **Riesgo**: medio. Con la bandera encendida, **todo** el juego se ve con el FOV 4:3
  (en 16:9 se ve "estrecho", con bandas negras a los lados de la escena). Es una opción
  para el que quiera el FOV original; el jugador la_probablemente querrá apagada.
  Interacción: `DontTouchFOV` anula también el multiplicador de C16 y el zoom de
  cinemática de C4 (igual que en el master, donde `DontTouchFOV` va antes que todo).
- **-> verify**: con la bandera encendida, `CDraw::GetScaledFOV() == CDraw::GetFOV()`
  (sin conversión) en juego normal **y** en cinemática; con la bandera apagada,
  `GetScaledFOV()` sigue siendo `ConvertFOV(GetFOV())` en 16:9.

---

### C4 (parte de FOV) FOV de cinemática ligado al zoom del borde

- **Fichero**: `src/renderer/Draw.cpp`
- **Bloque**: `src/renderer/Draw.cpp:96-106` (`CDraw::SetFOV`, rama de cinemática en la
  línea **103**) y `78-94` (`CDraw::ConvertFOV`, donde se añade la función nueva).
- **Qué hace hoy**:
  - `src/renderer/Draw.cpp:102-103` → en cinemática **no se convierte el FOV**:
    `ms_fScaledFOV = fov;`. El puerto no tiene ninguna noción de "zoom de borde".
  - El master (`Draw.ixx:239-247`) convierte el FOV de cinemática con el zoom que
    escribe el sistema de bordes **cada frame** (`Sprite2d.ixx:542`:
    `g_cutsceneCameraZoom = s_cameraZoom;`):

```cpp
float CDraw::ConvertFOVforCutscene(float fov)
{
    if (g_cutsceneCameraZoom != 1.0f)
    {
        float hfov = DEGTORAD(fov);
        return RADTODEG(2.0f * Atan(tan(hfov / 2.0f) * g_cutsceneCameraZoom));
    }
    return fov;
}
```

    El zoom va en **espacio tangente** (`tan`), no en grados: multiplicar el FOV por el
    zoom en grados no es lo mismo.
- **Cambio propuesto** (mecanismo en W1; el *valor* del zoom lo pone W4):
  1. En `src/renderer/Draw.cpp`, añadir el estado y la conversión:

```cpp
float CDraw::ms_fCutsceneCameraZoom = 1.0f;

float
CDraw::ConvertFOVforCutscene(float fov)
{
	if(ms_fCutsceneCameraZoom != 1.0f){
		float rad = DEGTORAD(fov);
		return RADTODEG(2.0f * Atan(tan(rad * 0.5f) * ms_fCutsceneCameraZoom));
	}
	return fov;
}
```

  2. En `SetFOV`, la rama de cinemática usa esa conversión (ya escrito en el item
     `DontTouchFOV`):

```cpp
	else
		ms_fScaledFOV = ConvertFOVforCutscene(fov);
```

  3. Declaraciones en `src/renderer/Draw.h` (junto a `ConvertFOV`, líneas 62-64):

```cpp
	static float ConvertFOVforCutscene(float fov);
	static void SetCutsceneCameraZoom(float zoom);
```

  4. **Quién alimenta el zoom**: el sistema de bordes de cinemática. En el port ese
     sistema es `CCamera::DrawBordersForWideScreen` (`src/core/Camera.cpp:2786-2806`) +
     `CCamera::GetScreenRect` (`2273-2290`), con el borde **fijo al 30 %**
     (`m_ScreenReductionPercentage = 30.0f`, `Camera.cpp:2781`). El zoom equivalente al
     del master, con la geometría actual, sería `1/(1-0.30) = 1.4286` (espacio
     tangente); con la geometría adaptativa de **C3/W4** hay que recalcularlo a partir
     del rectángulo de contenido:
     `zoom = (alto_pantalla / alto_contenido) · (4/3) / aspecto_pantalla` **en espacio
     tangente**, es decir `zoom = tan(FOV/2_original) / tan(FOV/2_de_contenido)`.
     **W1 deja el zoom en 1.0 (sin cambio); W4/C3 lo alimenta.**
- **Bandera/clave que necesita**: `VICEEXT_WFP_CUTSCENE_FOV`. **Sin clave de INI en W1**
  (el INI del master es `NoCutsceneBorderAnimation`, `Sprite2d.ixx:507-545`, que es de
  W4; aquí sólo se prepara el mecanismo).
- **Dependencia**: `src/renderer/Draw.h` (fuera del territorio). **W4/C3** es quien
  calcula el zoom del borde y llama a `CDraw::SetCutsceneCameraZoom()` cada frame
  (el master lo hace en `Sprite2d.ixx:542`, dentro del dibujado de sprites 2D — en el
  port, el dibujado del borde está en `Camera.cpp:2786-2806`, que se llama desde el
  render 2D).
- **Riesgo**: **cero en W1** (zoom = 1.0 → `ConvertFOVforCutscene(fov) == fov`, idéntico
  a hoy). En W4 el riesgo es real: el FOV de las cinemáticas cambia (se ensancha para
  compensar el recorte de las barras) → **C4 es CONFLICTO REAL** y necesita el visto
  bueno del jugador (tabla §3.3 del recibo, prioridad 3).
- **-> verify**: con W1 y el zoom a 1.0, `GetScaledFOV() == GetFOV()` en cinemática
  (igual que hoy); con el zoom a 1.4286, `GetScaledFOV()` pasa a
  `2·atan(tan(fov/2)·1.4286)`.

---

### C30 FOV de la cámara de rueda

- **Fichero**: `src/core/Cam.cpp`
- **Bloque**: `CCam::Process_WheelCam`, líneas **3974-4053**; el FOV está en la
  **línea 3977** (`FOV = DefaultFOV;`), justo al principio de la función.
- **Qué hace hoy**:
  - `src/core/Cam.cpp:3977` → `FOV = DefaultFOV;` (70°), sin ninguna corrección por
    aspecto. Al pasar por `CDraw::SetFOV` → `ConvertFOV`, en 16:9 sale a ≈86° de hFOV
    (el 70° es un FOV **4:3**, no horizontal).
  - El master (`Main.ixx:103-109`) hookea `CCam::Process_WheelCam`: neutraliza la
    escritura del retail `cam->FOV = 70.0f` (patrón `C7 85 ? ? ? ? ? ? ? ? 8B 95 ...`,
    `Main.ixx:104-105`) y la sustituye por
    `*(float*)(regs.ebp + 0xB4) = CDraw::ConvertFOVInverse(70.0f);`
    → el FOV de la cámara de rueda pasa a ser el que, **después** de la conversión del
    render, da exactamente 70° **horizontales** en el aspecto actual.
- **Cambio propuesto** (una línea en `src/core/Cam.cpp:3977`):

```cpp
bool
CCam::Process_WheelCam(const CVector&, float, float, float)
{
#ifdef VICEEXT_WFP_WHEELCAM_FOV
	FOV = CDraw::ConvertFOVInverse(DefaultFOV);
#else
	FOV = DefaultFOV;
#endif
```

  `CDraw::ConvertFOVInverse` es la función nueva del item C5 (misma forma, mismo fichero
  `src/renderer/Draw.cpp`). `Cam.cpp` ya usa `CDraw::CalculateAspectRatio()` (líneas
  1809, 1831, 5546, 5804, 6615) → `Draw.h` ya está incluido: **sin cambios de cabecera
  en `Cam.cpp`**.
  Números medidos (FOV 70): en 16:9, `ConvertFOVInverse(70)` = **55.36°** 4:3, que tras
  `ConvertFOV` vuelve a dar 70° horizontales; en 4:3 da 70° (sin cambio). Es decir: la
  cámara de rueda deja de "abrirse" en pantallas anchas y conserva 70° horizontales.
- **Bandera/clave que necesita**: `VICEEXT_WFP_WHEELCAM_FOV`. **Sin clave de INI**
  (el master no da clave a esta feature: `Main.ixx:103-109`).
- **Dependencia**: **C5** (comparten `CDraw::ConvertFOVInverse`, que hay que añadir en
  `src/renderer/Draw.cpp` y declarar en `src/renderer/Draw.h` — fuera del territorio de
  este subagente). Si C5 no entra, C30 no puede entrar (o se implementa con la fórmula
  local, lo que se evita por duplicación).
- **Riesgo**: bajo y **muy acotado**: sólo afecta al modo `MODE_WHEELCAM`
  (`Cam.cpp:286-288`), una cámara que se usa poco (motos/carrozas). Con la bandera
  apagada, cero cambio. Con la bandera encendida, en 16:9 la cámara de rueda ve
  ~16° menos de hFOV.
- **-> verify**: con la bandera encendida, en 16:9, el `Cams[ActiveCam].FOV` de la
  cámara de rueda vale ≈55.4° (4:3) y el hFOV final sigue siendo 70°; con la bandera
  apagada, 70° (4:3) → hFOV ≈86°.

---

## Tabla resumen final

| # | Item | Fichero:bloque | Cambio | Bandera | Clave INI | Depende de |
|---|---|---|---|---|---|---|
| 1 | C15 FOV por velocidad (cámara VC) | `src/core/Cam.cpp:2122-2137` | término de velocidad del master en `Process_Cam_On_A_String` | `VICEEXT_WFP_CAR_SPEED_FOV` | `CarSpeedDependantFOV` (`[MISC]`) | T1, config.h |
| 2 | C16 API de multiplicadores | `src/renderer/Draw.cpp:8-16, 96-106` + `Draw.h` | registro de multiplicadores aplicado en `SetFOV` | `VICEEXT_WFP_FOV_MULTIPLIERS` | — | `Draw.h` |
| 3 | FOV vehículos de emergencia | `src/core/Cam.cpp:5123, 5333` | `FOV = DefaultFOV` en las cámaras de FOV fijo | `VICEEXT_WFP_EMERGENCY_FOV` | — | RE previa (0x46DEB7) |
| 4 | Salto de colisión con AR ≥ 2 | `src/core/Camera.cpp:2158-2160` | `return` temprano si aspecto ≥ 2.0 | `VICEEXT_WFP_SKIP_CAMERA_COLLISION` | — | — |
| 5 | C5 FOV inverso en la colisión | `src/core/Camera.cpp:2207` + `Draw.cpp` | `ConvertFOVInverse` en el radio de colisión | `VICEEXT_WFP_INVERSE_FOV_COLLISION` | — | `Draw.h`, item 4 |
| 6 | `DontTouchFOV` | `src/renderer/Draw.cpp:96-106` + `Draw.h` | rama que deja el FOV sin estirar | `VICEEXT_WFP_DONT_TOUCH_FOV` | `DontTouchFOV` (`[MAIN]`) | `Draw.h`, T1 |
| 7 | C4 FOV de cinemática | `src/renderer/Draw.cpp:96-106, 78-94` + `Draw.h` | `ConvertFOVforCutscene` + zoom del borde | `VICEEXT_WFP_CUTSCENE_FOV` | — | `Draw.h`, W4/C3 |
| 8 | C30 FOV de la cámara de rueda | `src/core/Cam.cpp:3974-3977` | `FOV = ConvertFOVInverse(DefaultFOV)` | `VICEEXT_WFP_WHEELCAM_FOV` | — | item 5 (`ConvertFOVInverse`) |

### Banderas a declarar en `src/core/config.h` (bloque WFP, al final del grupo de la línea 511)

```cpp
#define VICEEXT_WFP_CAR_SPEED_FOV		// FOV por velocidad en la camara VC (CarSpeedDependantFOV)
#define VICEEXT_WFP_FOV_MULTIPLIERS		// API de multiplicadores de FOV por hash (SetFOVMultiplier/RemoveFOVMultiplier/GetCurrentFOV)
#define VICEEXT_WFP_EMERGENCY_FOV		// FOV de las camaras cinematicas de emergencia: el estandar, no uno fijo
#define VICEEXT_WFP_SKIP_CAMERA_COLLISION	// con aspecto >= 2 la camara no choca con la geometria
#define VICEEXT_WFP_INVERSE_FOV_COLLISION	// el FOV de la colision de camara se convierte a la inversa (4:3) antes de usarse
#define VICEEXT_WFP_DONT_TOUCH_FOV		// DontTouchFOV: el FOV no se estira por aspecto
#define VICEEXT_WFP_CUTSCENE_FOV		// el FOV de cinematica sigue el zoom del borde
#define VICEEXT_WFP_WHEELCAM_FOV		// el FOV de la camara de rueda se mantiene en 70 grados horizontales
```

### Requisitos de cabecera fuera del territorio de este subagente

| Cabecera | Qué hace falta |
|---|---|
| `src/renderer/Draw.h` | declarar `ConvertFOVInverse`, `ConvertFOVforCutscene`, `SetCutsceneCameraZoom`, `SetFOVMultiplier`, `RemoveFOVMultiplier`, `GetCurrentFOV` y el `static bool ms_bDontTouchFOV` |
| `src/core/config.h` | las 8 banderas `VICEEXT_WFP_*` de arriba |
| `src/core/re3.cpp` | las lecturas de INI de T1 (`CarSpeedDependantFOV`, `DontTouchFOV`) |

### Decisiones que deja el jugador (no las decide el subagente)

1. **C15**: ¿el término de velocidad se aplica a **cualquier** vehículo (master) o sólo
   a coches y motos (como hace la cámara SA del port)? La propuesta copia el master.
2. **C3/C4 y C5**: son `CONFLICTO REAL` (tabla §3.3 del recibo, prioridades 3 y 4):
   cambian lo que el jugador ve y nota. Necesitan visto bueno explícito antes de W1/W4.
3. **Item 3 (emergencia)**: la RE de `0x46DEB7` decide si el cambio Proposed (cámaras de
   arresto) o el alternativo (`m_fFOV_Wide_Screen`, `Camera.cpp:2779`) es el fiel.
4. **Item 4**: el master salta **una** llamada a la colisión; la propuesta las salta
   **todas** (el punto de retail no está mapeado a un símbolo del port).
