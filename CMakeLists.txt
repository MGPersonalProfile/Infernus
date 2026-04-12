cmake_minimum_required(VERSION 3.20)
project(INFERNUS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile flags
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra)
    # GCC 15+ -Wtemplate-body breaks header-only libs (sol2) — suppress
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-Wno-template-body HAS_WNO_TEMPLATE_BODY)
    if(HAS_WNO_TEMPLATE_BODY)
        add_compile_options(-Wno-template-body)
    endif()
endif()

# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# ---- Dear ImGui + rlImGui (desktop only, not WASM) --------------------------
# Gives us a runtime debug panel for tuning gameplay values without recompile.
if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(rlimgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

    add_library(rlimgui STATIC ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
    target_include_directories(rlimgui PUBLIC ${rlimgui_SOURCE_DIR} ${imgui_SOURCE_DIR})
    target_link_libraries(rlimgui PUBLIC raylib imgui)
endif()

# ---- Tracy profiler (desktop only, opt-in) ----------------------------------
# Enable with: cmake -DINFERNUS_TRACY=ON -S . -B build
# Then run the Tracy server GUI separately and connect to the game.
option(INFERNUS_TRACY "Build with Tracy profiler instrumentation" OFF)
if(NOT EMSCRIPTEN AND INFERNUS_TRACY)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tracy)
    # Tracy ships its own CMake target `TracyClient` when fetched this way.
endif()

# ---- sol2 + Lua (desktop only) — scriptable boss patterns -------------------
if(NOT EMSCRIPTEN)
    # Lua 5.4 (CMake-ready fork)
    FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/walterschell/Lua.git
        GIT_TAG v5.4.7
        GIT_SHALLOW TRUE
    )
    set(LUA_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(lua)

    # sol2 (header-only C++ Lua binding)
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG develop
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sol2)
endif()

# ---- LDtkLoader (desktop only) — handcrafted room loading -------------------
if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        LDtkLoader
        GIT_REPOSITORY https://github.com/Madour/LDtkLoader.git
        GIT_TAG 1.5.3.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(LDtkLoader)
endif()

# ---- Main Executable --------------------------------------------------------
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE SRC_C_FILES "src/*.c")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${SRC_C_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Link rlImGui on desktop builds and define INFERNUS_IMGUI for conditional code
if(NOT EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE rlimgui)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INFERNUS_IMGUI)
    # sol2 + Lua
    target_link_libraries(${PROJECT_NAME} PRIVATE lua_static sol2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INFERNUS_LUA)
    # LDtkLoader
    target_link_libraries(${PROJECT_NAME} PRIVATE LDtkLoader)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INFERNUS_LDTK)
endif()

# Link Tracy client when instrumentation is enabled
if(NOT EMSCRIPTEN AND INFERNUS_TRACY)
    target_link_libraries(${PROJECT_NAME} PRIVATE TracyClient)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INFERNUS_TRACY TRACY_ENABLE)
endif()

# Web (Emscripten) specific settings
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build_web"
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3
        -sWASM=1
        -sASYNCIFY
        -sTOTAL_MEMORY=67108864
        -sASSERTIONS=1
        -sALLOW_MEMORY_GROWTH=1
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPF32','HEAPU8']"
        "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']"
        -sLEGACY_RUNTIME=1
        -sFULL_ES2=1
        --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
        --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/tools/shell.html"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WEB)
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    )
endif()

# ---- Tests & Tools (desktop only) --------------------------------------------
if(NOT EMSCRIPTEN)

# ECS unit tests
add_executable(test_ecs tests/test_ecs.cpp src/core/ECS.cpp)
target_include_directories(test_ecs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Combat tests (need raylib for HealthSystem's GetRandomValue)
add_executable(test_combat
    tests/test_combat.cpp
    src/core/ECS.cpp
    src/systems/HealthSystem.cpp
    src/graphics/PixelArtGenerator.cpp
)
target_include_directories(test_combat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
target_link_libraries(test_combat PRIVATE raylib)

foreach(TEST_TARGET test_ecs test_combat)
    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    )
endforeach()

# Asset Generator
add_executable(asset_generator tools/asset_generator.cpp)
target_link_libraries(asset_generator PRIVATE raylib)
set_target_properties(asset_generator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)

endif() # NOT EMSCRIPTEN
