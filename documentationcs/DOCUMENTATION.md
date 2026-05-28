# BlazeBolt Lua API 1.0

Полная документация по Lua API игрового движка BlazeBolt 1.0.

> **ВНИМАНИЕ:** Данная документация описывает только реально существующие функции Lua API,
> полученные из исходного кода C++ (`luaEngine.cpp`). Все функции, приведённые ниже,
> доступны для вызова из Lua-скриптов.

## Оглавление
- [Система координат](#система-координат)
- [Жизненный цикл](#жизненный-цикл)
- [Спрайты](#спрайты)
- [Анимированные спрайты](#анимированные-спрайты)
- [Текст](#текст)
- [Меши](#меши)
- [Камера](#камера)
- [Частицы](#частицы)
- [Окно](#окно)
- [Ввод](#ввод)
- [Звук](#звук)
- [Управление объектами](#управление-объектами)
- [Физика](#физика)
- [Шейдеры](#шейдеры)
- [Утилиты](#утилиты)
- [Скрипты и сцены](#скрипты-и-сцены)
- [Пример: полная мини-игра](#пример-полная-мини-игра)
- [Лучшие практики](#лучшие-практики)
- [Структура проекта](#структура-проекта)
- [Справочник всех функций](#справочник-всех-функций)

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

### Текстура
```lua
BlazeBolt.SpriteSetTexture(entity, texturePath)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| texturePath | string | Путь к файлу текстуры (PNG) |

Смена текстуры во время выполнения.

### Позиция
```lua
BlazeBolt.SpriteSetPosition(entity, x, y)
x, y = BlazeBolt.SpriteGetPosition(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `x, y` (number, number) — позиция в NDC

### Текстурная область (кадр)
```lua
BlazeBolt.SpriteSetTextureRect(entity, x, y, width, height)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| x | number | X-смещение в текстуре (пиксели) |
| y | number | Y-смещение в текстуре (пиксели) |
| width | number | Ширина области (пиксели) |
| height | number | Высота области (пиксели) |

Позволяет отобразить часть текстуры (например, для спрайт-листов).

### Размер
```lua
BlazeBolt.SpriteSetSize(entity, width, height)
width, height = BlazeBolt.SpriteGetSize(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| width | number | Ширина в NDC |
| height | number | Высота в NDC |

**Возвращает:** `width, height` (number, number) — размер в NDC

### Точка привязки
```lua
BlazeBolt.SpriteSetOrigin(entity, originX, originY)
originX, originY = BlazeBolt.SpriteGetOrigin(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| originX | number | Точка привязки X (0–1) |
| originY | number | Точка привязки Y (0–1) |

**Возвращает:** `originX, originY` (number, number) — точка привязки

Значения от `0` до `1`. `(0.5, 0.5)` — центр, `(0, 0)` — левый нижний угол.

### Вращение
```lua
BlazeBolt.SpriteSetRotation(entity, degrees)
degrees = BlazeBolt.SpriteGetRotation(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| degrees | number | Угол в градусах |

**Возвращает:** `degrees` (number) — угол в градусах

### Цвет
```lua
BlazeBolt.SpriteSetColor(entity, r, g, b, a)
r, g, b, a = BlazeBolt.SpriteGetColor(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| r | number | Красный (0–1) |
| g | number | Зелёный (0–1) |
| b | number | Синий (0–1) |
| a | number | Альфа (0–1) |

**Возвращает:** `r, g, b, a` (number, number, number, number) — цвет

Значения от `0` до `1`. По умолчанию `(1, 1, 1, 1)` — белый.

### Видимость
```lua
BlazeBolt.SpriteSetVisible(entity, visible)
visible = BlazeBolt.SpriteIsVisible(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор спрайта |
| visible | boolean | Видимость (`true`/`false`) |

**Возвращает:** `visible` (boolean) — видимость

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

## Анимированные спрайты

### Создание
```lua
entity = BlazeBolt.CreateAnimatedSprite(texturePath, frameWidth, frameHeight, totalFrames, framesPerRow, fps, x, y)
```
| Параметр | Тип | Описание |
|---|---|---|
| texturePath | string | Путь к спрайт-листу (PNG) |
| frameWidth | int | Ширина одного кадра (px) |
| frameHeight | int | Высота одного кадра (px) |
| totalFrames | int | Общее количество кадров |
| framesPerRow | int | Количество кадров в строке |
| fps | number | Частота кадров в секунду |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `entity` (integer) — идентификатор анимированного спрайта

### Воспроизведение
```lua
BlazeBolt.AnimatedSpritePlay(entity)
isPlaying = BlazeBolt.AnimatedSpriteIsPlaying(entity)
BlazeBolt.AnimatedSpritePause(entity)
BlazeBolt.AnimatedSpriteStop(entity)
BlazeBolt.AnimatedSpriteRestart(entity)
```
| Функция | Параметры | Описание |
|---|---|---|
| `AnimatedSpritePlay` | entity | Начать воспроизведение |
| `AnimatedSpriteIsPlaying` | entity | Проверить, воспроизводится ли |
| `AnimatedSpritePause` | entity | Приостановить |
| `AnimatedSpriteStop` | entity | Остановить, сброс на первый кадр |
| `AnimatedSpriteRestart` | entity | Перезапустить с начала |

**Возвращает (AnimatedSpriteIsPlaying):** `isPlaying` (boolean)

### Настройки
```lua
BlazeBolt.AnimatedSpriteSetLooping(entity, looping)
looping = BlazeBolt.AnimatedSpriteIsLooping(entity)
BlazeBolt.AnimatedSpriteSetPlaybackSpeed(entity, speed)
speed = BlazeBolt.AnimatedSpriteGetPlaybackSpeed(entity)
BlazeBolt.AnimatedSpriteSetFrame(entity, frameIndex)
frameIndex = BlazeBolt.AnimatedSpriteGetCurrentFrame(entity)
totalFrames = BlazeBolt.AnimatedSpriteGetNumFrames(entity)
```
| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `AnimatedSpriteSetLooping` | entity, looping (bool) | — | Зацикливание |
| `AnimatedSpriteIsLooping` | entity | looping (bool) | Зациклен ли |
| `AnimatedSpriteSetPlaybackSpeed` | entity, speed (number) | — | Множитель скорости (1.0 = норма) |
| `AnimatedSpriteGetPlaybackSpeed` | entity | speed (number) | Текущая скорость |
| `AnimatedSpriteSetFrame` | entity, frameIndex (int) | — | Перейти к конкретному кадру |
| `AnimatedSpriteGetCurrentFrame` | entity | frameIndex (int) | Текущий кадр |
| `AnimatedSpriteGetNumFrames` | entity | totalFrames (int) | Всего кадров |

### Позиция
```lua
BlazeBolt.AnimatedSpriteSetPosition(entity, x, y)
x, y = BlazeBolt.AnimatedSpriteGetPosition(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор анимированного спрайта |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `x, y` (number, number) — позиция в NDC

### Размер
```lua
BlazeBolt.AnimatedSpriteSetSize(entity, width, height)
width, height = BlazeBolt.AnimatedSpriteGetSize(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор анимированного спрайта |
| width | number | Ширина в NDC |
| height | number | Высота в NDC |

**Возвращает:** `width, height` (number, number) — размер в NDC

### Точка привязки
```lua
BlazeBolt.AnimatedSpriteSetOrigin(entity, originX, originY)
originX, originY = BlazeBolt.AnimatedSpriteGetOrigin(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор анимированного спрайта |
| originX | number | Точка привязки X (0–1) |
| originY | number | Точка привязки Y (0–1) |

**Возвращает:** `originX, originY` (number, number) — точка привязки

### Вращение
```lua
BlazeBolt.AnimatedSpriteSetRotation(entity, degrees)
degrees = BlazeBolt.AnimatedSpriteGetRotation(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор анимированного спрайта |
| degrees | number | Угол в градусах |

**Возвращает:** `degrees` (number) — угол в градусах

### Цвет
```lua
BlazeBolt.AnimatedSpriteSetColor(entity, r, g, b, a)
r, g, b, a = BlazeBolt.AnimatedSpriteGetColor(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор анимированного спрайта |
| r | number | Красный (0–1) |
| g | number | Зелёный (0–1) |
| b | number | Синий (0–1) |
| a | number | Альфа (0–1) |

**Возвращает:** `r, g, b, a` (number, number, number, number) — цвет

### Пример
```lua
function Start()
    local anim = BlazeBolt.CreateAnimatedSprite("player_sheet.png", 32, 32, 8, 4, 10, 0, 0)
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
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| text | string | Текст для отображения |

**Возвращает:** `text` (string) — текущий текст

### Позиция
```lua
BlazeBolt.TextSetPosition(entity, x, y)
x, y = BlazeBolt.TextGetPosition(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `x, y` (number, number) — позиция в NDC

### Цвет
```lua
BlazeBolt.TextSetColor(entity, r, g, b, a)
r, g, b, a = BlazeBolt.TextGetColor(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| r | number | Красный (0–1) |
| g | number | Зелёный (0–1) |
| b | number | Синий (0–1) |
| a | number | Альфа (0–1) |

**Возвращает:** `r, g, b, a` (number, number, number, number) — цвет

Значения от `0` до `1`.

### Масштаб
```lua
BlazeBolt.TextSetScale(entity, scaleX, scaleY)
scaleX, scaleY = BlazeBolt.TextGetScale(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| scaleX | number | Масштаб по X |
| scaleY | number | Масштаб по Y |

**Возвращает:** `scaleX, scaleY` (number, number) — масштаб

### Точка привязки
```lua
BlazeBolt.TextSetOrigin(entity, originX, originY)
originX, originY = BlazeBolt.TextGetOrigin(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| originX | number | Точка привязки X (0–1) |
| originY | number | Точка привязки Y (0–1) |

**Возвращает:** `originX, originY` (number, number) — точка привязки

Значения от `0` до `1`. `(0.5, 0.5)` — центр текста.

### Вращение
```lua
BlazeBolt.TextSetRotation(entity, degrees)
degrees = BlazeBolt.TextGetRotation(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| degrees | number | Угол в градусах |

**Возвращает:** `degrees` (number) — угол в градусах

### Выравнивание
```lua
BlazeBolt.TextSetAlignment(entity, alignment)
alignment = BlazeBolt.TextGetAlignment(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| alignment | TextAlignment | Выравнивание (см. константы ниже) |

**Возвращает:** `alignment` (TextAlignment) — выравнивание

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
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор текста |
| visible | boolean | Видимость (`true`/`false`) |

**Возвращает:** `visible` (boolean) — видимость

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

**Возвращает:** `entity` (integer) — идентификатор меша

### Установка данных
```lua
BlazeBolt.MeshSetData(entity, vertices, indices)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор меша |
| vertices | table | Таблица вершин `{x, y, u, v}` |
| indices | table | Таблица индексов треугольников |

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
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор меша |

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

## Камера

### Создание
```lua
entity = BlazeBolt.CreateCamera()
```
**Возвращает:** `entity` (integer) — идентификатор камеры

### Позиция
```lua
BlazeBolt.CameraSetPosition(entity, x, y)
x, y = BlazeBolt.CameraGetPosition(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор камеры |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `x, y` (number, number) — позиция в NDC

### Зум
```lua
BlazeBolt.CameraSetZoom(entity, zoom)
zoom = BlazeBolt.CameraGetZoom(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор камеры |
| zoom | number | Коэффициент увеличения |

**Возвращает:** `zoom` (number) — коэффициент увеличения

### Вращение
```lua
BlazeBolt.CameraSetRotation(entity, degrees)
degrees = BlazeBolt.CameraGetRotation(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор камеры |
| degrees | number | Угол в градусах |

**Возвращает:** `degrees` (number) — угол в градусах

### Пример
```lua
function Start()
    cam = BlazeBolt.CreateCamera()
    BlazeBolt.CameraSetPosition(cam, 0.5, 0)
    BlazeBolt.CameraSetZoom(cam, 1.5)
end

function Update(dt)
    if BlazeBolt.IsKeyPressed(Keys.RIGHT) then
        local x, y = BlazeBolt.CameraGetPosition(cam)
        BlazeBolt.CameraSetPosition(cam, x + 0.5 * dt, y)
    end
end
```

---

## Частицы

### Создание системы частиц
```lua
entity = BlazeBolt.CreateParticleSystem(texturePath, x, y)
```
| Параметр | Тип | Описание |
|---|---|---|
| texturePath | string | Путь к текстуре частицы (PNG) |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

**Возвращает:** `entity` (integer) — идентификатор системы частиц

### Позиция
```lua
BlazeBolt.ParticleSystemSetPosition(entity, x, y)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |

### Текстура
```lua
BlazeBolt.ParticleSystemSetTexture(entity, texturePath)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| texturePath | string | Путь к текстуре (PNG) |

### Параметры эмиссии
```lua
BlazeBolt.ParticleSystemSetEmissionRate(entity, rate)
rate = BlazeBolt.ParticleSystemGetEmissionRate(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| rate | number | Количество частиц в секунду |

**Возвращает:** `rate` (number) — количество частиц в секунду

### Время жизни
```lua
BlazeBolt.ParticleSystemSetLifetime(entity, lifetime)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| lifetime | number | Время жизни частицы (секунды) |

### Скорость
```lua
BlazeBolt.ParticleSystemSetSpeed(entity, speed)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| speed | number | Скорость частиц |

### Размер
```lua
BlazeBolt.ParticleSystemSetSize(entity, size)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| size | number | Начальный размер частицы |

### Конечный размер
```lua
BlazeBolt.ParticleSystemSetEndSize(entity, endSize)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| endSize | number | Конечный размер частицы |

### Цвет
```lua
BlazeBolt.ParticleSystemSetColor(entity, r, g, b, a)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| r | number | Красный (0–1) |
| g | number | Зелёный (0–1) |
| b | number | Синий (0–1) |
| a | number | Альфа (0–1) |

### Направление
```lua
BlazeBolt.ParticleSystemSetDirection(entity, direction)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| direction | number | Угол направления в градусах |

### Скорость вращения
```lua
BlazeBolt.ParticleSystemSetRotationSpeed(entity, speed)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| speed | number | Скорость вращения частиц (градусы/сек) |

### Активность
```lua
BlazeBolt.ParticleSystemSetActive(entity, active)
active = BlazeBolt.ParticleSystemIsActive(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| active | boolean | Активна ли система (`true`/`false`) |

**Возвращает:** `active` (boolean) — активность

### Видимость
```lua
BlazeBolt.ParticleSystemSetVisible(entity, visible)
visible = BlazeBolt.ParticleSystemIsVisible(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| visible | boolean | Видимость (`true`/`false`) |

**Возвращает:** `visible` (boolean) — видимость

### Принудительный выброс
```lua
BlazeBolt.ParticleSystemEmit(entity, count)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |
| count | int | Количество частиц для выброса |

### Очистка
```lua
BlazeBolt.ParticleSystemClear(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |

### Счётчик
```lua
count = BlazeBolt.ParticleSystemGetCount(entity)
```
| Параметр | Тип | Описание |
|---|---|---|
| entity | integer | Идентификатор системы частиц |

**Возвращает:** `count` (int) — количество активных частиц

### Пример
```lua
function Start()
    ps = BlazeBolt.CreateParticleSystem("particle.png", 0, 0)
    BlazeBolt.ParticleSystemSetEmissionRate(ps, 100)
    BlazeBolt.ParticleSystemSetLifetime(ps, 1.5)
    BlazeBolt.ParticleSystemSetSpeed(ps, 0.5)
    BlazeBolt.ParticleSystemSetSize(ps, 0.03)
    BlazeBolt.ParticleSystemSetEndSize(ps, 0.01)
    BlazeBolt.ParticleSystemSetColor(ps, 1, 0.5, 0, 1)
    BlazeBolt.ParticleSystemSetDirection(ps, 90)
    BlazeBolt.ParticleSystemSetActive(ps, true)
end

function Update(dt)
    if BlazeBolt.IsMouseButtonJustPressed(MouseButtons.LEFT) then
        local mx = BlazeBolt.GetMouseX()
        local my = BlazeBolt.GetMouseY()
        local ndcX = mx / GetScreenWidth() * 2 - 1
        local ndcY = 1 - my / GetScreenHeight() * 2
        BlazeBolt.ParticleSystemSetPosition(ps, ndcX, ndcY)
        BlazeBolt.ParticleSystemEmit(ps, 50)
    end
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
| Функция | Параметры | Описание |
|---|---|---|
| `SetMainWindowTitle` | title (string) | Заголовок окна |
| `SetMainWindowSize` | width, height (int) | Размер окна в пикселях |
| `SetMainWindowPosition` | x, y (int) | Позиция окна в пикселях |
| `SetMainWindowIcon` | iconPath (string) | Путь к иконке (PNG) |
| `SetMainWindowShouldClose` | flag (boolean) | Флаг закрытия окна |

### Главное окно — получение
```lua
title  = BlazeBolt.GetMainWindowTitle()          -- string
width  = BlazeBolt.GetMainWindowWidth()          -- integer (px)
height = BlazeBolt.GetMainWindowHeight()         -- integer (px)
x, y   = BlazeBolt.GetMainWindowPosition()       -- integer, integer (px)
should = BlazeBolt.IsMainWindowShouldClose()     -- boolean
```
| Функция | Возвращает | Описание |
|---|---|---|
| `GetMainWindowTitle` | title (string) | Заголовок окна |
| `GetMainWindowWidth` | width (int) | Ширина окна в пикселях |
| `GetMainWindowHeight` | height (int) | Высота окна в пикселях |
| `GetMainWindowPosition` | x, y (int, int) | Позиция окна в пикселях |
| `IsMainWindowShouldClose` | should (boolean) | Флаг закрытия |

### Дополнительные окна
```lua
windowPtr = BlazeBolt.CreateWindow(width, height, title)
BlazeBolt.SetWindowTitle(windowPtr, title)
BlazeBolt.SetWindowSize(windowPtr, width, height)
BlazeBolt.SetWindowIcon(windowPtr, iconPath)
```
| Функция | Параметры | Описание |
|---|---|---|
| `CreateWindow` | width, height (int), title (string) | Создать новое окно |
| `SetWindowTitle` | windowPtr (int), title (string) | Заголовок окна |
| `SetWindowSize` | windowPtr (int), width, height (int) | Размер окна в пикселях |
| `SetWindowIcon` | windowPtr (int), iconPath (string) | Путь к иконке (PNG) |

**Возвращает (CreateWindow):** `windowPtr` (integer) — указатель на окно

### Размеры экрана
```lua
width  = GetScreenWidth()    -- integer (px)
height = GetScreenHeight()   -- integer (px)
```

**Возвращает:** `width` / `height` (integer) — ширина/высота экрана в пикселях

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
| Функция | Параметры | Возврат |
|---|---|---|
| `IsKeyPressed` | key (Keys) | boolean — зажата |
| `IsKeyJustPressed` | key (Keys) | boolean — нажата |

### Мышь — позиция
```lua
x  = BlazeBolt.GetMouseX()            -- позиция X курсора (px от левого края)
y  = BlazeBolt.GetMouseY()            -- позиция Y курсора (px от верхнего края)
dx = BlazeBolt.GetMouseDeltaX()       -- изменение X с прошлого кадра
dy = BlazeBolt.GetMouseDeltaY()       -- изменение Y с прошлого кадра
```
| Функция | Параметры | Возврат |
|---|---|---|
| `GetMouseX` | — | x (number) — позиция X в px |
| `GetMouseY` | — | y (number) — позиция Y в px |
| `GetMouseDeltaX` | — | dx (number) — смещение X |
| `GetMouseDeltaY` | — | dy (number) — смещение Y |

### Мышь — кнопки
```lua
isDown    = BlazeBolt.IsMouseButtonPressed(button)
isPressed = BlazeBolt.IsMouseButtonJustPressed(button)
```
| Функция | Параметры | Возврат |
|---|---|---|
| `IsMouseButtonPressed` | button (MouseButtons) | boolean — зажата |
| `IsMouseButtonJustPressed` | button (MouseButtons) | boolean — нажата |

### Мышь — прокрутка
```lua
scrollY = BlazeBolt.GetScrollY()      -- вертикальная прокрутка за кадр
```
| Функция | Параметры | Возврат |
|---|---|---|
| `GetScrollY` | — | scrollY (number) — вертикальная прокрутка |

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
Keys.MENU, Keys.LAST
```

### Константы кнопок мыши
```lua
MouseButtons.LEFT    -- 0
MouseButtons.RIGHT   -- 1
MouseButtons.MIDDLE  -- 2
```

### Константы выравнивания текста
```lua
TextAlignment.LEFT    -- 0
TextAlignment.CENTER  -- 1
TextAlignment.RIGHT   -- 2
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
| filename | string | Путь к звуковому файлу (WAV / MP3) |
| soundName | string | Имя для идентификации |
| loop | boolean | Зацикливание воспроизведения |

**Возвращает:** `soundId` (integer) — идентификатор звука

### Воспроизведение
```lua
BlazeBolt.PlaySound(soundName)
BlazeBolt.PlaySoundById(soundId)
```
| Функция | Параметры | Описание |
|---|---|---|
| `PlaySound` | soundName (string) | Воспроизвести по имени |
| `PlaySoundById` | soundId (int) | Воспроизвести по ID |

### Остановка
```lua
BlazeBolt.StopSound(soundName)
BlazeBolt.StopAllSounds()
```
| Функция | Параметры | Описание |
|---|---|---|
| `StopSound` | soundName (string) | Остановить по имени |
| `StopAllSounds` | — | Остановить все звуки |

### Громкость
```lua
BlazeBolt.SetSoundVolume(soundName, volume)   -- 0.0 - 1.0
```
| Параметр | Тип | Описание |
|---|---|---|
| soundName | string | Имя звука |
| volume | number | Громкость (0.0–1.0) |

### Проверка
```lua
isPlaying = BlazeBolt.IsSoundPlaying(soundName)
```
| Параметр | Тип | Описание |
|---|---|---|
| soundName | string | Имя звука |

**Возвращает:** `isPlaying` (boolean) — воспроизводится ли

### Поддерживаемые форматы
- **WAV** — PCM 8/16-bit, mono/stereo
- **MP3** — любой битрейт, mono/stereo

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
| Функция | Параметры | Описание |
|---|---|---|
| `Destroy` | entity (integer) | Удалить конкретный объект |
| `DestroyAll` | — | Удалить все объекты |

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

Движок использует собственную физическую систему с дискретным детектированием коллизий.

**Ключевые особенности:**
- **Fixed timestep (1/120 с):** Физика не зависит от FPS. Симуляция работает с фиксированным шагом.
- **Rotation-aware коллизия (OBB vs SAT):** Прямоугольники используют OBB (Oriented Bounding Box). Коллизия считается через SAT (Separating Axis Theorem).
- **Circle vs OBB:** Корректное определение пересечения окружности с повёрнутым прямоугольником.
- **Момент инерции:** Автоматически вычисляется из форм тела.
- **Импульсное разрешение:** Коллизии разрешаются через импульсы в точке контакта.

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
| Параметр | Тип | Описание |
|---|---|---|
| x | number | Гравитация по X |
| y | number | Гравитация по Y |

**Возвращает:** `x, y` (number, number) — гравитация

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
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |
| radius | number | Радиус окружности |
| offsetX | number | Смещение коллайдера по X |
| offsetY | number | Смещение коллайдера по Y |
| halfWidth | number | Полуширина прямоугольника |
| halfHeight | number | Полувысота прямоугольника |

### Скорость
```lua
BlazeBolt.PhysicsSetLinearVelocity(bodyEntity, vx, vy)
vx, vy = BlazeBolt.PhysicsGetLinearVelocity(bodyEntity)

BlazeBolt.PhysicsSetAngularVelocity(bodyEntity, av)
av = BlazeBolt.PhysicsGetAngularVelocity(bodyEntity)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |
| vx | number | Линейная скорость по X |
| vy | number | Линейная скорость по Y |
| av | number | Угловая скорость |

**Возвращает:** `vx, vy` (number, number) — линейная скорость; `av` (number) — угловая скорость

### Силы и импульсы
```lua
BlazeBolt.PhysicsApplyForce(bodyEntity, fx, fy)
BlazeBolt.PhysicsApplyForceAtPoint(bodyEntity, fx, fy, px, py)
BlazeBolt.PhysicsApplyImpulse(bodyEntity, ix, iy)
BlazeBolt.PhysicsApplyImpulseAtPoint(bodyEntity, ix, iy, px, py)
BlazeBolt.PhysicsApplyTorque(bodyEntity, torque)
```
| Функция | Параметры | Описание |
|---|---|---|
| `PhysicsApplyForce` | bodyEntity, fx, fy | Приложить силу |
| `PhysicsApplyForceAtPoint` | bodyEntity, fx, fy, px, py | Сила в точке |
| `PhysicsApplyImpulse` | bodyEntity, ix, iy | Импульс |
| `PhysicsApplyImpulseAtPoint` | bodyEntity, ix, iy, px, py | Импульс в точке |
| `PhysicsApplyTorque` | bodyEntity, torque | Крутящий момент |

### Позиция и вращение
```lua
BlazeBolt.PhysicsSetPosition(bodyEntity, x, y)
x, y = BlazeBolt.PhysicsGetPosition(bodyEntity)

BlazeBolt.PhysicsSetAngle(bodyEntity, angle)
angle = BlazeBolt.PhysicsGetAngle(bodyEntity)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |
| x | number | Позиция X в NDC |
| y | number | Позиция Y в NDC |
| angle | number | Угол в **радианах** |

> **Важно:** `PhysicsSetAngle` / `PhysicsGetAngle` используют **радианы**, а не градусы.

**Возвращает:** `x, y` (number, number) — позиция; `angle` (number) — угол в радианах

### Свойства тела
```lua
BlazeBolt.PhysicsSetGravityScale(bodyEntity, scale)
scale = BlazeBolt.PhysicsGetGravityScale(bodyEntity)

BlazeBolt.PhysicsSetActive(bodyEntity, active)
active = BlazeBolt.PhysicsIsActive(bodyEntity)

BlazeBolt.PhysicsSetFixedRotation(bodyEntity, fixed)
fixed = BlazeBolt.PhysicsIsFixedRotation(bodyEntity)

BlazeBolt.PhysicsSetBullet(bodyEntity, bullet)
bullet = BlazeBolt.PhysicsIsBullet(bodyEntity)

mass = BlazeBolt.PhysicsGetMass(bodyEntity)

BlazeBolt.PhysicsSetFriction(bodyEntity, friction)
friction = BlazeBolt.PhysicsGetFriction(bodyEntity)

BlazeBolt.PhysicsSetRestitution(bodyEntity, restitution)
restitution = BlazeBolt.PhysicsGetRestitution(bodyEntity)
```
| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `PhysicsSetGravityScale` | bodyEntity, scale (number) | — | Множитель гравитации |
| `PhysicsGetGravityScale` | bodyEntity | scale (number) | Множитель гравитации |
| `PhysicsSetActive` | bodyEntity, active (bool) | — | Вкл/выкл тело |
| `PhysicsIsActive` | bodyEntity | active (bool) | Активно ли тело |
| `PhysicsSetFixedRotation` | bodyEntity, fixed (bool) | — | Фиксировать вращение |
| `PhysicsIsFixedRotation` | bodyEntity | fixed (bool) | Зафиксировано ли вращение |
| `PhysicsSetBullet` | bodyEntity, bullet (bool) | — | Режим пули |
| `PhysicsIsBullet` | bodyEntity | bullet (bool) | Режим пули активен |
| `PhysicsGetMass` | bodyEntity | mass (number) | Масса тела |
| `PhysicsSetFriction` | bodyEntity, friction (number) | — | Трение (0–1) |
| `PhysicsGetFriction` | bodyEntity | friction (number) | Трение |
| `PhysicsSetRestitution` | bodyEntity, restitution (number) | — | Упругость (0–1) |
| `PhysicsGetRestitution` | bodyEntity | restitution (number) | Упругость |

### Синхронизация со спрайтом
```lua
BlazeBolt.PhysicsSyncSprite(bodyEntity, spriteEntity)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |
| spriteEntity | integer | Идентификатор спрайта |

Автоматически копирует позицию и угол физического тела в спрайт. Вызывайте в `Update()`.

### Синхронизация с текстом
```lua
BlazeBolt.PhysicsSyncText(bodyEntity, textEntity)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |
| textEntity | integer | Идентификатор текста |

### Синхронизация с анимированным спрайтом
```lua
BlazeBolt.PhysicsSyncAnimatedSprite(bodyEntity, animEntity)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |
| animEntity | integer | Идентификатор анимированного спрайта |

### Обновление симуляции
```lua
BlazeBolt.PhysicsStep()
```
Выполняет один шаг физической симуляции. Вызывайте в `Update(dt)`.

### Удаление
```lua
BlazeBolt.PhysicsDestroyBody(bodyEntity)
```
| Параметр | Тип | Описание |
|---|---|---|
| bodyEntity | integer | Идентификатор физического тела |

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
    BlazeBolt.PhysicsSyncSprite(body, player)

    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        local _, vy = BlazeBolt.PhysicsGetLinearVelocity(body)
        if vy < 0.1 then
            BlazeBolt.PhysicsApplyImpulse(body, 0, 5)
        end
    end
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
| Параметр | Тип | Описание |
|---|---|---|
| shaderId | integer | Идентификатор шейдера |

### Привязка шейдера к объекту
```lua
BlazeBolt.SetEntityShader(entity, shaderId)
shaderId = BlazeBolt.GetEntityShader(entity)
```
| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `SetEntityShader` | entity, shaderId | — | Привязать шейдер к объекту |
| `GetEntityShader` | entity | shaderId | Получить шейдер объекта |

`shaderId = 0` — использовать стандартный шейдер.

### Использование шейдера
```lua
BlazeBolt.UseShader(shaderId)
```
| Параметр | Тип | Описание |
|---|---|---|
| shaderId | integer | Идентификатор шейдера |

### Установка uniform-переменных
```lua
BlazeBolt.SetShaderFloat(shaderId, name, value)
BlazeBolt.SetShaderInt(shaderId, name, value)
BlazeBolt.SetShaderVec2(shaderId, name, x, y)
BlazeBolt.SetShaderVec3(shaderId, name, x, y, z)
BlazeBolt.SetShaderVec4(shaderId, name, x, y, z, w)
```
| Функция | Параметры | Описание |
|---|---|---|
| `SetShaderFloat` | shaderId, name (string), value (number) | Float uniform |
| `SetShaderInt` | shaderId, name (string), value (int) | Int uniform |
| `SetShaderVec2` | shaderId, name (string), x, y (number) | Vec2 uniform |
| `SetShaderVec3` | shaderId, name (string), x, y, z (number) | Vec3 uniform |
| `SetShaderVec4` | shaderId, name (string), x, y, z, w (number) | Vec4 uniform |

### Пример
```lua
function Start()
    local shader = BlazeBolt.CreateShader("invert", "shaders/invert.vert", "shaders/invert.frag")

    local sprite = BlazeBolt.CreateSprite("icon.png", 0, 0)
    BlazeBolt.SpriteSetSize(sprite, 0.5, 0.5)

    BlazeBolt.SetEntityShader(sprite, shader)
end

function Update(dt)
    BlazeBolt.UseShader(shader)
    BlazeBolt.SetShaderFloat(shader, "time", GetTime())
end
```

---

## Утилиты

### Консоль
```lua
BlazeBolt.Print(...)
BlazeBolt.AddConsoleMessage(msg, type)
```
| Функция | Параметры | Описание |
|---|---|---|
| `Print` | `...` (any) | Вывод в консоль (поддерживает любые типы) |
| `AddConsoleMessage` | msg (string), type (int) | Сообщение в консоль (0=info, 1=warning, 2=error) |

### Время
```lua
dt   = BlazeBolt.GetDeltaTime()  -- время между кадрами в секундах
time = GetTime()                 -- время с запуска движка в секундах
```
| Функция | Параметры | Возврат |
|---|---|---|
| `GetDeltaTime` | — | dt (number) — время между кадрами (сек) |
| `GetTime` | — | time (number) — время с запуска (сек) |

### Случайные числа
```lua
val      = BlazeBolt.Random(min, max)        -- float, min по умолчанию 0, max по умолчанию 1
intVal   = BlazeBolt.RandomInt(min, max)     -- integer (min и max обязательны)
BlazeBolt.SetRandomSeed(seed)                -- установить seed генератора
```
| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `Random` | min (number), max (number) | val (number) | Случайное float [min, max] |
| `RandomInt` | min (int), max (int) | intVal (int) | Случайное целое [min, max] |
| `SetRandomSeed` | seed (int) | — | Установить seed генератора |

### Выход
```lua
BlazeBolt.Quit()
```

### Пример
```lua
function Start()
    BlazeBolt.SetRandomSeed(42)
    BlazeBolt.Print("Hello from BlazeBolt!")
end

function Update(dt)
    local r = BlazeBolt.Random(0, 100)
    local ri = BlazeBolt.RandomInt(1, 6)
    BlazeBolt.Print("Float: " .. r .. ", Int: " .. ri)
    BlazeBolt.Print("Delta: " .. dt .. ", Time: " .. GetTime())
end
```

---

## Скрипты и сцены

### Загрузка скриптов
```lua
result = BlazeBolt.LoadScript(path)
result = BlazeBolt.LoadScriptsFromList(path)
result = BlazeBolt.ReloadScript(path)
result = BlazeBolt.ReloadAllScripts()
BlazeBolt.EnableScript(name, enabled)
loaded = BlazeBolt.IsScriptLoaded(name)
list = BlazeBolt.GetLoadedScripts()
```
| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `LoadScript` | path (string) | result (bool) | Загрузить Lua-скрипт |
| `LoadScriptsFromList` | path (string) | result (bool) | Загрузить список скриптов |
| `ReloadScript` | path (string) | result (bool) | Перезагрузить скрипт |
| `ReloadAllScripts` | — | result (bool) | Перезагрузить все скрипты |
| `EnableScript` | name (string), enabled (bool) | — | Включить/выключить скрипт |
| `IsScriptLoaded` | name (string) | loaded (bool) | Проверить загрузку |
| `GetLoadedScripts` | — | list (table) | Таблица загруженных скриптов |

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
Рекомендуется использовать формат модуля во избежание конфликтов:
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
BlazeBolt.LoadScene(sceneName)
name = BlazeBolt.GetCurrentScene()
```
| Функция | Параметры | Возврат | Описание |
|---|---|---|---|
| `LoadScene` | sceneName (string) | — | Переключить на другую сцену |
| `GetCurrentScene` | — | name (string) | Имя текущей сцены |

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

## Справочник всех функций

### Спрайты (16)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateSprite` | `path, x, y` | `entity` |
| `BlazeBolt.SpriteSetTexture` | `entity, path` | — |
| `BlazeBolt.SpriteSetPosition` | `entity, x, y` | — |
| `BlazeBolt.SpriteGetPosition` | `entity` | `x, y` |
| `BlazeBolt.SpriteSetTextureRect` | `entity, x, y, w, h` | — |
| `BlazeBolt.SpriteSetSize` | `entity, w, h` | — |
| `BlazeBolt.SpriteGetSize` | `entity` | `w, h` |
| `BlazeBolt.SpriteSetOrigin` | `entity, ox, oy` | — |
| `BlazeBolt.SpriteGetOrigin` | `entity` | `ox, oy` |
| `BlazeBolt.SpriteSetRotation` | `entity, degrees` | — |
| `BlazeBolt.SpriteGetRotation` | `entity` | `degrees` |
| `BlazeBolt.SpriteSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.SpriteGetColor` | `entity` | `r, g, b, a` |
| `BlazeBolt.SpriteSetVisible` | `entity, bool` | — |
| `BlazeBolt.SpriteIsVisible` | `entity` | `bool` |

### Анимированные спрайты (24)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateAnimatedSprite` | `path, fw, fh, total, perRow, fps, x, y` | `entity` |
| `BlazeBolt.AnimatedSpritePlay` | `entity` | — |
| `BlazeBolt.AnimatedSpriteIsPlaying` | `entity` | `bool` |
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

### Текст (18)
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

### Меши (3)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateMesh` | — | `entity` |
| `BlazeBolt.MeshSetData` | `entity, vertices, indices` | — |
| `BlazeBolt.MeshDraw` | `entity` | — |

### Камера (7)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateCamera` | — | `entity` |
| `BlazeBolt.CameraSetPosition` | `entity, x, y` | — |
| `BlazeBolt.CameraGetPosition` | `entity` | `x, y` |
| `BlazeBolt.CameraSetZoom` | `entity, zoom` | — |
| `BlazeBolt.CameraGetZoom` | `entity` | `zoom` |
| `BlazeBolt.CameraSetRotation` | `entity, degrees` | — |
| `BlazeBolt.CameraGetRotation` | `entity` | `degrees` |

### Система частиц (19)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.CreateParticleSystem` | `path, x, y` | `entity` |
| `BlazeBolt.ParticleSystemSetPosition` | `entity, x, y` | — |
| `BlazeBolt.ParticleSystemSetTexture` | `entity, path` | — |
| `BlazeBolt.ParticleSystemSetEmissionRate` | `entity, rate` | — |
| `BlazeBolt.ParticleSystemGetEmissionRate` | `entity` | `rate` |
| `BlazeBolt.ParticleSystemSetLifetime` | `entity, lifetime` | — |
| `BlazeBolt.ParticleSystemSetSpeed` | `entity, speed` | — |
| `BlazeBolt.ParticleSystemSetSize` | `entity, size` | — |
| `BlazeBolt.ParticleSystemSetEndSize` | `entity, endSize` | — |
| `BlazeBolt.ParticleSystemSetColor` | `entity, r, g, b, a` | — |
| `BlazeBolt.ParticleSystemSetDirection` | `entity, direction` | — |
| `BlazeBolt.ParticleSystemSetRotationSpeed` | `entity, speed` | — |
| `BlazeBolt.ParticleSystemSetActive` | `entity, bool` | — |
| `BlazeBolt.ParticleSystemIsActive` | `entity` | `bool` |
| `BlazeBolt.ParticleSystemSetVisible` | `entity, bool` | — |
| `BlazeBolt.ParticleSystemIsVisible` | `entity` | `bool` |
| `BlazeBolt.ParticleSystemEmit` | `entity, count` | — |
| `BlazeBolt.ParticleSystemClear` | `entity` | — |
| `BlazeBolt.ParticleSystemGetCount` | `entity` | `count` |

### Окно (14)
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

### Ввод (9)
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

### Звук (7)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.LoadSound` | `file, name, loop` | `id` |
| `BlazeBolt.PlaySound` | `name` | — |
| `BlazeBolt.PlaySoundById` | `id` | — |
| `BlazeBolt.StopSound` | `name` | — |
| `BlazeBolt.StopAllSounds` | — | — |
| `BlazeBolt.SetSoundVolume` | `name, vol` | — |
| `BlazeBolt.IsSoundPlaying` | `name` | `bool` |

### Управление объектами (2)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.Destroy` | `entity` | — |
| `BlazeBolt.DestroyAll` | — | — |

### Физика (37)
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
| `BlazeBolt.PhysicsSetAngle` | `bodyEntity, angle (radians)` | — |
| `BlazeBolt.PhysicsGetAngle` | `bodyEntity` | `angle (radians)` |
| `BlazeBolt.PhysicsSetGravityScale` | `bodyEntity, scale` | — |
| `BlazeBolt.PhysicsGetGravityScale` | `bodyEntity` | `scale` |
| `BlazeBolt.PhysicsSetActive` | `bodyEntity, bool` | — |
| `BlazeBolt.PhysicsIsActive` | `bodyEntity` | `bool` |
| `BlazeBolt.PhysicsSetFixedRotation` | `bodyEntity, bool` | — |
| `BlazeBolt.PhysicsIsFixedRotation` | `bodyEntity` | `bool` |
| `BlazeBolt.PhysicsSetBullet` | `bodyEntity, bool` | — |
| `BlazeBolt.PhysicsIsBullet` | `bodyEntity` | `bool` |
| `BlazeBolt.PhysicsDestroyBody` | `bodyEntity` | — |
| `BlazeBolt.PhysicsGetMass` | `bodyEntity` | `mass` |
| `BlazeBolt.PhysicsSetFriction` | `bodyEntity, friction` | — |
| `BlazeBolt.PhysicsGetFriction` | `bodyEntity` | `friction` |
| `BlazeBolt.PhysicsSetRestitution` | `bodyEntity, restitution` | — |
| `BlazeBolt.PhysicsGetRestitution` | `bodyEntity` | `restitution` |
| `BlazeBolt.PhysicsStep` | — | — |
| `BlazeBolt.PhysicsSyncSprite` | `bodyEntity, spriteEntity` | — |
| `BlazeBolt.PhysicsSyncText` | `bodyEntity, textEntity` | — |
| `BlazeBolt.PhysicsSyncAnimatedSprite` | `bodyEntity, animEntity` | — |

### Шейдеры (10)
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

### Утилиты (8)
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

### Скрипты и сцены (9)
| Функция | Параметры | Возврат |
|---|---|---|
| `BlazeBolt.LoadScript` | `path` | `bool` |
| `BlazeBolt.LoadScriptsFromList` | `path` | `bool` |
| `BlazeBolt.ReloadScript` | `path` | `bool` |
| `BlazeBolt.ReloadAllScripts` | — | `bool` |
| `BlazeBolt.EnableScript` | `name, bool` | — |
| `BlazeBolt.IsScriptLoaded` | `name` | `bool` |
| `BlazeBolt.GetLoadedScripts` | — | `table` |
| `BlazeBolt.LoadScene` | `sceneName` | — |
| `BlazeBolt.GetCurrentScene` | — | `name` |

### Глобальные функции (2)
| Функция | Параметры | Возврат |
|---|---|---|
| `GetScreenWidth` | — | `width` |
| `GetScreenHeight` | — | `height` |

### Константы

**TextAlignment**
```lua
TextAlignment.LEFT    = 0
TextAlignment.CENTER  = 1
TextAlignment.RIGHT   = 2
```

**Keys**
```lua
Keys.A .. Keys.Z                    -- Буквы A-Z
Keys._0 .. Keys._9                  -- Цифры 0-9
Keys.F1 .. Keys.F25                 -- Функциональные клавиши
Keys.UP, Keys.DOWN                  -- Стрелки
Keys.LEFT, Keys.RIGHT
Keys.SPACE, Keys.ENTER              -- Специальные
Keys.ESCAPE, Keys.TAB
Keys.BACKSPACE, Keys.DELETE
Keys.INSERT, Keys.HOME, Keys.END
Keys.PAGE_UP, Keys.PAGE_DOWN
Keys.CAPS_LOCK, Keys.SCROLL_LOCK    -- Блокировка
Keys.NUM_LOCK, Keys.PRINT_SCREEN
Keys.PAUSE
Keys.LEFT_SHIFT, Keys.RIGHT_SHIFT   -- Модификаторы
Keys.LEFT_CONTROL, Keys.RIGHT_CONTROL
Keys.LEFT_ALT, Keys.RIGHT_ALT
Keys.LEFT_SUPER, Keys.RIGHT_SUPER
Keys.KP_0 .. Keys.KP_9              -- Цифровая клавиатура
Keys.KP_DECIMAL, Keys.KP_DIVIDE
Keys.KP_MULTIPLY, Keys.KP_SUBTRACT
Keys.KP_ADD, Keys.KP_ENTER
Keys.KP_EQUAL
Keys.MENU
Keys.LAST
```

**MouseButtons**
```lua
MouseButtons.LEFT    = 0
MouseButtons.RIGHT   = 1
MouseButtons.MIDDLE  = 2
```

---

BlazeBolt Game Engine 1.0
