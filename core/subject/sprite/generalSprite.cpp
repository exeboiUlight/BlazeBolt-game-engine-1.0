#include "generalSprite.hpp"

namespace BlazeBolt {
    SpriteMesh::SpriteMesh() : vertexArrayObject() {}
    SpriteMesh::SpriteMesh(const QuadVertexBufferObject2D &vertexBufferObject) : vertexArrayObject() {
        this->setVertexBuffer(vertexBufferObject);
    }
    void SpriteMesh::setVertexBuffer(const QuadVertexBufferObject2D &vbo) {
        this->vertexArrayObject.bind();
        vbo.bind();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertexBufferObject2D::Vertex), nullptr);
        glEnableVertexAttribArray(0);
    }
    void SpriteMesh::draw() const {
        this->vertexArrayObject.bind();
        glDrawArrays(QuadVertexBufferObject2D::DRAW_MODE, 0, QuadVertexBufferObject2D::VERTEX_COUNT);
    }

    SpriteShader2D::SpriteShader2D() : shaderProgram() {
        constexpr GLchar vertexShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 a_Position;
            layout (location = 0) out vec2 v_TexCoord;
            layout (location = 1) out vec2 v_WorldPos;

            uniform float u_AspectRatio;
            uniform mat3 u_MVPMatrix;
            uniform vec4 u_TexRect;
            uniform vec2 u_WorldPos;

            void main() {
                vec3 transformed = u_MVPMatrix * vec3(a_Position, 1.0);
                gl_Position = vec4(transformed.xy, 0.0, 1.0);
                gl_Position.x /= u_AspectRatio;
                v_TexCoord = a_Position * u_TexRect.zw + u_TexRect.xy;
                v_TexCoord.y = 1.0 - v_TexCoord.y;
                v_WorldPos = u_WorldPos;
            }
        )";
        constexpr GLchar fragmentShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 v_TexCoord;
            layout (location = 1) in vec2 v_WorldPos;
            layout (location = 0) out vec4 f_Color;

            uniform sampler2D u_Texture;
            uniform vec4 u_Color;

            #define MAX_LIGHTS 8
            uniform int u_LightCount;
            uniform vec2 u_LightPos[MAX_LIGHTS];
            uniform vec3 u_LightColor[MAX_LIGHTS];
            uniform float u_LightIntensity[MAX_LIGHTS];
            uniform float u_LightRadius[MAX_LIGHTS];
            uniform vec3 u_AmbientColor;
            uniform float u_AmbientIntensity;

            void main() {
                vec4 texColor = texture(u_Texture, v_TexCoord) * u_Color;
                vec3 lighting = u_AmbientColor * u_AmbientIntensity;
                for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; i++) {
                    float dist = distance(v_WorldPos, u_LightPos[i]);
                    float radius = u_LightRadius[i];
                    float att = clamp(1.0 - dist / radius, 0.0, 1.0);
                    att = att * att;
                    lighting += u_LightColor[i] * u_LightIntensity[i] * att;
                }
                f_Color = vec4(texColor.rgb * lighting, texColor.a);
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

        printf("Sprite2D shader program compiled and linked successfully\n");
    }
    void SpriteShader2D::bind() const {
        this->shaderProgram.bind();
    }
    void SpriteShader2D::setAspectRatio(float aspectRatio) const {
        glUniform1f(glGetUniformLocation(this->shaderProgram.get(), "u_AspectRatio"), aspectRatio);
    }
    void SpriteShader2D::setMVPMatrix(const Matrix3x3 &matrix) const {
        // FIXME: Might not work, but the "toFloatArray" method must be removed anyways
        glUniformMatrix3fv(glGetUniformLocation(this->shaderProgram.get(), "u_MVPMatrix"), 1, GL_FALSE, &matrix.m[0][0]);
    }
    void SpriteShader2D::setColor(const Vector4 &color) const {
        glUniform4f(glGetUniformLocation(this->shaderProgram.get(), "u_Color"), color.x, color.y, color.z, color.w);
    }
    void SpriteShader2D::setTextureRect(const Vector4 &rect) const {
        glUniform4f(glGetUniformLocation(this->shaderProgram.get(), "u_TexRect"), rect.x, rect.y, rect.z, rect.w);
    }
    void SpriteShader2D::setWorldPos(const Vector2 &pos) const {
        glUniform2f(glGetUniformLocation(this->shaderProgram.get(), "u_WorldPos"), pos.x, pos.y);
    }
    void SpriteShader2D::setLightData(int count, const float* positions, const float* colors, const float* intensities, const float* radii, const Vector3& ambientColor, float ambientIntensity) const {
        glUniform1i(glGetUniformLocation(this->shaderProgram.get(), "u_LightCount"), count);
        if (count > 0) {
            glUniform2fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightPos"), count, positions);
            glUniform3fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightColor"), count, colors);
            glUniform1fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightIntensity"), count, intensities);
            glUniform1fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightRadius"), count, radii);
        }
        glUniform3f(glGetUniformLocation(this->shaderProgram.get(), "u_AmbientColor"), ambientColor.x, ambientColor.y, ambientColor.z);
        glUniform1f(glGetUniformLocation(this->shaderProgram.get(), "u_AmbientIntensity"), ambientIntensity);
    }

    SpriteBatchShader2D::SpriteBatchShader2D() : shaderProgram() {
        constexpr GLchar vertexShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 a_Position;
            layout (location = 1) in vec2 a_TexCoord;
            layout (location = 2) in vec4 a_Color;
            layout (location = 0) out vec2 v_TexCoord;
            layout (location = 1) out vec4 v_Color;
            layout (location = 2) out vec2 v_WorldPos;

            uniform float u_AspectRatio;
            uniform mat3 u_MVPMatrix;

            void main() {
                vec3 transformed = u_MVPMatrix * vec3(a_Position, 1.0);
                gl_Position = vec4(transformed.xy, 0.0, 1.0);
                gl_Position.x /= u_AspectRatio;
                v_TexCoord = a_TexCoord;
                v_TexCoord.y = 1.0 - v_TexCoord.y;
                v_Color = a_Color;
                v_WorldPos = a_Position;
            }
        )";
        constexpr GLchar fragmentShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 v_TexCoord;
            layout (location = 1) in vec4 v_Color;
            layout (location = 2) in vec2 v_WorldPos;
            layout (location = 0) out vec4 f_Color;

            uniform sampler2D u_Texture;

            #define MAX_LIGHTS 8
            uniform int u_LightCount;
            uniform vec2 u_LightPos[MAX_LIGHTS];
            uniform vec3 u_LightColor[MAX_LIGHTS];
            uniform float u_LightIntensity[MAX_LIGHTS];
            uniform float u_LightRadius[MAX_LIGHTS];
            uniform vec3 u_AmbientColor;
            uniform float u_AmbientIntensity;

            void main() {
                vec4 texColor = texture(u_Texture, v_TexCoord) * v_Color;
                vec3 lighting = u_AmbientColor * u_AmbientIntensity;
                for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; i++) {
                    float dist = distance(v_WorldPos, u_LightPos[i]);
                    float radius = u_LightRadius[i];
                    float att = clamp(1.0 - dist / radius, 0.0, 1.0);
                    att = att * att;
                    lighting += u_LightColor[i] * u_LightIntensity[i] * att;
                }
                f_Color = vec4(texColor.rgb * lighting, texColor.a);
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

        printf("SpriteBatch shader program compiled and linked successfully\n");
    }
    void SpriteBatchShader2D::bind() const {
        this->shaderProgram.bind();
    }
    void SpriteBatchShader2D::setAspectRatio(float aspectRatio) const {
        glUniform1f(glGetUniformLocation(this->shaderProgram.get(), "u_AspectRatio"), aspectRatio);
    }
    void SpriteBatchShader2D::setMVPMatrix(const Matrix3x3 &matrix) const {
        glUniformMatrix3fv(glGetUniformLocation(this->shaderProgram.get(), "u_MVPMatrix"), 1, GL_FALSE, &matrix.m[0][0]);
    }
    void SpriteBatchShader2D::setLightData(int count, const float* positions, const float* colors, const float* intensities, const float* radii, const Vector3& ambientColor, float ambientIntensity) const {
        glUniform1i(glGetUniformLocation(this->shaderProgram.get(), "u_LightCount"), count);
        if (count > 0) {
            glUniform2fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightPos"), count, positions);
            glUniform3fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightColor"), count, colors);
            glUniform1fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightIntensity"), count, intensities);
            glUniform1fv(glGetUniformLocation(this->shaderProgram.get(), "u_LightRadius"), count, radii);
        }
        glUniform3f(glGetUniformLocation(this->shaderProgram.get(), "u_AmbientColor"), ambientColor.x, ambientColor.y, ambientColor.z);
        glUniform1f(glGetUniformLocation(this->shaderProgram.get(), "u_AmbientIntensity"), ambientIntensity);
    }
}