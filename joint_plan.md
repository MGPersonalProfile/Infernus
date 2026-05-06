# INFERNUS — Plan Conjunto de Ejecución
## Aprobado por Claude (Opus 4.7) y Antigravity (Gemini)
## Fecha: 2026-05-06
## Status: **APROBADO POR EL USUARIO — LISTO PARA EJECUTAR**

---

## CONTEXTO

Este documento es el contrato de trabajo entre las dos IAs principales del proyecto. Resulta del diagnóstico técnico (claude_analysis.md) + debate del bridge (task_1778098208165). El usuario aprobó el plan con un añadido explícito: **partir Game.cpp en módulos** (organización de archivos, no refactor de UI).

Restricciones absolutas:
- Cero presupuesto. Solo herramientas gratuitas.
- No migración de motor.
- El usuario dirige; las IAs ejecutan.

Tesis: el proyecto tiene base técnica sólida pero **subutilizada**. La estrategia es exponer y wirear lo ya integrado, no reescribir.

---

## FASES (en orden de ejecución)

### FASE 1 — Iteración instantánea (Claude, ~1 semana)

**Objetivo:** convertir Constants.h en parámetros Lua hot-reloadables y el DebugPanel en un tablero de tuning real.

1. **Spike de validación (primer día, 2-3h):** verificar que sol2 + F5 + ImGui sliders funcionan end-to-end en Windows + Emscripten. Si hay problemas serios con sol2 hot-reload, replantear estrategia antes de comprometer la semana.
2. Crear `assets/scripts/feel.lua` con todos los parámetros de game feel (Constants.h game-feel section).
3. Extender `LuaEngine`: `getFeel(name) -> float`, callable desde sistemas.
4. Refactorizar Combat/Camera/Animation/Stamina/Player para leer de `LuaEngine::getFeel()` en vez de `Constants::*`.
5. F5 recarga `feel.lua` en runtime, sin reiniciar.
6. Expandir `DebugPanel` con sliders en vivo para 30 parámetros (hitstop, shake, parry, dash, camera lerp, etc.).
7. Botón "Save preset" → escribe `feel_preset_N.lua`. "Load preset" → recarga.
8. Si trivial, exponer también los uniforms del shader CRT_Vignette al feel.lua (vignette intensity, infernal tint mix, etc.).

**Criterio de finalización:** mover un slider del DebugPanel cambia el game feel inmediatamente sin recompilar.

### FASE 2 — Pulido de feel sobre lo existente (Claude, ~3-4 días)

Con Fase 1 lista, atacar los puntos de impacto alto:

1. **Camera lookahead + zoom dinámico** (CameraSystem.h). 1 día.
2. **camera.zoom = 1.5f** default para resolver escala. 2h. Si rompe UI, ajustar offsets.
3. **Animation cross-fade** (AnimationSystem.h). 1 día.
4. **Animation events vía JSON** (mejora propuesta por Antigravity). Yo implemento dispatcher; Antigravity configura events por sprite. Schema: `{frame, action, params}`. Acciones disponibles: `spawn_particles`, `activate_hitbox`, `screen_shake`, `play_sfx`, `add_hitstop`. 1-2 días.
5. **Wirear `PartikelEmitters`**: spawnBlood en hits, spawnDashDust en dash, spawnSlamShockwave en boss slam, spawnFireTrail si proc. 0.5 días.
6. **Squash & stretch en más estados**: DASH, HIT received, parry success. Multiplicadores leídos de feel.lua. 0.5 días.

**Criterio de finalización:** los 6 puntos de la métrica de éxito (sección "Definición de completo") se cumplen al jugar.

### FASE 3 — Limpieza, splits y bugs (Claude, ~2-3 días)

1. **Partir Game.cpp en módulos** (añadido por petición del usuario). Cero cambio de comportamiento, solo reorganización:
   - `Game.cpp` (~600 LOC) — init, run, render shell, member state, transitions.
   - `Game_UI.cpp` (~1500 LOC) — todas las funciones `draw*` (HUD, menus, screens).
   - `Game_StateMachine.cpp` (~500 LOC) — el switch grande de update() por GameState.
   - `Game_RoomFlow.cpp` (~300 LOC) — spawnRoom, checkRoomClear, processLootPickups.
2. **Eliminar `AnimationController.h`** (código muerto, nunca instanciado).
3. **Fix de palette en UI:**
   - Stamina bar verde → ámbar/dorado (Color{220, 180, 100})
   - Floating HP bars (RenderSystem.cpp:94): GREEN → rojo gradiente
   - Sinergia activa: verde → dorado pálido
   - Stat de speed: verde → blanco
   - Indicadores ON/OFF en options: mantener verde/rojo (convención UX, excepción justificada)
4. **Verificar bugs concretos** del PDF de QA:
   - Centrados de paneles
   - Pantalla roja persistente al morir (probable que ya esté arreglado, verificar)
5. **Wirear LDtkRoomLoader** con una arena de prueba hardcodeada para que Antigravity pueda diseñar (precondición de la Fase 4 Opción C). 0.5 días.

**Criterio de finalización:** Game.cpp tiene <800 líneas; no quedan greens prohibidos en UI; LDtk loader carga arena de prueba.

### FASE 4 — Assets paralelos (Antigravity, paralelo a 1-3, ~2 semanas)

Antigravity ejecuta en paralelo con Claude desde el día 1:

**Semana 1:**
- **Opción B:** recortar `floor_hd.png` (1024x1024) y `decor_hd.png` (1024x1024) en tilesets de 64x64 utilizables. Independiente del estado del motor.
- **Opción A inicio:** pase 2 de `melee_*.png` y `ranged_*.png` con 8-10 frames usando pipeline procedural Pillow (no híbrido). Frame size puede ser distinto a 32x48 si la silueta lo requiere — el código ya lo soporta vía JSON.

**Semana 2:**
- **Opción A completa:** `assassin`, `tank`, `bomber` con 8-10 frames cada clip.
- **Opción C:** diseñar 4 arenas en LDtk (boss_arena, shop_room, rest_room, entry_hall) — solo cuando Claude haya wireado el loader (Fase 3 punto 5).
- **Wall tiles con más detalle** si camera.zoom = 1.5f revela que son demasiado sosos.

**Criterio de finalización:** los 5 enemigos tienen sprites con 8+ frames legibles; existen 4 arenas LDtk integradas; tilesets HD utilizables.

### FASE 5 — Iteración y convergencia (Tú diriges, ~indefinido)

Con Fases 1-4 completas:
- Tú juegas el build.
- Identificas qué se siente mal por punto de la métrica.
- Claude ajusta `feel.lua` o código según sea necesario, F5/rebuild.
- Antigravity ajusta sprites o arenas específicas vía bridge.
- **Pipeline de screenshots automatizado** (mejora propuesta por Antigravity): script Python toma screenshots del WASM build vía Puppeteer; Antigravity hace auditoría visual sin molestarte.

Esta fase termina cuando los 6 criterios de la métrica de éxito se cumplen y tú das el visto bueno.

---

## DEFINICIÓN DE "COMPLETO" (métrica de éxito)

Acordada con Antigravity. El juego se siente "bien" cuando:

1. **El combate se lee.** El jugador puede ver el windup de un enemigo y reaccionar. Ataques con 8+ frames y telegrafía clara.
2. **El player se siente ágil.** Dash, attack, movement responsivos (Lua-tuneados).
3. **La cámara acompaña.** Lookahead + zoom hacen que el player no sea un mosquito perdido.
4. **Los impactos se sienten.** Partículas (libpartikel wired), hitstop, screen shake, squash & stretch — todo el juice encendido.
5. **La primera impresión es buena.** Entras al juego y no ves bugs de UI verde, ni pantalla roja stuck, ni texto solapado.
6. **Una sala tiene diseño.** Al menos la boss arena se siente "diseñada" (LDtk), no procedural genérica.

Al cumplirse los 6, el usuario debería poder jugar 10 minutos y decir "esto se siente como un juego de verdad".

---

## DECISIONES TÉCNICAS LOCKEADAS

| # | Decisión | Resolución |
|---|---|---|
| 1 | Orden Fase 1 vs 4 | Ambas en paralelo. Claude empieza por Fase 1; Antigravity por Fase 4 Opción B. |
| 2 | Frame counts enemigos | 8-10 frames, pipeline procedural Pillow. NO modelo híbrido. |
| 3 | LDtk | Sí — Claude wirea loader primero (Fase 3), Antigravity diseña después. |
| 4 | Escala visual | `camera.zoom = 1.5f` primero. Rehacer sprites solo si zoom rompe algo. |
| 5 | Hot-reload de shader | Opcional — incluir en Fase 1 si trivial; si no, Fase 5. |
| 6 | UI refactor | NO refactor a ImGui. SÍ split de Game.cpp en módulos (Fase 3). Pintado handrolled se queda. |
| 7 | Definición de completo | Los 6 criterios arriba. |

---

## MEJORAS APORTADAS POR ANTIGRAVITY (integradas)

- **Animation events en JSON.** Más data-driven que mi propuesta original C++. Aceptado.
- **Screenshot comparison pipeline.** Auditoría visual automatizada en Fase 5. Aceptado.

## DECISIÓN DE ÚLTIMA HORA DEL USUARIO

- **Split Game.cpp en módulos.** Añadido a Fase 3. Es organización de archivos, no refactor de UI rendering.
- **Scope expandido.** No solo game feel — mejorar todo. Claude decide qué añadir.

## SCOPE EXPANDIDO (decisión técnica de Claude)

Añadidos al plan, mismo timeline general (algunos paralelos al trabajo principal):

### Añadidos a Fase 2 (gameplay completo)
- **Active abilities Q/E** (5 habilidades activas con cooldown). Bloque B.3 del MASTER_PLAN, pendiente. Convierte combate de "JK spam" a souls-like con herramientas. Schema JSON, UI HUD, balance vs Constants. ~2-3 días.
- **AI pathfinding** (Bloque B.2). A* sobre tile grid; enemigos no se atascan contra paredes. ~1-2 días.

### Añadidos a Fase 3 (calidad técnica)
- **Audio pass**: música transicionable combat/exploration, hit reactions por damage type, audio espacial básico. ~1-2 días. Coordinar con Antigravity si necesitan generar nuevos SFX (libre, ej. via ChipTone o jsfxr).
- **JSON validation**: errores claros y line-numbers cuando un data file está malformado. Reemplazar el fallback silencioso por logs útiles. ~0.5 días.
- **Gamepad support**: raylib lo soporta nativo. Detectar gamepad, mapear botones a InputManager. ~1 día.
- **Smoke tests** mínimos en `tests/`: combat math (damage + crit + resist), collision detection, save round-trip. ~1 día.

### Añadidos a Fase 5 (validación)
- **Tracy + perf budget**: capturar primer profile completo del PLAYING state. Fijar presupuesto: 16ms/frame, identificar hotspots reales. Optimizar solo lo que el profiler señale.

### Defiero explícitamente a próxima vuelta (no cabe ahora)
- Localización (Inglés)
- Steam achievements / cloud saves
- Círculos II-IX (seguimos en Círculo VII)
- Multiplayer / netcode
- Mobile / consolas

---

## REPARTO DE TRABAJO

| Fase | Owner | Coordinación |
|---|---|---|
| 1 — Lua + ImGui tuning | Claude | — |
| 2 — Camera, animation events, particles, S&S | Claude | Antigravity define events JSON cuando dispatcher esté listo |
| 3 — Cleanups, palette, split Game.cpp | Claude | — |
| 4 Opción A — Sprites de enemigos | Antigravity | Reporta a Claude vía bridge cuando entrega |
| 4 Opción B — Recorte de tilesets HD | Antigravity | Reporta a Claude vía bridge cuando entrega |
| 4 Opción C — Arenas LDtk | Antigravity | Espera a que Claude wireé loader (Fase 3 punto 5) |
| 5 — Iteración | Tú diriges | Claude ajusta feel.lua / código; Antigravity ajusta assets |

---

## RIESGOS Y MITIGACIONES

| Riesgo | Mitigación |
|---|---|
| sol2 hot-reload bugs en Win/WASM | Spike de 2-3h primer día. Si falla, replantear. |
| `feel.lua` con error → crash | try/catch + fallback a defaults hardcoded |
| Bridge async lento | Plan ya consensuado. Mínimo ping-pong. |
| Tokens limitados de Claude Pro | Commits atómicos por fase. Si se corta, queda consistente. |
| camera.zoom 1.5x revela tiles sosos | Antigravity rehace wall.png con más detalle |
| Antigravity entrega sprites con frame size custom | Código ya lo soporta vía JSON `animation` block |

---

## TIMELINE ESTIMADO

- **Semana 1:** Fase 1 (Claude) + Fase 4 Opción B inicio (Antigravity)
- **Semana 2:** Fase 2 (Claude) + Fase 4 Opción A inicio (Antigravity)
- **Semana 3:** Fase 3 (Claude) + Fase 4 Opción A completa + Opción C inicio (Antigravity)
- **Semana 4+:** Fase 5 — iteración dirigida por el usuario

---

## FIRMA

- Claude — diagnóstico, plan original, fases 1-3, 5 (lado código).
- Antigravity — debate, mejoras propuestas, fase 4, 5 (lado assets).
- Usuario — aprobación, dirección, validación final.

**Plan en vigor desde 2026-05-06. Modificable solo por el usuario o por consenso de las dos IAs.**
