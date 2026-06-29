#include "VkSampler.h"

namespace BlazeBolt {
static VkFilter toVkFilter(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::NEAREST: return VK_FILTER_NEAREST;
        case TextureFilter::LINEAR:  return VK_FILTER_LINEAR;
    }
    return VK_FILTER_LINEAR;
}

static VkSamplerAddressMode toVkAddressMode(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrap::CLAMP:           return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrap::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
}

VkSampler::VkSampler(VkDevice device, const SamplerDesc& desc)
    : device(device), desc(desc) {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = toVkFilter(desc.mag);
    samplerInfo.minFilter = toVkFilter(desc.min);
    samplerInfo.addressModeU = toVkAddressMode(desc.wrapU);
    samplerInfo.addressModeV = toVkAddressMode(desc.wrapV);
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

VkSampler::~VkSampler() {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
}

void VkSampler::bind(uint32_t /*slot*/) const {
    // Vulkan binds samplers via descriptor sets at draw time, not through
    // direct sampler binding. This method is a no-op for the Vulkan backend.
}
} // namespace BlazeBolt
