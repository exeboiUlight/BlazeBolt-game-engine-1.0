#include "GLBuffer.h"

static GLenum glBufferType(BufferType t) {
    switch (t) {
        case BufferType::VERTEX: return GL_ARRAY_BUFFER;
        case BufferType::INDEX:  return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::UNIFORM: return GL_UNIFORM_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

static GLenum glBufferUsage(BufferUsage u) {
    switch (u) {
        case BufferUsage::STATIC:  return GL_STATIC_DRAW;
        case BufferUsage::DYNAMIC: return GL_DYNAMIC_DRAW;
        case BufferUsage::STREAM:  return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}

GLBuffer::GLBuffer(const BufferDesc& desc)
    : handle(0), size(desc.size), type(desc.type), usage(desc.usage)
{
    glGenBuffers(1, &handle);
    GLenum target = getTarget();
    glBindBuffer(target, handle);
    glBufferData(target, size, nullptr, glBufferUsage(usage));
    glBindBuffer(target, 0);
}

GLBuffer::~GLBuffer() {
    if (handle) glDeleteBuffers(1, &handle);
}

void GLBuffer::upload(const void* data, uint32_t dataSize, uint32_t offset) {
    GLenum target = getTarget();
    glBindBuffer(target, handle);
    if (offset == 0 && dataSize == size) {
        glBufferData(target, size, data, glBufferUsage(usage));
    } else {
        glBufferSubData(target, offset, dataSize, data);
    }
    glBindBuffer(target, 0);
}

void GLBuffer::bind() const {
    glBindBuffer(getTarget(), handle);
}

GLenum GLBuffer::getTarget() const {
    return glBufferType(type);
}
