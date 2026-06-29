#pragma once
#include "Types.h"

class IPipeline;
class IBuffer;
class ITexture;

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    virtual bool beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void beginRenderPass() = 0;
    virtual void endRenderPass() = 0;

    virtual void bindPipeline(IPipeline* pipeline) = 0;
    virtual void bindVertexBuffer(IBuffer* buffer, uint32_t stride) = 0;
    virtual void bindIndexBuffer(IBuffer* buffer) = 0;
    virtual void bindTexture(uint32_t slot, ITexture* texture) = 0;

    virtual void setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;
    virtual void setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) = 0;

    virtual void clear(float r, float g, float b, float a) = 0;

    virtual uint64_t getId() const = 0;
};
