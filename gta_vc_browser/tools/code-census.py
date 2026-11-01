#!/usr/bin/env python3
import argparse
import difflib
import hashlib
import os
import re
import sys

BASE = os.path.join('gta_vc_browser', 'tmp', 'revc-a16fcd8d', 'src')
SRC = 'src'
ATTR = re.compile(r'PORTADO|\u00a9|\(c\)|MIT|https?://|courtesy|cr\u00e9dito|credito|\bautor\b|\bauthor\b', re.I)
FUNC = re.compile(r'^[A-Za-z_~][\w \*&:<>,]*\([^;{}]*\)\s*$')


def read_text(path):
    with open(path, 'rb') as fh:
        raw = fh.read()
    return raw.decode('utf-8', 'replace').replace('\r\n', '\n')


def read_lines(path):
    with open(path, 'rb') as fh:
        raw = fh.read()
    return raw.decode('utf-8', 'replace').splitlines(keepends=True)


def rel_files():
    out = []
    for root, _, files in os.walk(BASE):
        for name in files:
            bp = os.path.join(root, name)
            rel = os.path.relpath(bp, BASE)
            cur = os.path.join(SRC, rel)
            if os.path.exists(cur):
                out.append(rel.replace(os.sep, '/'))
    for root, _, files in os.walk(SRC):
        for name in files:
            cur = os.path.join(root, name)
            rel = os.path.relpath(cur, SRC).replace(os.sep, '/')
            if not os.path.exists(os.path.join(BASE, rel)):
                out.append(rel)
    return sorted(out)


def is_comment_line(s):
    t = s.strip()
    return t.startswith('//') or t.startswith('/*') or t.startswith('*') or t.startswith('*/')


def added_lines(rel):
    cur = os.path.join(SRC, rel)
    base = os.path.join(BASE, rel)
    b = read_text(cur).split('\n')
    if not os.path.exists(base):
        return [(i + 1, line) for i, line in enumerate(b)]
    a = read_text(base).split('\n')
    if a == b:
        return []
    out = []
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag in ('insert', 'replace'):
            for j in range(j1, j2):
                out.append((j + 1, b[j]))
    return out


def own_comments(rel):
    path = os.path.join(SRC, rel)
    try:
        text = read_text(path)
        if not _utf8_ok(path):
            return []
    except OSError:
        return []
    lines = text.split('\n')
    full = fully_comment_lines(text)
    out = []
    for ln, _ in added_lines(rel):
        if ln - 1 < len(full) and full[ln - 1]:
            out.append((ln, lines[ln - 1]))
    return out


def _utf8_ok(path):
    with open(path, 'rb') as fh:
        raw = fh.read()
    try:
        raw.decode('utf-8')
        return True
    except UnicodeDecodeError:
        return False


def comment_mask(text):
    mask = [False] * len(text)
    i = 0
    n = len(text)
    state = 0
    prev = ''
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if state == 0:
            if c == '/' and nxt in '/*' and prev and (prev.isalnum() or prev in '_) ]'):
                i += 1
                prev = c
                continue
            if c == '/' and nxt == '/':
                mask[i] = mask[i + 1] = True
                i += 2
                while i < n and text[i] != '\n':
                    mask[i] = True
                    i += 1
                continue
            if c == '/' and nxt == '*':
                mask[i] = mask[i + 1] = True
                state = 1
                i += 2
                continue
            if c == '"' or c == "'":
                q = c
                i += 1
                while i < n:
                    if text[i] == '\\':
                        i += 2
                        continue
                    if text[i] == q:
                        i += 1
                        break
                    i += 1
                prev = q
                continue
            prev = '' if c == '\n' else (c if not c.isspace() else prev)
            i += 1
        else:
            mask[i] = True
            if c == '*' and nxt == '/':
                mask[i + 1] = True
                state = 0
                i += 2
                continue
            i += 1
    return mask


def strip_comments(text):
    mask = comment_mask(text)
    out = []
    for ch, m in zip(text, mask):
        if not m:
            out.append(ch)
        elif ch == '\n':
            out.append(ch)
    return ''.join(out)


def block_depths(text):
    out = [0]
    state = 0
    prev = ''
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if state == 0:
            if c == '/' and nxt in '/*' and prev and (prev.isalnum() or prev in '_) ]'):
                i += 1
                prev = '/'
                continue
            if c == '/' and nxt == '/':
                state = 1
                i += 2
                continue
            if c == '/' and nxt == '*':
                state = 2
                i += 2
                continue
            if c == '"' or c == "'":
                q = c
                i += 1
                while i < n:
                    if text[i] == '\\':
                        i += 2
                        continue
                    if text[i] == q:
                        i += 1
                        break
                    i += 1
                prev = q
                continue
            if c == '\n':
                out.append(0)
                prev = ''
            elif not c.isspace():
                prev = c
            i += 1
        elif state == 1:
            if c == '\n':
                state = 0
                out.append(0)
                prev = ''
            i += 1
        else:
            if c == '*' and nxt == '/':
                state = 0
                i += 2
                continue
            if c == '\n':
                out.append(1)
            i += 1
    return out


def comment_balance(text):
    out = 0
    state = 0
    prev = ''
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if state == 0:
            if c == '/' and nxt in '/*' and prev and (prev.isalnum() or prev in '_) ]'):
                i += 1
                prev = '/'
                continue
            if c == '/' and nxt == '/':
                state = 1
                i += 2
                continue
            if c == '/' and nxt == '*':
                state = 2
                i += 2
                continue
            if c == '"' or c == "'":
                q = c
                i += 1
                while i < n:
                    if text[i] == '\\':
                        i += 2
                        continue
                    if text[i] == q:
                        i += 1
                        break
                    i += 1
                prev = q
                continue
            prev = '' if c == '\n' else (c if not c.isspace() else prev)
            i += 1
        elif state == 1:
            if c == '\n':
                state = 0
                prev = ''
            i += 1
        else:
            if c == '*' and nxt == '/':
                state = 0
                i += 2
                continue
            i += 1
    return state


def cmd_integridad(args):
    bad = []
    for rel in rel_files():
        path = os.path.join(SRC, rel)
        if not _utf8_ok(path):
            continue
        if comment_balance(read_text(path)) != 0:
            bad.append(rel)
    for rel in bad:
        print('COMENTARIO SIN CERRAR: src/%s' % rel)
    print('TOTAL ficheros con comentario sin cerrar: %d' % len(bad))
    return 1 if bad else 0


def fully_comment_lines(text):
    mask = comment_mask(text)
    lines = text.split('\n')
    pos = 0
    out = []
    for line in lines:
        seg = mask[pos:pos + len(line)]
        full = bool(line.strip()) and len(seg) == len(line) and all(
            m or ch in ' \t' for ch, m in zip(line, seg))
        out.append(full)
        pos += len(line) + 1
    return out


def removable_lines(path, rel):
    text = read_text(path)
    lines = text.split('\n')
    commentable = set(ln for ln, s in added_lines(rel))
    full = fully_comment_lines(text)
    depth = block_depths(text)
    out = []
    i = 0
    while i < len(lines):
        if not full[i] or (i + 1) not in commentable:
            i += 1
            continue
        j = i
        while j + 1 < len(lines) and full[j + 1] and (j + 2) in commentable:
            j += 1
        if depth[i] == 0 and depth[min(j + 1, len(depth) - 1)] == 0:
            out.extend(range(i, j + 1))
        i = j + 1
    return out


def cmd_comentarios(args):
    total = 0
    files = 0
    for rel in rel_files():
        items = own_comments(rel)
        if not items:
            continue
        files += 1
        total += len(items)
        print('%5d  src/%s' % (len(items), rel))
        if args.verbose:
            for ln, s in items:
                print('        src/%s:%d  %s' % (rel, ln, s.strip()[:110]))
    print('TOTAL comentarios propios: %d en %d ficheros' % (total, files))
    return 1 if total else 0


def cmd_atribuciones(args):
    total = 0
    for rel in rel_files():
        hits = [(ln, s) for ln, s in own_comments(rel) if ATTR.search(s)]
        if not hits:
            continue
        total += len(hits)
        for ln, s in hits:
            print('src/%s:%d  %s' % (rel, ln, s.strip()[:110]))
    print('TOTAL lineas de atribucion: %d' % total)
    return 1 if total else 0


def funcs_in(text):
    lines = text.split('\n')
    out = []
    i = 0
    while i < len(lines):
        if FUNC.match(lines[i]) and i + 1 < len(lines) and lines[i + 1].strip().startswith('{'):
            depth = 0
            start = i
            j = i + 1
            while j < len(lines):
                depth += lines[j].count('{') - lines[j].count('}')
                if depth <= 0:
                    break
                j += 1
            name = lines[i].strip()
            out.append((name, start + 1, j + 1))
            i = j + 1
        else:
            i += 1
    return out


def cmd_funciones(args):
    rows = []
    for rel in rel_files():
        text = read_text(os.path.join(SRC, rel))
        for name, a, b in funcs_in(text):
            n = b - a + 1
            if n >= args.minimo:
                rows.append((n, rel, a, b, name))
    rows.sort(reverse=True)
    for n, rel, a, b, name in rows:
        print('%5d  src/%s:%d-%d  %s' % (n, rel, a, b, name[:90]))
    print('TOTAL funciones >= %d lineas: %d' % (args.minimo, len(rows)))
    return 0


def cmd_duplicados(args):
    windows = {}
    for rel in rel_files():
        lines = [s.strip() for _, s in added_lines(rel) if not is_comment_line(s)]
        lines = [re.sub(r'"[^"]*"', 'S', re.sub(r'\b\d+(\.\d+)?f?\b', 'N', s)) for s in lines]
        lines = [re.sub(r'\s+', ' ', s) for s in lines if len(s) > 21]
        for i in range(len(lines) - 2):
            k = (lines[i], lines[i + 1], lines[i + 2])
            windows.setdefault(k, []).append((rel, i))
    hits = [(k, v) for k, v in windows.items() if len(v) >= args.minimo]
    hits.sort(key=lambda kv: -len(kv[1]))
    for k, v in hits:
        rel, _ = v[0]
        print('%3dx src/%s  %s' % (len(v), rel, k[0][:80]))
    print('TOTAL ventanas repetidas >= %d: %d' % (args.minimo, len(hits)))
    return 0


def cmd_purgar(args):
    removed = 0
    touched = 0
    for rel in rel_files():
        path = os.path.join(SRC, rel)
        idx = removable_lines(path, rel)
        if not idx:
            continue
        keep = set(idx)
        lines = read_lines(path)
        before = ''.join(lines)
        after = ''.join(l for i, l in enumerate(lines) if i not in keep)
        if ''.join(strip_comments(before).split()) != ''.join(strip_comments(after).split()):
            print('SKIP (contenido cambiaria): src/%s' % rel)
            continue
        if not _utf8_ok(path):
            print('SKIP (no utf-8): src/%s' % rel)
            continue
        if not args.ensayo:
            with open(path, 'wb') as fh:
                fh.write(after.encode('utf-8'))
        removed += len(idx)
        touched += 1
        print('%5d  src/%s' % (len(idx), rel))
    print('%s: %d lineas de comentario en %d ficheros' % ('ENSAYO' if args.ensayo else 'BORRADAS', removed, touched))
    return 0


def cmd_firma(args):
    h = hashlib.sha256()
    for rel in rel_files():
        path = os.path.join(SRC, rel)
        code = ''.join(strip_comments(read_text(path)).split())
        h.update(rel.encode() + b'\0' + code.encode() + b'\0')
    print(h.hexdigest())
    return 0


def main():
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument('--comentarios', action='store_true')
    ap.add_argument('--atribuciones', action='store_true')
    ap.add_argument('--funciones', action='store_true')
    ap.add_argument('--duplicados', action='store_true')
    ap.add_argument('--purgar', action='store_true')
    ap.add_argument('--ensayo', action='store_true')
    ap.add_argument('--firma', action='store_true')
    ap.add_argument('--integridad', action='store_true')
    ap.add_argument('--verbose', action='store_true')
    ap.add_argument('--minimo', type=int, default=4)
    args = ap.parse_args()
    if not os.path.isdir(BASE):
        print('falta el baseline %s' % BASE)
        return 2
    if args.comentarios:
        return cmd_comentarios(args)
    if args.atribuciones:
        return cmd_atribuciones(args)
    if args.funciones:
        return cmd_funciones(args)
    if args.duplicados:
        return cmd_duplicados(args)
    if args.purgar:
        return cmd_purgar(args)
    if args.firma:
        return cmd_firma(args)
    if args.integridad:
        return cmd_integridad(args)
    ap.print_help()
    return 2


if __name__ == '__main__':
    sys.exit(main())
