#include "GLRenderContext.h"
#include "GLBuffer.h"
#include "GLTexture.h"
#include "GLPipeline.h"
#include <algorithm>

static GLenum toGLPrimitive(PrimitiveType p) {
    switch (p) {
        case PrimitiveType::TRIANGLES:     return GL_TRIANGLES;
        case PrimitiveType::TRIANGLE_FAN:  return GL_TRIANGLE_FAN;
        case PrimitiveType::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case PrimitiveType::LINES:         return GL_LINES;
        case PrimitiveType::POINTS:        return GL_POINTS;
    }
    return GL_TRIANGLES;
}

GLRenderContext::GLRenderContext()
    : currentPipeline(nullptr)
    , currentPrimitive(PrimitiveType::TRIANGLES)
    , boundVBO(0), boundIBO(0), boundStride(0), boundPipelineId(0)
    , vaoDirty(false)
{
}

GLRenderContext::~GLRenderContext() {
    for (auto& pair : vaoCache) {
        glDeleteVertexArrays(1, &pair.second);
    }
}

void GLRenderContext::resetCachedState() {
    currentPipeline = nullptr;
    boundVBO = 0;
    boundIBO = 0;
    boundStride = 0;
    boundPipelineId = 0;
    vaoDirty = true;
}

bool GLRenderContext::beginFrame() {
    return true;
}

void GLRenderContext::endFrame() {
}

void GLRenderContext::beginRenderPass() {
}

void GLRenderContext::endRenderPass() {
}

void GLRenderContext::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderContext::setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glViewport(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
}

void GLRenderContext::setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glScissor(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
}

void GLRenderContext::bindPipeline(IPipeline* pipeline) {
    auto* glPipeline = static_cast<GLPipeline*>(pipeline);
    if (currentPipeline != pipeline) {
        glPipeline->bind();
        currentPipeline = pipeline;
        currentPrimitive = glPipeline->getDesc().primitive;
        boundPipelineId = glPipeline->getId();
        vaoDirty = true;
    }
}

void GLRenderContext::bindVertexBuffer(IBuffer* buffer, uint32_t stride) {
    auto* glBuffer = static_cast<GLBuffer*>(buffer);
    GLuint handle = glBuffer->getHandle();
    if (boundVBO != handle || boundStride != stride) {
        boundVBO = handle;
        boundStride = stride;
        vaoDirty = true;
    }
}

void GLRenderContext::bindIndexBuffer(IBuffer* buffer) {
    GLuint handle = buffer ? static_cast<GLBuffer*>(buffer)->getHandle() : 0;
    if (boundIBO != handle) {
        boundIBO = handle;
        vaoDirty = true;
    }
}

void GLRenderContext::bindTexture(uint32_t slot, ITexture* texture) {
    if (texture) {
        static_cast<GLTexture*>(texture)->bind(slot);
    }
}

GLuint GLRenderContext::getOrCreateVAO() {
    if (!vaoDirty) {
        GLuint vao = 0;
        for (auto& pair : vaoCache) {
            if (pair.first.vbo == boundVBO && pair.first.ibo == boundIBO &&
                pair.first.pipelineId == boundPipelineId && pair.first.stride == boundStride) {
                vao = pair.second;
                break;
            }
        }
        if (vao) {
            glBindVertexArray(vao);
            return vao;
        }
    }

    VAOCacheKey key{boundVBO, boundIBO, boundPipelineId, boundStride};

    // Delete old VAO for this key if it exists
    auto it = vaoCache.find(key);
    if (it != vaoCache.end()) {
        glDeleteVertexArrays(1, &it->second);
        vaoCache.erase(it);
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, boundVBO);
    if (boundIBO) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boundIBO);
    }

    // Set up vertex attributes from the pipeline descriptor
    if (currentPipeline) {
        auto* glPipeline = static_cast<GLPipeline*>(currentPipeline);
        const auto& attribs = glPipeline->getDesc().vertexAttributes;
        for (const auto& attr : attribs) {
            glEnableVertexAttribArray(attr.location);
            glVertexAttribPointer(attr.location, attr.size, GL_FLOAT,
                                  attr.normalized ? GL_TRUE : GL_FALSE,
                                  boundStride, (void*)(uint64_t)attr.offset);
        }
    } else {
        // Default: position (2f) + texcoord (2f)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, boundStride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, boundStride, (void*)(2 * sizeof(float)));
    }

    glBindVertexArray(0);
    vaoCache[key] = vao;
    vaoDirty = false;

    glBindVertexArray(vao);
    return vao;
}

void GLRenderContext::draw(uint32_t vertexCount, uint32_t instanceCount) {
    getOrCreateVAO();
    if (instanceCount > 1) {
        glDrawArraysInstanced(glPrimitive(), 0, vertexCount, instanceCount);
    } else {
        glDrawArrays(glPrimitive(), 0, vertexCount);
    }
    glBindVertexArray(0);
}

void GLRenderContext::drawIndexed(uint32_t indexCount, uint32_t instanceCount) {
    getOrCreateVAO();
    if (instanceCount > 1) {
        glDrawElementsInstanced(glPrimitive(), indexCount, GL_UNSIGNED_INT, nullptr, instanceCount);
    } else {
        glDrawElements(glPrimitive(), indexCount, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

GLenum GLRenderContext::glPrimitive() const {
    return toGLPrimitive(currentPrimitive);
}
