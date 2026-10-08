---
name: freeze-audio-txd-pal8-saves
status: EXECUTED
type: bugfix
domain: engine
owner_rules: .agents
created: 2026-09-15
---

# Plan — freeze en cinemática (audio), TXDs paletizados y saves (15/09/2026)

Estado: RAM 1,3–1,4 GB estable 10+ min (fantasma `txd.img` eliminado). Tres frentes abiertos, diagnóstico cerrado con evidencia del usuario. Todo lo de abajo es **implementación**, no más diagnóstico.

## F1b — Freeze silencioso en cinemática: `CStream::Update` girando

**Evidencia (usuario, pestaña crasheada):**
- 4/4 pauses caen en `CStream::Update()` → `loop` llamando a `$alGetSourcei` (`AL_BUFFERS_PROCESSED`, param 4118), vía `cSampleManager::Service() ← … ← EmscriptenTick()`.
- Network limpio (todo HTTP 200, 4–6 ms, cero `pending`): red absuelta.
- Sin líneas `[od] STALL/REINTENTO/FAIL` de red: el hilo giraba, no estaba suspendido en Asyncify (el watchdog no podía hablar).
- En Network aparece `law_1b.mp3` recién descargado = speech de cinemática. El stream de speech entra en estado sin progreso y el servicio gira para siempre.

**Fix:**
1. Leer el bucle exacto en `src/audio/oal/stream.cpp` (`CStream::Update`): acotar iteraciones por tick en `__EMSCRIPTEN__` (tope + salida si un `alGetSourcei`/relleno no avanza) y marcar el stream como fallido en vez de girar.
2. Endurecer `alGetSourcei` nulo: si `AL.getSourceParam` devuelve `null` ( bridge JS `reVC.js:6784` retorna sin escribir `pValue`), el contador queda con basura → inicializar a 0 antes de cada consulta.
3. Rebuild + validación: cinemática completa sin freeze (la del crash sirve de caso).

## F3b/c — musgo negro: CERRADO (17/09 tarde)

**Causa raíz (probada):** el flag `hasAlpha` del raster GL lo sobrescribía el
**último mip subido**. `gl3raster.cpp::rasterSetFromImage` asignaba
`natras->hasAlpha = image->hasAlpha()` en cada nivel y el camino vivo
(navegador sin S3TC) es el fallback por `Image` de `raster.cpp`, que llama a
`setFromImage` **nivel a nivel**: los mips pequeños de un cutout (4x4, 2x2,
1x1) no tienen transparencia -> flag a 0 para toda la textura -> alpha-test y
blend OFF -> el cutout se pinta OPACO y su zona transparente (RGB negro) sale
como mancha negra. **Fix:** `if(image->hasAlpha()) natras->hasAlpha = 1;`.
Nuevas trazas de cierre: `CONVDXT ALFA` (lvl0A vs finalA por textura) y
`CONVDXT ALFAMIP` (mip sin alfa dentro de un cutout, cupo propio 250).
Herramientas: `tools/probe_alpha.py`, `tools/scan_cutouts.py`,
`tools/scan_mipalpha.py` (firma del bug en todo el juego: 4 texturas de 1361
TXD, tres de ellas vegetación de playa: `kbgrass_test2`, `sjmgrss`,
`fuzzyplant256`). Validación visual: pendiente de sesión del usuario (el arnés
no llega a la playa; sí confirma el estado de carga: `finalA=1`).

### Antecedentes de esta investigación (histórico)

- Sesión corre build nueva (TXDIN-16 + GenericLoad en log). FAILs: solo radar* (+washwater/icons8/HMOTR esporádicos). Modelos 100%, cero SKIP, cero nils.
- `grass2sand` decodificado offline: 100% opaco por diseño (no es el musgo).
- `beachtyretrackbs` (DXT3) y `kbgrass_test2` (DXT1) decodificados offline: cutouts reales con alfa (57% y 83% texels alfa 0).
- Cadena de conversión verificada por lectura: alfa sobrevive (`removeMask` saltado en web para todo DXT1, `hasAlpha()` correcto → C8888). Si el musgo sigue negro con textura cargada y alfa intacta, queda lado material: alpha-test desactivado para ese material, o nivel 0 negro, o bind erróneo. Siguiente dato: traza de estado del material en esas superficies (o confirmación visual de que el fix de alfa ya lo resolvió).

**Evidencia nueva (usuario):** pasto BIEN a la izquierda y NEGRO a la derecha **en el mismo frame** + Malibú/hotel transparentes + andén negro + musgo negro. Misma textura según distancia = fallo de **mipmaps**, no de carga.

**Dos mecanismos separados, ambos confirmados por evidencia:**
- **F3a (load-time, `Failed to load TXD`):** radar/wash/HFYMD/BFOBE/WMYLG son DXT plataforma 8 que mueren en silencio en el fallback de `convertTexToCurrentPlatform` (cero RWERROR). Traza `[texconv]` ya compilada en el wasm; falta el dato (usuario no reportó líneas).
- **F3b (upload-time, negro-a-lo-lejos + transparentes):** textura que "carga" (nivel 0 OK de cerca) pero cadena de mips incompleta vs `MAX_LEVEL`/filtro MIP → WebGL la muestrea como negro transparente `(0,0,0,0)` (documentado MDN/webglfundamentals). En `setFilterMode` (`gl3device.cpp:538`) el filtro MIP se elige por `numLevels > 1`: si los niveles pequeños faltan o suben corruptos (bucle por nivel del fallback sin verificación), de lejos = negro, con alfa = transparente. El path software-DXT ya tiene clamp `MAX_LEVEL`; la vía viva (d3d8→`toImage`→GL3) NO lo verifica.

**Resultado odtrace 15-16/09 (leído por mí):** FAILs frescos y acotados: `radar*` + `WFOSH/BMOTR/HMOTR`. Red limpia (todo 200, tamaños OK), ficheros sanos en disco, **cero nils en lectura (`hackRead` mudo) y cero en conversión (`texconv` solo 4 clamps buenos)**.
- Conclusión firme: el nil está ANTES de leer texturas: `FindChunk(STRUCT)`/conteo en `RwTexDictionaryGtaStreamRead` (`TexRead.cpp:82-88`) o `FindChunk(TEXDICTIONARY)` en `LoadTxd` (`TxdStore.cpp:131`). Ambos = stream en memoria mal posicionado/corto al entrar ⇒ **capa CdStream** (nuestro backend sector→fichero), determinista por fichero (los reintentos fallan idéntico).
- Teoría del usuario adoptada: transparente/negro/beige = misma textura no cargada (una causa F3a). Alfa-render solo si con cero FAILs algo sigue roto.
- Siguiente (una build, yo leo el log): traza a la entrada de `LoadTxd`: streamId, offset/sectores pedidos, bytes reales en `mem`, primeros 12 bytes. Comparar un TXD que falla (radar60) con uno que carga: la diferencia dice mapping vs contenido. Fix donde caiga.

**Antecedentes (ya verificados):**
- Parse offline de los TXD (`plat=8`, layouts D3D8): `radar60` = C565/DXT1 128x128 1 nivel; `washwater` = C4444/DXT3; `HFYMD/BFOBE/WMYLG` = MIPMAP|C565/DXT1 9 niveles. **Ninguno es paletizado** (mi rama `usePalettized` en gl3 solo corre para plataforma 12: código muerto inofensivo).
- Ruta real: `Texture::streamReadNative` despacha por plataforma (`texture.cpp:476`): plataforma 8 → `d3d8::readNativeTexture` → `convertTexToCurrentPlatform` → `d3d_to_gl3` nil sin S3TC (`raster.cpp:430`) → fallback por `Image`. Vía viva: `rwNativeTextureHackRead` (`src/fakerw/fake.cpp:308`).
- `streamed-404.log` limpio + cero `RWERROR` en consola: llegan bien, mueren en silencio (mensaje de `Streaming.cpp:632`).
- Calles beige (sin textura, solo vertex colors): tercer mecanismo pendiente (modelo antes que su TXD por latencia); se re-evalúa cuando no haya FAILs.
- **16/09 noche (sin código): los TXD del Malibú (`cl_ext1/2`, vía .ide de `club_exterior*`) NO están en el set FAIL** → su transparencia NO es fallo de carga del dict. Teoría principal ahora: dicts parciales — el `continue` web salta texturas sueltas nil y el dict "carga" incompleto (sin FAIL): los materiales sin textura se ven transparentes/negros/beige según material. Siguiente dato: loguear QUÉ texturas se saltan (nombres) por TXD para confirmar que Malibú/musgo/beige corresponden a skips.

## F2 — Saves y ajustes (ampliado 15/09: no persiste nada nuevo)

**Síntomas (usuario):**
- 3 slots guardados en sesión → tras recargar solo aparece 1 viejo (de hace ~4 h, era 2 GB). Lo guardado antes del crash se perdió.
- Ajustes (gráficos, controles, idioma) tampoco persisten.
- El slot viejo NO viene precocido en `bootseed.list` (verificado): se sincronizó una vez con suerte; lo nuevo nunca baja a disco.

**Causa raíz (verificada en código, sin tocar nada):**
- Escritura OK en sesión: `/userfiles` montado IDBFS (`web/src/main.js:184-190` + `syncfs(true)` al boot) y `__wrap_fopen` deja pasar a `__real_fopen` lo que no está en el manifiesto → saves, `gta_vc.set` (vía `SetDirMyDocuments`) y `reVC.ini` (absoluto `/userfiles/reVC.ini`, `re3.cpp:200`) caen en IDBFS en memoria.
- Persistencia a IndexedDB SOLO en `beforeunload` (`main.js:58-63`, fire-and-forget). El navegador no garantiza completar IDB asíncrono durante teardown; en crash/freeze ni se dispara. Resultado: todo lo de la sesión se pierde siempre.
- `shell.html:48-71` tiene un SEGUNDO cableado IDBFS (monta `/vc_user`): probable legacy; verificar si se usa o es peso muerto (Vite sirve `index.html`).

**Fix (aplicado 15/09 parcial, pendiente validación):**
1. `C_PcSave::SaveSlot` (éxito) y `DeleteSlot` → `EM_ASM FS.syncfs` (`src/save/PCSave.cpp`, macro `WEB_FLUSH_USERFILES`).
2. `CMenuManager::SaveSettings()` final → `EM_ASM FS.syncfs` (`src/core/Frontend.cpp:3295`): cubre `gta_vc.set` + `reVC.ini` (`SaveINISettings`) de una vez, cada vez que cambia cualquier ajuste.
3. Red: `setInterval` 30 s + `visibilitychange` en `web/src/main.js` (+ `beforeunload` existente).
4. **Hallazgo 15/09 (wasm nuevo confirmado por el reload de F4, hooks vivos, y aun así cero entradas de hoy en IDB): los saves NO caen en `/userfiles`.** Causa: `DefaultPCSaveFileName` (`PCSave.cpp:22`, `%s\GTAVCsf` relativo) y `gta_vc.set` (vía `SetDirMyDocuments`, relativo) dependen del CWD, que cambia (precedente idéntico: `reVC.ini` se absolutizó por esto mismo en `re3.cpp:201`). En sesión listan bien; tras recargar no existen. **Pendiente: absolutizar ambas rutas a `/userfiles` (mismo patrón que `reVC.ini`).**
5. Limpieza: decidir `shell.html` (si no se sirve, fuera del camino).
6. Validación: guardar slot AHORA → mirar IDB a los 30 s (debe aparecer `GTAVCsf*.b` con timestamp de hoy); cambiar ajuste → `gta_vc.set` por primera vez; recargar → todo sigue.

**Necesito del usuario (1 min, DevTools → Application → IndexedDB):**
- ¿Qué BBDD existen (`vcod2`, alguna de IDBFS tipo `EMSCRIPTEN_FS`)? ¿Tienen stores con ficheros tras guardar un slot? Esto confirma que el mount IDBFS está vivo y solo falta el flush.
- **Confirmado por usuario 15/09 (captura):** existe BBDD `/userfiles` con store `FILE_DATA` (ese es el IDBFS de Emscripten: una BBDD por mountpoint) con solo 2 entradas: `/userfiles/GTAVCsf1.b` (14/09 15:40, el slot viejo) y `/userfiles/reVC.ini` (14/09 20:32). Nada de las sesiones de hoy. `vcod2/files` (2973 entradas) es solo la caché OD, no hay que revisarla.
- Conclusión: mount y escritura OK; el flush solo gana la carrera del `beforeunload` de vez en cuando (2 suertudas en 2 días). `gta_vc.set` ni siquiera llegó a IDB nunca.
- **Hallazgo 15/09 (consola usuario): `warning: N FS.syncfs operations in flight at once` escalando hasta 7.** Los syncfs se APILAN sin completar: mi intervalo 30 s + EM_ASM por save/settings + visibilitychange + beforeunload se solapan (Emscripten avisa de no solaparlos) y la cola IDB se atasca → ningún flush completa hoy. **Fix: serializar (flag en JS: si hay uno en vuelo, saltar) + absolutizar rutas (punto 4).** Sin esto, ni el flush explícito sirve.

## F2-load — WIPE POST-CARGA (16/09 noche, probado en log, fix aplicado)
- Secuencia log: `GenericLoad OK` + `loaded pos` → 0.1-0.3 s después `WR wipe state=INIT_PLAYING load=0` (tres veces). El wipe pisa lo cargado.
- Mecanismo: el caso 2 limpiaba `m_bWantToLoad=false` al entrar; los ticks restantes del restart caían al `else` de `AfterInner` (wipe `INIT_PLAYING`). El fix de estado quedaba sobrescrito al tick siguiente.
- Fix (build 16/09 18:31): no limpiar al entrar; se limpia al completar (glfw, ya existía) o en fallo (rama else, ya existía). Comportamiento de fallo/nueva partida intacto.

**Prueba en log del usuario (misma sesión):** `save slot=0 pos=219.4,-1273.6` (22:26) y `save slot=1 pos=219.7,-1273.6` (22:28) vs `loaded pos=220.3,-1275.5` — guardar y cargar devuelven EL MISMO SITIO (metros). El tracking es live (0.3 m entre saves). Mecánica probada.
**Lectura honesta:** dos saves a 90 s y 0.3 m = quieto (testeando o afk). Si ese sitio ES el inicio (se guarda al tomar control tras la intro), cargar devuelve el inicio CORRECTAMENTE y parece roto sin estarlo. Test discriminante (1 min, solo el usuario puede): guardar LEJOS del inicio (otra isla, Malibú, estadio, anotar el sitio) → cargar → ¿mismo sitio (FUNCIONA, caso cerrado) o inicio (bug real: override de spawn)? Sin esto no se puede concluir más.

**Secuencia exacta del bug (código + logs del usuario):**
1. Menú carga slot → validación TRUE + `GenericLoad` TRUE (ambos en `odtrace.log`, slots 6 y 8). El save SE CARGA de verdad.
2. Al terminar el restart troceado, **nadie toca `gGameState`** (sigue `GS_FRONTEND`).
3. Tick siguiente: `InnerFrame`, caso `GS_FRONTEND` (`glfw.cpp:2209`): como el menú ya se cerró, pone `GS_INIT_PLAYING_GAME`.
4. Siguiente: caso `GS_INIT_PLAYING_GAME` → `InitialiseGameStep()` → **partida nueva → intro**. Lo cargado queda huérfano.
5. Por eso: validación OK + carga OK + intro siempre. Desde pausa funcionaría (estado ya PLAYING); desde el menú principal, jamás.
**Fix (quirúrgico, solo web, solo rama éxito):** al completar `GenericLoad` con éxito en el restart troceado (`Game.cpp` caso 2), fijar `gGameState = GS_PLAYING_GAME`. La rama de fallo y la de nueva partida quedan intactas (su hijack a INIT_PLAYING es el comportamiento correcto ahí).
**Antecedentes ya cerrados:** `LoadFileName` vacío (fix aplicado, validación TRUE lo prueba), rutas absolutas (IDB con entradas de hoy), flush visible+serializado+por-escritura, `DeleteFile` con `/`, `FED_LFL` ausente porque la ruta web lo omite por diseño.

## F4 — Salir al menú en vez de matar la pestaña (nuevo)

**Síntoma (usuario):** salir del juego desde el menú no debería hacer nada visible más que devolver al menú; hoy mata el juego (solo queda cerrar la pestaña).

**Causa (verificada):** menú Quit → `DrawQuitGameScreen` → `RsEventHandler(rsQUITAPP)` (`Frontend.cpp:5752/5794`) → `skeleton.cpp:216-222` pone `RsGlobal.quit = TRUE` → el tick ve `InnerShouldRun()==false` → `emscripten_cancel_main_loop() + FinalCleanup()` (`glfw.cpp:2485`) → canvas muerto. En PC eso terminaba el proceso; en web no hay proceso que terminar.

**Fix (aplicado 15/09 v1, confirmado en uso):** en `__EMSCRIPTEN__`, `rsQUITAPP` ya NO pone `RsGlobal.quit` (`src/skel/skeleton.cpp`): hace `FS.syncfs` + `location.reload()`. Confirmado por usuario (salta el modal de recarga = el código corre).
4. UX pendiente (reporte 15/09): el reload dispara el `beforeunload` y sale el modal "¿Quieres volver a cargar?" — suprimirlo con flag cuando el reload es intencional (quit). Retorno al menú *sin recargar página* queda como spike futuro (requiere teardown gameplay→frontend inexistente en el motor).

## Revisión cruzada otro agente (16/09) — veredicto honesto

**Donde tiene razón (se adopta):**
- F2: mi fix `LoadFileName` no afecta a la validación (`CheckDataNotCorrupt` reconstruye el nombre; ya lo sabía, lo vendí mal como "raíz" cuando solo ayuda a `GenericLoad`/`RestoreForStartLoad`). Correcto y aceptado.
- F2: `TRUE` de `GenericLoad` ≠ contenido=progreso. Falta discriminar pos/misión/reloj al guardar vs tras cargar (ya estaba en mi plan pendiente).
- F2: carrera de lectura IDB — `syncfs(true)` sin `addRunDependency`, el juego no espera. Real aunque enmascarada por timing. Fix barato: dependency hasta completar.
- F2: re-derivar `LoadFileName` en `GenericLoad` desde `m_nCurrSaveSlot` en vez de fiarse del global (endurecimiento barato y correcto).
- F3: traza `sectors .dir vs fsize` por cada FAIL + entrada a `LoadTxd` (offset/sectores/bytes/first-12, failing vs working). Es el siguiente dato correcto.
- F3: `GtaStreamRead1:139` (big-path) sigue destruyendo el dict entero — inconsistencia real con mis `continue` (aunque nuestros FAILs son small-path; alinear igual).
**Donde no tiene razón / matices con prueba:**
- Vía de escritura "no probada": las entradas de hoy en IDB (`GTAVCsf4.b`, `reVC.ini` con timestamp de hoy) PRUEBAN que escribe en `/userfiles`. Dato > teoría.
- Cola con ceros (`OdDoRead` memset): los ficheros sueltos ya traen el slack del `.img` (radar60: 10240 en disco vs 8360 contenido) y el parseo es size-driven, no llega a la cola. El log sectors-vs-fsize lo confirmará o refutará.
- Asyncify anidado en `open()`: especulativo, sin evidencia (cero STALLs); se revisa después, no ahora.
- No toca mi causa raíz de estado (`GS_FRONTEND`→hijack→intro), que sigue en pie con logs.
**Acuerdo:** fusionar — sus trazas + las mías en UNA build, yo leo `odtrace.log`.

## F3a — CAUSA RAÍZ: lecturas agrupadas cortadas (16/09 noche, fix aplicado)
- Dato: `ODSHORT` muestra `want` > fichero SIEMPRE empezando en offset 0 (radar19: quiere 15, hay 5; nbhospgrnd: 67 vs 22...). Cero MISS, cero open-fail.
- Causa probada por lectura (`Streaming.cpp:2215-2244`): el motor agrupa hasta 4 ficheros adyacentes en UNA lectura. Nuestro backend servía solo el primero + ceros. Los TXD 2º-4º del grupo parseaban ceros → FAIL determinista, siempre en parejas vecinas (36/37, 44/45, 60/61 — tal cual el log).
- Por eso en fantasma iba todo: `txd.img` es un archivo continuo, sin lógica por-fichero.
- Fix (build 16/09 21:09, `CdStreamPosix.cpp`): servir en cadena entre entradas como un `.img` real; ceros solo en huecos sin entrada. MISS inicial sigue siendo error.
- Tabla de veredicto (supervisor): head MEMFS OK + length OK + FindChunk OK tras el fix ⇒ FAILs a cero. Cierre: radar61 3 sesiones sin FAIL radar* + minimapa con tiles.
- `.dir` verificado byte-idéntico al retail: la basura tras nulls es de Rockstar. No regenerado. Caso cerrado.
- `OdDoRead` split 3 salidas (MISS/open-fail con errno+fp/short con fileOff/fsize/got/want/entsec), acotado 60, odtrace+consola. Sin cambio de comportamiento (se restauró `lastHit` intacto).
- `TXDIN` extendido a head 16 B (cubre small+big path; el trace en TxdStore.cpp sería redundante).
- Prohibición respetada: sin tocar TexRead.cpp, fake.cpp ni cooldown en esta pasada.
- Criterio de cierre acordado: radar61 carga 3 sesiones seguidas con cero FAIL radar* + minimapa con tiles visibles.

## Orden de ejecución (16/09 noche)

**NOTA BUILD (17/09): el CMake del sistema desapareció (`C:\Program Files\CMake` vacío) y ninja quería regenerar → todo build fallaba con `CreateProcess failed`. Solución: `python -m pip install cmake` (4.4.3, user-local) + reconfigurar (`emcmake cmake -S . -B gta_vc_browser/build/web`). En CADA build futura anteponer al PATH: `C:\Users\s0rno\AppData\Local\hermes\hermes-agent\venv\Lib\site-packages\cmake\DATA\bin`.
F1b (aplicado: `stream.cpp` tope 8 + seguir con izquierdo) → F3 (aplicado: rama `usePalettized` en `gl3raster.cpp`) → F2 (aplicado) + F4 (aplicado v1). Validación siempre jugando: cinemática (F1b), capturas Malibú/calle (F3), slots+ajustes (F2), salir al menú (F4).
