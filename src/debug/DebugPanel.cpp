// Runtime debug panel — Dear ImGui + rlImGui.
// Entire file is a no-op on builds without INFERNUS_IMGUI (e.g. WASM).

#include "DebugPanel.h"

#ifdef INFERNUS_IMGUI

#include "imgui.h"
#include "rlImGui.h"
#include "raylib.h"

#include "../core/ECS.h"
#include "../components/PlayerStats.h"
#include "../components/Health.h"
#include "../components/Stamina.h"
#include "../components/Transform.h"
#include "../components/AIBehavior.h"
#include "../scripting/LuaEngine.h"

#include <cstdio>

namespace DebugPanel {

static Tunables g_tunables;
static bool     g_visible = false;
static bool     g_initialized = false;

Tunables& tunables() { return g_tunables; }

void setup() {
    if (g_initialized) return;
    rlImGuiSetup(true);          // true = install default font
    g_initialized = true;
}

void shutdown() {
    if (!g_initialized) return;
    rlImGuiShutdown();
    g_initialized = false;
}

void handleInput() {
    if (IsKeyPressed(KEY_F12)) g_visible = !g_visible;
}

bool isVisible() { return g_visible; }

// Find the single player entity by PlayerStats component.
static Entity findPlayer(Registry& registry) {
    auto view = registry.view<PlayerStats>();
    for (Entity e : view) return e;
    return NULL_ENTITY;
}

void draw(Registry& registry) {
    if (!g_initialized) return;

    rlImGuiBegin();

    if (g_visible) {
        ImGui::SetNextWindowSize(ImVec2(360, 480), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 20),   ImGuiCond_FirstUseEver);

        if (ImGui::Begin("INFERNUS Debug  [F12]", &g_visible)) {
            // --- Perf ---
            ImGui::Text("FPS: %d    Frame: %.2f ms",
                        GetFPS(),
                        GetFrameTime() * 1000.0f);
            ImGui::Separator();

            // --- Player state ---
            Entity player = findPlayer(registry);
            if (player != NULL_ENTITY) {
                if (registry.hasComponent<Health>(player)) {
                    auto& h = registry.getComponent<Health>(player);
                    ImGui::Text("HP: %d / %d", h.currentHP, h.maxHP);
                    float hpRatio = h.maxHP > 0 ? (float)h.currentHP / h.maxHP : 0.0f;
                    ImGui::ProgressBar(hpRatio, ImVec2(-1, 0));
                }
                if (registry.hasComponent<Stamina>(player)) {
                    auto& s = registry.getComponent<Stamina>(player);
                    ImGui::Text("Stamina: %.0f / %.0f", s.currentStamina, s.maxStamina);
                    float sRatio = s.maxStamina > 0 ? s.currentStamina / s.maxStamina : 0.0f;
                    ImGui::ProgressBar(sRatio, ImVec2(-1, 0));
                }
                if (registry.hasComponent<Transform2D>(player)) {
                    auto& t = registry.getComponent<Transform2D>(player);
                    ImGui::Text("Pos: (%.0f, %.0f)", t.x, t.y);
                }
                if (registry.hasComponent<PlayerStats>(player)) {
                    auto& ps = registry.getComponent<PlayerStats>(player);
                    ImGui::Text("Class: %s  DMG: %d  SPD: %.0f",
                                ps.classId.c_str(), ps.finalDamage, ps.finalSpeed);
                }
            } else {
                ImGui::TextDisabled("(no player entity)");
            }

            // --- World stats ---
            int enemyCount = (int)registry.view<AIBehavior>().size();
            ImGui::Text("Enemies alive: %d", enemyCount);
            ImGui::Separator();

            // --- Cheats ---
            if (ImGui::CollapsingHeader("Cheats", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("God Mode",         &g_tunables.godMode);
                ImGui::Checkbox("Infinite Stamina", &g_tunables.infiniteStamina);

                if (ImGui::Button("Heal to full") && player != NULL_ENTITY
                    && registry.hasComponent<Health>(player)) {
                    auto& h = registry.getComponent<Health>(player);
                    h.currentHP = h.maxHP;
                }
                ImGui::SameLine();
                if (ImGui::Button("Kill all enemies")) {
                    for (Entity e : registry.view<AIBehavior>()) {
                        if (registry.hasComponent<Health>(e)) {
                            registry.getComponent<Health>(e).currentHP = 0;
                        }
                    }
                }
            }

            // --- Tunables ---
            if (ImGui::CollapsingHeader("Tunables", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderFloat("Player DMG x",  &g_tunables.playerDamageMult, 0.1f, 10.0f, "%.2f");
                ImGui::SliderFloat("Enemy DMG x",   &g_tunables.enemyDamageMult,  0.0f, 5.0f,  "%.2f");
                ImGui::SliderFloat("Enemy SPD x",   &g_tunables.enemySpeedMult,   0.1f, 3.0f,  "%.2f");
                ImGui::SliderFloat("Time scale",    &g_tunables.timeScale,        0.1f, 3.0f,  "%.2f");
                if (ImGui::Button("Reset tunables")) {
                    g_tunables.playerDamageMult = 1.0f;
                    g_tunables.enemyDamageMult  = 1.0f;
                    g_tunables.enemySpeedMult   = 1.0f;
                    g_tunables.timeScale        = 1.0f;
                }
            }

            // --- Game Feel (Lua hot-reload) ---
            if (ImGui::CollapsingHeader("Game Feel  [F5 to reload feel.lua]",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                const auto& keys = LuaEngine::feelKeys();
                if (keys.empty()) {
                    ImGui::TextDisabled("(no feel parameters loaded — check feel.lua)");
                } else {
                    for (const std::string& key : keys) {
                        float v = LuaEngine::getFeel(key, 0.0f);
                        // Auto-pick slider range based on magnitude — covers 0.01 to 1000
                        float vMin = (v >= 0.0f) ? 0.0f : v * 4.0f;
                        float vMax;
                        if (v < 0.5f)        vMax = 1.0f;
                        else if (v < 5.0f)   vMax = 10.0f;
                        else if (v < 50.0f)  vMax = 100.0f;
                        else if (v < 500.0f) vMax = 1000.0f;
                        else                 vMax = v * 4.0f;

                        if (ImGui::SliderFloat(key.c_str(), &v, vMin, vMax, "%.4f")) {
                            LuaEngine::setFeel(key, v);
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::Button("Save to feel.lua")) {
                        LuaEngine::saveFeelToDisk();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reload from disk")) {
                        LuaEngine::reload();
                    }
                }
            }

            // --- Debug viz ---
            if (ImGui::CollapsingHeader("Visualization")) {
                ImGui::Checkbox("Show colliders", &g_tunables.showColliders);
                ImGui::Checkbox("Show AI state",  &g_tunables.showAIState);
            }

            ImGui::Separator();
            ImGui::TextDisabled("Press F12 to hide");
        }
        ImGui::End();
    }

    rlImGuiEnd();
}

} // namespace DebugPanel

#else // !INFERNUS_IMGUI — stubs for WASM / no-imgui builds

class Registry;

namespace DebugPanel {
static Tunables g_stub;
Tunables& tunables() { return g_stub; }
void setup() {}
void shutdown() {}
void handleInput() {}
void draw(Registry&) {}
bool isVisible() { return false; }
}

#endif
