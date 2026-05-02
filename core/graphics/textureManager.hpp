#pragma once
#include <graphics/gl.hpp>
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
        const GL::Texture2D &create2D(const std::string &path);
        const GL::Texture2D *loadFromFile2D(const std::string &path);
        const GL::Texture2D &getDefault2D() const;
    };
}