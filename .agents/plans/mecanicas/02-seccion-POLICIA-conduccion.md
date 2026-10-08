---
name: 02-seccion-POLICIA-conduccion
status: EXECUTED
type: feature
domain: gameplay
owner_rules: .agents
created: 2026-09-19
---

# Sección 2 — Policía y conducción (para **aux 1**)

Dos bloques de mecánica + uno opcional que depende de la sección 1. Contexto
general y reglas: `00-INDICE.md` (léelo antes; ahí están el arnés, la propiedad
de ficheros y los avisos de `node`/`chrome`/`dataTag`).

Fuente de estas mecánicas: su **changelog** (v1.0 → v2510) y su `features.ini`
(el mod no publica código; las referencias abiertas de SA/plugin-sdk son
*especificación*, no código pegable).

---

## P1 — Esconderse de la policía (v1.0: "Changed wanted system")

> "The player has the ability to hide from the police. When the cops do not see
> the player, they will try to find him."

En vanilla VC ya existe *algo* de esto (bajan las estrellas si no te ven), pero
el mod lo convierte en un sistema de **búsqueda**: los policías se dirigen a la
última posición conocida, registran la zona y desisten; el jugador puede
esconderse incluso con nivel alto.

**Punto de partida medido (hoy):**

- `src/core/Wanted.{h,cpp}`: `CWanted` con `m_nWantedLevel`, `SetWantedLevel`,
  `SetWantedLevelNoDrop`, `UpdateWantedLevel()`, `Update()`, `m_nChaos`.
- `src/peds/CopPed.cpp`: `CCopPed::CopAI` (persecución, disparo, arresto) y
  `CCopPed::ProcessControl`; `CCopPed::SetPartner`/`SetHeli`.
- `src/peds/PedAI.cpp:3708`: el bucle de "ocupante de vehículo" ya comprueba
  `IsPedDoingDriveByShooting`/`m_nPedState == PED_DRIVING`.
- `src/control/CarAI.cpp` y `src/core/Wanted.cpp` registran/limpian la
  persecución; `CWanted::Update()` es el latido del sistema.

**Tareas (en orden):**

1. **Medir antes de tocar**: con una sonda nueva, subir el nivel de búsqueda
   (cheat `YOUWONTTAKEMEALIVE` ya existe en `Pad.cpp`) y registrar en
   `odtrace.log` (o en la consola de la página) cuántos segundos tarda en
   bajar el nivel sin que te vean, y qué hacen los `CCopPed` mientras tanto.
   Escribir esas cifras en la sección "Estado" de este fichero **antes** de
   cambiar código (es la línea base contra la que se compara).
2. Definir el contrato mínimo del bloque detrás de `#define VICEEXT_HIDE_COPS`:
   - estado nuevo de búsqueda: `última posición conocida` + temporizador;
   - regla de visibilidad: un policía solo "ve" si hay línea de visión
     (`CWorld::ProcessLineOfSight`) y distancia < radio (elegir y documentar);
   - el nivel de búsqueda **no** baja mientras te vean; sí cuando no, con el
     temporizador de la sonda (empezar por lo medido en vanilla, sin inventar).
3. Implementarlo en `CWanted` (estado) + `CopPed`/`PedAI` (ir a la última
   posición conocida y registrar la zona). Nada de rutas nuevas de IA: usar
   `SetFollowPath`/`Seek`/`WanderPath` existentes.
4. Cheat de prueba propio (añadir **al final** del bloque de cheats de
   `src/core/Pad.cpp`, con comentario `// Sección 2:`): algo tipo `CRAZYCOP` que
   ponga nivel 3 sin matar a nadie, para poder medir siempre igual.
   **Ojo**: el literal de `Cheat_strncmp` va codificado con desplazamientos por
   posición y al revés — copiar el patrón exacto de `CRAZYTOOLS`/`CRAZYRIDES` y
   **contar los bytes** (un literal de 11 para 10 teclas no dispara nunca y no
   avisa). La sección 1 puede prestarte el literal si lo pides.
5. Sonda `tools/hidecops-smoke-test.mjs` (copiar `vehicles-smoke-test.mjs`):
   cargar slot, teclear el cheat, esconderse (campos conocidos del save de
   prueba), esperar X s, comprobar que el nivel baja y que **no hay
   `RuntimeError`**; capturas antes/después.

**Verificación (PASS):** el nivel de búsqueda baja en el tiempo medido o menos
si estás fuera de la vista, no baja si te ven, y la partida no revienta
(0 asserts en la consola de la página). Captura del HUD con estrellas.

**Riesgos:** es el bloque con más superficie de regresión (policía + arresto +
misión). `CWanted::Update()` se llama siempre: cualquier bloqueo ahí congela el
juego. Si algo se tuerce, el define permite apagarlo sin tocar datos.

---

## P2 — Drive-by ampliado (v1.5: "Drive-by shooting")

> "Drive-by shooting" + "Movement while aiming" (v1.5).

Medido hoy: **ya existe** el drive-by en coche, moto y barco —
`CAutomobile::DoDriveByShootings` (`Automobile.cpp:3855`),
`CBike::DoDriveByShootings` (`Bike.cpp:2021`),
`CBoat::DoDriveByShootings` (`Boat.cpp:1413`) — pero está **restringido**:

- `playerInfo->m_bDriveByAllowed` (si es falso, no hay drive-by);
- el arma debe ser de **slot** `WEAPONSLOT_SUBMACHINEGUN` (en moto, literal
  `m_nWeaponSlot != 5`);
- coche: además `bLowVehicle` cambia las animaciones
  (`ANIM_STD_CAR_DRIVEBY_{LEFT,RIGHT}[_LO]`), y hay variantes por asiento;
- moto: existen `ANIM_BIKE_DRIVEBY_{LHS,RHS,FORWARD}` ✓ (así que el "hacia
  delante" ya está previsto);
- la cámara decide `lookingLeft/lookingRight` (`CCam::LookingLeft/Right`,
  `MODE_TOPDOWN`, `m_bObbeCinematicCarCamOn`).

**Tareas (en orden):**

1. **Medir**: lista de qué armas están permitidas por clase de vehículo hoy
   (leer los tres `DoDriveByShootings` y `WEAPONSLOT_*`), qué animaciones
   `ANIM_*_DRIVEBY_*` existen en los grupos de anim del port, y qué pasa con
   los **pasajeros** (`pPassengers[]`, `ANIM_STD_CAR_DRIVEBY_*` por asiento).
   Añadir la tabla a "Estado" antes de tocar.
2. Contrato del bloque (`#define VICEEXT_DRIVEBY_WIDE`): permitir **pistolas
   además de SMGs** (una mano), y en moto/barco mantener el disparo hacia
   delante que ya existe. Dos manos (rifles/lanzacohetes) **fuera**: el mod
   tampoco los da en drive-by.
3. Implementar en los tres `DoDriveByShootings` (una condición nueva por
   clase, sin duplicar el cuerpo de la función) + `CPad`/`CPlayerInfo` si hace
   falta relajar `m_bDriveByAllowed`.
4. Sonda `tools/driveby-smoke-test.mjs`: cargar partida, subir a un coche (el
   save de prueba puede tener uno cerca), teclear `CRAZYTOOLS` para tener
   pistola+SMG, disparar en marcha con `GetCarGunFired` (tecla de disparo del
   coche) y comprobar por capturas que el brazo sale por la ventanilla y que no
   hay asserts. Repetir en moto (barco si el save lo permite).

**Verificación (PASS):** con pistola, desde coche **y** moto, se dispara con
animación lateral (no la de conducir), y sigue funcionando la SMG (no
regresión).

**Riesgos:** `m_bDriveByAllowed` lo toca el script en misiones (hay misiones que
lo prohíben): respetar el valor cuando sea `false` **por script** (si el script
lo apaga, no se enciende). Revisar `Script*.cpp` antes de forzar nada.

---

## P3 — La moto policial VCPD (opcional, depende de la sección 1)

Su changelog v3.0: "VCPD Wintergreen police motorcycle. Can be seen in the
traffic". El modelo ya está dentro (`polwintergreen`, ID 6507, clase `ignore`
en su IDE) pero hoy **no circula** (el tráfico lo decide la clase). Este bloque
es "que se vea y se comporte como vehículo policial":

- que aparezca en el tráfico / en las zonas policiales (coordinar con la
  sección 1, que es la dueña de los datos `default.ide`/`carcols.dat`);
- luces de sirena (coronas) en una moto: `CAutomobile` tiene las coronas de
  sirena en un `switch (GetModelIndex())`, `CBike` **no** tiene equivalente →
  sin ellas la moto policial no parpadea. Implementarlas para
  `MI_VEEXT_POLWINTERG` (coronas `TYPE_STAR` rojo/azul, posición del faro)
  puede ser un bloque corto y visible;
- que los policías la usen (opcional, si P1 deja sitio).

Empezar este bloque **solo** cuando la sección 1 haya confirmado que el modelo
circula; si no, se convierte en "perseguir un coche que no aparece".

> **Actualización de coordinación (sección 1, bloque D3).** La parte de las
> **coronas ya está hecha** por la sección 1 (define
> `VICEEXT_POLICE_BIKE_LIGHTS`): `CBike::ProcessControl` registra las dos luces
> `TYPE_STAR` rojo/azul del manillar y `CVehicle::UsesSiren()` ya incluye
> `MI_VEEXT_POLWINTERG` (sin esa entrada la moto no tenía sirena *de ningún
> tipo*: ni claxon que la conmute, ni bucle de audio). Lo que queda de P3 es lo
> de conducción, y es lo que de verdad aporta:
>
> 1. que **circule** (clase del IDE — `ignore` hoy —, `carcols`, frecuencia:
>    `default.ide` es de la sección 1, así que pedid el cambio en vez de
>    tocarlo);
> 2. que la **use la policía**: `CVehicle::IsLawEnforcementVehicle()` y
>    `CCopPed`/`CarCtrl` no la conocen todavía;
> 3. opcional: que los agentes de tráfico la traten como vehículo policial
>    (`MakeWayForCarWithSiren` ya funciona porque va por `UsesSiren()`).
>
> El cheat `CRAZYRIDES` crea la moto con la sirena encendida para poder verla
> parpadear sin conducirla: úsala como banco de pruebas.

---

## Estado (lo rellena aux 1)

- [x] P1 medido  → cifras: **abajo (§ P1 línea base, build `ve8` + traza)**
- [x] P1 implementado → `VICEEXT_HIDE_COPS` (`config.h`, `Wanted.{h,cpp}`, `CopPed.cpp`)
- [x] P1 sonda PASS → **`hidecops-smoke-test.mjs` modo `feature`, `reVC.wasm` del 19/09
  21:20, 0 errores, bajada medida a los 15.0 s (§ P1 cierre)**
- [x] P2 medido → tabla de armas permitidas: **abajo (§ P2 medición)**
- [x] P2 implementado → `VICEEXT_DRIVEBY_WIDE` (`config.h`, `Ped.{h,cpp}`,
  `Automobile.cpp`, `Bike.cpp`, `Boat.cpp`, `Pad.cpp` = cheat `CRAZYPISTOL`):
  **abajo (§ P2 cierre)**
- [x] P1 verificación por partida real → **`tools/hidecops-log-check.mjs`** (nuevo,
  sólo lectura de `odtrace.log`) sobre la partida del 20/09 10:03→10:12 (nivel máx 4):
  la traza por segundo funciona y **el contrato no queda contradicho**, pero no hubo
  ningún episodio de esconderse (los 13 s sin que te viera ningún perseguidor tenían
  policía a 18 m: no estabas escondido). Ver § P1 partida real.
- [x] P2 verificación → **la primera partida real (20/09 10:04) cazó un fallo**: la
  pistola que llevabas en la mano se cambiaba por la SMG al subir al vehículo
  (4 entradas `switch-smg` + 4 salidas `restore-stored`). **Arreglado** (precedencia en
  `RemoveWeaponWhenEnteringVehicle`, `Ped.cpp`) y **re-publicado a las 10:19 con
  `rev=2`** en la traza (la confirmación llegó en la partida siguiente: ver la fila
  **P2 CERRADO** de abajo). La sonda `driveby-smoke-test.mjs` sigue escrita y sin
  correr: el jugador pidió no lanzar más pruebas de Chrome/swiftshader. Ver
  § P2 corrección por partida.
- [x] **P2 CERRADO — PASS en partida real** (20/09 12:57→13:10, `JS build=2026-09-20-ve10`,
  `P2 rev=2`): 34 disparos de **pistola (arma 17) desde coche** con animación lateral
  (`left` x24, `right` x10) y **4 entradas con `outcome=keep-weapon`** (la pistola se
  queda en la mano); SMG sin regresión (`anim` laterales en moto y coche, 1 entrada
  `switch-smg`). Comando: `node gta_vc_browser/tools/driveby-log-check.mjs`.
  Ver § P2 confirmación en partida real.
- [x] P1 verificación por partida real **#2** (20/09 12:57→13:06) → nivel máximo **1**,
  así que el bloque (que sólo actúa a nivel ≥ 2) **no se ejerció**: 0 episodios. Se
  aprovecha como **línea base de nivel 1 medida en vivo**: la estrella cayó a los
  ~21 s del último avistamiento, exactamente al cruzar `chaos` 50→49 (vanilla).
  Ver § P1 segunda partida real.
- [x] Traza saneada y coste del bloque acotado (`rev=4`, 20/09 13:2x):
  `WANTEDCOP join` estaba inundando el log (~60 líneas/s dentro del radio de un taller:
  `Garages.cpp:410` llama a `CWorld::CallOffChaseForArea` en **cada frame**). Ahora
  1 línea/s con `burst=N`, una línea por sesión `WANTEDHIDEINIT … rev=4 …`
  (prueba qué `.wasm` jugó) y el **barrido de visibilidad a 5 Hz** en vez de en cada
  frame. Ver § Traza: ruido de `WANTEDCOP join` y coste del bloque.
- [x] Cheat propio de P1 → **`CRAZYCOP`** (20/09 13:26): `FindPlayerPed()->SetWantedLevel(3)`,
  es decir 3 estrellas **sin cometer crímenes**, que es lo que pedía la tarea 4 (medir
  siempre desde el mismo nivel). Añadido **al final** de la cadena de `Pad.cpp` detrás de
  `CRAZYPISTOL`, con el literal `"STJZg\\UJ"` verificado por herramienta nueva
  `tools/cheat-literal-check.mjs` (decodifica la tabla `ccmp()` del propio `Pad.cpp`,
  imprime el literal para un nombre y avisa de nombres repetidos y de literales
  **prefijo** que te robarían el cheat; se autocomprueba reproduciendo los literales ya
  existentes). Dentro del `.wasm` publicado (`grep -a -F`).
- [ ] P3 (opcional) — **bloqueado por DATO, pedido a la sección 1** (20/09):
  `6507 polwintergreen` sigue con **clase `ignore`** en `bootseed/data/default.ide`
  (0 apariciones en el log de hoy), así que la moto no circula y "que la use la
  policía" (`CVehicle::IsLawEnforcementVehicle`, `CarCtrl`) no es comprobable. Las
  coronas ya las hizo la sección 1 (`VICEEXT_POLICE_BIKE_LIGHTS`). Petición concreta
  en § P3 petición a la sección 1; no toco `gta_vc_browser/`.

### P3 — petición a la sección 1 (dato: que la moto circule)

Para poder cerrar P3 (opcional) hace falta **un cambio de dato**, y `default.ide` es
propiedad de la sección 1. Lo que se pide, mínimo y reversible:

1. `6507 polwintergreen`: clase del IDE **`ignore` → una clase de tráfico** (p. ej.
   la de un vehículo raro/policial). Sin esto el tráfico nunca la elige; medido hoy:
   0 apariciones de `6507` en una partida de 13 min.
2. Si entra en tráfico: `carcols.dat` (2 colores, blanco/verde del VCPD) y
   **frecuencia baja** — es una moto policial, no debe salir como un Esperanto.
3. Confirmarlo con su traza (`CARPOOL`/`CARSPAWN`): basta con **una** línea de `6507`.

Mientras el punto 1 no esté, lo demás (policía usándola) no se puede ni medir: no
toco ficheros de datos ni `CVehicle::IsLawEnforcementVehicle` (no son míos).

### P2 — medición (19/09 23:2x local, solo lectura de código y `data/weapon.dat`)

**Las tres funciones y su puerta de entrada** (`Automobile.cpp:3855`,
`Bike.cpp:2109`, `Boat.cpp:1413`). Las tres hacen, en este orden:

1. `CPlayerInfo::m_bDriveByAllowed` (si el script lo apagó, fuera);
2. `CWeaponInfo::GetWeaponInfo(weapon)->m_nWeaponSlot != WEAPONSLOT_SUBMACHINEGUN`
   → fuera (en la moto el literal es `!= 5`, que es el mismo valor: `WEAPONSLOT_SUBMACHINEGUN = 5`);
3. elegir animación y disparar con `weapon->FireFromCar(...)`.

**Qué armas están permitidas hoy** (leído de `bootseed/data/weapon.dat`, columna
final = slot; el enum está en `WeaponType.h`):

| Slot | Armas (en este `weapon.dat`) | ¿Drive-by hoy? |
|---|---|---|
| 3 `HANDGUN` | `Colt45`, `Beretta`, `Python` | **no** |
| 5 `SUBMACHINEGUN` | `Tec9`, `Uzi`, `SilencedIngram`, `Uziold`, `Mp5` | **sí** |
| 4 `SHOTGUN`, 6 `RIFLE`, 7 `HEAVY`, 8 `SNIPER` | `Shotgun2`, `Ak47`, `M16`, `Steyr`, `Gr_launcher`… | no (y así se queda: dos manos) |

**Animaciones** (`AnimationId.h`): coche y barco comparten
`ANIM_STD_CAR_DRIVEBY_{LEFT,RIGHT}` (+ `_LO` para `bLowVehicle`); la moto tiene
`ANIM_BIKE_DRIVEBY_{LHS,RHS,FORWARD}` en el grupo del propio modelo
(`m_bikeAnimType`). Las tres existen y son las que se añaden al clump del
conductor: **no hay que crear animación nueva**.

**Diferencias que importan para la sonda:**

- **Coche y barco**: sólo disparan si la cámara mira de lado
  (`lookingLeft || lookingRight`), y ese estado lo pone `CCam::LookLeft/Right`
  → en el mando es `LeftShoulder2`/`RightShoulder2` (driving:
  `VEHICLE_LOOKLEFT = PADEND/'Q'`, `VEHICLE_LOOKRIGHT = PADDOWN/'E'`). Sin mirar
  de lado, `weapon->Reload()` y ningún disparo.
- **Moto**: además dispara con sólo `GetCarGunFired()` (entonces usa
  `ANIM_BIKE_DRIVEBY_FORWARD`): es la única clase que se puede medir sin tocar
  las teclas de cámara.
- **Pasajeros**: el drive-by del *jugador* sólo existe para el conductor
  (`pDriver` en los tres ficheros). Los NPC de un asiento disparan por otra vía
  (`CPed::IsPedDoingDriveByShooting`, `PedAI.cpp:3708`); no la toco.
- Detalle de coche: si hay pasajero en el asiento 0, el lado derecho no usa la
  animación lateral (para no atravesarlo), y en 1ª persona sí.
- **Tecla de disparo en el coche**: `CPad::GetCarGunFired()` = `NewState.Circle`,
  que en este port llega de `VEHICLE_FIREWEAPON` (`config.h:375` tiene
  `BIND_VEHICLE_FIREWEAPON`): ratón 1, `LCTRL` o `Insert`.

**El detalle que decide el diseño (y que el plan no había visto).** Al *entrar*
en un vehículo, `CPed::RemoveWeaponWhenEnteringVehicle` (`Ped.cpp:4747`) sólo
deja el arma en la mano si es de slot 5; con cualquier otra **borra el modelo**
(`RemoveWeaponModel`), y al salir `ReplaceWeaponWhenExitingVehicle` se lo vuelve
a poner sólo si el arma actual es de slot 5. Es decir: con el drive-by ampliado,
una pistola **dispara** desde el coche pero con la mano vacía, y si el jugador
tiene un arma de slot 5 con balas el motor le cambia de arma al entrar, con lo
que la pistola no se elegiría nunca en la práctica.

**Consecuencia para el bloque**: hacen falta dos cosas más que la condición de
los tres ficheros — (a) que la pistola en la mano siga siendo el arma del
conductor al entrar (dos líneas en `Ped.cpp`, **fichero que no está en la tabla
de propiedad de nadie**) y (b) una forma determinista de tener la pistola
seleccionada en la sonda (hoy `CRAZYTOOLS` da pistola **y** `Uziold` con 150
balas, y el ciclo de armas del port va por teclas del numérico, que en
Puppeteer no llegan sin `location: 3`).

### P1 — línea base medida (20/09, 02:0x local)

**Cómo se midió.** Sonda nueva `gta_vc_browser/tools/hidecops-smoke-test.mjs`
(modo `baseline`): carga el slot 0, teclea `YOUWONTTAKEMEALIVE` (+2 niveles:
0→2, chaos 200), espera 120 s con los policías encima y lee la traza que el
motor manda por `ODTRACES` a `odtrace.log` (`WANTED … lvl chaos cops presence18
seeing near objs`, una línea por segundo, más `WANTEDCHANGE`/`WANTEDCOP`).
Instrumentación añadida en `Wanted.cpp`/`CopPed.cpp`: **solo lee estado**, no
cambia ninguna decisión. Medición PASS (sonda) aunque el nivel no bajara: en
`baseline` el PASS significa "corrida limpia", no "baja el nivel".

**Cifras (vanilla, build `ve8`, 3 policías persiguiendo):**

| Pregunta | Medido |
|---|---|
| Segundos hasta que baja el nivel sin que te vean, con 2 estrellas | **infinitos: no baja nunca.** chaos clavado en 200 durante los 120 s, nivel 2 → 2 |
| ¿Y a nivel 1? | 1 punto de chaos/s **solo** si no hay policía en 18 m: desde el nivel 1 (chaos 70) son **~21 s** hasta 0 estrellas |
| ¿Y a nivel ≥2 aunque no te vean? | nada: `Update()` ni toca `m_nChaos` (`if (m_nWantedLevel > 1) { m_nLastUpdateTime = now; }`) |
| Qué hacen los `CCopPed` | entran en persecución hasta `m_MaxCops` (3 con 2 estrellas; `WANTEDCOP join`), siguen tu posición **viva** y se quedan con `m_objective = 8` = `OBJECTIVE_KILL_CHAR_ON_FOOT`; el más cercano bajó a **14.2 m** y ya no se despegan |
| Vías de escape reales en vanilla | `WANTEDPURGE` (respawn de taller: `Garages.cpp` → `Suspend()`) y muerte/detención (`m_pWanted->Reset()`); en una corrida previa una **detención** bajó el nivel a 0 — no fue decaimiento |
| "¿Te ven?" | vanilla **no lo mira**: solo cuenta policía a menos de 18 m (`WorkOutPolicePresence`), con o sin línea de visión |

**Detalle del motor que condiciona el diseño:** ese bloque de 1 s de
`CWanted::Update()` se ejecuta **en cada frame** mientras haya policía a menos de
18 m (`m_nLastUpdateTime` solo se refresca en la rama de decaimiento). Cualquier
temporizador nuevo tiene que ser **por tiempo**, no por número de llamadas
(medido: ráfagas a ~60 ms entre líneas durante la persecución).

### P1 — cierre: implementado y verificado (19/09, 23:0x local)

**Build medido**: `web/public/build/reVC.wasm` de 21:20 (huella
`23179142@2026-09-20T02:20:33Z`) — el mismo build que usan las otras secciones
no cambia mi código: entre la implementación (21:06) y la corrección de la
sonda no he tocado el motor, así que la evidencia es del código que está en el
árbol.

**Sonda**: `tools/hidecops-smoke-test.mjs` modo `feature` (a pie, sin intento de
coche: ver `VC_HIDECOPS_CAR`). Tres corridas: dos PASS (21:10 y 23:0x) y una
**INCONCLUSA** que me obligó a arreglar el veredicto (abajo). Añadidos: la
latencia se mide con el **reloj del motor** (`t=` de `WANTEDHIDE start/drop`),
no contando muestras (la traza llega en ráfagas), y las líneas de otras
sesiones se cuentan y se ignoran (` tag=`).

**Traza cruda de la corrida PASS** (tag de sesión `692708` en `odtrace.log`,
reloj del motor en ms):

| Línea | t (motor) | Lectura |
|---|---|---|
| `WANTEDCHANGE 0->2 chaos=200` | 3173302 | cheat `YOUWONTTAKEMEALIVE` |
| `WANTEDHIDE start lvl=2` | 3173362 | nadie te ve (línea de visión + 40 m) |
| `WANTEDHIDE seen` + `start` | 3182008 | te vuelven a ver y se reinicia la racha |
| `WANTEDHIDE start lvl=2` | 3190255 | racha buena |
| `WANTEDHIDE drop lvl=2->1 chaos=200` | 3205258 | **15.0 s después** (contrato) |
| `WANTEDCHANGE 2->1 chaos=70`, `WANTEDHIDE end` | 3205258 | `SetWantedLevel(1)` (chaos canónico del nivel 1) |
| `WANTEDCHANGE 1->0 chaos=49` | 3226235 | ya es la regla vanilla de nivel 1 (18 m / 1 chaos-s) |

**Números de la corrida**: 1 bajada, **0 bajadas estando visto con nivel ≥2**,
0 errores de página, `busted` 0, sin purgas ni suspensiones, 5 ×
`WANTEDCOP search-last-known` (policías a pie yendo a la última posición
conocida), capturas en `%TEMP%\vc-hidecops-p1c` (`5s`, `30s`, `65s`, `95s`,
`120s`, `99-final`).

**Lo que la sonda aprendió por el camino** (dos trampas que hay que respetar si
se re-mide):

1. **Contar muestras no es contar segundos.** La traza de `Wanted.cpp` sale
   "a bloques" (se pierden líneas) y las ráfagas engañan: una corrida de 120 s
   dio 54 muestras y otra 118. La latencia se mide con el campo `t=` del motor y
   la racha escondida con el campo `unseen` (que ya es un contador de segundos
   del motor).
2. **Una corrida que no llega a esconderte 15 s no es un FAIL, es
   INCONCLUSA** (código 3, mensaje propio). La tercera corrida entró en modo
   coche, condujo hacia los policías y solo aguantó 7 s escondido; el veredicto
   viejo lo llamaba "la búsqueda nunca se activó", que era falso. Ahora el modo
   coche es opt-in (`VC_HIDECOPS_CAR=1`) y el defecto es a pie.

**Límite conocido (no cerrado)**: los perseguidores **en coche** siguen yendo a
la posición viva del jugador (`CarAI.cpp`, fuera de la tabla de propiedad de
esta sección). La mecánica de esconderse aplica a los de a pie; los coches
siguen igual que vanilla.

**Contrato que implemento (P1, detrás de `VICEEXT_HIDE_COPS`):**

- **Visibilidad**: un policía "te ve" si está a < 40 m **y** tiene línea de
visión libre (`CWorld::ProcessLineOfSight` con edificios/objetos, ignorando
al propio policía y su vehículo). Vanilla no tenía este concepto.
- **Nivel ≤1**: no lo toco (ahí vanilla ya tiene su regla de 18 m / 1 chaos/s).
- **Nivel ≥2**: no baja mientras alguien te vea; **sí baja una estrella cada
  15 s** de estar sin que nadie te vea. Los 5 s de gracia son una **puerta**, no
  un retardo que se sume: exigen llevar 5 s sin ser visto para que el contador
  de 15 s pueda pagar, y como al empezar la racha ya llevas 0 s, la primera
  estrella cae a los **15 s** (medido, no supuesto): 2 → 1 en 15 s y 6 → 0 en
  90 s (una por cada 15 s). Documentado aquí porque el primer borrador decía
  20 s / 95 s y la medida lo desmintió.
- **Comportamiento**: mientras no te ve nadie, los perseguidores **a pie**
van a la **última posición conocida** (`OBJECTIVE_GOTO_AREA_ON_FOOT`, que es
el objetivo de área que ya usa el script) en vez de a tu posición viva. Los
coches policiales siguen persiguiendo como en vanilla: su IA vive en
`CarAI.cpp`, fuera de mi tabla de propiedad (lo anoto como límite).

### P2 — cierre: implementado (20/09, 09:35 local)

**Contrato implementado** (todo detrás de `#define VICEEXT_DRIVEBY_WIDE`, que está
**encendido** en el bloque de `config.h`; apagándolo el código compila y se
comporta exactamente como vanilla, que es lo que usaría la línea base si hay que
re-medirla):

| Qué | Dónde |
|---|---|
| Puerta nueva de los tres drive-by | `CPed::CanDoDriveByWithCurrentWeapon()` (`Ped.cpp`) reemplaza el `m_nWeaponSlot != 5` de `CAutomobile`/`CBike`/`CBoat`. Sin el define devuelve **solo slot 5** (idéntico a vanilla); con él añade **slot 3 (pistola) y pide `m_nAmmoTotal > 0`**. Rifle/escopeta/lanzacohetes siguen fuera (dos manos, el mod tampoco los da). |
| El arma se queda en la mano | `CPed::KeepsWeaponInHandWhileDriving()` + `RemoveWeaponWhenEnteringVehicle`/`ReplaceWeaponWhenExitingVehicle`: con el define, si el jugador entra con una pistola (y `m_bDriveByAllowed`) **no se borra el modelo** ni al entrar ni al salir (antes: mano vacía). Sin el define, las dos funciones son vanilla. |
| Cheat de prueba | `WeaponCheat5()` / `"CRAZYPISTOL"` al final de la cadena de `Pad.cpp` (sección 2): da **solo** la Beretta y la deja elegida a pie. Necesario porque `CRAZYTOOLS` da también `Uziold` (slot 5) y el motor cambia de arma al entrar al vehículo, con lo que la pistola no se elegiría nunca. Literal codificado verificado (`OT[TVk\aB]P`, autotesteado contra la tabla de `CRAZYTOOLS`). |
| Traza (para confirmar sin sondas) | `Ped.cpp`: `DRIVEBY state` (1/s conduciendo: clase, arma, slot, ammo, `fire`, `lookL/R`, modelo, velocidad), `DRIVEBY shot` (**uno por disparo que sale de verdad**, con la animación usada), `DRIVEBY enter|exit` (con `outcome=keep-weapon|remove-model|switch-smg|restore-stored|add-model|keep-smg`). |

**Ficheros tocados** (respetando la tabla de propiedad): `src/core/config.h` (define),
`src/vehicles/{Automobile,Bike,Boat}.cpp` (míos), `src/core/Pad.cpp` (cheat, al final
como manda la regla) y **`src/peds/Ped.{h,cpp}`**, que **no estaban en la tabla de
nadie**: el plan ya decía que hacían falta las dos líneas; se ha hecho con 5
declaraciones + 3 funciones y **todo el comportamiento nuevo detrás del define**, así
que sin él `Ped.cpp` es vanilla. Avisado en `.agents/HISTORIAL.md` por si la
coordinación quiere fijar la propiedad de `Ped.*`.

**Build medido**: `gta_vc_browser/web/public/build/reVC.wasm` de **09:37:29**
(23.216.990 bytes) + `reVC.data` de 09:37:09; contiene las tres cadenas de traza y
el literal del cheat (comprobado con `grep -a -F` sobre el `.wasm`).

**Cómo se verifica ahora (sin sondas pesadas, tal como pidió el jugador el 20/09):**
1. Jugar una partida normal. Teclear `CRAZYPISTOL` (deja la pistola en la mano).
2. Subirse a un **coche** y disparar **mirando de lado** (`Q`/`E` + gatillo del
   vehículo: `Insert` o `CTRL izq`); y a una **moto** (ahí basta el gatillo, dispara
   hacia delante). Vale también un barco.
3. `node gta_vc_browser/tools/driveby-log-check.mjs` → resume la sesión y dice si la
   pistola dispara desde cada clase y si la SMG sigue igual. Es solo lectura de
   `odtrace.log` (0 CPU): no abre navegador ni toca la partida.

**Qué evidencia da el contrato en los logs**: con la pistola, `DRIVEBY enter …
slot=3 … outcome=keep-weapon` + `DRIVEBY shot … veh=car|bike slot=3
anim=left|right|left-lo|right-lo|forward|lhs|rhs`; con el arma de slot 5 sigue
apareciendo `outcome=switch-smg` y disparos con `slot=5` (no regresión). En un build
sin el define lo que se ve es `outcome=remove-model` y **cero** disparos con `slot=3`
(pistola rechazada): esa es la línea base, medible con el mismo comando cambiando el
build.

**Pendiente / límites:**
- La **sonda** `tools/driveby-smoke-test.mjs` (modos `baseline`/`feature`, con
  instantánea del build para ser inmune a las recompilaciones de los otros agentes)
  está escrita y sin ejecutar: es la confirmación "de laboratorio" y se puede correr
  cuando la máquina esté libre.
- **P3 sigue bloqueado**: `6507 polwintergreen` continúa con clase `ignore` en
  `gta_vc_browser/bootseed/data/default.ide` (medido hoy), así que la moto **no
  circula** y el bloque sigue en "perseguir un coche que no aparece". Las coronas ya
  las hizo la sección 1; lo que queda (que la use la policía:
  `CVehicle::IsLawEnforcementVehicle`, `CarCtrl`) vive en ficheros que no son míos.

### P1 — partida real leída por log (20/09, 10:03→10:12 local)

Primera vez que la traza de P1 corre en una partida de verdad. Verificador nuevo:
`node gta_vc_browser/tools/hidecops-log-check.mjs` (sólo lee `odtrace.log`, no abre
navegador; se le puede pasar `--file=`/`--tag=`).

**Cifras de la sesión** (`JS build=2026-09-20-ve10`, tag 0 = partida libre):

| Dato | Valor |
|---|---|
| Duración de la traza | 455 s (1 línea/s) |
| Nivel máximo | 4 (cambios `0->1, 1->0, 0->1, 1->2, 0->1, 1->2, 2->3, 3->4`) |
| Segundos a nivel ≥ 2 | 75 s |
| Segundos a nivel ≥ 2 **sin que te viera ningún perseguidor** | 13 s — **los 13 con policía a 18 m** (`presence18=1`) |
| Episodios `WANTEDHIDE` | `start=0 seen=0 end=0 drop=0` |
| Otras vías vistas | 1 respray (`WANTEDPURGE … lvl=2 cops=0`) |

**Lectura**: el contrato no queda contradicho, pero **tampoco confirmado en
partida**: para que empiece el conteo de 15 s hay que estar sin que te vea ningún
policía (ni de patrulla, `AnyCopSeesPlayer` barre también la calle), y en esa sesión
siempre hubo una unidad a la vista o a 18 m. La confirmación de laboratorio sigue
siendo la de § P1 cierre (sonda `feature`, bajada a los 15.0 s).

**Lo que se aprendió para el verificador**: la primera versión acusaba al build
("falta `VICEEXT_HIDE_COPS`") en cuanto había nivel ≥ 2 sin `start`; ahora separa
"sin que te vean" de "sin policía cerca" (`presence18`) y sólo avisa si hay ≥ 10 s
ciego-y-solo, que es el caso en el que el motor sí debería haber empezado.

### P2 — corrección por partida (20/09, 10:04 → build 10:19 con `rev=2`)

La partida del jugador (misma sesión, tag 0) dejó 193 líneas de P2: 137 s conduciendo
(coche + moto), 48 disparos y 4 entradas/salidas de vehículo. Las salidas delataron el
fallo:

```
DRIVEBY enter … wep=24 slot=5 ammo=270 rev=1 outcome=switch-smg      (x4)
DRIVEBY exit  … wep=48 slot=3 ammo=100 rev=1 outcome=restore-stored  (x4)
```

**Causa**: en `CPed::RemoveWeaponWhenEnteringVehicle` la rama vanilla `switch-smg` se
evaluaba **antes** que la mía `keep-weapon`, así que llevar una SMG en el inventario
bastaba para que la pistola en la mano se cambiara al entrar. Es exactamente el riesgo
que ya avisaba § P2 medición ("si el jugador tiene un arma de slot 5 con balas el motor
le cambia de arma al entrar, con lo que la pistola no se elegiría nunca en la
práctica"): estaba medido y aun así se coló en la implementación.

**Arreglo** (`src/peds/Ped.cpp`, una rama): si el jugador lleva **pistola (slot 3) con
balas** en la mano y `m_bDriveByAllowed`, esa pistola manda sobre el cambio automático
a la SMG (no hay nada que guardar ni que cambiar). Sin el define, sin balas o con el
script apagando `m_bDriveByAllowed`, la rama vanilla sigue igual.

**No-regresión ya medida en esa sesión** (la SMG sigue intacta): moto
`anim=lhs x19 / rhs x8 / forward x13`, coche `anim=right x8`, munición bajando de 269 a
222 entre los 48 disparos, y `outcome=switch-smg` en las 4 entradas cuando se entra
con la SMG.

**`rev=2`**: nueva marca en la traza `DRIVEBY enter|exit`. La etiqueta `JS build=`
**no cambia cuando sólo se recompone el wasm**, así que sin la marca un log no puede
probar qué binario corrió. El verificador la imprime
(`P2 rev=1 (sin el arreglo)` / `P2 rev=2 (con el arreglo de precedencia)`).

**Build publicado**: `reVC.wasm` de 10:19:04 (23.228.868 bytes) con `rev=%d outcome=%s`
dentro; `Ped.cpp.o` compilado a las 10:11:33, después del arreglo y antes del enlace.

**Cómo repetir la confirmación**: recargar la página (para cargar el wasm nuevo), jugar
con `CRAZYPISTOL`, subirse a un coche **con la pistola en la mano** y luego
`node gta_vc_browser/tools/driveby-log-check.mjs`. PASS = `rev=2` y `DRIVEBY enter …
slot=3 … outcome=keep-weapon` + disparos `slot=3` con animación lateral. **Hecho**:
ver § P2 confirmación en partida real.

### P2 — confirmación en partida real (20/09 12:57→13:10, `rev=2`) → **PASS**

Segunda partida con el build arreglado. Comando (ojo: el log **rota por sesión**, así
que la partida que acaba de terminar suele estar en `odtrace.prev.log`; se le puede
apuntar con `VC_ODTRACE=` o `--file=`):

```bash
VC_ODTRACE=$PWD/gta_vc_browser/web/odtrace.prev.log node gta_vc_browser/tools/driveby-log-check.mjs
```

| Dato | Valor |
|---|---|
| Sesión | 17:57:59Z → 18:05:16Z (tag 0 = partida libre) |
| Build | `JS build=2026-09-20-ve10`, `P2 rev=2` |
| Conduciendo | 154 s (coche modelo 210 + moto 6506) |
| Disparos | 34, **todos pistola (arma 17)** desde coche: `anim=left` x24, `anim=right` x10 |
| Entradas | `pistola → keep-weapon` x4; `SMG → switch-smg` x1 |
| Salidas | `pistola → keep-weapon` x4; `puños → restore-stored` x1 |

**Lectura**: el contrato se cumple en conducción real: con la pistola en la mano se
dispara desde el coche con animación lateral (no la de conducir) y el arma **no se
cambia** al entrar (4 entradas `keep-weapon`), que era justo el fallo de la partida
anterior. La vía vanilla sigue viva: entrar con la SMG da `switch-smg`.

**Límite honesto**: los 34 disparos son desde **coche**; en esta sesión no hubo
disparos de pistola desde moto/barco (la moto se usó con la SMG de la vía vanilla). La
sonda de laboratorio (`driveby-smoke-test.mjs`) sigue escrita y **sin ejecutar** por la
petición del jugador de no lanzar Chrome/swiftshader.

### P1 — segunda partida real (20/09 12:57→13:06) + línea base de nivel 1 en vivo

| Dato | Valor |
|---|---|
| Sesión | 17:57:30Z → 18:02:51Z (tag 0), `JS build=2026-09-20-ve10` |
| Duración de la traza | 273 s (1 línea/s) |
| Nivel máximo | **1** (cambios `0->1, 1->0`) |
| Episodios `WANTEDHIDE` | `start=0 seen=0 end=0 drop=0` (correcto: el bloque sólo actúa a nivel ≥ 2) |
| Persecución observada | `WANTEDCOP join` (slot 0, `cops=1/1`), policía viéndote a 4–22 m, `listed=1` |

**Línea base de nivel 1 medida en vivo** (vanilla puro; confirma que el bloque **no**
debe tocar el nivel 1, tal como dice su contrato):

- `WANTEDCHANGE 0->1 chaos=57` a t=4525422 ms; último `seeing=1` a t≈4530455 ms.
- `WANTEDCHANGE 1->0 chaos=49` a t=4552956 ms.
- Conclusión: **~21 s** desde el último avistamiento, y la estrella cae exactamente al
  cruzar `chaos` de 50 a 49 (decremento de 1/s mientras no hay policía a 18 m).

**Lo que sigue faltando para P1**: una confirmación en partida **a nivel ≥ 2**
(2-3 estrellas, esconderse ~20 s sin que te vea nadie ni haya patrulla a 18 m).
Mientras tanto, la evidencia del bloque a nivel alto es la de laboratorio
(§ P1 cierre, bajada a los 15.0 s).

### Traza: ruido de `WANTEDCOP join` (churn de taller), coste del bloque y `rev=4`

Hallazgo al leer la partida de hoy: **140 líneas `WANTEDCOP join` en pocos segundos**,
con `slot=0` y `cops=1/1` repetidos cada ~16 ms. **No es un fallo del port**:

- `Garages.cpp:410-416`: **mientras conduces** dentro de `DISTANCE_TO_ACTIVATE_GARAGE`
  de un taller, el motor llama a `CWorld::CallOffChaseForArea(...)` **en cada frame**.
- Eso llama a `CCopPed::ClearPursuit()` (una vez por `GetCurrentScanCode()`) y en el
  frame siguiente `CCopPed::CopAI` lo vuelve a meter en la persecución
  (`m_CurrentCops(0) < m_MaxCops(1)` → `SetPursuit(true)`): unión/soltado a ~60 Hz.
- Efecto real: en el radio del taller la persecución no puede progresar (es la
  intención de la zona). En el log solo se veía como ruido.

**Arreglo (solo instrumentación)**: la línea se filtra a **1/s** y cuenta la ráfaga
(`burst=N` = uniones desde la última línea impresa). Y se añade **una línea por
sesión**:

```
WANTEDHIDEINIT tag=0 rev=4 star_ms=15000 grace_ms=5000 sight_m=40 sweep_ms=200 t=…
```

Es lo único que prueba **qué `.wasm` jugó**: la etiqueta `JS build=` no cambia cuando
sólo se recompone el wasm (el mismo problema que resolvió `rev=2` en P2). El
verificador imprime la revisión (con las constantes que corrieron de verdad) y avisa
si es < 3.

**Coste del bloque acotado (`rev=4`)**: `CWanted::UpdateHiding()` corre **en cada
frame** a nivel ≥ 2 y su paso 2 (`AnyCopSeesPlayer`) recorre **todo el pool de peds**
(140 slots) y lanza un `CWorld::ProcessLineOfSight` por policía a menos de 40 m. El
contrato es de segundos (5 s de gracia + 15 s por estrella), así que ese barrido pasa a
refrescarse cada `VICEEXT_HIDE_SWEEP_MS` = **200 ms** (5 Hz); los perseguidores
(`m_pCops`, ≤ 10) siguen mirándose cada frame, que es lo que decide de verdad si te
están viendo. Peor caso del filtro: 200 ms de decisión "vieja", muy por debajo de la
tolerancia del verificador (2.5 s). El temporizador es un `static` local a propósito:
no se añaden campos a `CWanted` (la estructura se copia entera en los guardados de
misión). **No medido en partida a nivel ≥ 2 todavía** (no ha habido ninguna): la
justificación es de coste por lectura de código, no una medida.

**Build publicado**: `reVC.wasm` de **13:20:20** (23.239.121 bytes, `reVC.js` de la
misma hora) con `WANTEDHIDEINIT tag=%u rev=%d … sweep_ms=%d` y `burst=%d` dentro
(comprobado con `grep -a -F` sobre el `.wasm`). Sustituye al de 10:19 (que era
`rev=2`; el de P2 sigue estando: `rev=%d outcome=%s` y `keep-weapon` siguen en el
binario).**Para que la traza nueva salga hay que RECARGAR la página** y volver a
entrar en partida (la línea `WANTEDHIDEINIT` la imprime `CWanted::Initialise()`, es
decir al crear el jugador / cargar partida).

---

# Partida del jugador 20/09 18:42→18:47Z (13:42→13:47 local) — informe y plan de la sección 2

El jugador jugó con el build servido (`JS build=2026-09-20-ve12`, `data=ve10`) y
reportó una lista larga de fallos, **válida para las tres secciones**. Este apartado
separa lo mío (policía / conducción / drive-by), lo diagnostica con el log y deja lo
ajeno con su dueño y su evidencia (para que cada sección lo recoja).

## 1. Qué pasó de verdad en el log (cronología reconstruida)

| Hora (Z) | Hecho |
|---|---|
| 18:42:35 | Arranque. `JS build=ve12`, `data=ve10`. **Sesión nueva, log rotado** (la anterior, de menú, quedó en `odtrace.prev.log`). |
| 18:43:01 | Carga de partida (`loaded pos=223.1,-1276.7,12.0 level=1`), `WANTEDHIDEINIT tag=0 rev=4 … t=4305365`. |
| 18:43:04→:22 | `STREAM preload … MOB_52A…H.WAV` (audio de misión) — sin `cut=1` en `WEBHB` (no fue cinemática). |
| 18:44:27→:51 | Nivel 1, un policía a pie pegado (`near=1.5`), `STREAM preload … BUST_01.WAV` → **el jugador fue DETENIDO** (no crash): la traza `WANTED tag=` de esa sesión **termina aquí**. |
| 18:45:04 | Carga de partida desde el menú (`CheckSlotDataValid slot=1`, `GTAVCsf2.b`) → **segunda sesión en el mismo fichero** (por eso hay dos `WANTEDHIDEINIT` y el `t=` del motor **retrocede**). |
| 18:45:10→18:47:08 | Niveles 1→2→3 (`WANTEDCHANGE 2->3 chaos=561 t=3745038` a las 18:46:29). La P1 nueva entra en acción: `WANTEDHIDE start/seen` en ráfaga cada 200 ms (siempre había un policía viéndote). |
| 18:46:29.871 | **Segundo exacto del `2->3`**: el motor pide los activos del nivel 3 (`Streaming.cpp`, `AreMiamiViceRequired()` y `NumOfHelisRequired()`): `ODSHORT open-fail …/vice1.txd`, `chopper.txd`, `vice2.txd`, `vicechee.txd` + `CARFAIL ch=1 status=254 intentos=0/1`, y acto seguido `CARLOAD model=165 clase=-1 (ignore)` y `CARLOAD model=236 clase=-1 (ignore)`. |
| 18:47:25.874 | `VICEEXT gastank sin-dummy model=165` → **un disparo del jugador impactó en el helicóptero policial** (modelo 165 = `chopper`). |
| **18:47:31.874** | **Última línea del log** (todo normal: 60 fps, `FPHASE`, `CORONAREND`, `CARRATE`). El log se corta **exactamente 6.000 s** después de ese impacto, sin degradación previa (no es un cuelgue por CPU ni OOM del tab: no hay subida de `heap0`, ni `JS ODCAP` raro, ni `hitch`). |

**Dos consecuencias de método (importantes para las tres secciones):**

1. **El reloj del motor retrocede al cargar partida.** `CTimer::GetTimeInMilliseconds()`
   viene del guardado (sesión A: `t≈4.4M`; sesión B: `t≈3.66M`). Cualquier guarda de
   traza que compare `now < last + 1000` con un `static` **deja de imprimir para
   siempre**: eso pasó con mi `WantedTraceState` (0 líneas `WANTED tag=` en toda la
   sesión B, mientras `WANTEDHIDE`/`WANTEDCHANGE` sí salían). Arreglado en este bloque
   (§ P6). Aviso para las otras secciones: **sus trazas con filtro temporal por
   `static` sufren lo mismo** si el jugador carga partida (revisar `CARPED`/`SIGHT`/
   `CAM1P`/`IFPFILE`…).
2. **No hay crash en el log a las 18:44** (fue una detención). El único corte duro es
   el de 18:47:31. La capa web **sí** vacía el log al primer error JS
   (`window.addEventListener('error'/'unhandledrejection', onAnyError)` → `flush(true)`),
   pero **el texto del error va a la consola del navegador, no al log**
   (`console.log('[FATAL] …')`). Por eso un corte limpio = aborto/ excepción del wasm,
   y **no se puede identificar la función desde el log tal como está**.

## 2. Mis dos fallos reales, con su causa exacta en el código

### B1 — «con la pistola desde el coche salen balas como si fuera de SMG» (P2, mío) ✅ CONFIRMADO

El jugador tiene razón y son **dos** causas independientes, las dos visibles en el
código que ya está en el árbol:

| # | Dónde | Qué pasa |
|---|---|---|
| a | `Automobile.cpp` ~4016, `Bike.cpp` ~2188, `Boat.cpp` ~1469 | Los tres fijan la cadencia a mano: `weapon->m_nTimer = CTimer::GetTimeInMilliseconds() + 70;`. Es el valor **del SMG de vanilla** (14 disparos/s): con una pistola, dispara como una metralleta. |
| b | `Weapon.cpp::FireFromCar` | El sonido se pide **al audio del VEHÍCULO**: `DMAudio.PlayOneShot(shooter->m_audioEntityId, SOUND_WEAPON_SHOT_FIRED, 0.0f)`. En `AudioLogic.cpp` (rama `params.m_pVehicle`, ~3500) ese evento para un coche **no** mira el arma del conductor: elige `SFX_UZI_LEFT` (o la variante del slot 5). Es decir: **el sonido es literalmente el del Uzi/SMG**, con cualquier arma. La rama del PED (~4589) sí mira `ped->GetWeapon()->m_eWeaponType` y daría `SFX_COLT45_LEFT` para la Beretta/Colt45. |

**Referencias de la comunidad que hacen exactamente esto** (buscadas hoy, para no
diseñarlo de cero): el plugin **«Manual Driveby»** (SpitFire, libertycity 190370) anuncia
en su ficha *«Custom animations for every vehicle (bike, boat, car) — Ability to switch
between Pistols and SMGs — **Correct sounds for each weapon** — **Settings from
weapon.dat**»*; y **«Manual Driveby (VC)»** (BirbsLeHecker, libertycity 213323, psdk,
reescrito el 12/09/2026) lista entre sus arreglos *«**custom weapon support with proper
fire rates**, **no more rapid fire bugs**»*. O sea: la solución que se espera es
**cadencia de `weapon.dat` + sonido del arma**, no un número fijo.

### B2 — «carro de la policía sin sirena» 🟡 PARCIAL (falta identificar el vehículo de la foto)

Barrido de datos + código (`default.ide` del bootseed, `Vehicle.cpp`):

| Vehículo | `UsesSiren()` | Coronas de sirena |
|---|---|---|
| `police` (156), `enforcer` (157), `fbiranch` (220), `vicechee` (236), `ambulan`, `firetruck` | sí | **sí** (`Automobile.cpp`, switch por modelo) |
| **`fbicar` (147, «FBI Washington», flags=7 = law enforcer)** | **sí** | **NO** ← hueco real: sirena que suena y despeja tráfico, pero **sin coronas** |
| `predator` (160, lancha policial) y `chopper` (165, heli) | `UsesSiren()`: sí en ambos | **NO** en el barco; el heli usa otro camino (`Heli.cpp`) |
| `mrwhoop` (153) | sí (es el camión de los helados) | no (tiene melodía) |

El **caso limpio y comprobable** es `fbicar`: es vehículo policial en los datos
(`flags=7`), el motor ya lo trata como sirena (`CarAI::MakeWayForCarWithSiren`,
`UsesSiren()`), y no tiene ni una corona. Lo implemento (B5). Si la foto del jugador
era de un **coche patrulla corriente** (`police`), entonces el fallo es otro y necesito
que me diga cuál era (lo pregunto al final del informe).

### B3 — «con 3 estrellas se crasheó el juego» 🔴 EVIDENCIA, SIN CULPABLE TODAVÍA

Lo que el log **sí** demuestra:

- El corte coincide con la secuencia del **helicóptero policial**: el jugador le
  disparó a las 18:47:25.874 (`VICEEXT gastank … model=165`) y el log muere 6.000 s
  después, con el juego a 60 fps justo antes (aborto, no cuelgue). En `Heli.cpp`, un
  heli tocado pasa a `HELI_STATUS_SHOT_DOWN` y `m_nExplosionTimer = now + 10000`; la
  "primera parte" de la explosión va a +3 s y la final (con `CWorld::Remove(pHelis[i]);
  delete pHelis[i];`) a +10 s. **Ese `delete` con la tripulación (policías) dentro y
  `pHelis[i] = nil` es el punto que hay que mirar primero**, más aún sabiendo que el
  `CWorld::Remove` de una entidad no limpia `CWanted::m_pCops` (solo lo hace
  `CCopPed::ClearPursuit`, que sí está en `~CCopPed`): si algún día el ped sobrevive al
  vehículo, queda un puntero a un vehículo liberado.
- El otro sospechoso del mismo minuto es el **estallido de activos del nivel 3**: en el
  segundo exacto del `2->3` el motor pide `chopper`, `vicechee` y **8 skins `vice1..8`**
  (`Streaming.cpp`), y esas peticiones **fallan al abrir** (`ODSHORT open-fail`, con
  recuperación posterior: los ficheros existen y están en el manifiesto). Ese es el
  camino de la sección 1 (capa on-demand) y del `Streaming` del port.

Lo que **no** se puede saber desde el log, y cómo conseguirlo barato:

1. **El texto del fallo ya llega al log**: lo ha implementado la **sección 1 a las
   14:07** en `web/lib/index.js` (`pushFatal` → líneas `JSERR …` / `ENGERR …` en la cola
   de trazas, con `flush(true)` síncrono antes de morir la página). Era exactamente lo
   que esta sección iba a pedir: **ya no hay que pedirlo**. A partir de ese build, un
   crash deja su motivo escrito en `odtrace.log` y el `FPSLOG` ausente marca el segundo.
2. Por tanto, la acción para cerrar B3 es del jugador: **recargar (Ctrl+Shift+R) y
   repetir 3 estrellas → derribar el helicóptero policial**; luego
   `grep -a "JSERR\|ENGERR" gta_vc_browser/web/odtrace.log` da el motivo real.
3. Mi parte del 3-estrellas que **sí** puedo tocar: la auditoría de P7 (abajo) y dejar el
   camino del nivel 3 sin dependencias nuevas. Lo demás (`Heli.cpp`, `Streaming`,
   on-demand) es de otras secciones: queda anotado aquí con la evidencia.

## 3. Lo que NO es mío (reparto para las otras secciones, con lo que dice el mod)

Recuperé la **lista oficial de features** del mod (libertycity 167639, «Vice Extended
(October 2025 Update)», que es el `features.ini` v2510). Sirve de contrato para todos:

| Queja del jugador | Dueño | Lo que dice el mod / referencia |
|---|---|---|
| «no carga ninguna cinemática y el juego se crashea» | 1 (on-demand/Streaming) + 3 | Ligado al punto 2 de arriba: hace falta el `[FATAL]` en el log. |
| «texturas que no cargan o cargan en la resolución más baja por el LOD» | 1 | Regresión del cargador on-demand: los `ODSHORT open-fail` son aplazamientos que **deben** recuperarse (`ODSRECOVER`). En esta partida: 35 fallos, 12 recuperaciones impresas (tope de líneas). Candidato: el cambio de hoy en `ChooseCarModelToLoad`/`CdStreamPosix`. |
| `V` no hace nada (1ª persona) | 3 (C1) | v1.5 «First-person view»; el mod original y el plugin «First Person View for VC» usan la **tecla de vista** (V). La traza `CAM1P` de la sesión tiene `tog=0` en todas las líneas: **el conmutador nunca se pulsó** (o la tecla no llega). |
| la mira se ve siempre, debería verse solo al apuntar | 3 / 1 (D6) | No está en la lista del mod; es criterio del jugador sobre `VICEEXT_WEAPON_SIGHTS`. |
| «no puedo caminar agachado» | 3 (nuevo) | **No está en el mod** (v2510). Referencia real de la comunidad: **«SA Crouch Movement»** (lomonosov, libertycity 227457): agacharse, rodar y moverse agachado como en SA. |
| «no puedo correr con armas de 2 manos» | 1/3 (`VICEEXT_SPRINT_HEAVY`) | v1.5 «Sprint with heavy weapons». Referencias: «Sprint With Two-Handed Weapons» (BirbsLeHecker) y «Sprinting With Two Handed Weapons [VC]». El define está encendido: si no se nota, hay que ver por qué no entra. |
| «no puedo apuntar con la escopeta» | 3 | v1.0 «Changed aiming system» / v1.5 «Movement while aiming». Referencias: «Manual Aiming v1.5» (DimikVRN), «Move While Shooting». |
| «no puedo nadar… solo cae Tommy al vacío» | 3 | v1.0/v3.0 «Swimming. Swimming in water **without the risk of dying**» (y conmutable en `features.ini`). Referencias: «Sprint With All Weapons + Swimming» (Tommy_Vercetti102, 08/2026: nada y sale del agua como en VCS, se ahoga con salud < 5) y «The ability to swim…» (2021). **El detalle de «cae al vacío» hay que medirlo: puede ser que al entrar al agua no haya colisión con el agua y el ped caiga fuera del mundo.** |
| slot extra al cargar partida; el autoguardado debería ir primero, llamarse «autosave» y no poder sobrescribirse | 3 (C2) | v2.5 «Autosave after completing a mission» + «Saving anywhere». |
| autocentrado de cámara solo en vehículos (nunca a pie) | 3 | Ya tiene plan propio: `.agents/plans/camara-libre-autocentrado.md`. |
| «al darle a la R no recarga, no hace nada» | 3 (C3.1) | v2.5 «Reloading a weapon on the key». `VICEEXT_MANUAL_RELOAD` está encendido y en la partida anterior dejó `VICEEXT reload done clip=17/17`… pero esa línea era del camino **automático** (la sección 3 ya lo detectó y añadió `manual=1`). En esta partida **no hay ni una línea `reload`**: ni manual ni automático. Referencia: «Real Reload», «GTA 3 - Reloading (CLEO SCRIPT)», «Manual Aiming v1.5» (recarga con R). |
| coches que arden en vez de **explotar** al disparar al depósito | 3 (C3.3) | El mod dice literalmente «**Gas tank. When shot, the car explodes**»: la implementación actual (`m_fHealth = 250` → llama → explosión a los 5 s) **no cumple el contrato**; hay que explosionar ya. |
| «se ve un slot por fuera al cargar partida» | 3 | Igual que el autoguardado (C2). |

## 4. Plan de esta sección (bloques de esta partida)

| Bloque | Qué | Ficheros | Veredicto |
|---|---|---|---|
| **P4** | drive-by por arma: cadencia de `weapon.dat` (`m_nFiringRate`) en vez del `+70` fijo, y **sonido del arma** (ped) para las pistolas, dejando el sonido del vehículo para el SMG (paridad vanilla) | `Automobile.cpp`, `Bike.cpp`, `Boat.cpp`, `Weapon.cpp` (helper compartido, detrás del define) | traza `DRIVEBY shot … ms=` |
| **P5** | sirenas que faltan: coronas para `fbicar` (147); revisar `predator` (barco) | `Automobile.cpp` / `Boat.cpp` | `CORONAREND` + captura del jugador |
| **P6** | robustez de mi instrumentación: guardas de tiempo que **no** se rompen al cargar partida (reloj que retrocede) | `Wanted.cpp`, `CopPed.cpp` | `WANTED tag=` vuelve a salir tras cargar |
| **P7** | auditoría de seguridad de P1 a nivel 3 (mio): que ninguna ruta nueva pueda tocar un puntero muerto | `Wanted.cpp`, `CopPed.cpp` | revisión + comentario |

**P3 sigue bloqueado por dato** (`6507 polwintergreen`, clase `ignore`): pedido a la
sección 1, sin cambios.

## 5. P4–P7 implementados (20/09 ~14:0x) — qué lleva el código

### P4 — drive-by por arma (mis tres ficheros + el helper compartido)

- `CWeapon::GetDriveByShotDelay()` (`Weapon.h`/`Weapon.cpp`): devuelve **70 ms para el
  slot 5** (SMG, vanilla exacto) y, con `VICEEXT_DRIVEBY_WIDE`, la cadencia real del arma
  (`CWeaponInfo::m_nFiringRate`, que **sí** es una cadencia en ms: sale de la ventana de
  animación `(m_fAnimLoopEnd - m_fAnimLoopStart) * 900`; Colt45/Beretta **210 ms**,
  Python **600 ms**, Uzi 90 ms), con suelo de 70 ms. Los tres `DoDriveByShootings`
  (`Automobile.cpp`, `Bike.cpp`, `Boat.cpp`) usan ese valor en vez del `+ 70` literal.
  Decisión documentada: se mantiene **disparo mantenido** (no "uno por clic"): con
  210 ms la pistola deja de parecer una metralleta sin cambiar el contrato de vanilla.
- Sonido por arma (`Weapon.cpp`, `FireFromCar`): con el bloque encendido y un arma que
  **no** es de slot 5, el evento `SOUND_WEAPON_SHOT_FIRED` se pide al **audio del PED**
  (cuya rama de `AudioLogic` elige la muestra por `m_eWeaponType`: `SFX_COLT45_LEFT` para
  Colt45/Beretta) en vez de al del vehículo (que da siempre `SFX_UZI_LEFT`). El SMG
  mantiene la vía vanilla.
- Prueba en el log: `DRIVEBY shot … slot=3 anim=left delay=210 ammo=…`. El verificador
  `tools/driveby-log-check.mjs` ya lo lee y **avisa si ve `delay=70` con slot 3**.

### P5 — la sirena que faltaba

`MI_FBICAR` (147) añadido al grupo de coronas de `fbiranch`/`vicechee` en
`CAutomobile::ProcessControl`. Límites anotados en el propio código: `predator` (lancha)
y `chopper` (helicóptero) **siguen sin coronas** (no pasan por esa función; el heli usa
`Heli.cpp`, que no es de esta sección). Si la foto del jugador era de un `police`
corriente, el diagnóstico es otro y hace falta su confirmación.

### P6 — la instrumentación ya no se queda muda al cargar partida

Las tres guardas de "una línea por segundo" de mi sección (`Wanted.cpp`:
`WantedTraceState` y el barrido de visibilidad de `UpdateHiding`; `CopPed.cpp`: la traza
`WANTEDCOP join`) **reanclan cuando el reloj del motor retrocede** (cargar partida). Sin
esto, la sesión B de esta partida no imprimió **ni una** línea `WANTED tag=`.

### P7 — auditoría de P1 a nivel 3

- `UpdateHiding()` ahora sale si `FindPlayerPed() == nil` **antes** de llamar a
  `FindPlayerCoors()` (que desreferencia al jugador sin comprobarlo, `PlayerInfo.cpp`).
  Es la única ruta nueva de P1 que corría en cada frame, en cualquier estado del juego.
- Revisado el resto del camino nuevo: la lista de perseguidores se limpia en
  `~CCopPed` (vía `ClearPursuit`) y en `CWanted::ResetPolicePursuit`; el hook de
  `CopPed` solo actúa `m_bIsInPursuit && !bInVehicle` (los que van en vehículo o
  helicóptero quedan fuera); el barrido del pool solo lee. **No he encontrado ninguna
  ruta nueva que pueda tocar un puntero muerto**; el sospechoso del crash sigue siendo el
  `delete` del helicóptero (`Heli.cpp`, fuera de mi tabla) y el estallido de activos del
  nivel 3 (sección 1).

### Estado del build de este bloque

Mis 7 objetos compilan (`Weapon.cpp`, `Ped.cpp`, `Automobile.cpp`, `Bike.cpp`,
`Boat.cpp`, `Wanted.cpp`, `CopPed.cpp`). **El enlace no se ha podido publicar todavía**
por un error en vuelo de la **sección 3** (`src/core/Cam.cpp:5209: no member named
'GetLookLeftRight' in 'CPad'`), ajeno a esta sección: se reintenta en cuanto su árbol
compile y entonces el `.wasm` servido llevará P4-P7.

---

# 5ª partida del jugador (21/09) — lo entregado y lo que queda

El jugador confirmó **P2 funcionando** y dio una lista nueva. Entregado en el build
publicado de **08:04:37** (105/105 + enlace, 23.270.509 bytes; las cadenas nuevas se
comprobaron con `grep -a -F` sobre el `.wasm`):

| Qué | Dónde | Qué se hizo |
|---|---|---|
| **P1 `rev=6`** — esconderse no hacía nada | `src/core/Wanted.cpp` (mío) | El "te ve" ahora exige que el policía **mire hacia ti** (±75°: `WANTED_SIGHT_COS 0.26`, `DotProduct` con `GetForward`) y el avistamiento tiene que **durar 400 ms** (`VICEEXT_HIDE_SEEN_MS`) para cortar la búsqueda. Causa medida en la partida del 20/09: 1.205 `seen` encadenados cada 200 ms (racha máxima 9 s de los 15 que hacen falta) → 0 estrellas bajadas en toda la partida con 649 s a nivel ≥ 2. `WANTEDHIDEINIT` ahora imprime `seen_ms=` y `sight_dot=`. |
| **D6c** — la mira seguía viéndose al bajarse del coche | `src/renderer/Hud.cpp` (bloque D6 de la sección 1, avisado) | `ViceExtWantsSight()` ya no se conforma con `FindPlayerVehicle() != nil` (seguía devolviendo el vehículo durante la animación de salida): ahora exige `m_nPedState == PED_DRIVING` (o una vista que ES la mira). |
| **P4 retroceso** — "no mueve la mira, sacude la cámara como si algo explotara" | `src/weapons/Weapon.cpp` + `.h` (sin dueño en la tabla) | El retroceso ya **sube la retícula**: patada por slot de arma acumulada en `CCamera::m_f3rdPersonCHairMultY` (es a la vez la altura a la que el HUD pinta la mira y el ángulo con el que sale la bala, `Weapon.cpp` → `Find3rdPersonCamTargetVector`) con vuelta automática de 0.12 ud/s y tope de 0.12 (`ViceExtRecoilKick`/`ViceExtRecoilUpdate`, el segundo desde `CWeapon::UpdateWeapons` cada frame). La sacudida de cámara baja de 0.05 a `0.004 + 0.20*patada` y el golpe de mando de 120/96 a 30+patada*900. |
| **Recarga con `R`** | `src/peds/PlayerPed.cpp` (`ViceExtTryManualReload`, bloque C3.1 de la sección 3, avisado) | Si la acción `PED_RELOAD` no apunta a `'R'` (config de controles guardada de antes de que la acción existiera: el hueco era `UNKNOWN_ACTION`), **también** se acepta `'R'` directa. La traza de pulsación ahora imprime la tecla resuelta: `VICEEXT reload key tecla=<código> …`. |

**Cómo se confirma (sólo logs, sin Chrome):** `node gta_vc_browser/tools/hidecops-log-check.mjs`
(avisa si la sesión jugada es `rev<6`) y `node gta_vc_browser/tools/driveby-log-check.mjs`.
Los dos verificadores se actualizaron al formato nuevo (`seen d=<ms>`, `seen_ms=`, `sight_dot=`) y
pasan `node --check`.

## Lo que queda de mi sección (con dueño y con el diagnóstico ya hecho)

1. **P1 en vivo**: ahora el PASS es posible de verdad — teclear `CRAZYCOP` (3★), esconderse
   donde no te vea nadie ~20 s y esperar `drop` (~15 s). **No medido aún**: el build con
   `rev=6` acaba de publicarse.
2. **Moverse apuntando** (v1.5, cabecera de mi bloque P2) — **sin empezar**. Incluye lo que
   pide el jugador: *"cuando estoy apuntando no debería poder correr solo caminar"*.
   Referencia de comunidad: **Manual Aiming v1.5** (apuntar con el botón derecho, moverse
   mientras se apunta, recargar con `R`). El camino es el bloque de apuntado de
   `ProcessPlayerWeapon` (fichero de la **sección 3**).
3. **P3 (moto VCPD)** — sigue **bloqueado por dato**: `6507 polwintergreen` con clase
   `ignore` en `default.ide` (sección 1). Petición ya escrita más arriba.

## Lo que el jugador pidió y NO es de mi sección (con dueño, para que no se pierda)

- **Caminar agachado estilo SA** (y levantarse si empiezas a correr) → **sección 3, C5**.
  Hoy `ViceExtCrouchControl` sólo actúa **desarmado/melee** (`m_nAmountofAmmunition <= 1`) y
  con arma lo lleva el motor (`ProcessPlayerWeapon`) — que es el agachado de *disparo*: no
  hay estado de andar agachado. Los clips ya están servidos (`Crouch_Idle/Forward/Backward`,
  `ASSOCGRP_PLAYERCROUCH`).
- **Animación de apuntar con la escopeta**: hoy `ViceExtCanAim()` (C7) añade el flag
  `CANAIM` a las escopetas, pero `CAN_AIM_WITH_ARM` sigue pidiendo `WEAPONFLAG_CANAIM_WITHARM`
  (`PlayerPed.cpp:1425`), que la escopeta no tiene → sale la mira pero **no la pose de
  apuntar**. Es la **sección 3, C7**.
- **Las miras son muy gruesas / borde grande**: es el dibujo de `ViceExtDrawSight` (D6,
  `Hud.cpp`): cada mira se pinta en una caja de `32*0.6` (mira) o `32*0.4` (resto) px de
  semi-lado, la misma que la cruz de serie. Reducir la escala o re-exportar la textura es de
  la **sección 1**, y para "buscar la mira original del mod" la fuente es su propio
  `models/weaponSights.txd` (7 texturas) — no hay arte que traer de fuera.
- **Escalar** → bloque opcional **E1** (`04-opcional-escalar.md`), y su `features.ini` lo trae
  **a 0**. Los clips existen (`CLIMB_*`).
- **Sirena del coche policial**: el jugador ha dicho que **deje de encargarme de eso** (mi P5
  queda retirado; el hueco real medido era `fbicar`/147 sin coronas, ya descrito arriba).
- **Cuelgues de cinemática / 3 estrellas, LOD, 1ª persona, nadar, depósito, ranuras y
  autoguardado, autocentrado**: de las secciones 1 y 3, ya repartidos en
  `05-hallazgos-4a-partida.md` (J2).

> **Aviso de propiedad**: para cerrar esta lista he tocado dos ficheros que no son de la
> tabla de la sección 2 — `src/renderer/Hud.cpp` (una condición, bloque D6 de la sección 1)
> y `src/peds/PlayerPed.cpp` (la lectura de la tecla de recarga, bloque C3.1 de la sección 3).
> Los dos cambios son **locales y detrás del define que ya existía**; si la sección dueña
> prefiere hacerlos suyos, se quitan sin tocar nada más. Avisado también en `HISTORIAL.md`.

## 2ª tanda (21/09, misma sesión) — lo que pidió tras jugar

El jugador jugó con `rev=6` y dijo: **las estrellas sí bajan** (el log lo confirma:
`WANTEDHIDE drop 3->2 en 15.0 s PASS`, 2 caídas sin que le vieran) **pero nunca
desaparecen del todo**, la retícula **no tiene retroceso**, y dio la lista de
agachado, escopeta, miras, apuntar-corriendo y escalada. Todo eso va en el build
de **08:23:27** (23.284.179 bytes):

| Qué | Dónde | Qué se hizo |
|---|---|---|
| **P1 `rev=7`**: la última estrella también cae | `src/core/Wanted.cpp` (mío) | La regla de esconderse cubre ahora **también el nivel 1**: antes el nivel 1 quedaba en manos de la regla vanilla (1 punto de chaos/s *sólo* si no hay policía a menos de 18 m), así que con una patrulla cerca la última estrella no caía nunca. Misma gracia (5 s) y mismo temporizador (15 s). |
| **Retroceso v2** | `src/weapons/Weapon.cpp` (sin dueño) | El jugador dijo "la retícula ahora no tiene recoil": la patada se **dobla** (pistola 0.045, escopeta 0.060, SMG 0.018, rifle 0.030, pesada 0.075, sniper 0.100), la vuelta pasa de 0.12 a **0.25 ud/s** (tope 0.22) y **se aplica en el mismo frame del disparo** (antes se aplicaba en el `UpdateWeapons` del frame siguiente, así que el primer tiro de cada ráfaga no se notaba). Referencias de comunidad para esto: el CLEO `WeaponRecoilAuto` (gtaforums 953286) y `Bullet Spread/Recoil Fix` (jenksta, gtagaming) — los dos empujan la mira **hacia arriba** por disparo y la dejan volver, en vez de sacudir la cámara. Traza nueva `VICEEXT recoil kick slot=… patada=… subida=… multY=…` (máx. 5/s). |
| **Apuntando sólo se camina** | `src/peds/PlayerPed.cpp` (`PlayerControlZelda`; define nuevo `VICEEXT_AIM_WALK` en `config.h`) | *"Cuando estoy apuntando no debería poder correr, sólo caminar"*: apuntando (o agachado) la velocidad se limita a paso de andar (1.0) y el esprint se anula, así que la animación elegida también es la de andar. Cabecera de mi bloque P2 ("Movement while aiming", v1.5). |
| **Agachado estilo SA** | `src/peds/PlayerPed.cpp` (bloque C5 de la sección 3, avisado) | Antes sólo funcionaba **desarmado**: con arma se salía de la función y el agachado lo llevaba el motor (el de **disparo**, que te deja quieto) — por eso el jugador no podía andar agachado. Ahora el estado es uno solo (`odCrouched`): con arma lo sigue del motor (`bIsDucking`), sin arma lo togglea la tecla C, y **correr o saltar se pone de pie** (`VICEEXT crouch off motivo=carrera`). Se superponen los clips del mod `Crouch_Idle/Forward/Backward`. |
| **Apuntar con la escopeta** | `src/peds/PlayerPed.cpp` (bloque C7, avisado) | El apuntado del motor elegía la pose con `WEAPONFLAG_CANAIM_WITHARM`, que las escopetas no traen, así que caía en la rama de sólo girar el cuerpo (la mira salía, la pose no). Ahora las escopetas del mod (`SHOTGUN`, `STUBBY`, `SPAS12`, `SHOTGUN2`) usan la pose de apuntar con brazo. |
| **Miras más finas** | `src/renderer/Hud.cpp` (bloque D6, avisado) | *"Muy gruesas, el borde muy grande"*: la caja era la de la cruz de serie (32×0.6 = 38 px) y el arte del mod se estiraba; ahora 0.42/0.26 (≈27/17 px). |
| **Escalar (E1, nuevo `VICEEXT_CLIMB`)** | `src/peds/PlayerPed.cpp` + grupo nuevo | Los clips `CLIMB_*` del mod **no los usaba nadie** (no existía grupo de animación). Se añade `ASSOCGRP_PLAYERCLIMB` (`animation/AnimManager.h`, `AnimationId.h`, `AnimManager.cpp` — ficheros compartidos, avisado) y la mecánica: saltando mirando a un borde de 0.55–1.85 m, con sitio libre arriba, se reproduce `CLIMB_Pull` y en 0.7–1.1 s el ped sube al borde (`CLIMB_Stand_finish`). Si no hay borde, salta como siempre. Trazas `VICEEXT climb start borde=…` / `climb fin`; se abandona solo si el reloj retrocede o si pierde el control. |

**Cómo se comprueba en la próxima partida** (todo por log, sin sondas):

* esconderse con 3★ debe llegar a **0 estrellas** (`WANTEDHIDE drop` x3; el verificador
  imprime la revisión y avisa si es `<6`);
* disparar: `VICEEXT recoil kick slot=3 patada=0.045 subida=…` y la retícula subiendo;
* `VICEEXT crouch on/off` (y `off motivo=carrera` al esprintar);
* `VICEEXT climb start borde=…` al saltar contra un muro bajo.

**Sigue pendiente en mi sección**: sólo **P3** (moto VCPD circulando), bloqueado por el
**dato** de la sección 1 (`6507` con clase `ignore` en `default.ide`).
