#define STB_IMAGE_IMPLEMENTATION
#include <cstdio>
#include <chrono>
#include <graphics/window.h>
#include <utils/input.hpp>
#include <engine/lua.h>

int main() {
    Window window = Window(1200, 600, "Untitled project | BlazeBolt");
    if (window.getGLFWwindow() == nullptr) {
        fprintf(stderr, "Failed to create main window\n");
        return 1;
    }
    window.setClearColor(0.5803921f, 0.5803921f, 0.5803921f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Input::getInstance().init(window.getGLFWwindow());
    LuaEngine::LuaEngine luaEngine = LuaEngine::LuaEngine(window);
    if (!luaEngine.isInitialized()) {
        fprintf(stderr, "Failed to initialize Lua engine\n");
        return 1;
    }
    if (!luaEngine.loadScriptsFromList("engine/.BlazeBoltProject")) {
        fprintf(stderr, "Failed to load scripts\n");
        return 1;
    }

    luaEngine.callFunction("Start");
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
    float fpsTimer = 0.0f;
    uint32_t frameCount = 0;

    printf("Game Engine initialized successfully\n");
    printf("Screen size: %dx%d\n", window.getWidth(), window.getHeight());

    int previousWindowWidth = window.getWidth();
    int previousWindowHeight = window.getHeight();
    while (!window.shouldClose()) {
        window.pollEvents();
        Input::getInstance().preUpdate(); // IMPORTANT! Must be called after pollEvents() and before anything else

        int newWindowWidth = window.getWidth();
        int newWindowHeight = window.getHeight();
        if (newWindowWidth != previousWindowWidth || newWindowHeight != previousWindowHeight) {
            luaEngine.setSpriteScreenSize(newWindowWidth, newWindowHeight);
            previousWindowWidth = newWindowWidth;
            previousWindowHeight = newWindowHeight;
        }

        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        fpsTimer += deltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            printf("FPS: %u\n", frameCount);
            fpsTimer -= 1.0f;
            frameCount = 0;
        }

        // Update
        luaEngine.callUpdate(deltaTime);
        luaEngine.physicsStep();
        luaEngine.updateAll(deltaTime);

        window.clear();
        luaEngine.callDraw();
        luaEngine.drawAll();
        window.swapBuffers();

        Input::getInstance().postUpdate(); // IMPORTANT! Must be called at the end of the frame after everything else
    }

    luaEngine.callEnd();
    printf("Game Engine shut down\n");

    return 0;
}