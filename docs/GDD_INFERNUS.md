# 🔥 INFERNUS — Game Design Document

> *"Lasciate ogne speranza, voi ch'intrate"*
> — Abandonad toda esperanza, los que aquí entráis.

---

## 1. Concepto General

**INFERNUS** es un **roguelike 2D con combate souls-like** ambientado en el Infierno de la Divina Comedia de Dante Alighieri. El jugador asciende desde el noveno círculo (Traición) hasta la superficie, enfrentando bosses épicos, construyendo builds a través de sinergias de habilidades, y sobreviviendo un infierno diseñado para que no escapes.

**Género:** Roguelike / Action RPG / Souls-like
**Perspectiva:** 2D Side-scroller (pixel art)
**Plataformas:** PC (potencialmente consolas)
**Estética:** Pixel art oscuro, infernal, opaco — inspirado en Blasphemous, Dark Souls y las ilustraciones clásicas de Gustave Doré.

---

## 2. Pilares de Diseño

| Pilar | Descripción |
|---|---|
| **Combate Souls-like** | Cada enemigo es una amenaza. Los bosses tienen patrones memorizables, múltiples fases y presencia visual imponente. |
| **Sinergias Roguelike** | Cada run es diferente. Elige entre habilidades que se combinan entre sí para crear builds únicos y devastadores. |
| **Estética Infernal** | Paleta opaca y oscura. Rojos profundos, negros, fuego, ceniza, hielo corrupto. Cada píxel respira sufrimiento. |
| **Riesgo vs. Recompensa** | Mecánicas como el reroll con vida obligan al jugador a tomar decisiones difíciles constantemente. |
| **Narrativa Emergente** | No eres el héroe. Eres un oportunista aprovechando el caos. La historia se cuenta a través del entorno y los encuentros. |

---

## 3. Narrativa

### Premisa

Dante Alighieri acaba de atravesar el Infierno. Su paso ha dejado caos, puertas rotas y demonios heridos. Tú no eres Dante. No eres un héroe. Eres un **alma condenada** atrapada en el **Cocytus** — el lago helado del noveno círculo, el más profundo del Infierno — desde hace una eternidad.

Pero ahora, por primera vez, **la puerta está abierta**.

Mientras los demonios se reorganizan y los guardianes intentan restaurar el orden, tú ves tu oportunidad. No tienes poderes. No tienes armas. Solo tienes la desesperación de alguien que lleva demasiado tiempo en el hielo.

**Saltas hacia la puerta.**

### Estructura Narrativa

- El jugador **asciende** del Círculo 9 al Círculo 1 (inversión de la Divina Comedia).
- A lo largo del juego se encuentran **las consecuencias del paso de Dante**: puertas destrozadas, demonios debilitados, NPCs que hablan de él.
- El protagonista **no debería estar ahí**. Los guardianes de cada círculo lo tratan como un intruso.
- Tono: Desesperación, determinación, un toque de humor oscuro.

### Personaje Principal

Un alma sin nombre, sin pasado definido (el jugador puede imaginar su pecado). Comienza con nada — todo lo que obtenga será durante el intento de escape.

---

## 4. Estructura del Juego — Los 9 Círculos

El jugador asciende del Círculo 9 al 1. Cada círculo es un **bioma** con estética, enemigos y boss únicos.

| Orden de juego | Círculo | Pecado | Ambientación | Boss |
|---|---|---|---|---|
| 1º | **Círculo IX** | Traición | Lago helado, Cocytus. Hielo negro, cadenas. | **Lucifer** (tutorial/escape — no se derrota, se huye) |
| 2º | **Círculo VIII** | Fraude | Fosas ardientes, ilusiones, laberintos. | **Gerión** (la bestia del engaño) |
| 3º | **Círculo VII** | Violencia | Río de sangre hirviente (Flegetonte), bosques de suicidas. | **El Minotauro** |
| 4º | **Círculo VI** | Herejía | Tumbas de fuego, ciudadela de Dite. | **Farinata degli Uberti** (hereje gigante) |
| 5º | **Círculo V** | Ira | Pantano del Estigia, fuego y barro ardiente. | **Flegias** (el barquero furioso) |
| 6º | **Círculo IV** | Avaricia | Oro fundido, trampas mecánicas, tesoros malditos. | **Plutón** (demonio de la riqueza) |
| 7º | **Círculo III** | Gula | Lluvia ácida eterna, carne putrefacta, barro. | **Cerbero** (tres cabezas, tres fases) |
| 8º | **Círculo II** | Lujuria | Tormenta eterna, vientos que arrastran almas. | **Minos** (el juez de los condenados) |
| 9º (final) | **Círculo I** | Limbo | Penumbra melancólica, niebla, silencio. | **Caronte** (el barquero — boss final antes de la libertad) |

> **Nota:** Cada círculo contiene múltiples salas procedurales + salas de élite + la sala del boss.

---

## 5. Sistema de Combate

### Filosofía
Souls-like en 2D: cada golpe importa, cada esquiva es vital. El jugador debe aprender los patrones de cada enemigo y boss.

### Mecánicas Base

| Mecánica | Descripción |
|---|---|
| **Ataque ligero** | Rápido, poco daño, permite combos. |
| **Ataque pesado** | Lento, alto daño, puede romper guardias. |
| **Esquiva/Dash** | I-frames (marcos de invulnerabilidad). Recurso limitado por stamina. |
| **Stamina** | Limita ataques y esquivas. Se regenera al no actuar. |
| **Habilidad activa** | Obtenida durante la run. Consume un recurso (maná / ícor / alma). |
| **Parry** (opcional) | Alto riesgo, alta recompensa — anula un ataque y abre una ventana de daño. |

### Armas
El jugador comienza sin arma y encuentra una en las primeras salas. Tipos posibles:
- **Espada** — Equilibrada.
- **Maza / Martillo** — Lenta, alto daño, rompe guardias.
- **Daga / Garra** — Rápida, bajo daño individual, buen DPS sostenido.
- **Guadaña** — Rango medio, habilidades de área.
- **Cadenas** — Ataques a distancia media, control de multitudes.

---

## 6. Sistema de Progresión Roguelike

### Progresión Intra-Run (dentro de cada intento)

#### Elección de Habilidades
Al completar una sala, el jugador elige entre **3 opciones**:(Las tres mostradas son ilustrativas, obviamente no son las unicas)

```
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  🔴 HABILIDAD   │  │  🟡 HABILIDAD   │  │  🟢 UTILIDAD    │
│    ACTIVA       │  │    PASIVA       │  │                 │
│                 │  │                 │  │  Curación       │
│ "Lanza de      │  │ "Sangre         │  │  parcial,       │
│  Flegetonte"   │  │  Ardiente"      │  │  stamina boost, │
│                 │  │  +15% daño de   │  │  escudo temp.   │
│ Proyectil de   │  │  fuego          │  │                 │
│ fuego que       │  │                 │  │                 │
│ atraviesa       │  │                 │  │                 │
└─────────────────┘  └─────────────────┘  └─────────────────┘
                         [ 🔄 REROLL ]
                      (Cuesta vida usarlo)
```

- **Habilidades activas:** Ataques especiales, invocaciones, transformaciones.
- **Habilidades pasivas:** Buffs permanentes para esa run.
- **Utilidades:** Curación, escudos, stamina, recursos.

#### Reroll con Vida
Si ninguna opción convence, el jugador puede **gastar un porcentaje de su vida máxima** para rerollear las 3 opciones. Cuanto más rerollees, más cuesta. **Riesgo puro.**

#### Sinergias Temáticas
Las habilidades se agrupan por **elementos/temas** asociados a los círculos:

| Tema | Ejemplo de Sinergia |
|---|---|
| 🔥 **Fuego** (Herejía/Ira) | Habilidades de fuego + "Sangre Ardiente" = Los ataques dejan charcos de lava |
| 🩸 **Sangre** (Violencia) | Habilidades de sangre + "Sed de Vida" = Lifesteal aumentado |
| ❄️ **Hielo** (Traición) | Habilidades de hielo + "Cocytus Interior" = Ralentización en área |
| ☠️ **Peste** (Gula) | Habilidades de veneno + "Plaga" = Daño en área persistente |
| 💀 **Sombra** (Fraude) | Habilidades de ilusión + "Doble Sombra" = Clon que ataca |
| ⚡ **Tormenta** (Lujuria) | Habilidades de rayo + "Ojo del Huracán" = Proyectiles que rebotan |

> **Cuantas más habilidades del mismo tema acumules, más potentes se vuelven las sinergias.**

---

### Progresión Meta (entre runs)

Lo que conservas al morir:

| Tipo | Descripción |
|---|---|
| **Habilidades desbloqueadas** | Al llegar a un círculo por primera vez, desbloqueas habilidades de ese tema para que aparezcan en runs futuras desde el inicio. |
| **Personajes** | Nuevos personajes con ventajas/desventajas iniciales asociadas a un pecado. |
| **Skins** | Cosméticas. Se desbloquean por logros (derrotar un boss sin recibir daño, completar una run en X tiempo, etc.) |
| **Lore** | Fragmentos de historia que se van desbloqueando, contando más sobre el protagonista, los demonios y el caos de Dante. |

---

## 7. Personajes Jugables (Meta-progresión)

Cada personaje está asociado a un pecado y tiene ventajas/desventajas únicas:

| Personaje | Pecado | Ventaja | Desventaja |
|---|---|---|---|
| **El Condenado** | Ninguno | Equilibrado, sin modificadores. | Personaje inicial, sin bonus. |
| **El Iracundo** | Ira | +25% daño cuerpo a cuerpo. | -20% vida máxima. |
| **El Avaro** | Avaricia | Encuentra más recursos y opciones de reroll gratis. | Menos habilidades ofrecidas (2 en vez de 3). |
| **El Hereje** | Herejía | Empieza con una habilidad de fuego. | Vulnerable al hielo (+daño recibido). |
| **El Traidor** | Traición | Empieza con dash mejorado (más i-frames). | Daño base reducido. |
| **El Glotón** | Gula | +40% vida máxima. | Stamina se regenera más lento. |

---

## 8. Diseño de Bosses

### Filosofía
Cada boss debe ser un **evento memorable**. Patrones claros pero exigentes, múltiples fases, y un espectáculo visual que llene la pantalla de fuego, proyectiles, plagas o lo que la temática del círculo dicte.

### Estructura de un Boss Fight

```
FASE 1 (100% - 60% HP)
├── 2-3 patrones de ataque básicos
├── Ventanas de castigo claras
└── Introducción a la mecánica del boss

FASE 2 (60% - 30% HP)
├── Patrones nuevos + variaciones de los anteriores
├── Mecánica de arena (suelo cambia, plataformas, etc.)
└── Cinemática breve de transición

FASE 3 (30% - 0% HP)
├── Modo furia: más rápido, más agresivo
├── Patrón especial que llena la pantalla
└── La música se intensifica
```

### Ejemplo: Cerbero (Círculo III — Gula)

| Fase | Ataques | Visual |
|---|---|---|
| **Fase 1** | Mordiscos de cada cabeza (uno a uno), lluvia ácida lenta. | El boss ocupa medio escenario. Baba ácida gotea. |
| **Fase 2** | Dos cabezas atacan simultáneamente, vómito tóxico que deja charcos. | El suelo se va cubriendo de ácido. Espacio seguro se reduce. |
| **Fase 3** | Las tres cabezas atacan a la vez, carga devastadora. | Pantalla tiembla. Lluvia ácida se intensifica. Rugido que distorsiona el audio. |

---

## 9. Dirección Artística

### Paleta de Colores

```
COLORES PRIMARIOS:
██ Negro profundo     (#0A0A0F)  — Fondo, sombras
██ Rojo sangre        (#8B0000)  — Fuego, sangre, peligro
██ Gris ceniza        (#2B2B2B)  — Piedra, ruinas, estructuras

COLORES DE ACENTO:
██ Naranja ardiente   (#CC4400)  — Llamas, lava
██ Dorado corrupto    (#8B7500)  — Riqueza maldita, interfaces
██ Azul hielo         (#1A1A3E)  — Cocytus, traición
██ Verde pútrido      (#2E4A1E)  — Gula, veneno, enfermedad

COLORES DE CONTRASTE:
██ Blanco hueso       (#D4C5A9)  — Texto, almas, highlights
██ Púrpura oscuro     (#2D1B4E)  — Misticismo, fraude
```

### Estilo Visual
- **Pixel art de alta resolución** (estilo Blasphemous / Death's Gambit).
- **Animaciones fluidas** en combate: cada frame de ataque y esquiva debe sentirse pesado y satisfactorio.
- **Partículas abundantes**: chispas, ceniza flotante, ascuas, gotas de sangre, copos de hielo negro.
- **Iluminación dinámica**: Antorchas que parpadean, lava que ilumina desde abajo, destellos de habilidades.
- **Fondos parallax** con profundidad: capas de ruinas, almas torturadas, arquitectura infernal.

### Influencias Visuales
- **Blasphemous** — Pixel art oscuro religioso
- **Dark Souls** — Arquitectura opresiva, escala monumental
- **Gustave Doré** — Ilustraciones clásicas de la Divina Comedia
- **Darkest Dungeon** — Opresión visual, sensación de desesperanza

---

## 10. Audio y Música

### Dirección Musical
- **Coros ominosos** (estilo gregoriano pero distorsionado).
- **Cuerdas graves** que construyen tensión.
- **Percusión tribal/industrial** en combates de boss.
- **Silencio** como herramienta — El Limbo (Círculo I) debería ser casi silencioso, un contraste aterrador.

### Efectos de Sonido
- Golpes con **peso**: impactos que se sienten.
- Fuego crepitante constante como ambientación base.
- Gritos lejanos de almas torturadas (ambiente, no intrusivo).
- Cada boss tiene un **rugido/sonido signature** único.

---

## 11. Mapa de Referencia — Flujo de una Run

```
    ╔══════════════════════════════════════════════════╗
    ║                  S U P E R F I C I E             ║
    ║                    ★ LIBERTAD ★                  ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo I  — LIMBO         🏛️  Boss: Caronte   ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo II — LUJURIA       🌪️  Boss: Minos     ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo III — GULA         🐍  Boss: Cerbero   ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo IV — AVARICIA      💰  Boss: Plutón    ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo V  — IRA           🔥  Boss: Flegias   ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo VI — HEREJÍA       ⚰️  Boss: Farinata  ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo VII — VIOLENCIA    🩸  Boss: Minotauro ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo VIII — FRAUDE      👁️  Boss: Gerión    ║
    ╠══════════════════════════════════════════════════╣
    ║  Círculo IX — TRAICIÓN      ❄️  INICIO (Escape) ║
    ╠══════════════════════════════════════════════════╣
    ║              TÚ ESTÁS AQUÍ ↑                    ║
    ╚══════════════════════════════════════════════════╝
```

---

## 12. Resumen Ejecutivo

| Aspecto | Detalle |
|---|---|
| **Nombre** | INFERNUS |
| **Género** | Roguelike Souls-like 2D |
| **Arte** | Pixel art oscuro, alta resolución |
| **Ambientación** | Infierno de la Divina Comedia |
| **Dirección** | Del Círculo 9 (abajo) al 1 (arriba) |
| **Narrativa** | Alma condenada que escapa aprovechando el caos de Dante |
| **Combate** | Souls-like: stamina, esquivas, patrones, peso |
| **Progresión (run)** | Elige 1 de 3 habilidades + reroll con vida + sinergias temáticas |
| **Progresión (meta)** | Desbloqueo de habilidades, personajes, skins, lore |
| **Bosses** | 9 bosses temáticos, multifase, espectáculo visual |
| **Tono** | Desesperación, determinación, humor oscuro sutil |

---

> *"En el fondo del Infierno, donde nadie mira, una puerta se abrió. Y tú fuiste lo bastante estúpido — o lo bastante valiente — para cruzarla."*

---

**Estado del documento:** 📝 GDD Inicial — v0.1
**Última actualización:** 28 de Febrero, 2026
