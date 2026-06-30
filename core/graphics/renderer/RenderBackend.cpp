#include "RenderBackend.h"
#include <graphics/renderer/RenderAPI.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#include <glad/glad.h>
#include <vulkan/vulkan.h>

#include <graphics/vk/VkRenderDevice.h>
#include <graphics/vk/VkSwapChain.h>
#include <graphics/vk/VkRenderContext.h>
#include <graphics/renderer/RenderContext.h>
#include <graphics/renderer/SwapChain.h>

#include <cstdio>
#include <cstdlib>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stb_image/stb_image.h>

namespace RenderBackend {

static RenderAPI s_currentAPI = RenderAPI::OpenGL;
static RenderAPI s_pendingAPI = RenderAPI::OpenGL;
static bool s_initialized = false;
static GLFWwindow* s_window = nullptr;

// Vulkan state
static VkRenderDevice* s_vkDevice = nullptr;
static VkSwapChain* s_vkSwapChain = nullptr;
static VkRenderContext* s_vkContext = nullptr;
static VkDescriptorPool s_imguiDescriptorPool = VK_NULL_HANDLE;
static std::string s_vulkanError;

// ----------------------------------------------------------------
// Forward declarations of internal helpers
// ----------------------------------------------------------------
static bool initOpenGL();
static void shutdownOpenGL();
static bool initVulkan();
static void shutdownVulkan();
static GLFWwindow* recreateWindow(RenderAPI targetAPI, GLFWwindow* oldWindow,
                                   int x, int y, int w, int h, int maximized);

// ----------------------------------------------------------------
// Public API
// ----------------------------------------------------------------

bool Init(RenderAPI api, GLFWwindow* window) {
    s_window = window;

    // Always init GLFW with the right hints before window creation.
    // The window is already created externally, but we need the right
    // client API hint. Since GLFW hints can't be changed after
    // window creation, we handle this at creation time in main.cpp.

    s_currentAPI = api;
    s_pendingAPI = api;

    switch (api) {
        case RenderAPI::Vulkan:
            if (!initVulkan()) {
                std::string msg = "Vulkan initialization failed.\nFalling back to OpenGL.\n\nError:\n";
                msg += s_vulkanError.empty() ? "(unknown)" : s_vulkanError;
                fprintf(stderr, "RenderBackend: Vulkan init failed: %s\n", s_vulkanError.c_str());
                MessageBoxA(nullptr, msg.c_str(), "BlazeBolt - Vulkan Error", MB_OK | MB_ICONERROR);
                s_vulkanError.clear();
                s_currentAPI = RenderAPI::OpenGL;
                s_pendingAPI = RenderAPI::OpenGL;
                // Recreate window with GL context if it was GLFW_NO_API for Vulkan
                s_window = recreateWindow(RenderAPI::OpenGL, s_window, 100, 100, 1280, 800, GLFW_FALSE);
                if (!s_window) return false;
                if (!initOpenGL()) return false;
            }
            break;
        case RenderAPI::OpenGL:
        default:
            if (!initOpenGL()) return false;
            break;
    }

    s_initialized = true;
    return true;
}

void Shutdown() {
    switch (s_currentAPI) {
        case RenderAPI::Vulkan: shutdownVulkan(); break;
        case RenderAPI::OpenGL: shutdownOpenGL(); break;
    }
    s_initialized = false;
    s_window = nullptr;
}

bool BeginFrame() {
    // Make GL context current — Hub/Editor use GL calls in their Render() methods,
    // and Shutdown() may call OpenGL cleanup (ImGui_ImplOpenGL3_Shutdown etc.)
    glfwMakeContextCurrent(s_window);

    // Check for pending API switch — recreate window with appropriate client API hint
    if (s_pendingAPI != s_currentAPI) {
        int oldX = 100, oldY = 100, oldW = 1280, oldH = 800, oldMax = GLFW_FALSE;
        GLFWwindow* oldWin = s_window;
        if (oldWin) {
            glfwGetWindowPos(oldWin, &oldX, &oldY);
            glfwGetWindowSize(oldWin, &oldW, &oldH);
            oldMax = glfwGetWindowAttrib(oldWin, GLFW_MAXIMIZED);
        }
        Shutdown();  // sets s_window = nullptr
        s_window = recreateWindow(s_pendingAPI, oldWin, oldX, oldY, oldW, oldH, oldMax);
        if (!s_window) {
            fprintf(stderr, "RenderBackend: failed to recreate window for API switch\n");
            return false;
        }
        if (!Init(s_pendingAPI, s_window)) {
            fprintf(stderr, "RenderBackend: failed to init new API after switch\n");
            return false;
        }
    }

    switch (s_currentAPI) {
        case RenderAPI::Vulkan: {
            if (!s_vkContext->beginFrame()) {
                // Swapchain out of date
                int fbW, fbH;
                glfwGetFramebufferSize(s_window, &fbW, &fbH);
                if (fbW > 0 && fbH > 0) {
                    s_vkSwapChain->resize(fbW, fbH);
                }
                return false;
            }
            s_vkContext->beginRenderPass();
            return true;
        }
        case RenderAPI::OpenGL: {
            int display_w, display_h;
            glfwGetFramebufferSize(s_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            return true;
        }
    }
    return true;
}

void EndFrame() {
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    switch (s_currentAPI) {
        case RenderAPI::Vulkan: {
            // Render main ImGui inside active render pass
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                s_vkContext->getCurrentCommandBuffer());
            // End main window render pass
            s_vkContext->endRenderPass();
            // Render child viewport windows (their own command buffers)
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
            // Submit and present (catch exceptions to avoid hard crash)
            try {
                s_vkContext->endFrame();
            } catch (const std::exception& e) {
                fprintf(stderr, "RenderBackend: Vulkan endFrame exception: %s\n", e.what());
            }
            break;
        }
        case RenderAPI::OpenGL: {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                if (backup) glfwMakeContextCurrent(backup);
            }
            glfwSwapBuffers(s_window);
            break;
        }
    }
}

void NewImGuiFrame() {
    switch (s_currentAPI) {
        case RenderAPI::Vulkan:
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            break;
        case RenderAPI::OpenGL:
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            break;
    }
    ImGui::NewFrame();
}

void RenderImGui() {
    // Rendering happens inside EndFrame()
}

void ResizeSwapchain(int width, int height) {
    if (s_currentAPI == RenderAPI::Vulkan && s_vkSwapChain) {
        s_vkSwapChain->resize(width, height);
    }
}

RenderAPI GetCurrentAPI() { return s_currentAPI; }

void RequestSwitch(RenderAPI api) {
    s_pendingAPI = api;
}

RenderAPI CheckPendingSwitch() {
    return s_pendingAPI;
}

bool IsInitialized() { return s_initialized; }

GLFWwindow* GetWindow() { return s_window; }

// ----------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------

static GLFWwindow* recreateWindow(RenderAPI targetAPI, GLFWwindow* oldWindow,
                                   int x, int y, int w, int h, int maximized) {
    if (oldWindow) {
        glfwDestroyWindow(oldWindow);
    }

    glfwDefaultWindowHints();
    if (targetAPI == RenderAPI::Vulkan) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    }
    glfwWindowHint(GLFW_MAXIMIZED, maximized);

    GLFWwindow* newWindow = glfwCreateWindow(w, h, "BlazeBolt", nullptr, nullptr);
    if (!newWindow) return nullptr;

    glfwSetWindowPos(newWindow, x, y);

    // Reload window icon
    {
        int iw, ih, ch;
        unsigned char* data = stbi_load("icon.png", &iw, &ih, &ch, 4);
        if (!data) data = stbi_load("assets/icon.png", &iw, &ih, &ch, 4);
        if (data) {
            GLFWimage img;
            img.width = iw;
            img.height = ih;
            img.pixels = data;
            glfwSetWindowIcon(newWindow, 1, &img);
            stbi_image_free(data);
        }
    }

    return newWindow;
}

// ----------------------------------------------------------------
// OpenGL backend
// ----------------------------------------------------------------

static bool initOpenGL() {
    glfwMakeContextCurrent(s_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "RenderBackend: failed to init GLAD\n");
        return false;
    }
    glfwSwapInterval(1);

    ImGui_ImplGlfw_InitForOpenGL(s_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    return true;
}

static void shutdownOpenGL() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

// ----------------------------------------------------------------
// Vulkan backend
// ----------------------------------------------------------------

static bool initVulkan() {
    s_vkDevice = nullptr;
    s_vkSwapChain = nullptr;
    s_vkContext = nullptr;
    s_imguiDescriptorPool = VK_NULL_HANDLE;

    int fbW, fbH;
    glfwGetFramebufferSize(s_window, &fbW, &fbH);
    uint32_t w = fbW > 0 ? static_cast<uint32_t>(fbW) : 1280;
    uint32_t h = fbH > 0 ? static_cast<uint32_t>(fbH) : 800;

    try {
        s_vkDevice = new VkRenderDevice(w, h, s_window);
    } catch (const std::exception& e) {
        s_vulkanError = e.what();
        fprintf(stderr, "RenderBackend: VkRenderDevice creation failed: %s\n", e.what());
        return false;
    }

    s_vkSwapChain = static_cast<VkSwapChain*>(s_vkDevice->getSwapChain());
    s_vkContext = static_cast<VkRenderContext*>(s_vkDevice->getContext());

    ImGui_ImplGlfw_InitForVulkan(s_window, true);

    // Create ImGui descriptor pool
    {
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
        };
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = 64;
        if (vkCreateDescriptorPool(s_vkDevice->getDevice(), &poolInfo, nullptr, &s_imguiDescriptorPool) != VK_SUCCESS) {
            s_vulkanError = "Failed to create ImGui descriptor pool";
            fprintf(stderr, "RenderBackend: %s\n", s_vulkanError.c_str());
            delete s_vkDevice;
            s_vkDevice = nullptr;
            return false;
        }
    }

    // Init ImGui Vulkan backend
    {
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = VK_API_VERSION_1_0;
        initInfo.Instance = s_vkDevice->getInstance();
        initInfo.PhysicalDevice = s_vkDevice->getPhysicalDevice();
        initInfo.Device = s_vkDevice->getDevice();
        initInfo.QueueFamily = s_vkDevice->getGraphicsFamily();
        initInfo.Queue = s_vkDevice->getGraphicsQueue();
        initInfo.PipelineInfoMain.RenderPass = s_vkSwapChain->getRenderPass();
        initInfo.DescriptorPool = s_imguiDescriptorPool;
        initInfo.MinImageCount = 2;
        uint32_t imgCount = s_vkSwapChain->getImageCount();
        if (imgCount < 2) {
            s_vulkanError = "Swapchain image count < 2, cannot initialize ImGui";
            fprintf(stderr, "RenderBackend: %s\n", s_vulkanError.c_str());
            vkDestroyDescriptorPool(s_vkDevice->getDevice(), s_imguiDescriptorPool, nullptr);
            delete s_vkDevice;
            s_vkDevice = nullptr;
            return false;
        }
        initInfo.ImageCount = imgCount;
        initInfo.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) {
                fprintf(stderr, "ImGui Vulkan error: %d\n", static_cast<int>(err));
            }
        };

        if (!ImGui_ImplVulkan_Init(&initInfo)) {
            s_vulkanError = "ImGui_ImplVulkan_Init failed";
            fprintf(stderr, "RenderBackend: %s\n", s_vulkanError.c_str());
            vkDestroyDescriptorPool(s_vkDevice->getDevice(), s_imguiDescriptorPool, nullptr);
            delete s_vkDevice;
            s_vkDevice = nullptr;
            return false;
        }
    }

    return true;
}

static void shutdownVulkan() {
    if (!s_vkDevice) return;

    vkDeviceWaitIdle(s_vkDevice->getDevice());
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    if (s_imguiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(s_vkDevice->getDevice(), s_imguiDescriptorPool, nullptr);
        s_imguiDescriptorPool = VK_NULL_HANDLE;
    }
    delete s_vkDevice;
    s_vkDevice = nullptr;
    s_vkSwapChain = nullptr;
    s_vkContext = nullptr;
}

} // namespace RenderBackend
