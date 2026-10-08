---
name: 06-fuentes-externas-y-mapeo
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 06 — Fuentes públicas, CLEO-lite en profundidad y mapeo «dónde y cómo»

Tercera pasada. Tercera pasada. Objetivo del usuario: **nutrir los % del `05` con código real ya
escrito** (autores de los mismos mods), y dejar comparado contra nuestro código **dónde y cómo**
hacer cada mejora, **qué evitar y por qué**. Todo verificado con `web_search` + GitHub API el
24/09/2026.

---

## 1. Repositorios encontrados (por mod)

| Mod | Repo público | Licencia ✅/⚠️ | Qué tomar exacto |
|---|---|---|---|
| **SilentPatch** (III/VC/SA) | `github.com/CookiePLMonster/SilentPatch` (rama `dev`) | **MIT** ✅ (cópia con atribución) | `SilentPatch/*.cpp` + `CHANGELOG-VC.md` + `Config/SilentPatchVC.ini`; el fix de VC se implementa con patrones multi-versión (mira `MemoryMgr.GTA.h`, `RWGTA.cpp`) |
| **SkyGfx III/VC** | `github.com/aap/skygfx_vc` | **SIN LICENSE** ⚠️ (404 en /license) — referencia/reimplementar, no cópia literal | `src/matfx.cpp` (27 KB: dual-pass + env map), `src/leedsCarpipe.cpp` (18 KB: tubería vehículo PS2/Xbox), `src/leeds.cpp`, `src/WaterLevel.cpp` (= ps2Water), `src/main.cpp` (33 KB: pipes + INI), `shaders/` (25 HLSL: `curvePS`/`gradingPS` = ColorFilter, `rimVS/glossPS`, `vehiclePass1/2VS`, `ps2StandardEnvVS`, `vc_worldPS`) |
| **Classic Axis** (III/VC) | `github.com/gennariarmando/classic-axis` | Sin LICENSE en README ⚠️ — referencia | `source/CamNew.cpp` (17 KB: cámara IV/apuntado), `source/Main.cpp` (57 KB: ganchos + `bForceLegsMovements`), `source/Settings.*`, `vendor/GInputAPI/GInputAPI.h` (= nuestro `mods/ginput/.../GInputAPI.h`) |
| **WidescreenFixesPack GTAVC** | `github.com/ThirteenAG/WidescreenFixesPack` → `source/GTAVC.WidescreenFix/dllmain.cpp` + `data/GTAVC.WidescreenFix/scripts/GTAVC.WidescreenFix.ini` | **MIT** ✅ | Fórmulas de FOV/HUD/letterboxing + opciones nuevas del doc oficial: **No Island Loading, Seamless Interiors, SMAA, Xbox360 Gamma, Transparent Menu, Speed Sensitive FOV, VCS Camera Shake, Text Outline, Frame Limiter en menú** |
| **GInput VC** | ❌ **cerrado** (solo binario + `GInputAPI.h` + docs) | — | Spec: `mods/ginput/GInputVC.ini` + `(docs, api)/` (5 setups, deadzones). Es reimplementar sobre Gamepad API |
| **FramerateVigilante** | sin fuente localizada (fork binario `shaneomac1337/FramerateVigilante-Fixed`) | — | Ya hecho al 100% por nosotros (T1/T2) |
| **CLEO-lite (runtime)** | `github.com/sannybuilder/library` = **spec oficial** de TODOS los comandos III/VC/SA + CLEO4/5/Redux | Docs (atribución) | `vc/vc.json` (838 KB: comandos con tipos entrada/salida), `vc/docs/*.md` (semántica por comando), `vc/statements/` (if/while), `vc/enums.json`, `vc/snippets/`. Complemento: `gtamods.com/wiki/List_of_opcodes`, "Opcodes Restoration Project" |
| **plugin-sdk** (auxiliar) | `github.com/DK22Pac/plugin-sdk` | — | Headers reversados (`CPed`, `CCamera`, `CVehicle`…) = referencia de nombres/offsets para traducir hooks |
| **climbing** | ya lo tenemos (fuente reVC completo en `mods/climbing/source_code/`) | — | Ver `04` |

Los ASIs **nunca se compilan ni cargan** en nuestro wasm: lo que se lleva es su **lógica**,
anclada en nuestro `src/`. `injector/`, `ModUtils`, ASI-loaders, `rwd3d9` (driver D3D9 de RW) y
el wrapper d3d8/d3d9 **no aplican** (nosotros ya usamos librw GL3/WebGL2).

---

## 2. CLEO-lite en profundidad

### 2.1 Por qué es barato (recordatorio del `05` §1-3)
Los `.cs` son bytecode SCM estándar. Nuestro intérprete (`src/control/Script2..8.cpp`,
`case COMMAND_XXX:`, 0–1499) ya ejecuta casi todo. CLEO solo añade **opcodes ≥ 0x05DC**.

### 2.2 Arquitectura propuesta (≈3 componentes)

```
gta_vc_browser/tools/cleo_disasm.py     → lista qué opcodes usa un .cs (herramienta de inspección)
gta_vc_browser/tools/gen_cleo_ops.py    → code-gen: lee vc/vc.json → genera ScriptCleo.inc
src/control/ScriptCleo.cpp              → dispatch de opcodes ≥1500 + puente VICEEXT_*
src/control/Script6.cpp (loader)        → volcar cleo/*.cs en ranuras + StartNewScript()
```

1. **`cleo_disasm.py`** (≈150 líneas): desensamblador lineal con la tabla de tipos de arg
   (`0x01` int32, `0x02` var global, `0x03` var local, `0x04` int16, `0x09` string8…) del formato
   ya decodificado en el `05` §2. Uso: `python tools/cleo_disasm.py swim.cs` → salida de opcodes
   usados + referencias a variables. **Herramienta previa indispensable**: antes de implementar
   un opcode, sabemos exactamente cuáles exige cada script.
2. **`gen_cleo_ops.py`** (≈200 líneas): consume `vc/vc.json` (comandos con nombre, id, inputs y
   outputs tipados) y emite la tabla `ScriptCleo.inc`: `case 0x05E6: return Cmd_IsKeyPressed();`
   + firmas con los tipos exactos. **No se escribe la spec a mano: se code-genera** — esto evita
   errores de traducción de args (el error clásico de intérpretes hechos a mano).
3. **`ScriptCleo.cpp`**: solo los cuerpos de los opcodes que nos interesen. Cada cuerpo delega en
   nuestras mecánicas ya portadas:
   - test de teclas/pad → nuestro pad (`CPad`) → nado/agachado ya portados;
   - animaciones → `CAnimManager` (con **comprobación de rango**, trampa conocida);
   - cámara → nuestro `CCamera`;
   - flags y timers → `CTheScripts`.
4. **Loader** en el arranque (`Script6.cpp:403` ya hace esto para misiones):
   `CFileMgr::Read(fd, &ScriptSpace[slot], SIZE_MISSION_SCRIPT); CTheScripts::StartNewScript(slot);`
   Nuevo: escanear `streamed/cleo/*.cs` (pasados por el `ondemand.js` igual que los datos), ranura
   por script, nombre de hilo via `03A4`. **Sin .cm/.fxt primero** (fase 2).

### 2.3 Fases
- **F1** `cleo_disasm.py` + inventario de opcodes de `swim.cs` y `CrouchMovement(forClassicAxis).cs`
  (ya localizados en `mods/`). ≈ mediodía de trabajo.
- **F2** loader + los ~6 opcodes de esos dos scripts → prueba de que **el sistema** corre.
- **F3** `gen_cleo_ops.py` + tabla completa de stubs (los no implementados: log y no-op con traza).
- **F4** ir rellenando cuerpos según los mods que queramos meter.

### 2.4 Qué evitar y por qué
- ❌ **Opcodes de CLEO Redux / natives**: es otro ABI (JS-like). Si un script lo usa, mejor
  reimplementarlo nativo. Documentado en el `05` §2.
- ❌ **`MemoryModule`/plugins CLEO** (`0BA2`): carga de DLLs — imposible en wasm.
- ❌ **Opcodes de acceso a memoria cruda** (`0A8C`-style): no implementar salvo lo mínimo; nuestro
  `ScriptSpace` está en un HEAP de Emscripten, escribir a "direcciones" del juego nativo no
  significa nada. Cuando un script lo necesite, sustituir por un opcode puente nuestro.
- ❌ **Compilar los `.cs` de golpe sin F1**: sin desensamblador estamos adivinando argumentos.

---

## 3. Mapeo «dónde y cómo» — propuesta externa vs nuestro código

Anclas verificadas hoy en `src/` (el resto: localizar con `code_search` al implementar).

| Mejora | Fuente externa (fichero exacto) | Nuestro `src/` (dónde) | Cómo | Evitar / por qué |
|---|---|---|---|---|
| FOV widescreen, letterboxing, Speed FOV, VCS shake | `WidescreenFixesPack/source/GTAVC.WidescreenFix/dllmain.cpp` (MIT) | `core/Camera.cpp:290` `CCamera::Process` (+ `CCamera::CamControl`) | Traducir sus fórmulas de aspecto/FOV a nuestro `CCamera` (mismo juego-base: sus direcciones = nuestras funciones) | No traer su hooking ni su `injector`; no «ForceAspectRatio» (el canvas web ya es libre) |
| HUD/radar/subtítulos escalado + Text Outline + HUD constraint | ídem `dllmain.cpp` + `global.ini` | `renderer/Hud.cpp:328` `CHud::Draw` (+ `CRadar`, `CFont`, `CMessages`) | Escalado por `RsGlobal`/aspect; outline = doble render de `CFont::PrintString` con offset | No romper el 4:3 original (mus por defecto igual que el mod) |
| **No Island Loading / Seamless Interiors** | ídem (features nuevas del doc oficial) | `core/FileLoader.cpp`, `core/Streaming.cpp`, `core/ZoneCull`… (zona de carga de islas) | Cargar las 3 islas + interiores al inicio (presupuesto de RAM/streaming actual ya gestiona 1,4 GB on-demand — hay margen) | **⚠️ Asincronía**: nada de control-flow nuevo dentro de `LoadAllRequestedModels` (trampa Asyncify conocida) |
| Coranas, dobles ruedas, transparencias, DrawBackfaces | `SilentPatch/*` (MIT) + `CHANGELOG-VC.md` + `Data/*.txt` | `renderer/Coronas.cpp`, `renderer/Renderer.cpp`, `render/RenderBuffer`… | Copiar la lógica del fix (no su patch por direcciones): el bug está en el mismo sitio en reVC (mismo origen) | No aplicar los fixes Win32 (DEP, Alt+F4, ventana) |
| Timer brazo (10 s), ráfagas, gasolina, puertas, blips, pantalla partida 4:3 | `SilentPatch/Timer.cpp` + `CHANGELOG-VC.md` | `core/Timer.cpp`, `vehicles/Vehicle.cpp`, `peds/Ped.cpp`, `core/Radar.cpp`, `frontend/…` | Uno por uno con su traza + criterio PASS (bloque estilo T) | Meterlos todos a la vez sin trazas |
| Neo shininess (X2.10-12), env map, dual pass, ps2Water, ColorFilter | `skygfx_vc/src/matfx.cpp`, `leedsCarpipe.cpp`, `WaterLevel.cpp`, `shaders/*.hlsl` | `renderer/WaterLevel.cpp:911` `RenderWater` / `:1206` `RenderTransparentWater` / `:3017` `RenderSeaBirds` `:3067` `RenderShipsOnHorizon`; vehículos en `vehicles/Vehicle.cpp` (render); shaders → **nuestros GLSL de librw GL3** | (1) datos `neo/*.dat` ya extraídos; (2) `ps2Water` = reescribir `RenderTransparentWater`; (3) env/rim = `RwMatFX` nuestro; (4) HLSL → GLSL ES3.0 manual | ⚠️ Sin LICENSE: lógica de referencia, implementación propia. NO usar `rwd3d9` ni el wrapper d3d8. NO traducir HLSL automáticamente (semántica PS2 vs GL) |
| Cámara IV/apuntado, piernas libres (bForceLegsMovements) | `classic-axis/source/CamNew.cpp`, `Main.cpp` | `core/Camera.cpp` (nuestros bloques C2/R20c ya ahí), `peds/Ped.cpp` (R20c giro lateral) | `CamNew.cpp` = algoritmo de cámara de apuntado: pasarlo a nuestro `CCamera`; `Main.cpp` = semántica de strafe/piernas → nuestro `CPed::SetMoveAnim` con `walk_left/right/back` | ⚠️ Sin LICENSE: adaptar, no copiar literal. Depende de plugin-sdk (nombres → nuestros nombres reVC) |
| FPS cap + timers independientes | (sin fuente) | `core/Timer.cpp` | ✅ hecho (T1/T2) | — |
| Mando: 5 setups, deadzones, sniper zoom, iconos | spec: `mods/ginput/GInputVC.ini` + docs | `controls/Pad.cpp` (+ interfaz web Gamepad API en `gta_vc_browser/`) | Reimplementar mapeos y curvas; iconos btn = txd ya servidos | ❌ XInput/SCP/Sixaxis y vibración XInput: en web, Gamepad API + GamepadHapticActuator (parcial) |
| Misiones/Script changes (garages, DEagle, pickups…) | `mods/extended/.../data/main.scm` | datos (`bootseed/data/main.scm`) | ✅ solo servirlo + regresión (ver `05` §4) | Sin regresión de misiones |
| Ecosistema CLEO | `sannybuilder/library` `vc/vc.json`… | `control/ScriptCleo.cpp` (nuevo) + `Script6.cpp:403` | §2 de este plan (code-gen desde la spec) | Redux natives, MemoryModule, memoria cruda |
| Birds/ship (X2.14/15) | (está en SkyGfx `misc.cpp` parcialmente + Extended) | `renderer/WaterLevel.cpp:3017/3067` ya existen `RenderSeaBirds`/`RenderShipsOnHorizon` | Ajustar visibilidad/parámetros a Extended | Rehacer el sistema de pájaros entero |

---

## 4. Qué NO hacer (global, con porqué)

1. **No compilar ni inyectar los ASIs** — wasm no carga DLLs; el hooking (`MemoryMgr`, `injector`,
   `ModUtils`) es inaplicable. Se porta la lógica al sitio homólogo de `src/`.
2. **Copiar de cualquier fuente** (actualizado 27/09/2026): la licencia ya no
   bloquea — de `skygfx_vc`, `classic-axis` o cualquier mod se toma código, lógica
   o datos sin ceremonia. La atribución en comentarios se mantiene como **cortesía
   y trazabilidad**, no como obligación.
3. **No traducir HLSL a GLSL automáticamente** — la semántica de MatFX/PS2 no es 1:1; traducir
   entendiendo cada pass (el `matfx.cpp` documenta el orden).
4. **No tocar `LoadAllRequestedModels` con control-flow nuevo** (Asyncify) — trampa pagada.
5. **No escribir specs a mano para CLEO-lite** — code-gen desde `vc/vc.json` (§2.2).
6. **No implementar primero lo vistoso (SkyGfx) antes que lo vital**: el orden del `05` §9 sigue
   mandando (main.scm → escalada → piernas → C7 → SilentPatch → Neo → CLEO-lite).
7. **No olvidar las trampas de la casa**: CRLF en `.cpp` (editar en binario), `VERSION++` por
   build, `dataTag` solo con datos nuevos, `GetAnimation(id)` con rango, `m_vecMoveSpeed` en m/s÷50.

---

## 5. % actualizados (con fuentes en la mano)

| Área | % antes (`05`) | % ahora | Por qué sube |
|---|---|---|---|
| Classic AXIS | 70% | **85%** | `CamNew.cpp` + `Main.cpp` completos como referencia |
| SkyGfx | 60% | **72%** | `matfx.cpp`, `leedsCarpipe.cpp`, `WaterLevel.cpp` + 25 HLSL (a traducir) |
| SilentPatch | 80% | **85%** | Código MIT copiable, no solo readme |
| WidescreenFix | 70% | **85%** | `dllmain.cpp` MIT + `global.ini` (y features nuevas: No Island Loading, Seamless Interiors) |
| Ecosistema CLEO | 85% viable | **90% viable** | Spec oficial `vc/vc.json` → code-gen |
| GInput | 75% | 75% | Sin fuente (cerrado); ya era spec |
| **GLOBAL del objetivo** | ≈45% | **≈30 % o menos (corrección del jugador, 24/09 — ver nota)** | los fuentes en la mano suben lo *copiable*, no lo *portado* |

> **Corrección del jugador (24/09/2026 — E22):** el % global NO es ≈50 %: es
> menos — «falta lo más vital y notorio de Extended». La revisión «con fuentes
> en la mano» solo subió el material copiable (fuentes/HLSL/INIs); las
> mecánicas vitales de la X-list (`vice-extended-pendientes.md`) siguen sin
> portar. **Cifra fijada por el jugador: ≈30 % o menos.**

Nuevos activos que no estaban en el plan y aparecen en `dllmain.cpp`/doc de WidescreenFix
(valor añadido para la X-list futura): **No Island Loading**, **Seamless Interiors**, SMAA,
Xbox360 Gamma, Transparent Menu, Speed Sensitive FOV, VCS Camera Shake, Text Outline, Frame
Limiter en el menú de display.
