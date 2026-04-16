<!-- topic:general | updated:2026-04-14T00:00:00 | by:claude-code -->

## Sesion de Debugging Intensivo — 13 Abril 2026

### Bugs encontrados y arreglados por Claude (codigo):
1. **Entity spam en boss charge** — Throttleado a 1 cada 0.1s
2. **ashParticles nunca se limpiaba** — Ahora se limpian en spawnRoom() y startGame()
3. **startGame() no reseteaba estado previo** — Arreglado
4. **checkAABB ignoraba collider offsets** — Cambiado a CheckCollisionRecs()
5. **return en vez de continue en BossAISystem** — Arreglado
6. **ECS isAlive() era O(n)** — Ahora O(1) con unordered_set
7. **GetFrameTime() directo en subsistemas** — Arreglado
8. **Camera2D initializer warnings** — Arreglado

### Sesion 14 Abril 2026
9. **Player IDLE frame count** — Corregido 2→6 en PlayerFactory
10. **MiniBossAISystem crash si player muere** — isAlive guards
11. **AISystem/BossAISystem dangling target** — isAlive guards
12. **RoomGenerator spawn silencioso** — Fallback determinista
13. **Animation data-driven** — EnemyFactory y BossFactory leen de JSON
