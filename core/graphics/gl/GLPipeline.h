#pragma once
#include <graphics/renderer/Pipeline.h>
#include <graphics/renderer/Shader.h>
#include <glad/glad.h>
#include <unordered_map>

class GLPipeline : public IPipeline {
public:
    GLPipeline(const PipelineDesc& desc, GLuint shaderProgram);
    ~GLPipeline() override;

    void bind() const override;
    uint64_t getId() const override { return reinterpret_cast<uint64_t>(this); }

    void setUniform(const std::string& name, float value) override;
    void setUniform(const std::string& name, int value) override;
    void setUniform(const std::string& name, float x, float y) override;
    void setUniform(const std::string& name, float x, float y, float z) override;
    void setUniform(const std::string& name, float x, float y, float z, float w) override;
    void setUniformMatrix3(const std::string& name, const float* matrix) override;
    void setUniformMatrix4(const std::string& name, const float* matrix) override;

    GLuint getProgram() const { return shaderProgram; }
    const PipelineDesc& getDesc() const { return desc; }

private:
    GLuint shaderProgram;
    PipelineDesc desc;
    mutable std::unordered_map<std::string, GLint> uniformCache;

    GLint getUniformLoc(const std::string& name) const;
};
