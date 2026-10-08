#!/usr/bin/env python3
"""Estadísticas de capturas PNG (para las sondas del port, sin dependencias).

Sirve para convertir "en la captura se ve la sirena" en números: cuenta píxeles
rojos y azules **saturados** (los de una corona `TYPE_STAR` roja/azul), da su
centroide y, con `--diff`, el porcentaje de píxeles que cambian entre dos
capturas. Nada de OCR: sólo lo que se puede afirmar sin ver la imagen.

Uso:
  python tools/shot_stats.py <a.png> [b.png ...]
  python tools/shot_stats.py --diff <a.png> <b.png> [--umbral 24]
  python tools/shot_stats.py --serie <a.png> <b.png> <c.png> [...]
  Opciones: --region x0,y0,x1,y1 (en píxeles, para mirar sólo una zona;
            vale también con --serie, donde es lo que deja fuera el HUD)

Salida por captura: tamaño, brillo medio, nº de píxeles rojos y azules
saturados y su centroide (para saber DÓNDE están).
"""
import argparse
import struct
import sys
import zlib


def read_png(path):
    """-> (width, height, canales, bytes de píxeles (fila a fila))."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit("ERROR: %s no es PNG" % path)

    pos = 8
    idat = bytearray()
    w = h = depth = color = None
    while pos + 8 <= len(data):
        length, = struct.unpack_from(">I", data, pos)
        ctype = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if ctype == b"IHDR":
            w, h, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if depth != 8 or color not in (2, 6) or interlace != 0:
                raise SystemExit("ERROR: %s: sólo PNG de 8 bits RGB/RGBA sin "
                                 "interlace (depth=%d color=%d interlace=%d)"
                                 % (path, depth, color, interlace))
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"IEND":
            break
        pos += 12 + length

    canales = 3 if color == 2 else 4
    raw = zlib.decompress(bytes(idat))
    stride = w * canales
    out = bytearray(h * stride)
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        filt = raw[p]
        p += 1
        line = bytearray(raw[p:p + stride])
        p += stride
        if filt == 1:      # Sub
            for i in range(canales, stride):
                line[i] = (line[i] + line[i - canales]) & 0xFF
        elif filt == 2:    # Up
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif filt == 3:    # Average
            for i in range(stride):
                a = line[i - canales] if i >= canales else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif filt == 4:    # Paeth
            for i in range(stride):
                a = line[i - canales] if i >= canales else 0
                b = prev[i]
                c = prev[i - canales] if i >= canales else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return w, h, canales, bytes(out)


def stats(path, region=None):
    w, h, canales, px = read_png(path)
    x0, y0, x1, y1 = region if region else (0, 0, w, h)
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(w, x1), min(h, y1)
    rojo = azul = n = 0
    rojo_fuerte = azul_fuerte = 0
    cxf = cyf = 0
    sumr = sumg = sumb = 0
    cx = cy = 0
    xs = ys = 0
    for y in range(y0, y1):
        base = y * w * canales
        for x in range(x0, x1):
            i = base + x * canales
            r, g, b = px[i], px[i + 1], px[i + 2]
            n += 1
            sumr += r; sumg += g; sumb += b
            # "domina un canal": sirve para rojo/azul de una corona o de una
            # luz de vehículo. Se pide dominancia (no saturación pura) porque
            # el núcleo aditivo de una corona se va a blanco y el halo es lo
            # que conserva el color.
            if r > 120 and r - g > 50 and r - b > 50:
                rojo += 1; cx += x; cy += y
            elif b > 120 and b - r > 50 and b - g > 50:
                azul += 1; cx += x; cy += y
            # "dominante y brillante": lo que hace una corona aditiva (o un
            # fogonazo). Mucho más selectivo: en una calle normal no aparece.
            if r > 200 and r - g > 120 and r - b > 120:
                rojo_fuerte += 1; cxf += x; cyf += y
            elif b > 200 and b - r > 120 and b - g > 120:
                azul_fuerte += 1; cxf += x; cyf += y
    tot = rojo + azul
    centro = (round(cx / tot), round(cy / tot)) if tot else None
    if centro:
        for y in range(y0, y1):
            base = y * w * canales
            for x in range(x0, x1):
                i = base + x * canales
                r, g, b = px[i], px[i + 1], px[i + 2]
                if (r > 120 and r - g > 50 and r - b > 50) or \
                   (b > 120 and b - r > 50 and b - g > 50):
                    xs += (x - centro[0]) ** 2
                    ys += (y - centro[1]) ** 2
    dispersion = (round((xs / tot) ** 0.5), round((ys / tot) ** 0.5)) if tot else None
    totf = rojo_fuerte + azul_fuerte
    foco = (round(cxf / totf), round(cyf / totf)) if totf else None
    return dict(path=path, w=w, h=h, n=n, rojo=rojo, azul=azul,
                rojo_fuerte=rojo_fuerte, azul_fuerte=azul_fuerte,
                centro=centro, dispersion=dispersion, foco=foco,
                brillo=(sumr + sumg + sumb) / (3.0 * n) if n else 0.0,
                px=px, canales=canales)


def serie(paths, region=None):
    """Analiza una **serie** de capturas de la misma escena y cámara.

    Para cada píxel cuenta en cuántos fotogramas tiene *tinte* azul (o rojo) y
    devuelve los que están encendidos **en unos fotogramas y en otros no**:
    eso es exactamente un parpadeo, y es inmune a la deriva de la escena (el
    atardecer sube el rojo de TODO el cuadro, pero a todos los fotogramas por
    igual; un objeto azul fijo está en los N). Es la métrica que distingue una
    sirena de "hay cosas rojas en la calle".
    """
    imgs = [read_png(p) for p in paths]
    w, h, canales, _ = imgs[0]
    if any(im[0] != w or im[1] != h for im in imgs):
        raise SystemExit("ERROR: la serie mezcla tamaños distintos")
    N = len(imgs)
    x0, y0, x1, y1 = region if region else (0, 0, w, h)
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(w, x1), min(h, y1)
    cb = bytearray(w * h)
    cr = bytearray(w * h)
    for _w, _h, c, px in imgs:
        for y in range(y0, y1):
            base = y * w * c
            fila = y * w
            for x in range(x0, x1):
                i = base + x * c
                r, g, b = px[i], px[i + 1], px[i + 2]
                if b > 50 and b - r > 25 and b - g > 25:
                    cb[fila + x] += 1
                if r > 50 and r - g > 25 and r - b > 25:
                    cr[fila + x] += 1

    # Rejilla de 64x64: un parpadeo real (una corona) concentra cientos de
    # píxeles en una o dos celdas; el ruido de fondo (banderas, agua, HUD) se
    # reparte. El máximo por celda es un detector de "foco compacto" barato.
    CELDA = 64
    cw, ch = (w + CELDA - 1) // CELDA, (h + CELDA - 1) // CELDA

    def cuenta(c):
        parcial = tot = 0
        sx = sy = 0
        celdas = [0] * (cw * ch)
        for idx, v in enumerate(c):
            if v == 0:
                continue
            if v == N:
                tot += 1
            else:
                parcial += 1
                x, y = idx % w, idx // w
                sx += x; sy += y
                celdas[(y // CELDA) * cw + (x // CELDA)] += 1
        centro = (round(sx / parcial), round(sy / parcial)) if parcial else None
        mejor = max(celdas) if celdas else 0
        i = celdas.index(mejor) if mejor else 0
        foco = (int((i % cw) * CELDA + CELDA / 2), int((i // cw) * CELDA + CELDA / 2)) if mejor else None
        return parcial, tot, centro, mejor, foco

    pb, tb, centrob, foco_b, celda_b = cuenta(cb)
    pr, tr, centror, foco_r, celda_r = cuenta(cr)
    return dict(n=N, azul_parcial=pb, azul_total=tb, azul_centro=centrob,
                azul_max_celda=foco_b, azul_celda=celda_b,
                rojo_parcial=pr, rojo_total=tr, rojo_centro=centror,
                rojo_max_celda=foco_r, rojo_celda=celda_r)


UMBRAL_ALT = 4.0   # rango mínimo (canal) para considerar que una celda "alterna"
# Techo del rango: una corona de sirena suma como mucho ~42 por canal (el color
# va dividido por 6 en el propio motor, `255/6`), así que una celda cuyo canal
# oscila mucho más que eso NO es una luz aditiva: es la escena cambiando entre
# fotogramas (streaming de texturas, cámara asentándose, un coche cruzando).
UMBRAL_ALT_MAX = 55.0
# Además del rango y la anti-correlación se pide **que oscile de verdad**: con 4
# muestras, un cambio a mitad de serie (streaming de texturas, cámara
# asentándose) da corr=-1 con un único escalón. Una luz que parpadea sube Y baja
# dentro de la serie, así que el signo de las diferencias cambia al menos una vez
# en los dos canales.
def _oscila(serie):
    signos = [(serie[i + 1] > serie[i]) - (serie[i + 1] < serie[i])
              for i in range(len(serie) - 1)]
    signos = [s for s in signos if s]
    return any(a != b for a, b in zip(signos, signos[1:]))


def parpadeo(paths, region=None, celda=32, top=8):
    """Busca el parpadeo por **rango temporal del canal**, sin umbrales de color.

    Nació del bloque D3: la sirena del VC suma ~42 al canal que le toca (los
    colores van divididos por 6 en el propio motor) sobre un fondo que ya es
    cálido, así que "píxel con azul dominante" puede no aparecer nunca aunque la
    luz se esté dibujando (y eso hacía fallar la sonda sin motivo). Lo que no
    depende del fondo es *cuánto cambia* un píxel entre fotogramas de la misma
    cámara: un foco que parpadea deja una celda con rango alto en su canal y
    casi nada en los otros, durante toda la serie.

    Devuelve, por celda, el rango (max-min) de la media de cada canal a lo largo
    de la serie, ordenado por rango azul. `n` es el nº de capturas.
    """
    imgs = [read_png(p) for p in paths]
    w, h, c, _ = imgs[0]
    if any(im[0] != w or im[1] != h for im in imgs):
        raise SystemExit("ERROR: la serie mezcla tamaños distintos")
    x0, y0, x1, y1 = region if region else (0, 0, w, h)
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(w, x1), min(h, y1)
    cw, ch = (w + celda - 1) // celda, (h + celda - 1) // celda
    medios = {ch_: [[0.0, 0.0, 0.0] for _ in range(cw * ch)]
              for ch_ in range(len(imgs))}
    for k, (_w, _h, cc, px) in enumerate(imgs):
        for cy in range(y0 // celda, (y1 - 1) // celda + 1):
            for cx in range(x0 // celda, (x1 - 1) // celda + 1):
                bx0, bx1 = max(x0, cx * celda), min(x1, (cx + 1) * celda)
                by0, by1 = max(y0, cy * celda), min(y1, (cy + 1) * celda)
                sr = sg = sb = 0
                n = 0
                for y in range(by0, by1):
                    base = y * w * cc
                    for x in range(bx0, bx1):
                        i = base + x * cc
                        sr += px[i]; sg += px[i + 1]; sb += px[i + 2]
                        n += 1
                if n:
                    medios[k][cy * cw + cx] = [sr / n, sg / n, sb / n]
    def corr(xs, ys):
        """Correlación de Pearson (0 si alguna serie es plana)."""
        n = len(xs)
        mx, my = sum(xs) / n, sum(ys) / n
        sx = sum((x - mx) ** 2 for x in xs) ** 0.5
        sy = sum((y - my) ** 2 for y in ys) ** 0.5
        if sx < 1e-6 or sy < 1e-6:
            return 0.0
        return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / (sx * sy)

    filas = []
    for idx in range(cw * ch):
        vals = [medios[k][idx] for k in range(len(imgs))]
        rb = max(v[2] for v in vals) - min(v[2] for v in vals)
        rr = max(v[0] for v in vals) - min(v[0] for v in vals)
        rg = max(v[1] for v in vals) - min(v[1] for v in vals)
        serie_b = [v[2] for v in vals]
        serie_r = [v[0] for v in vals]
        # `corr` negativo = el rojo sube cuando el azul baja: exactamente lo que
        # hace una sirena (y lo que NO hace una nube, el agua ni el tráfico, que
        # mueven los tres canales a la vez). Es el discriminador que la métrica
        # de "píxel azul dominante" no podía ver: sobre un fondo cálido, sumar
        # 42 de azul no cambia el dominante, pero sí la alternancia.
        filas.append((rb, rr, rg, idx % cw, idx // cw, vals,
                      corr(serie_b, serie_r), serie_b, serie_r))
    filas.sort(reverse=True)
    # Celdas que alternan: rojo y azul con rango apreciable y anti-correlacionados.
    alternan = [f for f in filas
                if UMBRAL_ALT <= f[0] <= UMBRAL_ALT_MAX
                and UMBRAL_ALT <= f[1] <= UMBRAL_ALT_MAX and f[6] <= -0.8
                and _oscila(f[8]) and _oscila(f[7])]
    return dict(n=len(imgs), w=w, h=h, celda=celda, filas=filas[:top],
                compacto=sum(1 for f in filas if f[0] >= 12),
                max_b=filas[0][0] if filas else 0.0,
                max_r=max(f[1] for f in filas) if filas else 0.0,
                alternan=len(alternan), mejores=alternan[:top])


def diff(a, b, umbral, region=None):
    """Cuenta píxeles distintos entre dos capturas (todo, o sólo `region`).

    `region` (x0,y0,x1,y1) es imprescindible cuando lo que se compara es un
detalle pequeño sobre una escena viva: un cambio de mira son unos cientos de
    píxeles en el centro, y medir la imagen entera lo diluye en el tráfico, el
    agua y las nubes que se mueven solos.
    """
    wa, ha, ca, pa = read_png(a)
    wb, hb, cb, pb = read_png(b)
    if (wa, ha, ca) != (wb, hb, cb):
        raise SystemExit("ERROR: %s y %s no tienen el mismo formato" % (a, b))
    x0, y0, x1, y1 = 0, 0, wa, ha
    if region:
        x0, y0, x1, y1 = region
        x0, y0 = max(0, x0), max(0, y0)
        x1, y1 = min(wa, x1), min(ha, y1)
        if x1 <= x0 or y1 <= y0:
            raise SystemExit("ERROR: --region fuera de la imagen")
    distintos = 0
    for y in range(y0, y1):
        base = y * wa * ca
        for x in range(x0, x1):
            i = base + x * ca
            if (abs(pa[i] - pb[i]) + abs(pa[i + 1] - pb[i + 1]) +
                    abs(pa[i + 2] - pb[i + 2])) > umbral * 3:
                distintos += 1
    n = (x1 - x0) * (y1 - y0)
    return distintos, n


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("shots", nargs="+")
    ap.add_argument("--diff", action="store_true",
                    help="compara los dos primeros ficheros en vez de medir")
    ap.add_argument("--umbral", type=int, default=24,
                    help="diferencia mínima por canal para contar un píxel (24)")
    ap.add_argument("--region", default=None, help="x0,y0,x1,y1")
    ap.add_argument("--serie", action="store_true",
                    help="trata los ficheros como una serie de la misma escena y "
                         "busca lo que parpadea (tinte en unos fotogramas y no en otros)")
    ap.add_argument("--parpadeo", action="store_true",
                    help="serie: rango temporal por celda (sin umbrales de color); "
                         "saca las celdas que más cambian y en qué canal")
    ap.add_argument("--celda", type=int, default=32, help="lado de la celda (32)")
    ap.add_argument("--top", type=int, default=8, help="celdas a listar (8)")
    a = ap.parse_args()

    region = None
    if a.region:
        vals = [int(v) for v in a.region.split(",")]
        if len(vals) != 4:
            raise SystemExit("ERROR: --region espera x0,y0,x1,y1")
        region = tuple(vals)

    if a.parpadeo:
        if len(a.shots) < 3:
            raise SystemExit("ERROR: --parpadeo necesita al menos 3 capturas")
        p = parpadeo(a.shots, region, a.celda, a.top)
        print("parpadeo de %d capturas (celda %d): celdas con rango azul>=12: %d | "
              "max rango azul=%.1f rojo=%.1f"
              % (p["n"], p["celda"], p["compacto"], p["max_b"], p["max_r"]))
        for rb, rr, rg, cx, cy, vals, co, sb, sr in p["filas"]:
            print("  celda (%4d,%4d) azul=%s rojo=%s dAzul=%5.1f dRojo=%5.1f corr=%+.2f"
                  % (cx * p["celda"], cy * p["celda"],
                     ",".join("%.0f" % v for v in sb),
                     ",".join("%.0f" % v for v in sr), rb, rr, co))
        print("alternan (rango de azul y rojo en [%g,%g], corr<=-0.8 y los dos "
              "canales oscilando): %d" % (UMBRAL_ALT, UMBRAL_ALT_MAX, p["alternan"]))
        for rb, rr, rg, cx, cy, vals, co, sb, sr in p["mejores"]:
            print("  ALTERNA celda (%4d,%4d) azul=%s rojo=%s corr=%+.2f"
                  % (cx * p["celda"], cy * p["celda"],
                     ",".join("%.0f" % v for v in sb),
                     ",".join("%.0f" % v for v in sr), co))
        return 0

    if a.serie:
        if len(a.shots) < 3:
            raise SystemExit("ERROR: --serie necesita al menos 3 capturas")
        # `--region` también aquí: en una serie es lo que deja fuera el HUD (la
        # esquina de la radio parpadea sola y suele ser el mayor foco falso).
        s = serie(a.shots, region)
        print("serie de %d: azul_parcial=%d (max/celda=%d en %s) azul_total=%d | "
              "rojo_parcial=%d (max/celda=%d en %s) rojo_total=%d | azul_centro=%s"
              % (s["n"], s["azul_parcial"], s["azul_max_celda"], s["azul_celda"],
                 s["azul_total"], s["rojo_parcial"], s["rojo_max_celda"],
                 s["rojo_celda"], s["rojo_total"], s["azul_centro"]))
        return 0

    if a.diff:
        if len(a.shots) != 2:
            raise SystemExit("ERROR: --diff necesita exactamente dos capturas")
        d, n = diff(a.shots[0], a.shots[1], a.umbral, region)
        print("%s vs %s%s: %d/%d píxeles distintos (%.1f%%) con umbral %d"
              % (a.shots[0], a.shots[1],
                 "" if not region else " [%d,%d,%d,%d]" % region,
                 d, n, 100.0 * d / n, a.umbral))
        return 0

    for p in a.shots:
        s = stats(p, region)
        print("%-28s %4dx%-4d rojo=%-6d azul=%-6d rojoF=%-5d azulF=%-5d brillo=%.1f%s"
              % (s["path"].split("/")[-1].split("\\")[-1], s["w"], s["h"],
                 s["rojo"], s["azul"], s["rojo_fuerte"], s["azul_fuerte"],
                 s["brillo"],
                 ("  foco=%s" % (s["foco"],)) if s["foco"] else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
