#pragma once
#include <graphics/renderer/Shader.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VkShader : public IShader {
public:
    VkShader(VkDevice device, VkShaderStageFlagBits stage, const std::vector<char>& spirvCode);
    VkShader(VkDevice device, VkShaderStageFlagBits stage, VkShaderModule module);
    ~VkShader() override;

    void bind() const;
    uint64_t getId() const override { return reinterpret_cast<uint64_t>(this); }

    VkShaderModule getModule() const { return shaderModule; }
    VkShaderStageFlagBits getStage() const { return stage; }
    VkPipelineShaderStageCreateInfo getStageCreateInfo() const;

private:
    VkDevice device;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage;
    bool ownsModule;
};
