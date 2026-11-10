---
name: 054-menu-enum-and-ascreens-table-must-stay-aligned
status: accepted
date: 2026-11-07
domain: reVC-WASM-web-y-menus
supersedes: []
---

# ADR-054: `eMenuScreen` and `aScreens[]` are one indexed mapping, enforced at compile time

## Context

`CMenuManager` reads screens as `aScreens[m_nCurrScreen]`, where `m_nCurrScreen` is an
`eMenuScreen` value (`src/core/Frontend.h`, `src/core/Frontend.cpp`). There is no name
lookup and no bounds check: the enum's numeric value *is* the array index. Every
conditional screen must therefore add one enum value **and** one array entry, under the
**same** feature guard, at the **same** position.

Plan 29.0 (30/10) removed the WFP "Ajustes de pantalla" screen at the player's request.
It disabled `VICEEXT_WFP_MENU` in `src/core/config.h` (which guards both the `FET_WSF`
row and the `FET_WSF` screen entry), but left `MENUPAGE_WIDEFIX_SETTINGS` in the enum
under `VICEEXT_WFP_OPTIONS` — a different guard that stays active because it also
controls reading the WFP INI options. This silently produced `MENUPAGES = 57` against
56 array entries. It was harmless while the phantom value was last, because no page was
ever routed to it.

Plan 43.0 appended six new screens after that phantom value. From index 50 onward every
enum value pointed at the *next* array entry. The player's session (`odtrace-2026-11-07_19-51-05.log`,
`FEMENU scr=53` repeated) showed "Galería" opening the delete-photos confirmation,
"Gestionar música" opening the delete-songs confirmation, and "Gestionar partidas"
opening "no hay espacio"; `m_PreviousPage` pointed back into the same displaced page, so
GoBack looped forever and forced a tab reload.

Because `aScreens` was declared `extern CMenuScreenCustom aScreens[MENUPAGES]`, a short
initializer list is simply zero-filled: the compiler cannot see the mismatch and there
is no runtime error. A verification script that counted *source lines* rather than the
preprocessed result also missed it.

## Decision

1. Declare `extern CMenuScreenCustom aScreens[]` (unknown bound) in `Frontend.h`, so that
   in the translation unit containing the definition `sizeof(aScreens)` reflects the
   real number of initializers. Other translation units only index the array, which is
   legal with an incomplete bound; no code uses `sizeof(aScreens)` elsewhere.
2. Add, immediately after the `aScreens[]` definition in `MenuScreensCustom.cpp`:
   `static_assert(sizeof(aScreens)/sizeof(aScreens[0]) == MENUPAGES, ...)`. Any future
   enum/table divergence becomes a **build error**, not a runtime menu bug.
3. Align the guard of `MENUPAGE_WIDEFIX_SETTINGS` to `VICEEXT_WFP_MENU`, matching its
   screen entry. Its only code reference (`MenuScreensCustom.cpp`, the `FET_WSF` row) is
   already under the same guard, so no dangling reference remains.
4. Treat "count the enum and the table under the real preprocessor" as the required
   verification for any change to the menu tables; a source-text count is not evidence.

A separate, independent defect found in the same session: `MENUACTION_WEB_MUSIC_DELETE`
called `window.__vcMusicDeleteAll()` through `EM_ASM`, which called
`Module._ViceExtRescanUserMusic()` synchronously — a WASM -> JS -> WASM re-entry inside
one frame, incompatible with `-sASYNCIFY=1`. `deleteAllMusic()` now schedules the
rescan with `setTimeout(rescanUserMusic, 0)`, outside the engine frame.

## Consequences

- The three native pages now open the page that was requested, "No" and "Volver" return
  to Configuración extra / Sonido / Iniciar partida, and "Sí" no longer crashes.
- Every future menu-page addition must touch both the enum and the table under identical
  guards; forgetting either fails `build.sh` with the static_assert message.
- Using an incomplete array bound for `aScreens` means any future `sizeof(aScreens)` in
  another translation unit will not compile; that is intentional — the count belongs to
  the defining TU only.
- `VERSION` rose to `2026-11-07-43.0-T2`; `dataTag` stayed `2026-11-07-43.0-T1` because
  no served data (the 14 GXT labels) changed.

## Alternatives Considered

- Adding a seventh dummy `{ "", 0, nil, nil, }` array entry for the phantom enum value:
  rejected — it would preserve a dead screen id and still let guards drift apart.
- Guarding the enum value with `VICEEXT_WFP_OPTIONS` and re-enabling `VICEEXT_WFP_MENU`:
  rejected — the player explicitly asked to remove that WFP screen in plan 29.0.
- A runtime check in `CMenuManager`: rejected — the failure mode is a wrong page and a
  navigation loop; a compile-time failure is cheaper and unambiguous.
- Keeping `extern ... aScreens[MENUPAGES]` and relying on the verification script:
  rejected — that combination is exactly what let the defect through.

## State

`accepted`
