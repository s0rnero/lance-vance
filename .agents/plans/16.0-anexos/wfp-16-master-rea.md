# WFP master (GitHub) — Recibo de ingeniería inversa (subagente 16-B)

> Mod: **WidescreenFixesPack for GTA VC** (ThirteenAG).
> Objetivo: (1) fórmula **medida** de `CarSpeedDependantFOV` en la build instalada de 2018,
> (2) inventario completo de las features del **master actual de GitHub** que la build instalada
> **no** tiene, (3) tabla de conflictos contra lo ya portado en reVC, (4) verificación del master
> contra el **retail 1.0** (`gta-vc.exe`).
> **No se implementa nada en el motor.** Este documento es el único fichero escrito.
>
> Regla de oro: **todo lo que va marcado como medido está medido** (bytes sobre el fichero,
> descompilación con Ghidra 12.1.4 headless sobre el proyecto `ghidra_vanilla` del retail y sobre
> el propio ASI). Lo que no se ha podido medir va marcado `supuesto` **con el motivo**.
>
> Precedente: `gta_vc_browser/tmp/rea/wfp-16-rea.md` (recibo de la build instalada, 65 KB).
> Este documento **completa** ese recibo: cierra su riesgo **R7** (`CarSpeedDependantFOV` sin
> fórmula) y añade el inventario del master.

---

## 0 · Veredicto en una línea

La build instalada de 2018 y el master **no son dos versiones del mismo mod**: son dos
implementaciones incompatibles de la misma idea. La build instalada es el camino `Legacy.ixx`
(96 de sus 106 patrones coinciden exactamente con el master); el master es una **reescritura
modular** que añade **228 patrones** repartidos en **22 módulos nuevos**. La feature
`CarSpeedDependantFOV` es el ejemplo perfecto del divergente: **la build instalada la resuelve
enganchando el zoom del radar** y la usa como proxy de la velocidad, mientras que **el master la
reescribe con un hook a `CCam::Process_Cam_On_A_String`** — y el port **ya tiene la fórmula del
master** implementada a mano en su cámara SA (`src/core/Cam.cpp:6046-6068`).

---

## 1 · Fórmula EXACTA de `CarSpeedDependantFOV` en la build instalada

### 1.1 · Qué se lee y dónde (medido)

| Qué | Valor medido |
|---|---|
| Lectura de la clave | `FUN_10010e70(local_94, &DAT_10035640, 4, "CarSpeedDependantFOV", 0x14, 0.0)` — **float**, default **0.0** (a diferencia de `HudWidthScale`/`HudHeightScale`/`RadarWidthScale`/`SubtitlesScale`, que sustituyen 0.0 por 0.8/0.8/0.9/0.8) |
| Dirección de la lectura | ASI VA `0x1000A73F` (`.text`), dentro de `FUN_10009CB0` (`ReadSettings`+`ApplyIniOptions`, 7467 B) |
| Variable donde queda | `DAT_1004CB84` (`.data` del ASI, VA `0x1004CB84`), tipo `float` |
| Referencias a `DAT_1004CB84` | `0x100021EA` (lectura en el ajuste de FOV), `0x10002235` (división), `0x1000A780` (la escritura de la lectura) — **3 en total, ninguna más** |

### 1.2 · La función que la consume: `FUN_10002180` (ASI) — ensamblado medido

El ASI sustituye por un salto el **setter de FOV del retail** (`FUN_0054a300`, VA `0x54A300`,
`mov [0x696658], eax`) mediante el patrón `D9 44 24 04 D9 1D ? ? ? ? C3`
(instalado con `MakeJMP` desde `FUN_1000CF00`).

Ensamblado **literal** de `FUN_10002180` (obtenido con `tools.rea.procedure_assembly`):

```
0x10002180: PUSH EBP
0x10002181: MOV EBP, ESP
0x10002183: MOVSS XMM0, dword ptr [0x100352ec]     ; 70.0f
0x1000218b: MOVSS XMM1, dword ptr [EBP + 0x8]       ; fFactor (parámetro)
0x10002190: DIVSS XMM0, XMM1                        ; XMM0 = 70.0f / fFactor
0x10002194: MOV EAX, [0x1004cb80]                   ; puntero al float de FOVControl
0x10002199: MOVSS dword ptr [0x1004cb78], XMM0
0x100021a1: TEST EAX, EAX
0x100021a3: JZ 0x100021ab
0x100021a5: MOVSS XMM0, dword ptr [EAX]             ; FOVFactor = *ptr (1.0f escrito por FOVControl)
0x100021a9: JMP 0x100021b3
0x100021ab: MOVSS XMM0, dword ptr [0x100352b0]      ; FOVFactor = 1.0f
0x100021b3: MOV EAX, [0x1004cbd0]                   ; puntero a bIsInCutscene
0x100021b8: MOVSS dword ptr [0x1004cb7c], XMM0       ; guarda FOVFactor
0x100021c0: CMP byte ptr [EAX], 0x1
0x100021c3: JNZ 0x100021ce
0x100021c5: CMP byte ptr [0x1004cb76], 0x0          ; RestoreCutsceneFOV
0x100021cc: JNZ 0x100021d7
0x100021ce: CMP byte ptr [0x1004cb77], 0x0          ; DontTouchFOV
0x100021d5: JZ 0x100021e6
0x100021d7: MOV EAX, [0x1004cae8]                   ; &FOV (0x696658)
0x100021dc: MULSS XMM0, XMM1                        ; XMM0 = FOVFactor * fFactor
0x100021e0: MOVSS dword ptr [EAX], XMM0
0x100021e4: POP EBP
0x100021e5: RET
0x100021e6: MOVSS XMM0, dword ptr [0x1004cb84]      ; CarSpeedDependantFOV
0x100021ee: UCOMISS XMM0, dword ptr [0x100352e0]    ; != 0.0f ?
0x100021f5: LAAH
0x100021f6: TEST AH, 0x44
0x100021f9: JNP 0x10002257                          ; si == 0 -> rama normal
0x100021fb: CALL dword ptr [0x1004cbdc]             ; FUN_004bc200() : "el jugador va en vehiculo?"
0x10002201: TEST EAX, EAX
0x10002203: JZ 0x10002252
0x10002205: MOV EAX, [0x1004cae4]                   ; &aspectRatio (0x94DD40)
0x1000220a: SUB ESP, 0x8
0x1000220d: MOVSS XMM0, dword ptr [EAX]
0x10002211: MOVSS dword ptr [ESP + 0x4], XMM0
0x10002217: MOVSS XMM0, dword ptr [EBP + 0x8]       ; fFactor
0x1000221c: MOVSS dword ptr [ESP], XMM0
0x10002221: CALL 0x10006a70                         ; AdjustFOV(fFactor, aspectRatio)
0x10002226: MOVSS XMM0, dword ptr [0x1004cb88]      ; speed_term
0x1000222e: ADD ESP, 0x8
0x10002231: DIVSS XMM0, dword ptr [0x1004cb84]      ; speed_term / CarSpeedDependantFOV
0x10002239: MOV EAX, [0x1004cae8]
0x1000223e: FMUL float ptr [0x1004cb7c]             ; AdjustFOV * FOVFactor
0x10002244: FSTP float ptr [EBP + 0x8]
0x10002247: ADDSS XMM0, dword ptr [EBP + 0x8]       ; suma
0x1000224c: MOVSS dword ptr [EAX], XMM0
0x10002250: POP EBP
0x10002251: RET
0x10002252: MOVSS XMM1, dword ptr [EBP + 0x8]       ; rama normal (sin término de velocidad)
0x10002257: MOV EAX, [0x1004cae4]
0x1000225c: SUB ESP, 0x8
0x1000225f: MOVSS XMM0, dword ptr [EAX]
0x10002263: MOVSS dword ptr [ESP + 0x4], XMM0
0x10002269: MOVSS dword ptr [ESP], XMM1
0x1000226e: CALL 0x10006a70
0x10002273: FMUL float ptr [0x1004cb7c]
0x10002279: ADD ESP, 0x8
0x1000227c: MOV EAX, [0x1004cae8]
0x10002281: FSTP float ptr [EAX]
0x10002283: POP EBP
0x10002284: RET
```

**Fórmula medida (build instalada):**

```
si (cutscene Y RestoreCutsceneFOV) O DontTouchFOV:
        FOV = FOVFactor * fFactor
si no:
        base = AdjustFOV(fFactor, aspectRatio) * FOVFactor
        si (CarSpeedDependantFOV != 0.0) Y (FUN_004bc200() != 0):
                FOV = base + speed_term / CarSpeedDependantFOV
        si no:
                FOV = base
```

`AdjustFOV` (ASI `FUN_10006A70`) medido:

```
d  = (double)(fFactor * 0.5f * 0.017453292f)     ; medio-FOV en radianes
d  = (double)((float)d * (aspectRatio / 1.3333334f))
out = roundf(((float)d + (float)d) * 57.295776f * 100.0f) / 100.0f
```

Es decir: **`AdjustFOV(f, a) = f * (a / (4/3))`, redondeado a 2 decimales** — estiramiento **lineal**
del FOV, no la conversión `tan/atan` que usa el port y el master moderno. Las dos llamadas a
`FUN_1001F170` / `FUN_1001EE80` son **conversores CRT float→double**, no matemáticas (verificado
por decompilación: sólo comprueban `MXCSR`/denormales y delegan en `FUN_1002817F`/`FUN_1002805F`).

Direcciones del retail measuredas por el propio ASI (operando del patrón):

| Puntero del ASI | Patrón | Dirección en el retail |
|---|---|---|
| `DAT_1004cae4` (aspecto) | `FF 35 ? ? ? ? FF 74 24 10 6A 00 50` | **`0x94DD40`** (float) |
| `DAT_1004cae8` (FOV) | `D8 35 ? ? ? ? D9 9B F0 00 00 00 D9 45 08 D8 1D` | **`0x696658`** (float) |
| `DAT_1004cbd0` (`bIsInCutscene`) | `80 3D ? ? ? ? ? 74 04 83 C4 38 C3` | **`0x7E46FD`** (byte) |
| `DAT_1004cb80` (FOVControl) | `DE D9 DE D9 EB E9` | **`0x5766A6`** (float) — confirma la corrección del recibo previo (R5): el comentario del INI decía `0x576686`, la verdad es `+0x20` |
| `DAT_1004cbdc` (predicado "en vehiculo") | `0F B6 05 ? ? ? ? 6B C0 2E 8B 0C C5 ? ? ? ? 85 C9 74 10` | **`0x4BC200`** |
| `DAT_1004cbe8` (float del radar) | `D9 1D ? ? ? ? EB 46` | **`0x974BF4`** (float) |

### 1.3 · De dónde sale `speed_term` — el proxy es el **radio del radar**

`DAT_1004CB88` lo escribe `FUN_10009C70` (ASI), invocado desde la trampoline `0x10008C90`
(patrón Injector `pushfd/pushal … call … popal/popfd`) que el ASI instala con `MakeJMP` en la
dirección **`0x4C5D2A`** del retail, y que engancha con el patrón `D9 1D ? ? ? ? EB 46`.

Descompilación **medida** de la función retail `FUN_004C5C80` (246 B, contiene `0x4C5D2A`):

```c
void FUN_004c5c80(void)
{
  if ((DAT_007e46fd != 1) && (DAT_00a10b4d != 0)) {
    FUN_004c3da0();
    iVar2 = FUN_004bc200();                       // "jugador en vehiculo"
    if (iVar2 == 0) {
      _DAT_00974bf4 = 180.0;                      // radio del radar = 180
    } else {
      pfVar3 = (float *)FUN_004bc230();            // vector de velocidad del objetivo
      fVar1 = SQRT(pfVar3[2]*pfVar3[2] + pfVar3[0]*pfVar3[0] + pfVar3[1]*pfVar3[1]);
      if (fVar1 < _DAT_0068fefc) {
        _DAT_00974bf4 = 180.0;
      } else if (fVar1 < _DAT_0068ff00) {
        _DAT_00974bf4 = (fVar1 - _DAT_0068fefc) * _DAT_0068ff04 * _DAT_0068ff08 + _DAT_0068ff0c;
      } else {
        _DAT_00974bf4 = 350.0;
      }
    }
    puVar4 = (undefined4 *)FUN_004bc040();
    _DAT_00704738 = puVar4[1];
    _DAT_00704734 = *puVar4;
    if (DAT_00869644 != 1) {
      FUN_004c17e0();                             // dibuja el radar
    }
  }
}
```

Constantes del retail **leídas byte a byte** en `gta-vc.exe` (`.data` VA `0x683000`, `raddr 0x280A00`):

| Dirección | Bytes | Float | Significado |
|---|---|---|---|
| `0x68FEFC` | `9A 99 99 3E` | **0.3** | umbral inferior de velocidad |
| `0x68FF00` | `66 66 66 3F` | **0.9** | umbral superior de velocidad |
| `0x68FF04` | `00 00 2A 43` | **170.0** | multiplicador 1 |
| `0x68FF08` | `56 55 D5 3F` | **1.6666667** (5/3) | multiplicador 2 |
| `0x68FF0C` | `00 00 34 43` | **180.0** | offset |

Comprobación: en `speed = 0.9` → `(0.9-0.3)*170*(5/3)+180 = 170+180 = 350` ✓ (coincide con el
`else` que fija 350). En `speed = 0.3` → 180 ✓.

Es decir: **el radio del radar crece linealmente de 180 a 350 entre velocidad 0.3 y 0.9**, y el ASI
usa **ese radio como proxy de la velocidad**.

La trampoline del ASI hace exactamente (medido en `FUN_10009C70`):

```c
DAT_1004cb88 = (float)ST0;      // valor que el retail acaba de calcular (el radio)
*DAT_1004cbe8 = DAT_1004cb88;   // lo devuelve a 0x974BF4 (el radar sigue igual)
DAT_1004cb88 = DAT_1004cb88 - 180.0;   // speed_term = radio - 180  ∈ [0, 170]
```

Por tanto:

```
speed_term = (speed - 0.3) * 283.3333      si 0.3 <= speed < 0.9
speed_term ∈ [0, 170]                      (limitado por las ramas del retail)
```

**Aviso medido (bug de la build instalada):** el punto de captura (`0x4C5D2A`) está **sólo en la
rama intermedia** del retail. Si la velocidad es `< 0.3` o `>= 0.9`, el retail **no pasa por esa
instrucción**, la trampoline **no se ejecuta** y `DAT_1004CB88` conserva el valor anterior (o 0 al
arrancar, lo que daría `FOV = -180/CSV`, es decir **-9° con el default 20.0**). El término de
velocidad sólo se actualiza dentro de la ventana de velocidad válida.

### 1.4 · El predicado `FUN_004BC200` (medido)

```c
undefined4 FUN_004bc200(void)
{
  iVar1 = (&DAT_0094ad30)[(uint)DAT_00a10b03 * 0x5c];
  if ((iVar1 != 0) && (*(char *)(iVar1 + 0x3ac) != 0))
    return *(undefined4 *)(iVar1 + 0x3a8);
  return 0;
}
```

Devuelve el puntero a la entidad "en vehiculo" del slot de jugador `DAT_00A10B03`, o 0 si el
jugador **no va en vehiculo**. Es el mismo predicado que usa el retail para decidir si el radar
se auto-zoomea. `FUN_004BC230` devuelve `entidad + 0x70` (los 3 floats de velocidad).

### 1.5 · Contraste con el master (`Misc.ixx:60-89`)

El master **reescribe** la feature con otra técnica: hook **inline** al retail `0x47FF22`
(`C7 83 ? ? ? ? ? ? ? ? 80 7B` → `mov dword ptr [ebx+0xB4], 0x428C0000` = `cam->FOV = 70.0f`),
neutraliza esa instrucción con `MakeNOP(…, 10)` y aplica su propia lógica en un `safetyhook::create_mid`:

```cpp
constexpr auto DefaultFOV = 70.0f;
auto cam = (CCam*)regs.ebx;
if (cam->ResetStatics) {
    cam->FOV = DefaultFOV;
} else if (cam->CamTargetEntity) {
    CVehicle* car = (CVehicle*)cam->CamTargetEntity;
    float forwardSpeed = DotProduct(car->GetForward(), car->m_vecMoveSpeed);
    if (forwardSpeed > 0.4f)
        cam->FOV += (forwardSpeed - 0.4f) * CTimer::ms_fTimeStep;
    if (cam->FOV > DefaultFOV)
        cam->FOV = pow(0.98f, CTimer::ms_fTimeStep) * (cam->FOV - DefaultFOV) + DefaultFOV;
    if (cam->FOV > DefaultFOV + 30.0f)
        cam->FOV = DefaultFOV + 30.0f;
}
```

**Fórmula del master:**

```
si ResetStatics:                      FOV = 70
si no, y hay entidad objetivo:
        forwardSpeed = dot(forward(vehículo), velocidad)
        si forwardSpeed > 0.4:  FOV += (forwardSpeed - 0.4) * dt
        si FOV > 70:           FOV = pow(0.98, dt) * (FOV - 70) + 70      (relajación)
        si FOV > 100:          FOV = 100                                  (tope +30°)
```

### 1.6 · Las dos fórmulas, cara a cara

| | Build instalada (2018) | Master (GitHub) |
|---|---|---|
| Magnitud medida | `radio_del_radar - 180` ∈ [0,170] | `dot(forward, velocidad)` (m/s) |
| Origen del dato | hook al **zoom del radar** (retail `0x4C5D2A`) | hook a **`CCam::Process_Cam_On_A_String`** (retail `0x47FF22`) |
| Escala / divisor | `/ CarSpeedDependantFOV` (float del INI, default 20.0 → hasta **+8.5°**) | multiplica por `dt` (0.4 → umbral de arranque) |
| Relajación | **ninguna** (el término es acumulativo mientras esté en la ventana de velocidad) | `pow(0.98, dt)` (decae ~2 %/frame) |
| Tope | **ninguno** (salvo el tope natural de 170/20 = 8.5°) | `70 + 30 = 100°` |
| Tipo de clave INI | **float** (`20.0`) | **int** (`0|1`, `Misc.ixx:32`) |
| Semántica del INI | "cuanto **menor**, más FOV" (divisor) | "0/1" (interruptor) |
| ¿Existe en el retail vanilla? | no | **no** — el retail 1.0 **no contiene** el float `0.98f` en absoluto (medido: 0 apariciones de `0x7AE147AE` en todo `.text`), luego el master **añade** la feature |

### 1.7 · Qué usaría el port, y por qué

**El port ya tiene la fórmula del master, escrita a mano.** Medido en
`src/core/Cam.cpp:6046-6068` (dentro de `CCam::Process_FollowCar_SA`, la cámara al estilo SA):

```cpp
if (ResetStatics) {
    FOV = DefaultFOV;
} else {
    if (isCar || isBike) {
        if (DotProduct(car->GetForward(), car->m_vecMoveSpeed) > 0.4f)
            FOV += (DotProduct(car->GetForward(), car->m_vecMoveSpeed) - 0.4f) * CTimer::GetTimeStep();
    }
    if (FOV > DefaultFOV)
        FOV = Pow(0.98f, CTimer::GetTimeStep()) * (FOV - DefaultFOV) + DefaultFOV;
    ...
    FOV = Clamp(FOV, DefaultFOV, DefaultFOV + 30.0f);
}
```

Es **idéntica** a `Misc.ixx:60-89` (mismo 0.4, mismo `pow(0.98f, dt)`, mismo tope `+30`). Además el
port ya tiene `m_fFovSpeedOverOneFrame` (`src/core/Cam.cpp:966`), `DefaultFOV`, `Cams[].FOV` y
`CPhysical::m_vecMoveSpeed` (`src/entities/Physical.h:21`, `GetMoveSpeed()` en `:111`).

**Lo que NO tiene el port**: la cámara **nativa de VC** (`CCam::Process_Cam_On_A_String`,
`src/core/Cam.cpp:2123`) pone `FOV = DefaultFOV` en `:2131` y **no** tiene término de velocidad.
El master aplica su hook **exactamente ahí**.

**Recomendación (medida, no supuesto):**

1. **No portar la fórmula de la build instalada.** Es un artefacto: depende del radio del radar
   (un valor **visual** de 180..350 px) y produce un término de FOV que salta (no se relaja) y
   que **depende del zoom del radar**, no de la velocidad real. Además es *insensible* a la
   velocidad fuera de la ventana 0.3..0.9 (bug medido en §1.3). Portarla metería una dependencia
   del radar en el FOV.
2. **Portar la del master** (o, mejor, **extender la que ya hay**): añadir el bloque de
   `Cam.cpp:6046-6068` a `CCam::Process_Cam_On_A_String` (`src/core/Cam.cpp:2123`), con la misma
   constante 0.4 y el mismo `pow(0.98f, dt)`, y con el tope `DefaultFOV + 30`.
3. **El contrato de INI hay que elegirlo**: la build instalada usa un **float** (`20.0`, "cuanto
   menor, más FOV"), el master un **int** (`0|1`). Si el objetivo es el master, la clave debe ser
   **binaria**; si se quiere conservar el contrato de la build instalada, hay que inventar una
   tercera forma (p. ej. `CarSpeedDependantFOV = 1` + `CarSpeedFOVDivisor = 20.0`).
   **Decisión del jugador**: el master es la especificación → clave **binaria**.
4. **No tocar `CDraw::SetFOV`** (`src/renderer/Draw.cpp:96-106`): el término de velocidad del
   master va en `cam->FOV` (el FOV del juego), **antes** de que el motor lo convierta con
   `ConvertFOV`. La conversión del port (`Draw.cpp:83-93`, `tan/atan`) es **la misma** que la del
   master (`Draw.ixx:229-237`) → **sin conflicto**.

### 1.8 · Cierre del riesgo R7 del recibo previo

`wfp-16-rea.md` §3 feature 12 decía: "cuál es exactamente la fórmula en esta build no se ha
medido → marcado como supuesto". **Cerrado**: la fórmula está medida (§1.2) y el proxy de
velocidad está medido en el retail (§1.3). El "supuesto" ya no es necesario.

---

## 2 · Inventario de features del master

### 2.1 · Cifras medidas del master

| Métrica | Valor medido |
|---|---|
| Ficheros en `source/GTAVC.WidescreenFix/` | **32** = **29 `.ixx`** + `dllmain.cpp` + `common.h` + `rw.h` |
| Llamadas `pattern(...)` | **368** (de ellas 333 con literal directo, 35 construidas con `pattern_str`/`to_bytes`) |
| Literales de patrón **literales** distintos | **324** (319 de `hook::pattern("…")` + 5 incrustados en `pattern_str`/otros) |
| Patrones del master **también** en el binario instalado | **96 / 324** (coincidencia exacta, normalizada) |
| Patrones **sólo del master** (posteriores) | **228 / 324** |
| Patrones **sólo del binario instalado** | **10** (los P40/P42/P76/P88/P89/P91/P93/P94/P95/P97 del recibo previo) |
| Claves de INI del master | **32** únicas en 3 secciones: `[MAIN]` 16, `[MISC]` 13, `[GRAPHICS]` 5 (`HideAABug` y `ForceMultisamplingLevel` se leen en dos secciones) |
| Patrones del master que **coinciden** en el retail 1.0 | **324 / 324** (≥1 coincidencia cada uno) |

Reparto de los **228 patrones sólo del master** por fichero (medido):

| Fichero master | Patrones sólo del master |
|---|---|
| `Frontend.ixx` | 59 |
| `InteriorLoading.ixx` | 28 |
| `Loading.ixx` | 26 |
| `Sprite2d.ixx` | 19 |
| `Hud.ixx` | 12 |
| `Main.ixx` | 11 |
| `TransparentMenu.ixx` | 11 |
| `Menu.ixx` | 10 |
| `Draw.ixx` | 8 |
| `Radardisc.ixx` | 7 |
| `dllmain.cpp` | 6 |
| `MSAA.ixx` | 5 |
| `Misc.ixx` | 4 |
| `Legacy.ixx` | 3 (los 3 están **comentados** en el master) |
| `PostFX.ixx` | 3 |
| `Sprite.ixx` | 3 |
| `Timer.ixx` | 3 |
| literales incrustados (`pattern_str`, `range_pattern`) | 3 |
| `CutsceneMgr.ixx` | 2 |
| `Skeleton.ixx` | 2 |
| `Camera.ixx` | 1 |
| `Vehicle.ixx` | 1 |
| `VisibilityPlugins.ixx` | 1 |

### 2.2 · Tabla de features del master (65 filas)

Columna **Build 2018**: `SÍ` = la feature está en la build instalada (patrón compartido o
equivalente medido); `NO` = **POSTERIOR**, sólo en el master.
Columna **Port**: `YA ESTABA` / `PARCIAL` / `A PORTAR` / `NO APLICA`, con `fichero:línea` medido.

| # | Feature | Clave INI | Master (fichero:línea) | Qué hace | Build 2018 | Estado en el port |
|---|---|---|---|---|---|---|
| 1 | Forzar resolución | `ResX`,`ResY` | `Legacy.ixx:327-328`, `:473-498` | Escribe la resolución en los campos de modo de vídeo del retail y anula la rama que forzaba 640×480 | SÍ | **YA ESTABA** — el canvas del navegador *es* la resolución (`src/core/re3.cpp:645` `LoadINISettings`, `src/skel/glfw/glfw.cpp:912`) |
| 2 | Aspecto forzado `W:H` | `ForceAspectRatio` | `Legacy.ixx:329`, `:500-504`, `:900-906` | Sustituye el cálculo de aspecto del juego por un `W:H` libre | SÍ | **PARCIAL** — el port ofrece una lista cerrada (`AR_AUTO/4:3/5:4/16:10/16:9/21:9`, `src/renderer/Draw.cpp:35-52`) |
| 3 | Aspecto del frontend | `FrontendAspectRatio` | `Legacy.ixx:330`, `:1010-1078` | Recorta el quad *fullscreen* del dibujado 2D al aspecto pedido y pinta barras laterales | SÍ | **PARCIAL** — el port deduce el aspecto del raster (`src/renderer/Sprite2d.cpp:115-139`, bajo `VICEEXT_FIX_WFP`) |
| 4 | No tocar el FOV | `DontTouchFOV` | `Legacy.ixx:331`, `:204-205` | Escribe el FOV tal cual, sin estirar por aspecto | SÍ | **NO APLICA** tal cual — el port convierte siempre (`src/renderer/Draw.cpp:99-104`); falta el interruptor |
| 5 | FOV original en cinemática | `RestoreCutsceneFOV` | `Legacy.ixx:332`, `:206-207` | En cutscene el FOV no se estira (queda a 4:3, cap 16:9) | SÍ | **YA ESTABA** — `src/renderer/Draw.cpp:100-103` (en cutscene no convierte) |
| 6 | Multiplicador de escala del HUD (X) | `HudWidthScale` | `Legacy.ixx:334`, `:829-836`; `Frontend.ixx:642`, `:1000-1028` | Multiplica por 0.8 la escala horizontal de 87 slots del HUD (excluye los índices 15-17, 20, 66, 67) | SÍ | **PARCIAL** — el port escala con `SCREEN_SCALE_X` (`src/core/common.h:201`, 186 usos en `src/renderer/Hud.cpp`) pero **sin** multiplicador configurable |
| 7 | Multiplicador de escala del HUD (Y) | `HudHeightScale` | `Legacy.ixx:335`, `:838-845`; `Frontend.ixx:643`, `:1030-1037` | Igual en vertical, 74 slots (excluye 13-15, 18, 57, 58) | SÍ | **PARCIAL** — ídem (`SCREEN_SCALE_Y`, `src/core/common.h:202`) |
| 8 | Escala del radar (legacy) | `RadarWidthScale` | `Legacy.ixx:336`, `:867-886` | Factor 0.9 sobre el ancho del radar y el "you are here" a 94×0.9 | SÍ | **PARCIAL** — `src/core/Radar.cpp:1577-1581` escala la posición, sin el multiplicador |
| 9 | Tamaño del radar (nuevo) | `RadarWidth`,`RadarHeight` | `Frontend.ixx:644-645`, `:1056-1111` | Escribe 97.5 / 89.0 en 3 slots de ancho y 3 de alto del radar | NO | **A PORTAR** — `src/core/Radar.h:181-182` usa 82×82 (`ms_bFixRadar`) o 94×76 |
| 10 | Escala de subtítulos | `SubtitlesScale` | `Legacy.ixx:337`, `:888-898`; `Frontend.ixx:646`, `:1113-1132` | Multiplica por el factor las 3 constantes de escala del texto de subtítulos (1.2/1.22/0.58) | SÍ | **A PORTAR** — `src/renderer/Hud.cpp:1500-1532` (`DrawSubtitles`) |
| 11 | Sombras de texto más pequeñas | — (en la build sí: `SmallerTextShadows`) | `Legacy.ixx:997-1000` (`SetDropShadowPosition`) | Intercepta el setter de posición de sombra y fuerza 1 (sombra pequeña) en vez de 2 | SÍ (como `SmallerTextShadows`) | **A PORTAR** — el port tiene `CFont::SetDropShadowPosition` (`src/renderer/Font.cpp:2114-2116`) pero sólo con 0/2 |
| 12 | Contorno en vez de sombra | `ReplaceTextShadowWithOutline` | `Misc.ixx:31`, `:42-58`; `Legacy.ixx:920-955`; `Frontend.ixx:171-301` | Sustituye la sombra difusa por un contorno de 16 direcciones (o 4), con alfa compensado | SÍ (parcial) | **PARCIAL / CONFLICTO** — `CFont::PrintOutlinedString` está **declarada y sin usar** (`src/renderer/Font.h:186`); el texto se dibuja con sombra en `src/renderer/Font.cpp:909-925` |
| 13 | Cursor sin sombra | (misma clave) | `Misc.ixx:44-45`; `Legacy.ixx:944-945` | Pone a 0 el alfa de la sombra del cursor del menú | SÍ | **PARCIAL** |
| 14 | Sombra de radio / título de menú | (misma clave) | `Misc.ixx:51-57`; `Legacy.ixx:947-954` | Anula la sombra del texto de la radio y del título de menú | SÍ | **PARCIAL** |
| 15 | Bordes de cinemática inteligentes | `SmartCutsceneBorders` | `Legacy.ixx:239-303`, `:914-918` | Sustituye el borde fijo por letterbox/pillarbox calculado del aspecto (`dScreenHeightWeWannaCut = 1 - 9/16·AR`, polinomio de borde) | SÍ | **PARCIAL** — el port tiene el interruptor (`src/core/config.h:696`, `src/core/Camera.cpp:2279-2280`) pero geometría **fija al 30 %** (`src/core/Camera.cpp:2786-2806`) |
| 16 | Bordes de cinemática animados | `NoCutsceneBorderAnimation` | `Frontend.ixx:647`; `Sprite2d.ixx:507-545`; `Draw.ixx:29-31` | Anima el zoom de la cámara en cutscene y escribe `g_cutsceneCameraZoom` para el FOV | NO | **A PORTAR** — el port no tiene nada de esto |
| 17 | MenuCanvas (lienzo centrado) | `MenuAspectRatioConstraint` | `MenuConstraint.ixx:252`; `Draw.ixx:111-222` | Reduce `RsGlobal->width` a un lienzo 16:9 centrado para el menú, re-mapea el ratón y avisa del cambio de layout | NO | **A PORTAR** — el port usa el raster completo (`src/core/Frontend.cpp:2904`) |
| 18 | Restricción de aspecto del HUD | `HudAspectRatioConstraint` | `Draw.ixx:328`, `:64-75` | Calcula `fWidescreenHudOffset` para anclar el HUD en pantallas anchas | NO | **A PORTAR** — el port ancla a la derecha con `SCREEN_SCALE_FROM_RIGHT` |
| 19 | Menú a pantalla completa | — | `Menu.ixx` (10 patrones) | Reescribe el dibujado de los menús para que cubran el viewport | NO | **YA ESTABA** — `src/core/Frontend.cpp:2904` (`CMenuManager::DrawBackground`) |
| 20 | Menú translúcido | `TransparentMenu` | `TransparentMenu.ixx:46`, `TransparentMenuDX8.ixx`, `TransparentMenuDX9.ixx` (11+11+11) | Pinta el fondo del menú con alpha + blur, por API D3D8/D3D9 | NO | **NO APLICA** — D3D8/D3D9; el port renderiza con librw/WebGL2 |
| 21 | Tope de FPS del menú | `MenuFrameLimit` | `Menu.ixx:567` | Limita el frame rate del menú | NO | **NO APLICA** — el frame rate lo da el navegador |
| 22 | HUD anclado a los bordes | — | `Hud.ixx` (12 patrones) | Mueve el HUD a izquierda o derecha según el ancho, reordenando el dibujado del HUD | NO | **CONFLICTO** — el port dibuja el HUD anclado a la derecha con `SCREEN_SCALE_FROM_RIGHT` (p. ej. `src/renderer/Hud.cpp:666, 805, 1243`) |
| 23 | FOV por velocidad de coche | `CarSpeedDependantFOV` | `Misc.ixx:32`, `:60-89` | Hook a `CCam::Process_Cam_On_A_String`: FOV += (vel-0.4)·dt, relajación `pow(0.98,dt)`, tope +30° | NO (en la build es otra técnica, §1) | **CONFLICTO / PARCIAL** — el port ya tiene la fórmula en `src/core/Cam.cpp:6046-6068`, pero **sólo** en la cámara SA; la nativa VC (`src/core/Cam.cpp:2123`) no |
| 24 | Sacudida de cámara VCS | `VCSCamShake` | `Misc.ixx:33`, `:91-95`; `Camera.ixx:613-642` | Aplica `UpdatePlayerVehicleSpeedBlur` (temblor aleatorio proporcional a la velocidad) en `CalculateDerivedValues` | NO | **A PORTAR** — `src/core/Camera.cpp` no tiene el blur de velocidad |
| 25 | Multiplicadores de FOV por hash | — | `Draw.ixx:259-294`; `Legacy.ixx:222-235`; `dllmain.cpp:77-99` | API exportada `SetFOVMultiplier`/`RemoveFOVMultiplier`/`GetCurrentFOV` para que otros mods ajusten el FOV | NO | **A PORTAR** — `src/renderer/Draw.cpp:96-106` no tiene multiplicador |
| 26 | Disparo de niebla/D3D8 | — | `VisibilityPlugins.ixx:15-16` | Cambia `m_alphaList` de 150 a 600 (el FOV ancho deja fuera objetos) | NO | **A PORTAR** (o NO APLICA si el port no tiene lista de alpha) |
| 27 | Luces brillantes sólo de lejos | — | `Draw.ixx:339-342` | Evita renderizar `CBrightLights` si la cámara está cerca | NO | **A PORTAR** — `src/renderer/Coronas.cpp` |
| 28 | Distancia de trayectoria de tráfico | — | `Main.ixx:63-76` | Divide la distancia de la trayectoria por `GenerationDistMultiplier` para no rechazar tráfico válido en anchas | NO | **A PORTAR** — `src/core/Camera.cpp` (`GenerationDistMultiplier`) |
| 29 | Humo de impacto de bala | — | `Main.ixx:90-97` | Cambia `push eax` por `push ebp` en `CWeapon::DoBulletImpact` (desalinea la pila) | NO | **A PORTAR** — `src/weapons/Weapon.cpp` |
| 30 | FOV de la cámara de rueda | — | `Main.ixx:103-109` | Sustituye el FOV de `CCam::Process_WheelCam` por `ConvertFOVInverse(70)` | NO | **CONFLICTO** — `src/core/Cam.cpp` (`Process_WheelCam`) |
| 31 | FOV inverso en `AvoidTheGeometry` | — | `Main.ixx:27-31`, `:111-113` | Convierte el FOV a la inversa antes de la colisión de cámara | NO | **CONFLICTO** — `src/core/Camera.cpp` (`AvoidTheGeometry`) |
| 32 | `CameraSize` con view window real | — | `Main.ixx:21-25`, `:78-79` | Fuerza `SCREEN_VIEWWINDOW` y el aspecto real al crear la cámara Rw | NO | **YA ESTABA** — `src/core/common.h:192` ya usa `Tan(DEGTORAD(CDraw::GetScaledFOV()*0.5f))` |
| 33 | `GetScreenRect` en el fundido | — | `Main.ixx:33-40`, `:100-101` | Durante el fundido usa el rectángulo de contenido de la cinemática, no la pantalla entera | NO | **A PORTAR** — `src/renderer/Credits.cpp:801`, `src/renderer/Hud.cpp:1513` |
| 34 | Escala de sprites y partículas | — | `Sprite.ixx:46-62` | Reescribe `CalcScreenCoors`/`CalcParticleScreenCoors` para corregir el tamaño horizontal en anchas | NO | **CONFLICTO CON MOD** — el 13.0 ya trae `ScriptSpritesScaling` portado (`src/renderer/Sprite2d.cpp`) |
| 35 | Escalado de sprites 2D / letterbox | — | `Sprite2d.ixx` (19 patrones) | Reescribe `SetVertices`/`Draw`/`DrawRect` para recortar al aspecto del frontend, dibujar barras y fundidos limpios | NO | **YA ESTABA (parcial)** — `src/renderer/Sprite2d.cpp:115-139` |
| 36 | Posición del mapa del menú | — | `Frontend.ixx:493-600` | Da al mapa del menú un lienzo de 448 px de alto con su propio centro y zoom, y reescala 27 escalas + 27 anchos | NO | **CONFLICTO** — `src/core/Frontend.cpp` (dibujo del mapa) |
| 37 | Texto de script en lienzo centrado | — | `Frontend.ixx:423-489` | Mueve los textos de script (títulos de misión, intro) al lienzo 4:3 centrado y los escala por `ResXInvRefs`/`ResYInvRefs` | NO | **CONFLICTO** — el port ya centra con `SCALE_AND_CENTER_X` (`src/renderer/Hud.cpp:1466`, `:1820`) |
| 38 | Escalas del frontend por clase | — | `Frontend.ixx:667-743` | Sustituye 24 punteros de escala (eCDarkel, eCGarages, eCMenuManager, eCLoadingScreen, eCRadar, eCHud, eCParticle, eCSpecialFX, ecMusicManager, eCSceneEdit, eConvertingTexturesScreen, eCReplay) por inversos de resolución | NO | **CONFLICTO** — el port escala el frontend con `SCREEN_SCALE_*` (68 `SCREEN_SCALE` + 45 `SCREEN_STRETCH` en `src/core/Frontend.cpp`) |
| 39 | Arreglos de Brief / Stats / Leyenda | — | `Frontend.ixx:745-771` | Reescribe 6 constantes de posición/anchura del briefing, de las estadísticas y de la leyenda del mapa | NO | **CONFLICTO (leve)** — `src/core/Frontend.cpp` |
| 40 | Márgenes de máscara del menú | — | `Frontend.ixx:602-632` | Escala las coordenadas X de las 4 máscaras animadas del fondo del menú | NO | **NO APLICA en retail 1.0** (§4.3: la guarda `last.size() != 1` falla) |
| 41 | Anclaje del mapa (radio del carousel) | — | `Frontend.ixx:773-816` | Conserva la fracción de scroll de la radio al cambiar de resolución | NO | **A PORTAR** |
| 42 | Reservar el radar en los subtítulos | — | `Frontend.ixx:102-117`, `:1138-1141` | Limita `CentreSize` de los subtítulos para no invadir el radar | NO | **A PORTAR** — `src/renderer/Hud.cpp:1529-1548` |
| 43 | Posición de los subtítulos | — | `Frontend.ixx:119-138`, `:1143-1145` | Sube los subtítulos justo lo que la barra de cinemática se ha encogido respecto al 4:3 | NO | **A PORTAR** |
| 44 | Anchura de subtítulos | — | `Frontend.ixx:1113-1132` | Escala anchura y alto del texto de subtítulo por `SubtitlesScale` | NO | **A PORTAR** |
| 45 | Sin pantalla de carga entre islas | `NoIslandLoading` | `Loading.ixx:291-387` (26 patrones) | Precarga todos los edificios grandes de la isla y salta `LoadScene` | NO | **PENDIENTE con motivo** — rompe el on-demand (D15), `docs/mods/ATTRIBUTION.md:321` |
| 46 | Transición sin pantalla de carga | `NoInteriorLoading` | `InteriorLoading.ixx:527`, `:531-737` (28 patrones) | Reescribe la pantalla de carga de interior para que el interior y el exterior encadenen sin fundido | NO | **A PORTAR** (mismo motivo D15) |
| 47 | Disco del radar HQ | `HQRadarDisc` | `Radardisc.ixx:65-169` (7 patrones) | Sustituye el raster 128×128 del `radardisc` por uno de 256×256 dibujado por código (anillo con rampa de color) | NO | **NO APLICA** — decidido por el jugador el 04/10: el mod Extended ya trae `HD radar` y el `hud.txd` servido trae `radardisc` (`docs/mods/ATTRIBUTION.md:322`) |
| 48 | MSAA forzado | `ForceMultisamplingLevel` | `MSAA.ixx:44-54`; `Legacy.ixx:959-968` | Sobrescribe la variable de nivel de multisampling del motor con el valor del INI o el máximo del driver | SÍ (legacy) | **NO APLICA** — depende de `IDirect3DDevice8` |
| 49 | Ocultar el bug de AA de 1 px | `HideAABug` | `MSAA.ixx:73-105`; `Legacy.ixx:263-272`, `:908-912` | Dibuja 1 px negro arriba/izquierda (4 lados con valor 2) para tapar el borde que dejaba el MSAA de D3D8 | SÍ (legacy) | **NO APLICA** — artefacto del MSAA de Direct3D 8 |
| 50 | Gamma de consola / SMAA | `ConsoleGamma`,`SMAA` | `PostFX.ixx:35-75` (3 patrones) | Curva de gamma al estilo Xbox 360 y SMAA como post-proceso | NO | **NO APLICA** — requiere `IDirect3DDevice9` |
| 51 | FOV de vehículos de emergencia | — | `Legacy.ixx:200-202`, `:529-538` | Recalcula el FOV de la cámara de emergencia a `70/factor` | SÍ | **PARCIAL** — el port tiene `m_fFOV_Wide_Screen` (`src/core/Camera.cpp:2779`) |
| 52 | Salto de `AvoidTheObstacles` con AR ≥ 2 | — | `Legacy.ixx:506-522`, `:529-530` | Con aspecto ≥ 2 salta la rutina de colisión de cámara | SÍ | **A PORTAR** |
| 53 | Retícula escalada por aspecto | — | `Legacy.ixx:721-740` | Escribe `fCrosshairPosFactor` y `fCrosshairHeightScaleDown` en 3 sitios | SÍ | **PARCIAL** — `src/core/Camera.cpp:246-250` (`m_f3rdPersonCHairMultX/Y`), `src/renderer/Hud.cpp:313` (rango de armas de la cruz) |
| 54 | Coronas de vehículo más pequeñas | — (en la build: `FixVehicleLights`) | `Legacy.ixx:970-974` | Escribe `2.0f` en el factor de estirado de las luces de vehículo | SÍ (como `FixVehicleLights`) | **A PORTAR** — `src/renderer/Coronas.cpp:374-394` |
| 55 | Sin "cuadrado" en las luces | `NoLightSquare` | `Legacy.ixx:616-638` | Anula las llamadas a `CBrightLights::RegisterOne` en el render de coches y motos | NO | **A PORTAR** — `src/renderer/Coronas.cpp` |
| 56 | Alt-Tab sin pausar | `AllowAltTabbingWithoutPausing` | `Legacy.ixx:1002-1006` | Parchea la función de pausa por pérdida de foco a `ret` | SÍ | **NO APLICA** — en web manda `visibilitychange` |
| 57 | Ratón "exclusive" | — | `Legacy.ixx:1107` | Salta la rutina de ratón en modo exclusivo | SÍ | **NO APLICA** — el navegador usa Pointer Lock |
| 58 | "WIDESCREEN" → "BORDERS" | — | `Legacy.ixx:1140-1147` | Cambia la etiqueta `FED_WIS` por `L"BORDERS"` | SÍ | **YA ESTABA** — el port tiene etiquetas propias (`src/core/MenuScreensCustom.cpp`) |
| 59 | Gancho de estado de carga | — | `Legacy.ixx:1119-1154` | Al pasar a estado de carga (< 9) re-aplica escalas tardías y llama a `SilentPatchCompatibility` | SÍ | **PARCIAL** — el port los re-aplica a mano en 4 sitios (`src/renderer/Hud.cpp:1481, 1492, 1829, 1834`) |
| 60 | Compatibilidad con SilentPatch | — | `Legacy.ixx:441-464` | Reescribe 4 instrucciones del render de texto si detecta el parche de SP | SÍ | **CONFLICTO CON MOD** — el 14.0 ya tiene su propia capa (`src/core/SilentPatchOptions.cpp`, `src/renderer/Font.cpp`) |
| 61 | Escalado del HUD completo | — | `Legacy.ixx:659-719` | Inyecta el factor de escala en ~100 punteros de HUD, partículas, FX 2D, replay, mapa y radio | SÍ | **YA ESTABA** — `src/core/common.h:195-217` |
| 62 | Escalado de los textos de menú | — | `Legacy.ixx:541-596` | Reescribe 13 constantes float de escala del menú divididas por el aspecto | SÍ | **YA ESTABA** — `src/core/Frontend.cpp` |
| 63 | `ResX`/`ResY` + resolución de escritorio | — | `Legacy.ixx:473-498`; `Main.ixx:47-55` | Además, el master fuerza la resolución del escritorio por defecto si no se especifica | SÍ | **NO APLICA** — en web manda el canvas |
| 64 | Hotkeys de arranque (`safetyhook`) | — | `dllmain.cpp:9-67` | Reescribe el arranque: `onInitEvent`, `onGameInitEvent`, `onGameProcessEvent`, `onBeforeReset`, `onEndScene`, `onShutdownEvent` | NO | **NO APLICA** — el port no carga ASI; sus eventos son funciones propias |
| 65 | Ruta de movie separada | — | `dllmain.cpp:20-23`; `Movie.ixx` | En VC la movie va por su propia ruta y hay que restaurar el foco | NO | **NO APLICA** — no hay reproductor de movies en el port |

### 2.3 · Resumen numérico

| Bloque | Nº de features |
|---|---|
| Filas de la tabla §2.2 | **65** |
| **Ya en la build 2018** (patrón compartido o equivalente medido) | **28** |
| **POSTERIORES (sólo master)** | **37** |
| → de las 37 posteriores: **NO APLICA** por plataforma (D3D8/D3D9/MSAA/ventana nativa/x86/arranque) | **8** (20, 21, 26\*, 40\*\*, 47, 50, 64, 65) |
| → de las 37 posteriores: **PENDIENTE con motivo ya decidido** (D15) | **2** (45, 46) |
| → de las 37 posteriores: **A PORTAR** sin conflicto | **12** (9, 16, 24, 27, 28, 29, 33, 41, 42, 43, 44, 55) |
| → de las 37 posteriores: **CONFLICTO** (real / con mod / duplicado) | **11** (17, 18, 22, 23, 30, 31, 34, 36, 37, 38, 39) |
| → de las 37 posteriores: **YA ESTABA en el port** (el master lo reescribe, el port ya lo tenía) | **3** (19, 32, 35) |
| → de las 37 posteriores: **SIN CONFLICTO** (API aditiva) | **1** (25) |

\* la 26 (`VisibilityPlugins`, `m_alphaList` 150→600) es A PORTAR **si** el port tiene la lista de
alpha; en el port medido no aparece esa lista → se clasifica NO APLICA por ahora.
\*\* la 40 (`MenuBorders`) es NO APLICA **en retail 1.0** por una guarda de cantidad (§4.4), no por
plataforma.

---

## 3 · TABLA DE CONFLICTOS

Zonas ya portadas (medido en `src/`): HUD (`src/renderer/Hud.cpp`), radar (`src/core/Radar.cpp`,
`src/core/Radar.h`), menús (`src/core/Frontend.cpp`, `src/core/MenuScreensCustom.cpp`), bordes de
cinemática (`src/core/Camera.cpp:2786`), sprites 2D (`src/renderer/Sprite2d.cpp`), texto
(`src/renderer/Font.cpp`), flares/coronas (`src/renderer/Coronas.cpp`), carga/streaming
(`src/core/Streaming.cpp`), cámara/FOV (`src/core/Cam.cpp`, `src/core/Camera.cpp`,
`src/renderer/Draw.cpp`).

### 3.1 · Conflictos con todo el detalle

| # | Zona portada | Qué haría el master | Qué hace hoy el port | Veredicto | Se pierde / Se gana |
|---|---|---|---|---|---|
| **C1** | HUD: `src/renderer/Hud.cpp` (186 `SCREEN_SCALE`), `src/core/common.h:201-207` | `Frontend.ixx:1000-1053` + `Legacy.ixx:827-865`: escribe `fCustomWideScreenWidthScaleDown` (= factor de escala × `HudWidthScale`) en **87** slots de escala horizontal y **74** verticales, saltando los índices 15-17, 20, 66, 67 / 13-15, 18, 57, 58 | El port escala cada elemento del HUD con las macros `SCREEN_SCALE_X/Y`, sin ningún multiplicador de usuario (`src/core/common.h:201-202`) | **CONFLICTO REAL** | **Se pierde**: el tamaño actual del HUD (el port no tiene el 0.8 del mod). **Se gana**: un multiplicador configurable (0.8–1.0) por separado en X e Y. Si se implementa ingenuamente sobre las macros, el HUD se **encoge dos veces** (el `SCREEN_SCALE_X` ya incluye el factor de aspecto). |
| **C2** | Radar: `src/core/Radar.h:181-182`, `src/core/Radar.cpp:1577-1581` | `Frontend.ixx:1056-1111`: escribe `RadarWidth` (97.5) y `RadarHeight` (89.0) en 3+3 slots; `Legacy.ixx:976-993` escribe 88.5/81 | El port usa 82×82 con `CDraw::ms_bFixRadar` (`src/renderer/Draw.cpp:27`) o 94×76 sin él; dibuja el disco con `SCREEN_SCALE_X(RADAR_WIDTH)` | **CONFLICTO REAL** | **Se pierde**: el tamaño del disco del radar que el jugador ya ve. **Se gana**: radio configurable (y el modo IV 88.5×81). El 13.0 ya trae `HD radar`, así que el disco "grande" del mod puede no ser deseable. |
| **C3** | Bordes de cinemática: `src/core/Camera.cpp:2786-2806` | `Legacy.ixx:238-303` (`GetScreenRect`): letterbox/pillarbox adaptativo con `dScreenHeightWeWannaCut = 1 − (9/16)·AR` y polinomio `−2.89286·AR² + 8.07321·AR − 5.51143`; `Sprite2d.ixx:507-545` añade además la **animación** del borde y `g_cutsceneCameraZoom` | `src/core/Camera.cpp:2790-2792`: borde **fijo al 30 %** (`m_ScreenReductionPercentage = 30.0f`, `src/core/Camera.cpp:2781`), con `SCREEN_SCALE_Y(22.f)`/`(14.f)` de sangrado | **CONFLICTO REAL** | **Se pierde**: el borde fijo del 30 % (que el jugador ya tiene). **Se gana**: bordes que se adaptan al aspecto (en 16:9 el borde del mod es 0, en 21:9 hay pillarbox) y bordes animados. Toca además `src/renderer/Draw.cpp:97-106` (el FOV de cinemática). |
| **C4** | FOV de cinemática: `src/renderer/Draw.cpp:96-106` | `Draw.ixx:239-247` (`ConvertFOVforCutscene`) usa `g_cutsceneCameraZoom`, que `Sprite2d.ixx:542` escribe cada frame | El port en cutscene **no convierte** el FOV (`src/renderer/Draw.cpp:100-103`: `ms_fScaledFOV = fov`) | **CONFLICTO REAL** | **Se pierde**: el FOV de cinemática tal cual (4:3). **Se gana**: FOV de cinemática que sigue el zoom del borde. |
| **C5** | Colisión de cámara: `src/core/Camera.cpp` (`AvoidTheGeometry`) | `Main.ixx:27-31` + `:111-113`: convierte el FOV a la inversa (`ConvertFOVInverse`) antes de la colisión, porque la cámara native trabaja en 4:3 | El port pasa el FOV tal cual | **CONFLICTO REAL** | **Se pierde**: el comportamiento actual de la colisión en anchas. **Se gana**: la cámara no "se mete" en las paredes en pantalla ancha. Afecta a la sensación de cámara: **es de las que el jugador nota**. |
| **C6** | HUD anclado: `src/renderer/Hud.cpp` (p.ej. `:666`, `:805`, `:1243`, `:1286`) | `Hud.ixx:33-118` (12 hooks): mete el HUD a la izquierda o a la derecha según el ancho, reordenando el dibujado (`CFont::DrawFonts` se llama en el punto correcto) | El port dibuja el HUD **anclado a la derecha** con `SCREEN_SCALE_FROM_RIGHT` | **CONFLICTO REAL** | **Se pierde**: la posición actual del HUD. **Se gana**: HUDRecentrado en anchas. **Aviso**: esto mueve *todo* el HUD de golpe; es el cambio más visible de la lista. |
| **C7** | Menú: `src/core/Frontend.cpp:2904` (`CMenuManager::DrawBackground`) | `Draw.ixx:111-222` (`MenuCanvas`) + `MenuConstraint.ixx`: el menú se dibuja en un **lienzo 16:9 centrado**, con `RsGlobal->width` reducido y el ratón re-mapeado (`GetMouseX`) | El port usa el raster completo para el menú | **CONFLICTO REAL** | **Se pierde**: el menú a pantalla completa (que el 2.0-B ya verificó como correcto: `docs/mods/ATTRIBUTION.md:320`). **Se gana**: un menú 16:9 con márgenes, que es lo que hacen los mods modernos. |
| **C8** | Escalas del frontend: `src/core/Frontend.cpp` (68 `SCREEN_SCALE` + 45 `SCREEN_STRETCH`) | `Frontend.ixx:667-743`: sustituye 24 punteros de escala por **inversos de resolución** (`ResXInvRefs`/`ResYInvRefs`), uno por clase de frontend, y `Frontend.ixx:751-816` recalcula todo en cada cambio de resolución | El port escala el frontend con `SCREEN_STRETCH_*`/`SCREEN_SCALE_*` | **CONFLICTO DUPLICADO** | La intención es la misma (escalar el menú al ancho); el master lo hace con punteros a float y el port con macros. **Se pierde**: nada (implementación equivalente). **Se gana**: nada nuevo, salvo la corrección de las 6 constantes de Brief/Stats/Leyenda (`Frontend.ixx:745-771`). |
| **C9** | Mapa del menú: `src/core/Frontend.cpp` (dibujo del mapa) | `Frontend.ixx:493-600` (`MenuMap`): lienzo de 448 px de alto, centro propio, 27 escalas + 27 anchos + 7 centros reescritos | El port dibuja el mapa con las macros de escalado | **CONFLICTO REAL** | **Se pierde**: la posición/zoom actual del mapa del menú. **Se gana**: un mapa que no se deforma en anchas. Toca el mismo `CMenuManager` que C7 → **C7 y C9 deben decidir juntos**. |
| **C10** | Texto de script: `src/renderer/Hud.cpp:1466`, `:1820` | `Frontend.ixx:423-489` (`ScriptDraw`): mueve los textos de script a un lienzo 4:3 centrado y reescala `m_fScaleX/m_fScaleY/m_fWrapX/m_fCenterSize/m_fAtX/m_fAtY` (4 patterns) | El port ya centra con `SCALE_AND_CENTER_X` (`src/core/common.h:208`) y escala con `SCREEN_SCALE_*` | **CONFLICTO DUPLICADO** | La intención es la misma. **Se pierde**: nada; **se gana**: nada nuevo. Implementar los dos a la vez = doble desplazamiento. |
| **C11** | Texto: `src/renderer/Font.cpp:909-925`, `src/renderer/Font.h:186`, `src/core/SilentPatchOptions.cpp` | `Frontend.ixx:171-301` (`CFont::PrintOutlinedString`): contorno de **16 direcciones** con alfa compensado (`1-(1-a)^(1/16)`, boost ×2) y fuerza de 0.25/0.5; `Misc.ixx:42-58` anula la sombra del cursor, de la radio y del título de menú | El port tiene `PrintOutlinedString` **declarada y sin usar** (`src/renderer/Font.h:186`); el texto se dibuja con una sombra de 1 desplazamiento (`src/renderer/Font.cpp:909-925`) | **CONFLICTO CON MOD** (14.0 SilentPatch ya tiene su propia capa de sombra/outline en `src/renderer/Font.cpp` y opciones en `src/core/SilentPatchOptions.cpp`) | **Se pierde**: la sombra difusa actual (1 px) si se aplica el contorno. **Se gana**: contorno de 16 direcciones (mucho más limpio en anchas). **Riesgo**: aplicar ambos = doble contorno o alfa mal compensado. Hay que decidir **una sola** de las dos capas. |
| **C12** | Flares/coronas: `src/renderer/Coronas.cpp:374-394`, `:418-419` | `Legacy.ixx:970-974`: escribe `2.0f` en el factor de estirado de las luces de vehículo (`SmallerVehicleCorona`); `Legacy.ixx:616-638`: anula `CBrightLights::RegisterOne` (`NoLightSquare`) | El port dibuja las coronas con `spritew * size * wscale` y escala los flares con `SCREEN_STRETCH_X/Y(1.0f)` (portado del 13.0) | **CONFLICTO REAL (leve)** | **Se pierde**: el tamaño actual de la corona de los coches. **Se gana**: coronas de vehículo más pequeñas (el "sangrado" que el mod arregla). Ojo: `EnableVehicleCoronaFixes` del 14.0 ya resolvió otra cosa (`docs/mods/ATTRIBUTION.md:189`), no la misma. |
| **C13** | Escala de sprites/partículas: `src/renderer/Sprite2d.cpp`, `src/renderer/Particle.cpp` | `Sprite.ixx:13-62`: reescribe `CalcScreenCoors`/`CalcParticleScreenCoors` para corregir el tamaño horizontal en anchas (`SCREEN_SCALE_AR`) | El port escala los sprites con `SCREEN_SCALE_*` y `FIX_SPRITES` (`src/core/config.h:459`) | **CONFLICTO CON MOD** (13.0: `ScriptSpritesScaling` ya portado en `src/renderer/Sprite2d.cpp`; `docs/mods/ATTRIBUTION.md:318`) | **Se pierde**: nada si se omite. **Se gana**: la corrección del ancho de las **partículas** (el 13.0 sólo cubre los sprites de script). Se puede aplicar **sólo** la parte de partículas. |
| **C14** | Carga/streaming: `src/core/Streaming.cpp` | `Loading.ixx:291-387` + `InteriorLoading.ixx:527-737`: saltan la pantalla de carga al cambiar de isla/interior (precargando edificios grandes) | El port deja la pantalla de carga; el 2.0-B lo dejó **PENDIENTE con motivo** (`docs/mods/ATTRIBUTION.md:321`): saltar `LoadScene` rompe el on-demand | **CONFLICTO REAL (riesgo alto)** | **Se pierde**: la pantalla de carga (que el jugador ve y espera). **Se gana**: transiciones sin fundido. **No se debe aplicar sin el diseño D15**: rompería el streaming de datos. |
| **C15** | FOV del juego: `src/core/Cam.cpp` (`CCam::Process_Cam_On_A_String`, `:2123`) | `Misc.ixx:60-89`: hook a esa misma función; FOV += (vel−0.4)·dt con relajación `pow(0.98,dt)` y tope +30° | La cámara SA (`Process_FollowCar_SA`, `src/core/Cam.cpp:6046-6068`) **ya tiene esa fórmula exacta**; la VC nativa (`:2123`) no la tiene | **CONFLICTO REAL** | **Se pierde**: nada hoy (la VC nativa no tiene FOV por velocidad). **Se gana**: FOV que crece al acelerar en la cámara de VC. **Ojo**: si se aplica también en la cámara SA se duplica el término → FOV disparado. |
| **C16** | FOV: `src/renderer/Draw.cpp:96-106` | `Draw.ixx:259-294` (`FOVManager`): multiplicadores de FOV por hash, aplicados en `SetFOV` | El port no tiene multiplicador | **SIN CONFLICTO** (aditivo) | **Se pierde**: nada. **Se gana**: una API para que otros mods ajusten el FOV. Pero **toca el mismo `SetFOV`** que C4 → aplicar C4 y C16 en el mismo sitio. |
| **C17** | Ratón del menú: `src/core/Frontend.cpp` | `Draw.ixx:156-162` (`GetMouseX`): recorta la X del ratón al lienzo del menú | El port usa el ratón sobre el raster completo | **CONFLICTO REAL** (sigue de C7) | Sólo aplica si se aplica C7. |
| **C18** | Subtítulos: `src/renderer/Hud.cpp:1500-1550` | `Frontend.ixx:102-138`, `:1113-1145`: escala del subtítulo, reserva del radar y subida del texto | El port dibuja el subtítulo a tamaño fijo (`src/renderer/Hud.cpp:1532`) | **CONFLICTO REAL (leve)** | **Se pierde**: la posición/tamaño actual del subtítulo. **Se gana**: subtítulos que no chocan con el radar y que se adaptan a la barra de cinemática. |
| **C19** | `RsGlobal`/ventana | `Main.ixx:47-55` + `Legacy.ixx:473-498`: fuerza la resolución del escritorio por defecto | El canvas del navegador manda | **NO APLICA** | — |
| **C20** | Post-proceso | `PostFX.ixx`, `MSAA.ixx`, `TransparentMenu*.ixx`: gamma de consola, SMAA, MSAA forzado, menú translúcido | El port renderiza con librw/WebGL2 | **NO APLICA** (D3D8/D3D9) | — |

### 3.2 · Recuento

| Veredicto | Nº | IDs |
|---|---|---|
| **CONFLICTO REAL** | **13** | C1, C2, C3, C4, C5, C6, C7, C9, C12, C14, C15, C17, C18 |
| **CONFLICTO CON MOD** | **2** | C11 (14.0 SilentPatch), C13 (13.0 Extended) |
| **CONFLICTO DUPLICADO** | **2** | C8, C10 |
| **SIN CONFLICTO / A PORTAR** | 3 | C16, y el resto de §2.2 |
| **NO APLICA** | 4 | C19, C20, + features de §2.2 marcadas NO APLICA |

### 3.3 · TABLA FINAL — lo que hay que avisar al jugador ANTES de aplicar

Ordenado por impacto visual. **Todos estos cambian algo que el jugador ya tiene delante.**

| Prioridad | ID | Qué se aplica | **Qué se pierde** | **Qué se gana** | Dónde |
|---|---|---|---|---|---|
| 1 | **C6** | HUD anclado a los bordes (`Hud.ixx`) | El HUD se mueve de sitio **entero** | HUD centrado en pantallas anchas | `src/renderer/Hud.cpp` |
| 2 | **C7 + C9 + C17** | Menú en lienzo 16:9 + mapa con canvas 448 + ratón re-mapeado (`Draw.ixx:111-222`, `Frontend.ixx:493-600`) | El menú deja de ocupar la pantalla completa | Menú 16:9 con márgenes y mapa sin deformar | `src/core/Frontend.cpp`, `src/core/MenuScreensCustom.cpp` |
| 3 | **C3 + C4** | Bordes de cinemática adaptativos y animados (`Legacy.ixx:238-303`, `Sprite2d.ixx:507-545`) | El borde fijo del 30 % | Bordes que se adaptan al aspecto, con animación | `src/core/Camera.cpp:2786`, `src/renderer/Draw.cpp:97` |
| 4 | **C5** | FOV inverso en `AvoidTheGeometry` (`Main.ixx:27-31`) | La colisión de cámara actual | La cámara no se mete en las paredes en anchas | `src/core/Camera.cpp` |
| 5 | **C2** | Tamaño del radar 97.5×89 (`Frontend.ixx:1056-1111`) | El disco del radar actual (82×82 / 94×76) | Radio configurable | `src/core/Radar.h:181-182`, `src/core/Radar.cpp:1577` |
| 6 | **C1** | Multiplicador de escala del HUD 0.8 (`Frontend.ixx:1000-1053`) | El HUD tal cual está | HUD configurable en X e Y | `src/renderer/Hud.cpp`, `src/core/common.h:201` |
| 7 | **C11** | Contorno de texto de 16 direcciones (`Frontend.ixx:171-301`) | La sombra difusa de 1 px **y** la capa de texto del 14.0 | Contorno limpio, sin sombra difusa | `src/renderer/Font.cpp`, `src/renderer/Font.h:186` |
| 8 | **C15** | FOV por velocidad en la cámara VC (`Misc.ixx:60-89`) | Nada (hoy no está) | FOV que crece al acelerar — **cuidado con duplicarlo en la cámara SA** | `src/core/Cam.cpp:2123` |
| 9 | **C13** | Escala de partículas en anchas (`Sprite.ixx:34-44`) | Nada (sólo toca partículas) | Partículas con el ancho correcto | `src/renderer/Particle.cpp` |
| 10 | **C12** | Coronas de vehículo más pequeñas (`Legacy.ixx:970-974`) | El tamaño actual de la corona de los coches | Sin sangrado de corona | `src/renderer/Coronas.cpp:374-394` |
| 11 | **C18** | Subtítulos escalados y reposicionados (`Frontend.ixx:102-138`) | La posición/tamaño actual del subtítulo | Subtítulos que no invaden el radar | `src/renderer/Hud.cpp:1500-1550` |
| 12 | **C14** | Sin pantalla de carga entre islas (`Loading.ixx`, `InteriorLoading.ixx`) | La pantalla de carga | Transiciones sin fundido — **no aplicar sin el diseño D15** | `src/core/Streaming.cpp` |
| 13 | **C8 + C10** | Escalas del frontend por clase + texto de script centrado | Nada | Corrección de las 6 constantes de Brief/Stats/Leyenda | `src/core/Frontend.cpp`, `src/renderer/Hud.cpp:1466` |

**Recomendación de orden** (si el jugador dice "aplica todo"): empezar por los **sin impacto visual**
(C16 `FOVManager`, C13 partículas, C12 coronas, C2 radar, C1 HUD, C18 subtítulos) y dejar los
que **mueven la pantalla** (C6, C7/C9/C17, C3/C4) para el final, con el jugador mirando.

---

## 4 · Verificación del master contra el retail 1.0

### 4.1 · Método

Escaneo propio sobre `gta-vc.exe` (sha256 `acc6a54681b3c5ce996c956b71be9e4c1315c3b66aea37b896afc913fe28e868`,
3 088 896 B, image base `0x400000`), con el **mismo algoritmo que el mod** (`?` = comodín, 2 dígitos
hex por byte). Se recorren **todas** las secciones; se mide el número de coincidencias por patrón
y, para los patrones largos, el desglose `.text` vs resto.

### 4.2 · Resultado global

| Métrica | Valor medido |
|---|---|
| Patrones del master escaneados | **324** |
| Patrones con **≥ 1** coincidencia en el retail 1.0 | **324 / 324** |
| Patrones con **0** coincidencias | **0** |
| Coincidencias totales (suma de todos los patrones) | **84 132** (de las cuales **82 655** en `.text`) |

Los dos patrones más cortos (`E8 ? ? ? ?` y `E8 ? ? ? ? ?`)_account por ~80 000 de esas
coincidencias y no son útiles como indicador.

### 4.3 · El master deriva patrones cortos de los largos: diferencia de sitios (medido)

El master **no** reusa los patrones largos: los usa para **extraer una dirección** (`get<uint32_t*>(2)`)
y construir un patrón **corto y único** `D8 0D <direccion>`. Medido sobre el retail 1.0:

| Patrón derivado (master) | Padre largo | Sitios del padre | **Sitios del derivado** | Δ |
|---|---|---|---|---|
| `DrawHudHorScalePattern` | `D8 0D ? ? ? ? D8 ? ? ? ? ? D9 1C 24 E8` | 51 | **87** | **+36** |
| `DrawHudVerScalePattern` | `D8 0D ? ? ? ? D8 8D ? ? ? ? D8 0D` | 1 | **74** | **+73** |
| `CRadarPattern` | `D8 0D ? ? ? ? 83 EC 50 DD D9` | 1 | **34** | **+33** |
| `CDarkelDrawMessagesPattern` | `D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24` | 105 | **8** | −97 |
| `CDarkelDrawMessagesPattern2` | `50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50` | 26 | **8** | −18 |
| `CParticleRenderPattern` | `D8 0D ? ? ? ? DD DA D9 43 3C D8 4C 24 34 D8 CA` | 2 | **8** | +6 |
| `CSpecialFXRender2DFXsPattern` | `D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59` | 22 | **2** | −20 |
| `CSceneEditDrawPattern` | (mismo padre, otra ocurrencia) | 22 | **1** | −21 |
| `sub61DEB0Pattern` | `D8 0D ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 DD DA D9 C1 D9 5C 24 24` | 1 | **6** | +5 |
| `MenuPattern15625` | `89 04 24 DB 04 24 D8 0D` | 38 | **67** | +29 |

Consecuencias medidas:

1. **El patrón derivado puede cubrir MUCHOS más sitios que el padre** (HUD-H: 87 vs 51; HUD-V: 74 vs 1).
   El master, por tanto, escala **más** elementos del HUD que la build instalada, que usaba el patrón
   largo (51 sitios en retail 1.0).
2. **Esto arregla un bug de la build instalada** (riesgo R13 del recibo previo, ya cerrado):
   `Legacy.ixx:878-879` escribe en los índices **66** y **67** de la lista (los dos slots del radar).
   En la build instalada esos índices **no existen** en retail 1.0 (la lista larga tiene 51 sitios),
   así que allí los dos slots del radar **sí** reciben la escala del HUD; en el master (87 sitios)
   **sí** existen y quedan excluidos. **Comportamiento distinto entre build y master en retail 1.0.**
3. `MenuPattern15625` (master `Legacy.ixx:388`) usa `get<uint32_t*>(8)` sobre el patrón
   `89 04 24 DB 04 24 D8 0D`: el offset 8 es el **byte de opcode** `0D`, no el displacement (que
   empieza en el 9). La dirección extraída es basura (`0x0068C350` con el byte `0D` pegado delante),
   el patrón derivado resultante `D8 0D 0D 50 C3 68` tiene **0 sitios** en el retail y la sub-feature
   "MenuLotsOfStuffScale" **queda muerta** en el master. (Medido con la expresión exacta del master.)

### 4.4 · Qué features del master **sólo** funcionan en el retail 1.0 y cuáles **no**

Todas las features **por patrón** funcionan en retail 1.0 (324/324). Pero dos caminos de código
del master **muere** en el retail 1.0 por una guarda de cantidad de coincidencias:

| Código del master | Guarda | Coincidencias en retail 1.0 | Resultado |
|---|---|---|---|
| `Legacy.ixx:705-706` y `:1134-1135` — escala horizontal del **texto de replay** | `pattern.count(3).get(2)` sobre `D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC` | **1** (se requieren 3) | **NO FUNCIONA** en retail 1.0: el índice 2 no existe → el texto del replay **no** se reescala |
| `Frontend.ixx:620` (`MenuBorders::Init`) | `last.size() != 1` sobre `DE D9 DE D9 E8 ? ? ? ? 83 C4 24 80 3D ? ? ? ? 00` | **2** | **NO FUNCIONA** en retail 1.0: `MenuBorders::Init` sale antes de instalar nada → **las máscaras del menú no se escalan** |

Además, un caso de **índice mal usado pero inofensivo**:

| Código del master | Guarda | retail 1.0 | Nota |
|---|---|---|---|
| `Legacy.ixx:1133` (`pattern.get(34)` sobre `50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05`) | — | 38 sitios | OK (34 < 38) |
| `Legacy.ixx:388` (`MenuPattern15625`) | offset equivocado | 0 sitios | Sub-feature muerta (§4.3) |

Y un caso en el que **la build instalada falla y el master no** (complemento de R13):

| Build instalada | retail 1.0 | Master | retail 1.0 |
|---|---|---|---|
| `DAT_1004b338` (lista de 51 sitios) con exclusiones `{i>3, i≠20, i≠66, i≠67, i≠15, i≠16, i≠17}` | los índices 66/67 **no existen** → los slots del radar **sí** se escalan con el HUD | `DrawHudHorScalePattern` (87 sitios) con las mismas exclusiones | los índices 66/67 **existen** → los slots del radar **no** se escalan |

### 4.5 · Direcciones del retail 1.0 medidas (las que el master da como commentarios)

El master trae comments con direcciones **del build de Steam**; en el retail 1.0 son distintas.
Medido:

| Qué | Dirección en el master (comment) | **Medido en retail 1.0** |
|---|---|---|
| `dwGameLoadState` | `0x60041D` | `0x60043D` (el recibo previo ya lo midió) |
| `RsGlobal` | `0x9B48D8` | `0x9B48E0` |
| `pfScreenAspectRatio` | `0x94DD38` | **`0x94DD40`** |
| `pfScreenFieldOfView` | `0x696658` | **`0x696658`** ✓ |
| `bWideScreen` | `0x869652` | `0x869652` ✓ |
| `bIsInCutscene` | `0x7E46F5` | **`0x7E46FD`** |
| `bIsInCutscene` (Legacy) / `bBackgroundOn` | `0x97F83B` | — |
| `bDropShadowPosition` | `0x97F860` | — |
| `bFontDropColorA` | `0x97F865` | — |
| `font94B924` | `0x94B924` | — |
| `CMenuManager_m_PrefsLanguage` | `0x869680` | — |
| FOV scale (FOVControl) | `0x576686` (comment del INI) | **`0x5766A6`** |
| Radar radius | — | **`0x974BF4`** |
| `FUN_004bc200` (predicado "en vehículo") | — | **`0x4BC200`** |
| `cam->FOV = 70.0f` (hook del master) | `0x47FF22` | **`0x47FF22`** ✓ (1 coincidencia) |

**Conclusión de §4**: el master es **más robusto** que la build instalada frente al retail 1.0
(patrones largos → derivados, 87 vs 51 sitios de HUD; `SetColorAddr` con la variante `8B 5C`
(`Legacy.ixx:432`) que **sí** resuelve en retail 1.0 con 2 coincidencias, frente a los **0** de la
variante `8B 5D` de la build instalada — riesgo R4 del recibo previo). Pero tiene **dos features
muertas** en retail 1.0 (escala del replay y máscaras del menú), y **un bug de índice** que deja
muerta la escala de "muchas cosas del menú".

---

## 5 · Metodología y reproducibilidad

| Qué | Cómo |
|---|---|
| Descripción del ASI (formato, secciones, exports) | Parser PE propio + `tools.rea.open_binary` (sha256 idéntico: `83d5113e19a7b2bbf2217c58fb3ccd41b2a27f38638b956d469790c555aad33b`) |
| Decompilación del ASI | REA (Ghidra 12.1.4 headless, perfil propio): `procedure_pseudo_code` / `procedure_assembly` sobre `0x10009CB0`, `0x10002180`, `0x10009C70`, `0x1000CF00`, `0x10006A70`, `0x1001F170`, `0x1001EE80` |
| Decompilación del retail | Ghidra headless sobre el proyecto **ya existente** `gta_vc_browser/tmp/rea/ghidra_vanilla` (programa `/gta-vc.exe`), con un script propio (`R2Dec.java`, en el temporal) que descompila la función que contiene una dirección dada. Direcciones: `0x47FE8C`, `0x4BC040`, `0x4BC200`, `0x4BC230`, `0x54A300`, `0x4C5C80` |
| Bytes del retail | Parser PE + lectura directa (constantes `0x68FEFC`…`0x68FF0C`) y `capstone` para desensamblar |
| Patrones del master | Extracción propia sobre `source/GTAVC.WidescreenFix/*` con `hook::pattern(...)` y `pattern_str(...)`; 368 llamadas, 333 literales directos + 5 literales incrustados = **324 únicos normalizados** |
| Contraste master ↔ binario instalado | Comparación exacta normalizada (`??`→`?`, mayúsculas) contra los 106 patrones de `gta_vc_browser/tmp/extsrc/wfp_binary_patterns.txt` |
| Escaneo del retail | Script propio (regex con `?` → `.`), todas las secciones, con el mismo criterio que el mod |
| Patrones derivados | Script propio: se aplica la expresión exacta del master (`count(N).get(M).get<uint32_t*>(K)`) sobre el retail y se cuenta el patrón `D8 0D <dir>` resultante |
| Estado del port | Lectura directa de `src/**` con número de línea (todos los `fichero:línea` citados están medidos) |
| Mods 13.0/14.0/15.0 | `docs/mods/ATTRIBUTION.md` §2, §5, §9, §13 y `src/core/SilentPatchOptions.cpp` |

> Nota de método: **la sesión REA re-enlaza el documento abierto entre llamadas** (comprobado: un
> `procedure_pseudo_code` con `document:` explícito falló con *"active program:
> target-83d5113e19a7.asi"*). Todo lo "medido en el binario" de este recibo se obtuvo con
> scripts propios sobre el fichero, o con Ghidra headless en un proceso aparte.

---

## 6 · Conclusión

1. **La fórmula de `CarSpeedDependantFOV` de la build instalada está medida** y es
   `FOV = AdjustFOV(f, AR)·FOVFactor + (radio_del_radar − 180) / CarSpeedDependantFOV`, con el proxy
   de velocidad tomado del **auto-zoom del radar** del retail (180→350 px entre velocidad 0.3 y 0.9).
   El recibo previo lo dejó como `supuesto` (riesgo R7): **cerrado**.
2. **La fórmula del master es otra** (hook a `CCam::Process_Cam_On_A_String`, `(v−0.4)·dt` con
   relajación `pow(0.98,dt)` y tope +30°), y **el port ya la tiene escrita** en
   `src/core/Cam.cpp:6046-6068`. Lo que falta es extenderla a la cámara nativa de VC
   (`src/core/Cam.cpp:2123`). **La fórmula de la build instalada no conviene portarla**: depende
   del radio del radar y tiene un bug de rango de velocidad medido.
3. El master tiene **228 patrones** que la build instalada no tiene, repartidos en **22 módulos**,
   que se traducen en **37 features posteriores**; de ellas **8 son NO APLICA** (D3D8/D3D9/MSAA/
   ventana nativa/x86/arranque), **2 son PENDIENTE con motivo ya decidido** (D15) y **11 tocan
   zonas ya portadas** con conflicto.
4. **13 CONFLICTO REAL + 2 CONFLICTO CON MOD + 2 CONFLICTO DUPLICADO**. Los que hay que decidir
   con el jugador antes de aplicar son los de la tabla §3.3 (13 filas), de los que los 6 primeros
   **mueven la pantalla**.
5. Contra el **retail 1.0**: los **324** patrones del master **coinciden**; dos caminos de código
   del master **no funcionan** (escala del replay, máscaras del menú) y uno queda **muerto** por un
   índice mal calculado (`MenuPattern15625`).
