# BlazeBolt Lua API 1.0

Полная документация по Lua API игрового движка BlazeBolt 1.0.

## Оглавление
- [Система координат](#система-координат)
- [Жизненный цикл](#жизненный-цикл)
- [Спрайты](#спрайты)
- [Анимации](#анимации)
- [Текст](#текст)
- [Меши](#меши)
- [Окно](#окно)
- [Ввод](#ввод)
- [Звук](#звук)
- [Управление объектами](#управление-объектами)
- [Физика](#физика)
- [Утилиты](#утилиты)
- [Скрипты и сцены](#скрипты-и-сцены)

---

## Система координат

Движок использует стандартные координаты OpenGL NDC (Normalized Device Coordinates):

- **Центр экрана:** `(0, 0)`
- **Правый верхний угол:** `(1, 1)`
- **Левый нижний угол:** `(-1, -1)`
- **Правый нижний угол:** `(1, -1)`
- **Левый верхний угол:** `(-1, 1)`

Размеры задаются в NDC: `0.1` = 10% от высоты экрана.

### Конвертация пикселей в NDC
```lua
function pixelsToNDC(px, py)
    local ndcX = (px / GetScreenWidth()) * 2 - 1
    local ndcY = 1 - (py / GetScreenHeight()) * 2
    return ndcX, ndcY
end
```

---

## Жизненный цикл

### Callbacks
```lua
function Start()       -- вызывается один раз при запуске
function Update(dt)    -- вызывается каждый кадр (dt в секундах)
function Draw()        -- вызывается для отрисовки каждый кадр
function End()         -- вызывается при завершении работы
```

### Callbacks сцен
```lua
function OnMenuLoad()    -- при загрузке сцены "Menu"
function OnMenuUnload()  -- при выгрузке сцены "Menu"
```
Имя функции формируется как `On<SceneName>Load` / `On<SceneName>Unload`.

---

## Спрайты

### Создание
```lua
entity = BlazeBolt.CreateSprite(texturePath, x, y)
```
| Параметр | Тип | Описание |
|---|---|---|
| texturePath | string | Путь к файлу текстуры (PNG) |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `entity` (integer) — идентификатор объекта

### Позиция
```lua
BlazeBolt.SpriteSetPosition(entity, x, y)
x, y = BlazeBolt.SpriteGetPosition(entity)
```

### Размер
```lua
BlazeBolt.SpriteSetSize(entity, width, height)
width, height = BlazeBolt.SpriteGetSize(entity)
```

### Точка привязки
```lua
BlazeBolt.SpriteSetOrigin(entity, originX, originY)
originX, originY = BlazeBolt.SpriteGetOrigin(entity)
```
Значения от `0` до `1`. `(0.5, 0.5)` — центр, `(0, 0)` — левый нижний угол.

### Вращение
```lua
BlazeBolt.SpriteSetRotation(entity, degrees)
degrees = BlazeBolt.SpriteGetRotation(entity)
```
Угол в градусах.

### Цвет
```lua
BlazeBolt.SpriteSetColor(entity, r, g, b, a)
r, g, b, a = BlazeBolt.SpriteGetColor(entity)
```
Значения от `0` до `1`. По умолчанию `(1, 1, 1, 1)` — белый.

### Текстура
```lua
BlazeBolt.SpriteSetTexture(entity, texturePath)
```
Смена текстуры во время выполнения.

### Видимость
```lua
BlazeBolt.SpriteSetVisible(entity, visible)
visible = BlazeBolt.SpriteIsVisible(entity)
```
`visible` — `true` или `false`.

### Полный пример
```lua
function Start()
    local sprite = BlazeBolt.CreateSprite("icon.png", 0, 0)
    BlazeBolt.SpriteSetSize(sprite, 0.3, 0.3)
    BlazeBolt.SpriteSetOrigin(sprite, 0.5, 0.5)
    BlazeBolt.SpriteSetColor(sprite, 1, 0.5, 0, 1)
    BlazeBolt.SpriteSetRotation(sprite, 45)
end
```

---

## Анимации

### Создание из GIF
```lua
entity = BlazeBolt.CreateAnimation(gifPath, true, x, y)
```

### Создание из спрайт-листа
```lua
entity = BlazeBolt.CreateAnimationFromSheet(texturePath, frameWidth, frameHeight, totalFrames, framesPerRow, frameDelayMs, x, y)
```
| Параметр | Тип | Описание |
|---|---|---|
| texturePath | string | Путь к спрайт-листу |
| frameWidth | int | Ширина одного кадра (px) |
| frameHeight | int | Высота одного кадра (px) |
| totalFrames | int | Общее количество кадров |
| framesPerRow | int | Количество кадров в строке |
| frameDelayMs | int | Задержка кадра (мс), по умолчанию 100 |
| x, y | number | Позиция в NDC |

### Воспроизведение
```lua
BlazeBolt.AnimationPlay(entity)
BlazeBolt.AnimationPause(entity)
BlazeBolt.AnimationStop(entity)        -- остановить, сброс на первый кадр
BlazeBolt.AnimationRestart(entity)     -- перезапустить с начала
```

### Настройки
```lua
BlazeBolt.AnimationSetLooping(entity, looping)  -- зацикливание (boolean)
BlazeBolt.AnimationSetSpeed(entity, speed)       -- множитель скорости (1.0 = норма)
BlazeBolt.AnimationSetFrame(entity, frameIndex)  -- перейти к конкретному кадру
```

### Информация
```lua
frameCount = BlazeBolt.AnimationGetFrameCount(entity)
isPlaying  = BlazeBolt.AnimationIsPlaying(entity)
```

### Позиция и размер
```lua
BlazeBolt.AnimationSetPosition(entity, x, y)
BlazeBolt.AnimationSetSize(entity, width, height)
```

### Пример
```lua
function Start()
    local anim = BlazeBolt.CreateAnimationFromSheet("player_sheet.png", 32, 32, 8, 4, 100, 0, 0)
    BlazeBolt.AnimationSetSize(anim, 0.4, 0.4)
    BlazeBolt.AnimationSetLooping(anim, true)
    BlazeBolt.AnimationSetSpeed(anim, 2.0)
end

function Update(dt)
    if not BlazeBolt.AnimationIsPlaying(anim) then
        BlazeBolt.AnimationRestart(anim)
    end
end
```

---

## Текст

### Создание
```lua
entity = BlazeBolt.CreateText(fontPath, text, x, y)
```
| Параметр | Тип | Описание |
|---|---|---|
| fontPath | string | Путь к TTF-шрифту |
| text | string | Текст для отображения |
| x, y | number | Позиция в NDC |

**Возвращает:** `entity` (integer) — идентификатор объекта

### Содержимое
```lua
BlazeBolt.TextSetString(entity, text)
text = BlazeBolt.TextGetString(entity)
```

### Позиция
```lua
BlazeBolt.TextSetPosition(entity, x, y)
x, y = BlazeBolt.TextGetPosition(entity)
```

### Цвет
```lua
BlazeBolt.TextSetColor(entity, r, g, b, a)
r, g, b, a = BlazeBolt.TextGetColor(entity)
```
Значения от `0` до `1`.

### Масштаб
```lua
BlazeBolt.TextSetScale(entity, scaleX, scaleY)
scaleX, scaleY = BlazeBolt.TextGetScale(entity)
```

### Точка привязки
```lua
BlazeBolt.TextSetOrigin(entity, originX, originY)
originX, originY = BlazeBolt.TextGetOrigin(entity)
```
Значения от `0` до `1`. `(0.5, 0.5)` — центр текста.

### Вращение
```lua
BlazeBolt.TextSetRotation(entity, degrees)
degrees = BlazeBolt.TextGetRotation(entity)
```
Угол в градусах.

### Выравнивание
```lua
BlazeBolt.TextSetAlignment(entity, alignment)
alignment = BlazeBolt.TextGetAlignment(entity)
```
| Константа | Описание |
|---|---|
| `TextAlignment.LEFT` | По левому краю (по умолчанию) |
| `TextAlignment.CENTER` | По центру |
| `TextAlignment.RIGHT` | По правому краю |

### Видимость
```lua
BlazeBolt.TextSetVisible(entity, visible)
visible = BlazeBolt.TextIsVisible(entity)
```

### Пример
```lua
function Start()
    fpsText = BlazeBolt.CreateText('fonts/arial.ttf', 'FPS: 60', -0.8, 0.8)
    BlazeBolt.TextSetColor(fpsText, 1, 1, 1, 1)
    BlazeBolt.TextSetScale(fpsText, 0.1, 0.1)
    BlazeBolt.TextSetOrigin(fpsText, 0.5, 0.5)
    BlazeBolt.TextSetAlignment(fpsText, TextAlignment.CENTER)
end

function Update(dt)
    local fps = math.floor(1 / dt)
    BlazeBolt.TextSetString(fpsText, 'FPS: ' .. fps)
end
```

---

## Меши

### Создание
```lua
entity = BlazeBolt.CreateMesh()
```

### Установка данных
```lua
BlazeBolt.MeshSetData(entity, vertices, indices)
```

`vertices` — таблица вершин, каждая вершина:
```lua
{ x = <number>, y = <number>, u = <number>, v = <number> }
```
где `x, y` — позиция в NDC, `u, v` — UV-координаты текстуры (0-1).

`indices` — таблица индексов для отрисовки треугольников.

### Отрисовка
```lua
BlazeBolt.MeshDraw(entity)
```

### Пример
```lua
function Start()
    local mesh = BlazeBolt.CreateMesh()
    local vertices = {
        { x = -0.5, y =  0.5, u = 0, v = 1 },
        { x =  0.5, y =  0.5, u = 1, v = 1 },
        { x =  0.5, y = -0.5, u = 1, v = 0 },
        { x = -0.5, y = -0.5, u = 0, v = 0 }
    }
    local indices = { 0, 1, 2, 2, 3, 0 }
    BlazeBolt.MeshSetData(mesh, vertices, indices)
end

function Draw()
    BlazeBolt.MeshDraw(mesh)
end
```

---

## Окно

### Главное окно — установка
```lua
BlazeBolt.SetMainWindowTitle(title)
BlazeBolt.SetMainWindowSize(width, height)
BlazeBolt.SetMainWindowPosition(x, y)
BlazeBolt.SetMainWindowIcon(iconPath)
BlazeBolt.SetMainWindowShouldClose(flag)
```

### Главное окно — получение
```lua
title  = BlazeBolt.GetMainWindowTitle()          -- string
width  = BlazeBolt.GetMainWindowWidth()          -- integer (px)
height = BlazeBolt.GetMainWindowHeight()         -- integer (px)
x, y   = BlazeBolt.GetMainWindowPosition()       -- integer, integer (px)
should = BlazeBolt.IsMainWindowShouldClose()     -- boolean
```

### Дополнительные окна
```lua
windowPtr = BlazeBolt.CreateWindow(width, height, title)
BlazeBolt.SetWindowTitle(windowPtr, title)
BlazeBolt.SetWindowSize(windowPtr, width, height)
BlazeBolt.SetWindowIcon(windowPtr, iconPath)
```

### Размеры экрана
```lua
width  = GetScreenWidth()    -- integer (px), аналог BlazeBolt.GetMainWindowWidth()
height = GetScreenHeight()   -- integer (px), аналог BlazeBolt.GetMainWindowHeight()
```

### Пример
```lua
function Start()
    BlazeBolt.SetMainWindowTitle("My Game")
    print("Window: " .. GetScreenWidth() .. "x" .. GetScreenHeight())
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.F11) then
        BlazeBolt.SetMainWindowSize(1920, 1080)
    end
end
```

---

## Ввод

### Клавиатура
```lua
isDown    = BlazeBolt.IsKeyPressed(key)          -- зажата ли клавиша сейчас
isPressed = BlazeBolt.IsKeyJustPressed(key)      -- нажата в этом кадре
isReleased = BlazeBolt.IsKeyJustReleased(key)    -- отпущена в этом кадре
```

### Мышь — позиция
```lua
x  = BlazeBolt.GetMouseX()            -- позиция X курсора (px от левого края)
y  = BlazeBolt.GetMouseY()            -- позиция Y курсора (px от верхнего края)
dx = BlazeBolt.GetMouseDeltaX()       -- изменение X с прошлого кадра
dy = BlazeBolt.GetMouseDeltaY()       -- изменение Y с прошлого кадра
```

### Мышь — кнопки
```lua
isDown    = BlazeBolt.IsMouseButtonPressed(button)
isPressed = BlazeBolt.IsMouseButtonJustPressed(button)
```

### Мышь — прокрутка
```lua
scroll = BlazeBolt.GetScrollY()       -- значение прокрутки за кадр
```

### Константы клавиш
```lua
-- Буквы
Keys.A, Keys.B, Keys.C, ..., Keys.Z

-- Цифры
Keys._0, Keys._1, ..., Keys._9

-- Стрелки
Keys.UP, Keys.DOWN, Keys.LEFT, Keys.RIGHT

-- Специальные
Keys.SPACE, Keys.ENTER, Keys.ESCAPE
Keys.TAB, Keys.BACKSPACE, Keys.DELETE

-- Модификаторы
Keys.LEFT_SHIFT, Keys.RIGHT_SHIFT
Keys.LEFT_CONTROL, Keys.RIGHT_CONTROL
Keys.LEFT_ALT, Keys.RIGHT_ALT

-- Функциональные
Keys.F1, Keys.F2, ..., Keys.F12
```

### Константы кнопок мыши
```lua
MouseButtons.LEFT
MouseButtons.RIGHT
MouseButtons.MIDDLE
```

### Пример
```lua
function Update(dt)
    -- Движение
    if BlazeBolt.IsKeyPressed(Keys.A) then print("Move left") end
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then print("Jump!") end
    
    -- Мышь
    if BlazeBolt.IsMouseButtonPressed(MouseButtons.LEFT) then
        local mx, my = BlazeBolt.GetMouseX(), BlazeBolt.GetMouseY()
        print("Click at: " .. mx .. ", " .. my)
    end
    
    -- Прокрутка
    local scroll = BlazeBolt.GetScrollY()
    if scroll ~= 0 then
        print("Scroll: " .. scroll)
    end
end
```

---

## Звук

### Загрузка
```lua
soundId = BlazeBolt.LoadSound(filename, soundName, loop)
```
| Параметр | Тип | Описание |
|---|---|---|
| filename | string | Путь к звуковому файлу |
| soundName | string | Имя для идентификации |
| loop | boolean | Зацикливание воспроизведения |

**Возвращает:** `soundId` (integer) — идентификатор звука

### Воспроизведение
```lua
BlazeBolt.PlaySound(soundName)
BlazeBolt.PlaySoundById(soundId)
```

### Остановка
```lua
BlazeBolt.StopSound(soundName)
BlazeBolt.StopAllSounds()
```

### Громкость
```lua
BlazeBolt.SetSoundVolume(soundName, volume)   -- 0.0 - 1.0
```

### Проверка
```lua
isPlaying = BlazeBolt.IsSoundPlaying(soundName)
```

### Пример
```lua
function Start()
    BlazeBolt.LoadSound("music/bgm.mp3", "bgm", true)
    BlazeBolt.LoadSound("sfx/click.wav", "click", false)
    BlazeBolt.PlaySound("bgm")
    BlazeBolt.SetSoundVolume("bgm", 0.5)
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.PlaySound("click")
    end
end
```

---

## Управление объектами

### Удаление
```lua
BlazeBolt.Destroy(entity)      -- удалить конкретный объект
BlazeBolt.DestroyAll()         -- удалить все объекты
```

### Пример
```lua
function Start()
    local temp = BlazeBolt.CreateSprite("icon.png", 0, 0)
    BlazeBolt.Destroy(temp)    -- удалить сразу
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.R) then
        BlazeBolt.DestroyAll() -- удалить всё
    end
end
```

---

## Физика

Движок использует Box2D для симуляции физики. Физические тела привязываются к спрайтам для визуального отображения.

### Инициализация
```lua
BlazeBolt.PhysicsInit(gravityX, gravityY)
```
| Параметр | Тип | Описание |
|---|---|---|
| gravityX | number | Гравитация по X |
| gravityY | number | Гравитация по Y |

### Гравитация
```lua
BlazeBolt.PhysicsSetGravity(x, y)
x, y = BlazeBolt.PhysicsGetGravity()
```

### Создание тела
```lua
entity = BlazeBolt.PhysicsCreateBody(bodyType, x, y, mass, friction, restitution)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyType | string | Тип: `"dynamic"`, `"static"`, `"kinematic"` |
| x, y | number | Позиция в NDC |
| mass | number | Масса тела |
| friction | number | Трение (0–1) |
| restitution | number | Упругость (0–1) |

### Формы
```lua
BlazeBolt.PhysicsAddCircle(bodyEntity, radius, offsetX, offsetY)
BlazeBolt.PhysicsAddRectangle(bodyEntity, halfWidth, halfHeight)
```

### Скорость
```lua
BlazeBolt.PhysicsSetLinearVelocity(bodyEntity, vx, vy)
vx, vy = BlazeBolt.PhysicsGetLinearVelocity(bodyEntity)

BlazeBolt.PhysicsSetAngularVelocity(bodyEntity, av)
av = BlazeBolt.PhysicsGetAngularVelocity(bodyEntity)
```

### Силы и импульсы
```lua
BlazeBolt.PhysicsApplyForce(bodyEntity, fx, fy)
BlazeBolt.PhysicsApplyForceAtPoint(bodyEntity, fx, fy, px, py)
BlazeBolt.PhysicsApplyImpulse(bodyEntity, ix, iy)
BlazeBolt.PhysicsApplyImpulseAtPoint(bodyEntity, ix, iy, px, py)
BlazeBolt.PhysicsApplyTorque(bodyEntity, torque)
```

### Позиция и вращение
```lua
BlazeBolt.PhysicsSetPosition(bodyEntity, x, y)
x, y = BlazeBolt.PhysicsGetPosition(bodyEntity)

BlazeBolt.PhysicsSetAngle(bodyEntity, angle)
angle = BlazeBolt.PhysicsGetAngle(bodyEntity)
```

### Свойства тела
```lua
BlazeBolt.PhysicsSetGravityScale(bodyEntity, scale)    -- множитель гравитации
scale = BlazeBolt.PhysicsGetGravityScale(bodyEntity)

BlazeBolt.PhysicsSetActive(bodyEntity, active)         -- вкл/выкл тело
active = BlazeBolt.PhysicsIsActive(bodyEntity)

BlazeBolt.PhysicsSetFixedRotation(bodyEntity, fixed)   -- фиксировать вращение
fixed = BlazeBolt.PhysicsIsFixedRotation(bodyEntity)

BlazeBolt.PhysicsSetBullet(bodyEntity, bullet)         -- режим пули (для быстрых объектов)
bullet = BlazeBolt.PhysicsIsBullet(bodyEntity)

mass = BlazeBolt.PhysicsGetMass(bodyEntity)
```

### Синхронизация со спрайтом
```lua
BlazeBolt.PhysicsSyncSprite(bodyEntity, spriteEntity)
```
Автоматически копирует позицию и угол физического тела в спрайт. Вызывайте в `Update()`.

### Обновление симуляции
```lua
BlazeBolt.PhysicsStep()
```
Выполняет один шаг физической симуляции. Вызывайте в `Update(dt)`.

### Удаление
```lua
BlazeBolt.PhysicsDestroyBody(bodyEntity)
```

### Пример
```lua
function Start()
    BlazeBolt.PhysicsInit(0, -9.8)
    
    player = BlazeBolt.CreateSprite("icon.png", 0, 0)
    BlazeBolt.SpriteSetSize(player, 0.1, 0.1)
    
    body = BlazeBolt.PhysicsCreateBody("dynamic", 0, 0, 1, 0.3, 0.5)
    BlazeBolt.PhysicsAddCircle(body, 0.05, 0, 0)
    
    ground = BlazeBolt.CreateSprite("ground.png", 0, -0.8)
    BlazeBolt.SpriteSetSize(ground, 2, 0.1)
    
    groundBody = BlazeBolt.PhysicsCreateBody("static", 0, -0.8, 0, 0.5, 0.3)
    BlazeBolt.PhysicsAddRectangle(groundBody, 1, 0.05)
end

function Update(dt)
    BlazeBolt.PhysicsStep()
    BlazeBolt.PhysicsSyncSprite(body, player)
    BlazeBolt.PhysicsSyncSprite(groundBody, ground)
    
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        local _, vy = BlazeBolt.PhysicsGetLinearVelocity(body)
        if vy < 0.1 then
            BlazeBolt.PhysicsApplyImpulse(body, 0, 5)
        end
    end
end
```

---

## Утилиты

### Консоль
```lua
BlazeBolt.Print(...)           -- вывод в консоль (поддерживает любые типы)
```

### Время
```lua
dt   = BlazeBolt.GetDeltaTime()  -- время между кадрами в секундах
time = GetTime()                 -- время с запуска движка в секундах
```

### Случайные числа
```lua
val      = BlazeBolt.Random(min, max)        -- float, min по умолчанию 0, max по умолчанию 1
intVal   = BlazeBolt.RandomInt(min, max)     -- integer (min и max обязательны)
BlazeBolt.SetRandomSeed(seed)                -- установить seed генератора
```

### Выход
```lua
BlazeBolt.Quit()               -- закрыть приложение
```

### Пример
```lua
function Start()
    BlazeBolt.SetRandomSeed(42)
    BlazeBolt.Print("Hello from BlazeBolt!")
end

function Update(dt)
    local r = BlazeBolt.Random(0, 100)
    local ri = BlazeBolt.RandomInt(1, 6)  -- кубик
    BlazeBolt.Print("Float: " .. r .. ", Int: " .. ri)
    BlazeBolt.Print("Delta: " .. dt .. ", Time: " .. GetTime())
end
```

---

## Скрипты и сцены

### Загрузка скриптов
```lua
result = BlazeBolt.LoadScript(path)              -- загрузить Lua-скрипт
result = BlazeBolt.ReloadScript(path)            -- перезагрузить скрипт
result = BlazeBolt.ReloadAllScripts()            -- перезагрузить все скрипты
BlazeBolt.EnableScript(name, enabled)            -- включить/выключить скрипт
loaded = BlazeBolt.IsScriptLoaded(name)          -- проверить, загружен ли
list = BlazeBolt.GetLoadedScripts()              -- таблица загруженных скриптов
```

### Сцены
```lua
BlazeBolt.LoadScene(sceneName)       -- переключить на другую сцену
name = BlazeBolt.GetCurrentScene()   -- получить имя текущей сцены
```

### Файл scripts.list
```
# Комментарии начинаются с #
# Обычные скрипты: имя=путь
main=engine/scripts/main.lua
player=engine/scripts/player.lua

# Сцены (с префиксом @):
@menu=engine/scenes/menu.lua
@game=engine/scenes/game.lua
```

### Пример переключения сцен
```lua
function Start()
    print("Current scene: " .. BlazeBolt.GetCurrentScene())
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.ENTER) then
        BlazeBolt.LoadScene("game")
    end
end
```

---

## Пример: полная мини-игра

```lua
-- player.lua
local playerSprite
local playerBody
local score = 0
local scoreText

function Start()
    BlazeBolt.SetMainWindowTitle("Mini Game")
    
    -- Игрок
    playerSprite = BlazeBolt.CreateSprite("player.png", 0, 0)
    BlazeBolt.SpriteSetSize(playerSprite, 0.08, 0.08)
    BlazeBolt.SpriteSetOrigin(playerSprite, 0.5, 0.5)
    
    -- Физика игрока
    playerBody = BlazeBolt.PhysicsCreateBody("dynamic", 0, 0, 1, 0.2, 0.6)
    BlazeBolt.PhysicsAddRectangle(playerBody, 0.04, 0.04)
    
    -- Земля
    local ground = BlazeBolt.CreateSprite("ground.png", 0, -0.85)
    BlazeBolt.SpriteSetSize(ground, 3, 0.05)
    
    local groundBody = BlazeBolt.PhysicsCreateBody("static", 0, -0.85, 0, 0.5, 0.3)
    BlazeBolt.PhysicsAddRectangle(groundBody, 1.5, 0.025)
    
    -- Стены
    for _, x in ipairs({-0.95, 0.95}) do
        local wall = BlazeBolt.CreateSprite("wall.png", x, 0)
        BlazeBolt.SpriteSetSize(wall, 0.05, 2)
        local wb = BlazeBolt.PhysicsCreateBody("static", x, 0, 0, 0.5, 0.3)
        BlazeBolt.PhysicsAddRectangle(wb, 0.025, 1)
    end
    
    -- HUD
    scoreText = BlazeBolt.CreateText("fonts/arial.ttf", "Score: 0", -0.8, 0.85, 20)
    BlazeBolt.TextSetColor(scoreText, 1, 1, 1, 1)
    
    -- Музыка
    BlazeBolt.LoadSound("music/bgm.mp3", "bgm", true)
    BlazeBolt.LoadSound("sfx/jump.wav", "jump", false)
    BlazeBolt.LoadSound("sfx/collect.wav", "collect", false)
    BlazeBolt.PlaySound("bgm")
    
    -- Физика
    BlazeBolt.PhysicsInit(0, -9.8)
    
    -- Сбор монет
    spawnCoins()
end

function spawnCoins()
    coins = {}
    coinBodies = {}
    for i = 1, 5 do
        local cx = BlazeBolt.Random(-0.6, 0.6)
        local cy = BlazeBolt.Random(-0.5, 0.6)
        local coin = BlazeBolt.CreateSprite("coin.png", cx, cy)
        BlazeBolt.SpriteSetSize(coin, 0.06, 0.06)
        BlazeBolt.SpriteSetOrigin(coin, 0.5, 0.5)
        
        local cb = BlazeBolt.PhysicsCreateBody("static", cx, cy, 0, 0, 0)
        BlazeBolt.PhysicsAddCircle(cb, 0.03, 0, 0)
        
        table.insert(coins, coin)
        table.insert(coinBodies, cb)
    end
end

function Update(dt)
    -- Управление
    local vx, vy = BlazeBolt.PhysicsGetLinearVelocity(playerBody)
    
    if BlazeBolt.IsKeyPressed(Keys.A) then
        BlazeBolt.PhysicsApplyForce(playerBody, -15, 0)
    end
    if BlazeBolt.IsKeyPressed(Keys.D) then
        BlazeBolt.PhysicsApplyForce(playerBody, 15, 0)
    end
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) and vy < 0.1 then
        BlazeBolt.PhysicsApplyImpulse(playerBody, 0, 8)
        BlazeBolt.PlaySound("jump")
    end
    
    -- Физика
    BlazeBolt.PhysicsStep()
    BlazeBolt.PhysicsSyncSprite(playerBody, playerSprite)
    
    -- Проверка сбора монет
    local px, py = BlazeBolt.PhysicsGetPosition(playerBody)
    for i = #coins, 1, -1 do
        local cx, cy = BlazeBolt.PhysicsGetPosition(coinBodies[i])
        local dist = math.sqrt((px - cx)^2 + (py - cy)^2)
        if dist < 0.08 then
            BlazeBolt.Destroy(coins[i])
            BlazeBolt.PhysicsDestroyBody(coinBodies[i])
            table.remove(coins, i)
            table.remove(coinBodies, i)
            score = score + 10
            BlazeBolt.TextSetString(scoreText, "Score: " .. score)
            BlazeBolt.PlaySound("collect")
        end
    end
    
    -- Респавн монет
    if #coins == 0 then
        spawnCoins()
    end
    
    -- Выход
    if BlazeBolt.IsKeyJustPressed(Keys.ESCAPE) then
        BlazeBolt.Quit()
    end
end
```

---

## Лучшие практики

### Организация кода
- Разделяйте логику на отдельные скрипты через `scripts.list`
- Используйте `BlazeBolt.EnableScript(name, false)` для отладки
- Храните пути к ресурсам в константах

### Производительность
- Не создавайте объекты в `Update()` — создавайте в `Start()`
- Используйте `Destroy()` для удаления ненужных объектов
- Не загружайте текстуры повторно — `SpriteSetTexture` использует кеш

### Масштабирование
- Проектируйте игру в NDC — она будет работать на любом разрешении
- Для pixel-art используйте `Nearest` фильтр текстур (по умолчанию)
- Тестируйте на разных размерах окна через `SetMainWindowSize`

### Отладка
- Используйте `BlazeBolt.Print()` для вывода значений
- `BlazeBolt.GetDeltaTime()` помогает отслеживать FPS: `1 / dt`
- Перезагружайте скрипты через `ReloadAllScripts` без перезапуска

### Обработка ошибок
- Проверяйте `entity ~= 0` после создания — `0` означает ошибку
- Проверяйте `loaded = IsScriptLoaded(name)` перед использованием
- Звуки могут не загрузиться — проверяйте `soundId >= 0`

---

## Структура проекта

```
bin/versions/engine/
├── scripts.list              # Список загружаемых скриптов
├── engine/
│   ├── scripts/
│   │   ├── main.lua          # Главный скрипт (обязательно)
│   │   ├── player.lua        # Логика игрока
│   │   └── enemy.lua         # Логика врагов
│   ├── scenes/
│   │   ├── menu.lua          # Сцена меню
│   │   └── game.lua          # Сцена игры
│   ├── assets/
│   │   ├── textures/         # PNG-текстуры
│   │   ├── fonts/            # TTF-шрифты
│   │   ├── music/            # Фоновая музыка
│   │   └── sfx/              # Звуковые эффекты
│   └── icon.png              # Иконка окна
```

### scripts.list — формат
```
# Комментарии начинаются с #
# Скрипты: имя=путь
main=engine/scripts/main.lua
player=engine/scripts/player.lua

# Сцены (префикс @):
@menu=engine/scenes/menu.lua
@game=engine/scenes/game.lua
```

---

## Справочник всех функций

### Спрайты
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateSprite` | `path, x, y` | `entity` |
| `BlazeBolt.SpriteSetPosition` | `entity, x, y` | — |
| `BlazeBolt.SpriteGetPosition` | `entity` | `x, y` |
| `BlazeBolt.SpriteSetSize` | `entity, w, h` | — |
| `BlazeBolt.SpriteGetSize` | `entity` | `w, h` |
| `BlazeBolt.SpriteSetOrigin` | `entity, ox, oy` | — |
| `BlazeBolt.SpriteSetRotation` | `entity, degrees` | — |
| `BlazeBolt.SpriteSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.SpriteSetTexture` | `entity, path` | — |
| `BlazeBolt.SpriteSetVisible` | `entity, bool` | — |

### Анимации
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateAnimation` | `path, isGif, x, y` | `entity` |
| `BlazeBolt.CreateAnimationFromSheet` | `path, fw, fh, total, perRow, delay, x, y` | `entity` |
| `BlazeBolt.AnimationPlay` | `entity` | — |
| `BlazeBolt.AnimationPause` | `entity` | — |
| `BlazeBolt.AnimationStop` | `entity` | — |
| `BlazeBolt.AnimationRestart` | `entity` | — |
| `BlazeBolt.AnimationSetLooping` | `entity, bool` | — |
| `BlazeBolt.AnimationSetSpeed` | `entity, speed` | — |
| `BlazeBolt.AnimationSetFrame` | `entity, frame` | — |
| `BlazeBolt.AnimationGetFrameCount` | `entity` | `count` |
| `BlazeBolt.AnimationIsPlaying` | `entity` | `bool` |
| `BlazeBolt.AnimationSetPosition` | `entity, x, y` | — |
| `BlazeBolt.AnimationSetSize` | `entity, w, h` | — |

### Текст
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateText` | `font, text, x, y, size` | `entity` |
| `BlazeBolt.TextSetString` | `entity, text` | — |
| `BlazeBolt.TextGetString` | `entity` | `text` |
| `BlazeBolt.TextSetPosition` | `entity, x, y` | — |
| `BlazeBolt.TextSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.TextSetScale` | `entity, scale` | — |
| `BlazeBolt.TextSetVisible` | `entity, bool` | — |

### Меши
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateMesh` | — | `entity` |
| `BlazeBolt.MeshSetData` | `entity, vertices, indices` | — |
| `BlazeBolt.MeshDraw` | `entity` | — |

### Окно
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.SetMainWindowTitle` | `title` | — |
| `BlazeBolt.GetMainWindowTitle` | — | `title` |
| `BlazeBolt.SetMainWindowSize` | `w, h` | — |
| `BlazeBolt.GetMainWindowWidth` | — | `width` |
| `BlazeBolt.GetMainWindowHeight` | — | `height` |
| `BlazeBolt.SetMainWindowPosition` | `x, y` | — |
| `BlazeBolt.GetMainWindowPosition` | — | `x, y` |
| `BlazeBolt.SetMainWindowIcon` | `path` | — |
| `BlazeBolt.SetMainWindowShouldClose` | `bool` | — |
| `BlazeBolt.IsMainWindowShouldClose` | — | `bool` |
| `BlazeBolt.CreateWindow` | `w, h, title` | `windowPtr` |
| `BlazeBolt.SetWindowTitle` | `ptr, title` | — |
| `BlazeBolt.SetWindowSize` | `ptr, w, h` | — |
| `BlazeBolt.SetWindowIcon` | `ptr, path` | — |
| `GetScreenWidth` | — | `width` |
| `GetScreenHeight` | — | `height` |

### Ввод
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.IsKeyPressed` | `key` | `bool` |
| `BlazeBolt.IsKeyJustPressed` | `key` | `bool` |
| `BlazeBolt.IsKeyJustReleased` | `key` | `bool` |
| `BlazeBolt.GetMouseX` | — | `x` |
| `BlazeBolt.GetMouseY` | — | `y` |
| `BlazeBolt.GetMouseDeltaX` | — | `dx` |
| `BlazeBolt.GetMouseDeltaY` | — | `dy` |
| `BlazeBolt.IsMouseButtonPressed` | `button` | `bool` |
| `BlazeBolt.IsMouseButtonJustPressed` | `button` | `bool` |
| `BlazeBolt.GetScrollY` | — | `scroll` |

### Звук
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.LoadSound` | `file, name, loop` | `id` |
| `BlazeBolt.PlaySound` | `name` | — |
| `BlazeBolt.PlaySoundById` | `id` | — |
| `BlazeBolt.StopSound` | `name` | — |
| `BlazeBolt.StopAllSounds` | — | — |
| `BlazeBolt.SetSoundVolume` | `name, vol` | — |
| `BlazeBolt.IsSoundPlaying` | `name` | `bool` |

### Управление объектами
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.Destroy` | `entity` | — |
| `BlazeBolt.DestroyAll` | — | — |

### Физика
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.PhysicsInit` | `gravityX, gravityY` | — |
| `BlazeBolt.PhysicsSetGravity` | `x, y` | — |
| `BlazeBolt.PhysicsGetGravity` | — | `x, y` |
| `BlazeBolt.PhysicsCreateBody` | `bodyType, x, y, mass, friction, restitution` | `entity` |
| `BlazeBolt.PhysicsAddCircle` | `bodyEntity, radius, offsetX, offsetY` | — |
| `BlazeBolt.PhysicsAddRectangle` | `bodyEntity, halfWidth, halfHeight` | — |
| `BlazeBolt.PhysicsSetLinearVelocity` | `bodyEntity, vx, vy` | — |
| `BlazeBolt.PhysicsGetLinearVelocity` | `bodyEntity` | `vx, vy` |
| `BlazeBolt.PhysicsSetAngularVelocity` | `bodyEntity, av` | — |
| `BlazeBolt.PhysicsGetAngularVelocity` | `bodyEntity` | `av` |
| `BlazeBolt.PhysicsApplyForce` | `bodyEntity, fx, fy` | — |
| `BlazeBolt.PhysicsApplyForceAtPoint` | `bodyEntity, fx, fy, px, py` | — |
| `BlazeBolt.PhysicsApplyImpulse` | `bodyEntity, ix, iy` | — |
| `BlazeBolt.PhysicsApplyImpulseAtPoint` | `bodyEntity, ix, iy, px, py` | — |
| `BlazeBolt.PhysicsApplyTorque` | `bodyEntity, torque` | — |
| `BlazeBolt.PhysicsSetPosition` | `bodyEntity, x, y` | — |
| `BlazeBolt.PhysicsGetPosition` | `bodyEntity` | `x, y` |
| `BlazeBolt.PhysicsSetAngle` | `bodyEntity, angle` | — |
| `BlazeBolt.PhysicsGetAngle` | `bodyEntity` | `angle` |
| `BlazeBolt.PhysicsSetGravityScale` | `bodyEntity, scale` | — |
| `BlazeBolt.PhysicsGetGravityScale` | `bodyEntity` | `scale` |
| `BlazeBolt.PhysicsSetActive` | `bodyEntity, active` | — |
| `BlazeBolt.PhysicsIsActive` | `bodyEntity` | `bool` |
| `BlazeBolt.PhysicsSetFixedRotation` | `bodyEntity, fixed` | — |
| `BlazeBolt.PhysicsIsFixedRotation` | `bodyEntity` | `bool` |
| `BlazeBolt.PhysicsSetBullet` | `bodyEntity, bullet` | — |
| `BlazeBolt.PhysicsIsBullet` | `bodyEntity` | `bool` |
| `BlazeBolt.PhysicsDestroyBody` | `bodyEntity` | — |
| `BlazeBolt.PhysicsGetMass` | `bodyEntity` | `mass` |
| `BlazeBolt.PhysicsStep` | — | — |
| `BlazeBolt.PhysicsSyncSprite` | `bodyEntity, spriteEntity` | — |

### Утилиты
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.Print` | `...` | — |
| `BlazeBolt.GetDeltaTime` | — | `dt` |
| `GetTime` | — | `time` |
| `BlazeBolt.Random` | `min, max` | `float` |
| `BlazeBolt.RandomInt` | `min, max` | `int` |
| `BlazeBolt.SetRandomSeed` | `seed` | — |
| `BlazeBolt.Quit` | — | — |

### Скрипты и сцены
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.LoadScript` | `path` | `bool` |
| `BlazeBolt.ReloadScript` | `path` | `bool` |
| `BlazeBolt.ReloadAllScripts` | — | `bool` |
| `BlazeBolt.EnableScript` | `name, bool` | — |
| `BlazeBolt.IsScriptLoaded` | `name` | `bool` |
| `BlazeBolt.GetLoadedScripts` | — | `table` |
| `BlazeBolt.LoadScene` | `sceneName` | `bool` |
| `BlazeBolt.GetCurrentScene` | — | `name` |

---

BlazeBolt Game Engine 1.0
