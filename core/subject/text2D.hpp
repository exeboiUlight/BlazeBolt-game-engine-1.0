#pragma once
#include <unordered_map>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <graphics/gl.hpp>
#include <graphics/quad.hpp>
#include <utils/math/vector.h>

// struct Character {
    
//     float SizeX, SizeY;
//     float BearingX, BearingY;
//     unsigned int Advance;
// };

namespace BlazeBolt {
    struct GlyphInfo {
        Vector2 size, bearing, uvOffset, uvSize;
        float advance;
    };
    struct FreeType {
    private:
        FT_Library freetype;
    public:
        FreeType();
        ~FreeType();

        bool isValid() const;
        const FT_Library get() const;
    };
    struct Font {
    private:
        std::unordered_map<char, GlyphInfo> glyphInfos;
        GL::Texture2D textureAtlas;
        bool valid;
    public:
        Font(const FreeType &freeType, const std::string &fontPath);
        ~Font() = default;

        const GlyphInfo *getGlyphInfo(char character) const;
        const GL::Texture2D &getTextureAtlas() const;
        bool isValid() const;
    };
    struct FontShader2D {
    private:
        GL::ShaderProgram shaderProgram;
    public:
        FontShader2D();
        ~FontShader2D() = default;

        void bind() const;
        void setAspectRatio(float aspectRatio) const;
        void setMVPMatrix(const Matrix3x3 &matrix) const;
        void setColor(const Vector4 &color) const;
    };

    // TODO: Either implement ECS or a Node (OOP) system, so parts of the objects like visibility, transform and rendering could be part of a base, e.g. "Node2D/Object2D" class, and just add a custom objects, or just have an object with Transform2D component, Text2D/Sprite2D component, etc.
    struct Text2D {
    public:
        enum class Alignment : uint8_t { Left, Center, Right };
    private:
        Font *font;
        std::string text;
        GL::VertexArrayObject vertexArrayObject;
        GL::VertexBufferObject instanceBufferObject;
        GLsizei instanceCount;

        Matrix3x3 modelMatrix;
        Vector4 color;
        Vector2 position, scale, origin;
        float rotation;
        Alignment alignment;
        bool visible;

        void updateModelMatrix();
    public:
        Text2D(const QuadVertexBufferObject2D &vertexBufferObject, Font &font);
        ~Text2D() = default;
        void draw(const FontShader2D &fontShader, const Matrix3x3 &projectionViewMatrix) const;

        void setColor(float r, float g, float b, float a);
        void setColor(const Vector4 &color);
        const Vector4 &getColor() const;

        void setPosition(float x, float y);
        void setPosition(const Vector2 &position);
        const Vector2 &getPosition() const;

        void setScale(const Vector2 &scale);
        void setScale(float width, float height);
        const Vector2 &getScale() const;

        void setOrigin(float x, float y);
        void setOrigin(const Vector2 &origin);
        const Vector2 &getOrigin() const;

        void setRotation(float rotation);
        float getRotation() const;

        void setAlignment(Alignment alignment);
        Alignment getAlignment() const;

        void setText(const std::string &text);
        const std::string &getText() const;

        float getStringWidth() const;
        float getStringHeight() const;

        void setFont(Font &font);
        Font *getFont() const;

        void setVisible(bool visible);
        bool isVisible() const;
    };
}