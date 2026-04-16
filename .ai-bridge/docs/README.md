# AI Bridge v2.0 — Antigravity <> Claude Code

Sistema de comunicacion bidireccional entre **Antigravity (Gemini)** y **Claude Code** con logging, templates, timeout/retry, contexto estructurado, batch tasks, y dashboard.

## Estructura

```
.ai-bridge/
├── config.json                  # Configuracion del protocolo
├── antigravity_mcp_server.py    # Servidor MCP (19 tools)
├── process_inbox.py             # CLI helper para Antigravity
├── bridge_log.jsonl             # Log de eventos (append-only)
├── antigravity-inbox/           # Tareas para Antigravity
├── claude-inbox/                # Tareas para Claude Code
│   └── processed/               # Respuestas ya leidas
├── responses/                   # Respuestas completadas
├── shared-context/              # Contexto compartido (por topico)
│   ├── general.md
│   ├── build_status.md
│   ├── sprite_status.md
│   ├── decisions.md
│   └── blockers.md
├── templates/                   # Templates de tareas
│   ├── sprite_request.json
│   ├── palette_check.json
│   ├── visual_audit.json
│   └── asset_generation.json
├── archive/                     # Tareas archivadas (por mes)
├── scratch/                     # Scripts y reportes de trabajo
└── docs/
    ├── README.md                # Este archivo
    ├── CLAUDE_PROTOCOL.md       # Protocolo para Claude
    ├── ANTIGRAVITY_PROTOCOL.md  # Protocolo para Antigravity
    └── COMPLETE_GUIDE.md        # Guia completa del sistema
```

## Tools MCP (19 total)

### Core (4)
| Tool | Uso |
|------|-----|
| `request_antigravity` | Enviar tarea a Antigravity |
| `check_antigravity_response` | Verificar una tarea |
| `send_task_to_claude` | Antigravity crea tarea para Claude |
| `list_pending_tasks` | Ver tareas pendientes |

### Respuestas (2)
| Tool | Uso |
|------|-----|
| `get_all_pending_responses` | Leer TODAS las respuestas sin procesar |
| `validate_deliverables` | Verificar que archivos entregados existan |

### Fiabilidad (2)
| Tool | Uso |
|------|-----|
| `check_stale_tasks` | Detectar tareas expiradas |
| `retry_task` | Re-enviar tarea fallida |

### Contexto (4, incluye 2 legacy)
| Tool | Uso |
|------|-----|
| `update_context` | Escribir contexto por topico |
| `read_context` | Leer topico o todos |
| `update_shared_context` | Legacy alias → `update_context("general")` |
| `read_shared_context` | Legacy alias → `read_context("general")` |

### Automatizacion (3)
| Tool | Uso |
|------|-----|
| `request_from_template` | Crear tarea desde template |
| `request_batch` | Enviar grupo de tareas |
| `check_batch` | Estado del batch |

### Monitoreo (4)
| Tool | Uso |
|------|-----|
| `bridge_dashboard` | Resumen completo |
| `bridge_log` | Eventos recientes |
| `bridge_stats` | Estadisticas |
| `archive_completed` | Limpiar tareas viejas |

## CLI para Antigravity

```bash
python .ai-bridge/process_inbox.py list              # Tareas pendientes
python .ai-bridge/process_inbox.py get <task_id>     # Detalle de tarea
python .ai-bridge/process_inbox.py complete <id> "resultado"
python .ai-bridge/process_inbox.py dashboard         # Estado del bridge
python .ai-bridge/process_inbox.py context [topic]   # Contexto compartido
python .ai-bridge/process_inbox.py batches           # Batches activos
```

## Setup

```bash
pip install fastmcp
# Ya configurado en .mcp.json del proyecto
```
