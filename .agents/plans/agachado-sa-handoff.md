---
name: agachado-sa-handoff
status: EXECUTED
type: feature
domain: player-controls-aim-crouch
owner_rules: .agents
created: 2026-09-23
---

# Agachado (Vice Extended / SA) — handoff · build `ve51` · 23/09/2026

Contexto para el siguiente agente: **qué se está haciendo, qué se acaba de arreglar,
cómo se mide, qué queda y qué trampas del motor ya están pagadas.** El plan sin código
está en `agachado-sa.md` y el historial largo en `../HISTORIAL.md`.

**Build actual: `ve51`** (`gta_vc_browser/web/lib/index.js` → `VERSION`). Salida del
build: `gta_vc_browser/web/public/build/reVC.js` + `.wasm`, que es lo que sirve el
navegador. Recompilar:

```bash
cd /c/Users/s0rno/OneDrive/Documents/re3
export EMSDK=/c/Users/s0rno/emsdk
export PATH="$EMSDK:$EMSDK/upstream/emscripten:$PATH"
ninja -C gta_vc_browser/build/web          # incremental: ~1-2 min (1 fichero + enlace)
```

**Sube `VERSION` en CADA build**: va también en la URL de `reVC.js`/`reVC.wasm` y es lo
único que fuerza al navegador a bajar el binario nuevo. Un build enlazado con el mismo
tag se sigue sirviendo de caché (pasó el 23/09 y mareó dos rondas de medidas).

---

## 1. LÉELO EN 60 SEGUNDOS

- El objetivo es **solo** el agachado del mod (`Vice Extended` v1.5): agacharse con C,
  andar agachado en las 8 direcciones, rueda de lado apuntando y pose de apuntar
  agachado. Nada más entra hasta que esto esté fino.
- El movimiento del agachado lo pone **código** (bloque R20c en
  `CPed::CalculateNewVelocity`, `src/peds/Ped.cpp`), no el clip, porque el `ped.ifp`
  del mod no tiene clips de paso lateral. Velocidad objetivo **0,90 m/s**
  (`VICEEXT_CROUCH_WALK_MPS`), rueda **2,35 m/s** y sólo apuntando.
- **La causa de todos los «va a super velocidad» era UNIDADES**, y costó tres rondas
  (§2). Si vuelves a tocar velocidades, lee §2 antes de escribir un número.
- El jugador prueba **jugando** y deja los datos en `logs/odtrace-*.log`
  (`gta_vc_browser/logs/`). Pidió expresamente **no** añadir query params ni volver a
  correr el arnés headless por ahora: se mide con las trazas de SUS partidas.
- Lo primero que hay que hacer: pedirle una partida agachada con W y comprobar en el
  log nuevo que `mvec ≈ obju` y `velo ≈ mps` (§5).

---

## 2. LA REGLA QUE COSTÓ TRES RONDAS: LAS UNIDADES

El motor mueve la posición con

```cpp
GetMatrix().Translate(m_vecMoveSpeed * CTimer::GetTimeStep());   // entities/Physical.cpp:425
```

y `CTimer::GetTimeStep()` vale **~1,0 a 50 fps** (`ms_fTimeStep = frameTime/1000*50`,
`core/Timer.cpp:150`). O sea: **`m_vecMoveSpeed` está en «metros por frame de 50 fps» =
m/s ÷ 50**, NO en m/s. El proyecto ya trae la constante:
`GAME_SPEED_TO_METERS_PER_SECOND 50.0f` y `METERS_PER_SECOND_TO_GAME_SPEED`
(`src/control/CarCtrl.h`).

El bloque del agachado escribía el objetivo en m/s directo, así que iba **×50**:
medido en la traza del jugador, `obj=0,90` → **331,51 m en 7,38 s = 44,90 m/s**, con
`mvec=0,90`. Eso era el *«voy más rápido que cualquier auto»*, el *«me muero si camino a
un lado»* (la rueda: 2,35 × 50 = 117 m/s) y los *«los pies no van a la par»* (los pies
barren a 0,9 m/s mientras el cuerpo iba a 45). Arreglado en R25 con
`odMps * METERS_PER_SECOND_TO_GAME_SPEED`; el nadado ya lo hacía así.

**Cómo se ve en el log** (así no vuelve a pasar inadvertido): `CROUCH2` publica las dos
unidades, `mps` (m/s) y `obju` (unidades del motor). Tienen que cuadrar siempre
`mvec ≈ obju` y `velo ≈ mps`.

---

## 3. ENCARGOS DEL JUGADOR (literal, en orden)

1. **16ª:** «el agachado caminando hacia adelante se ve perfecto; caminando a un lado se
   desplaza pero no se gira el personaje al lado donde está caminando, y falta que al
   apuntar estando agachado haga su animación de apuntado; si apunto y presiono el lado
   izquierdo o derecho el personaje rueda hacia ese lado — es el mismo sistema del San
   Andreas, aplicado a este mod».
2. **18ª:** «al estar agachado y moverme el personaje se mueve a una super velocidad sin
   razón, me muero si camino hacia un lado, la animación de apuntado no es la que
   debería; revisa los datos del mod, ajusta la velocidad del caminado como debería».
3. **19ª:** «no hagas más test headless; dime qué encontraste y qué propones, pruebo
   jugando» → «quítame query params, no necesito ni pedí eso; basate en logs, deja las
   cosas en logs así puedes ver de primera mano todo» → «al cargar la escopeta el juego
   crashea» → «no estás modificando el valor de la velocidad del personaje: mide con
   logs cuántos metros se ha desplazado desde que tiene la W pulsada, estando agachado
   voy más rápido que cualquier auto» (esto último es lo que cazó el fallo de unidades).
4. **Regla de velocidad que dio él:** «el estar agachado es más lento o la misma
   velocidad que el caminar». Andar de pie del motor = **0,79-1,13 m/s** (traza `PEDAT`).

---

## 4. DÓNDE ESTÁ CADA COSA

| Qué | Dónde |
|---|---|
| Define del bloque | `src/core/config.h` → `VICEEXT_CROUCH`, `VICEEXT_CROUCH_CAM_DROP` (0,55) |
| Estado y control (C, cancela al correr/saltar/coche/muerte) | `src/peds/PlayerPed.cpp` → `CPlayerPed::ViceExtCrouchControl` |
| Clip del mod, pose de apuntar, rueda, traza `CROUCH2`/`CROUCHMOVE` | `src/peds/PlayerPed.cpp` → `CPlayerPed::ViceExtCrouchAnim` |
| Velocidad objetivo (m/s) | `ViceExtCrouchMoveSpeed` / `ViceExtCrouchTargetSpeed` / `VICEEXT_CROUCH_WALK_MPS` 0,90, `VICEEXT_CROUCH_ROLL_MPS` 2,35 |
| Ritmo del clip = velocidad ÷ raíz del clip | `ViceExtCrouchRateFor` (se aplica en `odAssoc->speed`, `PlayerPed.cpp`) |
| Retirada de la pose al levantarse (`ASSOC_DELETEFADEDOUT` + `blendDelta = -4`) | `ViceExtCrouchStopAnims`, `ViceExtCrouchAimPoseOff` |
| **Movimiento en las 8 direcciones (R20c)** | `src/peds/Ped.cpp` → `CPed::CalculateNewVelocity` (bloque `#ifdef VICEEXT_CROUCH`) |
| Clips registrados (`Crouch_Idle/Forward/Backward/Roll_L/Roll_R`) | `src/animation/AnimationId.h` + tabla `aCrouchAnimDescs` en `src/animation/AnimManager.cpp` |
| Caída de cámara agachado | `src/core/Cam.cpp` (`TargetCoors.z -= VICEEXT_CROUCH_CAM_DROP`) |
| Traza de armas (`WINFO`) — aquí estaba el crash | `src/peds/PlayerPed.cpp` → `ViceExtWeaponInfoTrace` / `ViceExtClipName` |

**Constantes y su origen (no cambiarlas a ojo):**

| Constante | Valor | De dónde sale |
|---|---|---|
| `VICEEXT_CROUCH_WALK_MPS` | 0,90 m/s | regla del jugador (agachado ≤ caminar; caminar = 0,79-1,13) |
| `VICEEXT_CROUCH_ROLL_MPS` | 2,35 m/s | `Crouch_Roll_L/R` del mod (2,33/2,42 medidos), es una esquiva, no un paso |
| raíz de los clips | `Crouch_Forward` 3,58 m/s (2,615 m/0,731 s), `Backward` 1,85, `Roll_L` 2,33, `Roll_R` 2,42 | `python gta_vc_browser/tools/ifp_inspect.py <ped.ifp> [patrón]` |
| `VICEEXT_CROUCH_CAM_DROP` | 0,55 m | lo que baja la cabeza agachado; 0,95 metía la cámara dentro del ped |

---

## 5. LAS TRAZAS (esto es el instrumento de medida)

Ficheros: `gta_vc_browser/logs/odtrace-*.log` (los del jugador incluidos).

| Línea | Qué dice |
|---|---|
| `CROUCH2 …` | 1/s mientras está agachado. `mps` = m/s pedidos, `obju` = los mismos en unidades del motor, `pies` = `speed` del clip (ritmo), `mvec` = velocidad del motor, `moved`/`anim` = lo que da la animación, `avance`/`velo` = desplazamiento medido, `nf`/`hit` = frames de la muestra y si hubo tirón, `peso` = peso del clip agachado, `camdist`/`baja` = cámara, `mira`/`pesomira` = pose de apuntar, `rueda` = rodando |
| `CROUCHMOVE fin …` | **un tramo entero**: desde que pulsa la tecla hasta que la suelta. `m` = metros (sumando frame a frame), `t` = segundos de mundo, `mps` = m/t (velocidad real), `maxfr` = mayor desplazamiento de un frame (~0,02 m andando bien; si sale grande, hubo salto/catapultazo), `obj`/`obju` = lo pedido, `mvec`/`fspd`/`anim` = lo que cree el juego |
| `VICEEXT crouch roll lado=izq|der clip= veloc= dur=` | evento por rueda (no va por el reloj de 1/s) |
| `VICEEXT crouch on/off …`, `CROUCHPOSE` | entrada/salida del agachado y prueba de que el cuerpo vuelve a estar de pie |
| `PEDAT x= y= z= av= velo= …` | estado general y desplazamiento (sirve de referencia de pie) |
| `FPSLOG maxdelta=` / `FPHASE gap=` | tirones del navegador: invalidan las medidas por muestra de 1 s (`hit=1` las marca) |

**Leer las medidas con esto en la cabeza**: un muestreo de 1 s se ensucia con un tirón
o con una muerte/respawn (el ped salta decenas de metros y sale un «velo» absurdo). El
tramo (`CROUCHMOVE`) no se ensucia: suma distancias y tiempos de los mismos frames.

---

## 6. LO QUE QUEDA (por orden)

1. **Verificar el arreglo de unidades en el log del jugador** (build `ve51`): pedir una
   partida agachada con W varias veces y comprobar `velo ≈ 0,90` y `mvec ≈ obju`.
2. **Los pies.** Con el ritmo acoplado (`ritmo = velocidad ÷ raíz del clip`) los pies
   barren lo mismo que avanza el cuerpo y no patinan, pero a 0,90 m/s el ritmo es 0,25:
   las piernas van lentas (una zancada cada ~2,9 s). Es matemática del clip del mod
   (zancada de 2,615 m por ciclo), no un fallo: o se sube la velocidad o se ve
   slow-motion. Decidir con el jugador, mirando `pies` vs `mps`.
3. **`GunMove_*` para apuntar agachado andando.** Son del mod (`GunMove_FWD/BWD` 1,85,
   `GunMove_L/R` ±1,803 m) y hoy no se usan: apuntando agachado y andando hay una mezcla
   (pose a peso 0,4). Registrarlos como ids nuevos (patrón `aCrouchAnimDescs`) y pedirlos
   cuando `odAim && odWalking`.
4. **Apuntar agachado quieto**: la pose es `WEAPON_crouch` (el mod no trae
   `WEAPON_crouchfire` ni `WEAPON_crouchreload`, comprobado en su `ped.ifp`). Verificado
   en el arnés (1,00 quieto / 0,40 andando / 0 al soltar) pero **sin validar por el
   jugador a ojo**.
5. **Giro del cuerpo andando agachado sin apuntar.** Hoy los lados usan el clip de
   adelante y el desplazamiento lo pone el código. Girar el cuerpo es el modelo de SA,
   pero con ratón `walkAngle = m_fRotationCur + ángulo del mando` y el mando se lee en el
   sistema del cuerpo: girar `m_fRotationCur` sin más hace que el ped ande en círculo.
   Haría falta guardar el rumbo de avance en el MUNDO (una vez por petición).
6. **Bordes**: morir apuntando agachado, entrar/salir de coche, agua, atropello.
7. **Punto de prueba sin tráfico** para el arnés (los coches atropellan al ped y el motor
   cancela el agachado: `CARPED … model=210` → `crouch pose off`).

---

## 7. CÓMO SE PRUEBA

**Ahora mismo: jugando.** El jugador pidió no correr el arnés headless mientras se
depura el agachado; su log es la medida.

```bash
# Logs de partida (los más nuevos arriba)
ls -lt gta_vc_browser/logs/ | head
grep -n "CROUCHMOVE\|CROUCH2" gta_vc_browser/logs/odtrace-<fecha>.log | tail -20
```

**No hay palancas de URL ni de consola** (se quitaron a petición expresa). El agachado
son constantes en `src/peds/PlayerPed.cpp`: si hay que cambiar un número, se cambia la
constante y se recompila. **No volver a añadirlas.**

Si en algún momento se retoma el arnés (existe, y está pagado):

```bash
cd gta_vc_browser
node tools/vc-test.mjs --escenario agachado-mira   # 28 s: apuntado, pose y rueda
node tools/vc-test.mjs --escenario agachado        # 8 direcciones + cámara (~2 min)
node tools/vc-test.mjs --estatico                  # 3 s: binario servido y marcas
bash tools/check-served-build.sh                   # marcas del paquete servido
```

---

## 8. TRAMPAS DEL MOTOR (ya pagadas — no volver a pisarlas)

1. **Unidades**: `m_vecMoveSpeed` y `m_moved` no van en m/s, van en «metros por frame de
   50 fps» (m/s ÷ 50). Ver §2. La misma trampa vale para cualquier velocidad que se
   escriba en el motor.
2. **`CAnimBlendAssocGroup::GetAnimation(id)` no comprueba rango** (`&assocList[id -
   firstAnimId]`): pedir el nombre de un clip que ese grupo no tiene devuelve un puntero
   basura y un `%s` tumba el wasm (`memory access out of bounds` en `printf`). Fue el
   crash al cambiar de arma. Comprobar `firstAnimId`/`numAssociations` antes de tocar
   la asociación (ver `ViceExtClipName`).
3. **Un buffer de traza corto pierde justo el final de la línea** (`snprintf` trunca sin
   avisar): `CROUCH2` salía cortado en `rued`. Ahora es de 480.
4. **Un tirón del navegador invalida las medidas por muestra de 1 s** (4,3 s de stall →
   «velo=45 m/s» falso). Marca `hit`/`nf` y, para medir de verdad, usa tramos.
5. **`BlendAnimation` reinicia el clip terminado** (`if(!found->IsRunning() &&
   found->currentTime == totalLength) found->Start(0)`): las poses que se quedan en su
   último fotograma se piden sólo si no están o se apagan, nunca cada frame.
6. **El barrido de `BlendAnimation` sólo apaga animaciones del MISMO tipo**: los clips de
   agachado son NO parciales y la pose de apuntar PARCIAL → se mezclan sin pisarse.
7. **La pose parcial se come la traslación de los clips no parciales**
   (`1 - totalBlendAmount` en `AnimationFrameUpdate`/`FrameUpdate.cpp`). A peso 1 se
   pierde la traslación entera. R20c no depende de eso: mueve con la velocidad objetivo.
8. **Para un clip de movimiento, el ritmo efectivo es `speed`** (`RpAnimBlend.cpp`
   calcula `relSpeed = totalBlend/totalLength` con `totalLength += clipLen/speed`), y el
   avance de la raíz escala con él: poner `speed = velocidad ÷ raíz del clip` acopla pies
   y cuerpo.
9. **`bIsDucking` clava al jugador y apaga el IK del brazo** (`CPed::AimGun`). Nada del
   agachado propio debe activarlo (por eso la pose se mezcla a mano).
10. **Apuntando, la cámara cambia de modo** (`camdist` 0,61 m vs 3,56 m medidos) y hay
    código detrás de cortes por modo: comprobar el modo antes de dar por hecho que un
    bloque corre. `m_fRotationDest` lo reescribe el control de ratón cada frame.
11. **`ViceExtCrouchAnim` se llama dos veces por frame** (el clip se pide también desde
    `SetRealMoveAnim`): cualquier acumulación por frame se filtra con
    `CTimer::GetFrameCounter()`.
12. **El mundo sigue vivo durante las pruebas**: un coche atropella al ped, el motor
    cancela el agachado y además lo teletransporta. La corrida larga del arnés se cayó
    así.
13. **Trucos del mod**: `CRAZYTOOLS` mete armas en el inventario pero no las pone en la
    mano; `CRAZYPISTOL` deja la Beretta en mano. El ciclo de armas con teclado no llega
    al motor en el navegador; la rueda del ratón sí.
14. **No hay clips de paso lateral agachado** en el `ped.ifp` del mod: lo más parecido
    son los `Crouch_Roll_*`, y su raíz viaja HACIA ATRÁS (−2,17/+2,25 m en Y), así que el
    desplazamiento lateral lo tiene que poner el código (R20c).
15. **Cuidado con subir una constante para pasar un test**: `VICEEXT_CROUCH_CAM_DROP` a
    0,95 es lo que metió la cámara dentro del ped. Si un test pide un número raro, mirar
    primero si el test mide lo que importa.

---

## 9. ESTADO DE LA VERIFICACIÓN

- [x] R23 (`ve48`): crash al cambiar de arma arreglado (rango del grupo de animación).
- [x] R24 (`ve49`): palancas de URL/consola fuera; velocidad fija 0,90 m/s.
- [x] R25 (`ve50`/`ve51`): medidor de tramo (`CROUCHMOVE`) y **conversión de unidades**
      del agachado (×50); buffer de `CROUCH2` a 480; `obju` en la traza.
- [x] Compila y se sirve con el tag `ve51`.
- [ ] **Prueba del jugador sobre `ve51`**: `velo ≈ 0,90` y `mvec ≈ obju` en un tramo
      agachado con W. Es lo primero que hay que mirar.
- [ ] Pies a la par (decidir con él si el slow-motion de las piernas a ritmo 0,25 es
      aceptable o se sube la velocidad).
- [ ] Apuntar agachado (pose quieto/andando) validado a ojo por el jugador.
- [ ] Rueda de lado: dirección medida sin teletransportes de tráfico.
- [ ] Suite headless completa (aplazada a petición del jugador).

---

## R26 (23/09, post-ve51) — velocidad 1,00 + giro a los lados sin apuntar

Pedido del jugador tras verificar ve51 ("ahora si va a la velocidad que
deberia"): (1) el caminar agachado a la velocidad del caminar normal menos un
poquito (`VICEEXT_CROUCH_WALK_MPS` 0,90 -> **1,00 m/s**; el andar de pie mide
0,79-1,13 en PEDAT). El ritmo sale solo (1,00/3,58 = 0,28): las piernas lentas
siguen abiertas en el punto 2. (2) A los lados y sin apuntar, el cuerpo **gira**
hacia el avance (antes se desplazaba mirando al frente):
`ViceExtCrouchSideHeading` devuelve el rumbo en el MUNDO (camara + mando, fijo
mientras no se toquen: sin el circulo del punto 5),
`PlayerControl1stPersonRunAround` lo pone de destino en vez del forzado a
camara, y R20c avanza en esa misma base. Apuntando no se gira (ahi se encara y
se rueda). Verificacion: `rumbo` de CROUCH2 siguiendo a `ang` a los lados + ojo
del jugador. Build `ve52`.
y `giro=`/`hdgr=` en CROUCH2 (motivo del helper + headingRate, para
cazar por que no giraba en ve52: el `rumbo` se quedaba fijo con `ang=+-90`).

## R26b (23/09) — cadencia de piernas x1,7 + instrumento del giro (`ve54`)

El jugador (ve52): las piernas van en camara lenta, el cuerpo a 1,00 bien; y a
los lados sigue sin girar. (1) `VICEEXT_CROUCH_CADENCE 1,70` solo para caminar
(adelante/atras; la rueda va a ritmo natural): piernas a cadencia de andar
(ciclo ~1,5 s) con el cuerpo a 1,00. Fisica honesta: a igual zancada (2,6 m),
mas cadencia = los pies barren x1,7 lo que avanza el cuerpo (patinan); es lo
pedido ("piernas mas rapido, cuerpo igual") y va en una constante. (2) El giro
no enganchaba en ve52 con todo en verde en estatica: CROUCH2 lleva `giro=`
(0 = girar; 2 sin agachado, 3 apuntando, 4 mando flojo, 5 no-lateral) y `hdgr=`
(headingRate) para cazarlo en una sesion corta de strafes. Build `ve54`.
 + `giro=`/`hdgr=` en CROUCH2 (cazan si el destino llega al cuerpo).

## R26d (23/09) — velocidad 0,90 + giro ejecutado en R20c (`ve55`)

El jugador (ve54): piernas bien, cuerpo a 1,00 bien; giro sin funcionar
(`rumbo` fijo con `ang=+-90` y `giro=0`) y regla nueva de velocidad: caminar=1
-> agachado=0,90 (la cadencia x1,7 NO se toca). Apuntar agachado debe ir a la
misma velocidad que andar agachado (en el log iban igual: 0,99; sin tope
distinto en el codigo, queda unificado por construccion).
Cambios: `WALK_MPS` 1,00 -> 0,90; el giro se ejecuta con snap Cur+Dest en R20c
(`Ped.cpp`, que corre siempre agachado: en ve52-ve54 solo se ponia el destino
en el control y nunca llegaba al cuerpo). Apuntar no gira (R21). Build `ve55`.
