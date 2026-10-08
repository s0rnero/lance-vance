---
name: 03-widescreen-fixes-pack
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-21
---

# Plan · WidescreenFixesPack (GTAVC.WidescreenFix)

Origen: `ThirteenAG/WidescreenFixesPack` · **MIT** © 2018 ThirteenAG
Código: `source/GTAVC.WidescreenFix/*.ixx` (módulos por clase del motor) ·
Datos: `data/GTAVC.WidescreenFix/` (`d3d8.ual` + `scripts`) · assets:
`textures/`, `resources/`.

## Por qué este es el plan “de casa”: ya tenemos media base

Nuestro motor (re3) ya trae soporte de pantalla ancha: `SCREEN_SCALE_X/Y`,
`SCREEN_STRETCH_X/Y`, `SCREEN_SCALE_FROM_BOTTOM`, `CDraw::CalculateAspectRatio()`
y `TheCamera.m_fFOV_Wide_Screen` (`src/core/Camera.cpp:2766`). Por eso **no
copiamos** los parches de bytes del WFP: portamos la **lógica** que le falta a
nuestra capa 2D y a la cámara.

## Mapa: módulo del WFP → nuestro fichero

| `.ixx` | Contenido (lo aprovechable) | Nuestro destino |
|---|---|---|
| **`Sprite2d.ixx`** | **La joya**: geometría 4:3 del modo cinemática, tamaño real del letterbox, «textured fullscreen» vs «elemento UI», escalado de UI, fundidos con `m_bFading` ya limpio (fundidos de tiempo cero) | `src/renderer/Sprite2d.cpp` (+ `CutsceneMgr`) |
| `Radardisc.ixx` | Si el disco del radar viene del TXD en 64×64, **lo sustituye por un anillo dibujado por código** en alta calidad (`:70-156`) | `src/control/Radar.cpp` |
| `CutsceneMgr.ixx` | Barras del modo cinemática según aspecto | `src/animation/CutsceneMgr.cpp` |
| `Hud.ixx` | Anclaje del HUD a los bordes («mover HUD a izquierda/derecha» según lo que se esté pintando) | `src/renderer/Hud.cpp` |
| `Frontend.ixx`, `Menu.ixx`, `TransparentMenu*.ixx` | Menús a pantalla completa, menú translúcido | `src/core/FrontEnd*.cpp` |
| `Loading.ixx`, `InteriorLoading.ixx` | `gbNoIslandLoading`, `gbNoInteriorLoading`: quitar la pantalla de carga al cambiar de isla/interior | `src/core/Streaming.cpp`, `src/core/Game.cpp` |
| `Camera.ixx` | Estructuras de cámara y FOV | (referencia; nuestro `Camera.cpp`) |
| `Radardisc`, `Draw`, `Entity`, `Vehicle`, `Physical`, `Placeable`, `PostFX`, `ModelInfo`, `VisibilityPlugins`, `MSAA`, `Timer`, `Misc` | Módulos por sistema: usar como **referencia de síntomas** (cada uno documenta un fallo de aspecto/LOD/2D) | según el caso |
| `Skeleton.ixx`, `common.h`, `rw.h`, `dllmain.cpp` | Infraestructura del mod (hookeo, plugin-sdk) | **NO se copia** |

## Qué portar, por valor

**Alto (se ve en cada partida):**

1. **Lógica de aspecto de `CSprite2d`** (`Sprite2d.ixx`): es la que decide si un
   elemento es UI (escalar), textura a pantalla completa (recortar) o fundido
   (estirar). Nuestra capa 2D hoy usa `SCREEN_SCALE_*` a pelo en muchos sitios:
   comparar caso por caso y adoptar su criterio. **PASS**: a 16:9 y a 21:9 nada
   de UI se estira y los fundidos siguen tapando la pantalla entera.
2. **Disco del radar en alta calidad dibujado por código** (`Radardisc.ixx`):
   sin assets, sin datos nuevos. **PASS**: el borde del radar no se ve pixelado
   al ampliar.
3. **Letterbox de cinemáticas** (`CutsceneMgr.ixx` + `Sprite2d.ixx`): barras
   correctas en cualquier aspecto. **PASS**: captura en 16:9 y 4:3.
4. **Anclaje del HUD** (`Hud.ixx`): que el dinero/armas/radar no se peguen al
   centro en pantallas anchas. **PASS**: vídeo + captura.

**Medio:**

5. **Saltar la pantalla de carga** al cambiar de isla/interior
   (`Loading.ixx`/`InteriorLoading.ixx`): en la web ese salto ya lo troceamos,
   pero la pantalla extra se sigue viendo. **PASS**: cambio de isla sin pantalla
   intermedia de carga.
6. **Menús** (`Frontend.ixx`, `Menu.ixx`, `TransparentMenu*.ixx`).
7. **`ModelInfo.ixx`/`VisibilityPlugins.ixx`**: leer como referencia para el
   asunto del **LOD** que el jugador ha reportado y que ya tocamos (distancia de
   dibujado +20 m; `ModelInfo.cpp`/`Streaming.cpp` son del agente C).

## Qué NO se copia (y por qué)

- **Assets** (`textures/`, `resources/`): son arte derivado del juego. Si un
  arreglo pide una textura mejor, se **genera por código** (como hace su propio
  `Radardisc`).
- `data/GTAVC.WidescreenFix/d3d8.ual` y `scripts`: parches binarios del render
  D3D8 original. Aquí el render es librw.
- Patrones de bytes, `hook::pattern`, `SafetyHook`, `GameRef`, `MSAA.ixx`
  (estados D3D8/DX9 de MSVC que no existen en WebGL/GLES).

## Cómo se verifica (sin depender del ojo del jugador)

- **Captura por aspecto**: el motor corre en el navegador; se redimensiona la
  ventana a 4:3, 16:9 y 21:9 y se guardan capturas (`preview_screenshot` o
  fotogramas de vídeo con `tools/frames-at.sh`).
- **Traza de aspecto**, una línea al cambiar el tamaño:
  `ASPECT w= h= ratio= fov= hudAnchor=`. Con eso, el log demuestra que la lógica
  vio cada aspecto y qué decidió (y `check-served-build.sh` gana la marca).
- **Bloque `A`** en `tools/viceext-log-check.py`: comprueba que en cada aspecto
  visitado el anclaje y el FOV son los esperados.

## Orden y dueño

Este plan es del **agente B** (HUD/textos/escala), que ya es dueño de
`Hud.cpp`, `Font.cpp`, `Sprite2d.cpp`, `Radar.cpp` y `FrontEnd*`. Los módulos que
toquen `Streaming.cpp`/`ModelInfo.cpp` (índice 5 y 7) se **piden al agente C** en
vez de escribirlos: un fichero, un dueño.
