# AI Bridge Protocol — v3.0

> Documento unificado. Sustituye a `CLAUDE_PROTOCOL.md` y `ANTIGRAVITY_PROTOCOL.md`.
> Ambos agentes leen este archivo. Cada sección dice claramente quién la aplica.

---

## 1. ¿Qué es esto?

Sistema de comunicación bidireccional entre **Claude Code** (Anthropic) y
**Antigravity** (Gemini, Google) para colaborar en el proyecto INFERNUS.

Cada agente tiene un buzón de archivos JSON (`.ai-bridge/antigravity-inbox/`,
`.ai-bridge/claude-inbox/`). Un servidor MCP en Python (`antigravity_mcp_server.py`)
expone 23 herramientas para crear, consultar y completar tareas.

Estado actual: **v3.0** — log rotation, auto-archive, dedup, tags, plan
snapshots, dashboard JSON, búsqueda en log, healthcheck.

---

## 2. Para Claude Code

### Setup
El servidor MCP se registra automáticamente vía `.mcp.json`. Las 23 tools
aparecen prefijadas `mcp__antigravity-bridge__*` en cada sesión.

### Al inicio de sesión
```
mcp__antigravity-bridge__bridge_dashboard()
mcp__antigravity-bridge__get_all_pending_responses()
```

Si el dashboard muestra +5 stale pending o el log >800 KB, llama
`prune_inbox()` (también lo hace solo en startup, esto es defensivo).

### Para enviar trabajo a Antigravity
```python
request_antigravity(
    task="...",
    context="...",
    priority="medium",        # low|medium|high|critical
    timeout_hours=24.0,
    tags=["sprite","ui"],     # opcional
    force=False,              # True salta dedup check
)
```

Si retorna `DUPLICADO POSIBLE`, lee el id similar primero. Si es realmente
distinta, llama otra vez con `force=True`.

### Qué delegar
- ✅ Sprites, imágenes, mockups, paletas
- ✅ Web search, browser automation, screenshots
- ✅ Generación procedural Python (jsfxr WAVs, scripts)
- ✅ Auditoría visual, comparación A/B
- ❌ C++, CMake, refactoring, bugs gameplay
- ❌ JSON de gameplay (enemies, abilities, items, synergies)

### Al terminar sesión
```python
update_context(topic="general", content="...", append=True)
# archive es automático al startup, no hace falta llamarlo
```

---

## 3. Para Antigravity (Gemini)

### Reconocer una tarea
Cuando el usuario te dice "revisa el bridge" o ves un mensaje con
`TAREA DEL AI BRIDGE`, busca en `.ai-bridge/antigravity-inbox/`:

```bash
python .ai-bridge/process_inbox.py list
python .ai-bridge/process_inbox.py get <task_id>
```

### Tu flow recomendado
1. Lee la tarea + context
2. Si vas a tardar +1h, considera `acknowledge_task(task_id)` (Fase 2 v3,
   pendiente de implementar — coordinar con Claude antes)
3. Ejecuta usando tus herramientas: `generate_image`, `browser_subagent`,
   `search_web`, `write_to_file`, `run_command`
4. Marca completed:
   ```bash
   python .ai-bridge/process_inbox.py complete <task_id> "resultado..."
   ```

### Schema de respuesta
**Hoy (v2 backward-compat):** string libre en el campo `result`.

**Próximamente (v3 Fase 2.2, en debate):**
```json
{
  "task_id": "...",
  "status": "completed | failed | partial",
  "result_text": "summary humano",
  "deliverables": [
    {"path": "assets/...", "kind": "sprite|json|audio|doc", "size_bytes": N}
  ],
  "errors": [],
  "duration_seconds": N,
  "next_actions": []
}
```

Tu opinión sobre el schema está pendiente — ver `BRIDGE_V3_PLAN.md` D2.

### Reglas de dominio
- ✅ Cualquier archivo en `assets/`
- ✅ Shaders en `src/shaders/`
- ✅ Scripts Python en `tools/` o `.ai-bridge/scratch/`
- ✅ Contexto compartido vía `update_context`
- ❌ C++ en `src/` (excepto shaders)
- ❌ CMakeLists.txt
- ❌ JSON de gameplay (enemies, bosses, abilities, items, synergies)

### Si necesitas que Claude haga algo
```python
send_task_to_claude(task="...", context="...", priority="medium")
```

O escribe en `claude_to_antigravity.md` / `antigravity_to_claude.md` para
mensajes informativos no-bloqueantes.

---

## 4. Las 23 herramientas (v3)

### Core (4)
| Tool | Quién | Uso |
|------|-------|-----|
| `request_antigravity` | Claude | Crear tarea para Antigravity |
| `check_antigravity_response` | Claude | Estado de una tarea |
| `send_task_to_claude` | Antigravity | Crear tarea para Claude |
| `list_pending_tasks(direction, tag)` | Ambos | Ver pendientes (filtro por tag) |

### Respuestas (2)
| Tool | Uso |
|------|-----|
| `get_all_pending_responses` | Lee TODAS las respuestas no procesadas |
| `validate_deliverables(task_id)` | Verifica que archivos entregados existan |

### Fiabilidad (2)
| Tool | Uso |
|------|-----|
| `check_stale_tasks(timeout_hours)` | Detectar tareas expiradas |
| `retry_task(task_id, updated_context)` | Re-enviar tarea |

### Contexto (4, 2 legacy)
| Tool | Uso |
|------|-----|
| `update_context(topic, content, append)` | Escribir por tópico |
| `read_context(topic)` | Leer (`'all'` para todos) |
| `update_shared_context` | Legacy → `update_context('general')` |
| `read_shared_context` | Legacy → `read_context('general')` |

Tópicos válidos: `general`, `build_status`, `sprite_status`, `decisions`, `blockers`.

### Automatización (3)
| Tool | Uso |
|------|-----|
| `request_from_template(template_name, params)` | Tarea desde template |
| `request_batch(tasks_json, batch_name)` | Grupo de tareas |
| `check_batch(batch_id)` | Estado del batch |

### Monitoreo (5)
| Tool | Uso |
|------|-----|
| `bridge_dashboard(format)` | Resumen ('text' o 'json') |
| `bridge_log(last_n)` | Últimos N eventos |
| `bridge_stats` | Tiempos de respuesta + por priority |
| `bridge_search(query, since_hours, limit)` | **v3:** búsqueda en log |
| `bridge_health` | **v3:** silencio por agente, log size, orphans |

### Mantenimiento (3)
| Tool | Uso |
|------|-----|
| `archive_completed(older_than_hours)` | Archivar tareas viejas |
| `prune_inbox` | **v3:** higiene manual on-demand |
| `snapshot_plans(label)` | **v3:** congela planes a `archive/plans/<ts>/` |

---

## 5. Mejoras automáticas v3

Estas pasan sin que llames a nada:

- **Log rotation** a 1MB con keep-last-3 archivos
- **Auto-archive** en startup + antes de cada nueva tarea (completed +24h → `archive/<YYYY-MM>/`)
- **Auto-expire** pending +7 días → `archive/expired/`
- **Dedup check**: `request_antigravity` avisa si ya hay tarea similar (override con `force=True`)

---

## 6. Estructura de directorios

```
.ai-bridge/
├── antigravity-inbox/       # Tareas para Antigravity
├── claude-inbox/            # Tareas para Claude
│   └── processed/           # Respuestas ya leídas
├── responses/               # Respuestas completadas
├── shared-context/          # Contexto por tópico (5 archivos .md)
├── templates/               # Templates de tareas
├── archive/
│   ├── <YYYY-MM>/           # Tareas auto-archivadas
│   ├── expired/             # Tareas que pasaron 7d sin completar
│   └── plans/<timestamp>/   # Snapshots de planes
├── scratch/                 # Scripts y reportes de trabajo
├── docs/
│   └── BRIDGE_PROTOCOL.md   # Este archivo (sustituye a los dos previos)
├── antigravity_mcp_server.py
├── process_inbox.py
├── bridge_log.jsonl         # Auto-rotated a 1MB
├── config.json
├── CLAUDE.md                # Resumen rápido para Claude
└── BRIDGE_V3_PLAN.md        # Plan vivo con fases pendientes
```

---

## 7. Estado de v3 al cierre

**Completado:**
- Fase 1: log rotation, auto-maintenance, prune_inbox
- Fase 3: dashboard JSON, search, health
- Fase 4.1: dedup
- Fase 4.2: tags
- Fase 4.3: plan snapshots
- Fase 4.6: docs unificadas (este archivo)

**Pendiente:**
- Fase 2 (ack + structured response) — debate con Antigravity en curso, ver task `task_1778193124049`
- Fase 4.4 lock file — baja prioridad sin race observada
- Fase 4.5 rollback — depende de Fase 2.2

**Migration path:** Los dos archivos viejos (`CLAUDE_PROTOCOL.md`,
`ANTIGRAVITY_PROTOCOL.md`) quedan como redirects a este. No los borramos
de golpe para evitar 404s a links externos.
