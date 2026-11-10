---
name: 021-transplant-equivalence-check
status: proposed
date: 2026-10-23
domain: mods
supersedes: []
---

# ADR-021: A transplant is validated block by block before it counts as done

## Context

ADR-016 orders each Vice Extended feature to be transplanted from the mod's own source
(or measured with REA when 2506/2510 differs), removing the old code of that feature in
the same step. Its evidence is one trace per family and one marker in
`check-served-build.sh`, and nothing else.

That evidence cannot tell a transplant from a look-alike, in either direction:

1. The vehicle siren block is a **hybrid** no check flags: vanilla positions kept, two
   coronas when the `.dff` carries `servicelights_*`, a port-invented radius/range of
   `0.55/60`, no rear pair on the ambulance and no camera-angle gating, while the mod's
   block has all three.
2. The save menu **invented** `SAVESLOT_9`, moved the autosave row to the top, deleted the
   `FEM_SL8` row, hardcoded its label and left `Slots[]` indexed by row position, so every
   row operates on the wrong slot. No trace and no marker caught it.
3. Conversely, on 23/10 `CPlayerPed::ProcessCrouch` was accused of being a reduced copy of
   the mod. A line-by-line comparison proved it is faithful (every guard, `PED_ROLL`,
   `CROUCH_BACKWARD`, the `padUpDown` condition and the aim branches). The gate also
   protects good code from being rewritten for no reason.

Traces measure what their author remembered to trace (ADR-006) and markers prove the wasm
carries code, not that the code is the mod's. Neither answers the question the method
actually needs answered: does this function match its source block.

## Decision

Before a transplanted function counts as done, it is compared **block by block** against
the mod's source block or the REA receipt:

1. Every row of the comparison is one construct of the mod's block: a condition, an early
   return, a constant, a clip or animation id, an enum id, a call, a write to a field.
2. Each row is classified as exactly one of:
   - `present`: the same construct exists in the port's block;
   - `translated`: it exists in the port's equivalent form (Gamepad API, GLSL, OpenAL,
     glfw/emscripten), with the equivalent named;
   - `excluded`: it is deliberately left out, only when an ADR, the plan or the player
     names the exclusion and its reason;
   - `gap`: it is missing and closes in a later batch.
3. A `gap` row is written in the plan with its closing batch and re-checked at every batch
   closure. A gap whose batch landed is either closed or re-declared with a new reason.
4. A port-invented value that the mod does not have is a row too, classified `excluded`
   with its reason: it must be a decision, never a left-over.
5. The table lives in the plan's Closure, or in `gta_vc_browser/tmp/rea/` when it comes
   from a receipt, and the closure cites it.
6. A feature with open `gap` rows is not "transplanted". Its DoD checkbox stays unchecked
   and the plan reports it as an accepted pending item.

## Consequences

- A half-transplant is caught the day it enters, not by a player session weeks later, and
  a faithful one is not "fixed" twice.
- Cost: minutes of bookkeeping per function, plus an honest `gap` list that grows before it
  shrinks. That list is the project's real transplant backlog.
- The mod's source stays the specification and the binary keeps winning ties (ADR-016).

## Alternatives Considered

- **Trust the traces and the served-build markers (the ADR-016 status quo)**: rejected —
  it is exactly what missed both the hybrid siren block and the invented save slot.
- **Compare by line count or textual diff**: rejected — the port legitimately rewrites
  Windows/D3D9/XInput/Miles pieces, so counts differ without anything being missing.
- **Enforce it only in review, without a written table**: rejected — the previous reviews
  approved both the hybrid block and the invented slot; the table is what makes the
  divergence visible.

## State

`proposed` (raised by the player on 23/10/2026 during plan 17.0; becomes `accepted` when
the player confirms it).

<!-- USER-START -->

## Local additions (preserved on update)

### Indicator flash route (plan 42.1, 03/11/2026)

Compared against the mod's own two blocks: the material block of
`CAutomobile::RenderLights` (`tmp/upstream-gta-extended/src/vehicles/Automobile.cpp:3877-3967`)
and the corona block of `CAutomobile::DoVehicleLights` (`:8353-8475`). Port side:
`src/vehicles/Automobile.cpp` `ViceExtIndicatorAtomicCB` / `ViceExtSetIndicatorMaterial`
(:2449-2481), `ViceExtIndicatorCorona` (:2396) and the 8-node pass (:3814-3873).

| mod construct | class | port |
|---|---|---|
| amber `material->color` (255,225,0)/(150,125,0) on every `CAR_INDICATOR_*` node | `excluded` (scoped) | same two colours, but only when `material->texture == nil \|\| name == "lights"`. The mod paints amber over whatever lens the node holds; measured face-by-face, most real car lenses are white (front) or red (rear), so the project rule (plan 42.1 mother rule, memory `project-indicator-doctrine`) is amber only on a lens measured amber. Reason: invented amber was defect #1 and #6 of the 03/11 session. |
| `ambient = ambientOn/ambientOff` (10.0/1.0, mod :3766-3767) on `materials[0]` of one atomic | `translated` | same 10.0/1.0, but every atomic and every material of the node (`ViceExtIndicatorAtomicCB` walks `RwFrameForAllObjects` + `matList`): the mod's data is one atomic/one material per node; served vanilla cars split one lens over both, which left half the lens dark. |
| corona block gated by `mi->bNewLights` only — **not** by `bLightsOn` (it sits after the `if/else if` chain closes at :8351) | `present` | the pass is also outside the `bLightsOn` gate (:3814, function scope). An earlier note in this plan called that a deviation; the mod's own block shows it is equivalence, so it is not a local addition. |
| anchor `mi->m_positions[CAR_POS_INDICATORS_*]` and `lightL -= GetRight()*2*pos.x` | `excluded` | per-side geometric anchor measured from the node's own lens faces (`ViceExtIndicatorAnchor`, baricentres of the faces whose texture is not `*body*`). Reason: only 22 of the 3473 served DFF carry the indicator position tracks, so on a vanilla model the mod's anchor is (0,0,0) and the flash appeared in the middle of the bonnet — defect #6 (`hermes`, `glendale`). |
| corona colour `(255,127,0)` hardcoded | `excluded` | colour taken from `IndicatorLensTable.h` (306 rows, 84 models, each row carrying the measured `amber` verdict, generated offline by `gta_vc_browser/tools/gen_indicator_lens.py` over the served tree), normalised so the brightest channel reaches 255, with the mod's `(255,127,0)` kept only for the generic `lights` lens. Reason: material colour is clamped by the shader, so the only way to make the flash survive daylight (defect #1) without inventing a hue is additive corona at full brightness in the measured hue. |
| corona `size 0.2f` | `excluded` | `Clamp(spread*2.5, 0.26, 0.55)` where `spread` is the measured half-extent of the lens. Reason: a fixed 0.2 was a dot on a Moonbeam cluster and a halo on a one-face island. |
| corona ids `(uintptr)this + 20/32/34/35/25/23/36/37` | `excluded` | `+40..+47`. Reason: this port uses +20/+22/+25/+27/+29/+31 for headlight/taillight shadows and corona slots, and the mod's indicator ids land on top of them; +40..47 verified free for every stride the corona pool uses. |
| `GetFrameLightStatus(node) == LIGHT_STATUS_OK` per node | `translated` | `ViceExtLampAlive(this, VEHLIGHT_FRONT_LEFT/RIGHT, REAR_LEFT/RIGHT)` = the car-level rule plus the per-side `Damage.GetLightStatus`. Reason: defect #3 — a wrecked car kept lit lenses. |
| camera-angle gating: `DotProduct(lightR - cam, GetForward()) < 0` front, `> 0` rear, `GetRight()` for the `2_` pairs; the off branch calls `UpdateCoronaCoors` | `excluded` | no angle gate: a lit lens registers from any camera position; the off branch keeps `UpdateCoronaCoors`. Reason: the mod's gate hides the front flash whenever the player looks at the car from behind, which reads as "the indicator does nothing" (defect #2/#4). Cost, measured rather than described (`lamp46r.py --mult 2.5`, 48 lenses in 14 models, `C:\tmp\iav42\lamp46r.final.txt`): with the shipped `nearDist` of 1.5 m the disc lays **12 % / 35 % / 45 %** of its pixels (front / 55° side / 75° side) on geometry more than 10 cm in front of the lens, i.e. on the bonnet, which is the player's C2 of 06/11 ("la luz no debería atravesar el material del coche"). Plan 46.0 T4 sets `nearDist = 0.10 m` for the indicator coronas and that column becomes **0.0 % in all three views**, while the disc stays where the chassis really hides it (13 % / 34 % / 46 %). What is still visible through the body of the car it belongs to is only the additive disc, and that is the cost of dropping the gate, not of the size. |
| `someAngle = Abs(Acos(behindness))` passed to both corona calls | `translated` | `0.0f`: the argument only feeds the streak direction and both the mod and the port register these coronas with `STREAK_OFF`, so the value is inert. |
| `drawDist = 50.0f*TheCamera.LODDistMultiplier`, `TYPE_NORMAL`, `FLARE_NONE`, `REFLECTION_ON`, `LOSCHECK_OFF` | `present` | identical. |
| point/bright lights for the player car (`this == FindPlayerVehicle() && !alarmOff`) | `excluded` | extra `&& odLampGlare` (:3735) so the mid-cycle of a blink does not sweep the road with a directional point light. Reason: without it every indicator beat re-lit the asphalt.

### Lamp damage and bike flashes (plan 42.1 T110, 03/11/2026)

Compared against `tmp/upstream-gta-extended/src/vehicles/Automobile.cpp:5596-5745`
(`CAutomobile::VehicleDamage`) and `Bike.cpp` (which has no indicator handling).

| mod construct | class | port |
|---|---|---|
| bumper/taillight breakage gated by `modelInfo->newLightsData.bBumperHeadlights`, `bBumperForwardIndicators1/2`, `bBumperTaillights`, `bBumperRearIndicators1/2`, `bBumperBrakelights`, `bBumperReversinglights` (`:5606-5620`) and by `bWingHeadlights`/`bWingIndicators1/2` on a wing hit (`:5740-5745`) | `present` (restored) | the flags are already parsed in this port (`src/modelinfo/VehicleModelInfo.cpp:980-1012`, derived from the lamp node's parent dummy in the DFF), and `VehicleDamage` now consults them before breaking anything. The port had dropped the gate and broke both front lamps on every bumper hit, which is the player's "damage is way too exaggerated"; measured on the served tree (`parents_t110.py`, 3475 DFF): only 6 models carry a headlight under `bump_front_dummy`, 5 a taillight under `bump_rear_dummy`, 14 have lamps under `wing_lf_dummy`, and 370 of the lamp nodes hang on `chassis_dummy`/`chassis_hi` — so for most cars a bumper bang now breaks nothing. |
| 14 independent per-node lamp statuses (`SetFrameLightStatus(CAR_HEADLIGHT_L/R, INDICATOR_LF/RF/2_LF/2_RF, TAILLIGHT_*, BRAKELIGHT_*, REVERSINGLIGHT_*)`) | `translated` | the engine has four statuses per car (`eLights`: front/rear x left/right), so a bumper or bonnet event sets the two statuses of that end. Consequence accepted: on a model whose bumper holds only an indicator, breaking that side's indicator also drops that side's headlight status, because the engine cannot tell them apart. Reason for not adding 10 more statuses: nothing in the renderer consumes per-node lamp status, so it would be state with no observable effect. |
| bonnet and boot roll one die **per lamp** (`:5634-5639`, `:5680-5688`, twelve separate `GetRandomNumberInRange(0.0f, 600.0f) > comparedValue` tests) | `translated` | one roll per status of the end (`ViceExtBreakLamp(this, VEHLIGHT_FRONT_LEFT)` and `..._RIGHT` as two independent tests). The port previously rolled once for both, which broke the pair together far more often than the mod does. |
| bikes: no indicator code path at all in the mod's `Bike.cpp` | `local addition` | `ViceExtIndicatorCoronas` runs for `CBike` too (4 slots, `BIKE_INDICATOR_*` frames keyed onto `CAR_INDICATOR_*` table nodes, corona ids `+40..+43`, `damage = nil`). Reason: the measured fleet has real amber lenses on bikes (`faggio` rear x2, 6 faces each) and the player's rule is that every turn signal must show its own colour. Deliberate scope limit: bikes do **not** get the Banshee own-lamp blink route (the mod has none, and `m_bIndicatorState` does not drive bike materials) and no per-lamp damage, because `CDamageManager` is a `CAutomobile` member (`src/vehicles/Automobile.h:29`) and `CVehicle` has none. Bike coronas use `GetMatrix()` while the body leans, so the flash is a few centimetres off the lens at full lean. |
| amber decision made in engine code | `excluded` | the amber verdict is data: `gen_indicator_lens.py` writes an `amber` column per measured row (338 rows: 214 amber, 124 not) and `ViceExtIndicatorLensKind` only reads it. A node with `amber=0` registers **no** corona of its own — it blinks the headlight/taillight corona the engine already had (the Banshee route), which is what the player asked for ("do not create a new red or white flash"). Since plan 44.0 N5 a cell **without** a row is `ODIND_OWN` too: the two engine-side guesses that used to call a lens amber from its texture name (`tex == "lights"`) or paint the flash `{255,127,0}` are gone, so every amber on screen is backed by a measured row. |
| `RwFrameGetRoot` | `translated` | declared at `src/fakerw/rwcore.h:314` but commented out in `fake.cpp:105`, so the anchor walks `RwFrameGetParent` to the root and measures `Invert(LTM(root)) * LTM(atomic)`. Not a mod construct: it is what makes the anchor independent of the entity matrix, which was the cause of the one-sided flashes. |

### Banshee route re-verified on the shipped binary (plan 42.1 T114, 04/11/2026)

Target: `C:\Users\s0rno\Desktop\Gamez\(g)old\Grand Theft Auto Vice City\ViceEx.exe`
— PE x86, imagebase `0x400000`, sha256
`4df2726e03654899e1f77a14acb3175f03c04ab4c4b36f0693eec086fc74c48b`, 7085 procedures,
assert strings that pin it to `D:\Modding\GTA\VC\gta-extended\src\...`, so it is the
mod's own source compiled. Read with `rea-agents@6.4.0` + Ghidra:
`CAutomobile::DoVehicleLights` = `FUN_00612670`, the lamp region of
`CAutomobile::PreRender`/`Render` at `0x62cd60`, `GetFrameLightStatus` `0x618c50`,
`RegisterCorona` `0x5b3e50`, `UpdateCoronaCoors` `0x5b5900`, per-lamp liveness
`0x64cf30`, `RpAtomicGetFlags` `0x546f80`, `GetFirstObject` `0x601370`. Pseudocode
kept at `C:\tmp\iav42\lights.c` (1617 lines) and `C:\tmp\iav42\render.c`.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| `mi->bNewLights` is a standalone bool at `CVehicleModelInfo+0x4cc` and the ini byte `bStandardCarsUseTurnSignals` (`0xa061e3`) is read **only** on the `!bNewLights` side: `lights.c:115` opens the ini-gated copy, `lights.c:566-660` computes the four like-indicator states with no ini reference | `present` then superseded (see the 44.0 rows below) | at T114 this became `const bool odStdSignals = mi->bNewLights || bStandardCarsUseTurnSignals;`, which reproduced the binary's asymmetry but kept the ini byte as a live gate for the 90 models with no lamp nodes. Plan 44.0 N2 removed the term: the branch is decided per side by the measured lens (`odLikeFwdL = odIndLeft && !odAmberLf`), so the byte now decides nothing here and is still parsed only because the mod's `features.ini` writes it (`src/core/re3.cpp:325`). |
| the secondary headlight and taillight coronas (`this+26/27` at `lights.c:725-829`, `this+28/29` at `lights.c:883-949`) are driven by the **same** like-indicator states as the primary pair, not by `bLightsOn`; on `alarmOff` the binary registers a zeroed `TYPE_STREAK` corona rather than leaving the flare lit | `present` (restored) | ids `+60/+61` front and `+62/+63` rear — the mod's own ids are unusable in this base because `src/vehicles/Vehicle.cpp:2414` registers `+27+i` in the same corona pool. Outer `&& bLightsOn` dropped, each `RegisterCorona` gated per side with `odFwdOnL/R` / `odBackOnL/R`, and an `alarmOff` branch zeroes them (`src/vehicles/Automobile.cpp:4089-4166`). Why it mattered here: the steady secondary flare behind the blinking primary is what made the Banshee read as "two white lights per side and one of them never goes out". |
| every lamp corona `RegisterCorona`/`UpdateCoronaCoors` hangs on the per-node liveness test (`GetFrameLightStatus`, `lights.c:660-950`) | `present` | `Damage.GetLightStatus(VEHLIGHT_*) == LIGHT_STATUS_OK` per side, kept from T110. |
| blink phase `*(dword*)0x987dd0 & 0x200` (CTimer milliseconds) | `present` | `CTimer::GetTimeInMilliseconds() & 512` — same bit, same period. |
| `CCoronas::RegisterCorona` takes 16 stack args in the binary; the vendored signature has 14 and the extras are the trailing `0` and `1.5f` | `present` | no change needed: the port passes the values the modern defaults would carry. |
| `CShadows::RenderIndicatorShadow` (`0x5e5a10`) is **not** a lamp effect | `excluded` (not applicable) | decompiles to `C3dMarkers::PlaceMarkerSet(nID, MARKERTYPE_CYLINDER, *pPosn, max(fFrontX,-fSideY), 252, 138, 242, 228, 2048, 0.2f, 0)` (`Shadows.cpp:2513-2522`): a pulsing 3D marker cylinder, neither a corona nor a material change. Nothing to port — a previous pass had flagged it as a missing flash source. |
| Banshee lamp ownership: `headlight_l/r < bump_front_dummy < chassis_dummy` and every rear lamp `< chassis_hi < chassis_dummy`, so `newLightsData.bBumperHeadlights` is its **only** lamp flag (`bBumperTaillights`, `bWingHeadlights` and all indicator flags false) | `present` | already the T110 damage gate. Measured consequence for the player: a Banshee keeps its taillights through any rear impact, and no door, wheel or rear-wing hit can extinguish a lamp on it. |

Two deviations carried over from the rows above, re-confirmed against the binary and
left as they are: the amber verdict comes from the measured lens faces
(`IndicatorLensTable.h`) rather than from the presence of `indicator_*` nodes, and the
port drops the mod's camera-angle gate on its own indicator pass. Both are the player's
mother rule and the fix for defect #2, not oversights.

### Fleet-wide Banshee method (plan 44.0, 04/11/2026, T116)

Reference behaviour the player signed off on the Banshee (42.1): a side with no real
amber lens blinks **its own** headlight/taillight, corona and ground shadow included.
Plan 44.0 makes that the rule for the whole fleet and removes the last invented colours.
Compared against the shipped binary rows above and against the mod's own data.

| mod construct | class | port |
|---|---|---|
| the ini byte gates the copy of the four like-indicator booleans for models **without** lamp nodes (`lights.c:115`), and with the mod's `features.ini` (`StandardCarsUseTurnSignals = 0`) that copy never blinks | `local addition` | plan 44.0 N2 drops the term for every model: `odLikeFwdL = odIndLeft && !odAmberLf` and its three siblings. What the binary does is legitimate **for the mod**, because there the blink is written into the lamp *material* and a model with no `headlight_l`/`taillight_l` node has no material to write into; extending it to those 90 models costs a corona plus shadow (`headlights`/`taillights` dummies exist on every car — measured 101/115, the 14 missing are boats/planes that do not run `DoVehicleLights`). Consequence the player accepted as the Banshee's own cost: the flash and its light shadow blink **also by day**. |
| `bStandardCarsUseTurnSignals` decides the route | `excluded` | inert in this port: still parsed and logged because the mod's `features.ini` writes it (`src/core/re3.cpp:325`, commented there). Its only other consumer never existed here. |
| the amber of a generic lens lives in the engine: the mod paints its `lights` atlas at load time (`lights.c`, the material's `onColor`) | `translated` | plan 44.0 N1 declares it in data instead: `gen_indicator_lens.py` marks a cell amber when **every** counted face carries the mod's own lens atlas (`lights`/`lightson`), keeping its measured colour. 32 cells that the calibrated hue window had dropped (class `O`) now have a row. Table 306 → 338 rows, amber 176 → **214**, not the 212 predicted: `baggage` LF/RF turned out to be `lights`-atlas cells too. `sabre`/`sabretur` front go `amber=0 → 1`, which is the player's "los del parachoque no tienen efecto ambar"; `pony` and `topfun` rear stay `amber=0` (measured red lenses, not asked for). |
| `ViceExtIndicatorAtomicCB` raised `ambient` on **every** material of the indicator frame | `local addition` | the lens mask now rules the paint too (`44.0 N3`): materials whose texture is not body, or all of them when the atomic has nothing else. It is the same formula the data uses (`lens_mats` in the generator) and the anchor sweep already used (`ViceExtIndScanCB`), so measurement and drawing exclude the same faces. Measured on the served tree: 1829 → 1665 faces reachable (164 saved, 20 atomics with body dragged in). The fallback is not cosmetic — `cuban`, `stinger`, `zebra`, `boxville` and others have their painted lens **in** a body texture, and a blind "no body" rule would delete those lenses (that is why the mod's own redesigns are unaffected: its nodes carry one lens material). Hermes `indicator_lf`: 16 faces touched → 2, which is the "square patch on the bonnet" the player reported. |
| the mod anchors one indicator corona per side at `m_positions[CAR_POS_INDICATORS_*]` | `local addition` | 44.0 N4: the sweep groups by atomic and registers one corona per group, `id = idFirst + g*n + k` (cars `+48..+55`, bikes `+44..+47`, both inventoried free), each with its own measured anchor, spread and row (`occ` = index of the **measured atomic** in the traversal, not of the cache slot, so a stretch rejected by the plausibility gate cannot shift the colour of the next one; table rows are emitted in that same atomic traversal order). Measured: 14 cells in 6 models (`esperant`, `landstal`, `packer`, `rancher`, `sandking`, `zebra`), maximum 2 groups (310 cells have 1 row, 14 have 2, so the `ODIND_MAXGROUP = 3` merge branch never runs on this fleet and `occ = i` is exact stretch by stretch), face counts identical between the table rows and the engine-side groups. Without it the merged centre of mass sat between the two stretches — `packer` has them **4.16 m** apart, which is the flash "floating in the middle of the cargo bed". |
| colour of the flash: the binary takes it from the material it just painted | `excluded` | the two engine-side guesses are gone (44.0 N5): no row ⇒ `ODIND_OWN`, and `ViceExtIndicatorCorona` draws only the measured triple (hue-normalised to the top of the additive range, which invents no hue). A cell that the generator could not measure now simply does not flash. |

Open item, not a `gap`: the in-game signature of the eight models the player named
(mesa, sentinel, esperanto, regina, hermes, landstalker, sabre, pheonix, plus banshee as
control) is step 10 of the plan and belongs to the player.

### Lente ambar en carroceria y faro lleno (plan 45.0, 05/11/2026)

Re-read of the shipped binary, bounded to the one question T7 asks: does the
mod animate a lens through anything other than the `indicator_*` node frames,
and is there a per-model table that animates a lens painted into bodywork?
Target as in T114: `C:\Users\s0rno\Desktop\Gamez\(g)old\Grand Theft Auto Vice City\ViceEx.exe`
(sha256 `4df2726e03654899e1f77a14acb3175f03c04ab4c4b36f0693eec086fc74c48b`), retained
pseudocode `C:\tmp\iav42\render.c` = `FUN_0062cd60`, lamp-material region.

What the binary does, indicator by indicator (`render.c:499-544`, and the same
shape repeated up to slot `0x154`):

- `0xff007d96` / `0xff00e1ff` are built once outside the blocks (`:499-500`).
  Unpacked as `RWRGBAINT(r,g,b,a)` (`vendor/librw/src/rwbase.h:156`) they are
  (150,125,0,255) and (255,225,0,255) — the mod's own `indicatorOffColor` /
  `indicatorOnColor` (`tmp/upstream-gta-extended/src/vehicles/Automobile.cpp:3877-3878`).
- Eight **consecutive** node slots, `in_ECX[0x14d]` through `in_ECX[0x154]`, tested
  non-null and each passed to `FUN_00547db0(frame, FUN_00618bc0, &atomic)` =
  `RwFrameForAllObjects` + `GetCurrentAtomicObjectCB`. Those are the eight
  `CAR_INDICATOR_*` entries, which are consecutive in this port's enum too
  (`src/vehicles/Vehicle.h:59-66`), in the same order the mod's source writes them
  (LF, RF, 2_LF, 2_RF, LR, RR, 2_LR, 2_RR — `Automobile.cpp:3880-3967`).
- Per block: `iVar2 = **(int **)(local_14[5] + 0x4c)` = `geometry->matList.materials[0]`
  — the **first material only** — then two stores into it: `(iVar2+8) = 0x41200000 |
  0x3f800000` (10.0f/1.0f, the mod's `ambientOn`/`ambientOff`, `:3766-3767`) and
  `(iVar2+4) =` the chosen colour.
- The on/off choice is `(m_bIndicatorState side == 1) && (DAT_00987dd0 & 0x200)`
  (`:505`, `:528`): the state byte plus the CTimer millisecond bit 512. **No model
  index is read in any of the eight blocks.** The four blocks before them
  (`:431-498`, slots `0x149-0x14c`) are the brake/reversing pair: they also write only
  ambient plus `mi->lightsOnTexture`/`lightsOffTexture` (`iVar3+0x4c8` / `+0x4c4`).
- Negative result (bounded search): in the vendored source the binary is compiled
  from, the only material animations are in `vehicles/Automobile.cpp` and
  `vehicles/Bike.cpp` and every one of them is keyed on `m_aCarNodes[...]` /
  `m_aBikeNodes[...]`; a search for any per-model lens table
  (`IndicatorLens|aIndicator|lensTable|bodyIndicator`) returns **0 hits**. So in the
  shipped game a lens that lives in the bodywork is **static**: it never changes
  colour or ambient, and the only way the mod makes such a car blink properly is to
  ship a redesigned model whose lens hangs on an `indicator_*` node.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| the animation reaches only materials hanging under the eight `indicator_*` frames, so an amber lens painted into the bodywork (the player's D1: `virgo` front, `washing` rear) stays dark in the binary | `extended` | plan 45.0 T4 moves the measured lens into the node instead of animating outside it: 52 lens ops (95 faces, 5781,1 cm²) and 41 panel ops (92 faces, 6071,6 cm²) over **27 DFF**, 83 new atomics and 4 new `panel_*` frames, with the silhouette verified identical (0 faces moved in position or look). The engine code stays the binary's rule — node frame → its lens materials — so what is extended is the **data**: a lens the binary leaves static now blinks, and the bevel/pilot faces that used to share the node were exiled to `panel_*` so they do not blink with it. The 5 `NO-INVENTAR` racimos and the `esperant` rear clusters keep the binary's verdict (no node, no animation) because inventing one there is exactly the defect the player banned. |
| one material per node: `materials[0]` | `translated` | the row's measured material mask `mats` (a 32-bit bitmask; max materials per geometry measured after the split = 10, `IndicatorLensTable.h`) decides which materials of the atomic get painted, so a lens the served model splits over two or three materials lights all of them and the rest of the atomic — the panel — stays untouched. Measured: 262 rows, 166 cells (name, node, side), 58 models. |
| `indicatorOnColor (255,225,0)` / `indicatorOffColor (150,125,0)` hardcoded for every car | `translated` | the colour comes from the row: virgo front `{165,90,51}`, washing rear `{172,69,25}`/`{182,72,24}`/`{175,72,24}`, washing front `{150,79,23}` — all measured off the texel footprint × material colour, none typed in. Two measured rules gate where it may be written: (a) a cluster whose faces carry the mod's `lights`/`lightson` atlas is painted, because those texels are **white by design** and the material colour is the only place its amber can live (117 rows, design colour (255,225,0) kept — so on the atlas side this row is `present`, not `translated`); (b) a cluster with its own texture is **not** painted at all, because its pixel already carries the tone and multiplying two ambers would darken the lens. Measured check: 0 atlas faces inside a node that its rows do not declare lens, 0 own-texture amber clusters painted. |
| blink phase and on/off colours written to `materials[0]` at `ambient = 10.0/1.0` | `present` | same bit (`CTimer::GetTimeInMilliseconds() & 512`), same two ambient values, same first-atomic callback. |
| the binary has no flash at all for a lens outside a node; the flash it does have is one corona per side | `extended` | 45.0 T5 registers one corona per **welded cluster** (`ViceExtIndScanCB` now unions lens faces by shared vertex, cap 4096 vertices per geometry, `ODIND_MAXGROUP = 5` after the measured tally 106/36/16/4/4), each sized from its measured amber area (`dia = 0.02*sqrt(area/pi)`, clamped as before). Not a mod construct: the row↔group pairing is a contract this port needs because its table is generated from the same served files it loads — order (atomic sweep, then face order) plus a 25 cm centroid cross-check with a nearest-row fallback, verified fleet-wide at 166 cells / 262 pairs, 0 recuento mismatches and 0 centroids beyond tolerance. Clusters 3..5 moved to `idFirst + 60 + …` because the previous band collided with the `+60..+69` headlight/taillight coronas of `Automobile.cpp` in this same corona pool. |

### Atlas de lente derivado y nearDist del destello (plan 46.0, 06/11/2026)

Two defects the player opened on the signed 45.0 build (06/11, two captures):
**C1** — inside a blinking lens there are unpainted patches (triangles, sometimes
lines, on amber *and* on red clusters) that "should all be painted"; the player's
condition was one root rule for every car. **C2** — the front-right flash renders
above the bonnet, "as if the material were not there".

Evidence is the served tree plus the retained binary pseudocode of T114
(`C:\tmp\iav42\render.c`, `ViceEx.exe` sha256 `4df2726e…`); plan 46.0 ran **no**
new REA pass, so the binary claims below are the T114 rows, not fresh decompilation.
Detection and re-measurement: `lamp46t.py` (135 mixed faces / 22 models at the
15 % cut), `gta_vc_browser/tools/gen_lens_atlas.py` (generator + DXT1 encoder) and
`lamp46v.py --served` (criterion 1, measured on the **decoded served bytes**, so
encoder loss is inside the number).

| mod construct | class | port |
|---|---|---|
| the lens texture is swapped at load, not animated per face: `mi->lightsOnTexture` / `lightsOffTexture` (and the `servicelights` pair) are written into the material, `render.c:431-498`, resolved in `src/modelinfo/VehicleModelInfo.cpp:517-525` | `extended` (same mechanism, new pair) | plan 46.0 T3 reuses that exact route for a **derived** atlas: `indicatorLensOff[4]` / `indicatorLensOn[4]` + `numIndicatorLensAtlases` on `CVehicleModelInfo`, filled in `SetClump` by one `RwTexDictionaryForAllTextures` walk that pairs `<atlas>` with `<atlas>_lens` **by pointer** (never by name lookup per material), and applied in `ViceExtIndicatorAtomicCB` after the row's `mats` gate — `RpMaterialSetTexture(material, p.on ? lensOn : lensOff)`, nil = untouched. Pairing by pointer means only materials that really draw the lens atlas can match, so the re-tinted texels cannot leak onto chrome or bodywork (the collateral that killed the "rewrite the original atlas in place" route). One derived atlas per model is all the served tree needs (`VICEEXT_LENS_ATLAS_PAIRS = 4` is headroom, measured: 1 atlas carries lens faces per car). |
| an amber lens lives in the model's own atlas; the mod ships redesigned cars whose lens region is warm and never touches a face that straddles warm and neutral texels of the same atlas | `extended` (data, not engine) | the engine paints **by material**, so a face whose UV footprint covers both a warm lens zone and a chrome/shadow zone of the same atlas comes out half grey × amber — there is no engine-side fix, it is data. The generator copies the car atlas and re-tints only the texels **inside the lens footprints** that carry no hue (`delta/max ≤ 0.18`) and have enough light to show one (luminance ≥ 60), to the chroma measured from the warm texels of that same footprint (sum-of-channels mean normalised to 3), preserving luminance: the glass drawing survives, nothing is invented. Measured over the 22 models: 17 derived atlases, **925 texels**, +136 KB of DXT1; hue-less faces at ≥15 % **68 → 4** (8116 → 491 cm²) and absolute-neutral **161 → 83** (14066 → 4862 cm²). Residual, reported and not hidden: the 4 are `bobcat`'s rear clusters, whose own pool has 6 warm texels (< the gate of 30), no mirror side and no `IndicatorLensTable.h` row — the chain ends in skip-and-report, so they stay as the binary leaves them; the absolute residue is dark-neutral (mean < 60), which re-tinting cannot brighten without inventing light. Round-trip cost of the encoder is inside those numbers (max per-channel error up to 105, mean 0.268 over 128×128 DXT1), measured after decoding. Runtime safety of the injected chunk, re-read with the engine's own parser rules (`d3d8::readNativeTexture` via `rwNativeTextureHackRead`): platform `0x00000008` = `PLATFORM_D3D8`, 0 unreadable textures, dictionary counters and chunk tiling exact in 17/17 (so `RwTexDictionaryFindNamedTexture` really finds the `_lens`), raster header bit-identical to the sibling base atlas in the same TXD, and 0 index-3 texels in the injected atlases (and 0 in their bases) so `dxt1HasAlpha` stays 0 — no `TXDDROP` of a whole dictionary, no new transparency, no alpha-test flip. |
| `CCoronas::RegisterCorona` is called with 16 stack args and the trailing pair is `0, 1.5f` (T114 row above), i.e. the mod uses the engine default `nearDist` for the indicator coronas | `excluded` (deliberate deviation) | plan 46.0 T4 passes `false, VICEEXT_IND_NEAR` (`0.10f`, `src/vehicles/Automobile.cpp:2809`) for **indicator** coronas only; headlight, taillight and shadow coronas keep the engine default. Reason: `Coronas.cpp:359` subtracts `nearDist` from the sprite coordinate unconditionally, so with 1.5 m the disc is planted 1.5 m in front of the lens and reads on top of the bonnet (C2). Measured cost/benefit in the camera-angle row of the 42.1 table: bleed beyond 10 cm 12 %/35 %/45 % → **0.0 %/0.0 %/0.0 %**, occlusion by real chassis 13 %/34 %/46 %; `0 m` would eat 50 %/61 %/63 % of the disc because the bodywork stands a median 9.4 cm in front of the lens plane. `useNearDist` stays `false`, as in the binary: it only gates the 35–50 m fade. |
| the mod's data has no per-cluster colour for a lens that is not on an `indicator_*` node (45.0 rows above) | `present` (unchanged) | 46.0 does not add any: the derived atlas only re-tints texels whose hue already exists somewhere in the same footprint, and the 45.0 lens/panel split, the measured row colours and the damage gate are untouched. |

Open, bounded question (T7, not run): whether the mod rewrites the `nearDist`
default inside `CCoronas::RegisterCorona`/`CCoronas::Render` for vehicle coronas.
The claim above rests on the T114 call-site evidence; a fresh REA pass would only
confirm that no other writer exists.
### Aspecto del destello, trafico y textos del menu (plan 51.0, 07/11/2026)

Five corrections the player opened on the 46.0 build after playing it (07/11, with
one capture of their own local Vice City Extended as the look reference): the flash
disc too big (D1), the amber too yellow (D2), traffic signalling when it is not
turning at a corner (D3), the player's indicators dying the moment they step out
(D4), and the control-config page naming three camera actions where the indicator
keys live (D5). Evidence: `C:\tmp\iav42\ind47_measure.py` and `ind47_glow.py`
(radial profile and colour of both captures), the served `IndicatorLensTable.h`,
the vendored mod source, and the six `streamed/TEXT/*.gxt`. No new REA pass: the
binary claims below are the T114/render.c rows already recorded in this ADR.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| the flash's world size is not a binary constant: this port sizes each corona from the measured amber area (`dia = 0.02·sqrt(area/π)`, `size = Clamp(2.5·dia, 0.26, 0.55)`) | `extended` (look retuned to the mod's own render) | 51.0 T2: `VICEEXT_IND_SIZE_MULT 1.6f` with clamp `[0.09 ; 0.24]` m. Measured on the player's VCE capture (339×124; scale 0.58 cm/px from the rear width and 0.565 cm/px from the lens itself — the two agree): half-max radius 11 px = 6.4 cm and visible edge 19 px = 11.0 cm over a lens 11-12 px (≈6.7 cm) wide ⇒ sprite radius ≈ 1.6 × lens diameter. The same cluster in the served capture measured 22 cm at half-max and ≥38 cm at the edge: **3.4× the reference**. Over the 244 amber rows with area > 0 the median size goes 0.272 → **0.174 m** and the maximum 0.55 → **0.24 m**; 32 rows sit on the 0.09 floor (lenses under 5.6 cm) and 49 are capped. |
| `indicatorOnColor (255,225,0)` hardcoded for every car (the 45.0 row above kept it as `present` on the atlas side) | `translated` | 51.0 T1 replaces it with **the amber the mod actually shows**, measured on that capture: the two rear lens cores read (252,162,0) and (254,168,0) → mean **(253,165,0)**, g/r 0.65. Why it is a translation and not an invention: the mod multiplies (255,225,0) by *its own* redesigned lens texels, which already carry the hue; in the served vanilla models the `lights`/`lightson` texels are white by design, so the material colour is all that reaches the screen and g/r 0.88 reads as the «amarillo chillón» reported. The change lands only on the atlas rows: of 244 amber rows **117** carry the design colour and change; the other **127** measured rows are already at g/r ≤ 0.643 and came out byte-identical from the regenerated table (262 rows, 166 cells, 58 models — same counts). |
| NPC indicator = `DotProduct2D(GetRight(), m_nNextDirection * m_carPathLinks[m_nNextPathNodeInfo].GetDirection())` against ±0.5, evaluated over the whole segment (`upstream-gta-extended/src/vehicles/Automobile.cpp:3731-3741`) | `extended` | 51.0 T3 keeps the mod's angle **and its 0.5 threshold** (the port had loosened it to 0.2 ≈ 11.5°, which is most of the over-signalling) and adds the two gates the player asked for: the node ahead must be a junction (`CPathNode::numLinks ≥ 3`) and the car must already be approaching it (`dist ≤ max(8 m, 0.55 · longitud del tramo)`, the tramo measured between `m_nCurrentRouteNode` and `m_nNextRouteNode`, the pair the route rotation leaves in `CarCtrl.cpp:2065-2066`, which advances when the curve time ends — `CarCtrl.cpp:1288`). Straight through a junction signals nothing, and the signal dies on its own once the corner is behind. Cars and bikes share one function (`ViceExtNpcTurnSignalSide`) so the traffic has a single rule. |
| the mod has no hazard state for traffic: its NPC branch writes one side from the path angle and nothing else | `extended` (requested behaviour, absent in the binary) | 51.0 T3 also returns `hazard` while a driver is still aboard, the car is under `VICEEXT_IND_STOP_SPEED` (0.02) and its mission is `MISSION_NONE` — the state the engine itself uses to park and release occupants (handbrake at `CarCtrl.cpp:2691`; `TellOccupantsToLeaveCar` sets it at `CarAI.cpp:180-182` and `CarCtrl.cpp:3280/3297`) — or the ped is already in `PED_EXIT_CAR`. Both sides blink while the NPC gets out. |
| a car with no driver (or with its driver not in it) gets `m_bIndicatorState[LEFT] = m_bIndicatorState[RIGHT] = false` every frame (`:3739-3741`) | `excluded` (deliberate deviation) | 51.0 T4: what the player latched with the key survives stepping out (`m_bIndicatorLatched`), because the request was «deben mantenerse siempre activas hasta que se desactiven presionando el boton»; what an NPC was carrying is still cleared, as in the binary. Cost stated plainly: a car the player left blinking keeps blinking until they press the key again — from outside there is no key handler, the pad branch is `this == FindPlayerVehicle()` in the mod too. **51.1 E1 corrects how this row was implemented**: the 51.0 port additionally cancelled the hazard pair on the parked→moving transition, which the player never asked for («no pedi que si tengo las direccionales de parqueo y acelero se quiten, jamas dije eso»). That rule is gone from the binary (`parqueo-andando`: 0 occurrences in `reVC.wasm`) and hazards are the player's decision only; what replaces it is the NPC's own judgement — a car that parked with hazards on (`m_bIndicatorNpcHazard`, set by the `MISSION_NONE`/`PED_EXIT_CAR` rule above) switches them off itself when it moves again, named in the trace as `motivo=parqueo-apagando`. `ViceExtApplyIndicatorMaterials` only reads the two bools, so nothing else in the lamp chain changes. |
| not a binary construct: the port's own vehicle-controls page (`Frontend.cpp:2723-2725`) | `extended` (text) | `FEC_LTS`/`FEC_RTS` were created as copies of `FEC_LOL`/`FEC_LOR`, so the page read «Look left» / «Mirar izquierda» where the keys toggle the indicators, and `FEC_EML` said «emergency» where the player calls them parking lights. 51.0 T5 rewrites the three keys in the six `streamed/TEXT/*.gxt` (menu pair = TKEY/TDAT chunks 1/2) with a new `set` subcommand of `gxt_menu_keys.py` (`add` skips keys that already exist): EN and RU «Left/Right turn signal» + «Hazard (parking) lights» — the russian GXT ships untranslated (1 Cyrillic key in that pair), so it stays English like the rest of its file —, ES «Intermitente izquierdo/derecho» + «Luces de parqueo», FR «Clignotant gauche/droit» + «Feux de détresse (stationnement)», DE «Blinker links/rechts» + «Warnblinker (Parklicht)», IT «Freccia sinistra/destra» + «Quattro frecce (sosta)». Verified by re-reading every row of each pair (2620/2614/2686/2668/2688/2610): exactly 3 values differ per language, no duplicate keys, `check-tabl` OK with its 78 missions. |
| the mod has no indicator corona at all on bikes (`Bike.cpp` of `upstream-gta-extended` never registers one); the flash this port adds for a bike lens is ours | `extended` (our own effect, now anchored like the engine anchors the bike's lamps) | 51.1 E2: the player's capture of a leaned bike showed the two amber discs **standing in the air where the upright lens would be**. Read from the served DFF (`tools/dff_dump.py streamed/models/gta3.img/faggio.dff`): `indicator_lr`/`indicator_rr` hang under `chassis_dummy`, and `chassis_dummy` is the frame the engine writes the lean into (`Bike.cpp:2030-2034`, `BIKE_CHASSIS` in `VehicleModelInfo.cpp:192`). The anchor was stored in **root** space and multiplied by `CVehicle::GetMatrix()`, which carries no lean. Now, for bikes only, the cached anchor is re-expressed in the lens node's own space (`rel = Invert(LTM(node)) * LTM(root)`, computed inside the same hierarchy sweep so it cannot inherit the world-placement staleness that the 45.0 comment guards against) and the corona is placed with the node's **live** LTM, so the lean — and any chassis animation — follows automatically. Cars are untouched: their measurement, the plausibility gate (`|x| ≤ 2.5, |y| ≤ 9.0, |z| ≤ 2.5, spread ≤ 4.0` over the 162 measured cells) and the `xyz` row cross-check all still run in root space, before the re-base. |

### Lente ámbar de los camiones de carga (plan 55.0, 08/11/2026) — **REVERTIDO**

The player named three cargo vehicles (Boxville, Benson, Mule) that blink their
white head / red tail lights instead of their own amber lens, and asked whether the
detection tool had simply not covered that category. The 55.0 answer was wrong and
the player rejected it: «**los cambios que hiciste son invenciones, te pido
revertirlos**» (08/11, eight captures). Everything it touched is back out —
`benson.dff` restored byte for byte in streamed and bootseed (sha256
`3a88e498cdd8d9f0…`), table back to 262 rows with no benson row, the three relaxed
gates of `indicator_lens45.py` reinstated, `dataTag`/`VERSION` returned to the next
agent's values, manifest seal `60a0c0a3f340f41b170e4b6b21982b0d`, `build.sh` 0
errors, `check-served-build.sh` OK. What survives is the **re-measurement**, which
shows the 55.0 conclusions were built on a broken sampler; the work restarts as
plan 56.0.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| a side whose `indicator_*` node holds no amber lens blinks that side's headlight/taillight (`Automobile.cpp:3775`/`3805` of the mod, already ported as the Banshee route) | `present` (rule) — **the 55.0 application of it is retracted** | The engine rule stands. What the 55.0 claimed does not: it moved the benson's `chassis_vlo` patches (2 faces, 644 cm², mean (140,87,38), 65 % amber coverage) into `indicator_lf/rf` because they were the biggest amber-mean cluster, but the player's capture shows those are the **lateral markers** — their centroid sits 0.14 m **outboard** of the headlight anchor (0.78, +2.60, 0.03) while the lens they see blinks **under** the lamp. "Adjacent to the lamp" was never part of the test, so the test could not tell a lens from a marker. Reverted; 56.0 T2 makes the lamp anchor (`m_positions`/the `headlights`/`taillights` frames) part of the qualification rule. |
| the detection tool's coverage of this category | `extended` (detection) — **the 55.0 verdicts of "no amber lens" for mule and boxville are withdrawn** | Re-measured with a patch-vs-lamp probe (`C:\tmp\iav42\cargo56.py`) and an atlas-overlap probe (`cargo55uv.py`): the real blind spot is the **UV sampler**, not the name/area gates. `indicator_audit._face_render` returns `px=0` (and then the white material fallback) for the fbiranch's 12 front faces that provably sit inside the 358-texel amber region of `fbirancher868bit128` (`wing_lf/rf_hi_ok` at x=±1.10-1.13, y=+3.10-3.13 and `bump_front_hi_ok/_dam` at x=±0.80-0.90, y=+3.25-3.29) — exactly the "amber at the sides of the headlights and in the bumpers" the player pointed at. The mule's front lens is real but painted **inside** a 2563 cm² `chassis_vlo` face in the headlight plane (y=+2.78 vs anchor (0.71,+2.78,−0.08)) with 23 %/14 %/9 % amber coverage, so it needs a sub-face cut, not a move. The boxville's zero overlap is **not** evidence of absence while faces report `px=0`. Pony: rear measured (4 faces, 43 % and 15 % coverage, 0.19 m from the taillight anchor) plus a 104-texel amber region in the mod's `lights` atlas that the current rules exclude by name. Coach: its amber already sits in the nodes (161 and 180 cm², 0.46 m from the headlight anchor, 0.18 m below it). |
| the mod ships its own redesigned trucks, so this question never arises in the binary | `extended` (detection) — **reverted; kept only as the record of what was tried** | `indicator_lens45.py` had three gates that dropped this category: `clusters()` skipped any frame containing `vlo` (a truck's front bumper is low, not an underbody), skipped faces with `px < HUE_COVER_MIN` (these modellers paint a lens with **one** texel, so the UV triangle is degenerate — 8/8, 25/25 and 10/10 node faces of benson/boxville/mule), and `_verdict()` failed `FRAME=extra` by name. All three were relaxed and then **put back** when the player rejected the result, because relaxing them treated a marker as a lens. The trash case is the same story in miniature: its `TRASLADAR` on the front doors was applied, measured inside the node as `amber=0.0, frac=0.000` (its real amber is a ≤45 % stripe on `chassis_vlo` at z=+1,59 above the rear doors), and reverted from the backup byte for byte. Stinger needed nothing: its 20 node faces already read 77 % amber, and the tool correctly refused to empty geometry 26. |
| an indicator node is a frame + the mod's 9/8 `lights` lens, and the mod's trucks ship rear nodes even where vanilla has none | `extended` (data) — **56.0 T7: the creation path was already ported (plan 45.0), only its trigger was wrong** | `lens_patch` created nodes **only when the model had no `indicator_*` at all**, so fbiranch/esperant/fbicar — which have front nodes — never reached it, and T3b reported "no rear frame". Fixed as an explicit per-model opt-in (`--crear-faltantes`) instead of widening the trigger for the whole fleet, with a guard so an existing side is not duplicated into `indicator2_*` (the engine would then paint two lenses in one place). Applied to fbiranch and fbicar: each gained `indicator_lr` + `indicator_rr` (48→50 frames, 26→28 geometries/atomics, **0 faces touched outside the new nodes**) placed on the measured rear clusters at 0.19 m and 0.22 m from the taillight anchor, verified as the mod's 9/8 geometry with the `lights` material, `dff_readcheck` 0 errors, table back to 324 rows / 216 amber nodes with none lost. **Esperant was refused by the machinery's own geometry gate** (its rear amber sits 92 % at 0.12 m from the pilot but high on the tailgate, `zrel > 0.94`, outside the compact-lens boxes) and the gate was *not* relaxed for one model — that relaxation is precisely what made 55.0 invent lenses. The tank is covered by the camera rule with no change: `rhino` is already in the measured table and `FileLoader.cpp:1030` maps its IDE type to `VEHICLE_TYPE_CAR`, which is what `IsInteriorlessVehicle` requires. |

### Deteccion corregida y camara/luces del coach (plan 56.0, 08/11/2026)

Plan 55.0 was rejected as invention; 56.0 fixed the **instrument** first, then
acted only on what the player approved. Two independent sampler bugs were found
and are now fixed in `gta_vc_browser/tools/indicator_audit.py`; both were
silent — they returned "no amber here" for lenses that the GPU is drawing.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| the amber a lens shows comes from the texels the GPU actually samples | `present` (rule) — **the port's measurement of it was wrong, now corrected in two places** | (a) `_uv_tri` clamped UV to `[0,0.9999]`, so any face whose three V sit above 1.000 collapsed onto the atlas's last row, gave area 0, and fell back to white — 1193 `px=0` faces in fbiranch. It now wraps only when the whole triangle is outside `[0,1)`; wrapping unconditionally cost the stretch its two front cells (the 114.2 cm² face plan 45.0 rescued from the seam), so the condition is measured, not assumed. (b) `_face_render` dropped every texel whose byte alpha was 0 — but in textures the native header declares `hasAlpha = 0` that byte is converter junk, not transparency (`fbirancher868bit128`: 128×128, fmt 0x8600, hasAlpha 0, compression 0; lens texels read (156,80,8,**0**) and (164,89,8,**0**) while body grey carries 146). `_decode_native` now normalises alpha to opaque only when the texture has no alpha channel and is not DXT (DXT1's `c0<=c1` index-3 alpha is real; plan 46.0 measured 0 such texels in the injected atlases). Effect on fbiranch: **0 → 24 amber faces**, out of 2161 measured; the served table is byte-for-byte identical after the fix (sha `1ed42c7b5da6…`), so nothing the player had approved moved. |
| the ≥50 %-amber-area verdict must be an area measure, not a texel count | `extended` (detection) | Plan text asked for "cobertura por área … y su peso"; T1 had implemented centre-counting with supersample fill. With exact cell clipping (`_cell_cover`) the sandking front-left cluster reads **0.512** instead of 0.4947 — i.e. the centre-count was about to delete an amber lens the player has been testing since 51.0 because of a half-percentile of quantisation. Node-level check after the change: 164 amber nodes served → 198, **0 lost**, colour drift ≤ 13/255 on rows that already blinked. |
| a lens that exists on a body panel is not an indicator until it is in the node | `extended` (data), same construct as plan 45.0 | 56.0 T2's new tool `indicator_lamp_anchor.py` qualifies a cluster by **distance to the model's own `headlights`/`taillights` frame** (≤0.50 m, same side, same axle, coverage ≥25 %) instead of by biggest amber mean — the exact rule 55.0 lacked. Two known limits, stated with numbers: the radius is not scale-aware (the coach's own nodes sit 0.52 m from its headlight anchor), and "at the anchor" cannot separate an indicator from the lamp's own glass, which is why the lamp-named frames are filtered out of the surgery list. |
| vehicles without an interior | `absent` in the binary — the mod allows first person everywhere; **new local rule the player asked for** | No interior flag exists in the engine (`m_bNoInterior`, IDE bits and `carinfo` texdics are all absent — checked in `src/modelinfo/`). Criterion measured instead: faces inside the driver's cabin volume ahead of `ped_frontseat`. Of 112 measured vehicle models only manchez/rcbandit/sanchez (0), **coach (6)** and **rhino (11)** fall under 20, and the next model in the fleet has 24 while the median is 113 — the gap holds the threshold, not a chosen number. Generated data `src/vehicles/NoInteriorModelTable.h`; `CVehicle::IsInteriorlessVehicle` (car type only, so bike/plane rows stay inert) plus three camera sites: the cycle skips the first-person slot in the pressed direction, and the state is corrected after the ring wrap so entering such a vehicle while the preference is first person lands in third. |
| the ground light pool is anchored `lamp.y + constant` while the lamp position is already applied | `present` (vanilla rule) — **kept for every vehicle inside the fleet's own range; clamped beyond it** | Doubling the model length made the coach's brake pool sit 3.45 m **behind** the bus and its headlight pool start 3.67 m **ahead** of its lamps. The additive quad is *centred* on the position passed (`CastShadowEntityXY`, `src/renderer/Shadows.cpp:1628`, half-extents 8.0/4.5 m for headlights and 1.0/1.0 for brakes), so the offset is what must be bounded. `ViceExtLightShadowOffset` clamps to [6,9] and [-2,2] — the range the fleet already uses, measured over the 115 IDE models: 23 models move (18 by more than 0.5 m, all long: coach, bus, packer, trash, trash2, firetruk, linerun, barracks, flatbed, yankee, stretch, romero, hunter plus the air/water ones that never reach this function) and the six references the player names keep their exact values (police 8.29/-0.62, sabre 8.52/-0.76, cabbie 8.74/-0.75, rancher 8.58/-0.69, sandking 8.47/-0.31, benson 8.60/-1.53). Scaling the constant by box length was rejected: it would have moved every car. |
| surgery only where the player approved it | `extended` (data) | `indicator_audit.py --lens` (the plan 45.0 machinery, not a hand-written patch) rebuilt the nodes of boxville, mule, trash, pizzaboy (four corners each), fbiranch, esperant and fbicar (front). Every face delta lands **inside `indicator_*` frames only** (body frames keep identical counts) and no removed node face is left uncovered — the closest surviving geometry is ≤0.18 m away. `idaho` and `stinger` were **refused by the tool with its own reason** ("todos los nodos ya son la lente del mod" / "indicator_lr no tiene la lente 9/8") and stay untouched; the fbiranch/esperant/fbicar rear lenses are measured and approved but have no rear frame, and no frame was invented by hand. |
| the reverted 55.0 had left the preload tree stale | (record) | `bootseed/models/gta3.img/trash.dff` still carried the 55.0 build (165 888 bytes, sha `91a9f84c…`) while `streamed/` had the original (163 840, `7cbd6d81…` = the 3/11 backup): the `.data` the browser preloads was shipping the rejected truck. Restored, and the two trees now compare identical over all 6 022 `gta3.img` files. Gates after the change: `dff_readcheck.py` 8 files 0 errors, `repack_dir.py --mirror` (esperant 85→86 sectors, fbicar 94→93), `check_img_index.py` OK, `stage_bootseed.py` 2 162 files, manifest seal `affbe1fc90f453a02c04cbca01d38e75`, `build.sh` 244/244, `check-served-build.sh` OK with the four new 56.0 markers (172 markers total). |

### Tanda de direccionales del plan 56.0 (08/11/2026) — **REVERTIDA por el jugador**

The player's two captures (mule from behind, fbiranch from the front) killed the
whole indicator batch again, in the same words as 55.0: «**te pedía simplemente
que activaras los focos y las luces intermitentes de cada uno de esos que mencioné,
no que inventaras unos nuevos ni que cambiaras sus formas o tamaños… ya todos
tienen su forma, solo era hacer que se encendiera ese mismo foco y activara su
luz**», and «**revierte todos los cambios de direccionales… solo mantén los del
benson que sí funcionan**». What the captures show is the exact failure mode: the
mule got an indicator *inside a taillight that only has a red stop*, and the
fbiranch got two circles *inside its headlamps* while the giant orange lamps at
the sides — the real ones — were ignored.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| the mod's trucks keep their own lamp shapes; an indicator is an existing lamp that gets powered, not a new part | `present` (rule) — **the 56.0 T3b/T7 application is retracted** | Reverted: the nine DFFs of the batch (fbiranch, boxville, mule, trash, esperant, fbicar, pizzaboy, idaho, stinger) restored byte for byte in every tree where they exist (sha256 recorded in the plan), and `IndicatorLensTable.h` back to the 262 served rows **plus only the benson's two** (264 rows, diff = 2 added / 0 lost). Hard proof the data tree returned to the approved point: the manifest seal is `60a0c0a3f340f41b170e4b6b21982b0d` again, the value the 55.0 revert left. `dataTag`/`VERSION` → `2026-11-08-56.0-T8`. |
| creating a missing rear node, or moving a lens face into a node, is not how the mod lights an indicator | `extended` (data) — **retracted; the capability stays in the tool, unused** | `--crear-faltantes` and the face-moving path remain implemented and verified (structure readcheck, silhouette conservation) but ship nothing: no model gets a new frame or a relocated face unless the player's own capture shows an amber lamp there that does not light. The fbiranch is the case he names as trivial: it is the `rancher` body, whose nodes already work, so the fix is to reuse that vehicle's nodes — not to draw circles inside the headlamp. |
| the sampler fixes and the coach/panzer rules | (kept) | T1/T1b/T1c stay in the tool — they are what makes an existing lens visible without inventing one, and the benson row that survives comes from them. T4 (no first person without interior) and T5 (ground light pool anchored to its lamp) are explicitly kept: «los cambios de cámara y de luces para el autocar y el panzer funcionan». |

What is still owed, in the player's own terms: declare the measured amber for the
models whose lens is already inside its node (boxville rear, pony, rumpo,
securica, spand, topfun, gangbur, manana, cabbie, zebra, enforcer, packer,
rancher, plus mrwhoop/packer extras), cut in place — same silhouette, same
position — the two where the amber is painted inside a panel face (mule front,
fbiranch), and touch nothing where the amber is not an indicator (trash doors,
firetruck clearance lights, esperant tailgate, the 11 damage-skin-only models).

### Lentes de carrocería del recorrido del jugador (plan 58.0, 08/11/2026)

The player hand-audited the fleet and dictated, model by model, which amber lamps
must light and which sides must keep using the head/taillight («si no menciono
cambiar, es para que lo dejes así»). Two of his corrections became rules: the
manana's rear "amber" (67 cm² at 27 % coverage) is a **false positive** — it is the
red stop — and the trash front has no amber at all, so it keeps blinking its
headlamps. Sibling bodies share layouts: `fbiranch` = `rancher`, and
`cabbie` = `kaufman` = `zebra`, so the cabbie's rear fix applies verbatim to the
zebra and the rancher's rear lens is declared so it can serve as the fbiranch's
reference.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| an indicator is an existing lamp that gets powered; never a new part | `present` (rule) | T1 shipped **only** the 28+2 measured rows the player named (rumpo/spand/gangbur/manana/pony front, cabbie/zebra/rancher/boxville rear, enforcer rear two clusters, securica four corners, trash upper small lenses): table 264 → 294 rows, diff = added only, 0 lost, no other model touched. The missing halo on securica/enforcer was exactly the missing row: `ViceExtIndicatorCoronas` only emits when `ViceExtIndicatorLensKind == ODIND_AMBER`, which comes from the table. |
| the mod's derived-atlas swap (`<atlas>_lens`) lights only lens texels | `present` (46.0 mechanism) — **reused for body panels, 58.0 T3** | Six sites whose amber is painted on body panels (esperant rear, boxville front, fbiranch wings/bumper/rear, mule front, trash lower bumper) get derived atlases that differ from the original **only inside the approved footprints** (0 differing texels outside, measured on the installed TXDs); the engine swaps them per material on the body frames (`VICEEXT_BODY_LENS`, `ViceExtSetBodyLensMaterial`, corona per row at its measured centroid). OFF state byte-identical; **0 DFF geometry changes** in that step (sha census). |
| left and right glass sharing one material cannot blink per side | `extended` (data) — **58.0 T4: invisible material split** | Verification caught that the swap lit both sides (and, by row order, none on a left turn). Fixed by splitting the shared material into left/right slots, reassigning only each triangle's material index: **max vertex delta 0.0, 0 triangles added/removed** on all five models, material copies byte-identical (same texture/colour), `dff_readcheck` 0 errors, masks disjoint per side in `BodyLensTable.h`. No shape, size or position change — the player's prohibition holds. Tool: `gta_vc_browser/tools/split_lens_material.py`, idempotent. |
| the detector must not propose what the verdict rule would reject | `extended` (detection) — **58.0 T0** | `indicator_lamp_anchor.py` now labels clusters between the listing floor (0.25) and the declaration rule (0.50 of amber area) as `bajo-umbral`: measured, not declared, for the player's eye. The manana rear false positive is the regression case. |

Gates: `repack_dir --mirror` + `check_img_index` OK, `stage_bootseed` 2 162 files,
manifest seal `3dae55614e877414d010a8011359ea0b`, `VERSION`/`dataTag`
`2026-11-08-58.0-T4`, build exit 0, `check-served-build.sh` 178 OK / 0 missing.
`IndicatorLensTable.h` stayed byte-identical through T3/T4 (294 rows).

### Seis defectos de la prueba en juego (plan 58.1, 09/11/2026)

The player's in-game pass over 58.0 found six defects; five were real and are
fixed, one (fbiranch front) could not be reproduced from the served code and is
awaiting his fresh log.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| an indicator node must contain only its lens | `present` (rule) — **old plans had broken it** | The zebra's `indicator_lr/rr` each held a second atomic: a clone of tiger body paint moved into the node by an old plan, so the node's material mask lit a patch of the rear door (the player's "extra directional"). Fixed by re-parenting that atomic to the clump root with its world matrix preserved (the 3 triangles no longer existed in the body, so this restores paint with no hole): vertex delta 0.0, world-triangle union 0/0, nodes now hold only the `cabbie8bit128` lens, same count/placement as the kaufman. |
| a lens whose texture is only partly amber must still light as one piece when the player says so | `extended` (look, player-signed) | rumpo/pony (same van) front lenses lit only "a thread" because the amber texels are a strip inside the face. With `atlas=1` and a derived atlas brightening the **whole face footprint** (62 texels; 0 differing texels outside; front footprint disjoint from rear), the entire lens face lights as he asked. topfun's front got the same treatment plus its two missing node rows. |
| a red stop must never be declared amber | `present` (rule) — **58.0 T4 had broken it** | The trash body row pointed at the mid tailgate (44 % "amber" measured over a red stop). Deleted; the real four rear indicators (tiny top + low bumper) live in `indicator_lr/rr` and are painted by the untouched node row. The derived trash atlas was removed from the TXD. |
| body-panel amber above the headlight | `extended` (data) | mrwhoop front: chassis_hi material split per side (441/413 tris), two body rows (88 cm², 69 %), derived atlas limited to the amber texels of those faces. |
| fbiranch front | (open) | No change: its six body rows, per-side masks and derived atlas are present in the served build and `ViceExtSideHasAmberLens` counts them, so the headlight fallback should be off. The player's capture predates or contradicts that; diagnosis needs his session log with the `BODYLENS model=` trace. |

Gates: only zebra.dff and mrwhoop.dff changed (both vertex-identical), 5 TXDs
touched with OFF byte-identical and 0 differing texels outside footprints,
`dff_readcheck` 0 errors, tables 296 node rows / 14 body rows, manifest seal
`c9d7c5326f923878e9abdb8b049541c1`, VERSION/dataTag `2026-11-09-58.1-T1`, build 0
errors, check-served-build OK including the concurrent 57.0 track's markers
(that track was not touched).

<!-- USER-END -->

### Cara entera de la lente: material exclusivo medido en runtime (plan 59.0, 09/11/2026)

The player's list for this round was the six models whose indicator lights only
a triangle of its lens face, plus fbiranch blinking its headlight. 59.0 was
written by another agent with the atlas-coverage fix already inside; the audit
run before executing it found that fix could not work, and the shipped port
uses a different mechanism. Everything below was measured on the served
`streamed/models/gta3.img/*.dff|txd` before anything was applied.

| mod construct (shipped evidence) | class | port |
|---|---|---|
| a lens face split between the `indicator_*` node and the body (`chassis_hi`, `wing_*`, `bump_*`) | `present` (rule) | Confirmed and quantified: rumpo/pony/topfun front lens is one UV rectangle (0.89–0.984, 0.558–0.611) made of exactly two triangles — one in the node, one in `chassis_hi` material 5/6. The node half lights, the body half stays dark: that is the "hilo de color" he saw. |
| light the body half by painting the derived `<tex>_lens` atlas over the whole face | **rejected by measurement** | The same rectangle is sampled by 12 `chassis_hi` faces of the **bonnet** material and by 2 `extra2` faces. Painting those texels lights the car's paint while signalling — the exact invention the player has rejected twice. `gen_lens_coverage.py` (59.0's T1/T3 tool) was rewritten to refuse it: a texel is painted only when no foreign face samples it, the derived atlas is emitted **uncompressed ARGB8888** (DXT1 4×4 blocks moved up to 66/255 on 145–148 texels outside the lens, which its own `fuera==0` gate caught — and the old tool also emitted the TXD through `gen_lens_atlas.build_txd`, which writes a one-texture dictionary), and only `trash868bit128_lens` survived the filter (28 texels of the rear lenses). |
| a lens material the engine may tint without touching the paint | `translated` (no binary counterpart — the mod ships its own models, so it never needed the test) | The engine now measures, per body row, whether **every** triangle of the row's material inside the resolved frame falls within 0.35 m of **any** amber lens of that model (`ViceExtBodyLensExclusiveCB`, run once in `SetupModelNodes`). Only then does the body route raise the ambient to the measured amber exactly like the node route; otherwise it stays texture-only. Cold replay of the rule: rumpo 4/4, pony 4/4, topfun 4/4, cabbie 2/2, zebra 2/2, securica 2/2, enforcer 2/2, manana 4/4, spand 6/6, rancher 4/4 tint; boxville 1/6, fbiranch 2/8, esperant 0/4, mrwhoop 0/6, mule 0/6, trash 0/5 stay texture-only because their lens material is the panel's. |
| fbiranch blinks its white headlight instead of its own lenses | `translated` (detection) | The 58.2 nameless fallback compared the **material centroid** to the row position at 0.35 m; that material is spread over the whole truck (measured 3.02 m away for the rear rows), so at best 2 of 6 rows resolved — matching the player's `filas=2`. Replaced by the per-triangle rule (any triangle of the masked material within 0.25 m, `_dam` frames excluded, most-hits frame wins); all 69 body rows now resolve to the frame the table declares, and with the rows resolved `ViceExtSideHasAmberLens` is true on all four sides, so the headlight fallback is off. Proved only by his `BODYLENSROW … ok tint=` and `AMBERGATE … lf=1 rf=1` lines. |
| gangbur must behave like pony | `extended` (player-signed) | Same 46 frame names as pony but a different variant: its lens faces are already whole quads inside the nodes (8 front, 2 rear triangles, 76 % amber) and it has **no** rear amber lens anywhere (only `door_lr_hi_ok` markers). It was the one scope model with no derived atlas and `atlas=0`, so nothing tinted it. Its two front rows now carry the same tint flag pony/rumpo/topfun carry, with their already-measured amber `(192,116,26)`. No new row, no geometry. |
| rows the other agent added outside the frozen list | retracted | 8 rows deleted (zebra/cabbie `panel_lf`/`panel_rf`, enforcer `panel_lf`/`panel_rf` + `wing_lf_hi_ok`/`wing_rf_hi_ok`): the player's scope keeps those fronts untouched, and per-triangle resolution sent two of them to `wing_*_hi_ok` and two to `indicator_*` — the table said one frame, the engine would have painted another. 69 rows left, all resolving to their declared frame, none mixing sides in one material. |

Gates: no DFF touched; one TXD (`trash.txd`) rewritten with 9 textures intact,
`fuera=0`, alpha unchanged and the other 8 textures byte-identical;
`repack_dir --mirror` fixed 12 stale `gta3.dir` entries (6 TXDs grew sectors),
`check_img_index` OK with the copies synchronized; tables 296 node rows /
69 body rows; manifest seal `bdaa80dc6585c7edf7614500bbac4f16` (17488 entries);
VERSION/dataTag `2026-11-09-59.2-T1` (59.1 was taken by the concurrent 57.0
touch track); `build.sh` 0 errors; `check-served-build.sh` OK with the four new
markers `BODYLENSROW`, `AMBERGATE`, `ViceExtBodyLensExclusiveCB` and the dataTag.
Pending: the player's in-game pass (T5).

## 62.0 (2026-11-09) - la firma de 59.0 nunca llego: lo que midio su auditoria

El jugador no firmo el T5 del 59.0; volvio con cuatro defectos nuevos. Su lista es
la especificacion y cada fila below esta medida sobre el arbol servido
(`lens_footprint.py` / `body_lens_audit.py`), no inferida del codigo compilado.

| construct of the port (measured) | class | evidence and resolution |
|---|---|---|
| the derived atlas footprint as a band of rows | **gap, closed** | mrwhoop: 1180 texelas cambiadas sobre bbox (0,64)-(127,95) cuando la lente real son 186 en (0,70)-(32,80); fbiranch `fbirancher86body8bit64_lens`: 1998 texelas = 49 % del atlas con **0** caras ambar. El swap de textura alcanzaba a 51+49 tris de pintura, 20 de bajos y los parachoques: eso es el faro blanco y el piloto rojo amarilleando. Cerrado dando al cristal slot propio (ADR-056), no recortando la banda. |
| a body row whose material has no amber face at all | **excluded (invention)** | 18 filas borradas: mrwhoop `mat9/mat10` y sus dos traseras, fbiranch `chassis_hi mat9/mat10`, boxville `mat7/mat10` y sus dos traseras, rumpo/pony/topfun `LR/RR`, mule `LR/RR`. Medida: 0 caras de cristal a <= 0,12 m del xyz declarado. |
| declaring the rear lens to make the front `tint` gate pass | **excluded (invention, fuera de lista)** | `chassis_hi mat5` del rumpo guarda 1 tri delantero y 2 traseros: la puerta 59.0 exigia centros en ambos ejes. El jugador no pidio la trasera ("deberia solo ser su stop trasero default"). Con el slot por cuadrante la delantera pasa sola y la trasera vuelve al piloto rojo. |
| the dark half of the glass rectangle | `translated` (regla nueva, medida) | Tres caras de `chassis_hi` (mat0 x2 + mat5) con 21-30 % de texelas ambar: son cristal dibujado oscuro, no marco (verificado en el mapa UV). `is_lens_face` las rechaza, asi que el split las toma por `fr >= 20 %` anclado a la fila declarada, y el ambient (no el color) es lo que las aclara. |
| washing vs fbicar ("same car, same sites") | `present` (regla del jugador, confirmada) | `washing` tiene 12 filas de nodo y 0 caras ambar en carroceria; `fbicar` tiene 0 filas en las dos tablas, sus nodos `indicator_lf/rf` con 19 tri de pintura y **0** ambar, y 26 caras ambar en `chassis_hi mat0` en xyz identico (∓0,89; +2,40; −0,19 / ∓0,84; −2,97; −0,28). Declarado con 4 filas de carroceria en esos sitios y su cristal partido: `EXCL 6/0` y `8/0 tint=1`. |
| mule front lens | `gap`, declarado y no cerrado | Su lente es una sub-zona de UNA cara de 462 cm² (43,6 % ambar). Partir el material encendería el panel; cortar la cara es geometria nueva, prohibida en el alcance. Se deja en la ruta del atlas y se informa. |
| zebra/cabbie `panel_lf/rf`, enforcer `panel_*`, trash `chassis_hi mat6/7` | `gap`, fuera de alcance | `gen_lens_coverage.py --check` los lista como "caras de lente en carroceria SIN fila que las declare (quedan oscuras)". No estan en la lista del 62.0; van con su tanda. |

Gates del 62.0: 8 DFF partidos con delta de vertices 0,0, union de triangulos
identica y OFF byte-identico (`split_lens_material.py` + `dff_readcheck.py`, 0
errores); tablas 296 filas de nodo / 55 de carroceria; `repack_dir` +
`check_img_index` OK con las copias sincronizadas (esperant y pony subieron un
sector); sello `98595b49b3942f92cf65544b3bf53cb2`; VERSION y dataTag
`2026-11-09-62.0-T1`; `build.sh` 0 errores; `check-served-build.sh` 186 OK.
Pendiente: la firma en juego (T6).

## 62.1 (2026-11-09) - la firma parcial del 62.0

| construct of the port | class | evidence and resolution |
|---|---|---|
| the node ambient raised for every non-body material of an `indicator_*` node, row or not | **gap, closed** | Medido: es la unica causa de los dos defectos que quedaban. mrwhoop `indicator_lf/rf` guarda el faro blanco y no tiene fila ambar (su lente delantera es fila de carroceria); rumpo/pony/topfun `indicator_lr/rr` guardan 1 cara de cristal + 1 alojamiento de 220-246 cm² y tampoco tienen fila. Cerrado con ADR-057: sin `ODIND_AMBER` el nodo no se toca. Parque medido: 0 modelos con nodos indicator se quedan sin nodo de lampara, 0 motos con nodos indicator. |
| fbicar declarado con 4 filas de carroceria y su cristal partido | **retracted por el jugador** | Su palabra: «se ve fatal, puras invenciones, formas enormes que alumbran blanco… deshaz los cambios». Medido por que: el criterio `fr >= 20 %` del 62.0 dio LF 408 cm² y LR 257 cm² frente a los 228/90 cm² del `washing` que decia imitar. DFF restaurado al sha del respaldo y 4 filas fuera. Queda pendiente copiar el reparto del washing con una medida que de las mismas areas. |
| front glass of rumpo/pony/topfun | abierto, con traza | El slot exclusivo resuelve `tint=1` en la replica offline (20 filas en verde tras quitar fbicar), pero el jugador sigue viendo «solo un pedazo». No se cambia nada a ciegas: la puerta es su `BODYLENSROW … excl=3/0 tint=1 ok` y su captura. |

Gates del 62.1: fbicar restaurado (sha `3c187333b79c`, igual que el respaldo) en
streamed y gamefiles; tabla de carrocería 51 filas; sello `5e5c8cd3c6395fc7a8d205a4e803a269`;
VERSION y dataTag `2026-11-09-62.1-T1`; `build.sh` 0 errores;
`check-served-build.sh` OK. Pendiente: la firma en juego.

## 62.2 (2026-11-09) - el criterio de cristal medido mal y el fbicar copiado del washing

| construct of the port | class | evidence and resolution |
|---|---|---|
| "glass face" decided by the share of amber texels (>= 20 %) | **gap, closed** | Depende del brillo con el que esta pintada la lente, no de si es lente. Medido en pony: las cuatro caras del rectangulo tienen saturacion 0,85-0,87 y la misma curva de matiz; las tres del chasis se caian por `r >= 105` y `g - b >= 30` de `_is_amber` (colores (95,41,14), (87,36,11), (90,38,12)). Y por el mismo criterio entraba la cara de bajos `chassis_vlo` de 1034,5 cm2 con sat 0,22, que es la «forma enorme que alumbra blanco» del fbicar. Cerrado con matiz 15..55 + saturacion >= 0,60 (`is_any_lens_face`), una sola regla en `indicator_lens45.py`. |
| pony / topfun front lens | `translated` | De 6 a 8 caras (una mas por lado) y de 3 a 4 caras de cristal por direccional; esperant de 24 a 28 (sus alas de 2 a 4 caras, 43 -> 81 cm2). rumpo, mrwhoop, boxville y fbiranch: identicos, no se volvieron a partir por criterio, solo se restauraron y partieron una vez. |
| fbicar must be the washing | `present` (regla del jugador, confirmada al dígito) | Con el criterio nuevo, en los diez sitios del washing: LF/RF 6 caras / 407,8 cm2 y LR/RR 7 caras / 228,0 cm2 **en los dos modelos**, areas identicas cara a cara. La unica diferencia es el frame: `indicator_lf/lr` en el washing, `chassis_hi mat0` en el fbicar. Declarado con 4 filas de carrocería y su cristal partido; el washing no se toco. |
| the chrome frame around the strip | **excluded (no es cristal)** | Saturacion 0,00-0,04 (grises (74,74,74), (112,112,112)). No se pinto: iluminarlo es pintar plastico. Queda como decision del jugador si aun con la cuarta cara dentro quiere el marco encendido. |
| masks written with `sum(1 << mi)` per face instead of per distinct material | **gap, closed in-run** | Primera pasada de la reescritura salio con mascaras infladas (fbicar LF 0x600 con mats [8]). Detectado por `body_lens_audit.py` (tris=13 en una fila de 6 caras) y corregido antes de la cadena: la mascara es `sum(1 << i for i in {mis})`. |

Gates del 62.2: 8 DFF restaurados a su sha pre-split y vueltos a partir (delta de
vertices 0,0, triangulos mundo identicos, `dff_readcheck` 0 errores en los 8);
24 filas de carrocería con mascara recalculada, todas `EXCL n/0 tint=1` en la
replica offline; `repack_dir --mirror` 0 sectores; sello `4275142e0e9a005a755148b850c618a7`;
VERSION y dataTag `2026-11-09-62.2-T2`. Pendiente: la firma en juego.

## 62.3 (2026-11-10) - la huella del derivado y la corona de las filas de carroceria

| construct of the port | class | evidence and resolution |
|---|---|---|
| derived-atlas footprint taken from the node cluster only | **gap, closed** | Medido cara a cara: de las 4 caras del cristal delantero, el derivado cubria 100/96/78/**35** % (rumpo), 100/98/76/**22** % (pony) y 100/93/78/**35** % (topfun). El tinte no puede aclarar un texel oscuro, asi que esa fraccion quedaba negra: «solo pinta una pequena fraccion». Cerrado pintando la union de las caras que **declara la fila** (527 texelas, `fuera=0`), y las 4 caras quedan al 100 % medido sobre el TXD servido. |
| body faces joined to a lens by coplanarity + distance to the node | `translated` (regla cambiada) | De las 3 caras de chasis del rumpo solo 1 pasaba el test geometrico. El grupo de una lente es ahora lo que declara su fila de `BodyLensTable.h`: el mismo criterio que usa el motor. `cuerpo` 1 -> 3. |
| texels discarded because a non-lens face samples them | **excluded (guarda obsoleta)** | Tras el split la pintura no ve el derivado: el swap es por material. El descarte se aplica solo a caras cuyo material si se cambia y no es lente de esa fila (alojamiento dentro del nodo; 12 texelas del esperant). |
| a body row emits no corona because `r.coverage` | **gap, closed** | El aro de luz del fbicar no salia: las 4 filas del fbicar llevan `coverage=1` y `ViceExtBodyLensCoronas` las saltaba. La regla correcta es por lente, no por marca: se salta la fila de carroceria solo si el nodo de ese `node`+`side` tiene ya lente ambar declarada. fbicar gana sus 4 aros; rumpo/pony/topfun/esperant no duplican (verificado: esperant tiene fila de nodo LF/RF ambar). |
| fbiranch derived atlas | `gap`, declarado y no cerrado | El generador se nego a reescribirlo: `STOP: fbirancher868bit128_lens cambio el canal alfa`. Es su propia compuerta; no se forzo. Queda como pendiente con su motivo. |

Gates del 62.3: 4 TXD reescritos con `fuera=0`, alfa intacto y las demas
texturas byte a byte (rumpo 77->105, pony 69->97, topfun 34->62, esperant
29->57 sectores; +0,2 MB); `repack_dir --mirror` + `check_img_index` OK;
sello `4334eacb5f53df5e2977901f7e9cdc72`; VERSION y dataTag `2026-11-09-62.3-T2`;
`build.sh` 0 errores; `check-served-build.sh` OK. Cero DFF tocados en esta tanda.
Pendiente: la firma en juego.
