# INFERNUS

> *"Lasciate ogne speranza, voi ch'intrate"*

Roguelite side-scroller souls-like ambientado en el Infierno de Dante.
Side-scroller 2D pixel art. Inspiración de feel: **Skul: The Hero Slayer**
y **Dead Cells**. Tono: **Blasphemous + Gustave Doré** (referencias del GDD).

Construido por **dos IAs** (Claude para sistemas/código, Antigravity para
arte) dirigidas por **Juan Miguel** (director creativo).

## Estado

Reescritura desde cero en **Godot 4 + GDScript**. Versión anterior
(C++/Raylib, top-down) descartada — divergía del pilar side-scroller del
GDD. Ver `git log` del commit `418e88c` para forensics.

## Cómo abrir

1. Tener `Godot_v4.6.x-stable_win64.exe` en la raíz (o accesible).
2. Abrir Godot, **Importar**, seleccionar `project.godot`.
3. Play (F5) — debería ver un Label con "INFERNUS — setup ok".

## Estructura

```
project.godot         Configuración Godot
scenes/               Escenas .tscn
  main.tscn           Escena raíz (placeholder)
src/
  engine/             Capa Engine — reusable, agnóstica al juego
  game/               Capa Game — lógica concreta del juego
content/              Data del juego (enemies, abilities, items, etc)
assets/               Sprites, audio, fonts, icons
docs/
  GDD_INFERNUS.md     La biblia del diseño
  ARCHITECTURE.md     Decisiones arquitectónicas
references/           Capturas / refs visuales (que el director sube)
```

## Documentación clave

- `docs/GDD_INFERNUS.md` — Game Design Document (el norte)
- `docs/ARCHITECTURE.md` — Arquitectura del código (cómo organizamos)
