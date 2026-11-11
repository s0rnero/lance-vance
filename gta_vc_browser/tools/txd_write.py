#!/usr/bin/env python3
"""Genera un TXD de iconos de botones a partir de PNG RGBA clonando las
cabeceras nativas de un TXD plantilla (x360btns.txd).

Uso:
    python tools/txd_write.py <salida.txd> <plantilla.txd> <nombre=png.png ...>

Cada textura resultante es 128x128 RGBA8888 (orden B,G,R,A como lectora
txd_raster.decode depth==32). La cabecera nativa (plataforma, formato,
hasAlpha, profundidad, niveles, compresion, mascara) se clona de la primera
textura de la plantilla; solo cambian nombre, dimensiones, nLevels y datos.
Los nombres deben coincidir con los que busca CFont::LoadButtons.
"""
import struct
import sys

SIZE = 64

ID_STRUCT = 0x01
ID_TEXTURE_NATIVE = 0x15
ID_DICT = 0x16
ID_EXTENSION = 0x03


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def parse_template(data):
    """Devuelve (version_tointment_dict, dict_struct_bytes, nat_hdr_bytes,
    nat_version, struct_version_15, tex_version, ext_bytes)."""
    if u32(data, 0) != ID_DICT:
        raise SystemExit("ERROR: la plantilla no es un TextureDictionary")
    dict_ver = u32(data, 8)
    p = 12
    if u32(data, p) != ID_STRUCT:
        raise SystemExit("ERROR: el diccionario no empieza por STRUCT")
    dict_struct = data[p:p + 12 + u32(data, p + 4)]
    p += 12 + u32(data, p + 4)
    chunks = []
    end = min(12 + u32(data, 4), len(data))
    while p + 12 <= end:
        t, size = u32(data, p), u32(data, p + 4)
        chunk = data[p:p + 12 + size]
        if t == ID_TEXTURE_NATIVE:
            chunks.append(chunk)
        p += 12 + size
    if not chunks:
        raise SystemExit("ERROR: la plantilla no tiene texturas nativas")
    # Extensión final (0x03) si existe: en x360btns.txd viene como chunk de
    # tamaño 0 al final del bucle anterior.
    ext = b""
    if p + 12 <= len(data) and u32(data, p) == ID_EXTENSION:
        ext = data[p:p + 12 + u32(data, p + 4)]
    first = chunks[0]
    # Cabecera nativa de 88 bytes: empieza en pos+24 (ver txd_raster.read_native).
    nat = bytearray(first[24:24 + 88])
    nat_ver = u32(first, 20)   # version del chunk STRUCT de la textura
    tex_ver = u32(first, 8)    # version del chunk 0x15
    return dict_ver, dict_struct, nat, nat_ver, tex_ver, ext


def build_texture(name, png_rgba, nat_hdr, nat_ver, tex_ver):
    """Chunk 0x15 con la cabecera clonada y el PNG (RGBA) como nivel 0."""
    import io
    from PIL import Image
    img = Image.open(io.BytesIO(png_rgba))
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    # 64x64 como los TXD de iconos originales: en el mensaje se pintan a
    # ~17x19 unidades y un icono de 192 px se convierte en una mancha.
    img = img.resize((SIZE, SIZE), Image.LANCZOS)
    w, h = SIZE, SIZE
    # Los PNG del HUD son suaves (alfa maximo ~170): sin normalizar, sobre el
    # recuadro negro los iconos salian como cuadros grises translucidos.
    r, g, b, a = img.split()
    top = max(a.getdata()) or 255
    lut = [0 if (v * 255.0 / top) < 26 else min(255, int(255 * (v / top) ** 0.55)) for v in range(256)]
    a = a.point(lut)
    img = Image.merge("RGBA", (r.point(lambda v: 255), g.point(lambda v: 255), b.point(lambda v: 255), a))
    px = img.tobytes()  # RGBA por pixel
    bgra = bytearray(len(px))
    for i in range(w * h):
        r, g, b, a = px[4 * i:4 * i + 4]
        base = 4 * i
        bgra[base] = b
        bgra[base + 1] = g
        bgra[base + 2] = r
        bgra[base + 3] = a
    data = bytes(bgra)
    if len(data) != w * h * 4:
        raise SystemExit("ERROR: tamano de datos inesperado para %s" % name)
    struct_size = 88 + 4 + len(data)
    chunk_size = 12 + struct_size  # tamaño = cuerpo (sin la cabecera de 12)
    out = bytearray()
    out += struct.pack("<III", ID_TEXTURE_NATIVE, chunk_size, tex_ver)
    out += struct.pack("<III", ID_STRUCT, struct_size, nat_ver)
    hdr = bytearray(nat_hdr)
    nb = name.encode("latin1")[:31]
    hdr[8:8 + 32] = nb + b"\0" * (32 - len(nb))
    struct.pack_into("<HH", hdr, 80, w, h)
    hdr[84] = 32      # depth (clonado de la plantilla; por seguridad)
    hdr[85] = 1       # nLevels
    out += hdr
    out += struct.pack("<I", len(data))
    out += data
    return bytes(out)


def main():
    if len(sys.argv) < 5:
        raise SystemExit(__doc__)
    salida, plantilla = sys.argv[1], sys.argv[2]
    entries = sys.argv[3:]
    data = open(plantilla, "rb").read()
    dict_ver, dict_struct, nat, nat_ver, tex_ver, ext = parse_template(data)
    chunks = []
    for e in entries:
        if "=" not in e:
            raise SystemExit("ERROR: entrada sin '=': %s" % e)
        name, path = e.split("=", 1)
        chunks.append(build_texture(name, open(path, "rb").read(), nat, nat_ver, tex_ver))
    payload = dict_struct + b"".join(chunks) + ext
    out = struct.pack("<III", ID_DICT, len(payload), dict_ver) + payload
    open(salida, "wb").write(out)
    print("OK %s: %d texturas, %d bytes" % (salida, len(chunks), len(out)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
