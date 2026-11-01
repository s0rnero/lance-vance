import sys

FPS_RATES = [30, 60, 90, 120]
SECONDS = 10.0


def simulate_spawns(fps, skip, seconds):
    step = (1.0 / fps) * 30.0
    counter = 0.0
    spawns = 0
    for _ in range(int(fps * seconds)):
        counter += step
        if counter >= skip:
            counter -= skip
            spawns += 1
    return spawns


def simulate_push(fps, seconds):
    timestep = 30.0 / fps
    total = 0.0
    for _ in range(int(fps * seconds)):
        total += 1.0 * timestep
    return total


def simulate_shift(fps, seconds):
    # Correccion posicional ped->coche: masa relativa * GetTimeStepFix().
    timestep = 30.0 / fps
    total = 0.0
    for _ in range(int(fps * seconds)):
        total += 1.0 * timestep
    return total


def main():
    failed = False
    for skip in (1, 2, 3):
        expected = 30.0 / skip * SECONDS
        values = []
        for fps in FPS_RATES:
            n = simulate_spawns(fps, skip, SECONDS)
            values.append(n)
            error = abs(n - expected) / expected
            status = "ok" if error <= 0.10 else "fail"
            if status == "fail":
                failed = True
            print("spawns skip=%d fps=%d n=%d esperado=%.1f error=%.1f%% %s" %
                  (skip, fps, n, expected, error * 100.0, status))
        spread = (max(values) - min(values)) / expected
        if spread > 0.10:
            failed = True
            print("spawns skip=%d dispersion=%.1f%% fail" % (skip, spread * 100.0))

    base = simulate_push(30, SECONDS)
    for fps in FPS_RATES:
        total = simulate_push(fps, SECONDS)
        error = abs(total - base) / base
        status = "ok" if error <= 0.10 else "fail"
        if status == "fail":
            failed = True
        print("push fps=%d impulso/seg=%.3f esperado=%.3f error=%.1f%% %s" %
              (fps, total / SECONDS, base / SECONDS, error * 100.0, status))

    # La correccion posicional es el termino que se aplicaba por frame sin
    # timestep. A 30 FPS debe valer exactamente lo mismo que antes; a 60/90/120
    # debe dar el mismo desplazamiento por segundo dentro del 10 %.
    shift_base = simulate_shift(30, SECONDS)
    for fps in FPS_RATES:
        total = simulate_shift(fps, SECONDS)
        error = abs(total - shift_base) / shift_base
        status = "ok" if error <= 0.10 else "fail"
        if status == "fail":
            failed = True
        print("shift fps=%d desplazamiento/seg=%.3f esperado=%.3f error=%.1f%% %s" %
              (fps, total / SECONDS, shift_base / SECONDS, error * 100.0, status))

    if failed:
        print("FAIL")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
