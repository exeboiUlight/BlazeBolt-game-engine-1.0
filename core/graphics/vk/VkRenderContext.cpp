#include "VkRenderContext.h"
#include "VkSwapChain.h"
#include "VkPipeline.h"
#include "VkTexture.h"
#include "VkBuffer.h"
#include <stdexcept>

VkRenderContext::VkRenderContext(VkDevice device, VkPhysicalDevice physicalDevice,
                                 VkQueue graphicsQueue, VkQueue presentQueue,
                                 VkCommandPool commandPool,
                                 VkSwapChain* swapChain)
    : device(device)
    , physicalDevice(physicalDevice)
    , graphicsQueue(graphicsQueue)
    , presentQueue(presentQueue)
    , commandPool(commandPool)
    , swapChain(swapChain)
{
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vulkan command buffer");
    }

    createDescriptorResources();
}

VkRenderContext::~VkRenderContext()
{
    destroyDescriptorResources();
    if (commandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        commandBuffer = VK_NULL_HANDLE;
    }
}

void VkRenderContext::createDescriptorResources()
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan descriptor set layout");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 16;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 16;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        throw std::runtime_error("Failed to create Vulkan descriptor pool");
    }

    std::vector<VkDescriptorSetLayout> layouts(16, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 16;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        throw std::runtime_error("Failed to allocate Vulkan descriptor sets");
    }
}

void VkRenderContext::destroyDescriptorResources()
{
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void VkRenderContext::updateDescriptorSet(uint32_t slot, VkImageView imageView, VkSampler sampler)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[slot];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

bool VkRenderContext::beginFrame()
{
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

void VkRenderContext::endFrame()
{
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to end Vulkan command buffer");
    }

    VkSemaphore waitSemaphore = swapChain->getImageAvailableSemaphore();
    VkSemaphore signalSemaphore = swapChain->getRenderFinishedSemaphore();
    VkFence fence = swapChain->getInFlightFence();
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &waitSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signalSemaphore;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Vulkan command buffer");
    }

    swapChain->present();
}

void VkRenderContext::beginRenderPass()
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChain->getRenderPass();
    renderPassInfo.framebuffer = swapChain->getFramebuffer(swapChain->getImageIndex());
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain->getExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VkRenderContext::endRenderPass()
{
    vkCmdEndRenderPass(commandBuffer);
}

void VkRenderContext::bindPipeline(IPipeline* pipeline)
{
    BlazeBolt::VkPipeline* vkPipeline = static_cast<BlazeBolt::VkPipeline*>(pipeline);
    currentPipeline = vkPipeline;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->getHandle());

    if (vkPipeline->getPushConstantOffset() > 0)
    {
        vkCmdPushConstants(commandBuffer, vkPipeline->getLayout(),
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, vkPipeline->getPushConstantOffset(),
                          vkPipeline->getPushConstantData());
    }
}

void VkRenderContext::bindVertexBuffer(IBuffer* buffer, uint32_t stride)
{
    BlazeBolt::VkBuffer* vkBuffer = static_cast<BlazeBolt::VkBuffer*>(buffer);
    currentVkVertexBuffer = vkBuffer->getHandle();

    VkBuffer vertexBuffers[] = {vkBuffer->getHandle()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

void VkRenderContext::bindIndexBuffer(IBuffer* buffer)
{
    BlazeBolt::VkBuffer* vkBuffer = static_cast<BlazeBolt::VkBuffer*>(buffer);
    vkCmdBindIndexBuffer(commandBuffer, vkBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
}

void VkRenderContext::bindTexture(uint32_t slot, ITexture* texture)
{
    VkTexture* vkTexture = static_cast<VkTexture*>(texture);
    updateDescriptorSet(slot, vkTexture->getImageView(), vkTexture->getSampler());

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           currentPipeline->getLayout(), 0, 1,
                           &descriptorSets[slot], 0, nullptr);
}

void VkRenderContext::setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkViewport viewport{};
    viewport.x = static_cast<float>(x);
    viewport.y = static_cast<float>(y) + static_cast<float>(h);
    viewport.width = static_cast<float>(w);
    viewport.height = -static_cast<float>(h);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void VkRenderContext::setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkRect2D scissor{};
    scissor.offset.x = static_cast<int32_t>(x);
    scissor.offset.y = static_cast<int32_t>(y);
    scissor.extent.width = w;
    scissor.extent.height = h;

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VkRenderContext::draw(uint32_t vertexCount, uint32_t instanceCount)
{
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
}

void VkRenderContext::drawIndexed(uint32_t indexCount, uint32_t instanceCount)
{
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
}

void VkRenderContext::clear(float r, float g, float b, float a)
{
    clearColor.color.float32[0] = r;
    clearColor.color.float32[1] = g;
    clearColor.color.float32[2] = b;
    clearColor.color.float32[3] = a;
}
