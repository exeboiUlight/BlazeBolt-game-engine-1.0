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
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// ----------------------------------------------------------------
// Convert GLSL #version 410 (desktop) to Vulkan-compatible GLSL.
//   - Changes #version 410 -> #version 450
//   - Wraps non-sampler uniforms in layout(push_constant) uniform PushBlock { ... } push;
//   - Adds layout(binding = 0) to sampler2D uniforms
//   - Replaces uniform variable references with push.<name>
// ----------------------------------------------------------------
static std::string convertGlslToVulkan(const std::string& source)
{
    std::string result = source;

    // 1. Replace #version
    {
        size_t pos = result.find("#version 410");
        if (pos != std::string::npos)
            result.replace(pos, 12, "#version 450");
    }

    // 2. Collect non-sampler uniform declarations and build push constant block
    std::vector<std::string> uniformNames;
    std::vector<std::string> uniformDecls;
    std::ostringstream pushBlock;

    std::istringstream stream(source);
    std::string line;
    bool hasNonSamplerUniforms = false;

    while (std::getline(stream, line))
    {
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);

        // Check if this is a non-sampler uniform declaration
        if (trimmed.find("uniform ") == 0 &&
            trimmed.find("sampler") == std::string::npos)
        {
            // Extract variable name (remove trailing ;)
            std::string decl = trimmed;
            if (!decl.empty() && decl.back() == ';')
                decl.pop_back();

            // Extract name after last space
            size_t lastSpace = decl.find_last_of(" \t");
            std::string varName;
            if (lastSpace != std::string::npos)
            {
                varName = decl.substr(lastSpace + 1);
                // Remove array brackets for the name
                size_t bracket = varName.find('[');
                if (bracket != std::string::npos)
                    varName = varName.substr(0, bracket);
            }

            if (!varName.empty())
            {
                uniformNames.push_back(varName);
                // Remove 'uniform ' prefix and add to push block
                std::string pushDecl = decl.substr(8); // remove "uniform "
                pushBlock << "    " << pushDecl << ";\n";
                hasNonSamplerUniforms = true;
            }
        }
    }

    // 3. Modify the source:
    //    a) Wrap non-sampler uniforms in push constant block
    //    b) Add layout(binding=0) to sampler2D
    //    c) Replace variable references

    if (hasNonSamplerUniforms)
    {
        // Replace uniform declarations with push constant block
        // We insert the block before the first non-sampler uniform
        // and remove all individual non-sampler uniform lines

        std::string processed;
        std::istringstream inStream(result);
        bool pushBlockInserted = false;

        while (std::getline(inStream, line))
        {
            std::string trimmed = line;
            size_t firstNonSpace = trimmed.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos)
                trimmed = trimmed.substr(firstNonSpace);

            bool isNonSamplerUniform = (trimmed.find("uniform ") == 0 &&
                                        trimmed.find("sampler") == std::string::npos);

            if (isNonSamplerUniform)
            {
                if (!pushBlockInserted)
                {
                    processed += "layout(push_constant) uniform PushBlock {\n";
                    processed += pushBlock.str();
                    processed += "} push;\n\n";
                    pushBlockInserted = true;
                }
                // Skip this line (it's now in the push constant block)
                continue;
            }

            processed += line + "\n";
        }

        result = processed;

        // Replace variable references: u_Name -> push.u_Name
        for (const auto& name : uniformNames)
        {
            size_t pos = 0;
            while ((pos = result.find(name, pos)) != std::string::npos)
            {
                // Check if already prefixed with "push."
                if (pos >= 5 && result.substr(pos - 5, 5) == "push.")
                {
                    pos += name.size();
                    continue;
                }
                // Check preceding char: should be non-alphanumeric
                if (pos > 0)
                {
                    char prev = result[pos - 1];
                    if (std::isalnum(prev) || prev == '_')
                    {
                        pos += name.size();
                        continue;
                    }
                }
                result.insert(pos, "push.");
                pos += name.size() + 5;
            }
        }
    }

    // Add layout(binding=0) to sampler2D uniforms
    {
        size_t pos = 0;
        while ((pos = result.find("uniform sampler2D", pos)) != std::string::npos)
        {
            size_t lineStart = result.rfind('\n', pos);
            if (lineStart == std::string::npos) lineStart = 0;
            else lineStart++;

            std::string prefix = "layout(binding = 0) ";
            result.insert(lineStart, prefix);
            pos = lineStart + prefix.size() + 1;
        }
    }

    return result;
}

static VkShader* compileGLSL(VkDevice device, VkShaderStageFlagBits stage, const std::string& source)
{
    if (source.empty()) return nullptr;

    std::string vulkanSource = convertGlslToVulkan(source);

    // Resolve glslc path from VULKAN_SDK environment variable
    const char* sdkPath = std::getenv("VULKAN_SDK");
    if (!sdkPath)
    {
        std::fprintf(stderr, "VkShaderProgram: VULKAN_SDK environment variable not set\n");
        return nullptr;
    }
    std::string glslcPath = std::string(sdkPath) + "\\Bin\\glslc.exe";

    // Create unique temp file names in system temp directory
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath) == 0)
        std::strcpy(tempPath, ".");
    char tempName[MAX_PATH];
    if (GetTempFileNameA(tempPath, "BB", 0, tempName) == 0)
    {
        std::fprintf(stderr, "VkShaderProgram: failed to generate temp file name\n");
        return nullptr;
    }
    std::string base = tempName;
    std::string srcFile = base + ((stage == VK_SHADER_STAGE_VERTEX_BIT) ? ".vert" : ".frag");
    std::string spvFile = base + ".spv";

    // Write converted GLSL to temp source file
    {
        std::ofstream out(srcFile, std::ios::binary);
        if (!out)
        {
            std::fprintf(stderr, "VkShaderProgram: failed to create temp source file\n");
            return nullptr;
        }
        out << vulkanSource;
    }

    // Invoke glslc as a subprocess
    std::string cmd = "\"" + glslcPath + "\" \"" + srcFile + "\" -o \"" + spvFile + "\"";
    int ret = std::system(cmd.c_str());
    if (ret != 0)
    {
        std::fprintf(stderr, "VkShaderProgram: glslc compilation failed (exit code %d)\n", ret);
        std::remove(base.c_str());
        std::remove(srcFile.c_str());
        std::remove(spvFile.c_str());
        return nullptr;
    }

    // Read compiled SPIR-V binary
    std::ifstream in(spvFile, std::ios::binary | std::ios::ate);
    if (!in)
    {
        std::fprintf(stderr, "VkShaderProgram: failed to read compiled SPIR-V\n");
        std::remove(base.c_str());
        std::remove(srcFile.c_str());
        std::remove(spvFile.c_str());
        return nullptr;
    }
    size_t fileSize = static_cast<size_t>(in.tellg());
    in.seekg(0);
    std::vector<uint32_t> spirv(fileSize / sizeof(uint32_t));
    in.read(reinterpret_cast<char*>(spirv.data()), static_cast<std::streamsize>(fileSize));
    in.close();

    // Clean up temp files
    std::remove(base.c_str());
    std::remove(srcFile.c_str());
    std::remove(spvFile.c_str());

    if (spirv.empty()) return nullptr;

    // Create Vulkan shader module from SPIR-V
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule module;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VkShaderProgram: failed to create shader module\n");
        return nullptr;
    }

    return new VkShader(device, stage, module);
}

struct VkShaderProgram : IShader
{
    VkShader* vertShader;
    VkShader* fragShader;

    VkShaderProgram(VkDevice device, const std::string& vsSource, const std::string& fsSource)
        : vertShader(nullptr), fragShader(nullptr)
    {
        vertShader = compileGLSL(device, VK_SHADER_STAGE_VERTEX_BIT, vsSource);
        fragShader = compileGLSL(device, VK_SHADER_STAGE_FRAGMENT_BIT, fsSource);
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
    if (debugMessenger != VK_NULL_HANDLE && g_hasDebugUtilsExtension)
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

    // Check if validation layers are actually available
    bool useValidation = g_enableValidationLayers;
    if (useValidation) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        bool found = false;
        for (const auto& layer : availableLayers) {
            if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::fprintf(stderr, "VK_LAYER_KHRONOS_validation not found, disabling validation\n");
            useValidation = false;
        }
    }

    // Check VK_EXT_debug_utils support before adding it, to avoid VK_ERROR_EXTENSION_NOT_PRESENT
    bool debugUtilsSupported = false;
    if (useValidation) {
        uint32_t extCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> availableExts(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, availableExts.data());
        for (const auto& ext : availableExts) {
            if (std::strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                debugUtilsSupported = true;
                break;
            }
        }
    }

    std::vector<const char*> extensions = getRequiredExtensions(useValidation && debugUtilsSupported);
    g_hasDebugUtilsExtension = useValidation && debugUtilsSupported;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (useValidation)
    {
        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_ERROR_LAYER_NOT_PRESENT && useValidation) {
        std::fprintf(stderr, "VK_LAYER_KHRONOS_validation not present, retrying without\n");
        useValidation = false;
        g_hasDebugUtilsExtension = false;
        extensions = getRequiredExtensions(false);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        result = vkCreateInstance(&createInfo, nullptr, &instance);
    }

    if (result != VK_SUCCESS)
        throw std::runtime_error("VkRenderDevice: failed to create instance");
}

void VkRenderDevice::setupDebugMessenger()
{
    if (!g_hasDebugUtilsExtension) return;

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
    {
        VkResult res = func(instance, &createInfo, nullptr, &debugMessenger);
        if (res != VK_SUCCESS)
            std::fprintf(stderr, "VkRenderDevice: failed to set up debug messenger (VkResult=%d)\n", static_cast<int>(res));
    }
}

void VkRenderDevice::createSurface()
{
    VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (res != VK_SUCCESS) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "VkRenderDevice: failed to create window surface (VkResult=%d)", static_cast<int>(res));
        throw std::runtime_error(buf);
    }
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
