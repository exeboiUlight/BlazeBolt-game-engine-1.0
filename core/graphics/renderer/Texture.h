#pragma once
#include "Types.h"

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual void upload(const void* pixels) = 0;
    virtual void bind(uint32_t slot = 0) const = 0;
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
};
