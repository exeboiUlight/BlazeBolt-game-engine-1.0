#include <cstdio>
#include <cstring>
#include <chrono>
#include <graphics/window.h>
#include <graphics/gl.hpp>
#include <utils/input.hpp>
#include <engine/lua.hpp>

#include <debugTimer.h>

const unsigned char logo_image_data[] = {
    #embed "logo.png"
}

static void showSplashScreen(Window &window) {
    int logoW = 0, logoH = 0, logoCh = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *logoPixels = stbi_load_from_memory(
        logo_image_data, static_cast<int>(sizeof(logo_image_data)),
        &logoW, &logoH, &logoCh, 4
    );
    if (logoPixels == nullptr || logoW <= 0 || logoH <= 0) {
        fprintf(stderr, "Failed to decode embedded logo for splash screen\n");
        return;
    }

    GLuint logoTex = 0;
    glGenTextures(1, &logoTex);
    glBindTexture(GL_TEXTURE_2D, logoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, logoW, logoH, 0, GL_RGBA, GL_UNSIGNED_BYTE, logoPixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(logoPixels);

    const char *vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        out vec2 vUV;
        uniform vec2 uOffset;
        uniform vec2 uScale;
        void main() {
            vUV = aPos;
            gl_Position = vec4(aPos * uScale + uOffset, 0.0, 1.0);
        }
    )";
    const char *fragSrc = R"(
        #version 330 core
        in vec2 vUV;
        out vec4 fragColor;
        uniform sampler2D uTex;
        void main() {
            fragColor = texture(uTex, vUV);
        }
    )";

    auto compile = [](GLenum type, const char *src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            fprintf(stderr, "Splash shader compile error: %s\n", log);
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    float quad[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    float logoAspect = static_cast<float>(logoW) / static_cast<float>(logoH);
    float scaleY = 0.35f;
    float scaleX = scaleY * logoAspect;
    if (scaleX > 0.5f) { scaleX = 0.5f; scaleY = scaleX / logoAspect; }
    float offX = -scaleX / 2.0f;
    float offY = -scaleY / 2.0f;

    glUseProgram(prog);
    glUniform2f(glGetUniformLocation(prog, "uOffset"), offX, offY);
    glUniform2f(glGetUniformLocation(prog, "uScale"), scaleX, scaleY);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    glUseProgram(0);

    auto t0 = std::chrono::steady_clock::now();
    while (!window.shouldClose()) {
        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
        if (elapsed >= 2.0f) break;
        window.pollEvents();
        glClearColor(0.5803921f, 0.5803921f, 0.5803921f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, logoTex);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        window.swapBuffers();
    }

    glDeleteTextures(1, &logoTex);
    glDeleteProgram(prog);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    GL::unbindVertexArray();
    GL::unbindVertexBuffer(GL_ARRAY_BUFFER);
    GL::unbindShaderProgram();
    GL::bindTexture2D(0);

    printf("Splash screen finished\n");
}

int main() {
    Window window = Window(1200, 600, "Untitled project | BlazeBolt");
    if (window.getGLFWwindow() == nullptr) {
        fprintf(stderr, "Failed to create main window\n");
        return 1;
    }
    window.setClearColor(0.5803921f, 0.5803921f, 0.5803921f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    showSplashScreen(window);

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

    while (!window.shouldClose()) {
        window.pollEvents();
        Input::getInstance().preUpdate();

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

        Input::getInstance().postUpdate();
    }

    luaEngine.callEnd();
    printf("Game Engine shut down\n");

    return 0;
}
