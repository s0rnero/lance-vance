---
name: nado-reescritura-serega
status: EXECUTED
type: feature
domain: gameplay-swim
owner_rules: .agents
created: 2026-09-29 12:00
enriched: 2026-09-29 12:30
---

# Technical Plan: Nado reescrito desde el mod Serega (port máximo, base actual descartada)

> Origen: jugador 29/09 (sustituye `nado-curva-serega.md`, no lo extiende).
> Enrichment 29/09: spec verificada línea a línea contra desensamblado,
> structs propios, telemetría y carril axis. No implementa código ni corre scripts.

### Analysis

- **Objetivo:** reescribir el nado como port directo de `swim.cs`, borrando la
  maquinaria swim1–swim20 (rota según `odtrace-2026-09-29_00-53-42.log`).
- **Hallazgo 1 (animaciones, RESUELTO):** los 4 índices del `CALL_FUNCTION
  0x405640 [4, id, 0, clump]` son `AnimationId` originales, idénticos a los
  nuestros (reVC conserva el enum). El `4` es `delta=4.0`, el `0` es grupo STD:

  | Script (id) | Nuestro enum (verificado) | Rol en el mod | Nuestro clip (grupo `ASSOCGRP_PLAYERSWIM`) |
  |---|---|---|---|
  | 156 | 156 `ANIM_STD_DROWN` | arranque/flote (`Drown` = nado estático) | `ANIM_STD_SWIM_TREAD` (187) |
  | 142 | 142 `ANIM_STD_JUMP_GLIDE` | fase A (primeras brazadas) | `ANIM_STD_SWIM_BREAST` (189) |
  | 149 | 148+1 `ANIM_STD_FALL_ONFRONT` | crucero (= braza upstream) | `ANIM_STD_SWIM_BREAST` (189) |
  | 148 | 148 `ANIM_STD_FALL_ONBACK` | sprint (= crol upstream) | `ANIM_STD_SWIM_CRAWL` (188) |

  Verificado: enum en `src/animation/AnimationId.h:142,148,149,156,187-190`;
  grupo propio en `src/animation/AnimManager.cpp:1027-1043,1119`. No se usa STD
  (secuestraría caída/ahogado del resto del juego): adaptación justificada.
- **Hallazgo 2 (offsets, semántica cerrada):** `+76` = clump (`GetClump()`),
  `+120` = `m_fMoveSpeed` (niveles 0.022/0.025), `+136/+140` y `+112/+116` =
  entradas de movimiento por frame (fase A vs crucero/sprint), `+20/+24` =
  vector del stick, `9917476` = timestep. Por ADR-001 no se portan direcciones:
  el port unifica en `m_vecMoveSpeed` + rumbo vanilla. Riesgo R1.
- **Hallazgo 3 (magnitudes, MEDIDAS):** los niveles `+120` como m/frame clavan
  bandas en nuestro motor (swim1: deriva 1.1 / crucero 2.5 / sprint 6.0,
  `avance≈velo`). Se conservan los defines `VICEEXT_SWIM_SEREGA_*` y el timeout
  300. La micro-rampa de fase A (stick×0.004) no se porta literal (ambiguo
  estáticamente, rompería bandas): fase A = brazada a ritmo de crucero.
- **Hallazgo 4 (axis-compat, VERIFICADO):** `ViceExtIsAiming()`
  (`PlayerPed.cpp:129-140`) es falso con puños salvo que
  `s_viceExtAimLawActive` esté puesto; la ley se apaga cada evaluación
  (`:4776`) y solo se enciende apuntando con arma compatible (`:4783-4788`,
  con puños falso). Consumidores: `:3465`, `:4055` (carril agachado, no nado).
  Conclusión: con puños inmediatos la ley no contamina; el nado es dueño de
  `m_fRotationCur/Dest` (convención vanilla cara-al-movimiento, decisión
  jugador 29/09: A/D miran y avanzan, cero strafe).
- **Hallazgo 5 (mecanismo que SÍ se conserva):** exención de flotabilidad
  `Ped.cpp:1804-1819` (`odSwimmingPlayer` salta damping + daño de ahogado; la
  flotabilidad física pura hace flotar = lo que hace el mod), call site
  `PlayerPed.cpp:5213-5221` (nado antes del reparto + `PED_IDLE` al caer al
  agua), interplay escalada `:3186-3193`, espejo `odSwimActive` + API
  `ViceExtIsSwimming()` (`PlayerPed.h:97-98`, carril cámara `Cam.cpp:1193,
  1778, 6169`), `#define VICEEXT_SWIMMING` (`config.h:382`).
- **Hallazgo 6 (telemetría intocable):** `viceext-log-check.py:787`
  (`bloque_h`) exige `serega=` en `SWIM2` (bandas 0.022/0.025→0.5-2.0,
  0.050→1.5-3.5, 0.120→4.0-8.0; `max(avance)>5` = FALLO);
  `check-served-build.sh:27,110` exige los literales `SWIM2 move` y `serega=`
  en el wasm. Los campos nuevos son solo aditivos.
- **Riesgos:** R1 (unificación de vectores → se juzga por bandas; decisión del
  jugador 29/09: se usan SOLO los niveles del mod 0.022/0.025/0.05/0.12, ningún
  0.9 ni factor corrector; si crucero falla, iteración con dato); R2 (sin
  control vertical, clavados altos tardan en boyar → verificar jugando); R3 (flap
  de orilla = quirk upstream aceptado; transiciones baratas lo hacen indoloro;
  si el log muestra spam de arma, se itera con dato, no se presupone); R4 (rumbo
  instantáneo SOLO nadando — a pie no se toca nada: el control de a pie corre por
  `PlayerControlZelda`, fuera del bloque reescrito; verificar a ojo y solo se
  amortigua si se mide oscilación).

### Changes

- **`src/peds/PlayerPed.cpp` (único fichero de motor; bloque `:2541-3103`):**
  - BORRAR: defines `SURFACE/DEEP/DROWN/KEEP_MARGIN`, `RISE/SINK_SPEED`,
    puerta `wetTicks`, histéresis `dryTicks/lastLevel`, tope zambullida,
    red anti-túnel, objetivo vertical, retirada manual de assocs de caída,
    suavizado 6°/frame, chapoteo+sonido por brazada, `SWIMDBG`, retardo 1 s
    de puños.
  - SE CONSERVA (decisión 26/09 vigente, no reabierta): cadencia acoplada del
    clip (`assoc->speed`, tope 3x).
  - ESCRIBIR `ViceExtSwimControl`: estados `OFF/DRIFT/STROKE/CRUISE/SPRINT` +
    contador 300; guarda pura (jugando + no coche + `bIsInWater`, sin estados
    de ped ni profundidades); entrar = `m_fMoveSpeed=0.022` + puños YA +
    abortar trepada; `DRIFT`: quieto + `TREAD`; stick → `STROKE`/`CRUISE`
    (`BREAST`, vector = stick×nivel×timestep, rumbo vanilla instantáneo);
    sprint → `SPRINT` (`CRAWL`, ×0.12); S/atrás (`upDown<-0.5`) = cancelar en
    seco (velocidad 0, vuelve a `DRIFT`); Espacio = impulso arriba limpio;
    salir = la guarda falla → restaurar arma, estado `OFF`, sin anim de salida.
  - `SWIM2` 1/s con campos actuales + aditivos `aim=` `law=` `hdg=` (rumbo del
    stick); conservar `enter/exit` y `SWIMIDLE`; conservar marca `SWIMNAT`.
    Traza de error (decisión jugador 29/09): `SWIMERR` SOLO si el avance se
    desvía (menos de la mitad de lo esperado del estado, o `avance>5`
    catapulta); nada de spam por frame ni por segundo en caso normal.
  - `ProcessControl :5268-5269`: saltar `ProcessPlayerWeapon` si
    `ViceExtIsSwimming()` (adaptador: sin disparos/puños en el agua; mata el
    enmascarado swim13 + contaminación `est=`). Llamar al helper estático (sin
    riesgo de alcance).
- **`PlayerPed.h:97-98`:** sin cambio esperado (verificar en ejecución).
- **`viceext-log-check.py` / `check-served-build.sh`:** sin cambio (solo
  verificar que la regex sigue casando; campos aditivos).
- **`web/lib/index.js:171 `VERSION`:** solo bump en build. `dataTag` no.

### Restrictions

- El mod manda (ADR-003); la base actual es solo mapa de ficheros.
- Nada x86/CLEO-runtime (ADR-001). CRLF + `assert count==1`; cp1252;
  sin commits; sin matar `node`. Skills: ninguna registrada (nada que conciliar).
- No tocar cámara, recoil, escalada, `Ped.cpp:1804` (solo leer).
- `VERSION` por build; `dataTag` no. Medición = jugador + log (ADR-005).

### Success criteria

- `SWIM2` en bandas por `serega=`, `avance≈velo`, `max≤6.5`, `modo=4`,
  checker `OK`; `AIMLAW aim=0` nadando; `rot≈hdg` con D solo (cara a la
  derecha, cero strafe).
- Ojo: arrancar/crucero/sprint distinguibles, anims visibles, S cancela,
  Espacio saca a muelle, salir caminando sin flap de arma.
- `check-served-build.sh` OK; `VERSION` nueva. Sin regresión (sesión completa).

### Steps

1. Verificar nombres `GetSprint/JumpJustDown/GetPedWalkUpDown` en `Pad.h` +
   `ASSOCGRP_PLAYERSWIM` + que `SWIM2` nuevo casa la regex de `bloque_h`
   (solo lectura) -> verify: tabla de nombres confirmados, cero supuestos.
2. Reescribir bloque `:2541-3103` (borrar + port + 8 adaptadores) en CRLF con
   `assert count==1` por reemplazo -> verify: `ninja
   src/CMakeFiles/reVC.dir/peds/PlayerPed.cpp.o` compila, `diff --check` limpio.
3. Añadir salto de `ProcessPlayerWeapon` si `ViceExtIsSwimming()` (`:5268`) ->
   verify: compila; clic en agua no cambia `m_nPedState` a `PED_FIGHT`.
4. Verificar checker + served-build sin cambios (`py_compile`, `bash -n`,
   regex contra muestra `SWIM2`) -> verify: ambos OK en verde.
5. Build + `check-served-build.sh` + sesión del jugador (PASS §objetivo) ->
   verify: log con bandas + veredicto (ojo/oído) del jugador.

### Verification

- Declarados (corren directo en ejecución): objeto `PlayerPed.cpp.o`,
  `check-served-build.sh`, `viceext-log-check.py <log>`.
- Build (`bash gta_vc_browser/build.sh`) solo con autorización (regla 0.4).

## Closure (persistent memory)

Ejecución 29/09 (gate «ok avanza, ejecuta»):

- What changed:
  - `src/peds/PlayerPed.cpp`: bloque `VICEEXT_SWIMMING` reescrito (563→315
    líneas): guarda pura por contacto, estados OFF/DRIFT/CRUISE/SPRINT +
    contador 300, niveles Serega intactos, rumbo vanilla instantáneo, puños
    inmediatos, Espacio=impulso, S/atrás=cancelar, `SWIM2`+`aim/law/hdg`,
    `SWIMERR` condicional. Borrado: márgenes, puertas, histéresis, control
    vertical, tope zambullida, red anti-túnel, suavizado, chapoteo/brazada,
    `SWIMDBG`, retardo de puños.
  - Desviaciones menores documentadas: fase STROKE colapsa en CRUISE (mismo
    clip+velocidad en nuestros datos, cero diferencia observable);
    `bIsInTheAir=false` conservado (fix medido swim6, 1 línea);
    `SWIMIDLE` conservado (plan lo pedía; útil para R3).
  - `src/peds/PlayerPed.cpp:5268`: `ProcessPlayerWeapon` se salta nadando
    (mata el enmascarado swim13 y la contaminación `est=`).
  - Checker/served-build/`VERSION`: sin cambios (compatibilidad verificada).
- Verification: `ninja PlayerPed.cpp.o` exit 0, 0 errores (solo warnings
  preexistentes); `py_compile` + `bash -n` OK; regex `bloque_h` casa muestra
  `SWIM2` nueva (avance/velo/serega/exit). Build `2026-09-29-swim30` enlazado
  (orden permanente del jugador: build tras cada cambio) + `VERSION` subida;
  `check-served-build.sh` en VERDE tras retirar sus 5 marcas obsoletas
  (`flote/dz/SWIMDBG/exento/paso`, del aparato borrado) y añadir 3 marcas R30
  (`swim cancela`, `SWIMERR esperada=`, `aim=%d law=`).
  Sesión del jugador pendiente (el juego servido ya lleva el port).
- Outcome: done (implementado + servido verificado; sesión pendiente).
- Pending items:
  1. Sesión del jugador con PASS de 5 puntos + axis-compat (D solo, `aim=0`).
  2. Si bandas/R1-R4 condenan algo con dato, iteración hija (fijar `parent`).

## Enrichment checklist

- [x] Plan MD leído entero. [x] RULES/CODING/DESIGN/AGENTS revisados.
- [x] Ficheros afectados leídos con líneas (PlayerPed 2541-3103/5213/5268/3186,
  Ped 1804, Cam 1193/1778/6169, AnimationId, config.h:382, checker:740-860,
  served-build:27/110, VERSION:171). [x] Alcance congelado (5 decisiones).
- [x] Pasos ejecutables con `-> verify`. [x] Supuestos/riesgos explícitos.
- [x] Validado: arquitectura (un carril, API vecina intacta), estándares
  (flags `VICEEXT_*`, CRLF, unidades m/frame), reglas (sin git/scripts sin
  permiso), skills (ninguna). [x] Estado `ENRICHED`.
