---
name: agachado-correcciones-axis
status: EXECUTED
type: bugfix
domain: gameplay-crouch
owner_rules: .agents
created: 2026-09-27 21:00
---

# Plan: Correcciones del agachado («el sentado») + interop con ClassicAXIS

> Origen: petición del jugador (27/09). Síntomas literales: *«el sentado está roto,
> va a destiempo que las pisadas, va lento, no rueda, el apuntado y disparo agachado
> es raro, hay animaciones raras y el personaje no gira hacia la dirección que
> camina»*. Además: *«hay en curso 2 agentes haciendo modificaciones del axis y nado
> del axis, supongo que debes hacer que al caminar interactúe correctamente el
> axis»*.
>
> Continúa: `agachado-sa.md` (análisis del sistema), `agachado-calibrado.md`
> (R27/R28, EXECUTED pendiente de verificación), `agachado-sa-handoff.md`
> (R20-R26d + trampas del motor). Espec del mod: `sa-crouch-movement`
> (`mods/sa-crouch-movement_1774634386_626056/`, solo SPEC por licencia
> prohibitiva; desensamblado del CLEO en `gta_vc_browser/tmp/c12-crouch.txt`).

## 0. Resumen del análisis (investigación cerrada antes de escribir el plan)

Build medida: `JS build=2026-09-26-swim9 data=2026-09-26-ve14`
(`gta_vc_browser/logs/odtrace-2026-09-28_00-54-29.log`, y sesión previa
`odtrace-2026-09-27_22-16-32.log`). Los siete síntomas tienen causa raíz
localizada en código o en datos; **ninguno es un misterio**, y tres son la misma
matemática (la zancada del clip) o la misma frontera (el giro y el apuntado):

| # | Síntoma | Causa raíz | Dónde |
|---|---|---|---|
| A | Pisadas a destiempo (patinaje) | rate 0,51 = 1,13/3,75 ×1,7 (R26c) → los pies barren **1,92 m/s** con el cuerpo a 1,13 | `ViceExtCrouchRateFor` + `VICEEXT_CROUCH_CADENCE` |
| B | «Va lento» | velocidad real OK (1,12 m/s por tramo) ⇒ la lentitud es **perceptual**: piernas al 51 % de su ritmo, o se quiere la velocidad «paso vivo» del mod | D1 de este plan |
| C | No rueda | gates rotos/desajustados: `odAim` exige `GetTarget()+CanAim` (con puños `arma=0` → `sin-mira`, 6 skips en el log) y el lock R22 deja `bloqueada`; el `.cs` del mod **re-dispara en bucle** manteniendo la tecla | `ViceExtCrouchAnim` (rueda) |
| D | Apuntado/disparo agachado raro | la pose `WEAPON_crouch` se retira (presupuesto R28, `otros=1.00 pose=-`) y el aim queda como `colt45_crouchfire` **sostenido** (un clip de DISPARO como postura); el mod usa `ANIM_WEAPON_CROUCH` como pose de apuntado | `PedFight.cpp` R28 + conflicto C18-1 |
| E | Animaciones raras | movimiento lateral/diagonal con clip `GunCrouchFwd` **sin girar el cuerpo** = deslizamiento; saltos de peso de parciales (0,00→0,58→1,00); entradas blend 10→4 | consecuencia de F + presupuesto R28 |
| F | No gira hacia donde camina | `ViceExtCrouchSideHeading` solo gira en 50-130° (el log da `giro=5` «no-lateral» con `ang=±45`); fuera de eso `m_fRotationDest = TheCamera.Orientation` forzado | `PlayerPed.cpp:1173/3250` + R26d |
| G | Interop con el axis | `odAim` y compañía leen `GetTarget()` a su manera; el plan ClassicAXIS reescribe el apuntado y sus C18 tocan **los mismos ficheros** que este carril | ver §3 |

**Lo que NO está roto** (para no reabrirlo): la velocidad por tramo cumple el
objetivo (`CROUCHMOVE fin mps=1.10-1.12` contra `obj=1.13`), `maxfr≈0.02` (sin
catapultazos), las unidades m/s↔motor siguen cuadradas (`obju=0.023 ≈ mvec`), la
rueda **sí sale** cuando se cumplen sus gates (log 22:16: `roll lado=der clip=248
veloc=2.35 dur=1018ms motivo=ok`) y el presupuesto de parciales R28 evita la
deformación de cuerpo.

## 1. La spec del mod (fuente de verdad, ya leída y contrastada)

Del CLEO desensamblado (`tmp/c12-crouch.txt`, 284 instr.) y de
`VC.CustomAnimsData.dat` (clips 173-178 en grupo `ped`; flags 88 = bucle/movimiento,
212 = one-shot):

| Entrada | Condición del mod | Clip |
|---|---|---|
| Cruceta arriba | agachado, sin disparar | `GunCrouchFwd` (173) |
| Cruceta abajo | agachado, sin disparar | modo 1 → `GunCrouchBwd` (174); si no → `GunCrouchFwd` |
| Lateral + **apuntando** (botón 6) + sin disparar (17) + sin arriba/abajo + arma 17-27 | una **rueda** por trigger, **re-dispara en bucle** con `WAIT 500` mientras se mantiene | `Crouch_Roll_L/R` (175/176) |
| Lateral **sin apuntar** | modo 1 → `GunMove_L/R` (177/178); si no → `GunCrouchFwd` + entrada | — |
| Entrada (GOSUB) | `SET_CHAR_CROUCH 0` → `PLAY_ANIMATION blend 10` → `WAIT 500` → `SET_CHAR_CROUCH 1` | — |
| Pose de apuntado | `BlendAnimation(ANIM_GROUP_MAN, ANIM_WEAPON_CROUCH, 4.0)` (classicaxis `Main.cpp:1393-1415`, C18-4) | `WEAPON_crouch` (159) |
| Armamento | arma **≥ 17 y sin 28-33** para rueda/lateral-apuntado; pesadas 28-33 = bloque propio (blend 30) | — |

Claves: (1) la rueda del mod **no** es «una por pulsación»: el script vuelve a
dispararla mientras el botón está pulsado (el `WAIT 500` del GOSUB de entrada es
su cadencia) — la regla R22 «una por pulsación» fue nuestra lectura del vídeo, no
la spec; (2) la pose de apuntado es `WEAPON_crouch`, **no** el `*_crouchfire`;
(3) `GunMove_L/R` y `GunMove_FWD/BWD` (1,85/±1,80 m por ciclo) **ya están
servidos** en el `ped.ifp` y no se usan para nada hoy: son el movimiento
apuntando que pide el jugador («apuntar y desplazarse», agachado-sa §6.3).

## 2. El problema de fondo de A+B: la zancada del clip no cuadra con ninguna velocidad honesta

Medido (`tools/ifp_inspect.py --curva`): `GunCrouchFwd/Bwd` = **±2,740 m / 0,731 s
= 3,75 m/s** a ritmo 1 (son clips de SA para su propio sistema de movimiento).

- rate = velocidad/raíz pega los pies SIEMPRE (R22), pero a 1,13 m/s el rate es
  0,30 → una zancada cada **2,43 s** = piernas en cámara lenta (ya rechazado).
- El apaño R26c (×1,7 → rate 0,51) sube la cadencia a 1,43 s/zancada (aceptable)
  a costa de que los pies barren **1,92 m/s** contra 1,13 del cuerpo = patinaje
  ×1,7 = **«va a destiempo que las pisadas»**. Es matemática, no un bug suelto.
- Con una zancada de 2,74 m **no existe** combinación que dé a la vez pies
  pegados y cadencia de andar a 1,13 m/s. Para cadencia de andar (1,2-1,5 s por
  zancada) hacen falta **1,4-1,7 m de zancada**: la raíz del clip hay que
  **reescalarla en el dato servido** (tenemos escritor de IFP: `tools/ifp_add.py`
  con verificación por rango y `.bak`), o elegir otra velocidad.

## 3. Frontera con los carriles en curso (MUY IMPORTANTE)

Hay dos agentes trabajando: **ClassicAXIS** (`apuntado-classicaxis-100.md`,
`Cam.cpp`/`Camera.cpp`/`Hud.cpp`/`Pad.cpp` + C18 en `PedFight.cpp`/`PlayerPed.cpp`)
y **nado** (`PlayerPed.cpp`, `ViceExtSwim*`). Zonas de riesgo:

| Zona | Conflicto | Regla de este plan |
|---|---|---|
| `src/peds/PedFight.cpp` (SetPointGunAt/Attack/Reload) | R28 de este carril vs **C18-1/C18-2/C18-5** del plan axis | ver §4: la costura se define aquí y la consume el plan axis |
| `src/peds/PlayerPed.cpp` | funciones `ViceExtCrouch*` (este carril) vs `ViceExtSwim*` (nado) vs C18-3/C18-4 (axis) | cada bloque con su cabecera `// R##`/`// C18-##`; **no reordenar** funciones ajenas; avisar en el historial al tocar la zona compartida |
| Lectura del «estoy apuntando» | `odAim` propio (`GetTarget()+ViceExtCanAim`) vs el aim del axis en construcción | **API única** `ViceExtIsAiming()` (§4) que leerán rueda, pose y giro |
| `src/peds/Ped.cpp` (R20c) | solo este carril | intacto para los otros |

**C18-1 (el conflicto declarado)**: el plan axis propone aplicar literal «arma sin
`WEAPONFLAG_CROUCHFIRE` → apaga `bCrouchWhenShooting`» y anotar la pérdida de los
`*_crouchfire`. Armonización propuesta (más fiel y sin pérdidas, a confirmar por
ambos carriles):
- arma **con** `WEAPONFLAG_CROUCHFIRE` → se mantienen los clips `*_crouchfire`/
  `*_crouchreload` (la parte valiosa de R28, que el motor ya sabe usar);
- arma **sin** el flag → C18-1 literal (sin disparo agachado forzado) + pose
  `WEAPON_crouch` del mod.
Con eso los dos planes son verdad a la vez y no se pierde nada.

## 4. Cambios propuestos (por coste; el orden de ejecución es este)

### 4.1 Costura de apuntado (G, destraba D y la rueda)
- **`ViceExtIsAiming()`** (helper único, `PlayerPed.cpp/.h`): `GetTarget()` +
  `ViceExtCanAim` + lo que el port del axis defina. La rueda, la pose de
  apuntado, el no-giro y `CROUCH2 mira=` leen SOLO este helper.
- **Semántica C18 armonizada** (§3) en `PedFight.cpp`: `ViceExtCrouchShooting()`
  pasa a exigir `WEAPONFLAG_CROUCHFIRE`; sin el flag manda C18-1.
- **Pose de apuntado = `WEAPON_crouch`** (mod, C18-4): al apuntar agachado sin
  disparar manda `ANIM_WEAPON_CROUCH` a 4.0; `*_crouchfire` solo **durante** el
  disparo y `*_crouchreload` durante la recarga. El presupuesto de parciales R28
  se mantiene para que no peleen.

### 4.2 Giro hacia donde se camina (F, destraba E)
- Sin apuntar, **el cuerpo gira hacia la dirección de movimiento en todas las
  direcciones** (modelo SA/agachado-sa §4.2), no solo 50-130°: el helper
  `ViceExtCrouchSideHeading` deja de cortar a «no-lateral» y el destino de giro
  se fija en base **mundo** (evitando el círculo del handoff §6.5: el rumbo se
  guarda una vez por petición, no se realimenta con `m_fRotationCur`).
- El giro pasa de **snap** (R26d, `Cur=Dest` instantáneo) a giro suave con el
  `headingRate` del motor (el `hdgr=15` ya trazado), para que no «salte».
- Apuntando **no se gira** (ahí el encaramiento lo manda el axis) y el
  desplazamiento usa los `GunMove_*` del mod por dirección.
- Decisión D3 (abajo): qué hacer con el atrás puro.

### 4.3 Rueda (C)
- Repetible según spec: manteniendo apuntar+lateral la rueda se re-lanza tras su
  duración (≈931 ms/rate, la cadencia del `.cs` es `WAIT 500` entre triggers);
  se retira el lock «una por pulsación» (R22) o se reduce a la duración del clip.
- Gates desde `ViceExtIsAiming()`; el filtro de armas se mantiene fiel (≥17, sin
  28-33) salvo decisión D4 (puños).
- Se comprueba la **visibilidad**: mientras rueda, `ViceExtCrouchRollClearPartials`
  ya retira parciales; añadir a la traza el peso real del clip de rueda para
  confirmar que entra con peso alto.

### 4.4 Movimiento: pies y velocidad (A+B) — requiere D1
- Según D1: (opción recomendada) **reescalar la raíz** de `GunCrouchFwd/Bwd` en el
  `ped.ifp` servido (2,74 → ~1,5 m/ciclo; herramienta nueva `ifp_scale_root.py`
  al estilo `ifp_add.py`: dry-run, `.bak`, verificación por rango) + quitar el
  ×1,7 (`VICEEXT_CROUCH_CADENCE` a 1,0) → rate ≈ 0,55 con **pies pegados** y
  cadencia 1,3 s/zancada (= andar). Cambio de datos ⇒ `dataTag` +1 y
  re-descarga del jugador (aceptado en `agachado-calibrado` E2).
- Si D1 elige velocidad «paso vivo» (~1,5 m/s), misma mecánica con stride ~1,8 m.
- Diagnóstico de audio (por si «pisadas» son los sonidos): comprobar si
  `CPed::PlayFootSteps` dispara pasos con los clips del mod y a qué ritmo;
  añadir `pasos=` a `CROUCH2` si hace falta.

### 4.5 Movimiento apuntando (el «interactúa con el axis al caminar»)
- Apuntando + mando: encaramiento = cámara (axis), clips `GunMove_FWD/BWD/L/R`
  por ángulo (ya servidos), velocidad = la del aim-walk de pie (tope R9),
  lateral → rueda cuando se cumplen sus gates (spec).
- Se mantiene sin tocar: cámara del agachado (`VICEEXT_CROUCH_CAM_DROP 0.55`),
  cancelaciones (coche, muerte, agua, carrera), la tecla C.

## 5. Decisiones pendientes del jugador (bloquean la ejecución de 4.4)

| # | Pregunta | Opciones | Recomendación |
|---|---|---|---|
| D1 | Velocidad + pies | (a) 1,13 m/s + zancada reescalada (pies pegados, cadencia de andar) · (b) ~1,5 m/s «paso vivo» + zancada reescalada · (c) mantener ×1,7 (patina) | **(a)**: cumple su regla «agachado ≤ andar» y mata los síntomas A+B de una vez |
| D2 | Tras reescalar, ¿se acepta cambio de dato (`dataTag`, re-descarga ~160 MB)? | sí / no (y entonces solo se ajustan rates) | **sí** (ya aceptado en E2) |
| D3 | Atrás puro (S) | (a) `GunCrouchBwd` sin girar (hoy) · (b) girar 180° + `GunCrouchFwd` (SA puro) | **(a)**: es lo que hace el `.cs` en modo normal |
| D4 | Rueda con puños (arma < 17) | (a) no rueda (spec literal, ≥17) · (b) sí | **(a)**: el mod manda |
| D5 | Rueda repetible manteniendo tecla | (a) sí, cada ~931 ms (spec `.cs`) · (b) una por pulsación (R22 actual) | **(a)** |

## 6. Verificación (criterio PASS definido ANTES de que juegue)

Sin arnés headless (regla del carril: jugador + log), con
`tools/viceext-log-check.py` (bloque R27 ampliado) y
`bash gta_vc_browser/tools/check-served-build.sh` antes de pedir partida:

- **Pies**: `CROUCH2 pies=` ≈ velocidad/raíz sin multiplicador; raíz del clip
  servido = la nueva medida (verificar con `ifp_inspect.py`); sin patinaje a ojo.
- **Giro**: `giro=0` (girar) en las 8 direcciones sin apuntar; `rumbo` convergiendo
  hacia `ang` sin snap visible; apuntando `giro=3` (sin giro).
- **Rueda**: ≥2 ruedas seguidas manteniendo apuntar+lateral (`motivo=ok`), peso del
  clip de rueda > 0,5 mientras dura, cero `skip` injustificados (los `sin-mira`
  con puños son correctos si D4=(a)).
- **Apuntar agachado**: `nomarma=` vacío y `pose=WEAPON_crouch` al apuntar sin
  disparar; `nomarma=*_crouchfire` solo con gatillo; sin pelea de parciales
  (`otros+peso <= 1.02`).
- **Sin regresión**: velocidad por tramo 1,13±15 % (o la de D1), `mvec≈obju`,
  `maxfr<0.05`, cancelaciones y cámara intactas, nado y recoil **sin tocar**.

## 7. Restricciones (reglas de casa)

- `.cpp` = CRLF, edición con python binario (`assert count==1`); consola cp1252 →
  `PYTHONIOENCODING=utf-8`. Sin commits ni staging amplio.
- `VERSION` sube en cada build; `dataTag` solo si entra el cambio de datos (D2).
  `check-served-build.sh` OK antes de pedir partida.
- El mod manda (12-handoff §14): se reescribe desde la spec, no se parchea lo
  existente. Excepciones intocables: auto-centrado de cámara y recoil.
- No tocar `Cam.cpp`/`Camera.cpp`/`Hud.cpp`/`Pad.cpp` (carril axis) ni
  `ViceExtSwim*` (carril nado); `PedFight.cpp`/`PlayerPed.cpp` solo en los
  bloques de este carril, coordinando C18 con el plan axis (§3).
- Los `*_smoke-test.mjs`/arnés headless no se corre (petición expresa del
  jugador); la medida es su partida + log.

## 8. Pasos ejecutables (enriquecido 27/09)

> Cada paso tiene fichero, acción y detalle técnico. Orden de ejecución:
> E3.1-4 → E3.5-6 → E3.7 → E1/E2 (datos) → E3.8 → E3.9 → E4 → build → partida.
> Nada se ejecuta sin el gate «ejecuta el plan» del jugador + decisiones D1-D5
> (§5); E1/E2 exigen además D2 = sí (cambio de dato).

### E1. Herramienta nueva `ifp_scale_root.py` (reescalar la raíz del clip servido)

- **create** `gta_vc_browser/tools/ifp_scale_root.py`, mismo patrón que
  `ifp_add.py` (SIMULACRO por defecto, `.bak` de una sola copia, verificación
  post-escritura por rango, fail-closed):
  - reutiliza `ifp_inspect.parse_ifp()`; sintaxis
    `<destino.ifp> --patron "GunCrouch(Fwd|Bwd)" --factor 0.55` (o `--metros
    1.5`, que deriva `factor = metros / |dy|` actual de cada clip);
  - SIMULACRO: imprime por clip `dy` antes/después, raíz (m/s) resultante y
    sha1 del rango; con `--aplicar` escribe dejando `<destino>.bak` (si ya
    existe, `ABORT` como en `ifp_add.py`);
  - la reescritura es **in place** y solo toca los floats de **traslación** de
    las claves de raíz, que están en los índices **4:7 de cada clave** (maquetación
    verificada en `ifp_inspect.parse_ifp`: cuaternión 0:3, traslación 4:7, y
    después — solo en KRTS 0x2C/11f — escala 7:9 y tiempo 10; en KRT0 0x20/8f
    el tiempo va en el índice 7; KR00 0x14/5f **no** lleva traslación → `ABORT`
    si la raíz del clip es KR00). El desplazamiento se reescala **respecto a la
    primera clave** (`v' = v0 + (v - v0) * factor`) para no mover el origen del
    clip: el tamaño del fichero **no cambia**;
  - verificación por rango (como `ifp_add.py`): cada animación NO tocada intacta
    byte a byte en su `start:end`, `num_anims` y `meta['end']` sin tocar, y la
    `raiz` medida de las tocadas = el objetivo (misma medida que
    `ifp_inspect.py --curva`);
  - fail-closed antes de escribir: patrón sin match, clip sin curva de raíz
    (`dy is None`), factor ≤ 0 o > 2.
- **run** (solo con D1+D2): dry-run → `--aplicar` sobre el `ped.ifp` servido (el
  mismo destino que R27 usó con `ifp_add.py`) → `ifp_inspect.py --curva` para
  confirmar la raíz nueva. Factor según D1: ~0,55 (1,5 m/ciclo a 1,13 m/s) o
  ~0,65 (1,8 m/ciclo si D1=(b) ~1,5 m/s).

### E2. Datos servidos y `dataTag` (solo si D2 = sí)

- **edit** `gta_vc_browser/web/ondemand.js:125`: `dataTag`
  `2026-09-26-ve14` → siguiente marca (`2026-09-27-ve1`); solo con cambio de
  datos (regla de casa).
- **run** los mismos pasos de datos que R27 (`stage_bootseed.py` +
  `gen_manifest.py`) para regenerar manifest/bootseed del `ped.ifp` tocado; y
  `bash gta_vc_browser/tools/check-served-build.sh` OK **antes** de pedir
  partida (el jugador re-descarga el paquete de datos).

### E3. Código (bloques con cabecera `// R29` de este carril; `.cpp` CRLF, edición con python binario `assert count==1`)

1. **edit** `src/peds/PlayerPed.h` (~94, junto a `ViceExtCrouchSideHeading`):
   declarar `static bool ViceExtIsAiming();`.
2. **edit** `src/peds/PlayerPed.cpp` (~77-90, junto a `ViceExtCanAim`): definir
   `ViceExtIsAiming()` = `padUsed->GetTarget()` + `ViceExtCanAim(tipo, info)` +
   lo que el port del axis defina (C18, cuando entre). Consumidores obligados:
   `odAim` de `ViceExtCrouchAnim` (~3796), los gates de la rueda (~3830-3890),
   el presupuesto/pose (~3451-3540), el corte de giro (~3816) y `CROUCH2 mira=`
   — todos leen SOLO este helper (hoy cada uno lee `GetTarget()` por su cuenta,
   que es el síntoma G).
3. **edit** `src/peds/PedFight.cpp:180-235` (bloque R28 de
   `SetPointGunAt`/`PointGunAt`): `ViceExtCrouchShooting()` exige
   `WEAPONFLAG_CROUCHFIRE`; sin el flag manda C18-1 literal (sin disparo
   agachado forzado) — es la armonización §3, que el plan axis consumirá.
4. **edit** `src/peds/PlayerPed.cpp` presupuesto R28 (~3451-3540):
   `AimPose`/`AimPoseOff` pasan a pedir `ANIM_WEAPON_CROUCH` (grupo
   `ANIM_GROUP_MAN`) con `BlendAnimation(..., 4.0)` al apuntar agachado sin
   disparar (C18-4, spec del mod: `WEAPON_crouch` es la POSE); `*_crouchfire`
   solo con gatillo y `*_crouchreload` solo recargando (la parte valiosa de R28
   que el motor ya usa). El presupuesto se conserva: una sola parcial propia a
   la vez, `otros+peso <= 1.02`.
5. **edit** `src/peds/PlayerPed.cpp` `ViceExtCrouchSideHeading` (~3199-3260) y
   su uso en `PlayerPed.cpp:1173`: sin apuntar, el cuerpo gira hacia la
   dirección de movimiento en las **8 direcciones** (quitar el corte 50-130° y
   el `giro=5` «no-lateral» de `VICEEXT_CROUCH_SIDE_DEG`/`BACK_DEG`); destino de
   giro calculado en base MUNDO **una vez por petición**, sin realimentar con
   `m_fRotationCur` (círculo del handoff §6.5). Apuntando: sin giro
   (`giro=3`, lo manda el axis).
6. **edit** `src/peds/Ped.cpp:1461-1580` (bloque R20c): el giro de laterales
   pasa del snap R26d (`Cur = Dest`) a giro suave con el `headingRate` del
   motor (la traza `hdgr=15` ya existe — sin snap visible).
7. **edit** `src/peds/PlayerPed.cpp` rueda (`ViceExtCrouchAnim` ~3830-3890):
   retirar el lock R22 «una por pulsación» → re-lanzable manteniendo
   apuntar+lateral, cadencia = duración del clip (≈931 ms a rate 1; el `.cs`
   re-dispara con `WAIT 500` entre triggers); gates desde `ViceExtIsAiming()`
   (destraba los 6 `skip motivo=sin-mira arma=0`); filtro de armas fiel (≥17,
   sin 28-33) salvo D4; la traza de rueda añade el peso real del clip (debe
   entrar > 0,5 con `RollClearPartials` limpiando).
8. **edit** `src/peds/PlayerPed.cpp` `ViceExtCrouchRateFor` (~3360): con D1
   elegido y la raíz reescalada (E1), `VICEEXT_CROUCH_CADENCE` 1.70f → 1.0f
   (fin del ×1,7 = fin del patinaje); si D1=(b), también
   `VICEEXT_CROUCH_WALK_MPS` (~3154) 1.13f → 1.5f. Actualizar el comentario de
   R26c y las medidas de `ViceExtCrouchClipSpeed` (~3330) a la raíz nueva.
9. **edit** `src/peds/PlayerPed.cpp` CROUCHMOVE (~3940+): apuntando + mando,
   clips `GunMove_FWD/BWD/L/R` (ya servidos, 1,85/±1,80 m por ciclo) elegidos
   por ángulo — mapearlos por nombre como R27 hizo con `GunCrouchFwd/Bwd` —,
   velocidad = la del aim-walk de pie (tope R9), encaramiento = cámara (axis);
   lateral con gates de rueda → rueda (spec). Sin apuntar manda E3.5.

### E4. Verificador, marcas y traza

- **edit** `gta_vc_browser/tools/viceext-log-check.py` (bloque R27, ampliar):
  `pies=` ≈ velocidad/raíz servida sin multiplicador (mata A); `giro=0` en las
  8 direcciones sin apuntar y `giro=3` apuntando (mata F); ≥2 ruedas seguidas
  `motivo=ok` con peso > 0,5 (mata C); `pose=WEAPON_crouch` con `nomarma=`
  vacío al apuntar sin disparar y `nomarma=*_crouchfire` solo con gatillo (mata
  D); mantener `otros+peso<=1.02`, banda de velocidad (1,13±15 % o la de D1),
  `mvec≈obju`, `maxfr<0.05`.
- **edit** `gta_vc_browser/tools/check-served-build.sh`: marca nueva para la
  raíz reescalada + `dataTag` (junto a las marcas R27/R28 existentes).
- **edit** `src/peds/PlayerPed.cpp` traza `CROUCH2`: añadir `pies=` (rate ×
  raíz servida) y `pasos=` si procede (diagnóstico de audio §4.4:
  `CPed::PlayFootSteps` con los clips del mod).

### E5. Build, autorizaciones y riesgos

- **build**: objeto único `cd gta_vc_browser/build/web && ninja
  src/CMakeFiles/reVC.dir/peds/PlayerPed.cpp.o` (más `Ped.cpp.o` y
  `PedFight.cpp.o` según bloques); luego `bash gta_vc_browser/build.sh` con
  `export PATH=/c/Users/s0rno/emsdk/upstream/emscripten:...` (emsdk_env.sh no
  basta). `VERSION` de `gta_vc_browser/web/lib/index.js` sube en **cada** build
  (hoy `2026-09-26-swim9`); `dataTag` solo en E2.
- **autorizaciones** (regla 0.4): ejecución solo con el gate «ejecuta el plan»;
  E1/E2 solo con D2=sí; el jugador mide con su partida + log (sin arnés).
- **riesgos ya pagados, no repetir**: `GetAnimation` sin check de rango (solo
  ids/clips verificados con `ifp_inspect.py`, nunca inventados); `ViceExtCrouchAnim`
  se llama 2 veces/frame (el dedupe por `CTimer::GetFrameCounter()` se
  conserva); NADA de control-flow nuevo dentro de `LoadAllRequestedModels`
  (Asyncify); `BlendAnimation` reinicia clips terminados (poses solo si no están);
  `bIsDucking` no se clava (cancelaciones intactas); unidades m/s ÷ 50 al
  volcar a `m_vecMoveSpeed` (`METERS_PER_SECOND_TO_GAME_SPEED`).
- **intocables**: `Cam.cpp`/`Camera.cpp`/`Hud.cpp`/`Pad.cpp` (carril axis),
  `ViceExtSwim*` (carril nado), auto-centrado de cámara y recoil; nada de
  hooks/direcciones (port por atribución, `docs/mods/ATTRIBUTION.md`).

## Closure (persistent memory)

> Se rellena al cerrar el plan. Sin esta entrada el plan no se da por
> terminado (DoD).
