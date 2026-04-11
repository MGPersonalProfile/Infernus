# INFERNUS — Instrucciones para Claude

## Antes de programar CUALQUIER cosa

1. Lee `docs/GDD_INFERNUS.md` — el diseño del juego, la biblia creativa
2. Lee `docs/MASTER_PLAN.md` — el estado actual, arquitectura, tareas pendientes, y plan de produccion
3. Si existe `antigravity_to_claude.md` en la raiz del proyecto, leelo ANTES de cualquier otra cosa — es el handover del otro agente (Antigravity) con trabajo visual o cambios que hizo mientras no estabas. Procesalo, aplicalo, y cuando termines muevelo a `docs/handover_archive/` con la fecha al final del nombre para que no se lea dos veces.

No empieces a escribir codigo sin haber leido ambos. Si la tarea toca combate, lee CombatSystem. Si toca IA, lee AISystem. Si toca generacion, lee RoomGenerator. Siempre lee el codigo relevante antes de modificar.

## Division de trabajo con Antigravity (otro agente IA)

El proyecto tiene dos agentes cooperando:

- **Claude (yo):** C++, sistemas ECS, gameplay logic, build, factories, JSON data, combat/AI/generation, refactors, bugs, integracion. Dueno de todo `src/` que no sea arte o shaders.
- **Antigravity (Gemini):** arte visual, spritesheets PNG, UI layout, shaders, paletas, composicion, portraits, backgrounds. Dueno de `assets/` y `src/shaders/`.

Regla: **nunca toco el area del otro sin un handover explicito**. Si necesito arte nuevo, escribo `claude_to_antigravity.md` en la raiz con el brief exacto. Si Antigravity necesita que cableye algo de codigo, lo escribe en `antigravity_to_claude.md`. Git es el canal de sincronizacion — commits en `main` son los mensajes.

Cuando Antigravity entrega arte nuevo que reemplaza codigo procedural mio en `PixelArtGenerator.cpp`, mi trabajo es:
1. Verificar que los PNG existen en `assets/sprites/...`
2. Borrar el metodo `getXxx()` correspondiente en `PixelArtGenerator.h/cpp`
3. Borrar la interception del path en `ResourceManager.h::getTexture()` para que caiga al `LoadTexture()` natural
4. Build + smoke test + commit

## Stack

- C++17, Raylib 5.x, ECS custom, JSON data-driven (nlohmann/json), CMake
- Build: `cmake -S . -B build -G "MinGW Makefiles" && mingw32-make -C build -j8`
- Ejecutar: `build/INFERNUS.exe` (desde la raiz del proyecto, necesita assets/)

## Reglas

- Todo dato de gameplay (enemigos, habilidades, items, sinergias, bosses) va en JSON, no hardcodeado
- Seguir el patron de las factories existentes (BossFactory, EnemyFactory, MiniBossFactory) para crear entidades
- Los componentes son datos puros (structs), la logica va en los sistemas
- No crear archivos .md nuevos sin permiso explicito
- Actualizar MASTER_PLAN.md cuando se complete una tarea (marcar [x])
- El usuario habla en espanol y prefiere respuestas directas y concisas
- La dificultad del juego debe ser agresivamente alta — es un souls-like, no un paseo
