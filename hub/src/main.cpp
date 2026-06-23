#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "hub.hpp"
#include "editor.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static fs::path find_engine_root() {
    fs::path exe_dir = fs::current_path();
    for (int i = 0; i < 5; i++) {
        if (fs::exists(exe_dir / "void.zip")) return exe_dir;
        if (exe_dir.has_parent_path()) exe_dir = exe_dir.parent_path();
        else break;
    }
    return fs::current_path();
}

int main(int argc, char** argv) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 800, "BlazeBolt", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwTerminate();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());

    ImFontConfig font_cfg;
    font_cfg.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;

    fs::path engine_root = find_engine_root();
    fs::path font_path = engine_root / "arial.ttf";
    if (!fs::exists(font_path)) {
        font_path.clear();
        for (auto& p : { "C:/Windows/Fonts/arial.ttf", "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf", "/usr/share/fonts/TTF/arial.ttf" }) {
            if (fs::exists(p)) { font_path = p; break; }
        }
    }
    if (!font_path.empty()) {
        io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), 16.0f, &font_cfg);
    } else {
        io.Fonts->AddFontDefault();
    }

    Hub hub(engine_root);
    Editor editor(engine_root);

    enum AppState { STATE_HUB, STATE_EDITOR };
    AppState state = STATE_HUB;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (state == STATE_HUB) {
            hub.Render();
            if (hub.ShouldOpenEditor()) {
                editor.OpenProject(hub.GetSelectedProjectPath());
                state = STATE_EDITOR;
                hub.ResetOpenFlag();
            }
        } else if (state == STATE_EDITOR) {
            editor.Render();
            if (editor.ShouldReturnToHub()) {
                state = STATE_HUB;
                editor.ResetReturnFlag();
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current);
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
