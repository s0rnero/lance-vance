#!/usr/bin/env python3
"""Censo de logica por frame presentado (plan 49.0, ADR-050).

El motor corre UN tick por frame presentado (`Idle()` -> `CTimer::Update()`,
`src/core/main.cpp:2005`). Lo que acumule o cuente "por frame" sin escalar cambia
su duracion en tiempo real con los FPS: a 120 Hz son 4 ticks donde antes habia 1.
ADR-050 fija las tres formas canonica de convertirlo: `GetTimeStepFix()` para el
incremento afinado por frame, tick logico con `GetLogicalFrameCounter()` /
`GetLogicalFramesPassed()` para ventanas y presupuestos, y
`GetTimeInMillisecondsNonClipped()` para relojes que no pueden perder tiempo.

Este script NO decide: TRIA. Mira solo dentro de funciones que corren por frame
(Update/Process/Render/Do/Think/Manage/Service/Tick/Apply/Collide/Shift...),
descarta lo ya normalizado y lo que no es acumulacion (envolturas de angulo,
indices de bucle, lineas de traza), y ordena por severidad. Lo que vive fuera de
esas funciones no sale aqui: por eso el censo se repasa a mano despues del filtro.

Uso:
  python gta_vc_browser/tools/census_fps.py                # resumen + top 40
  python gta_vc_browser/tools/census_fps.py --sites 200    # top N
  python gta_vc_browser/tools/census_fps.py --solo-alta    # solo ALTA
  python gta_vc_browser/tools/census_fps.py --all          # todos
"""
import argparse
import bisect
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

# ya normalizado (categoria a de ADR-050) -> no es candidato
NORM = re.compile(
    r"CTimer::GetTimeStep|GetTimeStepFix|GetLogicalFrame|GetTimeInMilliseconds|"
    r"ms_fTimeStep|GetTimerSpeed|ChancePerTick"
)
# tampoco es acumulacion de tiempo: envolturas de angulo, indices, desplazamientos.
# `+= TWOPI` / `-= 2*PI` son NORMALIZACION de angulo, no acumulacion por frame.
NOISE = re.compile(
    r"[-+]=\s*(TWOPI|HALFPI|PI|2(\.0)?f?\s*\*\s*PI|PI\s*\*\s*2)|"
    r"while\s*\(|for\s*\(|do\s*\{|"
    r"[\w.]*(?:Offset|offset|Byte|byte|Index|index|Size|size|Pad|pad)\w*\s*(?:[-+]=|\+\+|--)|"
    r"^\s*(?:i|j|k|n|it|iter)\s*(?:[-+]=|\+\+|--)"
)
# solo cuenta el incremento CONTRA UNA LITERAL: `x += y` es integracion legitima
ACC = re.compile(r"([A-Za-z_][\w.]*(?:->[\w.]+)?(?:\[[\w.]+\])?)\s*([+-]=)\s*(\d+\.?\d*f?|0\.\d+f?)\s*;")
INC = re.compile(r"([A-Za-z_][\w.]*(?:->[\w.]+)?)\s*(\+\+|--)\s*;")
DAMP = re.compile(r"([A-Za-z_][\w.]*(?:->[\w.]+)?)\s*\*=\s*(0\.\d+f?|1\.0f?)\s*;")
WINDOW = re.compile(
    r"([A-Za-z_][\w.]*(?:->[\w.]+)?(?:[Tt]imer|[Cc]ounter|[Ff]rames?|[Tt]icks?|[Cc]ount))"
    r"\s*([<>]=?|==)\s*(\d+)"
)
PHYS = re.compile(
    r"MoveSpeed|TurnSpeed|AngVel|MoveForce|TurnForce|Rotation|Rotor|WheelSpeed|WheelRot|"
    r"Steering|Beta|Alpha|Fov|Shift|Impulse|Speed|Angle|Force|Spin|Sway|Position",
    re.I,
)
TIMERISH = re.compile(r"Timer|Counter|Ticks|Frames|Count", re.I)
TRACE = re.compile(r"ODTRACE|odtrace|__odq|LOGV|LOGH|snprintf|printf|debug\(|%[sdf]", re.I)
SKIP_LINE = re.compile(r"^\s*(//|/\*|\*)|\bcase\b|^\s*default\s*:|^\s*\}")
# ficheros que no son camino de frame o se auditan aparte
SKIP_FILES = re.compile(
    r"3rdParty|purevm|/oal/|sampman_|Credit|LoadInfo|MemManager|Pools\.cpp|"
    r"General\.cpp|Col(?:Model|Store|Box|Line|Sphere|Poly)|Raycast|AnimBlend|"
    r"Visibility|CutsceneData|RunningScript"
)
FN_HEAD = re.compile(r"^([A-Za-z_][\w:~<>]*::[\w~<>]+)\s*\(")
TYPE_ONLY_LINE = re.compile(
    r"^(?:static\s+)?(?:inline\s+)?(?:void|int\d*_t|uint\d*_t|float|bool|char|double|[CcRwI]\w+)\s*$"
)
FRAME_FN = re.compile(
    r"Update|Process|Render|Draw|\bDo[A-Z]|Think|Manage|Service|Tick|Step|Collide|Shift|"
    r"Apply|Control|Handle|PreRender|Post"
)


def function_heads(lines):
    """Cabeceras de funcion (linea, nombre). Estilo reVC: empiezan en columna 0 y
    a veces el tipo vive solo en la linea de arriba."""
    heads = []
    for i, ln in enumerate(lines):
        if ln[:1] in ("", " ", "\t", "}"):
            continue
        m = FN_HEAD.match(ln)
        if m:
            heads.append((i, m.group(1)))
        elif i > 0 and TYPE_ONLY_LINE.match(lines[i - 1].strip()):
            heads.append((i, ln.split("(")[0].strip()))
    return heads


def classify(lines, i):
    """Devuelve (severidad, forma, texto) o None."""
    raw = lines[i].rstrip()
    if SKIP_LINE.match(raw) or TRACE.search(raw) or NOISE.search(raw):
        return None
    ctx = raw + " " + " ".join(lines[i + 1: i + 3])
    if NORM.search(ctx):
        return None

    m = ACC.search(raw)
    if m:
        var, kind = m.group(1), "acumulador"
    else:
        m = DAMP.search(raw)
        if m:
            var, kind = m.group(1), "amortiguacion"
        else:
            m = INC.search(raw)
            if m:
                var, kind = m.group(1), "contador"
            else:
                w = WINDOW.search(raw)
                if not w:
                    return None
                return ("MEDIA",
                        f"ventana-en-frames {w.group(1)} {w.group(2)} {w.group(3)}",
                        raw.strip())

    if PHYS.search(var):
        sev = "ALTA"
    elif TIMERISH.search(var):
        sev = "MEDIA"
    else:
        sev = "BAJA"
    return sev, f"{kind} {var}", raw.strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sites", type=int, default=40)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--solo-alta", action="store_true")
    a = ap.parse_args()

    rows = []
    nfiles = nlines = 0
    for f in sorted(SRC.rglob("*.cpp")):
        rel = str(f.relative_to(ROOT)).replace("\\", "/")
        if SKIP_FILES.search(rel):
            continue
        try:
            lines = f.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue
        nfiles += 1
        nlines += len(lines)
        heads = function_heads(lines)
        starts = [h[0] for h in heads]
        names = [h[1] for h in heads]
        for i in range(len(lines)):
            k = bisect.bisect_right(starts, i) - 1
            if k < 0 or not FRAME_FN.search(names[k]):
                continue
            c = classify(lines, i)
            if not c:
                continue
            sev, kind, txt = c
            rows.append((sev, rel, i + 1, names[k], kind, txt))

    order = {"ALTA": 0, "MEDIA": 1, "BAJA": 2}
    rows.sort(key=lambda r: (order[r[0]], r[1], r[2]))
    na = sum(1 for r in rows if r[0] == "ALTA")
    nm = sum(1 for r in rows if r[0] == "MEDIA")
    nb = sum(1 for r in rows if r[0] == "BAJA")
    print(f"ficheros leidos {nfiles}, lineas {nlines}")
    print(f"candidatos en funciones de frame: {len(rows)}  (ALTA {na}, MEDIA {nm}, BAJA {nb})")
    by = {}
    for r in rows:
        parts = r[1].split("/")
        d = parts[1] if len(parts) > 2 else parts[-1]
        by.setdefault(d, [0, 0, 0])[order[r[0]]] += 1
    for d in sorted(by, key=lambda k: (-by[k][0], -by[k][1], k)):
        print(f"   {d:<14} ALTA={by[d][0]:<4} MEDIA={by[d][1]:<4} BAJA={by[d][2]}")
    show = rows if a.all else (rows[:na] if a.solo_alta else rows[: a.sites])
    print()
    for sev, rel, ln, fn, kind, txt in show:
        if a.all or a.solo_alta or sev != "BAJA":
            print(f"{sev:<5} {rel}:{ln}  [{fn}]  {kind}  |  {txt[:92]}")


if __name__ == "__main__":
    main()
