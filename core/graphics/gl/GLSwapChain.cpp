#include "GLSwapChain.h"
#include <glad/glad.h>

GLSwapChain::GLSwapChain(int w, int h)
    : width(w), height(h), swapBuffers(nullptr)
{
}

void GLSwapChain::present() {
    if (swapBuffers) swapBuffers();
}

void GLSwapChain::resize(int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
}

void GLSwapChain::setSwapBuffersFunc(void (*func)()) {
    swapBuffers = func;
}
