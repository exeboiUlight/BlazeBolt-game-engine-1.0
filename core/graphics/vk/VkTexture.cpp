#include "VkTexture.h"
#include <graphics/vk/VkUtils.h>
#include <cstring>

VkFormat VkTexture::toVkFormat(TextureFormat fmt) const {
    switch (fmt) {
        case TextureFormat::R8:            return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG8:           return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RGB8:          return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA8:         return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::SRGB8_ALPHA8:  return VK_FORMAT_R8G8B8A8_SRGB;
    }
    return VK_FORMAT_R8G8B8A8_SRGB;
}

VkTexture::VkTexture(VkDevice device, VkPhysicalDevice physicalDevice,
                     VkCommandPool commandPool, VkQueue graphicsQueue,
                     const TextureDesc& desc)
    : device(device), physicalDevice(physicalDevice),
      commandPool(commandPool), graphicsQueue(graphicsQueue),
      width(desc.width), height(desc.height), format(desc.format) {

    VkFormat vkFormat = toVkFormat(format);

    if (!createImage(physicalDevice, device, width, height, vkFormat,
                     VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     image, imageMemory)) {
        return;
    }

    imageView = createImageView(device, image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    if (imageView == VK_NULL_HANDLE) {
        return;
    }

    transitionImageLayout(device, commandPool, graphicsQueue, image, vkFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        sampler = VK_NULL_HANDLE;
    }
}

VkTexture::~VkTexture() {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, imageMemory, nullptr);
        imageMemory = VK_NULL_HANDLE;
    }
}

void VkTexture::upload(const void* pixels) {
    if (!pixels || image == VK_NULL_HANDLE) {
        return;
    }

    VkFormat vkFormat = toVkFormat(format);

    uint32_t bpp = 4;
    switch (format) {
        case TextureFormat::R8:           bpp = 1; break;
        case TextureFormat::RG8:          bpp = 2; break;
        case TextureFormat::RGB8:         bpp = 3; break;
        case TextureFormat::RGBA8:        bpp = 4; break;
        case TextureFormat::SRGB8_ALPHA8: bpp = 4; break;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * bpp;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    if (!createBuffer(physicalDevice, device, imageSize,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingMemory)) {
        return;
    }

    void* data = nullptr;
    vkMapMemory(device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingMemory);

    transitionImageLayout(device, commandPool, graphicsQueue, image, vkFormat,
                          currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    currentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, image, width, height);

    transitionImageLayout(device, commandPool, graphicsQueue, image, vkFormat,
                          currentLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void VkTexture::bind(uint32_t /*slot*/) const {
    // Vulkan binds textures via descriptor sets at draw time, not through
    // direct texture binding. This method is a no-op for the Vulkan backend.
}
