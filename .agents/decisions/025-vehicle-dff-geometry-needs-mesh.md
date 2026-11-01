---
name: 025-vehicle-dff-geometry-needs-mesh
status: proposed
date: 2026-10-30
domain: assets
supersedes: []
---

# ADR-025: A rewritten vehicle geometry carries its mesh chunk

## Context

`indicator_audit.py` separates the real orange lens of a vehicle into its own
`indicator_*` node by rewriting geometry chunks in the `.dff`. The first version
wrote each geometry as `0x01 STRUCT + 0x08 MATLIST` and dropped the `0x03`
extension. Two independent facts made that fatal:

1. `Geometry::streamRead` ends in `PluginList::streamRead()`, which calls
   `findChunk(ID_EXTENSION)` and returns `false` when there is none. Without the
   extension the reader scans into the next chunk, the stream desynchronises and
   `CFileLoader::StartLoadClumpFile` fails the whole model (`FAIL model`), so
   the car never appears.
2. The renderer does not draw the struct triangle list: `gl3pipe::instanceMesh`
   builds the index buffer from `geo->meshHeader`, read from the `0x50E` mesh
   inside that extension. Removing faces from the struct alone changes nothing
   on screen, and a new node without a mesh cannot draw only the lens.

## Decision

Any tool that rewrites or creates a geometry in a vehicle `.dff` writes it as
`STRUCT + MATLIST + 0x03`, and the extension contains a `0x50E` mesh rebuilt
from that geometry's own triangle list:

- mesh flags `0` (triangle list), one entry per material with faces,
  `totalIndices` equal to `3 * numTriangles` and to the sum of the entries;
- empty materials are omitted (`instanceMesh` asserts a non-empty min vertex);
- the `TRISTRIP` bit of the geometry struct is cleared, because the written
  mesh is a list, not a strip;
- a geometry that keeps its original mesh is left untouched.

The verifier accepts a `.dff` only when every geometry is `[0x01, 0x08, 0x03]`,
each material keeps its own extension, the struct measures exactly what
`parse_geometry()` computes and the mesh chunk covers
`12 + 8*numMeshes + 4*totalIndices` bytes.

## Consequences

- Patched vehicle models load again and the separated lens actually leaves the
  body surface: the split is visible, not just consistent on paper.
- Every future edit to a geometry's triangle list must also rewrite its mesh,
  or the change will not render (and, without the extension, will not load).
- The mesh is the authoritative surface for these models; the struct triangle
  list stays as the source used to regenerate it.

## Alternatives Considered

- **Keep only the struct triangle list**: rejected — the model fails to load
  and, even if it loaded, the renderer would ignore the edit.
- **Copy the source geometry's mesh into the new node**: rejected — the node
  would draw the whole body surface a second time instead of just the lens.
- **Rebuild the mesh at runtime after loading**: rejected for data-authored
  splits — the `.dff` must be self-contained for the on-demand loader and for
  any other tool that reads it.

## State

`proposed` (found while repairing the 28.0 splits during plan 29.1; the player
confirms it).
