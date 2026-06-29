#include "spriteBatch2D.hpp"
#include <cstring>

namespace BlazeBolt {
    SpriteBatch2D::SpriteBatch2D(SpriteBatch2D&& other) noexcept :
        maxSize(other.maxSize),
        spriteEntities(std::move(other.spriteEntities)),
        texture(other.texture),
        rhiTexture(other.rhiTexture),
        vao(std::move(other.vao)),
        vbo(std::move(other.vbo)),
        ibo(std::move(other.ibo)),
        device(other.device),
        rhiVbo(other.rhiVbo),
        rhiIbo(other.rhiIbo),
        pipeline(other.pipeline),
        dirty(other.dirty),
        lastRebuiltVertices(other.lastRebuiltVertices)
    {
        other.maxSize = 0;
        other.texture = nullptr;
        other.rhiTexture = nullptr;
        other.device = nullptr;
        other.rhiVbo = nullptr;
        other.rhiIbo = nullptr;
        other.pipeline = nullptr;
        other.dirty = false;
        other.lastRebuiltVertices = 0;
    }

    SpriteBatch2D &SpriteBatch2D::operator=(SpriteBatch2D&& other) noexcept {
        if (this != &other) {
            maxSize = other.maxSize;
            spriteEntities = std::move(other.spriteEntities);
            texture = other.texture;
            rhiTexture = other.rhiTexture;
            vao = std::move(other.vao);
            vbo = std::move(other.vbo);
            ibo = std::move(other.ibo);
            device = other.device;
            rhiVbo = other.rhiVbo;
            rhiIbo = other.rhiIbo;
            pipeline = other.pipeline;
            dirty = other.dirty;
            lastRebuiltVertices = other.lastRebuiltVertices;
            other.maxSize = 0;
            other.texture = nullptr;
            other.rhiTexture = nullptr;
            other.device = nullptr;
            other.rhiVbo = nullptr;
            other.rhiIbo = nullptr;
            other.pipeline = nullptr;
            other.dirty = false;
            other.lastRebuiltVertices = 0;
        }
        return *this;
    }

    SpriteBatch2D::SpriteBatch2D(uint32_t maxSize) :
        maxSize(maxSize > 0 ? maxSize : 25),
        spriteEntities(),
        texture(nullptr),
        rhiTexture(nullptr),
        vao(),
        vbo(),
        ibo(),
        device(nullptr),
        rhiVbo(nullptr),
        rhiIbo(nullptr),
        pipeline(nullptr),
        dirty(false),
        lastRebuiltVertices(0)
    {
        buildIndexBuffer();
        buildVAO();

        size_t vertexBufferSize = this->maxSize * 4 * sizeof(BatchVertex);
        this->vbo.bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_DYNAMIC_DRAW);
    }

    void SpriteBatch2D::init(IRenderDevice* dev) {
        this->device = dev;

        BufferDesc vbDesc;
        vbDesc.size = this->maxSize * 4 * sizeof(BatchVertex);
        vbDesc.type = BufferType::VERTEX;
        vbDesc.usage = BufferUsage::DYNAMIC;
        this->rhiVbo = dev->createBuffer(vbDesc);

        BufferDesc ibDesc;
        ibDesc.size = this->maxSize * 6 * sizeof(uint16_t);
        ibDesc.type = BufferType::INDEX;
        ibDesc.usage = BufferUsage::STATIC;
        this->rhiIbo = dev->createBuffer(ibDesc);

        size_t indexCount = this->maxSize * 6;
        uint16_t* indices = new uint16_t[indexCount];
        for (uint32_t i = 0; i < this->maxSize; i++) {
            uint16_t base = i * 4;
            uint16_t offset = i * 6;
            indices[offset + 0] = base + 0;
            indices[offset + 1] = base + 1;
            indices[offset + 2] = base + 2;
            indices[offset + 3] = base + 0;
            indices[offset + 4] = base + 2;
            indices[offset + 5] = base + 3;
        }
        this->rhiIbo->upload(indices, indexCount * sizeof(uint16_t), 0);
        delete[] indices;
    }

    void SpriteBatch2D::buildIndexBuffer() {
        this->ibo.bind(GL_ELEMENT_ARRAY_BUFFER);
        size_t indexCount = this->maxSize * 6;
        GLushort* indices = new GLushort[indexCount];
        for (uint32_t i = 0; i < this->maxSize; i++) {
            GLushort base = i * 4;
            GLushort offset = i * 6;
            indices[offset + 0] = base + 0;
            indices[offset + 1] = base + 1;
            indices[offset + 2] = base + 2;
            indices[offset + 3] = base + 0;
            indices[offset + 4] = base + 2;
            indices[offset + 5] = base + 3;
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
        delete[] indices;
    }

    void SpriteBatch2D::buildVAO() {
        this->vao.bind();
        this->vbo.bind(GL_ARRAY_BUFFER);
        this->ibo.bind(GL_ELEMENT_ARRAY_BUFFER);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, x));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, u));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, r));
        glEnableVertexAttribArray(2);
    }

    void SpriteBatch2D::setMaxSize(uint32_t size) {
        if (size == this->maxSize || size == 0) return;
        this->maxSize = size;
        this->spriteEntities.clear();
        this->dirty = true;

        size_t vertexBufferSize = this->maxSize * 4 * sizeof(BatchVertex);
        this->vbo.bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_DYNAMIC_DRAW);

        this->ibo.bind(GL_ELEMENT_ARRAY_BUFFER);
        size_t indexCount = this->maxSize * 6;
        GLushort* indices = new GLushort[indexCount];
        for (uint32_t i = 0; i < this->maxSize; i++) {
            GLushort base = i * 4;
            GLushort offset = i * 6;
            indices[offset + 0] = base + 0;
            indices[offset + 1] = base + 1;
            indices[offset + 2] = base + 2;
            indices[offset + 3] = base + 0;
            indices[offset + 4] = base + 2;
            indices[offset + 5] = base + 3;
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
        delete[] indices;

        if (this->device != nullptr) {
            if (this->rhiVbo != nullptr) this->device->destroyBuffer(this->rhiVbo);
            if (this->rhiIbo != nullptr) this->device->destroyBuffer(this->rhiIbo);

            BufferDesc vbDesc;
            vbDesc.size = vertexBufferSize;
            vbDesc.type = BufferType::VERTEX;
            vbDesc.usage = BufferUsage::DYNAMIC;
            this->rhiVbo = this->device->createBuffer(vbDesc);

            BufferDesc ibDesc;
            ibDesc.size = this->maxSize * 6 * sizeof(uint16_t);
            ibDesc.type = BufferType::INDEX;
            ibDesc.usage = BufferUsage::STATIC;
            this->rhiIbo = this->device->createBuffer(ibDesc);

            this->rhiIbo->upload(indices, this->maxSize * 6 * sizeof(uint16_t), 0);
        }
    }

    uint32_t SpriteBatch2D::getMaxSize() const {
        return this->maxSize;
    }

    void SpriteBatch2D::setTexture(const GL::Texture2D& tex) {
        this->texture = &tex;
    }

    void SpriteBatch2D::setTexture(ITexture* tex) {
        this->rhiTexture = tex;
    }

    const GL::Texture2D* SpriteBatch2D::getTexture() const {
        return this->texture;
    }

    ITexture* SpriteBatch2D::getTextureRHI() const {
        return this->rhiTexture;
    }

    void SpriteBatch2D::setPipeline(IPipeline* p) {
        this->pipeline = p;
    }

    IPipeline* SpriteBatch2D::getPipeline() const {
        return this->pipeline;
    }

    bool SpriteBatch2D::add(Entity entity) {
        if (spriteEntities.size() >= this->maxSize) return false;
        for (auto e : spriteEntities) {
            if (e == entity) return false;
        }
        spriteEntities.push_back(entity);
        this->dirty = true;
        return true;
    }

    bool SpriteBatch2D::remove(Entity entity) {
        for (auto it = spriteEntities.begin(); it != spriteEntities.end(); ++it) {
            if (*it == entity) {
                spriteEntities.erase(it);
                this->dirty = true;
                return true;
            }
        }
        return false;
    }

    void SpriteBatch2D::clear() {
        spriteEntities.clear();
        this->dirty = true;
    }

    void SpriteBatch2D::rebuild(World<Sprite2D>& spriteWorld) {
        if (!this->dirty) return;
        this->dirty = false;

        uint32_t count = static_cast<uint32_t>(spriteEntities.size());
        if (count == 0) return;
        if (count > this->maxSize) count = this->maxSize;

        BatchVertex* vertices = new BatchVertex[count * 4];
        uint32_t vertexIndex = 0;

        for (uint32_t i = 0; i < count; i++) {
            const Sprite2D* sprite = spriteWorld.getEntity(spriteEntities[i]);
            if (sprite == nullptr) continue;

            Vector4 color = sprite->getColor();
            Vector4 texRect = sprite->getTextureRect();

            Vector2 origin = sprite->getOrigin();
            Vector2 size = sprite->getScale();

            float left = -origin.x * size.x;
            float right = (1.0f - origin.x) * size.x;
            float bottom = -origin.y * size.y;
            float top = (1.0f - origin.y) * size.y;

            Vector2 pos = sprite->getPosition();
            float rot = sprite->getRotation();
            float cosA = cosf(rot * 3.14159265f / 180.0f);
            float sinA = sinf(rot * 3.14159265f / 180.0f);

            float corners[4][2] = {
                {left,  bottom},
                {right, bottom},
                {right, top},
                {left,  top}
            };

            float uvs[4][2] = {
                {texRect.x,                texRect.y},
                {texRect.x + texRect.z,    texRect.y},
                {texRect.x + texRect.z,    texRect.y + texRect.w},
                {texRect.x,                texRect.y + texRect.w}
            };

            for (int j = 0; j < 4; j++) {
                float wx = corners[j][0] * cosA - corners[j][1] * sinA + pos.x;
                float wy = corners[j][0] * sinA + corners[j][1] * cosA + pos.y;

                vertices[vertexIndex].x = wx;
                vertices[vertexIndex].y = wy;
                vertices[vertexIndex].u = uvs[j][0];
                vertices[vertexIndex].v = uvs[j][1];
                vertices[vertexIndex].r = color.x;
                vertices[vertexIndex].g = color.y;
                vertices[vertexIndex].b = color.z;
                vertices[vertexIndex].a = color.w;
                vertexIndex++;
            }
        }

        this->lastRebuiltVertices = vertexIndex;

        if (vertexIndex > 0) {
            this->vbo.bind(GL_ARRAY_BUFFER);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexIndex * sizeof(BatchVertex), vertices);

            if (this->rhiVbo != nullptr) {
                this->rhiVbo->upload(vertices, vertexIndex * sizeof(BatchVertex), 0);
            }
        }

        delete[] vertices;
    }

    void SpriteBatch2D::draw(const GL::Texture2D& defaultTexture) const {
        uint32_t spriteCount = this->lastRebuiltVertices / 4;
        if (spriteCount == 0) return;

        this->vao.bind();

        if (this->texture != nullptr) {
            GL::setActiveTextureUnit(0);
            this->texture->bind();
        } else {
            GL::setActiveTextureUnit(0);
            defaultTexture.bind();
        }

        glDrawElements(GL_TRIANGLES, spriteCount * 6, GL_UNSIGNED_SHORT, nullptr);
    }

    void SpriteBatch2D::draw(IRenderContext* context, ITexture* defaultTexture) const {
        uint32_t spriteCount = this->lastRebuiltVertices / 4;
        if (spriteCount == 0) return;

        if (this->pipeline != nullptr) {
            context->bindPipeline(this->pipeline);
        }

        if (this->rhiVbo != nullptr) {
            context->bindVertexBuffer(this->rhiVbo, sizeof(BatchVertex));
        }
        if (this->rhiIbo != nullptr) {
            context->bindIndexBuffer(this->rhiIbo);
        }

        ITexture* tex = this->rhiTexture != nullptr ? this->rhiTexture : defaultTexture;
        if (tex != nullptr) {
            context->bindTexture(0, tex);
        }

        context->drawIndexed(spriteCount * 6);
    }
}
