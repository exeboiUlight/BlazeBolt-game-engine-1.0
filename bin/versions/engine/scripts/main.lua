local mesh
local entity
local x_s = 0
local y_s = 0

function Start()
    entity = BlazeBolt.CreateSprite("engine/assets/textures/wall.jpg", x_s, y_s)
end

function Update(dt)
    x_s = x_s + 0.01 * dt
    BlazeBolt.SpriteSetPosition(entity, x_s, y_s)
end

function Draw()
end

function End()
end