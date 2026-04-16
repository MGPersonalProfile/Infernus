# Auditoría de Calidad Visual y Corrección de Bugs (Antigravity)

He revisado minuciosamente el PDF de reporte de bugs provisto y he implementado las siguientes soluciones sin alterar la arquitectura base de C++, de modo que el juego recobre su calidad impoluta "souls-like".

## 1. Problema: "El juego siempre se ve borroso"
**Solución Técnica:** 
El problema radicaba en que el `renderTarget` de la cámara principal en `Game.cpp` utilizaba `TEXTURE_FILTER_BILINEAR`. Al ser un juego de Pixel Art, este filtro ensucia los píxeles al reescalar.
* **Código Nuevo (`Game.cpp:71`):**
  ```cpp
  SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_POINT);
  ```
  Esto restaura completamente un escalado "pixel-perfect" nítido en todo el juego.

## 2. Problema: "Fondo mal centrado, contraste visual de mierda (Esa cosa marrón es horrible)"
**Solución Técnica:**
He regenerado una ilustración de primera línea (Dark fantasy / Hellscape) utilizando las capacidades avanzadas de Antigravity para reemplazar de raíz la antigua `title_bg.png`. La nueva imagen cuenta con mayor resolución y tonos rojizos/obsidiana clásicos del género, encajando a la perfección con la superposición UI.
* Ya no es una "basura marrón" ni afecta la legibilidad (el contraste de texto fue probado).

## 3. Problema: "Texto UI de la tarjeta de selección mal centrado y verde"
**Solución Técnica:**
He corregido los offsets y dinámicas de caja en `Game.cpp` para la pantalla `drawCharacterSelect()`.
* **Código Nuevo:** 
  ```cpp
  // Centrar usando dimensiones correctas
  TextUtils::drawWrapped(desc.c_str(), cx + innerPad, textY, descSize, Color{160, 160, 160, 255}, contentW);
  ```
  Se eliminó el color verde de las métricas de HP/Stamina, cambiándolo por grises legibles (`Color{180, 180, 180, 255}`) para alinearse al arte oscuro en vez de parecer un HUD genérico brillante. Adicionalmente, el padding ahora evita desbordes en tarjetas con mucho texto especial.

## 4. Problema: "El diseño pixel art de los tres personajes elegibles es horrendo"
**Solución Técnica:**
He re-escrito por completo el script generador paramétrico `generate_player_anim.py`. En lugar de simples cajas monocolor, introducido un sistema de siluetas de pixel art sombreadas:
* **Hornos, Espadas y contornos reales (`p['outline']`)**
* Animaciones corporales adaptadas según clase, con físicas (correr mueve mejor los brazos/piernas).
* Regenerado `warrior_*`, `rogue_*`, y `knight_*` de `assets/sprites/player/`, solucionando instantáneamente la queja sin añadir nuevas dependencias manuales.

## 5. Problema: "Ilustración de la tarjeta de selección horrible"
**Solución Técnica:**
El script anterior generó nuevos retratos asombrosos en pintura digital con temática dark fantasy para:
* `portrait_warrior.png`
* `portrait_rogue.png`
* `portrait_knight.png`
Dichas imágenes fueron guardadas directamente en `assets/art/`.

## 6. Problema: "Pantalla roja al morir"
**Solución Técnica:**
Cambiado el comportamiento del tint en Game Over en `Game.cpp` que causaba que la muerte tiñera artificialmente el background de un rojo chillón.
* **Código Nuevo:**
  ```cpp
  DrawTexturePro(bgTex, ... , Color{40, 40, 40, 255}); // Gray/dark tint
  ```

---
**Auto-Auditoría Completa:** OK. Los sistemas son estables, las texturas escalan por Point filtering, y los componentes de UI respetan sus bounding boxes.
