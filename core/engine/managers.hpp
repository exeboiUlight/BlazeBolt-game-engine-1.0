#pragma once
#include <graphics/gl.hpp>
#include <subject/text2D.hpp>
#include <string>
#include <unordered_map>

namespace BlazeBolt {
    struct TextureManager {
    private:
        std::unordered_map<std::string, GL::Texture2D> cache;
        const GL::Texture2D default2D;
    public:
        TextureManager();
        ~TextureManager() = default;
        GL::Texture2D &create2D(const std::string &path);
        GL::Texture2D *loadFromFile2D(const std::string &path);
        GL::Texture2D *findTexture2D(const std::string &path);
        const GL::Texture2D &getDefault2D() const;
    };
    struct FontManager {
    private:
        std::unordered_map<std::string, BlazeBolt::Font> cache;
        BlazeBolt::FreeType freeType;
    public:
        FontManager();
        ~FontManager() = default;
        BlazeBolt::Font &loadFromFile(const std::string &path);
        BlazeBolt::Font *findFont(const std::string &path);
    };
}