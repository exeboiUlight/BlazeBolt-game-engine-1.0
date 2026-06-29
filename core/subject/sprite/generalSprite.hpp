#pragma once
#include <graphics/gl.hpp>
#include <graphics/quad.hpp>
#include <utils/math/vector.h>
#include <graphics/renderer/Pipeline.h>
#include <graphics/renderer/RenderContext.h>
#include <graphics/renderer/RenderDevice.h>
#include <graphics/renderer/Buffer.h>

namespace BlazeBolt {
    struct SpriteMesh {
    private:
        GL::VertexArrayObject vertexArrayObject;
        IBuffer* vertexBuffer = nullptr;
        IPipeline* pipeline = nullptr;
        IRenderDevice* device = nullptr;
        bool ownsBuffer = false;
    public:
        SpriteMesh();
        explicit SpriteMesh(const QuadVertexBufferObject2D &vertexBufferObject);
        SpriteMesh(IRenderDevice* device, const QuadVertexBufferObject2D &vertexBufferObject);
        ~SpriteMesh();

        void setVertexBuffer(const QuadVertexBufferObject2D &vertexBufferObject);
        void setVertexBuffer(const QuadVertexBufferObject2D &vertexBufferObject, IRenderDevice* dev);

        void draw() const;
        void draw(IRenderContext* context) const;

        void setPipeline(IPipeline* p);
        IPipeline* getPipeline() const { return pipeline; }
    };

    struct SpriteShader2D {
    private:
        GL::ShaderProgram shaderProgram;
        IRenderDevice* device = nullptr;
        IShader* vs = nullptr;
        IShader* fs = nullptr;
        IPipeline* pipeline = nullptr;
    public:
        SpriteShader2D();
        explicit SpriteShader2D(IRenderDevice* device);
        ~SpriteShader2D();

        void bind() const;
        IPipeline* getPipeline() const { return pipeline; }

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
        IRenderDevice* device = nullptr;
        IShader* vs = nullptr;
        IShader* fs = nullptr;
        IPipeline* pipeline = nullptr;
    public:
        SpriteBatchShader2D();
        explicit SpriteBatchShader2D(IRenderDevice* device);
        ~SpriteBatchShader2D();

        void bind() const;
        IPipeline* getPipeline() const { return pipeline; }

        void setAspectRatio(float aspectRatio) const;
        void setMVPMatrix(const Matrix3x3 &matrix) const;
        void setLightData(int count, const float* positions, const float* colors, const float* intensities, const float* radii, const Vector3& ambientColor, float ambientIntensity) const;
    };
}
