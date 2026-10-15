---
number: 015
title: An IPL `inst` line with id -1 resolves its model by name
status: accepted
date: 2026-10-14
deciders: Buffy (agent), for the Vice Extended port
related: 001, 003
---

# ADR-015: An IPL `inst` line with id -1 resolves its model by name

## Context

The mod ships a piece of world of its own (`modloader/ViceExtended/maps/bryx/*`), and the
port serves it bit for bit (family 7 of plan 12.0). Its two `inst` lines start with `-1`
in the id column:

```
inst
-1, bryx_lights, 0, 568.5395508, -4.800708771, 13.2636261, 1, 1, 1, 0, 0, 0, 1
-1, lodbryx_lights, 0, 568.5395508, -4.800708771, 13.2636261, 1, 1, 1, 0, 0, 0, 1
end
```

`-1` is not a model index: the name in the second column is there precisely so the engine
can find the model by it, and that is how the mod's own engine places this piece. Measured
over the whole served world: of 9135 `inst` lines, exactly those two carry a negative id,
so the vanilla data never exercises the path.

The port's loader took the column as an array index:

```c
mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(id);   // ms_modelInfoPtrs[id]
```

and `CModelInfo::GetModelInfo(int)` does not check the range. With `-1` it reads the
pointer word that sits immediately before `ms_modelInfoPtrs`; when that word is not nil it
calls into a "model" whose vtable is whatever those bytes describe. Measured 2026-10-14
with the mod's world served: the browser build aborts the startup with
`RuntimeError: null function at CFileLoader::LoadObjectInstance`. The abort moved with the
code layout only — an earlier build of the same sources with the same data silently
dropped the two lines instead (the neighbouring word happened to be nil, so the loader
returned early).

## Decision

`CFileLoader::LoadObjectInstance` resolves the model by name when the id column is
negative, using the loader's existing `CModelInfo::GetModelInfo(const char*, int*)`, and
gives up (no object) when that name is not in the model list:

```c
if (id < 0) {
    int idPorNombre = -1;
    if (CModelInfo::GetModelInfo(name, &idPorNombre) == nil)
        return;
    id = idPorNombre;
}
```

The name lookup belongs to the engine's loader (ADR-001), not to the mod loader, and it
follows the mod's file rather than the engine's previous behaviour (ADR-003). The static
`tools/viceext-world-check.py` already validates these references **by name**, so the
checker and the engine now agree on what the line means.

## Consequences

- The mod's `bryx.ipl` piece loads: both lines resolve to the simple models its IDE
  declares, so the level load completes and the loader's `assert(mi->IsSimple())` holds
  (asserts are on in the `RelWithDebInfo` browser build, and the startup now reaches
  `initstep 18`).
- It is inert for vanilla data: no served `inst` line other than those two uses a negative
  id, so no other model placement changes.
- An `inst` line whose name resolves to nothing is skipped instead of aborting — the same
  "the world is described but not served" class of failure `viceext-world-check.py`
  reports.
- Positive ids outside `MODELINFOSIZE` are still unchecked. Making
  `CModelInfo::GetModelInfo(int)` itself range-safe is a wider change and is **not** part of
  this decision.
