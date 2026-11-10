#!/usr/bin/env python3
"""Parte en dos (izquierda/derecha) el material de lente de un geometry (58.0 T4).

Defecto que arregla: en esperant/boxville/mule/trash (chassis_hi) y fbiranch
(chassis_hi detras, bump_front_hi_ok delante) las lentes de los dos costados
viven en UN geometry y comparten UN objeto material. El motor (58.0 T3) cambia
la textura del material por su atlas derivado en el flanco encendido, asi que
con un solo material la ultima fila procesada gana: giro a la izquierda no
enciende nada y giro a la derecha enciende los dos cristales.

La operacion es INVISIBLE: solo reagrupa triangulos bajo dos slots de material.
  * El array de vertices, UVs, normales, prelight y posiciones NO se toca: el
    STRUCT se reescribe con el mismo prefix/suffix y solo cambian los 16 bits
    bajos de la segunda palabra de cada triangulo (el indice de material, ver
    dff_geom.parse_geometry y el comentario de indicator_audit._struct_remap).
  * Los dos slots nuevos son COPIAS BYTE A BYTE del chunk del material original
    (misma textura, color, ambient): el estado OFF es identico pixel a pixel, y
    librw resuelve la textura por nombre (TexDictionary::find), asi que las dos
    copias comparten puntero de RwTexture y el cambio por puntero del motor
    funciona igual en cada slot.
  * Cada triangulo del material va al slot izquierdo si el centroide de sus tres
    vertices en coordenadas de MUNDO (matriz completa del frame del atomic)
    tiene x < 0; si no, al derecho. Si el material no tiene caras en los dos
    costados, o alguna cae en x == 0, no se escribe nada (STOP).

Los slots originales conservan su indice (las mascaras de otras tablas no se
desplazan); las copias se anaden al final: izquierda = n, derecha = n+1. El
material original queda sin triangulos: esa es la senal de idempotencia (una
segunda pasada no vuelve a partir). OJO: fbiranch chassis_hi ya trae de fabrica
un material byte-identico al de la lente (mat6 == mat3), asi que la deteccion
NO puede ser "ya existe una copia".

Uso:
    python gta_vc_browser/tools/split_lens_material.py --dry   # solo informa
    python gta_vc_browser/tools/split_lens_material.py         # parte y verifica
    python gta_vc_browser/tools/split_lens_material.py --model mule

Escribe en las tres cajas donde el fichero exista (streamed, bootseed,
gamefiles). El respaldo previo lo hace el llamante (C:\\tmp\\iav42\\bak58t4).
"""
import argparse
import collections
import hashlib
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import dff_geom as dg            # noqa: E402
import indicator_audit as ia     # noqa: E402
import indicator_lens45 as L     # noqa: E402
import body_lens_audit as BA  # noqa: E402

STREAMED = os.path.join(REPO, "streamed", "models", "gta3.img")
TREES = [STREAMED,
         os.path.join(REPO, "bootseed", "models", "gta3.img"),
         os.path.join(REPO, "gamefiles", "models", "gta3.img")]

# (frame, indice del material de lente en ESE geometry). Los indices son los
# medidos ANTES del split; tras el, ese slot queda sin triangulos y la herramienta
# lo detecta como ya partido. Los sitios del plan 58.0 T3/T4 mas mrwhoop delante
# (58.1, chassis_hi mi=2). trash salio del set de carroceria en 58.1 (su fila se
# quito): su DFF ya estaba partido y se queda como esta (invisible).
TARGETS = {
    "esperant": [("chassis_hi", 0)],
    "boxville": [("chassis_hi", 2)],
    "mule": [("chassis_hi", 1)],
    "mrwhoop": [("chassis_hi", 2)],
    "fbiranch": [("chassis_hi", 3), ("bump_front_hi_ok", 1)],
}

# Plan 62.0: los modelos cuyo cristal comparte material con pintura, bajos o el
# cristal de otro eje. El modo cristal no usa TARGETS: se deriva de la medida.
# mule queda fuera a proposito: su lente es una sub-zona de UNA cara de 462 cm2
# (43,6 % ambar); partirle el material encenderia el panel entero. Se le deja en
# la ruta del atlas con la huella recortada, y se informa.
CRISTAL_MODELOS = ["mrwhoop", "rumpo", "pony", "topfun", "fbicar",
                   "boxville", "esperant", "fbiranch"]


def _sha(b):
    return hashlib.sha256(b).hexdigest()[:12]


def _world_tris(frames, fi, g):
    """Triangulos en coordenadas de mundo: (indices, material, centroide, verts)."""
    w = dg.world_of(frames, fi)
    out = []
    for t in g["tris"]:
        vs = [dg.mul(w, g["verts"][v]) for v in t[:3]]
        c = [sum(p[k] for p in vs) / 3.0 for k in range(3)]
        out.append((t[:3], t[3], c, vs))
    return out


def split_geometry(raw_geom, frames, fi, mi, groups):
    """Devuelve ((version, body), informe) o (None, motivo) si no hay que partir.

    `groups` es una lista de (etiqueta, indices de triangulo del geometry). Cada
    grupo recibe un slot nuevo al final del matlist; los triangulos del material
    `mi` que no estan en ningun grupo se QUEDAN en `mi`. Con un solo grupo que
    cubra todo el material se obtiene el particionado por costado del 58.0.

    Solo se reescriben STRUCT (palabras de triangulo), MATLIST (las copias al
    final) y el chunk de malla 0x50E. El resto de chunks del geometry (p.ej. el
    0x105 del esperant) se conservan byte a byte y en su orden.
    """
    gv, gb = raw_geom
    subs = ia.chunks(gb, 0, len(gb))
    stv, stb = ia.find(subs, 0x01)
    mlv, mlb = ia.find(subs, 0x08)
    if stv is None or mlv is None:
        raise SystemExit("geometry sin STRUCT o MATLIST")
    geo = ia.parse_geometry(stb)
    mlch = ia.chunks(mlb, 0, len(mlb))
    msv, msb = ia.find(mlch, 0x01)
    mats = [(v, b) for t, v, b in mlch if t == 0x07]
    nmat = struct.unpack_from("<i", msb, 0)[0]
    if nmat != len(mats) or mi >= nmat:
        raise SystemExit("matlist incoherente: n=%d mats=%d mi=%d" % (nmat, len(mats), mi))

    tris_mi = [n for n, t in enumerate(geo["tris"]) if t[3] == mi]
    if not tris_mi:
        return None, "ya partido (el slot %d no tiene triangulos)" % mi

    mover = {}
    for k, (etiqueta, tset) in enumerate(groups):
        ns = sorted(tset & set(tris_mi))
        if not ns:
            return None, "nada que partir: el grupo %s no toca el slot %d" % (etiqueta, mi)
        for n in ns:
            mover[n] = nmat + k
    if not mover:
        return None, "nada que partir en el slot %d" % mi
    for k, (etiqueta, tset) in enumerate(groups):
        for n in sorted(tset - set(tris_mi)):
            if geo["tris"][n][3] == mi:
                raise SystemExit("STOP: el grupo %s reclama un triangulo que no es del slot"
                                 % etiqueta)

    newtris = []
    for n, (a, b, c, m) in enumerate(geo["tris"]):
        newtris.append((a, b, c, mover.get(n, m)))

    # STRUCT: prefix y suffix intactos; solo cambian los 16 bits bajos de la
    # segunda palabra de los triangulos partidos. flags intactas.
    newst = ia.build_struct(geo, newtris, clear_tristrip=False)
    # MATLIST: cuenta + n, indices -1, materiales originales intactos y una copia
    # byte a byte del chunk del material por grupo nuevo.
    idxs = struct.unpack_from("<%di" % nmat, msb, 4)
    if any(i != -1 for i in idxs):
        raise SystemExit("STOP: matlist con indices != -1 (material compartido)")
    newml = ia.chunk(0x01, msv, struct.pack("<i", nmat + len(groups)) +
                     struct.pack("<%di" % (nmat + len(groups)),
                                 *([-1] * (nmat + len(groups)))))
    newml += b"".join(ia.chunk(0x07, v, b) for t, v, b in mlch if t == 0x07)
    for k, _g in enumerate(groups):
        newml += ia.chunk(0x07, mats[mi][0], mats[mi][1])

    # Malla: se reagrupa por material; el chunk 0x105 (u otros) del extension se
    # conserva byte a byte. Versiones originales de 0x03 y 0x50E intactas.
    out = []
    replaced_mesh = False
    for t, v, b in subs:
        if t == 0x01:
            out.append(ia.chunk(0x01, stv, newst))
        elif t == 0x08:
            out.append(ia.chunk(0x08, mlv, newml))
        elif t == 0x03:
            inner = ia.chunks(b, 0, len(b))
            nb = b""
            for it, iv, ib in inner:
                if it == 0x50E:
                    nb += ia.chunk(0x50E, iv, ia.build_mesh(newtris)[12:])
                    replaced_mesh = True
                else:
                    nb += ia.chunk(it, iv, ib)
            if not replaced_mesh:
                raise SystemExit("STOP: extension sin malla 0x50E")
            out.append(ia.chunk(0x03, v, nb))
        else:
            out.append(ia.chunk(t, v, b))
    info = {"mi": mi, "tris": len(tris_mi), "mover": len(mover),
            "nmat": (nmat, nmat + len(groups)),
            "grupos": [{"etiqueta": g[0], "slot": nmat + k,
                        "tris": len(sorted(g[1] & set(tris_mi)))}
                       for k, g in enumerate(groups)]}
    return (gv, b"".join(out)), info


def grupos_lado(g, frames, fi, mi):
    """Los dos grupos izquierda/derecha del 58.0 T4 sobre el material mi."""
    w = dg.world_of(frames, fi)
    izq, der, cero = set(), set(), 0
    for n, t in enumerate(g["tris"]):
        if t[3] != mi:
            continue
        cx = sum(dg.mul(w, g["verts"][v])[0] for v in t[:3]) / 3.0
        if cx < 0.0:
            izq.add(n)
        elif cx > 0.0:
            der.add(n)
        else:
            cero += 1
    if cero or not izq or not der:
        raise SystemExit("STOP: material %d con left=%d right=%d x0=%d: no se puede "
                         "partir por costado sin mover geometria"
                         % (mi, len(izq), len(der), cero))
    return [("LF", izq), ("RF", der)]


PULGADA = 0.12


def grupos_cristal(model, geoms, frames, atomics):
    """Triangulos de cristal de cada fila declarada, por geometry y material.

    La lente de una fila son las caras que (a) estan a menos de PULGADA del `xyz`
    declarado de la fila y (b) son cristal por `is_any_lens_face`: la regla del
    proyecto, o matiz de lente con saturacion de lente aunque el valor este bajo.
    Lo segundo es lo que faltaba: el modelador pinta media lentija en un ambar
    oscuro (rumpo y pony: (95,41,14) sat 0,85) y con la regla sola esa mitad se
    quedaba apagada, la linea vertical del jugador. El cromo (sat 0,00-0,04) y los
    bajos (sat 0,22) siguen fuera: el corte por porcentaje de texelas metia alli
    una cara de 1034,5 cm2, que es la forma enorme que alumbra blanco.

    Anclar a la fila declarada es lo que impide partir geometria de lentes que
    el jugador no pidio (el mule trasero tiene caras ambar y no tiene fila). Se
    miran TODOS los materiales del frame, no solo los de la mascara: la mitad
    oscura del cristal del rumpo esta en el material de la pintura. Solo se
    deja un material intacto cuando todo lo que tiene es una unica lente.
    """
    filas = BA.rows({model})
    if model == "fbicar":
        filas = filas + BA.rows({"washing"})
    if not filas:
        return {}
    _b, _fr, _ge, _at, faces = L.measure(model)
    grupos = {}
    for r in filas:
        for f in faces:
            if f["frame"] != r["frame"]:
                continue
            if f["mod"] or "_dam" in f["frame"] or "vlo" in f["frame"]:
                continue
            d = sum((a - b) ** 2 for a, b in zip(f["world"], r["xyz"])) ** 0.5
            if d > PULGADA:
                continue
            if not L.is_any_lens_face(f):
                continue
            fi = next(a[0] for a in atomics if a[1] == f["gi"])
            et = "%s%s" % ("L" if f["world"][0] < 0.0 else "R",
                           "F" if r["node"].endswith("F") else "R")
            grupos.setdefault((fi, f["gi"], f["mi"]), {}).setdefault(et, set()).add(f["ti"])
    out = {}
    for (fi, gi, mi), gs in grupos.items():
        n_mi = sum(1 for t in geoms[gi]["tris"] if t[3] == mi)
        movidos = sum(len(s) for s in gs.values())
        if len(gs) == 1 and movidos >= n_mi:
            continue
        out[(fi, gi, mi)] = sorted(gs.items())
    return out


def patch_dff(buf, model, modo):
    """Aplica los splits del modelo. Devuelve (bytes_nuevos, informes)."""
    frames, geoms, atomics = dg.parse(buf)
    top = ia.chunks(buf, 0, len(buf))
    clump_ver, cb = ia.find(top, 0x10)
    cc = ia.chunks(cb, 0, len(cb))
    if any(t == 0x0F for t, _v, _b in cc):
        raise SystemExit("%s: geometrias sueltas en el clump; no soportado" % model)
    glv, glb = ia.find(cc, 0x1A)
    gch = ia.chunks(glb, 0, len(glb))
    gstruct_ver, gstruct = ia.find(gch, 0x01)
    raw_geoms = [(v, b) for t, v, b in gch if t == 0x0F]
    if len(raw_geoms) != len(geoms):
        raise SystemExit("%s: lista de geometrias dividida" % model)
    if struct.unpack_from("<I", gstruct, 0)[0] != len(raw_geoms):
        raise SystemExit("%s: cuenta de geometrias incoherente" % model)

    if modo == "cristal":
        blancos = grupos_cristal(model, geoms, frames, atomics)
        if not blancos:
            return None, []
        targets = sorted({(fi, mi) for (fi, _gi, mi) in blancos})
    else:
        targets = []
        for fname, mi in TARGETS[model]:
            fis = [f["i"] for f in frames if (f["name"] or "").lower() == fname]
            if len(fis) != 1:
                raise SystemExit("%s: frame %s aparece %d veces" % (model, fname, len(fis)))
            targets.append((fis[0], mi))

    informes = []
    new_geoms = list(raw_geoms)
    base = {}

    def cur(gi):
        """Estado actual del geometry: dos splits en el mismo geometry tienen
        que verse entre ellos, si no el segundo calcula sus slots sobre los
        bytes originales y se los come."""
        if gi not in base:
            _f2, g2, _a2 = dg.parse(_wrap(ia.chunk(0x0F, new_geoms[gi][0],
                                                   new_geoms[gi][1])))
            base[gi] = g2[0]
        return base[gi]

    for fi, mi in targets:
        fname = next((f["name"] or "").lower() for f in frames if f["i"] == fi)
        gis = [gi for (afi, gi) in atomics if afi == fi]
        if len(gis) != 1:
            raise SystemExit("%s: %s tiene %d atomics" % (model, fname, len(gis)))
        gi = gis[0]
        users = [afi for (afi, agi) in atomics if agi == gi]
        if users != [fi]:
            raise SystemExit("%s: geo %d compartido por %s" % (model, gi, users))
        if modo == "cristal":
            groups = blancos[(fi, gi, mi)]
        else:
            groups = grupos_lado(geoms[gi], frames, fi, mi)
        newg, info = split_geometry(new_geoms[gi], frames, fi, mi, groups)
        if newg is None:
            informes.append({"frame": fname, "gi": gi, "skip": info})
            continue
        # verificacion dura antes de escribir: vertices y triangulos identicos
        g0 = cur(gi)
        gg = dg.parse(_wrap(ia.chunk(0x0F, newg[0], newg[1])))[1][0]
        if len(gg["verts"]) != len(g0["verts"]) or gg["verts"] != g0["verts"]:
            raise SystemExit("%s: vertices distintos tras el split" % model)
        old_st = ia.find(ia.chunks(new_geoms[gi][1], 0, len(new_geoms[gi][1])), 0x01)[1]
        new_st = ia.find(ia.chunks(newg[1], 0, len(newg[1])), 0x01)[1]
        po, pn = ia.parse_geometry(old_st), ia.parse_geometry(new_st)
        if pn["prefix"] != po["prefix"] or pn["suffix"] != po["suffix"]:
            raise SystemExit("%s: UVs/normales/prelight distintos (bytes)" % model)
        if len(gg["tris"]) != len(g0["tris"]):
            raise SystemExit("%s: numero de triangulos distinto" % model)
        movidos = {n for _e, s in groups for n in s}
        for n, (t0, t1) in enumerate(zip(g0["tris"], gg["tris"])):
            if t0[:3] != t1[:3]:
                raise SystemExit("%s: indices de triangulo distintos" % model)
            if n in movidos:
                if t0[3] != mi:
                    raise SystemExit("%s: se movio un triangulo de otro slot" % model)
            elif t0[3] != t1[3]:
                raise SystemExit("%s: material de otro slot cambiado" % model)
        for g in info["grupos"]:
            if gg["mats"][g["slot"]] != g0["mats"][mi]:
                raise SystemExit("%s: la copia del slot %d no es identica al original"
                                 % (model, g["slot"]))
        new_geoms[gi] = newg
        base.pop(gi, None)
        info.update({"frame": fname, "gi": gi, "fi": fi,
                     "tex": g0["mats"][mi][1], "color": g0["mats"][mi][0]})
        informes.append(info)

    if not informes or all("skip" in i for i in informes):
        return None, informes
    new_gl = ia.chunk(0x01, gstruct_ver, gstruct) + b"".join(
        ia.chunk(0x0F, v, b) for v, b in new_geoms)
    body = b""
    for t, v, b in cc:
        body += ia.chunk(t, v, new_gl if t == 0x1A else b)
    out = ia.chunk(0x10, clump_ver, body)
    if len(out) % 2048:
        out += b"\0" * (2048 - len(out) % 2048)
    return out, informes


def _wrap(geomchunk):
    """Envuelve un chunk GEOMETRY en un clump minimo para dg.parse."""
    gl = ia.chunk(0x01, 0xC02FFFF, struct.pack("<I", 1)) + geomchunk
    fl = ia.chunk(0x01, 0xC02FFFF, struct.pack("<I", 0))
    at = ia.chunk(0x14, 0xC02FFFF,
                  ia.chunk(0x01, 0xC02FFFF, struct.pack("<4I", 0, 0, 5, 0)))
    return ia.chunk(0x10, 0xC02FFFF,
                    ia.chunk(0x01, 0xC02FFFF, struct.pack("<III", 1, 0, 0)) +
                    ia.chunk(0x0E, 0xC02FFFF, fl) +
                    ia.chunk(0x1A, 0xC02FFFF, gl) + at)


def _raw_structs(buf):
    """Cuerpos STRUCT de cada geometry, en orden de la lista (todo va en 0x1A)."""
    top = ia.chunks(buf, 0, len(buf))
    _cv, cb = ia.find(top, 0x10)
    cc = ia.chunks(cb, 0, len(cb))
    _glv, glb = ia.find(cc, 0x1A)
    out = []
    for t, _v, b in ia.chunks(glb, 0, len(glb)):
        if t == 0x0F:
            _stv, stb = ia.find(ia.chunks(b, 0, len(b)), 0x01)
            out.append(stb)
    return out


def verify_pair(old, new, model):
    """Verificacion completa antes/despues: geometria intacta, solo materiales.

    La igualdad de vertices/UVs/normales/prelight se comprueba BYTE A BYTE sobre
    el prefix/suffix del STRUCT (hay UVs con NaN en alas del fbiranch: comparar
    floats daria falso aunque el dato sea identico).
    """
    f0, g0, a0 = dg.parse(old)
    f1, g1, a1 = dg.parse(new)
    assert f0 == f1 and a0 == a1, "%s: frames/atomics cambiaron" % model
    assert len(g0) == len(g1)
    st0, st1 = _raw_structs(old), _raw_structs(new)
    assert len(st0) == len(g0) and len(st1) == len(g1)
    total_v = total_t = 0
    maxdelta = 0.0
    added = removed = 0
    for gi, (x, y) in enumerate(zip(g0, g1)):
        p0, p1 = ia.parse_geometry(st0[gi]), ia.parse_geometry(st1[gi])
        assert p0["prefix"] == p1["prefix"], "%s geo%d: prefix (verts/UVs/prelight) distinto" % (model, gi)
        assert p0["suffix"] == p1["suffix"], "%s geo%d: suffix (verts/normales) distinto" % (model, gi)
        assert len(x["verts"]) == len(y["verts"]), "%s geo%d: vertices" % (model, gi)
        total_v += len(x["verts"])
        assert len(x["tris"]) == len(y["tris"]), "%s geo%d: triangulos" % (model, gi)
        total_t += len(x["tris"])
        for p, q in zip(x["verts"], y["verts"]):
            for k in range(3):
                if p[k] == p[k] or q[k] == q[k]:   # NaN solo es igual a NaN
                    maxdelta = max(maxdelta, abs(p[k] - q[k]))
        # union de triangulos en mundo: la identidad se comprime por frame del
        # atomic que usa el geometry; clave en bytes para ser inmune a NaN.
        fi = next((afi for (afi, agi) in a0 if agi == gi), None)
        w = dg.world_of(f0, fi) if fi is not None else None
        def keyset(g):
            s = set()
            for t in g["tris"]:
                if w is not None:
                    pts = b"".join(struct.pack("<3f", *dg.mul(w, g["verts"][v]))
                                   for v in t[:3])
                else:
                    pts = b"".join(struct.pack("<3f", *g["verts"][v]) for v in t[:3])
                s.add((t[:3], pts))
            return s
        s0, s1 = keyset(x), keyset(y)
        added += len(s1 - s0)
        removed += len(s0 - s1)
        # los indices de vertice de cada triangulo no cambian, en orden
        assert [t[:3] for t in x["tris"]] == [t[:3] for t in y["tris"]]
    print("  %s: verts=%d (max delta %.1f) tris=%d anadidos=%d quitados=%d" % (
        model, total_v, maxdelta, total_t, added, removed))
    assert maxdelta == 0.0 and added == 0 and removed == 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry", action="store_true")
    ap.add_argument("--mode", choices=["lado", "cristal"], default="lado")
    ap.add_argument("--model", action="append", default=None)
    a = ap.parse_args()
    modelos = a.model or (sorted(CRISTAL_MODELOS) if a.mode == "cristal"
                          else sorted(TARGETS))
    for m in modelos:
        src = os.path.join(STREAMED, m + ".dff")
        buf = open(src, "rb").read()
        out, informes = patch_dff(buf, m, a.mode)
        print("==== %s (%d bytes) modo=%s" % (m, len(buf), a.mode))
        for i in informes:
            if "skip" in i:
                print("  %s geo%d: %s" % (i["frame"], i["gi"], i["skip"]))
                continue
            slots = " ".join("%s=slot%d(%d tris)" % (g["etiqueta"], g["slot"], g["tris"])
                             for g in i["grupos"])
            print("  %-18s geo%d mi=%d tex=%s: %d tris, %d movidos -> %s; mats %d->%d"
                  % (i["frame"], i["gi"], i["mi"], i["tex"], i["tris"], i["mover"],
                     slots, i["nmat"][0], i["nmat"][1]))
        if out is None:
            print("  sin cambios (idempotente)")
            continue
        verify_pair(buf, out, m)
        print("  salida: %d bytes  sha=%s" % (len(out), _sha(out)))
        if a.dry:
            continue
        for tree in TREES:
            target = os.path.join(tree, m + ".dff")
            if not os.path.exists(target):
                continue
            cur = open(target, "rb").read()
            if cur == out:
                print("  = %s (ya al dia)" % target)
                continue
            if cur != buf:
                raise SystemExit("STOP: %s difiere de streamed; trabajo concurrente, "
                                 "no se sobrescribe" % target)
            tmp = target + ".tmp"
            with open(tmp, "wb") as f:
                f.write(out)
            os.replace(tmp, target)
            print("  -> %s" % target)


if __name__ == "__main__":
    main()