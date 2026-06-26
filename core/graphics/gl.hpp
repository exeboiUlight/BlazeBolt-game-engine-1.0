#pragma once
#include <optional>
#include <glad/glad.h>

namespace GL {
    void resetCachedState();
    void bindVertexArray(GLuint vao);
    void unbindVertexArray();
    void bindVertexBuffer(GLuint vbo, GLenum usage);
    void unbindVertexBuffer(GLenum usage);
    void bindShaderProgram(GLuint program);
    void unbindShaderProgram();
    void setActiveTextureUnit(GLuint unit);
    void bindTexture2D(GLuint texture);
    void unbindTexture2D();

    struct VertexArrayObject {
    private:
        GLuint id;
    public:
        VertexArrayObject();
        VertexArrayObject(const VertexArrayObject&) = delete;
        VertexArrayObject &operator=(const VertexArrayObject&) = delete;
        VertexArrayObject(VertexArrayObject&& other) noexcept;
        VertexArrayObject &operator=(VertexArrayObject&& other) noexcept;
        ~VertexArrayObject();

        void bind() const;
        GLuint get() const;
    };
    struct VertexBufferObject {
    private:
        GLuint id;
    public:
        VertexBufferObject();
        VertexBufferObject(const VertexBufferObject&) = delete;
        VertexBufferObject &operator=(const VertexBufferObject&) = delete;
        VertexBufferObject(VertexBufferObject&& other) noexcept;
        VertexBufferObject &operator=(VertexBufferObject&& other) noexcept;
        ~VertexBufferObject();

        void bind(GLenum usage) const;
        GLuint get() const;
    };
    struct Texture2D {
    private:
        GLuint id;
    public:
        Texture2D();
        Texture2D(const Texture2D&) = delete;
        Texture2D &operator=(const Texture2D&) = delete;
        Texture2D(Texture2D&& other) noexcept;
        Texture2D &operator=(Texture2D&& other) noexcept;
        ~Texture2D();

        void bind() const;
        GLuint get() const;
    };
    struct ShaderProgram {
    private:
        GLuint id;
    public:
        ShaderProgram();
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram &operator=(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&& other) noexcept;
        ShaderProgram &operator=(ShaderProgram&& other) noexcept;
        ~ShaderProgram();

        bool tryToLink();

        void bind() const;
        GLuint get() const;
    };
    struct Shader {
    private:
        GLuint id;
    public:
        Shader(GLenum type);
        Shader(const Shader&) = delete;
        Shader &operator=(const Shader&) = delete;
        Shader(Shader&& other) noexcept;
        Shader &operator=(Shader&& other) noexcept;
        ~Shader();

        static std::optional<Shader> fromSource(GLenum type, const char *source);

        GLuint get() const;
    };
}