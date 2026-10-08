---
name: agachado-calibrado
status: EXECUTED
type: feature
domain: gameplay-crouch
owner_rules: .agents
created: 2026-09-26
---

# Plan técnico: Agachado fiel al mod (sa-crouch + ViceExtended) — clips servidos, rueda y calibración

> Origen: `12-handoff-tanda2.md` §13.3 y §14. Carril R20c…R26d documentado en `agachado-sa.md` y
> `agachado-sa-handoff.md` (builds ve41-ve55). Este plan cierra lo pendiente con la spec delante y
> con las decisiones del jugador del 26/09. **Enriquecido el 26/09** (gate «enriquece el plan»).

> **READY** por aprobación explícita del jugador (26/09), tras la revisión crítica del orquestador
> (fidelidad a la spec, cero alcance extra, supuesto del loader reconfirmado, acoplamiento código+dato
> en la misma build). `READY` solo habilita solicitar la ejecución: no autoriza modificar código ni
> correr builds/tests/scripts. Eso requiere después el gate **«ejecuta el plan»** y, aun con él, cada
> comando de proyecto necesita autorización independiente (RULES 0.4 / E6).

## Decisiones del jugador (26/09) — mandan

1. **Clips**: **servir los clips del sa-crouch** (`GunCrouchFwd`/`GunCrouchBwd`) en el `ped.ifp` servido.
   Acepta el **cambio de datos** (re-empaquetar + `dataTag` nuevo + re-descarga).
2. **Prioridad = como está en los mods** (no «como ahora»). Literal: *«el agachado está malísimo, no deja
   rodar entre muchas otras cosas; la idea es priorizar como está en los mods»*. ⇒ Carril de **fidelidad +
   arreglo**; la rueda entra como ítem de reproducción/corrección.
3. No se reabren: velocidad de cuerpo **0,90 m/s** (R26d) y cadencia de piernas **x1,7** (R26b/R26c).
   **R28 (26/09, tras jugar `crouch1`): el jugador reabre la velocidad — pasa a 1,13 m/s, la del andar
   de pie. Ver la ronda R28 al final del plan.**

## Análisis (investigación hecha, 26/09)

### 1. La fuente: qué dice el mod (solo-lectura)

- `mods/sa-crouch-movement_1774634386_626056/cleo/CrouchMovement(forClassicAxis).cs` — desensamblado
  completo (284 instr.) en `gta_vc_browser/tmp/c12-crouch.txt` + `c12-crouch-hist.txt`:
  - Guarda principal: `IS_PLAYER_PLAYING` + `NOT IS_CHAR_IN_ANY_CAR` + bit de agachado
    (`READ ped+336`, `>>4 &1`) + `NOT IS_BUTTON_PRESSED 17`.
  - **Botones** (mapeo del motor, `src/control/Script5.cpp:1400`): `8/9/10/11` = cruceta
    arriba/abajo/izq/der; **`6` = `RightShoulder1` = apuntar**; **`17` = `Circle` = disparar**;
    `14/16` = Square/Cross; `18` = LeftShock. Deadzone del mando: **±16**.
  - Direcciones: arriba → `GunCrouchFwd` (173); abajo → según `GET_CONTROLLER_MODE`, modo 1 →
    `GunCrouchBwd` (174), si no → `GunCrouchFwd` (173); lados → **apuntar (`6`)** + **sin disparar (`17`)**
    + **sin cruceta arriba/abajo** + **arma 17-27** → modo 1 → `GunMove_L/R` (177/178), si no →
    `Crouch_Roll_L/R` (175/176).
  - **Entrada** por petición: `SET_CHAR_CROUCH 0` → `PLAY_ANIMATION blend 10` → `WAIT 500` → `SET_CHAR_CROUCH 1`.
  - Armas pesadas 28-33: bloque propio (blend 30 + re-marca); estados de ped 42/54: control a 0 + KO anims.
- `scripts/VC.CustomAnimsData.dat` — añade al grupo `ped`: `GunCrouchFwd 173 (88)`, `GunCrouchBwd 174 (88)`,
  `Crouch_Roll_L 175 (212)`, `Crouch_Roll_R 176 (212)`, `GunMove_L 177 (212)`, `GunMove_R 178 (212)`;
  usa `DUCK_down/low 157/158` y `WEAPON_crouch 159` del base. Flags (13 §3.2): 88 = loop/movimiento, 212 = one-shot.
- Licencia del mod: **prohibitiva explícita** (13 §4) ⇒ **solo spec, sin copia de código**. El `.asi` es
  Win32 (inportable). El `anim/ped.ifp` del mod (240 anims) solo se usa como **fuente de datos** de los 2 clips.

### 2. Medidas (dato, no ojo) — `tools/ifp_inspect.py --curva`

| Clip | Hoy | Raíz | Natural |
|---|---|---|---|
| `Crouch_Forward` | servido | +2,615 m / 0,731 s | 3,58 m/s |
| `Crouch_Backward` | servido | −1,853 m / 1,000 s | 1,85 m/s |
| `Crouch_Roll_L/R` | servido | −2,172 / +2,253 m / 0,931 s | 2,33 / 2,42 m/s |
| `WEAPON_crouch` | servido | 0 / 0,500 s | pose |
| `GunMove_FWD/BWD/L/R` | servido | 1,854 / −1,853 / ±1,803 m / 1,0 s | 1,85 / lateral |
| **`GunCrouchFwd`** | **sa-crouch** (`tmp/a4/GunCrouchFwd.ifp`) | **+2,740 m / 0,731 s** | **3,75 m/s** |
| **`GunCrouchBwd`** | **sa-crouch** (`tmp/a4/GunCrouchBwd.ifp`) | **−2,740 m / 0,731 s** | **−3,75 m/s** |

- Los dos clips son **simétricos y lineales** (0,125 m por clave): con raíz 3,75 y `x1.7` el rate queda
  **0,408** y los pies barren **1,53 m/s — exactamente lo de hoy** (0,827 × 1,85). El cambio de clip **no**
  altera el patinaje validado; solo cambia la pose y la simetría adelante/atrás.

### 3. Hechos del motor/datos que condicionan el diseño (verificados en esta pasada)

- **Loader de IFP** (`src/animation/AnimManager.cpp:1427-1600`): los payloads de `DGAN` y `CPAN` **no se
  consumen**; `INFO` lleva `int numAnims/numSeqs` + nombre de bloque (24 B) y su `size` se usa redondeado;
  `NAME` lleva nombre + `numFrames` en +28 + `boneTag` en +40; las claves van `numFrames × stride`
  (`KRTS` 0x2C, `KRT0` 0x20, `KR00` 0x14) sin padding. ⇒ **Añadir animaciones = insertar sus bytes al final
  del fichero + subir `num_anims` (272 → 274)**; nada más.
- **`bootseed.list:1731` incluye `anim/ped.ifp`** ⇒ el dato va en el paquete inicial; `build.sh` precarga
  `bootseed/` (`--preload-file`). `gen_manifest.py` reescribe `web/public/manifest.json` desde `streamed/`.
  Pipeline: `streamed/` (servido on-demand) + `stage_bootseed.py` (copia a `bootseed/`) + `gen_manifest.py` +
  `dataTag` (`web/ondemand.js:120`, hoy `2026-09-21-ve13`).
- **Apuntar** = `CPad::GetTarget` (`Pad.cpp:3138`) = `RightShoulder1`; el `.cs` usa exactamente eso (botón 6).
  En PC, `PED_LOCK_TARGET` está en `R1`/mando, ratón por defecto el botón 3 (`ControllerConfig.cpp:327`).
  **Disparar** = `CPad::GetWeapon` (`Pad.cpp:2884`) = `Circle` = botón 17 del `.cs`.
- **Nombres de clip**: el motor empareja por nombre sin distinguir mayúsculas (hallazgo registrado en
  `agachado-sa.md` §6); `GunCrouchFwd/Bwd` caben en el campo NAME (24 B).
- **Estado real de la rueda**: la cadena existe (apuntar + lateral + arma 17-27 + bloqueo de una por
  pulsación) y **pasó el arnés** en su día (`agachado-mira`), pero los logs del jugador del 26/09
  (`02-21-11` … `13-47-21`) tienen **0 ruedas y 0 muestras con `mira=1`**: el fallo hay que **reproducirlo**
  con una sesión dedicada. No se cambia la rueda a ciegas: primero trazas de motivo, luego fix.

## Enriquecimiento técnico (26/09) — detalle ejecutable

### E1 · Herramienta nueva `gta_vc_browser/tools/ifp_add.py` (lectura + escritura de IFP)

- **CLI**: `python tools/ifp_add.py <destino.ifp> <origen.ifp> --patron "GunCrouch(Fwd|Bwd)" [--aplicar]`.
  Sin `--aplicar` = dry-run (imprime qué añadiría y con qué hashes); con `--aplicar` escribe y deja copia
  `<destino>.bak` (una sola, aviso si ya existe).
- **Traversal**: el mismo de `ifp_inspect.py` (replicando los quirks del loader: DGAN/CPAN sin payload,
  `INFO` con `numAnims`+nombre, `NAME`+`numFrames@28`, claves por `numFrames × stride`). Devuelve por
  animación `(nombre, offset_ini, offset_fin, num_frames)`.
- **Validaciones fail-closed**:
  1. `assert ANPK`; `parsed_end + tail == len(fichero)` con `tail ∈ {0, 4, 8}` y aviso si `tail != 0`.
  2. Rechaza si el nombre a añadir ya existe en el destino.
  3. Cada animación del origen debe parsear entera (nombre + nº de secuencias + claves) y su bloque debe ser `ped`.
  4. Tras escribir: re-parsea el resultado y **compara hash de cada rango original** (nada existente cambia),
     nº de animaciones = 274, y presencia/medida de los dos nuevos (3,75 m/s, avance lineal).
- **Cambios en bytes**: inserta los rangos `NAME..última clave` antes del `tail`; sube `num_anims` (u32 en el
  payload del `INFO` raíz). No toca `info.size`, `NAME.size` ni strides.
- **Salida**: tabla `nombre  frames  bytes  sha1` + veredicto `OK` / `ABORT` con motivo.

### E2 · Datos (una vez; `dataTag` nuevo)

1. `python gta_vc_browser/tools/ifp_add.py gta_vc_browser/streamed/anim/ped.ifp "mods/sa-crouch-movement_1774634386_626056/anim/ped.ifp" --patron "GunCrouch(Fwd|Bwd)"` (dry) → `--aplicar`.
2. Verificar: `python tools/ifp_inspect.py --curva streamed/anim/ped.ifp "GunCrouch"` →
   `animaciones=274`, `+2.740 / −2.740 en 0,731 s`, y `Crouch_*`/`WEAPON_crouch` intactos (mismas medidas).
3. `python gta_vc_browser/tools/stage_bootseed.py` → `bootseed/anim/ped.ifp` = mismo md5 que `streamed/`.
4. `python gta_vc_browser/tools/gen_manifest.py` → manifiesto con el tamaño nuevo.
5. `gta_vc_browser/web/ondemand.js:120` → `dataTag: '2026-09-26-ve14'` (sube **una** vez; purga la caché IDB
   del jugador y provoca la re-descarga que él ha aceptado).
6. (build, autorizado aparte) `bash gta_vc_browser/build.sh` regenera `reVC.data` con el `ped.ifp` nuevo.
7. Nota de memoria: `tmp/a4/*.ifp` (copia ya extraída) se usa como **cross-check** de los rangos insertados;
   la fuente canónica del patch es el `ped.ifp` del mod en `mods/`.

### E3 · Código

- **`src/animation/AnimManager.cpp:971-977`** — `aCrouchAnimations`:
  `"crouch_forward"` → `"GunCrouchFwd"`, `"crouch_backward"` → `"GunCrouchBwd"` (mismo orden/ids; los
  `aCrouchAnimDescs` no cambian de flags). Actualizar el comentario de medidas (2,740 m / 0,731 s).
- **`src/peds/PlayerPed.cpp` → `ViceExtCrouchClipSpeed` (~3155-3166)**:
  `ANIM_STD_CROUCH_FORWARD → 3.75f` y `ANIM_STD_CROUCH_BACKWARD → 3.75f`, con comentario que cita las dos
  medidas (servida vieja 2,615 y nueva 2,740) y el porqué (raíz del clip **servido**).
  `ROLL_L/R` se quedan (mismos clips 2,33/2,42).
- **Comentarios de ritmo (~3190-3208)**: `0,90/3,75 = 0,24; ×1,7 = 0,408`; dejar escrito que los pies
  barren 1,53 m/s (idéntico a antes del cambio).
- **Rueda (~3495-3516)** — dos cambios fieles al `.cs`, sin tocar el disparador:
  1. **Excluir disparo**: añadir `!padUsed->GetWeapon()` a la condición (espejo de `NOT IS_BUTTON_PRESSED 17`).
  2. **Motivo por estado** para no volver a ciegas: `static int s_odRollWhy`; al cambiar, emitir
     `VICEEXT crouch roll skip motivo=sin-mira|arma|bloqueada|activa|no-lateral` (una por cambio) y añadir
     `motivo=ok` al evento `VICEEXT crouch roll` existente. **No** se cambia el «una rueda por pulsación» (R22).
- **Trazas**: en `VICEEXT crouch move` (~3609) añadir `cal=1 clipr=%.2f` (raíz usada por el clip en curso);
  en `CROUCH2` (~3696) añadir `entrada=%.0f` (ms desde el cambio de clip = ventana del blend 10, reutiliza
  `s_odCrouchEnterMs`); buffer `char t[480]` → `[560]` (ya se truncó una vez, R23).
- **No se toca**: velocidad 0,90, cadencia x1,7, giro lateral (R26d), deadzone ±16, pose `WEAPON_crouch`,
  caída de cámara 0,55, cancelaciones y el orden de gates de la rueda.

### E4 · Verificador y marcas

- **`gta_vc_browser/tools/viceext-log-check.py`** (bloques de agachado ~743-1339):
  - Parsear los campos nuevos; **sin `cal=1` → `INCONCLUSIVE`** (nunca PASS con build vieja).
  - Bandas: caminar `GunCrouchFwd/Bwd` con `mps`/`velo` = 0,90 ±15 % y `pies` ≈ 0,408 ±15 %;
    `CROUCHMOVE fin maxfr < 0,05 m`; `mvec ≈ obju`; rueda `rueda=1` ≤1 por pulsación y **solo** con
    `mira=1` + arma 17-27; `peso ≥ 0,5` agachado; `CROUCHPOSE` a 0 al segundo de levantarse.
  - Los `skip` de rueda se aceptan como diagnóstico, pero la sesión debe cerrar al menos una rueda `motivo=ok`.
- **`gta_vc_browser/tools/check-served-build.sh`**: marcas `cal=1`, `clipr=%.2f`, `motivo=ok`; y corregir la
  fila `:76` que aún dice «(1,85 m/s)» → 0,90.

### E5 · Diagnóstico de la rueda (árbol de decisión con la sesión del jugador)

Con la build nueva, el log resuelve en una sola pasada:
1. `CROUCH2 tgt=0` mientras él mantiene apuntar → el mando no ve el apuntado en ese estado; revisar de dónde
   lee `PlayerControl1stPersonRunAround`/`ProcessPlayerWeapon` el apuntado y usar la misma fuente
   (`tgt=` ya existe en `AIMDIR`/`CROUCH2`).
2. `tgt=1` y `andando=1` pero `ang≈0` con A/D pulsado → el ángulo del mando se lee en base distinta con la
   cámara de ratón; fix: para la rueda, tomar el **signo de `GetPedWalkLeftRight`** (como el `.cs`, que solo
   necesita el eje/DPad), no el ángulo cuerpo-relativo.
3. `motivo=arma` → tipo exacto fuera del filtro (registrar `arma=` y comparar con 17-27; las nuevas 48+ pasan).
4. `motivo=bloqueada` pegado → el desbloqueo exige soltar el lateral; liberar también al soltar apuntar.
5. `motivo=activa` repetido → correcto (una por pulsación); no tocar.
6. `motivo=ok` sin rueda visible → problema de animación (descriptor/flags): ronda siguiente con el dato del log.
Si el fix es claro al leer el código en el paso 1-4, entra **en la misma build**; si no, la build lleva las
trazas y el fix va en `crouch2` con la evidencia del log.

### E6 · Autorizaciones necesarias (cada una, explícita e independiente)

| Comando | Para qué |
|---|---|
| `python tools/ifp_add.py …` + `ifp_inspect`/md5 | patch y verificación del dato |
| `python tools/stage_bootseed.py` / `gen_manifest.py` | re-empaquetar datos servidos |
| `ninja …/peds/PlayerPed.cpp.o` (+ objetos de AnimManager) | compilar sin enlazar |
| `bash gta_vc_browser/build.sh` | enlazar con `VERSION` nueva (`2026-09-26-crouch1`) |
| `bash gta_vc_browser/tools/check-served-build.sh` | marcas del wasm servido |
| `python tools/viceext-log-check.py <log>` | veredicto del log del jugador |

Sin estas autorizaciones, el plan queda implementado pero sin verificar (regla 0.4).

### E7 · Orden de ejecución y dependencias

1. Edits de código (E3) — compilan sin el dato nuevo (el clip solo se pide en runtime).
2. Dato (E2) con dry-run **antes** de aplicar; verificación `ifp_inspect` obligatoria antes de seguir.
3. Repack (`stage_bootseed` + `gen_manifest` + `dataTag`) y build con `VERSION` nueva (código + dato juntos:
   si el wasm pide `GunCrouchFwd` con el dato viejo, `BlendAnimation` devuelve null y no hay clip de agachado).
4. `check-served-build.sh` OK + `ifp_inspect` del servido OK → avisar al jugador con el criterio PASS.
5. Sesión del jugador (rueda + calibración en la misma) → log → verificador.
6. Si la rueda falla: leer motivo, fix, `crouch2`. Si todo pasa: plan + `ATTRIBUTION.md` + fila del
   `12-handoff` y cierre (DoD).

### E8 · Riesgos y paradas

- Si tras `--aplicar` el `ifp_inspect` no da 274 / 3,75 / los viejos intactos → **restaurar `.bak`**, parar y reportar.
- Si el mote al arrancar escupe `[od] tabla anims llena` o `Loading ANIMS ped` falla → revertir dato y build.
- `dataTag` nuevo = purga de caché del jugador (aceptado); documentarlo en el aviso de la sesión.
- No mezclar con otros carriles (nado/recoil/cámara): cualquier conflicto de fichero, se para y se reporta.

## Restricciones (reglas de casa del plan 12)

- El mod manda; se reescribe desde la spec. Excepciones intocables: auto-centrado de cámara y recoil.
- `.cpp` = CRLF; editar con python binario (heredoc quoted + `assert count==1`); consola cp1252 →
  `PYTHONIOENCODING=utf-8`. Sin palancas de URL/consola (R24). Sin commits ni staging amplio.
- `VERSION` sube en cada build; `dataTag` sube por el cambio de datos (una vez). Antes de pedir partida,
  `check-served-build.sh` OK.
- Medición = jugador + log; nada de arnés headless para el agachado. Criterio PASS definido antes de que juegue.
- Escritura permitida: `src/`, `gta_vc_browser/tools/`, `gta_vc_browser/web/`, `docs/mods/`, `.agents/plans/`.

## Pasos ejecutables (resumen, detalle en E1-E8)

1. Implementar `tools/ifp_add.py` (E1) y probarlo en dry-run contra el `ped.ifp` servido.
2. Aplicar el dato (E2) + verificar; re-stage + manifiesto + `dataTag` ve14.
3. Editar código (E3): nombres, raíces 3,75, exclusión de disparo en la rueda, motivo de rueda, `cal=1`/`clipr=`/`entrada=`, buffer.
4. Actualizar verificador y marcas (E4).
5. Compilar objeto (autorización) → enlazar con `VERSION` nueva (autorización) → `check-served-build.sh`.
6. Sesión del jugador con PASS; leer log; si la rueda falla, fix según E5.
7. Cerrar: actualizar plan (memoria), `ATTRIBUTION.md`, fila del handoff.

## Verificación (criterio PASS explícito, antes de que juegue)

- **Rueda**: agachado + apuntar + lateral, arma 17-27 → **1 rueda por pulsación** (`VICEEXT crouch roll motivo=ok`,
  `rueda=1`); sin repetir al mantener; **no** disparando; **no** con armas 28-33.
- **Clips**: `clip=GunCrouchFwd/Bwd` en `CROUCHMOVE`; partida con `data=2026-09-26-ve14` en la cabecera del log.
- **Velocidad/cadencia**: `mps`/`velo` 0,90 ±15 % adelante y atrás; `pies` ≈ 0,408 ±15 %; `maxfr < 0,05 m`;
  `mvec ≈ obju`.
- **Direcciones**: clip por ángulo (adelante/lados/atrás con giro lateral), deadzone ±16.
- **Apuntar agachado**: `pesomira` 1,0 quieto / 0,4 andando; `mira=1` solo con arma apuntable y `GetTarget`.
- **Sin restos**: `pose off` + `CROUCHPOSE` a 0 en ≤1 s; cancelaciones OK.
- **Build/datos**: `check-served-build.sh` OK con marcas nuevas; `VERSION` nueva; `dataTag` ve14;
  verificador PASS/FAIL/INCONCLUSIVE (sin `cal=1` → INCONCLUSIVE).

## Pendientes relacionados (fuera de alcance)

- **`gta_vc_browser/tools/harness/esquemas.mjs`** sigue esperando `crouch_forward` en sus
  escenarios de agachado (líneas ~230-245, 420, 624, 1037). El agachado **no** se mide con el arnés
  headless (regla del carril: jugador + log), así que no se ha tocado; si alguien lo corre, esos
  escenarios darán fallo falso hasta que se actualicen los nombres a `GunCrouchFwd`.

- Variante «classic» del `.cs` (`GunMove_L/R` al apuntar de lado, `GunCrouchBwd` atrás) → carril ClassicAXIS.
- Armas pesadas (blend 30) y estados KO 42/54: revisión de no-op; si hay artefacto, ronda aparte.
- Deriva del agachado quieto (0,15-0,17 m/s en la muestra 13:47): vigilar, no arreglar aquí.
- `GunMove_*` para el aim-walk de pie (H4) y audio `swim-12`/`reload-2`: otros carriles.

## Ejecución (26/09) — hecho, pendiente de la sesión

Estado `EXECUTED`: implementación completa; la verificación es la partida del jugador.

- **Dato (E1/E2)**: `tools/ifp_add.py` (nuevo; reutiliza `ifp_inspect.parse_ifp`) insertó los dos
  clips al final del `ped.ifp` servido: 272 → 274 anims, 23 claves cada uno,
  +2,740/−2,740 m en 0,731 s = ±3,75 m/s, y **cada animación que ya estaba intacta byte a byte**
  (comprobado por el escritor y con `ifp_inspect --curva`). Original en
  `gta_vc_browser/streamed/anim/ped.ifp.bak`. `stage_bootseed.py` + `gen_manifest.py` OK (mismo md5
  en `streamed/` y `bootseed/`), `dataTag` → `2026-09-26-ve14`.
  - *Avería encontrada y corregida en la herramienta*: la primera verificación post-escritura
    comparaba el prefijo entero del fichero, y ese prefijo incluye el contador `num_anims` que se
    parchea a propósito ⇒ ABORT con el dato bien puesto. Ahora compara por animación (nombre,
    frames y su rango de bytes). Se restauró el `.bak` y se repitió la aplicación completa (E8).
- **Código (E3)**: `aCrouchAnimations` = `GunCrouchFwd`/`GunCrouchBwd`; raíces 3,75 en las dos
  direcciones; la rueda exige `!padUsed->GetWeapon()` (spec: `NOT IS_BUTTON_PRESSED 17`); trazas
  `cal=1 clipr=` (movimiento y CROUCH2), `entrada=` (CROUCH2) y `VICEEXT crouch roll skip motivo=…`
  (`activa`/`sin-mira`/`disparo`/`bloqueada`, una por cambio); buffer 480 → 560.
- **Verificador y marcas (E4)**: bloque `R27` nuevo en `viceext-log-check.py`
  (PASS/FAIL/INCONCLUSIVE; sin `cal=1` → INCONCLUSIVE; también detecta el corte de línea por
  `entrada=`), D5 acepta 272/274 y 5 marcas nuevas en `check-served-build.sh` (la fila de 1,85 m/s
  ya corregida a 0,90).
- **Build**: `VERSION` → `2026-09-26-crouch1`; `build.sh` OK (log en `gta_vc_browser/tmp/build-crouch1.log`);
  `check-served-build.sh` = OK con todas las marcas (incluidas las 5 de R27); `reVC.data` contiene
  `GunCrouchFwd`/`GunCrouchBwd` y el wasm ya no conoce `Crouch_Forward`.
- **Autorizaciones**: el jugador autorizó el paso de datos y el build («haz el build para probar»).
- **Desviaciones respecto al plan**: (a) el motivo `no-lateral` no se emite (el aviso sale con el lado
  pulsado) y se añadió `disparo`; (b) `clipr=` va también en CROUCH2 (no sólo en la traza de
  movimiento), que es lo que permite comprobar `pies`; (c) la fila de `ATTRIBUTION.md` se escribió ya
  (el dato está servido), no en el cierre.
- **Pendiente (verificación)**: sesión del jugador → log → `viceext-log-check.py` (bloque R27).

## Ronda R28 (26/09, build `crouch2`) — los cuatro defectos de la sesión de `crouch1`

El jugador jugó `crouch1` y reportó: «no veo ningún cambio, se agacha lento, al caminar camina
lentísimo (debe ser casi la misma velocidad que el caminado normal al presionar wasd), no rueda al
apuntar, al pararse de estar sentado Tommy se buguea la textura y se ve apilado a la derecha» (con
captura), «no puedo disparar manteniendo presionado, como que la animación de disparar agachado lucha
entre 2 animaciones». Pidió investigar cada comportamiento.

### Diagnóstico (de su log del 26/09 21:35 y del motor; nada de arnés)

- **Velocidad**: `CROUCH2 velo` mediana **0,83 m/s** con objetivo 0,90 y pies a 0,51 de ritmo; el
  andar de pie de serie son **1,13 m/s** («PEDAT»). Iba al 80 % del andar: de ahí el «lentísimo».
- **Rueda**: SÍ salía (10 eventos `motivo=ok`, `ang=±90`, `rueda=1` en las muestras), pero el clip de
  la rueda es de MOVIMIENTO y nuestra pose de apuntar es PARCIAL a peso 1: entraba multiplicado por
  `1 - 1 = 0`. El jugador solo veía al ped deslizarse. (Además el corte de 50° dejaba fuera W+A / W+D.)
- **Textura deformada / «lucha entre 2 animaciones»**: dos poses parciales a peso 1. La suma de
  cuaterniones puede quedar casi nula (matriz degenerada = cuerpo estirado/aplastado) y
  `BlendAnimation` retira toda parcial ajena: nosotros echábamos el clip del arma cada frame y el
  motor lo volvía a pedir (`SetPointGunAt`/`SetAttack`), en bucle.
- **Disparo agachado**: la pose de apuntar del arma y el clip de disparo los elige el motor con
  `bCrouchWhenShooting && bIsDucking`, banderas que R6 limpia al agacharse: elegía `colt45_fire` (de
  pie) en vez de `colt45_crouchfire`. El `weapon.dat` servido SÍ trae `WEAPONFLAG_CROUCHFIRE`
  (`Colt45 ... 680C0`) y los clips `*_crouchfire`/`*_crouchreload` SÍ están en los `.ifp` por arma
  (`colt45.ifp`, 24 huesos, verificado antes).

### Cambios (esta ronda)

- `PlayerPed.cpp`: velocidad objetivo **1,13 m/s** (`VICEEXT_CROUCH_WALK_MPS`/`_WALK_SPEED`/`_SPEED`),
  comentarios de ritmo recalculados (1,13/3,75 x1,7 = 0,51; pies a 1,92 m/s).
- `PlayerPed.cpp` R28: `ViceExtCrouchOtherPartials` + `ViceExtCrouchHeaviestPartial` +
  `ViceExtCrouchRollClearPartials` y `ViceExtCrouchAimPose(ped, andando, rodando)` con el
  **presupuesto de parciales**: si el motor tiene puesta su parcial (el clip del arma) la pose del mod
  se retira y no se vuelve a pedir; si no, entra con `1 - ajenas` (0,4 andando, 1 quieto); rodando se
  retiran todas las parciales. Traza nueva en `CROUCH2`: `otros= pose= pesoarma= nomarma=` (buffer
  560 -> 720).
- `Ped.h` + `PedFight.cpp` R28: `CPed::ViceExtCrouchShooting()` (acepta `bCrouchWhenShooting &&
  bIsDucking` o el agachado del port) usado en `SetPointGunAt` (pose de apuntar del arma), `SetAttack`
  y `Attack` (clip de disparo y recarga), `FinishedAttackCB` (vuelta a la pose) y `FinishedReloadCB`.
  Arma sin `WEAPONFLAG_CROUCHFIRE`: el motor no pone pose y la pone el mod.
- `PlayerPed.cpp`: el corte del costado para la rueda baja de 50° a **25°** (diagonales rodan, spec:
  cruceta + apuntar + sin disparar).
- Verificador: bloque `R27` con bandas **1,13 m/s ±15 %**, chequeo nuevo del presupuesto
  (`otros` + `peso` <= 1,02) y del nombre de la parcial del arma (`*_crouchfire`).
- `check-served-build.sh`: 5 marcas de R28 + textos 0,90 -> 1,13. `VERSION` -> `2026-09-26-crouch2`
  (el `dataTag` NO cambia: esta ronda no toca datos).

### Pendiente (verificación)

- Sesión del jugador + log -> `viceext-log-check.py` (bloque R27): agachado ~1,13 m/s, rueda con A/D
  apuntando (también en diagonal), `nomarma=*_crouchfire` al disparar agachado, `otros + peso <= 1` y
  sin deformación al levantarse.

## Cierre (memoria persistente)

> Pendiente. Se rellena al cerrar el plan (DoD de `.agents/WORKFLOW.md`): qué cambió, verificación
> ejecutada (o pendiente con motivo), resultado y pendientes.
