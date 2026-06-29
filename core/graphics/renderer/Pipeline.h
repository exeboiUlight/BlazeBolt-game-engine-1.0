#pragma once
#include "Types.h"
#include <string>

class IPipeline {
public:
    virtual ~IPipeline() = default;

    virtual void bind() const = 0;
    virtual uint64_t getId() const = 0;

    virtual void setUniform(const std::string& name, float value) = 0;
    virtual void setUniform(const std::string& name, int value) = 0;
    virtual void setUniform(const std::string& name, float x, float y) = 0;
    virtual void setUniform(const std::string& name, float x, float y, float z) = 0;
    virtual void setUniform(const std::string& name, float x, float y, float z, float w) = 0;
    virtual void setUniformMatrix3(const std::string& name, const float* matrix) = 0;
    virtual void setUniformMatrix4(const std::string& name, const float* matrix) = 0;
};
