#pragma once
// Runtime debug panel powered by Dear ImGui + rlImGui.
// All calls compile to no-ops on builds without INFERNUS_IMGUI (e.g. WASM).
// Toggle visibility with F12 in game.

class Registry;

namespace DebugPanel {

// Global tunables the panel can mutate at runtime. Systems read these each frame.
// Defaults match vanilla gameplay, so the panel is purely additive.
struct Tunables {
    bool  godMode            = false;
    bool  infiniteStamina    = false;
    float playerDamageMult   = 1.0f;   // multiplies outgoing damage
    float enemyDamageMult    = 1.0f;   // multiplies incoming damage
    float enemySpeedMult     = 1.0f;
    float timeScale          = 1.0f;   // slows/speeds the whole game
    bool  showColliders      = false;
    bool  showAIState        = false;
};

Tunables& tunables();

// Lifecycle
void setup();           // call once after InitWindow()
void shutdown();        // call once before CloseWindow()

// Input — call once per frame in update(), before game logic. Toggles F12.
void handleInput();

// Rendering — call inside BeginDrawing()/EndDrawing(), after your game frame.
// Uses the Registry to introspect player/enemy state. Zero Game coupling.
void draw(Registry& registry);

bool isVisible();

} // namespace DebugPanel
