---
name: 017-script-abi-is-the-mods-2510
status: accepted
date: 2026-10-15
domain: mods
supersedes: []
---

# ADR-017: The engine's script ABI follows the mod's 2510 build

## Context

The `main.scm` the port serves is the mod's 2510 script, bit for bit (1,191,180 bytes,
`41a932cc593e`). A compiled SCM addresses engine tables by number, so the engine must match
the numbers that script was compiled against. The old port does not:

- `eWeaponType`: the port appended the mod weapons at 48-56 with `HEALTH`=37, while the
  mod inserts them after `CAMERA`. When the script says "37" the port reads "health".
- `eRadarSprite`: the script uses sprites 41-47 (`HOUSE_FOR_SALE` ... `RAMPAGE`); the port
  stops at 40, so `CRadar::DrawRadarSprite` reads past `RadarSprites[]`. This is the crash
  on starting a new game.
- The mod's own opcodes, model ID ranges and garage type 33.

## Decision

`eWeaponType`, `eRadarSprite`, the mod's script opcodes, model IDs (`MODELINFOSIZE`, new
weapons from 6660, new vehicles 6500-6599) and garage types follow the mod's **2510**
binary. Where 2510 differs from the v3.0 source, the REA receipt decides
(`gta_vc_browser/tmp/rea/viceex-14-rea-2025.md`). Garage type 33 (tuning shop) is accepted
and stays inert, because tuning is out of scope.

The comment in `WeaponType.h` that defends the appended order ("IDs 0..47 are burned into
`main.scm`") applies to the vanilla script only, and goes away with the old entries.

## Consequences

- The served script runs against matching tables: no out-of-range sprite reads, and the
  weapons it gives or checks are the right ones.
- Saves made with the old port may stop loading once the enums move.
- Every table indexed by these enums (weapon info, pickups, HUD sprites, weapon effects)
  moves in the same step.

## Alternatives Considered

- Keep the port's enum order and remap IDs when the script is read: it hides the mismatch
  in a translation layer that every opcode would have to go through, and it does not match
  the mod's saves.
- Serve the vanilla `main.scm`: it drops the mod's script features (family 13 of the plan).

## State

`accepted` (plan 13.0, batch T1).

<!-- USER-START -->

## Local additions (preserved on update)

<!-- USER-END -->
