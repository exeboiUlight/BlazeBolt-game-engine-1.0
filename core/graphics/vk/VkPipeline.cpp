#include "VkPipeline.h"
#include "VkShader.h"
#include <cstring>
#include <stdexcept>
#include <array>

namespace BlazeBolt {
static VkPrimitiveTopology mapPrimitiveTopology(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::TRIANGLES:      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveType::TRIANGLE_FAN:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case PrimitiveType::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveType::LINES:          return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveType::POINTS:         return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default:                            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static VkFormat mapVertexFormat(uint8_t size, bool normalized)
{
    if (normalized)
    {
        switch (size)
        {
            case 1: return VK_FORMAT_R32_SFLOAT;
            case 2: return VK_FORMAT_R32G32_SFLOAT;
            case 3: return VK_FORMAT_R32G32B32_SFLOAT;
            case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
    }
    else
    {
        switch (size)
        {
            case 1: return VK_FORMAT_R32_SFLOAT;
            case 2: return VK_FORMAT_R32G32_SFLOAT;
            case 3: return VK_FORMAT_R32G32B32_SFLOAT;
            case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
    }
}

static VkBlendFactor mapBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
        case BlendFactor::ZERO:                return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::ONE:                 return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DST_ALPHA:           return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        default:                               return VK_BLEND_FACTOR_ONE;
    }
}

static VkBlendOp mapBlendOp(BlendOp op)
{
    switch (op)
    {
        case BlendOp::ADD:               return VK_BLEND_OP_ADD;
        case BlendOp::SUBTRACT:          return VK_BLEND_OP_SUBTRACT;
        case BlendOp::REVERSE_SUBTRACT:  return VK_BLEND_OP_REVERSE_SUBTRACT;
        default:                         return VK_BLEND_OP_ADD;
    }
}

VkPipeline::VkPipeline(VkDevice device, VkRenderPass renderPass,
                       VkPipelineLayout layout,
                       VkShader* vertShader, VkShader* fragShader,
                       const PipelineDesc& desc)
    : device(device)
    , pipelineLayout(layout)
    , primitiveTopology(mapPrimitiveTopology(desc.primitive))
{
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        vertShader->getStageCreateInfo(),
        fragShader->getStageCreateInfo()
    };

    std::vector<VkVertexInputBindingDescription> bindingDescs;
    std::vector<VkVertexInputAttributeDescription> attributeDescs;

    if (!desc.vertexAttributes.empty())
    {
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = desc.vertexStride;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs.push_back(bindingDesc);

        attributeDescs.reserve(desc.vertexAttributes.size());
        for (const auto& attr : desc.vertexAttributes)
        {
            VkVertexInputAttributeDescription vkAttr{};
            vkAttr.location = attr.location;
            vkAttr.binding = 0;
            vkAttr.format = mapVertexFormat(attr.size, attr.normalized);
            vkAttr.offset = attr.offset;
            attributeDescs.push_back(vkAttr);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = primitiveTopology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 0.0f;
    viewport.height = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {0, 0};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = desc.rasterizer.cullFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.rasterizer.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = desc.blend.enabled ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = mapBlendFactor(desc.blend.srcColor);
    colorBlendAttachment.dstColorBlendFactor = mapBlendFactor(desc.blend.dstColor);
    colorBlendAttachment.colorBlendOp = mapBlendOp(desc.blend.colorOp);
    colorBlendAttachment.srcAlphaBlendFactor = mapBlendFactor(desc.blend.srcAlpha);
    colorBlendAttachment.dstAlphaBlendFactor = mapBlendFactor(desc.blend.dstAlpha);
    colorBlendAttachment.alphaBlendOp = mapBlendOp(desc.blend.alphaOp);
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan graphics pipeline");
    }

    pushConstantData.resize(128, 0);
    pushConstantOffset = 0;
}

VkPipeline::~VkPipeline()
{
    if (pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
}

void VkPipeline::bind() const
{
}

void VkPipeline::setUniform(const std::string& name, float value)
{
    if (pushConstantOffset + 4 > pushConstantData.size()) return;
    std::memcpy(pushConstantData.data() + pushConstantOffset, &value, 4);
    pushConstantOffset += 4;
}

void VkPipeline::setUniform(const std::string& name, int value)
{
    if (pushConstantOffset + 4 > pushConstantData.size()) return;
    std::memcpy(pushConstantData.data() + pushConstantOffset, &value, 4);
    pushConstantOffset += 4;
}

void VkPipeline::setUniform(const std::string& name, float x, float y)
{
    if (pushConstantOffset + 8 > pushConstantData.size()) return;
    float vec[2] = { x, y };
    std::memcpy(pushConstantData.data() + pushConstantOffset, vec, 8);
    pushConstantOffset += 8;
}

void VkPipeline::setUniform(const std::string& name, float x, float y, float z)
{
    if (pushConstantOffset + 12 > pushConstantData.size()) return;
    float vec[3] = { x, y, z };
    std::memcpy(pushConstantData.data() + pushConstantOffset, vec, 12);
    pushConstantOffset += 12;
}

void VkPipeline::setUniform(const std::string& name, float x, float y, float z, float w)
{
    if (pushConstantOffset + 16 > pushConstantData.size()) return;
    float vec[4] = { x, y, z, w };
    std::memcpy(pushConstantData.data() + pushConstantOffset, vec, 16);
    pushConstantOffset += 16;
}

void VkPipeline::setUniformMatrix3(const std::string& name, const float* matrix)
{
    if (pushConstantOffset + 36 > pushConstantData.size()) return;
    std::memcpy(pushConstantData.data() + pushConstantOffset, matrix, 36);
    pushConstantOffset += 36;
}

void VkPipeline::setUniformMatrix4(const std::string& name, const float* matrix)
{
    if (pushConstantOffset + 64 > pushConstantData.size()) return;
    std::memcpy(pushConstantData.data() + pushConstantOffset, matrix, 64);
    pushConstantOffset += 64;
}
} // namespace BlazeBolt
