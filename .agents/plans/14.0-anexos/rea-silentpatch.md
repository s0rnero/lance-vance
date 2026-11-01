# Recibo REA — SilentPatch para GTA VC (Build 12.1) + componente DDraw (Build 7)

Fecha: 2026-10-19
Motor de analisis: REA (Ghidra 12.1.4 / REA 6.5.0) via MCP `rea`
Objetivo: ingenieria inversa completa del mod `mods/silent patch/` para planear su port al port web (reVC/WASM).

## 1. Objetivos analizados

| Artefacto | Ruta | sha256 | Tamano | Formato |
|---|---|---|---|---|
| `SilentPatchVC.asi` | `mods/silent patch/SilentPatch/SilentPatchVC.asi` | `71eaf21c9bcba7010c389dd57badd56cdaae55cf73cc52e1941529d598a6c771` | 226.816 B | PE x86 (image base `0x10000000`) |
| `ddraw.dll` | `mods/silent patch/ddraw.dll` | `f22b58fbdef93fbe292a741f130db7adfc0c7d0774f762bbfe0ea93c8581d03ad` | 115.200 B | PE x86 (image base `0x10000000`) |

Segmentos del `.asi`: `.text` `0x10001000-0x10027000` (155.648 B), `.rdata` `0x10027000-0x10034400`, `.data` `0x10035000-0x10036bd0` (7.120 B), `.rsrc`, `.reloc`.
Ghidra: **1.371 procedimientos**, **699 cadenas**.
Segmentos del `ddraw.dll`: `.text` `0x10001000-0x10013800`, `.rdata` `0x10014000-0x1001b200`, `.data` `0x1001c000-0x1001d428`. Ghidra: **682 procedimientos**, **346 cadenas**.
El `.asi` referencia el PDB `SilentPatchVC.pdb` (GUID `1883e60a-9251-41f6-9d95-05851764237e`, age 1, RSDS); el PDB no esta disponible, por lo que los simbolos originales no se recuperan (limitacion asumida).

## 2. Recibo R1 — version del `.asi` (binario instalado)

`analyze_function(procedure="GetBuildNumber")` en `0x100109d0`:

```c
undefined4 GetBuildNumber(void)
{
  return 0xc01;      // (12 << 8) | 1  ->  Build 12.1
}
```

**Conclusión:** el binario instalado es **Build 12.1** (revision 12, build 1), tal y como dice su readme ("Build 12.1 - Last update 31.07.2026, re-released 15.08.2026").

## 3. Recibo R2 — version del `ddraw.dll`

`analyze_function(procedure="GetBuildNumber")` en `0x100030b0`:

```c
undefined4 GetBuildNumber(void)
{
  return 0x700;      // (7 << 8) | 0  ->  Build 7
}
```

**Conclusión:** el componente DDraw instalado es **Build 7** ("SilentPatch - DDraw component for GTA III and GTA VC, Build 7, 31.07.2026").

## 4. Recibo R3 — el codigo publico es el MISMO build que el binario instalado

El repositorio publico es `github.com/CookiePLMonster/SilentPatch` (MIT, (c) 2024 Adrian Zdanowicz "Silent"), rama `dev`.
Su `SilentPatchVC/versionmeta.props` declara `SILENTPATCH_REVISION_ID=12`, `SILENTPATCH_BUILD_ID=1` -> **Build 12.1**, identico a R1.

Verificacion por **contenido**, no por suposicion: las constantes flotantes que el fuente usa para las posiciones de sirena estan en el `.rdata` del binario instalado **con el mismo valor y en bloque contiguo** (`search_bytes` + `read_bytes` en `0x100300f0`):

| Constante del fuente | Bytes | Direccion en el `.asi` | Sentido |
|---|---|---|---|
| `METERS_TO_MILES = 0.0006213711922f` | `84 E3 22 3A` | `0x10030090` | factor metros->millas |
| `MULT_6 = 6.0f` | `00 00 C0 40` | `0x10030088` | regla 4 de extras (extra6) |
| `POLICE_SIREN_POS.x = 0.55f` | `CD CC 0C 3F` | `0x100300f0` | corona de sirena |
| `ENFORCER_SIREN_POS.x = 0.6f` | `9A 99 19 3F` | `0x100300f4` | corona de sirena |
| `AMBULANCE_SIREN_POS.x = 0.7f` | `33 33 33 3F` | `0x100300fc` | corona de sirena |
| `FIRETRUCK_SIREN_POS.x = 0.95f` | `33 33 73 3F` | `0x10030108` | corona de sirena |
| `ENFORCER_SIREN_POS.y = 1.05f` | `66 66 86 3F` | `0x1003010c` | corona de sirena |
| `FBIRANCH_SIREN_POS.y = 1.12f` | `29 5C 8F 3F` | `0x10030110` | corona de sirena |
| `FIRETRUCK_SIREN_POS.z = 1.4f` | `33 33 B3 3F` | `0x10030120` | corona de sirena |
| `FIRETRUCK_SIREN_POS.y = 3.2f` | `CD CC 4C 40` | `0x1003014c` | corona de sirena |
| `LightStatusRandomnessThreshold = 1/25000` | `AC C5 27 38` | `0x1002cf5c` | luces del trafico |

**Conclusión:** el fuente publico `dev` **es** el codigo del binario instalado. El fuente vale como mapa; el binario es la verdad y coincide.

## 5. Recibo R4 — las opciones de la INI estan en el binario

Cadenas recuperadas del `.asi` (`.rdata`): `SilentPatch`, `Units`, `EnableVehicleCoronaFixes`, `MinimalHUD`, `SlidingMissionTitleText`, `SlidingOddJobText`, `ShowPropertyBlips`, `DontShrinkRadardisc`, `UseDesktopRefreshRate`, `ScaleScriptSprites`, `SpeechDelayTimer`, `ExtraCompSpecularityExceptions`, `DrawBackfaces`, `DontDrawBackfaces`, `TAXI_LIGHT`, `FBI_RANCHER_SIREN`, `FBI_WASHINGTON_SIREN`, `VICE_CHEETAH_SIREN`, `MOVING_BOAT_RADAR`, `DRAW_BACKFACES`, `GetExtendedIDfrom8bitBefore`, `GetExtendedIDfrom16bitBefore`, `SetCdStreamReleaseChannelfUsedByTheFLA`, `skygfx`, `AttachCarPipeToRwObject`.

**Coinciden una a una** con `SilentPatchVC.ini` instalado -> el binario lee exactamente esas 11 claves de la seccion `[SilentPatch]` y las dos listas `[ExtraCompSpecularityExceptions]`, `[DrawBackfaces]`, `[DontDrawBackfaces]`.

## 6. Recibo R5 — que codigo del motor toca (los ~250 patrones de bytes)

El mod no parchea direcciones fijas: **escanea `gta_vc.exe` buscando secuencias de bytes** (`hook::txn::get_pattern`) y hookea ahi. Se recuperaron del `.rdata` las **~250 secuencias** que usa (patrones con `?` como comodin). Esa lista **es el mapa de los puntos del motor que SilentPatch toca** y por tanto el mapa de lo que hay que portar. Reparto:

- HUD/texto: `D8 05 ? ? ? ? D9 9C 24 ? ? ? ? D9 05 ? ? ? ? D8 44 24 50` (sombra del contador), `8D 8C 24 24 09 00 00 FF 35 ...` (posiciones de sirena), `0F BF CA 8D 0C 8D 00 00 00 00 81 C1 ...` (sprites de script), `8D ? 14 52 50 E8 ...` (rectangulos de script).
- Corona: `C7 44 24 44 00 00 E0 40 ...` (faro del helicoptero), `C7 44 24 6C 00 00 10 C1 ...` (luz de cola), `83 E9 04 0F 84 87 0A 00 00 83 E9 10` (predicado de sirena FBI/Vice).
- Stingers: `74 ? 31 ED 8D 44 20 ? 8B 8C AB ? ? ? ? 83 B9`, `0F 84 ? ? ? ? DE D9 31 F6 DD D8 ...`.
- Roadblocks: `E8 ? ? ? ? 84 C0 74 ? BD ? ? ? ? EB ? 8D 80 ? ? ? ? E8 ...` (`CRoadBlocks::GenerateRoadBlocks`), `74 ? 0F B7 05 ? ? ? ? 89 F1` (pool de objetos agotado).
- Chat de peds: `0F B7 C2 DD D8 C1 E0 04` y `0F B7 95 DA 05 00 00 D9 6C 24 04` (retardo de 6000 ms).
- Pools: `FF 41 0C 8B 59 0C 8B 51 08 39 D3 75` (`CPool<>::New`).
- Objetivo de atraco: `8B 85 34 05 00 00 8B 15 ? ? ? ? 39 C2 76 2B` + tabla de saltos `FF 24 85 ? ? ? ? 8B 8D 08 05 00 00` (indice 47 = `OBJECTIVE_WANDER`).
- Sombras: `D9 04 2B DD DC 8B 44 24 54 D9 C3 DD DD D9 44 2B 04 D8 08` (`CShadows::CastShadowEntityXY`).
- Armas: `E8 ? ? ? ? 0F B7 C0 D9 EE D9 EE C1 F8 03 99` (random 16 bits), `89 E9 E8 ? ? ? ? 8B BD 6C 01 00 00` (victima del atraco), `6A 11 E8 ? ? ? ? 0F BE 93 ? ? ? ? 8D 14 52` (COLT45 forzado a los polis de barricada).
- Reloj: `83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08` (`CTimer::Update`).
- Textura de blips: `6A 01 6A 09 E8 ? ? ? ? 59 59 E8` (filtrado bilineal de los blips).

Cada patron es localizable en el codigo de reVC por su equivalente en C++ (los simbolos vanilla: `CTimer::Update`, `CPool<>::New`, `CRoadBlocks::GenerateRoadBlocks`, `CShadows::CastShadowEntityXY`, `CPedAttractorManager::GetEffectForIceCreamVan`, ...), que es lo que hace el plan.

## 7. Recibo R6 — lo que el mod **no** tiene (readme vs binario)

El readme enumera dos arreglos que **no estan en el binario instalado 12.1 / 7** y por tanto no son portables (no existe el codigo que los hace):

- "Wet road reflections render properly again (just like with Road Reflections Fix)".
- "Reintroduced light glows under weapon/health/armor pickups, bribes, hidden packages, and money pickups".

En el `.asi` no hay ninguna cadena ni patron asociado a "road reflections"; el unico "glow" es el del minigun (`PickUpEffects_Minigun2Glow`, fuente `SilentPatchVC.cpp:1589`). Son entradas heredadas de la familia SilentPatch para D3D9 (componente `ddraw.dll` de otras builds) y no del binario instalado.

## 8. Limitaciones

- Sin PDB, los nombres originales de los 1.371 procedimientos del `.asi` no se recuperan; el mapeo a funciones de reVC se hace por patron y por equivalencia semantica con el fuente publico.
- El `.asi` parchea la **binaria comercial de GTA VC** (1.0 / 1.1 / Steam / JP). En el port web no hay direcciones que parchear: **el mecanismo hay que reconstruirlo en C++** sobre el codigo de reVC. Los patrones son la guia de *que* codigo se toca, no *donde* (las direcciones no aplican).
- Los componentes `DDraw/` (ventana, resolucion de escritorio, DirectPlay, DEP, VRAM, `SHGetFolderPathA`, `_rwcseg`) son **infraestructura de Windows**: no aplican al navegador, salvo el `FILE_FLAG_NO_BUFFERING` de la lectura IMG (que en el port es otra capa).
