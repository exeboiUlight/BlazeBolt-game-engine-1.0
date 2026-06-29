#pragma once
#include "Types.h"
#include "Buffer.h"
#include "Texture.h"
#include "Sampler.h"
#include "Shader.h"
#include "Pipeline.h"
#include "RenderContext.h"
#include "SwapChain.h"
#include <string>
#include <vector>

struct PipelineDesc;

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    virtual IBuffer* createBuffer(const BufferDesc& desc) = 0;
    virtual void destroyBuffer(IBuffer* buffer) = 0;

    virtual ITexture* createTexture(const TextureDesc& desc) = 0;
    virtual void destroyTexture(ITexture* texture) = 0;

    virtual ISampler* createSampler(const SamplerDesc& desc) = 0;
    virtual void destroySampler(ISampler* sampler) = 0;

    virtual IShader* createShader(const std::string& vertexSource, const std::string& fragmentSource) = 0;
    virtual void destroyShader(IShader* shader) = 0;

    virtual IPipeline* createPipeline(const PipelineDesc& desc) = 0;
    virtual void destroyPipeline(IPipeline* pipeline) = 0;

    virtual IRenderContext* getContext() = 0;
    virtual ISwapChain* getSwapChain() = 0;

    virtual int getMaxTextureSize() const = 0;
};
