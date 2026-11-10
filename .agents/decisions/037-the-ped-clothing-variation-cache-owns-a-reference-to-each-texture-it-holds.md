---
name: 037-the-ped-clothing-variation-cache-owns-a-reference-to-each-texture-it-holds
status: accepted
date: 2026-11-03
domain: rendering
supersedes: []
---

# ADR-037: the ped clothing-variation cache owns a reference to each texture it holds

## Context

The session ended with a crash. The trace stops with a JSERR `memory access out of bounds`
raised inside `Clump::destroy`, reached from `CEntity::DeleteRwObject`
(`src/entities/Entity.cpp:192`) while `ManagePopulation` deleted a civilian ped. The last
gameplay line before the trap was `VEX pedvar model=cop ropa=5 remap=1`: a ped that had
just been handed a clothing variation. The same signature appears on 2026-10-29 (T38) and
2026-11-02 (T98), so it is not a regression from this week.

The ownership chain explains it:

- A ped does not own its geometry. `CreateRwObject` clones the clump and `Atomic::clone`
  shares the geometry with the model prototype, so `matList.materials[0]` is one material
  shared by every live ped of that model.
- `CPed::Render` swaps a clothing variation into that shared material each frame
  (`src/peds/Ped.cpp:5714-5724`). That is the only way a per-ped skin works on a shared
  material.
- librw is refcounted and balanced. `Material::setTexture` releases the old texture and
  refs the new one (`vendor/librw/src/geometry.cpp:1022-1030`); `Material::destroy`
  releases whatever the material points at (`vendor/librw/src/geometry.cpp:1010-1020`).
- A texture created while streaming a TXD starts at one reference
  (`vendor/librw/src/texture.cpp:284`) and `TexDictionary::add` does not take one
  (`vendor/librw/src/texture.cpp:146-152`). `TexDictionary::destroy` walks its list and
  destroys every texture (`vendor/librw/src/texture.cpp:130-138`).

So when a ped TXD is streamed out, which the VRAM eviction of ADR-032 does on demand, every
entry of `textureClothingVariations` and `textureShadeVariations` dangles. The cache took
those pointers raw and never released or invalidated them: `CPedModelInfo::DeleteRwObject`
deleted the collision model but left the arrays full, and `SetClump` only zeroed them when
the dictionary was already loaded, so a reload with a not-yet-loaded TXD left the stale
pointers in place. `CPed::texClothingVariation` copies made the same freed pointer
reachable from the render path, and the next `Material::destroy` called `destroy` on freed
memory. Two owners were assumed for each texture, and neither was ours.

## Decision

Our cache participates in refcounting instead of borrowing. Nothing changes in
`vendor/librw`.

- `storeTextureVariation` calls `addRef` at the moment it stores a pointer, so a variation
  cannot be freed while the cache lists it. A texture kept alive this way still has a valid
  raster and stays renderable; `TexDictionary::destroy` clears its `dict` field first.
- `releaseVariationCache` (`src/modelinfo/PedModelInfo.cpp:21-49`) destroys and nulls both
  arrays. It runs from `CPedModelInfo::DeleteRwObject` (the stream-out hook, also called
  directly by `CStreaming::RemoveModel`) and from `SetClump` before the TXD lookup, so a
  stream-in with no dictionary yields an empty cache rather than a stale one.
- `CPed::DeleteRwObject` (`src/peds/Ped.cpp:644-653`) clears the ped's own copies. A ped
  without an rw object is never rendered and `SetModelIndex` re-derives the variation on
  restore.
- One capped trace, `PEDVARFREE model=... refs=N`, reports each release that drops
  references. It answers what static reading cannot: whether a ped TXD really is evicted
  while peds of that model are alive.

## Consequences

- The dangling-texture path is closed by construction, and the VRAM counter of ADR-033
  stays balanced because each `addRef` has exactly one matching `destroy` in the same
  module.
- Variations now live as long as the model info cache, not as long as the TXD. That is
  bounded by the loaded ped models and released on stream-out, but a kept-alive ped model
  holds its whole remap and shade set. `PEDVARFREE` plus the VRAM telemetry is the check.
- If the crash comes back in a session where `PEDVARFREE` never fired, this was not the
  only cause; the next suspect is the shared `matList` lifetime in `Clump::destroy`
  itself, which needs a vendor change and was deliberately left out of this build.
- `m_pSkinTexture` and the two player-skin assignments in Script8 and Pools still borrow
  without a reference; they stay valid because the player model info now holds refs on the
  same textures. They are the remaining instances of the old pattern.
- Accepted means the decision is taken, not verified in game.
