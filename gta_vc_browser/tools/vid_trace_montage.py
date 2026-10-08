import argparse
import base64
import glob
import os
import re
import subprocess
import sys

LINEA = re.compile(r"^(\S+)Z?.*?kind=(\w+)\b")


def hms(t):
    partes = t.split(":")
    if len(partes) != 3:
        raise SystemExit("hora mal formada: %s" % t)
    return int(partes[0]) * 3600 + int(partes[1]) * 60 + float(partes[2])


def eventos(ruta, kinds, desde, hasta, logstart):
    salida = []
    with open(ruta, "r", encoding="utf-8", errors="replace") as f:
        for linea in f:
            m = LINEA.match(linea)
            if not m:
                continue
            if m.group(2) not in kinds:
                continue
            t = m.group(1)
            hh, mm, resto = t[11:13], t[14:16], t[17:].rstrip("Zz")
            seg = int(hh) * 3600 + int(mm) * 60 + float(resto) - logstart
            if desde is not None and seg < desde:
                continue
            if hasta is not None and seg > hasta:
                continue
            salida.append((seg, m.group(2), linea.strip()))
    return salida


def extrae(ffmpeg, video, t0, dur, fps, destino, tag):
    patron = os.path.join(destino, "%s_%%03d.jpg" % tag)
    cmd = [ffmpeg, "-hide_banner", "-loglevel", "error", "-y",
           "-ss", "%.3f" % t0, "-t", "%.3f" % dur, "-i", video,
           "-vf", "fps=%d" % fps, "-q:v", "3", patron]
    subprocess.run(cmd, check=True)
    return sorted(glob.glob(os.path.join(destino, "%s_*.jpg" % tag)))


def hoja(filas, cols, salida, titulo):
    partes = []
    for ruta, etiqueta in filas:
        with open(ruta, "rb") as f:
            datos = base64.b64encode(f.read()).decode("ascii")
        partes.append('<figure><img src="data:image/jpeg;base64,%s">'
                      "<figcaption>%s</figcaption></figure>" % (datos, etiqueta))
    html = ("<!doctype html><html><head><meta charset=utf-8><title>%s</title>"
            "<style>body{background:#111;color:#eee;font:12px monospace;margin:8px}"
            "div{display:grid;grid-template-columns:repeat(%d,1fr);gap:4px}"
            "figure{margin:0}img{width:100%%;display:block}"
            "figcaption{background:#000;padding:2px 4px}</style></head>"
            "<body><h1>%s</h1><div>%s</div></body></html>"
            % (titulo, cols, titulo, "\n".join(partes)))
    with open(salida, "w", encoding="utf-8") as f:
        f.write(html)
    print("%d fotogramas -> %s" % (len(filas), salida))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--log", required=True)
    p.add_argument("--video", required=True)
    p.add_argument("--kinds", required=True)
    p.add_argument("--offset", type=float, default=-9.5)
    p.add_argument("--logstart", default=None)
    p.add_argument("--pre", type=float, default=0.7)
    p.add_argument("--post", type=float, default=0.7)
    p.add_argument("--fps", type=int, default=30)
    p.add_argument("--desde", type=float, default=None)
    p.add_argument("--hasta", type=float, default=None)
    p.add_argument("--ffmpeg", default="ffmpeg")
    p.add_argument("--frames", default="gta_vc_browser/tmp/vid/montaje")
    p.add_argument("--salida", required=True)
    p.add_argument("--cols", type=int, default=8)
    p.add_argument("--titulo", default="montaje")
    a = p.parse_args()

    kinds = set(k.strip() for k in a.kinds.split(",") if k.strip())
    logstart = hms(a.logstart) if a.logstart else 0.0
    evs = eventos(a.log, kinds, a.desde, a.hasta, logstart)
    if not evs:
        raise SystemExit("sin eventos %s en %s" % (a.kinds, a.log))
    os.makedirs(a.frames, exist_ok=True)
    filas = []
    for i, (seg, kind, texto) in enumerate(evs):
        t0 = seg + a.offset - a.pre
        if t0 < 0:
            t0 = 0.0
        dur = a.pre + a.post
        tag = "%02d_%s" % (i + 1, kind)
        for ruta in glob.glob(os.path.join(a.frames, tag + "_*.jpg")):
            os.remove(ruta)
        for j, ruta in enumerate(extrae(a.ffmpeg, a.video, t0, dur, a.fps, a.frames, tag)):
            tv = t0 + j / float(a.fps)
            mm = int(tv) // 60
            ss = tv - mm * 60
            filas.append((ruta, "%s log=%.3f v=%02d:%05.2f %s" % (tag, seg, mm, ss, texto[:150])))
    hoja(filas, a.cols, a.salida, a.titulo)


if __name__ == "__main__":
    sys.exit(main())
