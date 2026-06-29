#pragma once
#include <graphics/renderer/Texture.h>
#include <glad/glad.h>

class GLTexture : public ITexture {
public:
    GLTexture(const TextureDesc& desc);
    ~GLTexture() override;

    void upload(const void* pixels) override;
    void bind(uint32_t slot = 0) const override;
    uint32_t getWidth() const override { return width; }
    uint32_t getHeight() const override { return height; }
    TextureFormat getFormat() const override { return format; }

    GLuint getHandle() const { return handle; }

private:
    GLuint handle;
    uint32_t width;
    uint32_t height;
    TextureFormat format;

    GLenum glInternalFormat() const;
    GLenum glFormat() const;
    GLenum glType() const;
};
