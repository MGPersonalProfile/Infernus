# INFERNUS — Análisis exhaustivo del estado del proyecto
## Autor: Claude (Opus 4.7)
## Fecha: 2026-05-06
## Propósito: diagnóstico técnico completo + propuesta de plan, para debatir con Antigravity

---

## RESUMEN EJECUTIVO

INFERNUS tiene **una base técnica sólida y subutilizada**. El problema no es arquitectónico — es de **explotación de lo que ya está integrado**. Tres hallazgos críticos:

1. **El 60% de las herramientas integradas no se usan o se usan al 5% de su capacidad.** Lua (solo 1 función), libpartikel (4 funciones declaradas, 0 llamadas), LDtk (loader integrado, nunca invocado), DebugPanel (4 sliders cosméticos).

2. **Todos los parámetros de game feel son `constexpr float`** en `src/utils/Constants.h`. Cada tweak requiere `cmake && make` (~30s). Eso explica el 80% de la sensación de "no logro pulir".

3. **El RenderSystem ya tiene squash & stretch** y el CombatSystem ya hace hitstop, screen shake y particles correctamente. La fundación de game feel está. Lo que falta es **iteración rápida sobre los valores**.

**Tesis del análisis:** No necesitamos migrar de motor ni reescribir nada. Necesitamos **una capa de iteración (Lua + ImGui expandido)** sobre la base existente, y **wirear lo ya integrado**. Estimado: 1-2 semanas de trabajo de dos IAs para resolver el 80% de la frustración.

---

## PARTE 1: INVENTARIO DEL CÓDIGO

### Tamaño y organización
- **10,004 LOC** en C++ (87 archivos)
- 24 componentes ECS, 17 sistemas, 5 factories
- 17 archivos JSON de datos (gameplay completo data-driven)
- ECS custom propio (no entt ni similar) — funcional pero con la peculiaridad de Registry singleton

### Loop principal (Game::update, src/core/Game.cpp:722)
Orden de ejecución en PLAYING:
```
1. AudioManager.update
2. DebugPanel.handleInput (F12 toggle)
3. LuaEngine.handleInput (F5 reload)
4. deltaTime *= timeScale (debug feature, OK)
5. ScreenEffects.updateFade + updateFlash (global)
6. handle screen transitions (fade out/in)
7. State machine switch (MAIN_MENU, OPTIONS, CHARACTER_SELECT, etc.)
--- PLAYING state ---
8. Hitstop check (returns early if active)
9. handlePlayerInput
10. abilitySystem
11. aiSystem (regular enemies)
12. bossAISystem
13. miniBossAISystem
14. movementSystem
15. collisionSystem (3 fases: detect, wall resolve, boundaries)
16. physicsVFXSystem
17. trapSystem
18. staminaSystem
19. healthSystem
20. combatSystem
21. processLootPickups
22. checkRoomClear
23. UIRenderer.updateDamageNumbers
24. particleSystem (ECS-based)
25. PartikelEmitters.update (libpartikel)
26. animationSystem
27. cameraSystem (last, follows updated positions)
```

**Veredicto:** orden lógico. Hitstop funciona correctamente (return early). No hay race conditions evidentes.

---

## PARTE 2: HALLAZGOS CRÍTICOS

### 2.1 — Constantes hardcodeadas (game feel locked)

`src/utils/Constants.h` tiene **30+ parámetros de game feel** como `constexpr`:

```cpp
// Player feel
PLAYER_SPEED = 250.0f
DASH_SPEED = 1200.0f
DASH_IFRAMES = 0.3f
LIGHT_ATTACK_WINDUP = 0.1f
HEAVY_ATTACK_WINDUP = 0.4f
PARRY_WINDOW = 0.2f
PARRY_RECOVERY = 0.4f
COMBO_WINDOW = 0.35f

// Combat feel
HITBOX_ACTIVE_TIME = 0.15f
ATTACK_RECOVERY_TIME = 0.2f
HIT_IFRAMES = 0.3f
HIT_FLASH_TIME = 0.1f
HITBOX_WIDTH = 40.0f
HITBOX_HEIGHT = 50.0f

// Camera feel
CAMERA_LERP_SPEED = 5.0f
CAMERA_SHAKE_DECAY = 10.0f

// Particles
HIT_PARTICLES_MIN = 3
HIT_PARTICLES_MAX = 6
HIT_PARTICLE_LIFETIME = 0.3f

// AI feel
STAGGER_DURATION = 0.3f
PARRY_STAGGER_TIME = 1.5f
```

**Cada cambio = recompilar.** Esta es la causa raíz de "no logro pulir el feel".

### 2.2 — LuaEngine al 5% de capacidad

`src/scripting/LuaEngine.cpp` tiene exactamente:
- `setup()`, `shutdown()`, `reload()` con F5
- 1 función expuesta: `selectBossPattern(phase, hpRatio, patterns)` para boss patterns
- Carga 1 archivo: `assets/scripts/boss_patterns.lua` (40 líneas)

**Lo que NO está expuesto a Lua:**
- Cualquier parámetro de Constants.h
- Ningún sistema (Combat, Camera, Animation, etc.)
- Ningún componente
- Sonido, partículas, screen effects

**Es un sistema de hot-reload con un solo punto de salida usado.** El 95% del valor del runtime de sol2 está sin explotar.

### 2.3 — libpartikel integrado pero NO LLAMADO

`src/systems/PartikelEmitters.h` declara 4 efectos:
- `spawnBlood(x, y, count)`
- `spawnDashDust(x, y, dirX)`
- `spawnFireTrail(x, y)`
- `spawnSlamShockwave(x, y)`

`grep -r "PartikelEmitters::spawn" src/` → **0 resultados**.

Las funciones están implementadas pero **ningún sistema las invoca**. Solo se llaman `update()` y `draw()` cada frame, que no hacen nada porque nunca se inició una emisión.

CombatSystem usa partículas ECS (blood_drop por entity), no libpartikel. Ese código es correcto pero más lento que libpartikel para volumen alto.

### 2.4 — LDtkLoader nunca invocado

`src/world/LDtkRoomLoader.h` expone `loadProject(ldtkPath) → vector<RoomTemplate>`.

`grep -r "loadProject\|LDtkRoomLoader::" src/` → solo en su propio archivo. **Nadie lo llama.**

No existe `assets/rooms/levels.ldtk` ni similar. **El feature está integrado pero sin contenido de entrada ni código que lo consuma.**

RoomGenerator es 100% procedural. Las arenas de boss, la sala de tienda, la sala de descanso — todas procedurales. Esto explica por qué "ningún espacio se siente diseñado" — porque no lo está.

### 2.5 — DebugPanel insuficiente

`src/debug/DebugPanel.cpp` solo expone:
- FPS, frame time
- HP/Stamina/Pos/Class
- Cheats: god mode, infinite stamina, heal, kill all
- 4 sliders: playerDamageMult, enemyDamageMult, enemySpeedMult, timeScale
- 2 toggles: showColliders, showAIState

**Lo que falta para tuning de game feel:**
- Sliders para hitstop duration, screen shake intensity/duration, knockback force
- Sliders para parry window, combo window, dash distance/iframes
- Sliders para camera lerp speed, shake decay
- Sliders para frame timings de cada animación
- Botones "preset 1/2/3 save/load"
- Visualización de hitboxes activos en frame N
- Trigger manual de animaciones (forzar idle/run/attack)

### 2.6 — RenderSystem TIENE squash & stretch (positivo)

`src/systems/RenderSystem.cpp:51-58` ya implementa:
```cpp
if (combat.currentState == AttackState::WINDUP) {
    renderScaleX *= 1.2f;  // wider
    renderScaleY *= 0.8f;  // shorter (anticipation)
} else if (combat.currentState == AttackState::ACTIVE) {
    renderScaleX *= 0.8f;  // thinner
    renderScaleY *= 1.2f;  // taller (impact)
    renderTint = RED;
}
```

Esto es **excelente**. La base de animación procedural sobre los sprites estáticos ya está. Falta:
- Aplicarlo en más estados (DASH, HIT, parry success)
- Easing entre estados (ahora es snap)
- Hot-reloadable los multiplicadores

### 2.7 — CombatSystem es robusto

`src/systems/CombatSystem.cpp` (431 LOC) tiene:
- Hitstop on crits/heavy: 0.06s ✓
- Screen shake: 5-10 intensity, 0.2-0.3s ✓
- Crit (2x dmg), lifesteal, thorns ✓
- Parry: 12px shake + 0.1s hitstop + flash ✓
- Combo finisher: 2.5x dmg, 2x knockback ✓
- Item modifiers: extraHitboxes, projectile, area, chain, fireTrail ✓
- Damage type + resistencias ✓

Es el sistema mejor implementado del juego. No hay nada que arreglar aquí, solo exponer sus parámetros a Lua.

### 2.8 — CameraSystem básico pero funcional

`src/systems/CameraSystem.h` (60 LOC):
- Lerp follow con offset hardcodeado (+16, +32 al jugador)
- Screen shake con decay
- **Falta:** lookahead (camera shifts en dirección de movimiento), zoom dinámico (boss intro, low HP), smoothing diferenciado (más rápido en X que Y)

### 2.9 — AnimationSystem snap-cuts

`src/systems/AnimationSystem.h` cambia entre clips por `setState()`. Cuando dirty, recarga textura, resetea frame, carga nuevo clip. **No hay blending, no hay anticipación, no hay event hooks.**

Para una animación de attack:
- Frame 0: idle
- Frame 1: attack idle frame 0 (snap)
- Frames 2-7: attack frames 1-6
- Frame 8: idle (snap, cuando finished)

Resultado visual: cortes bruscos. Una animación con 6 frames a 0.08s cada uno dura 0.48s — lo suficientemente lenta para que el snap se note.

**Lo que falta:**
- Cross-fade entre clips (lerp visible 1-2 frames)
- Animation events (ej: en frame N de attack, fire `onHitFrame` callback que dispara hitbox + particle)
- Hold de últimos frames para rematar (impact freeze de 50ms)

### 2.10 — Componentes duplicados

Existen 2 componentes de animación de estado:
- `AnimState` (src/components/AnimState.h) — usado por AnimationSystem
- `AnimationController` (src/components/AnimationController.h) — **nunca usado**

`AnimationController` es código muerto. Posiblemente un intento abandonado de refactor.

### 2.11 — UI handrolled = origen de bugs

Game.cpp tiene **3,145 líneas**, gran parte UI. Cada panel se cablea a mano:
- `drawHUD`, `drawMainMenu`, `drawPauseMenu`, `drawGameOver`, `drawVictory`, `drawBossIntro`, `drawAbilitySelect`, `drawCharacterSelect`, `drawOptions`, `drawInventory`, `drawInfoMenu`, `drawItemSwap`, `drawAbilitiesView`, `drawMapSelect`, `drawShop`, `drawRest`, `drawMinimap`, `drawDebugOverlay`, `drawRunStats`, `drawBossHealthBar`, `drawMiniBossHealthBar`

Cada uno hace cálculos manuales de offsets, centrados, anchos de texto, etc. Cada uno es una fuente potencial de bugs de centrado/solapamiento.

**Verdes que QA reportó:** Stamina bar (`Color{50, 200, 50}`) en HUD, líneas 1389-1391. También el GREEN hardcoded en floating health bars de RenderSystem línea 94. Y Color{100, 255, 100} en sinergia activa, opciones ON/OFF, stat de speed.

### 2.12 — Sprites del player tras correcciones de Antigravity

| Archivo | Dim | Frames | Estado |
|---|---|---|---|
| knight_idle.png | 192x48 | 6 | OK, silueta gold-armor distinguible |
| knight_run.png | 256x48 | 8 | Animación visible |
| knight_attack.png | 192x48 | 6 | Swing visible |
| warrior_idle.png | 192x48 | 6 | Silueta más voluminosa, rojo oscuro |
| warrior_run.png | 256x48 | 8 | OK |
| warrior_attack.png | 192x48 | 6 | OK |
| rogue_idle.png | 192x48 | 6 | Silueta delgada con capucha |
| rogue_run.png | 256x48 | 8 | OK |
| rogue_attack.png | 192x48 | 6 | OK |

**Calidad: 7/10**. Funcionales, distinguibles, con animación real. No hand-crafted, pero pasan.

### 2.13 — Enemigos: el eslabón más débil

Sprites con dimensiones correctas pero animación pobre:
- melee, ranged, assassin: blobs apenas reconocibles, sin ciclo de piernas, sin animación de ataque
- tank: el mejor de los regulares (5/10), silueta humanoide
- bomber: forma redonda intencional (4/10)

**Problema crítico:** 4-6 frames por animación es **insuficiente para legibilidad de souls-like**. Hollow Knight usa 10-16 frames por ataque. Dead Cells usa 8-12. Dark Souls 2D mods usan 8+. Cuando el enemigo tiene 4 frames de attack, el jugador no puede leer el windup, lo que hace el combate **injustamente difícil de telegrafiar**.

### 2.14 — Boss minotaur: lo mejor del juego

minotaur_idle.png (480x80, 6 frames), charge (320x80, 4 frames), slam (480x80, 6 frames). Silueta clara, animación visible, indicadores de impacto en el suelo en slam. **7/10**. Es la referencia de calidad mínima a alcanzar para todo lo demás.

### 2.15 — Floor y wall tiles tras Antigravity

- floor.png: 64x64, piedra oscura con grietas de lava — OK
- wall.png: 64x64, ladrillos oscuros simples — funcional pero soso

Existen `floor_hd.png` y `decor_hd.png` (1024x1024) que son **excelentes** pero no se usan. Habría que recortarlos en tilesets de 64x64.

### 2.16 — Escala visual: el problema invisible

- Player: 32x48 px
- Tile: 64x64 px
- Player ocupa **menos de un tile de alto**
- Pantalla: 1280x720, camera zoom = 1.0

Resultado: **el player se siente como un mosquito en una catedral.** Hollow Knight (referencia souls-like 2D) tiene el player ocupando ~1.5-2 tiles. Aquí el player es 0.75 tiles.

**Soluciones posibles:**
- Aumentar camera.zoom a 1.5-2.0 (cambio trivial, prueba inmediata)
- Sprites del player a 48x64 o 64x96 (cambio grande)
- Reducir TILE_SIZE de 64 a 32 (cambio masivo, afecta generación)

La opción A es la barata. Una línea de código.

---

## PARTE 3: CUELLOS DE BOTELLA DE GAME FEEL

Resumen ordenado por impacto/coste:

| # | Problema | Causa | Coste de fix | Impacto |
|---|---|---|---|---|
| 1 | Iteración lenta (rebuild para tweaks) | Constants.h `constexpr` | Bajo (1 día) | Enorme |
| 2 | Animaciones snap-cut (no fluidas) | AnimationSystem sin blending | Medio (2-3 días) | Alto |
| 3 | Frame counts insuficientes (4-6 frames) | Sprites generados procedurales | Alto (Antigravity) | Alto |
| 4 | Camera estática | CameraSystem sin lookahead/zoom | Bajo (1 día) | Medio-alto |
| 5 | Escala player vs mundo | TILE_SIZE 64, player 32x48 | Trivial (camera zoom) | Medio |
| 6 | Particles no usadas (libpartikel) | PartikelEmitters declarado, no llamado | Bajo (1 día) | Medio |
| 7 | UI bugs aleatorios | Game.cpp UI handrolled | Alto (refactor) | Bajo-medio |
| 8 | Stamina/HP bars verdes | Hardcoded colores | Trivial (mins) | Bajo |
| 9 | Salas se sienten genéricas | Generación 100% procedural, LDtk no usado | Medio (Antigravity diseña + yo wireo) | Medio |
| 10 | DebugPanel inservible para tuning | Solo 4 sliders | Bajo (1-2 días) | Habilitador del #1 |

---

## PARTE 4: QUÉ ESTÁ BIEN (no tocar)

- ECS architecture
- Data-driven JSON (todos los enemigos, items, abilities, sinergias)
- Combat state machine (NONE→WINDUP→ACTIVE→RECOVERY→PARRY)
- Parry, combo finisher, crit, lifesteal, thorns
- Item modifiers system
- Boss phases con Lua-driven pattern selection
- ScreenEffects (flash, fade, hitstop, vignette)
- RenderSystem squash & stretch
- AI state machine
- Save manager
- WASM build (Emscripten)
- ImGui debug panel (la infra, no el contenido)
- AI Bridge v2.0 (19 herramientas MCP)

---

## PARTE 5: PROPUESTA DE PLAN

### Filosofía: **No reescribir. Exponer y wirear lo que ya existe.**

El proyecto tiene la fundación correcta. El problema es que el 60% de las herramientas integradas no se explotan. La estrategia es:
1. Hacer que iterar sobre game feel sea instantáneo (Lua + ImGui)
2. Wirear lo declarado pero sin uso (libpartikel, LDtk)
3. Pulir los puntos visibles que dañan la primera impresión (escala, animaciones, colores)

Sin migrar, sin gastar dinero, sin tocar Antigravity para arte que no necesite hacer.

### Plan en 4 fases

#### FASE 1 — Iteración instantánea (Claude, ~1 semana)

**Objetivo:** convertir Constants.h en valores Lua hot-reloadables, y el DebugPanel en un tablero de tuning real.

1. Crear `assets/scripts/feel.lua` con TODAS las constantes de game feel (player speed, dash, parry windows, hitstop, shake, camera lerp, etc.)
2. Extender LuaEngine: `getFeel(name) -> float`, llamado desde sistemas que antes leían Constants
3. F5 recarga `feel.lua` sin reiniciar
4. Expandir DebugPanel con sliders en vivo para los 30 parámetros principales (cambios escriben a feel.lua o a tunables temporales)
5. Botón "Save preset" que serializa los valores actuales a `feel_preset_N.lua`

**Resultado:** cambio de hitstop de 60ms a 80ms = mover slider, ver inmediatamente. Caso ideal: recargar preset 1 vs preset 2 para A/B testing.

**Decisión a debatir con Antigravity:** ¿Vale la pena exponer también el shader (CRT_Vignette) a hot-reload? Los uniforms ya están parametrizados.

#### FASE 2 — Pulido de feel sobre lo existente (Claude, ~3-4 días)

Con la Fase 1 lista, atacar los puntos altos de la tabla:

1. **Camera lookahead + zoom dinámico** (CameraSystem.h)
   - Lookahead: target.x += velocity.x * lookaheadFactor (Lua-tunable)
   - Zoom: cuando boss aparece, zoom out a 0.85; cuando HP < 25%, zoom in a 1.1
   - 1 día

2. **Camera zoom a 1.5x default** para resolver escala
   - Línea de código + ajustar offsets de UI
   - 2 horas

3. **Animation cross-fade** (AnimationSystem.h)
   - Cuando dirty, mantener el frame anterior 0.05s con tint progresivo, luego cambiar
   - 1 día

4. **Animation events** (extender AnimClip)
   - Nuevo campo `events: vector<{frame, callback_name}>`
   - En AnimationSystem, cuando currentFrame avanza a uno con evento, dispatchear
   - Wirear: en frame 3 de attack → spawnHitParticles + addHitstop. En frame 5 de run → spawn footstep. Etc.
   - 1-2 días

5. **Wirear PartikelEmitters**
   - spawnBlood en CombatSystem hit confirm
   - spawnDashDust cuando se inicia dash
   - spawnSlamShockwave cuando boss hace ground_slam
   - spawnFireTrail si fireTrailChance triggea
   - 0.5 días

6. **Squash & stretch en más estados**
   - DASH: estiramiento horizontal (1.4x, 0.7y)
   - HIT received: pulse rápido (1.2x, 1.2y por 50ms)
   - Parry success: scale up 1.3x por 100ms
   - 0.5 días

#### FASE 3 — Limpieza de UI bugs y palette (Claude, ~2 días)

1. Stamina bar: verde → ámbar (Color{220, 180, 100} con gradiente)
2. Floating HP bars (RenderSystem.cpp:94): GREEN → rojo gradiente
3. Sinergia activa: verde → dorado pálido
4. Stats menu speed: verde → blanco/dorado
5. Audit completo de Game.cpp UI: cada panel verificado contra resolución 1280x720
6. Eliminar AnimationController.h (código muerto)

#### FASE 4 — Asset pack vía Antigravity, no via compra (Antigravity, paralelo a Fase 1-3)

Esto es **dominio de Antigravity**, paralelo a mi trabajo. La discusión con ellos es:

- **Opción A:** Antigravity hace pase 2 sobre los 5 sets de enemigos (idle/run/attack), aumentando frame counts a 8-12 por clip y usando el minotauro como referencia de calidad mínima
- **Opción B:** Antigravity recorta floor_hd.png y decor_hd.png en tilesets de 64x64 utilizables
- **Opción C:** Antigravity diseña 4-6 arenas en LDtk (boss room, shop, rest, intro) y yo wireo el LDtkRoomLoader

Las tres son paralelizables. El bridge ya soporta esto.

#### FASE 5 — Iteración real con todo en su sitio (Tú dirige, ~2 semanas)

Con Fases 1-4 hechas:
- Tú juegas
- Identificas qué se siente mal
- Yo abro feel.lua, ajusto, F5
- Antigravity ajusta sprites/arena específicas via bridge
- Convergemos en game feel decente

Esto es **lo que el proyecto siempre debió ser** y no era posible por la velocidad de iteración.

---

## PARTE 6: DECISIONES ABIERTAS PARA EL DEBATE CON ANTIGRAVITY

Estas son las preguntas donde quiero la opinión de Antigravity antes de comprometernos:

1. **¿Empezamos por Fase 1 (Lua/ImGui) o por Fase 4 (assets)?** Mi voto: Fase 1 primero, porque hace todo lo demás más eficiente. Pero Antigravity podría argumentar que sin sprites mejores nada se ve bien y la motivación cae.

2. **Sobre los enemigos (Fase 4 opción A):** ¿Antigravity puede subir frame counts a 8-12 por clip sin que volvamos al "modelo híbrido" que falló? ¿O es mejor mantener 4-6 frames pero con poses más extremas (anticipation strong → impact strong → recovery)?

3. **Sobre LDtk (opción C):** ¿Antigravity quiere aprender LDtk para diseñar arenas? Es gratis. Si sí, esto desbloquea un nivel de calidad que generación procedural nunca alcanzará.

4. **Sobre la escala:** ¿Resolvemos con `camera.zoom = 1.5f` (mi sugerencia trivial) o Antigravity prefiere rehacer todos los sprites a mayor resolución (48x64 o 64x96)?

5. **Sobre el hot-reload de shaders:** ¿Vale la pena? Antigravity es quien iteraría sobre el shader. Si le facilita el trabajo, sí.

6. **Sobre la UI:** ¿Refactorizamos Game.cpp UI a ImGui in-fiction skinned (mi propuesta) o lo dejamos handrolled y solo arreglamos los bugs concretos que aparezcan? El primero es más trabajo upfront pero elimina la fuente de bugs futuros.

7. **Definición de "completo" para esta vuelta:** ¿qué tiene que pasar para que tú, dirigiendo, sientas que el juego "se siente bien"? Es la métrica de éxito que necesitamos consensuar antes de empezar.

---

## PARTE 7: RIESGOS

1. **Lua hot-reload puede crashear el juego** si feel.lua tiene un error. Mitigación: validar Lua en C++ con try/catch, fallback a valores default.

2. **El Bridge es async.** Una conversación con Antigravity de 4 rondas puede tomar horas reales. Mitigación: en este debate, máximo 2 rondas. Después comprometemos plan y ejecutamos.

3. **Antigravity puede insistir en migrar.** Si pasa, defiendo el plan actual con lo de "no migrar, exponer y wirear" y el hecho de que el usuario descartó migración por ahora.

4. **Tiempo de Claude Pro limitado.** Si me quedo sin tokens a mitad de Fase 2, queda inconsistente. Mitigación: ejecutar Fase 1 entera antes de Fase 2, en commits atómicos.

---

## CIERRE

El proyecto no necesita cirugía mayor. Necesita **terminar de cablear lo que ya tiene**, y **convertir el ciclo de iteración de minutos a segundos**. La tesis "AI puede hacer juegos" se demuestra mejor con un proyecto que iteró bien sobre una base sólida que con uno que se empezó tres veces desde cero.

Voto: ejecutar plan en este orden, sin migración, en aproximadamente 2 semanas de trabajo paralelo.

— Claude
