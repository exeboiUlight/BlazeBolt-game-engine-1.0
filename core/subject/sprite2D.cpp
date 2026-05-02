#include "sprite2D.hpp"

namespace BlazeBolt {
    SpriteMesh2D::SpriteMesh2D() : vao(), vbo() {
        constexpr float vertices[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
        this->vao.bind();
        this->vbo.bind();
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
    }
    void SpriteMesh2D::draw() const {
        this->vao.bind();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    SpriteShader2D::SpriteShader2D() : shaderProgram() {
        constexpr GLchar vertexShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 a_Position;

            uniform mat3 u_MVPMatrix;
            layout (location = 0) out vec2 v_TexCoord;

            void main() {
                vec3 transformed = u_MVPMatrix * vec3(a_Position, 1.0);
                gl_Position = vec4(transformed.xy, 0.0, 1.0);
                v_TexCoord = a_Position;
            }
        )";
        constexpr GLchar fragmentShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 v_TexCoord;
            layout (location = 0) out vec4 f_Color;

            uniform sampler2D u_Texture;
            uniform vec4 u_Color;

            void main() {
                f_Color = texture(u_Texture, v_TexCoord) * u_Color;
            }
        )";

        {
            GL::Shader vertexShader = GL::Shader(GL_VERTEX_SHADER);
            const GLchar *vertexShaderSourcePtr[] = { vertexShaderSource };
            glShaderSource(vertexShader.get(), 1, vertexShaderSourcePtr, nullptr);
            glCompileShader(vertexShader.get());
            GLint success = 0;
            glGetShaderiv(vertexShader.get(), GL_COMPILE_STATUS, &success);
            if (success == GL_FALSE) {
                GLint logLength = 0;
                glGetShaderiv(vertexShader.get(), GL_INFO_LOG_LENGTH, &logLength);
                std::string log = std::string(logLength, ' ');
                glGetShaderInfoLog(vertexShader.get(), logLength, nullptr, log.data());
                fprintf(stderr, "Failed to compile Sprite2D vertex shader:\n%s\n", log.c_str());
                return;
            }
            
            GL::Shader fragmentShader = GL::Shader(GL_FRAGMENT_SHADER);
            const GLchar *fragmentShaderSourcePtr[] = { fragmentShaderSource };
            glShaderSource(fragmentShader.get(), 1, fragmentShaderSourcePtr, nullptr);
            glCompileShader(fragmentShader.get());
            success = 0;
            glGetShaderiv(fragmentShader.get(), GL_COMPILE_STATUS, &success);
            if (success == GL_FALSE) {
                GLint logLength = 0;
                glGetShaderiv(fragmentShader.get(), GL_INFO_LOG_LENGTH, &logLength);
                std::string log = std::string(logLength, ' ');
                glGetShaderInfoLog(fragmentShader.get(), logLength, nullptr, log.data());
                fprintf(stderr, "Failed to compile Sprite2D fragment shader:\n%s\n", log.c_str());
                return;
            }

            glAttachShader(this->shaderProgram.get(), vertexShader.get());
            glAttachShader(this->shaderProgram.get(), fragmentShader.get());
            glLinkProgram(this->shaderProgram.get());
            success = 0;
            glGetProgramiv(this->shaderProgram.get(), GL_LINK_STATUS, &success);
            if (success == GL_FALSE) {
                GLint logLength = 0;
                glGetProgramiv(this->shaderProgram.get(), GL_INFO_LOG_LENGTH, &logLength);
                std::string log = std::string(logLength, ' ');
                glGetProgramInfoLog(this->shaderProgram.get(), logLength, nullptr, log.data());
                fprintf(stderr, "Failed to link Sprite2D shader program:\n%s\n", log.c_str());
                return;
            }
        }

        this->shaderProgram.bind();
        glUniform1i(glGetUniformLocation(this->shaderProgram.get(), "u_Texture"), 0);

        printf("Sprite2D shader program compiled and linked successfully\n");
    }
    void SpriteShader2D::bind() const {
        this->shaderProgram.bind();
    }
    void SpriteShader2D::setMVPMatrix(const Matrix3x3 &matrix) const {
        // FIXME: Might not work, but the "toFloatArray" method must be removed anyways
        glUniformMatrix3fv(glGetUniformLocation(this->shaderProgram.get(), "u_MVPMatrix"), 1, GL_FALSE, &matrix.m[0][0]);
    }
    void SpriteShader2D::setColor(const Vector4 &color) const {
        glUniform4f(glGetUniformLocation(this->shaderProgram.get(), "u_Color"), color.x, color.y, color.z, color.w);
    }

    Sprite2D::Sprite2D() :
        texture(nullptr), modelMatrix(), color(1.0f, 1.0f, 1.0f, 1.0f),
        position(), scale(1.0f, 1.0f), origin(0.5f, 0.5f), rotation(0.0f),
        visible(true)
    {
        this->updateModelMatrix();
    }

    void Sprite2D::updateModelMatrix() {
        Matrix3x3 translationMatrix = Matrix3x3::translation(this->position.x, this->position.y);
        Matrix3x3 rotationMatrix = Matrix3x3::rotation(this->rotation);
        Matrix3x3 scaleMatrix = Matrix3x3::scale(this->scale.x, this->scale.y);
        Matrix3x3 originTranslationMatrix = Matrix3x3::translation(-this->origin.x, -this->origin.y);
        this->modelMatrix = translationMatrix * rotationMatrix * scaleMatrix * originTranslationMatrix;
    }

    void Sprite2D::setTexture(const GL::Texture2D &texture) {
        this->texture = &texture;
    }
    void Sprite2D::setPosition(float x, float y) {
        this->position.x = x;
        this->position.y = y;
        this->updateModelMatrix();
    }
    void Sprite2D::setPosition(const Vector2 &position) {
        this->position = position;
        this->updateModelMatrix();
    }
    const Vector2 &Sprite2D::getPosition() const {
        return this->position;
    }
    
    void Sprite2D::setScale(float width, float height) {
        this->scale.x = width;
        this->scale.y = height;
        this->updateModelMatrix();
    }
    void Sprite2D::setScale(const Vector2 &scale) {
        this->scale = scale;
        this->updateModelMatrix();
    }
    const Vector2 &Sprite2D::getScale() const {
        return this->scale;
    }

    void Sprite2D::setOrigin(float x, float y) {
        this->origin.x = x;
        this->origin.y = y;
        this->updateModelMatrix();
    }
    void Sprite2D::setOrigin(const Vector2 &origin) {
        this->origin = origin;
        this->updateModelMatrix();
    }
    const Vector2 &Sprite2D::getOrigin() const {
        return this->origin;
    }
    void Sprite2D::setRotation(float degrees) {
        this->rotation = degrees;
        this->updateModelMatrix();
    }
    float Sprite2D::getRotation() const {
        return this->rotation;
    }

    void Sprite2D::setColor(float r, float g, float b, float a) {
        this->color.x = r;
        this->color.y = g;
        this->color.z = b;
        this->color.w = a;
    }
    void Sprite2D::setColor(const Vector4 &color) {
        this->color = color;
    }
    Vector4 Sprite2D::getColor() const {
        return this->color;
    }
    void Sprite2D::setVisible(bool visible) {
        this->visible = visible;
    }
    bool Sprite2D::isVisible() const {
        return this->visible;
    }

    void Sprite2D::draw(const TextureManager &textureManager, const SpriteShader2D &shader, const SpriteMesh2D &mesh, const Matrix3x3 &projectionViewMatrix) const {
        if (!this->visible) {
            return;
        }
        
        shader.bind();
        shader.setColor(this->color);
        shader.setMVPMatrix(projectionViewMatrix * this->modelMatrix);
        
        GL::setActiveTextureUnit(0);
        if (this->texture != nullptr) {
            this->texture->bind();
        } else {
            textureManager.getDefault2D().bind();
        }

        mesh.draw();
    }
}