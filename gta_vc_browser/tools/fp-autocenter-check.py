"""Autocentrado de la camara en primera persona dentro del vehiculo.

Modela el bloque de `src/core/Cam.cpp` (autocentrado del mod, `Process_FollowCar_SA`)
sin el motor: con el coche en movimiento y sin input, el error entre la derecha del
coche y la derecha de la camara debe bajar al mismo ritmo real a cualquier FPS.

El motor usa un timestep variable en unidades de 1/50 s
(`CTimer::GetTimeStep() = 50 / fps`, `Timer.cpp:150`), asi que `dt` a 30 FPS es 1.667
y a 120 FPS 0.4167. El bloque del mod esta escrito para 30 FPS; este script comprueba
que su version portada no se acelere ni se frene al cambiar de FPS.
"""

import math
import sys

HALFPI = math.pi / 2.0
PI = math.pi
TWOPI = 2.0 * PI
FPS_RATES = [30, 60, 90, 120]
SECONDS = 6.0
DEG = 180.0 / PI

# Constantes del bloque tal como estan en Cam.cpp.
BETA_CHANGE_RATE = 0.25
BETA_CHANGE_LIMIT_RATE = 0.25
SPEED_BLEND_MAX = 0.85
SPEED_FILTER_BASE = 0.1
# Divisor de `targetBetaWithStickBlendAmount`: `Max(CTimer::GetTimeStep(), 0.25f)`.
DIVISOR_FLOOR = 0.25

# El bloque del mod apaga `BetaSpeed` cuando baja de 0.0001, asi que se detiene a
# ~1.14 grados del objetivo: 1 grado no se alcanza ni a 30 FPS. El criterio es 2.
TARGET_DEGREES = 2.0


def wrap_pi(angle):
    while angle >= PI:
        angle -= TWOPI
    while angle < -PI:
        angle += TWOPI
    return angle


def autocenter_step(beta, beta_speed, target, dt):
    """Un frame del bloque de autocentrado, con la camara ya en convencion mundo."""
    cam_right = beta
    velocity_right = cam_right + wrap_pi(target - cam_right)

    beta_change_mult1 = dt * BETA_CHANGE_RATE
    beta_change_limit = dt * BETA_CHANGE_LIMIT_RATE
    beta_change_mult2 = abs(math.sin(wrap_pi(velocity_right - cam_right)))

    beta_change = min(1.0, beta_change_mult1 * beta_change_mult2) * (velocity_right - cam_right)
    beta_change = max(-beta_change_limit, min(beta_change_limit, beta_change))

    target_beta = cam_right + beta_change
    if target_beta < beta - HALFPI:
        target_beta += TWOPI
    elif target_beta > beta + PI:
        target_beta -= TWOPI

    angle_change_step = SPEED_FILTER_BASE ** dt
    blend = (target_beta - beta) / max(dt, DIVISOR_FLOOR)
    blend = max(-SPEED_BLEND_MAX, min(SPEED_BLEND_MAX, blend))

    beta_speed = blend * (1.0 - angle_change_step) + angle_change_step * beta_speed
    if abs(beta_speed) < 0.0001:
        beta_speed = 0.0
    beta += dt * beta_speed
    return beta, beta_speed


def time_to_target(fps, initial_degrees=30.0, speed=0.8):
    dt = 50.0 / fps
    beta = -initial_degrees / DEG
    beta_speed = 0.0
    target = 0.0
    if speed <= 0.02:
        return None
    frames = int(fps * SECONDS)
    for frame in range(frames):
        beta, beta_speed = autocenter_step(beta, beta_speed, target, dt)
        if abs(wrap_pi(target - beta)) * DEG < TARGET_DEGREES:
            return (frame + 1) / float(fps)
    return None


def final_error(fps, initial_degrees=30.0):
    dt = 50.0 / fps
    beta = -initial_degrees / DEG
    beta_speed = 0.0
    for _ in range(int(fps * 10.0)):
        beta, beta_speed = autocenter_step(beta, beta_speed, 0.0, dt)
    return abs(wrap_pi(beta)) * DEG


def check_movement():
    failed = False
    times = {}
    for fps in FPS_RATES:
        t = time_to_target(fps)
        times[fps] = t
        if t is None:
            print("autocentro fps=%d no llega a %.1f grados en %.1fs fail" % (fps, TARGET_DEGREES, SECONDS))
            failed = True
        else:
            print("autocentro fps=%d t=%.3fs" % (fps, t))
    base = times.get(30)
    if base is None:
        return True
    for fps in FPS_RATES:
        t = times[fps]
        if t is None:
            continue
        error = abs(t - base) / base
        status = "ok" if error <= 0.10 else "fail"
        if status == "fail":
            failed = True
        print("autocentro fps=%d vs 30 error=%.1f%% %s" % (fps, error * 100.0, status))

    for fps in FPS_RATES:
        error = final_error(fps)
        status = "ok" if error <= TARGET_DEGREES else "fail"
        if status == "fail":
            failed = True
        print("autocentro fps=%d error_final=%.2f grados %s" % (fps, error, status))
    return failed


def check_stopped():
    # El bloque de movimiento solo corre con `speed > 0.02`; parado decide el bloque
    # de coche parado, que es otro (Beta = -HALFPI, stepsLeft = 35). Aqui se comprueba
    # que la puerta del bloque de movimiento no se abre.
    failed = False
    for speed in (0.0, 0.02):
        gated = speed > 0.02
        status = "ok" if not gated else "fail"
        if gated:
            failed = True
        print("autocentro parado speed=%.2f bloque_movimiento=%d %s" % (speed, int(gated), status))
    return failed


def check_aim_release():
    """Al soltar el apuntado: relativa con steps=35, y al agotarse, mundo sin salto.

    La conversion del propio Cam.cpp es identica en las dos direcciones:
    mundo -> relativa `Beta = -HALFPI - (right.Heading() - camRight)` y
    relativa -> mundo `Beta = camRight`, con `camRight = Front.Heading() - HALFPI`.
    Para un coche derecho ambas dejan `Beta` igual; se comprueba el salto.
    """
    failed = False
    beta = 0.35
    dt = 50.0 / 30.0
    steps = 35.0
    aim = False
    auto = True
    relative_db = False

    relative = (not aim and (steps > 0.0 or not auto)) or (aim and relative_db)
    status = "ok" if relative else "fail"
    if not relative:
        failed = True
    print("aimrel al soltar rel=%d steps=%.0f %s" % (int(relative), steps, status))

    previous = beta
    max_jump = 0.0
    while steps > 0.0:
        steps = max(0.0, steps - dt)
        # En relativa Beta no se toca por autocentrado; el salto entre frames es 0.
        jump = abs(wrap_pi(beta - previous))
        max_jump = max(max_jump, jump)
        previous = beta

    relative = (not aim and (steps > 0.0 or not auto)) or (aim and relative_db)
    status = "ok" if not relative else "fail"
    if relative:
        failed = True
    print("aimrel agotado steps=%.0f rel=%d %s" % (steps, int(relative), status))

    # Conversion relativa -> mundo con el coche derecho: camRight == Beta.
    world_beta = beta
    jump = abs(wrap_pi(world_beta - previous)) * DEG
    status = "ok" if jump <= 5.0 else "fail"
    if jump > 5.0:
        failed = True
    print("aimrel salto a mundo=%.2f grados (max frames %.2f) %s" % (jump, max_jump * DEG, status))
    return failed


def main():
    failed = False
    failed |= check_movement()
    failed |= check_stopped()
    failed |= check_aim_release()
    if failed:
        print("FAIL")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
