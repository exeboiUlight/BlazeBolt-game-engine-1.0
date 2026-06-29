#pragma once
#include <graphics/renderer/SwapChain.h>

class GLSwapChain : public ISwapChain {
public:
    GLSwapChain(int width, int height);
    ~GLSwapChain() override = default;

    void present() override;
    int getWidth() const override { return width; }
    int getHeight() const override { return height; }
    void resize(int w, int h) override;

    void setSwapBuffersFunc(void (*func)());

private:
    int width;
    int height;
    void (*swapBuffers)();
};
