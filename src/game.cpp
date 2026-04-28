#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <iostream>
#include <vector>
#include <string>

#include <graphics/window.h>
#include <graphics/mesh.h>
#include <graphics/shader.h>

#include <subject/sprite2D.h>
#include <subject/text.h>
#include <subject/audio.h>
#include <subject/animatad2D.h>

#include <utils/input/input.h>

#include <world.h>
#include <engine/lua.h>

#include <chrono>
#include <iostream>

int main() {
    Window window(1200, 600, "BlazeBolt Game Engine");
    
    if (!window.getGLFWwindow()) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    
    Input::getInstance().init(window.getGLFWwindow());
    
    LuaEngine::LuaEngine engine;
    engine.Init();
    
    engine.setTextScreenSize(window.getWidth(), window.getHeight());
    
    if (!engine.loadScriptsFromList("engine/scripts.list")) {
        std::cerr << "Failed to load scripts" << std::endl;
        return -1;
    }
    
    engine.callFunction("Start");
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    float deltaTime = 0.0f;
    
    std::cout << "Game loop started" << std::endl;
    
    while (!window.shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        if (deltaTime > 0.033f) {
            deltaTime = 0.033f;
        }
        
        window.pollEvents();
        Input::getInstance().update();
        
        engine.callUpdate(deltaTime);
        engine.updateAll(deltaTime);
        
        window.clear();
        window.setClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        
        engine.callDraw();
        engine.drawAll();
        
        window.swapBuffers();
    }
    
    std::cout << "Game loop ended" << std::endl;
    
    return 0;
}