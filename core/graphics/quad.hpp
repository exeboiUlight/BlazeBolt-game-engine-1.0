#pragma once
#include "gl.hpp"
#include <graphics/renderer/Buffer.h>
#include <graphics/renderer/RenderDevice.h>

namespace BlazeBolt {
    struct QuadVertexBufferObject2D {
    private:
        GL::VertexBufferObject vbo;
    public:
        constexpr static GLuint VERTEX_COUNT = 4;
        constexpr static GLuint DRAW_MODE = GL_TRIANGLE_FAN;
        struct Vertex {
            float x, y;
        };

        QuadVertexBufferObject2D();
        ~QuadVertexBufferObject2D() = default;

        void bind() const;
        GLuint applyAttributes(GLint startIndex) const;

        IBuffer* createIBuffer(IRenderDevice* device) const;
    };
}