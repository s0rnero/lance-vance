# T0d · RE corta del retail (plan 16.0)

> Las dos RE que la decision **D9/D10** exigian antes de escribir codigo. Metodo: escaneo de
> bytes con mascara sobre `gta-vc.exe` (sha `acc6a546...`, el mismo algoritmo del mod) +
> proyecto Ghidra ya analizado del retail (`gta_vc_browser/tmp/rea/ghidra_vanilla`).

## 1. El FOV de emergencia (D9) — que camara es y que formula

### Lo medido en el retail

| Que | Medida |
|---|---|
| Patron | `D9 9B F4 00 00 00 D9 83 F0 00 00 00` |
| Coincidencias en el retail 1.0 | **1** (unica) |
| Direccion | **0x46DEB7** (la misma que da el master) |
| Instruccion | `FSTP dword ptr [EBX + 0xF4]` = **guarda** un float en la camara |
| Funcion que la contiene | `FUN_0046C58C` (7092 B) = **`CCamera::Process`** |
| Equivalente en el port | **`src/core/Camera.cpp:283` `CCamera::Process(void)`** (la misma que llama a `CDraw::SetFOV(FOV)` en `:608`) |
| De donde sale el valor | `[EBX + 0xF0]`, con minimo **3.6f** (`0x400CCCCD`) y multiplicado despues por el global `0x690220` |

Desensamblado medido (Ghidra, rango `0x46DE92`-`0x46DEC9`):

```
0046de92 FLD float ptr [EBX + 0xf0]
0046de98 FCOMP float ptr [0x0068aec8]
0046dea3 JNZ 0x0046deaf
0046dea5 MOV dword ptr [EBX + 0xf0],0x400ccccd
0046deaf FLDZ
0046deb1 FLD float ptr [EBX + 0xf0]
0046deb7 FSTP float ptr [EBX + 0xf4]      <-- el store que parchea el mod
0046debd FLD float ptr [EBX + 0xf0]
0046dec3 FMUL float ptr [0x00690220]
0046dec9 FSTP float ptr [EBX + 0xf0]
```

El master (`Legacy.ixx:202`) engancha `CDraw::SetFOV` (retail `0x54A2E0`) y calcula
`fEmergencyVehiclesFix = 70.0f / fFactor`; despues sustituye el store de `0x46DEB7` por ese
valor (`Legacy.ixx:536-538`).

### La adaptacion al port

El retail pasa a `SetFOV` un **factor** (1,0 = normal); el port pasa el **angulo**
(`src/renderer/Draw.cpp:97 CDraw::SetFOV(float fov)`, `ms_fFOV = fov`). Con el angulo, el
factor del retail es `fov / 70`, luego:

```
70 / factor  =  70 / (fov / 70)  =  DefaultFOV * DefaultFOV / fov
```

`DefaultFOV` son los 70 grados del motor (`src/core/Camera.h:32`).

**Lo que NO se ha medido**: a que rama de `CCamera::Process` pertenece el store (el retail
funciona por dentro y el port reescribio esa funcion en varias). Por eso la implementacion
del port es la de la formula, **no** un parche de una direccion, y **detras de su opcion**
(`g_bWSPOptionEmergencyFOV`, apagada de serie): con la opcion apagada no cambia nada, y el
jugador puede ver el efecto y decidir.

## 2. El salto de la colision de camara con AR >= 2 (D10)

### Lo medido en el retail

| Que | Medida |
|---|---|
| Patron del master | `E8 ? ? ? ? D9 EE D9 EE D9 44 24 38 D8 A3 7C 01 00 00 DD D9` |
| Coincidencias | **2** (en `0x480456` y `0x4855A9`); el master usa `count(1).get(0)`, o sea **la primera** |
| Llamada parcheada | `0x480456` = `CALL 0x00473AA4` |
| `0x473AA4` | `FUN_00473aa4` (2846 B) = **`CCamera::AvoidTheGeometry`** |
| Funcion que contiene la llamada | `FUN_0047FE8C` (3403 B) = **`CCam::Process_Cam_On_A_String`** (llama a `RotCamIfInFrontCar` = `0x4758C6`, la misma que cita el plan 15.0) |
| Que hace el master | Si el aspecto es **>= 2.0**, el hook **no llama** a `AvoidTheGeometry` (`ret 10h`); si no, la llamada normal |

### La adaptacion al port

El retail tiene **una** llamada; el port (reVC) la partio en un `if/else` segun la linea de
vision. Equivalent **medido**: `src/core/Cam.cpp:2177-2180`, dentro de
`CCam::Process_Cam_On_A_String`:

```cpp
if(CWorld::GetIsLineOfSightClear(CamTargetEntity->GetPosition(), m_cvecTargetCoorsForFudgeInter, true, false, false, true, false, false, true))
    TheCamera.AvoidTheGeometry(OrigSource, m_cvecTargetCoorsForFudgeInter, Source, FOV);
else
    TheCamera.AvoidTheGeometry(OrigSource, CamTargetEntity->GetPosition(), Source, FOV);
```

El descompilado del retail confirma la forma 1:1: `FUN_004da580` es la comprobacion de linea
de vision y el destino de la llamada se elige con un `if/else` **antes** de la llamada.

**Implementacion**: con la opcion `g_bWSPOptionSkipCameraCollision` encendida y el aspecto
**>= 2.0**, se salta **la llamada entera** (las dos ramas), que es lo que hace el master.

## 3. Resumen

| Item | Estado |
|---|---|
| FOV de emergencia (D9) | Medido: el store esta en `CCamera::Process` (retail `0x46C58C`); la formula del master es `DefaultFOV^2 / fov` en el port. La rama exacta de camara **no** esta medida -> se documenta |
| Salto de la colision con AR >= 2 (D10) | Medido: `CCam::Process_Cam_On_A_String`, `src/core/Cam.cpp:2177-2180`; se salta la llamada completa con aspecto >= 2.0 |
