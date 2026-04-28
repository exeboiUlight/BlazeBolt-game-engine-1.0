  BlazeBolt Game Engine Documentation \* { margin: 0; padding: 0; box-sizing: border-box; } body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; line-height: 1.6; color: #333; background: #f5f5f5; } /\* Header \*/ .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 60px 20px; text-align: center; } .header h1 { font-size: 48px; margin-bottom: 10px; } .header p { font-size: 20px; opacity: 0.95; } .version { display: inline-block; background: rgba(255,255,255,0.2); padding: 5px 15px; border-radius: 20px; margin-top: 15px; font-size: 14px; } /\* Container \*/ .container { max-width: 1200px; margin: 0 auto; padding: 20px; } /\* Sidebar \*/ .sidebar { position: fixed; left: 0; top: 0; width: 280px; height: 100vh; background: #2c3e50; color: white; overflow-y: auto; transform: translateX(-100%); transition: transform 0.3s ease; z-index: 1000; } .sidebar.open { transform: translateX(0); } .sidebar-header { padding: 20px; background: #1a252f; font-size: 20px; font-weight: bold; text-align: center; } .sidebar-nav { padding: 10px 0; } .sidebar-nav a { display: block; padding: 10px 20px; color: #ecf0f1; text-decoration: none; transition: background 0.2s; } .sidebar-nav a:hover { background: #34495e; } .sidebar-nav .section-title { font-weight: bold; color: #3498db; padding: 15px 20px 5px; font-size: 12px; text-transform: uppercase; letter-spacing: 1px; } /\* Menu Button \*/ .menu-btn { position: fixed; top: 20px; left: 20px; background: #667eea; color: white; border: none; width: 50px; height: 50px; border-radius: 50%; cursor: pointer; z-index: 1001; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 2px 10px rgba(0,0,0,0.2); transition: transform 0.2s; } .menu-btn:hover { transform: scale(1.05); } /\* Main Content \*/ .main-content { max-width: 1000px; margin: 0 auto; padding: 20px; } /\* Sections \*/ .section { background: white; border-radius: 10px; padding: 30px; margin-bottom: 30px; box-shadow: 0 2px 10px rgba(0,0,0,0.05); } .section h2 { color: #667eea; border-bottom: 3px solid #667eea; padding-bottom: 10px; margin-bottom: 25px; font-size: 28px; } .section h3 { color: #764ba2; margin: 20px 0 10px; font-size: 22px; } .section h4 { color: #555; margin: 15px 0 10px; font-size: 18px; } /\* Code blocks \*/ pre { background: #2d2d2d; color: #f8f8f2; padding: 15px; border-radius: 8px; overflow-x: auto; margin: 15px 0; font-family: 'Courier New', monospace; font-size: 14px; } code { background: #f4f4f4; padding: 2px 6px; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 14px; color: #e74c3c; } pre code { background: none; color: #f8f8f2; padding: 0; } /\* Tables \*/ table { width: 100%; border-collapse: collapse; margin: 20px 0; } th, td { border: 1px solid #ddd; padding: 12px; text-align: left; } th { background: #667eea; color: white; } tr:nth-child(even) { background: #f9f9f9; } /\* Notes and tips \*/ .note { background: #e8f4fd; border-left: 4px solid #3498db; padding: 15px; margin: 20px 0; border-radius: 5px; } .tip { background: #e8f8f5; border-left: 4px solid #27ae60; padding: 15px; margin: 20px 0; border-radius: 5px; } .warning { background: #fdf2e9; border-left: 4px solid #e67e22; padding: 15px; margin: 20px 0; border-radius: 5px; } /\* Badges \*/ .badge { display: inline-block; background: #3498db; color: white; padding: 2px 8px; border-radius: 4px; font-size: 12px; margin-left: 10px; } /\* Search \*/ .search-box { position: fixed; top: 20px; right: 20px; z-index: 1001; } .search-box input { padding: 10px 15px; width: 250px; border: none; border-radius: 25px; font-size: 14px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); } /\* Footer \*/ .footer { text-align: center; padding: 30px; color: #7f8c8d; border-top: 1px solid #ddd; margin-top: 40px; } /\* Responsive \*/ @media (max-width: 768px) { .main-content { padding: 10px; } .section { padding: 20px; } .search-box input { width: 180px; } } /\* Animation \*/ @keyframes fadeIn { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } } .section { animation: fadeIn 0.5s ease-out; } ☰

BlazeBolt Engine

Getting Started

[Overview](#overview) [Installation](#installation) [Basic Setup](#basic-setup)

Core API

[Core Systems](#core-systems) [Sprite API](#sprite-api) [Animation API](#animation-api) [Text API](#text-api) [Mesh API](#mesh-api) [Audio API](#audio-api) [Input API](#input-api)

Scripting

[Script Management](#script-management) [Utility Functions](#utility-functions)

Examples

[Complete Examples](#examples) [Best Practices](#best-practices) [Troubleshooting](#troubleshooting)

# 🎮 BlazeBolt Game Engine

Professional 2D Game Development Framework

Version 1.0.0 | Lua-Powered

## Overview

BlazeBolt is a powerful Lua-powered 2D game engine with support for sprites, animations, text rendering, audio, input handling, and custom meshes. The engine provides a comprehensive API for creating games entirely in Lua while leveraging C++ for performance-critical operations.

**✨ Key Features:**

*   🎨 2D Sprite Rendering with textures
*   🎬 GIF and Sprite Sheet animations
*   📝 TrueType font rendering
*   🔊 OpenAL audio support (WAV)
*   🎮 Full input handling (Keyboard + Mouse)
*   📐 Custom mesh support
*   📜 Lua scripting for game logic
*   ⚡ High performance C++ backend

## Basic Setup

### Main Lua Script

```
-- main.lua
local game = {}

function Start()
    BlazeBolt.Print("Game initialized!")
end

function Update(dt)
    -- Called every frame with delta time in seconds
    BlazeBolt.Print("Delta time:", dt)
end

function Draw()
    -- Called after all automatic drawing
end

return game
```

### Key Constants

```
-- Keyboard keys
Keys.W, Keys.A, Keys.S, Keys.D
Keys.UP, Keys.DOWN, Keys.LEFT, Keys.RIGHT
Keys.SPACE, Keys.ENTER, Keys.ESCAPE
Keys.TAB, Keys.BACKSPACE
Keys.LEFT_SHIFT, Keys.RIGHT_SHIFT
Keys.0, Keys.1, ..., Keys.9
Keys.A, Keys.B, ..., Keys.Z

-- Mouse buttons
MouseButtons.LEFT
MouseButtons.RIGHT
MouseButtons.MIDDLE
```

## Core Systems

### Print

```
BlazeBolt.Print(value1, value2, ...)
```

Prints values to the console with automatic type conversion.

```
BlazeBolt.Print("Hello", "World", 42, true)
-- Output: Hello	World	42	true
```

### GetDeltaTime

```
local dt = BlazeBolt.GetDeltaTime()
```

Returns the time since the last frame in seconds.

```
function Update(dt)
    local fps = 1 / dt
    BlazeBolt.Print("FPS:", math.floor(fps))
end
```

### Random Functions

```
-- Random float between min and max
local value = BlazeBolt.Random(min, max)

-- Random integer between min and max (inclusive)
local intValue = BlazeBolt.RandomInt(min, max)

-- Set random seed
BlazeBolt.SetRandomSeed(seed)
```

### Quit

```
BlazeBolt.Quit()
```

Exits the game immediately.

## Sprite API

### NewSprite

```
local spriteId = BlazeBolt.NewSprite(texturePath, x, y)
```

Creates a new sprite at position (x, y) with the specified texture.

| Parameter | Type | Description |
| --- | --- | --- |
| texturePath | string | Path to the texture image (PNG, JPG, etc.) |
| x | number | X position (default: 0) |
| y | number | Y position (default: 0) |

### Sprite Methods

```
-- Position
BlazeBolt.SpriteSetPosition(spriteId, x, y)
local x, y = BlazeBolt.SpriteGetPosition(spriteId)

-- Size
BlazeBolt.SpriteSetSize(spriteId, width, height)
local width, height = BlazeBolt.SpriteGetSize(spriteId)

-- Origin and Rotation
BlazeBolt.SpriteSetOrigin(spriteId, originX, originY)
BlazeBolt.SpriteSetRotation(spriteId, degrees)

-- Color and Visibility
BlazeBolt.SpriteSetColor(spriteId, r, g, b, a)
BlazeBolt.SpriteSetVisible(spriteId, visible)
```

**💡 Tip:** Origin values range from 0 to 1. (0,0) is top-left, (0.5,0.5) is center, (1,1) is bottom-right.

### Complete Sprite Example

```
local player = nil
local playerSpeed = 300

function Start()
    player = BlazeBolt.NewSprite("assets/player.png", 400, 300)
    BlazeBolt.SpriteSetSize(player, 64, 64)
    BlazeBolt.SpriteSetOrigin(player, 0.5, 0.5)
    BlazeBolt.SpriteSetColor(player, 1, 1, 0, 1)
end

function Update(dt)
    local dx, dy = 0, 0
    local speed = playerSpeed * dt
    
    if BlazeBolt.IsKeyPressed(Keys.W) then dy = dy - speed end
    if BlazeBolt.IsKeyPressed(Keys.S) then dy = dy + speed end
    if BlazeBolt.IsKeyPressed(Keys.A) then dx = dx - speed end
    if BlazeBolt.IsKeyPressed(Keys.D) then dx = dx + speed end
    
    if dx ~= 0 or dy ~= 0 then
        local px, py = BlazeBolt.SpriteGetPosition(player)
        BlazeBolt.SpriteSetPosition(player, px + dx, py + dy)
    end
end
```

## Animation API

### NewAnimation (GIF)

```
local animId = BlazeBolt.NewAnimation(path, isGif, x, y)
```

### NewAnimationFromSheet (Sprite Sheet)

```
local animId = BlazeBolt.NewAnimationFromSheet(
    texturePath, frameWidth, frameHeight, 
    totalFrames, framesPerRow, frameDelayMs, 
    x, y
)
```

### Animation Control

```
BlazeBolt.AnimationPlay(animId)
BlazeBolt.AnimationPause(animId)
BlazeBolt.AnimationStop(animId)
BlazeBolt.AnimationRestart(animId)

BlazeBolt.AnimationSetLooping(animId, loop)
BlazeBolt.AnimationSetSpeed(animId, speed)
BlazeBolt.AnimationSetFrame(animId, frame)

local frameCount = BlazeBolt.AnimationGetFrameCount(animId)
local isPlaying = BlazeBolt.AnimationIsPlaying(animId)

BlazeBolt.AnimationSetPosition(animId, x, y)
BlazeBolt.AnimationSetSize(animId, width, height)
```

**📝 Note:** GIF animations are loaded directly. Sprite sheets require frame dimensions and count.

### Animation Example

```
local player = BlazeBolt.NewAnimation("assets/player.gif", true, 400, 300)
BlazeBolt.AnimationSetLooping(player, true)
BlazeBolt.AnimationPlay(player)
BlazeBolt.AnimationSetSpeed(player, 1.5)

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        if BlazeBolt.AnimationIsPlaying(player) then
            BlazeBolt.AnimationPause(player)
        else
            BlazeBolt.AnimationPlay(player)
        end
    end
end
```

## Text API

### NewText

```
local textId = BlazeBolt.NewText(fontPath, text, x, y, fontSize)
```

| Parameter | Type | Description |
| --- | --- | --- |
| fontPath | string | Path to TTF font file |
| text | string | Initial text content |
| x | number | X position (default: 0) |
| y | number | Y position (default: 0) |
| fontSize | integer | Font size in pixels (default: 48) |

### Text Methods

```
BlazeBolt.TextSetString(textId, text)
local text = BlazeBolt.TextGetString(textId)
BlazeBolt.TextSetPosition(textId, x, y)
BlazeBolt.TextSetScale(textId, scale)
BlazeBolt.TextSetColor(textId, r, g, b, a)
BlazeBolt.TextSetVisible(textId, visible)
```

### Text Example

```
local score = 0
local scoreText = BlazeBolt.NewText("assets/fonts/arial.ttf", "Score: 0", 10, 10, 32)

function Update(dt)
    score = score + 1
    BlazeBolt.TextSetString(scoreText, "Score: " .. score)
    
    -- Change color based on score
    if score > 100 then
        BlazeBolt.TextSetColor(scoreText, 1, 0, 0, 1)
    elseif score > 50 then
        BlazeBolt.TextSetColor(scoreText, 1, 1, 0, 1)
    end
end
```

## Audio API

### LoadSound

```
local soundId = BlazeBolt.LoadSound(filename, soundName, loop)
```

### Playback Control

```
BlazeBolt.PlaySound(soundName)
BlazeBolt.PlaySoundById(soundId)
BlazeBolt.StopSound(soundName)
BlazeBolt.StopSoundById(soundId)
BlazeBolt.StopAllSounds()
```

### Sound Settings

```
BlazeBolt.SetSoundVolume(soundName, volume)
BlazeBolt.SetSoundVolumeById(soundId, volume)
BlazeBolt.SetSoundLooping(soundName, loop)
BlazeBolt.SetSoundLoopingById(soundId, loop)

local isPlaying = BlazeBolt.IsSoundPlaying(soundName)
```

**⚠️ Note:** Audio files must be in WAV format (PCM, not compressed).

### Audio Example

```
local jump = BlazeBolt.LoadSound("assets/jump.wav", "jump", false)
local music = BlazeBolt.LoadSound("assets/music.wav", "bgm", true)

BlazeBolt.PlaySound("bgm")
BlazeBolt.SetSoundVolume("bgm", 0.5)

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.PlaySound("jump")
        BlazeBolt.SetSoundVolume("jump", 0.8)
    end
    
    if BlazeBolt.IsKeyJustPressed(Keys.M) then
        local volume = BlazeBolt.GetSoundVolume("bgm")
        if volume > 0 then
            BlazeBolt.SetSoundVolume("bgm", 0)
        else
            BlazeBolt.SetSoundVolume("bgm", 0.5)
        end
    end
end
```

## Input API

### Keyboard Input

```
local pressed = BlazeBolt.IsKeyPressed(key)
local justPressed = BlazeBolt.IsKeyJustPressed(key)
local justReleased = BlazeBolt.IsKeyJustReleased(key)
```

### Mouse Input

```
local mouseX = BlazeBolt.GetMouseX()
local mouseY = BlazeBolt.GetMouseY()
local deltaX = BlazeBolt.GetMouseDeltaX()
local deltaY = BlazeBolt.GetMouseDeltaY()

local leftPressed = BlazeBolt.IsMouseButtonPressed(MouseButtons.LEFT)
local rightJustPressed = BlazeBolt.IsMouseButtonJustPressed(MouseButtons.RIGHT)

local scrollY = BlazeBolt.GetScrollY()

BlazeBolt.ResetScroll()
BlazeBolt.ResetMouseDelta()
```

### Input Example

```
local playerPos = {x = 400, y = 300}

function Update(dt)
    -- Keyboard movement
    local speed = 300 * dt
    if BlazeBolt.IsKeyPressed(Keys.W) then playerPos.y = playerPos.y - speed end
    if BlazeBolt.IsKeyPressed(Keys.S) then playerPos.y = playerPos.y + speed end
    if BlazeBolt.IsKeyPressed(Keys.A) then playerPos.x = playerPos.x - speed end
    if BlazeBolt.IsKeyPressed(Keys.D) then playerPos.x = playerPos.x + speed end
    
    -- Mouse click
    if BlazeBolt.IsMouseButtonJustPressed(MouseButtons.LEFT) then
        local mouseX = BlazeBolt.GetMouseX()
        local mouseY = BlazeBolt.GetMouseY()
        BlazeBolt.Print("Clicked at:", mouseX, mouseY)
    end
    
    -- Exit on Escape
    if BlazeBolt.IsKeyJustPressed(Keys.ESCAPE) then
        BlazeBolt.Quit()
    end
end
```

## Script Management

### Script List File (scripts.list)

```
# Comments start with #
main=scripts/main.lua
player=scripts/player.lua
ui=scripts/ui.lua
enemies=scripts/enemies.lua
```

### Loading Scripts

```
BlazeBolt.LoadScriptsFromList(listPath)
BlazeBolt.LoadScript(scriptPath)

BlazeBolt.ReloadScript(scriptPath)
BlazeBolt.ReloadAllScripts()

BlazeBolt.EnableScript(scriptName, enabled)
local loaded = BlazeBolt.IsScriptLoaded(scriptName)
local scripts = BlazeBolt.GetLoadedScripts()
```

### Script Organization Example

```
-- main.lua
local game = {}

function Start()
    BlazeBolt.Print("Game starting...")
    BlazeBolt.LoadScript("scripts/player.lua")
    BlazeBolt.LoadScript("scripts/enemies.lua")
end

function Update(dt)
    -- Game logic
end

return game
```

## Utility Functions

```
-- Timing
local dt = BlazeBolt.GetDeltaTime()
local time = BlazeBolt.GetTime()

-- Random
BlazeBolt.SetRandomSeed(seed)
local r = BlazeBolt.Random(min, max)
local ri = BlazeBolt.RandomInt(min, max)

-- Game control
BlazeBolt.Destroy(entityId)
BlazeBolt.Quit()

-- Debug
BlazeBolt.Print(...)
```

## Complete Examples

### Simple Platformer

```
-- platformer.lua
local game = {}
local player = nil
local platforms = {}
local score = 0

function Start()
    -- Create player
    player = BlazeBolt.NewSprite("assets/player.png", 400, 500)
    BlazeBolt.SpriteSetSize(player, 48, 48)
    BlazeBolt.SpriteSetOrigin(player, 0.5, 0.5)
    
    -- Create platforms
    for i = 1, 5 do
        local platform = BlazeBolt.NewSprite("assets/platform.png", 100 * i, 400 - i * 50)
        BlazeBolt.SpriteSetSize(platform, 80, 20)
        table.insert(platforms, platform)
    end
    
    -- Load sounds
    BlazeBolt.LoadSound("assets/jump.wav", "jump", false)
end

function Update(dt)
    -- Movement
    local speed = 400 * dt
    if BlazeBolt.IsKeyPressed(Keys.A) then
        local x, y = BlazeBolt.SpriteGetPosition(player)
        BlazeBolt.SpriteSetPosition(player, x - speed, y)
    end
    if BlazeBolt.IsKeyPressed(Keys.D) then
        local x, y = BlazeBolt.SpriteGetPosition(player)
        BlazeBolt.SpriteSetPosition(player, x + speed, y)
    end
    
    -- Jump
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.PlaySound("jump")
    end
end

return game
```

### Particle System

```
-- particles.lua
local particles = {}

function CreateExplosion(x, y)
    for i = 1, 20 do
        local particle = {
            id = BlazeBolt.NewSprite("assets/particle.png", x, y),
            life = 1.0,
            vx = BlazeBolt.Random(-200, 200),
            vy = BlazeBolt.Random(-200, 200)
        }
        BlazeBolt.SpriteSetSize(particle.id, 8, 8)
        BlazeBolt.SpriteSetColor(particle.id, 1, BlazeBolt.Random(0.5, 1), 0, 1)
        table.insert(particles, particle)
    end
end

function UpdateParticles(dt)
    for i = #particles, 1, -1 do
        local p = particles[i]
        p.life = p.life - dt * 2
        
        local x, y = BlazeBolt.SpriteGetPosition(p.id)
        BlazeBolt.SpriteSetPosition(p.id, x + p.vx * dt, y + p.vy * dt)
        BlazeBolt.SpriteSetColor(p.id, 1, 1, 0, p.life)
        
        if p.life <= 0 then
            BlazeBolt.Destroy(p.id)
            table.remove(particles, i)
        end
    end
end

return {
    CreateExplosion = CreateExplosion,
    UpdateParticles = UpdateParticles
}
```

## Best Practices

**✅ DO:**

*   Always multiply movement by delta time for frame-independent speed
*   Cache entity IDs instead of looking them up repeatedly
*   Use meaningful variable names
*   Check return values for error handling
*   Organize code into modules
*   Pre-load assets at start
*   Use sprite sheets for animations to reduce draw calls

**❌ DON'T:**

*   Create/destroy objects every frame (use pooling)
*   Ignore delta time in movement calculations
*   Load assets during gameplay
*   Use blocking operations in Update
*   Forget to check if entities exist before using them

### Performance Tips

```
-- Bad: Creating objects every frame
function Update(dt)
    local particle = BlazeBolt.NewSprite("particle.png", x, y) -- DON'T DO THIS
end

-- Good: Object pooling
local particlePool = {}
function GetParticle()
    if #particlePool > 0 then
        return table.remove(particlePool)
    else
        return BlazeBolt.NewSprite("particle.png", 0, 0)
    end
end

function ReturnParticle(particle)
    BlazeBolt.SpriteSetVisible(particle, false)
    table.insert(particlePool, particle)
end
```

## Troubleshooting

### Common Issues

**❓ Sprite not appearing**

*   Check texture path is correct
*   Verify sprite is visible: `BlazeBolt.SpriteSetVisible(id, true)`
*   Check position is within screen bounds

**❓ Animation not playing**

*   Verify animation loaded correctly
*   Call `BlazeBolt.AnimationPlay(id)`
*   Check looping is set if needed

**❓ Sound not playing**

*   Verify WAV format (PCM, not compressed)
*   Check volume is not zero
*   Verify OpenAL is initialized

**❓ Input not responding**

*   Check key constants are correct
*   Verify window has focus

**❓ Script not loading**

*   Check file path is correct
*   Verify Lua syntax is valid
*   Check for missing dependencies

BlazeBolt Game Engine - Professional 2D Game Development Framework

© 2024 BlazeBolt. All rights reserved.

function toggleSidebar() { const sidebar = document.getElementById('sidebar'); sidebar.classList.toggle('open'); } function searchDocs() { const input = document.getElementById('searchInput'); const filter = input.value.toLowerCase(); const sections = document.querySelectorAll('.section'); sections.forEach(section => { const text = section.textContent.toLowerCase(); if (text.includes(filter)) { section.style.display = 'block'; } else { section.style.display = 'none'; } }); } // Close sidebar when clicking outside on mobile document.addEventListener('click', function(event) { const sidebar = document.getElementById('sidebar'); const menuBtn = document.querySelector('.menu-btn'); if (sidebar.classList.contains('open') && !sidebar.contains(event.target) && !menuBtn.contains(event.target)) { sidebar.classList.remove('open'); } }); // Smooth scrolling for anchor links document.querySelectorAll('a\[href^="#"\]').forEach(anchor => { anchor.addEventListener('click', function (e) { e.preventDefault(); const target = document.querySelector(this.getAttribute('href')); if (target) { target.scrollIntoView({ behavior: 'smooth' }); // Close sidebar on mobile after click if (window.innerWidth <= 768) { document.getElementById('sidebar').classList.remove('open'); } } }); }); // Highlight code blocks document.querySelectorAll('pre code').forEach(block => { // You can add syntax highlighting library here if needed });