---
name: 020-mod-weapons-and-second-sound-bank-are-the-mods-own-code
status: accepted
date: 2026-10-15
domain: weapons-audio
supersedes: []
---

# ADR-020: The new weapons and their second sound bank are the mod's own code; on the web the bank is served as on-demand samples

## Context

The old port gave the eight Vice Extended weapons (AK-47, M16, Shotgun2, Micro-UZI, Desert
Eagle, Beretta, Steyr AUG and the 2510 grenade launcher) behaviour by guessing: unguarded
`case` lines "like its vanilla equivalent" in `Weapon.cpp`, `PedFight.cpp`, `Vehicle.cpp`,
`SpecialFX.cpp` and `PlayerPed.cpp`; a grenade launcher reconstructed without the binary; a
"shoot a projectile and it explodes" path of its own (`VICEEXT_PROJECTILE_BLOW`); and sounds
chosen by ear (`SFX_VICEEX_00..12`, `viceex-map.tsv`), with a per-sample reservation in the
web decoder (`ReserveSample`/`OdSfxReserve`) and test traces (`WINFO`, `WLOAD`,
`VICEEX sfx arma`, `P4 kind=uzi`).

In the mod (`x87/gta-extended` 44f4ab3) each weapon is a set of `EX_WEAPON_*` blocks in the
same engine functions, the projectile-explodes feature is an `IMPROVED_TECH_PART` block in the
three bullet paths, and the new sounds live in a second sample bank
(`ViceExtended/AUDIO/ViceEx.SDT` + `ViceEx.RAW`) loaded only by the Miles sample manager. The
grenade launcher, its sounds and the Micro-UZI crash fix exist only in the 2510 binary.

The web port never loads `sfx.RAW`: every effect is served as `Audio/sfx/<id>.mp3`, decoded on
demand, with its size and rate taken from the served `Audio/sfx.SDT`. MP3 cannot carry the
bank's 36000/33000/22000 Hz, so the served table was already extended (data tag `ve13`) with
the thirteen ViceEx entries at the REAL rate and size of their MP3s (ids 9941..9953).

## Decision

1. The weapons, the cops' random weapons and the projectile-explodes feature are the mod's
   code under `VICEEXT_WEAPON_*`, `VICEEXT_NEW_WEAPONS` and `VICEEXT_IMPROVED_TECH_PART` (plan
   13.0, T5). The grenade launcher, the 2510 sounds and the Micro-UZI fix follow the 2510
   binary (REA receipt `gta_vc_browser/tmp/rea/viceex-14-T5-armas.md`).
2. The second sample bank is ported to `sampman_oal.cpp` under `VICEEXT_SECOND_SAMPLE_BANK`
   with the mod's numbering: the new `eSfxSample` entries start right after
   `SFX_PLAYER_ON_FIRE_16` (`NEW_SAMPLEBANK_START`).
   - **Desktop (OpenAL):** as the mod does with Miles, the main `sfx.SDT` is read up to
     `NEW_SAMPLEBANK_START`, then `ViceExtended/audio/ViceEx.SDT` fills the new entries and
     `ViceEx.RAW` is loaded into its own buffer with bank 0. A missing bank only silences the
     new sounds (the mod refused to start the audio).
   - **Web:** the bank is served the way every other effect is: the new entries are read from
     the served `Audio/sfx.SDT` (9954 entries, real MP3 rate) and their PCM comes from
     `Audio/sfx/9941..9953.mp3`. `ViceEx.RAW` is not served.
   - `sampman_null` needs nothing beyond the larger enum.
3. The old per-weapon code, `VICEEXT_PROJECTILE_BLOW`, the `SFX_VICEEX_*` names, the sample
   reservation and the test traces are removed in the same batch.

## Consequences

- The served `Audio/sfx.SDT` and the thirteen MP3s are a data contract: rebuilding them from a
  different `ViceEx.RAW` requires `tools/add_viceex_sfx.py` (which writes the real rate) and a
  new `dataTag`.
- New weapon sounds go through the same on-demand decode budget as vanilla gunshots; there is
  no reservation any more.
- The projectile-explodes check lives in one helper (`CProjectileInfo::BlowUpIfThisIsAProjectile`)
  called from the three bullet paths instead of the mod's three copies, and it skips slots
  that are no longer in use (the mod could match a freed slot).
