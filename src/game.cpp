#define STB_IMAGE_IMPLEMENTATION
#include <cstdio>
#include <chrono>
#include <graphics/window.h>
#include <utils/input/input.h>
#include <engine/lua.h>

int main() {
    Window window = Window(1200, 600, "Untitled | BlazeBolt");
    if (window.getGLFWwindow() == nullptr) {
        fprintf(stderr, "Failed to create main window\n");
        return 1;
    }
    window.setClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    Input::getInstance().init(window.getGLFWwindow());
    LuaEngine::LuaEngine luaEngine = LuaEngine::LuaEngine();
    luaEngine.Init(); // TODO: Remove this method, it doesn't make any sense
    luaEngine.setMainWindow(&window);
    luaEngine.setTextScreenSize(window.getWidth(), window.getHeight());

    if (!luaEngine.loadScriptsFromList("engine/.BlazeBoltProject")) {
        fprintf(stderr, "Failed to load scripts\n");
        return 1;
    }

    // Вызываем Start с передачей размеров экрана
    luaEngine.callFunction("Start");
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

    printf("Game Engine initialized successfully\n");
    printf("Screen size: %dx%d\n", window.getWidth(), window.getHeight());

    int previousWindowWidth = window.getWidth();
    int previousWindowHeight = window.getHeight();
    while (!window.shouldClose()) {
        window.pollEvents();

        // Проверяем, не изменился ли размер окна
        int newWindowWidth = window.getWidth();
        int newWindowHeight = window.getHeight();
        if (newWindowWidth != previousWindowWidth || newWindowHeight != previousWindowHeight) {
            luaEngine.setTextScreenSize(newWindowWidth, newWindowHeight);
            luaEngine.setSpriteScreenSize(newWindowWidth, newWindowHeight);
            previousWindowWidth = newWindowWidth;
            previousWindowHeight = newWindowHeight;
        }

        // Delta Time update
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Update
        Input::getInstance().update();
        luaEngine.callUpdate(deltaTime);
        luaEngine.physicsStep();
        luaEngine.updateAll(deltaTime);

        // Render
        window.clear();
        luaEngine.callDraw();
        luaEngine.drawAll();
        window.swapBuffers();
    }

    luaEngine.callEnd();
    printf("Game Engine shut down\n");

    return 0;
}