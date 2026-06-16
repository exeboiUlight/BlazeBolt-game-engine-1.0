# BlazeBolt Lua API 1.0

Полная документация по Lua API игрового движка BlazeBolt 1.0.

## Оглавление
- [Система координат](#система-координат)
- [Жизненный цикл](#жизненный-цикл)
- [Спрайты](#спрайты)
- [Спрайтовые батчи (SpriteBatch)](#спрайтовые-батчи-spritebatch)
- [Анимации](#анимации)
- [Текст](#текст)
- [Меши](#меши)
- [Камера (Camera2D)](#камера-camera2d)
- [Окно](#окно)
- [Ввод](#ввод)
- [Звук](#звук)
- [Управление объектами](#управление-объектами)
- [Физика](#физика)
- [Утилиты](#утилиты)
- [Шумы (Noise)](#шумы-noise)
- [Шейдеры](#шейдеры)
- [Порядок отрисовки (Render Order)](#порядок-отрисовки-render-order)
- [Математические типы](#математические-типы)
- [Частицы (ParticleSystem2D)](#частицы-particlesystem2d)
- [Тайлсеты (Tileset2D)](#тайлсеты-tileset2d)
- [Освещение (Light2D)](#освещение-light2d)
- [Сетевое взаимодействие (Networking)](#сетевое-взаимодействие-networking)
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

### Область текстуры (Texture Rect)
```lua
BlazeBolt.SpriteSetTextureRect(entity, u, v, w, h)
```
Позволяет отобразить только часть текстуры. Параметры в UV-координатах (0–1):
| Параметр | Описание |
|---|---|
| `u` | Смещение по X в UV-пространстве |
| `v` | Смещение по Y в UV-пространстве |
| `w` | Ширина области в UV-пространстве |
| `h` | Высота области в UV-пространстве |

По умолчанию `(0, 0, 1, 1)` — вся текстура. Например, `(0, 0, 0.5, 1)` покажет левую половину текстуры.

Используется для работы с текстурными атласами (спрайт-листами) без создания отдельных файлов.

### Видимость
```lua
BlazeBolt.SpriteSetVisible(entity, visible)
visible = BlazeBolt.SpriteIsVisible(entity)
```
`visible` — `true` или `false`.

### Пример с текстурным атласом
```lua
function Start()
    -- Один файл текстурного атласа
    local player = BlazeBolt.CreateSprite("player_sheet.png", 0, 0)
    BlazeBolt.SpriteSetSize(player, 0.2, 0.2)
    BlazeBolt.SpriteSetOrigin(player, 0.5, 0.5)
    
    -- Показать только первый кадр (32x32 в атласе 128x64)
    local frameW, frameH = 32, 32
    local atlasW, atlasH = 128, 64
    local u = 0 / atlasW              -- 0
    local v = 0 / atlasH              -- 0
    local w = frameW / atlasW         -- 0.25
    local h = frameH / atlasH         -- 0.5
    BlazeBolt.SpriteSetTextureRect(player, u, v, w, h)
end
```

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

## Спрайтовые батчи (SpriteBatch)

Спрайтовый батч группирует несколько спрайтов в один draw call (`glDrawElements`).
Все спрайты в батче рисуются одной текстурой за один вызов отрисовки, что значительно повышает производительность при большом количестве спрайтов.

### Создание

```lua
batch = BlazeBolt.CreateSpriteBatch(maxSize)
```
| Параметр | Тип | Описание |
|---|---|---|
| maxSize | integer | Максимальное количество спрайтов в батче (по умолчанию 25) |

**Возвращает:** `batchEntity` (integer) — идентификатор батча

### Установка текстуры

```lua
BlazeBolt.SpriteBatchSetTexture(batchEntity, texturePath)
```
Все спрайты в батче рисуются с одной текстурой.

### Добавление и удаление спрайтов

```lua
ok = BlazeBolt.SpriteBatchAdd(batchEntity, spriteEntity)
ok = BlazeBolt.SpriteBatchRemove(batchEntity, spriteEntity)
BlazeBolt.SpriteBatchClear(batchEntity)
```

- `SpriteBatchAdd` — добавляет спрайт в батч. Возвращает `false`, если батч заполнен или спрайт уже в батче.
- `SpriteBatchRemove` — удаляет спрайт из батча. Возвращает `false`, если спрайт не найден.
- `SpriteBatchClear` — удаляет все спрайты из батча (сами спрайты не уничтожаются).

### Настройка размера батча

```lua
BlazeBolt.SpriteBatchSetMaxSize(batchEntity, maxSize)
maxSize = BlazeBolt.SpriteBatchGetMaxSize(batchEntity)
```
`SpriteBatchSetMaxSize` сбрасывает текущее содержимое батча.

### Текущее количество спрайтов

```lua
count = BlazeBolt.SpriteBatchGetCount(batchEntity)
```

### Отрисовка

```lua
BlazeBolt.SpriteBatchDraw(batchEntity)
```
Перестраивает вершинные данные из спрайтов и выполняет отрисовку. Вызывайте в `Draw()`.

### Уничтожение

```lua
BlazeBolt.DestroySpriteBatch(batchEntity)
```
Уничтожает батч. Спрайты, добавленные в батч, не уничтожаются.

### Важно
- Один спрайт можно добавить только в один батч. Добавление спрайта во второй батч проигнорируется.
- Батч использует текстуру, установленную через `SpriteBatchSetTexture`. Если текстура не установлена, используется текстура первого добавленного спрайта.
- Размер, поворот, цвет, видимость и texture rect каждого спрайта применяются при отрисовке батча.

### Полный пример

```lua
function Start()
    batch = BlazeBolt.CreateSpriteBatch(50)

    for i = 1, 10 do
        local sprite = BlazeBolt.CreateSprite("icon.png", (i - 5.5) * 0.15, 0)
        BlazeBolt.SpriteSetSize(sprite, 0.1, 0.1)
        BlazeBolt.SpriteBatchAdd(batch, sprite)
    end

    BlazeBolt.SpriteBatchSetTexture(batch, "icon.png")
end

function Draw()
    BlazeBolt.SpriteBatchDraw(batch)
end
```

---

## Анимации

### Обновление API

Старый набор функций `Animation*` заменён на `AnimatedSprite*`.

Быстрая замена имён:

- `CreateAnimation(...)` -> `CreateAnimatedSprite(...)`
- `AnimationPlay/Pause/Stop/Restart` -> `AnimatedSpritePlay/Pause/Stop/Restart`
- `AnimationSetLooping` -> `AnimatedSpriteSetLooping`
- `AnimationSetSpeed` -> `AnimatedSpriteSetPlaybackSpeed`
- `AnimationSetFrame` -> `AnimatedSpriteSetFrame`
- `AnimationGetFrameCount` -> `AnimatedSpriteGetNumFrames`
- `AnimationIsPlaying` -> `AnimatedSpriteIsPlaying`
- `AnimationSetPosition` -> `AnimatedSpriteSetPosition`
- `AnimationSetSize` -> `AnimatedSpriteSetSize`

### Создание
```lua
entity = BlazeBolt.CreateAnimatedSprite(texturePath, x, y)
```

> **Примечание:** GIF-анимация корректно обрабатывает кадры произвольного размера и смещения (partial frames). Каждый кадр композируется на полный холст `(xdim × ydim)` с учётом disposal mode (GIF_BKGD, GIF_PREV). Это исправляет некорректное отображение кадров после 3-го кадра в не-квадратных GIF.

### Воспроизведение
```lua
BlazeBolt.AnimatedSpritePlay(entity)
BlazeBolt.AnimatedSpritePause(entity)
BlazeBolt.AnimatedSpriteStop(entity)            -- остановить, сброс на первый кадр
BlazeBolt.AnimatedSpriteRestart(entity)         -- перезапустить с начала
```

### Настройки
```lua
BlazeBolt.AnimatedSpriteSetLooping(entity, looping)          -- зацикливание (boolean)
BlazeBolt.AnimatedSpriteSetPlaybackSpeed(entity, speed)      -- множитель скорости (1.0 = норма)
BlazeBolt.AnimatedSpriteSetFrame(entity, frameIndex)         -- перейти к конкретному кадру
```

### Информация
```lua
frameCount   = BlazeBolt.AnimatedSpriteGetNumFrames(entity)
currentFrame = BlazeBolt.AnimatedSpriteGetCurrentFrame(entity)
isPlaying    = BlazeBolt.AnimatedSpriteIsPlaying(entity)
isLooping    = BlazeBolt.AnimatedSpriteIsLooping(entity)
speed        = BlazeBolt.AnimatedSpriteGetPlaybackSpeed(entity)
```

### Позиция и размер
```lua
BlazeBolt.AnimatedSpriteSetPosition(entity, x, y)
x, y = BlazeBolt.AnimatedSpriteGetPosition(entity)
BlazeBolt.AnimatedSpriteSetSize(entity, width, height)
width, height = BlazeBolt.AnimatedSpriteGetSize(entity)
BlazeBolt.AnimatedSpriteSetOrigin(entity, originX, originY)
originX, originY = BlazeBolt.AnimatedSpriteGetOrigin(entity)
BlazeBolt.AnimatedSpriteSetRotation(entity, degrees)
degrees = BlazeBolt.AnimatedSpriteGetRotation(entity)
BlazeBolt.AnimatedSpriteSetColor(entity, r, g, b, a)
r, g, b, a = BlazeBolt.AnimatedSpriteGetColor(entity)
```

### Пример
```lua
function Start()
    local anim = BlazeBolt.CreateAnimatedSprite("player.gif", 0, 0)
    BlazeBolt.AnimatedSpriteSetSize(anim, 0.4, 0.4)
    BlazeBolt.AnimatedSpriteSetLooping(anim, true)
    BlazeBolt.AnimatedSpriteSetPlaybackSpeed(anim, 2.0)
    BlazeBolt.AnimatedSpritePlay(anim)
end

function Update(dt)
    if not BlazeBolt.AnimatedSpriteIsPlaying(anim) then
        BlazeBolt.AnimatedSpriteRestart(anim)
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
scrollY = BlazeBolt.GetScrollY()      -- вертикальная прокрутка за кадр
```

### Константы клавиш
```lua
-- Буквы
Keys.A, Keys.B, Keys.C, ..., Keys.Z

-- Цифры
Keys[0], Keys[1], ..., Keys[9]

-- Стрелки
Keys.UP, Keys.DOWN, Keys.LEFT, Keys.RIGHT

-- Специальные
Keys.SPACE, Keys.ENTER, Keys.ESCAPE
Keys.TAB, Keys.BACKSPACE, Keys.DELETE
Keys.INSERT, Keys.HOME, Keys.END, Keys.PAGE_UP, Keys.PAGE_DOWN

-- Модификаторы
Keys.LEFT_SHIFT, Keys.RIGHT_SHIFT
Keys.LEFT_CONTROL, Keys.RIGHT_CONTROL
Keys.LEFT_ALT, Keys.RIGHT_ALT
Keys.LEFT_SUPER, Keys.RIGHT_SUPER

-- Функциональные
Keys.F1, Keys.F2, ..., Keys.F25

-- Цифровая клавиатура
Keys.KP_0 .. Keys.KP_9, Keys.KP_DECIMAL, Keys.KP_DIVIDE
Keys.KP_MULTIPLY, Keys.KP_SUBTRACT, Keys.KP_ADD, Keys.KP_ENTER, Keys.KP_EQUAL

-- Блокировка
Keys.CAPS_LOCK, Keys.SCROLL_LOCK, Keys.NUM_LOCK, Keys.PRINT_SCREEN, Keys.PAUSE

-- Прочее
Keys.MENU
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
    -- Клавиатура
    if BlazeBolt.IsKeyPressed(Keys.A) then print("Move left") end
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then print("Jump!") end
    
    -- Мышь
    if BlazeBolt.IsMouseButtonPressed(MouseButtons.LEFT) then
        local mx, my = BlazeBolt.GetMouseX(), BlazeBolt.GetMouseY()
        print("Click at: " .. mx .. ", " .. my)
    end
    
    -- Прокрутка
    local scrollY = BlazeBolt.GetScrollY()
    if scrollY ~= 0 then
        print("Scroll: " .. scrollY)
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
| filename | string | Путь к звуковому файлу (WAV) |
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
    BlazeBolt.LoadSound("music/bgm.wav", "bgm", true)
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

Движок использует собственную физическую систему с дискретным детектированием коллизий. Поддерживаются коллайдеры: окружность, прямоугольник. Физические тела привязываются к спрайтам для визуального отображения.

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
| bodyType | int | Тип: `0` = static, `1` = dynamic, `2` = kinematic |
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

BlazeBolt.PhysicsSetFriction(bodyEntity, friction)     -- трение (0–1)
friction = BlazeBolt.PhysicsGetFriction(bodyEntity)

BlazeBolt.PhysicsSetRestitution(bodyEntity, restitution)  -- упругость (0–1)
restitution = BlazeBolt.PhysicsGetRestitution(bodyEntity)
```

### Синхронизация со спрайтом
```lua
BlazeBolt.PhysicsSyncSprite(bodyEntity, spriteEntity)
```
Автоматически копирует позицию и угол физического тела в спрайт. Вызывайте в `Update()`.

### Синхронизация с анимацией
```lua
BlazeBolt.PhysicsSyncAnimatedSprite(bodyEntity, animationEntity)
```
Копирует позицию и угол физического тела в анимированный спрайт (`AnimatedSprite2D`). Вызывайте в `Update()`.

### Синхронизация с текстом
```lua
BlazeBolt.PhysicsSyncText(bodyEntity, textEntity)
```
Копирует позицию и угол физического тела в текст (Text2D). Вызывайте в `Update()`.

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
    
    body = BlazeBolt.PhysicsCreateBody(1, 0, 0, 1, 0.3, 0.5)
    BlazeBolt.PhysicsAddCircle(body, 0.05, 0, 0)
    
    ground = BlazeBolt.CreateSprite("ground.png", 0, -0.8)
    BlazeBolt.SpriteSetSize(ground, 2, 0.1)
    
    groundBody = BlazeBolt.PhysicsCreateBody(0, 0, -0.8, 0, 0.5, 0.3)
    BlazeBolt.PhysicsAddRectangle(groundBody, 1, 0.05)
end

function Update(dt)
    BlazeBolt.PhysicsStep()
    BlazeBolt.PhysicsSyncSprite(playerBody, playerSprite)
    BlazeBolt.PhysicsSyncAnimatedSprite(playerBody, playerAnim)  -- для AnimatedSprite2D
    
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

Набор вспомогательных функций: вывод в консоль, работа со временем, генерация случайных чисел и выход из приложения.

### Консоль
```lua
BlazeBolt.Print(...)           -- вывод в консоль (поддерживает любые типы)
BlazeBolt.AddConsoleMessage(msg, type)  -- добавить сообщение в консоль (type: 0=info, 1=warning, 2=error)
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

## Шейдеры

### Создание шейдера
```lua
shaderId = BlazeBolt.CreateShader(name, vertexPath, fragmentPath)
```
| Параметр | Тип | Описание |
|---|---|---|
| name | string | Имя шейдера для идентификации |
| vertexPath | string | Путь к вершинному шейдеру (.vert) |
| fragmentPath | string | Путь к фрагментному шейдеру (.frag) |

**Возвращает:** `shaderId` (integer) — идентификатор шейдера

### Удаление шейдера
```lua
BlazeBolt.DestroyShader(shaderId)
```

### Привязка шейдера к объекту
```lua
BlazeBolt.SetEntityShader(entity, shaderId)
shaderId = BlazeBolt.GetEntityShader(entity)
```
`shaderId = 0` — использовать стандартный шейдер.

### Использование шейдера
```lua
BlazeBolt.UseShader(shaderId)
```
Активирует шейдер для последующей отрисовки.

### Установка uniform-переменных
```lua
BlazeBolt.SetShaderFloat(shaderId, name, value)
BlazeBolt.SetShaderInt(shaderId, name, value)
BlazeBolt.SetShaderVec2(shaderId, name, x, y)
BlazeBolt.SetShaderVec3(shaderId, name, x, y, z)
BlazeBolt.SetShaderVec4(shaderId, name, x, y, z, w)
```
Позволяет передать параметры в шейдер.

### Пример
```lua
function Start()
    -- Создание шейдера
    local shader = BlazeBolt.CreateShader("invert", "shaders/invert.vert", "shaders/invert.frag")
    
    -- Создание спрайта
    local sprite = BlazeBolt.CreateSprite("icon.png", 0, 0)
    BlazeBolt.SpriteSetSize(sprite, 0.5, 0.5)
    
    -- Привязка шейдера к спрайту
    BlazeBolt.SetEntityShader(sprite, shader)
end

function Update(dt)
    -- Можно менять uniform-переменные каждый кадр
    BlazeBolt.UseShader(shader)
    BlazeBolt.SetShaderFloat(shader, "time", GetTime())
end
```

---

## Порядок отрисовки (Render Order)

Позволяет управлять порядком отрисовки不同类型 объектов. По умолчанию порядок:
`{"Tilesets", "Sprites", "AnimatedSprites", "Texts", "Meshes", "Particles"}`

### Установка
```lua
BlazeBolt.SetRenderOrder(orderTable)
```
`orderTable` — таблица (массив) строк с именами типов объектов. Объекты рисуются в указанном порядке: первый в списке — самый задний (рисуется первым).

### Получение
```lua
orderTable = BlazeBolt.GetRenderOrder()
```
Возвращает текущий порядок отрисовки как таблицу строк.

### Пример
```lua
function Start()
    BlazeBolt.SetRenderOrder({"Particles", "Tilesets", "Sprites", "AnimatedSprites", "Texts", "Meshes"})
end

function Draw()
    local order = BlazeBolt.GetRenderOrder()
    for i, name in ipairs(order) do
        BlazeBolt.Print(i .. ": " .. name)
    end
end
```

---

## Камера (Camera2D)

Система камер позволяет изменять точку обзора: перемещать, масштабировать и вращать сцену.

Камера — объект-сущность (`entity`). Если камера не создана, рендеринг работает как обычно (identity-матрица).

### Создание
```lua
camera = BlazeBolt.CreateCamera()
```
**Возвращает:** `entity` (integer) — идентификатор камеры

### Позиция
```lua
BlazeBolt.CameraSetPosition(camera, x, y)
x, y = BlazeBolt.CameraGetPosition(camera)
```

### Масштаб (Zoom)
```lua
BlazeBolt.CameraSetZoom(camera, zoom)
zoom = BlazeBolt.CameraGetZoom(camera)
```
`zoom = 1.0` — обычный масштаб. `zoom = 2.0` — приближение (мир крупнее).

### Вращение
```lua
BlazeBolt.CameraSetRotation(camera, degrees)
degrees = BlazeBolt.CameraGetRotation(camera)
```

### Пример: камера, следящая за игроком
```lua
function Start()
    player = BlazeBolt.CreateSprite("player.png", 0, 0)
    BlazeBolt.SpriteSetSize(player, 0.1, 0.1)

    cam = BlazeBolt.CreateCamera()
    BlazeBolt.CameraSetZoom(cam, 1.5)
end

function Update(dt)
    local px, py = BlazeBolt.SpriteGetPosition(player)
    BlazeBolt.CameraSetPosition(cam, px, py)
end
```

---

## Частицы (ParticleSystem2D)

GPU-инстанциированная система частиц для создания визуальных эффектов: взрывы, искры, дым, огонь и др.

Каждая система частиц — объект-сущность (`entity`). Она автоматически обновляется и отрисовывается каждый кадр с использованием instanced-рендеринга OpenGL. Поддерживаются параметры эмиссии, времени жизни, скорости, размера (начальный и конечный), цвета (начальный и конечный), направления разлёта и скорости вращения. Эмиссия может быть непрерывной (с заданной частотой) или ручной (burst через `Emit`).

### Создание
```lua
ps = BlazeBolt.CreateParticleSystem()
```
**Возвращает:** `entity` (integer) — идентификатор системы частиц

### Позиция
```lua
BlazeBolt.ParticleSystemSetPosition(ps, x, y)
```

### Текстура
```lua
BlazeBolt.ParticleSystemSetTexture(ps, texturePath)
```
Если текстура не задана, используется стандартная текстура (белый квадрат).

### Параметры эмиссии
```lua
BlazeBolt.ParticleSystemSetEmissionRate(ps, rate)
rate = BlazeBolt.ParticleSystemGetEmissionRate(ps)
```
`rate` — количество частиц, испускаемых в секунду. `0` — эмиссия отключена.

### Время жизни частиц
```lua
BlazeBolt.ParticleSystemSetLifetime(ps, minSeconds, maxSeconds)
```
Каждая частица получает случайное время жизни в диапазоне от `minSeconds` до `maxSeconds`.

### Скорость частиц
```lua
BlazeBolt.ParticleSystemSetSpeed(ps, minSpeed, maxSpeed)
```
Каждая частица получает случайную скорость в указанном диапазоне.

### Размер частиц
```lua
BlazeBolt.ParticleSystemSetSize(ps, minSize, maxSize)
BlazeBolt.ParticleSystemSetEndSize(ps, minEndSize, maxEndSize)
```
Начальный размер и конечный размер (частица плавно меняет размер за время жизни).

### Цвет частиц
```lua
BlazeBolt.ParticleSystemSetColor(ps, r1, g1, b1, a1, r2, g2, b2, a2)
```
`r1, g1, b1, a1` — начальный цвет (0–1).
`r2, g2, b2, a2` — конечный цвет (0–1, по умолчанию прозрачный).

### Направление разлёта
```lua
BlazeBolt.ParticleSystemSetDirection(ps, minAngle, maxAngle)
```
Углы в градусах. `0` — вправо, `90` — вверх, `180` — влево, `270` — вниз.
По умолчанию `0–360` (во все стороны).

### Скорость вращения
```lua
BlazeBolt.ParticleSystemSetRotationSpeed(ps, degreesPerSecond)
```

### Активность
```lua
BlazeBolt.ParticleSystemSetActive(ps, active)
active = BlazeBolt.ParticleSystemIsActive(ps)
```
Если `false` — эмиссия и обновление частиц отключены.

### Видимость
```lua
BlazeBolt.ParticleSystemSetVisible(ps, visible)
visible = BlazeBolt.ParticleSystemIsVisible(ps)
```

### Ручной выброс (burst)
```lua
BlazeBolt.ParticleSystemEmit(ps, count)
```
Мгновенно испустить `count` частиц (помимо автоматической эмиссии).

### Очистка
```lua
BlazeBolt.ParticleSystemClear(ps)
```
Удаляет все частицы в системе.

### Количество частиц
```lua
count = BlazeBolt.ParticleSystemGetCount(ps)
```
Текущее количество активных частиц в системе.

### Пример: взрыв при клике
```lua
local ps

function Start()
    ps = BlazeBolt.CreateParticleSystem()
    BlazeBolt.ParticleSystemSetTexture(ps, "particle.png")
    BlazeBolt.ParticleSystemSetLifetime(ps, 0.3, 1.5)
    BlazeBolt.ParticleSystemSetSpeed(ps, 1.0, 3.0)
    BlazeBolt.ParticleSystemSetSize(ps, 0.02, 0.04)
    BlazeBolt.ParticleSystemSetEndSize(ps, 0.01, 0.01)
    BlazeBolt.ParticleSystemSetColor(ps, 1, 0.5, 0, 1, 1, 0, 0, 0)
    BlazeBolt.ParticleSystemSetDirection(ps, 0, 360)
    BlazeBolt.ParticleSystemSetEmissionRate(ps, 0)  -- только ручной burst
end

function OnClick()
    local mx = BlazeBolt.GetMouseX()
    local my = BlazeBolt.GetMouseY()
    local ndcX = mx / GetScreenWidth() * 2 - 1
    local ndcY = 1 - my / GetScreenHeight() * 2
    BlazeBolt.ParticleSystemSetPosition(ps, ndcX, ndcY)
    BlazeBolt.ParticleSystemEmit(ps, 50)
end
```

### Пример: постоянный дым
```lua
local ps

function Start()
    ps = BlazeBolt.CreateParticleSystem()
    BlazeBolt.ParticleSystemSetTexture(ps, "particle.png")
    BlazeBolt.ParticleSystemSetPosition(ps, 0, 0.8)
    BlazeBolt.ParticleSystemSetEmissionRate(ps, 30)
    BlazeBolt.ParticleSystemSetLifetime(ps, 1.0, 2.0)
    BlazeBolt.ParticleSystemSetSpeed(ps, 0.2, 0.5)
    BlazeBolt.ParticleSystemSetSize(ps, 0.03, 0.05)
    BlazeBolt.ParticleSystemSetEndSize(ps, 0.06, 0.08)
    BlazeBolt.ParticleSystemSetColor(ps, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0)
    BlazeBolt.ParticleSystemSetDirection(ps, 80, 100)
end
```

---

## Тайловые карты (Tileset2D)

Тайловые карты (tileset) позволяют строить 2D-уровни из набора тайлов — прямоугольных фрагментов текстуры (атласа). Каждый тайл имеет индекс, который определяет его позицию в атласе. Tileset2D эффективно рендерит большие карты, используя batching.

### Создание и настройка

```lua
tileset = BlazeBolt.CreateTileset(atlasPath, tileW, tileH, tileCountX, tileCountY)
-- atlasPath: путь к текстуре-атласу (PNG)
-- tileW: ширина тайла в пикселях (целое число)
-- tileH: высота тайла в пикселях (целое число)
-- tileCountX: количество тайлов по X в атласе
-- tileCountY: количество тайлов по Y в атласе

BlazeBolt.TilesetSetTileSize(tileset, tileW, tileH)
BlazeBolt.TilesetSetPosition(tileset, x, y)
```

### Карта тайлов

Карта — это таблица (массив массивов) с индексами тайлов. Индекс `-1` означает пустой тайл (не рисуется). Индекс `0+` — индекс тайла в атласе (слева направо, сверху вниз).

```lua
BlazeBolt.TilesetSetMap(tileset, mapTable)
tile = BlazeBolt.TilesetGetTile(tileset, col, row)
BlazeBolt.TilesetSetTile(tileset, col, row, tileIndex)
```

### Отрисовка и уничтожение

```lua
BlazeBolt.TilesetDraw(tileset)
count = BlazeBolt.TilesetGetTileCount(tileset)
BlazeBolt.DestroyTileset(tileset)
```

### Пример: простая карта

```lua
local tileset

function Start()
    -- Создаём тайлсет: атлас 4x4 тайла по 32px
    tileset = BlazeBolt.CreateTileset("atlas.png", 32, 32, 4, 4)
    BlazeBolt.TilesetSetPosition(tileset, 0, 0)
    
    -- Определяем карту 8x6 (индексы тайлов в атласе)
    local map = {
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 1, 1, 1, 1, 1, 1, 0},
        { 0, 1, 2, 2, 2, 2, 1, 0},
        { 0, 1, 2, 3, 3, 2, 1, 0},
        { 0, 1, 2, 2, 2, 2, 1, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
    }
    BlazeBolt.TilesetSetMap(tileset, map)
end

function Draw()
    BlazeBolt.TilesetDraw(tileset)
end
```

### Пример: редактор тайлов

```lua
local tileset
local currentTile = 1

function Start()
    tileset = BlazeBolt.CreateTileset("atlas.png", 16, 16, 8, 8)
    BlazeBolt.TilesetSetPosition(tileset, 0, 0)
    
    -- Пустая карта 16x12
    local map = {}
    for y = 1, 12 do
        map[y] = {}
        for x = 1, 16 do
            map[y][x] = -1
        end
    end
    BlazeBolt.TilesetSetMap(tileset, map)
end

function Update(dt)
    if BlazeBolt.IsMouseButtonJustPressed(MouseButton.LEFT) then
        local mx, my = BlazeBolt.GetMouseX(), BlazeBolt.GetMouseY()
        local tileW, tileH = BlazeBolt.TilesetGetTileSize(tileset)
        local col = math.floor(mx / tileW)
        local row = math.floor(my / tileH)
        BlazeBolt.TilesetSetTile(tileset, col, row, currentTile)
    end
end

function Draw()
    BlazeBolt.TilesetDraw(tileset)
end
```

---

## Освещение (Light2D)

Система освещения поддерживает точечные (point) и фоновые (ambient) источники света. Максимум **8 точечных источников** одновременно. Освещение автоматически применяется ко всем шейдерам спрайтов и батчей.

### Создание источников света

```lua
-- Точечный источник (все параметры опциональны)
pointLight = BlazeBolt.CreatePointLight(x, y, r, g, b, intensity, radius)
-- x, y: позиция (по умолчанию 0, 0)
-- r, g, b: цвет (по умолчанию 1, 1, 1 — белый)
-- intensity: интенсивность (по умолчанию 1.0)
-- radius: радиус действия (по умолчанию 1.0)

-- Фоновый свет (все параметры опциональны)
ambientLight = BlazeBolt.CreateAmbientLight(r, g, b, intensity)
-- r, g, b: цвет (по умолчанию 1, 1, 1)
-- intensity: интенсивность (по умолчанию 0.3 — тусклый фон)
```

### Управление параметрами

```lua
BlazeBolt.LightSetPosition(light, x, y)
x, y = BlazeBolt.LightGetPosition(light)

BlazeBolt.LightSetColor(light, r, g, b)
r, g, b = BlazeBolt.LightGetColor(light)

BlazeBolt.LightSetIntensity(light, intensity)
intensity = BlazeBolt.LightGetIntensity(light)

BlazeBolt.LightSetRadius(light, radius)    -- только для точечных
radius = BlazeBolt.LightGetRadius(light)

BlazeBolt.LightSetEnabled(light, bool)
enabled = BlazeBolt.LightGetEnabled(light)
```

### Уничтожение

```lua
BlazeBolt.DestroyLight(light)
```

### Пример: 기본ное освещение

```lua
local playerSprite
local playerLight
local ambient

function Start()
    BlazeBolt.SetMainWindowTitle("Lighting Demo")
    
    -- Игрок
    playerSprite = BlazeBolt.CreateSprite("player.png", 0, 0)
    BlazeBolt.SpriteSetSize(playerSprite, 0.08, 0.08)
    
    -- Фоновый свет (тусклый)
    ambient = BlazeBolt.CreateAmbientLight(0.2, 0.2, 0.3, 0.3)
    
    -- Точечный свет跟着玩家
    playerLight = BlazeBolt.CreatePointLight(0, 0, 1, 0.9, 0.7, 1.5, 0.5)
end

function Update(dt)
    -- Движение игрока
    local x, y = BlazeBolt.SpriteGetPosition(playerSprite)
    if BlazeBolt.IsKeyPressed(Keys.LEFT) then x = x - 0.5 * dt end
    if BlazeBolt.IsKeyPressed(Keys.RIGHT) then x = x + 0.5 * dt end
    if BlazeBolt.IsKeyPressed(Keys.UP) then y = y + 0.5 * dt end
    if BlazeBolt.IsKeyPressed(Keys.DOWN) then y = y - 0.5 * dt end
    BlazeBolt.SpriteSetPosition(playerSprite, x, y)
    
    -- Свет следует за игроком
    BlazeBolt.LightSetPosition(playerLight, x, y)
end

function Draw()
    BlazeBolt.SpriteDraw(playerSprite)
end
```

### Пример: несколько источников

```lua
local lights = {}

function Start()
    BlazeBolt.CreateAmbientLight(0.1, 0.1, 0.1, 0.2)
    
    -- Создаём 3 источника
    table.insert(lights, BlazeBolt.CreatePointLight(-0.5, 0.3, 1, 0, 0, 1.0, 0.4))
    table.insert(lights, BlazeBolt.CreatePointLight(0, -0.3, 0, 1, 0, 1.0, 0.4))
    table.insert(lights, BlazeBolt.CreatePointLight(0.5, 0.3, 0, 0, 1, 1.0, 0.4))
end

function Update(dt)
    local t = GetTime()
    for i, light in ipairs(lights) do
        local offset = (i - 1) * 2.094  -- 120 градусов в радианах
        local x = math.cos(t + offset) * 0.3
        local y = math.sin(t + offset) * 0.3
        BlazeBolt.LightSetPosition(light, x, y)
    end
end
```

---

## Сетевое взаимодействие (Networking)

Модуль сети позволяет создавать онлайн-игры с поддержкой протоколов TCP и UDP. TCP обеспечивает надёжную доставку данных (порядок, гарантированная доставка), UDP — быструю передачу без гарантий (подходит для позиций игроков в реальном времени).

### Инициализация

Перед использованием сети необходимо вызвать `NetInit()`:

```lua
BlazeBolt.NetInit()
```

Инициализирует подсистему сети (Winsock на Windows, POSIX-сокеты на Linux). Вызывается один раз в `Start()`.

### Завершение сети

```lua
BlazeBolt.NetShutdown()
```
Освобождает все ресурсы сети. Вызывается при завершении работы или перед повторной инициализацией.

---

### TCP (Transmission Control Protocol)

TCP — надёжный протокол с установлением соединения. Гарантирует порядок и доставку данных. Используйте для: лобби, чата, передачи игровых событий, действий, где важна надёжность.

#### TCP Server

**Создание сервера:**
```lua
serverId = BlazeBolt.CreateTCPServer(port)
```
| Параметр | Тип | Описание |
|---|---|---|
| port | integer | Порт для прослушивания (1–65535) |

**Возвращает:** `serverId` (integer) — идентификатор сервера, или `nil` при ошибке.

**Пример:**
```lua
server = BlazeBolt.CreateTCPServer(7777)
```

**Остановка сервера:**
```lua
BlazeBolt.TCPServerStop(serverId)
```
Закрывает все соединения и освобождает ресурсы.

**Проверка работы:**
```lua
running = BlazeBolt.TCPServerIsRunning(serverId)  -- boolean
```

**Обработка событий:**
```lua
BlazeBolt.TCPServerPoll(serverId)
```
Обрабатывает входящие соединения и данные. Вызывайте в `Update()` для обновления состояния сервера.

**Приём нового клиента:**
```lua
clientId, clientIP, clientPort = BlazeBolt.TCPServerAccept(serverId)
```
**Возвращает:**
- `clientId` (integer) — ID нового клиента (или `nil`, если новых нет)
- `clientIP` (string) — IP-адрес клиента
- `clientPort` (integer) — порт клиента

**Пример:**
```lua
function Update(dt)
    local id, ip, port = BlazeBolt.TCPServerAccept(server)
    if id then
        BlazeBolt.Print("Новый клиент: " .. ip .. ":" .. port .. " (id=" .. id .. ")")
        BlazeBolt.TCPServerSend(server, id, "Добро пожаловать!")
    end
end
```

**Отправка данных клиенту:**
```lua
success = BlazeBolt.TCPServerSend(serverId, clientId, data)  -- boolean
```
| Параметр | Тип | Описание |
|---|---|---|
| clientId | integer | ID клиента |
| data | string | Данные для отправки |

**Рассылка всем клиентам:**
```lua
success = BlazeBolt.TCPServerBroadcast(serverId, data)  -- boolean
```
Отправляет данные всем подключённым клиентам.

**Получение данных от клиента:**
```lua
data = BlazeBolt.TCPServerReceive(serverId, clientId)  -- string или nil
```
Возвращает строку с данными или `nil`, если данных нет.

**Пример чат-сервера:**
```lua
function Update(dt)
    -- Принять новых клиентов
    local id, ip, port = BlazeBolt.TCPServerAccept(server)
    if id then
        BlazeBolt.TCPServerBroadcast(server, "Игрок " .. id .. " подключился!")
    end

    -- Принять сообщения от всех клиентов
    for i = 1, BlazeBolt.TCPServerGetClientCount(server) do
        local data = BlazeBolt.TCPServerReceive(server, i)
        if data then
            BlazeBolt.TCPServerBroadcast(server, data)
        end
    end
end
```

**Отключение клиента:**
```lua
BlazeBolt.TCPServerDisconnect(serverId, clientId)
```

**Количество клиентов:**
```lua
count = BlazeBolt.TCPServerGetClientCount(serverId)  -- integer
```

**Проверка подключения клиента:**
```lua
connected = BlazeBolt.TCPServerIsClientConnected(serverId, clientId)  -- boolean
```

---

#### TCP Client

**Создание клиента:**
```lua
clientId = BlazeBolt.CreateTCPClient()  -- integer
```

**Подключение к серверу:**
```lua
success = BlazeBolt.TCPClientConnect(clientId, host, port)  -- boolean
```
| Параметр | Тип | Описание |
|---|---|---|
| host | string | IP-адрес или домен сервера |
| port | integer | Порт сервера |

**Пример:**
```lua
client = BlazeBolt.CreateTCPClient()
connected = BlazeBolt.TCPClientConnect(client, "127.0.0.1", 7777)
if connected then
    BlazeBolt.Print("Подключено к серверу!")
end
```

**Отправка данных:**
```lua
success = BlazeBolt.TCPClientSend(clientId, data)  -- boolean
```

**Получение данных:**
```lua
data = BlazeBolt.TCPClientReceive(clientId)  -- string или nil
```

**Пример клиента:**
```lua
function Start()
    BlazeBolt.NetInit()
    client = BlazeBolt.CreateTCPClient()
    BlazeBolt.TCPClientConnect(client, "127.0.0.1", 7777)
end

function Update(dt)
    local data = BlazeBolt.TCPClientReceive(client)
    if data then
        BlazeBolt.Print("Сервер: " .. data)
    end

    -- Отправка по нажатию пробела
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.TCPClientSend(client, "Привет от клиента!")
    end
end
```

**Отключение:**
```lua
BlazeBolt.TCPClientDisconnect(clientId)
```

**Проверка подключения:**
```lua
connected = BlazeBolt.TCPClientIsConnected(clientId)  -- boolean
```

---

### UDP (User Datagram Protocol)

UDP — быстрый протокол без установления соединения. Данные отправляются в виде дейтаграмм. Нет гарантии доставки или порядка. Используйте для: позиций игроков, состояния игры в реальном времени, стрельбы.

#### UDP Server

**Создание сервера:**
```lua
serverId = BlazeBolt.CreateUDPServer(port)  -- integer или nil
```

**Остановка сервера:**
```lua
BlazeBolt.UDPServerStop(serverId)
```

**Проверка работы:**
```lua
running = BlazeBolt.UDPServerIsRunning(serverId)  -- boolean
```

**Обработка событий:**
```lua
BlazeBolt.UDPServerPoll(serverId)
```
Обрабатывает входящие данные. Вызывайте в `Update()` для обновления состояния сервера.

**Получение данных от конкретного пира:**
```lua
data = BlazeBolt.UDPServerReceive(serverId, peerId)  -- string или nil
```

**Получение данных от любого пира:**
```lua
peerId, data = BlazeBolt.UDPServerReceiveAny(serverId)  -- integer, string или nil
```
Возвращает ID пира и данные, или `nil` если данных нет. Удобно для обработки сообщений от всех клиентов в одном вызове.

**Пример UDP-сервера:**
```lua
function Start()
    BlazeBolt.NetInit()
    server = BlazeBolt.CreateUDPServer(8888)
end

function Update(dt)
    local peerId, data = BlazeBolt.UDPServerReceiveAny(server)
    if peerId then
        BlazeBolt.Print("Пир " .. peerId .. ": " .. data)
        -- Эхо-ответ
        BlazeBolt.UDPServerSend(server, peerId, "Получено: " .. data)
    end
end
```

**Отправка данных пиру:**
```lua
success = BlazeBolt.UDPServerSend(serverId, peerId, data)  -- boolean
```

**Удаление пира:**
```lua
BlazeBolt.UDPServerRemovePeer(serverId, peerId)
```

**Количество пиров:**
```lua
count = BlazeBolt.UDPServerGetPeerCount(serverId)  -- integer
```

**Проверка существования пира:**
```lua
known = BlazeBolt.UDPServerIsPeerKnown(serverId, peerId)  -- boolean
```

---

#### UDP Client

**Создание клиента:**
```lua
clientId = BlazeBolt.CreateUDPClient()  -- integer
```

**Подключение (установка удалённого адреса):**
```lua
success = BlazeBolt.UDPClientConnect(clientId, host, port)  -- boolean
```
Для UDP "подключение" означает установку адреса получателя. Физическое соединение не устанавливается.

**Пример UDP-клиента:**
```lua
function Start()
    BlazeBolt.NetInit()
    client = BlazeBolt.CreateUDPClient()
    BlazeBolt.UDPClientConnect(client, "127.0.0.1", 8888)
    BlazeBolt.UDPClientSend(client, "PING")
end

function Update(dt)
    local data = BlazeBolt.UDPClientReceive(client)
    if data then
        BlazeBolt.Print("Ответ: " .. data)
    end
end
```

**Отправка данных:**
```lua
success = BlazeBolt.UDPClientSend(clientId, data)  -- boolean
```

**Получение данных:**
```lua
data = BlazeBolt.UDPClientReceive(clientId)  -- string или nil
```

**Отключение:**
```lua
BlazeBolt.UDPClientDisconnect(clientId)
```

**Проверка подключения:**
```lua
connected = BlazeBolt.UDPClientIsConnected(clientId)  -- boolean
```

---

### Пример: multiplayer-игра на TCP

```lua
-- server.lua
local server
local players = {}

function Start()
    BlazeBolt.NetInit()
    server = BlazeBolt.CreateTCPServer(7777)
    BlazeBolt.Print("Сервер запущен на порту 7777")
end

function Update(dt)
    -- Новые подключения
    local id, ip, port = BlazeBolt.TCPServerAccept(server)
    if id then
        players[id] = { x = 0, y = 0 }
        BlazeBolt.Print("Игрок " .. id .. " подключился с " .. ip)
        BlazeBolt.TCPServerBroadcast(server, "JOIN:" .. id)
    end

    -- Обработка сообщений
    for id, _ in pairs(players) do
        local data = BlazeBolt.TCPServerReceive(server, id)
        if data then
            local cmd, params = data:match("^(%w+):(.+)$")
            if cmd == "POS" then
                local x, y = params:match("([%d%.]+),([%d%.]+)")
                players[id].x = tonumber(x)
                players[id].y = tonumber(y)
                BlazeBolt.TCPServerBroadcast(server, "POS:" .. id .. ":" .. x .. "," .. y)
            end
        end
    end
end
```

```lua
-- client.lua
local client
local players = {}

function Start()
    BlazeBolt.NetInit()
    client = BlazeBolt.CreateTCPClient()
    BlazeBolt.TCPClientConnect(client, "127.0.0.1", 7777)
end

function Update(dt)
    local x, y = 0, 0
    if BlazeBolt.IsKeyPressed(Keys.W) then y = y + 1 end
    if BlazeBolt.IsKeyPressed(Keys.S) then y = y - 1 end
    if BlazeBolt.IsKeyPressed(Keys.A) then x = x - 1 end
    if BlazeBolt.IsKeyPressed(Keys.D) then x = x + 1 end

    if x ~= 0 or y ~= 0 then
        BlazeBolt.TCPClientSend(client, "POS:" .. x .. "," .. y)
    end

    local data = BlazeBolt.TCPClientReceive(client)
    while data do
        local cmd, params = data:match("^(%w+):(.+)$")
        if cmd == "JOIN" then
            local id = tonumber(params)
            players[id] = { sprite = BlazeBolt.CreateSprite("player.png", 0, 0) }
        elseif cmd == "POS" then
            local id, px, py = params:match("([%d%.]+):([%d%.]+),([%d%.]+)")
            id = tonumber(id)
            if players[id] then
                BlazeBolt.SpriteSetPosition(players[id].sprite, tonumber(px), tonumber(py))
            end
        end
        data = BlazeBolt.TCPClientReceive(client)
    end
end
```

---

### Выбор протокола

| Критерий | TCP | UDP |
|---|---|---|
| Надёжность доставки | Да | Нет |
| Порядок данных | Да | Нет |
| Скорость | Ниже | Выше |
| Установка соединения | Да | Нет |
| Подходит для | Чат, лобби, действия | Позиции, стрельба, FPS |

**Рекомендация:** Используйте TCP для важных событий (подключение, выстрелы, смерть) и UDP для частых обновлений позиций (60 раз в секунду).

### Важные замечания

- Вызывайте `BlazeBolt.NetInit()` в `Start()` перед использованием сети
- Все ID (серверов, клиентов, пиров) — целые числа, начинающиеся с 1
- Данные передаются как строки. Для структурированных данных используйте формат `"CMD:param1,param2"` или JSON
- Сокеты работают в non-blocking режиме — вызовы `Receive` не блокируют игру
- Максимальный размер пакета: 65507 байт (ограничение UDP)
- При завершении игры рекомендуется вызывать `TCPServerStop` / `TCPServerDisconnect` / `UDPServerStop` / `UDPClientDisconnect`

---

## Скрипты и сцены

### Загрузка скриптов
```lua
result = BlazeBolt.LoadScript(path)              -- загрузить Lua-скрипт
result = BlazeBolt.LoadScriptsFromList(path)     -- загрузить список скриптов из .BlazeBoltProject
result = BlazeBolt.ReloadScript(path)            -- перезагрузить скрипт
result = BlazeBolt.ReloadAllScripts()            -- перезагрузить все скрипты
BlazeBolt.EnableScript(name, enabled)            -- включить/выключить скрипт
loaded = BlazeBolt.IsScriptLoaded(name)          -- проверить, загружен ли
list = BlazeBolt.GetLoadedScripts()              -- таблица загруженных скриптов
```

### Файл .BlazeBoltProject / scripts.list
```
# Комментарии начинаются с #
# Обычные скрипты: имя=путь
main=engine/scripts/main.lua
player=engine/scripts/player.lua

# Сцены (с префиксом @):
@menu=engine/scenes/menu.lua
@game=engine/scenes/game.lua
```

Файл может называться `scripts.list` или иметь расширение `.BlazeBoltProject`.

### Параллельное выполнение

Все скрипты из списка загружаются и работают одновременно (параллельно):
- Каждый скрипт может определять функции `Start()`, `Update(dt)`, `Draw()`, `End()`
- Функции `Update(dt)` всех скриптов вызываются каждый кадр
- Функции `Draw()` всех скриптов вызываются каждый кадр

### Формат модуля

Рекомендуется использовать формат модуля для избежания конфликтов:

```lua
-- player.lua
local player = {}

function player:Start()
    print("Player initialized")
end

function player:Update(dt)
    -- обновление игрока
end

function player:Draw()
    -- отрисовка игрока
end

function player:End()
    -- код завершения
end

return player
```

### Сцены

Сцены регистрируются с префиксом `@`. При вызове `BlazeBolt.LoadScene(sceneName)`:
1. Выгружается текущая сцена (`On<Scene>Unload`)
2. Загружается новая сцена (`On<Scene>Load`)
3. Сцена добавляется в список активных скриптов

### Callbacks сцен
```lua
BlazeBolt.LoadScene(sceneName)       -- переключить на другую сцену
name = BlazeBolt.GetCurrentScene()   -- получить имя текущей сцены
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
    playerBody = BlazeBolt.PhysicsCreateBody(1, 0, 0, 1, 0.2, 0.6)
    BlazeBolt.PhysicsAddRectangle(playerBody, 0.04, 0.04)
    
    -- Земля
    local ground = BlazeBolt.CreateSprite("ground.png", 0, -0.85)
    BlazeBolt.SpriteSetSize(ground, 3, 0.05)
    
    local groundBody = BlazeBolt.PhysicsCreateBody(0, 0, -0.85, 0, 0.5, 0.3)
    BlazeBolt.PhysicsAddRectangle(groundBody, 1.5, 0.025)
    
    -- Стены
    for _, x in ipairs({-0.95, 0.95}) do
        local wall = BlazeBolt.CreateSprite("wall.png", x, 0)
        BlazeBolt.SpriteSetSize(wall, 0.05, 2)
        local wb = BlazeBolt.PhysicsCreateBody(0, x, 0, 0, 0.5, 0.3)
        BlazeBolt.PhysicsAddRectangle(wb, 0.025, 1)
    end
    
    -- HUD
    scoreText = BlazeBolt.CreateText("fonts/arial.ttf", "Score: 0", -0.8, 0.85)
    BlazeBolt.TextSetColor(scoreText, 1, 1, 1, 1)
    
    -- Музыка
    BlazeBolt.LoadSound("music/bgm.wav", "bgm", true)
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
        
        local cb = BlazeBolt.PhysicsCreateBody(0, cx, cy, 0, 0, 0)
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
- Разделяйте логику на отдельные скрипты через `.BlazeBoltProject` / `scripts.list`
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
├── .BlazeBoltProject         # Список загружаемых скриптов
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

### .BlazeBoltProject — формат
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

## Математические типы

Движок предоставляет глобальные типы для работы с математикой: `Vector2`, `Vector3`, `Vector4` и `Matrix3x3`. Все типы поддерживают арифметические операторы (`+`, `-`, `*`, `/`), сравнение (`==`), унарный минус (`-v`), а также строковое представление (`tostring`).

### Vector2

Двумерный вектор с полями `x` и `y`.

```lua
local v = Vector2(1.0, 2.0)   -- создание
v.x = 5.0                      -- изменение полей
v.y = 3.0
print(v)                        -- "(5, 3)"
```

**Методы:**
| Метод | Описание |
|---|---|
| `v:length()` | Длина вектора |
| `v:lengthSquared()` | Квадрат длины (быстрее) |
| `v:normalized()` | Нормализованный копия (не изменяет оригинал) |
| `v:normalize()` | Нормализует вектор на месте |
| `v:dot(other)` | Скалярное произведение |
| `v:cross(other)` | Векторное произведение (возвращает число) |
| `v:clone()` | Копия вектора |

**Операторы:**
```lua
local a = Vector2(1, 2)
local b = Vector2(3, 4)
local c = a + b        -- Vector2(4, 6)
local d = a - b        -- Vector2(-2, -2)
local e = a * 2        -- Vector2(2, 4)
local f = 3 * a        -- Vector2(3, 6)
local g = a / 2        -- Vector2(0.5, 1)
local h = -a           -- Vector2(-1, -2)
print(a == b)          -- false
```

### Vector3

Трёхмерный вектор с полями `x`, `y`, `z`.

```lua
local v = Vector3(1.0, 2.0, 3.0)
```

**Методы:** аналогичны Vector2 (`length`, `lengthSquared`, `normalized`, `normalize`, `dot`, `cross`, `clone`), но `cross` возвращает `Vector3`.

### Vector4

Четырёхмерный вектор с полями `x`, `y`, `z`, `w`.

```lua
local v = Vector4(1.0, 2.0, 3.0, 4.0)
```

**Методы:** `length`, `lengthSquared`, `normalized`, `normalize`, `dot`, `clone`, а также:
| Метод | Описание |
|---|---|
| `v:toVector3()` | Конвертация в Vector3 (отбрасывает `w`) |

### Matrix3x3

Матрица 3x3. Индексация элементов через числовые индексы 1–9 (column-major).

```lua
local m = Matrix3x3()          -- нулевая матрица
local id = Matrix3x3.identity() -- единичная матрица
```

**Статические методы создания:**
| Метод | Описание |
|---|---|
| `Matrix3x3()` | Нулевая матрица |
| `Matrix3x3.identity()` | Единичная матрица |
| `Matrix3x3.translation(x, y)` | Матрица переноса |
| `Matrix3x3.scale(x, y)` | Матрица масштабирования (`y` по умолчанию = `x`) |
| `Matrix3x3.rotation(degrees)` | Матрица вращения (в градусах) |

**Методы:**
| Метод | Описание |
|---|---|
| `m:get(col, row)` | Получить элемент (col и row от 1 до 3) |
| `m:set(col, row, val)` | Установить элемент |
| `m:toArray()` | Конвертировать в Lua-таблицу из 9 элементов |
| `m:clone()` | Копия матрицы |

**Операторы:**
```lua
local a = Matrix3x3.translation(1, 2)
local b = Matrix3x3.rotation(45)
local c = a * b          -- умножение матриц
print(a == b)            -- false
print(a)                 -- "[[1 0 1] [0 1 2] [0 0 1]]"
```

### Пример: движение через матрицу
```lua
function Start()
    local pos = Vector2(0, 0)
    local mat = Matrix3x3.translation(pos.x, pos.y)
               * Matrix3x3.rotation(45)
               * Matrix3x3.scale(2, 2)
    BlazeBolt.Print(tostring(mat))
end
```

---

## Справочник всех функций

### Камера
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateCamera` | — | `entity` |
| `BlazeBolt.CameraSetPosition` | `entity, x, y` | — |
| `BlazeBolt.CameraGetPosition` | `entity` | `x, y` |
| `BlazeBolt.CameraSetZoom` | `entity, zoom` | — |
| `BlazeBolt.CameraGetZoom` | `entity` | `zoom` |
| `BlazeBolt.CameraSetRotation` | `entity, degrees` | — |
| `BlazeBolt.CameraGetRotation` | `entity` | `degrees` |

### Частицы
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateParticleSystem` | — | `entity` |
| `BlazeBolt.ParticleSystemSetPosition` | `entity, x, y` | — |
| `BlazeBolt.ParticleSystemSetTexture` | `entity, path` | — |
| `BlazeBolt.ParticleSystemSetEmissionRate` | `entity, rate` | — |
| `BlazeBolt.ParticleSystemGetEmissionRate` | `entity` | `rate` |
| `BlazeBolt.ParticleSystemSetLifetime` | `entity, min, max` | — |
| `BlazeBolt.ParticleSystemSetSpeed` | `entity, min, max` | — |
| `BlazeBolt.ParticleSystemSetSize` | `entity, min, max` | — |
| `BlazeBolt.ParticleSystemSetEndSize` | `entity, min, max` | — |
| `BlazeBolt.ParticleSystemSetColor` | `entity, r1, g1, b1, a1, r2, g2, b2, a2` | — |
| `BlazeBolt.ParticleSystemSetDirection` | `entity, minAngle, maxAngle` | — |
| `BlazeBolt.ParticleSystemSetRotationSpeed` | `entity, degPerSec` | — |
| `BlazeBolt.ParticleSystemSetActive` | `entity, bool` | — |
| `BlazeBolt.ParticleSystemIsActive` | `entity` | `bool` |
| `BlazeBolt.ParticleSystemSetVisible` | `entity, bool` | — |
| `BlazeBolt.ParticleSystemIsVisible` | `entity` | `bool` |
| `BlazeBolt.ParticleSystemEmit` | `entity, count` | — |
| `BlazeBolt.ParticleSystemClear` | `entity` | — |
| `BlazeBolt.ParticleSystemGetCount` | `entity` | `count` |

### Тайловые карты (Tileset2D)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateTileset` | `atlasPath, tileW, tileH, tileCountX, tileCountY` | `entity` |
| `BlazeBolt.TilesetSetMap` | `entity, mapTable` | — |
| `BlazeBolt.TilesetGetTile` | `entity, col, row` | `tileIndex` |
| `BlazeBolt.TilesetSetTile` | `entity, col, row, tileIndex` | — |
| `BlazeBolt.TilesetSetTileSize` | `entity, w, h` | — |
| `BlazeBolt.TilesetGetTileSize` | `entity` | `w, h` |
| `BlazeBolt.TilesetSetPosition` | `entity, x, y` | — |
| `BlazeBolt.TilesetGetPosition` | `entity` | `x, y` |
| `BlazeBolt.TilesetGetMapWidth` | `entity` | `width` |
| `BlazeBolt.TilesetGetMapHeight` | `entity` | `height` |
| `BlazeBolt.TilesetGetTileCount` | `entity` | `count` |
| `BlazeBolt.TilesetDraw` | `entity` | — |
| `BlazeBolt.DestroyTileset` | `entity` | — |

### Освещение (Light2D)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreatePointLight` | `[x, y, r, g, b, intensity, radius]` | `entity` |
| `BlazeBolt.CreateAmbientLight` | `[r, g, b, intensity]` | `entity` |
| `BlazeBolt.LightSetPosition` | `entity, x, y` | — |
| `BlazeBolt.LightGetPosition` | `entity` | `x, y` |
| `BlazeBolt.LightSetColor` | `entity, r, g, b` | — |
| `BlazeBolt.LightGetColor` | `entity` | `r, g, b` |
| `BlazeBolt.LightSetIntensity` | `entity, intensity` | — |
| `BlazeBolt.LightGetIntensity` | `entity` | `intensity` |
| `BlazeBolt.LightSetRadius` | `entity, radius` | — |
| `BlazeBolt.LightGetRadius` | `entity` | `radius` |
| `BlazeBolt.LightSetEnabled` | `entity, bool` | — |
| `BlazeBolt.LightGetEnabled` | `entity` | `bool` |
| `BlazeBolt.DestroyLight` | `entity` | — |

### Спрайты
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateSprite` | `path, x, y` | `entity` |
| `BlazeBolt.SpriteSetPosition` | `entity, x, y` | — |
| `BlazeBolt.SpriteGetPosition` | `entity` | `x, y` |
| `BlazeBolt.SpriteSetSize` | `entity, w, h` | — |
| `BlazeBolt.SpriteGetSize` | `entity` | `w, h` |
| `BlazeBolt.SpriteSetOrigin` | `entity, ox, oy` | — |
| `BlazeBolt.SpriteGetOrigin` | `entity` | `ox, oy` |
| `BlazeBolt.SpriteSetRotation` | `entity, degrees` | — |
| `BlazeBolt.SpriteGetRotation` | `entity` | `degrees` |
| `BlazeBolt.SpriteSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.SpriteGetColor` | `entity` | `r, g, b, a` |
| `BlazeBolt.SpriteSetTexture` | `entity, path` | — |
| `BlazeBolt.SpriteSetTextureRect` | `entity, u, v, w, h` | — |
| `BlazeBolt.SpriteSetVisible` | `entity, bool` | — |
| `BlazeBolt.SpriteIsVisible` | `entity` | `bool` |

### Спрайтовые батчи (SpriteBatch)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateSpriteBatch` | `[maxSize]` | `batchEntity` |
| `BlazeBolt.SpriteBatchSetTexture` | `batchEntity, path` | — |
| `BlazeBolt.SpriteBatchAdd` | `batchEntity, spriteEntity` | `bool` |
| `BlazeBolt.SpriteBatchRemove` | `batchEntity, spriteEntity` | `bool` |
| `BlazeBolt.SpriteBatchClear` | `batchEntity` | — |
| `BlazeBolt.SpriteBatchSetMaxSize` | `batchEntity, maxSize` | — |
| `BlazeBolt.SpriteBatchGetMaxSize` | `batchEntity` | `maxSize` |
| `BlazeBolt.SpriteBatchGetCount` | `batchEntity` | `count` |
| `BlazeBolt.SpriteBatchDraw` | `batchEntity` | — |
| `BlazeBolt.DestroySpriteBatch` | `batchEntity` | — |

### Анимации
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateAnimatedSprite` | `path, x, y` | `entity` |
| `BlazeBolt.AnimatedSpritePlay` | `entity` | — |
| `BlazeBolt.AnimatedSpritePause` | `entity` | — |
| `BlazeBolt.AnimatedSpriteStop` | `entity` | — |
| `BlazeBolt.AnimatedSpriteRestart` | `entity` | — |
| `BlazeBolt.AnimatedSpriteSetLooping` | `entity, bool` | — |
| `BlazeBolt.AnimatedSpriteIsLooping` | `entity` | `bool` |
| `BlazeBolt.AnimatedSpriteSetPlaybackSpeed` | `entity, speed` | — |
| `BlazeBolt.AnimatedSpriteGetPlaybackSpeed` | `entity` | `speed` |
| `BlazeBolt.AnimatedSpriteSetFrame` | `entity, frame` | — |
| `BlazeBolt.AnimatedSpriteGetCurrentFrame` | `entity` | `frame` |
| `BlazeBolt.AnimatedSpriteGetNumFrames` | `entity` | `count` |
| `BlazeBolt.AnimatedSpriteIsPlaying` | `entity` | `bool` |
| `BlazeBolt.AnimatedSpriteSetPosition` | `entity, x, y` | — |
| `BlazeBolt.AnimatedSpriteGetPosition` | `entity` | `x, y` |
| `BlazeBolt.AnimatedSpriteSetSize` | `entity, w, h` | — |
| `BlazeBolt.AnimatedSpriteGetSize` | `entity` | `w, h` |
| `BlazeBolt.AnimatedSpriteSetOrigin` | `entity, ox, oy` | — |
| `BlazeBolt.AnimatedSpriteGetOrigin` | `entity` | `ox, oy` |
| `BlazeBolt.AnimatedSpriteSetRotation` | `entity, degrees` | — |
| `BlazeBolt.AnimatedSpriteGetRotation` | `entity` | `degrees` |
| `BlazeBolt.AnimatedSpriteSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.AnimatedSpriteGetColor` | `entity` | `r, g, b, a` |

### Константы выравнивания текста
```lua
TextAlignment.LEFT     -- По левому краю (по умолчанию)
TextAlignment.CENTER   -- По центру
TextAlignment.RIGHT    -- По правому краю
```

### Текст
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateText` | `font, text, x, y` | `entity` |
| `BlazeBolt.TextSetString` | `entity, text` | — |
| `BlazeBolt.TextGetString` | `entity` | `text` |
| `BlazeBolt.TextSetPosition` | `entity, x, y` | — |
| `BlazeBolt.TextGetPosition` | `entity` | `x, y` |
| `BlazeBolt.TextSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.TextGetColor` | `entity` | `r, g, b, a` |
| `BlazeBolt.TextSetScale` | `entity, sx, sy` | — |
| `BlazeBolt.TextGetScale` | `entity` | `sx, sy` |
| `BlazeBolt.TextSetOrigin` | `entity, ox, oy` | — |
| `BlazeBolt.TextGetOrigin` | `entity` | `ox, oy` |
| `BlazeBolt.TextSetRotation` | `entity, degrees` | — |
| `BlazeBolt.TextGetRotation` | `entity` | `degrees` |
| `BlazeBolt.TextSetAlignment` | `entity, alignment` | — |
| `BlazeBolt.TextGetAlignment` | `entity` | `alignment` |
| `BlazeBolt.TextSetVisible` | `entity, bool` | — |
| `BlazeBolt.TextIsVisible` | `entity` | `bool` |

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
| `BlazeBolt.GetMouseX` | — | `x` |
| `BlazeBolt.GetMouseY` | — | `y` |
| `BlazeBolt.GetMouseDeltaX` | — | `dx` |
| `BlazeBolt.GetMouseDeltaY` | — | `dy` |
| `BlazeBolt.IsMouseButtonPressed` | `button` | `bool` |
| `BlazeBolt.IsMouseButtonJustPressed` | `button` | `bool` |
| `BlazeBolt.GetScrollY` | — | `scrollY` |

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
| `BlazeBolt.PhysicsSetFriction` | `bodyEntity, friction` | — |
| `BlazeBolt.PhysicsGetFriction` | `bodyEntity` | `friction` |
| `BlazeBolt.PhysicsSetRestitution` | `bodyEntity, restitution` | — |
| `BlazeBolt.PhysicsGetRestitution` | `bodyEntity` | `restitution` |
| `BlazeBolt.PhysicsStep` | — | — |
| `BlazeBolt.PhysicsSyncSprite` | `bodyEntity, spriteEntity` | — |
| `BlazeBolt.PhysicsSyncAnimatedSprite` | `bodyEntity, animationEntity` | — |
| `BlazeBolt.PhysicsSyncText` | `bodyEntity, textEntity` | — |

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
| `BlazeBolt.AddConsoleMessage` | `msg, type` | — |

### Шумы (Noise)

Функции для генерации процедурного шума. Основаны на алгоритмах Перлина и симплекс-шума.

| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `PerlinNoise1D` | `x` | `float` | Одномерный шум Перлина, возвращает значение в диапазоне ~[-1, 1] |
| `PerlinNoise2D` | `x, y` | `float` | Двумерный шум Перлина, возвращает значение в диапазоне ~[-1, 1] |
| `PerlinNoise3D` | `x, y, z` | `float` | Трёхмерный шум Перлина, возвращает значение в диапазоне ~[-1, 1] |
| `SimplexNoise2D` | `x, y` | `float` | Двумерный симплекс-шум, менее артефактный чем Перлина |
| `ValueNoise2D` | `x, y` | `float` | Value-шум (простая интерполяция между случайными значениями), диапазон [0, 1] |
| `FbmNoise2D` | `x, y, [octaves=6], [lacunarity=2], [gain=0.5]` | `float` | Фрактальный шум Перлина (FBM) с настраиваемым количеством октав |
| `FbmSimplexNoise2D` | `x, y, [octaves=6], [lacunarity=2], [gain=0.5]` | `float` | Фрактальный симплекс-шум |
| `DomainWarpNoise2D` | `x, y, [warpScale=1]` | `float` | Шум Перлина с domain warping для более органичных текстур |
| `SetNoiseSeed` | `seed` | — | Устанавливает seed для генератора шума |

**Примеры использования:**
```lua
-- Базовый шум Перлина
local value = PerlinNoise2D(x * 0.1, y * 0.1)

-- Фрактальный шум (FBM) для ландшафта
local terrain = FbmNoise2D(x * 0.02, y * 0.02, 8, 2.0, 0.5)

-- Domain warping для облаков
local cloud = DomainWarpNoise2D(x * 0.01, y * 0.01, 2.0)

-- Шум с кастомным seed
SetNoiseSeed(12345)
local noise1 = PerlinNoise2D(x, y)
SetNoiseSeed(67890)
local noise2 = PerlinNoise2D(x, y)
```

---

### Шейдеры
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateShader` | `name, vertexPath, fragmentPath` | `shaderId` |
| `BlazeBolt.DestroyShader` | `shaderId` | — |
| `BlazeBolt.SetEntityShader` | `entity, shaderId` | — |
| `BlazeBolt.GetEntityShader` | `entity` | `shaderId` |
| `BlazeBolt.UseShader` | `shaderId` | — |
| `BlazeBolt.SetShaderFloat` | `shaderId, name, value` | — |
| `BlazeBolt.SetShaderInt` | `shaderId, name, value` | — |
| `BlazeBolt.SetShaderVec2` | `shaderId, name, x, y` | — |
| `BlazeBolt.SetShaderVec3` | `shaderId, name, x, y, z` | — |
| `BlazeBolt.SetShaderVec4` | `shaderId, name, x, y, z, w` | — |

### Порядок отрисовки
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.SetRenderOrder` | `orderTable` | — |
| `BlazeBolt.GetRenderOrder` | — | `table` |

### Математические типы (глобальные)

**Vector2**
| Операция | Описание |
|---|---|
| `Vector2(x, y)` | Создание |
| `v.x`, `v.y` | Поля |
| `v:length()` | Длина |
| `v:lengthSquared()` | Квадрат длины |
| `v:normalized()` | Нормализованная копия |
| `v:normalize()` | Нормализация на месте |
| `v:dot(other)` | Скалярное произведение |
| `v:cross(other)` | Векторное произведение |
| `v:clone()` | Копия |
| `v1 + v2`, `v1 - v2` | Сложение/вычитание |
| `v * scalar`, `scalar * v` | Умножение на число |
| `v / scalar` | Деление на число |
| `-v` | Унарный минус |
| `v1 == v2` | Сравнение |

**Vector3**
| Операция | Описание |
|---|---|
| `Vector3(x, y, z)` | Создание |
| `v.x`, `v.y`, `v.z` | Поля |
| Методы | Аналогичны Vector2 + `cross` возвращает Vector3 |

**Vector4**
| Операция | Описание |
|---|---|
| `Vector4(x, y, z, w)` | Создание |
| `v.x`, `v.y`, `v.z`, `v.w` | Поля |
| `v:toVector3()` | Конвертация в Vector3 |
| Методы | Аналогичны Vector2 |

**Matrix3x3**
| Операция | Описание |
|---|---|
| `Matrix3x3()` | Нулевая матрица |
| `Matrix3x3.identity()` | Единичная матрица |
| `Matrix3x3.translation(x, y)` | Перенос |
| `Matrix3x3.scale(x, y)` | Масштабирование |
| `Matrix3x3.rotation(degrees)` | Вращение |
| `m:get(col, row)` | Получить элемент (1–3) |
| `m:set(col, row, val)` | Установить элемент |
| `m:toArray()` | В Lua-таблицу |
| `m:clone()` | Копия |
| `m1 * m2` | Умножение матриц |
| `m1 == m2` | Сравнение |

### Сеть — Инициализация
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.NetInit` | — | `bool` |
| `BlazeBolt.NetShutdown` | — | — |

### Сеть — TCP Server
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateTCPServer` | `port` | `serverId` |
| `BlazeBolt.TCPServerStop` | `serverId` | — |
| `BlazeBolt.TCPServerIsRunning` | `serverId` | `bool` |
| `BlazeBolt.TCPServerPoll` | `serverId` | — |
| `BlazeBolt.TCPServerAccept` | `serverId` | `clientId, ip, port` |
| `BlazeBolt.TCPServerSend` | `serverId, clientId, data` | `bool` |
| `BlazeBolt.TCPServerBroadcast` | `serverId, data` | `bool` |
| `BlazeBolt.TCPServerReceive` | `serverId, clientId` | `data` |
| `BlazeBolt.TCPServerDisconnect` | `serverId, clientId` | — |
| `BlazeBolt.TCPServerGetClientCount` | `serverId` | `count` |
| `BlazeBolt.TCPServerIsClientConnected` | `serverId, clientId` | `bool` |

### Сеть — TCP Client
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateTCPClient` | — | `clientId` |
| `BlazeBolt.TCPClientConnect` | `clientId, host, port` | `bool` |
| `BlazeBolt.TCPClientSend` | `clientId, data` | `bool` |
| `BlazeBolt.TCPClientReceive` | `clientId` | `data` |
| `BlazeBolt.TCPClientDisconnect` | `clientId` | — |
| `BlazeBolt.TCPClientIsConnected` | `clientId` | `bool` |

### Сеть — UDP Server
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateUDPServer` | `port` | `serverId` |
| `BlazeBolt.UDPServerStop` | `serverId` | — |
| `BlazeBolt.UDPServerIsRunning` | `serverId` | `bool` |
| `BlazeBolt.UDPServerPoll` | `serverId` | — |
| `BlazeBolt.UDPServerSend` | `serverId, peerId, data` | `bool` |
| `BlazeBolt.UDPServerReceive` | `serverId, peerId` | `data` |
| `BlazeBolt.UDPServerReceiveAny` | `serverId` | `peerId, data` |
| `BlazeBolt.UDPServerRemovePeer` | `serverId, peerId` | — |
| `BlazeBolt.UDPServerGetPeerCount` | `serverId` | `count` |
| `BlazeBolt.UDPServerIsPeerKnown` | `serverId, peerId` | `bool` |

### Сеть — UDP Client
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateUDPClient` | — | `clientId` |
| `BlazeBolt.UDPClientConnect` | `clientId, host, port` | `bool` |
| `BlazeBolt.UDPClientSend` | `clientId, data` | `bool` |
| `BlazeBolt.UDPClientReceive` | `clientId` | `data` |
| `BlazeBolt.UDPClientDisconnect` | `clientId` | — |
| `BlazeBolt.UDPClientIsConnected` | `clientId` | `bool` |

### Скрипты и сцены
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.LoadScript` | `path` | `bool` |
| `BlazeBolt.LoadScriptsFromList` | `path` | `bool` |
| `BlazeBolt.ReloadScript` | `path` | `bool` |
| `BlazeBolt.ReloadAllScripts` | — | `bool` |
| `BlazeBolt.EnableScript` | `name, bool` | — |
| `BlazeBolt.IsScriptLoaded` | `name` | `bool` |
| `BlazeBolt.GetLoadedScripts` | — | `table` |
| `BlazeBolt.LoadScene` | `sceneName` | `bool` |
| `BlazeBolt.GetCurrentScene` | — | `name` |

---

BlazeBolt Game Engine 1.0
