---
name: 052-native-menu-pages-call-js-through-window-hooks
status: accepted
date: 2026-11-07
domain: reVC-WASM-web-y-menus
supersedes: []
---

# ADR-052: the three data managers are native menu pages that call JS through window hooks

## Context

Plan 43.0 originally described a DOM panel for the gallery. On 06/11 the player corrected
this: gallery, music and saves must be **pages of the in-game pause menu**, with the same
look and navigation, not separate DOM overlays. The plan's existing overlays
(`web/lib/gallery.js` grid, `web/lib/saves.js:155`) already worked from the shell console,
so duplicating their logic inside C++ would violate the zero-duplication rule
(RULES 0.6) and would leave two UIs to maintain.

The engine's `aScreens[]` table (`src/core/MenuScreensCustom.cpp`) is indexed directly by
the `eMenuScreen` enum (`aScreens[m_nCurrScreen]`), so any new page has to be appended at
the tail of **both** the enum and the table, in the same order, under the same feature
define. `MENUACTION_YES` and `MENUACTION_NO` only call
`SwitchToNewScreen(m_TargetMenu)` (`src/core/Frontend.cpp:5645-5648`) and therefore cannot
carry an action of their own; the plan's earlier note that a confirmation page could reuse
`MENUACTION_YES` was wrong.

## Decision

Three new feature defines `VICEEXT_WEB_GALLERY`, `VICEEXT_WEB_MUSIC_MANAGER`,
`VICEEXT_WEB_SAVE_MANAGER` guard six new enum values, six tail entries of `aScreens[]`,
nine new actions and the `m_bWebSaveDownload` flag.

- Entry rows are `MENUACTION_CHANGEMENU` from Configuracion extra (`MENUPAGE_OTHER_SETTINGS`),
  Sonido (`MENUPAGE_SOUND_SETTINGS`) and Iniciar partida (`MENUPAGE_NEW_GAME`).
- Each management page's rows call `EM_ASM` into `window.__vc*` hooks published by
  `web/lib/index.js`. The hooks reuse the very same functions as the DOM overlays; no
  filesystem, ZIP or slot logic was copied into C++.
- The two delete-confirmation pages use `MENUACTION_LABEL` for the question, `MENUACTION_NO`
  for "No", and a **custom action** (`MENUACTION_WEB_GALLERY_DELETE` /
  `MENUACTION_WEB_MUSIC_DELETE`) on the "Si" row. Only that row performs the deletion.
- "Descargar partida" sets `m_bWebSaveDownload`, switches to the existing native
  `MENUPAGE_CHOOSE_LOAD_SLOT` screen, and the `MENUACTION_CHECKSAVE` branch downloads the
  chosen `GTAVCsfN.b` without loading it, then returns to Gestionar partidas. The flag is
  cleared there and on GoBack, so normal "Cargar partida" is untouched.
- All 14 new labels live only in the six GXT pairs (`streamed/TEXT`, `bootseed/TEXT`),
  byte-identical pairwise. No raw string is drawn from C++ or the shell. The player
  measured that the menu renders accents and rare characters badly, so every label is
  plain ASCII (including a transliterated `GALEREYA` for Russian), and `FEM_YES`/`FEM_NO`
  are reused instead of being re-added. Because `gxt_menu_keys.py` enforces `MAXKEY = 7`,
  the planned `FESZ_QGF`/`FESZ_QGM` were shortened to `FES_QGF`/`FES_QGM`.

## Consequences

- The player sees one menu, in the game's own look, in all six languages. Engine and web
  each keep one implementation of each operation.
- The native pages and the DOM overlays now share hooks; the overlays remain reachable from
  the console but are not opened by native menu rows.
- Adding another page later requires appending to the enum and `aScreens[]` together; a
  mismatch would silently read the wrong screen. This was verified by counting both under
  the active defines.
- `check-served-build.sh` gained marks for the new pages, the confirmation key, the rescan
  export and the hooks, because the relinked package is the only proof that the menus
  really shipped.

## Alternatives Considered

- DOM-only panels opened by a native row: rejected by the player (06/11) and would have
  shown two UIs at once.
- Drawing the labels with hardcoded C strings: rejected by restriction 12 (GXT only) and
  by the player's rendering measurement.
- Putting the delete action on `MENUACTION_YES`: rejected because that action only switches
  screens; it would have asked the question and then deleted nothing.

## State

`accepted`
