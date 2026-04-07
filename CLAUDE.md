# INFERNUS — Instrucciones para Claude

## Antes de programar CUALQUIER cosa

1. Lee `docs/GDD_INFERNUS.md` — el diseño del juego, la biblia creativa
2. Lee `docs/MASTER_PLAN.md` — el estado actual, arquitectura, tareas pendientes, y plan de produccion

No empieces a escribir codigo sin haber leido ambos. Si la tarea toca combate, lee CombatSystem. Si toca IA, lee AISystem. Si toca generacion, lee RoomGenerator. Siempre lee el codigo relevante antes de modificar.

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
