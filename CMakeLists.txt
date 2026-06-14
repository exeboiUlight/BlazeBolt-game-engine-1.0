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
    src/stb_image_impl.cpp
    core/engine/luaEngine.cpp
    core/engine/managers.cpp
    core/graphics/animatedTexture2D.cpp
    core/graphics/gl.cpp
    core/graphics/quad.cpp
    core/graphics/spriteBatch2D.cpp
    core/physics/body.cpp
    core/physics/collision.cpp
    core/physics/world.cpp
    core/subject/sprite/animatedSprite2D.cpp
    core/subject/sprite/generalSprite.cpp
    core/subject/sprite/staticSprite2D.cpp
    core/subject/camera2D.cpp
    core/subject/particle2D.cpp
    core/subject/text2D.cpp
    core/utils/net/client.cpp
    core/utils/net/server.cpp
    core/utils/dr_mp3.cpp
    core/utils/input.cpp
    core/scene.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    core
    libs/headeronly/include
    libs/glfw/include
    libs/glad/include
    libs/freetype/include
    libs/openal/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    freetype
    lua
    openal
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()