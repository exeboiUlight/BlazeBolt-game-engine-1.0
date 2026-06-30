#pragma once
#include <string>
#include <cstdint>

enum class RenderAPI : uint8_t {
    OpenGL,
    Vulkan
};

inline const char* renderAPIToString(RenderAPI api) {
    switch (api) {
        case RenderAPI::OpenGL: return "opengl";
        case RenderAPI::Vulkan: return "vulkan";
    }
    return "opengl";
}

inline RenderAPI renderAPIFromString(const std::string& str) {
    std::string trimmed = str;
    while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\n'))
        trimmed.pop_back();
    if (trimmed == "vulkan") return RenderAPI::Vulkan;
    return RenderAPI::OpenGL;
}
