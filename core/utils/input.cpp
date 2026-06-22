#include "input.hpp"

Input::Input() :
    keysFrames(), mouseButtonsFrames(),
    mouseX(0.0), mouseY(0.0), mouseDeltaX(0.0), mouseDeltaY(0.0), scrollX(0.0), scrollY(0.0),
    gamepadsButtonsFrames(), gamepadsAxes(),
    currentFrame(1), window(nullptr)
{}
void Input::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key < 0 || key >= MAX_KEYS) return;
    Input &input = Input::getInstance();
    if (action == GLFW_PRESS) {
        input.keysFrames[key] = input.currentFrame;
    } else if (action == GLFW_RELEASE) {
        input.keysFrames[key] = 0;
    }
}

void Input::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) return;
    Input &input = Input::getInstance();
    if (action == GLFW_PRESS) {
        input.mouseButtonsFrames[button] = input.currentFrame;
    } else if (action == GLFW_RELEASE) {
        input.mouseButtonsFrames[button] = 0;
    }
}

void Input::cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    Input &input = Input::getInstance();
    input.mouseDeltaX = static_cast<float>(xpos) - input.mouseX;
    input.mouseDeltaY = static_cast<float>(ypos) - input.mouseY;
    input.mouseX = static_cast<float>(xpos);
    input.mouseY = static_cast<float>(ypos);
}

void Input::scrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
    Input &input = Input::getInstance();
    input.scrollX = static_cast<float>(xOffset);
    input.scrollY = static_cast<float>(yOffset);
}

void Input::joystickCallback(int jid, int event) {
    Input &input = Input::getInstance();
    switch (event) {
        case GLFW_CONNECTED:
            input.gamepadsButtonsFrames[jid] = std::array<uint64_t, 15>();
            input.gamepadsAxes[jid] = std::array<float, 6>();
            input.updateGamepad(jid);
            break;
        case GLFW_DISCONNECTED:
            input.gamepadsButtonsFrames[jid] = std::nullopt;
            input.gamepadsAxes[jid] = std::nullopt;
            break;
    }
}
void Input::updateGamepad(int id) {
    GLFWgamepadstate state;
    if (glfwGetGamepadState(id, &state) == GLFW_TRUE) {
        for (size_t i = 0; i < 15; i++) {
            if (!this->gamepadsButtonsFrames[id].has_value()) { continue; }
            if (state.buttons[i] == GLFW_PRESS && (*this->gamepadsButtonsFrames[id])[i] == 0) {
                (*this->gamepadsButtonsFrames[id])[i] = currentFrame;
            } else if (state.buttons[i] == GLFW_RELEASE) {
                (*this->gamepadsButtonsFrames[id])[i] = 0;
            }
        }
        for (size_t i = 0; i < 6; i++) {
            if (!this->gamepadsAxes[id].has_value()) { continue; }
            (*this->gamepadsAxes[id])[i] = state.axes[i];
        }
    }
}

Input &Input::getInstance() {
    static Input instance = Input();
    return instance;
}

void Input::windowFocusCallback(GLFWwindow *window, int focused) {
    Input &input = getInstance();
    if (focused && input.window) {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++) {
            if (glfwGetKey(input.window, key) == GLFW_PRESS) {
                if (input.keysFrames[key] == 0) {
                    input.keysFrames[key] = input.currentFrame;
                }
            } else {
                input.keysFrames[key] = 0;
            }
        }
        for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; button++) {
            if (glfwGetMouseButton(input.window, button) == GLFW_PRESS) {
                if (input.mouseButtonsFrames[button] == 0) {
                    input.mouseButtonsFrames[button] = input.currentFrame;
                }
            } else {
                input.mouseButtonsFrames[button] = 0;
            }
        }
    }
}

void Input::init(GLFWwindow *window) {
    this->window = window;
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetJoystickCallback(joystickCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);
}

void Input::preUpdate() {
    this->currentFrame++;
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++) {
        if (glfwJoystickPresent(i)) { this->updateGamepad(i); }
    }
}
void Input::postUpdate() {
    this->mouseDeltaX = 0.0f;
    this->mouseDeltaY = 0.0f;
    this->scrollX = 0.0f;
    this->scrollY = 0.0f;
}

bool Input::isKeyPressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return this->keysFrames[key] != 0;
}
bool Input::isKeyJustPressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return this->keysFrames[key] == this->currentFrame - 1;
}
bool Input::isMouseButtonPressed(int button) const {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
    return this->mouseButtonsFrames[button] != 0;
}
bool Input::isMouseButtonJustPressed(int button) const {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
    return this->mouseButtonsFrames[button] == this->currentFrame - 1;
}

float Input::getMouseX() const { return this->mouseX; }
float Input::getMouseY() const { return this->mouseY; }
float Input::getMouseDeltaX() const { return this->mouseDeltaX; }
float Input::getMouseDeltaY() const { return this->mouseDeltaY; }
float Input::getScrollX() const { return this->scrollX; }
float Input::getScrollY() const { return this->scrollY; }

bool Input::isGamepadConnected(int id) const {
    return id >= 0 && id < 16 && this->gamepadsButtonsFrames[id].has_value() && this->gamepadsAxes[id].has_value();
}
bool Input::isGamepadButtonPressed(int id, int button) const {
    if (id < 0 || id >= 16 || button < 0 || button >= 16) return false;
    return this->gamepadsButtonsFrames[id].has_value() && (*this->gamepadsButtonsFrames[id])[button] != 0;
}
bool Input::isGamepadButtonJustPressed(int id, int button) const {
    if (id < 0 || id >= 16 || button < 0 || button >= 16) return false;
    return this->gamepadsButtonsFrames[id].has_value() && (*this->gamepadsButtonsFrames[id])[button] == this->currentFrame - 1;
}
float Input::getGamepadAxis(int id, int axis) const {
    if (id < 0 || id >= 16 || axis < 0 || axis >= 8) return 0.0f;
    return this->gamepadsAxes[id].has_value() ? (*this->gamepadsAxes[id])[axis] : 0.0f;
}