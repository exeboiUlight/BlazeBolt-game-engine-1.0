-- =============================================
-- AnimationWheel — пример использования колеса состояний
-- =============================================

local sprite
local wheel
local stateLabel

function Start()
    -- 1. Создаём анимированный спрайт (начальное состояние загрузится позже)
    sprite = BlazeBolt.CreateAnimatedSprite("assets/engine/assets/textures/bongo.png", 400, 300)
    BlazeBolt.AnimatedSpriteSetSize(sprite, 200, 200)

    -- 2. Создаём колесо состояний, привязанное к спрайту
    wheel = BlazeBolt.CreateAnimationWheel(sprite)

    -- 3. Добавляем состояния: имя → gif путь, скорость, зацикливание
    BlazeBolt.AnimationWheelAddState(wheel, "idle",  "assets/idle.gif",  1.0, true)
    BlazeBolt.AnimationWheelAddState(wheel, "run",   "assets/run.gif",   1.5, true)
    BlazeBolt.AnimationWheelAddState(wheel, "jump",  "assets/jump.gif",  1.0, false)
    BlazeBolt.AnimationWheelAddState(wheel, "attack","assets/attack.gif", 2.0, false)

    -- 4. Можно настроить параметры каждого состояния
    BlazeBolt.AnimationWheelSetPlaybackSpeed(wheel, "run", 2.0)
    BlazeBolt.AnimationWheelSetLooping(wheel, "jump", false)

    -- 5. Устанавливаем начальное состояние
    BlazeBolt.AnimationWheelSetInitialState(wheel, "idle")
    BlazeBolt.AnimationWheelSetState(wheel, "idle")

    -- Текстовая метка для отображения текущего состояния
    stateLabel = BlazeBolt.CreateText("assets/engine/arial.ttf", "State: idle", 10, 10)
end

function Update(dt)
    -- Переключение состояний по клавишам
    if BlazeBolt.IsKeyJustPressed(Keys.ONE) then
        BlazeBolt.AnimationWheelSetState(wheel, "idle")
    elseif BlazeBolt.IsKeyJustPressed(Keys.TWO) then
        BlazeBolt.AnimationWheelSetState(wheel, "run")
    elseif BlazeBolt.IsKeyJustPressed(Keys.THREE) then
        BlazeBolt.AnimationWheelSetState(wheel, "jump")
    elseif BlazeBolt.IsKeyJustPressed(Keys.FOUR) then
        BlazeBolt.AnimationWheelSetState(wheel, "attack")
    end

    -- Обновляем текстовую метку
    local currentState = BlazeBolt.AnimationWheelGetState(wheel)
    BlazeBolt.TextSetString(stateLabel, "State: " .. currentState)
end

function Draw()
    -- Отрисовка происходит автоматически через render order
end
