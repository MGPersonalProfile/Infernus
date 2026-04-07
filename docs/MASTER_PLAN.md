# INFERNUS — Plan Maestro

> Estado actual del proyecto, arquitectura, y hoja de ruta completa.
> Ultima actualizacion: 7 de Abril, 2026

---

## Estado Global

| Dato | Valor |
|---|---|
| **Fase completada** | Fase 3 (Enemigos) — funcional |
| **Lo que existe jugable** | Circulo VII: 3 clases, 5 tipos de enemigos, 1 boss (Minotauro 3 fases), 3 minibosses, 24 habilidades, 29 items, 6 sinergias, generacion procedural, meta-progresion |
| **Proximo objetivo** | Bloque A: estabilizacion (colisiones, balance, fixes) |
| **Compila** | Si (GCC 15.2, MinGW, CMake, Raylib 5.5) |

---

## Arquitectura Tecnica

### Stack
C++17 · Raylib 5.x · ECS custom · JSON data-driven (nlohmann/json) · CMake 3.20+

### Estructura de Carpetas
```
INFERNUS/
├── CMakeLists.txt
├── CLAUDE.md                   # Instrucciones para el asistente IA
├── docs/
│   ├── GDD_INFERNUS.md         # Game Design Document (la biblia creativa)
│   └── MASTER_PLAN.md          # Este documento
├── src/
│   ├── core/                   # Game, ECS, GameState, ResourceManager
│   ├── components/             # 24 componentes ECS (datos puros)
│   ├── systems/                # 17 sistemas (logica)
│   ├── entities/               # Factories: Player, Enemy, Boss, MiniBoss, Projectile
│   ├── generation/             # Room.h (tiles, templates)
│   ├── world/                  # RoomGenerator (procedural)
│   ├── audio/                  # AudioManager
│   ├── input/                  # InputManager
│   ├── ui/                     # UIRenderer
│   ├── meta/                   # SaveManager
│   └── utils/                  # Constants
├── assets/
│   ├── data/                   # JSON: enemies, bosses, minibosses, abilities, items, synergies, lore, characters
│   ├── sprites/                # player, enemies, bosses, tiles, fx
│   ├── art/                    # portraits, parallax, title, ui panel
│   └── audio/                  # music (wav), sfx (wav)
├── tests/                      # test_ecs.cpp, test_combat.cpp
├── tools/                      # asset_generator.cpp, build_and_run.bat
└── lib/                        # json.hpp (nlohmann)
```

### ECS (Entity Component System)
- `Registry`: crea/destruye entidades, add/remove/get componentes, queries multi-componente via `view<T1, T2...>()`
- Componentes son structs puros que heredan de `Component`
- Sistemas procesan entidades que tengan los componentes requeridos
- Orden de update en game loop:
  1. Input → 2. AbilitySystem → 3. AISystem → 4. BossAISystem → 5. MiniBossAISystem
  6. MovementSystem → 7. CollisionSystem → 8. TrapSystem → 9. StaminaSystem
  10. HealthSystem → 11. CombatSystem → 12. ParticleSystem → 13. AnimationSystem

### Data-Driven
Todo dato de gameplay viene de JSON en `assets/data/`. Las factories cargan JSON y crean entidades ECS con fallback a valores por defecto si el archivo falta.

### Convenciones
- `PascalCase` para clases/structs, `camelCase` para funciones/variables, `UPPER_SNAKE` para constantes
- Headers con `#pragma once`, includes agrupados: STL → Externas → Proyecto
- Factories son clases estaticas header-only (patron consistente en todo el proyecto)

---

## Contenido Actual

### Personajes Jugables (3)
| Clase | HP | DMG | SPD | Stamina | Especial |
|---|---|---|---|---|---|
| Guerrero | 120 | 18 | 250 | 100 | Golpe Sismico (AoE, 3s CD) |
| Picaro | 80 | 12 | 320 | 140 | Golpe Sombra (teleport + linea, 2s CD) |
| Caballero | 180 | 14 | 200 | 80 | Escudo Oseo (invuln + espinas, 5s CD) |

### Enemigos (5 tipos, JSON en assets/data/enemies/)
| ID | Nombre | Tipo | HP | DMG |
|---|---|---|---|---|
| melee | Alma Violenta | melee | 50 | 15 |
| ranged | Centauro | ranged | 35 | 12 |
| tank | Bruto del Flegetonte | tank | 180 | 35 |
| assassin | Arpia | melee | 30 | 25 |
| bomber | Suicida Retorcido | ranged | 25 | 30 |

### Boss: El Minotauro (assets/data/bosses/minotaur.json)
- 1200 HP, 40 DMG, 3 fases
- Patrones: charge, ground_slam, stomp, combo, enraged_charge
- Multiplicadores por fase: 1.0 → 1.4 → 1.8

### Minibosses (6 JSONs, Game.cpp usa 3: infernal_knight, soul_archer, pit_fiend)
- Extras del dist: arpia_reina, neso, quiron (tematica Dante del Circulo VII)

### Habilidades: 24 (assets/data/abilities.json)
### Items: 29 (assets/data/items.json) — common a legendary, con efectos y sinergias
### Sinergias: 6 (assets/data/synergies.json) — flame_mastery, iron_fortress, wind_dancer, warriors_path, soul_harvest, endurance
### Lore: 15 entradas (assets/data/lore.json) — Circulo VII, Dante, Flegetonte

---

## Fases Completadas

### Fase 0 — Setup (28 Feb 2026)
- Proyecto compilando, CMake + Raylib, estructura de carpetas, ventana basica

### Fase 1 — ECS + Jugador (28 Feb 2026)
- ECS custom, componentes base, sistemas base, InputManager, PlayerFactory, sala placeholder, ResourceManager

### Fase 2 — Combate (1 Mar 2026)
- Health, Stamina, Combat, CombatSystem (maquina de estados NONE→WINDUP→ACTIVE→RECOVERY)
- Dash con i-frames, knockback, particulas, screen shake, barras de vida flotantes
- Controles: WASD (mover), J (ligero, 20 stam), K (pesado, 40 stam), SPACE (dash, 30 stam)

### Fase 3 — Enemigos (1 Mar 2026)
- AIBehavior con state machine, 3 arquetipos, EnemyFactory data-driven, ProjectileFactory, loot drops

### Post-Fase 3 (Mar 2026, antes del crash del disco)
- Boss Minotauro 3 fases, MiniBossFactory, AbilitySystem, ItemSystem, SynergySystem
- Seleccion de personaje, generacion procedural mejorada, meta-progresion (SaveManager)
- UI: menus, HUD, character select, inventory, item swap, boss intro, ability select
- AudioManager, ScreenEffects, partículas atmosfericas
- 25 runs de playtest (segun save del dist)

### Pase de Calidad UI (7 Abril 2026)
- Build WebAssembly (Emscripten 5.0.5) compilando y corriendo en navegador
- Pixel font PressStart2P integrado via TextUtils (centralizado)
- Todas las pantallas reescritas con tamanos tipograficos coherentes (sin solapamientos)
- Sistema de transiciones de pantalla (fade in/out) en todos los cambios de estado importantes
- Pause menu navegable con flechas + highlight visual (estilo Hades)
- Options menu con 6 entradas: SFX, Musica, Pantalla Completa, Screen Shake toggle, Damage Numbers toggle, Volver. Incluye referencia de controles
- Game Over y Victory con titulos pulsantes y lineas decorativas doradas
- Pipeline de QA visual via Puppeteer + Chrome headless

---

## Plan de Produccion

### BLOQUE A — Estabilizacion (SIGUIENTE)

> Que el Circulo VII funcione correctamente. No se anaden features, solo se arregla y pule.

#### A.1 — Fix de Colisiones
- [ ] CollisionSystem AABB vs tiles solidos: resolver empuje correctamente
- [ ] Resolucion por eje (X e Y separados)
- [ ] Jugador-muro: push-back sin overlapping
- [ ] Enemigos-muro: AI no pathea a traves de paredes
- [ ] Boss-muro: no salirse de la arena
- [ ] Proyectiles se destruyen al impactar muros
- [ ] Test: caminar contra cada pared, pilares, bordes

#### A.2 — Balance del Circulo VII
- [ ] Reducir salas a 3-4 antes del boss
- [ ] Ajustar enemigos por sala: empezar con 2, escalar a 3-4
- [ ] Mas loot drops
- [ ] Ajustar curva de dificultad (sigue siendo souls-like, pero justo)

#### A.3 — Fixes Menores
- [ ] Eliminar `static float introTimer` en BOSS_INTRO
- [ ] Limpiar warnings (unused variables)
- [ ] Verificar transicion ABILITY_SELECT → BOSS_INTRO
- [ ] Verificar que morir y reintentar limpia todo el estado
- [ ] RoomGenerator: salas transitables, no bloquear caminos
- [ ] Crear directorio `save/` automaticamente si no existe

#### A.4 — Playtest
- [ ] 10 runs completas del Circulo VII
- [ ] Documentar que se siente bien/mal/injusto
- [ ] Criterio de exito: run en 5-8 minutos, dificultad desafiante pero justa

---

### BLOQUE B — Core Loop Completo

> Un Circulo perfecto que represente la experiencia final.

#### B.1 — Combate Avanzado
- [ ] Parry (ventana 0.2s, anula dano, stagger 1.5s, 15 stamina)
- [ ] Combos (3 hits ligeros encadenados + finisher pesado)
- [ ] Dano elemental (PHYSICAL, FIRE, ICE, LIGHTNING, TOXIC) con resistencias

#### B.2 — Enemigos Mejorados
- [ ] AI pathfinding basico (no quedarse atascados contra paredes)
- [ ] Nuevos enemigos: Centauro (arco), Harpia (vuela), Alma en Pena (explota al morir)
- [ ] Oleadas y salas de elite (stats x1.5, recompensa garantizada)

#### B.3 — Habilidades Activas
- [ ] Habilidades con boton de activacion (Q, E), cooldown, slot limitado
- [ ] 5 habilidades activas: Lanza de Flegetonte, Escudo de Hielo, Paso Sombrio, Grito de Guerra, Drenar Alma
- [ ] Reroll con vida (10% HP max, escala con cada uso)
- [ ] Expandir pool a 30 habilidades

#### B.4 — Sinergias Expandidas
- [ ] 6 sinergias tematicas tier 1 (fuego, sangre, hielo, peste, sombra, tormenta)
- [ ] 3 sinergias cross-tema
- [ ] UI de sinergias con progreso visible

#### B.5 — Generacion Procedural Mejorada
- [ ] Grafo de salas con bifurcaciones
- [ ] Sala de tienda (gastar HP por habilidad), sala de descanso (30% HP)
- [ ] Minimap simple

#### B.6 — Arte del Circulo VII
- [ ] Pixel art real para jugador, enemigos, boss, tiles, parallax
- [ ] Paleta: rojos (#8B0000), gris oscuro (#2B2B2B), naranja lava (#CC4400)
- [ ] Particulas ambientales: ceniza, gotas de sangre

#### B.7 — Audio del Circulo VII
- [ ] Musica de exploracion + boss
- [ ] 15+ SFX esenciales
- [ ] Ambiente: fuego, gritos lejanos

---

### BLOQUE C — Expansion (8 Circulos restantes)

Cada circulo necesita: bioma (tileset, parallax, paleta), 3 enemigos, 1 boss multifase, musica, 2-3 habilidades, lore.

Orden: IX (Traicion/tutorial) → VIII (Fraude) → VI (Herejia) → V (Ira) → IV (Avaricia) → III (Gula) → II (Lujuria) → I (Limbo/final)

Ver GDD_INFERNUS.md para detalles de cada circulo.

---

### BLOQUE D — Polish y Lanzamiento

- 6 personajes jugables con mecanicas diferenciadas
- 50+ habilidades, 15+ sinergias
- 3 modos de dificultad (Purgatorio, Infierno, Malebolge)
- Contenido opcional: salas secretas, NPC Virgilio, desafios diarios
- UI final con animaciones, fuente pixel, pantalla de coleccion
- Audio completo: 9 tracks exploracion + 9 boss, 50+ SFX
- Localizacion: Espanol + Ingles
- Steam: page, trailer, screenshots, achievements

---

## Registro de Decisiones Tecnicas

| Fecha | Decision | Contexto |
|---|---|---|
| 28/02/2026 | C++17 + Raylib | Balance de control, rendimiento y velocidad de desarrollo |
| 28/02/2026 | ECS custom | Dependencias minimas, adaptado a sinergias |
| 28/02/2026 | Data-driven JSON | Iterar diseno sin recompilar |
| 05/04/2026 | Restauracion desde dist | Crash de disco, JSONs corruptos restaurados del zip de la alfa |
| 05/04/2026 | Unificacion de docs | 5 .md → 2 .md + CLAUDE.md |

---

> *"En el fondo del Infierno, donde nadie mira, una puerta se abrio. Y tu fuiste lo bastante estupido — o lo bastante valiente — para cruzarla."*
