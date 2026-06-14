#pragma once
#include <vector>
#include <cstdint>
#include <graphics/spriteBatch2D.hpp>
#include <subject/sprite/staticSprite2D.hpp>
#include <world.h>

namespace BlazeBolt {
    class Tileset2D {
    private:
        uint32_t tileWidth, tileHeight;
        uint32_t atlasCols, atlasRows;
        Vector2 position;
        std::vector<std::vector<int>> map;
        std::vector<Entity> tileEntities;
        SpriteBatch2D batch;

        void clearTiles(World<Sprite2D>& spriteWorld);
        void rebuildTiles(World<Sprite2D>& spriteWorld, const GL::Texture2D& atlas);

    public:
        Tileset2D(uint32_t tileW, uint32_t tileH, uint32_t cols, uint32_t rows, uint32_t maxTiles = 1024);
        ~Tileset2D() = default;

        void setMap(const std::vector<std::vector<int>>& newMap, World<Sprite2D>& spriteWorld, const GL::Texture2D& atlas);
        int getTile(uint32_t col, uint32_t row) const;
        void setTile(uint32_t col, uint32_t row, int tileIndex, World<Sprite2D>& spriteWorld, const GL::Texture2D& atlas);

        void setTileSize(uint32_t w, uint32_t h);
        void getTileSize(uint32_t& w, uint32_t& h) const;

        void setPosition(const Vector2& pos);
        const Vector2& getPosition() const;

        uint32_t getMapWidth() const;
        uint32_t getMapHeight() const;
        uint32_t getTileCount() const;

        SpriteBatch2D& getBatch() { return batch; }
        const SpriteBatch2D& getBatch() const { return batch; }

        void rebuild(World<Sprite2D>& spriteWorld);
        void draw(const GL::Texture2D& defaultTexture);
    };
}
