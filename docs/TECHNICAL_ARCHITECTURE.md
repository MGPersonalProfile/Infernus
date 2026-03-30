# ⚙️ INFERNUS — Documento Técnico de Arquitectura

> Stack: **C++17 · Raylib · ECS · JSON Data-Driven · CMake**

---

## 1. Stack Tecnológico

| Componente | Tecnología | Justificación |
|---|---|---|
| **Lenguaje** | C++17 | Rendimiento, control total, estándar de la industria |
| **Framework gráfico** | Raylib 5.x | API limpia para 2D, sprites, audio, input — todo incluido |
| **Build system** | CMake 3.20+ | Cross-platform, estándar en C++ |
| **Datos** | JSON (nlohmann/json) | Enemigos, habilidades, sinergias, configuración — todo data-driven |
| **Control de versiones** | Git | Versionado del proyecto completo |
| **Arte** | Aseprite | Pixel art y animaciones (spritesheets) |
| **Audio** | Raylib (built-in) | Soporte nativo para WAV, OGG, MP3 |

---

## 2. Estructura de Carpetas

```
INFERNUS/
│
├── CMakeLists.txt              # Build system principal
├── README.md                   # Descripción del proyecto
├── .gitignore                  # Exclusiones de Git
│
├── docs/                       # 📄 Documentación
│   ├── GDD_INFERNUS.md         # Game Design Document
│   ├── TECHNICAL_ARCHITECTURE.md  # Este documento
│   └── CHANGELOG.md            # Registro de cambios por versión
│
├── src/                        # 🔧 Código fuente
│   ├── main.cpp                # Entry point — inicialización y game loop
│   │
│   ├── core/                   # ⚡ Núcleo del engine
│   │   ├── Game.h / .cpp       # Clase principal: init, update, render, shutdown
│   │   ├── GameState.h / .cpp  # State machine (Menu, Playing, Paused, BossFight, GameOver)
│   │   ├── ECS.h / .cpp        # Entity Component System — registro, queries, sistemas
│   │   ├── EventBus.h / .cpp   # Sistema de eventos desacoplado (pub/sub)
│   │   └── ResourceManager.h / .cpp  # Carga y caché de texturas, sonidos, datos
│   │
│   ├── components/             # 📦 Componentes ECS (datos puros)
│   │   ├── Transform.h         # Posición, rotación, escala
│   │   ├── Sprite.h            # Textura, frame actual, layer de renderizado
│   │   ├── Animation.h         # Spritesheet, frames, velocidad, estado
│   │   ├── Health.h            # HP actual, HP máximo, invulnerabilidad temporal
│   │   ├── Stamina.h           # Stamina actual, regeneración, costes
│   │   ├── Combat.h            # Daño base, tipo de daño, knockback
│   │   ├── Velocity.h          # Velocidad, dirección, fricción
│   │   ├── Collider.h          # Hitbox, hurtbox, tipo de colisión
│   │   ├── AIBehavior.h        # Estado de IA, patrón actual, aggro range
│   │   ├── AbilityHolder.h     # Lista de habilidades activas/pasivas equipadas
│   │   ├── StatusEffect.h      # Buffs/debuffs activos (fuego, veneno, hielo, etc.)
│   │   ├── Loot.h              # Drops al morir
│   │   └── BossPhase.h         # Fase actual, umbrales de HP, patrones por fase
│   │
│   ├── systems/                # 🔄 Sistemas ECS (lógica)
│   │   ├── MovementSystem.h / .cpp     # Aplica velocidad, gravedad, fricción
│   │   ├── RenderSystem.h / .cpp       # Dibuja sprites, ordenados por layer
│   │   ├── AnimationSystem.h / .cpp    # Avanza frames de animación
│   │   ├── CombatSystem.h / .cpp       # Resuelve ataques, daño, parry, knockback
│   │   ├── CollisionSystem.h / .cpp    # Detección de colisiones AABB
│   │   ├── HealthSystem.h / .cpp       # Aplica daño, curación, muerte, i-frames
│   │   ├── StaminaSystem.h / .cpp      # Regeneración y consumo de stamina
│   │   ├── AISystem.h / .cpp           # Comportamiento de enemigos (state machine)
│   │   ├── AbilitySystem.h / .cpp      # Ejecución de habilidades, cooldowns, sinergias
│   │   ├── StatusEffectSystem.h / .cpp # Tick de efectos (burn, poison, freeze, etc.)
│   │   ├── ParticleSystem.h / .cpp     # Partículas: fuego, sangre, ceniza, chispas
│   │   ├── CameraSystem.h / .cpp       # Seguimiento, screenshake, zoom
│   │   └── UISystem.h / .cpp           # Renderizado de HUD y menús in-game
│   │
│   ├── entities/               # 🎭 Factories de entidades
│   │   ├── PlayerFactory.h / .cpp      # Crea la entidad del jugador con sus componentes
│   │   ├── EnemyFactory.h / .cpp       # Crea enemigos a partir de datos JSON
│   │   ├── BossFactory.h / .cpp        # Crea bosses con fases y patrones
│   │   ├── ProjectileFactory.h / .cpp  # Proyectiles, AOE, trampas
│   │   └── PickupFactory.h / .cpp      # Items, corazones, orbes de habilidad
│   │
│   ├── abilities/              # ✨ Sistema de habilidades
│   │   ├── Ability.h / .cpp            # Clase base de habilidad (activa/pasiva)
│   │   ├── AbilityPool.h / .cpp        # Pool de habilidades disponibles por run
│   │   ├── SynergyEngine.h / .cpp      # Motor de sinergias (detecta combos de temas)
│   │   └── AbilitySelection.h / .cpp   # UI y lógica de "elige 1 de 3" + reroll
│   │
│   ├── generation/             # 🗺️ Generación procedural
│   │   ├── CircleGenerator.h / .cpp    # Genera un círculo completo (conjunto de salas)
│   │   ├── RoomTemplates.h / .cpp      # Templates de salas prefabricadas
│   │   ├── RoomPopulator.h / .cpp      # Coloca enemigos, trampas, items en salas
│   │   └── MapGraph.h / .cpp           # Grafo de conexiones entre salas
│   │
│   ├── progression/            # 📈 Meta-progresión
│   │   ├── SaveManager.h / .cpp        # Guardar/cargar datos persistentes
│   │   ├── UnlockManager.h / .cpp      # Habilidades, personajes y skins desbloqueados
│   │   ├── RunStats.h / .cpp           # Estadísticas de la run actual
│   │   └── LoreJournal.h / .cpp        # Fragmentos de lore desbloqueados
│   │
│   ├── ui/                     # 🖼️ Interfaz de usuario
│   │   ├── HUD.h / .cpp               # Barra de vida, stamina, habilidades activas
│   │   ├── MainMenu.h / .cpp          # Menú principal
│   │   ├── PauseMenu.h / .cpp         # Menú de pausa
│   │   ├── AbilitySelectUI.h / .cpp   # Pantalla de selección de habilidades (1 de 3)
│   │   ├── DeathScreen.h / .cpp       # Game Over + estadísticas de la run
│   │   └── CharacterSelectUI.h / .cpp # Selección de personaje
│   │
│   ├── audio/                  # 🔊 Audio
│   │   ├── AudioManager.h / .cpp      # Reproducción de música y SFX
│   │   └── MusicTransition.h / .cpp   # Transiciones suaves entre tracks
│   │
│   ├── input/                  # 🎮 Input
│   │   ├── InputManager.h / .cpp      # Abstracción: teclado + gamepad
│   │   └── InputMapping.h / .cpp      # Rebindeo de teclas
│   │
│   └── utils/                  # 🧰 Utilidades
│       ├── Math.h              # Vectores, lerp, random, etc.
│       ├── Timer.h / .cpp      # Timers reutilizables
│       ├── Logger.h / .cpp     # Sistema de logging (debug)
│       └── Constants.h         # Constantes globales (resolución, FPS, etc.)
│
├── assets/                     # 🎨 Recursos del juego
│   ├── sprites/                # Pixel art
│   │   ├── player/             # Spritesheet del jugador (idle, run, attack, dash, death)
│   │   ├── enemies/            # Spritesheets por tipo de enemigo
│   │   │   ├── circle_9/      # Enemigos del Círculo IX (Traición)
│   │   │   ├── circle_8/      # Enemigos del Círculo VIII (Fraude)
│   │   │   ├── circle_7/      # ...y así sucesivamente
│   │   │   └── .../
│   │   ├── bosses/             # Sprites de bosses (grandes, multifase)
│   │   ├── tilesets/           # Tiles por círculo (suelo, paredes, decoraciones)
│   │   │   ├── circle_9/
│   │   │   ├── circle_8/
│   │   │   └── .../
│   │   ├── effects/            # Partículas, explosiones, impactos, fuego, hielo
│   │   ├── ui/                 # Iconos, marcos, barras, botones
│   │   └── backgrounds/        # Fondos parallax por círculo
│   │
│   ├── audio/
│   │   ├── music/              # Tracks por círculo + boss themes
│   │   │   ├── circle_9.ogg
│   │   │   ├── boss_cerberus.ogg
│   │   │   └── .../
│   │   └── sfx/                # Efectos de sonido
│   │       ├── combat/         # Golpes, esquivas, parry
│   │       ├── abilities/      # Sonidos de habilidades
│   │       ├── ui/             # Clicks, selección, confirmación
│   │       └── ambient/        # Fuego, viento, gritos lejanos
│   │
│   ├── fonts/                  # Fuentes (pixel fonts temáticas)
│   │
│   └── data/                   # 📊 Datos en JSON (data-driven)
│       ├── enemies/            # Definición de cada tipo de enemigo
│       │   ├── circle_9_enemies.json
│       │   ├── circle_8_enemies.json
│       │   └── .../
│       ├── bosses/             # Definición de bosses (fases, patrones, stats)
│       │   ├── lucifer.json
│       │   ├── geryon.json
│       │   ├── minotaur.json
│       │   └── .../
│       ├── abilities/          # Definición de habilidades
│       │   ├── active_abilities.json
│       │   ├── passive_abilities.json
│       │   └── synergies.json
│       ├── rooms/              # Templates de salas por círculo
│       │   ├── circle_9_rooms.json
│       │   └── .../
│       ├── characters.json     # Personajes jugables y sus stats
│       ├── config.json         # Configuración del juego (resolución, volumen, etc.)
│       └── lore.json           # Fragmentos de lore desbloqueables
│
├── lib/                        # 📚 Dependencias externas
│   ├── raylib/                 # Raylib (incluido como submodule o fuente)
│   └── nlohmann/               # nlohmann/json (header-only)
│
├── tools/                      # 🔨 Herramientas de desarrollo
│   ├── build.bat               # Script de compilación rápida (Windows)
│   ├── run.bat                 # Compilar + ejecutar
│   └── debug_overlay.h        # Overlay de debug (FPS, hitboxes, info de entidades)
│
└── tests/                      # 🧪 Tests
    ├── test_ecs.cpp            # Tests del sistema ECS
    ├── test_abilities.cpp      # Tests de habilidades y sinergias
    ├── test_generation.cpp     # Tests de generación procedural
    └── test_combat.cpp         # Tests del sistema de combate
```

---

## 3. Arquitectura — Entity Component System (ECS)

### ¿Por qué ECS?

En INFERNUS, las **sinergias** son el corazón del gameplay. ECS permite implementarlas de forma elegante:

```
EJEMPLO: Jugador obtiene "Sangre Ardiente" (sinergia Fuego + Sangre)

ANTES:
  Entity: Player
  Components: [Transform, Sprite, Health, Combat(type: physical)]

DESPUÉS:
  Entity: Player
  Components: [Transform, Sprite, Health, Combat(type: physical),
               StatusEffect(fire_damage: 5), Lifesteal(percent: 10)]

→ El CombatSystem detecta StatusEffect y aplica daño de fuego adicional.
→ El HealthSystem detecta Lifesteal y cura al atacar.
→ ¡Sin cambiar ni una línea de código de combate!
```

### Diagrama de Flujo ECS

```
┌──────────────┐     ┌──────────────────┐     ┌──────────────┐
│   ENTITIES   │────▶│   COMPONENTS     │◀────│   SYSTEMS    │
│              │     │                  │     │              │
│ Player (ID:0)│     │ Transform        │     │ Movement     │
│ Skeleton(ID:1│     │ Sprite           │     │ Combat       │
│ Fireball(ID:2│     │ Health           │     │ Collision    │
│ ...          │     │ Combat           │     │ Render       │
│              │     │ StatusEffect     │     │ AI           │
│              │     │ ...              │     │ ...          │
└──────────────┘     └──────────────────┘     └──────────────┘
```

---

## 4. Game Loop

```
main()
  └── Game::Run()
        ├── Init()           → Raylib, ResourceManager, carga de datos JSON
        │
        └── while (!shouldClose)
              ├── Input()    → InputManager recoge teclado/gamepad
              ├── Update()   → Sistemas ECS procesan en orden:
              │     ├── InputSystem        (aplica input a entidades)
              │     ├── AISystem           (IA de enemigos)
              │     ├── AbilitySystem      (cooldowns, ejecución)
              │     ├── MovementSystem     (posiciones)
              │     ├── CollisionSystem    (detección)
              │     ├── CombatSystem       (resolución de daño)
              │     ├── HealthSystem       (muerte, i-frames)
              │     ├── StatusEffectSystem (ticks de efectos)
              │     ├── StaminaSystem      (regeneración)
              │     ├── AnimationSystem    (frames)
              │     ├── ParticleSystem     (partículas)
              │     ├── CameraSystem       (seguimiento, shake)
              │     └── UISystem           (HUD)
              │
              └── Render()   → RenderSystem dibuja todo por layers:
                    ├── Background (parallax)
                    ├── Tiles
                    ├── Entities (sorted by Y)
                    ├── Effects / Particles
                    └── UI / HUD (overlay)
```

---

## 5. Sistema de Estados del Juego

```
                    ┌─────────────┐
                    │  MAIN MENU  │
                    └──────┬──────┘
                           │ Start
                    ┌──────▼──────┐
                    │  CHARACTER  │
                    │   SELECT    │
                    └──────┬──────┘
                           │ Confirm
               ┌───────────▼───────────┐
               │       PLAYING         │◀──────────────┐
               │  (gameloop activo)    │               │
               └───┬──────┬───────┬───┘               │
                   │      │       │                    │
              Pause│  Boss│   Die │              Retry │
                   │      │       │                    │
            ┌──────▼──┐ ┌─▼────┐ ┌▼─────────┐         │
            │  PAUSE  │ │ BOSS │ │DEATH      │         │
            │  MENU   │ │INTRO │ │SCREEN     ├─────────┘
            └─────────┘ └──────┘ │(stats,    │
                                 │ unlocks)  │
                                 └───────────┘
```

---

## 6. Data-Driven Design — Archivos JSON

### Ejemplo: Definición de Enemigo

```json
{
  "id": "skeleton_warrior",
  "name": "Guerrero Esqueleto",
  "circle": 7,
  "health": 45,
  "damage": 12,
  "speed": 80,
  "sprite": "sprites/enemies/circle_7/skeleton_warrior.png",
  "animation": {
    "idle": { "frames": 4, "speed": 0.15 },
    "walk": { "frames": 6, "speed": 0.10 },
    "attack": { "frames": 5, "speed": 0.08 },
    "death": { "frames": 4, "speed": 0.12 }
  },
  "ai_behavior": "melee_chase",
  "aggro_range": 150,
  "attack_range": 30,
  "drops": [
    { "type": "health_orb", "chance": 0.3 },
    { "type": "ability_orb", "chance": 0.1 }
  ]
}
```

### Ejemplo: Definición de Habilidad

```json
{
  "id": "lance_of_phlegethon",
  "name": "Lanza de Flegetonte",
  "type": "active",
  "theme": "fire",
  "rarity": "rare",
  "description": "Lanza un proyectil ardiente que atraviesa enemigos.",
  "cooldown": 3.0,
  "resource_cost": 25,
  "effects": [
    { "type": "projectile", "damage": 30, "speed": 400, "pierce": true },
    { "type": "burn", "dps": 5, "duration": 3.0 }
  ],
  "synergies": {
    "blood": {
      "id": "burning_blood",
      "name": "Sangre Ardiente",
      "effect": "Los ataques cuerpo a cuerpo dejan charcos de lava durante 2s"
    }
  }
}
```

### Ejemplo: Definición de Boss

```json
{
  "id": "cerberus",
  "name": "Cerbero",
  "circle": 3,
  "phases": [
    {
      "hp_threshold": 1.0,
      "patterns": ["single_bite", "acid_rain_slow"],
      "speed": 60,
      "description": "Una cabeza ataca a la vez. Lluvia ácida lenta."
    },
    {
      "hp_threshold": 0.6,
      "patterns": ["double_bite", "toxic_vomit", "acid_rain_medium"],
      "speed": 80,
      "description": "Dos cabezas atacan. Vómito tóxico crea charcos."
    },
    {
      "hp_threshold": 0.3,
      "patterns": ["triple_bite", "devastation_charge", "acid_rain_heavy"],
      "speed": 100,
      "description": "Las tres cabezas. Carga devastadora. Lluvia intensa."
    }
  ],
  "health": 800,
  "sprite": "sprites/bosses/cerberus.png",
  "music": "audio/music/boss_cerberus.ogg"
}
```

---

## 7. Dependencias Externas

| Librería | Versión | Tipo | Uso |
|---|---|---|---|
| **Raylib** | 5.5+ | Compilada | Rendering, audio, input, ventana |
| **nlohmann/json** | 3.11+ | Header-only | Parsing de archivos JSON |
| **stb_image** | (incluida en Raylib) | Header-only | Carga de imágenes |

> **Nota:** Se intenta minimizar las dependencias externas. Raylib ya cubre gráficos, audio e input.

---

## 8. Convenciones de Código

| Aspecto | Convención |
|---|---|
| **Naming** | `PascalCase` para clases/structs, `camelCase` para funciones/variables, `UPPER_SNAKE` para constantes |
| **Archivos** | `PascalCase.h / .cpp` para clases, `snake_case.json` para datos |
| **Comentarios** | Doxygen para headers públicos, `//` inline para lógica compleja |
| **Includes** | Agrupados: STL → Externas → Proyecto. Headers con `#pragma once` |
| **Smart pointers** | `std::unique_ptr` por defecto, `std::shared_ptr` solo si es necesario |
| **Const correctness** | Siempre que sea posible |

---

## 9. Fases de Desarrollo

| Fase | Contenido | Objetivo |
|---|---|---|
| **Fase 0** | Setup del proyecto: CMake, Raylib compilando, ventana abierta | Infraestructura funcional |
| **Fase 1** | ECS básico + jugador con movimiento y animación + 1 sala | Movimiento y renderizado |
| **Fase 2** | Sistema de combate (ataques, esquiva, stamina, hitboxes) | Combate funcional |
| **Fase 3** | IA de enemigos básica + 2-3 tipos de enemigos | Enemigos jugables |
| **Fase 4** | 1 boss completo (Gerión o Minotauro) con fases | Boss fight funcional |
| **Fase 5** | Sistema de habilidades (pool, selección 1 de 3, reroll) | Loop roguelike |
| **Fase 6** | Sinergias temáticas (2-3 temas con combos) | Sinergias jugables |
| **Fase 7** | Generación procedural de salas + layout de círculo | Niveles procedurales |
| **Fase 8** | UI completa (HUD, menús, death screen, selección) | UI jugable |
| **Fase 9** | Meta-progresión (save, unlocks, personajes) | Persistencia |
| **Fase 10** | Audio, música, partículas, polish | Experiencia completa |

---

## 10. Build y Ejecución

### Windows (CMD / PowerShell)

```bash
# Configurar
cmake -S . -B build

# Compilar
cmake --build build --config Release

# Ejecutar
./build/Release/INFERNUS.exe
```

### Script Rápido (`tools/build_and_run.bat`)

```batch
@echo off
cmake -S . -B build
cmake --build build --config Release
if %errorlevel% == 0 (
    echo [OK] Build exitoso. Ejecutando...
    .\build\Release\INFERNUS.exe
) else (
    echo [ERROR] Falló la compilación.
    pause
)
```

---

**Estado del documento:** 📝 Arquitectura Técnica — v0.1
**Última actualización:** 28 de Febrero, 2026
