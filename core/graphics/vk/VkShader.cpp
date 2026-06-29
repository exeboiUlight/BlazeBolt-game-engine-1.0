#include "VkShader.h"

VkShader::VkShader(VkDevice device, VkShaderStageFlagBits stage, const std::vector<char>& spirvCode)
    : device(device), stage(stage)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spirvCode.data());

    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
}

VkShader::~VkShader()
{
    if (shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
}

void VkShader::bind() const
{
}

VkPipelineShaderStageCreateInfo VkShader::getStageCreateInfo() const
{
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";
    return stageInfo;
}
