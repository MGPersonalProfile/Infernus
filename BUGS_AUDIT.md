# INFERNUS — Bug Audit (sesión de identificar)
## Fecha: 2026-05-09
## Status: NO FIX. Solo catalogación. Sesión separada para resolver.

> El usuario reporta: "el juego ha mejorado pero le falta un POQUITO".
> Análisis basado en 3 screenshots (warrior/rogue/knight) + grep del código.
> 8 categorías, 28 bugs identificados con file:line cuando corresponde.

---

## A — Inconsistencia visual (la queja principal)

### A.1 — Mezcla de estilos de pixel art
**Síntoma:** Los tiles del suelo y paredes son **detallados, casi hand-painted** (piedra con musgo, grietas, lava, ladrillo con erosión). Pero las decoraciones que se colocan ENCIMA (`decor_altar`, `decor_pillar`, `decor_tombstone`) son **pixel art simple**, casi 1-bit, que el usuario describe como "sillas con sangre cutres". El contraste de detalle hace que las decor parezcan placeholder.

**Evidencia:**
- `assets/sprites/tiles/floor_var1..6.png` y `wall_var2/3.png`: detalle alto, palette compleja
- `assets/sprites/tiles/decor_altar.png` / `decor_pillar.png` / `decor_tombstone.png`: simples, palette plana, parecen iconos

**Causa raíz:** Antigravity entregó dos generaciones distintas en momentos distintos. Los HD fueron uno, los decor fueron otro batch sin re-armonizar.

**Donde se usa:** `src/world/RoomGenerator.cpp` líneas 270-289 (3% chance de spawn como ambient).

### A.2 — Personajes desentonan con el mundo
**Síntoma:** El knight es **amarillo brillante**, el warrior **gris/blanco con destellos**, el rogue oscuro pero con cape demasiado uniforme. El mundo es oscuro, infernal, rojizo. Los personajes se ven como si fueran de otro juego.

**Evidencia:** screenshots 1 y 3 — el knight amarillo "salta" del background en vez de integrarse.

**Causa raíz:** Los sprites de personajes los hizo Antigravity con paleta independiente (intento de "destacarse del fondo" tomado al extremo). No se aplicó el infernal tint del shader CRT_Vignette en proporción.

**Decisión a tomar (sesión de fix):** ¿Re-tintamos sprites en el shader, o pedimos a Antigravity que rehaga sprites con paleta más oscura/integrada?

### A.3 — Saturación de decoraciones
**Síntoma:** En el screenshot 1 (warrior) cuento **~17 "fuentes de sangre"** visibles en una sola pantalla. Esto rompe la composición — cada altar/pillar pierde peso por repetición.

**Evidencia:** `RoomGenerator.cpp:270` — `else if (GetRandomValue(0, 99) < 3 && ...)` — 3% chance per tile.

**Cálculo:** arena boss_arena = 28×20 = 560 tiles. ~50% son floor → 280 candidatos. 3% chance → ~8 expected. PERO en una sala 22×14 (combat_room_0) con 50% floor → 154 candidatos × 3% = ~5. El screenshot muestra 17 — sospecho que se aplica incluso sobre tiles que tienen decor pequeño debajo (combinación + ambient cuando no debería).

**Problema secundario:** la lógica `if (15% small) else if (3% ambient)` los hace exclusivos pero ambos son "decor". Probablemente debería ser **2% ambient TOTAL** a través de toda la sala, no per-tile, para evitar saturación en salas grandes.

### A.4 — Suelo demasiado variado
**Síntoma:** En cada screenshot se ven 3-5 tiles de suelo diferentes mezclados. La variedad es buena pero el ratio es alto (50/50 base vs variants) y los variants tienen detalles muy distintos (algunos con sangre, otros con musgo, otros con grietas grandes).

**Evidencia:** `RoomGenerator.cpp:226-235` — `(GetRandomValue(0,99) < 50) ? 0 : GetRandomValue(1,6)`. 50% probabilidad de NO base.

**Decisión:** bajar el porcentaje de variants a 25% y/o agrupar variants visualmente similares.

### A.5 — Decor sin animación cuando se ve animable
**Síntoma:** las antorchas de pared (`torch.png`) animan (4 frames). Pero los `decor_torch_wall.png` (HD versions de Antigravity) **no se usan**. Las "fuentes de sangre" (altares) están estáticas — visualmente piden tener algo (gota de sangre cayendo, llama).

**Evidencia:** grep de `decor_torch_wall` en src/ → 0 hits. RoomGenerator.cpp:312-319 usa `torch.png`.

---

## B — UI/HUD problems

### B.1 — Q/E slots: nombre desbordado
**Síntoma:** Cuando el icono no se muestra (fallback), el nombre de la habilidad sale desbordado del slot. "Lanza de Flegetonte" cabe ~3x en el slot.

**Evidencia:** `src/core/Game_UI.cpp:137`:
```cpp
TextUtils::draw(a.name.c_str(), x + 4, y + slotSize / 2 - 4, 8,
                Color{200, 180, 140, 255});
```
Sin `TextUtils::truncate`, sin `TextUtils::measure`, sin word-wrap.

**Fix sugerido:** truncate a ~7 chars + "..." o ocultar el nombre y solo mostrar el key label más grande si no hay icono.

### B.2 — Iconos de Q/E invisibles
**Síntoma:** Los iconos existen en `assets/sprites/abilities/` pero en los screenshots NO se ven, solo el nombre fallback.

**Evidencia:** `assets/sprites/abilities/escudo_hielo.png` first row pixels = `[(0,0,0,255), ...]` — fondo NEGRO OPACO. El slot HUD bg es `Color{20, 12, 8, 220}` (también casi negro). Resultado: invisible.

**Fix sugerido:** Antigravity rehace los iconos con fondo TRANSPARENTE (RGBA alpha=0) o el slot bg cambia a tono claro.

### B.3 — L (special attack) no tiene HUD slot visible
**Síntoma:** El usuario dice "los cuadraditos eso de la Q y la E se desborda la letra, y la L y la Q hacen lo mismo". La L no tiene su propio slot — el cooldown del special se muestra como TEXTO en la esquina top-left:
```cpp
TextUtils::drawOutlined(TextFormat("L: %.1fs", specialCooldownTimer), 20, 102, ...)
```
**Asimetría visual:** Q y E tienen slots bonitos, L tiene texto suelto. Confunde que sean distintas habilidades activas.

### B.4 — Bare HP bar size desaprovecha espacio
**Síntoma (screenshot 3):** "HP 180/180" + barra HP roja arriba izq. La barra ocupa ~25% del width superior pero en pantalla de 1280px se ve pequeña, especialmente al lado del player que quedó muy a la derecha por la cámara.

**Decisión:** ¿escalamos la barra HP por viewport? ¿O posicionamos por porcentaje?

### B.5 — Stamina amber gradient se ve "raro"
**Síntoma:** la barra de stamina amber (Color{230, 190, 90} → {180, 140, 60}) es muy parecida tonalmente al borde dorado del panel. Falta contraste interno.

### B.6 — Sin indicador visual de ability disponibility
**Síntoma:** Si Q está en cooldown vs disponible, solo varía el cooldown overlay. No hay glow o efecto "ready".

---

## C — Input/controles

### C.1 — Conflicto conceptual L vs Q/E (USER REPORT)
**El usuario reporta:** "L y Q hacen lo mismo".

**Análisis:**
- `L (SPECIAL_ATTACK)` → ejecuta `executeSpecialAttack()` (Golpe Sismico/Sombra/Escudo Oseo). 30 stamina, cooldown timer dedicado. **1 habilidad fija por clase**.
- `Q (ABILITY_Q)` → ejecuta `tryUseActive(slot 0)`. Stamina+cooldown según ability. **Ability variable según loadout**.

**Son conceptualmente la misma cosa:** habilidad activa con cooldown y costo de stamina. Tener 3 botones (L, Q, E) para esto es redundante y confuso.

**Decisión a tomar:**
- (A) Eliminar L; mover special-class al slot Q por defecto.
- (B) Mantener L pero darle slot HUD y diferenciar "ultimate" vs "active normal".
- (C) Eliminar Q y mover ability a L; perder un slot.

Mi voto preliminar: **A** — eliminar L, slots Q/E manejan todo, special class ya viene equipada en Q.

### C.2 — Demasiadas teclas para layout de teclado
**Inventario actual de teclas en gameplay:**
- WASD (4): movimiento
- J: attack light
- K: attack heavy
- SPACE: dash
- F: parry
- L: special (¿eliminar?)
- Q: ability slot 1
- E: ability slot 2
- R: interact
- I: inventory
- TAB: info
- H: abilities view

**12-13 acciones**. Es demasiado para una mano si la otra está en WASD. Se requiere reach largo a I/H/R.

**Decisión:** consolidar info menus (TAB hace todo, abandonar I y H).

### C.3 — Distribución del mando (gamepad) mejorable
- Movimiento: stick L (analog)
- ATTACK_LIGHT: X (RIGHT_FACE_LEFT)
- ATTACK_HEAVY: Y (RIGHT_FACE_UP)
- DASH: A (RIGHT_FACE_DOWN)
- INTERACT: B (RIGHT_FACE_RIGHT)
- SPECIAL_ATTACK: LB
- PARRY: RB
- ABILITY_Q: LT (analog trigger usado como botón)
- ABILITY_E: RT

**Problema:** PARRY en RB (botón) y ABILITY_E en RT (trigger del mismo dedo). Pulsar parry mientras quieres ability E es awkward.

**Sugerencia:** ABILITY_Q ↔ PARRY swap. Parry es más reactivo (RT/RB común para parry en souls), abilities no necesitan trigger inmediato.

---

## D — Colisiones

### D.1 — Wall cache excluye entities con Velocity (enemies no sólidos)
**Síntoma:** El player puede atravesar enemies. ¿Es deseado?

**Evidencia:** `CollisionSystem.h:139`:
```cpp
if (registry.hasComponent<Velocity>(e)) continue; // skip movers
```

**Análisis:** Para roguelite/souls-like, depende:
- Pro overlap: combate más fluido, evita stuck-on-enemy
- Contra overlap: rompe inmersión, no hay "presence" de enemigos

**Decisión:** ver si "soft separation" (push-apart con elasticidad) es mejor que sólido total.

### D.2 — Decoración ambient sin collider (intencional o bug?)
**Síntoma:** Los altares/pillars/tombstones de RoomGenerator NO tienen Collider. Visual sin bloqueo.

**Evidencia:** `RoomGenerator.cpp:270-289` — solo `Sprite + Transform2D + RoomGeometry`. Sin Collider.

**Visual contradice gameplay:** un pillar de piedra debería ser sólido. Un tombstone también. Un altar quizás trigger interactivo.

**Decisión:** sí añadir collider a pillars/tombstones (no triggers). Altares quedan como decor.

### D.3 — `pillar.png` standalone (32x48) sí tiene collider sólido
**Evidencia:** `RoomGenerator.cpp:381` — `addComponent<Collider>(pillar, 32.0f, 48.0f, false)`.

**Asimetría:** pillar.png (sprite separado) sí bloquea, decor_pillar (decor PNG) no. Mismo concepto, distinto comportamiento.

### D.4 — Collider del player no centra
**Síntoma:** `Collider(fw * 0.75f, fh * 0.9f)` — anchos sin offset:
```cpp
registry.addComponent<Collider>(playerEntity, fw * 0.75f, fh * 0.9f, false);
```

El collider se posiciona desde top-left de Transform2D, sin offset. El sprite del player tiene el cuerpo centrado en el frame, pero el collider está top-left esquina del frame, no del cuerpo. **Hitbox desalineado del cuerpo visual.**

### D.5 — No hay raycast/swept collision
**Síntoma:** En frames con velocidad alta (ej. dash a 1200px/s con 16ms frame), el player puede saltar 19px por frame. Si una pared es de 8px de grosor, podría atravesarla en 1 frame.

**Implementación actual:** push-out después del move. Funciona si overlap es detectable, pero falla con tunneling rápido.

### D.6 — Sin coyote time / input buffer
**Síntoma de gameplay:** dashes y attacks que se "comen" si pulsas justo antes de aterrizar/recovery.

### D.7 — Trigger sin callback distinto por tipo
**Evidencia:** spike/fire_trap/pit todos `isTrigger=true`. Distingue por `Trap` component. OK pero los triggers se evalúan idéntico — no hay distinción de "área dañina periódica" vs "instakill" vs "área de status effect".

---

## E — Performance / sistemas no usados

### E.1 — Sprites de partículas entregados sin wirear
**Evidencia:** `assets/sprites/particles/` tiene:
- ash_particle.png, blood_drop.png (USADOS)
- dust_cloud, fire_burst, ground_crack, poison_trail, shockwave, soul_wisps, spark_burst (**NO usados**)

`PartikelEmitters` y `AnimEventDispatcher` usan función `spawnBlood`/`spawnDashDust`/`spawnFireTrail`/`spawnSlamShockwave` con texturas hardcoded internamente.

### E.2 — `floor_hd.png` y `decor_hd.png` originales 1024x1024 sin usar
**Evidencia:** `assets/sprites/tiles/floor_hd.png`, `decor_hd.png`, y carpeta `hd_originals/`. Antigravity los entregó como referencia pero no están integrados.

### E.3 — `warrior_spritesheet.png` 1024x1024 huérfano
**Evidencia:** existe en `player/` pero el código carga `warrior_idle.png`/`run.png`/`attack.png`. El spritesheet completo no se usa.

### E.4 — `player_idle.png`, `player_run.png`, `player_attack.png`, `player_death.png` legacy
**Evidencia:** están en `player/` pero el código usa `<class>_*.png`. Estos antiguos sin clase son legacy.

### E.5 — Debug AnimationController muerto eliminado ✓
Ya removido en commit anterior. OK.

### E.6 — `assassin_move.png`, `bomber_move.png` legacy
**Evidencia:** EnemyFactory usa `_run.png` no `_move.png`. Legacy.

---

## F — Data inconsistencies

### F.1 — `assets/data/anim_events.json` borrado vs `animation_events/events.json`
✓ Resuelto. Borré el legacy.

### F.2 — `sfx_specs.json` describe SFX que no existen aún
**Evidencia:** `assets/data/sfx_specs.json` describe SFX que jsfxr debe generar. Antigravity aún no entregó WAVs.

### F.3 — `active_abilities.json` defaultByClass solo para 3 clases
**Evidencia:** `defaultByClass: {warrior, rogue, knight}`. Si añadimos otra clase futura, no tendrá defaults. OK por ahora.

---

## G — Lore/UX

### G.1 — Sin tutorial / onboarding
El jugador entra y no sabe qué es Q/E/L/F/parry. El INFO menu (TAB) lo cubre pero no es prominente.

### G.2 — Sin feedback visual de stamina baja
**Síntoma:** si intentas atacar sin stamina, no pasa nada. Sin mensaje, sin shake, sin SFX.

### G.3 — Sin mini-mapa de la sala actual
**Evidencia:** existe `drawMinimap` pero solo muestra **progreso de runs**, no layout de sala.

---

## H — Lista TL;DR de prioridades para la sesión de FIX

### P0 (rompe inmersión / experiencia base)
- A.3 saturación de decoraciones (cap por sala, no per-tile)
- A.2 personajes desentonan con paleta
- B.1 Q/E nombre desbordado (truncate)
- B.2 iconos invisibles (fondo transparente o slot bg claro)
- C.1 conflicto L vs Q/E (eliminar L o consolidar)
- D.4 collider del player desalineado (offset central)

### P1 (feel del juego)
- A.1 inconsistencia de detalle en decor (Antigravity rehace decor en estilo HD)
- D.2 decor pillars/tombstones bloquean (añadir colliders)
- D.5 sin swept collision (tunneling de dash)
- D.6 input buffer / coyote time
- C.2 demasiadas teclas (consolidar I/H/TAB)

### P2 (polish)
- A.4 menos variedad de floor (25% vs 50%)
- A.5 torchas HD reemplazan torch.png simple
- B.3 L tiene su propio slot o se elimina
- B.5 stamina contraste
- B.6 indicador "ready" para abilities
- E.1-E.4 limpieza de sprites huérfanos
- G.1 tutorial básico
- G.2 stamina-low feedback

### P3 (defer)
- D.1 enemies sólidos (decisión de game design)
- C.3 gamepad reorder (preferencia personal)
- B.4 HP bar responsive

---

## Métrica de éxito de la sesión de FIX

Cuando juegues 5 minutos después del fix:
- ✅ No verás >5 "fuentes de sangre" en ninguna pantalla
- ✅ El nombre de Q/E cabe en el slot o es trunc + "..."
- ✅ Solo 1 sistema de abilities (no L+Q+E redundante)
- ✅ Hitbox del player se siente preciso (no atraviesas walls "casi", no te pegan a 5px de distancia)
- ✅ Pillars/tombstones bloquean visiblemente
- ✅ Personajes y mundo comparten paleta (uno no resalta artificialmente)
