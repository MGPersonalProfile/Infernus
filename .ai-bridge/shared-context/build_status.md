<!-- topic:build_status | updated:2026-04-14T20:16:46.683058 | by:claude-code -->

# Build Status — 2026-04-14

## Estado: COMPILA LIMPIO ✓

Build: `cmake -S . -B build -G "MinGW Makefiles" && mingw32-make -C build -j8`

## Fixes aplicados hoy (bugs del PDF):
1. ✅ Shader CRT_Vignette.fs — removida aberración cromática (causaba blur)
2. ✅ Ability select centrado verticalmente (era mainY=30, ahora centrado)
3. ✅ Shop colors: verde → rojo/naranja infernal
4. ✅ Rest room colors: azul → ámbar cálido
5. ✅ Death screen: flash rojo ahora se desvanece correctamente en GAME_OVER
6. ✅ Player IDLE frame count: 2→6 (bug real)
7. ✅ Animation data-driven: frame counts en JSON para enemies y boss

## Pendiente código:
- Wall collision testing (Bloque A.1)
- 10 full playtest runs (Bloque A.4)
