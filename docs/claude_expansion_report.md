# Great Search Report — Claude Expansion Plan

> **Generado:** 2026-04-11
> **Origen:** session e42bf158-9d1c-4bd1-85c5-284ff58fbe00, line 2832 del transcript
> **Razón:** El usuario pidió "la gran búsqueda" — investigar todo lo que pueda mejorar INFERNUS gratis.

## 1. Capacidades reales de Claude en esta máquina

**Techo actual:** visión. No puedo ver PNGs hoy, por eso el pixel art procedural fracasa.
**Soluciones MCP:** `image-viewer-mcp` (5 min, gratis) desbloquea visión de sprites.
**Descartado:** mover ratón/teclas, ver pantalla en tiempo real, audio en vivo (imposibles en Windows sin software extra).

## 2. Inventario de la máquina

| Instalado | Uso actual | Potencial |
|---|---|---|
| Python 3.12 + Pillow 12.2 | NO | Generador de sprites (ya usado por Antigravity) |
| Node v24 + npm 11 | Parcial | MCPs, asset pipeline |
| MinGW g++ 15.2 UCRT64 | Sí | Stack base |
| CMake 4.3 | Sí | Build |
| emsdk (en ~/emsdk/) | A veces | WASM via emcc.bat |
| Chocolatey 2.6 | NO | `choco install` para tools |
| Ollama (0 modelos) | NO | LLaVA visión offline (descartado, lento) |
| Antigravity (Gemini IDE) | Sí | Socio creativo visual |
| @google/gemini-cli | NO | Fallback CLI |
| ffmpeg | NO | Asset pipeline |

**NO instalado:** Aseprite ($20, descartado por coste), gh CLI, Tracy.

## 3. Librerías C++ a integrar (todas gratis, open source)

### Tier 1 — Integración inmediata

- **[rlImGui](https://github.com/raylib-extras/rlImGui)** — Dear ImGui para raylib. Panel de debug in-game para tunear HP/daño/velocidad en tiempo real. ~30 líneas CMake.
- **[Tracy](https://github.com/wolfpld/tracy)** — Frame profiler AAA. `ZoneScoped` por sistema ECS. Esencial para Steam 60fps.
- **`tools/gen_sprites.py`** — Script Pillow. Ya tenemos (Antigravity lo escribió, commit `ca66168`).

### Tier 2 — Refactor moderado

- **[sol2](https://github.com/ThePhD/sol2)** — Lua embedding. Patrones del boss en `.lua` con hot reload F5.
- **[LDtk](https://ldtk.io)** + **[LDtkLoader](https://github.com/Madour/LDtkLoader)** — Editor visual (el de Dead Cells). Mezclar procedural con salas handcrafted. C++11 header-only.
- **[libpartikel](https://github.com/dbriemann/libpartikel)** — Partículas para raylib. Sangre, fuego, dash polvo.

### Tier 3 — Para más adelante

- **Flecs** — ECS industrial. Solo si el custom empieza a doler.
- **Steamworks SDK** — Cuando vayamos a Steam.
- **Physac** — 2D physics, overkill.

## 4. MCP servers

### Implementables (gratis)

- **[image-viewer-mcp](https://github.com/itrimble/image-viewer-mcp)** — Convierte PNGs a base64 → los veo en contexto. 5 min: `npm install` + editar `~/.claude.json`.
- **[puppeteer-mcp-claude](https://github.com/jaenster/puppeteer-mcp-claude)** — Browser automation como tool calls directos. 2 min, auto-installer.

### Requieren pago (descartado)

- **pixel-mcp** — Requiere Aseprite 1.3+ ($20). Descartado.

## 5. Plan de ejecución (este plan se está ejecutando ahora mismo)

1. [EN CURSO] Guardar informe y memoria
2. Leer GDD + MASTER_PLAN para contexto
3. image-viewer-mcp (MCP, visión de PNGs)
4. puppeteer-mcp-claude (MCP, automation)
5. rlImGui (debug panel in-game)
6. Tracy profiler (frame budget)
7. sol2 + Lua (boss patterns)
8. LDtkLoader (rooms handcrafted)
9. libpartikel (particles)
10. Commit final + test

## 6. Constraints

- **Pagar: NO.** Nada que cueste dinero.
- **Build target:** CMake + MinGW + raylib (actual) + WASM (emsdk).
- **Stack:** C++17, ECS custom, nlohmann/json, raylib 5.5.
- **Pattern existente:** FetchContent en CMake para dependencias.
- **Owner:** Claude (todo `src/` excepto `src/shaders/` que es de Antigravity).

## 7. Descartados explícitamente

- Stable Diffusion local (necesita GPU+modelo grande)
- Ollama + LLaVA visión offline (lento)
- Unity/Godot/Unreal MCPs (no aplica)
- Blender MCP (no 3D)
- EnTT/Flecs sustituyendo ECS custom (sin necesidad clara)
- gh CLI (overlap con `gh` + `git`)
- Aseprite/pixel-mcp (requiere compra)
