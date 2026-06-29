#include "GLTexture.h"

GLTexture::GLTexture(const TextureDesc& desc)
    : handle(0), width(desc.width), height(desc.height), format(desc.format)
{
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat(), width, height, 0, glFormat(), glType(), nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLTexture::~GLTexture() {
    if (handle) glDeleteTextures(1, &handle);
}

void GLTexture::upload(const void* pixels) {
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat(), width, height, 0, glFormat(), glType(), pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, handle);
}

GLenum GLTexture::glInternalFormat() const {
    switch (format) {
        case TextureFormat::R8: return GL_R8;
        case TextureFormat::RG8: return GL_RG8;
        case TextureFormat::RGB8: return GL_RGB;
        case TextureFormat::RGBA8: return GL_RGBA;
        case TextureFormat::SRGB8_ALPHA8: return GL_SRGB_ALPHA;
    }
    return GL_RGBA;
}

GLenum GLTexture::glFormat() const {
    switch (format) {
        case TextureFormat::R8: return GL_RED;
        case TextureFormat::RG8: return GL_RG;
        case TextureFormat::RGB8: case TextureFormat::SRGB8_ALPHA8: return GL_RGB;
        case TextureFormat::RGBA8: return GL_RGBA;
    }
    return GL_RGBA;
}

GLenum GLTexture::glType() const {
    return GL_UNSIGNED_BYTE;
}
