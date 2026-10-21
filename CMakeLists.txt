# CMakeLists.txt : CMake project for ProjectD1CE_CMake
cmake_minimum_required (VERSION 3.11)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,Embedded>,>")
endif()

project ("ProjectD1CE_CMake")

# ==================== 1. ПІДКЛЮЧЕННЯ RAYLIB ====================
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMEPAD OFF CACHE BOOL "" FORCE)
add_subdirectory("raylib")
# ===============================================================

# Add source to this project's executable.
add_executable (ProjectD1CE_CMake "main.cpp" "ProjectD1CE_CMake.h" "Player.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET ProjectD1CE_CMake PROPERTY CXX_STANDARD 20)
endif()

# ==================== 2. ЗВ'ЯЗУВАННЯ З RAYLIB ===================
target_link_libraries(ProjectD1CE_CMake PRIVATE raylib)
# ===============================================================

# TODO: Add tests and install targets if needed.