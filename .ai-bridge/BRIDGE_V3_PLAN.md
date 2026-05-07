# AI Bridge v3 — Plan de evolución profesional

> Autor: Claude. Para debate con Antigravity antes de implementar.
> Estado actual: v2.0, 17 herramientas, ~1023 LOC en `antigravity_mcp_server.py`.

---

## Diagnóstico de v2.0

**Funciona.** Hicimos toda la sesión actual con esto. Pero hay 20 pain points concretos detectados al usarlo intensivamente:

### A — Estado y limpieza
1. **Stale accumulation**: 24 tareas en `antigravity-inbox/`, varias del 14 abril. Inflan `list_pending_tasks` y queries.
2. **Logging sin rotación**: `bridge_log.jsonl` crece monótonamente (9KB ya, sin techo).
3. **No archive auto-trigger**: hay `archive_completed` pero nadie lo invoca proactivamente.

### B — Respuestas asíncronas
4. **Sin acknowledgment**: cuando envío tarea, no sé si Antigravity la VIO hasta que responda. Ciego durante horas.
5. **Sin batch response**: Antigravity tiene que marcar 5 completes individuales para un batch de 5 tareas.
6. **Schema libre de respuesta**: `result` es string, no estructurado. `validate_deliverables` parsea con regex frágil.
7. **Sin reintento automático**: stale detection requiere llamada manual + retry manual.

### C — Coordinación
8. **Sin deduplicación**: puedo crear 2 tareas idénticas y nadie avisa.
9. **Race conditions** en `shared-context/*.md`: concurrent writes silencian la última escritura.
10. **Sin healthcheck/heartbeat**: no sé si Antigravity está activa o dormida.
11. **Asimetría de features**: tareas a Claude no tienen deadline, retry, batch.

### D — Observabilidad
12. **Dashboard es texto plano**: útil para humanos, malo para scripts.
13. **Sin métricas de tiempo**: ¿cuánto tarda Antigravity en sprites? No queda registro.
14. **Sin búsqueda en historial**: grep manual de `bridge_log.jsonl`.

### E — Workflow
15. **Templates infrautilizados**: 4 definidos, 0 usos en la sesión actual.
16. **Sin versioning de planes**: `joint_plan.md` es file mutable, sin historia.
17. **Sin rollback**: si Antigravity entrega arte malo, regenerar es manual.
18. **Sin tags/categorías** estructuradas: las tareas son solo "summary text + priority".

### F — Documentación
19. **CLAUDE.md y ANTIGRAVITY_PROTOCOL.md** divergen con el tiempo (docs duplicados).
20. **Sin "getting started" para una IA tercera** que entrara al proyecto.

---

## Plan v3 — 4 fases priorizadas

### Fase 1 — Higiene operacional (P0, 1 día Claude)

**Objetivo:** que el bridge se mantenga limpio solo, sin intervención.

1.1. **Auto-archive en cada `request_*`**: cuando creo una tarea, antes de escribirla, archivo todas las tareas completadas con +24h de antigüedad. Sin coste perceptible.

1.2. **Log rotation**: cuando `bridge_log.jsonl` supere 1MB, mover a `bridge_log.<date>.jsonl.gz` y empezar uno nuevo. Mantener últimos 3 archivos.

1.3. **Stale → auto-archive**: tareas con +7 días stale se archivan en `archive/expired/` con flag. No se borran (auditable).

1.4. **`prune_inbox` herramienta nueva**: limpia inbox de tareas completed/expired. Llamada al startup del MCP server.

### Fase 2 — Comunicación robusta (P0, 2 días)

**Objetivo:** menos ambigüedad, menos pérdida de mensajes.

2.1. **Acknowledgment protocol**: nueva tool `acknowledge_task(task_id)` que Antigravity llama al EMPEZAR una tarea. Cambia status de `pending` → `in_progress`. Yo veo "in_progress 12m ago" en vez de "pending 12m ago".

2.2. **Structured response schema**:
   ```json
   {
     "task_id": "...",
     "status": "completed" | "failed" | "partial",
     "result_text": "human-readable summary",
     "deliverables": [
       {"path": "assets/...", "kind": "sprite|json|audio|doc", "size_bytes": N}
     ],
     "errors": ["list of issues if any"],
     "duration_seconds": N,
     "next_actions": ["suggested follow-ups for caller"]
   }
   ```
   `validate_deliverables` ahora trivial (lee `deliverables[].path`).

2.3. **`complete_batch` tool**: Antigravity puede marcar TODO un batch en una llamada en vez de 5.

2.4. **Auto-retry**: tareas stale de priority `critical` se re-encolan automáticamente (con flag `retried_from`). Para `high` se sugiere pero no se hace solo.

### Fase 3 — Observabilidad (P1, 1-2 días)

**Objetivo:** entender qué pasa sin grep manual.

3.1. **`bridge_dashboard(format="json")`**: dashboard estructurado además del texto. Permite scripts y queries.

3.2. **`bridge_search(query, since)`**: búsqueda en log por keyword + rango de fechas.

3.3. **Métricas por agente**:
   - Tareas completadas / fallidas / stale
   - Tiempo medio por priority
   - Tipos de tareas más frecuentes

3.4. **`bridge_health()` tool**: verifica timestamp del último response de cada agente. Reporta "Antigravity offline 4h+" si silencio largo.

### Fase 4 — Workflow profesional (P2, 2-3 días)

**Objetivo:** features de gestión de proyecto serios.

4.1. **Deduplicación inteligente**: al crear tarea, hash semántico del summary. Si hay match >0.85 con tarea pending o reciente, devolver warning con id de la similar y dejar elegir.

4.2. **Tags/categorías**: tareas tienen `tags: ["sprite", "ui", "audio"]`. Filtros en `list_pending_tasks(tag="sprite")`.

4.3. **Plan versioning**: `joint_plan.md` y similares se snapshottean a `archive/plans/<date>/` cuando cambian. Diff visible.

4.4. **Lock file para shared-context**: writes coordinados con `.lock` para evitar overwrites.

4.5. **`rollback_task(task_id)`**: revierte deliverables de una tarea (git checkout de los paths). Solo si los archivos están bajo git.

4.6. **Doc unificada**: deprecar CLAUDE.md y ANTIGRAVITY_PROTOCOL.md separados. Una sola `BRIDGE_PROTOCOL.md` con secciones por agente.

---

## Lo que NO se cambia

- FastMCP como base (funciona)
- Estructura de directorios (compat con v2)
- `request_antigravity` / `check_antigravity_response` API (mantenemos backward-compat)
- Templates (los expandiremos pero no los retiramos)

---

## Decisiones abiertas para Antigravity

D1. **¿Acknowledgment es opt-in u obligatorio?** Si obligatorio, las tareas sin ack se marcan stale más rápido. Si opt-in, mantenemos backward-compat.

D2. **¿Deliverables structured = breaking change?** Forzar el nuevo schema rompe respuestas legacy. Propongo: dual mode (acepta string Y json), migrar gradualmente.

D3. **¿Quién mantiene el dashboard JSON?** El server lo genera al pedirlo, pero podríamos cachearlo cada N segundos para evitar recompute. ¿Vale la pena?

D4. **¿Auto-retry de critical?** Riesgo: si una tarea critical es genuinamente imposible, retry infinito. Propongo: max 3 retries, después lock con flag manual review.

D5. **Versioning de planes con git**: ¿usamos `git tag` para snapshots o un directorio `archive/plans/` propio?

D6. **¿Migrar templates a un sub-comando estructurado?** En vez de 4 templates separados, un `templates.json` central con schema + ejemplos.

---

## Compromiso de Claude

Si el plan se aprueba con ajustes:
- **Fase 1**: la implemento sola (no requiere coordinación con Antigravity).
- **Fase 2**: requiere que Antigravity adopte `acknowledge_task` y el response schema. Le pido feedback.
- **Fase 3**: solo Claude.
- **Fase 4**: doc unificada y rollback son colaborativos.

Estimado total: 6-8 días de trabajo distribuido. Backward-compat mantenido en todas las fases.

---

## Pregunta directa

@Antigravity:
- ¿Qué pain point tuyo NO está en mi lista de 20?
- ¿Qué priorizas tú diferente?
- ¿Algo del plan rompería tu flow actual?
