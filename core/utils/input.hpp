#pragma once
#include <GLFW/glfw3.h>
#include <optional>
#include <array>

class Input {
private:
    static constexpr int MAX_KEYS = 512;
    static constexpr int MAX_MOUSE_BUTTONS = 8;

    std::array<uint64_t, MAX_KEYS> keysFrames;
    std::array<uint64_t, MAX_MOUSE_BUTTONS> mouseButtonsFrames;
    float mouseX, mouseY, mouseDeltaX, mouseDeltaY, scrollX, scrollY;
    
    std::array<std::optional<std::array<uint64_t, 15>>, 16> gamepadsButtonsFrames;
    std::array<std::optional<std::array<float, 6>>, 16> gamepadsAxes;
    size_t currentFrame;
    GLFWwindow* window;

    Input();
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow *window, double xOffset, double yOffset);
    static void joystickCallback(int jid, int event);
    static void windowFocusCallback(GLFWwindow *window, int focused);
    void updateGamepad(int id);
public:
    ~Input() = default;
    static Input &getInstance();

    void init(GLFWwindow *window);
    void preUpdate();
    void postUpdate();

    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonJustPressed(int button) const;

    float getMouseX() const;
    float getMouseY() const;
    float getMouseDeltaX() const;
    float getMouseDeltaY() const;
    float getScrollX() const;
    float getScrollY() const;

    bool isGamepadConnected(int id) const;
    bool isGamepadButtonPressed(int id, int button) const;
    bool isGamepadButtonJustPressed(int id, int button) const;
    float getGamepadAxis(int id, int axis) const;
};