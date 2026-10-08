---
name: 10-seccion-1-9a-partida
status: EXECUTED
type: feature
domain: gameplay
owner_rules: .agents
created: 2026-09-21
---

# Sección 1 · 9ª partida (21/09/2026, tarde) — CAUSA RAÍZ de la lista de fallos

Build: **ve23** (enlazado 22:05). Datos: `dataTag ve13`.

## La causa raíz: `bFreeCam` cortaba el control con ratón

`CPed::CanStrafeOrMouseControl()` (`src/peds/Ped.cpp`) empezaba con:

```cpp
#ifdef FREE_CAM
    if (CCamera::bFreeCam) return false;
#endif
```

En este port `bFreeCam` **está encendido** y no significa "cámara libre de
depuración": significa *cámara moderna* (la de ratón a pie, y la de coche tipo SA
`Process_FollowCar_SA`). La prueba está en el log de la partida:
`1p set ... free=1` (11 de 11 pulsaciones) y `CAM1P ... mouse3d=1`.

Con `false`, se caían los tres sitios que **encaran al jugador con la cámara**:

| Sitio | Qué dejaba de pasar |
|---|---|
| `Cam.cpp:1621` (`Process_FollowPedWithMouse`) | `m_fRotationCur/Dest = Front.Heading()` |
| `Cam.cpp:4009` | lo mismo, segunda puerta |
| `PlayerPed::ProcessAnimGroups` | grupos de strafe PLAYERLEFT/RIGHT |

Consecuencias medidas en el log (no interpretadas):

- `AIMDIR desv` hasta **86,2°** con las armas del mod (54 steyr) y **154,8°** con
  la 50: el cuerpo no miraba donde la cámara → "el arma apunta a un costado".
- El pase a pie con ratón (Zelda/strafing, `Ped.cpp:1452`) no corría: de ahí el
  "va raro" agachado y el nado que no va hacia donde se mira.
- La 1ª persona quedaba con `puerta=0` por el mismo `bFreeCam` en su condición.

## Arreglos aplicados (ve23)

1. **`CanStrafeOrMouseControl`**: sólo corta si `bFreeCam && !m_bUseMouse3rdPerson`
   (ahí sí es la cámara libre de verdad).
2. **Conmutador de 1ª persona** (`Camera.cpp`): la puerta ya no exige
   `!bFreeCam` cuando hay cámara de ratón (`odGate1p`).
3. **Apuntado** (`PlayerPed.cpp`): el bloque de rotación de apuntado ya no exige
   `bFreeCam`; manda `m_bUseMouse3rdPerson`.
4. **Recarga a mano con ANIMACIÓN** (`ViceExtTryManualReload`): el motor sólo pone
   el clip dentro del anim de ataque, y recargando quieto ese bloque no corre; se
   mezcla aquí el mismo clip (`reload`/`crouchreload` del grupo del arma). Traza
   `VICEEXT reload anim grupo= clip=`.
5. **Agachado conmutable** (`ViceExtCrouchControl`): la C se leía dos veces por
   pulsación (pad + acción rebindable, con relojes distintos) → `off` + `on` en
   frames seguidos; el log lo enseña como pares. Antirrebote de 250 ms y traza
   `repetido=`.
6. **Autocentrado de coche** (`Cam.cpp`): dos caminos.
   - pasivo: 0,55 rad/s (~31°/s), sólo avanzando (≥1 m/s), tras 2,5 s sin mirar,
     con zona muerta de 6° → deja ver cómo el coche se ladea.
   - pedido: al **soltar** una tecla de mirar (cruceta/palo/mirar atrás) se da un
     empujón de 2,2 rad/s durante 0,5 s → "debería centrarla igual" en la moto.
   Traza `camauto2 ... pedido= avanza= vel=`.
7. **Lanzagranadas** (`ViceExtAimWithArm`): era el único arma del mod con CANAIM
   sin `CANAIM_WITHARM` (`weapon.dat`, flags `28040`) → sin pose ni retícula.

## Qué mirar en el próximo log

```bash
cd gta_vc_browser
bash tools/check-served-build.sh
python tools/viceext-log-check.py logs/odtrace-*.log
```

- `AIMDIR desv<5` con 54/50/55 (antes 86°).
- `1p set ... puerta=1 ... tog=1` y `CAM1P fp=1` con la V.
- `VICEEXT reload start` seguido de `reload anim ... clip=<no 0>`.
- `VICEEXT crouch off` sin `on` inmediato (antirrebote) y `repetido=1` cuando la
  pulsación se colaba.
- `camauto2 auto=1 pedido=0` mientras conduce recto y `pedido=1` justo después de
  soltar la cruceta.
- `SVLIGHTS model=156 dummies=4` (sigue pendiente, ver abajo).

## RESUELTO (10ª partida): la sirena de las patrullas — los dummies cuelgan de los extras

> Corrección de mi propio diagnóstico anterior. Decía que "fallan TODAS las
> búsquedas por nombre (`petrolcap` incluido)". **Era falso**: en la misma traza
> `gastank hit model=156 explota=1 reserva=0` significa que el dummy `petrolcap`
> **sí** se encontró (`reserva=0` ⇒ por dummy, no por caja). Las líneas
> `gastank reserva model=132/197/207/210/212` son modelos que de verdad no traen
> el dummy (documentado en `Weapon.cpp`: sólo ~25 de ~100 `.dff` lo traen, y el
> respaldo por caja está puesto a propósito). O sea: `FindDummyFrame` **funciona**;
> el problema era otro.

**Causa raíz (parser del `.dff` servido, no adivinada).** Parseando
`streamed/models/gta3.img/police.dff` entero (chunks RW: frame list, struct de 56 B
por frame con `parent`, y plugin NodeName `0x0253F2FE`):

```
servicelights_1  <- extra1 <- chassis_dummy <- police
servicelights_2  <- extra2 <- chassis_dummy <- police
servicelights_3  <- extra3 <- chassis_dummy <- police
petrolcap        <- police            (por eso sí se encontraba)
```

`extra1..6` son **componentes**: `CVehicleModelInfo::PreprocessHierarchy` los saca
del clump del modelo (`RwFrameRemoveChild`) y guarda sólo su **atomic** en
`m_comps`; `CreateInstance` clonaba **únicamente ese atomic** sobre un marco nuevo.
Resultado: los hijos (`servicelights_N`, que son las lentes de la barra, con su
propia malla) nunca llegaban a la instancia →

- la barra salía **sin luces** (lo que el jugador ve como "la sirena desapareció,
  sólo se ven colores"), y
- `SVLIGHTS dummies=0` **en todas las partidas** (no había nada que encontrar).

Datos de apoyo: cada variante es un juego completo (housing + lentes), se elige
**una al azar** por coche (`default.ide:208` → `comprules=0`; con `-2` el motor
elige con `GetRandomNumberInRange(0,3) < 2`, así que 1/3 de patrullas no lleva
ninguna barra — comportamiento de serie, no un fallo). Y el escaneo de los
**4.640 `.dff`** servidos dice que **sólo el `police.dff`** tiene componentes con
hijos: el arreglo toca exactamente ese coche.

**Arreglo (build `ve24`).**

1. `src/modelinfo/VehicleModelInfo.cpp`: `ViceExtCloneComponent` clona el
   **subárbol completo** del marco del componente (mismo patrón que
   `RecurseFrameChildrenToCloneCB` de `PedFight.cpp`), y copia el **nombre** del
   marco a mano (`RwFrameCreate` no lo trae; sin eso los clones se llamarían `""`
   y la búsqueda seguiría fallando).
2. `src/vehicles/Automobile.cpp`: las coronas de la sirena **ya no se anclan al
   dummy** (los marcos `servicelights_*`/`extra*` están en `(0,0,0)`: la malla se
   coloca por vértices) sino al **centro de la esfera envolvente** del atomic de
   las luces ± su radio sobre el eje derecho del coche; la caché pasa a ser por
   **(modelo, extras elegidos)** (`m_aExtras`) porque cada patrulla lleva una
   variante distinta, y el radio se acota a 0,35–1,1 m.
3. Trazas: `SVLIGHTS ... var=<0..5> extras=<a>,<b> radio=<m>`; bloque nuevo **SR**
   (`SIRENA tipo= model= sample= freq= vol= dist2=`) en
   `src/audio/AudioLogic.cpp` para dictaminar la sirena **audible** (si no aparece
   ninguna línea con la policía cerca, el fallo está antes:
   `m_bSirenOrAlarm` nunca se enciende en las patrullas de la IA).

**Qué mirar en la próxima partida** (bloque R11 del verificador):

- `dummies=1 var=N extras=<comp>,-1` con barra **con luces** encima del techo y las
  coronas sobre la barra → arreglado.
- `dummies=0 ... extras=-1` → esa patrulla no lleva barra (aleatorio de serie).
- Con 3 estrellas, `SR` con líneas `SIRENA ... sample=...` → el sonido sale del
  motor; sin líneas, el problema es de `m_bSirenOrAlarm`/IA.

---

## 11ª partida (21/09 noche) · «vi la v25 y todo sigue igual»

Lo que dice el log de esa sesión (`logs/odtrace-2026-09-22_04-04-07.log`, `ve25`,
2 min de juego) y lo que **no** dice:

| Queja | Dato del log | Conclusión |
|---|---|---|
| «no hay primera persona» | **0 líneas** `VICEEXT 1p key`; `CAM1P ... v=0` en las dos muestras; el verificador: *R5 SIN DATOS: no se pulsó la tecla V* | la V **no llegó al motor** en toda la sesión. El sospechoso no es el juego sino la **config de controles guardada** en el navegador: si la acción tiene OTRA tecla, ni la V ni el respaldo la ven |
| «las armas apuntan al costado» | `AIMDIR` de las 5 armas nuevas: **`desv=0.0`** con `peso=1.00` (antes de R6 era `desv=73.6`) | el cuerpo SÍ encara la cámara y la pose existe. Lo que faltaba era saber **qué clip** resuelve cada arma |
| «las pesadas no tienen animación de apuntado» | `weapon.dat` del mod: `Gr_launch 0x28040`, `Steyr 0x28050`... **idénticos a los flags del `m4` de serie** (`canaim=1`, `witharm=0`) | los flags NO son la causa: son los mismos que el arma que el jugador dice que apunta bien |
| «no hay animación de recarga» | `VICEEXT reload key tecla=1056` (acción sin tecla) + `no motivo=sin-municion arma=0` | la R se pulsó **con los puños** (arma 0): la recarga con arma real sigue sin dato |
| «no se escucha sonido» | `D8 OK: 5 muestras del mod disparadas y todas suenan` | el motor encola y decodifica las muestras nuevas |

### Lo aplicado (R13, `ve26`)

1. `src/core/Camera.cpp`: **la 'V' se lee siempre** — por la acción rebindable **y**
   por la tecla cruda. Traza `VICEEXT 1p bind tecla=` (una por sesión): 86 = 'V',
   1056 = `rsNULL`.
2. `src/peds/PlayerPed.cpp`: traza **`WINFO`** (una por arma que el jugador saca) con
   `flags`, `canaim`, `witharm`, `reload`, `crouchr`, `sight` y los **nombres de clip**
   que el grupo resuelve (`clips=disparo|agachado|recarga|recarga-agachado`; `-` = el
   arma no tiene ese flag, `?` = el grupo no trae ese clip). `AIMDIR` añade `tgt=` y
   `ik=0x…` (flags del IK del ped, ahí está `GUN_POINTED_SUCCESSFULLY`).
3. `src/renderer/Hud.cpp`: `SIGHT` añade `por=` (1=conduciendo, 2=modo de cámara,
   3=apuntando): dictamina «la mira sale sin apuntar» sin depender de la vista.
4. Verificador: bloque **R13** (`tools/viceext-log-check.py`) y 4 marcas más en
   `tools/check-served-build.sh` (21/21 OK).

**Qué mirar en la próxima partida:**

- `VICEEXT 1p bind tecla=X` → si X no es 86 ni 1056, ése era el motivo de la V.
- `VICEEXT 1p set ... tog=1` + `CAM1P ... tog=1` → primera persona aplicada.
- `WINFO ... clips=STEYR_fire|STEYR_crouchfire|STEYR_reload|STEYR_crouchreload` →
  cada arma nueva dice su clip; un `?` es un clip que el `.ifp` servido no trae
  (comprobado: `streamed/models/gta3.img/steyr.ifp` trae los cuatro).
- `AIMDIR ... tgt=1 ik=0x…` → si apuntando sale `ik=0x0`, el arma no queda encarada
  y el problema es el IK, no los datos.

---

## 12ª partida (22/09) · vídeos `browser.mp4` + `pc.mp4` y cuatro causas raíz

El jugador dejó **dos grabaciones** en `gta_vc_browser/`: `pc.mp4` (el mod en PC
nativo, 294 s, la referencia de cómo *debe* verse) y `browser.mp4` (nuestro port,
162 s). Se han comparado fotograma a fotograma (muestreos de 0,4-4 fps con
`ffmpeg -vf crop,tile` hacia una hoja de contactos) y con el log de esa misma
sesión (`logs/odtrace-2026-09-22_14-35-49.log`, `ve26`).

### Calibración de los vídeos (para futuras comparaciones)

`browser.mp4` termina ~1 min antes que el log; el desfase medido es
**`t_vídeo = t_log − 14:36:20`**. Con eso se sabe qué se está viendo:

| t del vídeo | qué se ve | líneas del log |
|---|---|---|
| 7-46 s | agachado (varias pulsaciones de C) | `VICEEXT crouch on/off` (14:36:27-37:06) |
| 48-92 s | cambio de armas y apuntado (escopeta, AUG, Colt, Micro-UZI, Desert Eagle) | `WINFO`/`AIMDIR`/`SIGHT` (14:37:08-52) |
| 101-140 s | conducción | `VICEEXT camauto*` (14:38:01-40) |
| 140-162 s | **nado** | `VICEEXT swim enter/move` (14:38:43+) |

### Lo que se vio en los fotogramas (dato, no impresión)

1. **Nado (t=140-162 s): la pantalla entera es la calzada vista desde arriba**,
   con el HUD encima y Tommy fuera de cuadro. Coincide exacto con
   `SWIM2 ... camz=12.02` constante mientras el ped avanzaba 1,3 m/s a z=5,4 con
   el agua a 6,1.
2. **Apuntado (t=53 y t=55, AUG):** Tommy de espaldas con el arma en la mano
   derecha, **cañón apuntando arriba-izquierda** mientras la retícula está en el
   centro. En t=77 (Desert Eagle) el arma está a la altura de la cadera con el
   cañón cruzado. Los `AIMDIR` de esa franja: `manoV=79°…87°` (la mano apunta
   casi en vertical), `desv=0.0` (el cuerpo sí encara).
3. **Caminado apuntando:** el clip que suena/es el de la velocidad *con tope*
   (≤1,0), o sea `ANIM_STD_WALK` — el paso lento, que en Vice City es el de las
   cinemáticas. El jugador quiere el clip del movimiento normal a cadencia menor.

### Causas raíz y arreglos (R14, build `ve27`)

| # | Causa raíz (dato) | Arreglo |
|---|---|---|
| 1 | `CCam::IsTargetInWater` daba **verdadero nadando** (nuestro nado lleva al ped 0,7-0,9 m bajo la superficie) → el motor pedía `MODE_PLAYER_FALLEN_WATER`, la cámara del que se ahoga, que se queda clavada en `m_vecLastAboveWaterCamPosition` | `src/core/Cam.cpp`: nadando (`CPlayerPed::ViceExtIsSwimming()`) el predicado devuelve **falso** y la cámara sigue al ped (el objetivo ya sube a la superficie en los dos procesos de *follow ped*) |
| 2 | El autocentrado de coche disparaba el **empujón de 2,2 rad/s** con el flanco de soltado de `mirando`, **y `mirando` incluía el ratón**: `camauto2 auto=1 mirando=0 pedido=1 vel=0.0` 64 veces en una sesión; alternando `mirando=1/0` decenas de veces por minuto | `Cam.cpp`: dos detectores separados —**teclas** (cruceta/palo/mirar atrás) para el empujón y **ratón** sólo para reiniciar el temporizador del retorno suave—; el retorno pasivo pasa a ser **proporcional al error** (suave al arrancar, frena al llegar), 1,2 s de espera y zona muerta de 4° |
| 3 | El retroceso movía `CCamera::m_f3rdPersonCHairMultY`, que es **la retícula** (a la vez altura del HUD y ángulo del tiro) | `Weapon.cpp`: la mira **no se toca** (se sigue midiendo en la traza `RECOIL3 miracheck multY=`, tiene que quedarse en 0,400) y el retroceso lo lleva la cámara con el offset de `Cam.cpp`; patadas nuevas: 0,30° SMG … 1,40° francotirador |
| 4 | Apuntando, el clip lo elegía la velocidad **con tope** (`m_fMoveSpeed ≤ 1,0` → `ANIM_STD_WALK`, el paso lento) | `PlayerPed.cpp` (H4): el clip lo elige la velocidad **sin tope** (el del movimiento normal) y la cadencia se escala con el cociente real (`speed = m_fMoveSpeed/odAimWalkUncapped`, acotado 0,35-1,0) |
| 5 | Los sonidos del mod se **posponen** (2 decodificaciones inline por frame y 1 de cola) o se **reciclan** de la caché; los one-shot de arma se quedaban sin arrancar o arrancaban frames después | `sampman_oal.cpp` + `sampling`: muestras **reservadas** (`ODSFXPROT sfx=`) que se decodifican al instante y no se tiran; reparto a 3 inline / 2 de cola por frame. Lo pide `AudioLogic.cpp` al primer disparo de cada arma del mod (también la pareja L/R `+1`) |
| 6 | La cámara agachado bajaba sólo **0,27 m** (con −0,55 de objetivo: al bajar el objetivo la cámara se separa del ped y vuelve a subir) — el bloque H lo marcaba en FALLO | `config.h` + `Cam.cpp`: `VICEEXT_CROUCH_CAM_DROP` pasa a **−0,95**; la traza `CROUCH2` añade `baja=` con el valor aplicado |

Nota: los **sonidos** están bien *de datos* (comprobado: el mp3 servido se decodifica
idéntico al PCM del mod — misma longitud en muestras, misma envolvente—; y la tasa
de la SDT coincide con la del fichero). El fallo era de **entrega**, no de contenido.

### Pendiente con dato concreto (siguiente paso)

**Apuntado de las armas del mod (AUG, Desert Eagle).** El vídeo dice que el arma
no apunta a la retícula, pero los `AIMDIR` que tenemos (`manoH`/`manoV`) no tienen
**referencia**: falta medir el **`m4` de serie**, que el jugador da por bueno, en
la *misma* sesión. Prueba de 2 minutos para la próxima partida:

1. Con el **M4 de serie** apuntado y quieto → 3-4 líneas `AIMDIR` (apunta a la
   retícula).
2. Con el **AUG**, apuntado y quieto → 3-4 líneas.
3. Con el **Desert Eagle**, igual.

Con las dos primeras se ve si `manoH/manoV` del AUG se parece al del M4 (entonces
el fallo es del **modelo del mod**: orientación del `.dff` respecto al hueso de la
mano, se corrige con un offset por modelo) o si difiere mucho (entonces es el
**clip** `STEYR_fire`: la pose, no el modelo).

También queda por verificar en la próxima partida: `SWIMCAM` con `cam` variando
(la cámara ya no clavada), `RECOIL3 multY=0.400`, `camauto2` con `pedido` bajo y
`CROUCH2 baja=0.95` con descenso ≥0,4 m.

---

## 13ª partida (22/09) · desagacharse devolvía la cámara pero no la pose → R15 (`ve28`)

Lo que preguntó el jugador, palabra por palabra:

> «vale el error de que la cámara se mantiene fija al agachar y nadar? por que al
> estar agachado le vuelvo a dar para pararse este solo cambia la altura de la
> cámara pero el personaje sigue agachado, ya podré caminar agachado y nadar sin
> errores en cámara?»

### Respuesta corta, por partes

| Qué preguntó | Estado | Dónde está |
|---|---|---|
| Cámara **fija agachado** | arreglado en `ve27` | R14-6: `VICEEXT_CROUCH_CAM_DROP` −0,95 (el objetivo bajaba tanto que la cámara se separaba del ped y volvía a subir ⇒ sólo 0,27 m). La sigue el ped desde R6 (el movimiento lo lleva el motor otra vez) |
| Cámara **fija nadando** | arreglado en `ve27` | R14-1: `CCam::IsTargetInWater` daba verdadero nadando ⇒ el motor pedía `MODE_PLAYER_FALLEN_WATER` (la cámara del ahogado, clavada en la última posición sobre el agua). Ahora nadando ese predicado es falso y el objetivo sube a la superficie (`SWIMCAM objetivo= nivel= cam=`) |
| **Desagacharse no devuelve la pose** | **era un fallo real** (nuevo) y se arregla aquí | R15, build `ve28` |

### Causa raíz del fallo nuevo (fichero:línea)

`CAnimManager::BlendAnimation` (`src/animation/AnimManager.cpp:1277`) retira las
animaciones que había puestas **sólo si son del mismo tipo**:

```
if(isPartial == anim->IsPartial()){ ... anim->blendDelta = -delta*anim->blendAmount;
                                     anim->flags |= ASSOC_DELETEFADEDOUT; }
```

Los tres clips del mod (`ANIM_STD_CROUCH_*`, `AnimManager.cpp:958-960`) son
`ASSOC_REPEAT | ASSOC_PARTIAL` en `ASSOCGRP_PLAYERCROUCH`; de pie, el motor mezcla
caminar/idle **no parciales** de `ASSOCGRP_STD` (`SetRealMoveAnim`). Tipos
distintos ⇒ el barrido no los toca ⇒ el clip de agachado **se queda puesto para
siempre** (`ASSOC_REPEAT`). Y como lo único que dependía de `odCrouched` era el
objetivo de altura de la cámara, el jugador veía «sólo cambia la altura de la
cámara». Lo mismo ocurría al entrar en un coche, morir o una cinemática (el early
return de `ViceExtCrouchControl` ponía `odCrouched = false` sin tocar la
animación).

### Arreglo (`src/peds/PlayerPed.cpp`, R15)

```
ViceExtCrouchStopAnims(ped)     // igual que CPed::ClearDuck:
  a->flags |= ASSOC_DELETEFADEDOUT; a->blendDelta = -4.0f;   // 3 clips, ~0,25 s
```
llamado en los tres caminos de salida: tecla (conmutar a off), correr/saltar y
pérdida de posesión. Trazas nuevas:
`VICEEXT crouch pose off clips=<0..3>` y
`CROUCHPOSE desde=<ms> idle=… fwd=… back=…` (vigilante 1/s, sólo con peso > 0,01).

### Cómo se dictamina (bloque `R15`)

Sin navegador, del log de una partida:

- `VICEEXT crouch off` sin `VICEEXT crouch pose off` ⇒ **FALLO** (motor viejo).
- Cualquier `CROUCHPOSE` con `desde >= 700` y peso > 0,05 ⇒ **FALLO**: «el cuerpo
  sigue agachado tras desagacharse».
- Autocomprobado con log sintético bueno y malo; y sobre el log real de la 12ª
  partida (`ve26`) da **FALLO** con el dato: 9 `on` / 9 `off` y 0 `pose off`.

Marca `CROUCHPOSE` añadida a `tools/check-served-build.sh` → build `ve28`,
**28/28** marcas OK.

### Qué mirar en la próxima partida (2 minutos)

1. **C** para agacharse y caminar agachado → la cámara sigue a Tommy y baja con él.
2. **C** otra vez → **de pie de verdad** (no sólo la cámara sube). En el log:
   `VICEEXT crouch pose off clips=1` justo después del `off`, y ninguna
   `CROUCHPOSE` con peso > 0,05 pasado un segundo.
3. Correr o saltar agachado → `VICEEXT crouch off motivo=carrera` + `pose off`.
4. **Nadar** y salir del agua → `SWIMCAM` con `cam` variando (no clavada) y, al
   salir, `VICEEXT swim exit motivo=poco-hondo|sin-agua`.

Con eso quedan cerrados R6 (andar agachado), R7 (nado), R14-1/R14-6 (las dos
cámaras) y R15 (pose al desagacharse).

## R19 (15ª partida): la velocidad es LA DEL CLIP, y la sonda ya no miente

**Qué se midió** (sonda `tools/crouch-swim-smoke-test.mjs`, `ve35`,
`RESULTADO: PASS` 19/19, dos pasadas):

- **Agachado andando:** 0,87 m/s de avance real (`CROUCH2 ... velo=`), peso del
  clip 1,00, clips 237/239, cámara **modo 4** (seguir-al-ped) a 3,56 m constante.
- **Nadando:** 2,32 m/s = la velocidad natural de la braza del mod, `dz≈-0,01 m`
  (flota sobre el punto de flote, no apoyado en el fondo: hondo 0,54), cámara
  **modo 4** a 3,43-3,70 m, sin cámara de ahogado.
- **Reloj:** `PEDAT ... ts` vs `ms/1000` = 1,00 en 160 muestras.

**El fallo de medida que había que arreglar sí o sí** (porque daba un FALSO
fallo): `ViceExtCrouchAnim` se llama **dos veces por frame** (a propósito: el clip
se pide también desde `SetRealMoveAnim`), así que el tiempo de mundo se sumaba dos
veces y `velo = avance / dt` salía a la mitad (0,44 de un andar de 0,87). Filtrado
por `CTimer::GetFrameCounter()`; traza con la marca `medida=1x` y bloque **R19**
del verificador (`dt≈1,00` + las dos formas de medir coinciden).

**Cómo se comprueba sin jugar** (2 minutos de máquina):

```bash
cd gta_vc_browser
node tools/crouch-swim-smoke-test.mjs        # agachado + navegar al agua + nadar
python tools/viceext-log-check.py web/odtrace.log   # R17 y R19 en OK
bash tools/check-served-build.sh             # 35/35 marcas
```

La sonda **navega** con `PEDAT ... mar=/dist=` (azimut y distancia al agua que
mide el motor): sostiene el rumbo que acerca al mar y gira ~45° si se atasca. Ya
no anda a ciegas 4×30 s (el 22/09 se quedó contra una casa y el nado quedó SIN
MEDIR, que se leía como fallo del mod).

**Si lo pruebas a mano:** agachado (C) y andar agachado → la cámara baja y sigue
a Tommy; **C** otra vez → de pie de verdad; nadar y salir del agua → la cámara
sigue nadando desde cerca. En el log: `CROUCH2`/`PEDAT` con `velo` ≈0,9 agachado
y ≈2,3 nadando, `modo=4` en los dos.
