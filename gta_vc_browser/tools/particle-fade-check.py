#!/usr/bin/env python3
"""Comprueba que el fundido de particulas dura lo mismo a 30/60/90/120 FPS.

El motor avanza m_nFadeToBlackTimer / m_nFadeAlphaTimer solo cuando
CTimer::GetLogicalFramesPassed() devuelve 1, es decir, en frames logicos de
30 Hz. A 60/90/120 FPS hay frames renderizados sin tick logico.

Uso:
    python gta_vc_browser/tools/particle-fade-check.py
"""

import sys

FPS_RATES = [30, 60, 90, 120]
TOLERANCE = 0.10
MAX_FRAMES = 10000


def logical_ticks(fps):
    """Genera 0/1 por frame renderizado, con 30 ticks por segundo."""
    acc = 0.0
    while True:
        acc += 30.0 / fps
        if acc >= 1.0:
            acc -= 1.0
            yield 1
        else:
            yield 0


def fade_seconds(fps, timer, start=255):
    value = start
    for frames, tick in enumerate(logical_ticks(fps), 1):
        value -= timer * tick
        if value <= 0:
            return frames / float(fps)
        if frames > MAX_FRAMES:
            return None
    return None


def check(name, timer, start=255):
    base = fade_seconds(FPS_RATES[0], timer, start)
    failed = False
    for fps in FPS_RATES:
        seconds = fade_seconds(fps, timer, start)
        if seconds is None:
            print("%s fps=%d sin fin" % (name, fps))
            failed = True
            continue
        error = abs(seconds - base) / base
        status = "ok" if error <= TOLERANCE else "fail"
        if status == "fail":
            failed = True
        print("%s fps=%d t=%.4fs esperado=%.4fs error=%.1f%% %s" %
              (name, fps, seconds, base, error * 100.0, status))
    return failed


def main():
    failed = False
    failed |= check("fade-to-black I=10", 10)
    failed |= check("fade-alpha IA=12", 12)
    if failed:
        print("FAIL")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
