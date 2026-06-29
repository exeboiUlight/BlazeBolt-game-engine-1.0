#pragma once
#include <cstdint>
#include <vector>
#include <string>

class IShader;

enum class BufferType : uint8_t { VERTEX, INDEX, UNIFORM };
enum class BufferUsage : uint8_t { STATIC, DYNAMIC, STREAM };
enum class TextureFormat : uint8_t { R8, RG8, RGB8, RGBA8, SRGB8_ALPHA8 };
enum class TextureFilter : uint8_t { NEAREST, LINEAR };
enum class TextureWrap : uint8_t { REPEAT, CLAMP, MIRRORED_REPEAT };
enum class PrimitiveType : uint8_t { TRIANGLES, TRIANGLE_FAN, TRIANGLE_STRIP, LINES, POINTS };
enum class BlendFactor : uint8_t { ZERO, ONE, SRC_ALPHA, ONE_MINUS_SRC_ALPHA, DST_ALPHA, ONE_MINUS_DST_ALPHA };
enum class BlendOp : uint8_t { ADD, SUBTRACT, REVERSE_SUBTRACT };

struct BufferDesc {
    uint32_t size;
    BufferType type;
    BufferUsage usage;
};

struct TextureDesc {
    uint32_t width;
    uint32_t height;
    TextureFormat format;
};

struct SamplerDesc {
    TextureFilter min = TextureFilter::LINEAR;
    TextureFilter mag = TextureFilter::LINEAR;
    TextureWrap wrapU = TextureWrap::CLAMP;
    TextureWrap wrapV = TextureWrap::CLAMP;
};

struct VertexAttrib {
    uint32_t location;
    uint32_t offset;
    uint32_t stride;
    uint8_t size;
    bool normalized;
};

struct BlendState {
    bool enabled = true;
    BlendFactor srcColor = BlendFactor::SRC_ALPHA;
    BlendFactor dstColor = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendOp colorOp = BlendOp::ADD;
    BlendFactor srcAlpha = BlendFactor::SRC_ALPHA;
    BlendFactor dstAlpha = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendOp alphaOp = BlendOp::ADD;
};

struct RasterizerState {
    bool depthTest = false;
    bool cullFace = false;
};

enum class UniformType : uint8_t { FLOAT, INT, VEC2, VEC3, VEC4, MAT3, MAT4 };
struct UniformDesc {
    std::string name;
    UniformType type;
};

struct PipelineDesc {
    IShader* vertexShader = nullptr;
    IShader* fragmentShader = nullptr;
    PrimitiveType primitive = PrimitiveType::TRIANGLES;
    BlendState blend;
    RasterizerState rasterizer;
    std::vector<VertexAttrib> vertexAttributes;
    uint32_t vertexStride = 0;
};
