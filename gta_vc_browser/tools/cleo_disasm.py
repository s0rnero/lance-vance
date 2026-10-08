#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
cleo_disasm.py - desensamblador SCM/CLEO de GTA Vice City (fase 1: SOLO LECTURA).

Uso:
    python cleo_disasm.py FICHERO.cs|.scm [opciones]

Opciones:
    --start N     offset inicial (decimal o 0xHEX)
    --end N       offset final (exclusivo)
    --bytes       muestra los bytes crudos de cada instruccion
    --hist        solo histograma de opcodes + pares ANDOR + desconocidos
    --header      interpreta la cabecera multi-script (main.scm) como el motor
    --vcjson P    ruta a vc.json (defecto: ../tmp/extsrc/vc.json relativo a esta herramienta)
    --max N       maximo de instrucciones a listar

FORMATO BYTECODE (oraculo = nuestro motor src/control, no documentacion externa):
  * opcode: u16 LE. Bit 0x8000 = NOT (m_bNotFlag; Script5.cpp UpdateCompareFlag
    niega la condicion individual).
  * args autodelimitados por tag de 1 byte (Script.h enum ARGUMENT_*):
      0 = ARGUMENT_END   (fin de las listas 'arguments')
      1 = ARGUMENT_INT32    -> i32 LE  (4 B)
      2 = ARGUMENT_GLOBALVAR -> u16 offset en ScriptSpace (se imprime $N)
      3 = ARGUMENT_LOCALVAR  -> u16 indice de local  (se imprime N@)
      4 = ARGUMENT_INT8      -> i8 (1 B)
      5 = ARGUMENT_INT16     -> i16 LE (2 B)
      6 = ARGUMENT_FLOAT     -> f32 IEEE LE (4 B)
    (Script.cpp CollectParameters/StoreParameters; ojo: dentro de la lista de
    ARGUMENT_START_NEW_SCRIPT (004F) el motor usa ReadFloatFromScript = i16/16,
    replicado aqui como QUIRK_004F.)
  * args 'string' del vc.json = 8 bytes crudos SIN tag
    (KEY_LENGTH_IN_SCRIPT = 8, Script.h:48; ReadTextLabelFromScript).
  * args 'arguments' del vc.json (004F, 05E1-E4, 05F5, 0AA5-A8, 0AB1...)
    = args etiquetados hasta tag 0x00 (Script.cpp:1624 COMMAND_START_NEW_SCRIPT).
    Para CALL_FUNCTION/CALL_METHOD el numero de params viaja aparte (numParams)
    y se contrasta con los leidos; el byte 0x00 final cierra la lista.
  * labels (GOTO/GOSUB/...) = int32 etiquetado; NEGATIVO = offset dentro del
    script (-valor); POSITIVO/0 = absoluto en ScriptSpace
    (Script.cpp SetIP(ScriptParams[0] >= 0 ? p : SIZE_MAIN_SCRIPT - p)).
  * ANDOR (00D6, Script.h enum + Script.cpp handler con state++):
      0      = condicion suelta (1)
      1..8   = AND  de (n+1) condiciones   (ANDS_1..ANDS_8 = 1..8)
      21..28 = OR   de (n-19) condiciones  (ORS_1..ORS_8 = 21..28)
    Un bloque de condiciones acaba en GOTO_IF_TRUE (004C) o GOTO_IF_FALSE (004D).

C11 (ids de vc.json): los ids son SIEMPRE 4 digitos hex ("05FE", "0001").
Hay que indexarlos con int(id, 16); int(id) decimal o indexar por NOMBRE provoca
colisiones (nombres repetidos en 34 pares CLEO/bitwise y CLEO1/CLEO4).
"""

import argparse
import json
import os
import struct
import sys

KEY_LENGTH = 8          # KEY_LENGTH_IN_SCRIPT (Script.h:48)
ARG_END = 0
TAG_SIZES = {1: 4, 2: 2, 3: 2, 4: 1, 5: 2, 6: 4}
TAG_NAMES = {1: "i32", 2: "gvar", 3: "lvar", 4: "i8", 5: "i16", 6: "f32"}

OP_ANDOR = 0x00D6
OP_GOTO_IF_TRUE = 0x004C
OP_GOTO_IF_FALSE = 0x004D
NOT_MASK = 0x8000


def default_vcjson():
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(here, "..", "tmp", "extsrc", "vc.json")


def load_vcjson(path):
    with open(path, encoding="utf-8") as f:
        d = json.load(f)
    by_op = {}
    name_to_ids = {}
    for e in d.get("extensions", []):
        for c in e.get("commands") or []:
            cid = c["id"]
            op = int(cid, 16)          # C11: hex fijo de 4 chars, nunca decimal
            entry = {
                "id": cid,
                "name": c.get("name", "?"),
                "ext": e.get("name", "?"),
                "input": [(a.get("name", ""), a.get("type", "any"))
                          for a in c.get("input") or []],
                "output": [(a.get("name", ""), a.get("type", "any"))
                           for a in c.get("output") or []],
            }
            if op in by_op:
                sys.stderr.write("AVISO: opcode 0x%04X duplicado en vc.json (%s/%s)\n"
                                 % (op, by_op[op]["name"], entry["name"]))
            by_op[op] = entry
            name_to_ids.setdefault(entry["name"], []).append(cid)
    return by_op, name_to_ids


class Derail(Exception):
    pass


def read_tagged(data, pos, opcode, allow_end=False):
    """Lee 1 arg autodelimitado por tag. Devuelve (texto, nueva_pos)."""
    if pos >= len(data):
        raise Derail("fin inesperado leyendo tag de arg")
    tag = data[pos]
    if tag == ARG_END:
        if allow_end:
            return None, pos + 1
        raise Derail("ARGUMENT_END (0x00) inesperado en @%04X" % pos)
    if tag not in TAG_SIZES:
        raise Derail("tag de arg desconocido 0x%02X en @%04X" % (tag, pos))
    p = pos + 1
    n = TAG_SIZES[tag]
    if p + n > len(data):
        raise Derail("fin inesperado leyendo arg (tag %d) en @%04X" % (tag, pos))
    if tag == 1:
        v = struct.unpack_from("<i", data, p)[0]
        return "%d" % v, p + 4
    if tag == 2:
        v = struct.unpack_from("<H", data, p)[0]
        return "$%d" % v, p + 2
    if tag == 3:
        v = struct.unpack_from("<H", data, p)[0]
        return "%d@" % v, p + 2
    if tag == 4:
        v = struct.unpack_from("<b", data, p)[0]
        return "%d" % v, p + 1
    if tag == 5:
        v = struct.unpack_from("<h", data, p)[0]
        return "%d" % v, p + 2
    if tag == 6:
        v = struct.unpack_from("<f", data, p)[0]
        return "%g" % v, p + 4
    raise Derail("tag %d sin implementar" % tag)


def read_arg(data, pos, atype, opcode, allow_end=False):
    """Lee 1 arg segun el tipo declarado en vc.json."""
    at = (atype or "any").lower()
    if at in ("string", "gxt_key", "zone_key"):
        if pos + KEY_LENGTH > len(data):
            raise Derail("fin inesperado leyendo string en @%04X" % pos)
        s = data[pos:pos + KEY_LENGTH].split(b"\0")[0]
        return "'%s'" % s.decode("latin1"), pos + KEY_LENGTH
    if at == "arguments":
        # lista de args etiquetados hasta ARGUMENT_END (0x00)
        out = []
        while True:
            if pos >= len(data):
                raise Derail("fin inesperado en lista 'arguments' de 0x%04X" % opcode)
            # quirk del motor: floats de 2 bytes dentro de la lista de 004F
            if opcode == 0x004F and data[pos] == 6:
                if pos + 3 > len(data):
                    raise Derail("fin inesperado en lista 'arguments' de 004F")
                v = struct.unpack_from("<h", data, pos + 1)[0] / 16.0
                out.append("%g(f16)" % v)
                pos += 3
                continue
            txt, pos = read_tagged(data, pos, opcode, allow_end=True)
            if txt is None:
                break
            out.append(txt)
        return "[" + ", ".join(out) + "]", pos
    return read_tagged(data, pos, opcode, allow_end=allow_end)


def andor_expected(param):
    """Condiciones esperadas para el arg de ANDOR (00D6)."""
    if param == 0:
        return 1
    if 1 <= param <= 8:
        return param + 1
    if 21 <= param <= 28:
        return param - 19
    return None


def find_cs_symtab(data):
    """Busca la tabla de simbolos de Sanny al final de un .cs: entradas
    [u32][nombre ASCII-Z] ordenadas por nombre hasta EOF. Devuelve su offset
    (fin del codigo) o None si no hay tabla."""
    n = len(data)
    cands = []
    for o in range(0, n - 4):
        p = o
        names = []
        ok = True
        while p < n:
            if p + 4 > n:
                ok = False
                break
            # tráiler del final: [u32][u32]['__SBFTR'\0] (con el tamaño de codigo)
            e2 = data.find(b"\0", p + 8) if p + 8 < n else -1
            if e2 > p + 8 and data[p + 8:e2] == b"__SBFTR" and e2 + 1 == n:
                names.append(b"__SBFTR")
                p = n
                break
            p += 4
            e = data.find(b"\0", p)
            if e <= p or e - p > 64:
                ok = False
                break
            nm = data[p:e]
            if not all(32 <= c <= 122 for c in nm) or not any(65 <= c <= 90 for c in nm):
                ok = False
                break
            names.append(nm)
            p = e + 1
        if ok and len(names) >= 5 and names == sorted(names):
            cands.append(o)
    return min(cands) if cands else None


def disasm(data, by_op, start, end):
    """Recorre el bytecode y devuelve (instrucciones, customs_detectados)."""
    ins = []
    customs = []
    pos = start
    while pos < end:
        if pos + 2 > len(data):
            break
        op_raw = struct.unpack_from("<H", data, pos)[0]
        notflag = bool(op_raw & NOT_MASK)
        op = op_raw & ~NOT_MASK
        entry = by_op.get(op)
        p = pos + 2
        args = []
        err = None
        if entry is None:
            err = "opcode 0x%04X no esta en vc.json" % op
        else:
            try:
                for _, at in entry["input"]:
                    txt, p = read_arg(data, p, at, op)
                    args.append(txt)
                for _, at in entry["output"]:
                    txt, p = read_arg(data, p, at, op)
                    args.append(txt)
            except Derail as ex:
                # fallback: args de texto 8B que vc.json no tipa como string
                q = p
                if (q + KEY_LENGTH <= len(data) and data[q] not in TAG_SIZES
                        and not any(1 <= c < 32 for c in data[q:q + KEY_LENGTH])):
                    s = data[q:q + KEY_LENGTH].split(b"\0")[0]
                    args.append("'%s'«8B»" % s.decode("latin1"))
                    p = q + KEY_LENGTH
                    try:
                        for _, at in entry["output"]:
                            txt, p = read_arg(data, p, at, op)
                            args.append(txt)
                    except Derail as ex2:
                        err = str(ex2)
                else:
                    err = str(ex)
        if err is None:
            ins.append({
                "pos": pos, "op": op, "not": notflag, "args": args,
                "name": entry["name"], "ext": entry["ext"],
                "raw": data[pos:p],
            })
            pos = p
            continue

        # DESALINEADO: probable opcode custom (p.ej. 0x0FA8/0x0FA9 del
        # main.scm de Vice Extended). Se re-sincroniza por fuerza bruta:
        # se prueban cortes de 1..32 B y se elige el que permite decodificar
        # mas instrucciones seguidas (los args raramente forman un stream
        # valido largo).
        def probe(q):
            nn, pp = 0, q
            while pp < end and nn < 40:
                if pp + 2 > len(data):
                    break
                o2 = struct.unpack_from("<H", data, pp)[0] & ~NOT_MASK
                e2 = by_op.get(o2)
                if e2 is None:
                    break
                pp2 = pp + 2
                try:
                    for _, at2 in e2["input"]:
                        _, pp2 = read_arg(data, pp2, at2, o2)
                    for _, at2 in e2["output"]:
                        _, pp2 = read_arg(data, pp2, at2, o2)
                except Derail:
                    break
                nn += 1
                pp = pp2
            return nn

        best = (0, 1)
        for k in range(1, 33):
            n2 = probe(pos + k)
            if n2 > best[0]:
                best = (n2, k)
            if n2 >= 40:
                break
        k = best[1]
        sys.stderr.write("[resync] @%04X opcode 0x%04X tratado como CUSTOM de %d B (%s) [%s]\n"
                         % (pos, op, k, data[pos:pos + k].hex(" "), err))
        customs.append((pos, op, k))
        ins.append({
            "pos": pos, "op": op, "not": notflag,
            "args": ["«CUSTOM %d B: %s»" % (k, data[pos + 2:pos + k].hex(" "))],
            "name": entry["name"] + "«args?»" if entry else "??CUSTOM",
            "ext": "custom", "raw": data[pos:pos + k],
        })
        pos += k
    return ins, customs


def annotate_andor(ins):
    """Marca los bloques ANDOR (param -> condiciones reales)."""
    for i, it in enumerate(ins):
        if it["op"] != OP_ANDOR or not it["args"]:
            continue
        try:
            param = int(it["args"][0])
        except ValueError:
            continue
        n = 0
        j = i + 1
        while j < len(ins):
            if ins[j]["op"] in (OP_GOTO_IF_TRUE, OP_GOTO_IF_FALSE):
                break
            n += 1
            j += 1
        exp = andor_expected(param)
        it["andor"] = (param, n, exp)


def fmt_ins(it, show_bytes):
    s = "%06X  " % it["pos"]
    if show_bytes:
        s += it["raw"].hex(" ").ljust(30) + "  "
    s += ("NOT " if it["not"] else "") + it["name"]
    if it["args"]:
        s += "  " + ", ".join(it["args"])
    if "andor" in it:
        param, n, exp = it["andor"]
        s += "     ; ANDOR=%d -> %d condiciones (esperadas %s)" % (
            param, n, exp if exp is not None else "?")
        if exp is not None and exp != n:
            s += "  *** DESAJUSTE ***"
    return s


def print_header(data):
    def u32(o):
        return struct.unpack_from("<I", data, o)[0]

    def u16(o):
        return struct.unpack_from("<H", data, o)[0]

    print("== cabecera multi-script (parser del motor: Script.cpp Init /")
    print("   Script5.cpp ReadObjectNamesFromScript / ReadMultiScriptFileOffsetsFromScript) ==")
    print("  [0:8]   :", data[:8].hex(" "))
    v = u32(3)
    print("  u32@3 (GetSizeOfVariableSpace) = %d (0x%X) -> variables en [8, %d)" % (v, v, v))
    print("  [%d:%d] (cabecera seccion objetos) :" % (v, v + 8), data[v:v + 8].hex(" "))
    obj_size = u32(v + 3)
    cnt = u16(v + 8)
    print("  u32@(V+3) = %d (0x%X) = fin del bloque de objetos (= inicio cabecera multiscript)"
          % (obj_size, obj_size))
    print("  u16@(V+8) = %d objetos usados; nombres de 24 B en [%d, %d)"
          % (cnt, v + 12, v + 12 + cnt * 24))
    for i in range(cnt):
        name = data[v + 12 + i * 24: v + 12 + (i + 1) * 24].split(b"\0")[0].decode("latin1")
        if name:
            print("      [%3d] %s" % (i, name))
    ip = obj_size + 8
    ms, lms, nms, nex = struct.unpack_from("<IIHH", data, ip)
    print("  multiscript@%d: MainScriptSize=%d LargestMission=%d NumMission=%d NumExclusive=%d"
          % (ip, ms, lms, nms, nex))
    # ojo: la cabecera del bloque son 12 B (u32+u32+u16+u16), NO 16
    for i in range(min(nms, 12)):
        off = u32(ip + 12 + 4 * i)
        print("      mission[%3d] @ %d (0x%X)" % (i, off, off))
    if nms > 12:
        print("      ... (%d mas)" % (nms - 12))
    print("  codigo principal: empieza tras la tabla, en %d" % (ip + 12 + 4 * nms))
    print("      (MainScriptSize es el OFFSET de fin del codigo principal = inicio del")
    print("      area de misiones; verificado: MultiScriptArray[0] == MainScriptSize")
    print("      en freeroam_miami.scm -> codigo = [%d, %d) = %d B)"
          % (ip + 12 + 4 * nms, ms, ms - (ip + 12 + 4 * nms)))


def main():
    ap = argparse.ArgumentParser(description="Desensamblador SCM/CLEO VC (fase 1, solo lectura)")
    ap.add_argument("file")
    ap.add_argument("--start", default=None)
    ap.add_argument("--end", default=None)
    ap.add_argument("--bytes", action="store_true")
    ap.add_argument("--hist", action="store_true")
    ap.add_argument("--header", action="store_true")
    ap.add_argument("--vcjson", default=default_vcjson())
    ap.add_argument("--max", type=int, default=0)
    a = ap.parse_args()

    with open(a.file, "rb") as f:
        data = f.read()
    by_op, name_to_ids = load_vcjson(a.vcjson)

    if a.header:
        print_header(data)
        return 0

    start = int(a.start, 0) if a.start else 0
    end = int(a.end, 0) if a.end else len(data)
    end = min(end, len(data))
    if a.end is None and a.start is None and data[:4] != b"\x02\x00\x01":
        # .cs de CLEO: puede acabar en tabla de simbolos de Sanny (no es codigo)
        # marcador "E VAR" = 45 00 56 41 52 00 y luego entradas [u32][nombre\0]
        sym = find_cs_symtab(data)
        if sym is not None and sym > 0:
            code_end = sym - 6 if data[sym - 6:sym] == b"E\0VAR\0" else sym
            sys.stderr.write("[info] tabla de simbolos Sanny desde 0x%X (%d B); codigo = [0, 0x%X)\n"
                             % (code_end, len(data) - code_end, code_end))
            end = code_end

    ins, customs = disasm(data, by_op, start, end)
    annotate_andor(ins)

    if a.hist:
        hist = {}
        for it in ins:
            k = (it["op"], it["name"], it["ext"])
            hist[k] = hist.get(k, 0) + 1
        print("== histograma (%d instrucciones decodificadas) ==" % len(ins))
        for (op, name, ext), n in sorted(hist.items(), key=lambda x: (-x[1], x[0][0])):
            custom = "" if op < 1500 else "  [>=1500]"
            print("  0x%04X  %-42s x%-4d (%s)%s" % (op, name, n, ext, custom))
        andors = [(it["andor"], it["pos"]) for it in ins if "andor" in it]
        if andors:
            print("== pares ANDOR (param, condiciones reales) ==")
            pairs = {}
            for (param, n, exp), _ in andors:
                pairs[(param, n, exp)] = pairs.get((param, n, exp), 0) + 1
            for (param, n, exp), c in sorted(pairs.items()):
                flag = "" if exp == n else "  *** DESAJUSTE ***"
                print("  ANDOR=%-3d -> %d conds (esperadas %s) x%d%s"
                      % (param, n, exp if exp is not None else "?", c, flag))
    else:
        shown = 0
        for it in ins:
            print(fmt_ins(it, a.bytes))
            shown += 1
            if a.max and shown >= a.max:
                print("... (cortado por --max)")
                break

    if customs:
        agg = {}
        for pos, op, k in customs:
            agg.setdefault((op, k), []).append(pos)
        sys.stderr.write("\n== opcodes CUSTOM detectados (resync) ==\n")
        for (op, k), ps in sorted(agg.items()):
            sys.stderr.write("  0x%04X  %d B (2 opcode + %d args)  x%d  @ %s...\n"
                             % (op, k, k - 2, len(ps),
                                ", ".join("%04X" % x for x in ps[:8])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
