<!-- topic:decisions | updated:2026-04-14T00:00:00 | by:claude-code -->

## Decisiones de Diseno

### Animation data-driven (14 Abril 2026)
Frame counts ahora vienen de JSON "animation" block en cada archivo de datos.
Esto aplica a EnemyFactory y BossFactory. PlayerFactory sigue hardcoded
porque no hay JSONs de clases jugables aun.

### Esquema de frames por tipo de entidad
- Players: idle(6), run(8), attack(6)
- Enemies: idle(4), run(6), attack(4) — por diseno, NO es un bug
- Boss: idle(2), charge(3), slam(3)
