// text.h
#pragma once

#include <graphics/mesh.h>
#include <graphics/shader.h>
#include <utils/math/vector.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <string>
#include <vector>
#include <iostream>

struct Character {
    GLuint TextureID;
    float SizeX, SizeY;
    float BearingX, BearingY;
    unsigned int Advance;
};

class Text {
private:
    std::map<char, Character> m_characters;
    Shader* m_shader;
    std::vector<Mesh2D*> m_glyphMeshes;
    float m_posX, m_posY;
    float m_scale;
    float m_colorR, m_colorG, m_colorB, m_colorA;
    std::string m_text;
    bool m_visible;
    bool m_ownsShader;
    int m_screenWidth;
    int m_screenHeight;
    float m_maxAscent;
    
    float toNDCX(float pixelX) const {
        return (pixelX / m_screenWidth) * 2.0f - 1.0f;
    }
    
    float toNDCY(float pixelY) const {
        return 1.0f - (pixelY / m_screenHeight) * 2.0f;
    }
    
    void initFreeType(const std::string& fontPath, unsigned int fontSize) {
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return;
        }
        
        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
            FT_Done_FreeType(ft);
            return;
        }
        
        FT_Set_Pixel_Sizes(face, 0, fontSize);
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        m_maxAscent = 0;
        
        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cerr << "ERROR::FREETYPE: Failed to load Glyph: " << c << std::endl;
                continue;
            }
            
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            float bearingY = (float)face->glyph->bitmap_top;
            if (bearingY > m_maxAscent) {
                m_maxAscent = bearingY;
            }
            
            Character character = {
                texture,
                (float)face->glyph->bitmap.width,
                (float)face->glyph->bitmap.rows,
                (float)face->glyph->bitmap_left,
                bearingY,
                (unsigned int)face->glyph->advance.x
            };
            m_characters[c] = character;
        }
        
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }
    
    void regenerateMeshes() {
        for (auto mesh : m_glyphMeshes) {
            delete mesh;
        }
        m_glyphMeshes.clear();

        if (m_screenWidth == 0 || m_screenHeight == 0) return;

        float x = m_posX;
        float y = m_posY + m_maxAscent * m_scale;

        for (size_t i = 0; i < m_text.length(); i++) {
            char c = m_text[i];
            auto it = m_characters.find(c);
            if (it == m_characters.end()) continue;

            Character ch = it->second;

            float xpos = x + ch.BearingX * m_scale;
            float ypos = y - ch.BearingY * m_scale;
            float w = ch.SizeX * m_scale;
            float h = ch.SizeY * m_scale;

            float left = toNDCX(xpos);
            float right = toNDCX(xpos + w);
            float top = toNDCY(ypos);
            float bottom = toNDCY(ypos + h);

            std::vector<Mesh2D::Vertex> vertices = {
                {left,  top,    0.0f, 0.0f},
                {right, top,    1.0f, 0.0f},
                {right, bottom, 1.0f, 1.0f},
                {left,  bottom, 0.0f, 1.0f}
            };

            std::vector<GLuint> indices = {0, 1, 2, 2, 3, 0};

            Mesh2D* mesh = new Mesh2D();
            mesh->setData(vertices, indices);
            m_glyphMeshes.push_back(mesh);

            x += (ch.Advance >> 6) * m_scale;
        }
    }

    void initDefaultShader() {
        const char* vertSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        const char* fragSrc = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            uniform sampler2D uTextTexture;
            uniform vec4 uColor;
            void main() {
                float alpha = texture(uTextTexture, TexCoord).r;
                FragColor = vec4(uColor.rgb, uColor.a * alpha);
            }
        )";
        
        m_shader = new Shader(vertSrc, fragSrc);
        m_ownsShader = true;
    }
    
public:
    Text() : m_shader(nullptr), m_posX(0), m_posY(0), 
             m_scale(1.0f), m_colorR(1), m_colorG(1), m_colorB(1), m_colorA(1),
             m_visible(true), m_ownsShader(false), m_screenWidth(1200), m_screenHeight(600), m_maxAscent(0) {}
    
    Text(const std::string& fontPath, unsigned int fontSize = 48)
        : m_shader(nullptr), m_posX(0), m_posY(0), 
          m_scale(1.0f), m_colorR(1), m_colorG(1), m_colorB(1), m_colorA(1),
          m_visible(true), m_ownsShader(false), m_screenWidth(1200), m_screenHeight(600), m_maxAscent(0) {
        initDefaultShader();
        initFreeType(fontPath, fontSize);
    }
    
    Text(Shader* shader, const std::string& fontPath, unsigned int fontSize = 48)
        : m_shader(shader), m_posX(0), m_posY(0),
          m_scale(1.0f), m_colorR(1), m_colorG(1), m_colorB(1), m_colorA(1),
          m_visible(true), m_ownsShader(false), m_screenWidth(1200), m_screenHeight(600), m_maxAscent(0) {
        initFreeType(fontPath, fontSize);
    }
    
    ~Text() {
        for (auto mesh : m_glyphMeshes) {
            delete mesh;
        }
        for (auto& pair : m_characters) {
            glDeleteTextures(1, &pair.second.TextureID);
        }
        if (m_ownsShader && m_shader) delete m_shader;
    }
    
    void setScreenSize(int width, int height) {
        m_screenWidth = width;
        m_screenHeight = height;
        regenerateMeshes();
    }
    
    void setText(const std::string& text) {
        m_text = text;
        regenerateMeshes();
    }
    
    std::string getText() const { return m_text; }
    
    void setPosition(float x, float y) {
        m_posX = x;
        m_posY = y;
        regenerateMeshes();
    }
    
    void setPosition(const Vector2& position) {
        m_posX = position.x;
        m_posY = position.y;
        regenerateMeshes();
    }
    
    Vector2 getPosition() const { return Vector2(m_posX, m_posY); }
    
    void setScale(float scale) {
        m_scale = scale;
        regenerateMeshes();
    }
    
    float getScale() const { return m_scale; }
    
    void setColor(float r, float g, float b, float a = 1.0f) {
        m_colorR = r;
        m_colorG = g;
        m_colorB = b;
        m_colorA = a;
    }
    
    void setColor(const Vector4& color) {
        m_colorR = color.x;
        m_colorG = color.y;
        m_colorB = color.z;
        m_colorA = color.w;
    }
    
    Vector4 getColor() const { return Vector4(m_colorR, m_colorG, m_colorB, m_colorA); }
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    void draw() const {
        if (!m_visible || !m_shader || m_glyphMeshes.empty()) return;
        
        m_shader->use();
        m_shader->setVec4("uColor", m_colorR, m_colorG, m_colorB, m_colorA);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        for (size_t i = 0; i < m_text.length() && i < m_glyphMeshes.size(); i++) {
            char c = m_text[i];
            auto it = m_characters.find(c);
            if (it != m_characters.end()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, it->second.TextureID);
                m_shader->setInt("uTextTexture", 0);
                m_glyphMeshes[i]->draw();
            }
        }
        
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    float getTextHeight() const {
        float maxHeight = 0;
        for (char c : m_text) {
            auto it = m_characters.find(c);
            if (it != m_characters.end()) {
                maxHeight = std::max(maxHeight, it->second.SizeY);
            }
        }
        return maxHeight * m_scale;
    }
    
    float getTextWidth() const {
        float width = 0;
        for (char c : m_text) {
            auto it = m_characters.find(c);
            if (it != m_characters.end()) {
                width += (it->second.Advance >> 6) * m_scale;
            }
        }
        return width;
    }
};