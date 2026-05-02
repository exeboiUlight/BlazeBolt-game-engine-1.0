#pragma once
#include <string>
#include <graphics/gl.hpp>
#include <graphics/shader.h>
#include <graphics/textureManager.hpp>
#include <utils/math/vector.h>

namespace BlazeBolt {
    struct SpriteMesh2D {
    private:
        GL::VertexArrayObject vao;
        GL::VertexBufferObject vbo;
    public:
        SpriteMesh2D();
        ~SpriteMesh2D() = default;
        void draw() const;
    };
    struct SpriteShader2D {
    private:
        GL::ShaderProgram shaderProgram;
    public:
        SpriteShader2D();
        ~SpriteShader2D() = default;
        void bind() const;
        void setMVPMatrix(const Matrix3x3 &matrix) const;
        void setColor(const Vector4 &color) const;
    };

    class Sprite2D {
    private:
        const GL::Texture2D *texture;
        Matrix3x3 modelMatrix;
        Vector4 color;
        Vector2 position;
        Vector2 scale;
        Vector2 origin;
        float rotation;
        bool visible;

        void updateModelMatrix();
    public:
        Sprite2D();
        ~Sprite2D() = default;

        void setTexture(const GL::Texture2D &texture);
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

        void draw(const TextureManager &textureManager, const SpriteShader2D &shader, const SpriteMesh2D &mesh, const Matrix3x3 &projectionViewMatrix) const;
    };
    
    /*
        float left = -m_size.x * m_origin.x;
        float right = m_size.x * (1.0f - m_origin.x);
        float bottom = -m_size.y * m_origin.y;
        float top = m_size.y * (1.0f - m_origin.y);
    */
}