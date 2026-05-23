#include "particle2D.hpp"
#include <cstdlib>
#include <cmath>

static float randFloat(float min, float max) {
    return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}

ParticleSystem2D::ParticleSystem2D()
    : position(0, 0)
    , texture(nullptr)
    , emissionRate(50.0f)
    , emissionAccumulator(0.0f)
    , active(true)
    , visible(true)
    , maxParticles(500)
    , lifetimeMin(0.5f), lifetimeMax(2.0f)
    , speedMin(50.0f), speedMax(150.0f)
    , sizeMin(4.0f), sizeMax(16.0f)
    , endSizeMin(1.0f), endSizeMax(4.0f)
    , angleMin(0.0f), angleMax(360.0f)
    , startColor(1.0f, 1.0f, 1.0f, 1.0f)
    , endColor(1.0f, 1.0f, 1.0f, 0.0f)
    , particleRotationSpeed(0.0f)
{
}

void ParticleSystem2D::setPosition(const Vector2& pos) {
    position = pos;
}

const Vector2& ParticleSystem2D::getPosition() const {
    return position;
}

void ParticleSystem2D::setTexture(const GL::Texture2D& tex) {
    texture = &tex;
}

void ParticleSystem2D::setEmissionRate(float rate) {
    emissionRate = (rate < 0.0f) ? 0.0f : rate;
}

float ParticleSystem2D::getEmissionRate() const {
    return emissionRate;
}

void ParticleSystem2D::setLifetime(float min, float max) {
    lifetimeMin = min;
    lifetimeMax = (max < min) ? min : max;
}

void ParticleSystem2D::setSpeed(float min, float max) {
    speedMin = min;
    speedMax = (max < min) ? min : max;
}

void ParticleSystem2D::setSize(float min, float max) {
    sizeMin = min;
    sizeMax = (max < min) ? min : max;
}

void ParticleSystem2D::setEndSize(float min, float max) {
    endSizeMin = min;
    endSizeMax = (max < min) ? min : max;
}

void ParticleSystem2D::setColor(const Vector4& start, const Vector4& end) {
    startColor = start;
    endColor = end;
}

void ParticleSystem2D::setDirection(float minAngle, float maxAngle) {
    angleMin = minAngle;
    angleMax = maxAngle;
}

void ParticleSystem2D::setRotationSpeed(float speed) {
    particleRotationSpeed = speed;
}

void ParticleSystem2D::setActive(bool a) {
    active = a;
}

bool ParticleSystem2D::isActive() const {
    return active;
}

void ParticleSystem2D::setVisible(bool v) {
    visible = v;
}

bool ParticleSystem2D::isVisible() const {
    return visible;
}

void ParticleSystem2D::setMaxParticles(int max) {
    maxParticles = (max < 1) ? 1 : max;
    if (static_cast<int>(particles.size()) > maxParticles) {
        particles.resize(maxParticles);
    }
}

void ParticleSystem2D::clear() {
    particles.clear();
}

int ParticleSystem2D::getParticleCount() const {
    return static_cast<int>(particles.size());
}

void ParticleSystem2D::spawnParticle() {
    if (static_cast<int>(particles.size()) >= maxParticles) return;

    Particle p;
    float angleDeg = randFloat(angleMin, angleMax);
    float angleRad = angleDeg * 3.14159265f / 180.0f;
    float speed = randFloat(speedMin, speedMax);

    p.position = position;
    p.velocity = Vector2(std::cos(angleRad) * speed, std::sin(angleRad) * speed);
    p.color = startColor;
    p.endColor = endColor;
    p.size = randFloat(sizeMin, sizeMax);
    p.endSize = randFloat(endSizeMin, endSizeMax);
    p.rotation = randFloat(0.0f, 360.0f);
    p.rotationSpeed = particleRotationSpeed;
    p.lifetime = randFloat(lifetimeMin, lifetimeMax);
    p.maxLifetime = p.lifetime;

    particles.push_back(p);
}

void ParticleSystem2D::emit(int count) {
    for (int i = 0; i < count; ++i) {
        spawnParticle();
    }
}

void ParticleSystem2D::update(float dt) {
    if (!active) return;

    // Spawn new particles
    if (emissionRate > 0.0f) {
        emissionAccumulator += emissionRate * dt;
        while (emissionAccumulator >= 1.0f) {
            spawnParticle();
            emissionAccumulator -= 1.0f;
        }
    }

    // Update existing particles
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime <= 0.0f) {
            it = particles.erase(it);
            continue;
        }

        float t = 1.0f - it->lifetime / it->maxLifetime;

        it->position.x += it->velocity.x * dt;
        it->position.y += it->velocity.y * dt;
        it->rotation += it->rotationSpeed * dt;

        it->color.x = startColor.x + (endColor.x - startColor.x) * t;
        it->color.y = startColor.y + (endColor.y - startColor.y) * t;
        it->color.z = startColor.z + (endColor.z - startColor.z) * t;
        it->color.w = startColor.w + (endColor.w - startColor.w) * t;

        it->size = it->endSize + (it->size - it->endSize) * (1.0f - t);

        ++it;
    }
}

void ParticleSystem2D::draw(const GL::Texture2D& defaultTexture, const BlazeBolt::SpriteShader2D& shader, const BlazeBolt::SpriteMesh& mesh, const Matrix3x3& projectionViewMatrix) const {
    if (!visible || particles.empty()) return;

    shader.bind();

    const GL::Texture2D* activeTex = texture ? texture : &defaultTexture;
    GL::setActiveTextureUnit(0);
    activeTex->bind();

    for (const auto& p : particles) {
        Matrix3x3 model = Matrix3x3::translation(p.position.x, p.position.y)
                        * Matrix3x3::rotation(p.rotation)
                        * Matrix3x3::scale(p.size, p.size);
        Matrix3x3 mvp = projectionViewMatrix * model;

        shader.setMVPMatrix(mvp);
        shader.setColor(p.color);
        shader.setTextureRect(Vector4(0.0f, 0.0f, 1.0f, 1.0f));

        mesh.draw();
    }
}
