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

    IBuffer* QuadVertexBufferObject2D::createIBuffer(IRenderDevice* device) const {
        Vertex vertices[] = {
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
        };
        BufferDesc desc;
        desc.size = sizeof(vertices);
        desc.type = BufferType::VERTEX;
        desc.usage = BufferUsage::STATIC;
        IBuffer* buffer = device->createBuffer(desc);
        if (buffer) {
            buffer->upload(vertices, sizeof(vertices), 0);
        }
        return buffer;
    }
}