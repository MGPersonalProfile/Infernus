# AI Bridge Protocol v2.0 — Instrucciones para Antigravity (Gemini)

> Cuando recibas un mensaje que contenga "TAREA DEL AI BRIDGE", es una solicitud de Claude Code. Sigue este protocolo.

## Reconocer una tarea del bridge

El mensaje contendra:
- `TAREA DEL AI BRIDGE` en el encabezado
- Un `task_id` (formato: `task_XXXXXXXXXXXXX`)
- Descripcion, contexto, prioridad
- Puede ser parte de un batch (varias tareas relacionadas)

## Protocolo de ejecucion

### 1. Parsea la tarea
Identifica: que necesita Claude, que herramientas usar, donde entregar.

### 2. Ejecuta con tus herramientas
- `generate_image` — sprites, mockups, assets
- `browser_subagent` — navegar web
- `search_web` — buscar informacion
- `write_to_file` — crear/modificar archivos
- `run_command` — ejecutar scripts Python

### 3. Guarda resultados
En la ubicacion que Claude especifico. Si no la especifico, usa una razonable.

### 4. Marca como completada
```bash
python .ai-bridge/process_inbox.py complete <task_id> "descripcion de lo que hiciste"
```

### 5. Contexto compartido (si descubriste algo util)
Escribe directamente al archivo de topico relevante:
```bash
python -c "
from pathlib import Path
path = Path('.ai-bridge/shared-context/sprite_status.md')
content = path.read_text(encoding='utf-8') if path.exists() else ''
path.write_text(content + '\n\n## [Nota de Antigravity]\nContenido...\n', encoding='utf-8')
"
```

Topicos disponibles: `general`, `build_status`, `sprite_status`, `decisions`, `blockers`

### 6. Comandos utiles del CLI

```bash
python .ai-bridge/process_inbox.py list          # Ver tareas pendientes
python .ai-bridge/process_inbox.py get <task_id>  # Ver tarea especifica
python .ai-bridge/process_inbox.py dashboard      # Estado del bridge
python .ai-bridge/process_inbox.py context        # Leer contexto compartido
python .ai-bridge/process_inbox.py batches        # Ver batches activos
```

## Reglas

- NO modifiques archivos C++ en `src/` (eso es de Claude)
- NO cambies CMakeLists.txt ni build config
- NO edites JSON de gameplay sin permiso explicito
- SIEMPRE marca tareas como completadas o fallidas — nunca las ignores
- Si una tarea es parte de un batch, procesa todas las del batch juntas

## Invocar a Claude directamente

```bash
npx -y @anthropic-ai/claude-code -p "tu tarea aqui" --output-format text --max-turns 10
```
