# C++ / Raylib Expansion Roadmap (Options for Claude)

## 1. Developer Tooling: Dear ImGui (`rlImGui`)
*   **Repo**: [https://github.com/raylib-extras/rlImGui](https://github.com/raylib-extras/rlImGui)
*   **Purpose**: Real-time Engine Debugging & Variable Tweaking.
*   **Implementation Steps**:
    1.  Add `rlImGui` as a submodule to `_deps`.
    2.  Update `CMakeLists.txt` to link `rlImGui` alongside Raylib.
    3.  Call `rlImGuiSetup()` in `Game::init()` and `rlImGuiShutdown()` in `Game::shutdown()`.
    4.  Wrap an ImGui window inside `Game::render()` (e.g. F12 toggle) for dynamic ECS component editing (Health, Speed, Shader parameters).

## 2. Scripting Integration: Lua + `sol2`
*   **Repo**: [https://github.com/ThePhD/sol2](https://github.com/ThePhD/sol2)
*   **Purpose**: Decouple Ability logic, Synergy math, and AI behavior from C++. Avoid C++ recompilation just to change balance numbers.
*   **Implementation Steps**:
    1.  Fetch `lua` and `sol2` via `FetchContent` or package manager.
    2.  Create `ScriptManager` wrapper in `src/core/`.
    3.  Bind standard ECS calls and math logic to Lua state.
    4.  Migrate `Game::executeSpecialAttack()` logic into `<ability_name>.lua`.

## 3. Handcrafted Level Design: LDtk + JSON Parser
*   **Tool**: [https://ldtk.io/](https://ldtk.io/)
*   **Purpose**: Import custom-built Boss arenas and non-procedural set pieces.
*   **Implementation Steps**:
    1.  Design `.ldtk` maps exporting to standard JSON.
    2.  Expand `RoomGenerator.cpp` to include a `RoomGenerator::instantiateFromLDtk(file_path)` method using the existing `nlohmann/json.hpp`.
    3.  Map LDTK layers to `Collider` generation and `Sprite` instantiation.

## 4. Frame Profiling: Tracy Profiler
*   **Repo**: [https://github.com/wolfpld/tracy](https://github.com/wolfpld/tracy)
*   **Purpose**: Identify CPU bottlenecks in EnTT system iteration, collision detection (`CollisionSystem.cpp`), or memory allocations.
*   **Implementation Steps**:
    1.  Fetch `tracy`.
    2.  Add `#ifdef TRACY_ENABLE` macros around major loops (`Game::update`, `CombatSystem::update`).
    3.  Compile with `TRACY_ENABLE` flag and run the external GUI client to trace frame budget.

## 5. WASM Deployment: Emscripten Toolchain
*   **Tool**: Emscripten SDK (`emsdk`).
*   **Purpose**: Compile C++ directly to `index.html` (WebAssembly) for zero-installation browser sessions.
*   **Implementation Steps**:
    1.  Install Emscripten locally.
    2.  Adapt CMake build using `emcmake cmake ..`.
    3.  Ensure asynchronous operations (if any) follow WebGL blocking rules.
    4.  Publish via Itch.io.
(revisa esto algun dia: https://www.reddit.com/r/aigamedev/comments/1sejfv0/we_got_tired_of_cloud_ai_subscriptions_and_python/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button)