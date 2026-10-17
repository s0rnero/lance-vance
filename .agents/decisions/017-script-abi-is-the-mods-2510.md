---
name: 017-script-abi-is-the-mods-2510
status: accepted
date: 2026-10-15
domain: mods
---

# ADR-017: The script ABI is the mod's 2510

## Context

The served `data/main.scm` is the mod's script bit for bit (1.191.180 B,
`41a932cc593e`), and it stays served by the player's decision. A script only
works if the engine honours the numeric contracts it was written against.
The 2510 contracts differ from both vanilla and this port's current values:
the port keeps the new weapons at the end of `eWeaponType` (48-56, HEALTH=37),
while the mod inserts them behind `CAMERA` (AK47=37 ... STEYR=43); and the
port's `eRadarSprite` ends at 40 while the script uses sprites 41-47, which
is the current new-game crash in `CRadar::DrawRadarSprite` (tanda `ve85e`).

## Decision

`eWeaponType`, `eRadarSprite`, the custom opcodes, the model IDs and the
garage types follow the mod's 2510:

- `eWeaponType` in 2510 order (with the mod's names, e.g.
  `WEAPONTYPE_DEAGLE`), with the grenade launcher included per the
  "REA 2025" receipts.
- `eRadarSprite` with the 7 script entries (`HOUSE_FOR_SALE`, `BOMB_SHOP`,
  `RACE`, `STORE`, `PACKAGE`, `STUNT`, `RAMPAGE`); `WAYPOINT`=40, as in both
  builds with `MAP_ENHANCEMENTS`.
- Opcodes `0FA0-0FA9` per the 2510 receipts (including `0FA8`/`0FA9`), with
  `UpdateCompareFlag` setting the `IF` result.
- Weapon models at 6660-6667, cars at 6500-6599.
- Garage type 33 (`GARAGE_MOD`) accepted and inert (tuning is excluded, but
  the engine must tolerate the type).

## Consequences

- The mod's 206 opcode call sites consume their parameters without
  desynchronising the stack, and its blip/weapon references resolve.
- Saves written by the old port may stop loading from T1 on (the weapon enum
  moves); T10 attempts real mod saves.
- The comment in `WeaponType.h` defending the trailing order only held for
  the vanilla script, not the served one.

## Alternatives Considered

- **Keep the trailing weapon order and remap in the script loader**:
  rejected, it would fork the served script from the mod's bit for bit.
- **Extend the radar sprite array without the enum**: rejected, same class of
  out-of-range read one row further down.

## State

`accepted` (2026-10-15, with plan `13.0-reimplementacion-limpia-extended`, T1
implements it).
