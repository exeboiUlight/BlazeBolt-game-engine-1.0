#include "sprite2D.hpp"

namespace BlazeBolt {
    SpriteMesh2D::SpriteMesh2D() : vertexArrayObject() {}
    SpriteMesh2D::SpriteMesh2D(const QuadVertexBufferObject2D &vertexBufferObject) : vertexArrayObject() {
        this->setVertexBuffer(vertexBufferObject);
    }

    void SpriteMesh2D::setVertexBuffer(const QuadVertexBufferObject2D &vbo) {
        this->vertexArrayObject.bind();
        vbo.bind();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertexBufferObject2D::Vertex), nullptr);
        glEnableVertexAttribArray(0);
    }
    void SpriteMesh2D::draw() const {
        this->vertexArrayObject.bind();
        glDrawArrays(QuadVertexBufferObject2D::DRAW_MODE, 0, QuadVertexBufferObject2D::VERTEX_COUNT);
    }

    SpriteShader2D::SpriteShader2D() : shaderProgram() {
        constexpr GLchar vertexShaderSource[] = R"(
            #version 410
            layout (location = 0) in vec2 a_Position;
            layout (location = 0) out vec2 v_TexCoord;

            uniform float u_AspectRatio;
            uniform mat3 u_MVPMatrix;

            void main() {
                vec3 transformed = u_MVPMatrix * vec3(a_Position, 1.0);
                gl_Position = vec4(transformed.xy, 0.0, 1.0);
                gl_Position.x /= u_AspectRatio;
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

    void Sprite2D::draw(const GL::Texture2D &defaultTexture, const SpriteShader2D &shader, const SpriteMesh2D &mesh, const Matrix3x3 &projectionViewMatrix) const {
        if (!this->visible) { return; }
        shader.bind();
        shader.setMVPMatrix(projectionViewMatrix * this->modelMatrix);
        shader.setColor(this->color);
        
        GL::setActiveTextureUnit(0);
        if (this->texture != nullptr) {
            this->texture->bind();
        } else {
            defaultTexture.bind();
        }

        mesh.draw();
    }
}