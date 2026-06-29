#pragma once

#include <stb_image/stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <graphics/renderer/RenderAPI.h>

class IRenderDevice;

class Window {
private:
    int _width, _height;
    int _windowedX, _windowedY, _windowedWidth, _windowedHeight;
    bool _isFullscreen;
    bool _isVSync;
    bool _ownsWindow;
    GLFWwindow* window;
    const char* _title;

public:
    Window(int width, int height, const char* title, GLFWwindow* share = nullptr) 
        : _width(width), _height(height), _title(title), window(nullptr),
          _isFullscreen(false), _isVSync(false), _ownsWindow(true),
          _windowedX(0), _windowedY(0), _windowedWidth(width), _windowedHeight(height) {
        
        if (!glfwInit()) {
            _ownsWindow = false;
            return;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(width, height, title, NULL, share);
        if (!window) {
            glfwTerminate();
            _ownsWindow = false;
            return;
        }
        
        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            return;
        }

        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
            glViewport(0, 0, width, height);
            Window* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
            if (self) {
                self->_width = width;
                self->_height = height;
            }
        });

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        glfwSwapInterval(0);
    }
    
    ~Window() {
        if (window && _ownsWindow) {
            glfwDestroyWindow(window);
        }
        if (_ownsWindow) {
            glfwTerminate();
        }
    }
    
    // Wrap an existing GLFW window without creating a new one
    Window(GLFWwindow* existing, int width, int height)
        : _width(width), _height(height), _title(""), window(existing),
          _isFullscreen(false), _isVSync(false), _ownsWindow(false),
          _windowedX(0), _windowedY(0), _windowedWidth(width), _windowedHeight(height) {
    }

    Window(Window&& other) noexcept 
        : _width(other._width), _height(other._height), 
          _title(other._title), window(other.window),
          _ownsWindow(other._ownsWindow) {
        other.window = nullptr;
        other._title = nullptr;
        other._ownsWindow = false;
    }
    
    Window& operator=(Window&& other) noexcept {
        if (this != &other) {
            if (window && _ownsWindow) {
                glfwDestroyWindow(window);
            }
            _width = other._width;
            _height = other._height;
            _title = other._title;
            window = other.window;
            _ownsWindow = other._ownsWindow;
            other.window = nullptr;
            other._title = nullptr;
            other._ownsWindow = false;
        }
        return *this;
    }
    
    void setIcon(const char *iconPath) {
        int width, height, channels;
        unsigned char *data = stbi_load(iconPath, &width, &height, &channels, 4);

        if (data == nullptr) {
            fprintf(stderr, "Failed to load window icon: %s\n", iconPath);
            return;
        }

        GLFWimage icon;
        icon.width = width;
        icon.height = height;
        icon.pixels = data;

        glfwSetWindowIcon(window, 1, &icon);
        stbi_image_free(data);

        printf("Window icon loaded: %s\n", iconPath);
    }
    
    bool shouldClose() const {
        return glfwWindowShouldClose(window);
    }
    
    void pollEvents() {
        glfwPollEvents();
    }
    
    void swapBuffers() {
        glfwSwapBuffers(window);
    }
    
    void clear() {
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    void setClearColor(float r, float g, float b, float a) {
        glClearColor(r, g, b, a);
    }
    
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    GLFWwindow* getGLFWwindow() const { return window; }
    
    void setTitle(const char* title) {
        _title = title;
        glfwSetWindowTitle(window, title);
    }
    const char* getTitle() const { return _title; }
    
    void setSize(int width, int height) {
        _width = width;
        _height = height;
        glfwSetWindowSize(window, width, height);
    }
    
    void getPosition(int* outX, int* outY) const {
        glfwGetWindowPos(window, outX, outY);
    }

    GLFWwindow* GetWindow() {
        return window;
    }

    GLFWwindow* release() {
        GLFWwindow* w = window;
        window = nullptr;
        _ownsWindow = false;
        return w;
    }
    
    void setShouldClose(bool flag) {
        glfwSetWindowShouldClose(window, flag);
    }

    bool isFullscreen() const {
        return _isFullscreen;
    }

    void setFullscreen(bool fullscreen) {
        if (_isFullscreen == fullscreen) return;

        if (fullscreen) {
            glfwGetWindowPos(window, &_windowedX, &_windowedY);
            glfwGetWindowSize(window, &_windowedWidth, &_windowedHeight);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            if (!monitor) return;
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (!mode) return;

            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            _width = mode->width;
            _height = mode->height;
        } else {
            glfwSetWindowMonitor(window, nullptr, _windowedX, _windowedY, _windowedWidth, _windowedHeight, 0);
            _width = _windowedWidth;
            _height = _windowedHeight;
        }

        _isFullscreen = fullscreen;
    }

    void toggleFullscreen() {
        setFullscreen(!_isFullscreen);
    }

    bool isVSync() const {
        return _isVSync;
    }

    void setVSync(bool enabled) {
        _isVSync = enabled;
        glfwSwapInterval(enabled ? 1 : 0);
    }

    void toggleVSync() {
        setVSync(!_isVSync);
    }

    IRenderDevice* createRenderDevice(RenderAPI api, uint32_t width = 1280, uint32_t height = 720);
};