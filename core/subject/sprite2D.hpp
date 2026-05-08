#pragma once
#include <string>
#include <graphics/gl.hpp>
#include <graphics/shader.h>
#include <graphics/quad.hpp>
#include <utils/math/vector.h>

namespace BlazeBolt {
    struct SpriteMesh2D {
    private:
        GL::VertexArrayObject vertexArrayObject;
    public:
        SpriteMesh2D();
        explicit SpriteMesh2D(const QuadVertexBufferObject2D &vertexBufferObject);
        ~SpriteMesh2D() = default;

        void setVertexBuffer(const QuadVertexBufferObject2D &vertexBufferObject);
        void draw() const;
    };
    struct SpriteShader2D {
    private:
        GL::ShaderProgram shaderProgram;
    public:
        SpriteShader2D();
        ~SpriteShader2D() = default;
        void bind() const;
        void setAspectRatio(float aspectRatio) const;
        void setMVPMatrix(const Matrix3x3 &matrix) const;
        void setColor(const Vector4 &color) const;
    };

    // TODO: Either implement ECS or a Node (OOP) system, so parts of the objects like visibility, transform and rendering could be part of a base, e.g. "Node2D/Object2D" class, and just add a custom objects, or just have an object with Transform2D component, Text2D/Sprite2D component, etc.
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

        void draw(const GL::Texture2D &defaultTexture, const SpriteShader2D &shader, const SpriteMesh2D &mesh, const Matrix3x3 &projectionViewMatrix) const;
    };
    
    /*
        float left = -m_size.x * m_origin.x;
        float right = m_size.x * (1.0f - m_origin.x);
        float bottom = -m_size.y * m_origin.y;
        float top = m_size.y * (1.0f - m_origin.y);
    */
}