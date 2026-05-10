# INFERNUS — Estado actual del proyecto
## Última actualización: 2026-05-09
## Mantenido por Claude. Sincronizado vía bridge para Antigravity.

> **Si eres Claude o Antigravity entrando a una nueva sesión, lee este archivo
> primero.** Es la fuente de verdad sobre dónde estamos.

---

## TL;DR

INFERNUS es un roguelite souls-like 2D ambientado en el Círculo VII del Infierno
de Dante. Construido por **dos IAs colaborando** (Claude = C++/sistemas/gameplay,
Antigravity = arte/sprites/audio/QA), dirigido por **Juan Miguel** (no programa,
valida y guía).

Estado: **jugable, en pulido**. Hemos pasado de prototipo a build con sistemas
profesionales (hot-reload Lua, ImGui tuning panel, A* pathfinding, animation
events JSON, audio ducking, LDtk arenas, dual-agent bridge v3).

---

## Documentación clave (lee en este orden)

1. **`docs/GDD_INFERNUS.md`** — biblia creativa (ambientación, enemigos, lore)
2. **`docs/MASTER_PLAN.md`** — fases de producción, bloques A-D, contenido
3. **`joint_plan.md`** — plan de fixing de calidad firmado entre ambas IAs
4. **`claude_analysis.md`** — diagnóstico arquitectónico de Claude
5. **`BUGS_AUDIT.md`** — 28 bugs catalogados de la última auditoría visual
6. **`.ai-bridge/docs/BRIDGE_PROTOCOL.md`** — protocolo unificado del bridge v3
7. **`.ai-bridge/BRIDGE_V3_PLAN.md`** — plan de evolución del bridge

---

## Última sesión completada (commits hasta `7e4ffa5`)

### Sesión de FIX (auditoría → resolver)

**Mi parte (Claude — 8 fixes):**
- A.3 cap 5 ambient decor por sala (era 17)
- D.2 pillar/tombstone bloquean (collider sólido)
- D.4 player collider centrado horizontal + anclado a pies
- B.1 Q/E ability names truncate + letra grande fallback
- C.1 **eliminada tecla L** — class ultimate ahora en Q/E (resolvió queja "L y Q hacen lo mismo")
- E.1 wireadas 4 partículas con texturas reales (blood/dust/fire/shockwave)
- B.4-B.6 HP bar 360x28, stamina 240x16, Q/E pulsa dorado cuando ready
- A.5 deferida (torch HD necesita 4-frame spritesheet)

**Parte de Antigravity:**
- A.1 ✓ decor HD (pillars/altars/tombstones rehechos)
- A.2 ✓ player sprites re-tinted (knight ya no amarillo)
- B.2 ⚠️ parcial — 3/5 iconos OK, 2 con poco alpha (`lanza_flegetonte`, `paso_sombrio`)

**Iniciativa de Antigravity sin pre-aviso (aceptada + refactorizada):**
- Sistema de telemetría QA E2E
- Movido a `src/debug/Telemetry.h` (era inline en Game_StateMachine)
- `tools/test_gameplay.py` lee `telemetry.jsonl` (Python E2E)
- Activado por env var `INFERNUS_TEST=1`

### Bridge v3 (mejoras del sistema mismo de comunicación)

- 23 tools (era 17 en v2)
- Auto-archive en startup, log rotation a 1MB
- Nueva: `prune_inbox`, `bridge_search`, `bridge_health`, `snapshot_plans`
- Dedup con SequenceMatcher en `request_antigravity` (override `force=True`)
- Tags en tareas + filtro
- Doc unificada `BRIDGE_PROTOCOL.md` (deprecó split docs)
- Política formalizada: Antigravity puede tocar C++ SOLO para QA/testing con
  pre-aviso, módulo dedicado, opt-in runtime

---

## Estado del build

- ✅ Compila limpio (MinGW + GCC 15.2 + CMake)
- ✅ 7/7 smoke tests pasan (`tests/test_features.cpp`: A* pathfinding,
  ActiveAbilities cooldown, JsonLoader)
- ✅ WASM build funcional (Emscripten)
- ✅ Tracy/ImGui/Lua/LDtk/libpartikel todos integrados y wireados

Comando build: `cmake -S . -B build -G "MinGW Makefiles" && mingw32-make -C build -j8`

---

## Trabajos en curso

### Antigravity (1 task abierta)
- `task_1778357478480` — re-do 2 iconos de abilities con alpha real
  (`lanza_flegetonte` y `paso_sombrio` aún tienen fondo casi opaco)

### Claude (próximas)
✅ **Telemetría headless implementada** — `INFERNUS_HEADLESS=1` o `--headless`
   skipea InitWindow, audio device, render, GL textures, input queries.
   Solo corre la lógica + telemetry. ResourceManager + InputManager
   tienen `setHeadless(true)` flag.
✅ **Auto-quit timer** — `INFERNUS_TEST_DURATION=60` o `--duration 60`
   termina el juego limpiamente tras N segundos.
✅ **Eventos discretos** en telemetry — `Telemetry::event(name, json)`
   cableado en CombatSystem (`hit_dealt`) y RoomFlow (`room_entered`).
   Snapshots periódicos siguen a 5Hz.

⚠️ **Sandbox test bloqueado** — el bash MSYS2 de Claude Code falla con exit 127
   al ejecutar `build/INFERNUS.exe --headless`. El binary es PE32+ Windows;
   parece que MSYS no propaga argv/env al sub-proceso correctamente. NO es
   un bug del código headless — el .exe sin args sí arranca (la ventana
   raylib se ve pero kill bg fuerza salida).

   **Próximo paso:** que Juan Miguel o Antigravity ejecuten desde un cmd.exe
   nativo / PowerShell / bash WSL real:
   ```
   cd Roguesouls-like
   build\INFERNUS.exe --headless --duration 5
   type telemetry.jsonl
   ```
   Si genera líneas válidas, el feature funciona. Me suben el archivo y yo
   lo analizo.

### Pending de fix sessions futuras (BUGS_AUDIT.md P2-P3)
- A.5 swap torch HD (necesita Antigravity entregar 4-frame spritesheet)
- D.5 swept collision (tunneling en dash a alta velocidad)
- D.6 input buffer / coyote time
- C.2 consolidar TAB/I/H (demasiadas teclas para info menus)
- G.1 tutorial básico
- G.2 feedback visual de stamina-low
- E.2-E.4 limpieza de sprites huérfanos (legacy player_*.png, warrior_spritesheet)

---

## Cómo comunicarnos

**Claude → Antigravity:**
- Bridge: `request_antigravity(task, context, priority, tags)`
- Para mensajes informativos no-bloqueantes: `claude_to_antigravity.md` raíz

**Antigravity → Claude:**
- Bridge: `send_task_to_claude` o `process_inbox.py complete <id>`
- Para mensajes informativos: `antigravity_to_claude.md` raíz

**Ambos → Juan Miguel:**
- Commits descriptivos con `Co-Authored-By` claro
- Archivos `.md` en raíz del repo se ven con git diff
- Este `PROJECT_STATUS.md` se actualiza al final de cada sesión grande

---

## Próximo punto de retoma

**Si Claude continúa:** seguir con telemetría headless (mismo archivo). El plan
detallado:
1. Probar si `SetConfigFlags(FLAG_WINDOW_HIDDEN)` permite arrancar el juego
   sin display visible en mi sandbox
2. Si no funciona, stubear las llamadas raylib problemáticas con
   `#ifdef INFERNUS_HEADLESS`
3. Wirear `--headless` flag en main.cpp + env var fallback
4. Probar lanzamiento desde el sandbox bash + leer telemetry.jsonl

**Si Antigravity continúa:** task `task_1778357478480` — re-do 2 iconos.
Después de eso, espera más fix sessions o iniciativas propias notificadas.

**Si Juan Miguel continúa:** la decisión más útil es ejecutar el juego con
los últimos fixes y reportar qué se siente. Especialmente:
- ¿Funcionan las habilidades Q/E con los iconos?
- ¿Hitbox del player se siente preciso después del centering?
- ¿La saturación de altares ya no es problema?
- ¿Las arenas LDtk se cargan? (mira el log `INFO: ROOM: loaded handcrafted ...`)
