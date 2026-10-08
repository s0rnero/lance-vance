---
name: 10-plan-pasada-siguiente
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-24
---

# 10 — Plan de la PASADA SIGUIENTE (fase 1: extracción/documentación, solo lectura)

> **ESTADO: LISTO PARA ARRANCAR — esperando el aviso del jugador. NO ejecutar
> nada hasta que diga «arrancar».**
>
> Naturaleza: continuación de la fase 1 (decodificar, verificar, clasificar,
> documentar). **Cero ficheros del juego modificados**: ni `src/`, ni
> `streamed/`, ni `bootseed/`, ni `gamefiles/`, ni `web/`. Salidas solo a
> `/tmp` y a los MD. Única escritura permitida: herramientas de análisis nuevas
> en `gta_vc_browser/tools/` (son nuestras) y los MD de planes.
>
> Contenido: los ítems PENDIENTES de la lista literal (`09` §3: A3-A5, B7-B10,
> C11-C13) + los añadidos de cierre (`09` §5: E18-E24). Los spikes D14-D15 ya
> están AUTORIZADOS (`09` §4) y van aparte (§Spikes). D16-D17 no entran (fase 2).

## Alcance prohibido (explícito)

- ❌ Escribir en cualquier fichero del juego o de datos.
- ❌ Compilar, enlazar, subir `VERSION`/`dataTag` (eso es D17/fase 2).
- ❌ Ejecutar D16/D17.
- ⚠️ D14/D15 solo como spike reversible con la condición del jugador (`09` §4):
  **al terminar, el código queda como estaba**; sobrevive solo lo documentado.

## Herramientas disponibles (no reinventar)

`gta_vc_browser/tools/`: `gxt_inspect.py`, `ifp_inspect.py`, `txd_inspect.py`,
`viceex-strings.py`, `check_ide_gxt.py`, `import_img.py`, `repack_dir.py`,
`import_mvl_vehicles.py` (oráculo MVL). Reutilizar también `/tmp/ve-audit/`
(`img_audit.py`, `txd_audit2.py`, `gxt_keys.py`, `ifp_names.py`, `scan_ids.py`)
y `/tmp/cbd6c006/diffs/`. Formatos ya decodificados: TKEY/TDAT (`08` §A1), TXD
D3D8/D3D9 (`vice-extended-inclusion.md` §2.7), bytecode SCM (`05` §2), chunks
DFF (`10-seccion-1-9a-partida.md`).

---

## Tanda 0 · Cierre de la pasada 08 (E18-E20) — coste BAJO

- **E18 · IDs `6670/6671`**: mapa de rangos de IDs (nuestro `default.ide` servido
  vs `default.ide` + `newVehicles.ide` del mod): 6500-6507 (coches), 6600-6619
  (wheels), 6620-6659 (vehmods), 6660-6699 (weapons), nuestros `bryx_lights`/
  `lodbryx_lights` 6670/6671. Entregable: tabla «rango → uso → ¿solape?» +
  propuesta de rango libre si choca. Cierre: cero dudas sobre 6670/6671.
- **E19 · `fronten2.txd` (+3 MB)**: `txd_inspect.py` sobre servido y mod,
  textura por textura (niveles, formato, compresión) → explicar la sobrecarga.
  La validación visual queda anotada para fase 2. Cierre: la diferencia de bytes
  explicada con número.
- **E20 · merge «6500-6507 sin duplicar»**: correspondencia 1:1 campo a campo
  entre nuestras defs inline y el `newVehicles.ide` del mod (oráculo:
  `import_mvl_vehicles.py` + su `output.txt`) → regla de merge inequívoca.

## Tanda A · Datos (A3-A5)

- **A3 · Contenido interno de los 8 `cdimages/*.img` entrada a entrada**:
  listar cada `.dir` del overlay y comparar **contenido** con lo servido
  (no solo nombre): igual / distinto / no servido, por imagen
  (anims, generic, objects, peds, player, radar, vehicles, weapons). Base parcial:
  `vice-extended-inclusion.md` §2.1-2.2. Entregable: tabla entrada→entrada.
  Cierre: el 100 % de las entradas clasificadas.
- **A4 · `GunCrouchFwd/Bwd` + set de crouchfire**: extraer los 2 clips del
  `ped.ifp` de sa-crouch a `/tmp` (ANPK) y medir con `ifp_inspect.py`; y decidir
  el set de crouchfire con criterio escrito (sirven duraciones distintas:
  servido 0,863 s vs ClassicAXIS 0,631 s en `RIFLE_crouchfire`) — fidelidad al
  mod vs continuidad con lo servido. Cierre: decisión documentada.
- **A5 · `ViceEx.RAW/SDT` vs nuestro audio + `gamecontrollerdb.txt`**: completar
  el mapeo de las 13 muestras del banco del mod (parejas L/R ya medidas en
  `vice-extended-inclusion.md` §8.F) frente a nuestro `sfx` servido → qué
  muestras nuevas aportaría y a qué armas; y extraer del `gamecontrollerdb.txt`
  (253 KB) los mapeos útiles para nuestra Gamepad API (cruce con los 5 setups
  GInput de `04` §9). Cierre: mapeo audible propuesto + tabla de mapeos.

## Tanda B · Código externo (B7-B10 + E23)

- **B7 · WFP `dllmain.cpp` (MIT)**: extraer las fórmulas EXACTAS de FOV/HUD/
  letterbox con fichero:línea del repo → tabla «fórmula → nuestro punto en
  `src/`» (`core/Camera.cpp`, `renderer/Hud.cpp`, `control/Radar.cpp`…). Es la
  spec lista para la tanda 3 de `00-INDICE` §8.
- **B8 · classic-axis `CamNew.cpp` + `Main.cpp`** (sin LICENSE → referencia):
  extraer el algoritmo de cámara IV/apuntado y la semántica de
  `bForceLegsMovements` como pseudocódigo + constantes, con su mapeo a nuestro
  `CCamera`/`CPed::SetMoveAnim` (`walk_left/right/back`). Cierre: spec de
  implementación sin copia literal.
- **B9 · SkyGfx `matfx/leedsCarpipe/WaterLevel` + 25 HLSL** (sin LICENSE):
  documento de traducción **pass a pass** a GLSL ES 3.00 (orden de passes,
  semántica PS2 vs GL — NO traducción automática), anclado a nuestro render de
  vehículo y `renderer/WaterLevel.cpp:911/1206`. Cierre: spec pass a pass.
- **B10 · Resto del árbol SilentPatch**: localizar los fuentes VC concretos
  (qué secciones de `SilentPatchVC.cpp` corresponden a VC vs III/SA) para que
  cada fix del catálogo B6 tenga su ancla exacta fichero:línea.
- **E23 · Inventario del `Config/SilentPatchVC.ini`**: transcribir TODAS sus
  claves como spec (toggle → qué hace → destino nuestro: define/parámetro/dato),
  complemento de `04` §6. Cierre: ninguna clave sin clasificar.

## Tanda C · CLEO/SCM (C11-C13 + E21)

- **C11 · Ids de `vc.json`**: fijar el formato mixto hex/dec (causó la colisión
  `MULT_INT_VAR_BY_VAL` del desensamblado) — normalizar la tabla de comandos.
- **C12 · `CALL_FUNCTION` + `.cs` enteros**: args variables de `CALL_FUNCTION` →
  desensamblar `swim.cs` ENTERO (se paró en 206/2.288 B) y
  `CrouchMovement(forClassicAxis).cs` entero con `tools/cleo_disasm.py`
  (herramienta nueva, F1 del plan `06` §2.3). Entregable: tabla
  «opcode → semántica → ¿memory-hack?» por script.
- **C13 · `freeroam_miami.scm` entero + cabecera**: desensamblar los 38 KB de
  lógica pura (banco de pruebas del desensamblador) y explicar la cabecera
  `02 00 01 20 86…` de `main.scm` (servido `02 00 01 20 86 00 00 6d` vs mod
  `02 00 01 40 87 00 00 6d`, `07` §13).
- **E21 · Zanjar el conflicto del `main.scm`**: con el desensamblador ampliado a
  SCM, inventario de opcodes del `main.scm` del mod vs servido (cuántos custom,
  cuáles, aridad) frente al `0FA8` ×11 vs ×22 ya medido. Entregable:
  recomendación escrita en una página (servir con stubs / servir tal cual /
  conservar el de serie) con consecuencias. **La decisión final es del jugador**
  (contradicción documental: `vice-extended-inclusion.md` §8.F lo descarta;
  `05` §4/`07` §13/`08` §A2 piden servirlo).

## Tanda D · Documental (E22, E24)

- **E22 · % global**: aplicar la corrección del jugador en `05` §9 y `06` §5
  (el % global NO es ≈50%: es menos — falta lo más vital y notorio de Extended),
  con nota literal suya. Las tablas por-mod se quedan (siguen válidas).
- **E24 · Fundido del outro (SilentPatch `:2384`)**: SOLO localizar en `src/`
  (Frontend/main) el fundido que parpadea + `m_nMenuFadeAlpha`; anclar
  fichero:línea y mecánica para la tanda 1 de integración.
  → **HECHO y PORTADO en fase 2 (build `ve56`)**: `Frontend.cpp:2476-2496`
  (fade: `m_nMenuFadeAlpha += 20` cruzaba 240→260) y `:5914-5955` (tick del
  outro); clamp `Min(+20, 255)` en origen + duración 2,5 s (`SilentPatch
  :4086`, tick 32 ms/75 cuentas, `MUCH_SHORTER_OUTRO_SCREEN` off).
- **Cierre**: presentar al jugador el resumen PASS/AVISO por ítem + las
  decisiones que le tocan (E21). La lista del `09` queda al día.

## Spikes AUTORIZADOS (D14-D15) — aparte, en cualquier momento de la pasada

Con la condición del jugador (`09` §4): prueba que **acaba con el código como
estaba**; se documenta el resultado, no se deja el prototipo.

- **D14 · Night vertex colors en librw GL3** (X2.11): spike de render — cómo
  reaccionan nuestros shaders GL3 a los colores de vértice de noche (el mod los
  usa; nosotros no). Medir: ¿carga de datos → shader? ¿coste? Documentar el
  alcance real y su dificultad para SkyGfx.
- **D15 · Presupuesto RAM/streaming para No Island Loading / Seamless
  Interiors**: spike de medición (lectura + aritmética sobre la capa on-demand
  actual: 1,4 GB on-demand + IDBFS 900 MB) → ¿caben las 3 islas + interiores
  residentes? ¿qué presupuesto haría falta? Documentar veredicto.

## Fuera de esta pasada

D16-D17 (fase 2), la implementación del catálogo B6, la fusión del
`fronten2.txd` (A5 de la lista E… no: el ítem «A5 fusión» vive en
`vice-extended-inclusion.md` §8.E como decisión de fase 2), `helipad_strutT`
(A6 de la reconstrucción → fase 2, 2 líneas en `d3d.cpp`), y toda escritura en
el juego.

---

## Resultados (ejecutada el 24/09/2026 — 11/11 ítems, solo lectura)

**Veredicto: PASS.** Todo documentado; 2 decisiones al jugador (E21 `main.scm`,
E22 cifra del % global) y 1 AVISO menor (A5: confirmación de oído). Cero
ficheros del juego tocados. Salidas de trabajo en `gta_vc_browser/tmp/`
(walks `.asm`, `extsrc/skygfx_shaders.txt`, muestras ViceEx). **Herramienta
nueva**: `gta_vc_browser/tools/cleo_disasm.py` (F1 del plan `06` §2.3).

### Tanda 0 — E18 / E19 / E20

- **E18 · IDs 6670/6671 — PASS (sin colisión).** El `default.ide` del mod
  reserva 6500-6599 vehículos · 6600-6619 wheels · 6620-6659 vehmods ·
  **6660-6699 weapons**; sus armas nuevas usan 6660-6669 y su `weapon.dat`
  confirma esos IDs como modelos `weap`. Nuestros `bryx_lights`/`lodbryx_lights`
  = 6670/6671 (`streamed/data/maps/bryx/bryx.ide`) caen en el rango reservado
  pero **el mod no lo usa más allá de 6666-6669** y su loader asigna ids
  dinámicos (`-1`) → sin colisión real. 6670/6671 quedan VALIDADOS.
- **E19 · `fronten2.txd` (+3 MB) — PASS (explicado).** Servido = 12.389.416 B
  (24 tex) = contenido del mod (9.504.424 B, 13 tex: up/down + mapBot/Mid/Top
  272×772) **+ 11 logos de emisora 288×260 A8R8G8B8** (11 × 262.272 B). La
  sobrecarga SON los logos ya fusionados (decisión `vice-extended-inclusion.md`
  §8.E). Validación visual: fase 2.
- **E20 · merge 6500-6507 — PASS (regla fijada).** Nuestro `handling.cfg`
  (205-211 + `!` handling2 262-264) y `carcols.dat` (236-239) ya traducen el
  MVL completo vía `import_mvl_vehicles.py` (oráculo:
  `mods/extended/AddingVehicles/MVLConverter/output.txt`, con id de ejemplo
  distinto). **Regla: NO duplicar defs del `newVehicles.ide` del mod** — sus
  secciones MVL (`default/handling/handling2/carcols/carcols4/sounds/shadow`)
  no las parsea nuestro loader y ya están traducidas inline.

### A3 · cdimages entrada↔entrada — PASS

8 IMG del overlay = 468 entradas comparadas **byte a byte** contra
`streamed/`: **459 idénticas, 0 no servidas, 9 distintas** — todas `.col` de
zonas vanilla en `objects.img`: `haiti, washints, docks, nbeachbt, nbeachw,
mall, airport, littleha, haitin` (el mod parchea COL que nosotros servimos
vanilla). Por imagen: anims 14/14 ✓, generic 1/1 ✓, objects 63/72 (9 COL
distintas), peds 129/129 ✓, player 46/46 ✓, radar 64/64 ✓, vehicles 48/48 ✓,
weapons 94/94 ✓. **Conclusión: el overlay NO aporta contenido nuevo; solo
reescribe 9 COL** → candidatas a [DATO] en fase 2 (con validación de colisión).

### A4 · `GunCrouchFwd/Bwd` + set crouchfire — PASS (decisión tomada)

- Clips extraídos a ANPK válidos en `gta_vc_browser/tmp/a4/` y verificados
  re-leyéndolos: ±2,740 m en Y, 0,731 s, ±3,75 m/s, KRT0, 22 seqs / 23 frm.
- `movements.img` extraído a 9 IFPs (`baseball, buddy, chainsaw, flame,
  grenade, m60, python, rifle, shotgun`) en `tmp/a4/`.
- **Decisión set crouchfire: el SERVIDO.** El set servido
  (`streamed/models/gta3.img/{rifle,shotgun,buddy,m60,python}.ifp`) ES
  literalmente el de Vice Extended (su `anims.img` es byte-idéntico, A3) y el de
  `movements.img` pertenece al addon ClassicAXIS con duraciones distintas
  (`RIFLE_crouchfire`: 0,863 s servido vs 0,631 s movements; `M60` idéntico en
  ambos). Fidelidad al mod = set servido. Los clips `GunCrouch*` del sa-crouch
  se quedan (son los suyos, usados por `CrouchMovement`).

### A5 · audio ViceEx + gamecontrollerdb — PASS con AVISO

- **`weapon.dat` del mod leído entero** (`mods/extended/GameFiles/ViceExtended/
  data/weapon.dat`): **no tiene columnas de audio** — los sonidos van
  hardcoded en el `ViceEx.exe` del mod. Por eso el mapeo era «de oído»: no hay
  tabla de datos que lo resuelva. Lo que sí confirma: los 10 ids 6660-6669
  como modelos de arma (coherente con E18) y las flags hex por arma.
- **Banco `ViceEx.RAW/SDT`** = 13 muestras (SDT 260 B = 13 × 20 B:
  offset/len/loopstart/loopend/rate). 5 parejas mismo tamaño+Hz + 3 sueltas.
  Clasificación **por envolvente** (ataque, rms/pico, ZCR = brillo, cola) —
  propuesta para `viceex-map.tsv` (el jugador confirma de oído):

| # | dur · Hz | firma | propuesta |
|---|---|---|---|
| 00/01 (par) | 0,102 s · 36 kHz | ataque 4 ms, plano, brillo alto (ZCR 0,24/0,18), corte seco | **disparos ligeros** — Beretta y/o Uziold |
| 07/08 (par) | 0,100 s · 44,1 kHz | ataque 3 ms, sostenido, oscuro (ZCR 0,11) | **disparo pesado** — Desert Eagle |
| 03/04 (par) | 0,512 s · 22 kHz | ataque 1 ms, pleno 50 % y decaimiento, oscuro | **disparo de escopeta** — Shotgun2 |
| 05/06 (par) | 1,004 s · 32 kHz | envolvente 1 s decaimiento, ZCR 0,06 = retumbe | **explosión larga** — granada/cohete (`grenade2`/`gr_launch_gren`) |
| 09/10 (par) | 1,065 s · 22 kHz | 1 s con rebotes secundarios | **2ª explosión** — `rcgrenade`/detonación |
| 02 | 0,321 s · 33 kHz | doble evento (15 % y 65-80 %) | **bombeo/recarga** (shotgun2) |
| 11 | 0,409 s · 22 kHz | pico TARDÍO (227 ms), ruido brillante (ZCR 0,37) | **mecanismo/recarga** |
| 12 | 0,584 s · 22 kHz | pico 168 ms, medio | **recarga/impacto** |

  AVISO: armas automáticas (ak47/m16/steyr) sin muestra evidente — pueden
  compartir sonido con `rifle` de serie o estar entre las sueltas. Confirmación
  final: de oído (columna «qué es» del tsv).
- **`gamecontrollerdb.txt`** (931 entradas SDL2, todas `platform:Windows`) →
  tabla de reglas SDL2→Gamepad API: el vocabulario de bindings del DB (a/b/x/y,
  dpup/dpdown/leftx/lefty, lefttrigger…) **es el mismo** del *standard mapping*
  de la Gamepad API → en web NO hace falta traducir bindings (el navegador ya
  da el mapping estándar). Uso real: (1) deadzones/sensibilidades de los 5
  setups GInput (`04` §9) como parámetros, (2) el DB solo como fallback para
  mandos sin standard mapping (filtrado `platform:Windows`). Rumble XInput →
  Gamepad `hapticActuators` parcial. Las 931 filas por-GUID no se transcriben
  (innecesarias en web).

### B7 · WFP FOV/HUD — PASS (fórmulas con fichero:línea)

`dllmain.cpp` es un **stub** (delega en `FOVManager`/`Legacy`); las fórmulas
reales están en los módulos `.ixx` (30 descargados en `tmp/extsrc/wfp/`).

| Fórmula | Origen (WFP) | Nuestro destino en `src/` |
|---|---|---|
| `ConvertFOV`: vFOV = 2·atan(tan(hFOV/2)/(4/3)); hFOV' = 2·atan(tan(vFOV/2)·ar) — conserva FOV vertical | `Draw.ixx:115-123` | `core/Camera.cpp:290` (FOV) |
| `ConvertFOVforCutscene`: 2·atan(tan(fov/2)·`g_cutsceneCameraZoom`) | `Draw.ixx:125-133` | idem, en cutscenes/widescreen |
| `ConvertFOVInverse` (para deshacer) | `Draw.ixx:135-143` | idem |
| `FOVManager`: FOV efectivo = FOV · Π multiplicadores(hash), clamp 0.5–2.0 por multiplicador | `Draw.ixx:145-181` + `SetFOV 184-195` | API de zoom (sniper/M16) |
| `SetFOV` → `fScaledFOV = DEGTORAD(GetScaledFOV()/2)` inyectado en `CalculateDerivedValues` | `Draw.ixx:~205-215` | `CCamera::CalculateDerivedValues` |
| Macros `DEFAULT_ASPECT_RATIO=4/3`, `DEFAULT_VIEWWINDOW=0.7`, `SCREEN_STRETCH/SCALE_X/Y`, `SCREEN_SCALE_AR(a)=a·(4/3)/ar`, `SCALE_AND_CENTER_X` | `common.h:5-32` | `renderer/Hud.cpp:328`, `control/Radar.cpp` |
| HUD: `fWidescreenHudOffset = CalculateWidescreenOffset(...)`; `GetHudOffset()=±offset`; `ScaleRect` | `Draw.ixx:26,63-73`; `Sprite2d.ixx:74-93` | `renderer/Hud.cpp:328` |
| Geometría cutscene: `contentAspect=(4/3)/(0,7+8/480)`; pillar si `ar>contentAspect` → `s_pillarWidth`; `s_cameraZoom=0,7167·ar/(4/3)`; letterbox con `VANILLA_CENTER_FUDGE_Y=18` y ratio top/bottom ≥ `MIN_TOP_BAR_BOTTOM_RATIO=0,2` | `Sprite2d.ixx:37-45, 466-502` | bordes de cutscene + zoom de cámara |
| Bordes vs fade: umbral `fadeAlpha>50` / `m_fTimeToFadeOut==0` → corte inmediato; si no animación `BORDER_ANIM_SECS=0,35` | `Sprite2d.ixx:415-460` | idem |
| Speed blur: `mul=clamp((min(1,v)-0,65)/0,35·75)`, shake `((min(1,v)-0,65)/0,35)/200` con jitter `(rand&0xF)-7` por eje; solo coches en `MODE_CAM_ON_A_STRING`, v>0,65, excluye boat/heli/plane/bike | `Camera.ixx:560-590` | motion blur/camera shake |
| Escalados por defecto: `HudHeightScale=1,0714285` (=480/448 NTSC), `HudWidth/Radar/Subtitles=1,0` | `Legacy.ixx:37-42` | `renderer/Hud.cpp`, Radar |
| Radar IV scaling (`IVRadarScaling`, `fRadarWidthScale`, `fCRadarRadarRange`) | `Legacy.ixx` (patterns `RadarScaling`) | `control/Radar.cpp` |
| Radardisc HQ procedural: anillo 256², `innerR=0,88·outerR`, feather 0,55, rampa RGB 7 puntos | `Radardisc.ixx:63-140` | disco del radar (dato o procedural) |

`Hud.ixx` no lleva fórmulas: hooks de **orden de dibujado**
(`g_wantsToMoveHudLeft/Right` + `CFont::DrawFonts`) = semántica «desplazar el
HUD a izq/der según elemento».

### B8 · ClassicAXIS `CamNew.cpp` — PASS (spec sin copia literal)

Dos orígenes distintos: fuente `CamNew.cpp`/`classicaxis_Main.cpp` (Plugin SDK,
sin LICENSE → referencia/reimplementación con atribución) y release empaquetado
`mods/1510564741_ca-1` (ClassicAXIS.asi v1.6, 2017, DK22Pac/gennariarmando) con
`ClassicAXIS.ini` = spec de toggles.

**Constantes** (`CamNew.cpp:22-28`): `minFOV=50`, `maxFOV=70`,
`maxFOVModern=70`, `wepMinRange=70`; follow: `minDist=2,0`,
`maxDist=2,0+PedZoomValueScript`, `heightOffset=0,4`, clamp vertical
**+60°/−89,5°**; aim: dist fija `2,7`, `heightOffset=0,25`, hombro `(0,2,0,0)`
o `(0,55,0,0)` (stories), clamp **±50°**; input stick: beta
`0,01·(1/20)·FOV/80·ts`, alfa `0,01·(0,6/20)·FOV/80·ts` × sensibilidad; ratón
`(−2,5x, 4y)·MouseAccel·FOV/80`; auto-rotación `WellBufferMe(0,1/0,06)`, umbral
0,06 rad; colisiones: nearClip min 0,05 (LOS <1,3 m) / 0,1 (esferas); agua:
nearClip 0,2 y z=nivel+0,6; crouch: `duck=−0,5 + ((f−FOV)/50·f)/100`.

**Pseudocódigo** (esencia):
```
Process_FollowPed(target):
  FOV → 50 si rifle aimable (canAim && !canAimWithArm && range≥70 && zoomForAssaultRifles) si no 70
  target += [modern: right*-0.25 + up*0.075] + z: heightOffset(0.4) + crouchOffset
  clamp dist a [2, 2+PedZoomValue]; ángulos desde stick/ratón; wrap ±π
  clamp vertical [+60°, -89.5°]; ForceCameraBehind → rotación auto (buffer 0.1/0.06)
  front = (cos v·cos h, cos v·sin h, sin v); source = target - front·len
  si ped en agua y source.z < nivel+0.6 → nearClip 0.2 y z=nivel+0.6
  Process_AvoidCollisions: LOS(target→source) → source=hit (nearClip=max(d-0.3,0.05) si d<1.3)
    + 5 esferas (r=viewPlaneWidth·nearClip) contra mundo: peds <0.5 m → invisibles en transición;
      no-peds → acercan nearClip (mín 0.1) y empujan source
Process_AimWeapon: como FollowPed pero dist 2.7, ±50°, hombro, y si lock-on:
  horShift/verShift por (m_f3rdPersonCHairMultX/Y) sobre viewPlane = tan(FOV/2)·ar·1.05
Process_FOVLerp: fovLerp = interpF(fovLerp, min(50) o 70, 0.1·ts); Minigun excluido en VC
Process_CrouchOffset: duck → -0.5 + corrección por zoom; interpF 0.1·ts
```

**`bForceLegsMovements`** (`ClassicAXIS.ini [MOVEMENTS]`, default **0**):
«hace que el jugador **se mueva incluso con armas a dos manos**; requiere la
carpeta `ClassicAXIS` (addon) y un `ped.ifp` externo» (ReadMe: solo VC). O sea:
fuerza el grupo de **locomoción de piernas** con el upper-body en puntería —
los clips `walk_left/right/back` (A4/`movements.img`) son esa locomoción.
Mapeo nuestro: `CPed::SetMoveAnim` (forzar grupo walk cuando apunta con arma a
dos manos) + cámara `Process_FollowPed_Rotation`. El addon `ClassicAXIS/` es
de dónde salen los clips de piernas; default 0 = mejor dejarlo opcional.

### B9 · SkyGfx pass a pass (HLSL→GLSL ES 3.00) — PASS

24 HLSL (8 PS + 16 VS) de `aap/skygfx_vc`, íntegros en
`tmp/extsrc/skygfx_shaders.txt`. Sin LICENSE → reimplementar con atribución.
Orden de passes y semántica PS2 vs GL:

1. **Mundo con lightmap** (`worldPS`/`vc_worldPS` + `ps2Standard*VS`):
   mundo VC = `col = t0·Color·(lm·t1 + 1−lm)`, `a = Color.a·t0.a·lm.a`
   (`vc_worldPS`); el `worldPS` (III) usa `t0·Color·(1+lm·(2·t1−1))`.
2. **Vehículo pass 1** (`vehiclePass1VS` + `pcEnvPS`/`mobileEnvPS`):
   difuso+ambient+4 luces; **envmap**: `R = reflect(V,N)` → `uv=R·0.5+0.5`
   (matriz `tex` = espacio de vista); `reflcolor = lerp(b⁵, 1, reflProps.y)·
   reflProps.x`, `b = 1−saturate(dot(V,N))` (fresnel). PS aditivo PC
   (`t0·Color + env·coeff`) vs mobile (`t0·(Color+env·coeff)`).
3. **Vehículo pass 2** (`vehiclePass2VS`): especular Blinn
   `specTerm = pow(saturate(dot(N, normalize(V+L))), reflProps.w)`;
   `out = directSpec·specTerm(N,−L,V,p) + Σ₄ lightCol·specTerm(…, p·2)` —
   **segundo draw aditivo** (semántica dual-pass del d3d9).
4. **Env-only** (`nolightEnvVS/EnvOnlyVS`): UV de env desde la normal
   (`mul(N, tex).xy`), color = vértice.
5. **Mundo Leeds** (`leedsWorldVS`): prelight tweak
   `saturate(pre·mult + add)`; `color = saturate(pre·(ambient,1) +
   emissive·surfEmiss)`, `a *= matCol.a`.
6. **Gloss** (`glossVS+glossPS`): N y V **empaquetados** `0.5·(1+·)` en COLOR;
   `out = dot(n,v)⁸ · t0`.
7. **Rim** (`rimVS`): `f = rim.x − rim.y·dot(N,−view)`;
   `r = saturate(lerp(rampEnd, rampStart, f)·rim.z)`; c = luces + r.
8. **ps2Standard vs pcStandard**: PS2 modula al final (`saturate(c)·matCol`),
   PC multiplica `matCol` por término — reproducción PS2 = matCol final.
9. **PostFX**: `gradingPS` (matriz 3×4 `red/green/blueGrade`), `curvePS`
   (LUT 2D por canal, `map(v, v)`), `contrastPS` (`rgb·mult + add`).
10. **Ambient** (`ambientVS`): `saturate(Prelight + ambient·surfAmb)·matCol`.

**Notas de traducción GLSL ES 3.00** (no automática): `mul(v, M)` HLSL →
`M * v` GLSL (transponer el orden); `saturate`→`clamp(x,0,1)`; `register(cN)`
→ uniforms nombrados por pass; `COLOR0/1` → `out vec4` (el packing `0.5(1+·)`
exige varying vec3 sin normalizar); `tex2D`→`texture()`; `#version 300 es` +
`out vec4 fragColor`; los 2 passes de vehículo = 2 draws (2º aditivo, Z func
equal); lightmap en la misma UV1. Anclaje de integración: nuestro render de
vehículo + `renderer/WaterLevel.cpp:911/1206` (ps2Water; fuente descargado:
`tmp/extsrc/skygfx_WaterLevel.cpp`).

### B10 · Fuentes VC concretos del árbol SilentPatch — PASS

Repo = `github.com/CookiePLMonster/SilentPatch` rama `dev` (`06` §1).
`SilentPatchVC/SilentPatchVC.cpp` (copia local `tmp/SilentPatchVC.cpp`, **4.764
líneas**) es **100 % VC** (incluye `EntityVC.h`, `ModelInfoVC.h`, `VehicleVC.h`);
lo compartido con III/SA vive en `Utils/`, `MemoryMgr.GTA.h`, `RWGTA.cpp`.
Mapa de secciones (namespace : línea → catálogo B6):
`ModCompat:37` · `UIScales:96` · `ClipCursorToGameWindow:344` (Win32, no web) ·
`PrintStringShadows:448` · `RadardiscFixes:541` (= E23 `DontShrinkRadardisc`) ·
`OnscreenCounterBarFixes:613` · `RadarTraceOutlineFixes:662` ·
`LoadingBarOutlineFixes:723` · `CreditsScalingFixes:773` ·
`SlidingTextsScalingFixes:802` (= `SlidingMissionTitleText`) ·
`DarkelTextPlacement:866` · `MinimalHUD:896` · `ShadowScalingFixes:948` ·
`TextRectPaddingScalingFixes:1001` · `BigMessage3ScalingFixes:1072` ·
`LegendBlipFix:1106` · `FixedLineWraps:1122` · `YouAreHereScalingFixes:1190` ·
`ZeroAmmoFix:1308` · `KeyboardInputFix:1325` (Win32) · `Localization:1341` ·
`SirenSwitchingFix:1373` · `FBISirenCoronaFix:1398` (= corona sirenas) ·
`RemoveDriverStatusFix:1455` · `EnvMapsOnExtras:1479` (= spec de extras) ·
`CPlane::LoadPath:1545` · `IS_PLAYER_TARGETTING_CHAR` (Wesser):`1610` ·
`ResetStats NewGame:1646` · `BackfaceCulling:1701` (= `DrawBackfaces`) ·
`ConstructionSiteLOD:1864` · `CoronaFlares:1931` · `RoadblockWeapons:1942` ·
`MuggingObjective:1952` · `CShadows:1967` · `ScaleScriptSprites:1999` ·
`BilinearScriptSprites:2063` · `SkimmerElevator:2082` ·
`PoliceChaseGiveUp` (backport SA):`2138` · `NPCsRPG:2168` · `CMBlur margins:2196`
· `Stingers:2309` · **`OutroSplashFix:2384` (= E24)** · `TommyFistShake:2400` ·
`ShellCasings:2453` · `IceCreamVanEffectFix:2469`. **Remate 2500-4764**:
`InjectDelayedPatches_VC_Common` (inyección por patrones de todo lo anterior +
compat con el módulo `skygfx` + check SSE) y export `GetBuildNumber()`.

### E23 · Inventario `SilentPatchVC.ini` — PASS (0 claves sin clasificar)

| Clave (sección) | Default | Qué hace | Destino nuestro |
|---|---|---|---|
| `Units` | -1 | unidades: 0 métrico, 1 imperial, -1 por locale | parámetro de config web |
| `EnableVehicleCoronaFixes` | 1 | corona de sirenas (Police, Enforcer, Firetruck, Ambulance, FBI Rancher, Vice Cheetah, +siren FBI Washington, luz de taxi, search/rotor Police Maverick) | [NATIVO] `FBISirenCoronaFix`+`SirenSwitchingFix` + posiciones por modelo (dummies `servicelights*` del mod) |
| `MinimalHUD` | 0 | fade de salud/arma/dinero al inactivarse (feature inacabada) | [NATIVO] opcional (`MinimalHUD:896`) |
| `SlidingMissionTitleText` | 0 | texto de misión deslizante (restos del juego) | [NATIVO] opcional (`SlidingTextsScalingFixes:802`) |
| `SlidingOddJobText` | 0 | ídem odd job (visible en betas de III) | [NATIVO] opcional |
| `ShowPropertyBlips` | 0 | blips de propiedades comprables (el script los crea, no se mostraban) | [NATIVO] trivial |
| `DontShrinkRadardisc` | 0 | opt-out de encoger 2 px el disco (para texturas custom/PS2) | [DATO/NATIVO] (`RadardiscFixes:541`) |
| `UseDesktopRefreshRate` | 1 | refresh del escritorio en fullscreen (flicker >60 Hz) | NO aplica web (rAF) |
| `ScaleScriptSprites` | 1 | sprites de script escalan a resolución (doble escala con CLEO que ya escala) | [NATIVO] (`ScaleScriptSprites:1999`) |
| `SpeechDelayTimer` | 0 | ms entre comentarios de NPC (-1 = 6000) | parámetro de config |
| `[ExtraCompSpecularityExceptions]` | `stallion`, `mesa` | quita spec a extras (techos de piel) | lista de datos (`EnvMapsOnExtras:1479`) |
| `[DrawBackfaces]` | **~300 modelos** (1 comentado: `mall_hardware`) | sin backface culling (como `DrawBackfaces.txt` móvil) | lista de datos (transcribir al integrar; fuente: el INI) |
| `[DontDrawBackfaces]` | vacío | inversa: fuerza BFC (skins de personajes) | lista de datos |

### C11 · Ids de `vc.json` — PASS (regla fijada)

Sanny Builder library **v0.407** (`library.sannybuilder.com/#/vc`).
**Regla: los ids son strings de 4 dígitos HEX SIN prefijo → `int(id, 16)`
SIEMPRE.** Censo: 9 extensiones — `default` 1.436 (0000-059E), `CLEO` 128,
`audio` 7, `bitwise` 14, `clipboard` 2, `file` 6, `imgui` 87, `ini` 6,
`memory` 3 = **1.689 comandos**; normalizados: **1.689 únicos, 0 colisiones**
(la colisión `MULT_INT_VAR_BY_VAL` era artefacto de leer ids como decimal).
Args: 130 tipos; los 3 textos de 8 B = `string` (133), `gxt_key` (32),
`zone_key` (7) → los 3 cuentan como string de 8 B al decodificar;
`arguments` (23+2) = varargs de `CALL_FUNCTION`. **La familia ViceEx 0FA0-0FA9
NO está en vc.json** (su semántica solo existe en el `ViceEx.exe` del mod).

### C12 · `CALL_FUNCTION` + `.cs` enteros — PASS

**`tools/cleo_disasm.py`** (nuevo): args auto-tag (1=int32, 2=global16,
3=local16, 4=int8, 5=float32) + strings 8 B sin tag; args terminados en
`ARGUMENT_END` (0x00 — era «el byte suelto tras CALL_FUNCTION»);
ANDOR (`00D6`): param `n` = AND de `n+1` condiciones, `20+n` = OR de `n+1`
(enum `ORS_1=21`, confirmado con el handler del motor: hace `state++`);
not-flag = `opcode|0x8000`; tabla de símbolos Sanny al final
(`E\0VAR\0` + entradas `[u32 offset][nombre\0]` alfabéticas + tráiler
`__SBFTR`) detectada automáticamente; resync ante opcodes custom.

Tabla «opcode → semántica → ¿memory-hack?»:

**`swim.cs`** (2.288 B — walk COMPLETO, 69 instrucciones, 19 opcodes): `ADD_VAL_TO_INT_LVAR`×9, `GOTO_IF_FALSE`×8, `IF`×6, `GET_PED_POINTER`×5, `MULT_FLOAT_LVAR_BY_FLOAT_LVAR`×5, `IS_PLAYER_PLAYING`×4, `NOT`×4, `WAIT`×4, `GOTO`×4, **`WRITE_MEMORY`×4 · `READ_MEMORY`×4 · `CALL_FUNCTION`×1 · `GET_PED_POINTER`×5 → MEMORY-HACK SÍ** (direcciones crudas del exe 1.0 + llamada a función por dirección), `IS_CHAR_IN_WATER`×3, `IS_BUTTON_PRESSED`×2, `SCRIPT_NAME`×1… Lógica: nado = estado + movimientos de cámara/gatillo. **No portable tal cual** → su mecánica ya está reimplementada nativa (nado portado).

**`CrouchMovement(forClassicAxis).cs`** (2.052 B — walk COMPLETO, 102
instrucciones, 25 opcodes): `GOTO_IF_FALSE`×15, `NOT`×13, `IF`×8,
`SET_LVAR_INT`×8, `BIT_AND`×5, `IS_BUTTON_PRESSED`×5, `IS_INT_LVAR_*`×10,
`BIT_SHR`×3, `SET_PLAYER_CONTROL`×3, `GET_POSITION_OF_ANALOGUE_STICKS`×1,
`EMULATE_BUTTON_PRESS_WITH_SENSITIVITY`×1 (CLEO), **`SET_CHAR_CROUCH`×1** y
`PLAY_ANIMATION`×1 (el agachado + clips `GunCrouch*`), `GOSUB`×2,
**`READ_MEMORY`×4 → memory-hack PARCIAL** (solo lectura de flags; sin
escritura), acaba en `RETURN` limpio. Sin opcodes custom, sin resync.

### C13 · `freeroam_miami.scm` + cabecera SCM — PASS

Cabecera `main.scm` determinada con **el motor como oráculo**
(`Script.cpp:745-804` `CTheScripts::Init`; `Script5.cpp:2755`
`ReadObjectNamesFromScript`; `Script5.cpp:2797`
`ReadMultiScriptFileOffsetsFromScript`; `Script.h:401-406` `SIZE_MAIN_SCRIPT`)
y verificada empíricamente:

- `[0..2]` = `02 00 01` (identificador); **`u32@3` = tamaño del espacio de
  variables V** = `0x8620` (34.336) servido / `0x8740` (34.624) mod (de ahí el
  `86`/`87` de las cabeceras medidas `02 00 01 20 86…` / `02 00 01 40 87…`).
- El código arranca con `GOTO V` (el walk confirma `000000 GOTO 34624` en el
  mod) y **en V+8 está la tabla de objetos usados**: `u16
  NumberOfUsedObjects` + nombres de 8 B (KEY_LENGTH 8 verificado) — los resync
  del walk caen justo ahí (`DTN_STADDOOR`, `LODMAIN_BODY`…).
- Bloque multiscript (tras los objetos): **cabecera de 12 B** —
  `MainScriptSize u32` · `LargestMissionScriptSize u32` ·
  `NumberOfMissionScripts u16` · `NumberOfExclusiveMissionScripts u16` — seguida
  de `MultiScriptArray u32×N`, con **`MultiScriptArray[0] == MainScriptSize`**
  (= fin del código principal). Ojo: la tabla va a **12** bytes de cabecera
  (una lectura previa a +16 estaba desplazada).
- `freeroam_miami.scm` (mod y servido, `data/` de ambos) desensamblado entero
  = banco de pruebas del tool (walks en `tmp/walk-*.asm`).

### E21 · Conflicto `main.scm` — INVENTARIO + RECOMENDACIÓN (decide el jugador)

Inventario por walk del desensamblador (`tmp/walk-*main.scm.asm`):

| | Servido | Mod (Extended v2510) |
|---|---|---|
| Instrucciones | 172.328 líneas de walk | 153.993 |
| Opcodes custom | **0** | **7 tipos: `0FA0` ×45, `0FA1` ×2, `0FA2` ×91, `0FA6` ×46, `0FA7` ×1, `0FA8` ×20, `0FA9` ×1 = 206 usos** (contados por consumo CUSTOM; el conteo crudo por bytes ≈ ×2 por dobles en los dumps) |
| Aridad custom | — | sin tipar (el tool los traga con longitud heurística 10-30 B); `0FA8` ~4 params según indicios (`vice-extended.md:12`, offset 45810 tras `HELP61`) |

Cuadra con `vice-extended-inclusion.md:441` (bytes `A8 0F`: 22 en el mod — de
ellos **20 son opcodes reales en código** y 2 ruido de datos; los 11 del
«de serie» de ese recuento eran ruido de datos: nuestro servido tiene **0**).
Semántica de los 7: **desconocida** (solo en el `ViceEx.exe` propietario del
mod; no están en `vc.json`).

**Recomendación (una página):**

1. **Conservar el de serie (RECOMENDADO).** Servimos nuestro `main.scm` actual
   (0 custom = 100 % compatible con nuestro intérprete hoy). Los «Script
   changes» de Extended se cubren después por paridad nativa/CLEO-lite sobre
   nuestro script. *Consecuencias:* cero riesgo de desync; coste = mantener una
   lista de paridad (la §4/`vice-extended-pendientes.md`) y perfeccionarla por
   pruebas de partida. Coincide con `vice-extended-inclusion.md` §8.F.
2. **Servir el del mod con stubs** para los 7 custom. *Consecuencias:*
   desbloquea los Script changes de una vez, PERO los stubs noop rompen en
   silencio lo que hagan `0FA2` (×91, el más usado), `0FA6`, `0FA0` y `0FA8` —
   sin semántica no se puede saber si «romper» es cosmético o mecánico; cada
   hallazgo exigiría desensamblar el `ViceEx.exe`.
3. **Servir el del mod tal cual** — DESCARTADO: 206 usos de opcodes sin
   implementar = desync/crash garantizado (reVC despacha 0-1499 → desconocido).

**La decisión final es del jugador** (contradicción documental ya zanjada de
hecho por el walk: `05` §4/`07` §13/`08` §A2 pedían servirlo creyendo que era
paridad directa; el inventario demuestra que trae motor propio embebido).
**Estado (24/09/2026): el jugador APLAZÓ la decisión a fase 2** — E21 queda
abierto con la recomendación 1 («conservar el de serie») como propuesta.
**ACTUALIZACIÓN (minería del `ViceEx.exe`, ver §Spikes): la opción 2 ya NO es a
ciegas** — el exe es un fork de reVC/librw y sus 10 handlers son portables a
nuestro intérprete (misma API). La decisión se toma con ruta técnica para las
dos opciones viables.

### E22 · % global — corregido en `05` §9 y `06` §5 (cifra a criterio del jugador)

Nota literal aplicada en ambos docs: «el % global no es ≈50 %: es menos —
falta lo más vital y notorio de Extended». El material copiable (fuentes,
HLSL, INIs) sube con esta pasada, pero las **mecánicas vitales de la X-list**
(`vice-extended-pendientes.md`) siguen sin portar → el % global baja. **Cifra
fijada por el jugador (24/09): ≈30 % o menos** — ya aplicada en `05` §9 y
`06` §5.

### E24 · Fundido del outro — PASS (anclado, sin tocar)

- **Nuestro lado**: `src/core/Frontend.cpp:2476-2498` —
  `m_nMenuFadeAlpha` avanza de 20 en 20 cada >30 ms (`forceFadeInCounter` como
  red de seguridad) y **pasa de 240 a 260 → clamp 255** en `:2497-2498`;
  `:2500-2521` dibuja la pantalla previa con `m_nMenuFadeAlpha = 255 − alpha`
  (doble render durante el fade) y `MENUPAGE_OUTRO → DrawQuitGameScreen`.
  Esa transición 240→260 es el **parpadeo de un frame** del splash del outro.
- **Fix del mod**: `SilentPatchVC.cpp:2384` `OutroSplashFix` — clamp del alfa a
  `[0,255]` en el setter RGBA (`RGBASet_Clamp`), 16 líneas.
- Mecánica para la tanda 1 de integración: clampear el alfa del fade (o el
  paso de +20 para que no cruce 255 sin clamp) en `Frontend.cpp`; criterio
  PASS = fundido del outro sin frame claro.

---

## Spikes D14/D15 + minería del ViceEx.exe (24/09/2026 — solo lectura + herramientas nuevas)

Cierre de la fase 1 ordenado por el jugador. **Cero ficheros del juego
modificados** (condición de los spikes cumplida: el código queda como estaba).

### D14 · Night vertex colors en librw GL3 — DIFICULTAD BAJA

- **Mecánica real del mod**: NO son datos nuevos. Es un **uniform `nightParam`**
  en su `vendor/librw/src/d3d/d3d9.cpp` (string presente en el exe) que
  **modula el prelight por pipeline** (mezcla día↔noche en el vertex color).
- **Nuestro lado**: shaders GL3 en `vendor/librw/src/gl/shaders/` — un solo
  `in_color` (ATTRIB_COLOR 2 en `header.vert`); `grep night` = 0 coincidencias.
- **DFF vanilla muestreados** (`streamed/models/gta3.img/`: `ap_hland_01`,
  `washbuild018`, `admiral`): SIN segundo set de colores de noche; extensiones
  encontradas = node-name plugin `0x253F2FE` (dummies `chassis_dummy`,
  `wheel_rf_dummy`…), HAnim `0x11E`, 2dfx `0x50E`.
- **Coste estimado**: ≈1 uniform + `mix()` en ~4 shaders (im3d/skin/matfx/
  default). Sin tocar datos ni formato DFF.

### D15 · Presupuesto RAM/streaming (No Island Loading / Seamless Interiors) — FACTIBLE CON RESERVAS

- **Pesos reales de `streamed/`**: total **1.492 MB** — Audio 958 MB
  (`sfx.RAW` 340 MB + 9 `.adf` ≈270 MB), models 397 MB (de los que
  **`gta3.img` = 363 MB**), anim 118 MB (`cuts.img` 115 MB).
- **Arquitectura actual**: `gta_vc_browser/web/ondemand.js` pool MEMFS
  **CAP 360 MB** (working set ≈150 MB mundo + 30 MB emisora, `warmMB` 64),
  IDBFS `idbCapMB: 900`.
- **Veredicto**: sin No Island Loading, CAP 360→**~700 MB** y primera carga
  ~363 MB (todo `gta3.img`); Seamless Interiors ya caben en `gta3.img`
  (+0-50 MB). RSS wasm ≈**1,2-1,8 GB** → solo-desktop al principio; medir en
  vivo con `odtrace` en fase 2.

### Minería del `ViceEx.exe` (opcodes `0xFA0`-`0xFA9`) — HALLAZGO MAYOR

- **Herramientas nuevas**: `gta_vc_browser/tools/viceex_opcodes.py` y
  `viceex_dispatch.py` (capstone 5.0.7 en `gta_vc_browser/tmp/pylibs`).
- **El `ViceEx.exe` es un BUILD de un fork de reVC/librw** («gta-extended»):
  strings `D:\Modding\GTA\VC\gta-extended\src\control\Script*.cpp`,
  `vendor\librw\...`, PDB `bin\win-x86-librw_d3d9-mss\DebugVE\ViceEx.pdb`
  (Debug → asserts legibles), `MODLOADER_REVC`.
- **Dispatch**: `CRunningScript::ProcessCommands4000To4099` VA `0x4946C0`
  (`eax = opcode − 4000; if (eax > 9) default; jmp tabla[eax]`), tabla de
  saltos VA `0x494A70` → **los 10 handlers SON `0x0FA0`-`0x0FA9`**
  (4000-4009, sin huecos).
- **Aridad** (pushes antes de `CollectParameters` `0x472a80`): case 0 `[2,1,2]`,
  case 1 `[1,2,1,2]`, case 2 `[2,1,2]`, cases 3-8 `[1,2]`, case 9 `[2]` →
  patrón ~2 args + 1 resultado (`StoreParameters` presente);
  **`0xFA9` sin escritura** (solo consulta).
- **Semántica parcial** (asserts + callees): `0x0FA0` = `pPed` +
  `CTxdStore::GetSlot` (asigna TXD/skin al ped); `0x0FA9` = `vehicle`;
  `0x0FA6` = lógica pura sin llamadas; `0xFA4/5/7` comparten callee
  `0x48b120`; `0xFA8`→`0x5e1070`; `0xFA3`→`0x604640`; `0xFA2` (×91)→`0x4f83d0`.
- **Conclusión para E21**: los handlers son **portables a nuestro intérprete**
  (misma API: `CollectParameters`/`StoreParameters`/`GetPointerToScriptVariable`)
  → la opción 2 del `main.scm` del mod ya tiene ruta técnica. Uso en sus
  scripts: `0FA2`×91, `0FA6`×46, `0FA0`×45, `0FA8`×20, `0FA1`×2, `0FA7`×1,
  `0FA9`×1.
- **Pendiente** (fase 2 o petición del jugador): aridad exacta y semántica
  completa de los 7 custom por desensamblado fino de los handlers.

### A5 · escucha del jugador — ✅ CERRADA (24/09/2026) — FASE 1 SIN PENDIENTES

Escucha de las **13 muestras** completada por el jugador. Resultado en
`gta_vc_browser/tmp/viceex-samples/viceex-map.tsv` (relleno, formato CRLF de
la plantilla conservado). El oído **refuta en parte la propuesta por
envolvente** (§Resultados A5) — la envolvente orienta, el oído manda:

| # | oído del jugador | propuesta envolvente | veredicto |
|---|---|---|---|
| 0/1 | **ROTOS — ruido al azar** (no se usan) | disparos ligeros Beretta/Uziold | ❌ envolvente falló |
| 2 | **recarga en general** (NO es bombeo — corrección del jugador) | bombeo/recarga (shotgun2) | ❌ parcial (es recarga, pero genérica) |
| 3/4 | **shotgun2** (par) | disparo de escopeta — Shotgun2 | ✅ coincide |
| 5/6 | **desert eagle** (par) | explosión larga granada/cohete | ❌ envolvente falló |
| 7/8 | disparo sin identificar (par) | disparo pesado — Desert Eagle | ❌ parcial (es disparo, arma sin ID) |
| 9/10 | **francotirador/fusil de precisión** (¿steyr?) (par) | 2ª explosión — rcgrenade | ❌ envolvente falló |
| 11 | **lanzagranadas: LANZAMIENTO del proyectil** (no es recarga — corrección del jugador) | mecanismo/recarga | ❌ parcial |
| 12 | **nadar (swim)** — no es arma | recarga/impacto | ❌ envolvente falló |

**Forense del RAW/SDT** (tras el aviso del jugador de que 0/1 «suenan a ruido
al azar») — el 0/1 está roto **de fábrica**, no por nuestra extracción:

- Extracción correcta: offsets contiguos, `Σtamaños = 364.850 B` = tamaño
  exacto del RAW, PCM16 coherente, sin magia MP3/ADPCM/Ogg en ninguna entrada,
  offsets pares, test estéreo intercalado descartado (ZCR par≈impar).
- 0/1: **envolvente RMS plana** (`@****++=+@@`, sin ataque/decaimiento) y
  **saturadas** (pico 32767/32768) → ruido de fábrica del banco del mod.
  Se dejan **vacías** en el tsv = no se sirven, manda el de serie.
- **5/6 = copia EXACTA byte a byte** (md5 idéntico) y **7/8 = copia EXACTA**
  (md5 idéntico): el autor duplicó la muestra sin variante real. 3/4 y 9/10
  sí son variantes reales (bytes distintos, mismo tamaño+Hz).

Ejecución pendiente en fase 2: `tools/split_sfx.py` (añadir al banco servido)
+ enganchar en `AudioLogic` a beretta/shotgun2/desert eagle/steyr/lanzagranadas
+ swim según esta tabla.

**Reproductor listo** (`gta_vc_browser/tools/viceex_samples.html`, 24/09):
sirve desde `tools/` con `python -m http.server` en `gta_vc_browser/` y muestra
las 13 filas con ▶ por muestra, «▶ Escuchar todo», onda dibujada por WebAudio,
la propuesta de envolvente con botón «usar», pares 🔗 enlazados (espejo + sufijo
«(par)» al exportar), autoguardado en localStorage y export **Copiar TSV** /
**Descargar `viceex-map.tsv`** con el formato exacto de la plantilla (CRLF,
índice a 2 columnas, tabuladores). Probado en navegador: reproducción (206
Media), ondas, espejo de par, formato del TSV y persistencia — PASS.
