#pragma once
#include <string>

namespace LuaEngine {
    const std::string OOP_INIT_SCRIPT = R"lua(-- BlazeBolt OOP convenience wrappers
-- Loaded automatically by LuaEngine at startup

-- ==================== Sprite ====================
BlazeBolt.Sprite = {}
BlazeBolt.Sprite.__index = BlazeBolt.Sprite

function BlazeBolt.Sprite.new(texturePath, x, y)
    local id = BlazeBolt.CreateSprite(texturePath or "", x or 0, y or 0)
    return setmetatable({id = id, _type = "sprite"}, BlazeBolt.Sprite)
end

function BlazeBolt.Sprite:SetTexture(path)
    BlazeBolt.SpriteSetTexture(self.id, path)
end

function BlazeBolt.Sprite:SetPosition(x, y)
    BlazeBolt.SpriteSetPosition(self.id, x, y)
end

function BlazeBolt.Sprite:GetPosition()
    local x, y = BlazeBolt.SpriteGetPosition(self.id)
    return x, y
end

function BlazeBolt.Sprite:SetSize(w, h)
    BlazeBolt.SpriteSetSize(self.id, w, h)
end

function BlazeBolt.Sprite:GetSize()
    local w, h = BlazeBolt.SpriteGetSize(self.id)
    return w, h
end

function BlazeBolt.Sprite:SetOrigin(x, y)
    BlazeBolt.SpriteSetOrigin(self.id, x, y)
end

function BlazeBolt.Sprite:GetOrigin()
    local x, y = BlazeBolt.SpriteGetOrigin(self.id)
    return x, y
end

function BlazeBolt.Sprite:SetRotation(rot)
    BlazeBolt.SpriteSetRotation(self.id, rot)
end

function BlazeBolt.Sprite:GetRotation()
    return BlazeBolt.SpriteGetRotation(self.id)
end

function BlazeBolt.Sprite:SetColor(r, g, b, a)
    BlazeBolt.SpriteSetColor(self.id, r, g, b, a)
end

function BlazeBolt.Sprite:GetColor()
    local r, g, b, a = BlazeBolt.SpriteGetColor(self.id)
    return r, g, b, a
end

function BlazeBolt.Sprite:SetTextureRect(x, y, w, h)
    BlazeBolt.SpriteSetTextureRect(self.id, x, y, w, h)
end

function BlazeBolt.Sprite:SetVisible(visible)
    BlazeBolt.SpriteSetVisible(self.id, visible)
end

function BlazeBolt.Sprite:IsVisible()
    return BlazeBolt.SpriteIsVisible(self.id)
end

function BlazeBolt.Sprite:Destroy()
    BlazeBolt.Destroy(self.id)
end

-- ==================== AnimatedSprite ====================
BlazeBolt.AnimatedSprite = {}
BlazeBolt.AnimatedSprite.__index = BlazeBolt.AnimatedSprite

function BlazeBolt.AnimatedSprite.new(texturePath, x, y)
    local id = BlazeBolt.CreateAnimatedSprite(texturePath or "", x or 0, y or 0)
    return setmetatable({id = id, _type = "animatedsprite"}, BlazeBolt.AnimatedSprite)
end

function BlazeBolt.AnimatedSprite:Play()
    BlazeBolt.AnimatedSpritePlay(self.id)
end

function BlazeBolt.AnimatedSprite:IsPlaying()
    return BlazeBolt.AnimatedSpriteIsPlaying(self.id)
end

function BlazeBolt.AnimatedSprite:Pause()
    BlazeBolt.AnimatedSpritePause(self.id)
end

function BlazeBolt.AnimatedSprite:Stop()
    BlazeBolt.AnimatedSpriteStop(self.id)
end

function BlazeBolt.AnimatedSprite:Restart()
    BlazeBolt.AnimatedSpriteRestart(self.id)
end

function BlazeBolt.AnimatedSprite:SetTexture(path)
    BlazeBolt.AnimatedSpriteSetTexture(self.id, path)
end

function BlazeBolt.AnimatedSprite:SetLooping(loop)
    BlazeBolt.AnimatedSpriteSetLooping(self.id, loop)
end

function BlazeBolt.AnimatedSprite:IsLooping()
    return BlazeBolt.AnimatedSpriteIsLooping(self.id)
end

function BlazeBolt.AnimatedSprite:SetPlaybackSpeed(speed)
    BlazeBolt.AnimatedSpriteSetPlaybackSpeed(self.id, speed)
end

function BlazeBolt.AnimatedSprite:GetPlaybackSpeed()
    return BlazeBolt.AnimatedSpriteGetPlaybackSpeed(self.id)
end

function BlazeBolt.AnimatedSprite:SetFrame(frame)
    BlazeBolt.AnimatedSpriteSetFrame(self.id, frame)
end

function BlazeBolt.AnimatedSprite:GetCurrentFrame()
    return BlazeBolt.AnimatedSpriteGetCurrentFrame(self.id)
end

function BlazeBolt.AnimatedSprite:GetNumFrames()
    return BlazeBolt.AnimatedSpriteGetNumFrames(self.id)
end

function BlazeBolt.AnimatedSprite:SetPosition(x, y)
    BlazeBolt.AnimatedSpriteSetPosition(self.id, x, y)
end

function BlazeBolt.AnimatedSprite:GetPosition()
    local x, y = BlazeBolt.AnimatedSpriteGetPosition(self.id)
    return x, y
end

function BlazeBolt.AnimatedSprite:SetSize(w, h)
    BlazeBolt.AnimatedSpriteSetSize(self.id, w, h)
end

function BlazeBolt.AnimatedSprite:GetSize()
    local w, h = BlazeBolt.AnimatedSpriteGetSize(self.id)
    return w, h
end

function BlazeBolt.AnimatedSprite:SetOrigin(x, y)
    BlazeBolt.AnimatedSpriteSetOrigin(self.id, x, y)
end

function BlazeBolt.AnimatedSprite:GetOrigin()
    local x, y = BlazeBolt.AnimatedSpriteGetOrigin(self.id)
    return x, y
end

function BlazeBolt.AnimatedSprite:SetRotation(rot)
    BlazeBolt.AnimatedSpriteSetRotation(self.id, rot)
end

function BlazeBolt.AnimatedSprite:GetRotation()
    return BlazeBolt.AnimatedSpriteGetRotation(self.id)
end

function BlazeBolt.AnimatedSprite:SetColor(r, g, b, a)
    BlazeBolt.AnimatedSpriteSetColor(self.id, r, g, b, a)
end

function BlazeBolt.AnimatedSprite:GetColor()
    local r, g, b, a = BlazeBolt.AnimatedSpriteGetColor(self.id)
    return r, g, b, a
end

function BlazeBolt.AnimatedSprite:Destroy()
    BlazeBolt.Destroy(self.id)
end

-- ==================== Text ====================
BlazeBolt.Text = {}
BlazeBolt.Text.__index = BlazeBolt.Text

function BlazeBolt.Text.new(fontPath, text, x, y)
    local id = BlazeBolt.CreateText(fontPath or "", text or "", x or 0, y or 0)
    return setmetatable({id = id, _type = "text"}, BlazeBolt.Text)
end

function BlazeBolt.Text:SetString(text)
    BlazeBolt.TextSetString(self.id, text)
end

function BlazeBolt.Text:GetString()
    return BlazeBolt.TextGetString(self.id)
end

function BlazeBolt.Text:SetPosition(x, y)
    BlazeBolt.TextSetPosition(self.id, x, y)
end

function BlazeBolt.Text:GetPosition()
    local x, y = BlazeBolt.TextGetPosition(self.id)
    return x, y
end

function BlazeBolt.Text:SetColor(r, g, b, a)
    BlazeBolt.TextSetColor(self.id, r, g, b, a)
end

function BlazeBolt.Text:GetColor()
    local r, g, b, a = BlazeBolt.TextGetColor(self.id)
    return r, g, b, a
end

function BlazeBolt.Text:SetScale(sx, sy)
    BlazeBolt.TextSetScale(self.id, sx, sy)
end

function BlazeBolt.Text:GetScale()
    local sx, sy = BlazeBolt.TextGetScale(self.id)
    return sx, sy
end

function BlazeBolt.Text:SetOrigin(x, y)
    BlazeBolt.TextSetOrigin(self.id, x, y)
end

function BlazeBolt.Text:GetOrigin()
    local x, y = BlazeBolt.TextGetOrigin(self.id)
    return x, y
end

function BlazeBolt.Text:SetRotation(rot)
    BlazeBolt.TextSetRotation(self.id, rot)
end

function BlazeBolt.Text:GetRotation()
    return BlazeBolt.TextGetRotation(self.id)
end

function BlazeBolt.Text:SetAlignment(align)
    BlazeBolt.TextSetAlignment(self.id, align)
end

function BlazeBolt.Text:GetAlignment()
    return BlazeBolt.TextGetAlignment(self.id)
end

function BlazeBolt.Text:SetVisible(visible)
    BlazeBolt.TextSetVisible(self.id, visible)
end

function BlazeBolt.Text:IsVisible()
    return BlazeBolt.TextIsVisible(self.id)
end

function BlazeBolt.Text:Destroy()
    BlazeBolt.Destroy(self.id)
end

-- ==================== Camera ====================
BlazeBolt.Camera = {}
BlazeBolt.Camera.__index = BlazeBolt.Camera

function BlazeBolt.Camera.new()
    local id = BlazeBolt.CreateCamera()
    return setmetatable({id = id, _type = "camera"}, BlazeBolt.Camera)
end

function BlazeBolt.Camera:SetPosition(x, y)
    BlazeBolt.CameraSetPosition(self.id, x, y)
end

function BlazeBolt.Camera:GetPosition()
    local x, y = BlazeBolt.CameraGetPosition(self.id)
    return x, y
end

function BlazeBolt.Camera:SetZoom(zoom)
    BlazeBolt.CameraSetZoom(self.id, zoom)
end

function BlazeBolt.Camera:GetZoom()
    return BlazeBolt.CameraGetZoom(self.id)
end

function BlazeBolt.Camera:SetRotation(rot)
    BlazeBolt.CameraSetRotation(self.id, rot)
end

function BlazeBolt.Camera:GetRotation()
    return BlazeBolt.CameraGetRotation(self.id)
end

-- ==================== Tileset ====================
BlazeBolt.Tileset = {}
BlazeBolt.Tileset.__index = BlazeBolt.Tileset

function BlazeBolt.Tileset.new(texturePath, tileW, tileH, atlasCols, atlasRows)
    local id = BlazeBolt.CreateTileset(texturePath or "", tileW or 32, tileH or 32, atlasCols or 1, atlasRows or 1)
    return setmetatable({id = id, _type = "tileset"}, BlazeBolt.Tileset)
end

function BlazeBolt.Tileset:SetMap(map)
    BlazeBolt.TilesetSetMap(self.id, map)
end

function BlazeBolt.Tileset:GetTile(col, row)
    return BlazeBolt.TilesetGetTile(self.id, col, row)
end

function BlazeBolt.Tileset:SetTile(col, row, tileIndex)
    BlazeBolt.TilesetSetTile(self.id, col, row, tileIndex)
end

function BlazeBolt.Tileset:SetTileSize(w, h)
    BlazeBolt.TilesetSetTileSize(self.id, w, h)
end

function BlazeBolt.Tileset:GetTileSize()
    local w, h = BlazeBolt.TilesetGetTileSize(self.id)
    return w, h
end

function BlazeBolt.Tileset:SetPosition(x, y)
    BlazeBolt.TilesetSetPosition(self.id, x, y)
end

function BlazeBolt.Tileset:GetPosition()
    local x, y = BlazeBolt.TilesetGetPosition(self.id)
    return x, y
end

function BlazeBolt.Tileset:GetMapWidth()
    return BlazeBolt.TilesetGetMapWidth(self.id)
end

function BlazeBolt.Tileset:GetMapHeight()
    return BlazeBolt.TilesetGetMapHeight(self.id)
end

function BlazeBolt.Tileset:GetTileCount()
    return BlazeBolt.TilesetGetTileCount(self.id)
end

function BlazeBolt.Tileset:Draw()
    BlazeBolt.TilesetDraw(self.id)
end

function BlazeBolt.Tileset:Destroy()
    BlazeBolt.DestroyTileset(self.id)
end

-- ==================== ParticleSystem ====================
BlazeBolt.ParticleSystem = {}
BlazeBolt.ParticleSystem.__index = BlazeBolt.ParticleSystem

function BlazeBolt.ParticleSystem.new()
    local id = BlazeBolt.CreateParticleSystem()
    return setmetatable({id = id, _type = "particlesystem"}, BlazeBolt.ParticleSystem)
end

function BlazeBolt.ParticleSystem:SetPosition(x, y)
    BlazeBolt.ParticleSystemSetPosition(self.id, x, y)
end

function BlazeBolt.ParticleSystem:SetTexture(path)
    BlazeBolt.ParticleSystemSetTexture(self.id, path)
end

function BlazeBolt.ParticleSystem:SetEmissionRate(rate)
    BlazeBolt.ParticleSystemSetEmissionRate(self.id, rate)
end

function BlazeBolt.ParticleSystem:GetEmissionRate()
    return BlazeBolt.ParticleSystemGetEmissionRate(self.id)
end

function BlazeBolt.ParticleSystem:SetLifetime(min, max)
    BlazeBolt.ParticleSystemSetLifetime(self.id, min, max)
end

function BlazeBolt.ParticleSystem:SetSpeed(min, max)
    BlazeBolt.ParticleSystemSetSpeed(self.id, min, max)
end

function BlazeBolt.ParticleSystem:SetSize(min, max)
    BlazeBolt.ParticleSystemSetSize(self.id, min, max)
end

function BlazeBolt.ParticleSystem:SetEndSize(min, max)
    BlazeBolt.ParticleSystemSetEndSize(self.id, min, max)
end

function BlazeBolt.ParticleSystem:SetColor(r, g, b, a, endR, endG, endB, endA)
    BlazeBolt.ParticleSystemSetColor(self.id, r, g, b, a, endR or r, endG or g, endB or b, endA or a)
end

function BlazeBolt.ParticleSystem:SetDirection(minAngle, maxAngle)
    BlazeBolt.ParticleSystemSetDirection(self.id, minAngle, maxAngle)
end

function BlazeBolt.ParticleSystem:SetRotationSpeed(speed)
    BlazeBolt.ParticleSystemSetRotationSpeed(self.id, speed)
end

function BlazeBolt.ParticleSystem:SetActive(active)
    BlazeBolt.ParticleSystemSetActive(self.id, active)
end

function BlazeBolt.ParticleSystem:IsActive()
    return BlazeBolt.ParticleSystemIsActive(self.id)
end

function BlazeBolt.ParticleSystem:SetVisible(visible)
    BlazeBolt.ParticleSystemSetVisible(self.id, visible)
end

function BlazeBolt.ParticleSystem:IsVisible()
    return BlazeBolt.ParticleSystemIsVisible(self.id)
end

function BlazeBolt.ParticleSystem:Emit(count)
    BlazeBolt.ParticleSystemEmit(self.id, count or 1)
end

function BlazeBolt.ParticleSystem:Clear()
    BlazeBolt.ParticleSystemClear(self.id)
end

function BlazeBolt.ParticleSystem:GetCount()
    return BlazeBolt.ParticleSystemGetCount(self.id)
end

-- ==================== Light ====================
BlazeBolt.Light = {}
BlazeBolt.Light.__index = BlazeBolt.Light

function BlazeBolt.Light.newPoint(x, y, r, g, b, intensity, radius)
    local id = BlazeBolt.CreatePointLight(x or 0, y or 0, r or 1, g or 1, b or 1, intensity or 1, radius or 300)
    return setmetatable({id = id, _type = "light"}, BlazeBolt.Light)
end

function BlazeBolt.Light.newAmbient(r, g, b, intensity)
    local id = BlazeBolt.CreateAmbientLight(r or 1, g or 1, b or 1, intensity or 0.3)
    return setmetatable({id = id, _type = "light"}, BlazeBolt.Light)
end

function BlazeBolt.Light:SetPosition(x, y)
    BlazeBolt.LightSetPosition(self.id, x, y)
end

function BlazeBolt.Light:GetPosition()
    local x, y = BlazeBolt.LightGetPosition(self.id)
    return x, y
end

function BlazeBolt.Light:SetColor(r, g, b)
    BlazeBolt.LightSetColor(self.id, r, g, b)
end

function BlazeBolt.Light:GetColor()
    local r, g, b = BlazeBolt.LightGetColor(self.id)
    return r, g, b
end

function BlazeBolt.Light:SetIntensity(intensity)
    BlazeBolt.LightSetIntensity(self.id, intensity)
end

function BlazeBolt.Light:GetIntensity()
    return BlazeBolt.LightGetIntensity(self.id)
end

function BlazeBolt.Light:SetRadius(radius)
    BlazeBolt.LightSetRadius(self.id, radius)
end

function BlazeBolt.Light:GetRadius()
    return BlazeBolt.LightGetRadius(self.id)
end

function BlazeBolt.Light:SetEnabled(enabled)
    BlazeBolt.LightSetEnabled(self.id, enabled)
end

function BlazeBolt.Light:GetEnabled()
    return BlazeBolt.LightGetEnabled(self.id)
end

function BlazeBolt.Light:Destroy()
    BlazeBolt.DestroyLight(self.id)
end

-- ==================== Mesh ====================
BlazeBolt.Mesh = {}
BlazeBolt.Mesh.__index = BlazeBolt.Mesh

function BlazeBolt.Mesh.new()
    local id = BlazeBolt.CreateMesh()
    return setmetatable({id = id, _type = "mesh"}, BlazeBolt.Mesh)
end

function BlazeBolt.Mesh:SetData(vertices, indices)
    BlazeBolt.MeshSetData(self.id, vertices, indices)
end

function BlazeBolt.Mesh:SetShader(vertexPath, fragmentPath)
    BlazeBolt.MeshSetShader(self.id, vertexPath, fragmentPath)
end

function BlazeBolt.Mesh:SetUniformFloat(name, value)
    BlazeBolt.MeshSetUniformFloat(self.id, name, value)
end

function BlazeBolt.Mesh:SetUniformInt(name, value)
    BlazeBolt.MeshSetUniformInt(self.id, name, value)
end

function BlazeBolt.Mesh:SetUniformVec2(name, x, y)
    BlazeBolt.MeshSetUniformVec2(self.id, name, x, y)
end

function BlazeBolt.Mesh:SetUniformVec3(name, x, y, z)
    BlazeBolt.MeshSetUniformVec3(self.id, name, x, y, z)
end

function BlazeBolt.Mesh:SetUniformVec4(name, x, y, z, w)
    BlazeBolt.MeshSetUniformVec4(self.id, name, x, y, z, w)
end

function BlazeBolt.Mesh:Draw()
    BlazeBolt.MeshDraw(self.id)
end

-- ==================== SpriteBatch ====================
BlazeBolt.SpriteBatch = {}
BlazeBolt.SpriteBatch.__index = BlazeBolt.SpriteBatch

function BlazeBolt.SpriteBatch.new(maxSize)
    local id = BlazeBolt.CreateSpriteBatch(maxSize or 25)
    return setmetatable({id = id, _type = "spritebatch"}, BlazeBolt.SpriteBatch)
end

function BlazeBolt.SpriteBatch:SetTexture(path)
    BlazeBolt.SpriteBatchSetTexture(self.id, path)
end

function BlazeBolt.SpriteBatch:Add(sprite)
    local spriteId = type(sprite) == "table" and sprite.id or sprite
    return BlazeBolt.SpriteBatchAdd(self.id, spriteId)
end

function BlazeBolt.SpriteBatch:Remove(sprite)
    local spriteId = type(sprite) == "table" and sprite.id or sprite
    return BlazeBolt.SpriteBatchRemove(self.id, spriteId)
end

function BlazeBolt.SpriteBatch:Clear()
    BlazeBolt.SpriteBatchClear(self.id)
end

function BlazeBolt.SpriteBatch:SetMaxSize(maxSize)
    BlazeBolt.SpriteBatchSetMaxSize(self.id, maxSize)
end

function BlazeBolt.SpriteBatch:GetMaxSize()
    return BlazeBolt.SpriteBatchGetMaxSize(self.id)
end

function BlazeBolt.SpriteBatch:GetCount()
    return BlazeBolt.SpriteBatchGetCount(self.id)
end

function BlazeBolt.SpriteBatch:Draw()
    BlazeBolt.SpriteBatchDraw(self.id)
end

function BlazeBolt.SpriteBatch:Destroy()
    BlazeBolt.DestroySpriteBatch(self.id)
end

-- ==================== AnimationWheel ====================
BlazeBolt.AnimationWheel = {}
BlazeBolt.AnimationWheel.__index = BlazeBolt.AnimationWheel

function BlazeBolt.AnimationWheel.new(animatedSprite)
    local animId = type(animatedSprite) == "table" and animatedSprite.id or animatedSprite
    local id = BlazeBolt.CreateAnimationWheel(animId)
    return setmetatable({id = id, _type = "animationwheel"}, BlazeBolt.AnimationWheel)
end

function BlazeBolt.AnimationWheel:AddState(name, gifPath, playbackSpeed, looping)
    BlazeBolt.AnimationWheelAddState(self.id, name, gifPath, playbackSpeed or 1.0, looping or false)
end

function BlazeBolt.AnimationWheel:RemoveState(name)
    BlazeBolt.AnimationWheelRemoveState(self.id, name)
end

function BlazeBolt.AnimationWheel:HasState(name)
    return BlazeBolt.AnimationWheelHasState(self.id, name)
end

function BlazeBolt.AnimationWheel:SetInitialState(name)
    BlazeBolt.AnimationWheelSetInitialState(self.id, name)
end

function BlazeBolt.AnimationWheel:GetInitialState()
    return BlazeBolt.AnimationWheelGetInitialState(self.id)
end

function BlazeBolt.AnimationWheel:SetState(name)
    BlazeBolt.AnimationWheelSetState(self.id, name)
end

function BlazeBolt.AnimationWheel:GetState()
    return BlazeBolt.AnimationWheelGetState(self.id)
end

function BlazeBolt.AnimationWheel:SetPlaybackSpeed(stateName, speed)
    BlazeBolt.AnimationWheelSetPlaybackSpeed(self.id, stateName, speed)
end

function BlazeBolt.AnimationWheel:GetPlaybackSpeed(stateName)
    return BlazeBolt.AnimationWheelGetPlaybackSpeed(self.id, stateName)
end

function BlazeBolt.AnimationWheel:SetLooping(stateName, looping)
    BlazeBolt.AnimationWheelSetLooping(self.id, stateName, looping)
end

function BlazeBolt.AnimationWheel:IsLooping(stateName)
    return BlazeBolt.AnimationWheelIsLooping(self.id, stateName)
end

function BlazeBolt.AnimationWheel:Destroy()
    BlazeBolt.DestroyAnimationWheel(self.id)
end

-- ==================== Physics API (convenience) ====================
BlazeBolt.Physics = {}

function BlazeBolt.Physics.Init(gravityX, gravityY)
    return BlazeBolt.PhysicsInit(gravityX or 0, gravityY or 9.81)
end

function BlazeBolt.Physics.SetGravity(x, y)
    BlazeBolt.PhysicsSetGravity(x, y)
end

function BlazeBolt.Physics.GetGravity()
    local x, y = BlazeBolt.PhysicsGetGravity()
    return x, y
end

function BlazeBolt.Physics.CreateBody(bodyType, x, y, mass, friction, restitution)
    return BlazeBolt.PhysicsCreateBody(bodyType, x or 0, y or 0, mass or 1, friction or 0.3, restitution or 0)
end

function BlazeBolt.Physics.AddCircle(body, radius, offsetX, offsetY)
    local bodyId = type(body) == "table" and body.id or body
    BlazeBolt.PhysicsAddCircle(bodyId, radius, offsetX or 0, offsetY or 0)
end

function BlazeBolt.Physics.AddRectangle(body, halfWidth, halfHeight)
    local bodyId = type(body) == "table" and body.id or body
    BlazeBolt.PhysicsAddRectangle(bodyId, halfWidth, halfHeight)
end

function BlazeBolt.Physics.SetLinearVelocity(body, vx, vy)
    local bodyId = type(body) == "table" and body.id or body
    BlazeBolt.PhysicsSetLinearVelocity(bodyId, vx, vy)
end

function BlazeBolt.Physics.GetLinearVelocity(body)
    local bodyId = type(body) == "table" and body.id or body
    local vx, vy = BlazeBolt.PhysicsGetLinearVelocity(bodyId)
    return vx, vy
end

function BlazeBolt.Physics.Step()
    BlazeBolt.PhysicsStep()
end

function BlazeBolt.Physics.SyncSprite(body, sprite)
    local bodyId = type(body) == "table" and body.id or body
    local spriteId = type(sprite) == "table" and sprite.id or sprite
    BlazeBolt.PhysicsSyncSprite(bodyId, spriteId)
end

function BlazeBolt.Physics.SyncText(body, text)
    local bodyId = type(body) == "table" and body.id or body
    local textId = type(text) == "table" and text.id or text
    BlazeBolt.PhysicsSyncText(bodyId, textId)
end

-- ==================== Audio convenience ====================
BlazeBolt.Audio = {}

function BlazeBolt.Audio.LoadSound(filename, soundName, loop)
    return BlazeBolt.LoadSound(filename, soundName or "", loop or false)
end

function BlazeBolt.Audio.Play(soundName)
    BlazeBolt.PlaySound(soundName)
end

function BlazeBolt.Audio.PlayById(id)
    BlazeBolt.PlaySoundById(id)
end

function BlazeBolt.Audio.Stop(soundName)
    BlazeBolt.StopSound(soundName)
end

function BlazeBolt.Audio.StopAll()
    BlazeBolt.StopAllSounds()
end

function BlazeBolt.Audio.SetVolume(soundName, volume)
    BlazeBolt.SetSoundVolume(soundName, volume)
end

function BlazeBolt.Audio.IsPlaying(soundName)
    return BlazeBolt.IsSoundPlaying(soundName)
end

-- ==================== Input convenience ====================
BlazeBolt.Input = {}

function BlazeBolt.Input.IsKeyPressed(key)
    return BlazeBolt.IsKeyPressed(key)
end

function BlazeBolt.Input.IsKeyJustPressed(key)
    return BlazeBolt.IsKeyJustPressed(key)
end

function BlazeBolt.Input.GetMousePosition()
    return BlazeBolt.GetMouseX(), BlazeBolt.GetMouseY()
end

function BlazeBolt.Input.GetMouseDelta()
    return BlazeBolt.GetMouseDeltaX(), BlazeBolt.GetMouseDeltaY()
end

function BlazeBolt.Input.IsMouseButtonPressed(btn)
    return BlazeBolt.IsMouseButtonPressed(btn)
end

function BlazeBolt.Input.IsMouseButtonJustPressed(btn)
    return BlazeBolt.IsMouseButtonJustPressed(btn)
end

function BlazeBolt.Input.GetScroll()
    return BlazeBolt.GetScrollY()
end

print("[Lua] BlazeBolt OOP wrappers loaded")
)lua";
}
