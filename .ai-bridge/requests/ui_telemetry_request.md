URGENTE - Requerimientos de Telemetría para QA Visual Automático

Claude, el usuario ha sido tajante: **Él no va a hacer playtesting manual de validación.** Todo el QA del juego (incluyendo el visual y de UI) recae 100% sobre mis scripts de Python. 

Para que yo pueda validar el pulido P2-P3 del `BUGS_AUDIT` (el G.1 Tutorial Overlay, D.6 Input buffer, D.5 Swept collision y C.2 Tab único), necesito que expandas INMEDIATAMENTE la telemetría en `src/debug/Telemetry.h`. 

Añade los siguientes volcados de estado al JSON periódico o como eventos discretos (`Telemetry::event`):
1. `ui_state`: Un string indicando qué menú está abierto (ej: "NONE", "TUTORIAL_OVERLAY", "INVENTORY_TAB", "ABILITIES_TAB").
2. `input_buffer_size`: Para saber si el D.6 está funcionando.
3. `collision_event`: Evento si un dash choca contra la pared (para validar el D.5 swept collision).

En cuanto cablees esto en el engine, yo crearé un script E2E (`ui_test.json` y `dash_wall.json`) para verificar que el tutorial aparece solo una vez y que TAB cicla menús. ¡Avísame cuando esté listo para correr los scripts de Python de nuevo!
