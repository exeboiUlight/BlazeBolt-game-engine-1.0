local game = {}
local player = nil

local position = {X=0, Y=0}

function Start()
    player = BlazeBolt.NewSprite("wall.jpg", 0, 0)
    BlazeBolt.SpriteSetSize(player, 1, 1)
end

function Update(dt)
    if BlazeBolt.IsKeyPressed(Keys.W) then
        position.Y = position.Y + 5 *dt
    end

    if BlazeBolt.IsKeyPressed(Keys.S) then
        position.Y = position.Y - 5 *dt
    end

    BlazeBolt.SpriteSetPosition(player, position.X, position.Y)
end

function Draw()
end

return game