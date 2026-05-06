#include "LuaEngine.h"

#ifdef INFERNUS_LUA

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "raylib.h"
#include <cstdio>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace LuaEngine {

static sol::state* g_lua = nullptr;
static const char* BOSS_SCRIPT = "assets/scripts/boss_patterns.lua";
static const char* FEEL_SCRIPT = "assets/scripts/feel.lua";

// Game feel cache: populated from feel.lua on reload(), read at zero overhead
// from systems via getFeel(name, fallback). ImGui sliders mutate via setFeel().
static std::unordered_map<std::string, float> g_feelCache;
static std::vector<std::string> g_feelKeys; // ordered list for UI iteration

static void populateFeelCache() {
    g_feelCache.clear();
    g_feelKeys.clear();
    if (!g_lua) return;

    sol::optional<sol::table> feelTable = (*g_lua)["feel"];
    if (!feelTable) {
        TraceLog(LOG_WARNING, "LUA: feel table not found in feel.lua");
        return;
    }

    for (const auto& kv : feelTable.value()) {
        if (kv.first.get_type() != sol::type::string) continue;
        if (kv.second.get_type() != sol::type::number) continue;
        std::string key = kv.first.as<std::string>();
        float value = kv.second.as<float>();
        g_feelCache[key] = value;
        g_feelKeys.push_back(key);
    }
    TraceLog(LOG_INFO, "LUA: feel cache populated (%d entries)", (int)g_feelCache.size());
}

void setup() {
    if (g_lua) return;
    g_lua = new sol::state();
    g_lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    reload();
}

void shutdown() {
    delete g_lua;
    g_lua = nullptr;
    g_feelCache.clear();
    g_feelKeys.clear();
}

void reload() {
    if (!g_lua) return;

    // Load boss patterns
    auto bossResult = g_lua->safe_script_file(BOSS_SCRIPT, sol::script_pass_on_error);
    if (!bossResult.valid()) {
        sol::error err = bossResult;
        TraceLog(LOG_WARNING, "LUA: %s — %s", BOSS_SCRIPT, err.what());
    } else {
        TraceLog(LOG_INFO, "LUA: Loaded %s", BOSS_SCRIPT);
    }

    // Load game feel
    auto feelResult = g_lua->safe_script_file(FEEL_SCRIPT, sol::script_pass_on_error);
    if (!feelResult.valid()) {
        sol::error err = feelResult;
        TraceLog(LOG_WARNING, "LUA: %s — %s", FEEL_SCRIPT, err.what());
    } else {
        TraceLog(LOG_INFO, "LUA: Loaded %s", FEEL_SCRIPT);
        populateFeelCache();
    }
}

void handleInput() {
    if (IsKeyPressed(KEY_F5)) {
        TraceLog(LOG_INFO, "LUA: Hot-reloading scripts...");
        reload();
    }
}

float getFeel(const std::string& name, float fallback) {
    auto it = g_feelCache.find(name);
    if (it == g_feelCache.end()) return fallback;
    return it->second;
}

void setFeel(const std::string& name, float value) {
    auto it = g_feelCache.find(name);
    if (it == g_feelCache.end()) {
        // New key — append to ordered list
        g_feelKeys.push_back(name);
    }
    g_feelCache[name] = value;
}

bool hasFeel(const std::string& name) {
    return g_feelCache.find(name) != g_feelCache.end();
}

const std::vector<std::string>& feelKeys() {
    return g_feelKeys;
}

// === Preset slots ===
// Each slot is a separate .lua file at assets/scripts/feel_preset_<slot>.lua.
// Saving captures the current cache; loading repopulates the cache without
// touching feel.lua. Useful for A/B testing values during a single session.

static std::string presetPath(int slot) {
    char buf[64];
    snprintf(buf, sizeof(buf), "assets/scripts/feel_preset_%d.lua", slot);
    return std::string(buf);
}

void savePreset(int slot) {
    if (slot < 1 || slot > 3) return;
    std::ofstream out(presetPath(slot));
    if (!out.is_open()) {
        TraceLog(LOG_WARNING, "LUA: Could not open preset slot %d for writing", slot);
        return;
    }
    out << "-- INFERNUS - Preset slot " << slot << "\n";
    out << "-- Snapshot of feel.lua at the moment of save.\n";
    out << "feel = {\n";
    for (const auto& key : g_feelKeys) {
        out << "    " << key << " = " << g_feelCache[key] << ",\n";
    }
    out << "}\n";
    out.close();
    TraceLog(LOG_INFO, "LUA: Saved preset slot %d", slot);
}

void loadPreset(int slot) {
    if (!g_lua) return;
    if (slot < 1 || slot > 3) return;
    std::string path = presetPath(slot);
    auto result = g_lua->safe_script_file(path, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        TraceLog(LOG_WARNING, "LUA: Could not load preset %d — %s", slot, err.what());
        return;
    }
    populateFeelCache();
    TraceLog(LOG_INFO, "LUA: Loaded preset slot %d", slot);
}

bool presetExists(int slot) {
    if (slot < 1 || slot > 3) return false;
    std::ifstream f(presetPath(slot));
    return f.good();
}

void saveFeelToDisk() {
    // Write current cache values back to feel.lua, preserving the format.
    // Strategy: rewrite the file with current cache. Comments are preserved
    // only for the `feel = {}` block header. Keys are written sorted by
    // their original load order (g_feelKeys preserves insertion order).
    std::ofstream out(FEEL_SCRIPT);
    if (!out.is_open()) {
        TraceLog(LOG_WARNING, "LUA: Could not open %s for writing", FEEL_SCRIPT);
        return;
    }

    out << "-- INFERNUS - Game Feel Tuning\n";
    out << "-- Hot-reload con F5. Modificable en vivo desde DebugPanel (F12).\n";
    out << "-- Auto-guardado desde DebugPanel \"Save preset\".\n\n";
    out << "feel = {\n";

    // Find max key length for alignment
    size_t maxLen = 0;
    for (const auto& k : g_feelKeys) {
        if (k.length() > maxLen) maxLen = k.length();
    }

    for (const auto& key : g_feelKeys) {
        out << "    " << key;
        for (size_t i = key.length(); i < maxLen; i++) out << ' ';
        out << " = " << g_feelCache[key] << ",\n";
    }

    out << "}\n";
    out.close();

    TraceLog(LOG_INFO, "LUA: Saved feel.lua (%d entries)", (int)g_feelCache.size());
}

std::string selectBossPattern(int phase, float hpRatio,
                              const std::vector<std::string>& patterns) {
    if (!g_lua) return patterns.empty() ? "charge" : patterns[0];

    sol::optional<sol::function> fn = (*g_lua)["select_boss_pattern"];
    if (!fn) {
        return patterns.empty() ? "charge" : patterns[GetRandomValue(0, (int)patterns.size() - 1)];
    }

    sol::table luaPatterns = g_lua->create_table();
    for (int i = 0; i < (int)patterns.size(); ++i) {
        luaPatterns[i + 1] = patterns[i];
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

#include <unordered_map>

namespace LuaEngine {
static std::vector<std::string> g_emptyKeys;

void setup() {}
void shutdown() {}
void reload() {}
void handleInput() {}
float getFeel(const std::string&, float fallback) { return fallback; }
void  setFeel(const std::string&, float) {}
bool  hasFeel(const std::string&) { return false; }
void  saveFeelToDisk() {}
const std::vector<std::string>& feelKeys() { return g_emptyKeys; }
void  savePreset(int) {}
void  loadPreset(int) {}
bool  presetExists(int) { return false; }
std::string selectBossPattern(int, float, const std::vector<std::string>& patterns) {
    return patterns.empty() ? "charge" : patterns[0];
}
}

#endif
