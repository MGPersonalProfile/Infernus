# INFERNUS — Instructions for Claude

> Lee `docs/GDD_INFERNUS.md` antes de tomar decisiones de gameplay.
> Lee `docs/ARCHITECTURE.md` antes de tomar decisiones de código.

---

## Estado del proyecto

**Reescritura en curso.** Versión C++/Raylib descartada (commit `418e88c
Wipe: clean slate for Godot rewrite`) porque divergía del pilar
side-scroller del GDD. Estamos reconstruyendo en Godot 4 + GDScript.

## Stack

- **Godot 4.6 stable** (Forward+ renderer)
- **GDScript** como lenguaje principal
- **Pixel art 2D** con nearest filter + snap a píxel
- El editor binario vive en `godot/` (gitignored)

## El norte (no se discute)

- **Side-scroller**, no top-down. Pilar 1 del GDD. La última divergencia
  costó 14k LOC.
- **Combate rápido pero NO frenético**. Algo intermedio entre Skul y
  Dead Cells, con tono del GDD (Blasphemous + Doré). Único.
- **Modular**. Sin God classes. Sin scripts > 300 líneas. Ver
  `docs/ARCHITECTURE.md` para reglas duras.

## Reglas para Claude (lecciones del proyecto anterior)

1. **El director es el director.** Si el GDD dice X y la implementación
   está saliendo Y, PARO y aviso. No racionalizo el desvío.
2. **"Más fácil de implementar" NO es razón para divergir del GDD.**
3. **Cualquier sistema con componente espacial/visual/gameplay**: verificar
   contra GDD antes de implementar. No presumir.
4. **Trimestralmente, auditar**: ¿lo que hay en pantalla coincide con el
   GDD? Si no, deuda creativa que se paga peor cuanto más tarde.

## Skills disponibles (instaladas en `.claude/skills/`)

Estas son skills que puedo invocar via el Skill tool cuando aplique:

| Skill | Para qué |
|---|---|
| `godot` | Workflow de Godot 4.x: testing con GdUnit4, exports web/desktop, deploy Vercel/itch.io. Lee la skill cuando vayas a testear, exportar o deployar. |
| `frontend-design` | Principios de diseño UI: bold direction, no "AI slop", aesthetic intentionality. Aplica a HUD/menús del juego pese a ser web-focused. |
| `mcp-builder` | Construir MCP servers custom. Si necesito uno (LDtk-style inspector, etc), seguir el patrón aquí. |
| `skill-creator` | Crear skills nuevas o tunear estas. |
| `algorithmic-art` | Procedural art / shaders / generative patterns. Útil para fondos, particles, fx. |
| `webapp-testing` | Puppeteer-based testing. Poco aplicable a Godot directo; útil si el juego se exporta web y hay que QA. |

Referencia externa clonada en `.claude/external/`:
- `anthropics/skills` — colección oficial de skills (más de las 5 copiadas)
- `anthropics/claude-code` — plugins oficiales (code-review, feature-dev, etc.)
- `randroids-dojo/godot-claude-skills` — fuente de la skill `godot`

## Estructura del repo

```
project.godot                    # Godot 4 project config
scenes/                          # .tscn (placeholder main.tscn)
src/
  engine/                        # Reusable, agnóstica al juego
  game/                          # Lógica concreta del juego
content/                         # Data-driven (enemies, abilities, etc)
assets/icons/icon.svg            # Placeholder, IX en círculos rojos
docs/
  GDD_INFERNUS.md                # La biblia
  ARCHITECTURE.md                # Reglas técnicas
references/                      # Director sube refs aquí
godot/                           # Binario Godot (gitignored)
.claude/
  skills/                        # Skills activas (gitignored)
  external/                      # Repos clonados como referencia (gitignored)
```

## Comandos útiles

```bash
# Abrir el proyecto en Godot (el director hace esto, no Claude)
godot/Godot_v4.6.3-stable_win64.exe --path .

# Headless run para tests (cuando tengamos)
godot/Godot_v4.6.3-stable_win64.exe --headless --path . --quit
```

## Convenciones de commit

- Mensaje en imperativo, primera línea < 70 chars
- Cuerpo explicando el "por qué", no solo el "qué"
- Co-Author trailer para que el git log refleje el dual-agent setup
- NO `--no-verify`, NO `--amend` de commits ya pushed
- Antes de push: confirmar con el director (excepto fixes triviales tipo typo)
