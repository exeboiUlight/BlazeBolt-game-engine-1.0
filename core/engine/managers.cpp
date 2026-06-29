#include "managers.hpp"
#include <graphics/gl/GLTexture.h>
#include <stb_image/stb_image.h>
#include <fstream>
#include <vector>
#include <gif_load.h>

namespace {
    struct GifDecodeContext {
        int canvasWidth;
        int canvasHeight;
        std::vector<uint8_t> canvas;
        std::vector<uint8_t> savedCanvas;
        std::vector<std::vector<uint8_t>> frames;

        GifDecodeContext() : canvasWidth(0), canvasHeight(0), canvas(), savedCanvas(), frames() {}
        ~GifDecodeContext() = default;
    };

    void gifFrameWriter(void *userData, GIF_WHDR *whdr) {
        if (userData == nullptr || whdr == nullptr) { return; }
        GifDecodeContext *context = static_cast<GifDecodeContext*>(userData);

        const int frameWidth = static_cast<int>(whdr->frxd);
        const int frameHeight = static_cast<int>(whdr->fryd);
        const int frameX = static_cast<int>(whdr->frxo);
        const int frameY = static_cast<int>(whdr->fryo);
        if (frameWidth <= 0 || frameHeight <= 0 || whdr->bptr == nullptr || whdr->cpal == nullptr) { return; }

        if (context->canvasWidth == 0 || context->canvasHeight == 0) {
            const int decodedWidth = static_cast<int>(whdr->xdim);
            const int decodedHeight = static_cast<int>(whdr->ydim);
            context->canvasWidth = (decodedWidth > 0) ? decodedWidth : frameWidth;
            context->canvasHeight = (decodedHeight > 0) ? decodedHeight : frameHeight;
            if (context->canvasWidth <= 0 || context->canvasHeight <= 0) { return; }

            const size_t canvasSize = static_cast<size_t>(context->canvasWidth * context->canvasHeight * 4);
            context->canvas.assign(canvasSize, 0);
        }

        if (whdr->mode == GIF_PREV) {
            context->savedCanvas = context->canvas;
        }

        const bool hasTransparency = whdr->tran >= 0;
        const uint8_t transparentIndex = static_cast<uint8_t>(whdr->tran);

        for (int y = 0; y < frameHeight; y++) {
            const int dstY = frameY + y;
            if (dstY < 0 || dstY >= context->canvasHeight) { continue; }

            for (int x = 0; x < frameWidth; x++) {
                const int dstX = frameX + x;
                if (dstX < 0 || dstX >= context->canvasWidth) { continue; }

                const int srcIndex = y * frameWidth + x;
                const uint8_t paletteIndex = whdr->bptr[srcIndex];
                if (hasTransparency && paletteIndex == transparentIndex) { continue; }

                const int dstIndex = (dstY * context->canvasWidth + dstX) * 4;
                context->canvas[dstIndex + 0] = whdr->cpal[paletteIndex].R;
                context->canvas[dstIndex + 1] = whdr->cpal[paletteIndex].G;
                context->canvas[dstIndex + 2] = whdr->cpal[paletteIndex].B;
                context->canvas[dstIndex + 3] = 255;
            }
        }

        context->frames.push_back(context->canvas);

        if (whdr->mode == GIF_BKGD) {
            const int clearX0 = std::max(0, frameX);
            const int clearY0 = std::max(0, frameY);
            const int clearX1 = std::min(context->canvasWidth, frameX + frameWidth);
            const int clearY1 = std::min(context->canvasHeight, frameY + frameHeight);

            for (int y = clearY0; y < clearY1; y++) {
                for (int x = clearX0; x < clearX1; x++) {
                    const int index = (y * context->canvasWidth + x) * 4;
                    context->canvas[index + 0] = 0;
                    context->canvas[index + 1] = 0;
                    context->canvas[index + 2] = 0;
                    context->canvas[index + 3] = 0;
                }
            }
        } else if (whdr->mode == GIF_PREV && context->savedCanvas.size() == context->canvas.size()) {
            context->canvas = context->savedCanvas;
        }
    }
}

namespace BlazeBolt {
    TextureManager::TextureManager() : cache(), animatedCache(), defaultGL2D(), defaultRHI2D(nullptr), device(nullptr) {
        printf("TextureManager initialized (no device)\n");
    }
    TextureManager::TextureManager(IRenderDevice* renderDevice)
        : cache(), animatedCache(), defaultGL2D(), defaultRHI2D(nullptr), device(renderDevice) {
        printf("TextureManager initialized\n");
        if (device) {
            TextureDesc desc = { 1, 1, TextureFormat::RGBA8 };
            defaultRHI2D = device->createTexture(desc);
            unsigned char whitePixel[] = { 255, 255, 255, 255 };
            defaultRHI2D->upload(whitePixel);
        }
    }
    TextureManager::~TextureManager() {
        for (auto& [path, tex] : cache) {
            if (device && tex) device->destroyTexture(tex);
        }
        if (device && defaultRHI2D) device->destroyTexture(defaultRHI2D);
    }
    void TextureManager::setDevice(IRenderDevice* renderDevice) {
        this->device = renderDevice;
        if (device && !defaultRHI2D) {
            TextureDesc desc = { 1, 1, TextureFormat::RGBA8 };
            defaultRHI2D = device->createTexture(desc);
            unsigned char whitePixel[] = { 255, 255, 255, 255 };
            defaultRHI2D->upload(whitePixel);
        }
    }
    ITexture* TextureManager::create2D(const std::string &path) {
        if (!device) return nullptr;
        auto it = this->cache.find(path);
        if (it != this->cache.end()) { return it->second; }
        TextureDesc desc = { 1, 1, TextureFormat::RGBA8 };
        ITexture* tex = device->createTexture(desc);
        this->cache[path] = tex;
        return tex;
    }
    const GL::Texture2D& TextureManager::getDefault2D() const {
        return this->defaultGL2D;
    }
    ITexture* TextureManager::getDefaultRHI() const {
        return this->defaultRHI2D;
    }
    ITexture* TextureManager::loadFromFile2D(const std::string &path) {
        if (!device) return nullptr;
        auto it = this->cache.find(path);
        if (it != this->cache.end()) { return it->second; }

        stbi_set_flip_vertically_on_load(false);

        int width, height, channels;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            printf("Failed to load texture: %s\n", path.c_str());
            return nullptr;
        }

        TextureFormat format;
        if (channels == STBI_grey) {
            format = TextureFormat::R8;
            printf("Loaded texture (grayscale): %s (%dx%d)\n", path.c_str(), width, height);
        } else if (channels == STBI_rgb) {
            format = TextureFormat::RGB8;
            printf("Loaded texture (RGB): %s (%dx%d)\n", path.c_str(), width, height);
        } else if (channels == STBI_rgb_alpha) {
            format = TextureFormat::RGBA8;
            printf("Loaded texture (RGBA): %s (%dx%d)\n", path.c_str(), width, height);
        } else {
            printf("Failed to load texture. It has unknown channels (%d): %s (%dx%d)\n", channels, path.c_str(), width, height);
            stbi_image_free(data);
            return nullptr;
        }

        TextureDesc desc = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), format };
        ITexture* tex = device->createTexture(desc);
        tex->upload(data);
        stbi_image_free(data);

        this->cache[path] = tex;
        return tex;
    }
    const GL::Texture2D* TextureManager::loadGLTexture(const std::string &path) {
        auto it = this->glCache.find(path);
        if (it != this->glCache.end()) { return &it->second; }

        stbi_set_flip_vertically_on_load(false);

        int width, height, channels;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            printf("Failed to load texture: %s\n", path.c_str());
            return nullptr;
        }

        GLenum fmt = (channels == STBI_rgb_alpha) ? GL_RGBA : (channels == STBI_rgb) ? GL_RGB : GL_RED;

        GL::Texture2D tex;
        tex.bind();
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(fmt), width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(data);

        auto result = this->glCache.emplace(path, std::move(tex));
        return &result.first->second;
    }

    ITexture* TextureManager::findTexture2D(const std::string &path) {
        auto it = this->cache.find(path);
        if (it != this->cache.end()) {
            return it->second;
        }
        return nullptr;
    }

    AnimatedTexture2D *TextureManager::loadFromFileAnimated2D(const std::string &path) {
        const std::unordered_map<std::string, BlazeBolt::AnimatedTexture2D>::iterator it = this->animatedCache.find(path);
        if (it != this->animatedCache.end()) { return &it->second; }

        std::ifstream file = std::ifstream(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::fprintf(stderr, "AnimatedTexture2D: failed to open GIF file: %s\n", path.c_str());
            return nullptr;
        }

        const std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) {
            std::fprintf(stderr, "AnimatedTexture2D: GIF file is empty: %s\n", path.c_str());
            return nullptr;
        }

        std::vector<uint8_t> gifData(static_cast<size_t>(fileSize));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(gifData.data()), fileSize);
        if (!file.good() && !file.eof()) {
            std::fprintf(stderr, "AnimatedTexture2D: failed to read GIF file: %s\n", path.c_str());
            return nullptr;
        }

        GifDecodeContext decodeContext;
        const long loadedFrames = GIF_Load(gifData.data(), static_cast<long>(gifData.size()), gifFrameWriter, nullptr, &decodeContext, 0);
        if (loadedFrames <= 0 || decodeContext.frames.empty()) {
            std::fprintf(stderr, "AnimatedTexture2D: GIF_Load failed for: %s (result=%ld)\n", path.c_str(), loadedFrames);
            return nullptr;
        }

        uint32_t numFrames = static_cast<uint32_t>(decodeContext.frames.size());
        
        const uint32_t frameWidth = static_cast<uint32_t>(decodeContext.canvasWidth);
        const uint32_t frameHeight = static_cast<uint32_t>(decodeContext.canvasHeight);
        if (frameWidth == 0 || frameHeight == 0) {
            return nullptr;
        }

        const uint32_t atlasWidth = frameWidth * numFrames;
        const uint32_t atlasHeight = frameHeight;
        std::vector<uint8_t> atlasPixels(static_cast<size_t>(atlasWidth) * static_cast<size_t>(atlasHeight) * 4, 0);

        for (uint32_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
            const std::vector<uint8_t> &framePixels = decodeContext.frames[frameIndex];
            if (framePixels.size() < static_cast<size_t>(frameWidth) * static_cast<size_t>(frameHeight) * 4) {
                return nullptr;
            }

            for (uint32_t row = 0; row < frameHeight; ++row) {
                const size_t srcOffset = static_cast<size_t>(row) * static_cast<size_t>(frameWidth) * 4;
                const size_t dstOffset = (static_cast<size_t>(row) * static_cast<size_t>(atlasWidth) + static_cast<size_t>(frameIndex) * static_cast<size_t>(frameWidth)) * 4;
                std::memcpy(atlasPixels.data() + dstOffset, framePixels.data() + srcOffset, static_cast<size_t>(frameWidth) * 4);
            }
        }

        if (!device) return nullptr;
        TextureDesc atlasDesc = { atlasWidth, atlasHeight, TextureFormat::RGBA8 };
        ITexture* atlasTex = device->createTexture(atlasDesc);
        atlasTex->upload(atlasPixels.data());
        AnimatedTexture2D &texture = this->animatedCache.try_emplace(path, atlasTex, numFrames).first->second;
        return &texture;
    }
    AnimatedTexture2D *TextureManager::findAnimated2D(const std::string &path) {
        std::unordered_map<std::string, AnimatedTexture2D>::iterator it = this->animatedCache.find(path);
        if (it != this->animatedCache.end()) { return &it->second; }
        return nullptr;
    }

    FontManager::FontManager() : cache() {
        printf("FontManager initialized\n");
    }
    BlazeBolt::Font &FontManager::loadFromFile(const std::string &path) {
        const auto [it, inserted] = this->cache.try_emplace(path, this->freeType, path);
        if (!inserted) { return it->second; }
        return it->second;
    }
    BlazeBolt::Font *FontManager::findFont(const std::string &path) {
        std::unordered_map<std::string, BlazeBolt::Font>::iterator it = this->cache.find(path);
        if (it != this->cache.end()) {
            return &it->second;
        }
        return nullptr;
    }
}