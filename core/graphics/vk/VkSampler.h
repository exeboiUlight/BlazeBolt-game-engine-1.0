#pragma once
#include <graphics/renderer/Sampler.h>
#include <graphics/renderer/Types.h>
#include <vulkan/vulkan.h>

namespace BlazeBolt {
class VkSampler : public ISampler {
public:
    VkSampler(VkDevice device, const SamplerDesc& desc);
    ~VkSampler() override;

    void bind(uint32_t slot = 0) const override;
    const SamplerDesc& getDesc() const override { return desc; }
    uint64_t getId() const { return reinterpret_cast<uint64_t>(this); }

    ::VkSampler getHandle() const { return sampler; }

private:
    VkDevice device;
    SamplerDesc desc;
    ::VkSampler sampler = VK_NULL_HANDLE;
};
} // namespace BlazeBolt
