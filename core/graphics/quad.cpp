#include "quad.hpp"

namespace BlazeBolt {
    QuadVertexBufferObject2D::QuadVertexBufferObject2D() {
        QuadVertexBufferObject2D::Vertex vertices[] = {
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
        };
        this->vbo.bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    void QuadVertexBufferObject2D::bind() const {
        this->vbo.bind(GL_ARRAY_BUFFER);
    }
}