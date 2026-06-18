cmake_minimum_required(VERSION 3.10)
project(BlazeBolt)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -s")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()

add_subdirectory(libs/glfw)
add_subdirectory(libs/glad)
add_subdirectory(libs/freetype)
add_subdirectory(libs/lua)

set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE BOOL "" FORCE)
set(ALSOFT_DLOPEN OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_DSOUND OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/openal)

if(TARGET OpenAL)
    target_compile_options(OpenAL PRIVATE -w)
endif()

set(BLAZEBOLT_SOURCES
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
    core/subject/sprite/animationWheel.cpp
    core/subject/sprite/generalSprite.cpp
    core/subject/sprite/staticSprite2D.cpp
    core/subject/camera2D.cpp
    core/subject/particle2D.cpp
    core/subject/tileset2D.cpp
    core/subject/light2D.cpp
    core/subject/text2D.cpp
    core/utils/net/client.cpp
    core/utils/net/server.cpp
    core/utils/dr_mp3.cpp
    core/utils/input.cpp
    core/scene.cpp
)

set(BLAZEBOLT_INCLUDES
    core
    libs/headeronly/include
    libs/glfw/include
    libs/glad/include
    libs/freetype/include
    libs/openal/include
)

set(BLAZEBOLT_LIBS
    glfw
    glad
    freetype
    lua_static
    OpenAL
)

add_library(BlazeBoltLib STATIC ${BLAZEBOLT_SOURCES})
target_include_directories(BlazeBoltLib PRIVATE ${BLAZEBOLT_INCLUDES})
target_link_libraries(BlazeBoltLib PRIVATE ${BLAZEBOLT_LIBS})
if(WIN32)
    target_link_libraries(BlazeBoltLib PRIVATE ws2_32)
endif()

add_executable(BlazeBolt ${BLAZEBOLT_SOURCES})
target_include_directories(BlazeBolt PRIVATE ${BLAZEBOLT_INCLUDES})
target_link_libraries(BlazeBolt PRIVATE ${BLAZEBOLT_LIBS})
if(WIN32)
    target_link_libraries(BlazeBolt PRIVATE ws2_32)
endif()
    if(MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        target_link_options(BlazeBolt PRIVATE -Wl,--gc-sections)
    endif()
add_custom_command(TARGET BlazeBolt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:BlazeBolt>
)

if(WIN32)
    add_executable(BlazeBoltWin ${BLAZEBOLT_SOURCES})
    set_target_properties(BlazeBoltWin PROPERTIES WIN32_EXECUTABLE TRUE)
    target_include_directories(BlazeBoltWin PRIVATE ${BLAZEBOLT_INCLUDES})
    target_link_libraries(BlazeBoltWin PRIVATE ${BLAZEBOLT_LIBS})
    target_link_libraries(BlazeBoltWin PRIVATE ws2_32)
        if(MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
            target_link_options(BlazeBoltWin PRIVATE -Wl,--gc-sections)
        endif()
    add_custom_command(TARGET BlazeBoltWin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:BlazeBoltWin>
    )
endif()