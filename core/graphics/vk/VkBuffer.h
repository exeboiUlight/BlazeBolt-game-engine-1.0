#pragma once
#include <graphics/renderer/Buffer.h>
#include <graphics/renderer/Types.h>
#include <vulkan/vulkan.h>

namespace BlazeBolt {
class VkBuffer : public IBuffer {
public:
    VkBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferDesc& desc);
    ~VkBuffer() override;

    void upload(const void* data, uint32_t size, uint32_t offset = 0) override;
    void bind() const override;
    uint32_t getSize() const override { return desc.size; }
    BufferType getType() const override { return desc.type; }
    uint64_t getId() const { return reinterpret_cast<uint64_t>(this); }

    ::VkBuffer getHandle() const { return vkBuffer; }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    BufferDesc desc;
    ::VkBuffer vkBuffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedData = nullptr;
};
} // namespace BlazeBolt
