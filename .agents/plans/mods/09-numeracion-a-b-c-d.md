---
name: 09-numeracion-a-b-c-d
status: EXECUTED
type: maintenance
domain: process
owner_rules: .agents
created: 2026-09-24
---

# 09 — Numeración de bloques A/B/C/D y flujo de fases (BLINDADO)

> Propósito: **que la numeración del trabajo no vuelva a perderse.** Vivió solo
> en el chat y un reinicio la borró (24/09/2026). **Lista LITERAL RECUPERADA el
> 24/09/2026 de la caché del chat de Freebuff**
> (`%APPDATA%\Freebuff\Cache\Cache_Data\f_005380`) — ya no es reconstrucción.
> **A partir de ahora MANDA ESTE FICHERO.**

Estado: vigente desde 24/09/2026. Contenido solo documental (cero código de juego).

---

## 1. Las 2 fases (acuerdo del jugador)

| Fase | Qué se hace | Cuándo |
|---|---|---|
| **1 · Extracción/documentación** | Decodificar, diferenciar, clasificar y dejarlo en `.agents/plans/`. Solo LECTURA de ficheros del juego/mod. Salidas a `/tmp` y a los MD. Herramientas de análisis nuevas en `gta_vc_browser/tools/` sí se pueden escribir (son nuestras). | **AHORA** |
| **2 · Integración** | Empezando por lo ya medido (servir `main.scm`, bloques de escalada, acciones de datos). Cada bloque = código + su traza + criterio PASS + `check-served-build.sh` antes de pedir partida. Un bloque = un build. | Cuando el jugador diga |

Regla de oro: **nada se toca del juego hasta que se extraiga y documente todo.**
Copiar/adaptar código ya funcional de los mods; nunca reinventar mecánicas que
ellos ya resuelven (la ingeniería está en ADAPTAR lo ya hecho y funcional).

## 2. Reglas por bloque

| Bloque | Naturaleza | Regla |
|---|---|---|
| **A · datos** (ítems 1-5) | Solo lectura | decodificar/diferenciar/clasificar → MDs. Sin escribir en `streamed/`, `bootseed/`, `gamefiles/` |
| **B · código externo** (ítems 6-10) | Solo lectura | leer su código/INI/docs → extraer fórmulas/algoritmos + mapeo «dónde y cómo» → MDs. Sin portar todavía |
| **C · CLEO-lite F1** (ítems 11-13) | Solo lectura (+ herramientas nuestras) | desensamblar `.cs`/`.scm`, cerrar la decodificación → MDs y `tools/` |
| **D · Motor** (ítems 14-17) | Spikes / integración | 14-15 = **spikes AUTORIZADOS** (§4); 16-17 = solo en fase 2 |

Licencias (actualizado 27/09/2026, ver `00-INDICE.md` §2): **la licencia ya NO es
una puerta** — de `mods/` se toma cualquier código que haga falta, con o sin
LICENSE; la atribución es cortesía. Lo único que sigue fuera es lo
técnicamente inaplicable en WASM (ASIs/x86, `.ual`, `rwd3d9`, `injector`,
`plugin-sdk`, memory-hacks de `.cs`).

**Etiquetas:** la referencia es `letra de sección + nº de ítem` (A1, A2, B6,
C11-C13, D14-D17…), tal como se usaban en el chat original.

## 3. LA LISTA LITERAL (recuperada, 1-17) — con estado

### A. Datos — diferenciar y decidir (prioridad alta)

| # | Ítem (texto literal del agente anterior, condensado) | Estado | Dónde |
|---|---|---|---|
| **A1** | Decodificar los GXT (TKEY/TDAT) servidos vs mod y listar las claves de esos +214 B — no asumir cuál es mejor | ✅ HECHO | `08` §A1: 0 textos distintos; servido = superset (+6 claves de vehículos) → se conservan los servidos |
| **A2** | Los 25 ficheros distintos, por familia: `main.scm`, `default.ide/.dat` (~900 B de defs), `particle.cfg`, `object.dat`, `occlu.ipl`, `gta_vc.dat`, `bryx.*` (1 byte = ¿CRLF?), frontends TXD ×7 (servidos MÁS grandes), `wheels.DFF/TXD` (el mod 12× = ruedas HD que NO tenemos) | ✅ HECHO (decisión) | `08` §A2 — «conservar lo servido» / «adoptar del MOD». Ejecución = fase 2 |
| **A3** | Comparar el **contenido interno** de los 8 `cdimages/*.img` del overlay entrada a entrada (antes solo se comparó por nombre de fichero) | ✅ HECHO | `10` §Resultados A3: 468 entradas, 459 idénticas, 0 no servidas, 9 distintas (todas `.col` vanilla en `objects.img`) |
| **A4** | Extraer `GunCrouchFwd/Bwd` (única pieza de IFP pendiente) y decidir el **set de crouchfire** (servido 0,86 s vs ClassicAXIS 0,63 s en `RIFLE_crouchfire`) | ✅ HECHO (decisión: **set servido** — es el de Vice Extended; movements.img es del addon ClassicAXIS) | `10` §Resultados A4 |
| **A5** | `ViceEx.RAW/SDT` vs nuestro audio servido; `gamecontrollerdb.txt` → mapeos útiles para Gamepad API (opcional) | ✅ CERRADO (24/09: escucha del jugador completada → `viceex-map.tsv` relleno; 0/1 rotos de fábrica (ruido) = vacías; 5/6 y 7/8 copias exactas; oído refuta la envolvente en 5/6, 9/10 y 12) | `10` §Resultados A5 + §A5 escucha cerrada (tabla oído vs envolvente + forense RAW/SDT) |

### B. Código externo — leer antes de implementar

| # | Ítem | Estado | Dónde |
|---|---|---|---|
| **B6** | `CHANGELOG-VC.md` de SilentPatch (10,6 KB): la lista **canónica** de fixes → clasificar aplicable/sí-no + punto exacto de `src/` | ✅ HECHO | `08` §B6 (~100 fixes clasificados) |
| **B7** | `GTAVC.WidescreenFix/dllmain.cpp` (MIT): las fórmulas **exactas** de FOV/HUD | ✅ HECHO (stub localizado; fórmulas reales en `Draw.ixx`/`Sprite2d.ixx`/`Camera.ixx`) | `10` §Resultados B7: tabla fórmula → fichero:línea → punto en `src/` |
| **B8** | `classic-axis/source/CamNew.cpp` + `Main.cpp`: cámara IV + `bForceLegsMovements` | ✅ HECHO (pseudocódigo + constantes + semántica del flag del `ClassicAXIS.ini`) | `10` §Resultados B8 |
| **B9** | `skygfx_vc/src/{matfx,leedsCarpipe,WaterLevel}.cpp` + HLSL→GLSL | ✅ HECHO (spec pass a pass con los 24 HLSL a la vista) | `10` §Resultados B9 |
| **B10** | El resto del árbol SilentPatch (el listado se truncó) — localizar los fuentes VC concretos | ✅ HECHO (mapa de secciones namespace:línea + remate identificado) | `10` §Resultados B10 |

### C. CLEO-lite F1 — cerrar la decodificación

| # | Ítem | Estado | Dónde |
|---|---|---|---|
| **C11** | Fijar el formato de ids de `vc.json` (colisiones dec/hex que falsearon nombres) | ✅ HECHO (regla: 4 hex sin prefijo → `int(id,16)`; 1.689 ids, 0 colisiones) | `10` §Resultados C11 |
| **C12** | Args variables de `CALL_FUNCTION` → desensamblar `swim.cs` **entero** (se paró en 206/2.288 B) y `CrouchMovement` entero | ✅ HECHO (`tools/cleo_disasm.py` + walks completos + tabla opcode→memory-hack) | `10` §Resultados C12 |
| **C13** | Desensamblar `freeroam_miami.scm` (38 KB de lógica pura = banco de pruebas ideal del desensamblador) y explicar la cabecera `02 00 01 20 86…` de `main.scm` | ✅ HECHO (cabecera = motor (`Script5.cpp`) + empiria: V=u32@3, objetos 8 B, multiscript a 12 B) | `10` §Resultados C13 |

### D. Motor — spikes antes de comprometer features

| # | Ítem | Estado | Nota |
|---|---|---|---|
| **D14** | Spike de **night vertex colors en librw GL3** (X2.11 del mod, lo único del SkyGfx que podría no salir) | ✅ SPIKE HECHO (24/09, sin tocar código = §4 cumplida) | `10` §Spikes D14: uniform `nightParam` = modulación del prelight; dificultad GL3 **BAJA** |
| **D15** | Presupuesto RAM/streaming para **No Island Loading / Seamless Interiors** (medir contra el on-demand de 1,4 GB + IDBFS 900 MB) | ✅ SPIKE HECHO (24/09, solo lectura) | `10` §Spikes D15: **FACTIBLE CON RESERVAS** — mundo entero 363 MB en `gta3.img`, pool MEMFS 360→~700 MB, RSS ≈1,2-1,8 GB; medir en vivo en fase 2 |
| **D16** | E1: probar `HAS_TRANSLATION` en el descriptor con traza + `viceext-log-check` (el clip ya sabemos que es bueno; `07` §14: la raíz está, el fix es el flag del descriptor) | ⏸ FASE 2 | solo en integración/prueba |
| **D17** | Ritual de casa antes de cualquier partida: `bash tools/check-served-build.sh`, `VERSION++`, trazas `odtrace` con criterio PASS por bloque | ⏸ FASE 2 | es el ritual completo de integración |

**Recomendación original del agente** (literal): «A1+A2+B6 son las que más verdad
desbloquean por hora invertida; C11-C13 cierran el CLEO-lite; D14-D15 evitan
sustos de arquitectura». A1+A2+B6 ✅ ejecutadas (doc `08`).

## 4. Luz verde para los spikes D14-D15 (concedida por el jugador, 24/09)

Condición literal del jugador: **«siempre y cuando sea una prueba, o sea haces tu
prueba y a la final dejas el código como lo encontraste»**. O sea:

1. El spike toca fuente SOLO como experimento medible (rama aparte o prototipo
   desechable).
2. **Al terminar, el código queda EXACTAMENTE como estaba** (revertido). Lo que
   sobrevive es el resultado documentado en el plan (medición, hallazgo,
   recomendación), no el código.
3. Si el spike demostrara que la feature merece la pena, su implementación real
   es fase 2 (con traza + PASS + ritual D17).

## 5. Ítems E — añadidos el 24/09 (no estaban en la lista literal)

Nacen de la pasada `08` y del blindaje; continúan la serie sin reciclar números:

| # | Qué | Estado |
|---|---|---|
| **E18** | Validar IDs `6670/6671` vs rangos del mod (6660-6699 etc.) — nació en `08` | ✅ HECHO (sin colisión real: el mod no usa más allá de 6669 y sus ids son dinámicos) |
| **E19** | Explicar el formato interno de `fronten2.txd` servido (+3 MB vs mod) — nació en `08` | ✅ HECHO (+3 MB = 11 logos de emisora 288×260 ya fusionados) |
| **E20** | Regla de merge «6500-6507 sin duplicar» (inline nuestro vs `newVehicles.ide` del mod) — nació en `08` | ✅ HECHO (regla: NO duplicar; secciones MVL ya traducidas inline) |
| **E21** | Zanjar el conflicto del `main.scm` (descartado en `vice-extended-inclusion.md` §8.F vs «servir + regresión» en `05`/`07`/`08`) con inventario de opcodes → recomendación; decide el jugador | ✅ documentado (servido 0 custom vs mod **7 tipos / 206 usos** `0FA0-0FA9`) + recomendación escrita + **minería del `ViceEx.exe`**: fork de reVC/librw, 10 handlers `0x0FA0-0x0FA9` portables (misma API) — ⏳ **DECISIÓN APLAZADA por el jugador a fase 2** (recomendación: conservar el de serie) | `10` §Resultados E21 + §Spikes |
| **E22** | Corregir el % global de integración en `05`/`06` (el jugador: NO es ≈50%; es menos — falta lo más vital y notorio) | ✅ CERRADO (nota literal aplicada en `05` §9 y `06` §5; cifra del jugador: **≈30 % o menos**) |
| **E23** | Inventario completo del `Config/SilentPatchVC.ini` → spec de toggles (pendiente de tanda 1 de `00-INDICE` §8) | ✅ HECHO (10 claves + 3 secciones-lista, 0 sin clasificar) | `10` §Resultados E23 |
| **E24** | Localizar (sin tocar) el fundido del outro que parpadea (SilentPatch `:2384`) → ancla fichero:línea (pendiente de tanda 1 de `00-INDICE` §8) | ✅ HECHO (`Frontend.cpp:2476-2521` + fix `SilentPatchVC.cpp:2384` clamp de alfa) | `10` §Resultados E24 |

## 6. Regla de numeración a partir de ahora

1. **Este fichero es la fuente de verdad.** Todo ítem nuevo se añade en su tabla
   con: número, qué es, base documentada y estado.
2. **Los números no se reciclan ni se reordenan.** Un ítem muerto se marca
   `❌ CERRADO` con motivo y su número queda ocupado.
3. Estados: `⏳` pendiente fase 1 · `✅` documentado/decidido · `🟢 AUTORIZADO`
   (spike con luz verde) · `⏸ FASE 2` · `🔨 INTEGRADO` (build + PASS + check) ·
   `❌ CERRADO`.
4. Un ítem solo pasa a `🔨 INTEGRADO` con: build etiquetada, su traza, criterio
   PASS cumplido y `check-served-build.sh` en verde (ritual D17).
5. Etiquetas: `letra de sección + nº` (A1…D17) para los de la lista literal;
   `E18+` para los añadidos después.

## 7. Índice cruzado de los docs donde vive la sustancia

| Doc | Qué cubre |
|---|---|
| `01-silentpatch-vc.md` | SilentPatch (T1/T2, ~90 fixes desde su readme) |
| `02-frameratevigilante.md` / `03-widescreen-fixes-pack.md` | FV y WFP |
| `04-extraccion-fuentes-mods.md` | inventario mod a mod, qué copiar/adaptar, `climbing` a fondo |
| `05-cleos-adaptador-y-paridad-scm.md` | CLEO, adaptador CLEO-lite, paridad SCM, tabla % por mod |
| `06-fuentes-externas-y-mapeo.md` | repos externos, CLEO-lite en profundidad, mapeo «dónde y cómo», qué NO hacer |
| `07-verificacion-ejecutada-por-mod.md` | todo lo EJECUTADO en solo lectura, por mod |
| `08-pasada-a1-a2-b6.md` | ejecución de A1 + A2 + B6 |
| `10-plan-pasada-siguiente.md` | el plan de la pasada que arranca con aviso del jugador |
