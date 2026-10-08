---
name: 06-plan-correcciones-5a-partida
status: EXECUTED
type: bugfix
domain: gameplay
owner_rules: .agents
created: 2026-09-21
---

# Plan de correcciones — 5ª partida (21/09/2026) · **v3 (enriquecido + mod minado)**

Plan **uno por uno** de los fallos que el jugador reportó tras jugar, más el
**backlog del mod Extended** que queda vivo. Cada bloque trae: **síntoma →
evidencia en la traza → causa localizada en código (fichero:línea) →
implementación concreta → referencia externa → criterio PASS → quién**.

> **v2:** se añade la investigación externa (§B) y, con ella, la **implementación
> concreta de cada arreglo** (nada de "investigar y ya"). Lo que se buscó fuera
> no es para copiar: es para **no repetir los errores que otros ya documentaron**
> (el blog de 1ª persona sobre re3 ahorra tres trampas reales).
>
> **v3 (21/09, tarde):** el hallazgo que cambia el método está en **§B2**: el mod
> `ViceEx.exe` **es un fork de reVC** (no un ASI del original) y su binario filtra
> sus nombres internos: los **dummies exactos** de las luces de servicio
> (`servicelights*`, `servicelightson`), su **proceso de cámara**
> (`CCam::Process_1stPerson`) con `FOV_FirstPerson`/`HeadBob1stPerson`, sus
> **acciones de control nuevas** (`PED_RELOAD`, `PED_1RST_PERSON_LOOK_*`,
> `PED_WALK`, `PED_CENTER_CAMERA_BEHIND_PLAYER`) y su árbol de fuentes completo.
> Para volver a minarlo: `tools/viceex-strings.py` (**§B3**).

---

# A. Partida analizada

| Dato | Valor |
|---|---|
| Fichero | `logs/odtrace-2026-09-21_21-24-32.log` (antes `web/odtrace.prev.log`) |
| Sesión | `21:24:32Z → 21:27:30Z` (≈3 min, 1.984 líneas) |
| Build / datos | `2026-09-21-ve17` / `2026-09-21-ve12` |
| Final | **tirón de 1.256 ms** (`FPSLOG avg=3 maxdelta=1257ms hitch=1`, `FPHASE gap=1256.8 wait=1256.5`) → sólo queda el latido de la pestaña |
| Segundo fichero | `logs/odtrace-2026-09-21_23-10-14.log`: 2 s de arranque y **deja de escribir** (misma clase de bloqueo, no hay `ENGERR`) |
| Veredicto de la utilidad | D2 OK · D4 OK (7/7 alcanzables, sólo Streetfighter llegó a la calle; moto policial 0) · D5 OK (272 clips) · D6 PARCIAL (se apuntó, la utilidad no contabiliza) · D7 PARCIAL (no hubo aviso con tecla) · J1 sin datos |
| Sin ejercitar | cinemáticas (`cut=0`), estrellas (`0` líneas `WANTED`), sirena de coche de policía |

**Lo primero: los logs ya no se pierden** (`R1`, hecho): cada sesión queda en
`gta_vc_browser/logs/odtrace-<fecha>_<hora-inicio>.log`.

---

# B. Referencias externas verificadas (y qué trae cada una)

Todas leídas, no supuestas:

| Fuente | URL | Qué enseña de verdad |
|---|---|---|
| **"Adding a first-person mode to GTA III"** (Juraj Borza, 11/03/2024) | `https://jborza.com/post/2024-11-03-first-person-gta-iii/` | **La mina para R5/R6/R7/R9.** Escrito sobre el mismo motor (re3/reVC): qué modos de cámara sirven y cuáles no, la trampa del pad que restringe controles en 1ª persona, que la puerta de la 1ª persona se cierra con `!IsPedInControl() \|\| m_fMoveSpeed > 0`, cómo fijar el rumbo del ped a la cámara, cómo poner la cámara en la cabeza (`m_pedIK.GetComponentPosition(HeadPos, PED_HEAD)` + 0,19 m al frente) y **la clave de R6/R7: en 1ª persona `m_fMoveSpeed` sólo empuja "hacia delante"; el movimiento real hay que escribirlo en `m_vecMoveSpeed` a partir del ángulo de cámara + stick** |
| **gta-reversed/gta-reversed** (GTA:SA 1.0 US reimplementado, público) | `https://github.com/gta-reversed/gta-reversed` | Fuente canónica de SA para portar mecánicas cuyo **arte ya está servido** en nuestro `ped.ifp` del mod (nado, agachado, sprint armado, escalada). Su árbol (`source/game_sa/…`, accesible por API) es la referencia para R6/R7 y para el retroceso estilo SA |
| **CLEO `WeaponRecoilAuto`** (gtaforums 953286) | citado ya en `src/weapons/Weapon.cpp:156` | El enfoque de la comunidad para retroceso en VC/SA: **empujar la mira**, no la cámara. Confirma que lo nuestro es "lo estándar" y que el kick de cámara que pide el jugador es un **extra** |
| **"GTA Vice City Custom Police Lights"** (mod de luces por **dummies** de vehículo) | `https://www.youtube.com/watch?v=1KwTe00JIRk` | El método de la comunidad para las luces de servicio: colocarlas **leyendo los dummies del clump** ("it works with vehicle dummies… add as many police lights as possible") — justo los `servicelights*` que trae el `police.dff` del mod |
| **Ficha del mod** (libertycity, October 2025 Update) | `https://libertycity.net/files/gta-vice-city/167639-vice-extended-october-2025-update.html` | Lista de cambios oficial (la misma de `ChangesEN.txt`) |
| **Hezkore/hez-gta-re3** (fork de re3/reVC con mejoras y nightlies) | `https://github.com/Hezkore/hez-gta-re3` | Contexto de paridad: mantiene el mismo motor con mejoras; documenta que los mods `.asi/.dll` **no** funcionan en re3/reVC y que CLEO Redux sí. Útil para saber qué esperar de "portar el mod" |
| **Documentación dentro del propio paquete del mod** | `Help.txt`, `features.ini`, `limits.ini`, `ChangesEN.txt` | **La especificación de comportamiento**: `RecoilWhenFiring=1`, `EnableSwimming=1`, `EnableClimbing=0`, `RocketLauncherThirdPersonAiming=1`, `EnableDistantLights=1`, `RandomVehicleModsInTraffic=1`; trucos; y los tamaños de pool (`NUMPEDS=140`, `NUMVEHICLES=130`, `NUMBUILDINGS=7000`, `NUMDUMMIES=2340`) para R22 |

---

### B2 · El mod **es un fork de reVC** (leído de su propio binario) — 21/09

`GameFiles/ViceEx.exe` (3,8 MB) **no** es un ASI del juego original: es un build de
**reVC** (lo delata su `libmpg123-0.dll` + `OpenAL32.dll` + `modloader.asi`, y lo
confirma el binario: cadenas `reVC`, `librw` ×16, `CViceEx.ini`,
`ViceExtended/features.ini` y **101 rutas de su árbol de fuentes**
`extended/src/...`). Y como es reVC, sus mecánicas nuevas viven **en los mismos
ficheros que las nuestras**: no añade ni un fichero de mecánicas (sólo `extras/`
para postfx: `custompipes.cpp`, `postfx.cpp`, `screendroplets.cpp`,
`debugmenu.cpp`).

**Consecuencia de método:** la paridad con el mod es un problema de ***diff***
sobre el mismo código, no de reinvención. Cada bloque nuestro (R5-R11) tiene su
fichero gemelo en el árbol del mod y las cadenas del binario dicen **qué hizo**
allí. Extraído hoy del `ViceEx.exe`:

| Cadena del binario | Qué significa para nosotros |
|---|---|
| `CCam::Process_1stPerson` | el mod **añade su propio proceso de cámara** de 1ª persona (no usa el *peek* de serie) → **R5** |
| `1st Person`, `1st Person run about`, `Real 1st Person`, `FOV_FirstPerson`, `HeadBob1stPerson`, `DoomMode_FirstPerson`, `RelativeCamInVeh_DriveBy_FirstPerson`, `AutocenterCamInVeh_FirstPerson` | su 1ª persona es **una opción del menú** (con FOV y *head bob*) y tiene variantes para el coche. Nuestro port la puso en una tecla suelta → **causa probable de R5** (si en el mod se enciende por menú, el jugador no puede esperar que `V` haga nada hasta que la tecla llegue de verdad) |
| `PED_RELOAD` | **acción de control propia** para recargar (nuestro `R` sólo funciona por el respaldo `rsNULL`) → **R10** |
| `PED_1RST_PERSON_LOOK_UP/DOWN/LEFT/RIGHT` | el mod permite **mirar con teclas** en 1ª persona |
| `PED_CENTER_CAMERA_BEHIND_PLAYER` | autocentrado (bloque ya pedido, sección 3) |
| `PED_WALK` | caminar con `ALT` (**X14**, más fácil de lo previsto: la acción ya tiene nombre) |
| `servicelights`, `servicelights_0`, `servicelights_1`, `servicelights_2`, `servicelights_3`, **`servicelightson`** | los **dummies exactos** que busca en el modelo para las luces de servicio → **R11 con nombre y apellidos** |
| `Swim_Breast`, `Swim_Crawl`, `Swim_Tread`, `Swim_jumpout`, `Crouch_Idle/Forward/Backward/Roll_L/Roll_R`, `WEAPON_crouch`, `CLIMB_*` | el arte que usa, ya servido por su `ped.ifp` (*sin* cambios de datos) |
| `EnableSwimming`, `EnableClimbing`, `RecoilWhenFiring`, `RocketLauncherThirdPersonAiming`, `RandomVehicleModsInTraffic`, `EnableDistantLights`, `MirrorModeByDefault`, `DisableBulletTraces`, `CameraShakeInVehicleAtHighSpeed`, `RemoveMoneyZerosInTheHud`, `WaypointColorRGB`, `GameSaveOnStartup`, `NeoVehicleShininess`… | su config (checklist de **X12** y de los ajustes de guardado) |
| `MouseAimSensX/Y`, `PadAimSensX/Y`, `HorizantalMouseSens`, `LeftStickDeadzone`, `InvertVertically`, `DisableMouseSteering`, `FrontendOptions` | ajustes de apuntado/ratón → **X16** |
| `DMAudio.Service` | audio propio de las luces de servicio (extra de R11) |
| `PED_RELOAD`, `PED_DUCK`, `TOGGLE_DPAD`… (lista de acciones) | su tabla de controles; nombre de las acciones nuevas que nosotros deberíamos crear igual |

### B3 · Herramienta para volver a minar el binario del mod

Nuevo `gta_vc_browser/tools/viceex-strings.py` (sin dependencias, no necesita build):

```bash
python gta_vc_browser/tools/viceex-strings.py              # volcado a tmp/viceex-strings.txt
python gta_vc_browser/tools/viceex-strings.py -i "1st person|recoil|servo"
python gta_vc_browser/tools/viceex-strings.py --src        # 101 ficheros de su árbol
python gta_vc_browser/tools/viceex-strings.py --ini        # candidatas a claves de config
```

Cuando un bloque dude "¿esto cómo lo hizo el mod?", **ante la duda se mira aquí
antes que en internet**: sus nombres salen tal cual (dummies, acciones, procesos
de cámara, claves de config).

---

# C. Correcciones de esta partida (R)

## R1 · Logs de cada partida con fecha — **HECHO**
- **Síntoma:** "cada partida debe dejar un log con fecha del día, no eliminarse ni sobreescribirse".
- **Por qué pasaba:** el servidor rotaba `odtrace.log` → `odtrace.prev.log`: sólo dos sesiones vivas.
- **Hecho:** `web/lib/vite.js` archiva la sesión que se cierra en
  `gta_vc_browser/logs/odtrace-<AAA-MM-DD>_<HH-MM-SS>.log` (hora de **inicio** de
  la partida; sufijos `-b`, `-c`… si coincidieran) y **nunca borra**; se conserva
  `*.prev.log` para las utilidades que ya lo leían. Las dos sesiones de hoy ya
  están archivadas.
- **Ojo:** reiniciar el servidor de Vite (el plugin se carga al arrancar) — **sin
  matar `node` de otros agentes**.
- **Prueba:** jugar dos veces y ver dos ficheros nuevos en `logs/`.

## R2 · Cargar una emisora congela el juego — **sección 1**
- **Síntoma:** "cargar una emisora freezea el juego de nuevo; estaba corregido".
- **Evidencia:** el tirón final es **espera**, no lógica
  (`FPHASE gap=1256.8 prev(L=0.1 … sum=0.3) wait=1256.5`). En esa sesión **no hay
  ni una petición de emisora** en `streamed-access.log` (sólo `models/gta3.img`,
  `Audio/sfx/*`, `weaponsights.txd`) → la que sonaba ya estaba en MEMFS.
- **Causa (fichero:línea):** `web/ondemand.js`
  - `ensure()` (~1015): el *fast-fail* de `.adf` sólo actúa si `wkFastFail()` dice
    sí, y `wkFastFail()` (~657) **devuelve `false` cuando `__vcODBlock > 0`** y
    **cuando el worker ya acumuló 3 aplazamientos** del mismo fichero → se cae a
    `await OD.wkWait(canon)`: **el frame se suspende** mientras llegan ~9-30 MB y
    se copian a MEMFS en el hilo principal.
  - `noteStation()` (~306): a los 3 s de notar la emisora actual precarga **la
    siguiente del dial** (hasta 30 MB) sin mirar si hay carga bloqueante en curso.
  - `PreloadStreamedFile()` (`src/audio/sampman_oal.cpp:2140`): la espera
    bloqueante sólo se aplica a `nStream != 0` (correcto para radio) pero **no
    deja traza**, así que "no sonó" y "se quedó esperando" son indistinguibles.
- **Implementación concreta:**
  1. **Traza `ODSTA`**: por petición de `.adf` → fichero, `memfs=0/1`, `ms` de
     `ensure`, `block=0/1`, `defer=`, `esperado=`. Una línea por petición.
  2. **Emisora (stream 0): nunca suspender.** `wkFastFail` ignora `__vcODBlock`
     para `.adf` de estación (el motor reintenta mientras `!IsStreamPlaying`); peor
     caso: radio muda 1-2 s, que es como se comporta el original.
  3. **Precarga de la siguiente emisora**: sólo si `__vcODBlock == 0`, y dejar los
     bytes en el worker/IndexedDB **sin** `writeFile` a MEMFS hasta que el motor
     pida el fichero.
  4. `STREAM preload` también para `nStream == 0`.
- **PASS:** cambiando de emisora 3-4 veces, **ningún `FPHASE wait` > 100 ms** y
  todas las líneas `ODSTA` de `.adf` con `block=0`.

## R3 · Textos en pantalla al azar, sin misión — **sección 1**
- **Síntoma:** "aparecen textos en pantalla al azar, sin estar en misión, y
  desaparecen muy rápido".
- **Evidencia:** esta sesión **no tiene ni una traza de texto** (`TXTMISS=0`,
  `MSGTABLE=0`, `TXTGXTFAIL=0`): el sistema que los pinta no está instrumentado.
- **Referencia:** el mod **arregla en 2510** "Fixed incorrect display of keys in
  game tips when holding down the Shift key" → el sistema de *tips* es sensible a
  combinaciones de teclas; un tip que se pinta sin motivo y se autoborra en 2 s es
  exactamente el síntoma de un aviso del canal 0 (`CMessages`) al que le falta su
  condición de limpieza.
- **Implementación concreta:** traza `SCRTXT` en `CHud::SetHelpMessage`,
  `CMessages::AddMessage*` y `CTheScripts::DisplayText`/`PrintBig`, con: clave GXT,
  canal, duración, texto resuelto (¿vacío?), si hay misión activa y quién lo pidió
  (opcode si viene del script). Limitar a 1 línea por texto. Reusar `TXTMISS`.
- **PASS:** `SCRTXT` explica el 100 % de los textos de una partida normal; 0 textos
  con clave vacía o sin emisor.

## R4 · Sonidos de las armas nuevas — **sección 1** (D8) — **ARREGLADO (D8b)**

- **Síntoma del jugador:** "los sonidos de las armas nuevas antes no sonaban,
  estaban mudas". **Era verdad** (y el plan lo daba por bueno: corregido).
- **Evidencia (partida 21/09 21:24-21:27):** 40 líneas `VICEEX sfx arma=51
  sample=9943` y `arma=54 sample=9953`, y en TODO el log **ni un** `ODSFXMISS
  sfx=9943` (la capa de audio sí trazó 153 muestras, pero ninguna del mod) ⇒ el
  motor encolaba el disparo y **descartaba la muestra al arrancar el canal**.
- **Causa (medida en código):** `cSampleManager::InitialiseChannel`
  (`src/audio/sampman_oal.cpp:1930`) sólo reproducía `nSfx < SAMPLEBANK_MAX`, y
  `SAMPLEBANK_MAX = SFX_FOOTSTEP_SAND_4 + 1` = **fin de la tabla SDT ORIGINAL**
  (`src/audio/AudioSamples.h:10144`). Las 13 muestras del mod viven en
  9941..9953, o sea POR ENCIMA ⇒ caían en la rama de comentarios de ped, no
  estaban en los slots y la función devolvía `FALSE`: **silencio total, sin
  traza**. La causa no era el mp3, ni el SDT (9954 entradas correctas), ni el
  banco, ni la tabla de armas.
- **Arreglo (D8b):** en `InitialiseChannel`, admitir el rango del mod por el
  mismo camino on-demand que `SFX_BANK_0`
  (`odViceExSample = nSfx >= SFX_VICEEX_00 && nSfx < TOTAL_AUDIO_SAMPLES`).
  Sólo cambia el rango que se admite; nada de vanilla se toca.
- **Segundo fallo, encontrado de camino:** el banco del mod declara frecuencias
  que **no existen en MP3** (36000, 33000, 22000 Hz). ffmpeg resamplea al más
  cercano (32000/22050) y la SDT seguía declarando la del mod ⇒ el buffer iba
  hasta un **12 % rápido y agudo** (9941/9942: 36000 → 32000). Ahora
  `tools/add_viceex_sfx.py` lee la frecuencia REAL del mp3 con ffprobe y escribe
  esa (y el tamaño PCM que le corresponde) en la SDT; los 13 mp3 se
  re-codificaron ⇒ **cambio de DATO** (`dataTag` `ve12` → `ve13`).
- **Pendiente (oídos del jugador):** la tabla arma→muestra sigue provisional; con
  `CRAZYTOOLS` + disparar cada arma el log dirá `arma=N sample=M` y se corrige una
  línea. Muestras en `gta_vc_browser/tmp/viceex-samples/`.
- **Herramientas nuevas (para que no vuelva a pasar sin que nadie lo vea):**
  - `tools/viceex-sfx-check.py` — **sin navegador**: tabla SDT + fichero + formato
    real (ffprobe) + que el motor admita el rango + tabla arma→muestra completa.
    Hoy: **OK, 13/13**.
  - `tools/viceext-log-check.py` bloque **D8** — lee el log y compara disparos con
    arranques: "disparó y no arrancó" = **arma muda**. Con el log de la partida
    muda el bloque da **FALLO** (probado), así que la próxima vez se ve sin oírlo.
- **Mejora concreta (sigue en pie):** usar los pares de igual tamaño/Hz como
  **segunda variante aleatoria** por arma, y la muestra de 1,07 s en el **impacto**
  del lanzagranadas.
- **PASS:** el bloque D8 del verificador da **OK** (toda muestra disparada arranca).

## R5 · Primera persona: no pasa nada — **sección 3**
- **Síntoma:** "sigue sin funcionar la primera persona".
- **Evidencia:** en toda la sesión `CAM1P … fp=0 … tog=0` (2 líneas) y **ni una**
  línea `VICEEX 1p key` (se emite en `Camera.cpp:1173` en cuanto se detecta la
  pulsación) → **la tecla no llega**. Y `CAM1P` sólo escribe al cambiar
  `tog/mode/fp` (`Camera.cpp:1096`), así que una pulsación perdida **es
  invisible**: hoy no hay dato para decidir entre "la tecla no llega" y "la puerta
  la bloquea".
- **Causa candidata 1 (probable):** la lectura de la acción con respaldo
  (`ControllerConfig.cpp:1196`) sólo cae a la tecla histórica `'V'` si la acción
  vale `rsNULL` o `0`. En la recarga con `R` se ve el mismo patrón
  (`VICEEXT reload key tecla=1056` → **1056 = `rsNULL`**): hay configuraciones
  guardadas donde la acción no tiene tecla.
- **Causa candidata 2:** la puerta
  `if(m_bLookingAtPlayer && !m_WideScreenOn && !CReplay::IsPlayingBack() && !m_bFirstPersonBeingUsed)`
  (`Camera.cpp:1183`) con la cámara de ratón de este port.
- **Lo que ya aprendimos de la referencia (blog) — tres trampas que hay que
  comprobar ANTES de dar R5 por cerrado:**
  1. **Trampa del pad:** `AffectControllerStateOn_ButtonDown`
     (`src/core/ControllerConfig.cpp:685-721`) marca `firstPerson = true` para
     `MODE_1STPERSON`, `SNIPER`, `ROCKETLAUNCHER`, `CAMERA` y `M16_1STPERSON`, y
     entonces **sólo se aceptan zoom in/out**: saltar, entrar al coche y demás
     dejan de funcionar (el blog los llama "traps"). Nuestro conmutador pide
     `MODE_1STPERSON_RUNABOUT`, que **no** está en esa lista → vamos por el camino
     bueno, pero hay que comprobarlo con traza (una vez encendido el modo).
  2. **La puerta de la 1ª persona se cierra sola** si el ped no está en control o
     `m_fMoveSpeed > 0` en el *peek* de serie; nuestro conmutador es propio, así
     que no debe heredar esa condición.
  3. **La cámara debe ir a los ojos**, no a la cabeza del hueso genérico:
     `m_pedIK.GetComponentPosition(HeadPos, PED_HEAD)` + desplazamiento al frente
     (el blog usó 0,19 m) con el **rumbo actual** del ped (`m_fRotationCur + HALFPI`),
     no el de la posición inicial; y hay que fijar
     `m_fRotationCur = m_fRotationDest = Front.Heading()` + `GetMatrix().UpdateRW()`.
- **Implementación concreta:**
  1. `CAM1P` escribe **también al pulsar** (una línea por pulsación, aunque el
     conmutador no se aplique) + la tecla cruda vista (`Keys['V']`) + el modo.
  2. La lectura con respaldo cae a `'V'` también cuando la acción vale **`rsNULL`,
     `0` o `1056`** (mismo arreglo que ya se hizo para `R`).
  3. Si la puerta bloquea: alinear la condición con `Cams[0].Using3rdPersonMouseCam()`.
  4. Una vez encienda: aplicar los tres puntos de la referencia (cámara en los
     ojos, rumbo del ped = rumbo de la cámara + `UpdateRW()`, y movimiento por
     `m_vecMoveSpeed`, no por `m_fMoveSpeed`).
  5. **Paridad con el mod (§B2):** su 1ª persona es `CCam::Process_1stPerson` con
     `FOV_FirstPerson` y `HeadBob1stPerson` **configurables desde el menú**
     (`1st Person run about`, `Real 1st Person`, `DoomMode_FirstPerson`) y cuatro
     acciones de mirada (`PED_1RST_PERSON_LOOK_*`). Nuestro port debe (a) crear esas
     cuatro acciones con teclas por defecto, (b) exponer el interruptor también en
     el menú/ajustes (no sólo como tecla), y (c) usar FOV propio en 1ª persona.
- **PASS:** `VICEEX 1p key ctrl=1 …` → `CAM1P … tog=1 mode=MODE_1STPERSON_RUNABOUT`,
  la vista cambia, **y** se puede andar hacia atrás/lateral, saltar y subir al
  coche dentro del modo (la prueba que el blog usa para saber que no caíste en la
  trampa del pad).

## R6 · Agachado: cámara fija y movimiento raro — **sección 3**

> **Estado (revisión de la sección 1, 21/09): CORREGIDO.** El agente paralelo
> dejó el movimiento al motor y el clip al bloque (lo correcto), pero el clip se
> mezclaba ANTES del control a pie normal, así que `SetRealMoveAnim()` lo pisaba
> en el mismo frame: el agachado se veía "a medias". Ahora el clip vive en
> `ViceExtCrouchAnim()` y `SetRealMoveAnim()` sale por ahí cuando `odCrouched`.
> La traza `CROUCH2` lleva `peso=` (peso real del clip en la mezcla) para poder
> demostrarlo desde el log. **PASS añadido:** `peso>=0.7` mientras andas agachado.

- **Síntoma:** "funcionan las animaciones de agacharse y caminar agachado, pero la
  cámara no va detrás de Tommy, se queda fija y Tommy se mueve raro, como hacia
  atrás".
- **Evidencia:** `VICEEXT crouch move spd=0.5 andando=127.0 clip=238 arma=0 mio=1
  estado=1` — el bloque **decide clip y velocidad** (`mio=1`) y el estado es
  `PED_IDLE`.
- **Causa (fichero:línea):** `ViceExtCrouchControl` (`src/peds/PlayerPed.cpp:2247`)
  toma `odOwnsMovement` y **se queda con el movimiento a pie** (escribe
  `m_fRotationDest` y `m_fMoveSpeed`), y `ProcessControl` (`PlayerPed.cpp:2481`)
  **no reparte el control a pie normal**. El giro y la velocidad real los aplica el
  camino normal del motor, así que el ped **avanza distinto de cómo mira** (el
  "hacia atrás") y la cámara, que sigue al ped, parece congelada.
- **Referencia (blog, sección "Why can't I go sideways or backwards…"):** el mismo
  callejón sin salida está documentado paso a paso: `m_fMoveSpeed` **sólo empuja
  hacia delante** y hay que escribir el movimiento en `m_vecMoveSpeed` a partir del
  ángulo de cámara y del stick:
  ```cpp
  float leftRight = padUsed->GetPedWalkLeftRight();
  float upDown    = padUsed->GetPedWalkUpDown();
  float padHeading = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
  float destAngle  = CGeneral::LimitRadianAngle(padHeading - TheCamera.Orientation + (PI/2));
  float neededY = Sin(destAngle), neededX = Cos(destAngle);
  float padMove = CVector2D(leftRight, upDown).Magnitude() / PAD_MOVE_TO_GAME_WORLD_MOVE;
  m_vecMoveSpeed.x = neededX * CROUCH_SPEED * padMove;
  m_vecMoveSpeed.y = neededY * CROUCH_SPEED * padMove;
  ```
  Y su propia nota: "lo mejor es engancharse a menos variables directas… hay un
  `CPed::UpdatePosition()` con un vector de cambio de velocidad, que probablemente
  sea el objetivo correcto" → **primera opción para nosotros: no reimplementar el
  movimiento**, dejar el control a pie del motor (camino Zelda) y limitarnos a (a)
  poner el clip `CROUCH_*` como blend **parcial** (que es lo que el comentario del
  propio bloque dice que hace el mod) y (b) acotar la velocidad.
- **Implementación concreta:**
  1. Devolver `odOwnsMovement = false` en el caso a pie: el movimiento lo pone el
     motor; nosotros sólo el clip y el tope de velocidad.
  2. Bajar el **objetivo de altura de la cámara** mientras `odCrouched` (que hoy
     apunta a la cabeza "de pie").
  3. Traza `CROUCHCAM` (1/s): altura del ped, altura objetivo de la cámara y
     `m_fRotationDest` vs. rumbo real → si los dos ángulos no coinciden, medido.
- **PASS:** agachado: la cámara sigue, avanza **hacia donde mira**, se puede ir
  atrás y de lado, y no patina.

## R7 · Nadar: entra, sale solo, sin animación — **sección 3**

> **Estado (revisión de la sección 1, 21/09): visto bueno + traza mejorada.**
> Histéresis y `m_vecMoveSpeed` con rumbo de cámara: coherente. Se añadió a
> `SWIM2 move` el campo **`avance=`** (metros recorridos desde la traza anterior)
> porque `spd` sólo dice lo que el motor quiso, no si el ped se movió: con
> `avance` el nado se dictamina desde el log. **PASS añadido:** nadando hacia
> delante, `avance>0.5` m/s.

- **Síntoma:** "el nadar sigue sin hacer nada; no hay animación, Tommy o se queda
  quieto en el fondo o cae al vacío".
- **Evidencia (2 s):**
  ```
  VICEEXT swim enter z=4.9 nivel=5.8 hondo=0.90
  VICEEXT swim move  spd=1.3 z=4.9 nivel=5.8 andando=127.0 clip=235
  VICEEXT swim exit  z=5.0 nivel=5.8 agua=1
  VICEEXT swim no pad=1 … control=0 estado=1 z=5.2 nivel=6.1
  ```
- **Causa 1 (oscilación):** "hondo" exige `nivel - z > 0,90`
  (`VICEEXT_SWIM_DEEP_MARGIN`) pero la flotación deja al ped a `nivel - 0,55`
  (`VICEEXT_SWIM_SURFACE_OFFSET`): al subir a la superficie **deja de estar
  "hondo"** → `swim exit` → se hunde → vuelve a entrar. Ni nada ni animación.
- **Causa 2 (sin control):** `no pad=1 control=0 estado=1` → `IsPedInControl()`
  falso y estado distinto de `PED_FALL`/`PED_JUMP` ⇒ `odCanSwim` falso y el bloque
  sale sin nadar (el "cae al vacío").
- **Referencia:** el mismo aviso del blog sobre el movimiento
  (`m_fMoveSpeed` sólo hacia delante / hay que escribir `m_vecMoveSpeed`) y la
  fuente de SA (gta-reversed) para los estados de nado, cuyos clips el mod ya
  sirve (`Swim_Breast`, `Swim_Crawl`, `Swim_Tread`, `Swim_jumpout`, `Drown`).
- **Implementación concreta:**
  1. **Histéresis**: entrar con `nivel - z > 0,90`, **seguir nadando** mientras
     `nivel - z > 0,45`; el objetivo de flotación sigue siendo `nivel - 0,55` y el
     ped **no** vuelve a caer.
  2. `odCanSwim`: aceptar `PED_IDLE`/`PED_FALL`/`PED_JUMP` y el caso sin control
     mientras haya agua honda (ya se hizo el mismo apaño para la caída al agua).
  3. Movimiento horizontal por `m_vecMoveSpeed` (misma lección que R6).
  4. Al salir a tierra: `swim_jumpout`, devolver el control **y subir el ped al
     nivel de la orilla**.
  5. Trazas: `swim exit motivo=…` y una línea con el clip realmente aplicado.
- **PASS:** 20 s nadando: **un** `swim enter`, clips 235/236/237 alternando con el
  movimiento, **ningún** `swim exit` intermedio, y sube a la orilla.

## R8 · El retroceso mueve la mira pero **no la cámara** — **sección 3** (ampliado)
- **Síntoma (jugador, ahora preciso):** "el de la mira sí mueve la mira, pero no
  la cámara".
- **Evidencia:** **sí se ejecuta**: 50 líneas `VICEEXT recoil kick slot=6
  patada=0.030 subida=0.030 multY=0.370` (slot=5 con 0,018).
- **Por qué sólo se mueve la mira (fichero:línea):**
  `CCamera::m_f3rdPersonCHairMultY` **es el vector de apuntado y la retícula**, no
  la cámara:
  - `src/weapons/Weapon.cpp:63` → `angleY = DEGTORAD((0.5 - multY) * 1.8 * 0.5 * FOV)`
    (dirección del disparo);
  - `src/core/Camera.cpp:4134/4153` (el mismo cálculo para el apuntado);
  - `src/renderer/Hud.cpp:363` → posición de la retícula en pantalla.
  Y el *pitch* de la vista es otro miembro: **`CCam::Alpha`**, que el ratón
  **recalcula entero cada frame** (`src/core/Cam.cpp:1397-1419`,
  `Alpha += AlphaOffset`) y acota en `Cam.cpp:1423-1424` (+45° / −89,5°).
  Consecuencia de diseño: una suma puntual a `Alpha` **se pierde al frame
  siguiente**; hay que llevar un **desplazamiento persistente** y sumarlo cada
  frame **después** del ratón.
- **Referencia:** el mod lo declara como `RecoilWhenFiring=1` y el enfoque de la
  comunidad (CLEO `WeaponRecoilAuto`) es **empujar la mira** (lo que ya tenemos);
  el kick de cámara es la mejora que el jugador pide y SA lo hace sobre el
  apuntado. No hay que reinventar nada: es la técnica estándar de "camera kick
  con recuperación" (`offset que decae`), aplicada al miembro correcto.
- **Implementación concreta:**
  1. `static float s_odRecoilAlpha` (radianes) y su gemelo aplicado
     (`s_odRecoilAlphaApplied`), como ya se hace con `multY` (diferencial, sin
     deriva).
  2. El kick (`CWeapon::ViceExtRecoilKick`) suma a `s_odRecoilAlpha` (además de a
     `multY`, que **se queda**: mueve la bala, y el jugador dice que eso ya va).
  3. En `CCam::Process_FollowPed_SA` (y en el proceso de apuntado de 1ª persona),
     **después** de aplicar `AlphaOffset` y **antes** del clamp:
     `Alpha += s_odRecoilAlphaApplied - …` y decaimiento por
     `VICEEXT_RECOIL_RETURN * dt`; usar el clamp existente para no romper el
     límite vertical.
  4. Patada por arma (pistola < SMG < rifle < escopeta) en **grados
     perceptibles** (arrancar en 0,35° el rifle y 0,8° la escopeta) y
     `CamShakeNoPos` proporcional (hoy `0.004 + 0.2*kick` ≈ 0,01 = invisible).
  5. Sólo a pie y con `VICEEXT_RECOIL` encendido (y respetar el `features.ini`).
  6. Traza: `VICEEXT recoil kick … alpha=… grados=…`.
- **PASS:** disparando, `Alpha` sube y **vuelve** al valor previo; en pantalla se
  ve subir el arma y la vista; la mira sigue moviéndose como ahora.

## R9 · El arma apunta a un costado (AUG, escopeta nueva) — **sección 3** (+ dato: sección 1)

> **Estado (revisión de la sección 1, 21/09): CORREGIDO.** El bloque que fija el
> rumbo a la cámara estaba **antes** del bloque de movimiento, que reasigna
> `m_fRotationDest = neededTurn` unas líneas después: el override se perdía en el
> mismo frame (y dejaba un giro brusco + deriva). Movido **después** del
> movimiento, que es donde su asignación manda. Sigue limitado a caminar,
> apuntando y con arma de apuntar.

- **Síntoma:** "las armas se apuntan a un costado y no hacia donde está la mira".
- **Descartado ya desde fuera:** el `.dff` del mod está byte a byte; los clips
  existen con los nombres que usa el motor (`STEYR_fire|crouchfire|reload|crouchreload`);
  los grupos (`AnimManager.cpp:1049-1051`) usan `aWeaponAnimDescs` (el layout
  estándar) y el `fire offset` del AUG (0,7 / −0,03 / 0,14) es del orden de los
  rifles de serie. La mira se traza bien (`SIGHT arma=54 mira=5`).
- **Referencia:** el blog documenta el caso "cámara arriba, ped mirando a otro
  lado" y su arreglo: fijar el rumbo del ped a la cámara
  (`m_fRotationCur = m_fRotationDest = Front.Heading()`, `SetHeading`,
  `GetMatrix().UpdateRW()`) y, si hay **blend parcial** de otro grupo
  (agachado/nado) sin peso a 0, la pose se corrompe.
- **Implementación concreta (medir primero, en este orden):**
  1. Traza `AIMDIR` (1/s apuntando): arma, **desviación en grados** entre la
     dirección de apuntado de la cámara y el rumbo del ped (y de su torso),
     grupo/clip de animación y peso de los blends activos.
  2. Si la desviación es de **todas** las armas al caminar → es nuestro bloque de
     apuntado en movimiento (`VICEEXT_AIM_WALK`, `PlayerPed.cpp:1633+`): aplicar el
     arreglo de rumbo de la referencia.
  3. Si sale **sólo** con las nuevas (`steyr`, `deagle`) → comparar el
     `AnimAssocDesc` del grupo nuevo contra los fotogramas reales de los clips del
     mod (lectura del `.ifp`, sección 1) y, mientras tanto, probar el grupo
     estándar equivalente (`steyr` → `rifle`, `Shotgun2` → `shotgun`) para aislar.
  4. Si sólo pasa tras agacharse o nadar → es el **blend parcial** que no vuelve a
     peso 0 (lo arregla R6/R7; la traza de (1) lo confirma con el peso).
- **PASS:** `AIMDIR` con desviación < 5° en todas las armas y captura del jugador
  con AUG y escopeta.

## R10 · `R` no recarga — **sección 3**
- **Síntoma:** "al darle a la R a un arma no recarga, no hace absolutamente nada".
- **Evidencia:** el camino **sí se activa**: `VICEEXT reload key tecla=1056
  arma=54 estado=0 clip=28/30 total=441` y `VICEEXT reload start clip=28
  total=441 type=54 ms=1000`. **Nunca** hay línea de recarga terminada (esa traza
  no existe). Las negativas (`motivo=sin-municion`, `motivo=cargador-lleno`) son
  correctas.
- **Dato del `weapon.dat` del mod (columna `a`, 4.º dígito):** sus armas llevan
  `RELOAD` (`0x28040`) → el motor **tiene que** cerrar la recarga al terminar la
  animación (`STEYR_reload` / `RIFLE_load`).
- **Implementación concreta:** traza de cierre
  `VICEEXT reload done|fail motivo=… clip=…/… total=…` en el punto donde el motor
  da la recarga por terminada **y** en el aborto; revisar que el bloque no corte
  la animación ni deje `m_nTimer` a medias.
- **Paridad con el mod (§B2):** el mod **crea una acción de control real**
  (`PED_RELOAD`) en vez de depender del respaldo. Hacer lo mismo: añadir la acción
  a `ControllerConfig` con tecla por defecto `R`, usarla en el bloque de recarga y
  dejar el respaldo actual sólo como red de seguridad. Así el jugador podrá
  rebindearla y el menú de controles la mostrará.
- **PASS:** `reload done` con cargador lleno y `total` reducido, dos veces
  seguidas.

---

# D. Fallo que necesita a otra sección

## R11 · Luces de la sirena del coche de policía — **sección 2**
- **Síntoma:** "las sirenas de los carros de policía siguen sin cargar" (se ven
  destellos de color, no la barra como en la captura del mod).
- **Dato ya medido:** el mod trae su propio `police.dff`/`police.txd` (los nuestros
  son byte a byte los suyos) y el modelo trae los dummies `servicelights`,
  `servicelights_1/2/3`. **No falta ningún fichero.**
- **Causa (fichero:línea):** las coronas de la sirena están **clavadas por índice
  de modelo y por posición fija** en `src/vehicles/Automobile.cpp:2166-2255`:
  para `MI_POLICE`, `pos1=(0.7,−0.4,1.0)`, `pos2=(−0.7,−0.4,1.0)`, corona de
  `0.4` de tamaño y `50` de alcance, `TYPE_STAR`. El código **no lee los dummies**
  del clump, así que con el modelo del mod (geometría distinta) las coronas caen
  donde no toca.
- **Referencia:** el método de la comunidad ("GTA Vice City Custom Police Lights"):
  **las luces de servicio se colocan leyendo los dummies del vehículo**. Es
  exactamente la función del mod "Service lights for service cars" (v3.0).
- **Implementación concreta:**
  1. Al cargar el clump del vehículo, buscar dummies `servicelights*` y guardar sus
     matrices en el vehículo (una vez, no por frame).
  2. En el bloque de sirena (2165-2255): si hay dummies, usar **sus posiciones**
     (interpolar rojo/azul entre ellas); si no, caer a las posiciones fijas
     actuales (paridad con lo que ya funciona en los coches de serie).
  3. Subir tamaño/alcance de la corona para el modelo del mod (0,4/50 es pequeño
     para una barra): comparar captura y ajustar.
  4. Traza `SVLIGHTS model=… dummies=N pos=…` para probarlo sin navegador.
  5. **Los nombres exactos ya no son una incógnita (§B2):** el binario del mod usa
     `servicelights`, `servicelights_0`, `servicelights_1`, `servicelights_2`,
     `servicelights_3` **y `servicelightson`** (esta última, marca de encendido).
     Buscar exactamente esos nombres y, si `servicelightson` existe, usarlo para
     saber cuándo la barra debe lucir (además del `m_bSirenOrAlarm`). El mod también
     reproduce `DMAudio.Service` para el audio de servicio.
- **PASS:** `SVLIGHTS dummies=4` en el coche del mod y captura con la barra
  encendida (rojo/azul alternando donde está la barra, no en el techo).

## R12 · Nota común: el tirón de 1.256 ms
- Es un **bloqueo de hilo**, no un cuelgue del motor (0 `ENGERR`/`JSERR`, y el
  fichero de las 23:10Z deja de escribir a los 2 s del arranque). El único culpable
  conocido es la capa on-demand → **R2**. Con `ODSTA` en el wasm, el log dirá el
  fichero exacto.

---

# E. Backlog del mod Extended (lo que queda vivo)

Excluido por decisión del jugador: taller de tuning, modo foto, GPS, ropa/tienda y
periódicos, ucraniano, CLEO Redux. Excluido por imposibilidad técnica: `main.scm`
del mod (opcodes propios), `ViceEx.exe`/`.asi`, su banco de audio entero y sus
`Script changes`. **No se repite** lo ya pedido y repartido (1ª persona, agachado,
nado, escalada, sprint armado, depósito, ranuras de guardado, autoguardado,
autocentrado, drive-by, esconderse de la policía, sirenas).

| # | Función | Versión | Dueño | Coste | Nota |
|---|---|---|---|---|---|
| X1 | **Luces de servicio** de policía/FBI | 3.0 / 1.0 | sección 2 | medio | = R11, con intermitentes del FBI Car/Rancher |
| X2 | **Triángulo de salud** sobre la cabeza | 3.0 | sección 1 | bajo | sólo pintar |
| X3 | **Un proyectil explota si le disparas** | 2.6 | sección 1 | bajo | encaja con `BlowUpExplosiveThings` (`Weapon.cpp`) |
| X4 | **Gasolinera explota** al chocarla | 2.6 | sección 2 | bajo | reutiliza el depósito (`gastank`, ya portado) |
| X5 | **Variaciones de peds** | 2.6 | sección 1 | medio | cuidado con `.ide`/GXT |
| X6 | **Luces lejanas** (`EnableDistantLights=1`) | 2.6 | sección 1 | medio | ya hay partículas |
| X7 | **Iconos de emisora** al cambiar + texturas | 2.6 | sección 1 | bajo | las 9 `R*` ya se cargan |
| X8 | Policías **se agachan antes de cubrirse** | 2.6 | sección 2 | medio | clips `Crouch_*` ya servidos |
| X9 | **Aspas del helicóptero sin piloto** | 2.6 | sección 2 | bajo | física del rotor |
| X10 | **Detalle de vehículo**: rueda que se cae, puertas que golpean, giro aleatorio de aparcados, rotación de ruedas al salir, intermitentes | 2506/2.0/2.5 | sección 2 | medio | piezas independientes |
| X11 | **Luces del coche**: textura, se rompen al chocar/disparar, haz separado, humo y fuego del capó | 2.5 | sección 2 | medio | mismo camino que R11 |
| X12 | **`features.ini` restante**: estrellas ocultas, quitar ceros del dinero, no saltar del coche en marcha, sacudida a alta velocidad, tanques disparando, sin trazas de balas, GPS RGB, coches que no brillan volcados, media vida al parar, modo espejo | 2.5/2506 | sección 1 | bajo | interruptores, no mecánicas |
| X13 | **Trucos nuevos**: `BIGHEADS`, `RCROCKET`, `AEZAKMI`, `IAMINVINCIBLE`, `AIRWAYS`, `BIGSMOKE` | 1.0-2.5 | sección 1 | bajo | `Pad.cpp` es compartido: añadir al final |
| X14 | **Caminar con ALT** | 2.0 | sección 3 | bajo | en el mod es la acción `PED_WALK` (§B2): mismo nombre y listo |
| X15 | **Sentarse y moverse sentado** + velocidad de peds | 1.0/1.5 | sección 3 | bajo | |
| X16 | **Sensibilidad de apuntado** y **precisión de IA** | 2506 | sección 3 + 2 | bajo | numérico |
| X17 | **Hints de coleccionables** en el mapa + blips (farmacias, Bomb Shop, estadio, casas en venta) | 2506/2.6/2.5 | sección 1 | medio | texto + iconos |
| X18 | **Sonido del agua al nadar** | 2510 | sección 1 | bajo | depende de R7 |
| X19 | **Teclas de pista al mantener `Shift`** | 2510 | sección 1 | bajo | toca D7 (`KEYICONS`) |
| X20 | **Paneles** de Ammu-Nation/ferreterías + edificios de Little Haiti | 2510 | sección 1 | medio | riesgo de LOD (`D10`) |
| X21 | **Colores de vértice de noche** | 2506 | sección 1 | medio | toca el sombreado |
| X22 | **Radio de aparición mayor** (vehículos/peds) | 1.0 | sección 1 | bajo | `CarCtrl` + `limits.ini` del mod (`NUMPEDS=140`, `NUMVEHICLES=130`) |
| X23 | **Crash del Micro-UZI** | 2510 | sección 1 | bajo | probar el arma a fondo |
| X24 | **Sueltos por datos**: saltar la escena de Ken, "burger" en Road Kill, pickups de cámara, granada de humo, 4 garajes, aparcados nuevos (Ambulancia/Stretch), blips, modelo de Lance | 2.6/2.5 | sección 1 | medio | `main.scm` no se puede portar; lo demás por datos |
| X25 | **Escalada** (`EnableClimbing=0`) | 3.0 | libre | medio | `04-opcional-escalar.md` |
| X26 | **Pulido vanilla+**: SWAT con gas, dos helicópteros a 5-6 estrellas, daño al Maverick por arma, cañón de agua, conductores que reaccionan, muro que protege, partículas más duraderas | 1.0 | sección 2 | alto | fase posterior |

**Orden recomendado:** `R1`(hecho) → `R2`, `R3` (sin build pesado) → `R7`, `R6`,
`R5`, `R8`, `R10`, `R9` (sección 3: nado y agachado son los que se notan como "no
existen"; `R8` es el que el jugador acaba de pedir) → `R11` (sección 2) → `X2`,
`X3`, `X7`, `X12`, `X13`, `X18`-`X19` → resto.

---

# F. Reparto para los sub-agentes (listo para pegar en cada hilo)

Yo (sección 1) no puedo escribir en los otros hilos; estos tres encargos están
escritos para que **se peguen tal cual** en `aux 1` (sección 2) y `aux 2`
(sección 3), y el tercero es el mío.

**Encargo para aux 2 (sección 3) — cámara y movimiento:**
> Lee `.agents/plans/mecanicas/06-plan-correcciones-5a-partida.md` (§B referencias
> y bloques R5-R10). Ejecuta **en este orden**: R8 (retroceso: kick de cámara
> sobre `CCam::Alpha` con offset persistente, nunca un pico aislado), R7 (nado con
> histéresis), R6 (agachado devolviendo el movimiento al motor), R5 (1ª persona:
> primero la tecla, luego cámara en los ojos y movimiento por `m_vecMoveSpeed`),
> R10 (cierre de recarga) y R9 (medir `AIMDIR` antes de tocar nada). Cada bloque:
> código + traza nueva + entrada al final de tu plan de sección y en
> `.agents/HISTORIAL.md`. Nada de commits. Verificación por log archivado en
> `gta_vc_browser/logs/`, sin sondas de navegador.

**Encargo para aux 1 (sección 2) — luces de servicio:**
> Lee `.agents/plans/mecanicas/06-plan-correcciones-5a-partida.md` (bloque R11 y
> X1/X4/X9/X10/X11). Empieza por R11: las coronas de sirena están clavadas por
> modelo y posición en `src/vehicles/Automobile.cpp:2166-2255`; hay que leer los
> dummies `servicelights*` del clump (método de la comunidad) con reserva a las
> posiciones actuales, traza `SVLIGHTS` y captura. Luego X4 y X9. Nada de commits.

**Mi encargo (sección 1):** R1 (hecho), R2, R3, R4 y, en cuanto se cierre el
enlace, subir `VERSION`/`dataTag` y dictar la partida desde `logs/`.

---

# G. Método

1. **Un bloque = un cambio + una traza + un criterio PASS escrito aquí.**
2. **Prohibido lanzar sondas de Chrome** (saturan la CPU del jugador y contaminan
   las medidas). Verificación: jugar → archivo en `logs/` → leer con
   `python gta_vc_browser/tools/viceext-log-check.py gta_vc_browser/logs/odtrace-<fecha>.log`
   (posición, **no** `--file=`).
3. Cada traza nueva se añade a la utilidad de lectura para que el veredicto salga
   automático.
4. Nada de commits; se anota cada bloque cerrado en el plan de su sección y en
   `.agents/HISTORIAL.md`.
