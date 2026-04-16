# HANDOVER COMPLETO: Claude -> Antigravity
## Fecha: 2026-04-17 | Prioridad: CRITICA
## Este archivo es tu biblia visual. Leelo ENTERO antes de tocar nada.

---

# PARTE 1: ESTADO ACTUAL DE TODOS LOS ASSETS

## 1.1 Art (assets/art/) — Ilustraciones de alta resolución

| Archivo | Dims | Estado | Notas |
|---------|------|--------|-------|
| title_bg.png | 1024x1024 | APROBADO | Excelente. Paisaje infernal con catedral, lava, caballero. NO TOCAR. |
| ui_panel.png | 1024x1024 | APROBADO | Panel con hueso/lava/ornamentos. Se usa como fondo de TODOS los menús. |
| parallax_dungeon.png | 1024x1024 | APROBADO | Río de sangre, árboles muertos, cielo tóxico. Fondo parallax. |
| portrait_warrior.png | 1024x1024 | APROBADO | Armadura pesada, hacha, cuernos. Paleta infernal correcta. |
| portrait_knight.png | 1024x1024 | APROBADO | Caballero caído, armadura ornamentada oscura. |
| portrait_rogue.png | 1024x1024 | APROBADO | v2 corregida. Dagas de fuego, fondo lava. |
| portrait_infernal_knight.png | 1024x1024 | APROBADO | Armadura con grietas de lava, ojos rojos. |
| portrait_pit_fiend.png | 1024x1024 | APROBADO | Demonio con cuernos y fuego. |
| portrait_minotaur.png | 1024x1024 | APROBADO | Pixel art, toro oscuro. |
| portrait_soul_archer.png | 1024x1024 | APROBADO | v2 corregida. Esqueleto con arco de fuego en catacumbas de lava. |
| palette_reference.png | 256x64 | OK | Referencia de paleta. |

## 1.2 Player Sprites (assets/sprites/player/) — CRITICO, RECHAZADOS

**TODOS LOS SPRITES DEL PLAYER SON INACEPTABLES Y NECESITAN REHACERSE.**

Dos intentos fallidos:
1. Primer intento: blobs oscuros sin detalle (~400 bytes), programáticos
2. Segundo intento: "modelo híbrido" (downscale de AI) — manchas horizontales irreconocibles

**Lo que existe ahora (todo RECHAZADO):**

| Archivo | Dims | Frames | Estado |
|---------|------|--------|--------|
| knight_idle.png | 192x48 | 6x32x48 | RECHAZADO — manchas grises |
| knight_run.png | 256x48 | 8x32x48 | RECHAZADO — rayas horizontales |
| knight_attack.png | 192x48 | 6x32x48 | RECHAZADO — ilegible |
| warrior_idle.png | 192x48 | 6x32x48 | RECHAZADO — formas vagas doradas |
| warrior_run.png | 256x48 | 8x32x48 | RECHAZADO — manchas |
| warrior_attack.png | 192x48 | 6x32x48 | RECHAZADO — ilegible |
| rogue_idle.png | 192x48 | 6x32x48 | RECHAZADO — puntos morados |
| rogue_run.png | 256x48 | 8x32x48 | RECHAZADO — rayas |
| rogue_attack.png | 192x48 | 6x32x48 | RECHAZADO — manchas |

**Archivos legacy (NO usados por el código actual, se pueden borrar):**
- player_idle.png (96x56) — viejo sprite genérico
- player_run.png (192x56) — viejo
- player_attack.png (144x56) — viejo
- player_death.png (144x56) — viejo
- warrior_spritesheet.png (1024x1024) — viejo/no usado

### ESPECIFICACIONES EXACTAS PARA PLAYER SPRITES

**Formato obligatorio:** PNG con fondo TRANSPARENTE (RGBA). Tira horizontal de frames contiguos.

**Dimensiones por frame:** 32 pixeles ancho x 48 pixeles alto. EXACTO. Ni un pixel más ni menos.

**Rendering:** Se usa `TEXTURE_FILTER_POINT` (nearest neighbor, sin antialiasing). Los sprites se renderizan con scaling 1x en una resolución de 1280x720. Cada pixel del sprite = 1 pixel en pantalla.

**Frame counts por clip:**

| Clip | Frames | Spritesheet total | Loop? |
|------|--------|-------------------|-------|
| IDLE | 6 | 192x48 | Sí (loop continuo) |
| RUN | 8 | 256x48 | Sí |
| ATTACK | 6 | 192x48 | No (revierte a idle al terminar) |

**Velocidades de animación (segundos por frame):**
- IDLE: 0.4s/frame (lento, relajado)
- RUN: 0.1s/frame (rápido)
- ATTACK: variable según clase, ~0.08-0.1s/frame

### DISEÑO POR CLASE — Pixel art a 32x48 nativo

**IMPORTANTE: El enfoque de downscalear imágenes grandes a 32x48 NO FUNCIONA. Se necesita dibujar pixel por pixel a resolución nativa. Piensa en sprites de Shovel Knight, Celeste, o Dead Cells.**

**Técnica correcta:** Usar Python + Pillow, dibujar cada pixel explícitamente. Contorno oscuro (1-2px), relleno con color sólido, highlights de 1px para dar volumen. Sin transparencia parcial ni antialiasing — solo pixels 100% sólidos o 100% transparentes.

#### KNIGHT (Caballero Infernal)
- **Silueta:** Ancho ~16px, alto ~44px (dentro del frame de 32x48, centrado)
- **Cabeza:** Casco cerrado con visor (4-5px ancho, 5-6px alto), gris claro metálico (#A0A0A0), línea oscura para visor
- **Torso:** Armadura completa gris con cruz/emblema dorado (#DAA520) en el pecho
- **Brazo der:** Espada recta (línea de 2px ancho, ~12px largo), mango marrón, hoja gris claro
- **Brazo izq:** Escudo pequeño (4x6px), azul oscuro con borde dorado
- **Piernas:** Greaves metálicas, posición firme, separadas ~6px
- **Capa:** Roja oscura (#8B0000), cayendo por detrás (2-3px ancho)
- **Color dominante:** Gris/plata con acentos rojos y dorados
- **IDLE anim:** Frames 1-3 torso sube 1px (inhalar), 4-6 baja 1px (exhalar). Movimiento sutil de capa.
- **RUN anim:** Ciclo de piernas claro (izq→centro→der), torso inclina 1-2px adelante, brazos alternan, capa ondea
- **ATTACK anim:** Windup (espada atrás) → swing (arco de espada visible) → recovery. Arco de 90° mínimo.

#### WARRIOR (Guerrero Brutal)
- **Silueta:** Ancho ~20px (MÁS ANCHO que knight), alto ~46px
- **Cabeza:** Casco con cuernos pequeños (5x5px), marrón oscuro (#5C3A1E)
- **Torso:** Armadura pesada rojo oscuro (#8B0000), MÁS VOLUMINOSA que knight
- **Hombreras:** Grandes (2-3px extra a cada lado del torso), metálicas
- **Arma:** Hacha grande en mano derecha (forma de T, 6x12px), mango marrón, filo gris
- **Piernas:** Gruesas, botas pesadas negras, más anchas que knight
- **Sin capa** — este es un bruto, no un noble
- **Color dominante:** Rojo oscuro y marrón con metal
- **IDLE anim:** Breathing más lento (el peso). Hacha descansa en una mano.
- **RUN anim:** Carrera pesada — pasos amplios, cuerpo balancea más que knight
- **ATTACK anim:** Levanta hacha arriba → golpe contundente hacia abajo. Más lento pero más grande que knight.

#### ROGUE (Pícaro de las Sombras)
- **Silueta:** Ancho ~12px (MÁS DELGADO que todos), alto ~42px
- **Cabeza:** Capucha puntiaguda (4x6px), púrpura oscuro (#3C1361)
- **Cara:** En sombra total, solo dos puntos de ojos brillantes (rojo/naranja)
- **Torso:** Túnica/leather ajustado, negro/gris oscuro, delgado
- **Armas:** Dagas en ambas manos (líneas diagonales de 3-4px), con brillo rojo/naranja en filo
- **Piernas:** Delgadas, botas ligeras negras
- **Capa:** Más larga que knight, oscura (#1A0A2E), ondea detrás
- **Color dominante:** Negro/púrpura oscuro con destellos naranja/rojo
- **IDLE anim:** Breathing sutil + ligero movimiento de dagas (las gira). Más inquieto que los otros.
- **RUN anim:** Carrera ágil/rápida — pasos cortos, torso bajo, capa vuela detrás
- **ATTACK anim:** Corte rápido con ambas dagas en X. Más rápido que knight/warrior.

### CLAVE ABSOLUTA
Las 3 clases deben ser **INMEDIATAMENTE distinguibles** por su silueta a zoom 1x (32x48 pixels). Si las pones juntas, debes poder decir "eso es el knight, eso es el warrior, eso es el rogue" sin color — solo por la forma.

---

## 1.3 Enemy Sprites (assets/sprites/enemies/) — FUNCIONALES PERO POBRES

Las dimensiones son correctas. La calidad visual es baja — se necesitan rehacer pero con menor prioridad que player.

### Enemigos regulares (usados por EnemyFactory)

| Enemigo | Frame | idle | run | attack | Calidad |
|---------|-------|------|-----|--------|---------|
| melee | 32x48 | 4f (128x48) | 6f (192x48) | 4f (128x48) | 2/10 — triángulos rojos |
| ranged | 28x40 | 4f (112x40) | 6f (168x40) | 4f (112x40) | 2/10 — parece un mueble |
| tank | 56x64 | 4f (224x64) | 6f (336x64) | 4f (224x64) | 5/10 — mejor, humanoide visible |
| assassin | 24x40 | 4f (96x40) | 6f (144x40) | 4f (96x40) | 1/10 — cápsulas púrpura |
| bomber | 32x32 | 4f (128x32) | 6f (192x32) | 4f (128x32) | 4/10 — blob redondo OK para bomber |

**Problemas principales:**
1. Sin animación real entre frames — todos son casi idénticos con 1px breathing
2. Sin ciclo de piernas en run — los enemigos "flotan"
3. Sin movimiento de ataque visible — el jugador no puede leer windups (anti-souls-like)
4. Melee/ranged/assassin son formas abstractas, no criaturas reconocibles

**Lo que necesitan:**
- Contornos claros y siluetas humanoides/criatura reconocibles
- Animación de piernas en run (mínimo 3 poses distintas en el ciclo)
- Animación de ataque visible (windup → golpe → recovery)
- Cada tipo debe ser inmediatamente distinguible por silueta

**Velocidades de animación (JSON):**
- idle: 0.15 s/frame
- run: 0.1 s/frame
- attack: 0.08 s/frame

**Personalidad visual por tipo:**
- MELEE (Alma Violenta): espectro rojo agresivo, espada/garra, ataque frontal
- RANGED (Centauro Esqueleto): criatura con arco, dispara a distancia, 28x40
- TANK (Demonio Blindado): grande (56x64), pesado, armadura gruesa, golpe lento
- ASSASSIN (Sombra): pequeño (24x40), rápido, dagas, sigiloso
- BOMBER (Criatura de Lava): redondo (32x32), se infla antes de explotar

### Boss Minotauro (los MEJORES sprites del juego)

| Archivo | Dims | Frames | Calidad |
|---------|------|--------|---------|
| minotaur_idle.png | 480x80 | 6x80x80 | 7/10 — silueta clara, breathing visible |
| minotaur_charge.png | 320x80 | 4x80x80 | 7/10 — inclinación progresiva |
| minotaur_slam.png | 480x80 | 6x80x80 | 7/10 — indicador de impacto en suelo |

**NO TOCAR** estos sprites — son los mejores que tenemos. Si algún día se rehacen, mantener la misma calidad mínima como referencia.

### Miniboss Sprites (usados por MiniBossFactory)

Los minibosses reusan sprites de 3 tipos base escalados x1.8:

| Tipo | Sprite base | Frame size | Archivo |
|------|-------------|------------|---------|
| melee | demon_idle.png | 32x32 | demon_*.png |
| ranged | lancer_idle.png | 24x48 | lancer_*.png |
| tank | brute_idle.png | 48x48 | brute_*.png |

Estos sprites son legacy (Apr 5) y no se actualizaron. Necesitan el mismo tratamiento que los enemigos regulares. Los archivos tienen clips: idle, move (not run), attack, death.

**NOTA:** Los minibosses NO tienen animation block en sus JSONs. La factory hardcodea las dimensiones. Si rehaces estos sprites, MANTENER las dimensiones exactas o avisarme para que actualice el código.

### Sprites legacy (NO usados, se pueden ignorar o borrar):
- assassin_move.png (72x40) — viejo, ahora se usa assassin_run.png
- bomber_move.png (96x32) — viejo, ahora se usa bomber_run.png

---

## 1.4 Tiles (assets/sprites/tiles/)

**TILE_SIZE = 64px** en el código. Los tiles se dibujan como cuadrados de 64x64 en el mapa.

| Archivo | Dims | Estado | Uso |
|---------|------|--------|-----|
| floor.png | 64x64 | POBRE | Cuadrado marrón oscuro casi sólido. Necesita textura de piedra/lava |
| wall.png | 64x64 | MEDIOCRE | Bloques de ladrillo marrón. Funcional pero aburrido |
| floor_hd.png | 1024x1024 | NO USADO | Existe pero el código NO lo carga. Hermoso pixel art de piedra con grietas de lava |
| decor_hd.png | 1024x1024 | NO USADO | Existe pero el código NO lo carga. Columna, altar, tumba, antorcha en pared — excelente |
| decor_blood.png | 64x64 | OK | Mancha de sangre decorativa |
| decor_bones.png | 64x64 | OK | Huesos decorativos |
| decor_crack.png | 64x64 | OK | Grieta en el suelo |
| decor_rune.png | 64x64 | OK | Runa decorativa |
| fire_trap.png | 128x64 | OK | Trampa de fuego (2 frames de 64x64) |
| pillar.png | 32x48 | OK | Pilar/columna |
| pit.png | 64x64 | OK | Pozo/agujero |
| spikes.png | 64x64 | OK | Pinchos |
| torch.png | 64x32 | OK | Antorcha de pared |

**PROBLEMA IMPORTANTE:** floor_hd.png y decor_hd.png son excelentes pero NO se usan en el juego. Son de 1024x1024 y el código espera tiles de 64x64. Opciones:
1. Recortarlos en tiles de 64x64 (tileset)
2. Rehacer floor.png y wall.png con calidad similar pero a 64x64 nativo

**Lo que necesita floor.png:** Piedra volcánica negra/gris oscuro con sutiles grietas rojas/naranjas de lava. NO un cuadrado sólido marrón.

**Lo que necesita wall.png:** Piedra tallada oscura con textura visible, quizás musgo seco o marcas de quemado. Bordes más oscuros para definir la pared.

---

## 1.5 Partículas (assets/sprites/particles/)

| Archivo | Dims | Frames | Estado |
|---------|------|--------|--------|
| ash_particle.png | 32x8 | 4x8x8 | OK — brasa que se apaga |
| blood_drop.png | 24x8 | 4x6x8 | OK — gota de sangre |

Estos están listos pero el código de partículas NO está completamente wired aún. Cuando Claude vuelva lo conectará.

---

# PARTE 2: SISTEMA DE ANIMACIÓN — CÓMO FUNCIONA EL CÓDIGO

Esto es CRÍTICO para que los sprites que generes funcionen correctamente.

## 2.1 Formato de spritesheet

```
[Frame0][Frame1][Frame2][Frame3]...
```

- Tira HORIZONTAL, frames contiguos de izquierda a derecha
- Cada frame tiene EXACTAMENTE frameWidth x frameHeight pixels
- El spritesheet total tiene: (frames * frameWidth) x frameHeight
- Fondo TRANSPARENTE (RGBA, alpha = 0 donde no hay sprite)
- SIN padding entre frames, SIN bordes, SIN espacios

## 2.2 Cómo el código lee los sprites

```
sourceRect.x = currentFrame * frameWidth   // avanza horizontalmente
sourceRect.y = 0                            // siempre fila 0
sourceRect.width = frameWidth
sourceRect.height = frameHeight
```

Si el spritesheet tiene dimensiones incorrectas, el juego:
- Mostrará frames cortados si es muy angosto
- Mostrará espacio vacío si es muy ancho
- Distorsionará el sprite si la altura no coincide

## 2.3 Clips de animación

Cada entidad tiene un AnimState con clips (IDLE, RUN, ATTACK). Cuando cambia de estado:
1. Carga la textura del clip correspondiente (ej: `knight_run.png`)
2. Resetea currentFrame a 0
3. Avanza frames según frameSpeed
4. Si loop=true, vuelve a frame 0 al terminar
5. Si loop=false (ATTACK), revierte a IDLE al terminar

## 2.4 Dirección (flip)

El sprite mira a la DERECHA por defecto. El código flipea horizontalmente con `sourceRect.width = -frameWidth` cuando el personaje se mueve a la izquierda. **Diseña todos los sprites mirando a la derecha.**

---

# PARTE 3: PALETA DE COLOR — CIRCULO VII

El juego se ambienta en el Círculo VII del Infierno de Dante (violencia). La paleta OBLIGATORIA es:

### Colores primarios
- Negro profundo: #0A0A0A (fondo, sombras)
- Rojo sangre: #8B0000 (sangre, violencia)
- Naranja lava: #CC4400 (fuego, lava)
- Gris volcánico: #333333 (piedra, ceniza)

### Colores de acento
- Dorado apagado: #DAA520 (ornamentos, UI, rareza)
- Rojo brillante: #FF2200 (ojos, peligro, highlights)
- Ámbar: #DDB460 (luz de antorcha, calidez)

### PROHIBIDOS
- Verde brillante (verde solo para veneno, y oscuro: #2A4A2A)
- Azul claro/cyan (azul solo para hielo/rareza rara, oscuro: #1E3A5F)
- Rosa/magenta (nunca)
- Blanco puro (solo para flashes de impacto, nunca en sprites)

### Shader post-proceso
El juego aplica un shader CRT con:
- Vignette (oscurece bordes)
- Warm infernal tint: `mix(rgb, vec3(0.12, 0.03, 0.02), 0.08)` — empuja todo ligeramente hacia rojo/marrón
- NO hay aberración cromática (eliminada por causar blur)

---

# PARTE 4: PRIORIDADES DE TRABAJO

## P0 — CRITICO (sin esto el juego es injugable)
1. **9 player spritesheets** — Knight, Warrior, Rogue x idle/run/attack. El jugador controla un blob ahora mismo. Esto es lo que se ve el 100% del tiempo.

## P1 — ALTA (el juego funciona pero se ve mal)
2. **floor.png** (64x64) — El suelo es un cuadrado sólido marrón. Necesita textura de piedra volcánica.
3. **wall.png** (64x64) — Ladrillos genéricos. Necesita ser más oscuro/volcánico.
4. **5 enemy sprite sets** (melee, ranged, tank, assassin, bomber) — Cada uno necesita idle/run/attack con animación real. Las dimensiones actuales son correctas, solo falta calidad y animación frame-by-frame.

## P2 — MEDIA (mejoras visuales)
5. **3 miniboss sprite sets** (demon 32x32, lancer 24x48, brute 48x48) — Los idle/move/attack/death actuales son legacy y muy básicos.
6. **Integrar decor_hd.png** — Extraer los 4 objetos decorativos como sprites individuales de 64x64 para usar en el juego.

## P3 — BAJA (polish)
7. **Más partículas** — Chispas de fuego, humo, polvo de impacto
8. **Spritesheet de muerte** para player (no existe aún)
9. **Efectos de ataque** — Trail de espada, explosión de bomba, etc.

---

# PARTE 5: REGLAS DE DOMINIO

## Lo que Antigravity PUEDE hacer:
- Generar/modificar cualquier archivo en `assets/`
- Generar/modificar shaders en `src/shaders/`
- Escribir scripts Python en `tools/` o `.ai-bridge/scratch/`
- Actualizar contexto compartido via bridge

## Lo que Antigravity NO PUEDE hacer:
- Editar archivos C++ en `src/` (excepto `src/shaders/`)
- Editar CMakeLists.txt
- Editar archivos JSON de gameplay (enemies, bosses, abilities, items, synergies)
- Modificar la arquitectura ECS
- Cambiar frame counts en los JSON (eso es trabajo de Claude)

Si necesitas que el código cambie algo (ej: nueva dimensión de sprite, nuevo clip de animación), escríbelo en una tarea al bridge o en `antigravity_to_claude.md` y Claude lo implementará.

---

# PARTE 6: ERRORES COMUNES A EVITAR

1. **NO uses antialiasing ni transparencia parcial** — TEXTURE_FILTER_POINT amplifica cualquier pixel semi-transparente y se ve horrible
2. **NO downscalees imágenes grandes a pixel art** — Se convierte en manchas ilegibles
3. **NO cambies las dimensiones de frame** sin coordinar con Claude — el código crashea
4. **NO generes sprites con fondo NO transparente** — El fondo del spritesheet DEBE ser alpha=0
5. **NO uses scripts que generan "breathing" como única animación** — 1px de offset vertical no es animación, es un tic
6. **NO mezcles estilos** — Si un portrait es pintura digital realista (1024x1024), TODOS los portraits deben serlo. Si un sprite es pixel art 32x48, TODOS los sprites del mismo tipo deben serlo.

---

# PARTE 7: INVENTARIO COMPLETO DE ARCHIVOS

## Archivos que el código CARGA activamente:

### Player (PlayerFactory.h)
```
assets/sprites/player/{class}_idle.png    — 192x48 (6 frames)
assets/sprites/player/{class}_run.png     — 256x48 (8 frames)  
assets/sprites/player/{class}_attack.png  — 192x48 (6 frames)
donde {class} = knight, warrior, rogue
```

### Enemigos (EnemyFactory.h, lee frame sizes de JSON)
```
assets/sprites/enemies/{type}_idle.png
assets/sprites/enemies/{type}_run.png
assets/sprites/enemies/{type}_attack.png
donde {type} = melee(32x48), ranged(28x40), tank(56x64), assassin(24x40), bomber(32x32)
```

### Boss (BossFactory.h, lee frame sizes de JSON)
```
assets/sprites/enemies/minotaur_idle.png    — 480x80 (6 frames de 80x80)
assets/sprites/enemies/minotaur_charge.png  — 320x80 (4 frames de 80x80)
assets/sprites/enemies/minotaur_slam.png    — 480x80 (6 frames de 80x80)
```

### Minibosses (MiniBossFactory.h, hardcodeado)
```
assets/sprites/enemies/demon_idle.png   — 32x32 frame (tipo melee)
assets/sprites/enemies/lancer_idle.png  — 24x48 frame (tipo ranged)
assets/sprites/enemies/brute_idle.png   — 48x48 frame (tipo tank)
+ _move.png, _attack.png, _death.png para cada uno
```

### Tiles (RoomGenerator.cpp)
```
assets/sprites/tiles/floor.png  — 64x64 (tile de suelo)
assets/sprites/tiles/wall.png   — 64x64 (tile de pared)
+ decor_*.png, fire_trap.png, pillar.png, pit.png, spikes.png, torch.png
```

### UI/Art (Game.cpp)
```
assets/art/title_bg.png                — fondo de título y game over
assets/art/ui_panel.png                — panel de menús
assets/art/parallax_dungeon.png        — fondo parallax gameplay
assets/art/portrait_{class}.png        — selección de personaje
assets/art/portrait_{miniboss}.png     — encuentros de miniboss
```

---

# PARTE 8: BUILD Y TEST

```bash
cmake -S . -B build -G "MinGW Makefiles" && mingw32-make -C build -j8
build/INFERNUS.exe
```

Para verificar sprites visualmente sin compilar, puedes abrir los PNG en cualquier visor con zoom y verificar:
1. Dimensiones exactas
2. Fondo transparente
3. Frames distinguibles
4. Siluetas legibles a 1x

---

# PARTE 9: COMUNICACION

Usa el AI Bridge para comunicarte conmigo:
- `python .ai-bridge/process_inbox.py list` — ver tareas pendientes
- `python .ai-bridge/process_inbox.py complete <task_id> "resultado"` — marcar tarea completada
- `python .ai-bridge/process_inbox.py dashboard` — estado general

Si necesitas que Claude haga algo cuando vuelva, crea una tarea:
```bash
python .ai-bridge/process_inbox.py # o escribe antigravity_to_claude.md
```

---

**RESUMEN EJECUTIVO:** El juego compila y corre. Los sistemas C++ están listos. Los portraits/art/UI están aprobados. El eslabón débil es el PIXEL ART de gameplay: player sprites (crítico), enemy sprites (alto), tiles (alto). El minotauro prueba que se puede hacer bien — úsalo como referencia de calidad mínima.
