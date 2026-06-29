#pragma once
#include <unordered_map>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <graphics/gl.hpp>
#include <graphics/renderer/Texture.h>
#include <graphics/renderer/Buffer.h>
#include <graphics/renderer/Pipeline.h>
#include <graphics/renderer/RenderContext.h>
#include <graphics/renderer/RenderDevice.h>
#include <graphics/quad.hpp>
#include <utils/math/vector.h>

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
        ITexture* rhiTextureAtlas;
        bool valid;
    public:
        Font(const FreeType &freeType, const std::string &fontPath);
        Font(const FreeType &freeType, const std::string &fontPath, IRenderDevice* device);
        ~Font();

        const GlyphInfo *getGlyphInfo(char character) const;
        const GL::Texture2D &getTextureAtlas() const;
        ITexture* getTextureAtlasRHI() const;
        bool isValid() const;
    };
    struct FontShader2D {
    private:
        GL::ShaderProgram shaderProgram;
        IPipeline* pipeline;
    public:
        FontShader2D();
        FontShader2D(IRenderDevice* device);
        ~FontShader2D();

        void bind() const;
        IPipeline* getPipeline() const;
        void setAspectRatio(float aspectRatio) const;
        void setMVPMatrix(const Matrix3x3 &matrix) const;
        void setColor(const Vector4 &color) const;
    };

    struct Text2D {
    public:
        enum class Alignment : uint8_t { Left, Center, Right };
    private:
        Font *font;
        IRenderDevice* rhiDevice;
        std::string text;
        GL::VertexArrayObject vertexArrayObject;
        GL::VertexBufferObject instanceBufferObject;
        IBuffer* rhiInstanceBuffer;
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
        ~Text2D();
        void draw(const FontShader2D &fontShader, const Matrix3x3 &projectionViewMatrix) const;
        void draw(IRenderContext* context, const FontShader2D &fontShader, const Matrix3x3 &projectionViewMatrix) const;

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

        void setDevice(IRenderDevice* device);
        IRenderDevice* getDevice() const;

        void setVisible(bool visible);
        bool isVisible() const;
    };
}
