---
name: 01-silentpatch-vc
status: EXECUTED
type: feature
domain: mods
owner_rules: .agents
created: 2026-09-21
---

# Plan · SilentPatch (Vice City)

Origen: `CookiePLMonster/SilentPatch` · **MIT** © 2024 Adrian Zdanowicz («Silent»)
Código de referencia: `SilentPatchVC/SilentPatchVC.cpp` (líneas citadas abajo),
`SilentPatchVC/Files/**` (datos) y `Config/SilentPatchVC.ini` (opciones).

Por qué sí sirve: es el mejor **catálogo de bugs del motor VC** que existe, cada
uno con su explicación y las líneas exactas del binario original. Nosotros no
necesitamos los parches de bytes: nos dice **dónde** está el fallo para
arreglarlo en nuestra fuente.

> **Estado de ejecución (21/09/2026).** Hecho: `:723` (contorno de la barra de
> carga, build `ve25`) y `:1397` (coronas de sirena, resuelto por nuestra vía con
> la causa raíz, build `ve24`). `:1324` comprobado como `YA ESTABA`, `:344` como
> `NO APLICA`. El resto, con dueño, en la tabla y en `docs/mods/ATTRIBUTION.md`.
> **Ojo con el coste de T3**: los `.ipl` están en el `bootseed` (precargado), así
> que tocarlos obliga a re-empaquetar el paquete de datos (~160 MB) y que el
> jugador lo vuelva a descargar ⇒ va **al final**, no en la tanda liviana.

## Regla previa (obligatoria): comprobar antes de portar

re3 arregla por su cuenta buena parte de esto (marcado con `FIX_BUGS`). Antes de
tocar nada, buscarlo en nuestro árbol:

1. ¿Existe la función? (`grep -rn "NombreDeLaFuncion" src`)
2. ¿El código ya hace lo correcto? (compararlo con el bloque del SilentPatch)
3. Si **ya estaba**, se anota en `docs/mods/ATTRIBUTION.md` como `YA ESTABA` y
   **no se toca el código**. Si no, se porta con cabecera de atribución.

## T1 · Bugs de juego (lo que el jugador ve) — el bloque más rentable

| SilentPatchVC.cpp | Qué arregla | Nuestro destino (a confirmar) | PASS |
|---|---|---|---|
| 1864 | El LOD del edificio en obras pierde su modelo HQ y se ve **siempre** | `src/core/ModelInfo.cpp` + `src/core/Streaming.cpp` | el HQ aparece al acercarse y el LOD desaparece con él (captura antes/después) |
| 1397 | Colocación de las **coronas de sirena** (policía, Firetruck, Ambulance, Enforcer, Vice Cheetah, FBI) | `src/vehicles/Automobile.cpp` + `src/renderer/Coronas.cpp` | ✔ **HECHO por nuestra vía** (`ve24`): la causa raíz no era la posición de las coronas sino que los dummies `servicelights_*` cuelgan de los extras del `police.dff` y no se clonaban; ver `.agents/plans/mecanicas/10-seccion-1-9a-partida.md` |
| 1372 | Sonido de sirena del **FBI Washington** | `src/vehicles/Automobile.cpp` | la sirena suena al encenderla en ese modelo |
| 1454 | Coche que **explota dos veces** si el conductor se baja mientras explota | `src/vehicles/Automobile.cpp` | una sola explosión (traza de `BlowUpCar`) |
| 1967 | `CShadows::CastShadowEntityXY` ignora la rotación **Up** del objeto | `src/renderer/Shadows.cpp` | sombra girada como el objeto (objeto rotado en el suelo) |
| 1545 | `CPlane::LoadPath`: líneas sin terminador nulo | cargador de rutas de avión (`src/objects/Plane.cpp`, a localizar) | no hay lectura basura al cargar rutas (`LoadPath` sin warnings) |
| 1576 | **No** resetear la sensibilidad del ratón al empezar partida nueva | `src/core/FrontEnd*.cpp` | ajustar sensibilidad → nueva partida → sigue igual |
| 1610 | `IS_PLAYER_TARGETTING_CHAR` da positivo en controles clásicos **sin estar apuntando** | `src/control/Script6.cpp` | con arma y sin apuntar devuelve falso; apuntando, verdadero (traza) |
| 1646 | Reset de stats y variables al empezar partida nueva | `src/control/Script*.cpp` | stats a cero tras New Game (comparado con partida anterior) |
| 1952 | Objetivo de **atraco (mugging)** roto | `src/peds/PedAI.cpp`/`src/peds/Ped.cpp` | el atracador cumple el objetivo |
| 2082 | Ascensor trasero del **Skimmer** no se anima | `src/objects/Plane.cpp` (Skimmer) | el ascensor se mueve (vídeo) |
| 2138 | Los coches de policía dejan de perseguir a NPCs si el jugador **no** tiene búsqueda (backport de SA) | `src/control/CarAI.cpp` | policía sigue persiguiendo sin estrellas (traza de persecución) |
| 2168 | NPCs no usan bien el **RPG** | `src/weapons/Weapon.cpp`/`src/peds/PedFight.cpp` | un NPC con RPG dispara el proyectil (no cae muerto al suelo) |
| 2309 | Varios bugs de las **púas (stingers)** | `src/objects/Stinger.cpp` | pinchar rueda funciona y se retira bien |
| 2384 | El splash del outro **parpadea un frame** al entrar en fundido | `src/core/main.cpp`/`src/core/FrontEnd*` | sin parpadeo al fundir (vídeo) · **sección 1** |
| 2400 | Tommy no saca los puños con **nudillos** y con casi todas las armas post-III | `src/peds/PedFight.cpp`/`src/peds/Ped.cpp` | animación de puñetazo correcta al ir a por Tommy |
| 2453 | **Casquillos** salen de armas que no los echan (Python, Sniper, Laser Scope) | `src/weapons/Weapon.cpp` | disparar esas armas no genera casquillo |
| 2469 | `CPedAttractorManager::GetEffectForIceCreamVan` devuelve memoria rancia (menos clientes de los previstos) | `src/peds/PedAttractor.cpp` | el carrito de helados atrae clientes como toca (contador en traza) |
| 1478 | Environment mapping en **componentes extra** del coche | `src/renderer/*` (matfx) | el extra refleja como el resto de la carrocería |
| 1701 | Backface culling desactivado en piezas desprendidas, peds y modelos concretos | `src/renderer/VisibilityPlugins`/atomics | piezas desprendidas se ven por ambos lados (vídeo) |
| 1239/1244 | `FixedRefValue` y referencia de instancia (bici) | `src/core/ModelInfo.cpp` | la bici del mod se ve bien tras cargar |
| 1258/1266/1290 | Timers que asumen 30 fps (`GetTimeSinceLastFrame`, `NewFrameRender`, `AutoPilotTimerFix_VC`) | `src/core/Timer.cpp`, `src/control/CarAI.cpp` | ver plan FV (`02`): mismo problema, mismo arreglo |

> La tabla se completa al leer cada bloque del `.cpp` original: el implementador
> copia su **comentario de causa** (es la mejor documentación que existe del bug)
> y adapta la corrección a nuestra clase.

## T2 · HUD y escala según resolución (solapa con WFP, ver plan `03`)

Estas son de **la misma familia** que el plan de pantalla ancha: se hacen una vez
y sirven para las dos fuentes.

| SilentPatchVC.cpp | Qué arregla | Nuestro destino |
|---|---|---|
| 723 | **El contorno de la barra de carga no escala** | `src/core/main.cpp` (`LoadingScreen`, `WebDrawLoadScreen`) — hoy usamos `hpos-1.0f`/`+1.0f` fijos: es exactamente ese fallo · **sección 1** | ✔ **PORTADO** (`ve25`): el margen pasa por `SCREEN_SCALE_X/Y(1.0)`; traza `LBAR w= h= borde=`, bloque `LB` |
| 802 | Mensajes grandes que **duran más** a alta resolución (recorte del texto deslizante) | `src/text/Messages.cpp` + `src/renderer/Hud.cpp` ← **sospechoso nº1 de los «textos blancos» del jugador** (bloque R3b) |
| 866 | Texto deslizante de **`CDarkel`** | `src/control/Darkel.cpp` |
| 947 | Sombras de texto que no escalan | `src/renderer/Font.cpp` |
| 1000 | Padding del fondo de texto que no escala | `src/renderer/Font.cpp` |
| 1071 | `Y` del texto de Ammu-Nation (mensaje grande tipo 3) | `src/renderer/Hud.cpp` |
| 1105 | Setup de vértices del contorno del blip de destino (leyenda del mapa) | `src/renderer/Hud.cpp`/`Radar` |
| 1121 | Ajustes de línea (`line wraps`) que no escalan | `src/renderer/Font.cpp` |
| 1189 | Sombra del «You are here» | `src/control/Radar.cpp` |
| 541/613/662/773/896 | Posición del radar y su sombra, contador en pantalla, contorno del blip del radar, créditos, «minimal HUD» | `src/control/Radar.cpp`, `src/renderer/Hud.cpp`, `src/core/FrontEnd*` |
| 1931 | Coronas de luz (`flares`) que no escalan | `src/renderer/Coronas.cpp` |
| 1999/2063 | Sprites de **script** a resolución + filtrado bilineal | `src/renderer/Sprite2d.cpp` |
| 2196 | `CMBlur::AddRenderFx`: márgenes de las «zonas seguras» mal escalados | `src/renderer/MBlur.cpp` |
| 344 | Clip del cursor a la ventana | **NO aplica** (no hay ventana nativa; el puntero es del navegador) |
| 1324 | Latencia del teclado (búferes de entrada) | **YA ESTABA**: comprobado en `src/core/Pad.cpp` (copia `OldKeyState`/`NewKeyState` en el orden correcto) |

## T3 · Datos del juego (sin código) — hacer primero, es barato

`SilentPatchVC/Files/data/maps/*` trae **parches de IPL** de 8 zonas:
`club`, `hotel`, `littleha`, `mansion`, `oceandn`, `oceandrv`, `stripclb`,
`washints` (objetos mal colocados en el mapa original).

Plan: un script `tools/apply_ipl_diffs.py` que aplique esos `.diff` a los IPL
**servidos** (los de `gta_vc_browser/streamed/`), con informe de qué línea cambia
y cuántos objetos toca. Al ser cambio de **dato**, sube `dataTag` (`ve13` → `ve14`)
y la caché se purga sola. Verificación: los objetos corregidos se ven en su sitio
(comparar con vídeo del original si hace falta).

También del repo, para inventariar opciones que aún no tenemos:
`Config/SilentPatchVC.ini` (leer y listar; las que aportemos van a
`features.ini`, como ya se hace con las opciones del mod).

## Lo que NO se copia

- Nada de `DDraw/`, `dllmain`, `hook::pattern`, offsets de versión (Steam/JP),
  ni el sistema SVF: es infraestructura de la **binaria original**.
- Ningún asset del repo (no los hay aquí, pero tampoco del `.ini`).

## Criterio de cierre del plan

Cada fila acaba en `docs/mods/ATTRIBUTION.md` con uno de estos estados:
`PORTADO` (con fichero:línea), `YA ESTABA`, `NO APLICA`, `PENDIENTE`. El plan no
se da por cerrado teniendo filas `PENDIENTE` sin motivo escrito.
