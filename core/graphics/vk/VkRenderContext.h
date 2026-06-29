#pragma once
#include <graphics/renderer/RenderContext.h>
#include <graphics/renderer/Pipeline.h>
#include <graphics/renderer/Buffer.h>
#include <graphics/renderer/Texture.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>

class VkSwapChain;
namespace BlazeBolt { class VkPipeline; }

class VkRenderContext : public IRenderContext {
public:
    VkRenderContext(VkDevice device, VkPhysicalDevice physicalDevice,
                    VkQueue graphicsQueue, VkQueue presentQueue,
                    VkCommandPool commandPool,
                    VkSwapChain* swapChain);
    ~VkRenderContext() override;

    bool beginFrame() override;
    void endFrame() override;
    void beginRenderPass() override;
    void endRenderPass() override;
    void bindPipeline(IPipeline* pipeline) override;
    void bindVertexBuffer(IBuffer* buffer, uint32_t stride) override;
    void bindIndexBuffer(IBuffer* buffer) override;
    void bindTexture(uint32_t slot, ITexture* texture) override;
    void setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    void setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) override;
    void clear(float r, float g, float b, float a) override;
    uint64_t getId() const override { return reinterpret_cast<uint64_t>(this); }

    VkCommandBuffer getCurrentCommandBuffer() const { return commandBuffer; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkCommandPool commandPool;
    VkSwapChain* swapChain;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    BlazeBolt::VkPipeline* currentPipeline = nullptr;
    VkBuffer currentVkVertexBuffer = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 16> descriptorSets{};
    int currentDescriptorSlot = 0;

    VkClearValue clearColor;

    void createDescriptorResources();
    void destroyDescriptorResources();
    void updateDescriptorSet(uint32_t slot, VkImageView imageView, VkSampler sampler);
};
