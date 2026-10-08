---
name: 00-INDICE
status: EXECUTED
type: research
domain: process
owner_rules: .agents
created: 2026-09-21
---

# Planes · mods de la comunidad que podemos aprovechar

Fecha: 21/09/2026 · sección 1 (Buffy) · estado: **investigación hecha, nada implementado**

Se pidió estudiar tres mods clásicos y decidir cómo aprovecharlos:

1. `CookiePLMonster/SilentPatch` (arreglos del juego original)
2. `GTAmodding/FramerateVigilante` (bugs que aparecen al subir los FPS)
3. `ThirteenAG/WidescreenFixesPack` (pantalla ancha: HUD, menús, cámara)

Planes por mod: `01-silentpatch-vc.md`, `02-frameratevigilante.md`,
`03-widescreen-fixes-pack.md`.

**Pasadas posteriores (23-24/09):** `04-extraccion-fuentes-mods.md`,
`05-cleos-adaptador-y-paridad-scm.md`, `06-fuentes-externas-y-mapeo.md`,
`07-verificacion-ejecutada-por-mod.md`, `08-pasada-a1-a2-b6.md`.

**FUENTE DE VERDAD de la numeración del trabajo (bloques A/B/C/D + fases):
`09-numeracion-a-b-c-d.md`** — blindada tras perderse una lista en un reinicio.
**Plan de la pasada siguiente: `10-plan-pasada-siguiente.md`** (fase 1, solo
lectura; ESTADO: **EJECUTADA y FASE 1 CERRADA el 24/09/2026** — 11/11 ítems +
spikes D14/D15 + minería del ViceEx.exe + escucha de las 13 muestras).
**HANDOFF DE LA FASE 1→2: `11-handoff-fase2.md`** — volcado completo del
contexto de la sesión (reglas de casa, decisiones tomadas, tablas de audio,
minería, cola de trabajo §8).
**HANDOFF ACTIVO (EMPEZAR POR AQUÍ): `12-handoff-tanda2.md`** — todo lo que
necesita saber el agente que retome: meta, estado (fase 1 y tanda 1 CERRADAS,
build `ve56`), reglas de casa y estilo de trabajo, orden restante (tandas 2 y 3
+ D15 + decisión E21) y arranque exacto de la tanda 2. La fase 2 está **EN
MARCHA**.

---

## 1. La decisión: **portar el código, NO montar un cargador de mods**

Pregunta del jugador: ¿hacemos un programa que cargue estos mods o metemos su
código en el motor?

**Portar.** Razones concretas, comprobadas al leer los tres repos:

- **Técnica de los tres**: hookean la **binaria original** por patrón de bytes
  (`hook::pattern(...)`, `WriteMemory<uint32_t>(0x005AF238, …)`, `.ual`). Esas
  direcciones **no existen** en un motor recompilado desde fuente: un cargador
  tendría que replicar la ABI de la DLL original (VTables de MSVC 2003, registro
  `EAX/ESP` a mano) dentro de WebAssembly. Es trabajo enorme para cero ventaja.
- **Tenemos el código fuente del motor.** Lo que el mod consigue parcheando bytes,
  nosotros lo conseguimos cambiando **la línea** donde está el bug. Es más corto,
  más claro y no se rompe al recompilar.
- **El render es nuestro** (librw). Los `.ual`/parches de WFP atacan el motor D3D8
  original; lo aprovechable es su **lógica** (aspecto, letterbox, escala del HUD),
  no sus bytes.
- **Web**: no hay DLL/ASI que inyectar en WebAssembly.

Lo que **sí** copiamos de ellos, con licencia MIT (ver §2):
el **conocimiento** (qué estaba mal y por qué, con la línea exacta), la
**lógica corregida** y, cuando toca, el **código** (adaptado a nuestras clases).

## 2. Licencias (los tres son MIT: se puede copiar dando crédito)

> **NORMA VIGENTE (27/09/2026) — la licencia NO es una puerta.** Proyecto
> **local y personal** (no se redistribuye nada): de `mods/` se toma **todo**
> lo que haga falta —codigo, logica, datos— tenga licencia o no. La cabecera
> de atribucion y `docs/mods/ATTRIBUTION.md` se mantienen como **cortesia y
> trazabilidad**, no como obligacion. Lo unico que sigue fuera es lo
> **tecnicamente inaplicable en WASM** (direcciones x86, `hook::pattern`,
> `.asi`, `.ual`, `rwd3d9`, `injector`, `plugin-sdk`, memory-hacks de `.cs`),
> nunca por la licencia de un mod. Lo de abajo se conserva como inventario.

| Repo | Licencia | Autoría |
|---|---|---|
| SilentPatch | MIT | © 2024 Adrian Zdanowicz («Silent») |
| FramerateVigilante | MIT | © 2023 GTA modding (Junior_Djjr) |
| WidescreenFixesPack | MIT | © 2018 ThirteenAG |

**Reglas de trabajo (obligatorias para todo lo que salga de aquí):**

1. Cabecera de atribución en cada bloque portado, justo encima del código:

```cpp
// ----------------------------------------------------------------------------
// PORTADO — SilentPatch (MIT, © 2024 Adrian Zdanowicz "Silent")
//   https://github.com/CookiePLMonster/SilentPatch
//   SilentPatchVC/SilentPatchVC.cpp:1864  ("construction site LOD …")
// Qué se toma: <la idea/medición>. Adaptación: <qué cambia en nuestro motor>.
// ----------------------------------------------------------------------------
```

2. Un manifiesto único, `docs/mods/ATTRIBUTION.md`, con una fila por arreglo
   (origen → fichero:línea nuestra → estado). Ese fichero es la prueba de que no
   copiamos a ciegas y de que damos crédito.

3. **Grupos apagables**: `VICEEXT_FIX_SILENTPATCH`, `VICEEXT_FIX_WFP`,
   `VICEEXT_FIX_FV` en `src/core/config.h` (misma convención que los
   `VICEEXT_*` actuales). Un sub-define propio (`VICEEXT_FIX_FV_ROTOR`) sólo
   cuando el arreglo toque algo que el jugador nota y queramos poder apagarlo
   suelto.

4. **Assets:** no hace falta copiarlos — el juego los sirve desde tu copia
   local (`streamed/`). Si un arreglo pide una textura/menú que no exista, se
   **dibuja por código** (el propio WFP ya lo hace con el disco del radar: genera
   el anillo en memoria, ver `Radardisc.ixx:70-156`). No es un veto legal, es que
   no aporta y añade datos que versionar.

5. **NO se copian** addresses, `hook::pattern`, `.ual`, ni `plugin-sdk`/
   `injector`: motivo **TÉCNICO** (es la parte atada a la binaria x86 original; en
   WASM no existen esas direcciones), **no** licencia.

## 3. Qué aporta cada mod (resumen del inventario)

- **SilentPatchVC**: ~45 secciones documentadas con comentario propio. Dos
  familias: (a) **bugs de juego** (LOD del edificio en obras, coronas de sirena
  de policía/FBI/Vice Cheetah, coches que explotan dos veces, detección de
  apuntado, casquillos de armas que no los echan, memorias rancias…) y (b)
  **HUD/escala según resolución** (barra de carga, sombras de texto, ajustes de
  línea, sprites de script, coronas). Además **datos**: parches `.ipl.diff` de 8
  zonas del mapa (club, hotel, littleha, mansion, oceandn, oceandrv, stripclb,
  washints) que podemos aplicar a nuestros datos derivados.
- **FramerateVigilante**: parches por dirección para III/VC/SA centrados en
  **FPS altos**: constantes por-frame que nadie escaló (`ms_fTimeStep`),
  temporizadores que asumen 30 fps, y entradas que se rompen (bocina/sirena,
  giro de rueda en raíles, velocidad de rotor). Su valor para nosotros es el
  **mapa de sitios frágiles**; sus direcciones no sirven.
- **WidescreenFixesPack (GTAVC)**: organizado por clase del motor (`.ixx`), con
  la joya en `Sprite2d.ixx` (geometría 4:3 del modo cinemática, letterbox,
  «elemento texturizado vs UI»), `Radardisc.ixx` (disco del radar en alta
  calidad **dibujado por código**) y módulos por sistema. Buena parte ya la trae
  re3 de serie (`SCREEN_SCALE_*`, `SCREEN_STRETCH_*`, `CalculateAspectRatio()`).

## 4. Reparto propuesto (3 subagentes + sección 1)

Nadie comparte fichero. Si alguien necesita un fichero de otro, lo pide.

| Agente | Tema | Ficheros (dueño único) |
|---|---|---|
| **A** | coches, peds y sombras (SilentPatch T1 vehículos/peds + FV vehículos) | `src/vehicles/*.cpp`, `src/control/CarCtrl.cpp`, `src/control/CarAI.cpp`, `src/peds/PedAttractor.cpp`, `src/renderer/Shadows.cpp`, `src/core/Timer.cpp` |
| **B** | HUD, textos, escala y pantalla ancha (SilentPatch T2 + WFP) | `src/renderer/Hud.cpp`, `src/renderer/Font.cpp`, `src/renderer/Sprite2d.cpp`, `src/renderer/Coronas.cpp`, `src/renderer/MBlur.cpp`, `src/control/Radar.cpp`, `src/text/Messages.cpp`, `src/core/FrontEnd*.cpp` |
| **C** | scripts, armas, mundo y datos (SilentPatch T1 resto + `.ipl.diff`) | `src/control/Script*.cpp`, `src/control/Pickups.cpp`, `src/weapons/Weapon.cpp`, `src/control/Darkel.cpp`, `src/core/ModelInfo.cpp`, `src/core/Streaming.cpp`, `tools/apply_ipl_diffs.py` (nuevo) |
| **Sección 1 (yo)** | arranque/carga, verificadores, enlace, manifiesto **y los datos del mapa** (T3) | `src/core/main.cpp`, `src/skel/glfw/*`, `gta_vc_browser/**`, `docs/mods/**`, `tools/apply_ipl_diffs.py` |

`src/core/config.h` lo toca **sólo la sección 1** (los demás piden su define).

## 5. Cómo se verifica cada arreglo (sin fe)

1. **Marca en el paquete**: `check-served-build.sh` ya dicta si el motor servido
   lleva lo implementado. Cada arreglo portado añade su marca.
2. **Traza**: `ODTRACES` con etiqueta propia, una línea por cambio de estado (no
   por frame). Se lee con `tools/viceext-log-check.py`.
3. **Bloque en el verificador**: cuando el arreglo es medible desde el log
   (números), bloque nuevo ahí, con PASS/FAIL explícito.
4. **Vídeo** cuando es visual: `tools/frames-at.sh` saca fotogramas en los
   segundos del log (ya funciona).
5. **Regla de oro ya aprendida**: nada se juzga sin que el `.wasm` enlazado lleve
   la marca; primero `check-served-build.sh`, después pedir partida.

## 6. Orden de trabajo recomendado

1. **Datos (.ipl.diff)** — barato, sin código, y arregla geometría del mapa.
2. **T1 SilentPatch** — bugs que el jugador ve y ya ha reportado (incluye las
   coronas de sirena, que cierran el bloque R11/§6 en curso).
3. **T2 + WFP (HUD/escala)** — afecta todo lo que se dibuja; mejor después de
   tener los arreglos de juego dentro.
4. **FV (FPS altos)** — se juzga jugando sin límite de FPS; necesita la parte 3
   para que lo que se mida no esté contaminado por otros bugs.

El detalle de **qué se ejecuta primero y por qué** está en §8 (reparto por
coste), que es el que manda a partir de ahora.

## 7. Cómo se arranca (prompts listos, uno por agente)

**Agente A — coches, peds, sombras y FPS altos**

```
Lee .agents/plans/mods/00-INDICE.md, luego 01-silentpatch-vc.md (tabla T1) y
02-frameratevigilante.md. Ejecuta los arreglos de tus ficheros (dueños en la
tabla del índice). Antes de cada uno, comprueba si ya estaba en nuestro código
y anótalo. Medir → arreglar → compilar SÓLO tus objetos con ninja (sin enlazar)
→ anotar en .agents/HISTORIAL.md. Ficheros: src/vehicles/*, src/control/CarCtrl.cpp,
src/control/CarAI.cpp, src/peds/PedAttractor.cpp, src/renderer/Shadows.cpp,
src/core/Timer.cpp. NO toques gta_vc_browser/**, ni Hud/Font/Sprite2d/Radar,
ni config.h, ni enlaces el paquete. Avisa: "A listo, sin enlazar".
```

**Agente B — HUD, textos, escala y pantalla ancha**

```
Lee .agents/plans/mods/00-INDICE.md, luego 01-silentpatch-vc.md (tabla T2) y
03-widescreen-fixes-pack.md. Empieza por el sospechoso de los textos que salen
solos (SilentPatchVC.cpp:802 «big messages … cut sliding text» y :866 CDarkel),
que ademas explica un fallo que el jugador ve y no sabiamos de donde salia.
Ficheros: src/renderer/Hud.cpp, src/renderer/Font.cpp, src/renderer/Sprite2d.cpp,
src/renderer/Coronas.cpp, src/renderer/MBlur.cpp, src/control/Radar.cpp,
src/text/Messages.cpp, src/core/FrontEnd*.cpp. Compila SÓLO tus objetos
(sin enlazar) y anota en .agents/HISTORIAL.md. Para Streaming.cpp/ModelInfo.cpp
pidelos al agente C. NO toques gta_vc_browser/** ni config.h. Avisa: "B listo,
sin enlazar".
```

**Agente C — scripts, armas, mundo y datos del mapa**

```
Lee .agents/plans/mods/00-INDICE.md y 01-silentpatch-vc.md (tabla T1, filas de
tu tabla de dueños) y empieza por los datos: script tools/apply_ipl_diffs.py que
aplique los .ipl.diff de SilentPatchVC/Files/data/maps/* a los IPL servidos
(gta_vc_browser/streamed/), con informe de objetos tocados. Luego los arreglos
de script/armas/mundo (bloque LOD :1864 incluido). Compila SÓLO tus objetos con
ninja (sin enlazar) y anota en .agents/HISTORIAL.md. NO toques gta_vc_browser/**
salvo el script de datos, ni config.h, ni enlaces. Avisa: "C listo, sin enlazar".
```

**Sección 1 (yo)**: datos de atribución (`docs/mods/ATTRIBUTION.md`), los dos
arreglos de arranque/carga que me tocan (`main.cpp:723` contorno de la barra de
carga y `:2384` parpadeo del splash del outro), los **datos del mapa**
(`tools/apply_ipl_diffs.py` → IPL servidos, sube `dataTag`), marcas en
`check-served-build.sh`, bloques nuevos en `viceext-log-check.py` y el **enlace
único** al final de los tres.

---

## 8. Reparto por coste (esto es lo que se ejecuta, en 3 tandas)

Se ordena por **coste y riesgo**, no por mod: primero lo que no toca lógica de
juego, después lo que el jugador ve en los ficheros de cada uno, y al final lo
que cambia dibujo/escala/streaming (donde un error se nota en todo).

### Tanda 1 · liviano (no toca lógica de juego)

| Qué | Dónde | Estado (21/09/2026) |
|---|---|---|
| Manifiesto de atribución de los tres mods | `docs/mods/ATTRIBUTION.md` | **HECHO** |
| Contorno de la barra de carga escalado (SilentPatch :723) | `src/core/main.cpp` | **HECHO** `ve25` + traza `LBAR` + bloque `LB` |
| Marcas y bloques nuevos (`SIRENA tipo=`, `LBAR w=`, bloques `SR`/`LB`) | `gta_vc_browser/tools/*` | **HECHO** |
| **Datos del mapa** (T3): `tools/apply_ipl_diffs.py` con los `.ipl.diff` de 8 zonas | `tools/`, `gta_vc_browser/bootseed/**` | **COSTE REAL MEDIDO** (21/09): los `.ipl` están en `bootseed.list` (líneas 1698+), o sea **precargados**: tocarlos obliga a re-empaquetar el paquete de datos (~160 MB) y a que el jugador lo re-descargue. Por eso **NO** es tanda 1: pasa a la **tanda 2**, junto al resto de cambios de datos. El arreglo es correcto y barato de escribir; lo caro es la descarga del jugador. |
| Inventario de `Config/SilentPatchVC.ini` → candidatas a `features.ini` | `.agents/plans/mods/` | PENDIENTE (barato, sin código) |
| Splash del outro sin parpadeo (SilentPatch :2384) + duración 2,5 s (:4086) | `src/core/Frontend.cpp:2476-2496` (fade) y `:5914-5955` (tick del outro), `src/core/config.h:454-456` | **HECHO** `ve56`: clamp del paso del fade en origen (`Min(+20, 255)`) + tick 32 ms/75 cuentas ≈2,5 s a cualquier fps (`MUCH_SHORTER_OUTRO_SCREEN` off). PASS = jugador: sin frame claro y splash legible. Random de 16 bits (script/CarGen) = **YA ESTABA** (`USE_PS2_RAND`) |

### Tanda 2 · medio (lo que el jugador ve)

| Qué | Dueño | Por qué medio |
|---|---|---|
| Luces de servicio (`ambulan`/`firetruk` por dummies; `fbicar`/`vicechee` ya tenian corona P5) | A | **HECHO `ve57`** (mismo metodo que `ve24`; `fbicar`/`vicechee` sin dummies en datos: se quedan) | de `chassis_dummy` (no de extras, así que se encuentra) |
| FV: rotor **PORTADO** `ve57` (1 linea `*GetTimeStep`); autopiloto **YA ESTABA**; bocina/sirena **NO APLICA**; rueda en railes PENDIENTE (motivo en ATTRIBUTION §6) | A | auditado 24/09, sin medicion 35/120 salvo rotor (PASS del jugador) |
| SilentPatch T1: :1454 **YA ESTABA** (`FIX_BUGS`), :1372 **PORTADO** `ve57` (FBICAR a la rama aguda), :2453 **PORTADO** `ve57` (guardia `AddGunshell`) | A/C | ver ATTRIBUTION §1 |
| Mensajes grandes (:802) **YA ESTABA** (deslizamiento independiente de la resolucion por macros + `GetTimeStep`) | B | ver ATTRIBUTION §2; los textos blancos siguen por la via `SCRTXT3` |

### Tanda 3 · pesado (dibujo, escala y streaming)

| Qué | Dueño |
|---|---|
| WFP `Sprite2d`: aspecto de UI vs textura vs fundido + letterbox de cinemáticas | B |
| WFP `Radardisc` (disco del radar dibujado por código) y anclaje del HUD | B |
| WFP `Loading`/`InteriorLoading` (pantalla de carga al cambiar de isla/interior) | C |
| SilentPatch :1864 (LOD del edificio en obras), :1478 (matfx en extras), :1701 (backface) | C/B |
| Menús a pantalla completa (`Frontend`/`Menu`/`TransparentMenu`) | B |

**Regla de la tanda**: cada fila acaba en `docs/mods/ATTRIBUTION.md` con estado y
(`fichero:línea`) cuando se porte; nada se da por cerrado en `PENDIENTE` sin
motivo escrito.
