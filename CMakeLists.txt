cmake_minimum_required(VERSION 3.10)
project(BlazeBolt)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(MINGW OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -s")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/include/glad)
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/libs)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${IMGUI_DIR}/texteditor/TextEditor.cpp
    ${IMGUI_DIR}/crude_json.cpp
)

set(ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/hub.cpp
    ${CMAKE_SOURCE_DIR}/src/editor.cpp
    ${GLAD_DIR}/glad.c
    ${CMAKE_SOURCE_DIR}/core/graphics/gl.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/renderer/RenderBackend.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/window.cpp
    # RHI renderer interfaces
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLBuffer.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLTexture.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLSampler.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLShader.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLPipeline.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLSwapChain.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLRenderContext.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/gl/GLRenderDevice.cpp
    # Vulkan backend
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkTexture.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkSampler.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkShader.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkPipeline.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkSwapChain.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkRenderContext.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/vk/VkRenderDevice.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/quad.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/text2D.cpp
    ${CMAKE_SOURCE_DIR}/core/engine/luaEngine.cpp
    ${CMAKE_SOURCE_DIR}/core/engine/managers.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/animatedTexture2D.cpp
    ${CMAKE_SOURCE_DIR}/core/graphics/spriteBatch2D.cpp
    ${CMAKE_SOURCE_DIR}/core/physics/body.cpp
    ${CMAKE_SOURCE_DIR}/core/physics/collision.cpp
    ${CMAKE_SOURCE_DIR}/core/physics/world.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/sprite/animatedSprite2D.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/sprite/animationWheel.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/sprite/generalSprite.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/sprite/staticSprite2D.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/camera2D.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/particle2D.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/tileset2D.cpp
    ${CMAKE_SOURCE_DIR}/core/subject/light2D.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/net/client.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/net/server.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/dr_mp3.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/input.cpp
    ${CMAKE_SOURCE_DIR}/core/scene.cpp
    ${CMAKE_SOURCE_DIR}/src/stb_image_impl.cpp
)

add_subdirectory(${LIBS_DIR}/glfw glfw_build)
add_subdirectory(${LIBS_DIR}/freetype freetype_build)
add_subdirectory(${LIBS_DIR}/lua lua_build)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${LIBS_DIR}/openal openal_build)

if(TARGET OpenAL)
    target_compile_options(OpenAL PRIVATE -w)
endif()

find_package(Python3 QUIET COMPONENTS Interpreter)
find_package(Vulkan REQUIRED)

if(WIN32)
    configure_file("BlazeBolt.rc.in" "BlazeBolt.rc" @ONLY)
    set(BLAZEBOLT_RC "${CMAKE_CURRENT_BINARY_DIR}/BlazeBolt.rc")

    if(Python3_FOUND)
        add_custom_command(
            OUTPUT "${CMAKE_SOURCE_DIR}/assets/icon.ico"
            DEPENDS "${CMAKE_SOURCE_DIR}/assets/icon.png"
            COMMAND Python3::Interpreter -c
                "from PIL import Image; img=Image.open('${CMAKE_SOURCE_DIR}/assets/icon.png'); img.save('${CMAKE_SOURCE_DIR}/assets/icon.ico', format='ICO', sizes=[(16,16),(32,32),(48,48),(64,64),(256,256)])"
            COMMENT "Generating icon.ico from icon.png"
        )
        add_custom_target(BlazeBoltIcon DEPENDS "${CMAKE_SOURCE_DIR}/assets/icon.ico")
    else()
        add_custom_target(BlazeBoltIcon
            COMMAND ${CMAKE_COMMAND} -E echo "Warning: Python3+PIL not found, creating placeholder icon.ico"
            COMMAND ${CMAKE_COMMAND} -E env PSModulePath= powershell -NoProfile -Command "[System.IO.File]::WriteAllBytes('${CMAKE_SOURCE_DIR}/assets/icon.ico', [byte[]]@(0,0,1,0,1,0, 0,0,0,0,0,0,1,0,32,0,44,0,0,0,22,0,0,0, 40,0,0,0,1,0,0,0,2,0,0,0,1,0,32,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0))"
            BYPRODUCTS "${CMAKE_SOURCE_DIR}/assets/icon.ico"
        )
    endif()
else()
    set(BLAZEBOLT_RC "")
endif()

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES} ${IMGUI_SOURCES} ${BLAZEBOLT_RC})

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
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw freetype lua_static OpenAL
        opengl32 gdi32 user32 kernel32 shell32 winmm dwmapi ws2_32
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_WINDOWS
        IMGUI_IMPL_OPENGL_LOADER_GLAD
        GLFW_INCLUDE_NONE
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    add_dependencies(${PROJECT_NAME} BlazeBoltIcon)
else()
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw freetype lua_static OpenAL
        OpenGL::GL ${X11_LIBRARIES} dl pthread m
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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if(EXISTS "${CMAKE_SOURCE_DIR}/hub/void.zip")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/void.zip $<TARGET_FILE_DIR:${PROJECT_NAME}>/void.zip
    )
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(FILES "${CMAKE_SOURCE_DIR}/BlazeBolt.desktop"
        DESTINATION share/applications)
    install(FILES "${CMAKE_SOURCE_DIR}/assets/icon.png"
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME blazebolt.png)
endif()
