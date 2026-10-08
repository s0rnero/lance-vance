# Parches Emscripten — aplicados y pendientes

Última actualización: build verde + gameplay (`GS_PLAYING_GAME`, cinemática
renderizada, captura) con Emscripten 6.0.9.
Convención: todo lo web va tras `#ifdef __EMSCRIPTEN__`, sin cambiar conducta nativa.

## Verificado (Chrome headless + SwiftShader)

Arranque → menú en español (captura) → input teclado (captura) →
`GS_PLAYING_GAME` con cinemática inicial renderizada (captura: Tommy con
skinning, texturas e iluminación) a ~60-170 ticks/s. Harness fuera del repo:
`C:\Users\s0rno\AppData\Local\Temp\opencode\vc-e2e` (puppeteer +
`?autostart=1`/`?fsprobe=1`, ganchos en `web/src/main.js`).
Ojo: Emscripten duplica cada `printf` (stdout+stderr van a la consola).

## Aplicados (orden cronológico del debugging)

### A. `Module.locateFile` en la página (el bug del "no carga")
`reVC.js` pedía `/reVC.data` (raíz) y Vite devolvía el index.html (SPA
fallback, 2436 bytes): el FS se llenaba de ficheros de **0 bytes sin error**
y el juego moría en un loop infinito en `CText::Load` (chunk size 0).
`locateFile: p => '/build/'+p` lo arregla. Moraleja: ante cuelgue mudo,
mirar `performance.getEntriesByType('resource')` (transfer sizes).

### B. Reloj: `CLOCK_MONOTONIC` en `psTimer()` (el bug del HUNG)
Emscripten no implementa `CLOCK_MONOTONIC_RAW/_FAST`: `clock_gettime`
fallaba dejando basura (`timer=1.37e14`), y `CTimer::Update()` molía
billones de iteraciones de catch-up (pestaña colgada, `RESULT_CODE_HUNG`).
Con reloj sano el arranque son segundos.

### C. Shaders GLSL ES 3.00 (pantalla negra → menú visible)
El port GLFW acepta el perfil GL 3.3 desktop aunque el contexto sea WebGL2;
librw emitía `#version 330` (ilegal en WebGL) y todo salía negro. En
Emscripten se salta a GLES 3.1 + nuevo `shaderDecl300es` (`#version 300 es`).

### D. `Atomic::RenderCB` devuelve `Atomic*` (crash `signature mismatch`)
`RpAtomicRender → CEntity::Render → RenderRoads` abortaba en wasm:
fakerw registraba callbacks `RpAtomic*(RpAtomic*)` donde librw esperaba
`void(Atomic*)` (x86 lo tolera, `call_indirect` no). Como `RpAtomic` es
`rw::Atomic`, el typedef ahora devuelve `Atomic*` y el cast de
`RpAtomicSetRenderCallBack()` es exacto. Candidato a upstream (ambos repos).

### E. Rutas de audio PC (`#undef PS2_AUDIO_PATHS` en web)
Con rutas PS2 el juego buscaba `AUDIO\CUTSCENE\*.VB` / `AUDIO\MUSIC\*.VB`
(inexistentes en PC) y las cinemáticas quedaban mudas. Sin el define usa la
tabla PC (`.MP3`/`.WAV`/`.ADF`), que sí está en el preload.

### F. Guardados persistentes (IDBFS)
`userfiles/` montado en IndexedDB (`FS.mount(IDBFS)` en `preRun` de la
página + `syncfs` al salir; `-lidbfs.js` en el link). Saves y `reVC.ini`
sobreviven recargas.

### G. Radios excluidas por defecto (ahorro ~504 MB de RAM)
`reVC.data`: 1.47 GB → ~970 MB. Las `Audio/*.adf` no se empaquetan
(`--exclude-file "*.adf"`, opt-in con `--with-radio`); el código de audio
tolera su ausencia (`IsOpened`) y quedan en silencio. `movies/` y `mp3/` de
usuario tampoco se empaquetan.
Intentado y REVERTIDO: lazy-load con `FS.createLazyFile()` — el SDK aborta
en el hilo principal tanto al crear (`!ENVIRONMENT_IS_WORKER`) como en
cualquier `stat/read` desde él (`forceLoadFile`), y el juego abre/mide
ficheros en el hilo principal. Solo serviría si TODO el acceso fuese desde
workers; no es el caso.

### H. Base: plataforma y toolchain- `src/skel/glfw/glfw.cpp`: stub `sysinfo`; gamepad-mapping desactivado (el
  port no implementa `glfwUpdateGamepadMappings`/`glfwGetGamepadState`;
  teclado+ratón OK, mando clásico como joystick); main loop extraído a
  `OuterSetup/InnerShouldRun/InnerFrame/AfterInner/FinalCleanup` +
  `EmscriptenTick` (`tools/refactor_main_loop.py`, one-shot con asserts);
  rama Emscripten en `psSelectDevice()` (modo 0 sintetizado, 1280x720x32 en
  ventana) + traza `[web]` de arranque (una sola vez por etapa, sin spam).
- `src/skel/skeleton.cpp`: traza `[web]` por etapas de `RsRwInitialize`.
- `src/core/config.h`: `USE_UNNAMED_SEM` (sin `sem_open` en Emscripten).
- `vendor/librw/src/gl/gl3device.cpp`: NULL-guard en `makeVideoModeList()`
  (bug latente real: la spec permite NULL).
- `gta_vc_browser/cmake/`: shims `Findglfw3/FindOpenAL/Findmpg123/FindOpenAL…`
  (`FindThreads` incluido) hacia los ports (`-sUSE_GLFW=3`, `-sFULL_ES3=1`,
  `-lopenal`, `-sUSE_MPG123=1`, `-pthread`). Sin tocar CMakeLists del repo.
- `gta_vc_browser/include/AL/`: `efx.h` (+`al.h`/`alc.h` del SDK con
  `AL_APIENTRY`) vía `CPATH`. EFX se desactiva solo en runtime.
- `src/core/re3.cpp`: `reVC.ini` (controles y opciones) a ruta absoluta
  `/userfiles/reVC.ini` en web — con ruta relativa caía en `/` (CWD
  cambiante, MEMFS volátil) y los controles no persistían. `gta_vc.set` ya
  iba a `userfiles/` vía `SetDirMyDocuments()`.
- Canvas según viewport (`EM_ASM` innerWidth, clamp 960–1920, 16:9) en vez
  de 720p fijo: nítido sin reescalar. El menú de resolución del juego sigue
  mostrando 1 modo (pendiente sintetizar varios).
- Página: sin pointer-lock automático (solo clic en canvas), botón 🔊 +
  reintentos de `AudioContext.resume()` (nace tarde y suspendido; no hay
  permiso que aceptar, basta gesto), sonda WebGL (GPU + DXT/ASTC/ETC),
  overlay `gta_vc_browser/mods/` + flag `--mods` para mods de assets.

## Almacenamiento en navegador (veredicto investigado)
- **MEMFS + preload** (lo actual): obligatorio para el grueso, porque el
  juego hace I/O síncrona desde el hilo principal y solo MEMFS la soporta
  (demostrado con el fallo del lazy-load).
- **IDBFS**: solo para `userfiles/` (pequeño, persistente). No sirve para
  assets: poblaría MEMFS igual (misma RAM + arranque lento).
- **OPFS/WorkerFS/Cache+rangos**: no sirven al hilo principal (mismo veto
  que el lazy). Vía válida a futuro: File System Access API (el usuario
  concede su carpeta, cero descarga) + mover la I/O a workers (refactor
  mayor del motor) o servidor local con rangos. Hoy: preload local.
- **F12 del juego** = tecla de captura (`screen_<time>.png` vía
  `RwGrabScreen`); en WebGL falla `glReadPixels` (formato/tipo) de forma
  inocua. No pulsar F12 si abre DevTools a la vez.

## Mejoras nivel dos.zone (2026-09-13)

### I. Radio ligera (504 MB → ~273 MB)
- `tools/convert_radio.py`: XOR 0x22 en Python + `ffmpeg -ar 22050 -ac 2 -b:a 64k`
  (talk KCHAT/VCPR: 16kHz mono 48k). Salida `radio_light/*.adf` con MP3 plano
  dentro (compatible dos.zone). KCHAT 47,6→35,7, VCPR 39,6→29,7, resto ~50%.
- `src/audio/oal/stream.cpp`: `CStream::Open` para `.adf` prueba `CADFFile`
  (XOR) y si falla reintenta `CMP3File` plano. Sin esto la radio convertida
  sonaría corrupta/silencio. Nativo intacto (originales siguen XOR).
- `build.bat/sh --with-radio-light`: mantiene `--exclude *.adf` del preload
  general y añade `--preload-file radio_light@/Audio` encima. `--with-radio`
  sigue siendo originales full.
- Generado local, no commiteado (`radio_light/` en `.gitignore`, copyright).

### J. NPCs negros (texturas DXT)
- `vendor/librw/src/gl/gl3raster.cpp`: `readNativeTexture` ya no aborta sin
  S3TC; decodifica DXT1/3/5 por software a RGBA8 (`dxtDecodeLevel`). Evita TXDs
  enteros en negro en WebGL sin `WEBGL_compressed_texture_s3tc`.
- `src/rw/TexRead.cpp`: en web una textura fallida ya no destruye todo el
  diccionario (`continue` tras `__EMSCRIPTEN__`); el resto del TXD sobrevive.
- `skin.vert` + `neoRimSkin.vert/.inc`: `DoFog(gl_Position.z)` → `.w`
  (igual que `default.vert`); el fog tiñe peds como al mundo.

### K. Freeze al cargar / cargar partida
- `src/core/Game.cpp`: `case 3` partido — init + `DEFAULT.DAT` en un tick,
  `GTA_VC.DAT` al inicio del `case 4`. El peor long-task se parte en dos.
- `CGame::InitialiseRestart{ResetSteps,Step}` (4 tramos): splash/restore,
  `ReInit`, `GenericLoad`+trenes/aviones, post. `glfw.cpp AfterInner` en web
  lo drivea un tramo por tick (`s_restartStarted`) en vez de bloquear 10-60 s.
  Nativo sigue síncrono.

### L. Base on-demand (sin romper arranque)
- `tools/extract_img.py`: extrae `gta3.img` a sueltos en minúsculas (formato
  dos.zone) para futuro. Build actual sigue MEMFS monolítico.
- `web/src/main.js`: `Module.getAsyncUrl` stub (log + `null`) como punto de
  enganche futuro `initFS` mínimo + fetch bajo demanda.
- `build.bat/sh`: `--exclude *.mpg` explícito (NO_MOVIES).

## Pendientes / conocidos (falta prueba en GPU real con sonido)
- **Sonido**: verificar de oído (SFX `sfx.RAW`, diálogos `.MP3`, música).
  EFX/reverb ausente por diseño (suena plano).
- **Radio ligera**: VERIFICADA a nivel motor en headless — `[web] radio lens ms:
  4107206 3587840 6235668 3793188 4644231 5184252 3698233 3546827 3966772`
  (las 9 > 0; FLASH coincide al ms con el mp3 convertido). Falta verificar de
  oído en GPU real (headless va muteado). El `mpg123 ... Giving up searching`
  en consola es benigno (sale al medir longitudes, el stream abre bien).
  Nota: el sniff MP3-sync en `CStream::Open` evita el spam del lector
  equivocado. Build actual en `public/build` lleva radio ligera (1,24 GB).
- **NPCs**: verificar en GPU real con/sin S3TC (forzar `about:flags` o
  SwiftShader). Queda `u_boneMatrices[64]` (~300 vec4, mínimo WebGL2 224):
  en integradas viejas podría truncar skinning; no tocado por riesgo.
- **Fondo del menú**: el panel gris es la textura original `background` de
  `FRONTEN1.TXD` (verificado en captura headless: logo + panel + sombras OK).
  dos.zone usa un `fronten1.txd` custom con su marca (exigido en su README),
  por eso se ve distinto allí. No es bug. El decoder J lo cubre si falta S3TC.
- **BG en trapecio**: es el diseño vanilla — `menuBg` es un `MenuTrapezoid`
  con tilt aleatorio por arranque (`Frontend.cpp:116-119`), texto recto.
  No es bug. Cerrado.
- **Cursor**: `glfwSetInputMode(CURSOR_HIDDEN)` sigue sin existir en el port,
  pero la página ya oculta el cursor del canvas con `pointerlockchange`
  (visible en menús, oculto en juego). Cerrado.
- **Cursor**: `glfwSetInputMode(CURSOR_HIDDEN)` no implementado (inocuo).
- **`alGetProcAddress() without a valid context`**: ruido del port al
  sondear EFX; inocuo.
- **Sin hilos**: si pthreads falla, `build.bat --no-threads`.
- **Rendimiento**: ~60-170 ticks/s en software; en GPU real sobrado.
- **RAM**: sigue monolito MEMFS (~966 MB sin radio, ~1,24 GB con light).
  La dieta real (50 MB + on-demand) exige FS asíncrono + I/O en workers
  (refactor mayor, veredicto vigente).
