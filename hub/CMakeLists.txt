cmake_minimum_required(VERSION 3.16)
project(BlazeBolt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -s -Wl,--gc-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/include/glad)
set(STB_DIR ${CMAKE_SOURCE_DIR}/include/stb_image)
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
)

set(ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/hub.cpp
    ${CMAKE_SOURCE_DIR}/src/editor.cpp
    ${CMAKE_SOURCE_DIR}/src/node_editor.cpp
    ${GLAD_DIR}/glad.c
)

add_subdirectory(${LIBS_DIR}/glfw glfw_build)
add_subdirectory(${LIBS_DIR}/freetype freetype_build)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBS_DIR}/glfw/include
    ${LIBS_DIR}/freetype/include
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/freetype
    ${IMGUI_DIR}/texteditor
    ${GLAD_DIR}
    ${STB_DIR}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        freetype
        opengl32
        gdi32
        user32
        kernel32
        shell32
        winmm
        dwmapi
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_WINDOWS
        IMGUI_IMPL_OPENGL_LOADER_GLAD
        GLFW_INCLUDE_NONE
    )
else()
    find_package(OpenGL REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        freetype
        OpenGL::GL
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
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/void.zip $<TARGET_FILE_DIR:${PROJECT_NAME}>/void.zip
)
