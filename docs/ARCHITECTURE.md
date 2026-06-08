# Arquitectura de INFERNUS

Decisiones técnicas y reglas de organización. Documento corto a propósito —
si esto crece a >300 líneas, algo va mal.

## Stack

- **Godot 4.6 stable** (renderer Forward Plus)
- **GDScript** como lenguaje principal
- **Pixel art 2D** con `snap_2d_transforms_to_pixel=true`
- **Viewport base**: 1280x720, stretch `canvas_items` con aspect `keep`
- Sin .NET. Sin C++ extensions. Si en algún punto necesitamos C# para
  un hot path concreto, lo añadimos como módulo separado — no migramos
  todo el proyecto.

## Por qué Godot y no Raylib (lección aprendida)

El proyecto anterior usaba C++/Raylib porque era lo que ya conocíamos.
Ese sesgo nos llevó a implementar top-down (más fácil técnicamente) en
vez de side-scroller (lo que pedía el GDD desde día 1). Resultado:
14k LOC tirados.

Godot nos fuerza modularidad (node system), nos da editor visual (el
director puede tunear sin tocar código), tiene hot-reload nativo, y es
el stack moderno para indies que apuntan a algo terminable.

**Regla derivada:** si el camino más fácil técnicamente diverge del
GDD, paramos y avisamos al director antes de seguir.

## Tres capas, contratos explícitos

```
┌─────────────────────────────────────────────┐
│ CONTENT                                     │
│ data/ : enemies, abilities, items, levels   │
│ assets/ : sprites, audio, fonts             │
└─────────────────────────────────────────────┘
                ▲ data-driven
                │
┌─────────────────────────────────────────────┐
│ GAME                                        │
│ src/game/ : player, enemies, combat,        │
│             world, progression, ui          │
└─────────────────────────────────────────────┘
                ▲ usa via API
                │
┌─────────────────────────────────────────────┐
│ ENGINE                                      │
│ src/engine/ : autoload, physics,            │
│               resources, util               │
└─────────────────────────────────────────────┘
```

### Reglas duras

1. **Nada en `engine/` conoce a `game/`.** Engine es reusable, agnóstico.
2. **Nada en `game/` conoce el path concreto de un asset.** Va por la API
   de carga (`DataLoader`, `ResourceCache`), no `load("res://assets/...")`
   directo desde lógica de gameplay.
3. **Cualquier script > 300 líneas se parte.** El proyecto anterior tenía
   God classes de 4000+ líneas. No vuelve a pasar.
4. **Un nodo, una responsabilidad.** Si un nodo gestiona combat + UI +
   audio, se separa en 3 nodos hijos con responsabilidades claras.
5. **Inyección por export var, no singletons.** Singletons solo para
   servicios genuinamente globales (AudioBus, EventBus, Logger).

## Convenciones de naming

- Scripts: `snake_case.gd`
- Escenas: `snake_case.tscn`
- Clases (con `class_name`): `PascalCase`
- Recursos `.tres`: `snake_case.tres`
- Señales: `snake_case` con verbo en pasado (`damage_taken`, `room_cleared`)
- Funciones: `snake_case`
- Constantes: `SCREAMING_SNAKE`
- Miembros privados: `_underscore_prefix`

## Pixel art settings

- **Texture filter**: Nearest (cero blur). Configurado en project.godot.
- **Snap a píxel**: activo. No queremos sub-pixel rendering en sprites.
- **Pixel snap también en cámara**: Cuando se implemente la cámara, hay
  que asegurarse de que la posición rebote a entero antes de aplicarse,
  o las texturas vibran.

## Testing

- Pendiente decidir framework. Candidatos: GUT (Godot Unit Test) —
  estándar de facto en Godot — o tests propios en `tests/` con escenas
  de regresión.
- Empezamos sin tests hasta que tengamos código que merezca testarse.
  No tests-on-empty.

## Lo que NO está decidido aún

- Sistema de cámara (look-ahead lateral, deadzone, smoothing)
- Formato de niveles (LDtk vía addon, TileMap nativo, o algo custom)
- Sistema de animación (AnimationPlayer estándar vs AnimationTree)
- Localización (si se hace, cuándo)

Estas se deciden cuando toque, no a priori.
