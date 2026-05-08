#pragma once

#include <graphics/mesh.h>
#include <graphics/shader.h>
#include <utils/math/vector.h>
#include <glad/glad.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#include <vector>
#include <iostream>

namespace Gui {

struct GuiChar {
    GLuint textureID;
    float sizeX, sizeY;
    float bearingX, bearingY;
    float advance;
};

class GuiFont {
private:
    std::map<char, GuiChar> chars;
    float m_maxAscent;
    bool m_loaded;
public:
    GuiFont() : m_maxAscent(0), m_loaded(false) {}

    ~GuiFont() {
        for (auto& pair : chars)
            glDeleteTextures(1, &pair.second.textureID);
        chars.clear();
    }

    bool load(const std::string& path, unsigned int size) {
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) return false;
        FT_Face face;
        if (FT_New_Face(ft, path.c_str(), 0, &face)) {
            FT_Done_FreeType(ft);
            return false;
        }
        FT_Set_Pixel_Sizes(face, 0, size);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        m_maxAscent = 0;
        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                         face->glyph->bitmap.width, face->glyph->bitmap.rows,
                         0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            float bearingY = (float)face->glyph->bitmap_top;
            if (bearingY > m_maxAscent) m_maxAscent = bearingY;
            chars[c] = {texture,
                        (float)face->glyph->bitmap.width,
                        (float)face->glyph->bitmap.rows,
                        (float)face->glyph->bitmap_left,
                        bearingY,
                        (float)face->glyph->advance.x};
        }
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        m_loaded = true;
        return true;
    }

    bool isLoaded() const { return m_loaded; }

    float measure(const std::string& text, float scale) const {
        float w = 0;
        for (char c : text) {
            auto it = chars.find(c);
            if (it != chars.end())
                w += (it->second.advance / 64.0f) * scale;
        }
        return w;
    }

    float getAscent() const { return m_maxAscent; }

    void render(const std::string& text, float px, float py, float scale,
                float cr, float cg, float cb, float ca,
                const Matrix3x3& transform, Shader* shader,
                int screenWidth = 1920, int screenHeight = 1080)
    {
        if (!m_loaded || text.empty()) return;
        float invW = 2.0f / (float)screenWidth;
        float invH = -2.0f / (float)screenHeight;
        float x = px * invW - 1.0f;
        float y = 1.0f + (py + m_maxAscent * scale) * invH;
        shader->use();
        shader->setVec4("uColor", cr, cg, cb, ca);
        shader->setMat3("uTransform", &transform.m[0][0]);
        for (size_t i = 0; i < text.length(); i++) {
            char c = text[i];
            auto it = chars.find(c);
            if (it == chars.end()) continue;
            GuiChar& ch = it->second;
            float xpos = x + ch.bearingX * scale * invW;
            float ypos = y - ch.bearingY * scale * invH;
            float w = ch.sizeX * scale * invW;
            float h = ch.sizeY * scale * invH;
            std::vector<Mesh2D::Vertex> verts = {
                {xpos, ypos,  0, 0},
                {xpos + w, ypos,  1, 0},
                {xpos + w, ypos + h, 1, 1},
                {xpos, ypos + h, 0, 1}
            };
            std::vector<GLuint> idx = {0, 1, 2, 2, 3, 0};
            Mesh2D mesh;
            mesh.setData(verts, idx);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            shader->setInt("uTexture", 0);
            mesh.draw();
            x += (ch.advance / 64.0f) * scale * invW;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

} // namespace Gui
