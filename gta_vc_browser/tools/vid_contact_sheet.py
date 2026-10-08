import argparse
import base64
import glob
import os


def build(patron, paso, cols, salida, titulo, desde):
    rutas = sorted(glob.glob(patron))
    if not rutas:
        raise SystemExit("sin fotogramas: " + patron)
    partes = []
    for i, ruta in enumerate(rutas):
        t = desde + i * paso
        mm = int(t) // 60
        ss = t - mm * 60
        with open(ruta, "rb") as f:
            datos = base64.b64encode(f.read()).decode("ascii")
        partes.append(
            '<figure><img src="data:image/jpeg;base64,%s"><figcaption>%02d:%05.2f '
            "| %s</figcaption></figure>" % (datos, mm, ss, os.path.basename(ruta))
        )
    html = (
        "<!doctype html><html><head><meta charset=utf-8><title>%s</title>"
        "<style>body{background:#111;color:#eee;font:13px monospace;margin:8px}"
        "div{display:grid;grid-template-columns:repeat(%d,1fr);gap:6px}"
        "figure{margin:0}img{width:100%%;display:block}"
        "figcaption{background:#000;padding:2px 4px}</style></head>"
        "<body><h1>%s</h1><div>%s</div></body></html>"
        % (titulo, cols, titulo, "\n".join(partes))
    )
    with open(salida, "w", encoding="utf-8") as f:
        f.write(html)
    print("%s fotogramas -> %s" % (len(rutas), salida))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--glob", required=True)
    p.add_argument("--paso", type=float, default=5.0)
    p.add_argument("--cols", type=int, default=5)
    p.add_argument("--desde", type=float, default=0.0)
    p.add_argument("--salida", required=True)
    p.add_argument("--titulo", default="contacto")
    a = p.parse_args()
    build(a.glob, a.paso, a.cols, a.salida, a.titulo, a.desde)


if __name__ == "__main__":
    main()
