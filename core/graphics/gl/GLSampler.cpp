#include "GLSampler.h"

static GLenum glFilter(TextureFilter f) {
    switch (f) {
        case TextureFilter::NEAREST: return GL_NEAREST;
        case TextureFilter::LINEAR:  return GL_LINEAR;
    }
    return GL_LINEAR;
}

static GLenum glWrap(TextureWrap w) {
    switch (w) {
        case TextureWrap::REPEAT:          return GL_REPEAT;
        case TextureWrap::CLAMP:           return GL_CLAMP_TO_EDGE;
        case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
    }
    return GL_CLAMP_TO_EDGE;
}

GLSampler::GLSampler(const SamplerDesc& d)
    : handle(0), desc(d)
{
    glGenSamplers(1, &handle);
    glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, glFilter(desc.min));
    glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, glFilter(desc.mag));
    glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, glWrap(desc.wrapU));
    glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, glWrap(desc.wrapV));
}

GLSampler::~GLSampler() {
    if (handle) glDeleteSamplers(1, &handle);
}

void GLSampler::bind(uint32_t slot) const {
    glBindSampler(slot, handle);
}
