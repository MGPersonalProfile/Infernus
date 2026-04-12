#pragma once
// Lua scripting engine — sol2 binding over Lua 5.4.
// Desktop only: all calls are no-ops when INFERNUS_LUA is not defined.
// Hot-reload scripts with F5 at runtime.

#include <string>
#include <vector>

namespace LuaEngine {

// Lifecycle — call setup() after game init, shutdown() before exit.
void setup();
void shutdown();

// Reload all scripts from disk (call on F5 for hot-reload).
void reload();

// Input — call once per frame. Checks F5 for hot-reload.
void handleInput();

// Query: ask Lua to pick the next boss pattern.
// `phase`    — current phase index (0-based)
// `hpRatio`  — boss HP fraction [0..1]
// `patterns` — available pattern names from JSON
// Returns a pattern name string. Falls back to random C++ pick on error.
std::string selectBossPattern(int phase, float hpRatio,
                              const std::vector<std::string>& patterns);

} // namespace LuaEngine
