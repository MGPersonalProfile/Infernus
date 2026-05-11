# MCP servers (third-party)

Esta carpeta es donde se instalan los MCP servers externos que el repo usa
pero no incluye en el tree (son clones de GitHub pesados).

## ffmpeg-mcp

Análisis y procesado de audio/video. Registrado en `.mcp.json` como
`ffmpeg-audio`. **Requiere `ffmpeg` en PATH** (`winget install Gyan.FFmpeg`).

Instalación:
```bash
cd tools/mcp_servers
git clone https://github.com/dubnium0/ffmpeg-mcp.git
pip install -r ffmpeg-mcp/requirements.txt
```

Después reinicia Claude Code o ejecuta `/mcp` para que reconecte.

40+ herramientas: `get_media_info`, `convert_audio`, `analyze_loudness`,
`detect_scene_changes`, etc. Útil para validar los SFX generados por
`tools/generate_sfx.py`.

## LDtk inspector (incluido)

Vive en `.ai-bridge/ldtk_inspector_server.py` (no aquí). Para registrarlo
añade a tu `.mcp.json` (personal, no commiteado):

```json
"ldtk-inspector": {
  "command": "python",
  "args": [".ai-bridge/ldtk_inspector_server.py"],
  "description": "Inspector de arenas .ldtk en assets/rooms/"
}
```

Tools: `list_arenas`, `arena_info`, `validate_arena`, `count_entities`,
`list_entity_defs`. Para Anti: úsalo para validar nuevas arenas que crees.

## Nota

`.mcp.json` es per-usuario (gitignored). Tanto Claude como Antigravity
registramos los MCPs en nuestra config local. Los **servers source** sí se
commitean (en `.ai-bridge/` o `tools/`), pero la config de registro no.
