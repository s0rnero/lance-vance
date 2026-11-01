#!/usr/bin/env python3
"""Alta de claves de menu en un GXT con varios pares TKEY/TDAT.

`gxt_inspect.py` trabaja con el ultimo par; este fichero recorre la cadena
completa y edita **solo** el par que ya contiene `FED_RGL` (el de menu),
dejando el resto de pares y la cola byte a byte iguales.

Uso:
  python tools/gxt_menu_keys.py check <fichero.gxt> CLAVE [CLAVE ...]
  python tools/gxt_menu_keys.py check-src <fichero.gxt> <dir-src>
  python tools/gxt_menu_keys.py add <fichero.gxt> <claves.tsv> [--dry]
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gxt_inspect as gx

ANCHOR = "FED_RGL"
MAGICS = (b"TABL", b"TKEY", b"TDAT")
_USED = re.compile(r'"(FE[A-Z]_[A-Z0-9_]{1,7})"')


def read_all(path):
    b = open(path, "rb").read()
    chunks = []
    pos = 0
    while pos + 8 <= len(b) and b[pos:pos + 4] in MAGICS:
        magic = b[pos:pos + 4]
        size = struct.unpack_from("<I", b, pos + 4)[0]
        if pos + 8 + size > len(b):
            break
        chunks.append((magic, b[pos + 8:pos + 8 + size]))
        pos += 8 + size
    if not chunks or chunks[0][0] != b"TABL":
        raise SystemExit("ERROR: %s no empieza por TABL" % path)
    return chunks, b[pos:]


def pairs_of(chunks):
    out = []
    i = 0
    while i + 1 < len(chunks):
        if chunks[i][0] == b"TKEY" and chunks[i + 1][0] == b"TDAT":
            out.append((i, i + 1))
            i += 2
        else:
            i += 1
    return out


def _value(tdat, off):
    end = off
    while end + 2 <= len(tdat) and tdat[end:end + 2] != b"\0\0":
        end += 2
    return tdat[off:end]


def rows_of(tkey, tdat):
    if len(tkey) % gx.KEY_ENTRY:
        raise SystemExit("ERROR: TKEY no es multiplo de %d" % gx.KEY_ENTRY)
    out = []
    for i in range(len(tkey) // gx.KEY_ENTRY):
        off = struct.unpack_from("<I", tkey, i * gx.KEY_ENTRY)[0]
        raw = tkey[i * gx.KEY_ENTRY + 4:i * gx.KEY_ENTRY + 4 + gx.KEY_LEN]
        key = raw.split(b"\0")[0].decode("latin1")
        if off >= len(tdat):
            raise SystemExit("ERROR: offset %d fuera de TDAT (%d)" % (off, len(tdat)))
        out.append((key, off, _value(tdat, off)))
    return out


def keys_of(tkey, tdat):
    out = {}
    for key, _off, raw in rows_of(tkey, tdat):
        out.setdefault(key, raw.decode("utf-16le", "replace"))
    return out


def all_keys(chunks):
    out = {}
    for a, b in pairs_of(chunks):
        for key, value in keys_of(chunks[a][1], chunks[b][1]).items():
            out.setdefault(key, value)
    return out


def pair_with_anchor(chunks):
    pares = pairs_of(chunks)
    if not pares:
        raise SystemExit("ERROR: el GXT no tiene pares TKEY/TDAT")
    for a, b in pares:
        if ANCHOR in keys_of(chunks[a][1], chunks[b][1]):
            return a, b
    return pares[0]


def rebuild(changed):
    out = bytearray()
    for magic, body in changed:
        out += magic + struct.pack("<I", len(body)) + body
    return bytes(out)


def write_pair(path, novos, dry=False):
    chunks, tail = read_all(path)
    a, b = pair_with_anchor(chunks)
    rows = rows_of(chunks[a][1], chunks[b][1])
    existentes = all_keys(chunks)
    omitidas = [k for k in novos if k in existentes]
    for k in omitidas:
        del novos[k]
    for k in novos:
        if len(k) > gx.MAXKEY:
            raise SystemExit("ERROR: clave '%s' > %d caracteres" % (k, gx.MAXKEY))
        if k != k.upper():
            raise SystemExit("ERROR: clave '%s' no esta en mayusculas" % k)
    if not novos:
        print("%s: nada que anadir (%d ya existian)" % (path, len(omitidas)))
        return 0

    tdat = bytearray(chunks[b][1])
    filas = list(rows)
    for k in sorted(novos):
        off = len(tdat)
        valor = novos[k].encode("utf-16le") + b"\0\0"
        tdat += valor
        filas.append((k, off, valor[:-2]))
    filas.sort(key=lambda fila: fila[0].encode("latin1"))

    tkey = bytearray()
    for k, off, _raw in filas:
        tkey += struct.pack("<I", off) + k.encode("latin1").ljust(gx.KEY_LEN, b"\0")

    nuevos_chunks = list(chunks)
    nuevos_chunks[a] = (b"TKEY", bytes(tkey))
    nuevos_chunks[b] = (b"TDAT", bytes(tdat))
    blob = rebuild(nuevos_chunks) + tail
    if blob[:4] == b"TABL":
        blob, movidos, problemas = gx.retarget_tabl(blob)
        if problemas:
            raise SystemExit("ERROR: %s: TABL incoherente: %s" % (path, "; ".join(problemas)))
        if movidos:
            print("  TABL: %d misiones reajustadas" % len(movidos))

    if dry:
        print("%s: (dry) anadiria %d claves al par %d/%d -> %d bytes" % (
            path, len(novos), a, b, len(blob)))
        return len(novos)

    tmp = path + ".tmp"
    with open(tmp, "wb") as f:
        f.write(blob)
    try:
        otro_chunks, otro_tail = read_all(tmp)
        otro_pares = pairs_of(otro_chunks)
        if len(otro_pares) != len(pairs_of(chunks)):
            raise SystemExit("ERROR: %s: cambia el numero de pares" % path)
        for (a0, b0), (a1, b1) in zip(pairs_of(chunks), otro_pares):
            if (a0, b0) == (a, b):
                continue
            antes = rows_of(chunks[a0][1], chunks[b0][1])
            ahora = rows_of(otro_chunks[a1][1], otro_chunks[b1][1])
            if [(k, v) for k, _o, v in antes] != [(k, v) for k, _o, v in ahora]:
                raise SystemExit("ERROR: %s: cambia un par que no se tocaba" % path)
        ahora = keys_of(otro_chunks[a][1], otro_chunks[b][1])
        for k, _o, v in rows:
            if ahora.get(k) != v.decode("utf-16le", "replace"):
                raise SystemExit("ERROR: %s: se perdio '%s'" % (path, k))
        for k, v in novos.items():
            if ahora.get(k) != v:
                raise SystemExit("ERROR: %s: no se lee '%s'" % (path, k))
        if otro_tail != tail:
            raise SystemExit("ERROR: %s: cambia la cola" % path)
    except SystemExit:
        os.unlink(tmp)
        raise
    os.replace(tmp, path)
    print("%s: %d claves al par %d/%d [verificado]" % (path, len(novos), a, b))
    return len(novos)


def tsv_pairs(path):
    out = {}
    for n, line in enumerate(open(path, encoding="utf-8"), 1):
        line = line.rstrip("\r\n")
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) != 2 or not parts[0].strip():
            raise SystemExit("ERROR: %s:%d no es CLAVE<TAB>texto" % (path, n))
        out[parts[0].strip().upper()] = parts[1]
    return out


def used_in_src(directory):
    out = {}
    for root, _dirs, files in os.walk(directory):
        for f in files:
            if not f.lower().endswith((".cpp", ".h")):
                continue
            p = os.path.join(root, f)
            try:
                texto = open(p, encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            for k in _USED.findall(texto):
                out.setdefault(k, set()).add(f)
    return out


def main():
    argv = sys.argv[1:]
    if len(argv) < 3:
        raise SystemExit(__doc__)
    cmd, path = argv[0], argv[1]
    if cmd == "check":
        tienen = all_keys(read_all(path)[0])
        faltan = [k.upper() for k in argv[2:] if k.upper() not in tienen]
        if faltan:
            print("FALTAN en %s: %s" % (path, " ".join(faltan)))
            return 1
        print("%s: todas las claves pedidas presentes" % path)
        return 0
    if cmd == "check-src":
        usadas = used_in_src(argv[2])
        tienen = all_keys(read_all(path)[0])
        faltan = sorted(k for k in usadas if k not in tienen)
        print("%s: %d claves usadas en src, %d ausentes" % (path, len(usadas), len(faltan)))
        for k in faltan:
            print("  %-8s %s" % (k, ",".join(sorted(usadas[k]))))
        return 1 if faltan else 0
    if cmd == "add":
        if len(argv) < 3:
            raise SystemExit(__doc__)
        write_pair(path, tsv_pairs(argv[2]), dry="--dry" in argv)
        return 0
    raise SystemExit(__doc__)


if __name__ == "__main__":
    sys.exit(main())
