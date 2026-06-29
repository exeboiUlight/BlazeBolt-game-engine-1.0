#pragma once
#include "Types.h"
#include <string>

class IShader {
public:
    virtual ~IShader() = default;
    virtual uint64_t getId() const = 0;
};
