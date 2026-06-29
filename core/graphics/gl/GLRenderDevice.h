#pragma once
#include <graphics/renderer/RenderDevice.h>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>

class GLRenderContext;
class GLSwapChain;
class GLBuffer;
class GLTexture;
class GLSampler;
class GLShader;
class GLPipeline;

class GLRenderDevice : public IRenderDevice {
public:
    GLRenderDevice(int windowWidth, int windowHeight, void (*swapBuffersFunc)());
    ~GLRenderDevice() override;

    IBuffer* createBuffer(const BufferDesc& desc) override;
    void destroyBuffer(IBuffer* buffer) override;

    ITexture* createTexture(const TextureDesc& desc) override;
    void destroyTexture(ITexture* texture) override;

    ISampler* createSampler(const SamplerDesc& desc) override;
    void destroySampler(ISampler* sampler) override;

    IShader* createShader(const std::string& vertexSource, const std::string& fragmentSource) override;
    void destroyShader(IShader* shader) override;

    IPipeline* createPipeline(const PipelineDesc& desc) override;
    void destroyPipeline(IPipeline* pipeline) override;

    IRenderContext* getContext() override;
    ISwapChain* getSwapChain() override;

    int getMaxTextureSize() const override;

private:
    GLRenderContext* context;
    GLSwapChain* swapChain;

    std::vector<GLBuffer*> buffers;
    std::vector<GLTexture*> textures;
    std::vector<GLSampler*> samplers;
    std::vector<GLShader*> shaders;
    std::vector<GLPipeline*> pipelines;
};
