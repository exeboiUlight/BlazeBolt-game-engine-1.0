#pragma once
#include <graphics/gl.hpp>
#include <graphics/renderer/Texture.h>
#include <graphics/renderer/Buffer.h>
#include <graphics/renderer/Pipeline.h>
#include <graphics/renderer/RenderContext.h>
#include <graphics/renderer/RenderDevice.h>
#include <subject/sprite/generalSprite.hpp>
#include <subject/sprite/staticSprite2D.hpp>
#include <world.h>
#include <vector>

namespace BlazeBolt {
    class SpriteBatch2D {
    public:
        struct BatchVertex {
            float x, y;
            float u, v;
            float r, g, b, a;
        };

    private:
        uint32_t maxSize;
        std::vector<Entity> spriteEntities;
        const GL::Texture2D* texture;
        ITexture* rhiTexture;

        GL::VertexArrayObject vao;
        GL::VertexBufferObject vbo;
        GL::VertexBufferObject ibo;

        IRenderDevice* device;
        IBuffer* rhiVbo;
        IBuffer* rhiIbo;
        IPipeline* pipeline;

        bool dirty;
        uint32_t lastRebuiltVertices;

        void buildIndexBuffer();
        void buildVAO();

    public:
        SpriteBatch2D(uint32_t maxSize = 25);
        SpriteBatch2D(SpriteBatch2D&& other) noexcept;
        SpriteBatch2D &operator=(SpriteBatch2D&& other) noexcept;
        ~SpriteBatch2D() = default;

        void init(IRenderDevice* dev);

        void setMaxSize(uint32_t size);
        uint32_t getMaxSize() const;

        void setTexture(const GL::Texture2D& tex);
        void setTexture(ITexture* tex);
        const GL::Texture2D* getTexture() const;
        ITexture* getTextureRHI() const;

        void setPipeline(IPipeline* p);
        IPipeline* getPipeline() const;

        bool add(Entity entity);
        bool remove(Entity entity);
        void clear();
        uint32_t count() const { return spriteEntities.size(); }

        void rebuild(World<Sprite2D>& spriteWorld);
        void draw(const GL::Texture2D& defaultTexture) const;
        void draw(IRenderContext* context, ITexture* defaultTexture) const;
    };
}
