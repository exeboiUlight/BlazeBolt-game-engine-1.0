#pragma once
#include <graphics/renderer/SwapChain.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

class VkSwapChain : public ISwapChain {
public:
    VkSwapChain(VkDevice device, VkPhysicalDevice physicalDevice,
                VkSurfaceKHR surface, VkRenderPass renderPass,
                uint32_t width, uint32_t height,
                GLFWwindow* window);
    ~VkSwapChain() override;

    bool acquireNextImage();
    void present() override;
    uint32_t getImageIndex() const { return currentImageIndex; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }
    int getWidth() const override { return static_cast<int>(swapChainExtent.width); }
    int getHeight() const override { return static_cast<int>(swapChainExtent.height); }
    uint64_t getId() const { return reinterpret_cast<uint64_t>(this); }
    void resize(int width, int height) override { recreate(static_cast<uint32_t>(width), static_cast<uint32_t>(height)); }

    VkSwapchainKHR getHandle() const { return swapChain; }
    VkExtent2D getExtent() const { return swapChainExtent; }
    VkFramebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; }
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    VkRenderPass getRenderPass() const { return renderPass; }
    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
    VkSemaphore getImageAvailableSemaphore() const { return imageAvailableSemaphore; }
    VkSemaphore getRenderFinishedSemaphore() const { return renderFinishedSemaphore; }
    VkFence getInFlightFence() const { return inFlightFence; }

    void recreate(uint32_t newWidth, uint32_t newHeight);

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkRenderPass renderPass;
    GLFWwindow* window;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    uint32_t currentImageIndex = 0;

    void createSwapChain(uint32_t width, uint32_t height);
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();
    void cleanupSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
};
