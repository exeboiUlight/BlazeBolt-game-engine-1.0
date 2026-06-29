#pragma once

#include <glad/glad.h>
#include <vector>
#include <graphics/renderer/Buffer.h>
#include <graphics/renderer/RenderContext.h>
#include <graphics/renderer/RenderDevice.h>

class Mesh2D {
public:
    struct Vertex {
        float x, y;
        float u, v;
    };

    Mesh2D() : VAO(0), VBO(0), EBO(0), indexCount(0),
               vertexBuffer(nullptr), indexBuffer(nullptr), device(nullptr) {}

    Mesh2D(IBuffer* vb, IBuffer* ib, uint32_t idxCount, IRenderDevice* dev)
        : VAO(0), VBO(0), EBO(0), indexCount(idxCount),
          vertexBuffer(vb), indexBuffer(ib), device(dev) {}

    ~Mesh2D() {
        cleanup();
    }

    void setData(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
        cleanup();

        if (vertices.empty() || indices.empty()) return;

        indexCount = indices.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        vertexBuffer = nullptr;
        indexBuffer = nullptr;
        device = nullptr;
    }

    void setData(IBuffer* vb, IBuffer* ib, uint32_t idxCount, IRenderDevice* dev) {
        cleanup();
        vertexBuffer = vb;
        indexBuffer = ib;
        indexCount = idxCount;
        device = dev;
    }

    void draw() const {
        if (indexCount == 0) return;

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void draw(IRenderContext* context) const {
        if (!context || !vertexBuffer || !indexBuffer) {
            draw();
            return;
        }
        context->bindVertexBuffer(vertexBuffer, sizeof(Vertex));
        context->bindIndexBuffer(indexBuffer);
        context->drawIndexed(indexCount, 0);
    }

private:
    GLuint VAO, VBO, EBO;
    GLsizei indexCount;

    IBuffer* vertexBuffer;
    IBuffer* indexBuffer;
    IRenderDevice* device;

    void cleanup() {
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        indexCount = 0;
    }
};
