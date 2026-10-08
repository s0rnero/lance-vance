---
name: agachado-sa
status: EXECUTED
type: feature
domain: player-controls-aim-crouch
owner_rules: .agents
created: 2026-09-22
---

# Agachado como en SA (plan de cambios, SIN tocar código)

Fecha: 22/09/2026 · build medida: `ve37` · encargo: «el agachado no se mueve en sus
8 direcciones, la velocidad va demasiado lenta y apuntar agachado debe funcionar
mejor; usa la lógica que YA existe (la del mod / la de GTA SA), no adivines».

Referencias usadas (todas dentro del repo, nada de memoria):

- `vice-extended-.../GameFiles/ViceExtended/anim/ped.ifp` → **los clips que trae
  el mod** (`python gta_vc_browser/tools/ifp_inspect.py --curva`).
- `vice-extended-.../GameFiles/ViceEx.exe` → el **ejecutable del mod** (ahí está
  su lógica de agachado; contiene su tabla de nombres de animación).
- `gta_vc_browser/pc.mp4` (198 s, 60 fps) y `browser.mp4` (129 s, 30 fps) →
  comparación visual de los dos, fotograma a fotograma.

---

## 1. Qué se ve hoy (medido, no opinado)

Medido con el arnés (`node tools/vc-test.mjs --escenario agachado`) y con la traza
`CROUCH2` de las partidas del jugador:

| | Valor medido |
|---|---|
| Avance agachado andando | **0,87 m/s** (objetivo del código: 0,9) |
| Cadencia | **1 ciclo cada 2,92 s** (≈ 2 pasos en 3 s = paso lento de película) |
| Clip que se pide | sólo `Crouch_Forward` / `Crouch_Backward` / `Crouch_Idle` |
| Direcciones | **el clip se elige por el SIGNO de arriba/abajo**: con izquierda, derecha o diagonal (arriba/abajo = 0) se pide el clip de **adelante** |
| Apuntar agachado | pose de disparo del grupo del arma, sin `WEAPON_crouch` |
| Cámara | modo 4 (sigue al ped) y baja 0,95 m — esto ya está bien |

Contraste con la referencia: **el andar de pie del motor mide 0,79-1,13 m/s**
(clips `WALK_player` 1,205 m/1,533 s y `WALK_civi` 1,321 m/1,167 s = lo que da el
`PEDAT velo` de las partidas del jugador), y el correr 6,39 m/s (`run_civi`). O
sea: el agachado va *por debajo del andar* **y encima con el paso a 1/4 de
velocidad**, que es lo que se ve como «lento y raro».

## 2. Por qué pasa (motor, con fichero y línea)

1. `CPlayerPed::SetRealMoveAnim` (`src/peds/PlayerPed.cpp:470`) **corta en seco
   cuando `odCrouched`** y llama a `ViceExtCrouchAnim()`; el selector de serie
   (caminar/correr/atrás/... por ángulo) no corre. El único clip que se mezcla es
   el que elige nuestro código.
2. `ViceExtCrouchAnim` (`PlayerPed.cpp:3019`) decide así:
   `padMove > 0,25 → (upDown < 0 ? CROUCH_BACKWARD : CROUCH_FORWARD)`, y si no,
   `CROUCH_IDLE`. **`leftRight` no se mira nunca.**
3. En este motor el ped se mueve con el **avance de la raíz del clip**
   (`m_vecAnimMoveDelta` → `CPed::CalculateNewVelocity` → `UpdatePosition`), así
   que *rate y velocidad son lo mismo*: nuestro `odAssoc->speed = objetivo/avance`
   con objetivo 0,9 sobre un clip que avanza **3,58 m/s** da el rate **0,25** →
   0,87 m/s y, con el mismo número, **el paso a 1/4 de velocidad**.
4. `Crouch_Roll_L/R` (`Crouch_Roll_L` = −2,17 m/0,93 s) y `WEAPON_crouch`
   (0,50 s, pose) **no están ni registrados**: no existen `ANIM_STD_CROUCH_LEFT/
   RIGHT/AIM` en `src/animation/AnimationId.h` (sólo IDLE/FORWARD/BACKWARD, al
   final de la lista). Por eso «no se mueve en 8 direcciones»: le faltan los
   clips laterales y la elección por ángulo.
5. El agachado de DISPARO del motor (`SetDuck`/`bIsDucking`, clips
   `DUCK_down`/`DUCK_low`) se dejó fuera con `VICEEXT_ENGINE_DUCK_KEY() false`
   (`PlayerPed.cpp:59`), pero sigue habiendo caminos que lo encienden (morir,
   trucos, IA) y ese agachado **clava al jugador en el sitio** (fallo de la 12ª
   partida).

## 3. La referencia: lo que trae el mod (su propio `ped.ifp`)

`272` animaciones; las del sistema de agachado y de apuntado son:

| clip | duración | avance de la raíz | qué es |
|---|---|---|---|
| `Crouch_Idle` | 0,800 s | 0,000 m | agachado quieto |
| `Crouch_Forward` | 0,731 s | **+2,615 m (3,58 m/s)** | avanzar agachado |
| `Crouch_Backward` | 1,000 s | **−1,853 m (−1,85 m/s)** | retroceder agachado |
| `Crouch_Roll_L` | 0,931 s | **−2,172 m (2,33 m/s)** | desplazarse a la IZQUIERDA |
| `Crouch_Roll_R` | 0,931 s | **+2,253 m (2,42 m/s)** | desplazarse a la DERECHA |
| `WEAPON_crouch` | 0,500 s | 0,000 m | **pose de apuntar agachado** |
| `Gun_stand` | 0,031 s | — | stance de apuntar de pie |
| `GunMove_FWD/BWD/L/R` | 1,00 s | 1,85 / −1,85 / ∓1,80 m | caminar apuntando (el mod lo llama “Movement while aiming”) |
| `DUCK_down` / `DUCK_low` | 0,567 s | −0,301 m | agacharse del motor de serie |

Los nombres son **los de GTA San Andreas** (`Crouch_Forward`, `Crouch_Roll_L/R`,
`Weapon_crouch`, `GunMove_*`: son los del `ped.ifp` de SA, y el mod los metió tal
cual); el `ped.ifp` servido en el navegador es **byte a byte el del mod**
(md5 `c61a251f…` en `streamed/anim/ped.ifp` y en el paquete del mod).

Y el ejecutable del mod confirma el sistema completo: `ViceEx.exe` lleva la tabla
de nombres en `0x2d7a40` — `… CLIMB_Stand_finish, Crouch_Idle, Crouch_Forward,
Crouch_Backward, Crouch_Roll_L, Crouch_Roll_R, Swim_Breast, Swim_Crawl,
Swim_jumpout, Swim_Tread, GunMove_BWD, GunMove_FWD, GunMove_L, GunMove_R,
Gun_stand, CAR_sit_DB …` —, o sea que el mod **espera y usa los cinco clips de
agachado** (los mismos que nuestro port ignora en parte).

Conclusión de bulto: **el sistema de agachado del mod ya está hecho y es el de SA
(4 clips direccionales + pose de apuntar); lo que falta es nuestro lado**, que
sólo usa 2 de los 5 clips y los elige por un dato que no es la dirección.

## 4. El sistema de SA que hay que copiar

1. **Agachado es un estado**, no una animación suelta: quieto (`Crouch_Idle`),
   avanzando, retrocediendo y desplazándose de lado, más una **pose de apuntar**
   (`WEAPON_crouch`) que se mezcla por encima.
2. **Las 8 direcciones no llevan 8 clips**: al moverse **sin apuntar** el ped
   **gira hacia donde anda** (como de pie) y el clip es el de avanzar; da igual si
   la dirección es diagonal, izquierda o atrás → lo que se ve es Tommy girando y
   avanzando agachado. Los clips de lateral se usan cuando **no** puede girar.
3. **Apuntando agachado** el ped NO gira: mantiene el encaramiento de la cámara y
   se desplaza con el clip de su dirección (adelante/atrás/lados) → apuntar y
   moverse a la vez (lo que el jugador llama «apuntar desde sentado»).
4. **Una sola velocidad** de agachado para todas las direcciones; la cadencia del
   clip se escala para que los pies acompañen (velocidad y cadencia se fijan por
   separado, no una de la otra).

## 5. Qué se ve en los dos vídeos (comparación)

- **`pc.mp4` (el mod, el objetivo).** Trompa agachado en `t≈63-68 s`: **postura
  muy baja** (rodillas dobladas, cuerpo inclinado), avanza **a paso vivo**
  (claramente más que el paseo del propio vídeo en `t≈16-21 s`, que va a 0,79 m/s)
  y con **cadencia de paso normal**; además en `t≈64-67 s` se desplaza **de lado
  sin girar** (y en `t≈70-75 s` apunta y dispara agachado con la pistola en una
  postura baja y estable, `t≈78 s` cambia al visor del rifle). No hay ningún
  momento en que el personaje se quede clavado ni en que dé pasos «a cámara
  lenta».
- **`browser.mp4` (el port).** En las ventanas equivalentes el agachado se ve
  (la pose es la del mod) pero: **el paso va a 1/4 de velocidad** (se nota en las
  piernas), el avance es **más lento que andar de pie** y los desplazamientos
  laterales/diagonales **no reproducen el clip lateral** (sigue el de adelante).
  Es exactamente lo que reporta el jugador.

## 6. Cambios propuestos (por tandas)

### Tanda 1 — dirección: las 8 direcciones y los clips que faltan

- `src/animation/AnimationId.h`: añadir al final (sin desplazar IDs existentes,
  como se hizo con nado/agachado/escalada): `ANIM_STD_CROUCH_LEFT` (`Crouch_Roll_L`),
  `ANIM_STD_CROUCH_RIGHT` (`Crouch_Roll_R`) y `ANIM_STD_CROUCH_AIM` (`WEAPON_crouch`).
- `src/animation/AnimManager.cpp` — el grupo `playercrouch` YA existe con sus
  propias listas (`aCrouchAnimations[]` + `aCrouchAnimDescs[]`, líneas 971-986,
  declarado en la tabla de grupos como `{ "playercrouch", "ped", MI_COP, … }`),
  así que **no se toca ninguna lista de serie**: se añaden los tres nombres a
  `aCrouchAnimations` y sus tres descs a `aCrouchAnimDescs` **en el mismo orden
  entre sí** (el motor empareja nombre↔desc por posición dentro del grupo):
  - `Crouch_Roll_L` / `Crouch_Roll_R` como
    `ASSOC_REPEAT | ASSOC_MOVEMENT | ASSOC_HAS_TRANSLATION | ASSOC_WALK` (igual
    que FORWARD/BACKWARD: así su raíz también empuja el cuerpo y suenan pasos);
  - `WEAPON_crouch` como `ASSOC_REPEAT` (pose, sin movimiento).
  (Los nombres del `ped.ifp` son `Crouch_*`/`WEAPON_crouch` y en nuestra lista
  van en minúscula, como los demás: el motor busca por nombre sin distinguir
  mayúsculas — funciona igual con `crouch_forward`.)
- `src/peds/PlayerPed.cpp` (`ViceExtCrouchAnim`, hoy `PlayerPed.cpp:3019`):
  elegir el clip **por ÁNGULO** (dirección pedida en el sistema del ped menos su
  encaramiento), no por el signo de `upDown`:
  - ±50° → `CROUCH_FORWARD`; 50°-130° → `CROUCH_LEFT/RIGHT`; >130° → `CROUCH_BACKWARD`;
  - **sin apuntar**, el ped gira hacia donde anda (ya lo hace el motor con
    `m_fRotationDest`) y el clip es el de avanzar aunque la dirección sea diagonal
    → eso es «moverse en sus 8 direcciones como caminando normal»;
  - **apuntando**, no se gira y manda el clip de la dirección.
- `SetRealMoveAnim` (`PlayerPed.cpp:470`): sustituir el corte «un solo clip» por
  «clip del ángulo» (misma idea: que el selector de serie no pise el clip, pero
  eligiendo entre los cinco).### Tanda 2 — velocidad y cadencia (decisión §7: los clips del mod, ritmo 1)

- `src/core/config.h`: se retira el objetivo de velocidad (`VICEEXT_CROUCH_SPEED`
  ya no manda) y queda **una sola palanca**: `VICEEXT_CROUCH_RATE` (por defecto
  `1.0f`) más la **banda de cadencia** (`VICEEXT_CROUCH_CAD_MIN/MAX` = 0,8-1,5
  ciclos/s) que sirve de red: si un clip se queda fuera de banda, se avisa en la
  traza en vez de cambiar la velocidad a escondidas.
- `src/peds/PlayerPed.cpp`: los defines de hoy (`VICEEXT_CROUCH_WALK_SPEED 0.9`,
  `VICEEXT_CROUCH_FWD_ROOT_M`, `_BACK_ROOT_M`, `VICEEXT_CROUCH_SPEED 0.9`) pasan a
  ser **una tabla por clip** con el avance REAL medido del `ped.ifp` servido
  (2,615 / 1,853 / 2,172 / 2,253) que sólo se usa para (a) informar de los m/s en
  la traza y (b) aplicar `VICEEXT_CROUCH_RATE`; **se elimina el recorte a 1,0** y,
  con él, el bucle objetivo/avance. `ViceExtCrouchLimitSpeed` deja de recortar a
  0,9 (sólo recorta lo que pida el mando por encima de lo que da el clip).
- Aviso de ingeniería: en un clip con `ASSOC_MOVEMENT` el motor **no usa
  `assoc->speed`** (`CAnimBlendAssociation::UpdateTimeStep` usa `relSpeed`), así
  que el rate tiene que aplicarse donde el motor lo lea de verdad; por eso la
  primera sub-tanda es **instrumentar y medir** (`CROUCH2 avance/velo` ya existe)
  antes de fijar el número, que es la lección de las últimas sesiones (la sonda
  mintió dos veces).

### Tanda 3 — apuntar agachado

- `PlayerPed.cpp`: cuando `odCrouched` y se apunta (`padUsed->GetTarget()` /
  `m_pPointGunAt`):
  - **pose**: `ANIM_STD_CROUCH_AIM` (`WEAPON_crouch`) mezclada por encima, en vez
    de la pose de disparo de pie del grupo del arma;
  - **movimiento**: encaramiento = cámara (`m_fRotationDest = m_fRotationCur =
    TheCamera.Orientation`, como ya hace H3 para el aim-walk de pie) y clip por
    dirección → **apuntar y desplazarse de lado, como en SA**;
  - **velocidad**: la del apuntado (el `weapon.dat` del mod ya trae los flags;
    usar el mismo tope que el aim-walk de pie, que ya funciona: R9).
- `src/core/Cam.cpp`: revisar que agachado + apuntando la cámara no baje dos veces
  (`VICEEXT_CROUCH_CAM_DROP 0.95` + la de apuntar) — comparar con `pc.mp4`.

### Tanda 4 — bordes y choques con el motor

- `SetDuck`/`bIsDucking` (el agachado de disparo del motor): repasar los caminos
  que lo encienden (morir, trucos, IA, `PedAI.cpp`) para que no deje al jugador
  clavado.
- Cancelar el agachado en: entrar a un coche, morir, caer al agua, interior;
  y límite de techo (agachado no se atraviesa un techo bajo).

## 7. La velocidad: qué es dato y qué era deducción mía (y decisión tomada)

**DATO (medido en el `ped.ifp` servido, `ifp_inspect.py --curva`):** el avance de
la raíz de cada clip por ciclo. `Crouch_Forward` +2,615 m en 0,731 s (3,58 m/s),
`Crouch_Backward` −1,853 m en 1,000 s, `Crouch_Roll_L` −2,172 m y
`Crouch_Roll_R` +2,253 m en 0,931 s. **Ya es asimétrico** (adelante avanza ~2×
más que atrás): la asimetría la trae el mod.

**DATO:** en este motor una animación de movimiento se reproduce a ritmo 1 y el
avance de su raíz **es** la velocidad (§2.3). Andar de pie = 0,79-1,13 m/s; correr
= 6,39 m/s.

**NO MEDIDO — y hay que decirlo:** el *ritmo* que el ejecutable del mod aplica a
estos clips. De `ViceEx.exe` se pudo leer su tabla de nombres (los cinco clips de
agachado, orden de SA) pero la constante está en código y no se puede sacar sin
desensamblar el ejecutable. **La «velocidad única de 1,6 m/s para las 4
direcciones» era una deducción mía** (lo que hace SA) para que la cadencia
quedara natural: no es un dato del mod y queda descartada como base.

**DECISIÓN (jugador, 22/09): «como está en el mod»** ⇒ se aplica la regla del
propio motor con los clips del mod tal cual, **ritmo 1**:

| dirección | clip | velocidad | cadencia |
|---|---|---|---|
| adelante | `Crouch_Forward` | **3,58 m/s** | 1,37 ciclos/s |
| atrás | `Crouch_Backward` | **1,85 m/s** | 1,00 ciclos/s |
| izquierda | `Crouch_Roll_L` | **2,33 m/s** | 1,07 ciclos/s |
| derecha | `Crouch_Roll_R` | **2,42 m/s** | 1,07 ciclos/s |

Es decir: se quita de en medio el recorte de hoy (objetivo 0,9 y `rate` 0,25) y se
deja que cada clip avance como el motor sabe hacerlo. La cadencia normal (1,0-1,4
ciclos/s) es justo lo que se ve en `pc.mp4` (paso vivo, nada de cámara lenta).

Queda **una sola palanca** por si la prueba lo pide: `VICEEXT_CROUCH_RATE` (0,5 =
mitad de todo, sin descuadrar los pies). Se implementa con el valor 1,0 y, si hace
falta, se baja con la comparación contra `pc.mp4`; el escenario del arnés imprime
los m/s de cada dirección para poder decidirlo con números y no a ojo.

## 8. Cómo se verifica (escenario del arnés, ampliado)

`node tools/vc-test.mjs --escenario agachado` (hoy 11 comprobaciones) pasa a:

1. **8 direcciones**: pulsar N/S/E/O y las 4 diagonales 1,5 s cada una y exigir
   `avance > 0` en las 8, el `clip` esperado por ángulo y —sin apuntar— el
   encaramiento apuntando a la dirección.
2. **Velocidad**: `velo` mediana por dirección ≈ la de su clip (3,58 / 1,85 /
   2,33 / 2,42 m/s) × `VICEEXT_CROUCH_RATE`, con tolerancia ±15 %; y **cadencia**
   dentro de la banda 0,8-1,5 ciclos/s (hoy: 0,34 ciclos/s = el fallo).
3. **Cámara**: modo 4, `camdist ≤ 8` y la altura sobre el ped en las 8 direcciones
   (ya se mide `camz−h`).
4. **Apuntar agachado**: peso de `WEAPON_crouch` > 0,5, `AIMDIR peso ≈ 1`, y que
   el ped **no gire** al desplazarse de lado.
5. **No dejar restos**: de pie ningún clip de agachado con peso > 0; al
   desagacharse se retiran (ya está).

Criterio de cierre: escenario en PASS con esos números en la traza y comparación
visual con `pc.mp4`.

## 9. Lo que NO se toca

- El `ped.ifp` del mod ni sus datos (`weapon.dat`, `main.scm`): son la referencia.
- El sistema de nado, apuntado de pie, 1ª persona y cámara general: ya funcionan;
  sólo se toca lo que el agachado necesita de ellos (encaramiento y tope de
  velocidad al apuntar).
- El estado de agachado del motor (`SetDuck`) no se reutiliza: el del mod es
  nuestro, como hasta ahora.

---

## 10. ESTADO: implementado y medido (22/09, build `ve41`)

Este documento era el plan. Lo que se hizo, con los números que lo cierran:

### 10.1 Los clips que faltaban, y una trampa del formato

Se registraron en el grupo `ASSOCGRP_PLAYERCROUCH` los dos clips de costado que el
`ped.ifp` del mod ya traía (`Crouch_Roll_L`, `Crouch_Roll_R`). Antes hubo que
**mover el bloque de agachado al final del `enum AnimationId`**: el índice dentro
de un grupo es `id − firstAnimId` y `firstAnimId` es el `animId` del PRIMER desc
(`CAnimManager::CreateAnimAssocGroups`), así que los IDs de un grupo tienen que ser
consecutivos — con la escalada en medio, el cuarto clip de agachado habría
apuntado a un clip de escalada.

### 10.2 Y la trampa de fondo: esos clips NO son de costado

Medido con `tools/ifp_inspect.py` (metros que avanza la raíz por ciclo, ejes
locales del clip; `walk_left` de serie como referencia: −1,836 en X y 0,03 en Y):

| clip | dx | dy | significado |
|---|---|---|---|
| `Crouch_Forward` | −0,002 | **+2,615** | adelante (3,58 m/s) |
| `Crouch_Backward` | 0,000 | **−1,853** | atrás (1,85 m/s) |
| `Crouch_Roll_L` | −0,219 | **−2,172** | **hacia ATRÁS** |
| `Crouch_Roll_R` | −0,244 | **+2,253** | **hacia ADELANTE** |

Y se confirmó en el motor: con `Crouch_Roll_L` puesto, pulsar izquierda movía al
ped **179,6° respecto a su rumbo** (o sea hacia atrás). Son clips de revolcarse, no
un paso lateral. O sea: el `ped.ifp` del mod **no trae ningún clip de costado
agachado**, y por eso «elegir clip por ángulo» (la versión A del plan) no podía
funcionar sola.

### 10.3 Cómo queda el movimiento (modelo de SA)

En SA la velocidad de un ped la pone el **código** y el clip sólo pone la pose; en
III/VC la pone la **raíz del clip**, y el motor sólo sustituye eso por el rumbo del
mando cuando `|localWalkAngle| < 50°` — de ahí que las diagonales ya fueran bien y
los lados no. Ahora, agachado, se aplica esa misma sustitución **en las 8
direcciones** (`CPed::CalculateNewVelocity`, bloque R20c), con la velocidad del
clip de la pose que toque. Resultado: dirección correcta (la del mando) y cadencia
del clip.

- Pose: `Crouch_Forward` adelante y lados, `Crouch_Backward` atrás (|ángulo| > 130°).
- `Crouch_Roll_L/R` quedan tras la palanca `?crouchlado=1`
  (`window.__vcCrouchSideClip`): su raíz viaja hacia atrás, así que la pose no está
  validada en partida.
- El cuerpo **no gira** con ratón en 3ª persona: se desplaza de lado, igual que al
  caminar de pie (ahí son `walk_left`/`walk_right`; agachado no existen y el
  desplazamiento lo pone el código).

### 10.4 Velocidad: la del clip del mod, y una palanca

Fuera el objetivo inventado de 0,9 m/s (que obligaba a bajar el ritmo a 0,25 y
dejaba los pies a un cuarto de velocidad). Los clips van a ritmo del motor (1,0):
**adelante 3,58 m/s, atrás 1,85, de lado 2,33/2,42**. `?crouch=X` o
`window.__vcCrouchRate = X` mueve avance y cadencia juntos, **en caliente**, sin
recompilar (el motor lo relee una vez por segundo).

### 10.5 Medición de cierre (`ve41`, `--escenario agachado`: PASS 23/23)

| dirección | clip | ángulo mando | desvío medido | esperado |
|---|---|---|---|---|
| adelante | `Crouch_Forward` | 0° | −0,44° | 0° |
| atrás | `Crouch_Backward` | −180° | −179,7° | 180° |
| izquierda | `Crouch_Forward` | 90° | −89,9° | −90° |
| derecha | `Crouch_Forward` | −90° | +86,5° | +90° |
| diag. adelante-izq | `Crouch_Forward` | 45° | −44,8° | −45° |
| diag. adelante-der | `Crouch_Forward` | −45° | +44,5° | +45° |
| diag. atrás-izq | `Crouch_Backward` | 135° | −132,8° | −135° |
| diag. atrás-der | `Crouch_Backward` | −135° | +134,4° | +135° |

Velocidades en régimen: 3,71 adelante (clip 3,58), 1,79-1,82 atrás (clip 1,85),
3,71 de lado (clip 3,58). «Desvío» = rumbo del desplazamiento medido con `PEDAT`
menos el del cuerpo (`rumbo`), con la convención del motor (adelante = −rumbo).

### 10.6 Pendiente

- **Apuntar agachado** (`WEAPON_crouch` como pose, sin girar): sin hacer. Al no
  haber clip lateral, la pose de costado apuntando queda a deber; el desplazamiento
  ya va bien.
- **Pose de costado**: decidir entre `Crouch_Forward` (por defecto) y
  `Crouch_Roll_L/R` (`?crouchlado=1`) mirándolo en partida.
