#include "animatedTexture2D.hpp"

namespace BlazeBolt {
    AnimatedTexture2D::AnimatedTexture2D(ITexture* tex, uint32_t frames) : texture(tex), numFrames(frames) {}
    AnimatedTexture2D::AnimatedTexture2D(AnimatedTexture2D &&other) noexcept : texture(other.texture), numFrames(other.numFrames) { other.texture = nullptr; other.numFrames = 0; }
    AnimatedTexture2D &AnimatedTexture2D::operator=(AnimatedTexture2D &&other) noexcept {
        if (this == &other) { return *this; }
        this->texture = other.texture;
        this->numFrames = other.numFrames;
        other.texture = nullptr;
        other.numFrames = 0;
        return *this;
    }

    ITexture* AnimatedTexture2D::getTexture() const { return this->texture; }
    uint32_t AnimatedTexture2D::getNumFrames() const { return this->numFrames; }
}
