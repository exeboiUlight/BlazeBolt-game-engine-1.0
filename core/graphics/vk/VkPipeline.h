#pragma once
#include <graphics/renderer/Pipeline.h>
#include <graphics/renderer/Types.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

class VkShader;

namespace BlazeBolt {
class VkPipeline : public IPipeline {
public:
    VkPipeline(VkDevice device, VkRenderPass renderPass,
               VkPipelineLayout layout,
               VkShader* vertShader, VkShader* fragShader,
               const PipelineDesc& desc);
    ~VkPipeline() override;

    void bind() const override;
    uint64_t getId() const override { return reinterpret_cast<uint64_t>(this); }

    void setUniform(const std::string& name, float value) override;
    void setUniform(const std::string& name, int value) override;
    void setUniform(const std::string& name, float x, float y) override;
    void setUniform(const std::string& name, float x, float y, float z) override;
    void setUniform(const std::string& name, float x, float y, float z, float w) override;
    void setUniformMatrix3(const std::string& name, const float* matrix) override;
    void setUniformMatrix4(const std::string& name, const float* matrix) override;

    ::VkPipeline getHandle() const { return pipeline; }
    VkPipelineLayout getLayout() const { return pipelineLayout; }
    VkPipelineBindPoint getBindPoint() const { return VK_PIPELINE_BIND_POINT_GRAPHICS; }
    const uint8_t* getPushConstantData() const { return pushConstantData.data(); }
    uint32_t getPushConstantOffset() const { return pushConstantOffset; }

private:
    VkDevice device;
    ::VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout;
    VkPrimitiveTopology primitiveTopology;
    std::vector<VkDynamicState> dynamicStates;

    mutable std::vector<uint8_t> pushConstantData;
    uint32_t pushConstantOffset = 0;
};
} // namespace BlazeBolt
