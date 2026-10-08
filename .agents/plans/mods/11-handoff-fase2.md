---
name: 11-handoff-fase2
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 11 · HANDOFF — Cierre de la fase 1 y arranque de la fase 2

> **Qué es este documento**: volcado de TODO el contexto de la sesión de
> cierre de la fase 1 (24/09/2026). Si este hilo muere, cualquier agente que
> lea este fichero + los referidos puede retomar la fase 2 **tal como la
> haría Buffy**. El jugador da la orden de arranque («yo te aviso cuando»);
> hasta entonces NO se toca código del juego.
>
> Estado al escribir esto: **FASE 1 CERRADA AL 100 %** (sin pendientes humanos
> ni técnicos). Fase 2 = integración en `src/` + medición en vivo.

---

## 1. El proyecto

- Port de **GTA Vice City (reVC, rama `miami`)** a **WebAssembly** en
  `/c/Users/s0rno/OneDrive/Documents/re3`.
- Integra el mod **Vice Extended** (`mods/extended/`) y otros de `mods/`.
- Motor web en `gta_vc_browser/` (build Emscripten), datos servidos en
  `streamed/`, capa on-demand en `gta_vc_browser/web/ondemand.js`.
- **Regla maestra de la fase 1** (sigue vigente como espíritu): copiar/ADAPTAR
  código funcional de los mods, **nunca reinventar**; cero integración sin
  visto bueno del jugador.

## 2. Reglas de casa (obligatorias)

| Regla | Detalle |
|---|---|
| CRLF | los `.cpp` del proyecto van en **CRLF**; editar con python binario (las tools de ficheros dejan LF) |
| VERSION | `gta_vc_browser/web/lib/index.js` → `VERSION` (= `2026-09-23-ve55`) y `dataTag` (= `2026-09-21-ve13`) — actualizar al publicar |
| Git | nada de `git commit` / `git add` / `push` — el jugador decide |
| Procesos | **no matar procesos node** (otros hilos pueden tener servidores vivos) |
| Partidas | `bash gta_vc_browser/check-served-build.sh` **antes** de pedir una partida |
| Medición | el jugador mide con **logs de sus partidas**; sin palancas de URL/consola |
| Escritura (fase 1) | solo `gta_vc_browser/tools/` y `.agents/plans/` (+ `tmp/` de trabajo) |
| Emscripten | emsdk en `/c/Users/s0rno/emsdk`; build: `bash gta_vc_browser/build.sh` |
| Espacio de nombres | `.agents/plans/mods/09-numeracion-a-b-c-d.md` blinda la numeración A/B/C/D (lista literal 1-17) + E18-E24 — no renumerar |

## 3. Decisiones ya tomadas por el jugador — NO REABRIR

| Decisión | Contenido |
|---|---|
| **E22** | el % global de integración es **≈30 % o menos** (literal: «no es ≈50 %: es menos — falta lo más vital y notorio de Extended»). Aplicado en `05` §9 y `06` §5 |
| **E21** (`main.scm`) | **APLAZADA a fase 2**. Recomendación registrada: «conservar el de serie». Tras la minería (§6) la opción del mod ya NO es a ciegas: los handlers `0xFAx` son portables |
| **A4** | set de crouchfire = **SERVIDO** (0,86 s; es el de Vice Extended. `movements.img` es del addon ClassicAXIS, no del mod) |
| **A5** | tabla de oído FINAL del banco ViceEx — §5 |
| D14/D15 | spikes ejecutados con la condición literal «a la final dejes el código como lo encontraste» → cumplida: cero ficheros del juego tocados |

## 4. Dónde está cada resultado de la fase 1

- **`09-numeracion-a-b-c-d.md`** → fuente de verdad de ESTADOS (A1-A5, B6-B10,
  C11-C13, D14-D17, E18-E24). Cada fila apunta a su sección del `10`.
- **`10-plan-pasada-siguiente.md`** → la pasada ejecutada (24/09/2026):
  - §Resultados: 11/11 ítems (A3-A5, B7-B10, C11-C13, E18-E24).
  - §Spikes: D14, D15, minería del `ViceEx.exe`.
  - §A5 escucha cerrada: tabla oído vs envolvente + forense del RAW/SDT.
- `05-cleos-adaptador-y-paridad-scm.md` §9 y `06-fuentes-externas-y-mapeo.md`
  §5 → % global corregido (E22).
- `08-*` → GXT (A1), decisiones 25 ficheros (A2), changelog SilentPatch (B6).
- Extracción/documentación original: `00-INDICE.md` → `07-*` (ver índice).

Resumen de los 11 ítems ejecutados (24/09):

| Ítem | Veredicto | Dónde |
|---|---|---|
| A3 IMG↔IMG | 468 entradas, 459 idénticas, 9 `.col` distintas (todas vanilla en `objects.img`) | `10` §A3 |
| A4 crouchfire | set **servido**; `GunCrouchFwd/Bwd` extraídos a `tmp/a4/` | `10` §A4 |
| A5 audio | weapon.dat del mod **sin columnas de audio** (sonidos hardcoded en el exe); 13 muestras → §5; gamecontrollerdb = solo fallback (bindings ya vienen en el *standard mapping* de Gamepad API) | `10` §A5 |
| B7 FOV/HUD WFP | `dllmain.cpp` es stub; fórmulas reales en `.ixx` (30 en `tmp/extsrc/wfp/`) → tabla fórmula:fichero:línea → punto en `src/` | `10` §B7 |
| B8 ClassicAXIS | pseudocódigo `CamNew.cpp` + `bForceLegsMovements` (spec, sin copia literal) | `10` §B8 |
| B9 SkyGfx | 24 HLSL pass a pass → GLSL ES 3.00 (`tmp/extsrc/skygfx_shaders.txt`) | `10` §B9 |
| B10 SilentPatch | mapa de secciones `SilentPatchVC.cpp` (4.764 líneas) + remate identificado (OutroSplashFix = E24) | `10` §B10 |
| C11 vc.json | 1.689 comandos, **0 colisiones**; ids `vc.json` = 4 hex sin prefijo → `int(id,16)` | `10` §C11 |
| C12 CALL_FUNCTION | `tools/cleo_disasm.py`; walks completos: `swim.cs` = memory-hack total (WRITE_MEMORY×4+CALL_FUNCTION), `CrouchMovement` = READ_MEMORY×4 | `10` §C12 |
| C13 cabecera SCM | `V = u32@3`, objetos 8 B en V+8, multiscript 12 B, `MultiScriptArray[0] == MainScriptSize` | `10` §C13 |
| E18-E20 | IDs 6670/6671 sin colisión (mod reserva 6500-6599 veh · 6600-6619 wheels · 6620-6659 vehmods · 6660-6699 weapons) | `10` §Tanda 0 |
| E21 inventario | servido 0 custom vs mod 7 tipos/206 usos (`0FA2`×91, `0FA6`×46, `0FA0`×45, `0FA8`×20, `0FA1`×2, `0FA7`×1, `0FA9`×1) + minería §6 | `10` §E21 |
| E23 INI | 13 claves `SilentPatchVC.ini`, 0 sin clasificar | `10` §E23 |
| E24 outro fade | `src/core/Frontend.cpp:2476-2521` (`m_nMenuFadeAlpha` +20 cada >30 ms, **240→260 → clamp 255** = parpadeo de 1 frame; `:2500-2521` dibuja con `255−alpha`) + fix `SilentPatchVC.cpp:2384` `OutroSplashFix` (clamp alfa `[0,255]`, 16 líneas) | `10` §E24 |

## 5. Audio ViceEx (A5) — tabla FINAL de oído del jugador (24/09)

`gta_vc_browser/tmp/viceex-samples/viceex-map.tsv` (relleno, CRLF de la
plantilla intacto). **El oído manda sobre la propuesta por envolvente**:

| # | Qué es (oído del jugador) | Propuesta envolvente | ¿Acertó? |
|---|---|---|---|
| 0/1 | **ROTOS — ruido al azar** → se dejan VACÍAS (manda el de serie) | disparos ligeros | ❌ |
| 2 | **recarga EN GENERAL** (NO es bombeo) | bombeo/recarga | ❌ parcial |
| 3/4 | **shotgun2** (par) | escopeta | ✅ |
| 5/6 | **desert eagle** (par) | granada/cohete | ❌ |
| 7/8 | disparo sin identificar (par) — arma sin ID | Desert Eagle | ~ |
| 9/10 | **francotirador/fusil de precisión** (¿steyr?) (par) | 2ª explosión | ❌ |
| 11 | **lanzagranadas: LANZAMIENTO del proyectil** (no es recarga) | mecanismo/recarga | ~ parcial |
| 12 | **nadar (swim)** — no es arma | recarga/impacto | ❌ |

**Forense del RAW/SDT** (tras el aviso del jugador sobre 0/1):
- Extracción correcta: offsets contiguos, `Σtamaños = 364.850 B` = RAW exacto,
  PCM16 coherente, sin MP3/ADPCM/Ogg, offsets pares, estéreo intercalado
  descartado (ZCR par≈impar).
- 0/1 **rotos de fábrica**: envolvente RMS plana (sin ataque/decaimiento) y
  datos saturados (pico 32767/32768) = ruido puro del banco del mod.
- **5/6 = copia EXACTA** byte a byte (md5 idéntico) y **7/8 = copia EXACTA**
  (md5 idéntico) — el autor duplicó sin variante. 3/4 y 9/10 sí son variantes
  reales (bytes distintos).

**Ejecución pendiente en fase 2**: `tools/split_sfx.py` (añadir las 11 muestras
útiles al banco servido) + enganchar en `AudioLogic` a beretta / shotgun2 /
desert eagle / (¿steyr?) / lanzagranadas-lanzamiento / swim.

## 6. Minería del `ViceEx.exe` (destraba E21) — HALLAZGO MAYOR

- **El exe es un BUILD de un fork de reVC/librw** («gta-extended»): strings
  `D:\Modding\GTA\VC\gta-extended\src\control\Script*.cpp`, `vendor\librw\...`,
  PDB `bin\win-x86-librw_d3d9-mss\DebugVE\ViceEx.pdb` (Debug → asserts
  legibles), `MODLOADER_REVC`.
- Fichero: `mods/extended/GameFiles/ViceEx.exe` (3.822.080 B, PE32,
  imagebase 0x400000; .text raw 0x400+0x2D6000, .rdata raw 0x2D6400+0x54800).
- **Dispatch**: `CRunningScript::ProcessCommands4000To4099` VA `0x4946C0`
  (`eax = opcode − 4000; if (eax > 9) default; jmp tabla[eax]`), tabla de
  saltos VA `0x494A70` → **los 10 handlers SON `0x0FA0`-`0x0FA9`** (4000-4009).
- **Aridad** (pushes antes de `CollectParameters` `0x472a80`): case 0 `[2,1,2]`,
  case 1 `[1,2,1,2]`, case 2 `[2,1,2]`, cases 3-8 `[1,2]`, case 9 `[2]` →
  patrón ~2 args + 1 resultado (`StoreParameters` presente);
  **`0xFA9` sin escritura** (solo consulta).
- **Semántica parcial** (asserts + callees): `0x0FA0` = `pPed` +
  `CTxdStore::GetSlot` (TXD/skin al ped); `0x0FA9` = `vehicle`; `0x0FA6` =
  lógica pura sin llamadas; `0xFA4/5/7` comparten callee `0x48b120`;
  `0xFA8`→`0x5e1070`; `0xFA3`→`0x604640`; `0xFA2` (×91)→`0x4f83d0`.
- **Conclusión**: los handlers son **portables a nuestro intérprete** (misma
  API: `CollectParameters`/`StoreParameters`/`GetPointerToScriptVariable`) →
  si el jugador elige el `main.scm` del mod en E21, hay ruta técnica.
- Pendiente (solo si se necesita aridad/semántica exacta): desensamblado fino
  de los handlers con `tools/viceex_opcodes.py` + `viceex_dispatch.py`
  (capstone 5.0.7 en `gta_vc_browser/tmp/pylibs`).

## 7. Spikes D14/D15 (veredictos)

- **D14 · Night vertex colors (SkyGfx X2.11) — DIFICULTAD BAJA**: NO son datos
  nuevos; es un **uniform `nightParam`** (string en el exe, vive en su
  `vendor/librw/src/d3d/d3d9.cpp`) que **modula el prelight por pipeline**.
  Nuestros shaders GL3 (`vendor/librw/src/gl/shaders/`, 18 ficheros; un solo
  `in_color` = ATTRIB_COLOR 2 en `header.vert`; `grep night` = 0). DFF vanilla
  muestreados (`ap_hland_01`, `washbuild018`, `admiral`) SIN segundo set de
  colores (extensiones: node-name `0x253F2FE`, HAnim `0x11E`, 2dfx `0x50E`).
  **Coste ≈ 1 uniform + `mix()` en ~4 shaders** (im3d/skin/matfx/default).
- **D15 · RAM/streaming (No Island Loading / Seamless Interiors) — FACTIBLE
  CON RESERVAS**: `streamed/` = 1.492 MB (Audio 958 MB: `sfx.RAW` 340 MB +
  9 `.adf` ≈270 MB; models 397 MB con **`gta3.img` = 363 MB**; anim 118 MB con
  `cuts.img` 115 MB). Actual: `ondemand.js` pool MEMFS **CAP 360 MB** (working
  set ≈150 MB mundo + 30 MB emisora, `warmMB` 64), IDBFS `idbCapMB: 900`.
  Sin No Island Loading: CAP 360→**~700 MB**, primera carga ~363 MB; Seamless
  Interiors ya caben (+0-50 MB). RSS wasm ≈**1,2-1,8 GB** → solo-desktop al
  principio; **medir en vivo con `odtrace` en fase 2**.

## 8. Fase 2 — cola de trabajo EN ESTE ORDEN (cómo la haría Buffy)

1. **Tanda 1 · integración (empezar aquí)**: fix **E24** (clamp del alfa del
   fade en `Frontend.cpp`; criterio PASS = fundido del outro sin frame claro)
   + quick-wins del catálogo **B6**/B10 (≈100 fixes de SilentPatch clasificados
   en `08` §B6, aplicables/sí-no con punto exacto en `src/`). Victoria
   temprana, riesgo bajo, todo anclado.
   > **PROGRESO (24/09/2026, build `ve56`)**: E24 **PORTADO**
   > (`Frontend.cpp:2476-2496`, clamp `Min(+20, 255)` del paso del fade en
   > origen) + duración del outro **PORTADA** (`SilentPatch :4086` → tick
   > `>32` ms / 75 cuentas ≈2,5 s a cualquier fps, `MUCH_SHORTER_OUTRO_SCREEN`
   > desactivado en `config.h:454-456`) + random de 16 bits (script y CarGen) =
   > **YA ESTABA** vía `USE_PS2_RAND` (`config.h:276`). Todo en
   > `docs/mods/ATTRIBUTION.md` §1. PASS del jugador: salir al escritorio →
   > sin frame claro + splash ≈2,5 s legibles. Siguientes quick-wins B6 (pools
   > llenos, GXT fuera de rango, textos blancos :802) o pasar al punto 2.
2. **Audio ViceEx**: `tools/split_sfx.py` + enganche en `AudioLogic` con la
   tabla §5 (11 muestras útiles; 0/1 descartadas).
3. **D14** night vertex colors → uniform `nightParam` + `mix()` en ~4 shaders
   GL3 (SkyGfx; spec B9 en `10` §B9 + HLSL en `tmp/extsrc/skygfx_shaders.txt`).
4. **B7/B8/B9**: FOV/HUD WFP (fórmulas con fichero:línea), ClassicAXIS
   (`CamNew.cpp` + `bForceLegsMovements`), SkyGfx pass a pass.
5. **E21** (decisión del jugador): si opta por el `main.scm` del mod → portar
   los 10 handlers `0xFAx` (§6); recomendación registrada = conservar el de
   serie.
6. **D15** con medición real (`odtrace`, RSS 1,2-1,8 GB) antes de subir caps.
7. Reglas de la fase 2 al arrancar: **confirmar con el jugador el alcance de
   escritura** (en fase 1 era solo `tools/` + `.agents/plans/`); `.cpp` en
   CRLF; bump de `VERSION`/`dataTag` al publicar; `check-served-build.sh`
   antes de pedir partidas de prueba.

## 9. Herramientas y artefactos ya creados

| Herramienta | Para qué |
|---|---|
| `gta_vc_browser/tools/cleo_disasm.py` | desensamblador SCM/CLEO (F1) — walks completos en `tmp/walk-*.asm` |
| `gta_vc_browser/tools/viceex_opcodes.py` + `viceex_dispatch.py` | minería del `ViceEx.exe` (dispatch/aridad de `0xFAx`) |
| `gta_vc_browser/tools/extract_viceex.py` | extracción del banco `ViceEx.RAW/SDT` → WAV + plantilla TSV |
| `gta_vc_browser/tools/viceex_samples.html` | **reproductor** para escuchar las 13 muestras y rellenar/exportar el TSV (sirve con `python -m http.server 8791` desde `gta_vc_browser/`) |
| `gta_vc_browser/tools/split_sfx.py` | añadir muestras al banco servido (paso pendiente §5) |
| `gta_vc_browser/tmp/pylibs/` | capstone 5.0.7 (para los tools de minería) |

Artefactos de trabajo en `gta_vc_browser/tmp/`:
`walk-*.asm` (main.scm mod/servido, swim.cs, CrouchMovement),
`extsrc/` (30 `.ixx` WFP, `CamNew.cpp`, `SilentPatchVC.cpp`, `skygfx_shaders.txt`,
`vc.json`), `viceex-samples/` (13 WAV + `viceex-map.tsv` **relleno**), `a4/`
(`GunCrouchFwd/Bwd`).

## 10. Chuleta de formatos decodificados

- **Bytecode SCM**: args auto-tag `1`=int32, `2`=global16, `3`=local16,
  `4`=int8, `5`=float32; strings 8 B **sin tag**; `ARGUMENT_END`=0x00;
  ANDOR: param `n` = AND de n+1, `20+n` = OR de n+1; not-flag = `opcode|0x8000`.
- **Cabecera SCM**: `V = u32@3`; objetos 8 B en V+8; cabecera multiscript 12 B;
  `MultiScriptArray[0] == MainScriptSize`.
- **Ids `vc.json`**: 4 hex sin prefijo → `int(id, 16)`.
- **Chunks RW DFF**: cabecera 12 B (`id, size, ver`); payload hijos desde +12.
- **SDT/RAW (ViceEx)**: 20 B/entrada (`off, size, freq, loopStart, loopEnd`);
  PCM s16le mono concatenado en el RAW.

## 11. Trampas ya pisadas (no repetir)

- Heredoc bash + backslashes python → `SyntaxError` (usar `os.path.*`).
- Consola cp1252 rompe con UTF-8 → `PYTHONIOENCODING=utf-8`.
- La tabla de saltos del dispatch NO son runs de punteros (es
  `jmp [eax*4+cst]` local) — leer la instrucción, no patrones de bytes.
- Inmediatos `0xFA0`=4000 decimal → falsos positivos de timers en búsquedas.
- Capstone desalineado = ruido: partir de los `call` de `CollectParameters`.
- VA de strings del exe: calcular con la RVA real de sección (.rdata raw
  0x2D6400 → RVA 0x2D7000).
- `write_file`/`str_replace` dejan LF: para `.cpp` del proyecto, CRLF por
  python binario.

## 12. Estado del árbol al cierre

- Cero ficheros del juego tocados en toda la fase 1 (condición D14/D15
  cumplida). Modificaciones solo en `.agents/plans/mods/`,
  `gta_vc_browser/tools/` y `gta_vc_browser/tmp/`.
- Rama `miami`. Nada commiteado (regla de casa).
- Pendientes humanos: **ninguno**. Dudas menores abiertas (no bloquean):
  identidad del arma de 7/8; confirmar si 9/10 es `steyr`.
- Decisión abierta del jugador en fase 2: **E21** (`main.scm`).

---
*Escrito 24/09/2026 al cerrar la fase 1. Para arrancar la fase 2: leer `09`
(estados) + `10` (resultados) + este fichero, y seguir §8 en orden.*
