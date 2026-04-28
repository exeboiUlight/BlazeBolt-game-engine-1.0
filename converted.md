# BlazeBolt Engine

## Getting Started

[Overview](#overview) [Installation](#installation) [Basic Setup](#basic-setup)

## Core API

[Core Systems](#core-systems) [Sprite API](#sprite-api) [Animation API](#animation-api) [Text API](#text-api) [Mesh API](#mesh-api) [Audio API](#audio-api) [Input API](#input-api)

## Scripting

[Script Management](#script-management) [Utility Functions](#utility-functions)

## Examples

[Complete Examples](#examples) [Best Practices](#best-practices) [Troubleshooting](#troubleshooting)

🎮 BlazeBolt Game Engine
========================

Professional 2D Game Development Framework

Version 1.0.0 | Lua-Powered

Overview
--------

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

Basic Setup
-----------

### Main Lua Script

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

### Key Constants

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

Core Systems
------------

### Print

    BlazeBolt.Print(value1, value2, ...)

Prints values to the console with automatic type conversion.

    BlazeBolt.Print("Hello", "World", 42, true)
    -- Output: Hello	World	42	true

### GetDeltaTime

    local dt = BlazeBolt.GetDeltaTime()

Returns the time since the last frame in seconds.

    function Update(dt)
        local fps = 1 / dt
        BlazeBolt.Print("FPS:", math.floor(fps))
    end

### Random Functions

    -- Random float between min and max
    local value = BlazeBolt.Random(min, max)
    
    -- Random integer between min and max (inclusive)
    local intValue = BlazeBolt.RandomInt(min, max)
    
    -- Set random seed
    BlazeBolt.SetRandomSeed(seed)

### Quit

    BlazeBolt.Quit()

Exits the game immediately.

Sprite API
----------

### NewSprite

    local spriteId = BlazeBolt.NewSprite(texturePath, x, y)

Creates a new sprite at position (x, y) with the specified texture.

Parameter

Type

Description

texturePath

string

Path to the texture image (PNG, JPG, etc.)

x

number

X position (default: 0)

y

number

Y position (default: 0)

### Sprite Methods

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

**💡 Tip:** Origin values range from 0 to 1. (0,0) is top-left, (0.5,0.5) is center, (1,1) is bottom-right.

### Complete Sprite Example

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

Animation API
-------------

### NewAnimation (GIF)

    local animId = BlazeBolt.NewAnimation(path, isGif, x, y)

### NewAnimationFromSheet (Sprite Sheet)

    local animId = BlazeBolt.NewAnimationFromSheet(
        texturePath, frameWidth, frameHeight, 
        totalFrames, framesPerRow, frameDelayMs, 
        x, y
    )

### Animation Control

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

**📝 Note:** GIF animations are loaded directly. Sprite sheets require frame dimensions and count.

### Animation Example

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

Text API
--------

### NewText

    local textId = BlazeBolt.NewText(fontPath, text, x, y, fontSize)

Parameter

Type

Description

fontPath

string

Path to TTF font file

text

string

Initial text content

x

number

X position (default: 0)

y

number

Y position (default: 0)

fontSize

integer

Font size in pixels (default: 48)

### Text Methods

    BlazeBolt.TextSetString(textId, text)
    local text = BlazeBolt.TextGetString(textId)
    BlazeBolt.TextSetPosition(textId, x, y)
    BlazeBolt.TextSetScale(textId, scale)
    BlazeBolt.TextSetColor(textId, r, g, b, a)
    BlazeBolt.TextSetVisible(textId, visible)

### Text Example

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

Audio API
---------

### LoadSound

    local soundId = BlazeBolt.LoadSound(filename, soundName, loop)

### Playback Control

    BlazeBolt.PlaySound(soundName)
    BlazeBolt.PlaySoundById(soundId)
    BlazeBolt.StopSound(soundName)
    BlazeBolt.StopSoundById(soundId)
    BlazeBolt.StopAllSounds()

### Sound Settings

    BlazeBolt.SetSoundVolume(soundName, volume)
    BlazeBolt.SetSoundVolumeById(soundId, volume)
    BlazeBolt.SetSoundLooping(soundName, loop)
    BlazeBolt.SetSoundLoopingById(soundId, loop)
    
    local isPlaying = BlazeBolt.IsSoundPlaying(soundName)

**⚠️ Note:** Audio files must be in WAV format (PCM, not compressed).

### Audio Example

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

Input API
---------

### Keyboard Input

    local pressed = BlazeBolt.IsKeyPressed(key)
    local justPressed = BlazeBolt.IsKeyJustPressed(key)
    local justReleased = BlazeBolt.IsKeyJustReleased(key)

### Mouse Input

    local mouseX = BlazeBolt.GetMouseX()
    local mouseY = BlazeBolt.GetMouseY()
    local deltaX = BlazeBolt.GetMouseDeltaX()
    local deltaY = BlazeBolt.GetMouseDeltaY()
    
    local leftPressed = BlazeBolt.IsMouseButtonPressed(MouseButtons.LEFT)
    local rightJustPressed = BlazeBolt.IsMouseButtonJustPressed(MouseButtons.RIGHT)
    
    local scrollY = BlazeBolt.GetScrollY()
    
    BlazeBolt.ResetScroll()
    BlazeBolt.ResetMouseDelta()

### Input Example

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

Script Management
-----------------

### Script List File (scripts.list)

    # Comments start with #
    main=scripts/main.lua
    player=scripts/player.lua
    ui=scripts/ui.lua
    enemies=scripts/enemies.lua

### Loading Scripts

    BlazeBolt.LoadScriptsFromList(listPath)
    BlazeBolt.LoadScript(scriptPath)
    
    BlazeBolt.ReloadScript(scriptPath)
    BlazeBolt.ReloadAllScripts()
    
    BlazeBolt.EnableScript(scriptName, enabled)
    local loaded = BlazeBolt.IsScriptLoaded(scriptName)
    local scripts = BlazeBolt.GetLoadedScripts()

### Script Organization Example

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

Utility Functions
-----------------

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

Complete Examples
-----------------

### Simple Platformer

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

### Particle System

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

Best Practices
--------------

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

Troubleshooting
---------------

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

© 2026 BlazeBolt. All rights reserved.