#pragma once
#include <cstdint>

class ISwapChain {
public:
    virtual ~ISwapChain() = default;

    virtual void present() = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual void resize(int width, int height) = 0;
};
