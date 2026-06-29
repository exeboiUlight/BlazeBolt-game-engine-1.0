# Vulkan as Additional Rendering System — Architecture Plan

## 1. Current State

BlazeBolt uses **OpenGL 3.3 Core Profile** with **no abstraction layer**. All rendering code is directly coupled to OpenGL:

- `GL::` namespace in `core/graphics/gl.hpp` — RAII wrappers (VAO, VBO, Texture2D, ShaderProgram)
- `Shader` class — direct `glCreateShader`/`glUniform*` calls
- `Mesh2D` class — direct VAO/VBO/EBO management
- `SpriteBatch2D`, `ParticleSystem2D`, `Text2D`, `Tileset2D` — all use `GL::` types directly
- `Window` class — wraps GLFW with OpenGL context creation
- `TextureManager` stores `GL::Texture2D` directly

## 2. Target Architecture: RHI (Render Hardware Interface)

### 2.1 Interface Layer (`core/graphics/renderer/`)

```
IRenderDevice          — main device, creates resources
IRenderContext         — immediate/submit command submission  
ISwapChain             — frame buffers, present
IBuffer                — vertex, index, uniform buffers
ITexture               — 2D textures
ISampler               — texture sampling state
IShader                — shader module (VS/FS)
IPipeline              — combined pipeline (shaders + vertex layout + blend)
IPipelineLayout        — descriptor layout / push constants
IDescriptorSet         — resource binding (textures, buffers, uniforms)
IFence / ISemaphore    — synchronization primitives
```

### 2.2 Common Types (`Types.h`)

```cpp
enum class Format { R8_UNORM, R8G8B8A8_UNORM, R8G8B8A8_SRGB, ... };
enum class BufferUsage { VERTEX, INDEX, UNIFORM, STORAGE };
enum class ShaderStage { VERTEX, FRAGMENT };
struct BufferDesc { uint64_t size; BufferUsage usage; };
struct TextureDesc { uint32_t width, height; Format format; };
struct PipelineDesc { ... };
```

### 2.3 OpenGL Backend (`core/graphics/gl/`)

| Interface | OpenGL Implementation |
|---|---|
| IRenderDevice | GLRenderDevice — wraps GL context, creates resources |
| IRenderContext | GLRenderContext — immediate GL calls |
| ISwapChain | GLSwapChain — wraps GLFW window + default framebuffer |
| IBuffer | GLBuffer — wraps GL buffer object |
| ITexture | GLTexture — wraps GL texture object |
| ISampler | GLSampler — wraps GL sampler object |
| IShader | GLShader — wraps GL shader + program |
| IPipeline | GLPipeline — wraps VAO + shader + blend state |
| IPipelineLayout | GLPipelineLayout — maps to uniform locations |
| IDescriptorSet | GLDescriptorSet — (thin, maps to glUniform*/glActiveTexture) |
| IFence | GLFence — wraps GL sync objects |

### 2.4 Vulkan Backend (`core/graphics/vk/`)

| Interface | Vulkan Implementation |
|---|---|
| IRenderDevice | VKRenderDevice — VkDevice, VkInstance, queue management |
| IRenderContext | VKRenderContext — command pool, primary command buffers |
| ISwapChain | VKSwapChain — VkSwapchainKHR, VkSurfaceKHR (GLFW) |
| IBuffer | VKBuffer — VkBuffer + VkDeviceMemory |
| ITexture | VKTexture — VkImage + VkImageView + VkDeviceMemory |
| ISampler | VKSampler — VkSampler |
| IShader | VKShader — VkShaderModule (SPIR-V) |
| IPipeline | VKPipeline — VkPipeline + VkPipelineLayout |
| IPipelineLayout | VKPipelineLayout — VkDescriptorSetLayout + push constant ranges |
| IDescriptorSet | VKDescriptorSet — VkDescriptorPool + VkDescriptorSet |
| IFence | VKFence — VkFence |
| ISemaphore | VKSemaphore — VkSemaphore |

## 3. Engine Integration

### 3.1 Resource Managers

```
TextureManager        -> stores ITexture* (was GL::Texture2D)
FontManager           -> stores Font with ITexture* atlas (was GL::Texture2D)
```

### 3.2 Subject Objects

All rendering-dependent types switch from `GL::` types to the RHI interfaces:

| Type | Current GL Dependency | New Dependency |
|---|---|---|
| `SpriteMesh` | `GL::VertexArrayObject` | `IPipeline*` + `IBuffer*` |
| `SpriteShader2D` | `GL::ShaderProgram` | `IPipeline*` |
| `SpriteBatchShader2D` | `GL::ShaderProgram` | `IPipeline*` |
| `FontShader2D` | `GL::ShaderProgram` | `IPipeline*` |
| `SpriteBatch2D` | `GL::VAO, VBO, Texture2D` | `IBuffer*`, `ITexture*` |
| `Text2D` | `GL::VAO, VBO, Texture2D` | `IBuffer*`, `ITexture*` |
| `ParticleSystem2D` | `GL::VAO, VBO, ShaderProgram` | `IBuffer*`, `IPipeline*` |
| `Tileset2D` | `SpriteBatch2D` (GL) | `SpriteBatch2D` (RHI) |
| `AnimatedTexture2D` | `GL::Texture2D` | `ITexture*` |

### 3.3 LuaEngine Changes

- Receives `IRenderDevice*` at construction
- Creates `IRenderContext*` for frame recording
- Shader management uses `IPipeline*` instead of `Shader*`
- `drawAll()` uses `renderContext->beginFrame()/endFrame()`
- Multiple windows use separate swapchains

### 3.4 Window Abstraction

`Window` class stays as GLFW wrapper, but adds:
```
IRenderDevice* createRenderDevice(RenderAPI api);
ISwapChain*    createSwapChain(Window* window);
```

## 4. Vulkan-Specific Details

### 4.1 Setup Requirements

- **Vulkan SDK** — must be installed (Vulkan SDK from LunarG)
- **GLFW** — already supports `VK_KHR_surface` + `VK_KHR_win32_surface`
- **Shader compilation** — need `glslc` (from Vulkan SDK) or `shaderc` library to compile GLSL to SPIR-V
- **Volk** — optional meta-loader (or use standard `vkGetInstanceProcAddr`)

### 4.2 Vulkan Device Selection

```
1. Create VkInstance (with debug messenger in debug builds)
2. Enumerate physical devices
3. Select discrete GPU if available, fallback to integrated
4. Create VkDevice with graphics queue + present queue
5. Create VkSwapchain with VkSurfaceKHR from GLFW
```

### 4.3 Rendering Loop (Vulkan)

```
for each frame:
  VKRenderContext::beginFrame()
    -> acquire next image from swapchain (vkAcquireNextImageKHR)
    -> wait for frame fence
    -> reset command pool, begin command buffer

  Record draws:
    -> vkCmdBeginRenderPass
    -> vkCmdBindPipeline
    -> vkCmdBindDescriptorSets
    -> vkCmdBindVertexBuffers / vkCmdBindIndexBuffer
    -> vkCmdDraw / vkCmdDrawIndexed
    -> vkCmdEndRenderPass

  VKRenderContext::endFrame()
    -> end command buffer
    -> submit to graphics queue
    -> present (vkQueuePresentKHR)
```

### 4.4 Texture Upload (Vulkan)

```
For each texture:
  1. Create staging buffer (host-visible)
  2. Copy pixel data to staging buffer
  3. Create VkImage (device-local)
  4. Record command to transition VkImage layout
     UNDEFINED -> TRANSFER_DST
  5. vkCmdCopyBufferToImage (staging -> VkImage)
  6. Transition to SHADER_READ_ONLY_OPTIMAL
  7. Create VkImageView
  8. Create VkSampler (or reuse)
  9. Free staging buffer
```

### 4.5 Push Constants vs Uniforms

Vulkan supports push constants (up to 128 bytes) which map well to the engine's small uniform sets (MVP matrix, color, light data). Use push constants for per-draw data and descriptor sets for textures.

```
Push constants:
  - MVP matrix (Matrix3x3 = 36 bytes = 9 floats)
  - Color (Vector4 = 16 bytes)
  - Texture rect (Vector4 = 16 bytes)
  Total: ~68 bytes — fits in 128 byte limit
```

### 4.6 Descriptor Set Layout

```
Layout:
  Binding 0: combined image sampler (fragment) — per texture
```

Descriptor sets are updated when textures change. The engine's low texture variation makes this efficient.

## 5. ImGui Integration

### 5.1 OpenGL Path (existing)

```
ImGui_ImplGlfw_InitForOpenGL(window);
ImGui_ImplOpenGL3_Init("#version 330");
```

### 5.2 Vulkan Path (new)

```
ImGui_ImplGlfw_InitForVulkan(window, true);
ImGui_ImplVulkan_InitInfo init_info = {};
init_info.Instance = vkInstance;
init_info.PhysicalDevice = vkPhysicalDevice;
init_info.Device = vkDevice;
init_info.QueueFamily = graphicsQueueFamily;
init_info.Queue = graphicsQueue;
init_info.PipelineCache = VK_NULL_HANDLE;
init_info.DescriptorPool = imguiDescriptorPool;
init_info.Subpass = 0;
init_info.MinImageCount = minImageCount;
init_info.ImageCount = imageCount;
init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
init_info.Allocator = nullptr;
init_info.CheckVkResultFn = check_vk_result;
ImGui_ImplVulkan_Init(&init_info, renderPass);
```

### 5.3 Engine Editor Integration

`main.cpp` switches between backends:

```cpp
if (renderAPI == "Vulkan") {
    VKRenderDevice* device = new VKRenderDevice(window);
    ImGui_ImplVulkan_Init(...);
} else {
    GLRenderDevice* device = new GLRenderDevice(window);
    ImGui_ImplOpenGL3_Init(...);
}
```

## 6. Build System Changes

### 6.1 CMake Additions

```cmake
# Vulkan files (conditionally compiled)
set(VULKAN_SOURCES
    core/graphics/vk/VKRenderDevice.cpp
    core/graphics/vk/VKRenderContext.cpp
    core/graphics/vk/VKSwapChain.cpp
    core/graphics/vk/VKBuffer.cpp
    core/graphics/vk/VKTexture.cpp
    core/graphics/vk/VKShader.cpp
    core/graphics/vk/VKPipeline.cpp
    core/graphics/vk/VKDescriptorSet.cpp
)
```

### 6.2 Renderer Selection

```cmake
option(BLAZEBOLT_USE_VULKAN "Enable Vulkan renderer" OFF)
if(BLAZEBOLT_USE_VULKAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BLAZEBOLT_VULKAN)
    target_sources(${PROJECT_NAME} PRIVATE ${VULKAN_SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
endif()
```

## 7. Migration Plan — Phases

### Phase 1: RHI Interfaces + OpenGL Refactor
- Define all interfaces in `core/graphics/renderer/`
- Move OpenGL code behind interfaces
- No functional change — everything still renders with OpenGL
- **Files affected**: ~15 files (new + refactored)
- **Risk**: Low — mostly mechanical refactoring

### Phase 2: Minimal Vulkan Backend
- Implement Vulkan device, swapchain, buffer, texture, shader, pipeline
- Goal: clear screen, draw a textured quad
- No particles, no batches yet
- **Files affected**: ~10 new files
- **Risk**: Medium — Vulkan boilerplate

### Phase 3: Full Vulkan Implementation
- Sprite batching, particles, text rendering, tilesets, lights
- Feature parity with OpenGL
- Runtime API selection (config file or command line)
- ImGui Vulkan backend
- **Files affected**: extends Phase 2 files
- **Risk**: Medium — complex state management

### Phase 4: Polish
- Shader compilation pipeline (glslc for Vulkan, keep GLSL for OpenGL)
- Descriptor set caching
- Pipeline cache
- Debug markers / Vulkan validation layers
- Synchronization optimization

## 8. Dependency Graph

```
Engine code (subjects, managers, LuaEngine)
    |
    v
IRenderDevice / IRenderContext / etc.
    |
    +---> GLRenderDevice (OpenGL 3.3)
    |
    +---> VKRenderDevice (Vulkan 1.1+)
              |
              +---> Vulkan SDK (VK_KHR_swapchain)
              +---> GLFW (VK_KHR_surface)
              +---> shaderc (optional, for runtime shader compile)
```

## 9. Key Design Decisions

1. **No virtual calls in hot path** — Use concrete backend types internally after creation (type erasure or pimpl). Virtual calls only at API boundary.

2. **Push constants over uniform buffers** — Small data fits in push constants, avoids buffer management overhead for uniforms.

3. **Single-frame descriptor pools** — Reset descriptor pool each frame for simplicity (engine doesn't have thousands of textures).

4. **Serial command recording** — Single graphics queue, single threaded command recording. No need for multi-threaded submission.

5. **OpenGL stays default** — Vulkan is an opt-in alternative. The engine defaults to OpenGL and uses Vulkan only when configured.

6. **Keep GLFW** — Window and input abstraction stays with GLFW. It handles both OpenGL context and Vulkan surface creation.
