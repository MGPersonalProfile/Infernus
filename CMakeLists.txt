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
endif()

# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# ---- Main Executable --------------------------------------------------------
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)

# ---- Tests -------------------------------------------------------------------

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

# ---- Asset Generator ---------------------------------------------------------
add_executable(asset_generator tools/asset_generator.cpp)
target_link_libraries(asset_generator PRIVATE raylib)
set_target_properties(asset_generator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
