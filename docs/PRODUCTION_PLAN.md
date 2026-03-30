# INFERNUS — Plan de Producción Completo

> Del estado actual (pre-alpha funcional con Círculo VII) al juego completo con 9 Círculos.
> Fecha de creación: 24 de Marzo, 2026

---

## Estado Actual del Proyecto

### Lo que existe (funcional)
- Custom ECS con deferred destruction y unique_ptr
- Jugador con movimiento, dash (i-frames), ataques ligero/pesado, stamina
- 3 arquetipos de enemigos (melee, ranged, tank) data-driven desde JSON
- Boss Minotauro con 3 fases, 5 patrones de ataque, transiciones con partículas
- Sistema de habilidades: 15 habilidades, pool desde JSON, selección entre salas
- Sistema de sinergias: 6 sinergias por tags, evaluación automática
- Generación procedural de salas: muros, obstáculos, pinchos, tamaño variable
- Selección de personaje: 3 clases (Guerrero, Pícaro, Caballero)
- Meta-progresión: SaveManager, RunStats, desbloqueos, lore
- AudioManager (framework listo, sin assets de audio)
- ScreenEffects: hitstop, flash, vignette, fade transitions
- UI: HUD, menú principal, pausa, game over, victoria, selección de habilidad

### Bugs conocidos (a corregir primero)
- **Colisiones con paredes/objetos sólidos** — el jugador y enemigos atraviesan geometría
- **Balance** — dificultad excesiva, caótico
- **Salas demasiado largas** — las runs se sienten eternas
- **El boss intro usa static local** — puede causar bugs en restarts

---

## Arquitectura del Plan

El juego completo según el GDD requiere:
- **9 Círculos** con biomas, enemigos y bosses únicos
- **6 personajes jugables** con mecánicas diferenciadas
- **50+ habilidades** con sinergias temáticas
- **9 bosses multifase** con patrones memorables
- **Pixel art** oscuro de alta resolución
- **Audio completo** (música, SFX, ambientación)
- **Meta-progresión** (desbloqueos, lore, estadísticas)

El plan se divide en **4 Bloques** secuenciales:

```
BLOQUE A: Estabilización (arreglar lo que hay)
BLOQUE B: Core Loop Completo (un círculo perfecto)
BLOQUE C: Expansión de Contenido (9 círculos)
BLOQUE D: Polish y Lanzamiento
```

---

## BLOQUE A — Estabilización

> **Meta:** Que el Círculo VII (lo que existe) funcione correctamente.
> Este bloque NO añade features nuevas. Solo arregla y pule lo existente.

### A.1 — Fix de Colisiones
- [ ] Revisar `CollisionSystem` — AABB vs tiles sólidos debe resolver empuje correctamente
- [ ] Implementar resolución de colisión por eje (resolver X y Y por separado)
- [ ] Colisión jugador-muro: push-back, no overlapping
- [ ] Colisión enemigos-muro: AI no debe pathing a través de paredes
- [ ] Colisión boss-muro: el boss no debe salirse de la arena
- [ ] Proyectiles deben destruirse al impactar muros sólidos
- [ ] Test: caminar contra cada pared de la sala, contra pilares, contra bordes

### A.2 — Balance del Círculo VII
- [ ] Reducir número de salas de 5 a 3-4 antes del boss (runs más cortas)
- [ ] Ajustar enemigos por sala: empezar con 2, escalar a 3-4 (no 2+room)
- [ ] Reducir HP de enemigos melee/ranged un 20%
- [ ] Aumentar HP del jugador base a 120 (Guerrero a 150)
- [ ] Reducir daño del Minotauro fase 1 de 25 a 18
- [ ] Aumentar ventanas de castigo del boss (recovery más largas)
- [ ] Ajustar velocidad de charge del Minotauro (de 650 a 550)
- [ ] Más loot drops: aumentar probabilidad de orbes de vida

### A.3 — Fixes Menores
- [ ] Eliminar `static float introTimer` en BOSS_INTRO (usar variable miembro)
- [ ] Limpiar warnings de compilación (unused variables en BossAISystem)
- [ ] Verificar que la transición ABILITY_SELECT → BOSS_INTRO funciona
- [ ] Verificar que morir y reintentar limpia todo el estado correctamente
- [ ] El RoomGenerator debe generar salas transitables (no bloquear caminos)
- [ ] Crear directorio `save/` automáticamente si no existe

### A.4 — Playtest del Círculo VII
- [ ] Jugar 10 runs completas del Círculo VII
- [ ] Documentar: qué se siente bien, qué se siente mal, qué es injusto
- [ ] Iterar balance basado en los resultados
- [ ] **Criterio de éxito**: Una run se completa en 5-8 minutos, la dificultad es desafiante pero justa

---

## BLOQUE B — Core Loop Completo

> **Meta:** Un Círculo perfecto que represente la experiencia final.
> Al terminar este bloque, el Círculo VII debería sentirse como un juego terminado de un solo nivel.

### B.1 — Sistema de Combate Avanzado

#### B.1.1 — Parry
- [ ] Componente: `ParryState` con timer (ventana de parry ~0.2s)
- [ ] Input: tecla dedicada (L o similar)
- [ ] Si el jugador está en estado de parry cuando recibe un golpe:
  - Anula el daño
  - Aplica stagger al atacante (1.5s)
  - Flash visual + screen shake
  - Sonido satisfactorio
- [ ] Los ataques del boss también son parryeables (excepto AoE)
- [ ] Coste de stamina del parry: 15 (menor que dash)

#### B.1.2 — Combos
- [ ] Ataque ligero encadenado: hasta 3 hits (J, J, J)
- [ ] Cada golpe sucesivo es ligeramente más rápido pero con más recovery al final
- [ ] El tercer golpe tiene más knockback
- [ ] Ataque pesado después de combo ligero = finisher (más daño, más windup)

#### B.1.3 — Daño por tipo elemental
- [ ] `DamageType` ya existe (PHYSICAL, FIRE, ICE, LIGHTNING, TOXIC)
- [ ] Implementar resistencias en `Health` o nuevo componente `Resistances`
- [ ] Enemigos del Círculo VII resisten PHYSICAL un 10%, débiles a FIRE
- [ ] Habilidades elementales aplican su tipo de daño
- [ ] Feedback visual: color del número de daño según tipo

### B.2 — Enemigos Mejorados

#### B.2.1 — AI Pathfinding Básico
- [ ] Los enemigos no deben quedarse atascados contra paredes
- [ ] Implementar pathfinding simple: si hay obstáculo entre enemigo y jugador, el enemigo rodea
- [ ] Opción A: Raycast simple + desvío lateral
- [ ] Opción B: Grid-based A* simplificado (el grid ya existe en RoomGenerator)

#### B.2.2 — Nuevos enemigos para el Círculo VII
- [ ] **Centauro** — se mueve rápido, dispara flechas en arco, mantiene distancia
- [ ] **Harpía** — vuela (ignora colisión de suelo), ataque en picada, baja vida
- [ ] **Alma en Pena** — lenta, transparente, explota al morir (AoE)
- [ ] Cada uno con JSON de configuración en `assets/data/enemies/`

#### B.2.3 — Oleadas y salas de élite
- [ ] Salas normales: 1-2 oleadas de 2-3 enemigos
- [ ] Salas de élite: enemigos con stats x1.5, recompensa garantizada (habilidad rara+)
- [ ] Indicador visual de sala de élite (borde dorado en el minimap, color de fondo diferente)

### B.3 — Habilidades Completas

#### B.3.1 — Habilidades activas
- [ ] Las habilidades activas no son solo pasivas — tienen un botón de activación
- [ ] Componente `ActiveAbility` con cooldown, recurso (stamina o nuevo "Ícor")
- [ ] Slot de habilidad activa (máximo 1-2 activas equipadas)
- [ ] Input: Q y E para activar habilidades
- [ ] Implementar 5 habilidades activas iniciales:
  - **Lanza de Flegetonte** — Proyectil de fuego que atraviesa enemigos
  - **Escudo de Hielo** — 3s de 50% reducción de daño
  - **Paso Sombrío** — Teleport corto (como dash pero más largo)
  - **Grito de Guerra** — AoE stagger a enemigos cercanos
  - **Drenar Alma** — Canalizar: daño continuo a un enemigo, robo de vida

#### B.3.2 — Reroll con vida
- [ ] Botón de reroll en pantalla de selección de habilidad (R)
- [ ] Coste: 10% de HP máximo (se acumula: 10%, 15%, 20%...)
- [ ] Nuevas 3 opciones al rerollear
- [ ] Visual: el HP baja visiblemente, efecto de sangre
- [ ] Máximo 3 rerolls por sala

#### B.3.3 — Expandir pool de habilidades
- [ ] 30 habilidades totales (de las 15 actuales)
- [ ] Distribuidas por tema: Fuego(6), Sangre(5), Hielo(5), Peste(4), Sombra(4), Tormenta(3), Utilidad(3)
- [ ] Cada tema tiene al menos 1 activa, 2 pasivas, 1 utilidad
- [ ] Todas definidas en `abilities.json`

### B.4 — Sinergias Expandidas
- [ ] 6 sinergias temáticas (una por tema elemental):
  - **Fuego 3**: Los ataques dejan charcos de lava (2s, 5 dmg/s)
  - **Sangre 3**: Matar un enemigo recupera 5% HP
  - **Hielo 3**: Enemigos cerca del jugador son ralentizados 20%
  - **Peste 3**: Enemigos envenenados explotan al morir (AoE)
  - **Sombra 3**: 10% probabilidad de esquivar automáticamente
  - **Tormenta 3**: Los ataques tienen 15% de encadenar rayo a otro enemigo cercano
- [ ] 3 sinergias cruzadas:
  - **Fuego + Sangre (2+2)**: Lifesteal aplica también con daño de fuego
  - **Hielo + Tormenta (2+2)**: Enemigos congelados reciben x2 daño de rayo
  - **Sombra + Peste (2+2)**: Veneno es invisible (sin indicador para el enemigo)
- [ ] UI de sinergias: tooltip que muestra progreso (2/3 Fuego, etc.)

### B.5 — Generación Procedural Mejorada
- [ ] Grafo de salas: 3-4 salas normales → 1 sala de élite (opcional) → sala del boss
- [ ] El jugador elige bifurcaciones (2 caminos, diferentes recompensas)
- [ ] Sala de tienda (aparece 1 vez por run): gastar HP para elegir habilidad específica
- [ ] Sala de descanso (aparece 1 vez): recuperar 30% HP, sin combate
- [ ] Transiciones entre salas con fade
- [ ] Minimap simple en esquina: salas visitadas, sala actual, boss

### B.6 — Art Direction del Círculo VII
- [ ] Reemplazar placeholders con pixel art real para:
  - [ ] Jugador: idle, run, attack (3 frames), dash, hit, death (al menos 3-4 frames por anim)
  - [ ] Enemigos: idle, move, attack, death (2-3 frames por anim)
  - [ ] Boss Minotauro: idle, charge, slam, stomp, transition, death
  - [ ] Tiles: suelo de piedra sangrienta, muros oscuros, pinchos metálicos
  - [ ] Background parallax: río de sangre (Flegetonte), cielo rojo, ruinas lejanas
- [ ] Paleta del Círculo VII: rojos profundos (#8B0000), gris oscuro (#2B2B2B), naranja lava (#CC4400)
- [ ] Partículas ambientales: ceniza flotante, gotas de sangre cayendo
- [ ] Fuente pixel temática (cargar .ttf)

### B.7 — Audio del Círculo VII
- [ ] **Música**: track de exploración (cuerdas graves + percusión tribal suave)
- [ ] **Música de boss**: versión intensificada del tema del círculo
- [ ] **SFX esenciales** (mínimo 15 sonidos):
  - Ataque ligero, ataque pesado, impacto en enemigo, impacto en jugador
  - Dash, pasos, muerte de enemigo, muerte del jugador
  - Recogida de orbe, selección de habilidad, parry
  - Boss: rugido, charge, slam, transición de fase
- [ ] Ambiente: fuego crepitante constante, gritos lejanos
- [ ] Transiciones de música suaves (crossfade)

### B.8 — Playtest del Core Loop
- [ ] 20+ runs de playtest
- [ ] Documentar: tiempo medio de run, tasa de victoria, builds más comunes
- [ ] Ajustar curva de dificultad
- [ ] Ajustar frecuencia de drops
- [ ] **Criterio de éxito**: el Círculo VII se siente como un juego completo y pulido

---

## BLOQUE C — Expansión de Contenido

> **Meta:** Añadir los 8 Círculos restantes. Cada uno con identidad propia.
> El orden de implementación va del Círculo más simple al más complejo narrativamente.

### Estructura por Círculo
Cada círculo necesita:
1. **Bioma** — Tileset, background parallax, paleta de colores, partículas
2. **3 enemigos únicos** — Con mecánicas que reflejen el pecado
3. **1 Boss multifase** — 3 fases, patrones únicos, espectáculo
4. **Música** — Track de exploración + track de boss
5. **2-3 habilidades temáticas** nuevas para el pool
6. **Lore** — 3-5 fragmentos de texto

### C.1 — Círculo IX: Traición (Tutorial/Escape)

> Bioma: Lago helado, Cocytus. Hielo negro, cadenas, silencio.
> Boss: **Lucifer** (no se derrota — se huye)

- [ ] Tileset: hielo negro, cadenas rotas, oscuridad
- [ ] Paleta: azul hielo (#1A1A3E), negro, blanco hueso (#D4C5A9)
- [ ] Mecánica especial: es el tutorial. 1-2 salas cortas, enseña los controles
- [ ] Enemigos: Almas Congeladas (lentas, pocas, fáciles)
- [ ] Boss Lucifer: el jugador debe esquivar ataques mientras corre hacia la salida
  - No tiene barra de vida
  - Timer: 60 segundos para escapar
  - Ataques: columnas de hielo, viento helado, cadenas
- [ ] Cinemática: al escapar, la puerta se cierra detrás
- [ ] Habilidades desbloqueadas: tema Hielo

### C.2 — Círculo VIII: Fraude

> Bioma: Fosas ardientes, ilusiones, laberintos retorcidos.
> Boss: **Gerión** (la bestia del engaño)

- [ ] Tileset: fosas, fuego en trincheras, suelo agrietado
- [ ] Paleta: púrpura (#2D1B4E), naranja, negro
- [ ] Enemigos:
  - **Simulacro** — clon ilusorio, se duplica al recibir daño (pero los clones mueren de 1 hit)
  - **Lengua Bífida** — ataque de veneno a distancia, se esconde tras muros
  - **Embaucador** — parece un orbe de loot, ataca cuando te acercas
- [ ] Boss Gerión:
  - Fase 1: vuela fuera del alcance, dispara proyectiles — hay que esquivar
  - Fase 2: baja al suelo, ataques de cola y mordisco, crea clones
  - Fase 3: arena se reduce, Gerión ataca frenéticamente, clones simultáneos
- [ ] Habilidades desbloqueadas: tema Sombra

### C.3 — Círculo VI: Herejía

> Bioma: Tumbas de fuego, ciudadela de Dite, sarcófagos ardientes.
> Boss: **Farinata degli Uberti**

- [ ] Tileset: tumbas abiertas con fuego, pilares de la ciudadela
- [ ] Paleta: naranja ardiente (#CC4400), gris ceniza, rojo oscuro
- [ ] Enemigos:
  - **Hereje Ardiente** — se prende fuego al morir, deja charco de lava
  - **Portador de Féretro** — tanque, lleva un sarcófago como escudo
  - **Llama Errante** — proyectil que persigue al jugador lentamente
- [ ] Boss Farinata:
  - Fase 1: emerge de su tumba, ataques de columna de fuego
  - Fase 2: levanta sarcófagos como proyectiles, crea muros temporales
  - Fase 3: el suelo se llena de fuego progresivamente, Farinata invoca almas heréticas
- [ ] Habilidades desbloqueadas: tema Fuego (más)

### C.4 — Círculo V: Ira

> Bioma: Pantano del Estigia, barro ardiente, rabia perpetua.
> Boss: **Flegias** (el barquero furioso)

- [ ] Tileset: barro, agua turbia, plataformas flotantes
- [ ] Paleta: marrón oscuro, rojo ira, verde pantano
- [ ] Mecánica de bioma: zonas de barro ralentizan al jugador
- [ ] Enemigos:
  - **Iracundo** — carga hacia el jugador, cada golpe lo enfurece más (más rápido)
  - **Alma del Estigia** — surge del barro, agarra al jugador (hay que escapar con dash)
  - **Berserker** — baja vida, altísimo daño, se mueve erráticamente
- [ ] Boss Flegias:
  - Fase 1: ataca con su barca (golpes amplios), oleadas del pantano
  - Fase 2: destruye partes de la arena (plataformas se hunden)
  - Fase 3: abandona la barca, ataque cuerpo a cuerpo furioso, grito que aturde
- [ ] Habilidades desbloqueadas: tema Tormenta

### C.5 — Círculo IV: Avaricia

> Bioma: Oro fundido, trampas mecánicas, tesoros malditos.
> Boss: **Plutón** (demonio de la riqueza)

- [ ] Tileset: oro fundido, engranajes, cofres, monedas esparcidas
- [ ] Paleta: dorado corrupto (#8B7500), marrón, rojo
- [ ] Mecánica de bioma: cofres trampa (50% loot real, 50% enemigo o daño)
- [ ] Enemigos:
  - **Avaro** — recoge orbes de loot del suelo, se hace más fuerte con cada uno
  - **Golem de Oro** — enorme, lento, inmune a knockback
  - **Moneda Maldita** — pequeña, rápida, aparece en grupos de 5-6, bajo daño individual
- [ ] Boss Plutón:
  - Fase 1: lanza monedas de oro gigantes (rebote en muros)
  - Fase 2: invoca Golems de Oro, lluvia de monedas desde arriba
  - Fase 3: se fusiona con su tesoro, forma gigante, ataques de barrido
- [ ] Habilidades desbloqueadas: utilidades y defensivas

### C.6 — Círculo III: Gula

> Bioma: Lluvia ácida eterna, carne putrefacta, barro.
> Boss: **Cerbero** (tres cabezas, tres fases)

- [ ] Tileset: carne, barro, lluvia (efecto constante), charcos ácidos
- [ ] Paleta: verde pútrido (#2E4A1E), marrón, rojo descompuesto
- [ ] Mecánica de bioma: lluvia ácida hace 1 dmg/s a menos que estés bajo techo
- [ ] Enemigos:
  - **Glotón** — absorbe daño, al morir suelta orbes de vida
  - **Gusano** — sale del suelo, muerde, vuelve al suelo (ciclo)
  - **Nube Tóxica** — AoE móvil, deja trail de veneno
- [ ] Boss Cerbero:
  - Fase 1: mordiscos de cada cabeza (uno a uno), lluvia ácida lenta
  - Fase 2: dos cabezas atacan simultáneamente, vómito tóxico con charcos
  - Fase 3: las tres cabezas atacan a la vez, carga devastadora, pantalla tiembla
- [ ] Habilidades desbloqueadas: tema Peste

### C.7 — Círculo II: Lujuria

> Bioma: Tormenta eterna, vientos que arrastran almas.
> Boss: **Minos** (el juez de los condenados)

- [ ] Tileset: plataformas flotantes, nubes oscuras, viento visible
- [ ] Paleta: púrpura, rosa oscuro, gris tormenta
- [ ] Mecánica de bioma: viento empuja al jugador en una dirección (cambia cada X segundos)
- [ ] Enemigos:
  - **Alma Arrastrada** — se mueve con el viento, daño de contacto
  - **Susurro** — invisible hasta que ataca, aparece detrás del jugador
  - **Arpía Oscura** — vuela, ataque en picada, más fuerte que la del Círculo VII
- [ ] Boss Minos:
  - Fase 1: juzga con su cola (la enrolla y golpea), lanza almas como proyectiles
  - Fase 2: el viento se intensifica, Minos crea tornados que persiguen
  - Fase 3: Minos vuela, bombardeo aéreo, el suelo se fragmenta
- [ ] Habilidades desbloqueadas: tema Tormenta (más)

### C.8 — Círculo I: Limbo (Final)

> Bioma: Penumbra melancólica, niebla, silencio aterrador.
> Boss: **Caronte** (el barquero — boss final)

- [ ] Tileset: niebla, piedra gris, agua oscura, sin decoración
- [ ] Paleta: gris (#2B2B2B), blanco hueso (#D4C5A9), negro
- [ ] Mecánica de bioma: SILENCIO. No hay música. Solo pasos y ambiente. Genera tensión.
- [ ] Enemigos:
  - **Sombra del Limbo** — forma indefinida, aparece y desaparece
  - **Filósofo Errante** — no ataca, pero bloquea el camino (hay que empujarlo)
  - **Eco de Dante** — réplica del jugador con las mismas habilidades pero en IA
- [ ] Boss Caronte:
  - Fase 1: ataca con su remo (golpes amplios), el río sube
  - Fase 2: invoca almas del río, el agua es letal (reduce plataformas)
  - Fase 3: Caronte se transforma, ataques masivos, el jugador pelea en su barca
  - Al vencer: cinemática de escape a la superficie
- [ ] Habilidades: no se desbloquean (es el final)
- [ ] **Cinemática final:** el alma emerge a la superficie. Luz. Libertad. Créditos.

---

## BLOQUE D — Polish y Lanzamiento

### D.1 — Personajes Jugables Completos
- [ ] Expandir de 3 a 6 personajes según GDD:
  - **El Condenado** — Ya existe (Guerrero). Equilibrado.
  - **El Iracundo** — +25% daño, -20% HP máx. Color rojo.
  - **El Avaro** — Más loot, pero solo 2 opciones de habilidad (no 3). Color dorado.
  - **El Hereje** — Empieza con habilidad de fuego, vulnerable a hielo. Color naranja.
  - **El Traidor** — Dash mejorado (+0.2s i-frames), daño base -15%. Color azul.
  - **El Glotón** — +40% HP máx, stamina se regenera 30% más lento. Color verde.
- [ ] Condiciones de desbloqueo:
  - Iracundo: completar 3 runs
  - Avaro: recoger 50 orbes de loot totales
  - Hereje: llegar al Círculo VI
  - Traidor: esquivar 100 ataques con dash
  - Glotón: morir 10 veces (ironía de la gula)
- [ ] Sprite único por personaje (color + silueta ligeramente diferente)

### D.2 — Pool de Habilidades Final (50+)
- [ ] Expandir cada tema a 7-8 habilidades
- [ ] Asegurar al menos 15 habilidades activas con cooldowns
- [ ] Balancear rareza: 50% común, 35% raro, 15% épico
- [ ] Cada habilidad épica debe sentirse transformadora del build
- [ ] Testing de sinergias: verificar que ninguna combinación rompa el juego

### D.3 — Modos de Dificultad
- [ ] **Purgatorio** (fácil): -30% daño enemigo, +50% HP jugador, más loot
- [ ] **Infierno** (normal): valores base
- [ ] **Malebolge** (difícil): +50% daño enemigo, -20% HP jugador, menos loot, jefes con fase extra
- [ ] Selección en menú principal antes de empezar run

### D.4 — Contenido Opcional
- [ ] **Salas secretas**: 5% de probabilidad, contienen lore + habilidad épica garantizada
- [ ] **Eventos aleatorios**: NPC que ofrece tratos (vida por habilidad, etc.)
- [ ] **La Sombra de Virgilio**: aparece como tienda en 1 de cada 3 runs, vende habilidades por HP
- [ ] **Desafíos diarios**: seed fija, leaderboard local

### D.5 — UI Final
- [ ] Menú principal con fondo animado (partículas de fuego, almas flotando)
- [ ] Pantalla de colección: todas las habilidades encontradas, lore, estadísticas
- [ ] Pantalla de opciones: volumen, controles, dificultad
- [ ] Fuente pixel temática consistente en todo el juego
- [ ] Animaciones de UI: transiciones suaves, cards que aparecen con efecto

### D.6 — Audio Final
- [ ] 9 tracks de exploración (uno por círculo)
- [ ] 9 tracks de boss
- [ ] ~50 SFX (combate, habilidades, UI, ambiente)
- [ ] Ambiente por bioma (fuego, hielo, tormenta, silencio, etc.)
- [ ] Crossfade entre tracks al cambiar de sala/estado
- [ ] Sonido signature de cada boss (rugido al entrar)

### D.7 — Balance Final
- [ ] Curva de dificultad: cada círculo es ~15-20% más difícil que el anterior
- [ ] Tiempo ideal de run completa: 45-60 minutos
- [ ] Tiempo por círculo: ~5-7 minutos
- [ ] Tasa de victoria ideal: ~15-20% (souls-like pero roguelike)
- [ ] Las builds sinérgicas deben sentirse poderosas pero no invencibles
- [ ] Testing intensivo con al menos 3 personas externas

### D.8 — Localización
- [ ] Español (principal)
- [ ] Inglés
- [ ] Todas las strings de UI en archivo de localización JSON
- [ ] Habilidades, lore, nombres de boss — todo localizable

### D.9 — Preparación para Lanzamiento
- [ ] Steam page (si aplica)
- [ ] Trailer de gameplay (60s)
- [ ] Screenshots (5-10)
- [ ] Press kit
- [ ] README.md actualizado
- [ ] Build de release optimizado
- [ ] Testing en múltiples resoluciones

---

## Prioridades y Dependencias

```
A.1 (Colisiones) ──→ A.2 (Balance) ──→ A.3 (Fixes) ──→ A.4 (Playtest)
                                                              │
                                                              ▼
                     B.1 (Combate) ──→ B.2 (Enemigos) ──→ B.3 (Habilidades)
                                                              │
                     B.5 (Procgen) ◄──────────────────────────┤
                                                              │
                     B.4 (Sinergias) ──→ B.6 (Arte) ──→ B.7 (Audio) ──→ B.8 (Playtest)
                                                                              │
                                                                              ▼
                     C.1 (Círculo IX) ──→ C.2 (VIII) ──→ C.3 (VI) ──→ ...──→ C.8 (I)
                                                                              │
                                                                              ▼
                     D.1 (Personajes) ──→ D.2 (50+ habilidades) ──→ D.3-D.9 (Polish)
```

## Notas Importantes

1. **El Bloque A es obligatorio antes de todo.** Sin colisiones funcionales nada más tiene sentido.
2. **El Bloque B es el más importante.** Si el Círculo VII no es divertido, 9 círculos mediocres no salvan el juego.
3. **Los Círculos en Bloque C son modulares.** Se pueden implementar en cualquier orden.
4. **El arte puede ser placeholder hasta Bloque D.** El gameplay va primero.
5. **Cada Círculo nuevo reutiliza el 70% de la infraestructura.** El esfuerzo principal es contenido (enemigos, boss, tiles, música), no código nuevo.

---

> *"En el fondo del Infierno, donde nadie mira, una puerta se abrió. Y tú fuiste lo bastante estúpido — o lo bastante valiente — para cruzarla."*
