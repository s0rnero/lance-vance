#!/usr/bin/env python3
"""Lista las texturas de un TXD de RenderWare (VC/PC) con su formato de raster.

Nació del bloque D3 (la sirena policial que "desapareció" en el port): cuando una
textura no se ve, la pregunta siguiente es *con qué formato* entró al TXD. Si el
importador la volvió a empaquetar en un formato que el backend no sabe leer (DXT
sin soporte, alfa premultiplicado, 16 bits sin alfa…) la textura puede quedar
negra, invisible o directamente nula (`RwTextureRead` devuelve nil y las coronas
TYPE_STAR dejan de dibujarse, sin ningún error en pantalla).

Uso:
    python tools/txd_inspect.py <fichero.txd>            # todas las texturas
    python tools/txd_inspect.py <fichero.txd> --only a,b # filtra por nombre
    python tools/txd_inspect.py <a.txd> <b.txd> ...      # compara varios ficheros

Salida: una línea por textura con nombre, plataforma, tamaño, profundidad, los
flags de raster y el formato D3D, más un resumen de formatos contados.
"""
import argparse
import struct
import sys

DICT, STRUCT, TEXTURE, TEXNATIVE = 0x16, 0x01, 0x06, 0x15

# Flags de rwRASTERFORMAT (los que importan para diagnosticar alfa y compresión)
FLAGS = {
    0x0100: "1555", 0x0200: "565", 0x0300: "4444", 0x0400: "LUM8",
    0x0500: "8888", 0x0600: "888", 0x0700: "16", 0x0800: "24", 0x0900: "32",
    0x1000: "AUTO_MIPMAP", 0x2000: "PAL8", 0x4000: "PAL4", 0x8000: "MIPMAP",
}
FORMAT_MASK = 0x0F00

# Formatos D3D de VC (D3DFMT). Los 0x... altos son las variantes de FourCC
# (DXT1-5) que GetD3DFormatFromRasterFormat produce.
D3DFMT = {
    0x15: "A8R8G8B8", 0x16: "X8R8G8B8", 0x19: "R5G6B5", 0x1A: "X1R5G5B5",
    0x1B: "A1R5G5B5", 0x1C: "A4R4G4B4", 0x32: "L8", 0x50: "DXT1",
    0x51: "DXT2", 0x52: "DXT3", 0x53: "DXT4", 0x54: "DXT5",
}
COMPRESSION = {0: "-", 1: "DXT1", 2: "DXT3", 3: "DXT5"}


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def texturas(data):
    """Recorre el diccionario y devuelve los offsets de cada TextureNative."""
    if len(data) < 24 or u32(data, 0) != DICT:
        raise SystemExit("ERROR: no es un TextureDictionary (tipo 0x%X)" % u32(data, 0))
    out = []
    p = 12
    end = min(12 + u32(data, 4), len(data))
    if u32(data, p) != STRUCT:
        raise SystemExit("ERROR: el diccionario no empieza por STRUCT")
    p += 12 + u32(data, p + 4)
    while p + 12 <= end:
        t, size = u32(data, p), u32(data, p + 4)
        if t == TEXNATIVE:
            out.append(p)
            p += 12 + size
        else:
            p += 12 + size
    return out


def describe(data, pos):
    body = pos + 12
    if u32(data, body) != STRUCT:
        return None
    d = body + 12
    plataforma = u32(data, d)
    nombre = data[d + 8:d + 40].split(b"\0")[0].decode("latin1", "replace")
    mascara = data[d + 40:d + 72].split(b"\0")[0].decode("latin1", "replace")
    raster, d3d = u32(data, d + 72), u32(data, d + 76)
    ancho, alto = struct.unpack_from("<HH", data, d + 84)
    prof, niveles, tipo, comp = data[d + 88], data[d + 89], data[d + 90], data[d + 91]
    return dict(nombre=nombre, plataforma=plataforma,
                ancho=ancho, alto=alto, prof=prof, niveles=niveles,
                raster=raster, formato=FLAGS.get(raster & FORMAT_MASK, "0x%X" % (raster & FORMAT_MASK)),
                mipmap=bool(raster & 0x8000), auto_mipmap=bool(raster & 0x1000),
                alfa=bool(comp == 0 and prof == 32 and (raster & 0xF00) in (0x100, 0x300, 0x500, 0x900)),
                compresion=COMPRESSION.get(comp, str(comp)),
                d3d=D3DFMT.get(d3d, "0x%X" % d3d), d3d_raw=d3d,
                mip_niveles=niveles, tipo=tipo, mascara=mascara)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("txds", nargs="+")
    ap.add_argument("--only", default=None, help="nombres separados por comas")
    ap.add_argument("--resumen", action="store_true",
                    help="sólo el recuento por formato")
    a = ap.parse_args()
    filtro = set(n.strip().lower() for n in a.only.split(",")) if a.only else None

    for ruta in a.txds:
        data = open(ruta, "rb").read()
        posiciones = texturas(data)
        if not a.resumen:
            print("== %s (%d texturas, %d bytes)" % (ruta, len(posiciones), len(data)))
        cuenta = {}
        faltan = set(filtro) if filtro else None
        for pos in posiciones:
            t = describe(data, pos)
            if t is None:
                print("   !! chunk TextureNative ilegible en 0x%X" % pos)
                continue
            clave = (t["formato"], t["compresion"], t["prof"])
            cuenta[clave] = cuenta.get(clave, 0) + 1
            if filtro is not None:
                if t["nombre"].lower() not in filtro:
                    continue
                faltan.discard(t["nombre"].lower())
            if not a.resumen:
                print("   %-24s %4dx%-4d prof=%d %-5s comp=%-4s d3d=%-10s mip=%-4s niveles=%d%s"
                      % (t["nombre"], t["ancho"], t["alto"], t["prof"],
                         t["formato"], t["compresion"], t["d3d"],
                         "sí" if t["mipmap"] else ("auto" if t["auto_mipmap"] else "no"),
                         t["mip_niveles"], "  [32-bit]" if t["alfa"] else ""))
        if cuenta:
            resumen = ", ".join("%s/%s/%db=%d" % (f, c, p, n)
                                for (f, c, p), n in sorted(cuenta.items(), key=lambda kv: -kv[1]))
            print("   formatos: %s" % resumen)
        if faltan:
            print("   !! no están: %s" % ", ".join(sorted(faltan)))
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
