#pragma once
#include <graphics/gl.hpp>
#include <graphics/animatedTexture2D.hpp>
#include <subject/text2D.hpp>
#include <string>
#include <unordered_map>

namespace BlazeBolt {
    struct TextureManager {
    private:
        std::unordered_map<std::string, GL::Texture2D> cache;
        std::unordered_map<std::string, AnimatedTexture2D> animatedCache;
        const GL::Texture2D default2D;
    public:
        TextureManager();
        ~TextureManager() = default;
        GL::Texture2D &create2D(const std::string &path);
        GL::Texture2D *loadFromFile2D(const std::string &path);
        GL::Texture2D *findTexture2D(const std::string &path);
        AnimatedTexture2D *loadFromFileAnimated2D(const std::string &path);
        AnimatedTexture2D *findAnimated2D(const std::string &path);
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