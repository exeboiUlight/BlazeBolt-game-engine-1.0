#pragma once
#include <graphics/renderer/Shader.h>
#include <glad/glad.h>
#include <string>

class GLShader : public IShader {
public:
    GLShader(const std::string& vertexSource, const std::string& fragmentSource);
    ~GLShader() override;

    uint64_t getId() const override { return static_cast<uint64_t>(program); }
    GLuint getProgram() const { return program; }

    GLint getUniformLocation(const std::string& name) const;

private:
    GLuint program;

    static GLuint compileShader(GLenum type, const std::string& source);
    static bool checkCompileErrors(GLuint shader, const std::string& type, GLuint program = 0);
};
