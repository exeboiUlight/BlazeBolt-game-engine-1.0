#include "animatedSprite2D.hpp"

namespace BlazeBolt {
    AnimatedSprite2D::AnimatedSprite2D() :
        texture(nullptr), modelMatrix(), color(1.0f, 1.0f, 1.0f, 1.0f),
        position(), scale(1.0f, 1.0f), origin(0.5f, 0.5f), rotation(0.0f), visible(true),
        playbackSpeed(1.0f), elapsedTime(0.0f), currentFrame(0), looping(true), playing(false)
    {
        this->updateModelMatrix();
    }

    void AnimatedSprite2D::updateModelMatrix() {
        Matrix3x3 translationMatrix = Matrix3x3::translation(this->position.x, this->position.y);
        Matrix3x3 rotationMatrix = Matrix3x3::rotation(this->rotation);
        Matrix3x3 scaleMatrix = Matrix3x3::scale(this->scale.x, this->scale.y);
        Matrix3x3 originTranslationMatrix = Matrix3x3::translation(-this->origin.x, -this->origin.y);
        this->modelMatrix = translationMatrix * rotationMatrix * scaleMatrix * originTranslationMatrix;
    }

    void AnimatedSprite2D::update(float deltaTime) {
        if (!this->playing || this->texture == nullptr) { return; }
        this->elapsedTime += deltaTime;
        
        float frameDuration = 1.0f / this->playbackSpeed;
        if (this->elapsedTime >= frameDuration) {
            this->elapsedTime -= frameDuration;
            this->currentFrame++;
            if (this->currentFrame >= this->texture->getNumFrames()) {
                if (this->looping) {
                    this->currentFrame = 0;
                } else {
                    this->currentFrame--;
                    this->playing = false;
                }
            }
        }
    }

    void AnimatedSprite2D::setPlaybackSpeed(float speed) {
        this->playbackSpeed = speed;
    }
    float AnimatedSprite2D::getPlaybackSpeed() const {
        return this->playbackSpeed;
    }
    void AnimatedSprite2D::setLooping(bool looping) {
        this->looping = looping;
    }
    bool AnimatedSprite2D::isLooping() const {
        return this->looping;
    }
    void AnimatedSprite2D::setFrame(uint32_t frame) {
        this->currentFrame = frame;
    }
    uint32_t AnimatedSprite2D::getCurrentFrame() const {
        return this->currentFrame;
    }
    uint32_t AnimatedSprite2D::getNumFrames() const {
        return this->texture ? this->texture->getNumFrames() : 0;
    }

    void AnimatedSprite2D::play() {
        this->playing = true;
    }
    void AnimatedSprite2D::pause() {
        this->playing = false;
    }
    void AnimatedSprite2D::stop() {
        this->currentFrame = 0;
        this->elapsedTime = 0.0f;
        this->playing = false;
    }
    void AnimatedSprite2D::restart() {
        this->currentFrame = 0;
        this->elapsedTime = 0.0f;
        this->playing = true;
    }
    bool AnimatedSprite2D::isPlaying() const {
        return this->playing;
    }

    void AnimatedSprite2D::setTexture(const AnimatedTexture2D &texture) {
        this->texture = &texture;
    }
    void AnimatedSprite2D::setPosition(float x, float y) {
        this->position.x = x;
        this->position.y = y;
        this->updateModelMatrix();
    }
    void AnimatedSprite2D::setPosition(const Vector2 &position) {
        this->position = position;
        this->updateModelMatrix();
    }
    const Vector2 &AnimatedSprite2D::getPosition() const {
        return this->position;
    }
    
    void AnimatedSprite2D::setScale(float width, float height) {
        this->scale.x = width;
        this->scale.y = height;
        this->updateModelMatrix();
    }
    void AnimatedSprite2D::setScale(const Vector2 &scale) {
        this->scale = scale;
        this->updateModelMatrix();
    }
    const Vector2 &AnimatedSprite2D::getScale() const {
        return this->scale;
    }

    void AnimatedSprite2D::setOrigin(float x, float y) {
        this->origin.x = x;
        this->origin.y = y;
        this->updateModelMatrix();
    }
    void AnimatedSprite2D::setOrigin(const Vector2 &origin) {
        this->origin = origin;
        this->updateModelMatrix();
    }
    const Vector2 &AnimatedSprite2D::getOrigin() const {
        return this->origin;
    }
    void AnimatedSprite2D::setRotation(float degrees) {
        this->rotation = degrees;
        this->updateModelMatrix();
    }
    float AnimatedSprite2D::getRotation() const {
        return this->rotation;
    }

    void AnimatedSprite2D::setColor(float r, float g, float b, float a) {
        this->color.x = r;
        this->color.y = g;
        this->color.z = b;
        this->color.w = a;
    }
    void AnimatedSprite2D::setColor(const Vector4 &color) {
        this->color = color;
    }
    Vector4 AnimatedSprite2D::getColor() const {
        return this->color;
    }
    void AnimatedSprite2D::setVisible(bool visible) {
        this->visible = visible;
    }
    bool AnimatedSprite2D::isVisible() const {
        return this->visible;
    }

    void AnimatedSprite2D::draw(const SpriteShader2D &shader, const SpriteMesh &mesh, const Matrix3x3 &projectionViewMatrix) const {
        if (!this->visible || this->texture == nullptr) { return; }

        shader.bind();
        shader.setMVPMatrix(projectionViewMatrix * this->modelMatrix);
        shader.setColor(this->color);
        shader.setTextureRect(Vector4(
            static_cast<float>(this->currentFrame) / static_cast<float>(this->texture->getNumFrames()), 0.0f,
            1.0f / static_cast<float>(this->texture->getNumFrames()), 1.0f
        ));

        GL::setActiveTextureUnit(0);
        this->texture->getGL().bind();
        mesh.draw();
    }
}