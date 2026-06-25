cmake_minimum_required(VERSION 3.16)
project(BlazeBolt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -s -Wl,--gc-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/include/glad)
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/../libs)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${IMGUI_DIR}/texteditor/TextEditor.cpp
    ${IMGUI_DIR}/crude_json.cpp
)

set(CORE_DIR ${CMAKE_SOURCE_DIR}/../core)
set(_SRC_DIR ${CMAKE_SOURCE_DIR}/../src)

set(ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/hub.cpp
    ${CMAKE_SOURCE_DIR}/src/editor.cpp
    ${GLAD_DIR}/glad.c
    ${CORE_DIR}/graphics/gl.cpp
    ${CORE_DIR}/graphics/quad.cpp
    ${CORE_DIR}/subject/text2D.cpp
    # Engine runtime sources (for in-editor game execution)
    ${CORE_DIR}/engine/luaEngine.cpp
    ${CORE_DIR}/engine/managers.cpp
    ${CORE_DIR}/graphics/animatedTexture2D.cpp
    ${CORE_DIR}/graphics/spriteBatch2D.cpp
    ${CORE_DIR}/physics/body.cpp
    ${CORE_DIR}/physics/collision.cpp
    ${CORE_DIR}/physics/world.cpp
    ${CORE_DIR}/subject/sprite/animatedSprite2D.cpp
    ${CORE_DIR}/subject/sprite/animationWheel.cpp
    ${CORE_DIR}/subject/sprite/generalSprite.cpp
    ${CORE_DIR}/subject/sprite/staticSprite2D.cpp
    ${CORE_DIR}/subject/camera2D.cpp
    ${CORE_DIR}/subject/particle2D.cpp
    ${CORE_DIR}/subject/tileset2D.cpp
    ${CORE_DIR}/subject/light2D.cpp
    ${CORE_DIR}/utils/net/client.cpp
    ${CORE_DIR}/utils/net/server.cpp
    ${CORE_DIR}/utils/dr_mp3.cpp
    ${CORE_DIR}/utils/input.cpp
    ${CORE_DIR}/scene.cpp
    ${_SRC_DIR}/stb_image_impl.cpp
)

add_subdirectory(${LIBS_DIR}/glfw glfw_build)
add_subdirectory(${LIBS_DIR}/freetype freetype_build)
add_subdirectory(${LIBS_DIR}/lua lua_build)
add_subdirectory(${LIBS_DIR}/openal openal_build)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBS_DIR}/glfw/include
    ${LIBS_DIR}/freetype/include
    ${LIBS_DIR}/headeronly/include
    ${LIBS_DIR}/lua/lua-5.4.7/include
    ${LIBS_DIR}/openal/include
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/freetype
    ${IMGUI_DIR}/texteditor
    ${GLAD_DIR}
    ${CORE_DIR}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        freetype
        lua_static
        OpenAL
        opengl32
        gdi32
        user32
        kernel32
        shell32
        winmm
        dwmapi
        ws2_32
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_WINDOWS
        IMGUI_IMPL_OPENGL_LOADER_GLAD
        GLFW_INCLUDE_NONE
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
else()
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        freetype
        lua_static
        OpenAL
        OpenGL::GL
        ${X11_LIBRARIES}
        dl
        pthread
        m
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_LINUX
        IMGUI_IMPL_OPENGL_LOADER_GLAD
        GLFW_INCLUDE_NONE
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_FREETYPE_HARFBUZ=0
    IMGUI_DISABLE_DEMO_WINDOWS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/arial.ttf $<TARGET_FILE_DIR:${PROJECT_NAME}>/arial.ttf
)

if(EXISTS "${CMAKE_SOURCE_DIR}/void.zip")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/void.zip $<TARGET_FILE_DIR:${PROJECT_NAME}>/void.zip
    )
endif()
