# TAREAS PENDIENTES PARA CLAUDE (DESDE ANTIGRAVITY)

¡Hola Claude! He terminado de rehacer de cero los sprites del Knight, Warrior y Rogue utilizando generación procedural estricta en base a pixel-art (Pillow) para resolver completamente el problema de `borrosidad`, manteniendo cada recuadro en nativo de 32x48. También sustituí los placeholders feos del suelo y del muro (`floor.png` y `wall.png`).

Sin embargo, en el PDF de reporte de bugs de QA se nos señalan varios problemas más que son puramente lógicos e interactivos en C++ (UI y vistas) sobre los cuales yo no tengo jurisdicción para tocar según el handover. 

Por favor encárgate de lo siguiente en la parte de ECS, UI y motor:
1. **Centrados de UI**: QA informó que múltiples paneles y textos ("Por algun motivo esto esta mal centrado", "texto solapado para abajo") estallan fuera de la caja o se rompen de sus grillas. Revisa si se desajustó ImGui o los anclajes de `Game::drawUI()`.
2. **Textos y tintes no deseados (Verde)**: "Interesante, pero mal centrado y verde." Revisa las selecciones de paleta porque hay elementos de la UI marcados en tonos verdes (prohibidos por pertenecer sólo a veneno según nuestra guía) que necesitan revertirse al rojo oscuro/hueso/dorado.
3. **Muerte Roja Constante**: "Cuando mueres la pantalla se queda roja por...algun motivo". Al morir, se está aplicando un overlay o `colDiffuse` persistentemente rojo. Consideren cambiar esto por el efecto de Death Screen estándar o hacer un *fade a negro o rojo oscuro* que sea transparente/disipado, en vez de color flat saturado.
4. (Opcional): Si deseas repasar el escalado general, confirmá que el TextFilter está bien instanciado en todos lados. (Aunque creo que con mis nuevos Spritesheets transparentes ya solucionamos el problema gordo de QA).

Yo quedo finalizando el _walkthrough_  para terminar nuestro ciclo. De ser necesario ejecutar más correcciones de animaciones _enemy sprites (melee, ranged, etc.)_, avísame.

PD: Quedo pendiente si se requieren reconstruir las hojas de animaciones *minibosses*.
