---
name: 09-encargo-subagente-3
status: EXECUTED
type: research
domain: gameplay
owner_rules: .agents
created: 2026-09-21
---

# 09 · Encargo al subagente 3 — la partida del 21/09 (7ª): cámara, apuntado y nado

Estado: **listo para arrancar las dos partes a la vez**. La parte B (tú) es
§1/§6; la parte A (§8) la lleva la sección 1 y **no está empezada**: se arranca
al mismo tiempo para no pisarnos. Ficheros de cada uno, en la tabla de §2.

```
--- Para pegar en el hilo del subagente 3 ---
Lee .agents/plans/mecanicas/09-encargo-subagente-3.md y ejecuta la parte B
(H1, H2, H3 y el §6) en ese orden. Para cada bloque: código + traza + criterio
PASS, compilando SOLO tus objetos con ninja (sin enlazar) y anotándolo al final
de .agents/HISTORIAL.md. Respeta la tabla de ficheros de §2: no toques
src/renderer/Hud.cpp, src/text/Messages.cpp, src/core/config.h ni
gta_vc_browser/** (son de la sección 1). NO enlaces el paquete: lo enlaza la
sección 1 cuando los dos terminemos. Para mirar el vídeo usa
tools/frames-at.sh con la base que dice §0. Cuando acabes avisa:
"parte B lista, sin enlazar".
--- Fin ---
```
Fuente de verdad de esta partida: `gta_vc_browser/logs/odtrace-2026-09-22_01-23-46.log`
(537 KB, build **ve19**, datos **ve13**) y el vídeo `Grabación de pantalla 2026-09-21 203022.mp4`
(6:00, 30 fps) que se grabó **a la vez** que ese log, entre `01:24:22Z` y `01:30:22Z`.

> Regla nueva (21/09): antes de pedirle una partida al jugador, pasa
> `bash gta_vc_browser/tools/check-served-build.sh`. Dos partidas seguidas se
> perdieron por jugar un motor viejo; esa herramienta lo detecta en un segundo.

## 0. Cómo mirar el vídeo (esto ya se puede, y es la novedad)

Los fotogramas **se ven** con las herramientas de ficheros (se leen como imagen).
Para sacar el instante exacto de una traza del log:

```bash
cd /c/Users/s0rno/OneDrive/Documents/re3
gta_vc_browser/tools/frames-at.sh "Grabación de pantalla 2026-09-21 203022.mp4" \
    2026-09-22T01:24:22Z 2026-09-22T01:26:05.552Z 2026-09-22T01:27:15.549Z
# y luego leer los PNG de gta_vc_browser/tmp/frames/ (se ven)
```

`t = marca_UTC − 01:24:22Z`. Los PNG ya extraídos de esta partida están en
`gta_vc_browser/tmp/frames/` (`t3`, `t9..t20` agachado, `t103..t112` apuntado,
`t135/t141` policía, `t173..t179` nado).

## 1. Lo que el vídeo + el log demuestran (por bloque)

Todas las cifras salen del log citado; los `t=` son segundos de vídeo.

### H1 · AGACHADO: el clip sale, la CÁMARA no baja (esto es lo que se ve mal)

- **Log:** `CROUCH2 h=10.46 rotDest=-0.17 rumbo=-0.17 clip=239 peso=0.99` …
  `clip=238 peso=1.00` (25 muestras). El clip correcto se aplica y con peso 1.
- **Vídeo:** `t=11` y `t=20` muestran a Tommy **agachado de verdad**;
  `t=12` muestra la **cámara dentro de su cabeza** (la nuca/espalda llena media
  pantalla). `t=16` camina agachado con la cámara por encima.
- **Conclusión:** R6 funciona; lo que falta es **bajar la cámara con el ped**
  (`CPed::bIsDucking`/altura objetivo de la cámara en `Cam.cpp`). El motor no sabe
  que está agachado.
- **PASS:** nueva traza con la altura de cámara; mientras `odCrouched`,
  `cam.z − ped.z` baja ~0,5 m y en el vídeo no se ve la nuca.

### H2 · NADO: la cámara se queda clavada en la superficie y el estado "flapea"

- **Log:** `VICEEXT swim enter z=4.9 nivel=5.8 hondo=0.90`, luego
  `SWIM2 exit motivo=poco-hondo clip=236` **10 veces**, con
  `SWIM2 move spd=1.3 avance=2.09 sube=0.68 clip=235 hondo=0.57` … y
  `z=5.6 nivel=6.0` (profundidad 0,40 < margen de mantenimiento 0,45): el ped
  flota a `nivel−0.55`, el **nivel del agua sube con las olas** y el estado sale
  y entra del agua.
- **Vídeo:** `t=173`, `t=176`, `t=179` — la cámara está **en el plano del agua**
  (se ve la superficie desde dentro, con burbujas) y el ped nadando por debajo:
  no hay cámara detrás. Eso es exactamente "la cámara no sigue al personaje / no
  va en la dirección que se espera".
- **Arreglo:** (a) enganchar el estado (`s_odSwimming`) mientras haya agua y
  `hondo` > ~0,25 (no por el margen de 0,45, que el propio flotar cruza);
  (b) mientras `s_odSwimming`, llevar la cámara detrás del ped a la altura de la
  superficie (`Cam.cpp`, proceso de cámara propio o el de nado si existe).
- **PASS:** 0 `exit motivo=poco-hondo` estando en agua honda; `SWIM2 avance` > 0,5
  con la cámara detrás (comprobable en vídeo).

### H3 · APUNTADO de las armas del mod: la pose no apunta (AUG y lanzagranadas)

Mapa de ids (de `src/weapons/WeaponType.h`, los `arma=` del log):

| arma | qué es | grupo (`m_AnimToPlay`) | desv (min/med/max) | peso |
|---|---|---|---|---|
| 49 | DesertEagle | 61 = `ASSOCGRP_DEAGLE` | 0 / 0 / 0 | 1.00 |
| 50 | Shotgun2 | 16 = `BUDDY` | 0 / 16,9 / 21,6 | 1.00 |
| 51 | Uziold | 14 = `COLT` | 0 / 1,9 / 68,9 | 1.00 |
| 54 | **Steyr (AUG)** | 62 = `ASSOCGRP_STEYR` | 0 / 2,5 / **73,6** | 1.00 |
| 55 | **Gr. launcher** | 16 = `BUDDY` | 0 / 45,1 / **59,5** | **0.00** |

- **Vídeo:** `t=103`, `t=106`, `t=109`, `t=112` — armas largas **al costado**,
  cañón hacia abajo, **sin retícula**.
- **Datos ya verificados (no hace falta repetirlo):**
  `steyr.ifp` = `STEYR_fire`, `STEYR_crouchfire`, `STEYR_reload`,
  `STEYR_crouchreload`; `deagle.ifp`, `buddy.ifp`, `uzi.ifp`… existen en
  `gta_vc_browser/streamed/models/gta3.img/`. El nombre→clip es **case-insensitive**
  (`strcasecmp`, `AnimManager.cpp:1205`), así que las minúsculas de
  `aSwimAnimations`/`aCrouchAnimations` no son el problema.
  El mod da al lanzagranadas el grupo `buddy` en su propio `weapon.dat`
  (`Gr_launch … buddy 6 20 7 …`) — o sea que el dato es del mod, no un error
  nuestro.
- **Por dónde va la causa:** el ped **no gira hacia la cámara** para esas armas
  (`PlayerPed.cpp`, bloque R9 y su orden respecto al rumbo de movimiento) y en el
  lanzagranadas **no se resuelve la animación de apuntado/disparo** (`peso=0.00`):
  mirar `PedFight.cpp` (`GetFireAnimGround`/`GetFireAnim`/`GetCrouchFireAnim`) y
  `m_AnimToPlay` del arma 55, y cómo `Ped.cpp:8504` mezcla el grupo.
- **PASS:** `AIMDIR desv < 5` en las 8 armas nuevas y la pose de apuntado en
  vídeo, con la retícula dibujada.

### H4 · RETÍCULA: no sale con las nuevas → **parte A (sección 1)**, ver §8 L2/L3

### H5 · ICONO del lanzagranadas en el HUD → **parte A (sección 1)**, ver §8 L2/L3

Los dos son HUD y los lleva la sección 1 para que no haya dos manos en
`Hud.cpp`. Tú no los toques: si ves algo raro en ellos, anótalo en el HISTORIAL.

## 2. Reparto: **cero ficheros compartidos**

| Fichero | Quién |
|---|---|
| `src/core/Cam.cpp`, `src/core/Camera.cpp` | parte B (tú) |
| `src/peds/PlayerPed.cpp`, `src/peds/PedFight.cpp` | parte B (tú) |
| `src/vehicles/Vehicle.cpp`, `src/vehicles/Automobile.cpp` | parte B (tú) |
| `src/renderer/Hud.cpp`, `src/text/Messages.cpp` | parte A (sección 1) |
| `src/core/ControllerConfig.cpp` | parte A (sección 1) |
| `gta_vc_browser/**` (tools, datos, web) | parte A (sección 1) |
| `src/core/config.h` | **nadie**: se pide a la sección 1 (define nuevo = una línea) |

Si te hace falta tocar un fichero de la columna A, pide el cambio y sigue con
otro bloque: así nadie reescribe el trabajo del otro ni se pisan los objetos.

## 2b. Reglas del reparto (no romper lo que ya va)

- **No toques** `gta_vc_browser/**`, `src/text/Messages.cpp`, `src/renderer/Hud.cpp`
  ni `src/core/config.h` **para lo que ya está hecho**: pide el cambio a la
  sección 1. (H4/H5 sí son de HUD: coordina antes de tocar `Hud.cpp`; la sección 1
  lleva ahora mismo el HUD y los textos.)
- **Compila sólo tus objetos** con ninja por fichero (el enlace es lo caro):
  ```bash
  export EMSDK=/c/Users/s0rno/emsdk
  export PATH="$EMSDK/upstream/emscripten:$EMSDK/node/24.19.0_64bit:$PATH"
  ninja -C gta_vc_browser/build/web src/CMakeFiles/reVC.dir/peds/PlayerPed.cpp.o
  ```
  Ficheros que vas a tocar y su objeto: `core/Cam.cpp.o`, `core/Camera.cpp.o`,
  `peds/PlayerPed.cpp.o`, `peds/PedFight.cpp.o`, `renderer/Hud.cpp.o`.
- **NO enlaces el paquete**: lo enlaza la sección 1 cuando ambas partes terminen
  (y pasa `check-served-build.sh` antes de pedir partida).
- Cada bloque: **código + su traza + criterio PASS**, y una entrada al final de
  `.agents/HISTORIAL.md`.

## 3. Trazas que ya existen y hay que leer (no inventes otras)

`CROUCH2` (clip/peso/rotDest/rumbo), `SWIM2` (`move` con `avance`/`sube`, `exit`
con `motivo`), `AIMDIR` (arma/desv/grupo/clip/peso), `RECOIL2`, `CAM1P` +
`VICEEXT 1p key/set`, `SVLIGHTS`, `VICEEXT recoil kick slot=…`. El verificador
`gta_vc_browser/tools/viceext-log-check.py` ya dictamina R2/R3/D8; añade ahí lo
tuyo si quieres (es de la sección 1, pídelo).

## 4. Lo que NO es de este encargo

- Recojo de la sección 1 (audio D8/D8b, datos ve13, R1 logs con fecha, R2 emisora,
  R3 textos, R5b conmutador 1ª persona).
- Lo ya entregado por el subagente 2 (X8, X9, X4, X10, X11, X26).
- Sirenas por dummies (`SVLIGHTS`): **es de vehículos**. Ver §6, es un fallo de
  datos duro y hay pista.

## 6. SVLIGHTS: `dummies=0` con el modelo que SÍ los tiene (pista cerrada)

- Log: `SVLIGHTS model=156 dummies=0 on=0 pos=0.00,0.00,0.00` (una sola vez, o
  sea que el resultado quedó cacheado). `156` es **`police`**
  (`gta_vc_browser/streamed/data/default.ide:208`).
- El `.dff` servido **sí trae los dummies**:
  `grep -a -o -i "servicelight[a-z_0-9]*" gta_vc_browser/streamed/models/gta3.img/police.dff`
  → `servicelight`, `servicelights`, `servicelights_1`, `servicelights_2`,
  `servicelights_3` (**no hay `servicelights_0`**, y sí `ambulan.dff`/`firetruk.dff`).
- El código (`src/vehicles/Automobile.cpp:1860-1935`) busca `servicelights`,
  `servicelights_0..3` y `servicelightson`; el `.dff` no tiene `_0`, así que el
  array correcto es `servicelights` + `servicelights_1..3` (y `servicelightson`
  solo si existe). Con `n=0` no se pinta ninguna barra → "solo se ven colores"
  (las coronas fijas por modelo de `:2166-2255`).
- **Segunda pista, probablemente la buena:** `CVehicle::FindDummyFrame`
  (`src/vehicles/Vehicle.cpp:1471`) usa `RwFrameForAllChildren(...)`, que recorre
  **solo los hijos directos** del frame del clump. Si el mod colgó la barra más
  abajo en la jerarquía (debajo del chasis), no la ve y devuelve `nil` para todos
  los nombres. Hay que hacer el buscador **recursivo** (o buscar por profundidad)
  y volver a mirar el log. Confirmarlo cuesta poco: la jerarquía del `.dff` se
  lee con un volcado de chunks (clump → framelist → struct → frame, con el índice
  de padre en el byte 48 de cada frame).
- **PASS:** `SVLIGHTS model=156 dummies>=1` y la barra de la sirena en vídeo.

## 8. Parte A · sección 1 (lo liviano): qué voy a construir

Todo esto está preparado y **no está empezado**: se arranca a la vez que la
parte B para no pisarnos. Ninguno de estos ficheros está en la tabla de la
de arriba, así que se puede trabajar en paralelo sin conflictos.

- **L1 · Verificador de esta partida** (`gta_vc_browser/tools/viceext-log-check.py`,
  solo herramientas, riesgo cero): que dictamine solo, con el log de la próxima
  partida — 1ª persona (`VICEEXT 1p set` → `tog=1` al menos una vez),
  apuntado por arma (`AIMDIR desv<5` y `peso>0` en los ids 49-55),
  agachado (que la cámara baje: marca nueva de la parte B), nado
  (0 `exit motivo=poco-hondo` en agua honda) y sirenas
  (`SVLIGHTS dummies>=1`). PASS/FAIL por bloque, como ya hace con R2/R3/D8.
- **L2 · Icono del lanzagranadas + retícula** (`src/renderer/Hud.cpp`, único
  fichero de motor): `DrawWeaponIcon` (`:599`) elige textura por nombre de arma y
  el tipo 55 no tiene; y la retícula no se dibuja con las nuevas aunque apunten.
  Hay que cruzar los nombres con las TXD servidas (`tools/txd_inspect.py`) y
  revisar los flags del arma (`CANAIM`, `CANAIM_WITHARM`).
- **L3 · Duración de los avisos en pantalla** (`Messages.cpp`/`Hud.cpp`):
  `SCRTXT` demostró que los "textos al azar" son los avisos de truco
  (`time=0`) y `^ELIMINADO!` (`time=4000`). Decidir con el jugador cuánto deben
  durar (hoy el de truco se va casi al instante).
- **L4 · El enlace único** (`gta_vc_browser/build.sh`, `web/lib/index.js`):
  cuando la parte B diga "listo, sin enlazar", paso `check-served-build.sh`,
  subo la etiqueta y enlazo una sola vez. **Parcial ya hecho**: el 21/09 21:09 se
  enlazó `ve21` con lo de la sección 1 (pantalla de carga L6 + retícula +
  caza-todo de textos + conmutador de 1ª persona) para que el jugador pudiera
  ver la pantalla de carga sin esperar a la parte B. El enlace **final** sigue
  pendiente de H1-H5.
- **L6 · Pantalla de carga de partida: una sola portada y una sola barra**
  (`main.cpp` + `main.h` + `skel/glfw/glfw.cpp`, ficheros solo míos). Ver §8b.
- **L5 · Auditoría de la próxima partida**: leer el log nuevo, cruzar con el
  vídeo (`frames-at.sh`) y decir bloque por bloque qué pasó.
- **Cola de ayuda para la parte B** (si te sobra tiempo y no quieres tocar nada
  mío): volcar el árbol de frames de `police.dff` para confirmar la profundidad
  de los dummies `servicelights` y decir con datos si `FindDummyFrame` recursivo
  basta, o hacer el volcado de `desv` por arma del `AIMDIR` de las 104 muestras
  (el script está en el HISTORIAL).

## 9. Biblioteca de referencias (código de otros que SÍ sirve)

Barrido del 21/09 buscando código ajeno reutilizable para lo que nos queda.
Lo ordeno por "nuestro pendiente → de dónde copiar → qué sacar".

| Pendiente | Referencia | Qué sacar |
|---|---|---|
| **§6 sirenas** ("solo se ven colores") | **SilentPatchVC** — `CHANGELOG-VC.md`: *"⚙️ Fixed siren corona placements in Police, Firetruck, Ambulance, Enforcer, Vice Cheetah, and FBI Washington"* y `SilentPatchVC/SilentPatchVC.cpp:864-943` (`FBISirenCoronaFix`, vía el sistema SVF) | Las **posiciones de corona por modelo** y el mismo método por *dummies* que ya nos dio el mod (`servicelights*`). Es el arreglo exacto de nuestro fallo. |
| **Textos que salen unos fotogramas** (R3b) | SilentPatch: *"Mission title and 'Mission Passed' texts now stay on screen for the same duration, **regardless of screen resolution**"* y *"Fixed a rare, random crash ... texts added by other mods **outside of the GXT file**"* | (a) el temporizador de los mensajes **se escala con la resolución**: a 1840×928 un aviso de 1000 ms se puede quedar en 3-4 fotogramas, que es EXACTAMENTE lo que el jugador describe; (b) el mod añade textos fuera del `.gxt` (nuestro `TXTMISS`), que es donde el motor puede fallar. |
| **Apuntado de las nuevas** (H3/R9) | SilentPatch: *"The mouse **vertical** axis sensitivity now matches horizontal"* y *"The mouse vertical axis **does not lock during camera fade-ins**"* | Dos bugs conocidos del eje vertical del ratón en VC. Si nuestro puerto arrastra alguno, explican que el AUG apunte "al costado" sin que el modelo tenga nada malo. |
| **Recarga con R** ("no hace nada") | SilentPatch `ZeroAmmoFix` (`SilentPatchVC.cpp:752-765`): *armas que se entregan con 0 balas* | Un arma con 0 en el cargador puede quedar sin camino de recarga. Comprobar el caso del arma del mod con munición 0. |
| **Teclas "raras"** (V/1ª persona, `v=1` durante 3 s) | SilentPatch `:816-831`, `:1048-1060`, `:1097-1130`: los tres búferes de teclado (`NewKeyState`/`OldKeyState`/`TempKeyState`) y su orden de copia | **Ya verificado en nuestro código:** `src/core/Pad.cpp:1880` hace `OldKeyState = NewKeyState; NewKeyState = TempKeyState;` = el orden correcto. No es de ahí. |
| **Nado / agachado / apuntar andando** (H1/H2/H3) | `gta-reversed/gta-reversed` (GTA:SA reimplementado, público) — `CPed`/`CPlayerPed`/`CWeapon` | Es la lógica de San Andreas, y **los clips que sirve el mod son los de SA** (`Crouch_*`, `Swim_*`): velocidad agachado, rumbo y cámara de nado, y mover en `m_vecMoveSpeed` (la lección de la 1ª persona). |
| **1ª persona** (R5) | Blog *"Adding a first-person mode to GTA III"* (jborza.com), sobre **este mismo motor** | Las 3 trampas (el `MODE_1STPERSON` de serie es un *peek*; el pad restringe controles en modos 1ª persona; `m_fMoveSpeed` sólo empuja hacia delante → hay que escribir `m_vecMoveSpeed`) y la posición de ojos con `m_pedIK.GetComponentPosition(HeadPos, PED_HEAD)`. Ya aplicado. |
| **LOD / streaming** | `aap/librw` (upstream) + el propio changelog del mod (*"Last changes for librw from the original repository"*) | Comparar nuestro `vendor/librw` con upstream en el camino de streaming/`DrawDistance` antes de inventar un LOD propio. |
| **Luces de servicio (método)** | `niltwill/vc-cleo-scripts` → carpeta `ExtraFBICar` (CLEO con fuente) | Patrón de intermitencia y lectura de dummies del coche, tal cual. |
| **Comportamiento esperado (la especificación)** | El propio mod: `Help.txt`, `features.ini`, `limits.ini`, `ChangesEN.txt` | Checklist de features y límites del motor (`RecoilWhenFiring=1`, `EnableSwimming=1`, `NUMPEDS=140`, `NUMVEHICLES=130`…). Antes de inventar, se mira aquí. |
| **Controles modernos (la spec de "apuntar andando")** | Hilo *"Classic Axis for GTA III and VC"* (gtaforums) | Es la referencia de qué se espera del ratón (M16/Ruger apuntando en 1ª persona, apuntar moviéndose). Cerrado, pero es la especificación. |

Regla que sale de esto: **antes de implementar a mano, (1) el binario/INI del mod, (2) SilentPatchVC
para bugs del motor, (3) `gta-reversed` para mecánicas de SA, (4) el blog para este motor.**

## 7. Pendiente de la sección 1 (para que quede escrito)

- [x] Logs por sesión con fecha (`gta_vc_browser/logs/`, no se borran).
- [x] R2 emisora (`web/ondemand.js`: sin `await`, precarga retenida, `ODSTA`).
- [~] **R3/R3b textos: CORREGIDO el diagnostico.** La traza de encolado
  (`SCRTXT`, 4 funciones) solo cazo en esa partida avisos de truco
  (`canal=help`, `time=0`) y `^ELIMINADO!` (`canal=big`), pero el jugador aclaro
  que los suyos **salen ABAJO, con fuente blanca tipo mision, unos pocos
  fotogramas**, y **no** son de trucos. O sea: vienen por otro camino (las
  variantes con numero/cadena, el *brief*, o un `CFont::PrintString` directo).
  **Hecho:** caza-todo en `CFont::PrintString` (`Font.cpp`, la mitad inferior de
  la pantalla) que registra el texto literal: `SCRTXT3 x= y= texto="..."`, con
  dedupe por texto + banda de 16 px y 2 lineas/s de tope. El proximo log nombra
  al culpable y **no se toca ninguna duracion hasta saber cual es** (bloque `R3b`
  del verificador).
- [x] D8 audio (los 13 sonidos del mod suenan; falta afinar la tabla con su oído).
- [x] R5b: el conmutador de 1ª persona (46 pulsaciones y `tog=0` ni una vez).
- [x] **L2a**: retícula de las armas nuevas. La condicion del HUD era un rango que
  acaba en `WEAPONTYPE_RUGER` y las del mod van **al final** del enum (48-55):\
  quedaban sin `DrawCrossHairPC` y por tanto sin mira. Añadido el rango
  nuevo (`Hud.cpp`).
- [x] **L2b**: traza `HUDICON` (una linea por cambio de arma) con tipo, modelo,
  ranura de TXD, si esta cargado y si la textura existe: el icono del
  lanzagranadas se caza con datos (su `gr_launch.txd` **si** trae la textura
  `gr_launch`, asi que habra que ver que camino toma el HUD).
- [x] **L6 (21/09)**: pantalla de carga de partida. **Una** portada (`splash1`)
  y **una** barra progresiva, sin reinicios. Causas (leídas en el código, no
  supuestas): (1) `LoadingScreen()` dibuja su propia barra con
  `NumberOfChunksLoaded/TOTALNUMCHUNKS`, que arranca en 0 en cada llamada, y el
  init llama ahí muchas veces → segunda barra que parece reiniciar la primera;
  (2) `LoadSplash(nombre)` **recarga el TXD** cuando el nombre no es el cargado,
  y las pantallas de carga vanilla piden portada aleatoria
  (`GetRandomSplashScreen()` → `loadscN`, justo "la portada del juego" que se
  veía); (3) cada fase reprograma `gWebLoadFrac` desde abajo (0.01 → 0.02 →
  0.04...) y `WebDrawLoadScreen` lo pintaba tal cual → la barra retrocedía en
  cada frontera de fase. Hecho: `WebBeginLoadScreen()`/`WebEndLoadScreen()`
  (`gWebLoadScreenActive`), `LoadSplash` ignora otro nombre mientras dura,
  `LoadingScreen` delega en `WebDrawLoadScreen`, y ésta es **monótona** y no
  re-entra. Trazas `LOADSCR begin/end/clamp`. Bloque **L6** del verificador y
  marca en `check-served-build.sh`. **Enlazado en `ve21`**.
- [x] **L1**: `tools/viceext-log-check.py` ahora dictamina tambien **R5** (1ª
  persona), **R9** (apuntado por arma: `desv` y `peso`), **R11** (dummies del
  coche de policia), **L2** (iconos del HUD) y **R3b** (textos de abajo).
  Probado con el log de esta partida: R5 FALLO (46 pulsaciones, 0 resultados =
  el arreglo no estaba en el motor servido), R9 FALLO (`peso=0` en 26 y 55),
  R11 FALLO (`dummies=0`), L2/R3b SIN DATOS (traza nueva, aun sin enlazar).
- [ ] Bump de la patada de cámara del retroceso **si sigue sin notarse** una vez
  arreglado el apuntado (hoy es invisible porque la retícula no se dibuja).

## 10. Estado del reparto (21/09 noche)

- **Parte B (H1-H5 + §6): hecha y revisada.** H1 (cámara del agachado −0,55 +
  `camz=`), H2 (nado: margen 0,25, velocidades en m/frame y cámara a la
  superficie), H3 (snap del rumbo con ratón + `peso` real del CROUCHFIRE), §6
  (luces de servicio: no cachear el fallo + 6 nombres; `FindDummyFrame` ya era
  recursivo, verificado en fuente). H4/H5 quedaron resueltos en la sección 1
  (retícula de 48-55 y traza `HUDICON`).
- **Enlace final hecho**: `ve22`, `check-served-build.sh` = TODAS las marcas.
- **Lo que se juzga ahora en la partida** (automático con
  `tools/viceext-log-check.py`): `H` (agachado `camz` / nado `avance`), `R9`
  (`AIMDIR desv<5` y `peso>0`), `R11` (`SVLIGHTS dummies>=1`), `L6` (una sola
  secuencia de carga), `R5` (`1p key` → `tog=1`), `L2` (`HUDICON textura=1`),
  `R3b` (textos de abajo: nombrar los que salen solos) y `D8` (armas del mod que
  disparan y suenan).
