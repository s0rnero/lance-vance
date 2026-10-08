#!/usr/bin/env python3
"""Fusiona texturas de un TXD dentro de otro (formato RenderWare, VC/PC).

Sirve para el caso de Vice Extended: su `fronten2.txd` trae los mapas y botones
del menú pero le faltan los 11 logos de emisora, que el mod movió a su propio
`radio.txd`. Con esto se construye UN `fronten2.txd` con todo y el port no
necesita cargar `radio.txd` (que no forma parte de su set).

Estructura de un TXD de VC (la que lee el motor):
    chunk 0x16 (TextureDictionary)
        chunk 0x01 (STRUCT, 4 bytes) -> u32 nº de texturas
        N x chunk 0x15 (TextureNative)
              chunk 0x01 (STRUCT) -> platform, filterAddressing, name[32], ...
        (puede acabar con un chunk de extensión de tamaño 0: se conserva al final)

Uso:
    python tools/txd_merge.py base.txd extra.txd salida.txd
    python tools/txd_merge.py base.txd extra.txd salida.txd --only a,b,c
    python tools/txd_merge.py base.txd extra.txd salida.txd --check

`--check` solo informa (no escribe). El fichero de salida se revalida antes de
darlo por bueno: contador del STRUCT y nombres resultantes.
"""
import argparse
import struct

DICT, STRUCT, TEXTURE, TEXNATIVE = 0x16, 0x01, 0x06, 0x15


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def texture_name(buf, pos, nxt):
    """Nombre de la textura del chunk que empieza en `pos` (o None si no lo es)."""
    t = u32(buf, pos)
    if t == TEXNATIVE:
        native = pos
    elif t == TEXTURE:                       # wrapper: busca su TextureNative
        native = None
        p = pos + 12
        while p + 12 <= nxt:
            ct, cs, _ = struct.unpack_from("<III", buf, p)
            if ct == TEXNATIVE:
                native = p
                break
            p += 12 + cs
        if native is None:
            return None
    else:
        return None
    body = native + 12
    if body + 12 + 40 > len(buf) or u32(buf, body) != STRUCT:
        return None
    d = body + 12                          # platform, filterAddressing, name[32]
    return buf[d + 8:d + 40].split(b"\0")[0].decode("latin1", "replace")


def read_txd(path):
    """-> (version, struct_chunk_bytes, [(pos,nxt,name)], cola, data)

    `cola` son los bytes dentro del diccionario que NO son texturas (chunk de
    extensión vacío, padding) y que hay que devolver al final; `data` es el
    fichero entero en memoria.
    """
    data = open(path, "rb").read()
    if len(data) < 24 or u32(data, 0) != DICT:
        raise SystemExit("ERROR: %s no es un TextureDictionary (tipo 0x%X)" % (path, u32(data, 0)))
    ver = u32(data, 8)
    end = 12 + u32(data, 4)
    if end > len(data):
        raise SystemExit("ERROR: %s truncado (dice %d bytes, tiene %d)" % (path, end, len(data)))
    t, s, _ = struct.unpack_from("<III", data, 12)
    if t != STRUCT:
        raise SystemExit("ERROR: %s no empieza por STRUCT" % path)
    if s < 4:
        raise SystemExit("ERROR: %s: STRUCT sin contador" % path)
    st_pos, st_nxt = 12, 24 + s          # cabecera (12 B) + cuerpo del STRUCT
    st_chunk = data[st_pos:st_nxt]

    texs, ultimo = [], st_nxt
    pos = st_nxt
    while pos + 12 <= end:
        ct, cs, _ = struct.unpack_from("<III", data, pos)
        nxt = pos + 12 + cs
        if cs < 0 or nxt > end:
            break
        name = texture_name(data, pos, nxt)
        if name is not None:
            texs.append((pos, nxt, name))
            ultimo = nxt
        pos = nxt
    return ver, st_chunk, texs, data[ultimo:end], data


def merge(base_path, extra_path, out_path, only=None, check=False):
    bver, bstruct, btexs, bcola, bdata = read_txd(base_path)
    ever, _, etexs, _, edata = read_txd(extra_path)

    base_names = set(t[2] for t in btexs)
    add = [t for t in etexs
           if t[2] not in base_names and (only is None or t[2] in only)]

    print("base : %-30s %d texturas" % (base_path.split("/")[-1], len(btexs)))
    print("extra: %-30s %d texturas" % (extra_path.split("/")[-1], len(etexs)))
    print("añadir %d: %s" % (len(add), ", ".join(t[2] for t in add) or "-"))

    total = len(btexs) + len(add)
    if check:
        print("[check] no escribo nada. total resultante: %d texturas" % total)
        return total

    struct_chunk = bytearray(bstruct)
    struct.pack_into("<I", struct_chunk, 12, total)       # contador

    body = bytes(struct_chunk)
    body += b"".join(bdata[p:n] for p, n, _ in btexs)     # texturas de la base
    body += b"".join(edata[p:n] for p, n, _ in add)       # texturas añadidas
    body += bcola                                          # extensión/padding

    out = struct.pack("<III", DICT, len(body), bver) + body
    with open(out_path, "wb") as f:
        f.write(out)

    # validación: releer y comprobar contador y nombres
    _, _, chk_texs, _, _ = read_txd(out_path)
    got = set(t[2] for t in chk_texs)
    if len(chk_texs) != total:
        raise SystemExit("ERROR: el resultado tiene %d texturas, esperaba %d"
                         % (len(chk_texs), total))
    perdidos = sorted(base_names - got)
    if perdidos:
        raise SystemExit("ERROR: el resultado perdió nombres: %s" % perdidos)
    print("escrito %s: %d texturas, %.1f MB"
          % (out_path, len(chk_texs), len(out) / 1048576))
    print("nombres nuevos presentes: %d/%d" % (len(add), len(add)))
    return total


def main():
    ap = argparse.ArgumentParser(description="Fusiona texturas de un TXD en otro.")
    ap.add_argument("base")
    ap.add_argument("extra")
    ap.add_argument("salida")
    ap.add_argument("--only", help="lista separada por comas de nombres a añadir")
    ap.add_argument("--check", action="store_true", help="no escribir, solo informar")
    a = ap.parse_args()
    only = set(x.strip() for x in a.only.split(",")) if a.only else None
    merge(a.base, a.extra, a.salida, only, a.check)


if __name__ == "__main__":
    main()
