# Atribución · arreglos portados de mods de la comunidad

Regla de trabajo de `.agents/plans/mods/00-INDICE.md` §2: **todo** lo que venga de
estos mods se anota aquí, con el fichero y la línea de nuestro motor. Este
documento es, a la vez, el crédito y la prueba de que no se copia a ciegas.

Los tres mods son **MIT** (se puede usar su código dando crédito):

| Repo | Licencia | Autoría |
|---|---|---|
| [CookiePLMonster/SilentPatch](https://github.com/CookiePLMonster/SilentPatch) | MIT | © 2024 Adrian Zdanowicz «Silent» |
| [GTAmodding/FramerateVigilante](https://github.com/GTAmodding/FramerateVigilante) | MIT | © 2023 GTA modding (Junior_Djjr) |
| [ThirteenAG/WidescreenFixesPack](https://github.com/ThirteenAG/WidescreenFixesPack) | MIT | © 2018 ThirteenAG |

> **NORMA VIGENTE (27/09/2026):** la licencia **no es una puerta**. Proyecto
> local y personal: de `mods/` se toma lo que haga falta con o sin LICENSE. Este
> documento y las cabeceras de atribución se mantienen como **cortesía y
> trazabilidad**, no como obligación. Lo único excluido es lo técnicamente
> inaplicable en WASM.

Formato de la cabecera obligatoria, encima de cada bloque portado:

```cpp
// ---------------------------------------------------------------------------
// PORTADO — <Mod> (MIT, © <año> <autor>)
//   <URL>
//   <fichero>:<línea>  («<título del bloque en el mod>»)
// Qué se toma: <la idea/medición>. Adaptación: <qué cambia en nuestro motor>.
// ---------------------------------------------------------------------------
```

Estados: `PORTADO` (con `fichero:línea` nuestro) · `YA ESTABA` (re3 ya lo
arreglaba; se comprueba y **no se toca**) · `NO APLICA` (atado a la binaria
original, a Windows/D3D8 o a una ventana nativa) · `PENDIENTE` (con dueño).

**Lo que no se usa** (motivo **técnico**, no licencia): direcciones, `hook::pattern`,
`.ual`/ASI, `plugin-sdk`, `injector` — atados a la binaria x86 de Windows. Los
assets no se copian porque no hacen falta: el juego los sirve desde tu copia
local, y lo que no exista se **dibuja por código**.

---

## 1 · SilentPatch — bugs de juego (T1)

| Origen (`SilentPatchVC.cpp`) | Qué arregla | Nuestro destino | Estado |
|---|---|---|---|
| 1864 | El LOD del edificio en obras se queda visible siempre | `src/modelinfo/ModelInfo.cpp`, `src/core/Streaming.cpp` | PENDIENTE (C) |
| 1397 | Colocación de las coronas de sirena (policía, FBI, Vice Cheetah, ambulancia, bomberos, Enforcer) | `src/vehicles/Automobile.cpp:1838-2000` | **PORTADO** por nuestra vía (`ve24`): causa raíz real = los dummies `servicelights_*` cuelgan de `extra1..3` y no se clonaban con el componente; ver `.agents/plans/mecanicas/10-seccion-1-9a-partida.md` |
| 1372 | Sirena del FBI Washington (sonido) | `src/audio/AudioLogic.cpp` (rama aguda `FBIRANCH \|\| FBICAR` -> 12668 Hz) | **PORTADO** (`ve57`): `FBICAR=17`/`FBIRANCH=90` ya eran el mismo enum de audio; se anade `FBICAR` a la rama aguda. PASS = oido del jugador |
| 1454 | Coche que explota dos veces | `src/vehicles/Vehicle.cpp` (`CVehicle::RemoveDriver`) | **YA ESTABA**: `#ifdef FIX_BUGS if (GetStatus() != STATUS_WRECKED) SetStatus(STATUS_ABANDONED)` (mismo `if` del mod); `FIX_BUGS` activo en `config.h:304`. No se toca codigo |
| 1967 | `CShadows::CastShadowEntityXY` ignora la rotación Up | `src/renderer/Shadows.cpp` | PENDIENTE (A) |
| 1545 | `CPlane::LoadPath`: líneas sin terminador nulo | `src/objects/Plane.cpp` | PENDIENTE (A) |
| 1576 | Reset de la sensibilidad del ratón al empezar partida nueva | `src/core/Frontend.cpp` | PENDIENTE (B) |
| 1610 | `IS_PLAYER_TARGETTING_CHAR` positivo sin estar apuntando | `src/control/Script6.cpp` | PENDIENTE (C) |
| 1646 | Reset de stats al empezar partida nueva | `src/control/Script*.cpp` | PENDIENTE (C) |
| 1952 | Objetivo de atraco (mugging) roto | `src/peds/PedAI.cpp` | PENDIENTE (A) |
| 2082 | Ascensor trasero del Skimmer no se anima | `src/objects/Plane.cpp` | PENDIENTE (A) |
| 2138 | La policía deja de perseguir NPCs si el jugador no tiene búsqueda | `src/control/CarAI.cpp` | PENDIENTE (A) |
| 2168 | NPCs no usan bien el RPG | `src/weapons/Weapon.cpp`, `src/peds/PedFight.cpp` | PENDIENTE (C) |
| 2309 | Bugs de las púas (stingers) | `src/objects/Stinger.cpp` | PENDIENTE (A) |
| 2384 | El splash del outro parpadea un frame al fundir | `src/core/Frontend.cpp:2476-2496` | **PORTADO** (`ve56`): clamp del paso del fade en origen `Min(m_nMenuFadeAlpha + 20, 255)` (mismo criterio que `m_firstStartCounter`) con cabecera PORTADO encima; el frame con alfa 260 (splash truncado a 4/logo a 251) ya no existe. Criterio PASS = fundido del outro sin frame claro (jugador) |
| 2400 | Tommy no saca los puños con nudillos/nuevas armas | `src/peds/PedFight.cpp` | PENDIENTE (A) |
| 2453 | Casquillos salen de armas que no los echan (Python, Sniper, Laser) | `src/weapons/Weapon.cpp` (`FireInstantHit`, guardia en el `AddGunshell` del caso) | **PORTADO** (`ve57`): el enum ya coincide (18/28/29); el fogonazo/humo se quedan. PASS = Python/franco/laser sin casquillo, Colt45 con el |
| 2469 | `GetEffectForIceCreamVan` devuelve memoria rancia | `src/peds/PedAttractor.cpp` | PENDIENTE (A) |
| 1478 | Environment mapping en los extras del coche | `src/renderer/*` (matfx) | PENDIENTE (B) |
| 1701 | Backface culling en piezas desprendidas, peds y modelos concretos | `src/rw/VisibilityPlugins.cpp` | PENDIENTE (B) |
| 1239/1244 | `FixedRefValue` y referencia de instancia (bici) | `src/modelinfo/ModelInfo.cpp` | PENDIENTE (C) |
| 1258/1266/1290 | Timers que asumen 30 fps | `src/core/Timer.cpp`, `src/control/CarAI.cpp` | PENDIENTE (A, junto al plan FV) |
| 1324 | Latencia del teclado (búferes de entrada) | `src/core/Pad.cpp:1880` | **YA ESTABA** (comprobado: `OldKeyState = NewKeyState; NewKeyState = TempKeyState;` en el orden correcto) |
| 344 | Clip del cursor a la ventana | — | **NO APLICA** (no hay ventana nativa; el puntero es del navegador) |

| 4086 | Duración del outro splash (≈1,5-5 s según fps; en nuestro build eran 750 ms por `MUCH_SHORTER_OUTRO_SCREEN`) | `src/core/Frontend.cpp:5914-5955`, `src/core/config.h:454-456` | **PORTADO** (`ve56`): tick `>32` ms + cuenta 75 ≈2,5 s a cualquier fps; `MUCH_SHORTER_OUTRO_SCREEN` desactivado. PASS = el splash se ve ≈2,5 s legibles y sale solo |
| B6 (CHANGELOG) | Random de **script** de 16 bits (era de 15) | `src/core/common.h:326` (`MYRAND_MAX`) | **YA ESTABA**: `USE_PS2_RAND` activo (`config.h:276`) ⇒ `MYRAND_MAX` = 65535; no se toca código |
| B6 (CHANGELOG) | Random de **spawn de coches** de 16 bits | `src/vehicles/CarGen.cpp` (vía `GetRandomNumber`) | **YA ESTABA**: ídem `USE_PS2_RAND`; no se toca código |

## 2 · SilentPatch — HUD y escala por resolución (T2) · solapa con el WFP

| Origen | Qué arregla | Nuestro destino | Estado |
|---|---|---|---|
| 723 | El contorno de la barra de carga no escala | `src/core/main.cpp` (`LoadingScreen`, `WebDrawLoadScreen`) | **PORTADO** (`ve25`): margen del contorno por `SCREEN_SCALE_X/Y(1.0)`; traza `LBAR w= h= borde=`, bloque `LB` del verificador |
| 802 | Mensajes grandes que duran mas a alta resolucion (recorte del texto deslizante) | `src/renderer/Hud.cpp` (`BigMessageX[0]/[1]`) | **YA ESTABA** (24/09): velocidad (`SCREEN_SCALE_X(0.3/ms)`) y distancia escalan juntas -> el deslizamiento dura igual a cualquier resolucion; el hold de 120 usa `GetTimeStep` (tiempo, no frames). No se toca codigo |
| 866 | Texto deslizante de `CDarkel` | `src/control/Darkel.cpp` | PENDIENTE (C) |
| 947 / 1000 / 1121 | Sombras de texto, padding del fondo y ajustes de línea que no escalan | `src/renderer/Font.cpp` | PENDIENTE (B) |
| 1071 / 1105 | `Y` del texto de Ammu-Nation; vértices del blip de destino | `src/renderer/Hud.cpp`, `src/core/Radar.cpp` | PENDIENTE (B) |
| 1189 | Sombra del «You are here» | `src/core/Radar.cpp` | PENDIENTE (B) |
| 541/613/662/773/896 | Radar y su sombra, contador en pantalla, contorno del blip, créditos, minimal HUD | `src/core/Radar.cpp`, `src/renderer/Hud.cpp`, `src/core/Frontend.cpp` | PENDIENTE (B) |
| 1931 | Coronas de luz (`flares`) que no escalan | `src/renderer/Coronas.cpp` | PENDIENTE (B) |
| 1999/2063 | Sprites de script a resolución + filtrado bilineal | `src/renderer/Sprite2d.cpp` | PENDIENTE (B) |
| 2196 | `CMBlur::AddRenderFx`: márgenes de zonas seguras mal escalados | `src/renderer/MBlur.cpp` | PENDIENTE (B) |

## 3 · SilentPatch — datos del mapa (T3, sin código)

| Origen | Qué arregla | Nuestro destino | Estado |
|---|---|---|---|
| `SilentPatchVC/Files/data/maps/*.ipl` (8 zonas) | Objetos mal colocados: `club`, `hotel`, `littleha`, `mansion`, `oceandn`, `oceandrv`, `stripclb`, `washints` | IPL servidos (`streamed/` = ficheros de ViceEx byte a byte) | **YA CUBIERTO** (24/09, `ve57`, 0 cambios): diff hunk a hunk P-vs-E en las 8 zonas (72 hunks): el unico fix sustantivo (club: fuera `club_exterior03/04`) ya falta en los ficheros de ViceEx; `hotel` es ruido float (1 vs 0.99999988, `e-08` vs `e-008`) + `hotroomfan` con rotacion quaternion-equivalente (dot=-1.0); el resto son flags normalizados por ViceEx (13->0) y anadidos suyos (chandelier, puerta, secciones `cull`) que se conservan. Copiar los ficheros SP revertiria eso + costaria re-descarga de 160 MB. Sin `tools/apply_ipl_diffs.py`: no hace falta |
| `Config/SilentPatchVC.ini` | Inventario de opciones del mod que aún no tenemos (candidatas a `features.ini`) | — | PENDIENTE (inventario) |

## 4 · FramerateVigilante — sitios frágiles a FPS altos

| Origen | Qué arregla | Nuestro destino | Estado |
|---|---|---|---|
| `rotorFinalSpeed` | Velocidad del rotor de helicoptero, sumada por frame | `src/vehicles/Heli.cpp` (`CHeli::Render`: `+= (3.14f/6.5f) * CTimer::GetTimeStep()`) | **PORTADO** (`ve57`): 1 linea; el wrap a 6.28 igual. PASS = mismas vueltas/s a 35 y 120 fps |
| Rampa por frame (`+= 0.001f`) | Spool del heli IA: a 120 fps despegaba ~2,4x antes que a 50 | `src/control/CarCtrl.cpp:2757` (`SteerAIHeliTowardsTargetCoors`: `+= 0.001f * CTimer::GetTimeStep()`) | **PORTADO** (bloque 5/5): topes 0.22/0.15 igual; gemelo del heli del jugador en `src/vehicles/Automobile.cpp:1421-1425` pendiente de su dueno. PASS = mismo tiempo de despegue a 35 y 120 fps (jugador) |
| Spool del heli del JUGADOR (gemelo del anterior) | `src/vehicles/Automobile.cpp` (`+= 0.001f/0.003f * CTimer::GetTimeStep()`) | **PORTADO** (`ve58`). PASS = mismo tiempo de spool a 35 y 120 fps |
| Recoil por precision del arma (spec `WeaponRecoilAuto` + jenksta) | `src/weapons/Weapon.cpp` (acumulador `kick*acc(spread,range)` + `ViceExtRecoilPitch()` libre) + `src/core/Cam.cpp` (la camara recibe `kickAlpha*odAcc`; `multY` intacto en 0.400) | **PORTADO** (`ve58`). Cableado B1+B4 en 1 linea. PASS = rafaga sube la vista a escalones y vuelve; `RECOIL2 ... acc=` |
| Nado curva Serega + agachado calibrado (spec sa-crouch) | `src/peds/PlayerPed.cpp` (`ViceExtSwim*`: 0.022/0.05/0.12 + timeout 300; crouch 2.74m/0.731s + deadzone +-16 + filtro armas) + `src/peds/Ped.cpp` (R20c intacto) | **PORTADO** (`ve58`). Traza viva `SWIMNAT` (una/sesion: fija la marca del check que el postlink podaba del DWARF). PASS = `SWIM2 avance~=spd`, `CROUCHMOVE mps~=0.90` + ojo |
| Apuntado ClassicAXIS+GeniusZ (hombro, CrosshairMult, sensibilidades, near-clip dual, seguimiento coche) | `src/core/Cam.cpp`, `src/core/Camera.cpp`, `src/renderer/Hud.cpp` | **CORREGIDO 27/09: la fila decía `PORTADO (ve58)` y era FALSA** — de todo lo que listaba, en el árbol solo había dos cosas, y las dos se corrigen en el plan `apuntado-classicaxis-100`. (a) El `CrosshairMult 0.53/0.4` es `m_f3rdPersonCHairMultX/Y`, un **campo de la propia GTA** puesto en `Camera.cpp:283-284`: **YA ESTABA**, no un port. (b) El hombro `0.2f` (`Cam.cpp:606-610`) también **YA ESTABA** y además **ya iba en espacio de objeto** del ped: `CamTargetEntity->GetRight()` es `CPlaceable::GetRight()` → `m_matrix.GetRight()` (`Placeable.h:6,20`), o sea la matriz **de la entidad**, que es justo lo que hace el `TransformFromObjectSpace` del mod (`CamNew.cpp:265`); el propio árbol lo dice en `Cam.cpp:1778-1781`. (c) `near-clip dual` = `ViceExtNearClipOnFoot/InCar` (`Camera.cpp:289-295`), los dos a `DEFAULT_NEAR`: **no-op**, sin ningún valor (y es del GeniusZ, otro carril, §5.6). (d) Las sensibilidades por eje y el `WalkKey` **no existen**; la fórmula `*FOV/80` sí, pero con el `0.01f` fijo. Todo lo demás que el mod hace al apuntar (la ley `Process_AimWeapon` entera, el FOV 50 del rifle, el `horShift/verShift` del lock-on, `ForceAutoAim`, `ShowTriangleForMouseRecruit`, `StoriesPointingArm`, los 3 `PedIK.MoveLimb`, `Process_CrouchOffset`) **no existía**: el apuntado seguía siendo `Process_Syphon` (vanilla) con `MODE_AIMING` muerto en `Cam.cpp:396`. **AHORA**: PENDIENTE (`aim2`-`aim5`, plan `apuntado-classicaxis-100`). PASS = `AX` + las 13 trazas `AIM*` |
| `CarWheelOnRailsSpinFix` | Giro de rueda en railes, acumulado por frame | `src/control/CarCtrl.cpp` (`UpdateCarOnRails`) + `src/vehicles/Automobile.cpp:475-477`, `src/vehicles/Bike.cpp:333-335` | **YA ESTABA** (bloque 5/5): `UpdateCarOnRails` no tiene acumulador (todo en ms); el giro real en railes (`STATUS_SIMPLE`) pasa por `ProcessWheelRotation`, que ya devuelve `angularVelocity * CTimer::GetTimeStep()` (`src/vehicles/Vehicle.cpp:1105`). Sin cambios. PASS = mismo giro a 35 y 120 fps (jugador) |
| Bocina/sirena | El rebote de la entrada y los parpadeos por numero de frame se rompen a FPS altos | `src/vehicles/Automobile.cpp:1378/1587` | **NO APLICA** (24/09): el toggle es por flanco de entrada y el `&7` solo re-emite `MakeWayForCarWithSiren` (idempotente); el parpadeo visible va por ms (`&0x3FF`). No se toca codigo |
| `AutoPilotTimerFix_VC` | Timers del autopiloto contados en frames | `src/control/CarAI.cpp` | **YA ESTABA** (24/09): 0 contadores en frames; autopiloto ya en `GetTimeInMilliseconds`. No se toca codigo |
| `_fpsLimit`/`autoLimitFPS`, retardo de 14 ms | Límite de FPS por frecuencia del monitor y retraso de frame de Windows | — | **NO APLICA** (aquí manda el tope de `skel/glfw`; en web el frame lo da el navegador) |

## 5 · WidescreenFixesPack (GTAVC)

| Origen (`.ixx`) | Qué se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| `Sprite2d` | Geometría 4:3 del modo cinemática, letterbox, «textured fullscreen» vs UI, fundidos limpios | `src/renderer/Sprite2d.cpp`, `src/animation/CutsceneMgr.cpp` | PENDIENTE (B) |
| `Radardisc` | Disco del radar en alta calidad **dibujado por código** (sin assets) | `src/core/Radar.cpp` | PENDIENTE (B) |
| `CutsceneMgr` | Barras del modo cinemática según aspecto | `src/animation/CutsceneMgr.cpp` | PENDIENTE (B) |
| `Hud` | Anclaje del HUD a los bordes en pantallas anchas | `src/renderer/Hud.cpp` | PENDIENTE (B) |
| `Frontend`, `Menu`, `TransparentMenu*` | Menús a pantalla completa y translúcidos | `src/core/Frontend*.cpp` | PENDIENTE (B) |
| `Loading`, `InteriorLoading` | Quitar la pantalla de carga al cambiar de isla/interior | `src/core/Streaming.cpp`, `src/core/Game.cpp` | PENDIENTE (C) |
| `Camera` | Estructuras de cámara y FOV (referencia) | `src/core/Camera.cpp` | PENDIENTE (referencia) |
| `d3d8.ual`, `scripts`, `MSAA`, `Skeleton`, `common.h`, `rw.h`, `dllmain` | Parches binarios D3D8/DX9 y hookeo | — | **NO APLICA** (render librw; no hay D3D8 ni DLL que inyectar) |
| `textures/`, `resources/` | Assets de arte derivado del juego | — | **NO APLICA** (no se copian; si hace falta, se dibuja por código) |


## 6 - Tanda 2 (24/09, `ve57`) y veredictos de Tanda 3

| Origen | Que se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| SilentPatch `EnableVehicleCoronaFixes` (luces `ambulan`/`firetruk`) | `servicelights_0` cuelga de `chassis_dummy` (verificado en los `.dff` servidos) | `src/vehicles/Automobile.cpp` (casos `MI_FIRETRUCK`/`MI_AMBULAN`: bloque dummy-first + fallback fijo, mismo patron que R11/ve24) | **PORTADO** (`ve57`). `fbicar`/`fbiranch`/`vicechee` ya tenian su corona (P5) y sus `.dff` NO traen dummies -> se quedan como estan. PASS = barra roja/azul sobre la cabina (jugador) |
| ViceEx banco propio (13 muestras) + oido FINAL (24/09, `viceex-map.tsv`) | shotgun2->03/04, desert eagle->05/06, steyr->09/10, lanzagranadas->11 | `src/audio/AudioLogic.cpp` (`SOUND_WEAPON_SHOT_FIRED`: grupo ViceEx a 4 armas + `BERETTA`->`COLT45`, `UZIOLD`->`UZI`, `AK47`/`M16`->`M4/RUGER` de serie) | **PORTADO** (`ve57`). 0/1 rotas vacias (manda la serie); 2/7/8/12 sin enganchar (recarga ya suena por familia; swim necesita id de sonido nuevo: PENDIENTE auditado). Sin cambio de datos (el banco ya se servia): `dataTag` no sube. PASS = oido del jugador |
| ClassicAXIS `bForceLegsMovements` (moverse con armas a dos manos) | Forzar locomocion de piernas apuntando con 2H | `src/peds/PlayerPed.cpp` (H4: `odAimWalkActive/Uncapped`) | **YA ESTABA** (24/09): H4 elige el clip por velocidad sin tope para TODA arma apuntable, incluidas pesadas/2H. No se toca codigo |
| ClassicAXIS **C1/C2** `playerMovementType` + `playerShootingDirection` (Main.cpp:121-152) | El cuerpo gira hacia donde caminas; de costado solo apuntando; y el 360 con el raton parado | `src/peds/PlayerPed.cpp` (`ViceExtStrafeAiming`/`ViceExtIsAimingNow`, `PlayerControl1stPersonRunAround`) + `src/core/Cam.cpp` (`Process_FollowPedWithMouse`) | **PORTADO** (`ve74` / `aim4`, 28/09). La «regla previa» (12-handoff §4.2) queda **ANULADA** el 28/09: dio por hecho que el motor ya lo hacia y el defecto («el cuerpo gira siempre con la camara») seguia. Sin apuntar -> `TYPE_WALKAROUND`; apuntando -> `TYPE_STRAFE`; `forceRealMoveAnim` (Main.cpp:1307-1308) se CONSUME ahora (antes era un flag muerto). PASS = traza `AX7` (andando de lado sin apuntar, `desv` < 15 grados) |
| ClassicAXIS **C4** «No fight cam» (Main.cpp:113-117, 2 nops) | La camara nunca entra en fight cam | `src/core/Camera.cpp` (bloque «Check if entering fight cam», los dos `ReqMode = MODE_FIGHT_CAM`) | **PORTADO** (`ve74`, 28/09). Los dos nops 1:1 con los dos `ReqMode`. PASS = pelear a punos o con bate no cambia el encuadre |
| ClassicAXIS **C9** `doWeaponSmoothSpray` (Main.cpp:387-404) | Spray suave: `0.00001f` agachado, `-1.0f` si no | `src/peds/PlayerPed.cpp::DoWeaponSmoothSpray` | **PORTADO** (`ve74`, 28/09). El mod SUSTITUYE la funcion entera; aqui se anula el `switch` del motor (pistolas a `PI/112` etc.). PASS = sin tiron lateral al girar la camara |
| ClassicAXIS **C11** `clearAimFlag` (Main.cpp:207-211) | `ClearAimFlag()` si no se esta apuntando | `src/peds/PlayerPed.cpp::ProcessPlayerWeapon` (los 2 callsites de `SetAimFlag`) | **PORTADO** (`ve74`, 28/09). PASS = el ped no se queda con la pose de apuntado colgada |
| ClassicAXIS **R1** en la ley de A PIE (CamNew.cpp:82-86, :215-220, :411-429) | Offset de agachado, camara sobre el agua (nivel+0,6) y esconder peds a <0,5 m — tambien al caminar | `src/core/Cam.cpp::Process_FollowPedWithMouse` | **PORTADO** (`ve74`, 28/09). Antes los tres solo estaban en la ley de APUNTADO. PASS = agachado el encuadre baja ~0,5 m; en agua la camara no se hunde |
| ClassicAXIS **C18-5** (Main.cpp:1364-1378) | El assoc del arma entra con `blendAmount=0` + `blendDelta=8` (entrada rapida) | `src/peds/PedFight.cpp:220-226` | **YA ESTABA** — y lo hizo el **carril de agachado (R29)**, no este. El 28/09 se dio por PENDIENTE sin leerlo; corregido al leer (regla 12-handoff §4.2, en los dos sentidos) |
| ClassicAXIS **`StoriesAimingCoords`** | — | — | **NO APLICA**: 0 coincidencias en `Main.cpp` y `CamNew.cpp`. El identificador **no existe en el mod**: no hay nada que portar |
| ClassicAXIS **`bForceLegsMovements`** | — | — | **NO APLICA con ese nombre**: 0 coincidencias en el mod. Su equivalente real es **`forceRealMoveAnim`** (`Main.cpp:1307-1308`), portado dentro de C1/C2. La fila de arriba («YA ESTABA», 24/09) queda **sustituida** por esta |
| ClassicAXIS **C5** «No point gun cam» (Main.cpp:219-222, 3 nops) | Sin camara de point-gun | — | **PENDIENTE (binario)**: son 3 parches de bytes crudos sin código fuente que traducir, y en este motor **no existe `MODE_POINT_GUN`**. Hace falta el binario de VC desensamblado para identificar los 3 bloques. **No se inventa** |
| ClassicAXIS **C7** «Fix jump fail» (Main.cpp:118) | El salto que no dispara | — | **PENDIENTE (binario)**: `Set<BYTE>(0x4F0031, 0xEB)`, un byte que convierte un salto condicional en incondicional, sin codigo fuente. La funcion es la de la locomocion (0x4F002A es una de las 6 de `playerMovementType`), pero la condicion exacta no se puede leer sin el binario. **No se inventa** |
| SkyGfx `nightParam` (D14) | Modulacion prelight dia/noche por pipeline | `vendor/librw/src/gl/shaders/` (~4 shaders) + uniform por frame | PENDIENTE (motivo 24/09): no es quirurgico (plumbing UBO/no-UBO + uniforme por frame) y sin 2o set de colores en los DFF no hay con que mezclar: el look del mod no seria verificable. Spec lista en plan `10` B9 |
| WFP FOV/HUD (`ConvertFOV`, `FOVManager`, `HudOffset`, `ScaleRect`, pillar/letterbox) + `Radardisc` + `Loading` + menus + ClassicAXIS camara + SkyGfx passes | Escala/aspecto/camara/render | `src/core/Camera.cpp`, `src/renderer/*`, `src/core/Radar.cpp`, `src/core/Streaming.cpp` | PENDIENTE (motivo 24/09): base ya presente (`m_fFOV_Wide_Screen`, `CalculateAspectRatio`, `GetScaledFOV`); cada extra es un proyecto con PASS visual (capturas 4:3/16:9/21:9), ninguno es de una linea. Spec en planes `10` B7/B8/B9 y `03` |
| SilentPatch `:1864` (LOD obra), `:1478` (env en extras), `:1701` (backface) | Render | `src/core/ModelInfo|Streaming`, render matfx, `VisibilityPlugins` | PENDIENTE (motivo 24/09): ninguno tiene ancla de una linea en nuestro arbol (0 resultados para `bldngst2mesh`/`DrawBackfaces`); son proyectos de render con PASS visual. Spec en plan `01` T1 |
| WFP `No Island Loading` / `Seamless Interiors` (D15) | Mundo entero residente | `ondemand.js` CAP 360->~700 MB | PENDIENTE (el ultimo, por diseno): RSS wasm 1,2-1,8 GB, solo-desktop al principio, medir en vivo con `odtrace` antes de subir caps |
| Seguimiento coche (BARRIDO `ve59`, ley unica) — **CORREGIDO 27/09: NO es un port de CamNew** | `src/core/Cam.cpp` `Process_Cam_On_A_String` (`dist 2.0+zoom`, `0.8*dimZ`, stick/ratón `*FOV/80`, clamp +60/-89.5, retorno `WellBufferMe`, LOS+esferas; `Process_FollowCar_SA` delega) | **PROPIO DEL PROYECTO** (decisión §5.1a del jugador el 27/09). Motivo: **`CamNew.cpp` no tiene cámara de vehículo** — sus dos únicas leyes son `Process_FollowPed:52` y `Process_AimWeapon:233`, **las dos a pie**. Los números `2.0` y `0.4` salieron de `Process_FollowPed` y se aplicaron al coche **por analogía**, no por traducción; así que la fila decía `PORTADO (ve59)` atribuía al mod algo que él no tiene. La ley se queda **tal cual** (el jugador la validó en `ve65`-`ve75`: delante al subir, Q/E, recentrado a 1,5 s, no clavarse a 2,00 m) y el auto-centrado es **intocable** por decisión suya. Lo único que de este bloque sí viene del mod es el **agua**: `CamNew.cpp:215-220` (near-clip 0.2 + `z = nivel + 0.6` + recolocar por `Magnitude2D`) frente a nuestro `z = nivel` (`Cam.cpp:2710-2718`), que se corrige en `aim2`. PASS = orbita sin ladeos, retorno único suave, `CAMV2`/`CAMB2b` + ojo |
| Fixes cámara ve60 (delante al subir, Q/E abajo, delay 2s) + recoil barrido v2 | `src/core/Cam.cpp` (Beta=TargetOrientation+hold 5f al entrar; Q/E/cruceta a Beta yaw + `CA_MAX_DISTANCE`; `idleMs=2000`, `orbiting` con ratón crudo y teclas, sin pelear) + `src/weapons/Weapon.cpp` (fuera factor `spread` ficticio -siempre 0.50-; proxy `damage/range`; una sola física `s_odRecoilAlpha`, pitch a traza) | **PORTADO** (`ve60`). PASS = detrás al subir, Q/E a los lados, retorno a los 2s (`camauto2 idle=`), recoil a escalones con mira fija |
| Recoil v3: la MIRA sube (rescinde R14) + A/D no orbita | `src/weapons/Weapon.cpp` (`multY` sube `kick*acc` hasta 0.500, decae a reposo 0.400; bala la sigue: imprecisión real) + `src/core/Cam.cpp` (fuera DPad de Beta/orbiting: A/D es steer `LeftStickX+DPad`, mirar es palo derecho+Q/E+ratón) | **PORTADO** (`ve61`). `RECOIL3` ahora exige `multY>0.400` tras disparar. PASS = ráfaga con mira a escalones + agrupación peor; A/D gira sin mover cámara |
| Recoil v4: R14 restaurado (mira fija) + Q/E estable + retorno sin pelea | `src/weapons/Weapon.cpp` (fuera subida/decaimiento/reset de `multY`: 0.400 fijo; camara intacta) + `src/core/Cam.cpp` (fuera `+-127`/frame a Beta: Q/E estables por snap; `btnReq` exige `!orbiting`: mover nunca centra) | **PORTADO** (`ve62`). `RECOIL3` vuelve a exigir 0.400. PASS = reticula fija, tiron de camara; Q mantenido = lado; mover = `auto=0` |
| Raton en coche + recoil en Syphon | `src/core/Cam.cpp` (`orbiting` incluye `useMouse`/BetaOffset/AlphaOffset reales; apply del recoil en `Process_Syphon` tras el `WellBufferMe`) | **PORTADO** (`ve63`). Causas: crudo a 0 en ese modo; Syphon nunca aplicaba. PASS = raton orbita sin retorno; recoil visible apuntando |


## 8 · Lote 1.12 (02/10, `swim25`) — bugs faciles y grupos apagables

> Plan: `.agents/plans/2026-10-02-1.12-mods-lote-facil.md` (EXECUTED). Sin
> validacion en juego todavia: el PASS de cada fila es del jugador.

### Los grupos apagables (F1)

| Qué | Estado |
|---|---|
| `VICEEXT_FIX_SILENTPATCH` · `VICEEXT_FIX_WFP` · `VICEEXT_FIX_FV` · `VICEEXT_FIX_FV_ROTOR` en `src/core/config.h` | **HECHO** (`swim25`): antes solo estaban escritos en `00-INDICE.md:92-94` y no existían en el código |
| Rotor `src/vehicles/Heli.cpp` · spool IA `src/control/CarCtrl.cpp` · spool jugador `src/vehicles/Automobile.cpp` | **PORTADO** y ahora **apagable** (`VICEEXT_FIX_FV`, el rotor con `_ROTOR`) |
| Sirena FBI `src/audio/AudioLogic.cpp` · casquillos `src/weapons/Weapon.cpp` | **PORTADO** y ahora **apagable** (`VICEEXT_FIX_SILENTPATCH`) |

### Las filas del lote

| Origen | Qué | Nuestro destino | Estado |
|---|---|---|---|
| `SilentPatchVC.cpp:1105` | El blip de destino es un triángulo, no una raya | `src/core/Radar.cpp` (rama `BLIP_MODE_TRIANGULAR_DOWN`) | **PORTADO** (`swim25`) |
| `SilentPatchVC.cpp:896` (c) | El icono del arma se funde al cambiar | `src/renderer/Hud.cpp` (reactiva `VERTEXALPHAENABLE` antes del `Draw`) | **PORTADO** (`swim25`) |
| Extended 2.5 | Intermitentes | `src/core/config.h` (`VICEEXT_TURN_SIGNALS` ahora activo) | **PORTADO**, activo por peticion del jugador |
| Extended 2.5 | Salir de la moto por el lado del giro | `src/peds/PedAI.cpp` (`CPed::SetExitCar`) | **PORTADO** (`swim25`), usa `m_fSteerInput > 0.25f` |
| Extended 2.5 | Apagar motor y luces con el boton de salir | — | **FUERA por decision del jugador (03/10)**: *«ni lo intentemos mas, elimina todo lo que se haya hecho»*. El codigo borro entero en `swim27` (el define, los dos campos por vehiculo y los cuatro bloques). El cambio de `ENTER` a `F` en `VEHICLE_ENTER_EXIT` se queda: es necesario para F8 |
| Extended 2.0 | La bala no atraviesa la rueda del coche | **`src/core/World.cpp`** (`ProcessLineOfSightSectorList`) | **PORTADO** (`swim25`). El destino del borrador era `Weapon.cpp`, que esta bien: solo recibe el `pieceB` |
| Extended 2.5 | Saltar la llamada telefónica con `ENTER` | `src/core/ControllerConfig.{h,cpp}` + `src/control/Script.cpp` | **PORTADO** (`swim25`). Re-mapea `ENTER` desde `VEHICLE_ENTER_EXIT` |
| Extended 3.0 | La moto policial circulando | `src/control/CarCtrl.cpp` (`ChoosePoliceCarModel` + `RequestModel`) | **PORTADO** (`swim25`). Ver la nota de las dos causas |
| FV | `GetFrameCounter()&N` en 10 sitios jugables | `CarCtrl.cpp`, `Automobile.cpp`, `Bike.cpp`, `Heli.cpp`, `World.cpp` | **PORTADO** (`swim25`), bajo `VICEEXT_FIX_FV` |

### La moto de la policía: dos causas, no una

La auditoría anterior apuntaba a `default.ide:298` (`vehClass=ignore`). **Eso es
falso**: `ignore` es la clase de ~60 modelos, incluidos `police`, `enforcer`,
`fbicar`, `firetruk`, `ambulan`, `vicechee` y `rhino`, que sí spawnean.

Las dos causas reales, y hacen falta las dos:

1. `CCarCtrl::ChoosePoliceCarModel` **no tenía ninguna rama que devolviera
   `MI_VEEXT_POLWINTERG`** (ni la tiene en vanilla: devuelve Miami Vice, SWAT, FBI,
   ejército y `MI_POLICE`). Por eso sí salía con el cheat `CRAZYRIDES`: el cheat no
   pasa por esa función.
2. **El modelo no se pedía nunca.** `polwintergreen.dff/.txd` están en
   `streamed/models/gta3.img/` y en el `manifest.json` web, pero **no** en
   `bootseed.list`; `police` sí está. Los vehículos que salen se piden explícitamente
   (`RequestModel` de ambulan y bomberos). Sin el `RequestModel`, la rama nueva nunca
   habría visto el modelo cargado.

No sube `dataTag`: el `.dff` ya estaba servido.

### Lo que NO se hizo en la 1.12

| Qué | Por qué |
|---|---|
| El volante que gira (Extended 2.5) | Decisión del jugador: «creo que esto ya lo hace». **Medido como NO implementado**: `rg steeringwheel src/` = 0 sobre 543 ficheros, y el dummy existe en 18 `.dff`. Queda abierto |
| Fade del outro, duración del outro, contorno de la barra de carga | Los 3 ports de SilentPatch que **no** se envolvieron en `VICEEXT_FIX_SILENTPATCH` (tienen comentarios `PORTADO` de otra lane entrelazados) |
| `World.cpp` poda de referencias · 3 sitios `Automobile`/`Bike` "por medir" · `CarAI.cpp:192,252` | F10: fuera del triaje, o por decisión de no tocar gestión de memoria |
| Una sección propia de Vice Extended | Falta: el manifiesto cubre 3 de las 13 entradas de `mods/`, y de los 22 `VICEEXT_*` de `config.h` ninguno tiene fila. Pendiente de su propio plan |

## 7 - Agachado calibrado (26/09, `crouch1`) - plan `agachado-calibrado`

| Origen | Que se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| `sa-crouch-movement` (**solo SPEC**: licencia prohibitiva, no se copia codigo; el `.asi` es Win32) | Comportamiento del agachado del mod: botones (apuntar = `RightShoulder1`, disparar = `Circle`, cruceta), entrada `SET_CHAR_CROUCH 0` -> `PLAY_ANIMATION blend 10` -> `WAIT 500` -> `SET_CHAR_CROUCH 1`, filtro de armas 17-27 (fuera las pesadas 28-33) y una rueda por pulsacion | `src/peds/PlayerPed.cpp` (R27: la rueda no sale disparando; motivo de rueda en el log; `cal=1 clipr= entrada=`), `src/animation/AnimManager.cpp` (nombres `GunCrouchFwd`/`GunCrouchBwd`) | **PORTADO** (`crouch1`). PASS = jugador: rueda apuntando con arma 17-27 (una por pulsacion) y agachado andando a 0,90 m/s |
| `sa-crouch-movement` `anim/ped.ifp` (240 anims) | Sus dos clips de agachado con arma, medidos en el fichero: +2,740 / -2,740 m en 0,731 s = 3,75 m/s | `gta_vc_browser/streamed/anim/ped.ifp` + `bootseed/` (272 -> 274 anims, `tools/ifp_add.py`; `dataTag 2026-09-26-ve14`) | **SERVIDO** (datos, no codigo): insertados al final del bloque, las animaciones que ya estaban intactas byte a byte |

### 7b - `crouch2` (R28, 26/09): el reparto de las animaciones parciales

Los cuatro defectos que reporto el jugador al jugar `crouch1` (agachado lentisimo, rueda que no se
ve, cuerpo deformado al levantarse, disparo agachado con dos animaciones peleando) tenian una sola
raiz, en el motor: `FrameUpdate.cpp` (`totalBlendAmount`) + `CAnimBlendNode::Update` +
`CAnimBlendAssociation::GetBlendAmount` reparten asi el cuerpo:

- las animaciones **PARCIALES** (la pose de apuntar, el clip del arma) entran con su peso tal cual;
- las de **MOVIMIENTO** (caminar y los clips de agachado del mod, `ASSOC_MOVEMENT`) multiplican su
  peso por `1 - suma de parciales`;
- la pose de cada hueso es la **suma de cuaterniones** con esos pesos y se normaliza al final.

Con dos poses parciales a peso 1 (nuestra `WEAPON_crouch` mas el clip del arma) la suma pasa de 1:
las de movimiento entran en negativo y la suma de cuaterniones puede quedar casi nula, asi que al
normalizar sale una matriz degenerada: es el cuerpo estirado/aplastado de la captura del jugador.
Ademas `CAnimManager::BlendAnimation` retira (funde) toda parcial ajena del clump, de modo que pedir
nuestra pose cada frame echaba al clip del arma y el motor lo volvia a pedir en el frame siguiente:
la «lucha entre 2 animaciones». Y con la suma en 1 el clip de la rueda entraba multiplicado por 0: el
jugador veia al ped deslizarse y decia «no rueda».

| Origen | Que se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| `sa-crouch-movement` (SPEC) - velocidad y rueda | «que sea casi la misma velocidad que el caminado normal» y la rueda con el boton de costado (cruceta izq/der, `IS_BUTTON_PRESSED 10`/`11`) manteniendo apuntar y sin disparar | `src/peds/PlayerPed.cpp` (R28: 1,13 m/s, corte del costado 50 -> 25 grados, presupuesto de parciales, la rueda retira las poses) | **PORTADO** (`crouch2`). PASS = jugador: agachado a la velocidad del andar, rueda con A/D (tambien en diagonal), disparo agachado con el clip del arma y sin deformacion al levantarse |
| VC `weapon.dat` (`Colt45 ... 680C0` = `WEAPONFLAG_CROUCHFIRE`) + los `.ifp` por arma ya servidos | Los clips de agachado del arma (`colt45_crouchfire`/`colt45_crouchreload`) existen en los `.ifp` servidos, pero el motor solo los pedia con `bIsDucking` + `bCrouchWhenShooting`, banderas que R6 limpia al agacharse | `src/peds/Ped.h` (`CPed::ViceExtCrouchShooting()`) + `src/peds/PedFight.cpp` (`SetPointGunAt`, `SetAttack`, `Attack`, `FinishedAttackCB`, `FinishedReloadCB`) | **PORTADO** (`crouch2`). Sin cambio de datos (`dataTag` sigue `2026-09-26-ve14`) |
## 9 - Lote 2.0-B (04/10) - render, WFP y SkyGfx (agente B)

> Plan: `.agents/plans/2026-10-04-2.0-agente-B-render-wfp-skygfx.md` (ENRICHED->EXECUTED). Sin `build.sh`/enlace/`VERSION` (los sube A). Objetos ninja exit 0 por fichero. Sin sesion de jugador todavia: PASS pendiente de su ojo.

### SP render + blip

| Origen | Que se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| `SilentPatchVC.cpp:1105-1119` (`LegendBlipFix`: `x3 = x1 + (x1 - x4)`, sin simplificar por precision) | Vertice 3 del blip DOWN sombra (era `12.f` fijo de la 1.12) | `src/core/Radar.cpp:1670` (formula con `SCREEN_SCALE_X(8.0f/2.f)`) | **PORTADO** (2.0-B). Desviacion declarada: el mod hookea TODAS las `Draw2DPolygon`; aqui cambio quirurgico solo en la sombra DOWN (las otras 3 ramas ya cumplen `x3 = x1+(x1-x4)`). PASS = triangulo en el mapa, no raya |
| `SilentPatchVC.cpp:1932-1940` (`CoronaFlaresScaling`: `width * Stuff2d::Width()`, `height * Stuff2d::Height()`) | Flares de corona sin escalar (`4.0f*size` crudo; el ratio `spritew/spriteh` cancela la resolucion) | `src/renderer/Coronas.cpp:418-419` (`* SCREEN_STRETCH_X/Y(1.0f)`) | **PORTADO** (2.0-B). A 640x448 factor 1 = identico. PASS = flares crecen con la resolucion como la corona |
| `SilentPatchVC.cpp:2197+` (`MBlurSafeZoneRects`, con `UIScales::MBlur:327-342`) | Zonas seguras sin escalar + `TODO` vivo | `src/renderer/MBlur.cpp:571,574` (radar-x y HUD con `SCREEN_STRETCH_*`, anclaje derecho `WIDTH-240`; fuera el `TODO`) | **PORTADO** (2.0-B). La `y` ya iba por macros y se deja. PASS = gotas/haze no cubren radar ni HUD en ancha |
| `SilentPatchVC.cpp` (`CastShadowEntityFix`: `out = right*x + up*y + at*z + pos`) | `CastShadowEntityXY` ignora la rotacion Up (solo Right/Forward en `Shadows.cpp:1571-1576` y `2003-2005`) | `src/renderer/Shadows.cpp:2003-2005` (`+ p.z * GetUp().x/y/z`) | **PORTADO** (2.0-B). En geometria sin rotar `Up=(0,0,1)` = identico. PASS = sombras de objetos inclinados sin deformar |
| `SilentPatchVC.cpp:1562-1571` (`MouseSensNewGame`: default guardado y restaurado al crear) | Sensibilidad sin reset al empezar partida (`DoSettingsBeforeStartingAGame` solo tocaba emisoras) | `src/core/Frontend.cpp:931` (`Horzntl=0.0025f`, `Vertical=0.003f`, los defaults de `Camera.cpp:265`/`Frontend.cpp:5085`) | **PORTADO** (2.0-B). PASS = raton con sens de serie al empezar partida |

### WFP + SkyGfx

| Origen | Que se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| `SilentPatchVC.cpp:2013-2059` (`ScriptSpritesScaling`: `MaxW/640`, `MaxH/448`, sin wsfix a proposito) + `extsrc/wfp/Sprite2d.ixx` | Sprites de script a resolucion (el opcode pone el rect literal) | `src/renderer/Sprite2d.cpp` (`SetScriptSpriteScale` + `Draw`/`DrawRect` con flag) + `src/renderer/Sprite2d.h` (setter) + `src/renderer/Hud.cpp:1507,1516,1846,1849` (flag on/off en los 4 dibujos de script, sitios asignados 04/10) | **PORTADO** (2.0-B). PASS = tarjetas de intro y cajas de script a tamaño completo |
| `extsrc/wfp/Sprite2d.ixx` (`IsFullscreen`, `ComputeContentRect`, pillar bars) | Letterbox/pillar en `Draw` fullscreen con textura | `src/renderer/Sprite2d.cpp` (`Draw`: content-rect por aspecto del raster + barras si `cx-halfW>0`, bajo `VICEEXT_FIX_WFP`) | **PORTADO** (2.0-B, corregido por A en `swim29`: el default era 4:3 y el motor dibuja en aspecto real, ahora `SCREEN_WIDTH/HEIGHT`). PASS = fondos fullscreen sin estirar ni barras falsas en ancha |
| `extsrc/wfp/Frontend.ixx` + `Menu.ixx` | Menus a pantalla completa | `src/core/Frontend.cpp:2346-2373` (`DrawBackground` fullscreen + bordes `SCREEN_STRETCH_*`) | **YA ESTABA**: verificado, sin cambios. PASS = menu cubre el viewport |
| `extsrc/wfp/Loading.ixx` (`gbNoIslandLoading/gbNoInteriorLoading`) | Quitar pantalla de carga al cambiar de isla/interior | `src/core/Streaming.cpp` (sin tocar) | **PENDIENTE con motivo** (2.0-B): saltar `LoadScene` rompe el on-demand (los datos llegan por ahi); necesita garantia de residencia = diseno D15. No se inventa |
| `extsrc/wfp/Radardisc.ixx` (disco 256 procedural si `HQRadarDisc=1` y raster 128) | Disco del radar en alta calidad por codigo | — (sin tocar) | **NO APLICA** (04/10, corrección del jugador): el mod Extended ya trae `HD radar` (`ChangesEN.txt:24`) y el `hud.txd` servido (6.3 MB) trae `radardisc`. El reemplazo procedural solo tendría sentido sobre el raster 128 vanilla. Reabrir solo si a ojo se ve borroso |
| Rango de retícula 3ª persona (`Hud.cpp:370`: `COLT45..RUGER` = 17..27 dejaba fuera `SNIPERRIFLE` 28, `LASERSCOPE` 29, `ROCKETLAUNCHER` 30 — justo las de zoom) | Cruz del mod en las armas de alcance | `src/renderer/Hud.cpp:370` (rango a `COLT45..ROCKETLAUNCHER`; 34–36 con vía propia: detonador no apunta, heli/cámara tienen modo) | **PORTADO** (2.0-B, causa probada por A en `swim30`). PASS = cruz visible con sniper/láser/cohetes apuntando |
| SkyGfx `skygfx.ini` (sin `nightParam`; pipelines neo/leeds) + `vendor/librw` (cero `nightParam`/`SetNight`, solo comentarios `Prelighting`) | D14 `nightParam` | — (sin tocar) | **NO VA**: sin camino en librw hay que crearlo entero (uniform + UBO/no-UBO + set por frame + `mix()` en ~4 shaders). Pasa a recalculo de A |
## 10 - Lote 2.0-C (04/10) - logica, IA y agachado (agente C)

> Plan: `.agents/plans/2026-10-03-2.0-agente-C-logica-ia-agachado.md` (ENRICHED->EXECUTED).
> Sin `build.sh`/enlace/`VERSION` (los sube A). Objetos ninja exit 0 por fichero.
> Sin sesion de jugador todavia: **PASS pendiente de su ojo**, y los logs que hay son de
> build anterior a la servida (`swim25`/`swim26` contra `swim27`), o sea que solo dan
> linea base (`viceext-log-check.py`, bloque `B` en FALLO).

### SP logica + IA

| Origen | Que se aprovecha | Nuestro destino | Estado |
|---|---|---|---|
| `SilentPatchVC.cpp:2310-2345` (`StingerFixes`: `memset` en el ctor, `Remove` que no limpia el estado) | 2 de sus 4 arrangements | `src/objects/Stinger.cpp:39` (`memset(this, 0, sizeof(*this))`) + `:100` (`m_nSpikeState = STINGERSTATE_NONE`) | **PORTADO** (2.0-C). Los otros 2 **YA ESTABAN**: el guard de pool de `Init` (`:50-56`) y el de `Deploy` (`:109-113`). PASS = sin crash con el pool de puas lleno |
| (bug **nuestro**, no del mod: `StingerFixes` no lo arregla) | Desreferencias sin comprobar: `pSpikes[0]`, el bucle de segmentospSRicos y `->currentTime` de la asociacion | `src/objects/Stinger.cpp:124`, `:165`, `:202` | **PORTADO** (2.0-C) como **bug propio**, no como port. El `:124` es lo que hace seguro el `memset` del ctor |
| `SilentPatchVC.cpp:2139-2158` (`RamcarCloseMissionGiveUpFix`: `FindPlayerVehicle() == pVehicle->AutoPilot.m_pTargetCar`) | La poli no abandona la persecucion si persigue a un NPC | `src/control/CarAI.cpp:268-272` | **YA ESTABA**: expresion **identica**, bajo `FIX_BUGS`, que esta activo (`config.h:304`). Cero codigo |
| `SilentPatchVC.cpp:1953-1962` (`PedMugObjectiveFix`: `mov eax, [ebp+534h]`) | La victima del atraco entra por `break` y no por `wander` | `src/peds/PedAI.cpp:1585-1595` (ya tiene el `if/else` explicito) | **NO PORTABLE**, con motivo: el mod cambia una instruccion x86 y el dato no esta en ninguna fuente que tengamos. Dictaminar cual de las dos ramas es la del original exigiria desensamblar el binario |
| `SilentPatchVC.cpp:1258-1271` (`GetTimeSinceLastFrame` + `NewFrameRender`) | Instrumento para medir el frame real en el motor Win32 | `src/core/Timer.cpp:150` (`ms_fTimeStep`) + `:130-137` (`m_LogicalFrameCounter`) | **NO APLICA**: el equivalente ya esta en el motor |
| `SilentPatchVC.cpp:1285-1305` (`AutoPilotTimerFix_VC`: `nTimer -= nScaleFactor * fScaleCoef`) | Escalar un contador de frames | `src/control/CarAI.cpp` (13 sitios) | **NO APLICA**: `m_nTimeTempAction` no es un contador, es fecha absoluta en ms (`GetTimeInMilliseconds() + N`) |
| FramerateVigilante (`GetFrameCounter() & N` con intencion de frecuencia) | A FPS altos la mascara es aliasing: el sonido de salpicadura suena 4 veces mas y la IA recalcula ruta 4 veces mas | `src/control/CarAI.cpp:192,252` (2 sitios) | **PORTADO** (2.0-C), a `CTimer::GetLogicalFrameCounter()`, que ya cuenta 30 Hz reales (`Timer.cpp:130-137`). **Nunca** multiplicando la mascara. PASS = la IA no re-planifica la ruta mas veces por segundo al subir fps |
| FramerateVigilante, los 4 sitios de render | idem, pero en sonido y particulas | `src/vehicles/Automobile.cpp:4199,4881,4894` · `src/vehicles/Bike.cpp:2423` | **NO PORTADO, con motivo**: el proyecto va a **60 fps**, y el contador logico cuenta 30 Hz, o sea que convertirlos **habria hecho salir la arena y el agua a la mitad** de frecuencia de como estan ahora. El beneficio solo existe por encima de 60 fps, que no se juega. Se dejan como estan |
| `SilentPatchVC.cpp:1479-1546` (`EnvMapsOnExtras`: env map y pipe de vehiculo en las piezas extra) | Los extras (`m_comps`) se quedan sin env map y sin pipe: solo los recibe `m_clump` y las ruedas | `src/modelinfo/VehicleModelInfo.cpp:1170-1174` (env map) + `:1181-1185` (pipe), bajo `VICEEXT_FIX_SILENTPATCH` | **PORTADO a medias** (2.0-C): la mitad mecanica esta. La mitad de **specularidad** (`stallion`, `mesa`, `.ini:36`) queda **PENDIENTE con motivo**: nuestro motor no tiene un mando de specularidad por modelo (solo el global `CustomPipes::VehicleSpecularity`), y el del mod es por material, asi que habria que escribir `RwSurfaceProperties` material a material. PASS = los extras del coche reciben el mismo env map que la carroceria |
| `SilentPatchVC.cpp:1702-1843` (`SelectableBackfaceCulling`) + `.ini:45` (317 modelos) | Sin backface culling en peds (siempre), en piezas de coche (si `TEMP_OBJECT` con colores de vehiculo) y en los 317 modelos de la lista; los vehiculos lo llevan resuelto | `src/entities/Entity.cpp` (`odShouldDisableBackfaceCulling` + `odBackfaceNames`, bajo `VICEEXT_FIX_SILENTPATCH`, guarda y restaura `rwRENDERSTATECULLMODE`) | **PORTADO**, pero **lo hizo otro agente en mi fichero** mientras C trabajaba (D1 se lo habia dado a C). Verificado: logica identica al mod, con `m_nRefModelIndex` en vez del inexistente `m_wCarPartModelIndex`. `Entity.cpp.o` exit 0. PASS = peds y postes se ven desde los dos lados, sin atravesar paredes |

### Agachado: lo que dice la medicion (sin cambios de codigo)

| Que | Medido | Lectura |
|---|---|---|
| Cadencia y objetivo | `mps=1,13` · `clipr=2,05` (raiz servida 1,500 m / 0,731 s) · cadencia `1,13/2,05 = 0,5512` | **La cuenta esta bien**: la cadencia aplicada lleva el clip exactamente al objetivo |
| Velocidad realizada | `velo` = **0,57 m/s** (mediana de 19 muestras, partida `swim25` del 02/10) y **0,92** (9 muestras, mismo build) contra 1,13 ± 15 % | **La perdida es del motor, no del objetivo**: `m_vecMoveSpeed` casi vacia, o sea que el movimiento no va por velocidad sino por la traslacion del clip. Medido sobre **los 285 logs**: 1 525 muestras andando en 47 partidas, con medianas que van de **0,31 a 2,75** ⇒ la medida es ruidosa (mezcla giros y frenadas), y en los builds recientes se queda en **0,57-0,92**, por debajo del suelo 0,96 |
| Encaraamiento (`giro`) | 9 de 9 muestras correctas | **OK**: `giro=0` = gira (cuando no apunta), `giro=3` = no gira (cuando apunta). El maestro tenia el criterio al reves |
| Rueda | 0 eventos, 3 `motivo=sin-mira` | Correcto: el mod tambien exige apuntar. Falta una partida con *apuntar + tocar* el costado |
| Escalada | 21 usos de `VICEEXT_CLIMB`, 7 clips y 3 trazas, **pero nunca ha saltado** | `src/peds/PlayerPed.cpp:3186,3196` (2 trazas nuevas en el fallo) | **DIAGNOSTICADO, no arreglado** (2.0-C): en los **285 logs** hay **0** trazas `climb` y **0** `PEDCLAIM` de escalada, o sea que la funcion se llama pero **la deteccion de borde nunca acierta**, y **no habia ninguna traza en el camino de fallo** (por eso nunca se pudo saber). Anadidas `VICEEXT climb no: sin borde` y `VICEEXT climb no: sin sitio arriba`, que solo saltan al pulsar saltar. Falta la partida que las dispare |

**Sin cambio de codigo** en el agachado: con **una** muestra andando no se decide una
velocidad (los planes ya avisaron del riesgo de medir con n=1), y el defecto esta acotado
a "la traslacion del clip se realiza al 34 %", que necesita leer el camino del motor
`FrameUpdate`/`UpdatePosition`. Queda como medicion con criterio PASS declarado, no como
arreglo a ciegas.

## 11 - Lote 2.0-A (04/10) - camara, ClassicAXIS, 1a persona, backface y env maps (agente A)

Plan: `.agents/plans/2026-10-03-2.0-agente-A-camara-classicaxis-1p.md`. Sin LICENSE; la
fuente esta en `gta_vc_browser/tmp/extsrc/classicaxis_Main.cpp` (1596 lineas), y
`CamNew.cpp` (498) es parte del **mismo** mod (`#include "CamNew.h"`).

### Portados

| Item | Que | Fichero | Namespace del mod |
|---|---|---|---|
| `:1610` | `IS_PLAYER_TARGETTING_CHAR`: la heuristica solo cuenta en 1a persona de arma o raton en 3a, y **no** si el nado posee el apuntado | `src/control/Script6.cpp:1006-1008` | `IsPlayerTargettingCharFix` (`:1613`) |
| `:1610` (traza) | `TGTGUN base= heur= aimcam= nado= melee=`, el motivo de cada rechazo | `src/control/Script6.cpp:1032` | instrumentacion |
| crosshair | La cuenta del mod: `tan(halfFOV)` en vez del `1.8f` lineal, y el **aspecto en el eje del mod** (X sin aspecto, Y dividido) | `src/core/Camera.cpp:4342-4348` | `classicaxis_Main.cpp:526-533` |
| `:1701` | BFC por entidad: override por modelo (**317 nombres** del `.ini`), peds siempre, objetos con `m_nRefModelIndex != -1 && TEMP_OBJECT && bUseVehicleColours`; vehiculos nunca | `src/entities/Entity.cpp:416-533` | `SelectableBackfaceCulling` (`:1702`) |
| 1P arbitro | `PEDCAP_CAMARA` pasa a tener dueno: se **solicita** al encender y se **suelta** en los 4 apagados, y la puerta exige que la camara no sea de otro | `src/core/Camera.cpp` (toggle, control perdido, coche, Restore) | ADR-007 / ADR-008 |

El `m_wCarPartModelIndex` del mod es un offset de plugin-sdk; en reVC el campo es
`m_nRefModelIndex` (`Object.h:82`) y `Object.cpp:227` ya usa **exactamente** la misma
condicion de tres terminos, asi que el port es literal. Los numeros del mod (2, 3, 4)
coinciden con nuestro `eEntityType`.

### Sin cambio de codigo, con motivo

| Item | Motivo |
|---|---|
| near-clip dual (`Camera.cpp:334-340`) | **Ya estaba** (B4, ronda anterior), con el motivo escrito: el mod no publica valores de su `FirstPerson.cfg`. Ponerlos seria inventarlos |
| `AX6`/`AX7` cuerpo | **Ya estaban** en `PlayerPed.cpp:4677` (`ViceExtRotatePlayer`) y `:4876-4888`. Y hay una medicion del 28/09 (R9) que decidio **no** usar el lerp `0.02f` del mod: forzar `SetHeading` daba `desv=90.0` constante |
| `:1478` env maps en extras | **Ya estaba**: `VehicleModelInfo.cpp:1175-1190`, los dos bucles de `m_comps` (env map y `AttachVehiclePipe`) |
| `:2082` ascensor del Skimmer | **NO PORTABLE**: no existe en el arbol. No hay `Skimmer`, ni ascensor, ni el opcode; lo unico de barco es `COMMAND_ANCHOR_BOAT`. El maestro lo confundio con `IsSlideObjectUsedWrongByScript` (`Script4.cpp:44-58`), que es del **G-Spotlight** |
| `:1999` sprites de script | **No es de A**: la namespace es `ScriptSpritesScaling` y hookea `CSprite2d::Draw` + `CDraw::DrawRect`, o sea `Sprite2d.cpp`, que es de **B** |
| 1P en el menu | **BLOQUEADO**: la etiqueta del menu vive dentro de los `.gxt` servidos (`american.gxt`, 429 KB). Anadir opcion = cambio de dato, sube `dataTag` y purga cache. No se hace a ciegas |
| FOV/sensibilidad de 1P | El mod es un binario cerrado y su spec (`13` §3.3:95) **no da ni un numero**: son calibraciones, no puertos |
| el recoil | Preguntado y **no necesita nada**. Intocable por `ADR-003`, validado, y el cambio del crosshair no lo parte: la copia vanilla de `Weapon.cpp:62-63` solo corre con `bFreeCam` |

### Compilacion

| Objeto | Resultado |
|---|---|
| `src/CMakeFiles/reVC.dir/control/Script6.cpp.o` | **exit 0** |
| `src/CMakeFiles/reVC.dir/core/Camera.cpp.o` | **exit 0** |
| `src/CMakeFiles/reVC.dir/entities/Entity.cpp.o` | **exit 0** |

Los 6 ficheros de A son **CRLF puro** y se comprobo tras cada edicion que no se
introdujera LF ni CR duplicado.

> **Nota de ruta, util para B y C**: el target de ninja es
> `src/CMakeFiles/reVC.dir/<ruta-relativa-a-src>.cpp.o`, **sin** el `src/` repetido
> dentro de `dir/`. Con el `src/` duplicado da `ninja: unknown target`.

## 12 - Lote 2.0-A, segunda pasada (04/10) - lo que estaba "bloqueado", desbloqueado

El jugador dijo que no havia por que bloquearse. Se busco en internet y en el binario, y
**los dos bloqueos eran falsos**: no faltaban los datos, faltaba donde estaban.

### Desbloqueado 1 · la 1a persona se lee de un `FirstPerson.cfg`, con los nombres del mod

El `.asi` de GeniusZ **si dice como se llaman las claves**. Se leyeron del binario:

| Offset en `FirstPerson.asi` | Clave |
|---|---|
| 168456 | `Near Clip on foot` |
| 168480 | `Near Clip in vehicle` |
| 168520 | `Mouse Sensitive` |
| 168592 | `Vehicle Offsets` |

Se leen de la seccion `[FirstPerson]` con el `ReadIniIfExists` de la casa:

- `src/core/Camera.h`: `s_viceExt1PNearClipOnFoot`, `s_viceExt1PNearClipInCar`,
  `s_viceExt1PMouseSens`, `s_viceExt1PVehicleOffset`
- `src/core/Camera.cpp`: los defaults de serie, y los dos `static const` del near-clip
  pasan a ser esos (`#define` alias, sin tocar el uso de `Camera.cpp:416`)
- `src/core/re3.cpp`: las 4 llamadas, con el nombre de clave exacto del mod

**No se inventa ni un numero**: sin cfg, cada ajuste se queda en su default de serie. Pero
**el ajuste existe**, y si descargas el `FirstPerson.cfg` original de la mod funciona aqui
sin tocar nada, porque los nombres coinciden. Eso es lo que hacia falta y no hacia falta.

### Desbloqueado 2 · la 1a persona **tambien** esta en el menu

El bloqueo era "hay que meter una etiqueta en los `.gxt`". Se puede: `tools/gxt_inspect.py`
**sabe anadir claves**, y no habria sido problema.

| | |
|---|---|
| Etiqueta | `FEM_1PV`, anadida a los **18 `.gxt`** (bootseed, streamed y gamefiles x 6 idiomas). En `spanish.gxt`: `PRIMERA PERSONA` |
| Opcion | `FIRST_PERSON_TOGGLE` en `src/core/MenuScreensCustom.cpp`, con el patron de casa (`#ifdef` / `#else` / `#endif`), y **usada en la lista**, no solo definida |
| Bandera | `bViceExt1stPersonView` pasa de `static` de fichero a **miembro de `CCamera`** (`m_bViceExt1stPersonView`), porque `CCFOSelect` necesita una `bool` con direccion, igual que `bFreeCam` |

**La tecla V sigue siendo el conmutador**: el menu escribe el mismo flag, no hay segundo
estado.

### Desbloqueado 3 · el arbitro con `PEDCAP_CAMARA`ньserializer de verdad

La 1a persona **solicita** `PEDCAP_CAMARA` como `PEDLANE_LEY` al encenderse y lo **suelta**
en los cuatro apagados (perder el control, apuntar, subir a un coche, `Restore`). Y la puerta
exige que la camara no sea de otro. Antes `PEDCAP_CAMARA` no tenia dueno.

### Lo que sigue sin hacer, y por que (sin disguise)

| Item | Motivo real |
|---|---|
| `AX6`/`AX7` | **Ya estan portados** (`PlayerPed.cpp:4677`, `:4876-4888`), con una medicion del 28/09 que **prohibe** copiar el lerp del mod (daba `desv=90.0`). Rehacerlo seria romper |
| near-clip **dual** | El mecanismo existia; ahora ademas **los dos valores salen de `FirstPerson.cfg`**, que es lo que faltaba |
| `:1478` env maps en extras | **Ya estaba** (`VehicleModelInfo.cpp:1175-1190`) |
| `:2082` ascensor del Skimmer | **No existe en el motor.** Se busco `FlyingControl` y resulta que es de avion/mariposa (`Boat.cpp:739`, `FLIGHT_MODEL_SEAPLANE`), no del ascensor. Lo unico de barco es `COMMAND_ANCHOR_BOAT`. No hay nada que arreglar |

### Compilacion (los 5)

| Objeto | Resultado |
|---|---|
| `src/CMakeFiles/reVC.dir/control/Script6.cpp.o` | **exit 0** |
| `src/CMakeFiles/reVC.dir/core/Camera.cpp.o` | **exit 0** |
| `src/CMakeFiles/reVC.dir/entities/Entity.cpp.o` | **exit 0** |
| `src/CMakeFiles/reVC.dir/core/re3.cpp.o` | **exit 0** |
| `src/CMakeFiles/reVC.dir/core/MenuScreensCustom.cpp.o` | **exit 0** |

> **Aviso de datos**: anadir `FEM_1PV` a los `.gxt` **es un cambio de dato servido**, asi que
> `dataTag` **tiene que subir** para purgar la cache del navegador. Lo sube el orquestador al
> final del tanda, junto con la `VERSION` y el enlace.
