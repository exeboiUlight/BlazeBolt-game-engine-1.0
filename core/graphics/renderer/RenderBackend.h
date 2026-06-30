#pragma once
#include <graphics/renderer/RenderAPI.h>
#include <GLFW/glfw3.h>
#include <cstdint>

class IRenderDevice;

namespace RenderBackend {

bool Init(RenderAPI api, GLFWwindow* window);
void Shutdown();
bool BeginFrame();
void EndFrame();
void NewImGuiFrame();
void RenderImGui();
void ResizeSwapchain(int width, int height);

RenderAPI GetCurrentAPI();
void RequestSwitch(RenderAPI api);
RenderAPI CheckPendingSwitch();
bool IsInitialized();
GLFWwindow* GetWindow();

} // namespace RenderBackend
