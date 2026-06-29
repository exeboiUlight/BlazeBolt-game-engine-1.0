#include "tileset2D.hpp"

namespace BlazeBolt {
    Tileset2D::Tileset2D(uint32_t tileW, uint32_t tileH, uint32_t cols, uint32_t rows, uint32_t maxTiles) :
        tileWidth(tileW > 0 ? tileW : 16),
        tileHeight(tileH > 0 ? tileH : 16),
        atlasCols(cols > 0 ? cols : 1),
        atlasRows(rows > 0 ? rows : 1),
        position(0, 0),
        map(),
        tileEntities(),
        batch(maxTiles > 0 ? maxTiles : 1024),
        currentGlAtlas(nullptr)
    {
    }

    void Tileset2D::clearTiles(World<Sprite2D>& spriteWorld) {
        for (Entity e : tileEntities) {
            spriteWorld.destroy(e);
        }
        tileEntities.clear();
        batch.clear();
    }

    void Tileset2D::rebuildTiles(World<Sprite2D>& spriteWorld, const GL::Texture2D& atlas) {
        clearTiles(spriteWorld);
        if (map.empty()) return;

        float texAtlasW = static_cast<float>(atlasCols);
        float texAtlasH = static_cast<float>(atlasRows);

        uint32_t rows = static_cast<uint32_t>(map.size());
        for (uint32_t r = 0; r < rows; r++) {
            uint32_t cols = static_cast<uint32_t>(map[r].size());
            for (uint32_t c = 0; c < cols; c++) {
                int tileIndex = map[r][c];
                if (tileIndex < 0) continue;

                uint32_t tCol = tileIndex % atlasCols;
                uint32_t tRow = tileIndex / atlasCols;
                if (tRow >= atlasRows) continue;

                float u = static_cast<float>(tCol) / texAtlasW;
                float v = 1.0f - static_cast<float>(tRow + 1) / texAtlasH;
                float tw = 1.0f / texAtlasW;
                float th = 1.0f / texAtlasH;

                float tileX = position.x + static_cast<float>(c) * tileWidth;
                float tileY = position.y + static_cast<float>(r) * tileHeight;

                Sprite2D* sprite = new Sprite2D();
                sprite->setTexture(atlas);
                sprite->setPosition(tileX, tileY);
                sprite->setScale(static_cast<float>(tileWidth), static_cast<float>(tileHeight));
                sprite->setOrigin(0, 0);
                sprite->setTextureRect(u, v, tw, th);
                sprite->setColor(1, 1, 1, 1);

                Entity entity = spriteWorld.spawn(sprite);
                tileEntities.push_back(entity);
                batch.add(entity);
            }
        }
    }

    void Tileset2D::setMap(const std::vector<std::vector<int>>& newMap, World<Sprite2D>& spriteWorld, const GL::Texture2D& atlas) {
        currentGlAtlas = &atlas;
        batch.setTexture(atlas);
        map = newMap;
        rebuildTiles(spriteWorld, atlas);
    }

    void Tileset2D::setMap(const std::vector<std::vector<int>>& newMap, World<Sprite2D>& spriteWorld, ITexture* atlas) {
        batch.setTexture(atlas);
        map = newMap;
        if (currentGlAtlas != nullptr) {
            rebuildTiles(spriteWorld, *currentGlAtlas);
        }
    }

    int Tileset2D::getTile(uint32_t col, uint32_t row) const {
        if (row >= map.size()) return -1;
        if (col >= map[row].size()) return -1;
        return map[row][col];
    }

    void Tileset2D::setTile(uint32_t col, uint32_t row, int tileIndex, World<Sprite2D>& spriteWorld, const GL::Texture2D& atlas) {
        if (row >= map.size()) return;
        if (col >= map[row].size()) return;
        currentGlAtlas = &atlas;
        map[row][col] = tileIndex;
        rebuildTiles(spriteWorld, atlas);
    }

    void Tileset2D::setTile(uint32_t col, uint32_t row, int tileIndex, World<Sprite2D>& spriteWorld, ITexture* atlas) {
        if (row >= map.size()) return;
        if (col >= map[row].size()) return;
        map[row][col] = tileIndex;
        batch.setTexture(atlas);
        if (currentGlAtlas != nullptr) {
            rebuildTiles(spriteWorld, *currentGlAtlas);
        }
    }

    void Tileset2D::setTileSize(uint32_t w, uint32_t h) {
        tileWidth = w > 0 ? w : 16;
        tileHeight = h > 0 ? h : 16;
    }

    void Tileset2D::getTileSize(uint32_t& w, uint32_t& h) const {
        w = tileWidth;
        h = tileHeight;
    }

    void Tileset2D::setPosition(const Vector2& pos) {
        position = pos;
    }

    const Vector2& Tileset2D::getPosition() const {
        return position;
    }

    uint32_t Tileset2D::getMapWidth() const {
        if (map.empty()) return 0;
        return static_cast<uint32_t>(map[0].size());
    }

    uint32_t Tileset2D::getMapHeight() const {
        return static_cast<uint32_t>(map.size());
    }

    uint32_t Tileset2D::getTileCount() const {
        return static_cast<uint32_t>(tileEntities.size());
    }

    void Tileset2D::rebuild(World<Sprite2D>& spriteWorld) {
        batch.rebuild(spriteWorld);
    }

    void Tileset2D::draw(const GL::Texture2D& defaultTexture) {
        batch.draw(defaultTexture);
    }

    void Tileset2D::draw(IRenderContext* context, ITexture* defaultTexture) {
        batch.draw(context, defaultTexture);
    }
}
