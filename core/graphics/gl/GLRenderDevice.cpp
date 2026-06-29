#include "GLRenderDevice.h"
#include "GLRenderContext.h"
#include "GLSwapChain.h"
#include "GLBuffer.h"
#include "GLTexture.h"
#include "GLSampler.h"
#include "GLShader.h"
#include "GLPipeline.h"
#include <algorithm>

GLRenderDevice::GLRenderDevice(int windowWidth, int windowHeight, void (*swapBuffersFunc)())
    : context(new GLRenderContext()), swapChain(new GLSwapChain(windowWidth, windowHeight))
{
    swapChain->setSwapBuffersFunc(swapBuffersFunc);
}

GLRenderDevice::~GLRenderDevice() {
    for (auto* p : pipelines) delete p;
    for (auto* s : shaders) delete s;
    for (auto* sm : samplers) delete sm;
    for (auto* t : textures) delete t;
    for (auto* b : buffers) delete b;
    delete swapChain;
    delete context;
}

IBuffer* GLRenderDevice::createBuffer(const BufferDesc& desc) {
    auto* buf = new GLBuffer(desc);
    buffers.push_back(buf);
    return buf;
}

void GLRenderDevice::destroyBuffer(IBuffer* buffer) {
    auto* buf = static_cast<GLBuffer*>(buffer);
    auto it = std::find(buffers.begin(), buffers.end(), buf);
    if (it != buffers.end()) buffers.erase(it);
    delete buf;
}

ITexture* GLRenderDevice::createTexture(const TextureDesc& desc) {
    auto* tex = new GLTexture(desc);
    textures.push_back(tex);
    return tex;
}

void GLRenderDevice::destroyTexture(ITexture* texture) {
    auto* tex = static_cast<GLTexture*>(texture);
    auto it = std::find(textures.begin(), textures.end(), tex);
    if (it != textures.end()) textures.erase(it);
    delete tex;
}

ISampler* GLRenderDevice::createSampler(const SamplerDesc& desc) {
    auto* sampler = new GLSampler(desc);
    samplers.push_back(sampler);
    return sampler;
}

void GLRenderDevice::destroySampler(ISampler* sampler) {
    auto* s = static_cast<GLSampler*>(sampler);
    auto it = std::find(samplers.begin(), samplers.end(), s);
    if (it != samplers.end()) samplers.erase(it);
    delete s;
}

IShader* GLRenderDevice::createShader(const std::string& vertexSource, const std::string& fragmentSource) {
    auto* shader = new GLShader(vertexSource, fragmentSource);
    shaders.push_back(shader);
    return shader;
}

void GLRenderDevice::destroyShader(IShader* shader) {
    auto* s = static_cast<GLShader*>(shader);
    auto it = std::find(shaders.begin(), shaders.end(), s);
    if (it != shaders.end()) shaders.erase(it);
    delete s;
}

IPipeline* GLRenderDevice::createPipeline(const PipelineDesc& desc) {
    if (!desc.vertexShader || !desc.fragmentShader) return nullptr;
    auto* vs = static_cast<GLShader*>(desc.vertexShader);
    auto* fs = static_cast<GLShader*>(desc.fragmentShader);
    // Use vertex shader's program (VS and FS are linked together)
    GLuint program = vs->getProgram();
    auto* pipeline = new GLPipeline(desc, program);
    pipelines.push_back(pipeline);
    return pipeline;
}

void GLRenderDevice::destroyPipeline(IPipeline* pipeline) {
    auto* p = static_cast<GLPipeline*>(pipeline);
    auto it = std::find(pipelines.begin(), pipelines.end(), p);
    if (it != pipelines.end()) pipelines.erase(it);
    delete p;
}

IRenderContext* GLRenderDevice::getContext() {
    return context;
}

ISwapChain* GLRenderDevice::getSwapChain() {
    return swapChain;
}

int GLRenderDevice::getMaxTextureSize() const {
    GLint size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
    return static_cast<int>(size);
}
