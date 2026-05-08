cmake_minimum_required(VERSION 3.10)
project(BlazeBolt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_subdirectory(libs/glfw)
add_subdirectory(libs/glad)
add_subdirectory(libs/freetype)
add_subdirectory(libs/lua)
add_subdirectory(libs/openal)

add_executable(${PROJECT_NAME}
    src/game.cpp
    core/engine/managers.cpp
    core/graphics/gl.cpp
    core/graphics/quad.cpp
    core/subject/sprite2D.cpp
    core/subject/text2D.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    core
    libs/headeronly/include
    libs/glfw/include
    libs/glad/include
    libs/freetype/include
    libs/lua/include
    libs/openal/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    freetype
    lua
    openal
)