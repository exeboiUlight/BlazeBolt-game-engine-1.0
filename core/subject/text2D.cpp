#include "text2D.hpp"
#include <vector>
#include <algorithm>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace BlazeBolt {
    struct TextRhiVertex {
        float x, y;
        float u, v;
    };

    FreeType::FreeType() : freetype(nullptr) {
        if (FT_Init_FreeType(&this->freetype)) {
            fprintf(stderr, "Failed to initialize FreeType library\n");
        }
    }
    FreeType::~FreeType() {
        if (this->freetype == nullptr) {
            return;
        }
        FT_Done_FreeType(this->freetype);
    }
    bool FreeType::isValid() const {
        return this->freetype != nullptr;
    }
    const FT_Library FreeType::get() const {
        return this->freetype;
    }

    Font::Font(const FreeType &freeType, const std::string &fontPath) : glyphInfos(), textureAtlas(), rhiTextureAtlas(nullptr), valid(false) {
        FT_Face face = nullptr;
        if (FT_New_Face(freeType.get(), fontPath.c_str(), 0, &face) != FT_Err_Ok || face == nullptr) {
            fprintf(stderr, "Failed to load font: %s\n", fontPath.c_str());
            return;
        }

        constexpr float fontHeight = 72.0f;
        FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(fontHeight));

        uint32_t currentX = 0, currentY = 0, rowHeight = 0;
        const uint32_t atlasWidth = 512;
        uint32_t atlasHeight = 0;
        for (char i = 0; i < std::numeric_limits<char>::max(); i++) {
            if (FT_Load_Char(face, i, FT_LOAD_DEFAULT) != FT_Err_Ok) {
                continue;
            }

            uint32_t width = face->glyph->bitmap.width + 2;
            uint32_t height = face->glyph->bitmap.rows + 2;
            if (currentX + width >= atlasWidth) {
                currentX = 0;
                currentY += rowHeight;
                rowHeight = 0;
            }
            if (height > rowHeight) {
                rowHeight = height;
            }
            if (currentY + height >= atlasHeight) {
                atlasHeight = currentY + height;
            }

            const float glyphHeight = static_cast<float>(face->glyph->bitmap.rows) / fontHeight;
            this->glyphInfos.try_emplace(i, GlyphInfo {
                .size = Vector2(
                    static_cast<float>(face->glyph->bitmap.width) / fontHeight,
                    glyphHeight
                ),
                .bearing = Vector2(
                    static_cast<float>(face->glyph->bitmap_left) / fontHeight,
                    -glyphHeight + static_cast<float>(face->glyph->bitmap_top) / fontHeight
                ),
                .uvOffset = Vector2(static_cast<float>(currentX) / atlasWidth, static_cast<float>(currentY)),
                .uvSize = Vector2(static_cast<float>(face->glyph->bitmap.width) / atlasWidth, static_cast<float>(face->glyph->bitmap.rows)),
                .advance = static_cast<float>(static_cast<unsigned int>(face->glyph->advance.x) >> 6) / fontHeight
            });
            currentX += width;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        this->textureAtlas.bind();
        std::vector<GLubyte> zeroData(atlasWidth * atlasHeight, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, zeroData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        for (auto &[character, glyphInfo] : this->glyphInfos) {
            glyphInfo.uvOffset.y /= atlasHeight;
            glyphInfo.uvSize.y /= atlasHeight;
            if (FT_Load_Char(face, character, FT_LOAD_RENDER) != FT_Err_Ok) { continue; }
            glTexSubImage2D(
                GL_TEXTURE_2D, 0,
                static_cast<GLint>(glyphInfo.uvOffset.x * atlasWidth),
                static_cast<GLint>(glyphInfo.uvOffset.y * atlasHeight),
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                GL_RED, GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
        }

        FT_Done_Face(face);
        this->valid = true;
    }

    Font::Font(const FreeType &freeType, const std::string &fontPath, IRenderDevice* device) : glyphInfos(), textureAtlas(), rhiTextureAtlas(nullptr), valid(false) {
        FT_Face face = nullptr;
        if (FT_New_Face(freeType.get(), fontPath.c_str(), 0, &face) != FT_Err_Ok || face == nullptr) {
            fprintf(stderr, "Failed to load font: %s\n", fontPath.c_str());
            return;
        }

        constexpr float fontHeight = 72.0f;
        FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(fontHeight));

        uint32_t currentX = 0, currentY = 0, rowHeight = 0;
        const uint32_t atlasWidth = 512;
        uint32_t atlasHeight = 0;
        for (char i = 0; i < std::numeric_limits<char>::max(); i++) {
            if (FT_Load_Char(face, i, FT_LOAD_DEFAULT) != FT_Err_Ok) {
                continue;
            }

            uint32_t width = face->glyph->bitmap.width + 2;
            uint32_t height = face->glyph->bitmap.rows + 2;
            if (currentX + width >= atlasWidth) {
                currentX = 0;
                currentY += rowHeight;
                rowHeight = 0;
            }
            if (height > rowHeight) {
                rowHeight = height;
            }
            if (currentY + height >= atlasHeight) {
                atlasHeight = currentY + height;
            }

            const float glyphHeight = static_cast<float>(face->glyph->bitmap.rows) / fontHeight;
            this->glyphInfos.try_emplace(i, GlyphInfo {
                .size = Vector2(
                    static_cast<float>(face->glyph->bitmap.width) / fontHeight,
                    glyphHeight
                ),
                .bearing = Vector2(
                    static_cast<float>(face->glyph->bitmap_left) / fontHeight,
                    -glyphHeight + static_cast<float>(face->glyph->bitmap_top) / fontHeight
                ),
                .uvOffset = Vector2(static_cast<float>(currentX) / atlasWidth, static_cast<float>(currentY)),
                .uvSize = Vector2(static_cast<float>(face->glyph->bitmap.width) / atlasWidth, static_cast<float>(face->glyph->bitmap.rows)),
                .advance = static_cast<float>(static_cast<unsigned int>(face->glyph->advance.x) >> 6) / fontHeight
            });
            currentX += width;
        }

        TextureDesc texDesc;
        texDesc.width = atlasWidth;
        texDesc.height = atlasHeight;
        texDesc.format = TextureFormat::R8;
        this->rhiTextureAtlas = device->createTexture(texDesc);

        std::vector<uint8_t> pixels(atlasWidth * atlasHeight, 0);

        for (auto &[character, glyphInfo] : this->glyphInfos) {
            glyphInfo.uvOffset.y /= atlasHeight;
            glyphInfo.uvSize.y /= atlasHeight;
            if (FT_Load_Char(face, character, FT_LOAD_RENDER) != FT_Err_Ok) { continue; }

            uint32_t destX = static_cast<uint32_t>(glyphInfo.uvOffset.x * atlasWidth);
            uint32_t destY = static_cast<uint32_t>(glyphInfo.uvOffset.y * atlasHeight);
            for (unsigned int row = 0; row < face->glyph->bitmap.rows; row++) {
                memcpy(&pixels[(destY + row) * atlasWidth + destX],
                       &face->glyph->bitmap.buffer[row * face->glyph->bitmap.width],
                       face->glyph->bitmap.width);
            }
        }

        this->rhiTextureAtlas->upload(pixels.data());

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        this->textureAtlas.bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        FT_Done_Face(face);
        this->valid = true;
    }

    Font::~Font() {
    }

    const GlyphInfo *Font::getGlyphInfo(char character) const {
        std::unordered_map<char, GlyphInfo>::const_iterator it = this->glyphInfos.find(character);
        if (it != this->glyphInfos.end()) {
            return &it->second;
        }
        return nullptr;
    }
    const GL::Texture2D &Font::getTextureAtlas() const {
        return this->textureAtlas;
    }
    ITexture* Font::getTextureAtlasRHI() const {
        return this->rhiTextureAtlas;
    }
    bool Font::isValid() const {
        return this->valid;
    }

    FontShader2D::FontShader2D() : shaderProgram(), pipeline(nullptr) {
        constexpr GLchar vertexShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 a_Position;
            layout (location = 1) in vec4 i_Transform;
            layout (location = 2) in vec4 i_UV;
            layout (location = 0) out vec2 v_TexCoord;

            uniform float u_AspectRatio;
            uniform mat3 u_MVPMatrix;

            void main() {
                vec3 transformed = u_MVPMatrix * vec3(a_Position * i_Transform.zw + i_Transform.xy, 1.0);
                gl_Position = vec4(transformed, 1.0);
                gl_Position.x /= u_AspectRatio;
                v_TexCoord = vec2(a_Position.x, 1.0f - a_Position.y) * i_UV.zw + i_UV.xy;
            }
        )";
        constexpr GLchar fragmentShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 v_TexCoord;
            layout (location = 0) out vec4 f_Color;

            uniform sampler2D u_Texture;
            uniform vec4 u_Color;

            void main() {
                f_Color = vec4(u_Color.rgb, u_Color.a * texture(u_Texture, v_TexCoord).r);
            }
        )";

        {
            std::optional<GL::Shader> vertexShader = GL::Shader::fromSource(GL_VERTEX_SHADER, vertexShaderSource);
            if (!vertexShader.has_value()) { return; }

            std::optional<GL::Shader> fragmentShader = GL::Shader::fromSource(GL_FRAGMENT_SHADER, fragmentShaderSource);
            if (!fragmentShader.has_value()) { return; }

            glAttachShader(this->shaderProgram.get(), vertexShader->get());
            glAttachShader(this->shaderProgram.get(), fragmentShader->get());
            if (!this->shaderProgram.tryToLink()) { return; }
        }

        this->shaderProgram.bind();
        glUniform1i(glGetUniformLocation(this->shaderProgram.get(), "u_Texture"), 0);
    }

    FontShader2D::FontShader2D(IRenderDevice* device) : shaderProgram(), pipeline(nullptr) {
        constexpr GLchar vertexShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 a_Position;
            layout (location = 1) in vec2 a_TexCoord;
            layout (location = 0) out vec2 v_TexCoord;

            uniform float u_AspectRatio;
            uniform mat3 u_MVPMatrix;

            void main() {
                vec3 transformed = u_MVPMatrix * vec3(a_Position, 1.0);
                gl_Position = vec4(transformed, 1.0);
                gl_Position.x /= u_AspectRatio;
                v_TexCoord = a_TexCoord;
            }
        )";
        constexpr GLchar fragmentShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 v_TexCoord;
            layout (location = 0) out vec4 f_Color;

            uniform sampler2D u_Texture;
            uniform vec4 u_Color;

            void main() {
                f_Color = vec4(u_Color.rgb, u_Color.a * texture(u_Texture, v_TexCoord).r);
            }
        )";

        IShader* shader = device->createShader(vertexShaderSource, fragmentShaderSource);
        if (shader == nullptr) return;

        PipelineDesc desc;
        desc.vertexShader = shader;
        desc.fragmentShader = shader;
        desc.primitive = PrimitiveType::TRIANGLE_FAN;
        desc.vertexStride = sizeof(TextRhiVertex);

        VertexAttrib posAttrib;
        posAttrib.location = 0;
        posAttrib.offset = 0;
        posAttrib.stride = sizeof(TextRhiVertex);
        posAttrib.size = 2;
        posAttrib.normalized = false;
        desc.vertexAttributes.push_back(posAttrib);

        VertexAttrib uvAttrib;
        uvAttrib.location = 1;
        uvAttrib.offset = offsetof(TextRhiVertex, u);
        uvAttrib.stride = sizeof(TextRhiVertex);
        uvAttrib.size = 2;
        uvAttrib.normalized = false;
        desc.vertexAttributes.push_back(uvAttrib);

        desc.blend.enabled = true;
        desc.blend.srcColor = BlendFactor::SRC_ALPHA;
        desc.blend.dstColor = BlendFactor::ONE_MINUS_SRC_ALPHA;

        this->pipeline = device->createPipeline(desc);

        device->destroyShader(shader);

        if (this->pipeline) {
            this->pipeline->setUniform("u_Texture", 0);
        }
    }

    FontShader2D::~FontShader2D() {
    }

    void FontShader2D::bind() const {
        if (this->pipeline != nullptr) {
            this->pipeline->bind();
        } else {
            this->shaderProgram.bind();
        }
    }

    IPipeline* FontShader2D::getPipeline() const {
        return this->pipeline;
    }

    void FontShader2D::setAspectRatio(float aspectRatio) const {
        if (this->pipeline != nullptr) {
            this->pipeline->setUniform("u_AspectRatio", aspectRatio);
        } else {
            glUniform1f(glGetUniformLocation(this->shaderProgram.get(), "u_AspectRatio"), aspectRatio);
        }
    }

    void FontShader2D::setMVPMatrix(const Matrix3x3 &matrix) const {
        if (this->pipeline != nullptr) {
            this->pipeline->setUniformMatrix3("u_MVPMatrix", &matrix.m[0][0]);
        } else {
            glUniformMatrix3fv(glGetUniformLocation(this->shaderProgram.get(), "u_MVPMatrix"), 1, GL_FALSE, &matrix.m[0][0]);
        }
    }

    void FontShader2D::setColor(const Vector4 &color) const {
        if (this->pipeline != nullptr) {
            this->pipeline->setUniform("u_Color", color.x, color.y, color.z, color.w);
        } else {
            glUniform4f(glGetUniformLocation(this->shaderProgram.get(), "u_Color"), color.x, color.y, color.z, color.w);
        }
    }

    Text2D::Text2D(const QuadVertexBufferObject2D &vertexBufferObject, Font &font) :
        font(&font), rhiDevice(nullptr), text(),
        vertexArrayObject(), instanceBufferObject(), rhiInstanceBuffer(nullptr), instanceCount(0),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        position(0.0f, 0.0f), scale(1.0f, 1.0f), rotation(0.0f),
        alignment(Alignment::Left), visible(true)
    {
        this->vertexArrayObject.bind();
        vertexBufferObject.bind();
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertexBufferObject2D::Vertex), nullptr);

        this->instanceBufferObject.bind(GL_ARRAY_BUFFER);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        glVertexAttribDivisor(1, 1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(4 * sizeof(float)));
        glVertexAttribDivisor(2, 1);
    }

    Text2D::~Text2D() {
    }

    void Text2D::updateModelMatrix() {
        Matrix3x3 translationMatrix = Matrix3x3::translation(this->position.x, this->position.y);
        Matrix3x3 scaleMatrix = Matrix3x3::scale(this->scale.x, this->scale.y);
        Matrix3x3 rotationMatrix = Matrix3x3::rotation(this->rotation);
        Matrix3x3 originTranslationMatrix = Matrix3x3::translation(-this->origin.x * this->getStringWidth(), -this->origin.y * this->getStringHeight());
        this->modelMatrix = translationMatrix * rotationMatrix * scaleMatrix * originTranslationMatrix;
    }

    void Text2D::draw(const FontShader2D &fontShader, const Matrix3x3 &projectionViewMatrix) const {
        if (this->instanceCount == 0 || !this->visible) { return; }
        fontShader.bind();
        fontShader.setMVPMatrix(projectionViewMatrix * this->modelMatrix);
        fontShader.setColor(this->color);

        GL::setActiveTextureUnit(0);
        this->font->getTextureAtlas().bind();
        this->vertexArrayObject.bind();
        glDrawArraysInstanced(QuadVertexBufferObject2D::DRAW_MODE, 0, QuadVertexBufferObject2D::VERTEX_COUNT, this->instanceCount);
    }

    void Text2D::draw(IRenderContext* context, const FontShader2D &fontShader, const Matrix3x3 &projectionViewMatrix) const {
        if (this->instanceCount == 0 || !this->visible) { return; }
        if (this->rhiInstanceBuffer == nullptr) return;

        IPipeline* p = fontShader.getPipeline();
        if (p == nullptr) return;

        context->bindPipeline(p);
        Matrix3x3 mvp = projectionViewMatrix * this->modelMatrix;
        p->setUniformMatrix3("u_MVPMatrix", &mvp.m[0][0]);
        p->setUniform("u_Color", this->color.x, this->color.y, this->color.z, this->color.w);

        ITexture* atlas = this->font != nullptr ? this->font->getTextureAtlasRHI() : nullptr;
        if (atlas != nullptr) {
            context->bindTexture(0, atlas);
        }

        context->bindVertexBuffer(this->rhiInstanceBuffer, sizeof(TextRhiVertex));
        context->draw(this->instanceCount * 4);
    }

    void Text2D::setColor(float r, float g, float b, float a) {
        this->color.x = r;
        this->color.y = g;
        this->color.z = b;
        this->color.w = a;
    }
    void Text2D::setColor(const Vector4 &color) {
        this->color = color;
    }
    const Vector4 &Text2D::getColor() const {
        return this->color;
    }

    void Text2D::setPosition(float x, float y) {
        this->position.x = x;
        this->position.y = y;
        this->updateModelMatrix();
    }
    void Text2D::setPosition(const Vector2 &position) {
        this->position = position;
        this->updateModelMatrix();
    }
    const Vector2 &Text2D::getPosition() const {
        return this->position;
    }

    void Text2D::setScale(const Vector2 &scale) {
        this->scale = scale;
        this->updateModelMatrix();
    }
    void Text2D::setScale(float width, float height) {
        this->scale.x = width;
        this->scale.y = height;
        this->updateModelMatrix();
    }
    const Vector2 &Text2D::getScale() const {
        return this->scale;
    }

    void Text2D::setOrigin(const Vector2 &origin) {
        this->origin.x = origin.x;
        this->origin.y = origin.y;
        this->updateModelMatrix();
    }
    void Text2D::setOrigin(float x, float y) {
        this->origin.x = x;
        this->origin.y = y;
        this->updateModelMatrix();
    }
    const Vector2 &Text2D::getOrigin() const {
        return this->origin;
    }

    void Text2D::setRotation(float rotation) {
        this->rotation = rotation;
        this->updateModelMatrix();
    }
    float Text2D::getRotation() const {
        return this->rotation;
    }

    void Text2D::setAlignment(Alignment alignment) {
        this->alignment = alignment;
        this->setText(this->text);
    }
    Text2D::Alignment Text2D::getAlignment() const {
        return this->alignment;
    }

    void Text2D::setText(const std::string &text) {
        if (this->font == nullptr || !this->font->isValid()) {
            fprintf(stderr, "Cannot set text for Text2D: font is not set or invalid\n");
            return;
        }
        this->text = text;
        if (this->text.empty()) {
            this->instanceBufferObject.bind(GL_ARRAY_BUFFER);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
            this->instanceCount = 0;
            return;
        }

        float x = 0.0f, y = fmaxf(static_cast<float>(std::count(this->text.begin(), this->text.end(), '\n')), 0.0f);
        float totalWidth = 0.0f;
        std::vector<float> instanceData = std::vector<float>();
        instanceData.reserve(this->text.size() * 8);
        std::vector<float> linesWidths = std::vector<float>();
        for (char character : this->text) {
            if (character == '\n') {
                y -= 1.0f;
                linesWidths.push_back(x);
                x = 0.0f;
                continue;
            }
            const GlyphInfo *glyphInfo = this->font->getGlyphInfo(character);
            if (glyphInfo == nullptr) {
                fprintf(stderr, "Character '%c' is not available in the font\n", character);
                continue;
            }
            instanceData.push_back(x + glyphInfo->bearing.x);
            instanceData.push_back(y + glyphInfo->bearing.y);
            instanceData.push_back(glyphInfo->size.x);
            instanceData.push_back(glyphInfo->size.y);
            instanceData.push_back(glyphInfo->uvOffset.x);
            instanceData.push_back(glyphInfo->uvOffset.y);
            instanceData.push_back(glyphInfo->uvSize.x);
            instanceData.push_back(glyphInfo->uvSize.y);
            x += glyphInfo->advance;
            totalWidth = fmaxf(x, totalWidth);
        }
        linesWidths.push_back(x);
        size_t lineToProcess = 0, instanceDataIndex = 0;
        for (size_t i = 0; i < this->text.size(); i++) {
            if (this->text[i] == '\n') {
                lineToProcess++;
                continue;
            }
            if (this->font->getGlyphInfo(text[i]) == nullptr) { continue; }
            if (this->alignment == Alignment::Center) {
                instanceData[instanceDataIndex] = (totalWidth - linesWidths[lineToProcess]) * 0.5f + instanceData[instanceDataIndex];
            } else if (this->alignment == Alignment::Right) {
                instanceData[instanceDataIndex] = totalWidth - linesWidths[lineToProcess] + instanceData[instanceDataIndex];
            }
            instanceDataIndex += 8;
        }

        this->instanceBufferObject.bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);
        this->instanceCount = static_cast<GLsizei>(text.size());

        if (this->rhiInstanceBuffer == nullptr && this->rhiDevice != nullptr) {
            size_t bufSize = this->text.size() * 4 * sizeof(TextRhiVertex);
            if (bufSize == 0) bufSize = 256 * 4 * sizeof(TextRhiVertex);
            BufferDesc desc;
            desc.size = static_cast<uint32_t>(bufSize);
            desc.type = BufferType::VERTEX;
            desc.usage = BufferUsage::DYNAMIC;
            this->rhiInstanceBuffer = this->rhiDevice->createBuffer(desc);
        }

        if (this->rhiInstanceBuffer != nullptr) {
            std::vector<TextRhiVertex> expandedVerts;
            expandedVerts.reserve(this->text.size() * 4);

            size_t idx = 0;
            for (size_t i = 0; i < this->text.size(); i++) {
                if (this->text[i] == '\n') continue;
                const GlyphInfo *glyphInfo = this->font->getGlyphInfo(text[i]);
                if (glyphInfo == nullptr) continue;

                float charX = instanceData[idx];
                float charY = instanceData[idx + 1];
                float charW = instanceData[idx + 2];
                float charH = instanceData[idx + 3];
                float uvOffX = instanceData[idx + 4];
                float uvOffY = instanceData[idx + 5];
                float uvSizeX = instanceData[idx + 6];
                float uvSizeY = instanceData[idx + 7];
                idx += 8;

                static const float quadVerts[4][2] = {
                    {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
                };

                for (int j = 0; j < 4; j++) {
                    TextRhiVertex v;
                    v.x = charX + quadVerts[j][0] * charW;
                    v.y = charY + quadVerts[j][1] * charH;
                    v.u = uvOffX + quadVerts[j][0] * uvSizeX;
                    v.v = uvOffY + (1.0f - quadVerts[j][1]) * uvSizeY;
                    expandedVerts.push_back(v);
                }
            }

            this->rhiInstanceBuffer->upload(expandedVerts.data(), expandedVerts.size() * sizeof(TextRhiVertex), 0);
        }
    }
    const std::string &Text2D::getText() const {
        return this->text;
    }

    float Text2D::getStringWidth() const {
        if (this->text.empty() || this->font == nullptr || !this->font->isValid()) {
            return 0.0f;
        }

        float maxWidth = 0.0f, currentWidth = 0.0f;
        for (char character : this->text) {
            if (character == '\n') {
                if (currentWidth > maxWidth) {
                    maxWidth = currentWidth;
                }
                currentWidth = 0.0f;
                continue;
            }
            const GlyphInfo *glyphInfo = this->font->getGlyphInfo(character);
            if (glyphInfo != nullptr) {
                currentWidth += glyphInfo->advance;
            }
        }
        if (currentWidth > maxWidth) {
            maxWidth = currentWidth;
        }
        return maxWidth;
    }
    float Text2D::getStringHeight() const {
        if (this->text.empty() || this->font == nullptr || !this->font->isValid()) {
            return 0.0f;
        }
        return static_cast<float>(1 + std::count(this->text.begin(), this->text.end(), '\n'));
    }

    void Text2D::setFont(Font &font) {
        this->font = &font;
    }
    Font *Text2D::getFont() const {
        return this->font;
    }

    void Text2D::setDevice(IRenderDevice* device) {
        this->rhiDevice = device;
        if (this->rhiDevice != nullptr && this->rhiInstanceBuffer == nullptr) {
            size_t bufSize = (this->text.empty() ? 256 : this->text.size()) * 4 * sizeof(TextRhiVertex);
            BufferDesc desc;
            desc.size = static_cast<uint32_t>(bufSize > 0 ? bufSize : 256 * 4 * sizeof(TextRhiVertex));
            desc.type = BufferType::VERTEX;
            desc.usage = BufferUsage::DYNAMIC;
            this->rhiInstanceBuffer = this->rhiDevice->createBuffer(desc);
        }
    }

    IRenderDevice* Text2D::getDevice() const {
        return this->rhiDevice;
    }

    void Text2D::setVisible(bool visible) {
        this->visible = visible;
    }
    bool Text2D::isVisible() const {
        return this->visible;
    }
}
