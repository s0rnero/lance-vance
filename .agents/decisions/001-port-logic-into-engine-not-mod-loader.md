---
name: 001-port-logic-into-engine-not-mod-loader
status: accepted
date: 2026-09-27
domain: architecture
---

# ADR-001: Integrate community mods by porting their logic into the engine, not with a mod loader

## Context

The project integrates Vice Extended and other community mods (SilentPatch,
FramerateVigilante, WidescreenFixesPack, ClassicAxis, SkyGfx, climbing, GInput…).
These mods patch the **original x86 Win32 binary** by writing at fixed memory
addresses (`WriteMemory(0x005AF238, …)`) or by scanning byte patterns
(`hook::pattern`), and ship as ASI DLLs, `.ual` patches, CLEO scripts and D3D8/9
shims. This engine is reVC **compiled from source to WebAssembly**: there is no
x86 address space and no native DLL loader.

## Decision

Do **not** build a mod loader and do **not** use the mods' binary patches. Port
their **logic, specs and data** into the engine's own classes, behind `VICEEXT_*`
defines. What is taken from a mod is:

- the **knowledge** (which line is wrong and why, with `file:line` from the mod),
- the **corrected logic / algorithm**, and
- when useful, its **data** (tables, INI values), regenerated into served data.

What is never used, because it is technically inapplicable here: x86 addresses,
`hook::pattern`, `.asi`, `.ual`, `rwd3d9`, `injector`, `plugin-sdk`, and CLEO
scripts whose logic is `READ_MEMORY`/`WRITE_MEMORY`/`CALL_FUNCTION` on binary offsets.

## Consequences

- Changes are plain source edits: shorter, clearer, and they survive recompilation.
- Ported features can be toggled and validated individually (traces + verifiers).
- Some mod behaviour tied to the binary (exact camera feel, D3D pipelines) must be
  reimplemented by understanding it, not by copying it.

## Alternatives Considered

- **Mod loader / plugin host**: would have to replicate the original ABI (MSVC 2003
  VTables, register state) inside WASM. Enormous work for zero benefit.
- **Inject the ASI/`.cs` as-is**: impossible (no PE loader, no x86 addresses in WASM).

## State

`accepted`.
