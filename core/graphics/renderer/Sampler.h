#pragma once
#include "Types.h"

class ISampler {
public:
    virtual ~ISampler() = default;

    virtual void bind(uint32_t slot = 0) const = 0;
    virtual const SamplerDesc& getDesc() const = 0;
};
