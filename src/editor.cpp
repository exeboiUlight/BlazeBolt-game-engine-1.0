// editor.cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <iostream>
#include <string>

#include <graphics/window.h>
#include <graphics/shader.h>
#include <graphics/mesh.h>
#include <subject/sprite2D.h>
#include <subject/text.h>
#include <subject/audio.h>
#include <subject/animatad2D.h>
#include <utils/input/input.h>
#include <utils/math/vector.h>

#include <engine/gui.h>
#include <engine/lua.h>

int main() {
    Window window(1400, 800, "BlazeBolt Editor");
    window.setClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    window.setIcon("./icon.png");
    
    Audio audio;
    audio.init();
    
    Input::getInstance().init(window.getGLFWwindow());
    
    EditorGUI::EditorGUI editor(window);
    editor.init();
    
    std::cout << "Editor initialized" << std::endl;
    editor.addConsoleLine("> Editor started");
    editor.addConsoleLine("> BlazeBolt Game Engine Editor v1.0");
    
    auto lastTime = std::chrono::steady_clock::now();
    float deltaTime = 0;
    
    // Демонстрационные спрайты для сцены
    Sprite2D testSprite;
    testSprite.setTexture("./icon.png");
    testSprite.setPosition(0.0f, 0.0f);
    testSprite.setSize(0.5f, 0.5f);
    
    Sprite2D backgroundSprite;
    backgroundSprite.setTexture("engine/assets/textures/wall.jpg");
    backgroundSprite.setPosition(0.0f, 0.0f);
    backgroundSprite.setSize(2.0f, 2.0f);
    backgroundSprite.setColor(0.3f, 0.3f, 0.4f, 0.5f);
    
    Text fpsText("./engine/assets/arial.ttf", 16);
    fpsText.setColor(0.8f, 0.8f, 0.9f, 1.0f);
    fpsText.setPosition(10.0f, 25.0f);
    fpsText.setScreenSize(window.getWidth(), window.getHeight());
    
    float fpsTimer = 0;
    int frameCount = 0;
    float currentFPS = 60;
    
    while (!window.shouldClose()) {
        auto now = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (deltaTime > 0.033f) deltaTime = 0.033f;
        
        // FPS счетчик
        fpsTimer += deltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            currentFPS = frameCount / fpsTimer;
            frameCount = 0;
            fpsTimer = 0;
        }
        
        window.pollEvents();
        Input::getInstance().update();
        
        float mx = Input::getInstance().getMouseX();
        float my = Input::getInstance().getMouseY();
        
        editor.handleMouseMove(mx, my);
        
        if (Input::getInstance().isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            editor.handleMouseDown(mx, my, GLFW_MOUSE_BUTTON_LEFT);
        }
        if (Input::getInstance().isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            editor.handleMouseUp(mx, my, GLFW_MOUSE_BUTTON_LEFT);
        }
        
        static int lastKey = -1;
        for (int key = GLFW_KEY_A; key <= GLFW_KEY_Z; key++) {
            if (Input::getInstance().isKeyJustPressed(key)) {
                editor.handleKeyPress(key, 0);
            }
        }
        if (Input::getInstance().isKeyJustPressed(GLFW_KEY_BACKSPACE)) {
            editor.handleKeyPress(GLFW_KEY_BACKSPACE, 0);
        }
        if (Input::getInstance().isKeyJustPressed(GLFW_KEY_SPACE)) {
            editor.handleKeyPress(GLFW_KEY_SPACE, 0);
        }
        if (Input::getInstance().isKeyJustPressed(GLFW_KEY_ENTER)) {
            editor.handleKeyPress(GLFW_KEY_ENTER, 0);
        }
        
        editor.update(deltaTime);
        
        window.clear();
        
        // Рендер сцены
        backgroundSprite.draw();
        testSprite.draw();
        
        // Рендер FPS
        fpsText.setText("FPS: " + std::to_string((int)currentFPS) + " | Delta: " + std::to_string(deltaTime).substr(0, 5));
        fpsText.draw();
        
        // Рендер GUI редактора поверх сцены
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        editor.draw();
        glDisable(GL_BLEND);
        
        window.swapBuffers();
    }
    
    audio.shutdown();
    
    return 0;
}