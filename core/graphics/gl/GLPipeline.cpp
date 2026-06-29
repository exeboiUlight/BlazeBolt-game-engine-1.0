#include "GLPipeline.h"
#include <glad/glad.h>

static GLenum glPrimitive(PrimitiveType p) {
    switch (p) {
        case PrimitiveType::TRIANGLES:     return GL_TRIANGLES;
        case PrimitiveType::TRIANGLE_FAN:  return GL_TRIANGLE_FAN;
        case PrimitiveType::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case PrimitiveType::LINES:         return GL_LINES;
        case PrimitiveType::POINTS:        return GL_POINTS;
    }
    return GL_TRIANGLES;
}

GLPipeline::GLPipeline(const PipelineDesc& d, GLuint program)
    : shaderProgram(program), desc(d)
{
}

GLPipeline::~GLPipeline() {
    // Shader program is owned by GLShader, not here
}

void GLPipeline::bind() const {
    glUseProgram(shaderProgram);

    if (desc.blend.enabled) {
        glEnable(GL_BLEND);
        auto toGL = [](BlendFactor f) -> GLenum {
            switch (f) {
                case BlendFactor::ZERO: return GL_ZERO;
                case BlendFactor::ONE: return GL_ONE;
                case BlendFactor::SRC_ALPHA: return GL_SRC_ALPHA;
                case BlendFactor::ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::DST_ALPHA: return GL_DST_ALPHA;
                case BlendFactor::ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
            }
            return GL_ONE;
        };
        auto toBlendOp = [](BlendOp o) -> GLenum {
            switch (o) {
                case BlendOp::ADD: return GL_FUNC_ADD;
                case BlendOp::SUBTRACT: return GL_FUNC_SUBTRACT;
                case BlendOp::REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
            }
            return GL_FUNC_ADD;
        };
        glBlendFuncSeparate(
            toGL(desc.blend.srcColor), toGL(desc.blend.dstColor),
            toGL(desc.blend.srcAlpha), toGL(desc.blend.dstAlpha)
        );
        glBlendEquationSeparate(toBlendOp(desc.blend.colorOp), toBlendOp(desc.blend.alphaOp));
    } else {
        glDisable(GL_BLEND);
    }

    if (desc.rasterizer.depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (desc.rasterizer.cullFace) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

GLint GLPipeline::getUniformLoc(const std::string& name) const {
    auto it = uniformCache.find(name);
    if (it != uniformCache.end()) return it->second;
    GLint loc = glGetUniformLocation(shaderProgram, name.c_str());
    uniformCache[name] = loc;
    return loc;
}

void GLPipeline::setUniform(const std::string& name, float value) {
    glUniform1f(getUniformLoc(name), value);
}

void GLPipeline::setUniform(const std::string& name, int value) {
    glUniform1i(getUniformLoc(name), value);
}

void GLPipeline::setUniform(const std::string& name, float x, float y) {
    glUniform2f(getUniformLoc(name), x, y);
}

void GLPipeline::setUniform(const std::string& name, float x, float y, float z) {
    glUniform3f(getUniformLoc(name), x, y, z);
}

void GLPipeline::setUniform(const std::string& name, float x, float y, float z, float w) {
    glUniform4f(getUniformLoc(name), x, y, z, w);
}

void GLPipeline::setUniformMatrix3(const std::string& name, const float* matrix) {
    glUniformMatrix3fv(getUniformLoc(name), 1, GL_FALSE, matrix);
}

void GLPipeline::setUniformMatrix4(const std::string& name, const float* matrix) {
    glUniformMatrix4fv(getUniformLoc(name), 1, GL_FALSE, matrix);
}
