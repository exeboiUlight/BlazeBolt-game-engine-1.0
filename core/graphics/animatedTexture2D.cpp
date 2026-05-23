#include "animatedTexture2D.hpp"

namespace BlazeBolt {
    AnimatedTexture2D::AnimatedTexture2D(GL::Texture2D &&texture, uint32_t numFrames) : texture(std::move(texture)), numFrames(numFrames) {}
    AnimatedTexture2D::AnimatedTexture2D(AnimatedTexture2D &&other) noexcept : texture(std::move(other.texture)), numFrames(other.numFrames) { other.numFrames = 0; }
    AnimatedTexture2D &AnimatedTexture2D::operator=(AnimatedTexture2D &&other) noexcept {
        if (this == &other) { return *this; }
        this->texture = std::move(other.texture);
        this->numFrames = other.numFrames;
        other.numFrames = 0;
        return *this;
    }

    const GL::Texture2D &AnimatedTexture2D::getGL() const { return this->texture; }
    uint32_t AnimatedTexture2D::getNumFrames() const { return this->numFrames; }
}