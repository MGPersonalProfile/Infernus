# AI BRIDGE v2.0 — INSTRUCCIONES OBLIGATORIAS

> ESTE ARCHIVO SE LEE AUTOMATICAMENTE AL TRABAJAR EN .ai-bridge/. NO LO IGNORES.

Lee `docs/CLAUDE_PROTOCOL.md` para el protocolo completo.

## Resumen critico (17 herramientas MCP)

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
- `bridge_dashboard()` — resumen completo
- `bridge_log(last_n)` — eventos recientes
- `bridge_stats()` — estadisticas
- `archive_completed(older_than_hours)` — limpiar tareas viejas

## Reglas
- El bridge es AUTOMATICO — no le digas al usuario "dile a Antigravity"
- Delega: imagenes, sprites, web search, browser, analisis visual
- NO delegues: C++, CMake, JSON, refactoring, bugs
- Al iniciar sesion: `bridge_dashboard()` + `get_all_pending_responses()`
- Al terminar: `update_context()` + `archive_completed()`
