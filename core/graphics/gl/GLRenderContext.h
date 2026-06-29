#pragma once
#include <graphics/renderer/RenderContext.h>
#include <glad/glad.h>
#include <vector>
#include <cstdint>
#include <unordered_map>

struct VAOCacheKey {
    GLuint vbo;
    GLuint ibo;
    uint64_t pipelineId;
    uint32_t stride;

    bool operator==(const VAOCacheKey& o) const {
        return vbo == o.vbo && ibo == o.ibo && pipelineId == o.pipelineId && stride == o.stride;
    }
};

struct VAOCacheKeyHash {
    size_t operator()(const VAOCacheKey& k) const {
        return std::hash<GLuint>()(k.vbo) ^
               (std::hash<GLuint>()(k.ibo) << 1) ^
               (std::hash<uint64_t>()(k.pipelineId) << 2) ^
               (std::hash<uint32_t>()(k.stride) << 3);
    }
};

class GLRenderContext : public IRenderContext {
public:
    GLRenderContext();
    ~GLRenderContext() override;

    bool beginFrame() override;
    void endFrame() override;
    void beginRenderPass() override;
    void endRenderPass() override;

    void clear(float r, float g, float b, float a) override;
    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    void setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;

    void bindPipeline(IPipeline* pipeline) override;
    void bindVertexBuffer(IBuffer* buffer, uint32_t stride) override;
    void bindIndexBuffer(IBuffer* buffer) override;
    void bindTexture(uint32_t slot, ITexture* texture) override;

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) override;

    uint64_t getId() const override { return reinterpret_cast<uint64_t>(this); }

private:
    GLuint getOrCreateVAO();
    void resetCachedState();
    GLenum glPrimitive() const;

    IPipeline* currentPipeline;
    PrimitiveType currentPrimitive;
    GLuint boundVBO;
    GLuint boundIBO;
    uint32_t boundStride;
    uint64_t boundPipelineId;
    bool vaoDirty;

    std::unordered_map<VAOCacheKey, GLuint, VAOCacheKeyHash> vaoCache;
};
