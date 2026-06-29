#pragma once
#include "Types.h"

class IBuffer {
public:
    virtual ~IBuffer() = default;

    virtual void upload(const void* data, uint32_t size, uint32_t offset = 0) = 0;
    virtual void bind() const = 0;
    virtual uint32_t getSize() const = 0;
    virtual BufferType getType() const = 0;
};
