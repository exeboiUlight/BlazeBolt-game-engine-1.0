#pragma once
#include <graphics/gl.hpp>
#include <graphics/renderer/Texture.h>

namespace BlazeBolt {
    struct AnimatedTexture2D {
    private:
        ITexture* texture = nullptr;
        uint32_t numFrames;
    public:
        AnimatedTexture2D(ITexture* texture, uint32_t numFrames);
        ~AnimatedTexture2D() = default;
        AnimatedTexture2D(const AnimatedTexture2D&) = delete;
        AnimatedTexture2D &operator=(const AnimatedTexture2D&) = delete;
        AnimatedTexture2D(AnimatedTexture2D&& other) noexcept;
        AnimatedTexture2D &operator=(AnimatedTexture2D&& other) noexcept;

        ITexture* getTexture() const;
        uint32_t getNumFrames() const;
    };
}