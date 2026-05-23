#pragma once
#include <graphics/gl.hpp>

namespace BlazeBolt {
    struct AnimatedTexture2D {
    private:
        GL::Texture2D texture;
        uint32_t numFrames;
    public:
        AnimatedTexture2D(GL::Texture2D &&texture, uint32_t numFrames);
        ~AnimatedTexture2D() = default;
        AnimatedTexture2D(const AnimatedTexture2D&) = delete;
        AnimatedTexture2D &operator=(const AnimatedTexture2D&) = delete;
        AnimatedTexture2D(AnimatedTexture2D&& other) noexcept;
        AnimatedTexture2D &operator=(AnimatedTexture2D&& other) noexcept;

        const GL::Texture2D &getGL() const;
        uint32_t getNumFrames() const;
    };
}