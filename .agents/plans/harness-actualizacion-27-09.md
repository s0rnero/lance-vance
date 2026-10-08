---
name: harness-actualizacion
status: EXECUTED
type: maintenance
domain: harness
owner_rules: .agents
created: 2026-09-27 23:40
---

# Plan: Actualización del harness (.agents) y fin de la puerta de licencias

> Origen: instrucción directa del jugador (27/09): *«si actualiza eso y lo que
> mencionaste anteriormente en agents, actualiza todo el harness donde veas
> necesario»*. «Eso» = retirar la licencia como puerta; «lo anteriormente
> mencionado» = rellenar el perfil del proyecto, adaptar las convenciones y el
> diseño, y crear los ADR. Ejecutado en la misma sesión por instrucción explícita.

## 0. Objetivo

Dejar el harness (`.agents/`) fiel a la realidad del proyecto y coherente con la
decisión del jugador de que **la licencia no es una barrera** para tomar código de
`mods/`.

## 1. Alcance

- **Licencias**: `00-INDICE.md` §2, `09-numeracion` §2, `13-inventario` §4/§7,
  `06-fuentes-externas` §4.2, `12-handoff-tanda2` §13 y `docs/mods/ATTRIBUTION.md`.
- **Perfil**: `.agents/AGENTS.md` §1 (stack, estructura, scripts reales).
- **Convenciones**: `.agents/CODING_STANDARDS.md` (adaptado a C++/Emscripten y a las
  reglas de casa). **Diseño**: `.agents/DESIGN.md` (arquitectura real del port).
- **Decisiones**: ADR nuevos en `.agents/decisions/` (001-005).
- **Historial**: entrada en `.agents/HISTORIAL.md`.

## 2. Restricciones

- No se toca código del motor ni datos (`src/`, `gta_vc_browser/streamed|bootseed`).
- Los `.md` del harness son **LF** salvo `HISTORIAL.md` (**CRLF**): editar respetando
  el final de línea de cada fichero (python binario).
- Normativo del harness en **inglés** (RULES 0.14); los planes del usuario en español.

## 3. Cambios

1. **Licencias**: nota de norma vigente (27/09) en los planes citados y en
   `ATTRIBUTION.md`; se aclara que lo excluido es por motivo **técnico** (WASM), no
   legal. Se conservan las tablas como inventario histórico.
2. **Perfil** (`AGENTS.md` §1): stack (C++/reVC, Emscripten, CMake+Ninja, librw GL3,
   OpenAL, JS/Vite, Python, Node), estructura del repo y **scripts reales** (build,
   objeto único, stage/manifest, dev Vite, verificadores).
3. **`CODING_STANDARDS.md`**: estructura de ficheros y features detrás de `VICEEXT_*`,
   naming re3, contrato de unidades (m/s ÷ 50), verificación por verificadores + logs,
   reglas de casa (CRLF, python binario, `VERSION`/`dataTag`, sin commits, no matar
   node) y anti-patrones ya pagados.
4. **`DESIGN.md`**: capas (web / engine / datos), modelo de integración «portar, no
   cargar», fronteras de carriles, contratos (tags, unidades, trazas), rendimiento/caps,
   entornos y documentación de decisiones.
5. **ADR**: `001` portar lógica (no cargador de mods), `002` la licencia no es puerta,
   `003` el mod manda sobre lo existente, `004` API única `ViceExtIsAiming`,
   `005` medición = partida del jugador + logs.

## 4. Verificación

- Lectura de cada fichero editado para confirmar el contenido y el final de línea.
- `git status --short .agents docs/mods` para ver la lista de ficheros tocados.
- No hay build ni tests que correr: cambio documental puro.

## 5. Closure (memoria persistente)

- **Qué cambió**: política de licencias (6 ficheros + ATTRIBUTION); perfil real de
  `AGENTS.md`; `CODING_STANDARDS.md` y `DESIGN.md` adaptados al stack; 5 ADR nuevos;
  entrada en `HISTORIAL.md`; este plan.
- **Verificación**: lectura posterior (LF/CRLF correctos) y `git status` documentado.
  Sin build/tests (cambio documental).
- **Resultado**: aprobado (por instrucción directa del jugador).
- **Pendiente**: ninguno. Nota: los ficheros generados en la raíz (`AGENTS.md`,
  `CLAUDE.md`, `.cursorrules`, `.github/copilot-instructions.md`) son punteros al
  harness y no requieren regeneración (no contienen el perfil).

## 6. Iteración 1.1 — fuera el subagente de reviewer y el estado `CLOSED` (29/09)

> Jugador: «elimina subagente de reviewer y cualquier referencia a closed; esos
> es de un harness antiguo». `CLOSED` y `reviewer.md` no existen en el ciclo de
> `WORKFLOW.md` (que solo define `CREATED -> ENRICHED -> EXECUTED`); se arrastraban
> desde una versión anterior del arnés.

### Qué cambió

- **Eliminado** `.agents/subagents/reviewer.md` (borrado por el jugador; su función
  era la lista de cierre previa a `CLOSED`).
- **`.agents/AGENTS.md`**, 4 sitios: fuera la fila de `reviewer.md` del árbol de
  `.agents/` y del rol «Review»; el flujo pasa de «Closing review (reviewer.md) + DoD
  -> CLOSED» a «DoD verified by the orchestrator -> finished»; las reglas de estado
  pasan a los tres reales (`CREATED`, `ENRICHED`, `EXECUTED`) y se dice que el
  **propio orquestador** verifica el DoD, sin subagente ni estado extra; el resumen
  rápido apunta al DoD de `WORKFLOW.md`.
- **Plantilla de cierre** de `agachado-correcciones-axis.md` y
  `escalada-climbing-completo.md`: «sin esta entrada el plan no pasa a `CLOSED`» →
  «no se da por terminado».

### Verificación

- `CLOSED` y `reviewer` grep sobre todo `.agents/`: **0 referencias colgantes** en el
  harness. `RULES.md`, `WORKFLOW.md`, `PLANS.md`, `DESIGN.md` y
  `CODING_STANDARDS.md` ya no tenían ninguna de las dos.
- `AGENTS.md` sigue en LF (217 líneas, 0 CRLF); los dos planes tocados también LF.
- **Una referencia se deja a propósito**: `apuntado-classicaxis-100.md:2384` dice
  que «la revisión del checklist de `reviewer.md` lo pide como DoD». Es una closure
  **histórica** que describe lo que una revisión anterior hizo, y reescribirla sería
  falsear la memoria del proyecto (`RULES.md` 0.10). Queda pendiente de que el
  jugador decida.

### Resultado

- Hecho por instrucción directa del jugador. Sin build ni tests (cambio documental).

### Pendiente

- El jugador decide sobre la referencia histórica de `apuntado-classicaxis-100.md`.
- Los estados viejos `PENDING` y `READY` siguen apareciendo en `AGENTS.md` (§5 y §5
  Authorization Gates) y un plan (`escalada-climbing-completo.md`) está en `PENDING`.
  No se han tocado porque no estaban en el encargo; `WORKFLOW.md` nunca los define.

## 7. Iteración 1.2 — «sin comentarios en el código» como regla del harness (29/09)

> Jugador: «no dejes comentarios en el codigo, dejalo como regla». La convención
> del harness iba **en contra**: `CODING_STANDARDS.md` §1 obligaba a que cada añadido
> de carril llevara un comentario de cabecera (`// R29 ...`, `// C18-1 ...`). El
> jugador lleva-ops insistingiendo en ello desde la iteración 1.2 del nado, y la
> excepción estaba solo escrita en el plan de nado, no en la norma.

### Qué cambió

- **`CODING_STANDARDS.md` §1**: sustituida la convención del comentario de carril
  por la regla del proyecto — **cero comentarios en el código**, incluidos los
  encabezados de carril. Se dice explícitamente que la regla prevalece sobre la convención de re3, que
  la documentación vive en `.agents/plans/` y en la ADR, y que **los comentarios
  ya existentes en código que no tocamos se quedan** (`RULES.md` 0.17: no limpiar
  el trabajo de otro carril).
- **`CODING_STANDARDS.md` §7** (tabla de referencia rápida): fila nueva
  `Comments | Document in the plan, not in the code | Lane headers, explanations or
  notes in what you write or modify`.
- **`WORKFLOW.md`**, Definition of Done: el punto de higiene pasa a «No secrets,
  debug logs, **comments**, dead code or duplication», para que la regla se
  compruebe al cerrar cada plan.
- **`RULES.md`**, bloque local `USER-START` (sobrevive a las actualizaciones): la
  regla como regla absoluta, apuntando a `CODING_STANDARDS.md` §1 y al DoD.

### Verificación

- Las tres conexiones leídas tras el cambio: la convención en §1, la fila en §7, el
  punto del DoD y la entrada local de `RULES.md`. Los tres ficheros siguen en LF.
- No hay build ni tests: cambio documental puro.

### Resultado

- Hecho por instrucción directa del jugador.

### Pendiente

- Ninguno. Los comentarios ya escritos en el código del repo **no se borran**: la
  regla aplica a lo que se escriba o modifique a partir de ahora.
