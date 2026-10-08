---
name: 00-INDICE
status: EXECUTED
type: research
domain: process
owner_rules: .agents
created: 2026-09-19
---

# Plan de mecánicas — reparto en 3 secciones (19/09/2026)

Objetivo: terminar lo que queda del plan de Vice Extended. Los bloques "caros"
(que son **código sin fuente**, no dato) se parten en **1 bloque = 1 mecánica**,
y el trabajo se reparte en **3 secciones** para que se pueda avanzar en paralelo
sin pisarse.

| Sección | Plan detallado | Quién | Tema | Bloques |
|---|---|---|---|---|
| **1 (la más liviana)** | `01-seccion-DATOS-pulido.md` | Buffy (yo) | Cierre de datos y pulido visible | D1…D8 |
| **2** | `02-seccion-POLICIA-conduccion.md` | aux 1 | Policía y conducción | P1, P2, P3 |
| **3** | `03-seccion-CAMARA-guardado.md` | aux 2 | Cámara, guardado y movimiento | C1, C2, C3 |
| opcional | `04-opcional-escalar.md` | libre | Escalar (su `features.ini` lo trae **a 0**) | E1 |
| **reparto 20/09** | `05-hallazgos-4a-partida.md` | **los 3** | Reparto de la 4ª partida: qué pide el jugador, la evidencia en la traza y **qué sección hace cada cosa** | J0–J5 |
| **correcciones 21/09** | `06-plan-correcciones-5a-partida.md` | **los 3** | 5ª partida: los fallos reportados uno por uno (con la línea de log que los prueba y la **implementación concreta**), referencias externas verificadas (§B) y el **backlog del mod Extended** que queda vivo | R1–R12, X1–X26 |

**Encargos listos para pegar** (plan 06, §F): sección 3 → R5-R10 (empezar por R8,
que es el que el jugador acaba de pedir: el retroceso mueve la mira pero no la
cámara); sección 2 → R11 (luces de servicio del coche de policía).

**Reparto en 2 para ir en paralelo: `07-encargo-agente-paralelo.md`** (21/09).

**Reparto tras la 9ª partida: `08-encargo-subagente-2.md`** (21/09, build `ve19`).
Ojo con lo que cuenta: las "varias partidas sin ninguna mejora" fueron **el motor
viejo** (el paquete no se había enlazado). Trae la regla nueva —
`bash tools/check-served-build.sh` **antes** de pedir cualquier partida — y el
reparto siguiente: subagente = X8/X9/X4/X10/X11/X26 (IA, vehículos y mundo),
sección 1 = verificación desde log, audio, datos, HUD, menú de 1ª persona.
Contexto + reglas + **cómo compilar un solo fichero sin enlazar** (`ninja` por
objetivo) + las **7 tareas pesadas** (R5-R11, con ficheros, causa y PASS) para el
agente paralelo, y las livianas para la sección 1 (datos, textos, audio, web,
herramientas y **el único build final**). Incluye la frontera de ficheros para que
no haya dos versiones de un mismo `.cpp`.

**Reparto tras la 7ª partida: `09-encargo-subagente-3.md`** (21/09, build `ve19`,
log `logs/odtrace-2026-09-22_01-23-46.log` + **vídeo cruzado con el log**). Trae
por primera vez la **prueba visual**: `tools/frames-at.sh` saca fotogramas en las
marcas UTC del log y se ven como imagen. Con eso quedó repartido así — parte **B**
(subagente): H1 cámara del agachado (`CROUCH2` peso 1,0 y el clip correcto, pero
el vídeo `t=12` enseña la cámara **dentro de la cabeza**), H2 cámara/estado del
nado (10 `exit motivo=poco-hondo` y la cámara clavada en la superficie), H3
apuntado de las nuevas (`arma=54` Steyr `desv` 73,6°; `arma=55` lanzagranadas
`peso=0.00`), §6 sirenas por dummies (`SVLIGHTS dummies=0` con `police` = 156, y el
buscador solo mira hijos directos). Parte **A** (sección 1): verificador ampliado,
icono del lanzagranadas y retícula (`Hud.cpp`), duración de los textos, el enlace
único y la auditoría de la partida. **Cero ficheros compartidos** (§2).

Yo (sección 1) hago de **coordinación general**: mantengo el índice, la etiqueta
`VERSION` / `dataTag`, el árbol de datos y el arnés de sondas, y al final
integramos y validamos con el jugador. Los planes de sección no se editan entre
secciones: cada agente añade una sección "Estado" al final de su propio fichero.

## Cómo arrancar (sección 2 = aux 1, sección 3 = aux 2)

1. Leer **este índice** y vuestro fichero de sección
   (`02-seccion-POLICIA-conduccion.md` / `03-seccion-CAMARA-guardado.md`).
2. **Primer paso de cada bloque: medir la línea base y escribirla en vuestra
   sección "Estado"** antes de tocar código (el plan dice qué medir).
3. Un bloque = un build + una sonda + una captura. Avisad en `.agents/HISTORIAL.md`
   (añadiendo al final) de cada bloque cerrado, con la evidencia.
4. Si necesitáis algo de datos, cheats o etiquetas (`VERSION`/`dataTag`),
   pedídselo a la sección 1 en vez de tocar `gta_vc_browser/`.

## Reglas de convivencia (mismo checkout, 3 agentes)

1. **Nada de commits ni de `git add`.** Todo el port vive sin commitear; hay
   ~3.000 ficheros *staged* de `streamed/` que no son nuestros (no tocar).
2. **Un bloque = un build + una sonda + una captura.** Compilar es ~4-5 min;
   hacerlo dos veces porque se mezclaron dos mecánicas es tirar el trabajo.
3. **No tocar ficheros de otra sección.** Tabla de propiedad:

   | Ruta | Dueño |
   |---|---|
   | `gta_vc_browser/**` (datos, `tools/`, `web/lib`, `web/ondemand.js`) | sección 1 |
   | `src/modelinfo/**`, `src/core/config.h` (bloque "Vice Extended") | sección 1 |
   | `src/core/Wanted.*`, `src/peds/CopPed.*`, `src/vehicles/{Automobile,Bike,Boat}.cpp`, `src/peds/PedAI.cpp` | sección 2 |
   | `src/core/Camera.cpp`, `src/core/Cam.cpp`, `src/peds/PlayerPed.*` | sección 3 |
   | `src/control/Script.cpp`, `src/save/**`, `src/core/Frontend*.cpp` (guardado) | sección 3 |
   | `src/core/Pad.cpp` | compartido: **añadir solo al final** de su bloque (cheats o bindings nuevos), con comentario `// <sección N>: ...` |
   | `.agents/HISTORIAL.md` | compartido: **añadir al final**, un apartado por bloque |

4. **Si un cambio necesita un `#define`**, se añade al bloque
   `// Vice Extended: paridad con su features.ini` de `src/core/config.h`
   **al final, sin reordenar** (yo mantengo ese fichero; avisad en el historial).
5. **Cada mecánica va detrás de su define** (`VICEEXT_*`, como los toggles ya
   portados) para poder apagarla sin recompilar lógica.
6. **Prohibido "arreglar" de paso** cosas fuera del bloque: si aparece algo,
   se anota en la sección "Estado" y lo decide la coordinación.

## Arnés común (esto ya funciona; no hace falta reinventarlo)

```bash
# 1. Compilar (emsdk ya instalado en el equipo)
cd /c/Users/s0rno/OneDrive/Documents/re3
bash -c 'export EMSDK=/c/Users/s0rno/emsdk; \
  export PATH="$EMSDK/upstream/emscripten:$EMSDK/node/24.19.0_64bit/bin:$EMSDK/python/3.13.3_64bit:$PATH"; \
  sh gta_vc_browser/build.sh'          # salida: gta_vc_browser/web/public/build/

# 2. Servidor (Vite, puerto 2077) — normalmente YA está levantado
cd gta_vc_browser/web && npm run dev

# 3. Sondas headless (secuenciales: usan CPU de swiftshader, 4-8 min cada una)
node gta_vc_browser/tools/slot0-load-test.mjs      # base: menú -> slot 0 -> partida
node gta_vc_browser/tools/weapons-smoke-test.mjs   # cheat CRAZYTOOLS + disparos
node gta_vc_browser/tools/vehicles-smoke-test.mjs  # cheat CRAZYRIDES + asentado
```

- **Nunca matar procesos `node`** (`taskkill //F //IM node.exe` tumba el
  servidor de desarrollo). Para matar un navegador colgado:
  `taskkill //F //IM chrome.exe`.
- Para una mecánica nueva, **copiar** `weapons-smoke-test.mjs` a
  `tools/<mecanica>-smoke-test.mjs` y cambiar la fase de juego (teclas, tiempos,
  comprobaciones). El patrón: teclear un cheat / pulsar teclas, capturas PNG en
  `%TEMP%\vc-<mecanica>`, y veredicto `PASS/FAIL` con `process.exit(1)` si falla.
- **Ojo con los asserts**: se ven en la **consola de la página**
  (`REVC ASSERT FAILED ... Expression: X`), no en `odtrace.log`, y pueden
  **parecer un cuelgue** (el log se para de golpe). Las sondas imprimen el final
  de la consola justo después de la acción (ver el "diagnóstico temprano" de
  `vehicles-smoke-test.mjs`). Ejemplos reales ya cazados así:
  `CalculateTrianglePlanes: model` (faltaba un `.col`), `invalid bike model ID`.
- `odtrace.log` rota por sesión. Las trazas útiles: `TXDIN txd=...` (TXD
  cargado), `CHINIT` (arranque de canal de audio), `FPHASE` (fase de frame),
  `JS OD*` (capa on-demand), `ODSHORT … open-fail` (**cupo de 60 líneas**, es
  normal verlo lleno: es el camino del aplazamiento, no 60 fallos).
- **Datos nuevos on-demand** (cualquier fichero añadido bajo `streamed/`):
  `python tools/gen_manifest.py`. **Cambios en `data/`, `TEXT/`, `txd/`, `neo/`,
  `skins/`** (son `FULL_DIRS` del `bootseed`): `python tools/stage_bootseed.py`.
  Ambas cosas se materializan en el `.data` **solo al recompilar**.
- **Etiquetas**: `VERSION` en `gta_vc_browser/web/lib/index.js` y `dataTag` en
  `gta_vc_browser/web/ondemand.js`. **La sube la sección 1** al integrar cada
  bloque (el `dataTag` purga la caché IndexedDB; si no se sube y un bloque
  sustituye ficheros ya cacheados, se juega con bytes viejos).

## Estado de partida (medido, 19/09, actualizado 20/09 · sección 1)

- **Aviso de método (20/09):** el jugador ha pedido **parar de lanzar sondas de
  Chrome** (le saturan la CPU, y con otro Chrome abierto jugando las medidas se
  contaminan: se ha medido una sesión al 100% de CPU con 13 FPS). A partir de
  ahora: **código + logs**, y la verificación se hace leyendo la traza de una
  partida real con `python gta_vc_browser/tools/viceext-log-check.py
  --desde-marca` (dicta D2/D4/D5/D6/D7 sin navegador). Cada bloque de la
  sección 1 deja sus trazas (`CARPED`/`CARPOOL`/`CARBLOCK`/`CARSPAWN`,
  `IFPFILE`, `SIGHTS`/`SIGHT`, `KEYICONS`/`HINTKEY`, `TXTMISS`).
- Build y datos **`2026-09-20-ve9`** (`VERSION` en `web/lib/index.js`,
  `dataTag` en `web/ondemand.js`): bloque D1 cerrado y además dentro D5
  (`anim/ped.ifp` del mod, 272 clips), D6 (`models/weaponsights.txd`) y D7
  (`models/pcbtns.txd`). **Las secciones 2 y 3 pueden medir desde aquí.** Ojo:
  si tocáis datos, la etiqueta la sube la sección 1 (avisad en el historial).
- Bloques D4 (tráfico), D5 (`ped.ifp`), D6 (mira por arma) y D7 (iconos de tecla
  en los avisos) **implementados y compilados**; pendiente sólo el veredicto de
  una partida real (ver arriba cómo). Detalle en `01-seccion-DATOS-pulido.md` y
  en el HISTORIAL.

**Actualización 20/09 13:20 (sección 1).** La 2ª partida del jugador (build
`ve10`) dio **D2/D4/D5/D6/D7 = OK** leída sólo del log. De esa partida salieron
dos arreglos cuyo código está en el **wasm servido de 13:20** (mi enlace fue el
de 13:17 con etiqueta `ve11`; la sección 3 relinkeó después y la subió a
`ve12`, así que la etiqueta actual es esa): (1) el cargador de tráfico
**sortea por frecuencia** al pedir modelos (`ChooseCarModelToLoad`), porque el
refuerzo de peso del mod sólo pesaba entre los ya cargados — de los 7 vehículos
nuevos sólo 4 llegaron a pedirse; (2) el lector suelto del `.img` ahora
**demuestra** que un aplazamiento de la capa on-demand se recupera
(`ODSRECOVER`). Trazas nuevas para todos: `CARLOAD`, `CARFAIL`, `CARZONE`,
`CARRATE` y `freq=` en `CARPED`. **`dataTag` sigue `ve10`** (los datos no
cambian), así que sólo hay que recargar con **Ctrl+Shift+R**, no re-stagear.
Aviso para las otras secciones: `VERSION` es mía, no la subáis sin decirlo.
- El bug "streetfighter missing" **no** era sólo el GXT: la clave corta estaba
  escrita en el campo equivocado del IDE (campo 7, `anims`, en vez del 6,
  `gameName`) y eso además rompía el grupo de animación de las motos nuevas.
  Arreglado y verificado (`tools/check_ide_gxt.py`, `tools/gxt_inspect.py`
  autocomprobado). Si veis nombres raros en el HUD de un vehículo, es esta
  zona: avisad antes de tocar `streamed/` (es de la sección 1).
- Regresión de la sección 1 pendiente de confirmar en el momento de escribir
  esto: sonda base `slot0-load-test.mjs` sobre `ve8`.
- Packs de Vice Extended **1, 1b, 2, 3 (armas), 4 (vehículos)** dentro y con
  sondas en PASS. Toggles de su `features.ini` **encendidos** ya portados:
  `VICEEXT_SWIMMING`, `VICEEXT_RECOIL`, `VICEEXT_NO_CAR_BOUNCE`,
  `VICEEXT_SPRINT_HEAVY`.
- Descartado con medición: su `main.scm` (opcodes propios sin implementar) y su
  banco `ViceEx.{SDT,RAW}` (13 muestras sin nombres).
- Bug abierto reportado por el jugador: el HUD dice
  **"streetfighter missing"** al entrar en vehículos nuevos (clave GXT con
  formato incorrecto) → bloque D2, en curso.

**Actualización 20/09 (tarde) — leer `05-hallazgos-4a-partida.md`.** Tras la 4ª
partida el jugador dio una lista larga (cinemáticas que crashean, LOD pegado,
1ª persona, agachado, correr armado, nadar, escopeta, depósito, ranuras de
guardado, autocentrado, balas de pistola en drive-by, `R` que no recarga,
sirena del coche de policía). En ese fichero está el **reparto por sección con
la evidencia** de la traza. Dos datos que cambian el trabajo de otros:
1. El `anim/ped.ifp` que **ya se sirve** trae los clips de **nado, agachado,
   sprint armado y escalada** (`Swim_*`, `Crouch_*`/`DUCK_*`,
   `sprint_armed|rocket|csaw`, `CLIMB_*`) → no hay que traer arte, hay que
   asociar los grupos en `src/animation/AnimManager.cpp`.
2. Sólo **25 de ~100** `.dff` de vehículos traen el dummy `petrolcap` → el
   depósito necesita posición de reserva.
Sección 1 en este bloque: **D6b** (mira sólo al apuntar), **J1** (el motivo de un
cuelgue queda en `odtrace.log`: `JSERR`/`JSERR_REJ`/`ENGERR`), **D10**
(`LODLEFT`, diagnóstico del LOD pegado) y **D4c** (throttle de `CARRATE`). El
`.o` de los tres ficheros compila; el **enlace está bloqueado** por el error en
vuelo de la sección 3 (`Cam.cpp:5209 GetLookLeftRight`). La etiqueta
`VERSION` se sube a `ve13` al enlazar; `dataTag` sigue `ve10` (no cambian datos).

## 21/09/2026 (tarde) · 9ª partida · CAUSA RAÍZ + build ve23

`10-seccion-1-9a-partida.md`: `bFreeCam` cortaba `CanStrafeOrMouseControl()`, y
eso tumbaba de golpe el apuntado (desv hasta 86°), el pase a pie con ratón
(agachado/nado) y la 1ª persona. Con eso arreglado van también: recarga a mano con
animación, conmutador de agachado sin doble pulsación, autocentrado de coche suave
(con empujón al soltar la mirada) y apuntado del lanzagranadas. Enlazado `ve23`;
bloque **R12** nuevo en `tools/viceext-log-check.py` para dictaminarlo desde el log.
