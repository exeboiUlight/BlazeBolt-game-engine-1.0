#pragma once
#include <subject/sprite/generalSprite.hpp>
#include <graphics/animatedTexture2D.hpp>

namespace BlazeBolt {
    struct AnimatedSprite2D {
    private:
        const AnimatedTexture2D *texture;
        Matrix3x3 modelMatrix;
        Vector4 color;
        Vector2 position, scale, origin;
        float rotation;

        float playbackSpeed, elapsedTime;
        uint32_t currentFrame;
        bool looping, playing;

        bool visible;
        void updateModelMatrix();
    public:
        AnimatedSprite2D();
        ~AnimatedSprite2D() = default;

        void update(float deltaTime);

        void setPlaybackSpeed(float speed);
        float getPlaybackSpeed() const;
        void setLooping(bool looping);
        bool isLooping() const;

        void setFrame(uint32_t frame);
        uint32_t getCurrentFrame() const;
        uint32_t getNumFrames() const;

        void play();
        void pause();
        void stop();
        void restart();
        bool isPlaying() const;

        void setTexture(const AnimatedTexture2D &texture);
        void setPosition(float x, float y);
        void setPosition(const Vector2 &position);
        const Vector2 &getPosition() const;

        void setScale(float width, float height);
        void setScale(const Vector2 &scale);
        const Vector2 &getScale() const;

        void setOrigin(float x, float y);
        void setOrigin(const Vector2 &origin);
        const Vector2 &getOrigin() const;
        void setRotation(float degrees);
        float getRotation() const;

        void setColor(float r, float g, float b, float a);
        void setColor(const Vector4 &color);
        Vector4 getColor() const;
        void setVisible(bool visible);
        bool isVisible() const;

        void draw(const SpriteShader2D &shader, const SpriteMesh &mesh, const Matrix3x3 &projectionViewMatrix) const;
    };
}