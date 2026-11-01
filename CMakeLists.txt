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

file(GLOB SOURCES "src/*.cpp")

# --- ВИПРАВЛЕННЯ: Спершу створюємо виконуваний файл таргета ---
add_executable(${PROJECT_NAME} ${SOURCES})

# Add source to this project's executable.
target_include_directories(${PROJECT_NAME} PRIVATE . ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

# ==================== 2. ЗВ'ЯЗУВАННЯ З RAYLIB ===================
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
# ===============================================================


# для мапи
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/map
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/map
)

# TODO: Add tests and install targets if needed.