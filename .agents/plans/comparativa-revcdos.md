---
name: comparativa-revcdos
status: EXECUTED
type: research
domain: process
owner_rules: .agents
created: 2026-09-20
---

# Comparativa de métodos — revcdos (DOS.Zone) vs nuestro port

> Alcance: ideas y métodos observables desde su documentación pública
> (README) y conocimiento general de Emscripten. **Nada de su código,
> binarios ni assets se ha copiado ni se copiará**; la implementación aquí
> será original. Referencia: `https://github.com/Carter54git/revcdos`.

## Qué hacen ellos (métodos, no código)

1. **Pipeline de datos con Docker** (`revcdos-data-compile.py` + ffmpeg +
   Wine + 7z): convierten `.adf`→MP3, `.mp3`→MP3 minificado, `.wav`→MP3,
   **extraen `.img` en ficheros sueltos** (usando los `.dir`), procesan
   `.raw`→MP3 y empaquetan `output.zip`.
2. **Arranque con ~50 MB + resto bajo demanda**: `game.js` con dos ganchos,
   `Module.initFS` (ficheros de `preload_files.list`) y `Module.getAsyncUrl`
   (URL por fichero bajo demanda, protocolo `postMessage`). "Completamente
   asíncrono".
3. **Controles táctiles/gamepad** (`GamepadEmulator.js`, implementación propia).
4. **Saves**: IndexedDB (`idbfs.js`) + SDK de nube propio (opcional, con
   requisitos de crédito/logo que no nos aplican al no usar sus binarios).

## Qué extraemos (qué hacen mejor y por qué les funciona)

| Tema | Ellos (método) | Nosotros (hoy) | Mejora aplicable (original) |
|---|---|---|---|
| RAM en arranque | ~50 MB: solo lo esencial + resto bajo demanda | ~970 MB de golpe en MEMFS | Pipeline propio Fase 1 (P1 del plan): normalizar audio a MP3 (`sfx.RAW` 324 MB→~30 MB), reempaquetar por tipos. Fase 2: `.img` extraídos + backend bajo demanda |
| Audio | Todo normalizado a MP3 en el pipeline (un solo formato, sin `.adf`/`.VB`/EFX) | mpg123 + wav + adf + shims, EFX desactivado | Adoptar la normalización en nuestro pipeline (idea, no su script): un formato = menos código, menos RAM, menos fallos |
| Carga sin bloqueos | Juego "completamente asíncrono" (FS propio bajo demanda) | `InitialiseGame` monolítico en un frame | Troceado por secciones + FS bajo demanda Fase 2 (ver plan de carga). Sin atajos: es el proyecto grande |
| Controles | Emulador táctil/gamepad propio | Solo teclado/ratón + joystick crudo | Capa propia en `web/` (P2 del plan) |
| Saves | IndexedDB (+ nube opcional) | IDBFS ya implementado | Nada que copiar; el nuestro ya persiste. Nube: solo si se quiere, con SDK propio o ninguno |

## Qué hacemos igual o mejor (mantener)

- Shaders GLES 3.00 y modos de vídeo sintetizados (nuestroRender ya validado
  con capturas hasta gameplay).
- Fixes de corrección que ellos no documentan: `RenderCB` exacto,
  `CLOCK_MONOTONIC`, `USE_UNNAMED_SEM`, NULL-guards (candidatos a upstream).
- Harness E2E con capturas (ellos no muestran ninguno público).

## Qué NO hacemos (límites conscientes)

- No reutilizar su `game.js`, `GamepadEmulator.js`, SDK de nube ni binarios
  (sus términos exigen crédito/logo/nube; además queremos implementación propia).
- No prometer su cifra de RAM sin su refactor de I/O: con preload clásico,
  nuestro suelo realista tras Fase 1 es ~300-400 MB, no 50 MB.

## Decisión
Seguir el plan por fases (`.agents/plans/mejoras-web.md`): primero lo que
ya funciona (audio oído, multi-res), después carga progresiva, después
pipeline de datos. Sin copiar nada externo.
