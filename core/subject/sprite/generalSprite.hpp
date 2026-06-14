#pragma once
#include <graphics/gl.hpp>
#include <graphics/quad.hpp>
#include <utils/math/vector.h>

namespace BlazeBolt {
    struct SpriteMesh {
    private:
        GL::VertexArrayObject vertexArrayObject;
    public:
        SpriteMesh();
        explicit SpriteMesh(const QuadVertexBufferObject2D &vertexBufferObject);
        ~SpriteMesh() = default;

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
        void setTextureRect(const Vector4 &rect) const;
        void setWorldPos(const Vector2 &pos) const;
        void setLightData(int count, const float* positions, const float* colors, const float* intensities, const float* radii, const Vector3& ambientColor, float ambientIntensity) const;
    };

    struct SpriteBatchShader2D {
    private:
        GL::ShaderProgram shaderProgram;
    public:
        SpriteBatchShader2D();
        ~SpriteBatchShader2D() = default;
        void bind() const;
        void setAspectRatio(float aspectRatio) const;
        void setMVPMatrix(const Matrix3x3 &matrix) const;
        void setLightData(int count, const float* positions, const float* colors, const float* intensities, const float* radii, const Vector3& ambientColor, float ambientIntensity) const;
    };
}