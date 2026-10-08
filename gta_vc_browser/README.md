# GTA Vice City en el navegador (reVC + Emscripten)

Puerto del juego. Estado: corre hasta gameplay (menú ES, input, saves, misión).
Página: http://localhost:2077 (solo `web/index.html`).

> Necesitas **una copia legal de GTA Vice City (PC)**. Este repo no incluye
> assets del juego (`AUDIO/`, `MODELS/`, `gta3.img`...): van en `assets/`
> y jamás se commitean (ver `.gitignore`).

## Estructura

```
gta_vc_browser/
  assets/          <- TU copia del juego (ver assets/README.md). No se commitea.
  gamefiles/       <- overlays propios (GXT ES, menús HD, freeroam...). Sí se commitea.
  tools/           <- pipeline de datos (solo stdlib Python + ffmpeg).
  web/             <- app Vite: index.html + src/ + ondemand.js
    public/manifest.json  <- generado (ruta_minúsculas -> real). No se commitea.
  bootseed.list    <- lista del paquete inicial (fuente). bootseed/ se genera.
  build.bat/.sh    <- compilar el WASM (corre en la RAÍZ del repo base re3).
  cmake/ include/  <- shims del build Emscripten.
  patches/NOTES.md <- parches aplicados en src/ + riesgos (referencia del port).
```

Generados en local (no copiar al repo nuevo): `build/`, `streamed/` (~1.4 GB),
`bootseed/`, `radio_light/`, `web/public/build/`, `web/node_modules/`, `*.log`.

## Pipeline de datos (una vez por máquina)

Requiere: Python 3 (stdlib) + `ffmpeg` en PATH (radios y SFX).

```bat
:: 1. copia tu GTA VC PC en assets/ (ver assets/README.md)
:: 2. radios ligeras 22kHz (tarda; una vez)
python tools/convert_radio.py
:: 3. árbol suelto on-demand (assets + gamefiles + radio_light -> streamed/)
python tools/build_streamed.py
:: 4. SFX troceados + tabla (lee assets/AUDIO/sfx.RAW, escribe streamed/Audio/)
python tools/split_sfx.py
python tools/fix_sdt_rates.py
python tools/fix_sdt_sizes.py
:: 5. caché de duraciones de streams
python tools/gen_soundcache.py
:: 6. manifiesto de rutas (lo lee ondemand.js)
python tools/gen_manifest.py
:: 7. paquete inicial de arranque
python tools/stage_bootseed.py
```

Atajo: si ya tienes `streamed/`, `bootseed/` y `radio_light/` generados en otra
máquina, cópialos tal cual (son los pasos 2-7 hechos) y salta al build.

## Compilar y jugar

Requiere: emsdk activado, CMake + Ninja en PATH, submódulos
(`git submodule update --init --recursive`: `vendor/librw`, ogg/opus...).

```bat
C:\Users\s0rno\emsdk\emsdk_env.bat
gta_vc_browser\build.bat        :: salida: web/public/build/reVC.js/.wasm/.data
cd gta_vc_browser\web && npm install && npm run dev
:: abrir http://localhost:2077, pulsar Jugar
```

Primera compilación ~15-30 min (emcc). Incrementales: segundos-minutos.
Si cambian flags de link, el `reVC.data` (~1 GB) se reempaqueta.

## Cómo funciona

- `reVC` (rama `miami`) + `librw` GL3/GLFW + `emcc` -> `reVC.js/wasm` +
  `reVC.data` (paquete inicial `bootseed/`). Monohilo + Asyncify (como dos.zone).
- `web/ondemand.js` (pre-js): sirve el resto bajo demanda desde `/streamed/`
  (fetch + caché IDB). Sin pthreads.
- Guardados (`/userfiles/`) en IndexedDB, con flush por escritura (estilo dos.zone).
- Controles: teclado + ratón (click captura). F12 captura del juego.
- Diagnóstico: consola limpia (solo errores); detalle en `web/odtrace.log`
  (`?oddebug` para verlo en consola). Versión en `OD.buildTag` (título de pestaña:
  recarga con Ctrl+Shift+R si no coincide).
