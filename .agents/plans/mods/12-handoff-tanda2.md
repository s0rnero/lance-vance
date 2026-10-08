---
name: 12-handoff-tanda2
status: EXECUTED
type: research
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 12 · HANDOFF TANDA 2 — Todo lo que necesitas saber para retomar la fase 2

> **Para el agente que abra este fichero**: este documento te convierte en el
> agente que dejó el trabajo. Léelo entero, sigue el orden de la §11 («cómo
> arrancar»), y no reabras ninguna decisión de la §5. Si algo de este doc
> contradice un plan más antiguo, **este doc manda** (es el más reciente).
> Fecha de escritura: **24/09/2026**. Rama: `miami`. Build servido: **`ve56`**.

---

## 1. La meta (qué estamos construyendo)

Port de **GTA Vice City** (reVC, rama `miami`) a **WebAssembly**, jugable en el
navegador dentro de `gta_vc_browser/`, **integrando el mod Vice Extended**
(`mods/extended/`) y otros mods de `mods/`, por **tandas de integración** que el
jugador verifica en sus partidas.

Reglas de oro del proyecto:

1. **Copiar/adaptar código funcional de los mods. Nunca reinventar.** Los mods
   son la especificación: si Vice Extended hace X, el port hace X.
2. **Cero integración sin criterio PASS.** Cada cambio se cierra con un criterio
   de éxito medible (normalmente por el jugador en partida) y su traza si aplica.
3. **Documentar antes y después.** La verdad del proyecto vive en
   `.agents/plans/` (planes) y `docs/mods/ATTRIBUTION.md` (manifiesto de
   atribución). Nada queda `PENDIENTE` sin motivo escrito.

## 2. Estado del mundo al dejarlo (24/09/2026)

- **Fase 1 (extracción/documentación): CERRADA.** Todo el conocimiento está en
  `.agents/plans/mods/00` … `11`. Nada del juego se tocó en fase 1.
- **Fase 2 (integración): en marcha. Tanda 1 CERRADA en build `ve56`**:
  - **E24 · parpadeo del outro** (`SilentPatchVC.cpp:2384`) → **PORTADO** en
    `src/core/Frontend.cpp:2476-2496`: el paso del fade (`m_nMenuFadeAlpha += 20`)
    cruzaba 240→260 y el frame con alfa 260 trunca a uint8 (splash a 4, logo a
    251 = frame claro). Fix: clamp en origen `Min(m_nMenuFadeAlpha + 20, 255)`.
  - **Duración del outro** (`SilentPatchVC.cpp:4086`) → **PORTADO** en
    `src/core/Frontend.cpp:5914-5955` + `src/core/config.h:454-456`: tick
    `>32` ms y cuenta 75 ≈ **2,5 s a cualquier fps** (antes: 750 ms con
    `MUCH_SHORTER_OUTRO_SCREEN`, hoy desactivado).
  - **Random de 16 bits** (script y spawn de coches) → **YA ESTABA** vía
    `USE_PS2_RAND` (`config.h:276` ⇒ `MYRAND_MAX`=65535). No se tocó código.
- **PROGRESO 24/09 (build `ve57`, enlazado y verificado con `check-served-build.sh`): Tanda 2 EJECUTADA** — :1372/:2453/rotor/dummies-ambulan-firetruk/audio-oido PORTADOS; :802/:1454/autopiloto/B8-piernas YA ESTABA; bocina-FV NO APLICA; IPL-8-zonas YA CUBIERTO (0 cambios); PENDIENTE motivado: rueda-railes, swim-12/reload-2, resto Tanda 3 (detalle en `docs/mods/ATTRIBUTION.md` §6 e HISTORIAL). `dataTag` sigue `ve13` (sin cambios de datos).
- **Faltan: resto de Tanda 3** (reparto por coste, §6) + **D15** (streaming
  del mundo entero, el último) + la **decisión E21** (main.scm del mod).
- El árbol **no está commiteado** (regla: sin commits salvo petición explícita).
  Cambios vivos: `src/core/Frontend.cpp`, `src/core/config.h`,
  `docs/mods/ATTRIBUTION.md`, `gta_vc_browser/web/lib/index.js` (`VERSION`
  = `2026-09-24-ve56`), y los planes `00/10/11/12`.

## 3. Reglas de casa (obligatorias — se cumplen siempre)

| Regla | Detalle |
|---|---|
| `.cpp` del proyecto = **CRLF** | Editar con **python binario** (heredoc `<<'PYEOF'`), con `assert data.count(old) == 1` por cada reemplazo. Nunca un editor que convierta a LF. |
| `VERSION` en `gta_vc_browser/web/lib/index.js` | **Subir en CADA build** (hoy `2026-09-24-ve56`). Va también en la URL del motor (`reVC.js?v=...`): sin subirlo, el navegador sirve el `.wasm` de la caché y el jugador mide el binario viejo (pasó el 23/09). |
| `dataTag` en `gta_vc_browser/web/ondemand.js` | Hoy `2026-09-21-ve13`. Subir **solo** con cambios de datos servidos (IPL, sfx, TXD…). |
| Build | `export PATH="/c/Users/s0rno/emsdk/upstream/emscripten:/c/Users/s0rno/emsdk:$PATH"` y luego `bash gta_vc_browser/build.sh`. **`emsdk_env.sh` NO basta en esta shell** (no mete emcc en el PATH). Salida: `gta_vc_browser/web/public/build/reVC.js|reVC.wasm`. |
| Objeto único (rápido) | `cd gta_vc_browser/build/web && ninja src/CMakeFiles/reVC.dir/core/<Fichero>.cpp.o` (sin enlazar). Compilar así antes de pedir nada al jugador. |
| Antes de pedir partida | `bash gta_vc_browser/tools/check-served-build.sh` (desde la raíz del repo). Debe decir `OK: el motor servido lleva TODAS las marcas`. El `AVISO: no encuentro web/lib/index.js` es cosmetico de rutas, no rompe. |
| Sin commits / git add / push | Ni commits ni staging amplio. Si hay duda de propiedad de un fichero, dejarlo sin tocar y explicarlo. |
| No matar procesos node | Ni otros procesos ajenos. Servidores de otros hilos pueden estar vivos. |
| Medición = el jugador | Él juega y reporta (logs de sus partidas, su oído, su vista). No hay palancas de URL/consola. Tú defines el **criterio PASS** antes de que juegue. |
| Escritura permitida (fase 2) | `src/`, `gta_vc_browser/tools/`, `gta_vc_browser/web/`, `docs/mods/`, `.agents/plans/`. **Nunca**: assets del juego, datos personales, código fuera del port. |
| Consola Windows = cp1252 | Cualquier script python con acentos: `PYTHONIOENCODING=utf-8 python ...`. |

## 4. Cómo trabajar como el agente anterior («volverte Buffy»)

1. **Lee antes de tocar**: `00-INDICE.md` (mapa y reparto por tandas),
   `08-pasada-a1-a2-b6.md` §B6 (catálogo canónico de ~100 fixes SilentPatch con
   ✅/⚙️/❌), `01-silentpatch-vc.md` (tablas T1/T2 con criterios PASS),
   `06-fuentes-externas-y-mapeo.md` §3 (mapeo externo → `src/`),
   `09-numeracion-a-b-c-d.md` (estados), `11-handoff-fase2.md` (contexto de la
   fase 1 y §11 «trampas ya pisadas»).
2. ~~**Regla previa antes de portar NADA**~~ — **ANULADA el 28/09/2026 por el jugador.
   No se vuelve a aplicar.** No preguntes «¿el motor ya lo hace?» ni anotes
   `**YA ESTABA**` para saltarte un port. Ese estado **sigue existiendo** en
   `docs/mods/ATTRIBUTION.md` como **registro** de lo que se hizo en su día, pero ya
   **no es una exc para no trabajar**.
   **Por qué se anula:** se usó como excusa para no portar siete cosas del
   ClassicAXIS (C1/C2, C4/C5, C9, C18-5, C7, C11, `StoriesAimingCoords`) y el
   resultado medido el 28/09 fue que la mitad de la mecánica del mod no funcionaba,
   con un verificador que daba `OK` mientras el cuerpo iba 90° desviado. reVC tiene
   muchas capas encima (cámara de ratón, `bFreeCam`, strafe, free-aim) y **parecerse
   no es ser lo mismo**: la equivalencia hay que **demostrarla leyendo los dos lados**,
   y si no se demuestra, **se porta el del mod**. El jugador: *«mucho de lo hecho
   actual no sirve para nada o tiene un enfoque totalmente erróneo al del mod»*, y
   pidió portar al máximo *«para ahorrarnos reinventar la rueda»*: copiar lo que
   funciona en vez de rehacerlo a ojo.
   **Lo que sí se sigue evitando:** escribir dos veces lo mismo *dentro de nuestro
   propio* código (RULES 0.6), que es otro caso y se comprueba leyendo.
3. **Cada bloque portado lleva cabecera de atribución** justo encima, con este
   formato (copiar el de `Frontend.cpp:5914` o `main.cpp` LBAR):
   ```
   // PORTADO — <Mod> (MIT, © <año> <autor>)
   //   <URL del repo>
   //   <FicheroDelMod>:<línea> («título literal del bloque en el mod»)
   // Qué se toma: <la idea/números>. Adaptación: <qué cambia en nuestro motor>.
   // Medible: criterio PASS = <lo que el jugador debe ver/oír>.
   ```
4. **Fila nueva en `docs/mods/ATTRIBUTION.md`** (tabla por secciones): origen
   (`SilentPatchVC.cpp:línea`), qué arregla, nuestro destino
   (`fichero:línea`), estado `**PORTADO** (`veNN`)` / `YA ESTABA` / `NO APLICA` /
   `PENDIENTE (motivo)`.
5. **Un fix por vez**, anclado a `fichero:línea`, con traza `ODTRACES(...)` cuando
   sea medible por logs (convención ya usada: `SIRENA tipo=`, `LBAR w=`, y sus
   bloques `SR`/`LB` en las herramientas de verificación de `tools/`).
6. **Actualiza los planes al cerrar**: fila del `00-INDICE` §8, progreso del
   `11-handoff` §8, y este doc si cambia el orden.
7. **Decisiones del jugador = ley.** No reabras las de la §5. Si necesitas
   elegir entre enfoques, pregunta con opciones; si es reversible y menor, elige
   lo sensato y déjalo anotado.
8. **Honestidad brutal en el estado**: si algo falla o no se pudo verificar, se
   escribe tal cual. Un `PASS` sin medición es mentira.

## 5. Decisiones ya tomadas — NO REABRIR

| Decisión | Veredicto |
|---|---|
| **E22** (% global de paridad) | **≈30 % o menos**. Ya aplicado en `05` §9 y `06` §5. |
| **E21** (main.scm del mod) | **APLAZADA** a decisión del jugador; recomendación registrada = **conservar el de serie**. Técnicamente viable: los 10 handlers `0xFA0-0xFA9` del `ViceEx.exe` son portables (API de intérprete idéntica: `CollectParameters`/`StoreParameters`/`GetPointerToScriptVariable`). |
| **A4** (animaciones agachado) | Se sirve el **set del mod** (SERVIDO; `GunCrouchFwd/Bwd` en `tmp/a4/`). |
| **A5** (audio) | Tabla de oído **FINAL** (§7). 0/1 descartadas (rotas de fábrica). |
| **Outro** | 2,5 s legibles (`MUCH_SHORTER_OUTRO_SCREEN` desactivado). |
| Grado de integración | Los fixes de SilentPatch/otros entran por tandas con atribución; **sin** infraestructura Win32 (hooks, `hook::pattern`, direcciones, D3D8, DLL injection). |

## 6. El orden — qué queda y en qué secuencia

Reparto por coste (`00-INDICE` §8). Tanda 1 ✅. Ahora:

### Tanda 2 · medio — «lo que el jugador ve» (SIGUE AQUÍ)

**Bloque A — sin descarga (empezar por aquí):**
1. **Textos blancos** (`SilentPatchVC.cpp:802`): mensajes grandes que duran más a
   alta resolución → `src/text/Messages.cpp` + `src/renderer/Hud.cpp`. Es el
   sospechoso nº1 del bloque R3b y un fallo ya reportado por el jugador. PASS:
   la duración de los mensajes no depende de la resolución.
2. **SilentPatch ya reportados** (arreglos pequeños con traza):
   coche que explota dos veces (`:1454` → `src/vehicles/Automobile.cpp`),
   sirena del FBI Washington (`:1372` → `Automobile.cpp`/audio),
   casquillos de Python/Sniper/Laser (`:2453` → `src/weapons/Weapon.cpp`).
3. **Luces de servicio** de `ambulan`, `firetruk`, `fbicar`, `vicechee` — mismo
   método que `ve24` (dummies `servicelights_*`; en estos modelos cuelgan de
   `chassis_dummy`, así que se encuentran). Ya verificado en datos.
4. **FV** (FramerateVigilante): rotor, rueda en raíles, bocina/sirena,
   autopiloto — medir **por segundo** a 35 y 120 fps (plan `02`).

**Bloque B — datos, agruparlos en UN SOLO build** (evita dos re-descargas del
jugador de ~160 MB):
5. **IPL diffs** de 8 zonas (`club`, `hotel`, `littleha`, `mansion`, `oceandn`,
   `oceandrv`, `stripclb`, `washints`): crear/usar
   `tools/apply_ipl_diffs.py`, aplicar a los IPL servidos. Ojo: los `.ipl`
   están en `bootseed.list` (líneas 1698+), o sea **precargados** → tocarlos
   obliga a re-empaquetar el paquete de datos.
6. **Audio ViceEx** (punto 2 de la cola del `11-handoff`): `tools/split_sfx.py`
   para meter las 11 muestras útiles en el banco servido + enganche en
   `src/audio/AudioLogic.cpp` con la tabla de oído (§7). Mapeo de armas y
   sustitución según la tabla.

Ambos bloques del B suben `dataTag` (ve13 → ve14) **una sola vez**.

### Tanda 3 · pesado — dibujo, escala y streaming

- **D14 · night vertex colors** (look nocturno del mod, X2.16): uniform
  `nightParam` + `mix()` del prelight en ~4 shaders GL3 de
  `vendor/librw/src/gl/shaders/` (hoy un solo `in_color`). Los DFF vanilla NO
  traen segundo set de colores: es modulación por pipeline, no datos. Referencia
  HLSL: `gta_vc_browser/tmp/extsrc/skygfx_shaders.txt` (24 shaders SkyGfx).
- **B7/B8/B9**: FOV/HUD widescreen de WFP (fórmulas con `fichero:línea` en
  `10` §B7), ClassicAXIS (`CamNew.cpp` + `bForceLegsMovements` en `10` §B8),
  SkyGfx pass a pass (`10` §B9).
- WFP: `Sprite2d` (UI vs textura vs fundido, letterbox de cinemáticas),
  `Radardisc`, `Loading`/`InteriorLoading`, HUD anclado.
- SilentPatch de render: `:1864` (LOD del edificio en obras), `:1478` (matfx en
  extras), `:1701` (backface).
- Menús a pantalla completa.

### Final

- **D15 · mundo entero en RAM**: pool MEMFS 360 → ~700 MB (gta3.img = 363 MB),
  RSS wasm estimado 1,2-1,8 GB → **solo-desktop al principio**, medir en vivo
  con `odtrace` antes de subir caps. Es el cambio más riesgoso: **el último**.
- **E21** cuando el jugador decida.

## 7. Audio ViceEx — tabla FINAL de oído (24/09, ya en `viceex-map.tsv`)

| # | Qué es (oído del jugador) | Estado |
|---|---|---|
| 0/1 | **ROTOS de fábrica** (ruido; envolvente plana + saturados) | vacías → manda el de serie |
| 2 | **Recarga en general** (NO es bombeo) | útil |
| 3/4 | **shotgun2** (par; variantes reales) | útil |
| 5/6 | **desert eagle** (par; **copias exactas** byte a byte) | útil |
| 7/8 | Disparo sin identificar (par; copias exactas) | útil, arma sin fijar |
| 9/10 | **Francotirador / fusil de precisión** (¿steyr?) (par) | útil |
| 11 | **LANZAMIENTO del proyectil del lanzagranadas** (no es recarga) | útil |
| 12 | **Nadar (swim)** | útil |

Lecciones: la clasificación por envolvente falló en 5/6, 9/10 y 12 — **el oído
manda**. El banco no es solo armas (trae nadar). Archivos:
`gta_vc_browser/tmp/viceex-samples/` (13 WAV + `viceex-map.tsv` relleno;
reproductor HTML en `tools/viceex_samples.html` si hace falta re-escuchar).

## 8. Minería del `ViceEx.exe` (base de la decisión E21)

El `ViceEx.exe` es un **build Debug de un fork de reVC/librw** («gta-extended»):
usa **nuestra misma API** de intérprete ⇒ los handlers son portables.

- Dispatch `CRunningScript::ProcessCommands4000To4099` VA `0x4946C0`
  (`eax = opcode − 4000; if (eax > 9) default; jmp tabla[eax]`), tabla VA
  `0x494A70`, `CollectParameters` `0x472a80`.
- Los 10 handlers = **`0xFA0`-`0xFA9`** (4000-4009). Aridad (pushes antes de
  `CollectParameters`): case 0 `[2,1,2]`, case 1 `[1,2,1,2]`, case 2 `[2,1,2]`,
  cases 3-8 `[1,2]`, case 9 `[2]` → patrón ~2 args + 1 resultado
  (`StoreParameters`); `0xFA9` solo consulta.
- Semántica parcial: `0xFA0` = `pPed` + `CTxdStore::GetSlot` (skin de ped);
  `0xFA9` = vehicle; `0xFA6` = lógica pura; `0xFA4/5/7` comparten callee
  `0x48b120`; `0xFA8`→`0x5e1070`; `0xFA3`→`0x604640`; `0xFA2` (×91)→`0x4f83d0`.
- Uso en scripts del mod: `0FA2`×91, `0FA6`×46, `0FA0`×45, `0FA8`×20, `0FA1`×2,
  `0FA7`×1, `0FA9`×1. Herramientas: `tools/viceex_opcodes.py`,
  `tools/viceex_dispatch.py` (capstone en `tmp/pylibs/`).

## 9. Chuleta de formatos (detalles en `11` §10)

- **Bytecode SCM**: args auto-tag 1=int32, 2=global16, 3=local16, 4=int8,
  5=float32; strings 8 B sin tag; `ARGUMENT_END`=0x00; ANDOR: n=AND de n+1,
  20+n=OR de n+1; not-flag = opcode|0x8000.
- **Cabecera SCM**: `V = u32@3`, objetos 8 B en V+8, cabecera multiscript 12 B,
  `MultiScriptArray[0] == MainScriptSize`.
- **IDs `vc.json`**: 4 hex sin prefijo → `int(id, 16)`.
- **Chunks RW DFF**: cabecera 12 B (id, size, ver), payload hijos desde +12.
- **RAW/SDT de audio**: PCM s16le crudo; offsets contiguos; ojo con las tasas
  (`tools/fix_sdt_rates.py`, `fix_sdt_sizes.py` existen por los quirks).

## 10. Trampas ya pisadas (no repetir)

- Heredoc bash con backslashes python = `SyntaxError` → heredoc **quoted**
  (`<<'PYEOF'`) y construir strings con `str.encode("utf-8")`.
- **En bytes-literals NO usar `\uXXXX`**: `b"...\u2014"` deja `\u2014` **literal**
  en el fichero (pasó el 24/09 en un comentario; corregido).
- `str_replace`: el parámetro `replacements` es **array de objetos**
  `{oldString,newString}`, no un string JSON.
- Consola cp1252: `PYTHONIOENCODING=utf-8` siempre.
- `emsdk_env.sh` no da PATH en esta shell → `export PATH=".../emsdk/upstream/emscripten:.../emsdk:$PATH"` antes de `build.sh`.
- `run_terminal_command` **no tiene modo BACKGROUND** → procesos aparte con `nohup ... &`.
- `preview_screenshot` a veces «no produce frames» → verificar con
  `preview_snapshot` + `preview_evaluate`.
- `code_search` intermitente («Vendored ripgrep not found») → grep por terminal.
- Un build enlazado con el **mismo `VERSION`** se sirve de caché → subir tag en
  cada build (engañó al arnés el 23/09).
- Minería del exe: tabla de salto **NO** es runs de punteros (es
  `jmp [eax*4+cst]` local); capstone desalineado da ruido; el inmediato
  `0xFA0`=4000 decimal produce falsos positivos de timers; VAs de strings:
  calcular con la RVA real de sección (`.rdata` raw `0x2D6400` → RVA `0x2D7000`).
- **Asincronía**: nada de control-flow nuevo dentro de
  `LoadAllRequestedModels` (trampa Asyncify conocida).

## 11. Cómo arrancar TÚ ahora (primer día de la tanda 2)

1. Lee este doc, luego `00-INDICE.md` §8 (tablas de tandas) y
   `11-handoff-fase2.md` §8-§11 (cola + herramientas + trampas).
2. Comprueba el pulso del árbol: `git status --short src/ docs/` y
   `grep -n "VERSION" gta_vc_browser/web/lib/index.js` (debe decir `ve56`).
3. **Empieza por el bloque A, fix `:802` (textos blancos)** — es el que explica
   un fallo real del jugador y destraba confianza. Pasos: leer el bloque `:802`
   de `gta_vc_browser/tmp/SilentPatchVC.cpp` (su comentario explica la causa),
   localizar con grep el cálculo de duración en `src/text/Messages.cpp` /
   `src/renderer/Hud.cpp`, portar con cabecera
   `PORTADO`, fila en `ATTRIBUTION.md`, compilar el objeto, criterio PASS.
4. Sigue con `:1454`, `:1372`, `:2453`, luces de servicio y FV (uno por uno).
5. Cuando el bloque A esté cerrado, **avisa al jugador** y prepara el bloque B
   agrupado (IPL + audio, `dataTag` nuevo, una sola descarga).
6. Al publicar cualquier build: bump de `VERSION`, `build.sh`, y
   `check-served-build.sh` antes de pedir la partida de prueba.

## 12. Contacto con el jugador (cómo se trabaja con él)

- Él mide: sus partidas, su oído (él identificó las 13 muestras de audio), su
  vista. Tú defines el criterio PASS ANTES de que pruebe, y él responde con
  observaciones concretas («el 2 es de recarga en general», «parece ruido»).
- Sus correcciones **mandan** sobre cualquier clasificación técnica previa.
- Decisiones grandes se le consultan con opciones concretas (así se hizo E22,
  E21, A4). Las menores reversibles se toman y se anotan.
- Nada de sorpresas: al cerrar cada tanda, resumen PASS/AVISO por ítem.

---

*Fin del handoff. Si algo aquí está desactualizado porque la tanda 2 avanzó,
actualiza §2 y §6 al cerrar tu sesión — el siguiente agente confía en este doc.*

## 13. MERGE 24/09 -- tandas pendientes + re-trabajo de mecanicas (ESTE p. MANDA)

El jugador une este plan con `13-inventario-mods-fuente.md` y fija este orden
(el propuesto en el 13 p.6). Lo ya cerrado (Tanda 1, Tanda 2 en `ve57`) no se
reabre; lo que sigue va en este orden, un bloque = un build + PASS del jugador:

1. **Recoil por precision del arma** (spec `WeaponRecoilAuto` + jenksta):
   `Weapon.cpp` (acumulador por `accuracy` de `weapon.dat` + retorno) +
   `Cam.cpp` (aplica a mira). Traza `RECOIL2/3` ya existe.
2. **Nado con curva Serega** (mas estados VCS + plantilla MIT ThirteenAG):
   `PlayerPed.cpp` (`ViceExtSwimControl/Move`: deriva 0.022 / crucero 0.05 /
   sprint 0.12 + timeout 300, con `m_vecMoveSpeed`); clips del `ped.ifp` 272.
3. **Agachado calibrado** (spec sa-crouch): 2.74 m/0.731 s + secuencia de
   entrada + deadzone +-16 + filtro de armas; `GunCrouchFwd/Bwd` sueltos
   (`tmp/a4/`) si hace falta. `PlayerPed.cpp` + R20c en `Ped.cpp`.
4. **Apuntado ClassicAXIS+GeniusZ**: `CrosshairMult 0.53/0.4` + hombro
   conmutable + `LockOnTargetType` + sensibilidades `*FOV/80` + near-clip dual
   y offsets por vehiculo. `Camera.cpp`/`Cam.cpp`, `Hud.cpp`, `Pad.cpp`.
5. **FV-MIT** (`GTAmodding/FramerateVigilante`, MIT): `*ms_fTimeStep` donde
   falte (rueda-railes pendiente de medicion 35/120).
6. Despues: resto Tanda 3 (p.6: D14, B7/B9, Sprite2d/Radardisc/Loading, menus,
   `:1864/:1478/:1701`, D15 el ultimo) + E21 (decision del jugador) +
   swim-12/reload-2 (piden id de sonido nuevo).

Reglas que siguen: copiar/adaptar funcional (nunca reinventar), licencias irrelevantes desde 27/09/2026 (`00-INDICE` §2: se toma lo que haga
falta, con o sin LICENSE; la atribucion es cortesia), `VERSION` en cada
build, `check-served-build.sh` antes de pedir partida, PASS del jugador por
bloque.

## 14. REGLA PERMANENTE DEL JUGADOR (26/09) — el mod manda sobre lo existente

Para el nado y todos los mods que sigan: seguro que ya hay código en el árbol,
pero la mayoría está a medias, roto o sin pulir — **se ignora y se prioriza el
código, la lógica y el funcionamiento del mod**. No se parcha por encima de lo
existente: se reescribe desde la spec del mod. **Excepciones**: auto-centrado de
cámara y recoil (recientes, validados, no se tocan).
