# Architecture Decision Records (ADR)

Permanent registry of architecture and design decisions. It complements `.agents/plans/`:

- **Plans** record how a task was executed (context, steps, verification) and are the project's operational memory.
- **ADRs** record why the architecture is the way it is (decision, context, consequences) and are permanent design memory.

## Rules

- Every decision affecting the project's architecture or design is recorded as an ADR (template in `.agents/templates/adr.md`).
- ADRs are **permanent**: they are never deleted nor edited retroactively. A change of decision is recorded as a new ADR that marks the previous one as `superseded`.
- File naming: `<number>-<slug>.md` (e.g. `001-use-bun.md`).
- States: `proposed` -> `accepted` -> `superseded`.

## Flow

1. During a plan (or by direct user decision), an architecture decision is detected.
2. The ADR is created in `proposed` state with the decision and its consequences.
3. Once confirmed, it moves to `accepted`.
4. If time invalidates it, a new ADR marks it `superseded` with the cross-reference.

**Language note (RULES.md 0.14):** ADRs are written in English. ADR-001 is kept as a historical record of the decision it made; the English policy applies prospectively from ADR-002 onward.

<!-- USER-START -->

## Local additions (preserved on update)

- **Corrección en el sitio, autorizada (07/10/2026).** El jugador autorizó expresamente
  arreglar los ADR 004, 007, 009, 010 y 011 allí donde afirmaban algo que ya no se
  sostiene: los nombres de la API del árbitro (`ADR-008` los cambió), el conflicto del
  agachado dado por resuelto, el HUD dado por convertido, la excepción de la deagle en la
  pose de apuntar y las builds desfasadas. Se editó el texto equivocado y se conservó lo
  demás — sin parches al final. A partir de aquí vuelve a aplicar la regla 0.12: un cambio
  de decisión se registra como un **ADR nuevo** que marca el anterior `superseded`.
- Hoy el único feature validado de punta a punta por las partidas del jugador es el
  **nado** (07/10/2026): en cualquier ADR, `accepted` significa «decisión tomada», no
  «verificado en el juego».

<!-- USER-END -->
