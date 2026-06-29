#pragma once
#include <graphics/renderer/RenderDevice.h>
#include <graphics/renderer/Types.h>
#include <graphics/renderer/Pipeline.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <graphics/vk/VkBuffer.h>
#include <graphics/vk/VkSampler.h>
#include <graphics/vk/VkPipeline.h>

class VkSwapChain;
class VkRenderContext;
class VkTexture;

class VkRenderDevice : public IRenderDevice {
public:
    VkRenderDevice(uint32_t width, uint32_t height, GLFWwindow* window);
    ~VkRenderDevice() override;

    IBuffer* createBuffer(const BufferDesc& desc) override;
    void destroyBuffer(IBuffer* buffer) override;
    ITexture* createTexture(const TextureDesc& desc) override;
    void destroyTexture(ITexture* texture) override;
    ISampler* createSampler(const SamplerDesc& desc) override;
    void destroySampler(ISampler* sampler) override;
    IShader* createShader(const std::string& vertexSource, const std::string& fragmentSource) override;
    void destroyShader(IShader* shader) override;
    IPipeline* createPipeline(const PipelineDesc& desc) override;
    void destroyPipeline(IPipeline* pipeline) override;
    IRenderContext* getContext() override;
    ISwapChain* getSwapChain() override;
    int getMaxTextureSize() const override;

    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
    VkCommandPool getCommandPool() const { return commandPool; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
    GLFWwindow* window;
    uint32_t width;
    uint32_t height;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkSwapChain* swapChain = nullptr;
    VkRenderContext* context = nullptr;

    std::vector<BlazeBolt::VkBuffer*> buffers;
    std::vector<VkTexture*> textures;
    std::vector<BlazeBolt::VkSampler*> samplers;
    std::vector<IShader*> shaders;
    std::vector<BlazeBolt::VkPipeline*> pipelines;

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    void createPipelineLayout();
    void cleanup();

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
};
