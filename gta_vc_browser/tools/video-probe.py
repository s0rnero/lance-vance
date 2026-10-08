#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Sonda del VÍDEO del mod (PC) — mide comportamiento, no lo interpreta.

Para qué: el mod no trae código, así que su comportamiento real solo se puede
medir de dos sitios: su `ped.ifp` (clips) y sus vídeos. Esta herramienta saca del
vídeo tres números por segundo:

  h        altura en píxeles del personaje (la camiseta verde de Tommy) → sirve
           para saber CUÁNDO va agachado (agachado ≈ 25-35% más bajo).
  scroll   píxeles/segundo que se desplaza el suelo bajo sus pies (correlación de
           fases de una franja de asfalto). Es proporcional a su velocidad real.
  cadencia ciclos/segundo del movimiento de las piernas (autocorrelación de la
           diferencia entre fotogramas de la zona de las piernas): dice a qué
           RITMO se reproduce el clip, que es lo que hace que los pies no patinen.

Uso:
    python gta_vc_browser/tools/video-probe.py <vídeo> [inicio] [fin]
"""

import sys

import cv2
import numpy as np

# Zona de juego (fuera del HUD): en 1920x1080 el radar está abajo a la izquierda
# y el dinero/vida arriba a la derecha.
PLAY = (180, 60, 1740, 1000)     # x0, y0, x1, y1


def camiseta(frame):
    """Máscara de la camiseta de Tommy (verde-azulado saturado) dentro del juego."""
    x0, y0, x1, y1 = PLAY
    roi = frame[y0:y1, x0:x1]
    hsv = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)
    mask = cv2.inRange(hsv, (60, 60, 70), (105, 255, 255))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
    n, labels, stats, cent = cv2.connectedComponentsWithStats(mask, 8)
    if n <= 1:
        return None, None, roi.shape
    i = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    return stats[i], (x0, y0), roi.shape


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    path = sys.argv[1]
    t0 = float(sys.argv[2]) if len(sys.argv) > 2 else 0.0
    t1 = float(sys.argv[3]) if len(sys.argv) > 3 else 1e9

    cap = cv2.VideoCapture(path)
    fps = cap.get(cv2.CAP_PROP_FPS) or 60.0
    cap.set(cv2.CAP_PROP_POS_MSEC, t0 * 1000.0)

    print('# t      h_px  y_bajo  scroll_px/s  cadencia_ciclos/s')
    prev_gray = None
    prev_legs = None
    legs_hist = []            # (t, señal) para la autocorrelación de la cadencia
    scroll_acc = 0.0
    t_scroll = 0.0
    last_t = t0

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        t = cap.get(cv2.CAP_PROP_POS_MSEC) / 1000.0
        if t > t1:
            break
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        stats, off, shape = camiseta(frame)
        if stats is None:
            prev_gray, prev_legs = gray, None
            continue
        x, y, w, h = stats[cv2.CC_STAT_LEFT], stats[cv2.CC_STAT_TOP], \
            stats[cv2.CC_STAT_WIDTH], stats[cv2.CC_STAT_HEIGHT]
        cx = off[0] + x + w // 2          # centro del torso (coords de pantalla)
        y_abajo = off[1] + y + h          # pies
        alto = h

        # ── scroll del suelo: franja de asfalto justo bajo los pies ──────────
        y0 = max(0, y_abajo + 5)
        y1 = min(gray.shape[0], y0 + 70)
        x0 = max(0, cx - 260)
        x1 = min(gray.shape[1], cx + 260)
        if prev_gray is not None and y1 > y0 + 8 and x1 > x0 + 20:
            a = np.float32(prev_gray[y0:y1, x0:x1])
            b = np.float32(gray[y0:y1, x0:x1])
            (dx, dy), _ = cv2.phaseCorrelate(a, b)
            scroll_acc += dx
            t_scroll += 1.0 / fps

        # ── cadencia: diferencia en la zona de las piernas ──────────────────
        ly0, ly1 = max(0, y_abajo - int(1.1 * alto)), y_abajo
        lx0, lx1 = max(0, cx - int(0.9 * alto)), min(gray.shape[1], cx + int(0.9 * alto))
        legs = gray[ly0:ly1, lx0:lx1]
        if legs.size and prev_legs is not None and legs.shape == prev_legs.shape:
            d = float(np.mean(cv2.absdiff(legs, prev_legs)))
            legs_hist.append((t, d))
        prev_legs = legs

        if t - last_t >= 1.0:
            cad = 0.0
            if len(legs_hist) > int(2.5 * fps):
                s = np.array([v for _, v in legs_hist[-int(2.0 * fps):]])
                s = s - s.mean()
                if s.std() > 0.01:
                    ac = np.correlate(s, s, 'full')[len(s) - 1:]
                    # primer pico tras el hueco mínimo (~0,25 s)
                    lo = int(0.25 * fps)
                    hi = min(len(ac) - 1, int(1.6 * fps))
                    if hi > lo:
                        k = lo + int(np.argmax(ac[lo:hi]))
                        if k > 0:
                            cad = fps / k
            print('%.2f  %5d  %6d  %11.1f  %.2f' % (t, alto, y_abajo,
                  scroll_acc / max(t_scroll, 1e-6), cad))
            scroll_acc, t_scroll = 0.0, 0.0
            last_t = t
        prev_gray = gray

    return 0


if __name__ == '__main__':
    sys.exit(main())
