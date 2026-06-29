#include "window.h"
#include <graphics/renderer/RenderAPI.h>
#include <graphics/gl/GLRenderDevice.h>
#include <graphics/vk/VkRenderDevice.h>

IRenderDevice* Window::createRenderDevice(RenderAPI api, uint32_t width, uint32_t height)
{
    switch (api)
    {
    case RenderAPI::Vulkan:
        return new VkRenderDevice(width, height, window);
    case RenderAPI::OpenGL:
    default:
        return new GLRenderDevice(static_cast<int>(width), static_cast<int>(height), []() { glfwSwapBuffers(glfwGetCurrentContext()); });
    }
}