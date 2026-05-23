#pragma once
#include <vector>
#include <graphics/gl.hpp>
#include <graphics/quad.hpp>
#include <subject/sprite2D.hpp>
#include <utils/math/vector.h>

class ParticleSystem2D {
public:
    struct Particle {
        Vector2 position;
        Vector2 velocity;
        Vector4 color;
        Vector4 endColor;
        float size;
        float endSize;
        float rotation;
        float rotationSpeed;
        float lifetime;
        float maxLifetime;
    };

    ParticleSystem2D();
    ~ParticleSystem2D() = default;

    void setPosition(const Vector2& pos);
    const Vector2& getPosition() const;

    void setTexture(const GL::Texture2D& tex);

    void setEmissionRate(float rate);
    float getEmissionRate() const;

    void setLifetime(float min, float max);
    void setSpeed(float min, float max);
    void setSize(float min, float max);
    void setEndSize(float min, float max);
    void setColor(const Vector4& start, const Vector4& end);
    void setDirection(float minAngle, float maxAngle);
    void setRotationSpeed(float speed);

    void setActive(bool a);
    bool isActive() const;
    void setVisible(bool v);
    bool isVisible() const;
    void setMaxParticles(int max);

    void emit(int count);
    void clear();

    void update(float dt);
    void draw(const GL::Texture2D& defaultTexture, const BlazeBolt::SpriteShader2D& shader, const BlazeBolt::SpriteMesh2D& mesh, const Matrix3x3& projectionViewMatrix) const;

    int getParticleCount() const;

private:
    std::vector<Particle> particles;

    Vector2 position;
    const GL::Texture2D* texture;

    float emissionRate;
    float emissionAccumulator;
    bool active;
    bool visible;
    int maxParticles;

    float lifetimeMin, lifetimeMax;
    float speedMin, speedMax;
    float sizeMin, sizeMax;
    float endSizeMin, endSizeMax;
    float angleMin, angleMax;
    Vector4 startColor;
    Vector4 endColor;
    float particleRotationSpeed;

    void spawnParticle();
};
