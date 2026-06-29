#include "VkBuffer.h"
#include <graphics/vk/VkUtils.h>
#include <cstring>

namespace BlazeBolt {
VkBuffer::VkBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferDesc& desc)
    : device(device), physicalDevice(physicalDevice), desc(desc) {

    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    switch (desc.type) {
        case BufferType::VERTEX:  usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
        case BufferType::INDEX:   usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
        case BufferType::UNIFORM: usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
    }

    VkMemoryPropertyFlags memFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (!createBuffer(physicalDevice, device, desc.size, usageFlags, memFlags, vkBuffer, memory)) {
        return;
    }

    vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mappedData);
}

VkBuffer::~VkBuffer() {
    if (mappedData) {
        vkUnmapMemory(device, memory);
        mappedData = nullptr;
    }
    if (vkBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vkBuffer, nullptr);
        vkBuffer = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}

void VkBuffer::upload(const void* data, uint32_t size, uint32_t offset) {
    std::memcpy(static_cast<char*>(mappedData) + offset, data, size);
}

void VkBuffer::bind() const {
    // Vulkan binds vertex/index buffers at draw time via vkCmdBindVertexBuffers /
    // vkCmdBindIndexBuffer, and uniforms via descriptor sets. No persistent binding.
}
} // namespace BlazeBolt
