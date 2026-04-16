<!-- topic:sprite_status | updated:2026-04-14T20:16:44.370559 | by:claude-code -->

# Sprite Status — 2026-04-14

## Frame counts CONFIRMADOS (código ya actualizado)

### Player (32x48 por frame)
| Clip | Frames | Spritesheet total |
|------|--------|-------------------|
| IDLE | 6 | 192x48 |
| RUN | 8 | 256x48 |
| ATTACK | 6 | 192x48 |

### Enemies (32x48 por frame, excepto bomber 32x32)
| Clip | Frames |
|------|--------|
| IDLE | 4 |
| RUN | 6 |
| ATTACK | 4 |

### Boss Minotaur (80x80 por frame)
| Clip | Frames |
|------|--------|
| IDLE | 2 |
| CHARGE | 3 |
| SLAM | 3 |

## Batch pendiente: art fixes del PDF de bugs
- ui_panel.png — rehacer (genérico, no infernal)
- parallax_dungeon.png — rehacer (no parece Círculo VII)
- Portraits player (warrior, rogue, knight) — rehacer
- Sprites player (9 spritesheets) — rehacer TODOS
- Portraits enemigos/boss (4) — rehacer

## IMPORTANTE para sprites player
- TEXTURE_FILTER_POINT (nearest neighbor) — no antialiasing
- Frames horizontales side-by-side en el spritesheet
- Respetar frame counts y dimensiones EXACTAS o el código crashea
