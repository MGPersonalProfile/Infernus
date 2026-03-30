# 🗺️ INFERNUS — Plan de Acción Maestro

> **⚠️ INSTRUCCIÓN AL ASISTENTE IA:** Este documento es tu hoja de ruta. Debes actualizarlo **cada vez que completes una tarea**, marcando `[x]` las completadas, `[/]` las que estén en progreso, y añadiendo notas de contexto cuando sea relevante. Nunca trabajes sin consultar primero este archivo. Si surge una tarea nueva no prevista, añádela en la sección correspondiente.

---

## Estado Global del Proyecto

| Dato | Valor |
|---|---|
| **Fase actual** | ✅ Fase 3 — Completada |
| **Última actualización** | 1 de Marzo, 2026 |
| **Próxima acción** | Fase 4 — Primer Boss |
| **Bloqueos actuales** | Ninguno |
| **Decisiones pendientes** | Ninguna |

---

## Fase 0 — Setup del Proyecto 🏗️

> **Objetivo:** Tener el proyecto compilando con Raylib, ventana abierta, estructura de carpetas creada.
> **Criterio de éxito:** Ejecutar `INFERNUS.exe` y ver una ventana negra con un título.

- [x] Crear estructura de carpetas completa según `TECHNICAL_ARCHITECTURE.md`
  - [x] `src/` con todas las subcarpetas (core, components, systems, entities, abilities, generation, progression, ui, audio, input, utils)
  - [x] `assets/` con subcarpetas (sprites, audio, fonts, data)
  - [x] `lib/`, `tools/`, `tests/`, `docs/`
- [x] Configurar CMake
  - [x] `CMakeLists.txt` raíz con compilación de Raylib
  - [x] Integrar nlohmann/json (header-only, copiar a `lib/`)
  - [x] Flags de compilación: C++17, warnings activados
- [x] Integrar Raylib
  - [x] Descargar / incluir Raylib como dependencia
  - [x] Verificar compilación limpia
- [x] Crear `main.cpp` con ventana básica
  - [x] Ventana 1280x720 con título "INFERNUS"
  - [x] Fondo negro, FPS target 60
  - [x] Verificar que compila y ejecuta en Windows
- [x] Crear `.gitignore` (excluir build/, *.exe, *.obj)
- [x] Crear `tools/build_and_run.bat`
- [x] Mover documentos existentes a `docs/`
  - [x] `GDD_INFERNUS.md` → `docs/`
  - [x] `TECHNICAL_ARCHITECTURE.md` → `docs/`

**Notas de la Fase 0:**
> ✅ Fase completada el 28 de Febrero 2026.
>
> **Lo implementado según el plan:**
> - Estructura de carpetas completa (`src/`, `assets/`, `lib/`, `tools/`, `tests/`, `docs/`).
> - CMake configurado con Raylib 5.x via FetchContent, C++17, y nlohmann/json en `lib/`.
> - Ventana 1280×720 con fondo negro y título "INFERNUS".
> - Script `tools/build_and_run.bat` para compilar y ejecutar en un solo paso.
> - `.gitignore` configurado para excluir `build/`, `*.exe`, `*.obj`.
>
> **Añadido fuera del plan base:**
> - `compile_commands.json` generado por CMake (`CMAKE_EXPORT_COMPILE_COMMANDS ON`) para soporte de clangd/IntelliSense.
> - Archivo `.clangd` en la raíz con rutas `-isystem` a MSYS2 (`C:/msys64/ucrt64/include/c++/15.2.0/...`) para resolver errores de cabeceras estándar en el IDE.
>
> **Advertencias conocidas:**
> - CMake emite un warning sobre `CMP0135` (DOWNLOAD_EXTRACT_TIMESTAMP) de FetchContent. Es cosmético y no afecta la compilación.
> - Raylib emite un deprecation warning de `cmake_minimum_required < 3.10`. Es de Raylib, no del proyecto.

---

## Fase 1 — ECS + Jugador Básico 🧩

> **Objetivo:** Tener un jugador que se mueve en pantalla con animación, dentro de una sala renderizada.
> **Criterio de éxito:** Mover un sprite con WASD/flechas en una sala con tiles.

- [x] Implementar ECS core
  - [x] `ECS.h / .cpp` — Registro de entidades, asignación de componentes, queries
  - [x] Sistema de IDs de entidades (uint32_t incremental)
  - [x] Método para obtener todas las entidades con ciertos componentes
  - [x] Tests básicos del ECS (`tests/test_ecs.cpp`)
- [x] Implementar componentes base
  - [x] `Transform.h` — Posición (x, y), escala, rotación
  - [x] `Sprite.h` — Textura, source rect, flip, layer
  - [x] `Animation.h` — Spritesheet data, frame actual, timer, loop
  - [x] `Velocity.h` — Velocidad en x/y, fricción
  - [x] `Collider.h` — AABB (x, y, w, h), tipo (solid, trigger)
- [x] Implementar sistemas base
  - [x] `MovementSystem` — Aplica velocidad a posición, respeta colisiones con tiles
  - [x] `RenderSystem` — Dibuja sprites ordenados por layer
  - [x] `AnimationSystem` — Avanza frames, cambia estados de animación
  - [x] `CollisionSystem` — AABB vs AABB, resolución básica con tiles
- [x] Implementar `InputManager`
  - [x] Mapeo de teclas (WASD + flechas)
  - [x] Abstracción lista para gamepad (no implementar aún, solo preparar interfaz)
- [x] Crear `PlayerFactory`
  - [x] Crea entidad con: Transform, Sprite, Animation, Velocity, Collider
  - [x] Placeholder sprite (rectángulo de color hasta tener arte)
- [x] Implementar sala básica
  - [x] Tilemap simple (array 2D)
  - [x] Renderizado de tiles
  - [x] Colisión con bordes/paredes
- [x] Implementar `ResourceManager` básico
  - [x] Carga texturas y las cachea
  - [x] Descarga al cerrar
- [x] Implementar `Game.h / .cpp`
  - [x] Init → Update → Render loop
  - [x] Delta time correcto
  - [x] FPS counter (debug)

**Notas de la Fase 1:**
> ✅ Fase completada el 28 de Febrero 2026.
>
> **Lo implementado según el plan:**
> - ECS custom con `Registry` (entity creation/destruction, component add/remove/get/has, multi-component `view<>()`).
> - Componentes: `Transform2D` (x, y, scale, rotation), `Sprite` (Texture2D, sourceRect, flipX, layer, tint), `Animation` (frames, timer), `Velocity` (vx, vy, friction), `Collider` (AABB rect, isTrigger).
> - Sistemas: `MovementSystem` (aplica velocidad × dt), `RenderSystem` (ordena por layer, DrawTexturePro), `AnimationSystem` (avanza frames), `CollisionSystem` (AABB vs AABB + enforced boundaries).
> - `InputManager` con mapeo abstracto de teclas (WASD + flechas + J/K/SPACE) y preparación de interfaz para gamepad.
> - `PlayerFactory` crea entidad con todos los componentes necesarios.
> - Tilemap placeholder: grid de líneas 64×64 renderizada en world-space.
> - `ResourceManager` singleton para cachear y liberar texturas.
> - `Game.h/.cpp` con loop Init→Update→Render, delta time de Raylib, FPS counter.
>
> **Añadido fuera del plan base:**
> - `Sprite.tint` (Color) para soporte de efectos visuales de combate (flash rojo, semi-transparencia en i-frames). No estaba en la spec original de Fase 1.
> - `Sprite.flipX` se usa como indicador de dirección de facing. No hay un componente `Facing` separado — se reutiliza el flip del sprite.
> - `test_ecs.cpp` en `tests/` para verificación unitaria del ECS.
>
> **Decisiones técnicas:**
> - Entities son `uint32_t` incrementales (no se reciclan IDs por ahora).
> - Componentes se almacenan como `shared_ptr<Component>` en mapas anidados `type_index → entity_id → ptr`. Funciona bien para la escala actual (~100 entidades), pero sería ineficiente con miles.
> - El arte placeholder usa rectángulos de color generados en runtime (`GenImageColor`) en vez de archivos `.png`.

---

## Fase 2 — Sistema de Combate ⚔️

> **Objetivo:** El jugador puede atacar, esquivar, y tiene stamina. Los golpes conectan con hitboxes.
> **Criterio de éxito:** Atacar un dummy que recibe daño, esquivar con i-frames, stamina se gasta y regenera.

- [x] Implementar componentes de combate
  - [x] `Health.h` — HP actual/máximo, invulnerabilidad temporal (i-frames)
  - [x] `Stamina.h` — Stamina actual/máxima, regen rate, cooldown
  - [x] `Combat.h` — Daño base, tipo de daño, knockback, estado de ataque
- [x] Implementar `CombatSystem`
  - [x] Ataque ligero: activar hitbox temporal, consumir stamina
  - [x] Ataque pesado: más lento, más daño, más stamina
  - [x] Hitbox de ataque separada de la hitbox del personaje
  - [x] Detección de hit (hitbox atacante vs hurtbox defensor)
  - [x] Aplicar daño + knockback
  - [x] I-frames tras recibir daño (invulnerabilidad temporal)
- [x] Implementar esquiva/dash
  - [x] Dash en la dirección del movimiento
  - [x] I-frames durante el dash
  - [x] Coste de stamina
  - [x] Cooldown corto
- [x] Implementar `HealthSystem`
  - [x] Recibir daño (con check de i-frames)
  - [x] Curación
  - [x] Muerte (trigger evento)
  - [x] Flash visual al recibir daño (tint rojo)
- [x] Implementar `StaminaSystem`
  - [x] Consumo al atacar/esquivar
  - [x] Regeneración pasiva con delay post-acción
  - [x] No permitir acciones si stamina = 0
- [x] Implementar animaciones de combate
  - [x] Estado idle → attack → recovery → idle (Squash scaling proxy)
  - [x] Estado idle → dash → idle (Color I-Frames proxy)
  - [x] Estado hit → stagger → idle (Hitflash + Squash proxy)
- [x] Dummy de prueba
  - [x] Entidad estática con Health y Collider
  - [x] Feedback visual al recibir daño
  - [x] Muere al llegar a 0 HP
- [x] Implementar `CameraSystem` básico
  - [x] Seguimiento suave del jugador
  - [x] Screen shake al golpear/recibir daño
- [x] Implementar partículas básicas
  - [x] Chispas al golpear
  - [x] Flash de impacto

**Notas de la Fase 2:**
> ✅ Fase completada el 1 de Marzo 2026. Verificada con tests automatizados y prueba visual.
>
> **Lo implementado según el plan:**
> - Componentes: `Health` (maxHP, currentHP, invulnerabilityTimer, hitFlashTimer), `Stamina` (maxStamina, currentStamina, regenRate, regenDelay, cooldownTimer), `Combat` (baseDamage, knockbackForce, attackState, stateTimer, owner).
> - `CombatSystem`: máquina de estados NONE→WINDUP→ACTIVE→RECOVERY→NONE. Spawns de hitbox transitorias como entidades ECS separadas. Detección AABB hitbox-vs-hurtbox. Daño, knockback, i-frames, y flash visual.
> - Esquiva/Dash: consume 30 stamina, otorga 0.3s de i-frames, propulsa al jugador a 1200px/s en la dirección normalizada del input (o dirección de facing si está parado).
> - `HealthSystem`: cuenta regresiva de invulnerability timer y hitFlashTimer. Destruye entidades con HP ≤ 0.
> - `StaminaSystem`: regeneración pasiva con delay post-acción. Bloquea acciones si stamina = 0.
> - Animaciones proxy: squash/stretch según estado de ataque, color semi-transparente en i-frames, flash rojo al recibir daño.
> - Dummy de prueba: rectángulo azul 40×40 con 50 HP, recibe daño y muere.
> - `CameraSystem`: seguimiento suave con lerp, screen shake con decay.
> - Partículas: ráfagas de cuadrados blancos al impactar, con velocidad aleatoria y fade.
>
> **Añadido fuera del plan base:**
> - `Combat.owner` (Entity): campo de owner en el componente Combat para prevenir auto-daño. Los hitboxes llevan referencia al dueño.
> - Hitbox direccional: se posiciona a la izquierda o derecha del dueño según `Sprite.flipX`, no siempre a la derecha.
> - Barra de vida flotante: `RenderSystem` dibuja barras HP arriba de entidades dañadas (verde/rojo) con número de HP.
> - Normalización del dash: el vector de movimiento se normaliza antes de aplicar la velocidad del dash, evitando que el dash diagonal sea √2 más rápido.
> - `Lifetime` component: sistema genérico de auto-destrucción por tiempo, usado por hitboxes y partículas.
> - `Particle` component: datos de interpolación de color y escala para VFX.
> - Tests automatizados: `tests/test_phase_2.cpp` con `testHealthSystem()` y `testStaminaSystem()` que validan daño, muerte, y regeneración.
>
> **Bugs encontrados y corregidos:**
> - `CameraSystem::init()` no se llamaba → zoom=0 → todo invisible. Arreglado añadiendo la llamada en `Game::init()`.
> - Hitbox se auto-dañaba al jugador → Arreglado con campo `owner` y skip en collision detection.
> - Dash multiplicaba velocidades independientes en X/Y → Arreglado con normalización de vector.
> - `test_phase_2.cpp` en `src/` causaba doble `main()` → Movido a `tests/`.
>
> **Controles implementados:**
> | Acción | Tecla | Coste |
> |---|---|---|
> | Moverse | WASD / Flechas | 0 |
> | Ataque ligero | J | 20 stamina |
> | Ataque pesado | K | 40 stamina |
> | Dash/Esquiva | SPACE | 30 stamina |

---

## Fase 3 — Enemigos 👹

> **Objetivo:** 2-3 tipos de enemigos con IA funcional que atacan al jugador.
> **Criterio de éxito:** Entrar en una sala y pelear contra enemigos con comportamientos distintos.

- [x] Implementar componente `AIBehavior`
  - [x] State machine: Idle → Patrol → Chase → Attack → Stagger → Death
  - [x] Aggro range (detectar al jugador)
  - [x] Attack range (rango para atacar)
- [x] Implementar `AISystem`
  - [x] Patrullaje básico (ir y venir)
  - [x] Persecución (pathfinding simple hacia el jugador)
  - [x] Ataque cuando está en rango
  - [x] Stagger al recibir daño
  - [x] Muerte con animación
- [x] Crear `EnemyFactory`
  - [x] Carga stats desde JSON (`assets/data/enemies/`)
  - [x] Crea entidad con componentes apropiados
- [x] Implementar 3 arquetipos de enemigo
  - [x] **Melee básico** (esqueleto/demonio) — persigue y ataca
  - [x] **A distancia** (lanzador de fuego) — mantiene distancia, dispara proyectiles
  - [x] **Tanque** (bruto) — lento, mucha vida, ataques fuertes con telegrafía
- [x] Implementar `ProjectileFactory`
  - [x] Proyectiles de enemigos
  - [x] Velocidad, daño, duración
  - [x] Colisión con jugador y paredes
- [x] Feedback visual de los enemigos
  - [x] Barra de vida sobre el enemigo (opcional — decidir)
  - [x] Flash al recibir daño
  - [x] Partículas de muerte
- [x] Drops al morir
  - [x] Componente `Loot`
  - [x] Spawn de pickups (orbes de vida, orbes de habilidad)

**Notas de la Fase 3:**
> ✅ Fase completada el 1 de Marzo 2026. Build exitoso, ejecución visual verificada.
>
> **Lo implementado según el plan:**
> - Componente `AIBehavior` con state machine de 6 estados (IDLE, PATROL, CHASE, ATTACK, STAGGER, DEATH) y enumeración `EnemyType` (MELEE, RANGED, TANK).
> - `AISystem` completo: patrullaje con cambio de dirección temporizado, detección por rango de aggro, persecución directa, ataque delegado al `CombatSystem`, stagger al recibir daño (trigger desde `HealthSystem`), y death handling.
> - `EnemyFactory` con carga de stats desde JSON (`assets/data/enemies/`). Fallback a valores por defecto si el archivo no existe. Crea entidades con Transform, Sprite, Collider, Health, Combat, Velocity, AIBehavior, y Loot.
> - 3 arquetipos implementados con JSONs independientes:
>   - **Melee (Demonio Menor):** 30 HP, 10 daño, chaseSpeed 140, aggroRange 250
>   - **Ranged (Lanzafuego):** 20 HP, 8 daño, dispara proyectiles a 300px/s, retreatRange 150
>   - **Tank (Bruto):** 100 HP, 25 daño, chaseSpeed 70, windup 0.8s
> - `ProjectileFactory`: crea proyectiles con Velocity, Combat (owner), Lifetime, y Sprite (cuadrado naranja 10×10).
> - Barras de vida flotantes (heredadas de Fase 2 — ya estaban) sobre enemigos dañados.
> - Flash de impacto y partículas de muerte (heredadas del `HealthSystem` existente).
> - Componente `Loot` con tipo (health/stamina), probabilidad de drop, y valor de restauración.
> - `HealthSystem` actualizado: al morir un enemigo con `Loot`, genera orbes de loot (rosa=HP, cyan=stamina) que flotan, tienen collider trigger, y se auto-destruyen tras 8 segundos.
> - `Game.cpp`: lógica `processLootPickups()` detecta AABB overlap entre jugador y orbes, restaurando HP o stamina al contacto.
>
> **Añadido fuera del plan base:**
> - Componente `Projectile` separado de `Combat` para datos específicos de proyectiles (dirección, velocidad).
> - `retreatRange` para ranged enemies: si el jugador se acerca demasiado, el Lanzafuego retrocede.
> - `attackCooldown` en AIBehavior para evitar que los enemigos ataquen sin pausa.
> - Orbes de loot con `isTrigger = true` en su Collider y Lifetime de 8 segundos para despawn automático.
> - Stagger se dispara automáticamente desde `HealthSystem` al detectar `hitFlashTimer > 0` en entidades con `AIBehavior`.
>
> **Archivos nuevos (10):**
> - `src/components/AIBehavior.h`, `Projectile.h`, `Loot.h`
> - `src/systems/AISystem.h`
> - `src/entities/EnemyFactory.h`, `ProjectileFactory.h`
> - `assets/data/enemies/melee.json`, `ranged.json`, `tank.json`
>
> **Archivos modificados (3):**
> - `src/systems/HealthSystem.h` — loot spawn + stagger trigger
> - `src/core/Game.h` — AISystem instance + processLootPickups declaration
> - `src/core/Game.cpp` — enemy spawning, AI update, loot pickup, title "Fase 3"

---

## Fase 4 — Primer Boss 🐉

> **Objetivo:** Un boss completo con múltiples fases, patrones, y espectáculo visual.
> **Criterio de éxito:** Pelear y derrotar a un boss con 3 fases que se siente desafiante y épico.

- [ ] Implementar componente `BossPhase`
  - [ ] Fases definidas por umbrales de HP
  - [ ] Patrones de ataque por fase
  - [ ] Transición entre fases (cinemática breve / pausa + rugido)
- [ ] Implementar `BossFactory`
  - [ ] Carga datos de boss desde JSON
  - [ ] Crea entidad con componentes especiales
- [ ] Implementar IA de boss
  - [ ] Selección de patrón (rotación o aleatorio ponderado)
  - [ ] Telegrafía clara antes de cada ataque (indicadores visuales)
  - [ ] Ventanas de castigo después de ciertos ataques
  - [ ] Enrage al llegar a Fase 3 (más rápido, más agresivo)
- [ ] Primer boss: **Gerión** (Círculo VIII — Fraude) o **Minotauro** (Círculo VII — Violencia)
  - [ ] Fase 1: 2-3 patrones básicos
  - [ ] Fase 2: Patrones nuevos + mecánica de arena
  - [ ] Fase 3: Modo furia, ataques combinados
- [ ] Sala de boss
  - [ ] Arena cerrada (puerta se cierra al entrar)
  - [ ] Posible arena que cambia entre fases (lava, destrucción, etc.)
- [ ] Efectos visuales de boss
  - [ ] Partículas abundantes (fuego, sangre, escombros)
  - [ ] Screen shake intenso en ataques pesados
  - [ ] Flash de pantalla en transiciones de fase
- [ ] Audio placeholder de boss
  - [ ] Sonido de rugido
  - [ ] Impactos pesados

**Notas de la Fase 4:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Fase 5 — Sistema de Habilidades ✨

> **Objetivo:** El jugador puede obtener habilidades activas y pasivas, elegir 1 de 3, rerollear con vida.
> **Criterio de éxito:** Completar una sala → aparece selección → elegir → habilidad funciona en combate.

- [ ] Implementar `Ability` base
  - [ ] Habilidades activas: ejecutables con cooldown
  - [ ] Habilidades pasivas: modifican stats/componentes permanentemente en esa run
  - [ ] Utilidades: curación, stamina boost, escudo temporal
- [ ] Implementar `AbilityPool`
  - [ ] Pool de todas las habilidades disponibles (cargadas desde JSON)
  - [ ] Filtrado por rareza y tema
  - [ ] Exclusión de habilidades ya obtenidas
- [ ] Implementar `AbilitySelection`
  - [ ] Elegir 3 opciones aleatorias del pool
  - [ ] Al menos 1 de cada tipo si es posible (activa, pasiva, utilidad)
  - [ ] Reroll: coste en % de vida máxima (incrementa por cada reroll en esa sala)
- [ ] Implementar `AbilityHolder` (componente)
  - [ ] Lista de habilidades activas equipadas (máx 2-3)
  - [ ] Lista de pasivas activas (sin límite razonable)
  - [ ] Keybindings para habilidades activas
- [ ] Implementar 10-15 habilidades iniciales
  - [ ] 🔥 **Fuego**: Lanza de Flegetonte, Escudo de Brasas, Aura Ardiente
  - [ ] 🩸 **Sangre**: Drenaje Vital, Filo Sangriento, Sed Eterna
  - [ ] ❄️ **Hielo**: Aliento de Cocytus, Cadenas de Escarcha, Coraza Helada
  - [ ] ☠️ **Peste**: Nube Tóxica, Toque Putrefacto, Miasma
  - [ ] ⚡ **Tormenta**: Relámpago, Viento Cortante, Ojo del Huracán
  - [ ] 🟢 **Utilidad**: Curación Menor, Curación Mayor, Fortalecer, Segundo Aire
- [ ] Implementar `AbilitySelectUI`
  - [ ] 3 cartas con nombre, descripción, icono, rareza
  - [ ] Botón de reroll con coste mostrado
  - [ ] Animación de aparición y selección
- [ ] Tests de habilidades (`tests/test_abilities.cpp`)

**Notas de la Fase 5:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Fase 6 — Sinergias 🔗

> **Objetivo:** Las habilidades del mismo tema se combinan creando efectos especiales.
> **Criterio de éxito:** Tener 2 habilidades de fuego activa un bonus de sinergia visible.

- [ ] Implementar `SynergyEngine`
  - [ ] Detectar cuántas habilidades de cada tema tiene el jugador
  - [ ] Umbrales: 2 habilidades = sinergia tier 1, 3 = tier 2, 4+ = tier 3
  - [ ] Activar/desactivar sinergias dinámicamente al obtener habilidades
- [ ] Definir sinergias en JSON (`assets/data/abilities/synergies.json`)
  - [ ] Cada tema tiene 2-3 niveles de sinergia
  - [ ] Sinergias cross-tema (Fuego + Sangre = "Sangre Ardiente")
- [ ] Implementar 6 sinergias temáticas (tier 1 de cada tema)
  - [ ] 🔥 Fuego T1: Ataques dejan trail de fuego
  - [ ] 🩸 Sangre T1: Lifesteal en todos los ataques
  - [ ] ❄️ Hielo T1: Ataques ralentizan enemigos
  - [ ] ☠️ Peste T1: Enemigos muertos explotan en veneno
  - [ ] ⚡ Tormenta T1: Chance de rayos al atacar
  - [ ] 💀 Sombra T1: Dash deja clon que explota
- [ ] Implementar 3 sinergias cross-tema
  - [ ] Fuego + Sangre: Sangre Ardiente
  - [ ] Hielo + Tormenta: Tormenta de Hielo
  - [ ] Peste + Sombra: Plaga Fantasma
- [ ] UI de sinergias activas (icono en el HUD)
- [ ] Feedback visual al activar sinergia (flash dorado, texto)

**Notas de la Fase 6:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Fase 7 — Generación Procedural 🗺️

> **Objetivo:** Cada run genera un layout diferente de salas conectadas por círculo.
> **Criterio de éxito:** Jugar 3 runs y que las salas sean diferentes pero jugables.

- [ ] Implementar `MapGraph`
  - [ ] Grafo de salas: nodos = salas, aristas = puertas
  - [ ] Tipos de sala: Normal, Élite, Tienda/Descanso, Boss
  - [ ] Garantizar camino al boss
  - [ ] Layout ~8-12 salas por círculo
- [ ] Implementar `RoomTemplates`
  - [ ] 5-8 templates por tipo (variedad)
  - [ ] Templates definidos en JSON (posición de tiles, spawns, puertas)
  - [ ] Validación: todas las salas son completables
- [ ] Implementar `CircleGenerator`
  - [ ] Genera un círculo completo: selecciona y conecta templates
  - [ ] Escala de dificultad dentro del círculo
  - [ ] Coloca el boss al final
- [ ] Implementar `RoomPopulator`
  - [ ] Coloca enemigos según dificultad
  - [ ] Coloca trampas (pinchos, lava, ácido)
  - [ ] Coloca decoración ambiental
- [ ] Transiciones entre salas
  - [ ] Efecto de transición (fade? deslizamiento?)
  - [ ] Colocar al jugador en la puerta correcta
- [ ] Minimap (opcional en esta fase)
  - [ ] Salas visitadas / no visitadas
  - [ ] Posición actual

**Notas de la Fase 7:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Fase 8 — UI Completa 🖼️

> **Objetivo:** Todos los menús, HUD y pantallas del juego están implementados y son estéticamente coherentes.
> **Criterio de éxito:** Flujo completo: Menú → Selección → Jugar → Morir → Stats → Reintentar.

- [ ] Implementar `HUD`
  - [ ] Barra de vida (estilo souls — larga, con animación de pérdida)
  - [ ] Barra de stamina
  - [ ] Iconos de habilidades activas + cooldowns
  - [ ] Indicador de sinergias activas
  - [ ] Indicador de círculo actual
- [ ] Implementar `MainMenu`
  - [ ] Título "INFERNUS" con efecto de fuego/ambient
  - [ ] Opciones: Nueva Run, Colección, Opciones, Salir
  - [ ] Fondo animado (almas, fuego, ceniza)
- [ ] Implementar `CharacterSelectUI`
  - [ ] Personajes desbloqueados vs. bloqueados
  - [ ] Stats de cada personaje
  - [ ] Ventaja/desventaja visibles
- [ ] Implementar `PauseMenu`
  - [ ] Continuar, Opciones, Abandonar Run
- [ ] Implementar `DeathScreen`
  - [ ] Estadísticas de la run (enemigos, salas, círculo alcanzado, tiempo)
  - [ ] Desbloqueos obtenidos
  - [ ] Opciones: Reintentar, Menú Principal
- [ ] Implementar `AbilitySelectUI` (polish)
  - [ ] Animaciones de carta
  - [ ] Efecto de rareza (brillo, borde)
  - [ ] Tooltip de sinergias potenciales
- [ ] Fuente pixel temática (cargar TTF/OTF)
- [ ] Diseño cohesivo: todo con la paleta infernal del GDD

**Notas de la Fase 8:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Fase 9 — Meta-Progresión 💾

> **Objetivo:** Los progresos persisten entre runs: desbloqueos, personajes, lore.
> **Criterio de éxito:** Llegar al Círculo VII → cerrar juego → abrir → las habilidades de ese círculo están desbloqueadas.

- [ ] Implementar `SaveManager`
  - [ ] Guardar/cargar datos en JSON (save file local)
  - [ ] Auto-save al finalizar cada run
  - [ ] Datos: desbloqueos, stats globales, lore encontrado
- [ ] Implementar `UnlockManager`
  - [ ] Habilidades desbloqueadas por círculo alcanzado
  - [ ] Personajes desbloqueados por condiciones (llegar a X, derrotar boss Y sin daño, etc.)
  - [ ] Skins por logros
- [ ] Implementar `LoreJournal`
  - [ ] Fragmentos de lore desbloqueables
  - [ ] Visualizador en menú principal (Colección)
  - [ ] Entradas sobre enemigos, bosses, círculos, el protagonista
- [ ] Implementar `RunStats`
  - [ ] Tracking: enemigos derrotados, daño hecho, daño recibido, habilidades usadas
  - [ ] Tiempo de run
  - [ ] Mejor run (más círculos, más rápido)
- [ ] Estadísticas globales en menú
  - [ ] Total de runs, total de muertes
  - [ ] Boss más veces derrotado
  - [ ] Habilidad más elegida

**Notas de la Fase 9:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Fase 10 — Audio, Partículas y Polish ✨

> **Objetivo:** El juego se SIENTE bien. Audio, efectos visuales, juice, transiciones, todo pulido.
> **Criterio de éxito:** Un playtester dice "esto se siente profesional".

- [ ] Implementar `AudioManager`
  - [ ] Música por círculo (loop)
  - [ ] Transiciones suaves entre tracks
  - [ ] Música de boss (entra al empezar el fight)
  - [ ] Silencio/ambiente para el Limbo
- [ ] Implementar SFX completos
  - [ ] Combate: golpes, esquivas, parry, impactos
  - [ ] Habilidades: cada habilidad con sonido propio
  - [ ] UI: selección, confirmación, reroll
  - [ ] Ambiente: fuego, viento, gritos, goteo
- [ ] Implementar partículas completas
  - [ ] Fuego ambiental (antorchas, lava)
  - [ ] Ceniza/ascuas flotando
  - [ ] Sangre al golpear
  - [ ] Explosiones de habilidades
  - [ ] Efecto de muerte de enemigos
  - [ ] Trail de dash
- [ ] Screen effects
  - [ ] Screen shake (variable por impacto)
  - [ ] Hit stop (freeze frame de 1-2 frames al conectar golpe fuerte)
  - [ ] Flash de pantalla en transiciones de boss
  - [ ] Vignette en baja vida
- [ ] Transiciones
  - [ ] Fade entre salas
  - [ ] Efecto de inicio de run (caída al círculo)
  - [ ] Efecto de muerte (slow-mo → dissolve)
- [ ] Parallax backgrounds por círculo
- [ ] Balance pass
  - [ ] Ajustar HP/daño de enemigos
  - [ ] Ajustar costes de stamina
  - [ ] Ajustar rareza y poder de habilidades
  - [ ] Ajustar curva de dificultad entre círculos

**Notas de la Fase 10:**
> _(vacío — actualizar cuando se trabaje en esta fase)_

---

## Contenido Adicional (Post-Core) 📦

> Estas tareas son para DESPUÉS de que el core loop esté completo y pulido.

- [ ] Implementar los 9 circles completos (enemigos únicos, tilesets, bosses)
- [ ] Implementar los 6 personajes jugables con mecánicas únicas
- [ ] Expandir pool de habilidades a 50+
- [ ] Expandir sinergias a 15+ combinaciones
- [ ] Modo de dificultad (Purgatorio = fácil, Infierno = normal, Malebolge = difícil)
- [ ] Salas secretas / eventos aleatorios
- [ ] NPC que vende/intercambia habilidades (la sombra de Virgilio?)
- [ ] Localización (Español, Inglés como mínimo)
- [ ] Steam integration (achievements, cloud saves)
- [ ] Trailer / marketing assets

---

## Registro de Decisiones Técnicas 📋

| Fecha | Decisión | Contexto |
|---|---|---|
| 28/02/2026 | C++17 + Raylib como stack | Mejor balance de control, rendimiento y velocidad de desarrollo para un AI coder |
| 28/02/2026 | ECS custom sin librería externa | Mantener dependencias mínimas, sistema adaptado a sinergias |
| 28/02/2026 | Data-driven con JSON | Iterar diseño sin recompilar, facilita balanceo |
| 28/02/2026 | Empezar por Fase 0-1 (setup + jugador) | Core loop primero, contenido después |

---

## Registro de Bugs Conocidos 🐛

| ID | Descripción | Severidad | Estado | Fase |
|---|---|---|---|---|
| — | _(ninguno aún)_ | — | — | — |

---

## Notas Generales 📝

- **Prioridad absoluta:** que el juego sea DIVERTIDO de jugar. Si algo se siente mal, se itera.
- **Arte placeholder está bien** durante las fases 0-7. El polish visual viene en la fase 10.
- **Testear jugabilidad constantemente.** No esperar a tener todo listo para probar.
- **Committear con frecuencia.** Cada feature completa = commit con mensaje descriptivo.

---

> *"El plan sobrevive hasta el primer contacto con el código. Pero sin plan, no hay juego."*
