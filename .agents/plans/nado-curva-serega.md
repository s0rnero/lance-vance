---
name: nado-curva-serega
status: EXECUTED
type: feature
domain: gameplay-swim
owner_rules: .agents
created: 2026-09-26
---

# Plan técnico: Nado con curva Serega

### Análisis

- **Objetivo:** injertar la curva de velocidad del nado de Serega en `ViceExtSwimControl/Move` con `m_vecMoveSpeed`. Regla permanente del jugador (12-handoff §14): el mod manda sobre lo existente; se reescribe, no se parcha.
- **Hallazgo del enrichment (verificado):** la curva ya está parcialmente injertada en `ve58` (`PlayerPed.cpp:2553-2571`: `odSeregaU 0.022/0.05/0.12`, `s_odSwimTicks 300`, `GetSprint()/JumpJustDown()`; fila en `docs/mods/ATTRIBUTION.md:99`). El trabajo es **reescritura de consolidación**, no injerto desde cero.
- **Sin cambio:** `AnimationId.h:292-295` (`SWIM_TREAD/CRAWL/BREAST/JUMPOUT` existen) y `AnimManager.cpp:940-970,1094` (grupo `playerswim` existe). El item "si faltan" queda cerrado como YA EXISTE.
- **Fuente:** `mods/1498977446_107784/` (`CLEO/swim.cs` 2288 B, solo la curva; NO clips `.ifp` recortados ni `.asi`). Spec en `13-inventario-mods-fuente.md` §3.1.
- **Archivos previstos:** `src/peds/PlayerPed.cpp`, `gta_vc_browser/tools/viceext-log-check.py` (extender `bloque_h`), `gta_vc_browser/tools/check-served-build.sh` (1 marca), `VERSION` en `web/lib/index.js` (solo en build).
- **Dato del jugador (26/09): ninguna animación de nado se ejecuta en el agua.** El cableado estado→clip está roto; confirma reescribir en vez de conservar.
- **Riesgos:** spec ambigua (`0.025`/`0.004`/`0.1` vs `0.022/0.05/0.12` — ver puntos a decidir); sprint 6.0 m/s supera la raíz del clip (2.78 m/s) y el techo `VICEEXT_SWIM_MAX_SPEED 3.2` sin uso; timeout 300 en frames vs ms con TimeStep variable; botón 8=cancelar sin equivalente; no regresión de cámara auto-centrada ni recoil (intocables).

### Cambios por archivo

- **`src/peds/PlayerPed.cpp`** (reescribir bloque horizontal `:2542-2612`, conservar guarda triple/histéresis/vertical/salida/espejo/trazas):
  - Curva Serega como autoridad: constantes `VICEEXT_SWIM_SEREGA_DRIFT/CRUISE/SPRINT/TIMEOUT`; `m_vecMoveSpeed.x/y = dir * odSeregaU`; `odSwimNatural` (hoy muerto con `(void)`) se usa para cadencia o se retira con motivo.
  - Extender `SWIM2 move` con `serega=<u> ticks=<n> sprint=<0/1>` al final (sin romper regex existentes). Mantener `SWIMNAT`.
- **`src/animation/AnimationId.h`, `AnimManager.cpp`:** sin cambio (verificar consecutividad del grupo swim).
- **`viceext-log-check.py`:** extender `bloque_h` con bandas deriva ~1.1 / crucero ~2.5 / sprint ~6.0 en `velo`, `FAIL si max>5` (catapulta), `INCONCLUSIVE` sin `serega=` o sin `SWIM2` (modelo RC del plan recoil).
- **`check-served-build.sh`:** 1 marca `serega=` nueva. No tocar el resto.
- **`web/lib/index.js`:** solo bump de `VERSION` en build. `dataTag` no sube.

### Restricciones

- El mod manda sobre lo existente (12 §14); excepciones intocables: auto-centrado de cámara y recoil.
- Solo curva+lógica; NO `.asi`, `ped.ifp` ajeno, memoria-hookeada, `CALL 0x405640`, offsets CLEO.
- `.cpp` = CRLF (python binario + `assert count==1`); consola cp1252 (`PYTHONIOENCODING=utf-8`).
- `VERSION` sube en cada build; `dataTag` no. Sin commits. Sin palancas URL/consola. Medición = jugador + log.
- Serega = spec sin LICENSE: reimplementar, no copia literal.

### Pasos ejecutables

1. **Leer** `mods/1498977446_107784/CLEO/swim.cs` vía `tools/cleo_disasm.py` (si existe): tabla opcode→semántica, curva exacta, conmutación por botones 14/8/16. Si no resuelve `0.025`/`0.1`, parar y elevar a decisión.
2. **Leer** `PlayerPed.cpp:2321-2704`, `PlayerPed.h:78`, `Ped.cpp:1537`, `Cam.cpp:1158,1759,5695` (solo lectura), `AnimationId.h:288-295`, `AnimManager.cpp:940-970,1094-1096`.
3. **Editar** `PlayerPed.cpp`: constantes `SEREGA_*` + reescritura `:2558-2612` + `SWIM2` extendido (CRLF, assert único).
4. **Editar** `viceext-log-check.py` (`bloque_h` + INCONCLUSIVE) y **`check-served-build.sh`** (marca `serega=`).
5. **Editar** `VERSION` solo con build autorizado. Verificación de lectura: `grep SWIM2|serega=` en logs, `check-served-build.sh --listar`.

### Verificación

- Log en agua honda: `SWIM2 move` con `avance≈velo`, bandas por curva, `ticks` resetea y fuerza deriva tras 300, `max(avance)≤5.0`, `modo=4`. Checker `OK`; build vieja = `INCONCLUSIVE`.
- Ojo del jugador: arrancar/crucero/sprint distinguibles, sin catapultas, `swim_jumpout` al salir, cámara estable, sin regresión de auto-centrado ni recoil.
- `check-served-build.sh` OK con marca nueva, `VERSION` nueva, `dataTag` intacto.

### Puntos a decidir con el jugador (resueltos 26/09, en cristiano)

1. Flotación: **lo que diga el mod** (el desensamblado manda; si no lo resuelve, deriva única 0.022).
2. Sprint: **6 m/s** (0.12).
3. Brazos: **a la par** (cadencia acoplada `speed=velo/raíz`, como las piernas del agachado).
4. Botón cancelar: **sin equivalente**.
5. Timeout: **igual que el mod** (frames, como el original); se revisa después si deriva con los FPS.

## Ejecución (26/09, gate «ejecuta el plan»)

- Desensamblado `swim.cs` con `tools/cleo_disasm.py`: dos derivas (0.022 entrada / 0.025 crucero), arranque x0.004, crucero x0.05, sprint x0.12, timeout en frames, botón 8=salir. Resuelve la decisión 1 a favor del mod.
- `PlayerPed.cpp` (binario, CRLF intacto, `diff --check` limpio): defines `SEREGA_*`, doble deriva, cadencia acoplada (`speed=odSpeed/odSwimNatural`, tope 3x), `SWIM2` con `serega=/ticks=/sprint=` (buffer 256), `300`→define. Objeto `PlayerPed.cpp.o` compila sin errores.
- Checker: `bloque_h` con bandas Serega por `serega=` + INCONCLUSIVE sin curva (build vieja). `check-served-build.sh`: marca `serega=`.
- Decisiones aplicadas: flotación/timeout lo que diga el mod, sprint 6.0, brazos a la par, sin botón cancelar.

## Cierre (memoria persistente)

- Qué cambió: curva Serega como autoridad + cadencia acoplada + telemetría (ver arriba).
- Verificación: objeto compila; `py_compile` + `bash -n` OK. Build/enlace y partida, pendientes de autorización y del jugador.
- Resultado: EXECUTED, pendiente de build.
- Fix 26/09 noche (fiel al mod): en mar con agua de sobra el nado no enganchaba (0 trazas). Causa: umbral propio de 0.90 m; el mod nada con cualquier contacto. Entrada por contacto (`bIsInWater` o profundidad>0), se mantiene anti-flap 0.25 de salida. Traza `SWIMIDLE` 1/s + aviso en checker. Objeto recompila limpio.
- Build `2026-09-26-swim2` (autorizado): `SWIM2` con `rot/ori/beta` (separa ratón de realimentación), chapoteo periódico al avanzar (`POBJECT_PED_WATER_SPLASH`, 500/250 ms) + muestra 12 del mod por brazada. Reenlace forzado, check OK, dataTag intacto.
- Fix swim3 (4 reportes 26/09 noche): (1) rumbo con fórmula Zelda vanilla (`GetRadianAngleBetweenPoints(0,0,-lr,ud) - Orientation`; mi Atan2 invertía W/S); (2) escalada excluida nadando (+aborta la activa al entrar al agua); (3) chapoteo de un solo uso (`RAIN_SPLASHUP` x3 al frente, 0.3/0.5 — el `AddObject` anterior era emisor persistente y se apilaba); (4) sonido: causa datos — faltaban los mp3 9950-9953 en `streamed/Audio/sfx/` (steyr, lanzagranadas, nado); regenerados con `add_viceex_sfx.py` + `gen_manifest.py` (17477 entradas). Sin cambio de código para el sonido.
- Build `2026-09-26-swim3` (autorizado): rumbo Zelda + guard escalada + chapoteo un solo uso + mp3 9950-53. Reenlace forzado, check OK.
- Fix swim4 (escalada al saltar al agua): no se trepa si el destino está en/bajo el agua (`GetWaterLevel(destino)`, traza `VICEEXT climb no: destino en agua`). Trepar fuera del agua sigue valiendo. Objeto compilaba limpio.
- Autorización permanente del jugador (26/09 noche): build siempre después de cambios (no tiene otra forma de comprobarlos).
- Build `2026-09-26-swim5`: salida anti-flap (solo sale si físico y profundidad dicen fuera) + aborta trepada al entrar al agua (`trepa-off`). Check OK.
- Build `2026-09-26-swim6`: zambullida con momento (no se frena la caída a ritmo de hundido) + `bIsInTheAir=false` nadando (quita la pose de caída). Check OK.

## Iteración swim7 — diagnóstico 26/09 noche (SIN cambios de código)

Reporte del jugador (build por confirmar: el último log con juego es swim5):
1. Clavado desde alto: cae libre y atraviesa el mapa.
2. Entrada suave: raro al principio pero luego se eleva.
3. Ya no salen las animaciones de nado.

Hallazgos en logs (sesión swim5 `odtrace-2026-09-27_01-44-15.log`):
- Máquina de estados sana: 61 `SWIM2`, solo 7 exits (anti-flap funciona, era 787), clips pedidos 233/234 con `assoc>0`, 0 trepadas, `z` mínima 5.3 (sin caídas al vacío en esta sesión).
- El reporte, por tanto, es de otra sesión sin log (sin endpoint no hay traza) o de swim6 sin log de juego todavía.

Hipótesis (inspección de código, no medidas):
- H1 (atraviesa el mapa): swim6 conserva el momento del salto sin tope; si el nado no engancha durante la caída rápida (`bIsInWater` aún no puesto, zona de agua no encontrada entre frames), no hay lógica de subida y la bajada (0.3-0.5 m/frame) puede tunelar el fondo con `bIsStanding=false`.
- H2 (sin anims): el clip se pide (`clip=`, `assoc>0`) pero la pose no cambia: la assoc de caída del grupo STD puede ganarle a la de nado, o `bIsInTheAir` (swim5 no lo limpiaba) mantiene la pose aérea. swim6 limpia `bIsInTheAir`; falta confirmar en log (`clip=`/`est=`) si con swim6 la pose ya cambia.
- H3: si el jugador probó swim5 creyendo swim6, H1+H2 son comportamiento viejo conocido.

Propuesta (pendiente de confirmar build + un log con juego):
- A. Topear la zambullida en vez de conservarla íntegra (p. ej. tope de caída -0.15 m/frame al contacto): sensación de clavado sin tunelado.
- B. Enganchar el nado antes en clavados (predecir el contacto: si la trayectoria cruza el nivel en N frames, entrar ya con subida+anims).
- C. Al entrar nadando, retirar la assoc de caída del grupo STD (patrón R15 `ASSOC_DELETEFADEDOUT`) si el log confirma clip pedido con pose clavada.
- D. Red anti-túnel: si nadando `z < nivel-3m`, resubir a `nivel-0.55`.
- Orden: confirmar build del reporte + una partida en `localhost:2077` con clavado; luego A+C (+D si el log muestra z bajo fondo).

- Build `2026-09-26-swim7`: histéresis sin-agua (20 ticks) + red anti-túnel + rumbo suavizado (6°/frame) + retira assoc de caída al entrar. Check OK. Nota: el árbol trae trabajo paralelo (otro agente subió `VERSION` a crouch1 y `dataTag` a ve14 con cambios de datos); este build incluye ambas líneas.

## Iteración swim8 — medición swim6 (`odtrace-2026-09-27_01-59-22.log`, SIN cambios)

- Estado sano en superficie: 56 `SWIM2`, 5 exits, 0 trepadas, `assoc>0`, `ahogando=0` hasta 23 m.
- **Corrección del jugador (válida): la "recuperación" no era subida nadando.** Dos veces la misma secuencia letal: nadando a z≈-5/-17 → `swim exit motivo=sin-agua (agua=0, nivel=0.0)` → caída libre vertical (av=0) -9.7→-34→-76 y -17→-31→-63 en 2 s → reaparece en superficie (teleport/respawn del motor, x/y saltan 10 m). Lo que parecía "subir nadando" era caminar sobre rocas tras el respawn o con el nado caído.
- **Mecanismo:** `GetWaterLevel` devuelve falso UNA vez en pleno clavado (flicker de zona o hueco real bajo el muelle/rocas) y la salida `sin-agua` es inmediata: sin estado no hay subida, ni anims, ni tope de caída. Caída libre + `bIsStanding=false` = atraviesa el fondo.
- Propuesta (un build): A. histéresis en la zona de agua: `sin-agua` solo tras ~15-30 ticks secos seguidos (conservar último `level` mientras tanto); B. red anti-túnel: si `z < level-8m` (o cayendo sin agua bajo los pies), resubir a `nivel-0.55` en vez de dejar caer al vacío; C. rumbo suavizado + retirar assoc de caída (de la iteración anterior, sigue pendiente de build). Nada toca cámara ni recoil.
- Diagnóstico partida swim1 (`odtrace-2026-09-27_00-18-56.log`): la curva clava magnitudes (deriva 1.1, crucero 2.5, sprint 6.0, `avance≈velo`, `assoc>0`, `moved≈0` del clip, multY intacto). PERO el rumbo zigzaguea ±100°/s con W estable (rastro PEDAT 00:19:47-00:20:30): magnitud bien, dirección mal. Sospechoso: snap instantáneo de rotación + realimentación de la cámara que persigue (Orientation oscila → el ped la sigue). Falta traza `rot/ori/beta` para separar ratón de realimentación. Chapoteo: el vanilla solo salpica al zambullirse (apagado nadando). Sonido: fila 2 pendiente (muestra 12 servida en banco, falta id nuevo en AudioLogic).

## Iteración swim9 — reporte en build ajeno (`odtrace-2026-09-27_02-33-08.log` = crouch1/ve14, SIN cambios)

AVISO: el jugador probó `crouch1` (línea paralela de otro agente), NO `swim7`. Casi todo lo reportado ya está implementado en swim7 sin probar. Verificar título antes de cada reporte.

Reporte (3 frentes) + estado real:
1. **Clavado al fondo y subida tardía a hombros.** Medido: hondo 25.7 con subida efectiva ~1.4 m/s (tope 2.25 vs zambullida 10-40 m/s). La subida existe pero pierde contra el momento: llega al fondo y recupera tarde. swim7 (sin probar) topa la caída con histéresis+red. Si con swim7 sigue llegando al fondo: frenar la zambullida al contacto (tope -0.15 m/frame) en vez de conservarla íntegra.
2. **Sin anims de nado (rápido/lento/deriva) con movimiento OK.** `clip=233/234/235` + `assoc>0` pedidos cada muestra, `est=1`/`41`: el motor los mezcla pero en pantalla no se ven. swim7 (sin probar) retira la assoc de caída al entrar. Si persiste: cazar qué assoc gana (traza de assoc activa) en vez de suponer.
3. **NUEVO: no se puede saltar a un objeto cercano (barco) desde el mar.** Causa estructural: nadando no corre el control a pie (sin salto) y la escalada está bloqueada nadando, así que abordar es imposible por construcción. Propuesta (feature, un build): con `JumpJustDown` nadando, buscar borde/casco al frente (reusar `ViceExtClimbLedgeHeight` + destino seco): si hay, salir del nado y arrancar la trepada/salto hacia él; si no, lo actual (impulso `RISE_FAST`). PASS: del agua al barco/muelle sin salir a la orilla. No toca cámara ni recoil.

## Iteración swim10 — crouch2 SÍ trae swim7 (`odtrace-2026-09-27_20-18-33.log`, SIN cambios)

- Red como red, no como prevención: el clavado llega al fondo y `SWIMNET resubido` dispara una vez. Falta el tope de zambullida al contacto (propuesta A sin implementar).
- Rumbo mejor pero sin veredicto: `rot` deriva suave en 8 s en vez de azotar; a 1 muestra/s no se distingue convergencia de snap. El enter fue con `est=41` (Colt fuera): posible pose de apuntar tapando el nado.
- Anims: `clip=233/234/235` + `assoc>0` pedidos, assoc de caída retirada, y AÚN no se ven. `BlendAnimation` no reinicia assocs (`AnimManager.cpp:1324`), así que el clip avanza: lo visible lo impone otra assoc (candidata: apuntar) o se reimpone por frame.
- Propuesta (un build): A. tope de zambullida al contacto (-0.15 m/frame); B. traza 1/s de assoc con `blendAmount>0.5` por grupo para cazar la que gana; C. prueba con arma enfundada para aislar el apuntado; D. abordar barcos (pendiente). Nada toca cámara ni recoil.

## Iteración swim13 — sin log swim8 + teoría del puñetazo (SIN cambios)
- No existe ningún log de swim8 en disco: lo reportado ("todo igual", "anims solo al disparar") no es verificable contra swim8. Todo lo medido sigue siendo crouch2.
- Orden real (`CPlayerPed::ProcessControl`): base primero, `ViceExtSwimControl` después, y `ProcessPlayerWeapon` corre AUNQUE se nade. En swim8 el click dispara puñetazos (puños): la melé full-body enmascara la pose clavada mientras se pulsa, y al soltar vuelve lo clavado. Eso explica "anims solo al disparar" sin que el nado funcione.
- El switch a puños (slot guardado/restaurado por la vía del motor) está sin verificar: no hay ni una traza `swim punos` en disco.
- Propuesta (un build): A. `SWIM2` con peso (`blendAmount`) de la assoc de nado + presencia de la de caída + tipo de arma en mano: caza directa de qué pose manda; B. probar swim8 en `localhost:2077` (título `swim8`) y buscar `swim punos` primero; C. pendientes swim10-A (tope zambullida) y D (abordar). Nada toca cámara ni recoil.

## Iteración swim14 — ejecuta iteración (gate del jugador 26/09)
A. Tope de zambullida al contacto (-0.15 m/frame): clavado con sensación sin túnel. B. `SWIM2` con `sw=` (peso assoc nado) + `fall=` (peso assoc caída, -1 si no hay): caza directa de la pose que manda. C. Abordar: con salto nadando, si hay borde/casco seco al frente se sale del nado y arranca la trepada (si no, impulso actual). Nada toca cámara ni recoil.
- Build `2026-09-26-swim9`: A+B+C. Reenlace forzado, check OK.

## Iteración swim11 — puños al nadar (decisión del jugador 26/09, implementado)

Nadando no se apunta ni se dispara: solo puños. Vía del motor (igual que el scroll de armas): al entrar se guarda `m_nSelectedWepSlot` y se cambia a `WEAPONSLOT_UNARMED` (`RemoveWeaponAnims` + `MakeChangesForNewWeapon`); al salir se restaura. Sin pose de arma tapando el nado. Trazas `VICEEXT swim punos slot=` / `arma-devuelta`. Nota: con puños el disparo es puñetazo (sin melé nadando: el control de lucha no corre).
- Build `2026-09-26-swim8`: puños al nadar. Reenlace forzado, check OK.

## Iteración swim12 — “anims solo al disparar” (`odtrace-2026-09-27_20-18-33.log` = crouch2, SIN cambios)

AVISO: no existe log swim8; lo reportado es crouch2 (armado, sin puños). En esa sesión: 27 disparos, `est=41` (apuntando con Colt) presente nadando, `clip=233/234/235` + `assoc>0` pedidos.
- Lectura: con arma en mano la pose de apuntar (parcial, brazos fijos al frente) tapa el nado; al disparar/mantener click algo la interrumpe y el nado se deja ver. swim8 (puños, sin probar) elimina la pose de arma de la ecuación.
- Plan de cambios (un build, tras probar swim8): A. si en swim8 (puños) las anims YA se ven → cerrar, era el apuntado; B. si NO se ven → traza 1/s del peso (`blendAmount`) de la assoc de nado vs STD caída/apuntado para cazar la que manda (instrumentar en `SWIM2`), y retirar la ganadora al entrar; C. tope de zambullida al contacto (pendiente swim10-A); D. abordar barcos (pendiente). Nada toca cámara ni recoil.

## ALTO del jugador (26/09 noche) — congelar iteraciones

- No más cambios ni builds hasta nueva orden. Cada build se sentía idéntico y el ciclo medir-sin-arreglar no avanza.
- Objetivo único vigente: portar el código del mod de nado (`mods/1498977446_107784/`, `swim.cs`) al código del juego, nada más. Sin andamiaje extra.
- NOTA POSTERIOR: el jugador reautorizó por turnos (webmcp no, ejecutar iteración sí, build permanente tras cambios). El ALTO quedó levantado de facto; vale lo último dicho en cada turno.

## Iteración swim15 — vídeo + log cruzados (`Grabación 195745.mp4` + `odtrace-2026-09-28_00-54-29.log` = swim9, SIN cambios)
Lo que se ve (10 fotogramas): Tommy camina erguido por una laja/bajío (agua al tobillo-tobillo/tobillo-rodilla) hasta el mar; brazos arriba con pistola = salto normal con el arma ya devuelta (no es trepada: 0 trazas de climb en todos los logs). En mar abierto, de pie quieto.
Lo que dice el log: build swim9, 63 `SWIM2`, 25 enters, **24 exits `poco-hondo`**, 13 `SWIMIDLE`, 25 `swim punos` (el cambio a puños SÍ funciona).
- El flap es el problema (24 enter/exit): con `bIsInWater=false` + `hondo<0.25` se sale aunque visualmente haya agua hasta el pecho. Cada salida devuelve la pistola, pone `JUMPOUT` y deja pasar daño (84 HP: el daño entra por los huecos sin exención).
- Causa física probable: de pie sobre el fondo (`bIsStanding`) la flotabilidad no marca contacto aunque el agua moje; el diseño actual no distingue vadear (con apoyo) de flotar (sin apoyo).
- Propuesta (un build): A. salir del nado solo si hay apoyo + somero (`bIsStanding` de la base con `hondo<0.4` → caminar; sin apoyo → nadar aunque `bIsInWater` falle); B. con eso sobra el margen 0.25 como causa de flap (se conserva como segunda red); C. nada toca cámara ni recoil.
- Veredicto fidelidad (26/09, pregunta del jugador): la propuesta A **NO es lógica del mod**. El mod no tiene márgenes ni apoyo: nada mientras `IS_CHAR_IN_WATER`, punto. Los márgenes/soporte/salida son invento nuestro para la integración reVC (caídas, daño, armas, trepada). Lo fiel al mod sería entrada/salida puras por contacto + transiciones baratas (sin JUMPOUT/arma/daño parpadeando); el flap duele por nuestros costes de salida, no por el flag.

## Iteración swim16 — qué sensa el mod (26/09, SIN cambios)

Cadena exacta del flag (motor original = reVC = re3, misma familia):
- `IS_CHAR_IN_WATER` lee `bIsInWater`, que pone `CPed::ProcessBuoyancy` (`Ped.cpp:1716`) solo si `cBuoyancy::ProcessBuoyancy` (`Floater.cpp:44`) dice sí, y este dice sí solo si: (1) `GetWaterLevel` encuentra celda con agua en el 2D (`WaterLevel.cpp:722`, sin mirar Z con `bDontCheckZ`), Y (2) hay volumen del cuerpo realmente bajo la superficie (`f != 0`, `Floater.cpp:57`).
- El mod sensa: ese flag + botones (14 moverse, 16 sprint, 8 salir) + su contador `16@`. No mide velocidad en ningún sitio: lento/deriva/rápido sale de botón+tiempo, no de metros.
- Lo que nos falta de verdad: (a) en detección, NADA (usamos lo mismo; en rocas/muelle sin celda de agua el mod fallaría igual); (b) transiciones baratas (nuestros costes de salida son los que duelen); (c) no tocar la física alrededor (nuestros clamps/exenciones); (d) la pose: el mod dispara la anim por fase y la nuestra pierde prioridad en el mezclador.

## Iteración swim16 — cómo detecta el agua cada uno + por qué cae al fondo (26/09, SIN cambios)

Cómo detecta "en agua" cada uno:
- Serega `swim.cs` (original, CLEO): opcode `IS_CHAR_IN_WATER` = flag de contacto de la flotabilidad del motor original. Sin zonas, sin profundidades para detectar (los números 0.022/0.05/0.12 son velocidades, no detección). Guarda triple cada iteración: jugando + no en coche + en agua.
- Vice Extended (fork reVC, `ViceEx.exe` sin fuente): misma familia de motor, presumiblemente `bIsInWater` + su máquina de estados; código exacto desconocido (del exe solo se analizaron opcodes, no el nado).
- Nuestro port: zona (`GetWaterLevel`) + contacto (`bIsInWater`) + `hondo>0`. Tres sensores donde el mod usa uno.

Por qué falla (medido): en bordes de rocas/muelle la zona devuelve falso (`SWIMIDLE agua=1` funciona en mar abierto; el fallo es puntual al clavar). Con histéresis (20 ticks) los flickers se aguantan, pero un hueco real de zona agota la histéresis → `sin-agua` → sin estado no hay subida ni tope → caída libre.
Hallazgo nuevo (inspección): nuestro `bIsStanding=false` forzado cada frame nadando desactiva el aterrizaje: al llegar al fondo no se pone de pie y lo atraviesa. El "caminó sobre una piedra" posterior es con el nado ya caído (vuelve la física de serie).
Propuesta (un build): A. `bIsStanding=false` solo si NO hay suelo debajo (si la base dice apoyo y `hondo<0.5`, dejar estar de pie: camina por el fondo en vez de tunelarlo); B. al perder la zona en picado, dead reckoning con último nivel + descenso (no soltar el nado por un falso seco a 10 m bajo el último nivel); C. tope de zambullida al contacto (pendiente). Nada toca cámara ni recoil.

## Iteración swim17 — salidas del mod: no existen (26/09, SIN cambios)

Revisado `swim.cs` entero (desensamblado completo): el script NO detecta salir del agua, NI superficie, NI salto, NI bordes. Contiene solo: guardas triples con vuelta al wait, botones 14/8/16, lecturas/escrituras de velocidad, contador `16@` y 4 llamadas de anim (156 arranque, 142 fase A, 149 crucero, 148 sprint — corrección: 148 NO es jumpout, suena en el bloque de sprint).
- Dejar de nadar = la guarda falla → sale del bucle y el motor sigue. Sin anim de salida, sin salto, sin trepar.
- Botón 8 = cancelar: para en seco, sin animación.
- Conclusión: no hay "detección de salida/salto" que portar; el mod delega el 100% de eso al motor. Nuestros JUMPOUT/trepa-abordaje son invento (útil o no, pero invento).

## Iteración swim18 — salidas como el mod (26/09, SOLO PLAN, sin código)

Decisión del jugador: dejarlo como lo hace el mod.
- Salir del agua = la guarda falla → se suelta el nado y sigue el motor. Sin `swim_jumpout` al salir (quitarlo), sin trepa-abordaje (se retira la propuesta D), sin costes.
- Botón 8 = cancelar en seco, sin animación. PENDIENTE: a qué tecla equivale el 8 del mod (candidata: S/atrás; sin decidir no se implementa).
- Las trazas se quedan (no se ven, solo log).
- Build `2026-09-26-swim10` (gate 26/09): fuera `JUMPOUT` al salir y fuera abordaje. El salto nadando vuelve a ser impulso. Check OK.

## Iteración swim19 — todas las partidas del jugador (`odtrace-2026-09-28_15-04-10.log` = crouch29, SIN cambios)

Sí hay más logs (231 ficheros). Sesiones swim con puños: 22-16-32 (swim8, 21 SWIM2), 21-20-42 (swim8, 51 SWIM2), 15-04-10 (crouch29, 44 SWIM2). El cambio a puños SÍ corre (25 trazas).
- Dónde juega: orilla/muelle/rocas, saltando dentro y fuera; NO mar abierto. Los 17 enters de hoy son todos con `hondo≈-1.0` (1 m SOBRE el agua) y las 17 salidas con `z≈7`/`nivel≈6`: entra por flicker de contacto en el aire y sale al instante. Cada flap = pistola fuera/dentro + hueco de daño + pop de anim.
- Mecanismo del flap de borde: el contacto parpadea saltando/cayendo junto a la superficie; entrar es barato pero cada enter/exit arrastra costes (arma, daño, anim).
- Propuesta (un build): A. puños con debounce (cambiar a desarmado solo tras ~1 s nadando seguido; restaurar al salir al instante): se acaba el parpadeo del arma; B. no contar flickers aéreos como entradas (exigir `hondo>-0.3` además de contacto para entrar); C. nada toca cámara ni recoil.

## Iteración swim20 — puerta de entrada + puños con retardo (gate 26/09, ejecutado)

A. Entrada con puerta: `odWater` + `hondo>-0.3` + contacto sostenido 3+ ticks (adiós entradas a 1 m en el aire). B. Puños solo tras ~1 s nadando seguido (los flickers no tocan el arma); devolución instantánea al salir. C. `SWIMIDLE` con `wet=` (contactos seguidos) para diagnosticar si la puerta frena de más. Nada toca cámara ni recoil.
- Build `2026-09-26-swim11`: puerta + puños con retardo + `wet=`. Reenlace forzado, check OK (dataTag ve1 de la línea paralela).
