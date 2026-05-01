#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <iostream>
#include <vector>
#include <string>
#include <memory>

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
#include <thread>

class GameEngine {
private:
    std::unique_ptr<Window> mainWindow;
    std::unique_ptr<LuaEngine::LuaEngine> luaEngine;
    bool isRunning;
    float deltaTime;
    std::chrono::steady_clock::time_point lastTime;
    int windowWidth;
    int windowHeight;
    
public:
    GameEngine() : isRunning(true), deltaTime(0.0f), windowWidth(1200), windowHeight(600) {}
    
    ~GameEngine() {
        shutdown();
    }
    
    bool init(int width, int height, const char* title) {
        windowWidth = width;
        windowHeight = height;
        
        mainWindow = std::make_unique<Window>(width, height, title);
        
        if (!mainWindow->getGLFWwindow()) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }
        
        Input::getInstance().init(mainWindow->getGLFWwindow());
        
        luaEngine = std::make_unique<LuaEngine::LuaEngine>();
        luaEngine->Init();
        
        // Устанавливаем указатель на главное окно в LuaEngine
        luaEngine->setMainWindow(mainWindow.get());
        
        luaEngine->setTextScreenSize(width, height);
        
        if (!luaEngine->loadScriptsFromList("engine/scripts.list")) {
            std::cerr << "Failed to load scripts" << std::endl;
            return false;
        }
        
        // Вызываем Start с передачей размеров экрана
        luaEngine->callFunction("Start");
        
        lastTime = std::chrono::steady_clock::now();
        
        std::cout << "Game Engine initialized successfully" << std::endl;
        std::cout << "Screen size: " << width << "x" << height << std::endl;
        return true;
    }
    
    void run() {
        while (isRunning && !mainWindow->shouldClose()) {
            auto currentTime = std::chrono::steady_clock::now();
            deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            if (deltaTime > 0.033f) {
                deltaTime = 0.033f;
            }
            
            // Проверяем, не изменился ли размер окна
            int newWidth = mainWindow->getWidth();
            int newHeight = mainWindow->getHeight();
            if (newWidth != windowWidth || newHeight != windowHeight) {
                windowWidth = newWidth;
                windowHeight = newHeight;
                luaEngine->setTextScreenSize(windowWidth, windowHeight);
                luaEngine->setSpriteScreenSize(windowWidth, windowHeight);
            }
            
            update();
            render();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    void update() {
        mainWindow->pollEvents();
        Input::getInstance().update();
        
        luaEngine->callUpdate(deltaTime);
        luaEngine->physicsStep();
        luaEngine->updateAll(deltaTime);
        
        if (Input::getInstance().isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            shutdown();
        }
    }
    
    void render() {
        mainWindow->setClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        mainWindow->clear();
        
        luaEngine->callDraw();
        luaEngine->drawAll();
        
        mainWindow->swapBuffers();
    }
    
    void shutdown() {
        isRunning = false;
        
        if (luaEngine) {
            luaEngine->callEnd();
            luaEngine->shutdown();
            luaEngine.reset();
        }
        
        std::cout << "Game Engine shut down" << std::endl;
    }
    
    int getScreenWidth() const { return windowWidth; }
    int getScreenHeight() const { return windowHeight; }
};

int main(int argc, char* argv[]) {
    
    GameEngine game;
    
    if (!game.init(1200, 600, "BlazeBolt Game Engine")) {
        std::cerr << "Failed to initialize game engine" << std::endl;
        return -1;
    }
    
    game.run();
    
    return 0;
}