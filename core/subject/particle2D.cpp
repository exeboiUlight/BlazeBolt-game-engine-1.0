#include "particle2D.hpp"
#include <cstdlib>
#include <cmath>

static float randFloat(float min, float max) {
    return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}

ParticleSystem2D::ParticleSystem2D()
    : shaderReady(false)
    , vaoReady(false)
    , position(0, 0)
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

    if (emissionRate > 0.0f) {
        emissionAccumulator += emissionRate * dt;
        while (emissionAccumulator >= 1.0f) {
            spawnParticle();
            emissionAccumulator -= 1.0f;
        }
    }

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

void ParticleSystem2D::ensureShader() {
    if (shaderReady) return;

    constexpr GLchar vertexShaderSource[] = R"(
        #version 410
        layout (location = 0) in vec2 a_Position;
        layout (location = 1) in vec4 i_Transform;
        layout (location = 2) in vec4 i_Color;

        layout (location = 0) out vec2 v_TexCoord;
        layout (location = 1) out vec4 v_Color;

        uniform float u_AspectRatio;
        uniform mat3 u_MVPMatrix;
        uniform vec4 u_TexRect;

        void main() {
            vec2 scaled = a_Position * i_Transform.z;
            float rad = radians(i_Transform.w);
            float c = cos(rad);
            float s = sin(rad);
            vec2 rotated = vec2(scaled.x * c - scaled.y * s, scaled.x * s + scaled.y * c);
            vec2 pos = rotated + i_Transform.xy;

            vec3 transformed = u_MVPMatrix * vec3(pos, 1.0);
            gl_Position = vec4(transformed.xy, 0.0, 1.0);
            gl_Position.x /= u_AspectRatio;
            v_TexCoord = a_Position * u_TexRect.zw + u_TexRect.xy;
            v_TexCoord.y = 1.0 - v_TexCoord.y;
            v_Color = i_Color;
        }
    )";

    constexpr GLchar fragmentShaderSource[] = R"(
        #version 410
        layout (location = 0) in vec2 v_TexCoord;
        layout (location = 1) in vec4 v_Color;

        layout (location = 0) out vec4 f_Color;

        uniform sampler2D u_Texture;

        void main() {
            f_Color = texture(u_Texture, v_TexCoord) * v_Color;
        }
    )";

    {
        auto vertexShader = GL::Shader::fromSource(GL_VERTEX_SHADER, vertexShaderSource);
        if (!vertexShader.has_value()) return;

        auto fragmentShader = GL::Shader::fromSource(GL_FRAGMENT_SHADER, fragmentShaderSource);
        if (!fragmentShader.has_value()) return;

        glAttachShader(shaderProgram.get(), vertexShader->get());
        glAttachShader(shaderProgram.get(), fragmentShader->get());
        if (!shaderProgram.tryToLink()) return;
    }

    shaderProgram.bind();
    glUniform1i(glGetUniformLocation(shaderProgram.get(), "u_Texture"), 0);
    shaderReady = true;
}

void ParticleSystem2D::ensureVAO(const BlazeBolt::QuadVertexBufferObject2D& quadVBO) {
    if (vaoReady) return;

    vao.bind();
    quadVBO.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BlazeBolt::QuadVertexBufferObject2D::Vertex), nullptr);

    instanceVBO.bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(4 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    vaoReady = true;
}

void ParticleSystem2D::draw(const GL::Texture2D& defaultTexture, const BlazeBolt::QuadVertexBufferObject2D& quadVBO, float aspectRatio, const Matrix3x3& projectionViewMatrix) {
    if (!visible || particles.empty()) return;

    ensureShader();
    if (!shaderReady) return;
    ensureVAO(quadVBO);

    shaderProgram.bind();
    glUniform1f(glGetUniformLocation(shaderProgram.get(), "u_AspectRatio"), aspectRatio);
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram.get(), "u_MVPMatrix"), 1, GL_FALSE, &projectionViewMatrix.m[0][0]);
    glUniform4f(glGetUniformLocation(shaderProgram.get(), "u_TexRect"), 0.0f, 0.0f, 1.0f, 1.0f);

    const GL::Texture2D* activeTex = texture ? texture : &defaultTexture;
    GL::setActiveTextureUnit(0);
    activeTex->bind();

    struct InstanceData {
        float x, y, size, rotation;
        float r, g, b, a;
    };

    std::vector<InstanceData> instanceData;
    instanceData.reserve(particles.size());
    for (const auto& p : particles) {
        InstanceData id;
        id.x = p.position.x;
        id.y = p.position.y;
        id.size = p.size;
        id.rotation = p.rotation;
        id.r = p.color.x;
        id.g = p.color.y;
        id.b = p.color.z;
        id.a = p.color.w;
        instanceData.push_back(id);
    }

    instanceVBO.bind(GL_ARRAY_BUFFER);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

    vao.bind();
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(particles.size()));
}
