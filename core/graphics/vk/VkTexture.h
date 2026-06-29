#pragma once
#include <graphics/renderer/Texture.h>
#include <graphics/renderer/Types.h>
#include <vulkan/vulkan.h>

class VkTexture : public ITexture {
public:
    VkTexture(VkDevice device, VkPhysicalDevice physicalDevice,
              VkCommandPool commandPool, VkQueue graphicsQueue,
              const TextureDesc& desc);
    ~VkTexture() override;

    void upload(const void* pixels) override;
    void bind(uint32_t slot = 0) const override;
    uint32_t getWidth() const override { return width; }
    uint32_t getHeight() const override { return height; }
    TextureFormat getFormat() const override { return format; }
    uint64_t getId() const { return reinterpret_cast<uint64_t>(this); }

    VkImageView getImageView() const { return imageView; }
    VkSampler getSampler() const { return sampler; }
    VkImageLayout getLayout() const { return currentLayout; }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;
    uint32_t width, height;
    TextureFormat format;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkFormat toVkFormat(TextureFormat fmt) const;
};
