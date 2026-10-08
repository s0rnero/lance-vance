---
name: 07-encargo-agente-paralelo
status: EXECUTED
type: research
domain: gameplay
owner_rules: .agents
created: 2026-09-21
---

# Encargo para el agente paralelo — plan 06 (correcciones 5ª partida)

Documento de traspaso. Lo lee el agente que puede correr en paralelo y **no**
necesita nada más: aquí están el contexto, las reglas, cómo verificar barato y
**exactamente** qué toca, por qué y con qué criterio de aceptación.

- **Coordinador (yo, "sección 1"):** datos, textos, audio, capa web, herramientas,
  etiquetas `VERSION`/`dataTag` y **el único build final**.
- **Agente paralelo (tú):** los **7 bloques pesados de C++** de mecánicas
  (R5-R11). Son los que más código, más ficheros y más riesgo tienen.
- **El jugador:** juega y verifica. **No se lanzan sondas headless** (pidió
  expresamente no montar Chrome: le saturan la CPU).

---

## 1. El proyecto en 30 segundos

Port de **GTA Vice City (reVC, rama `miami`) a WebAssembly/Emscripten** en
`/c/Users/s0rno/OneDrive/Documents/re3`. El motor (`src/**`) es el mismo código que
`reVC` en escritorio; el port añade bloques `VICEEXT_*` (paridad con el mod **Vice
Extended**). Se sirve con Vite en `localhost:2077` (proceso ya levantado) leyendo
los datos de `gta_vc_browser/streamed/` bajo demanda.

**Hallazgo que cambia el método:** `ViceEx.exe` del mod **es un fork de reVC**, no
un ASI del juego original. Sus mecánicas viven en los **mismos ficheros que los
nuestros** (`extended/src/peds/Ped.cpp`, `core/Cam.cpp`, `vehicles/Automobile.cpp`,
`weapons/Weapon.cpp`…), y su binario **filtra los nombres internos** (dummies,
acciones de control, procesos de cámara). Antes de inventar nada:

```bash
python gta_vc_browser/tools/viceex-strings.py -i "servicelight|1st person|recoil|reload"
python gta_vc_browser/tools/viceex-strings.py --src      # 101 ficheros de su árbol
```

## 2. Reglas de oro (las mismas para todos)

1. **Nada de `git add`/`commit`.** El repo vive sin commitear a propósito.
2. **No matar procesos `node`** (tumba el servidor de dev de todos).
3. **No lanzar sondas headless** (el jugador juega; la verificación es por log).
4. **Un bloque = un cambio + una traza + un criterio PASS.** Nada de "arreglar de
   paso": si aparece otra cosa, se anota y se decide.
5. **Nada de tocar ficheros de otra zona** (ver §6). Si necesitas una traza en
   `src/renderer/Hud.cpp` o `src/text/Messages.cpp` (míos), pídemela.
6. **Un solo build, al final y lo lanzo yo.** Mientras alguien esté editando, no
   se compila el paquete.
7. Cada bloque cerrado se anota **al final** de `.agents/HISTORIAL.md` y en la
   sección "Estado" de tu plan.

## 3. Cómo verificar barato (esto es lo que hace válido el trabajo)

**Compilar UN fichero sin enlazar** (~30 s, es lo que uso yo; el enlace es lo
caro, esto no):

```bash
cd /c/Users/s0rno/OneDrive/Documents/re3
export EMSDK=/c/Users/s0rno/emsdk
export PATH="$EMSDK/upstream/emscripten:$EMSDK/node/24.19.0_64bit/bin:$EMSDK/python/3.13.3_64bit:$PATH"
# la ruta del .o es build/web/src/CMakeFiles/reVC.dir/<ruta del .cpp dentro de src/>
ninja -C gta_vc_browser/build/web \
      src/CMakeFiles/reVC.dir/peds/PlayerPed.cpp.o \
      src/CMakeFiles/reVC.dir/core/Cam.cpp.o            # etc. (varios de una vez)
```

Termina en `... warnings generated.` y código 0 = **compila**. Con eso se valida
todo antes del build final.

**JavaScript:** `node --check gta_vc_browser/web/ondemand.js` (o `web/lib/index.js`).

**Leer la partida del jugador** (los ficheros se archivan por sesión):

```bash
python gta_vc_browser/tools/viceext-log-check.py gta_vc_browser/logs/odtrace-<fecha>.log
```

**Build final** (solo uno, lo lanzo yo cuando ambos terminemos):

```bash
bash -c 'export EMSDK=/c/Users/s0rno/emsdk; \
  export PATH="$EMSDK/upstream/emscripten:$EMSDK/node/24.19.0_64bit/bin:$EMSDK/python/3.13.3_64bit:$PATH"; \
  sh gta_vc_browser/build.sh'
```

Cada traza nueva debería ser **una línea por suceso**, con prefijo propio
(`SVLIGHTS`, `AIMDIR`, `RELOAD2`, `SWIM2`, `CROUCH2`, `RECOIL2`, `FP1`), para que
yo la añada al verificador y el veredicto salga automático del log.

---

## 4. Estado actual (lo que YA está hecho; no lo repitas)

| Bloque | Estado | Ficheros |
|---|---|---|
| **R1** logs por sesión con fecha | **hecho** | `gta_vc_browser/web/lib/vite.js` (archiva en `gta_vc_browser/logs/odtrace-<fecha>.log`; **requiere reiniciar Vite**) |
| **R2** congelado al cargar emisora | **hecho en JS, verificado con `node --check`** | `web/ondemand.js`: trazas `ODSTA`/`ODWRITE`, la emisora **nunca** se espera (`await` fuera), precarga de la siguiente emisora **retenida** (sin escribir MEMFS) y sólo si `__vcODBlock == 0`, `wkReadyCap` 24→40 MB. La traza gemela en C++ **ya existía**: `STREAM preload …` (`src/audio/sampman_oal.cpp:2134`, la llama `MusicManager.cpp:1143`), y en la última partida salió **0 veces** → esa sesión **no abrió ninguna emisora**, así que el tirón de 1256 ms se localizará con `ODSTA`/`ODWRITE` en la próxima |
| **R3** textos sueltos | **traza hecha, verificada compilando los dos .o** | `src/text/Messages.cpp` (`SCRTXT canal=brief/big/bigQ/brief+str`) y `src/renderer/Hud.cpp` (`SCRTXT canal=help`) con clave GXT, duración, flag y si hay misión |

Contexto de la partida que abre este plan (para que no la re-analices):
`logs/odtrace-2026-09-21_21-24-32.log`, build `ve17`, datos `ve12`, 3 min, acaba en
un tirón de **1256 ms de `wait`**. Síntomas textuales del jugador y la línea de log
que los prueba están en §5.

---

## 5. Tus 7 bloques (pesados)

Todos con el mismo formato: **evidencia (log) → causa (fichero:línea) →
implementación → PASS**. El detalle largo, con las referencias externas, está en
`06-plan-correcciones-5a-partida.md`; aquí va lo accionable.

### R5 · Primera persona (`V`) — `src/core/Camera.cpp`, `src/core/ControllerConfig.cpp`, `src/peds/PlayerPed.cpp`
- **Evidencia:** en toda la sesión `CAM1P … fp=0 … tog=0` y **ni una** línea
  `VICEEX 1p key` (se emite en `Camera.cpp:1173` al detectar la pulsación) → **la
  tecla no llega**. Y `CAM1P` sólo escribe cuando cambia `tog/mode/fp`
  (`Camera.cpp:1096`), así que hoy una pulsación perdida es **invisible**.
- **Causa candidata 1:** `ViceExtActionKeyJustDown` (`ControllerConfig.cpp:1196`)
  sólo cae a la tecla histórica `'V'` si la acción vale `rsNULL`/`0`. Con `R` pasa
  lo mismo y el log lo enseña: `VICEEXT reload key tecla=1056` (**1056 = `rsNULL`**).
- **Causa candidata 2:** la puerta
  `if(m_bLookingAtPlayer && !m_WideScreenOn && !CReplay::IsPlayingBack() && !m_bFirstPersonBeingUsed)`
  (`Camera.cpp:1183`).
- **Implementación:**
  1. Que `CAM1P` escriba **también al pulsar** (+ tecla cruda `Keys['V']` y modo).
  2. Que la lectura con respaldo caiga a `'V'` también cuando la acción valga
     `rsNULL`, `0` **o `1056`** (mismo arreglo que ya se hizo para `R`).
  3. Si la puerta bloquea, alinearla con `Cams[0].Using3rdPersonMouseCam()`.
  4. Con el modo ya encendido, tres trampas documentadas por otro modder en este
     mismo motor (blog en §B del plan 06): **el pad restringe los controles a
     "sólo zoom"** en `MODE_1STPERSON/SNIPER/ROCKETLAUNCHER/CAMERA/M16_1STPERSON`
     (`ControllerConfig.cpp:685-721`; `MODE_1STPERSON_RUNABOUT` **no** está en la
     lista, ojo), la cámara debe ir **a los ojos**
     (`m_pedIK.GetComponentPosition(HeadPos, PED_HEAD)` + avance), y **el
     movimiento hay que escribirlo en `m_vecMoveSpeed`**: `m_fMoveSpeed` sólo
     empuja hacia delante.
  5. **Paridad con el mod:** `CCam::Process_1stPerson`, `FOV_FirstPerson`,
     `HeadBob1stPerson`, y la 1ª persona como **opción de menú** (`Real 1st
     Person`), más cuatro acciones `PED_1RST_PERSON_LOOK_UP/DOWN/LEFT/RIGHT`.
- **PASS:** `VICEEX 1p key …` → `CAM1P … tog=1 mode=MODE_1STPERSON_RUNABOUT`; se
  ve el cambio, se anda de lado/atrás, se salta y se sube al coche dentro del modo.

### R6 · Agachado: cámara fija y movimiento raro — `src/peds/PlayerPed.cpp`
- **Evidencia:** `VICEEXT crouch move spd=0.5 andando=127.0 clip=238 arma=0 mio=1
  estado=1` → el bloque decide clip y velocidad (`mio=1`), estado `PED_IDLE`.
- **Causa:** `ViceExtCrouchControl` (`PlayerPed.cpp:2247`) toma `odOwnsMovement` y
  **le quita el movimiento a pie al motor**; el ped avanza distinto de cómo mira
  (el "va hacia atrás" del jugador) y la cámara, que sigue al ped, parece fija.
- **Implementación:** devolver el movimiento al motor (camino a pie normal) y
  limitarse a (a) el `CROUCH_*` como blend **parcial** y (b) el tope de velocidad;
  bajar el **objetivo de altura de cámara** mientras está agachado; traza
  `CROUCH2` con altura del ped, altura objetivo y `m_fRotationDest` vs. rumbo real.
- **PASS:** la cámara sigue, avanza hacia donde mira, se puede ir atrás y de lado.

### R7 · Nado: entra, sale solo, sin animación — `src/peds/PlayerPed.cpp`
- **Evidencia (2 s):** `swim enter … hondo=0.90` → `swim move … clip=235` →
  `swim exit agua=1` → `swim no pad=1 … control=0 estado=1`.
- **Causa 1:** "hondo" exige `nivel - z > 0,90` (`VICEEXT_SWIM_DEEP_MARGIN`) pero
  la flotación deja al ped a `nivel - 0,55` (`VICEEXT_SWIM_SURFACE_OFFSET`): al
  subir a la superficie deja de estar "hondo" → sale → se hunde → vuelve a entrar.
- **Causa 2:** `odCanSwim` falso (`IsPedInControl()` falso y estado ≠
  `PED_FALL`/`PED_JUMP`) → no nada (el "cae al vacío").
- **Implementación:** **histéresis** (entrar con 0,90; seguir nadando hasta 0,45),
  aceptar `PED_IDLE`/`FALL`/`JUMP` y el caso sin control mientras haya agua honda,
  movimiento por `m_vecMoveSpeed` (misma lección que R6) y, al salir, devolver el
  control y subir el ped al nivel de la orilla. Traza `SWIM2` con el clip aplicado
  y `swim exit motivo=…`.
- **PASS:** 20 s nadando: **un** `swim enter`, clips alternando, **ningún** `exit`
  intermedio, y sube a la orilla.

### R8 · Retroceso: mueve la mira pero **no la cámara** — `src/weapons/Weapon.cpp` + `src/core/Cam.cpp`
- **Evidencia:** 50 líneas `VICEEXT recoil kick slot=6 patada=0.030 subida=0.030
  multY=0.370` → **sí se ejecuta**; el jugador dice "la mira sí, la cámara no".
- **Causa:** `m_f3rdPersonCHairMultY` **es el vector de apuntado y la retícula**
  (`Weapon.cpp:63`, `Camera.cpp:4134`, `Hud.cpp:363`). El *pitch* de la vista es
  **`CCam::Alpha`**, y el ratón lo **recalcula entero cada frame**
  (`Cam.cpp:1397-1419`) con tope en `Cam.cpp:1423-1424` → **un pico aislado sobre
  `Alpha` se pierde al frame siguiente**.
- **Implementación:** desplazamiento **persistente** (`s_odRecoilAlpha` +
  aplicado) que se suma a `Alpha` **después** del ratón y **antes** del clamp,
  decae con `VICEEXT_RECOIL_RETURN * dt`, patada por arma **visible** (≈0,35° rifle,
  0,8° escopeta) y `CamShakeNoPos` proporcional (hoy `0.004 + 0.2*kick` ≈ 0,01 =
  invisible). Se **mantiene** la parte de `multY` (mueve la bala y ya funciona).
  Sólo a pie y sólo con `VICEEXT_RECOIL`. Traza `RECOIL2` con `alpha=`/`grados=`.
- **PASS:** `Alpha` sube y vuelve al valor previo; en pantalla la mira sube y la
  vista acompaña; la mira sigue moviéndose como ahora.

### R9 · El arma apunta a un costado (AUG, escopeta nueva) — medir antes de tocar
- **Descartado ya:** `.dff` del mod byte a byte, clips `STEYR_*` existentes, grupos
  declarados (`AnimManager.cpp:1049-1051`), `fire offset` normal, mira correcta
  (`SIGHT arma=54 mira=5`).
- **Implementación (en este orden):**
  1. Traza `AIMDIR` (1/s apuntando): arma, **desviación en grados** entre la
     dirección de apuntado de la cámara y el rumbo del ped/torso, grupo y clip en
     uso, y **peso de los blends parciales** (agachado/nado) — un blend que no
     vuelve a 0 corrompe la pose.
  2. Si pasa con **todas** las armas al caminar → es `VICEEXT_AIM_WALK`
     (`PlayerPed.cpp:1633+`): fijar rumbo a la cámara
     (`m_fRotationCur = m_fRotationDest = Front.Heading()` + `SetHeading` +
     `GetMatrix().UpdateRW()`, patrón del blog).
  3. Si pasa **sólo** con las nuevas → comparar el `AnimAssocDesc` del grupo nuevo
     contra los fotogramas reales de los clips del mod (yo te leo el `.ifp`) y,
     mientras, probar el grupo estándar equivalente (`steyr`→`rifle`,
     `Shotgun2`→`shotgun`) para aislar.
- **PASS:** `AIMDIR` con desviación < 5° en todas las armas.

### R10 · `R` no recarga — `src/peds/PlayerPed.cpp`, `src/core/ControllerConfig.*`
- **Evidencia:** `VICEEXT reload key tecla=1056 arma=54 estado=0 clip=28/30
  total=441` → `VICEEXT reload start clip=28 total=441 type=54 ms=1000`. **Nunca**
  hay línea de recarga terminada (esa traza no existe). Negativas correctas:
  `motivo=sin-municion`, `motivo=cargador-lleno`.
- **Dato:** el `weapon.dat` del mod pone `RELOAD` (columna `a`, 4.º dígito) en sus
  armas → el motor **debe** cerrar la recarga al terminar la animación.
- **Implementación:** traza de cierre `VICEEXT reload done|fail` en el punto donde
  el motor da la recarga por terminada **y** en el aborto; comprobar que el bloque
  no corta la animación ni deja `m_nTimer` a medias; y **crear la acción de control
  real `PED_RELOAD`** (como hace el mod) con tecla por defecto `R`, dejando el
  respaldo actual como red de seguridad.
- **PASS:** `reload done` con cargador lleno y `total` reducido, dos veces seguidas.

### R11 · Luces de servicio del coche de policía — `src/vehicles/Automobile.cpp`
- **Evidencia:** el jugador ve los destellos de color pero **no la barra** como en
  la captura del mod.
- **Causa:** las coronas de la sirena están **clavadas por índice de modelo y
  posición fija** (`Automobile.cpp:2166-2255`: `MI_POLICE` en ±0,7/−0,4/1,0, corona
  0,4 y alcance 50, `TYPE_STAR`) y **no leen los dummies** del modelo — y el
  `police.dff` del mod **sí los trae**.
- **Implementación:**
  1. Al cargar el clump, buscar los dummies **`servicelights`**, `servicelights_0`,
     `_1`, `_2`, `_3` y **`servicelightson`** (nombres exactos, sacados del binario
     del mod) y guardar sus matrices en el vehículo (una vez, no por frame).
  2. En el bloque de sirena, usar **esas posiciones** (rojo/azul interpolando entre
     ellas) y caer a las fijas actuales si el modelo no las trae.
  3. Ajustar tamaño/alcance de la corona para que la barra se vea (0,4/50 es
     pequeño) y traza `SVLIGHTS model=… dummies=N pos=…`.
  4. Extra: el mod reproduce `DMAudio.Service` para el audio de servicio.
- **PASS:** `SVLIGHTS dummies=4` en el coche del mod y captura con la barra
  encendida donde está la barra (no en el techo).

---

## 6. Frontera de ficheros (para no pisarnos)

| Zona | Dueño | Ficheros |
|---|---|---|
| Datos, web, herramientas, etiquetas | **Buffy** | `gta_vc_browser/**` (incluido `web/ondemand.js` y `web/lib/*`), `.agents/**` |
| Textos/HUD (trazas) | **Buffy** | `src/text/Messages.cpp`, `src/renderer/Hud.cpp`, `src/text/Text.cpp` |
| Config de bloques | **Buffy** | `src/core/config.h` (los `#define VICEEXT_*`: **pedir**, no añadir) |
| Compartido (sólo al final de tu bloque) | ambos | `src/core/Pad.cpp`, `.agents/HISTORIAL.md` |
| **Mecánicas (tuyo)** | **agente paralelo** | `src/peds/PlayerPed.*`, `src/core/Camera.cpp`, `src/core/Cam.cpp`, `src/core/ControllerConfig.*`, `src/weapons/Weapon.cpp`, `src/weapons/WeaponInfo.cpp`, `src/vehicles/Automobile.cpp`, `src/animation/AnimManager.*` |
| Audio | **Buffy** | `src/audio/**` |

Si tu bloque necesita una traza en `Hud.cpp`/`Messages.cpp` o un `#define` nuevo,
**pídemelo** ("necesito `#define VICEEXT_X` en config.h y una traza `FOO` en Hud")
y lo pongo yo: así no hay dos versiones del mismo fichero.

## 7. Cierre y build único

1. Termina tus bloques con su `.o` compilado (ninja por fichero) y su traza.
2. Avisa por el canal del jugador: "R5-R11 listos, sin compilar el paquete".
3. Yo añado las trazas nuevas al verificador, subo `VERSION`/`dataTag` si toca
   datos y **lanzo el único build**.
4. El jugador juega; yo leo `logs/odtrace-<fecha>.log` y dicto PASS/FAIL por
   bloque; lo que falle se corrige en una segunda ronda (y entonces sí, un segundo
   build).

## 8. Orden recomendado (por relación efecto/riesgo)

1. **R8** (retroceso de cámara) — es lo que el jugador acaba de pedir y es
   contenido (`Weapon.cpp` + `Cam.cpp`).
2. **R7** (nado) — hoy se nota como "no existe"; el arreglo es de umbrales.
3. **R6** (agachado) — quitar el movimiento al motor.
4. **R5** (1ª persona) — primero la tecla (medir), luego cámara/movimiento.
5. **R10** (recarga) — traza de cierre + acción `PED_RELOAD`.
6. **R9** (apuntado) — **medir** con `AIMDIR` y decidir con dato.
7. **R11** (luces) — leer dummies; es el más independiente (queda para el final).
