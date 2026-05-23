#include "staticSprite2D.hpp"

namespace BlazeBolt {
    Sprite2D::Sprite2D() :
        texture(nullptr), modelMatrix(), color(1.0f, 1.0f, 1.0f, 1.0f),
        position(), scale(1.0f, 1.0f), origin(0.5f, 0.5f), textureRect(0.0f, 0.0f, 1.0f, 1.0f),
        rotation(0.0f), visible(true)
    {
        this->updateModelMatrix();
    }

    void Sprite2D::updateModelMatrix() {
        Matrix3x3 translationMatrix = Matrix3x3::translation(this->position.x, this->position.y);
        Matrix3x3 rotationMatrix = Matrix3x3::rotation(this->rotation);
        Matrix3x3 scaleMatrix = Matrix3x3::scale(this->scale.x, this->scale.y);
        Matrix3x3 originTranslationMatrix = Matrix3x3::translation(-this->origin.x, -this->origin.y);
        this->modelMatrix = translationMatrix * rotationMatrix * scaleMatrix * originTranslationMatrix;
    }

    void Sprite2D::setTexture(const GL::Texture2D &texture) {
        this->texture = &texture;
    }
    void Sprite2D::setPosition(float x, float y) {
        this->position.x = x;
        this->position.y = y;
        this->updateModelMatrix();
    }
    void Sprite2D::setPosition(const Vector2 &position) {
        this->position = position;
        this->updateModelMatrix();
    }
    const Vector2 &Sprite2D::getPosition() const {
        return this->position;
    }
    
    void Sprite2D::setScale(float width, float height) {
        this->scale.x = width;
        this->scale.y = height;
        this->updateModelMatrix();
    }
    void Sprite2D::setScale(const Vector2 &scale) {
        this->scale = scale;
        this->updateModelMatrix();
    }
    const Vector2 &Sprite2D::getScale() const {
        return this->scale;
    }

    void Sprite2D::setOrigin(float x, float y) {
        this->origin.x = x;
        this->origin.y = y;
        this->updateModelMatrix();
    }
    void Sprite2D::setOrigin(const Vector2 &origin) {
        this->origin = origin;
        this->updateModelMatrix();
    }
    const Vector2 &Sprite2D::getOrigin() const {
        return this->origin;
    }
    void Sprite2D::setRotation(float degrees) {
        this->rotation = degrees;
        this->updateModelMatrix();
    }
    float Sprite2D::getRotation() const {
        return this->rotation;
    }

    void Sprite2D::setTextureRect(float u, float v, float w, float h) {
        this->textureRect.x = u;
        this->textureRect.y = v;
        this->textureRect.z = w;
        this->textureRect.w = h;
    }
    void Sprite2D::setTextureRect(const Vector4 &rect) {
        this->textureRect = rect;
    }
    Vector4 Sprite2D::getTextureRect() const {
        return this->textureRect;
    }

    void Sprite2D::setColor(float r, float g, float b, float a) {
        this->color.x = r;
        this->color.y = g;
        this->color.z = b;
        this->color.w = a;
    }
    void Sprite2D::setColor(const Vector4 &color) {
        this->color = color;
    }
    Vector4 Sprite2D::getColor() const {
        return this->color;
    }
    void Sprite2D::setVisible(bool visible) {
        this->visible = visible;
    }
    bool Sprite2D::isVisible() const {
        return this->visible;
    }

    void Sprite2D::draw(const GL::Texture2D &defaultTexture, const SpriteShader2D &shader, const SpriteMesh &mesh, const Matrix3x3 &projectionViewMatrix) const {
        if (!this->visible) { return; }
        shader.bind();
        shader.setMVPMatrix(projectionViewMatrix * this->modelMatrix);
        shader.setColor(this->color);
        shader.setTextureRect(this->textureRect);
        
        GL::setActiveTextureUnit(0);
        if (this->texture != nullptr) {
            this->texture->bind();
        } else {
            defaultTexture.bind();
        }

        mesh.draw();
    }
}