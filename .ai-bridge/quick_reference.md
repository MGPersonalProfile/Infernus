# AI Bridge — Quick Reference para Antigravity

## Qué es
Sistema de comunicación bidireccional con Claude Code en el proyecto INFERNUS.
Ubicación: `c:/Users/Juan Miguel/Roguesouls-like/.ai-bridge/`

## Cuando recibas una tarea del bridge
El mensaje contiene "TAREA DEL AI BRIDGE" + task_id. Procesa así:
1. Ejecuta la tarea con tus herramientas
2. Marca completada: `python .ai-bridge/process_inbox.py complete <task_id> "resultado"`

## Cuando necesites a Claude
```bash
npx -y @anthropic-ai/claude-code -p "tarea" --output-format text --max-turns 10
```

## Archivos clave
- `.ai-bridge/antigravity_mcp_server.py` — servidor MCP (6 tools)
- `.ai-bridge/process_inbox.py` — helper para buzón
- `.ai-bridge/docs/COMPLETE_GUIDE.md` — guía exhaustiva (30KB)
- `.ai-bridge/docs/ANTIGRAVITY_PROTOCOL.md` — tu protocolo detallado
- `~/.vscode/extensions/ai-bridge-watcher/` — extensión VS Code

## Territorios
- TÚ: imágenes, sprites, browser, web search, análisis visual, shaders
- CLAUDE: C++, CMake, JSON gameplay, refactoring, bugs, ECS
