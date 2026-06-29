#pragma once
#include <graphics/gl.hpp>
#include <graphics/renderer/Texture.h>
#include <graphics/renderer/RenderDevice.h>
#include <graphics/animatedTexture2D.hpp>
#include <subject/text2D.hpp>
#include <string>
#include <unordered_map>

namespace BlazeBolt {
    struct TextureManager {
    private:
        std::unordered_map<std::string, ITexture*> cache;
        std::unordered_map<std::string, GL::Texture2D> glCache;
        std::unordered_map<std::string, AnimatedTexture2D> animatedCache;
        GL::Texture2D defaultGL2D;
        ITexture* defaultRHI2D = nullptr;
        IRenderDevice* device = nullptr;
    public:
        TextureManager();
        explicit TextureManager(IRenderDevice* renderDevice);
        ~TextureManager();
        void setDevice(IRenderDevice* renderDevice);
        ITexture* create2D(const std::string &path);
        ITexture* loadFromFile2D(const std::string &path);
        const GL::Texture2D* loadGLTexture(const std::string &path);
        ITexture* findTexture2D(const std::string &path);
        AnimatedTexture2D *loadFromFileAnimated2D(const std::string &path);
        AnimatedTexture2D *findAnimated2D(const std::string &path);
        const GL::Texture2D& getDefault2D() const;
        ITexture* getDefaultRHI() const;
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