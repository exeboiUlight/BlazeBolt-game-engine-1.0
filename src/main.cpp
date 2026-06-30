#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#include "hub.hpp"
#include "editor.hpp"

#include <graphics/renderer/RenderBackend.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <fstream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace fs = std::filesystem;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static fs::path find_engine_root() {
    // Use exe path, NOT current working directory (which can be anything)
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    fs::path exe_dir;
    if (len > 0 && len < MAX_PATH) {
        exe_dir = fs::path(buf).parent_path();
    } else {
        exe_dir = fs::current_path();
    }
    for (int i = 0; i < 5; i++) {
        if (fs::exists(exe_dir / "void.zip")) return exe_dir;
        if (exe_dir.has_parent_path()) exe_dir = exe_dir.parent_path();
        else break;
    }
    return exe_dir;
}

static RenderAPI load_render_api(const fs::path& engine_root) {
    std::ifstream f(engine_root / "render_api.txt");
    if (f.is_open()) {
        std::string line;
        std::getline(f, line);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (line == "vulkan") return RenderAPI::Vulkan;
    }
    return RenderAPI::OpenGL;
}

int main(int argc, char** argv) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    fs::path engine_root = find_engine_root();
    RenderAPI startAPI = load_render_api(engine_root);

    // Create initial window with the correct client API hint.
    // When switching at runtime, RenderBackend recreates the window with the
    // opposite hint so that the driver never sees OpenGL+Vulkan on the same HWND.
    if (startAPI == RenderAPI::Vulkan) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    }
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "BlazeBolt", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }

    // Set window icon (RenderBackend::recreateWindow also sets it on switch)
    {
        int iw, ih, ch;
        unsigned char* data = stbi_load("icon.png", &iw, &ih, &ch, 4);
        if (!data) data = stbi_load("assets/icon.png", &iw, &ih, &ch, 4);
        if (data) {
            GLFWimage img;
            img.width = iw;
            img.height = ih;
            img.pixels = data;
            glfwSetWindowIcon(window, 1, &img);
            stbi_image_free(data);
        }
    }

    // Init ImGui
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

    // Init fonts
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    ImFontConfig font_cfg;
    font_cfg.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
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

    // Init render backend (may recreate window if Vulkan fails → falls back to OpenGL)
    if (!RenderBackend::Init(startAPI, window)) {
        fprintf(stderr, "Failed to initialize render backend\n");
        ImGui::DestroyContext();
        window = RenderBackend::GetWindow();  // may have been recreated before failure
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    window = RenderBackend::GetWindow();  // pick up any window recreated during fallback

    Hub hub(engine_root);
    Editor editor(engine_root);

    enum AppState { STATE_HUB, STATE_EDITOR };
    AppState state = STATE_HUB;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!RenderBackend::BeginFrame()) {
            window = RenderBackend::GetWindow();
            continue;
        }

        RenderBackend::NewImGuiFrame();

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

        RenderBackend::EndFrame();
        window = RenderBackend::GetWindow();
    }

    RenderBackend::Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
