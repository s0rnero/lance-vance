---
name: camara-coche-sin-lucha
status: EXECUTED
type: feature
domain: camera
owner_rules: .agents
created: 2026-09-25
---

# Cámara de coche: mirar con el ratón sin lucha + recentrado pasivo (1,5 s desde ve75)

Fecha: 25/09/2026 · build base `ve64` (árbol actual, `Cam.cpp` 15:15). Petición
literal del jugador:

> "que al mover la camara en el coche el re centrado no se aplique? que me deje
> mover la camara como quiera en el mouse y despues de que termine el movimiento
> finalmente aplique el re centrado 2 segundos despues"

Y antes, de la 20ª sesión:

> "cuando yo mueva mi mouse debe estar desactivado y activarse 2 segundos despues
> de que deje de mover el mouse"

**Sin botón dedicado.** Decisión del jugador 25/09: no hace falta.

## Diagnóstico (informe 25/09, medido en su sesión de las 20:21)

- El pasivo (entonces 2 s) y el botón estuvieron **callados toda la partida**
  (1 sola línea `camauto2`, `auto=0 pedido=0`): la "lucha" no viene de ahí.
- Vías reales que recentran mientras miras:
  1. **`m_bUseTransitionBeta`** (`Camera.cpp:2664-2670` → `Cam.cpp:2349`): al
     entrar al coche, `Beta = TargetOrientation` cada frame mientras dure.
  2. **Snap de mirar** (`Cam.cpp:399-420`): RMB (mirar atrás), Q/E (mirar
     lados) teletransportan `Source`. RMB se toca sin querer al apuntar.
  3. **Radio = centrar** (`Pad.cpp:3405` + `ControllerConfig.cpp:205-206, 768`):
     `LeftShoulder1` (Insert/R) = `ForceCameraBehindPlayer()` = recentrado
     instantáneo. Confirmado con el jugador: la radio no debe recentrar.
- El pasivo exige `vel > 1.0`: parado nunca recentra (menor, pero se pide
  "2 s después de soltar", sin condición de velocidad).

## Comportamiento pedido (contrato)

1. Mientras el ratón (o Q/E/RMB) estén en uso, **ninguna** fuerza recentra.
2. Al soltar, 1,5 s de quietud → `WellBufferMe(TargetOrientation, 0.1/0.06)`.
   (ve75: el jugador pidió bajarlo de 2 s a 1,5 s.)
3. **La radio ya no recentra** (Insert/R sólo cambian emisora).
4. Vale también parado en el coche.
5. Q/E y RMB siguen mirando a los lados/atrás (no se quita esa función).

## Cambios (un solo fichero: `src/core/Cam.cpp`, Process_Cam_On_A_String)

- **C1 · fuera el botón**: el `btnReq` de `ForceCameraBehindPlayer()` deja de
  fijar `Rotating` en esta cámara (la radio ya no mueve la cámara). El pasivo
  hereda su rol. Otros usos de esa función (a pie, string-cam de serie) no se
  tocan.
- **C2 · pasivo sin condición de velocidad**: `driving && moving` → sólo
  `driving` (vel >= 0). La gracia sigue siendo el delay sin "mirar"
  (1500 ms desde ve75).
- **C3 · detector honesto**: `orbiting` ya incluye ratón por ventana + crudo +
  `useMouse` + offsets aplicados + Q/E/RMB (`keyLook`). Se conservan. Se añade
  traza una-por-cambio `BETASNAP motivo=transicion|mirar-atras|snap-lados` en
  los otros dos focos (transición y `CCam::Process` look-flags) para que la
  próxima partida nombre cualquier vía que quede viva.
- **C4 · transición**: mientras `m_bUseTransitionBeta`, si el jugador mueve el
  ratón, se libera el pin (se apaga `m_bUseTransitionBeta` local: el ratón manda
  desde el primer frame). El hold de 5 frames al entrar (A, ve59) se respeta.

## Verificación (sin sondas de Chrome; el jugador mide)

- **PASS del jugador**: en coche, mover el ratón y parar → nada empuja mientras
  miras; ~1,5 s después la cámara vuelve sola; la radio no mueve la cámara.
- **Log**: `camauto2 auto=1 ... pedido=0` (sólo pasivo), 0 `BETASNAP` con
  `motivo=transicion` fuera del momento de entrar, y en el verificador
  `camauto pedido` pasa a 0 siempre (se actualiza su leyenda).
- `check-served-build.sh` gana la marca `BETASNAP`.

## Riesgos / no tocar

- No se toca `Pad.cpp`/`ControllerConfig.cpp` (la radio conserva su binding
  vanilla; es la cámara la que deja de leer ese botón).
- No se toca el recoil ni las cámaras a pie.
- `.cpp` = CRLF (edición con python binario, `assert count == 1`).
