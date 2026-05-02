#include "textureManager.hpp"
#include <stb_image/stb_image.h>

namespace BlazeBolt {
    TextureManager::TextureManager() : cache(), default2D() {
        printf("TextureManager initialized\n");
        this->default2D.bind();
        unsigned char whitePixel[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    const GL::Texture2D &TextureManager::create2D(const std::string &path) {
        const auto [it, inserted] = this->cache.try_emplace(path);
        if (!inserted) {
            printf("Texture already exists: %s\n", path.c_str());
            return it->second;
        }
        return it->second;
    }
    const GL::Texture2D &TextureManager::getDefault2D() const {
        return this->default2D;
    }
    const GL::Texture2D *TextureManager::loadFromFile2D(const std::string &path) {
        const auto it = this->cache.find(path);
        if (it != this->cache.end()) {
            printf("Texture already exists: %s\n", path.c_str());
            return &it->second;
        }

        stbi_set_flip_vertically_on_load(true);

        int width, height, channels;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            printf("Failed to load texture: %s\n", path.c_str());
            return nullptr;
        }

        GLenum format, internalFormat;
        if (channels == STBI_grey) {
            format = internalFormat = GL_RED;
            printf("Loaded texture (grayscale): %s (%dx%d)\n", path.c_str(), width, height);
        } else if (channels == STBI_rgb) {
            format = internalFormat = GL_RGB;
            printf("Loaded texture (RGB): %s (%dx%d)\n", path.c_str(), width, height);
        } else if (channels == STBI_rgb_alpha) {
            format = internalFormat = GL_RGBA;
            printf("Loaded texture (RGBA): %s (%dx%d)\n", path.c_str(), width, height);
        } else {
            printf("Failed to load texture. It has unknown channels (%d): %s (%dx%d)\n", channels, path.c_str(), width, height);
            stbi_image_free(data);
            return nullptr;
        }

        const GL::Texture2D &texture = this->cache.try_emplace(path).first->second;
        texture.bind();
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(data);

        return &texture;
    }
}