#pragma once
#include <graphics/renderer/Buffer.h>
#include <glad/glad.h>

class GLBuffer : public IBuffer {
public:
    GLBuffer(const BufferDesc& desc);
    ~GLBuffer() override;

    void upload(const void* data, uint32_t size, uint32_t offset = 0) override;
    void bind() const override;
    uint32_t getSize() const override { return size; }
    BufferType getType() const override { return type; }

    GLuint getHandle() const { return handle; }
    GLenum getTarget() const;

private:
    GLuint handle;
    uint32_t size;
    BufferType type;
    BufferUsage usage;
};
