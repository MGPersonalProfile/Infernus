# AI Bridge Protocol v2.0 — Instrucciones para Claude Code

> **OBLIGATORIO.** Cuando necesites capacidades visuales/web, usa este protocolo.

## Tu companero: Antigravity (Gemini)

| Capacidad | Delega |
|-----------|--------|
| Imagenes, sprites, pixel art | Si |
| Navegacion web, formularios | Si |
| Busqueda web | Si |
| Analisis visual, screenshots | Si |
| Diseno CSS/HTML, UI visual | Si |
| C++, CMake, JSON, bugs, refactoring | **NO** |

## Herramientas MCP (17 tools)

### Core
| Tool | Uso |
|------|-----|
| `request_antigravity(task, context, priority, timeout_hours)` | Enviar tarea a Antigravity |
| `check_antigravity_response(task_id)` | Verificar una tarea |
| `send_task_to_claude(task, context, priority)` | Antigravity crea tarea para Claude |
| `list_pending_tasks(direction)` | Ver tareas pendientes |

### Respuestas
| Tool | Uso |
|------|-----|
| `get_all_pending_responses()` | Leer TODAS las respuestas de golpe (usa al inicio de sesion) |
| `validate_deliverables(task_id)` | Verificar que archivos entregados existan en disco |

### Fiabilidad
| Tool | Uso |
|------|-----|
| `check_stale_tasks(timeout_hours)` | Detectar tareas expiradas |
| `retry_task(task_id, updated_context)` | Re-enviar tarea fallida/expirada |

### Contexto compartido (por topico)
| Tool | Uso |
|------|-----|
| `update_context(topic, content, append)` | Escribir contexto por topico |
| `read_context(topic)` | Leer topico especifico o 'all' |

Topicos validos: `general`, `build_status`, `sprite_status`, `decisions`, `blockers`

### Automatizacion
| Tool | Uso |
|------|-----|
| `request_from_template(template_name, params)` | Crear tarea desde template |
| `request_batch(tasks_json, batch_name)` | Enviar grupo de tareas |
| `check_batch(batch_id)` | Estado de un batch |

Templates disponibles: `sprite_request`, `palette_check`, `visual_audit`, `asset_generation`

### Monitoreo
| Tool | Uso |
|------|-----|
| `bridge_dashboard()` | Resumen completo del bridge |
| `bridge_log(last_n)` | Ultimos N eventos del log |
| `bridge_stats()` | Estadisticas de uso |
| `archive_completed(older_than_hours)` | Archivar tareas viejas |

## Flujo de trabajo

### Al iniciar sesion
```
1. bridge_dashboard()           — ver estado general
2. get_all_pending_responses()  — recoger respuestas de Antigravity
3. read_context("all")          — leer contexto compartido
```

### Al enviar tarea
```
1. request_antigravity(task, context, priority)  — o request_from_template()
2. Informa al usuario: "Tarea enviada a Antigravity via bridge"
3. NO bloquees — continua trabajando
4. Mas tarde: check_antigravity_response(task_id)
5. Si llego: validate_deliverables(task_id) para verificar archivos
```

### Al terminar sesion
```
1. update_context("general", "resumen de lo hecho", append=True)
2. update_context("build_status", "estado actual del build")
3. archive_completed()  — limpiar tareas viejas
```

## Reglas
- El bridge es AUTOMATICO — Antigravity recibe tareas sin intervencion del usuario
- NUNCA digas "dile a Antigravity que revise el buzon"
- Se ESPECIFICO en las tareas: dimensiones, formato, paleta, path de salida
- Usa templates cuando aplique — son mas consistentes
- Usa batch para tareas relacionadas
- Actualiza contexto compartido cuando hagas descubrimientos importantes
- Verifica entregables con validate_deliverables() antes de integrar

## Ejemplo: solicitar sprite con template

```
request_from_template("sprite_request", '{"entity": "Harpia Enemiga", "dimensions": "32x40", "frames": "idle(4), run(6), attack(4)", "style": "pixel art Dark Souls, paleta Circulo VII", "output_path": "assets/sprites/enemies/harpy_idle.png", "extra_context": "Enemigo volador, alas extendidas en idle"}')
```

## Ejemplo: batch de assets

```
request_batch('[{"task": "Generar idle sprite para miniboss infernal_knight, 64x64, 4 frames", "priority": "high"}, {"task": "Generar charge sprite para miniboss infernal_knight, 64x64, 3 frames", "priority": "high"}]', batch_name="infernal_knight_sprites")
```
