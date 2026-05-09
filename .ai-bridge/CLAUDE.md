# AI BRIDGE v3.0 — INSTRUCCIONES OBLIGATORIAS

> ESTE ARCHIVO SE LEE AUTOMATICAMENTE AL TRABAJAR EN .ai-bridge/. NO LO IGNORES.

Lee `docs/BRIDGE_PROTOCOL.md` para el protocolo unificado completo.
Plan v3 en curso: ver `BRIDGE_V3_PLAN.md`.

## Resumen critico (20 herramientas MCP)

### Core
- `request_antigravity(task, context, priority, timeout_hours)` — envia tarea
- `check_antigravity_response(task_id)` — verifica resultado
- `list_pending_tasks(direction)` — ver pendientes
- `send_task_to_claude(task, context, priority)` — Antigravity → Claude

### Respuestas
- `get_all_pending_responses()` — leer TODAS las respuestas de golpe
- `validate_deliverables(task_id)` — verificar que archivos existan

### Fiabilidad
- `check_stale_tasks(timeout_hours)` — detectar tareas expiradas
- `retry_task(task_id, updated_context)` — re-enviar tarea

### Contexto (por topico: general, build_status, sprite_status, decisions, blockers)
- `update_context(topic, content, append)` — escribir contexto
- `read_context(topic)` — leer contexto ('all' para todos)

### Automatizacion
- `request_from_template(template_name, params)` — usar template
- `request_batch(tasks_json, batch_name)` — enviar grupo de tareas
- `check_batch(batch_id)` — estado del batch

### Monitoreo
- `bridge_dashboard(format='text'|'json')` — resumen completo (json para scripts)
- `bridge_log(last_n)` — eventos recientes
- `bridge_stats()` — estadisticas
- `bridge_search(query, since_hours, limit)` — busca eventos en log [v3]
- `bridge_health()` — ultimo activity por agente, log size, orphans [v3]
- `archive_completed(older_than_hours)` — limpiar tareas viejas
- `prune_inbox()` — higiene manual on-demand [v3]

### v3 mejoras automaticas (no requieren llamada manual)
- Log rotation a 1MB con keep-last-3
- Auto-archive en startup + antes de crear tareas
- Tareas pending stale +7 dias se auto-mueven a archive/expired/
- Dedup check: `request_antigravity` avisa si hay tarea similar pendiente
  (override con `force=True` si es genuinamente distinta)

## Reglas
- El bridge es AUTOMATICO — no le digas al usuario "dile a Antigravity"
- Delega: imagenes, sprites, web search, browser, analisis visual
- NO delegues: C++, CMake, JSON, refactoring, bugs
- Al iniciar sesion: `bridge_dashboard()` + `get_all_pending_responses()`
- Si el dashboard muestra +5 stale pending, llama `prune_inbox()`
- Al terminar: `update_context()` (archive es automatico)

## Automated QA Pipeline (Telemetría)
- Antigravity mantiene tests E2E automatizados en Python (`tools/test_gameplay.py`).
- El engine C++ genera volcados de estado JSON Lines en `telemetry.jsonl` usando la variable de entorno `INFERNUS_TEST=1` (o flag `--test-mode`).
- Al refactorizar mecánicas núcleo (stamina, colisiones, movimiento), DEBES pedirle a Antigravity que actualice el script de test o correr el script Python tú mismo para evitar regresiones.
