#include "LuaEngine.h"

#ifdef INFERNUS_LUA

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "raylib.h"
#include <cstdio>

namespace LuaEngine {

static sol::state* g_lua = nullptr;
static const char* BOSS_SCRIPT = "assets/scripts/boss_patterns.lua";

void setup() {
    if (g_lua) return;
    g_lua = new sol::state();
    g_lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    reload();
}

void shutdown() {
    delete g_lua;
    g_lua = nullptr;
}

void reload() {
    if (!g_lua) return;
    auto result = g_lua->safe_script_file(BOSS_SCRIPT, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        TraceLog(LOG_WARNING, "LUA: %s", err.what());
    } else {
        TraceLog(LOG_INFO, "LUA: Loaded %s", BOSS_SCRIPT);
    }
}

void handleInput() {
    if (IsKeyPressed(KEY_F5)) {
        TraceLog(LOG_INFO, "LUA: Hot-reloading scripts...");
        reload();
    }
}

std::string selectBossPattern(int phase, float hpRatio,
                              const std::vector<std::string>& patterns) {
    if (!g_lua) return patterns.empty() ? "charge" : patterns[0];

    sol::optional<sol::function> fn = (*g_lua)["select_boss_pattern"];
    if (!fn) {
        // Function not defined in Lua — fall back to random
        return patterns.empty() ? "charge" : patterns[GetRandomValue(0, (int)patterns.size() - 1)];
    }

    // Convert pattern list to a Lua table
    sol::table luaPatterns = g_lua->create_table();
    for (int i = 0; i < (int)patterns.size(); ++i) {
        luaPatterns[i + 1] = patterns[i]; // Lua 1-indexed
    }

    auto result = fn.value()(phase, hpRatio, luaPatterns);
    if (result.valid() && result.get_type() == sol::type::string) {
        return result.get<std::string>();
    }

    TraceLog(LOG_WARNING, "LUA: select_boss_pattern returned invalid result, falling back");
    return patterns.empty() ? "charge" : patterns[GetRandomValue(0, (int)patterns.size() - 1)];
}

} // namespace LuaEngine

#else // !INFERNUS_LUA — stubs

namespace LuaEngine {
void setup() {}
void shutdown() {}
void reload() {}
void handleInput() {}
std::string selectBossPattern(int, float, const std::vector<std::string>& patterns) {
    return patterns.empty() ? "charge" : patterns[0];
}
}

#endif
