import subprocess
import sys


def energia(ruta, ancho=64, alto=36, fps=30, desde=0, hasta=None):
    cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error"]
    if desde:
        cmd += ["-ss", str(desde)]
    cmd += ["-i", ruta]
    if hasta:
        cmd += ["-t", str(hasta - desde)]
    cmd += [
        "-vf", "fps=%d,scale=%d:%d,format=gray" % (fps, ancho, alto),
        "-f", "rawvideo", "-pix_fmt", "gray", "-",
    ]
    salida = subprocess.run(cmd, capture_output=True, check=True).stdout
    tam = ancho * alto
    n = len(salida) // tam
    prev = None
    res = []
    x0, x1 = int(ancho * 0.25), int(ancho * 0.75)
    y0, y1 = int(alto * 0.20), int(alto * 0.85)
    for i in range(n):
        cuadro = salida[i * tam : (i + 1) * tam]
        if prev is not None:
            s = 0
            for y in range(y0, y1):
                base = y * ancho
                for x in range(x0, x1):
                    d = cuadro[base + x] - prev[base + x]
                    s += d if d >= 0 else -d
            res.append((s / float((x1 - x0) * (y1 - y0)), desde + i / float(fps)))
        prev = cuadro
    return res


def picos(res, ventana=9, minimo=1.0, maximo=40):
    orden = []
    for i, (v, t) in enumerate(res):
        a = max(0, i - ventana)
        b = min(len(res), i + ventana + 1)
        vecinos = [x[0] for x in res[a:b]]
        if v >= max(vecinos) and v > minimo:
            orden.append((v, t))
    orden.sort(reverse=True)
    return orden[:maximo]


if __name__ == "__main__":
    ruta = sys.argv[1]
    desde = float(sys.argv[2]) if len(sys.argv) > 2 else 0.0
    hasta = float(sys.argv[3]) if len(sys.argv) > 3 else None
    res = energia(ruta, desde=desde, hasta=hasta)
    print("cuadros=%d" % len(res))
    for v, t in sorted(picos(res), key=lambda p: p[1]):
        mm = int(t) // 60
        print("pico %02d:%06.2f  energia=%.2f" % (mm, t - mm * 60, v))
