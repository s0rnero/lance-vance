#!/usr/bin/env python3
"""GXT de Vice City: leer y añadir/actualizar claves (nombres de vehículos, etc.).

Formato VC (ver `CText::Load` / `CKeyArray::Load` / `CData::Load` en
src/text/Text.cpp): el fichero es una **cadena de chunks** `magic[4] + size[4]`:
  TABL <n>  tabla de offsets de texto de misión (se conserva tal cual)
  TKEY <n>  n/12 entradas de {offset(4) + clave(8)} — **sin contador**
  TDAT <n>  texto UTF-16LE, cadenas terminadas en 00 00
`CText::Load` lee en bucle hasta tener un TKEY y un TDAT, y **para**: los GXT
reales de VC llevan después una cola con más pares TKEY/TDAT (el mod trae 79)
que el motor no lee. Esa cola se preserva byte a byte.

El motor busca con `strcmp` + `BinarySearch`, así que las claves van
**ordenadas (byte a byte)**, **mayúsculas**, y como máximo 7 caracteres + NUL.
El `offset` de TKEY es **relativo al inicio de TDAT** y apunta al **primer byte
de la cadena**. Una clave que no exista imprime "<clave> missing" en el HUD
(`CData::Search`).

IMPORTANTE — el escritor es **incremental y no destructivo**:
  * el prefijo de TDAT se conserva **byte a byte**, así que los offsets de las
    misiones (TABL) y los de las claves que no tocamos siguen siendo válidos;
  * las cadenas nuevas se **añaden al final** de TDAT (las viejas quedan como
    bytes muertos, inofensivos);
  * sólo se reescribe TKEY (ordenado por clave, **estable**, así que las claves
    repetidas —las hay en los ficheros del mod— conservan su orden relativo);
  * **no se toca una clave que ya existe** salvo que se pida con `--force`, y
    la cola del fichero se conserva tal cual.
Y tras escribir, **relee el fichero y comprueba** con un multiconjunto que
ninguna clave ajena a la operación haya cambiado de bytes. Si algo no cuadra,
sale con código 1 y el fichero queda como estaba (se escribe a un temporal y se
mueve encima sólo al final).

Uso:
  python tools/gxt_inspect.py self-test
  python tools/gxt_inspect.py claves <fichero.gxt> [filtro]
  python tools/gxt_inspect.py lista  <fichero.gxt>
  python tools/gxt_inspect.py check  <fichero.gxt> CLAVE [CLAVE ...]
  python tools/gxt_inspect.py add    <fichero.gxt> <CLAVE=valor> [<CLAVE=valor> ...] [--force]
  python tools/gxt_inspect.py add-tsv <fichero.gxt> <claves.tsv> [--force]
      (TSV `modelo<TAB>CLAVE<TAB>texto`, lo escribe tools/import_mvl_vehicles.py)
  python tools/gxt_inspect.py tabl       <fichero.gxt>
  python tools/gxt_inspect.py check-tabl <fichero.gxt>
  python tools/gxt_inspect.py fix-tabl   <fichero.gxt>
      (TABL = offsets de los textos de mision. `add` ya los reajusta solo; estos
       tres existen para comprobarlo en los GXT que ya estan en el repo)
"""
import collections
import os
import struct
import sys

KEY_ENTRY = 12
KEY_LEN = 8
MAXKEY = KEY_LEN - 1

MAGICS = (b"TABL", b"TKEY", b"TDAT")


def self_test():
    """Comprobación de ida y vuelta con claves de todas las longitudes (1..7).

    Vale la pena: leer mal el ancho de la clave no da error, sólo claves
    truncadas (una clave de 7 caracteres se lee como una de 4 y el motor
    imprime "<clave> missing"). Aquí se verifica sin tocar ningún fichero.
    """
    import tempfile
    claves = [("A", "uno"), ("AB12", "dos"), ("ABCDEF", "tres"),
              ("ABCDEFG", "cuatro"), ("ZZZ", "cinco")]
    tdat = bytearray()
    filas = []
    for k, v in claves:
        filas.append((k, len(tdat), v.encode("utf-16le")))
        tdat += v.encode("utf-16le") + b"\0\0"
    tkey = bytearray()
    for k, off, _v in sorted(filas, key=lambda f: f[0].encode("latin1")):
        tkey += struct.pack("<I", off) + k.encode("latin1").ljust(KEY_LEN, b"\0")
    blob = (b"TABL" + struct.pack("<I", 0)
            + b"TKEY" + struct.pack("<I", len(tkey)) + tkey
            + b"TDAT" + struct.pack("<I", len(tdat)) + tdat)
    with tempfile.NamedTemporaryFile(suffix=".gxt", delete=False) as f:
        f.write(blob)
        path = f.name
    try:
        _, tail, entries = parse(path)
        visto = {k: text_of(v) for k, _, v in entries}
        assert tail == b"", "la cola debía estar vacía"
        for k, v in claves:
            if visto.get(k) != v:
                return "FALLO: %s -> %r (leído %r)" % (k, v, visto.get(k))
        return "OK (%d claves)" % len(entries)
    finally:
        os.unlink(path)


def read_chunks(path):
    """-> (chunks, cola). La cola es todo lo que sigue al último chunk conocido,
    sin interpretar y preservada byte a byte al reescribir."""
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


def _value_end(tdat, off):
    s = off
    while s + 2 <= len(tdat) and tdat[s:s + 2] != b"\0\0":
        s += 2
    return s


def parse(path):
    """-> (chunks, cola, [(clave, offset, bytes_del_valor), ...])."""
    chunks, tail = read_chunks(path)
    tkey = tdat = None
    for magic, body in chunks:
        if magic == b"TKEY":
            tkey = body
        elif magic == b"TDAT":
            tdat = body
    if tkey is None or tdat is None:
        raise SystemExit("ERROR: %s no tiene TKEY/TDAT" % path)
    if len(tkey) % KEY_ENTRY:
        raise SystemExit("ERROR: %s: TKEY no es múltiplo de %d" % (path, KEY_ENTRY))
    entries = []
    for i in range(len(tkey) // KEY_ENTRY):
        off, = struct.unpack_from("<I", tkey, i * KEY_ENTRY)
        raw = tkey[i * KEY_ENTRY + 4: i * KEY_ENTRY + 4 + KEY_LEN]
        key = raw.split(b"\0")[0].decode("latin1")
        if off >= len(tdat):
            raise SystemExit("ERROR: %s: offset %d fuera de TDAT (%d)"
                             % (path, off, len(tdat)))
        end = _value_end(tdat, off)
        entries.append((key, off, bytes(tdat[off:end])))
    return chunks, tail, entries


def text_of(raw):
    return raw.decode("utf-16le", "replace")


# ---------------------------------------------------------------------------
# TABL: offsets de los textos de misión (CText::LoadMissionText)
#
# El motor abre el GXT, salta al offset de TABL de la misión, lee 8 bytes y
# exige que sean el NOMBRE de la tabla; después espera `TKEY` + `TDAT`. Si el
# offset está mal, no encuentra los textos y (ver Text.cpp, D18) avisa en vez
# de colgarse. Como TKEY y TDAT se reescriben al añadir claves, **todo lo que
# va detrás se mueve** y los offsets de TABL dejan de valer: hay que reajustarlos
# cada vez. Fue exactamente el fallo de la pantalla negra del 21/09 (-214 bytes).
# ---------------------------------------------------------------------------
def mission_blocks(blob):
    """-> [(i, nombre8, offset)] de la tabla TABL del fichero (primer chunk)."""
    if blob[:4] != b"TABL":
        raise SystemExit("ERROR: el fichero no empieza por TABL")
    size = struct.unpack_from("<I", blob, 4)[0]
    if size % 12:
        raise SystemExit("ERROR: TABL no es múltiplo de 12 bytes (%d)" % size)
    return [(i,
             bytes(blob[8 + i * 12: 8 + i * 12 + 8]),
             struct.unpack_from("<I", blob, 8 + i * 12 + 8)[0])
            for i in range(size // 12)]


def _block_positions(blob, nombre8):
    """Posiciones donde aparece `nombre8` + "TKEY" (el inicio del bloque de la
    misión). Se buscan TODAS para exigir que sea única."""
    pat = nombre8 + b"TKEY"
    hits, start = [], 0
    while True:
        p = blob.find(pat, start)
        if p < 0:
            return hits
        hits.append(p)
        start = p + 1


def retarget_tabl(blob):
    """-> (blob_nuevo, cambios, problemas).

    Deja cada offset de TABL apuntando al bloque real de su misión. `MAIN` se
    queda como está: apunta a la tabla global (un TKEY suelto, sin nombre).
    """
    tabl_size = struct.unpack_from("<I", blob, 4)[0]
    tabla = bytearray(blob[8:8 + tabl_size])
    cambios, problemas = [], []
    for i, nombre8, viejo in mission_blocks(blob):
        if nombre8.split(b"\0")[0] == b"MAIN":
            continue
        hits = _block_positions(blob, nombre8)
        if len(hits) != 1:
            problemas.append("%s: %d bloques encontrados" % (
                nombre8.split(b"\0")[0].decode("latin1"), len(hits)))
            continue
        if hits[0] != viejo:
            cambios.append((nombre8.split(b"\0")[0].decode("latin1"), viejo, hits[0]))
            struct.pack_into("<I", tabla, i * 12 + 8, hits[0])
    if not cambios:
        return blob, [], problemas
    return blob[:8] + bytes(tabla) + blob[8 + tabl_size:], cambios, problemas


def check_tabl(path):
    """Dice si los offsets de misión apuntan (o no) a su bloque. 1 = mal."""
    blob = open(path, "rb").read()
    _, cambios, problemas = retarget_tabl(blob)
    for nombre, viejo, bueno in cambios:
        print("  DESFASADO %-8s TABL=%d  bloque real=%d  (%+d)" % (nombre, viejo, bueno, bueno - viejo))
    for p in problemas:
        print("  PROBLEMA  %s" % p)
    if cambios or problemas:
        print("%s: TABL MAL (%d desfasadas, %d problemas)" % (path, len(cambios), len(problemas)))
        return 1
    print("%s: TABL OK (%d misiones)" % (path, len(mission_blocks(blob)) - 1))
    return 0


def fix_tabl(path):
    """Reajusta los offsets de TABL y verifica el resultado. Sólo toca TABL:
    el resto del fichero queda byte a byte igual."""
    blob = open(path, "rb").read()
    nuevo, cambios, problemas = retarget_tabl(blob)
    if problemas:
        raise SystemExit("ERROR: %s: %s" % (path, "; ".join(problemas)))
    if not cambios:
        print("%s: TABL ya estaba bien (%d misiones)" % (path, len(mission_blocks(blob)) - 1))
        return
    tmp = path + ".tmp"
    with open(tmp, "wb") as f:
        f.write(nuevo)
    # autocomprobación: releer y exigir que ya no haya nada que reajustar
    _, otra_vez, problemas2 = retarget_tabl(open(tmp, "rb").read())
    if problemas2 or otra_vez:
        os.unlink(tmp)
        raise SystemExit("ERROR: %s: el reajuste no se estabiliza" % path)
    _, _, e_antes = parse(path)
    _, _, e_despues = parse(tmp)
    if collections.Counter((k, v) for k, _, v in e_antes) != collections.Counter((k, v) for k, _, v in e_despues):
        os.unlink(tmp)
        raise SystemExit("ERROR: %s: se tocarían claves" % path)
    os.replace(tmp, path)
    print("%s -> TABL reajustado: %d misiones %s [verificado]" % (
        path, len(cambios),
        " ".join("%s%+d" % (n, b - a) for n, a, b in cambios[:6])))


def write_gxt(path, nuevos, force=False):
    """Añade/actualiza `nuevos` = {CLAVE: texto} y verifica el resultado."""
    chunks, tail, entries = parse(path)
    existentes = {k for k, _, _ in entries}

    omitidas = []
    if not force:
        for k in [k for k in nuevos if k in existentes]:
            omitidas.append(k)
            del nuevos[k]

    for k in nuevos:
        if len(k) > MAXKEY:
            raise SystemExit("ERROR: clave '%s' > %d caracteres" % (k, MAXKEY))
        if k != k.upper():
            raise SystemExit("ERROR: clave '%s' no está en mayúsculas" % k)

    tdat = bytearray()
    tkey_body = None
    for magic, body in chunks:
        if magic == b"TDAT":
            tdat = bytearray(body)
        elif magic == b"TKEY":
            tkey_body = body
    assert tkey_body is not None

    # nuevas cadenas al final de TDAT; las claves que se actualizan sueltan
    # todas sus copias previas (si las hay) y se quedan con la nueva.
    filas = [(k, off, v) for k, off, v in entries if k not in nuevos]
    for k, texto in nuevos.items():
        off = len(tdat)
        valor = texto.encode("utf-16le") + b"\0\0"
        tdat += valor
        filas.append((k, off, valor[:-2]))

    filas.sort(key=lambda fila: fila[0].encode("latin1"))  # stable: repetidas igual

    tkey = bytearray()
    for k, off, _v in filas:
        tkey += struct.pack("<I", off) + k.encode("latin1").ljust(KEY_LEN, b"\0")

    out = bytearray()
    for magic, body in chunks:
        if magic == b"TKEY":
            out += b"TKEY" + struct.pack("<I", len(tkey)) + tkey
        elif magic == b"TDAT":
            out += b"TDAT" + struct.pack("<I", len(tdat)) + tdat
        else:
            out += magic + struct.pack("<I", len(body)) + body
    out += tail
    blob = bytes(out)

    # TABL: al crecer TKEY/TDAT todo lo que va detrás se mueve, así que los
    # offsets de misión se recalculan aquí. Sin esto los textos de misión
    # quedan inalcanzables (fue la pantalla negra del 21/09).
    if blob[:4] == b"TABL":
        blob, movidos, problemas = retarget_tabl(blob)
        if problemas:
            raise SystemExit("ERROR: %s: TABL incoherente: %s" % (path, "; ".join(problemas)))
        if movidos:
            print("  TABL: %d misiones reajustadas (%s)" % (
                len(movidos), " ".join("%s%+d" % (n, b - a) for n, a, b in movidos[:4])))

    tmp = path + ".tmp"
    with open(tmp, "wb") as f:
        f.write(blob)

    # autocomprobación: releemos el temporal y comparamos TODAS las claves
    _, tail2, releidas = parse(tmp)
    if tail2 != tail:
        raise SystemExit("ERROR: %s: la cola del GXT no se preservó" % path)
    antes = collections.Counter((k, v) for k, _, v in entries if k not in nuevos)
    despues = collections.Counter((k, v) for k, _, v in releidas if k not in nuevos)
    if antes != despues:
        solo_antes = list((antes - despues))[:5]
        solo_despues = list((despues - antes))[:5]
        raise SystemExit("ERROR: %s: se tocarían claves ajenas: %r / %r"
                         % (path, solo_antes, solo_despues))
    visto = {k: v for k, _, v in releidas}
    for k, texto in nuevos.items():
        if visto.get(k) != texto.encode("utf-16le"):
            raise SystemExit("ERROR: %s: la clave %s no se releyó bien" % (path, k))
    os.replace(tmp, path)

    print("%s -> %d entradas, %d bytes (+%d)%s  [verificado]"
          % (path, len(releidas), len(blob), len(nuevos),
             ("; ya existían: " + " ".join(omitidas)) if omitidas else ""))


def pares_a_valores(pares):
    nuevos = {}
    for arg in pares:
        k, sep, v = arg.partition("=")
        if not sep:
            raise SystemExit("ERROR: '%s' no es CLAVE=valor" % arg)
        nuevos[k.upper()] = v
    return nuevos


def pares_de_tsv(tsv):
    pares = []
    with open(tsv, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            campos = line.split("\t")
            if len(campos) < 3:
                raise SystemExit("ERROR: %s: línea sin 3 campos: %r" % (tsv, line))
            pares.append("%s=%s" % (campos[1], campos[2]))
    if not pares:
        raise SystemExit("ERROR: %s no tiene claves" % tsv)
    return pares


def main():
    argv = [a for a in sys.argv[1:] if a != "--force"]
    force = "--force" in sys.argv
    if not argv:
        raise SystemExit(__doc__)
    if argv[0] == "self-test":
        print(self_test())
        return 0
    # Subcomandos de un solo fichero (se comprueban antes del mínimo de 3
    # argumentos, que es solo para los que llevan claves).
    if argv[0] in ("tabl", "check-tabl", "fix-tabl"):
        if len(argv) < 2:
            raise SystemExit(__doc__)
        if argv[0] == "tabl":
            for i, nombre8, off in mission_blocks(open(argv[1], "rb").read()):
                print("%-4d %-8s off=%d" % (i, nombre8.split(b"\0")[0].decode("latin1"), off))
            return 0
        if argv[0] == "check-tabl":
            return check_tabl(argv[1])
        fix_tabl(argv[1])
        return 0
    if len(argv) < 3:
        raise SystemExit(__doc__)
    cmd, path = argv[0], argv[1]
    if cmd == "lista":
        _, _, entries = parse(path)
        for k, _, v in entries:
            print("%-8s %s" % (k, text_of(v)))
    elif cmd == "claves":
        _, _, entries = parse(path)
        filt = argv[2].upper() if len(argv) > 2 else ""
        for k, _, v in entries:
            if filt in k:
                print("%-8s %s" % (k, text_of(v)))
        largas = [k for k, _, _ in entries if len(k) >= 8]
        print("# entradas=%d; claves de 8 caracteres=%d %s"
              % (len(entries), len(largas), largas[:6]), file=sys.stderr)
    elif cmd == "check":
        _, _, entries = parse(path)
        visto = {k for k, _, _ in entries}
        faltan = [k.upper() for k in argv[2:] if k.upper() not in visto]
        if faltan:
            print("FALTAN en %s: %s" % (path, " ".join(faltan)))
            return 1
        print("%s: %d claves, todas las pedidas presentes" % (path, len(visto)))
    elif cmd == "add":
        write_gxt(path, pares_a_valores(argv[2:]), force)
    elif cmd == "add-tsv":
        write_gxt(path, pares_a_valores(pares_de_tsv(argv[2])), force)
    else:
        raise SystemExit(__doc__)
    return 0


if __name__ == "__main__":
    sys.exit(main())
