---
name: 035-occupant-hit-detection-is-gated-by-the-glass-the-bullet-crosses
status: accepted
date: 2026-11-03
domain: combat
supersedes: []
---

# ADR-035: an occupant is hittable through any glass the bullet actually crosses

## Context

The player reported, after session T101: "no puedo matar a los NPCs dentro del
vehículo si les disparo al pecho, torso o cualquier parte de su cuerpo estando
en el vehículo; solo mueren dentro del vehículo si les disparo a la cabeza.
Debería poder ser a cualquier parte del cuerpo mientras la bala entre en el
ángulo".

`CWeapon::CheckForShootingVehicleOccupant()` (`src/weapons/Weapon.cpp`) tests two
spheres per occupant: the head (`PEDPIECE_HEAD`, r = 0.2) always, and the torso
(`PEDPIECE_TORSO`, r = 0.25 at `PED_MID`) only when `passesGlass` is true. In the
port `passesGlass` was computed as:

```
if (veh->IsCar()) {
    if (DotProduct(distVec, veh->GetForward()) < 0.0f && DotProduct(distVec, veh->GetUp()) <= 0.0f) {
        ... loop over SURFACE_GLASS triangles ...
    }
}
```

The outer test is a directional cone: the shot must travel backwards *and*
downwards in vehicle space. Only a ray coming from in front and above can ever
set `passesGlass`, so a shot through a side window or the rear windscreen —
which is exactly what happens when you stand next to a car and fire at the
driver — never reaches the triangle loop and the torso stays invulnerable. The
head sphere is above the glass line and keeps registering, which is the asymmetry
the player noticed.

Two facts make the pre-gate unnecessary rather than merely wrong:

- It is a port-local addition. The upstream reVC shape tests the head sphere and
  gates the torso on nothing; the glass test is ours.
- The triangle loop it guards is already cheap. `CCollision::CalculateTrianglePlanes`
  is LRU-cached per `CColModel`, and the loop runs over `numTriangles` of one
  vehicle's col model only when a bullet has already hit that vehicle.

## Decision

1. **Geometry decides, not direction.** The cone is gone. For any car, the shot
   line is transformed into vehicle space and tested against every
   `SURFACE_GLASS` triangle; if it crosses one, the torso is live. Sheet metal
   still stops the shot, because metal triangles are not `SURFACE_GLASS`.
2. **The windscreen keeps its own, narrower criterion.** The cone survived as a
   separate `frontGlass` flag, used only to decide whether to crack
   `VEHPANEL_WINDSCREEN`: a bullet that came through a side window damages the
   occupant but must not pop the windscreen.
3. **Limbs stay out of scope.** The function still tests head and torso spheres
   only. Adding the four limb spheres per occupant would be a different, larger
   change to in-car hit detection, and the player's report is about the torso.

## Consequences

- Shooting occupants through side and rear glass now works from the angle the
  player described. Damage per shot is unchanged: the same weapon damage applies
  to a torso hit as to any other torso hit.
- The glass test now runs on every bullet that hits a car, not only on the
  front-lower cone. The cost is one `Invert`, one line-triangle loop over a
  single col model, and a cached plane set; `ODSHORT`/`FPSLOG` in the next
  session is the check that this did not move frame time.
- A bullet that crosses *no* glass but hits the head sphere still registers
  (unchanged from before), so an open-top car keeps working.
- A torso hit through non-front glass no longer cracks the windscreen, because
  `frontGlass` gates the panel damage.
- `accepted` means the decision is taken and it compiles; the behaviour has not
  been observed in a browser.

## Alternatives considered

- **Widen the cone (e.g. drop only the `GetUp()` term).** Rejected: it keeps an
  approximation of a question the col model already answers exactly, and it still
  fails for a shot fired along the car's axis from behind.
- **Gate the torso on "the vehicle has any glass panel at all".** Rejected: it
  would make occupants fully hittable through sheet metal.
- **Test limbs as well.** Rejected for now (clause 3): more spheres per occupant
  per bullet for a benefit the player did not ask for.

## State

`accepted` (2026-11-03). Verification is the player's next session: a torso shot
through a side window should kill an NPC driver, and the windscreen should stay
intact when the bullet came through that side window.
