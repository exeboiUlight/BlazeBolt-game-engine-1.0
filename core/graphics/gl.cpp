#include "gl.hpp"
#include <string>

static GLuint boundVertexArrayObject = 0;
// FIXME: Only counts GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER, but it's enough for now
static GLuint boundVertexBufferObjects[2] = {0};
static GLuint boundTexture2Ds[32] = {0};
static GLuint activeTextureUnit = 0;
static GLuint boundShaderProgram = 0;

namespace GL {
    void resetCachedState() {
        boundVertexArrayObject = 0;
        boundVertexBufferObjects[0] = 0;
        boundVertexBufferObjects[1] = 0;
        for (auto &t : boundTexture2Ds) t = 0;
        activeTextureUnit = 0;
        boundShaderProgram = 0;
    }
    void bindVertexArray(GLuint vao) {
        if (boundVertexArrayObject == vao) { return; }
        glBindVertexArray(vao);
        boundVertexArrayObject = vao;
        boundVertexBufferObjects[1] = 0xFFFFFFFF; // Invalidate element buffer object;
    }
    void unbindVertexArray() {
        if (boundVertexArrayObject == 0) { return; }
        glBindVertexArray(0);
        boundVertexArrayObject = 0;
        boundVertexBufferObjects[1] = 0xFFFFFFFF; // Invalidate element buffer object;
    }
    void bindVertexBuffer(GLuint vbo, GLenum usage) {
        if (boundVertexBufferObjects[usage - GL_ARRAY_BUFFER] == vbo) { return; }
        glBindBuffer(usage, vbo);
        boundVertexBufferObjects[usage - GL_ARRAY_BUFFER] = vbo;
    }
    void unbindVertexBuffer(GLenum usage) {
        if (boundVertexBufferObjects[usage - GL_ARRAY_BUFFER] == 0) { return; }
        glBindBuffer(usage, 0);
        boundVertexBufferObjects[usage - GL_ARRAY_BUFFER] = 0;
    }
    void bindShaderProgram(GLuint program) {
        if (boundShaderProgram == program) { return; }
        glUseProgram(program);
        boundShaderProgram = program;
    }
    void unbindShaderProgram() {
        if (boundShaderProgram == 0) { return; }
        glUseProgram(0);
        boundShaderProgram = 0;
    }
    void setActiveTextureUnit(GLuint unit) {
        if (activeTextureUnit == unit) { return; }
        glActiveTexture(GL_TEXTURE0 + unit);
        activeTextureUnit = unit;
    }
    void bindTexture2D(GLuint texture) {
        if (boundTexture2Ds[activeTextureUnit] == texture) { return; }
        glBindTexture(GL_TEXTURE_2D, texture);
        boundTexture2Ds[activeTextureUnit] = texture;
    }
    void unbindTexture2D() {
        if (boundTexture2Ds[activeTextureUnit] == 0) { return; }
        glBindTexture(GL_TEXTURE_2D, 0);
        boundTexture2Ds[activeTextureUnit] = 0;
    }

    VertexArrayObject::VertexArrayObject() : id(0) { glGenVertexArrays(1, &this->id); }
    VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) noexcept : id(other.id) { other.id = 0; }
    VertexArrayObject &VertexArrayObject::operator=(VertexArrayObject&& other) noexcept {
        if (this == &other) { return *this; }
        if (this->id != 0) { glDeleteVertexArrays(1, &this->id); }
        this->id = other.id;
        other.id = 0;
        return *this;
    }
    VertexArrayObject::~VertexArrayObject() {
        if (this->id != 0) { glDeleteVertexArrays(1, &this->id); }
    }
    void VertexArrayObject::bind() const { bindVertexArray(this->id); }
    GLuint VertexArrayObject::get() const { return this->id; }

    VertexBufferObject::VertexBufferObject() : id(0) { glGenBuffers(1, &this->id); }
    VertexBufferObject::VertexBufferObject(VertexBufferObject&& other) noexcept : id(other.id) { other.id = 0; }
    VertexBufferObject &VertexBufferObject::operator=(VertexBufferObject&& other) noexcept {
        if (this == &other) { return *this; }
        if (this->id != 0) { glDeleteBuffers(1, &this->id); }
        this->id = other.id;
        other.id = 0;
        return *this;
    }
    VertexBufferObject::~VertexBufferObject() {
        if (this->id != 0) { glDeleteBuffers(1, &this->id); }
    }
    void VertexBufferObject::bind(GLenum usage) const { bindVertexBuffer(this->id, usage); }
    GLuint VertexBufferObject::get() const { return this->id; }

    Texture2D::Texture2D() : id(0) { glGenTextures(1, &this->id); }
    Texture2D::Texture2D(Texture2D&& other) noexcept : id(other.id) { other.id = 0; }
    Texture2D &Texture2D::operator=(Texture2D&& other) noexcept {
        if (this == &other) { return *this; }
        if (this->id != 0) { glDeleteTextures(1, &this->id); }
        this->id = other.id;
        other.id = 0;
        return *this;
    }
    Texture2D::~Texture2D() {
        if (this->id != 0) { glDeleteTextures(1, &this->id); }
    }
    void Texture2D::bind() const { bindTexture2D(this->id); }
    GLuint Texture2D::get() const { return this->id; }

    ShaderProgram::ShaderProgram() : id(glCreateProgram()) {}
    ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : id(other.id) { other.id = 0; }
    ShaderProgram &ShaderProgram::operator=(ShaderProgram&& other) noexcept {
        if (this == &other) { return *this; }
        if (this->id != 0) { glDeleteProgram(this->id); }
        this->id = other.id;
        other.id = 0;
        return *this;
    }
    ShaderProgram::~ShaderProgram() {
        if (this->id != 0) { glDeleteProgram(this->id); }
    }
    bool ShaderProgram::tryToLink() {
        glLinkProgram(this->id);
        GLint success = 0;
        glGetProgramiv(this->id, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            GLint logLength = 0;
            glGetProgramiv(this->id, GL_INFO_LOG_LENGTH, &logLength);
            std::string log = std::string(logLength, ' ');
            glGetProgramInfoLog(this->id, logLength, nullptr, log.data());
            fprintf(stderr, "Failed to link Sprite2D shader program:\n%s\n", log.c_str());
            return false;
        }
        return true;
    }
    void ShaderProgram::bind() const { bindShaderProgram(this->id); }
    GLuint ShaderProgram::get() const { return this->id; }

    Shader::Shader(GLenum type) : id(glCreateShader(type)) {}
    Shader::Shader(Shader&& other) noexcept : id(other.id) { other.id = 0; }
    Shader &Shader::operator=(Shader&& other) noexcept {
        if (this == &other) { return *this; }
        if (this->id != 0) { glDeleteShader(this->id); }
        this->id = other.id;
        other.id = 0;
        return *this;
    }
    Shader::~Shader() {
        if (this->id != 0) { glDeleteShader(this->id); }
    }

    std::optional<Shader> Shader::fromSource(GLenum type, const char *source) {
        Shader shader = Shader(type);
        glShaderSource(shader.get(), 1, &source, nullptr);
        glCompileShader(shader.get());
        GLint success = 0;
        glGetShaderiv(shader.get(), GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            GLint logLength = 0;
            glGetShaderiv(shader.get(), GL_INFO_LOG_LENGTH, &logLength);
            std::string log = std::string(logLength, ' ');
            glGetShaderInfoLog(shader.get(), logLength, nullptr, log.data());
            fprintf(stderr, "Failed to compile Sprite2D fragment shader:\n%s\n", log.c_str());
            return std::nullopt;
        }
        return shader;
    }
    GLuint Shader::get() const { return this->id; }
}