#pragma once
#include <graphics/renderer/Sampler.h>
#include <glad/glad.h>

class GLSampler : public ISampler {
public:
    GLSampler(const SamplerDesc& desc);
    ~GLSampler() override;

    void bind(uint32_t slot = 0) const override;
    const SamplerDesc& getDesc() const override { return desc; }

    GLuint getHandle() const { return handle; }

private:
    GLuint handle;
    SamplerDesc desc;
};
