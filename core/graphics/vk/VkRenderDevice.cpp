#include "VkRenderDevice.h"
#include "VkSwapChain.h"
#include "VkRenderContext.h"
#include "VkBuffer.h"
#include "VkTexture.h"
#include "VkSampler.h"
#include "VkShader.h"
#include "VkPipeline.h"
#include <graphics/vk/VkUtils.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <cstring>

static std::vector<char> readSpirvFile(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::fprintf(stderr, "VkRenderDevice: failed to open SPIR-V file: %s\n", path.c_str());
        return {};
    }
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(size));
    return buffer;
}

struct VkShaderProgram : IShader
{
    VkShader* vertShader;
    VkShader* fragShader;

    VkShaderProgram(VkDevice device, const std::string& vsPath, const std::string& fsPath)
        : vertShader(nullptr), fragShader(nullptr)
    {
        std::vector<char> vsCode = readSpirvFile(vsPath);
        std::vector<char> fsCode = readSpirvFile(fsPath);
        if (!vsCode.empty())
            vertShader = new VkShader(device, VK_SHADER_STAGE_VERTEX_BIT, vsCode);
        if (!fsCode.empty())
            fragShader = new VkShader(device, VK_SHADER_STAGE_FRAGMENT_BIT, fsCode);
    }

    ~VkShaderProgram() override
    {
        delete vertShader;
        delete fragShader;
    }

    uint64_t getId() const override { return reinterpret_cast<uint64_t>(this); }
};

VkRenderDevice::VkRenderDevice(uint32_t width, uint32_t height, GLFWwindow* window)
    : window(window), width(width), height(height)
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    createPipelineLayout();
    swapChain = new VkSwapChain(device, physicalDevice, surface, VK_NULL_HANDLE,
                                 width, height, window);
    context = new VkRenderContext(device, physicalDevice, graphicsQueue, presentQueue,
                                   commandPool, swapChain);
}

VkRenderDevice::~VkRenderDevice()
{
    vkDeviceWaitIdle(device);
    cleanup();
}

void VkRenderDevice::cleanup()
{
    for (auto* p : pipelines) delete p;
    pipelines.clear();
    for (auto* s : shaders) delete s;
    shaders.clear();
    for (auto* sm : samplers) delete sm;
    samplers.clear();
    for (auto* t : textures) delete t;
    textures.clear();
    for (auto* b : buffers) delete b;
    buffers.clear();

    delete context;
    context = nullptr;
    delete swapChain;
    swapChain = nullptr;

    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
    if (debugMessenger != VK_NULL_HANDLE)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }
    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}

void VkRenderDevice::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "BlazeBolt";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "BlazeBoltEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = getRequiredExtensions(g_enableValidationLayers);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (g_enableValidationLayers)
    {
        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("VkRenderDevice: failed to create instance");
}

void VkRenderDevice::setupDebugMessenger()
{
    if (!g_enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func)
        func(instance, &createInfo, nullptr, &debugMessenger);
}

void VkRenderDevice::createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("VkRenderDevice: failed to create window surface");
}

void VkRenderDevice::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("VkRenderDevice: no Vulkan-capable GPUs found");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& dev : devices)
    {
        if (isDeviceSuitable(dev))
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physicalDevice = dev;
                return;
            }
        }
    }

    for (const auto& dev : devices)
    {
        if (isDeviceSuitable(dev))
        {
            physicalDevice = dev;
            return;
        }
    }

    throw std::runtime_error("VkRenderDevice: no suitable GPU found");
}

bool VkRenderDevice::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if (!indices.isComplete()) return false;

    if (!checkDeviceExtensionSupport(device)) return false;

    SwapChainSupportDetails details = querySwapChainSupport(device, surface);
    if (details.formats.empty() || details.presentModes.empty()) return false;

    return true;
}

bool VkRenderDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (const auto& ext : availableExtensions)
    {
        if (std::strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            return true;
    }
    return false;
}

void VkRenderDevice::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    graphicsFamily = indices.graphicsFamily.value();
    presentFamily = indices.presentFamily.value();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo graphicsQueueInfo{};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.queueFamilyIndex = graphicsFamily;
    graphicsQueueInfo.queueCount = 1;
    graphicsQueueInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(graphicsQueueInfo);

    if (graphicsFamily != presentFamily)
    {
        VkDeviceQueueCreateInfo presentQueueInfo{};
        presentQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueInfo.queueFamilyIndex = presentFamily;
        presentQueueInfo.queueCount = 1;
        presentQueueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(presentQueueInfo);
    }

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("VkRenderDevice: failed to create logical device");

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
}

void VkRenderDevice::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("VkRenderDevice: failed to create command pool");
}

void VkRenderDevice::createPipelineLayout()
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("VkRenderDevice: failed to create descriptor set layout");

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 128;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
        throw std::runtime_error("VkRenderDevice: failed to create pipeline layout");
    }
}

IBuffer* VkRenderDevice::createBuffer(const BufferDesc& desc)
{
    auto* buf = new BlazeBolt::VkBuffer(device, physicalDevice, desc);
    buffers.push_back(buf);
    return buf;
}

void VkRenderDevice::destroyBuffer(IBuffer* buffer)
{
    auto* buf = static_cast<BlazeBolt::VkBuffer*>(buffer);
    auto it = std::find(buffers.begin(), buffers.end(), buf);
    if (it != buffers.end()) buffers.erase(it);
    delete buf;
}

ITexture* VkRenderDevice::createTexture(const TextureDesc& desc)
{
    auto* tex = new VkTexture(device, physicalDevice, commandPool, graphicsQueue, desc);
    textures.push_back(tex);
    return tex;
}

void VkRenderDevice::destroyTexture(ITexture* texture)
{
    auto* tex = static_cast<VkTexture*>(texture);
    auto it = std::find(textures.begin(), textures.end(), tex);
    if (it != textures.end()) textures.erase(it);
    delete tex;
}

ISampler* VkRenderDevice::createSampler(const SamplerDesc& desc)
{
    auto* sampler = new BlazeBolt::VkSampler(device, desc);
    samplers.push_back(sampler);
    return sampler;
}

void VkRenderDevice::destroySampler(ISampler* sampler)
{
    auto* s = static_cast<BlazeBolt::VkSampler*>(sampler);
    auto it = std::find(samplers.begin(), samplers.end(), s);
    if (it != samplers.end()) samplers.erase(it);
    delete s;
}

IShader* VkRenderDevice::createShader(const std::string& vertexSource, const std::string& fragmentSource)
{
    auto* program = new VkShaderProgram(device, vertexSource, fragmentSource);
    shaders.push_back(program);
    return program;
}

void VkRenderDevice::destroyShader(IShader* shader)
{
    auto* s = static_cast<VkShaderProgram*>(shader);
    auto it = std::find(shaders.begin(), shaders.end(), s);
    if (it != shaders.end()) shaders.erase(it);
    delete s;
}

IPipeline* VkRenderDevice::createPipeline(const PipelineDesc& desc)
{
    if (!desc.vertexShader || !desc.fragmentShader) return nullptr;
    auto* program = static_cast<VkShaderProgram*>(desc.vertexShader);
    VkRenderPass rp = swapChain->getRenderPass();
    auto* pipeline = new BlazeBolt::VkPipeline(device, rp, pipelineLayout,
                                     program->vertShader, program->fragShader, desc);
    pipelines.push_back(pipeline);
    return pipeline;
}

void VkRenderDevice::destroyPipeline(IPipeline* pipeline)
{
    auto* p = static_cast<BlazeBolt::VkPipeline*>(pipeline);
    auto it = std::find(pipelines.begin(), pipelines.end(), p);
    if (it != pipelines.end()) pipelines.erase(it);
    delete p;
}

IRenderContext* VkRenderDevice::getContext()
{
    return context;
}

ISwapChain* VkRenderDevice::getSwapChain()
{
    return swapChain;
}

int VkRenderDevice::getMaxTextureSize() const
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    return static_cast<int>(props.limits.maxImageDimension2D);
}
