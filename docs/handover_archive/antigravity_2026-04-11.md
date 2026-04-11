# Antigravity → Claude — Handover (11 Abril 2026)

## 1. Archivos Generados

| # | Archivo | Dimensiones | Confirmado |
|---|---------|-------------|------------|
| 1 | `assets/sprites/player/rogue_idle.png` | 288×224 | ✅ |
| 2 | `assets/sprites/player/knight_idle.png` | 288×224 | ✅ |
| 3 | `assets/sprites/bosses/minotaur_idle.png` | 160×80 | ✅ |
| 4 | `assets/sprites/enemies/demon_idle.png` | 32×32 | ✅ |
| 5 | `assets/sprites/enemies/lancer_idle.png` | 24×48 | ✅ |
| 6 | `assets/sprites/enemies/brute_idle.png` | 48×48 | ✅ |
| 7 | `assets/sprites/enemies/assassin_idle.png` | 24×40 | ✅ |
| 8 | `assets/sprites/enemies/bomber_idle.png` | 32×32 | ✅ |

Todos son PNG RGBA con fondo 100% transparente (alpha=0). Sin antialias, sin gradientes AI. Puros pixeles discretos.

## 2. Método Usado

**Script Python con Pillow** — `tools/gen_sprites.py`

Cada sprite fue dibujado pixel a pixel en grids 2D de indices de paleta, escalados con nearest-neighbor (`Image.NEAREST`) a las dimensiones finales. Cero AI image generation.

Para regenerar:
```bash
python tools/gen_sprites.py
```

Resoluciones nativas (antes de escalado):
- Players: 16×19 px/frame → ×3 → 48×57 → centrado en 48×56
- Minotaur: 20×20 px/frame → ×4 → 80×80
- Demon: 16×16 → ×2 → 32×32
- Lancer: 12×24 → ×2 → 24×48
- Brute: 16×16 → ×3 → 48×48
- Assassin: 12×20 → ×2 → 24×40
- Bomber: 16×16 → ×2 → 32×32

## 3. Paletas por Grupo (hex)

### Players (tonos heroicos, warm undertones)

**Compartido:**
- Outline: `#0F0C12`
- Skin base: `#C3A082`
- Skin shadow: `#916E55`
- Skin highlight: `#DCBE9B`

**Rogue (11 colores):**
| Idx | Hex | Uso |
|-----|---------|-----|
| 2 | `#371E48` | Capa púrpura oscura |
| 3 | `#52306C` | Capa púrpura media |
| 4 | `#734896` | Púrpura highlight |
| 6 | `#FF3232` | Ojos rojos brillantes |
| 7 | `#C8C8D7` | Daga blade |
| 8 | `#826446` | Daga grip |
| 9 | `#23192A` | Sombra capucha profunda |
| 10 | `#41305` | Pliegue de capa |

**Knight (11 colores):**
| Idx | Hex | Uso |
|-----|---------|-----|
| 2 | `#967323` | Gold base desgastado |
| 3 | `#C8A83C` | Gold medio |
| 4 | `#F0D26E` | Gold highlight |
| 5 | `#F5F0EB` | Pluma blanca |
| 6 | `#D2CDC3` | Pluma shadow |
| 8 | `#BEC3D2` | Blade steel |
| 9 | `#A0A5AF` | Steel shadow |
| 10 | `#785A19` | Gold deep shadow |
| 11 | `#646473` | Shield face |

### Enemigos (tonos amenazantes, cold/sickly undertones)

**Compartido:**
- Outline: `#0A0805`
- Red eye: `#EB2828`
- Yellow eye: `#FADC32`
- Blood: `#780F0A`

**Demon:** rojo sangre (`#A01C16`, `#C83228`, `#781E0C`), garras blancas `#E1DCD2`, fauces `#3C0805`, colmillos `#F0E6D7`

**Lancer:** marrón oscuro horse (`#55371E`, `#7A522D`, `#9B6E41`), skin torso `#AF8C69`, arco wood `#4E3016`, bowstring `#C8C3B4`

**Brute:** verde enfermizo (`#374828`, `#506937`, `#698746`), pus `#C8AF32`, mace rust `#784B23`, sombra profunda `#34341C`

**Assassin:** plumas oscuras (`#261E34`, `#3E324E`, `#554469`), talones hueso `#D7C8AF`, blood drip `#B41914`, pico `#C8B496`

**Bomber:** gris agrietado (`#505A50`, `#69696E`, `#3C3A41`), naranja cracks `#DC8219`, core amarillo `#FFC832`, orange brillo `#FF8C1E`

### Boss — Minotauro (14 colores)

- Fur: `#52341C` / `#734E2A` / `#8C5F37`
- Horn cream: `#EBD7AA` / shadow `#C8B991`
- Eyes: demon red `#EB2828`
- Nose ring bronze: `#C39B2D`
- Axe steel: `#9B9BAA` / shadow `#7D7D8C`
- Axe wood: `#5F3C1E`
- Blood on axe: `#B41E14`
- Scar tissue: `#AA6E46`
- Deep shadow fur: `#3C2312`

## 4. Notas Visuales para Cableado

### Players (rogue_idle.png, knight_idle.png)

**Layout de spritesheet (idéntico para ambos):**
- 6 columnas × 4 filas, cada celda 48×56 px
- **Row 0 (Idle):** 2 frames (cols 0-1). Frame 1 = pose base, Frame 2 = bob sutil (1px shift)
- **Row 1 (Walk):** 6 frames (cols 0-5). Cycle loop completo
- **Row 2 (Attack):** 3 frames (cols 0-2). F0=windup, F1=strike, F2=recover
- **Row 3 (Dash):** 2 frames (cols 0-1). Pose inclinada + cloak trailing

**Rogue específico:**
- Las dagas aparecen en cols 2 y 12 del sprite nativo (x=6-8 y x=36-40 en pixels finales del frame)
- En ataque F1 (strike), las dagas se cruzan hacia delante-abajo (rows 11-14 nativas → y≈33-42 final)
- Ojos rojos están en row 4 de datos nativos, cols 6 y 9 → visibles a x=18 y x=27 en frame final
- La capucha tiene deep shadow (idx 9) que da profundidad

**Knight específico:**
- Espada en mano derecha (lado DERECHO del sprite, cols 13-14 nativos → x=39-42 final)
- Escudo en mano izquierda (lado IZQUIERDO, cols 1-2 nativos → x=3-6 final)
- En ataque F2 (swing), la espada se extiende horizontal hacia la derecha: cols 11-15 nativos → x=33-47 final. 4 pixeles nativos de blade = 12px finales de arco de corte
- Pluma blanca sobresale por arriba (rows 0-1)
- Dash = escudo al frente comprimiendo el cuerpo de izquierda  

### Minotaur Boss (minotaur_idle.png)

- 2 frames de idle, cada uno 80×80
- Frame 0 en x=0..79, Frame 1 en x=80..159
- Frame 2 tiene hombros ligeramente expandidos (breathing animation)
- Cuernos blancos prominentes arriba (rows 0-2 nativas → y=0-8 final)
- Hacha a la derecha (cols 16-18 nativos → x=64-72 final), con manchas de sangre (idx 12)
- Aro de bronce en el hocico visible (row 5 nativa, cols 8-9 → x=32-36, y=20 final)
- Cicatrices en pecho (idx 14) rows 9-10 nativas → visibles centralmente

### Enemigos (single frame cada uno)

**Demon (32×32):** Cuernos asimétricos arriba, garras blancas a los lados (row 8 nativa). Boca abierta con colmillos row 6. Cuerpo compacto musculoso.

**Lancer (24×48):** Torso humanoide arriba (rows 0-10), cuerpo de caballo abajo (rows 11-22). El arco está a la izquierda (cols 1-2). La flecha/bowstring corre vertical por col 2. Silueta vertical alta y delgada.

**Brute (48×48):** Cuerpo masivo, llena la mayor parte del frame. Mace a la derecha (cols 14-15 nativos → x=42-45 final), arrastrando (rows 7-10). Pústulas amarillas (idx 6) en rows 6, 10 del cuerpo. Mandíbula colgante visible.

**Assassin (24×40):** Alas desplegadas (rows 6-10, cols 0-1 y 10-11). Talones de hueso blanco abajo con sangre (rows 15-17). Silueta con envergadura visible. Plumas oscuras negro-violáceas.

**Bomber (32×32):** Forma esférica centrada. Core naranja-amarillo brillante en el centro (rows 6-9, cols 6-9 nativos). Las grietas grises irradian del centro. Ojos rojos pequeños row 3. Brazos stub (muy cortos), patas stub abajo.

## 5. Lo que NO se pudo hacer / Limitaciones

1. **Warrior (guerrero)** — NO incluido porque el brief especificaba solo rogue y knight. El warrior existente sigue siendo interceptado por `PixelArtGenerator::getWarrior()`. Si quieres que haga uno, avísame.

2. **Resolución nativa baja** — Los sprites base son 16x16/16x19/12x20/12x24/20x20 escalados a las dimensiones finales. Esto es pixel art a tamaño de juego estilo NES/SNES. Si el juego renderiza estos sprites a tamaños mayores en pantalla, los pixeles serán claramente visibles (que es lo que queríamos). Asegúrate de que el `SetTextureFilter(tex, TEXTURE_FILTER_POINT)` esté activo al cargar (ya lo tienes en ResourceManager para los fallback generated).

3. **Animaciones de ataque del Rogue y Knight son sutiles** — Los frames de ataque muestran la acción pero las poses están limitadas por la resolución de 16x19 nativos. Si necesitas más drama en la animación, puedo hacer una segunda pasada con más frames de diferencia o mayor resolución base.

4. **Solo 2 frames de idle para Minotaur** — Suficiente para breathing. Si necesitas charge, ground_slam, etc. para el boss, eso requiere una spritesheet mucho más grande que haría falta coordinar.

## 6. Para que todo funcione

Cuando borres las intercepciones de `ResourceManager.h`:
- `rogue_idle.png`, `knight_idle.png` → caerán al `LoadTexture()` natural y cargarán los PNGs
- `minotaur_idle.png` → ídem
- Los 5 enemigos → ídem

**IMPORTANTE:** El ResourceManager actual hace `SetTextureFilter(tex, TEXTURE_FILTER_POINT)` solo para los sprites generados proceduralmente. Cuando caiga al `LoadTexture()` natural (línea 45), NO se aplica `TEXTURE_FILTER_POINT`. Necesitas añadir eso después del `LoadTexture` para que los sprites nuevos no se vean borrosos:

```cpp
// Línea 44-45 de ResourceManager.h, cambiar:
} else {
    textures[path] = LoadTexture(path.c_str());
}
// A:
} else {
    textures[path] = LoadTexture(path.c_str());
    SetTextureFilter(textures[path], TEXTURE_FILTER_POINT);
}
```

Si no haces esto, Raylib usará TEXTURE_FILTER_BILINEAR por defecto y los sprites se verán anti-aliased en pantalla.

---

*Generado por Antigravity — 11 de Abril de 2026*
