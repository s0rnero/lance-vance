---
name: 08-encargo-subagente-2
status: EXECUTED
type: research
domain: gameplay
owner_rules: .agents
created: 2026-09-21
---

# 08 · Reparto tras la 9ª partida ("no ha cambiado nada")

**Fecha:** 21/09/2026 · **Build:** `ve19` (enlazado 20:06) · **Datos:** `ve13`

## A · Qué pasó de verdad (importante para no repetirlo)

El jugador jugó **varias partidas con el motor viejo**. Medido:

| Qué | Dato |
|---|---|
| Motor servido antes del build de hoy | `web/public/build/reVC.wasm` del **21/09 08:38** (`ve17`) |
| Marcas dentro de ese wasm | `VICEEX sfx arma` sí; **`odViceExSample`, `SWIM2`, `CROUCH2`, `SVLIGHTS`, `RECOIL2`, `AIMDIR` = 0** |
| Capa web servida | bundle con `dataTag` **ve12** y **sin `ODSTA`** (el R2 no estaba) |

Es decir: R5-R11, D8b, R2 y R3 estaban **compilados como objeto** pero el paquete
nunca se había enlazado (se acordó "un único build al final" y no se hizo antes de
pedir la partida). **Todo su informe de "sigue exactamente igual" es correcto:** lo
que probó era el código de ayer. Desde el log esto NO se ve (la traza dice qué
corrió, no qué se esperaba que corriera).

### Regla nueva (obligatoria antes de pedir una partida)

```bash
bash gta_vc_browser/tools/check-served-build.sh
```

Dice, una por una, si el motor servido lleva las marcas de lo implementado y si el
`dataTag` es el último. Si sale FALLO, enlazar: `bash gta_vc_browser/build.sh`
(necesita `emcc` en el PATH; en este equipo: exportar `$EMSDK` + `$EMSDK/upstream/emscripten`).
Hoy sale **OK: todas las marcas** (build de 20:06).

## B · Reparto (el jugador pidió dividir de nuevo el trabajo)

**Subagente (mecánicas pesadas: IA, vehículos, mundo).** Tareas X8, X9, X4, X10,
X11, X26 — todas de la sección 2 y sin dependencia de una partida.
**Sección 1 (yo).** Verificación desde log, audio, datos y HUD: X2, X3, X7, X12,
X13, X17, X18, X19, X22, X23, X24, el audio de las luces de servicio
(`DMAudio.Service`), la opción de menú `Real 1st Person` + FOV/head-bob de R5, y
los bloques R5-R11 en el verificador de logs.

## C · Encargo para el subagente (pegar tal cual en su hilo)

> Lee este documento (`.agents/plans/mecanicas/08-encargo-subagente-2.md`) y el
> backlog (§E) de `.agents/plans/mecanicas/06-plan-correcciones-5a-partida.md`.
> Ejecuta **en este orden**, un bloque por tanda, cada uno con su traza y su
> criterio PASS, y compilando SOLO sus objetos con ninja (sin enlazar):
>
> 1. **X8 · Los policías se agachan antes de cubrirse** (mod 2.6). Clips
>    `Crouch_*` ya servidos por el `ped.ifp` del mod. Traza
>    `COPCOVER accion=agacha|cubre estado=…` (≤1/s por policía). PASS: en un
>    tiroteo con estrellas se ve `agacha` antes de `cubre`.
> 2. **X9 · Aspas del helicóptero sin piloto** (2.6): el rotor sigue girando si
>    nadie lo pilota. Traza `ROTOR on=… vel=…`. PASS: helicóptero vacío con rotor
>    girando.
> 3. **X4 · La gasolinera explota al chocarla** (2.6): reutilizar el depósito
>    (`gastank`) ya portado. Traza `GASSTAT blast motivo=choque`. PASS: chocar y
>    explotar.
> 4. **X10 · Detalle de vehículo** (2.5/2.0/2506): rueda que se cae con impacto
>    fuerte, puertas que golpean, giro aleatorio de coches aparcados, rotación de
>    ruedas al salir del coche, intermitentes en coches de serie. Una traza por
>    pieza (`WHEELOFF`, `DOORSLAM`, `PARKSTEER`, `TURNERS std=1`).
> 5. **X11 · Luces del coche** (2.5): textura del faro, se rompen al chocar y al
>    dispararles, haz separado, humo/fuego del capó si está cerrado. Mismo camino
>    que R11 (ya hecho).
> 6. **X26 · Pulido "vanilla+"** (1.0): SWAT lanza gas lacrimógeno, dos
>    helicópteros a 5-6 estrellas, daño al Maverick según el arma, cañón de agua
>    del camión de bomberos, conductores que reaccionan a los disparos, la pared
>    protege de la explosión, partículas de sangre/huellas más duraderas.
>
> Reglas: no toques `gta_vc_browser/**`, ni `src/text/Messages.cpp`, ni
> `src/renderer/Hud.cpp`, ni `src/core/config.h` (pídelos). **No enlaces el
> paquete** (lo enlaza la sección 1 con `check-served-build.sh` de por medio).
> Cada bloque: entrada propia al final de `.agents/HISTORIAL.md` con
> "causa → código → traza → PASS" y el estado de compilación por objeto.
> Nada de commits.
>
> Antes de empezar, mira `bash gta_vc_browser/tools/check-served-build.sh` para
> entender la trampa de la 9ª partida: lo que no esté enlazado no existe para el
> jugador.

## D · Qué hará la sección 1 en paralelo (para no pisarse)

- Añadir los bloques **R5-R11** a `tools/viceext-log-check.py`, para dictaminar
  desde el log: `1p key`/`CAM1P` (1ª persona), `RECOIL2` (retroceso), `CROUCH2
  peso=` (agachado), `SWIM2 avance=` (nado), `AIMDIR desv=` (apuntado),
  `SVLIGHTS dummies=` (luces de servicio) y `VICEEX sfx`/`ODSFXMISS` (sonidos).
- **X7** iconos de emisora, **X2** triángulo de salud, **X3** proyectil que
  explota al dispararle, **X12/X13** interruptores y trucos, **X18** sonido del
  agua, **X19** iconos de tecla con `Shift`, **X22** radio de aparición,
  **X23** Micro-UZI, **X24** datos sueltos.
- **Audio de las luces de servicio** (`DMAudio.Service`) — es audio, es mío.
- **R5 pendiente:** `Real 1st Person` en el menú + `FOV_FirstPerson` /
  `HeadBob1stPerson` (necesita `config.h` y `Frontend`, que son míos).
