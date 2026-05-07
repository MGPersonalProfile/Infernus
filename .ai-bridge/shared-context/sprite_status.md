<!-- topic:sprite_status | updated:2026-05-06T22:50:00 | by:antigravity -->

# Sprite & Tile Status — 2026-05-06

## Tiles (ACTUALIZADO - Fase 4B completada)

### Floor tiles
| Archivo | Dim | Estado | Fuente |
|---------|-----|--------|--------|
| floor.png | 64x64 | OK - HD recortado del centro | floor_hd.png (1024x1024) |
| floor_var1-6.png | 64x64 | OK - 6 variaciones | floor_hd.png |
| floor_tileset.png | 384x64 | OK - strip con 6 tiles | floor_hd.png |

### Wall tiles
| Archivo | Dim | Estado | Fuente |
|---------|-----|--------|--------|
| wall.png | 64x64 | OK - ladrillos infernales + lava | Procedural (Pillow) |
| wall_var2.png | 64x64 | OK - variacion roja | Procedural |
| wall_var3.png | 64x64 | OK - variacion oscura | Procedural |

### Decoraciones HD (nuevas)
| Archivo | Dim | Estado |
|---------|-----|--------|
| decor_pillar_hd.png | 32x64 | OK - pilar con calaveras |
| decor_altar_hd.png | 64x48 | OK - altar sangriento |
| decor_tombstone_hd.png | 32x56 | OK - tumba gotica |
| decor_torch_wall_hd.png | 24x48 | OK - antorcha en pared |
| decor_lava_crack.png | 64x64 | OK - overlay transparente |

### Decoraciones antiguas (siguen funcionales)
| Archivo | Dim | Estado |
|---------|-----|--------|
| decor_crack.png | 64x64 | OK |
| decor_blood.png | 64x64 | OK |
| decor_bones.png | 64x64 | OK (via PixelArtGenerator) |
| decor_rune.png | 64x64 | OK (via PixelArtGenerator) |

## Player sprites (sin cambios)
| Clip | Frames | Dim por frame |
|------|--------|---------------|
| IDLE | 6 | 32x48 |
| RUN | 8 | 32x48 |
| ATTACK | 6 | 32x48 |

## Enemigos (PENDIENTE Fase 4A - proxima tarea)
| Tipo | Clips | Frames actuales | Target |
|------|-------|-----------------|--------|
| melee | idle/run/attack | 4-6 | 8-10 |
| ranged | idle/run/attack | 4-6 | 8-10 |
| assassin | idle/run/attack | 4-6 | 8-10 |
| tank | idle/run/attack | 4-6 | 8-10 |
| bomber | idle/run/attack | 4-6 | 8-10 |

## IMPORTANTE
- TEXTURE_FILTER_POINT en todo (nearest neighbor)
- Tiles son 64x64, player es 32x48
- ResourceManager tiene bypass para floor.png y decor_bones/rune via PixelArtGenerator
