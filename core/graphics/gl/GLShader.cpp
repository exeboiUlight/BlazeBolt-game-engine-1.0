#include "GLShader.h"
#include <iostream>
#include <sstream>

GLShader::GLShader(const std::string& vertexSource, const std::string& fragmentSource)
    : program(0)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vs) return;

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fs) {
        glDeleteShader(vs);
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    if (!checkCompileErrors(program, "PROGRAM", program)) {
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

GLShader::~GLShader() {
    if (program) glDeleteProgram(program);
}

GLint GLShader::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(program, name.c_str());
}

GLuint GLShader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    if (!checkCompileErrors(shader, (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT")) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool GLShader::checkCompileErrors(GLuint shader, const std::string& type, GLuint program) {
    int success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR (" << type << "):\n" << infoLog << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR:\n" << infoLog << std::endl;
            return false;
        }
    }
    return true;
}
