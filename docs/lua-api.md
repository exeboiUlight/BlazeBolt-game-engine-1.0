# BlazeBolt Engine — Lua API Reference

> **Version:** 1.3  
> **Lua version:** 5.4  
> **Engine:** BlazeBolt Game Engine

---

## Table of Contents

1. [Обзор](#обзор)
2. [Жизненный цикл (Callbacks)](#жизненный-цикл-callbacks)
3. [Спрайты (Sprites)](#спрайты-sprites)
4. [Спрайт-батчи (SpriteBatch)](#спрайт-батчи-spritebatch)
5. [Анимированные спрайты (AnimatedSprite)](#анимированные-спрайты-animatedsprite)
6. [Колесо состояний анимаций (AnimationWheel)](#колесо-состояний-анимаций-animationwheel)
7. [Текст (Text)](#текст-text)
8. [Меши (Mesh)](#меши-mesh)
9. [Камера (Camera)](#камера-camera)
10. [Системы частиц (ParticleSystem)](#системы-частиц-particlesystem)
11. [Тайлсеты (Tileset)](#тайлсеты-tileset)
12. [Освещение (Light)](#освещение-light)
13. [Физика (Physics)](#физика-physics)
14. [Аудио (Audio)](#аудио-audio)
15. [Окна (Window)](#окна-window)
16. [Скрипты (Script Management)](#скрипты-script-management)
17. [Сцены (Scene Management)](#сцены-scene-management)
18. [Формат .scene файлов (Scene File Format)](#формат-scene-файлов-scene-file-format)
19. [Ввод (Input)](#ввод-input)
20. [Математические типы (Math Types)](#математические-типы-math-types)
21. [Шумы (Noise)](#шумы-noise)
22. [Утилиты (Utility)](#утилиты-utility)
23. [Сетевые функции (Networking)](#сетевые-функции-networking)
24. [Порядок рендера (Render Order)](#порядок-рендера-render-order)
25. [Константы (Constants)](#константы-constants)
26. [Управление графическим API (Graphics API)](#управление-графическим-api-graphics-api)
27. [OOP-стиль (Object-Oriented Wrappers)](#oop-стиль-object-oriented-wrappers)

---

## Обзор

**BlazeBolt Engine** — это лёгкий 2D-игровой движок с Lua 5.4 скриптингом. Он поддерживает **OpenGL 3.3** и **Vulkan** для рендеринга (через RHI-прослойку), Box2D для физики, OpenAL для звука и поддерживает сетевые функции (TCP/UDP) и процедурную генерацию шумов.

### Как устроен API

Все функции движка вызываются через глобальную таблицу `BlazeBolt`. Типичный вызов выглядит так:

```lua
BlazeBolt.ИмяФункции(аргументы)
```

Пример — создаём спрайт и двигаем его:

```lua
local sprite = BlazeBolt.CreateSprite("texture.png", 100, 200)
BlazeBolt.SpriteSetPosition(sprite, 300, 400)
```

Некоторые вспомогательные функции (получение размеров экрана, шумы) доступны как глобальные — без таблицы `BlazeBolt`:

```lua
local w = GetScreenWidth()
local h = GetScreenHeight()
local n = PerlinNoise2D(x, y)
```

### Тип Entity

Почти все функции создания объектов (спрайты, камеры, физические тела и т.д.) возвращают **`Entity`** — это целочисленный ID (integer). Вы сохраняете его в переменную и потом передаёте обратно в функции управления этим объектом. Например:

```lua
local player = BlazeBolt.CreateSprite("hero.png", 100, 100)  -- создаём, получаем ID
BlazeBolt.SpriteSetPosition(player, 200, 300)                -- используем ID
BlazeBolt.SpriteSetRotation(player, 45)                      -- тот же ID
```

### OOP-стиль (рекомендуемый)

Начиная с версии 1.3, движок предоставляет **OOP-обёртки** для всех типов. Вместо передачи Entity ID через функцию, вы работаете с таблицами-объектами и вызываете методы через `:`:

```lua
local player = BlazeBolt.Sprite.new("hero.png", 100, 100)
player:SetPosition(200, 300)
player:SetRotation(45)
```

Подробнее — в разделе [OOP-стиль](#oop-стиль-object-oriented-wrappers).

### Жизненный цикл

Движок сам вызывает ваши Lua-функции на определённых этапах работы. Вы просто определяете нужные функции в скрипте:

| Callback | Когда вызывается | Что тут делают |
|---|---|---|
| `Start()` | Один раз при запуске игры | Инициализация: загрузка текстур, создание объектов, настройка переменных |
| `Update(dt)` | Каждый кадр (обычно 60 раз/сек) | Логика игры: движение, проверка ввода, обновление состояний. `dt` — время в секундах с прошлого кадра |
| `Draw()` | Каждый кадр для отрисовки | Кастомная отрисовка: вызов `SpriteBatchDraw`, `MeshDraw`, `TilesetDraw` и т.д. |
| `End()` | При закрытии игры | Очистка: освобождение ресурсов, отключение сети |
| `On<SceneName>Load()` | При загрузке сцены | Логика, которая должна выполниться при входе в сцену |
| `On<SceneName>Unload()` | При выгрузке сцены | Логика, которая должна выполниться при выходе из сцены |

**Важно:** большинство объектов (спрайты, текст, камеры) отрисовываются автоматически. Функция `Draw()` нужна только если вы используете кастомные шейдеры, меши или спрайт-батчи.

### Типы данных

| Тип | Что это |
|---|---|
| `number` | Число с плавающей точкой (float) |
| `integer` | Целое число |
| `string` | Строка |
| `boolean` | `true` / `false` |
| `nil` | Пустота (отсутствие значения) |
| `table` | Таблица (массив или словарь) |
| `Entity` | Целочисленный ID объекта (integer) |
| `Vector2` | 2D-вектор (x, y) |
| `Vector3` | 3D-вектор (x, y, z) |
| `Vector4` | 4D-вектор (x, y, z, w) |
| `Matrix3x3` | Матрица 3x3 для трансформаций |

---

## Жизненный цикл (Callbacks)

Движок автоматически вызывает определённые Lua-функции на разных этапах работы:

| Callback | Описание |
|---|---|
| `Start()` | Вызывается один раз при запуске |
| `Update(dt)` | Вызывается каждый кадр. `dt` — время с прошлого кадра (секунды) |
| `Draw()` | Вызывается каждый кадр для отрисовки |
| `End()` | Вызывается при завершении работы |
| `On<SceneName>Load()` | Вызывается при загрузке сцены `<SceneName>` |
| `On<SceneName>Unload()` | Вызывается при выгрузке сцены `<SceneName>` |

Пример:

```lua
local player

function Start()
    player = BlazeBolt.CreateSprite("player.png", 100, 100)
end

function Update(dt)
    local x, y = BlazeBolt.SpriteGetPosition(player)
    BlazeBolt.SpriteSetPosition(player, x + 100 * dt, y)
end

function Draw()
    -- отрисовка происходит автоматически
end
```

---

## Спрайты (Sprites)

Спрайт — это основной графический объект в игре: персонаж, фон, препятствие, UI-элемент. Спрайт загружает текстуру из файла и отображает её на экране. Каждому спрайту можно менять позицию, размер, поворот, цвет (тинт) и видимость.

Спрайты отрисовываются **автоматически** — вам не нужно вызывать никаких функций отрисовки в `Draw()`, если вы не используете батчи или кастомные шейдеры.

### CreateSprite

Создаёт новый спрайт с загруженной текстурой. Это основной способ создать графический объект в игре.

```
BlazeBolt.CreateSprite(texturePath, [x], [y]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `texturePath` | string | Путь к файлу текстуры | — |
| `x` | number | Начальная позиция X | `0` |
| `y` | number | Начальная позиция Y | `0` |

**Возвращает:** `entity` (integer) — идентификатор спрайта.

---

### SpriteSetTexture

Заменяет текстуру спрайта на лету. Полезно когда нужно сменить картинку без создания нового спрайта (например, при смене оружия или состояния персонажа).

```
BlazeBolt.SpriteSetTexture(entity, texturePath)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `texturePath` | string | Путь к новой текстуре |

**Возвращает:** ничего.

---

### SpriteSetPosition

Задаёт позицию спрайта на экране. Координаты — в пикселях, начало координат (0,0) — верхний левый угол окна.

```
BlazeBolt.SpriteSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### SpriteGetPosition

Возвращает текущую позицию спрайта.

```
BlazeBolt.SpriteGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |

**Возвращает:** `x` (number), `y` (number).

---

### SpriteSetTextureRect

Устанавливает область текстуры из спрайтшита (атласа). Позволяет отображать только часть изображения — например, один кадр из анимации или иконку из набора.

```
BlazeBolt.SpriteSetTextureRect(entity, u, v, w, h)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `u` | number | X начала области |
| `v` | number | Y начала области |
| `w` | number | Ширина области |
| `h` | number | Высота области |

**Возвращает:** ничего.

---

### SpriteSetSize

Устанавливает размер спрайта.

```
BlazeBolt.SpriteSetSize(entity, width, height)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `width` | number | Ширина |
| `height` | number | Высота |

**Возвращает:** ничего.

---

### SpriteGetSize

Возвращает размер спрайта.

```
BlazeBolt.SpriteGetSize(entity) → width, height
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |

**Возвращает:** `width` (number), `height` (number).

---

### SpriteSetOrigin

Задаёт точку origin — центр вращения и масштабирования спрайта. Координаты от 0 до 1, где (0,0) — верхний левый угол, (0.5, 0.5) — центр, (1,1) — нижний правый. По умолчанию origin — (0,0), поэтому спрайт вращается вокруг своего верхнего левого угла.

```
BlazeBolt.SpriteSetOrigin(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `x` | number | Origin X |
| `y` | number | Origin Y |

**Возвращает:** ничего.

---

### SpriteGetOrigin

Возвращает точку origin спрайта.

```
BlazeBolt.SpriteGetOrigin(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |

**Возвращает:** `x` (number), `y` (number).

---

### SpriteSetRotation

Задаёт угол поворота спрайта в градусах. Положительные значения — по часовой стрелке. Вращение происходит вокруг точки origin.

```
BlazeBolt.SpriteSetRotation(entity, rotation)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `rotation` | number | Угол поворота в градусах |

**Возвращает:** ничего.

---

### SpriteGetRotation

Возвращает угол поворота спрайта.

```
BlazeBolt.SpriteGetRotation(entity) → rotation
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |

**Возвращает:** `rotation` (number).

---

### SpriteSetColor

Задаёт цвет спрайта (тинт). Значения компонентов от 0 до 1. Белый (1,1,1) — исходный цвет текстуры. Красный (1,0,0) — спрайт окрасится в красный. Чёрный (0,0,0) — спрайт станет чёрным. Альфа-канал управляет прозрачностью.

```
BlazeBolt.SpriteSetColor(entity, r, g, b, [a])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `entity` | integer | Идентификатор спрайта | — |
| `r` | number | Красный компонент (0–1) | — |
| `g` | number | Зелёный компонент (0–1) | — |
| `b` | number | Синий компонент (0–1) | — |
| `a` | number | Альфа-канал (0–1) | `1.0` |

**Возвращает:** ничего.

---

### SpriteGetColor

Возвращает цвет спрайта.

```
BlazeBolt.SpriteGetColor(entity) → r, g, b, a
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |

**Возвращает:** `r` (number), `g` (number), `b` (number), `a` (number).

---

### SpriteSetVisible

Устанавливает видимость спрайта.

```
BlazeBolt.SpriteSetVisible(entity, visible)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |
| `visible` | boolean | Видимость |

**Возвращает:** ничего.

---

### SpriteIsVisible

Проверяет, видим ли спрайт.

```
BlazeBolt.SpriteIsVisible(entity) → visible
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор спрайта |

**Возвращает:** `visible` (boolean).

---

### Пример: Спрайты

```lua
local player

function Start()
    player = BlazeBolt.CreateSprite("assets/player.png", 100, 200)
    BlazeBolt.SpriteSetOrigin(player, 0.5, 0.5)
    BlazeBolt.SpriteSetSize(player, 64, 64)
    BlazeBolt.SpriteSetRotation(player, 45)
    BlazeBolt.SpriteSetColor(player, 1.0, 1.0, 1.0, 0.8)
end

function Update(dt)
    local x, y = BlazeBolt.SpriteGetPosition(player)
    if BlazeBolt.IsKeyPressed(Keys.RIGHT) then
        x = x + 200 * dt
    end
    if BlazeBolt.IsKeyPressed(Keys.LEFT) then
        x = x - 200 * dt
    end
    BlazeBolt.SpriteSetPosition(player, x, y)

    local r = BlazeBolt.SpriteGetRotation(player)
    BlazeBolt.SpriteSetRotation(player, r + 90 * dt)
end

function Draw()
    if BlazeBolt.IsKeyPressed(Keys.H) then
        BlazeBolt.SpriteSetVisible(player, false)
    end
end
```

---

## Спрайт-батчи (SpriteBatch)

Спрайт-батч — это способ отрисовать много спрайтов с **одной текстурой** за один вызов. Вместо того чтобы рисовать каждый спрайт отдельно (что медленно из-за переключений текстур), батч собирает их все и отправляет на GPU пачкой. Это критически важно для производительности когда у вас hundreds or thousands of одинаковых объектов (трава, пули, монеты, тайлы).

**Как использовать:** создайте батч, назначьте ему текстуру, добавьте спрайты, и вызывайте отрисовку в `Draw()`. Не забудьте вызвать `DestroySpriteBatch` при завершении, если батч больше не нужен.

### CreateSpriteBatch

Создаёт новый спрайт-батч.

```
BlazeBolt.CreateSpriteBatch([maxSize]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `maxSize` | integer | Максимальное количество спрайтов | `25` |

**Возвращает:** `entity` (integer) — идентификатор батча.

---

### SpriteBatchSetTexture

Устанавливает текстуру для батча.

```
BlazeBolt.SpriteBatchSetTexture(batchEntity, texturePath)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |
| `texturePath` | string | Путь к текстуре |

**Возвращает:** ничего.

---

### SpriteBatchAdd

Добавляет спрайт в батч.

```
BlazeBolt.SpriteBatchAdd(batchEntity, spriteEntity) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |
| `spriteEntity` | integer | Идентификатор спрайта |

**Возвращает:** `success` (boolean).

---

### SpriteBatchRemove

Удаляет спрайт из батча.

```
BlazeBolt.SpriteBatchRemove(batchEntity, spriteEntity) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |
| `spriteEntity` | integer | Идентификатор спрайта |

**Возвращает:** `success` (boolean).

---

### SpriteBatchClear

Очищает батч (удаляет все спрайты).

```
BlazeBolt.SpriteBatchClear(batchEntity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |

**Возвращает:** ничего.

---

### SpriteBatchSetMaxSize

Устанавливает максимальный размер батча.

```
BlazeBolt.SpriteBatchSetMaxSize(batchEntity, maxSize)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |
| `maxSize` | integer | Новый максимальный размер |

**Возвращает:** ничего.

---

### SpriteBatchGetMaxSize

Возвращает максимальный размер батча.

```
BlazeBolt.SpriteBatchGetMaxSize(batchEntity) → maxSize
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |

**Возвращает:** `maxSize` (integer).

---

### SpriteBatchGetCount

Возвращает количество спрайтов в батче.

```
BlazeBolt.SpriteBatchGetCount(batchEntity) → count
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |

**Возвращает:** `count` (integer).

---

### SpriteBatchDraw

Отрисовывает батч.

```
BlazeBolt.SpriteBatchDraw(batchEntity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |

**Возвращает:** ничего.

---

### DestroySpriteBatch

Удаляет батч.

```
BlazeBolt.DestroySpriteBatch(batchEntity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `batchEntity` | integer | Идентификатор батча |

**Возвращает:** ничего.

---

### Пример: Спрайт-батчи

```lua
local batch

function Start()
    batch = BlazeBolt.CreateSpriteBatch(100)
    BlazeBolt.SpriteBatchSetTexture(batch, "assets/grass.png")

    for i = 0, 99 do
        local s = BlazeBolt.CreateSprite("assets/grass.png", (i % 10) * 32, math.floor(i / 10) * 32)
        BlazeBolt.SpriteBatchAdd(batch, s)
    end
end

function Draw()
    BlazeBolt.SpriteBatchDraw(batch)
end
```

---

## Анимированные спрайты (AnimatedSprite)

Анимированный спрайт — это спрайт, который последовательно проигрывает кадры из JSON-файла анимации (созданного в внешнем редакторе). Используется для персонажей, эффектов, интерфейсов — всего, что должно двигаться/меняться.

**Пример сценария:** у вас есть JSON-файл `player.json`, который описывает анимацию ходьбы. Вы создаёте анимированный спрайт, запускаете анимацию, и движок сам переключает кадры с заданной скоростью.

### CreateAnimatedSprite

Создаёт анимированный спрайт из JSON-файла анимации. Файл содержит описание кадров, таймингов и пути к текстурам.

```
BlazeBolt.CreateAnimatedSprite(path, [x], [y]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `path` | string | Путь к JSON-файлу анимации | — |
| `x` | number | Начальная позиция X | `0` |
| `y` | number | Начальная позиция Y | `0` |

**Возвращает:** `entity` (integer) — идентификатор анимированного спрайта.

---

### AnimatedSpritePlay

Запускает воспроизведение анимации.

```
BlazeBolt.AnimatedSpritePlay(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** ничего.

---

### AnimatedSpriteIsPlaying

Проверяет, воспроизводится ли анимация.

```
BlazeBolt.AnimatedSpriteIsPlaying(entity) → playing
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `playing` (boolean).

---

### AnimatedSpritePause

Приостанавливает анимацию.

```
BlazeBolt.AnimatedSpritePause(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** ничего.

---

### AnimatedSpriteStop

Останавливает анимацию.

```
BlazeBolt.AnimatedSpriteStop(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** ничего.

---

### AnimatedSpriteRestart

Перезапускает анимацию с начала.

```
BlazeBolt.AnimatedSpriteRestart(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** ничего.

---

### AnimatedSpriteSetTexture

Заменяет текстуру анимированного спрайта.

```
BlazeBolt.AnimatedSpriteSetTexture(entity, path)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `path` | string | Путь к новой текстуре |

**Возвращает:** ничего.

---

### AnimatedSpriteSetLooping

Включает/выключает зацикливание анимации.

```
BlazeBolt.AnimatedSpriteSetLooping(entity, looping)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `looping` | boolean | Зацикливание |

**Возвращает:** ничего.

---

### AnimatedSpriteIsLooping

Проверяет, зациклена ли анимация.

```
BlazeBolt.AnimatedSpriteIsLooping(entity) → looping
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `looping` (boolean).

---

### AnimatedSpriteSetPlaybackSpeed

Устанавливает скорость воспроизведения.

```
BlazeBolt.AnimatedSpriteSetPlaybackSpeed(entity, speed)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `speed` | number | Скорость (1.0 = нормальная) |

**Возвращает:** ничего.

---

### AnimatedSpriteGetPlaybackSpeed

Возвращает скорость воспроизведения.

```
BlazeBolt.AnimatedSpriteGetPlaybackSpeed(entity) → speed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `speed` (number).

---

### AnimatedSpriteSetFrame

Устанавливает текущий кадр анимации.

```
BlazeBolt.AnimatedSpriteSetFrame(entity, frame)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `frame` | integer | Номер кадра (начиная с 0) |

**Возвращает:** ничего.

---

### AnimatedSpriteGetCurrentFrame

Возвращает номер текущего кадра.

```
BlazeBolt.AnimatedSpriteGetCurrentFrame(entity) → frame
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `frame` (integer).

---

### AnimatedSpriteGetNumFrames

Возвращает общее количество кадров.

```
BlazeBolt.AnimatedSpriteGetNumFrames(entity) → count
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `count` (integer).

---

### AnimatedSpriteSetPosition

Устанавливает позицию анимированного спрайта.

```
BlazeBolt.AnimatedSpriteSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### AnimatedSpriteGetPosition

Возвращает позицию анимированного спрайта.

```
BlazeBolt.AnimatedSpriteGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `x` (number), `y` (number).

---

### AnimatedSpriteSetSize

Устанавливает размер анимированного спрайта.

```
BlazeBolt.AnimatedSpriteSetSize(entity, width, height)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `width` | number | Ширина |
| `height` | number | Высота |

**Возвращает:** ничего.

---

### AnimatedSpriteGetSize

Возвращает размер анимированного спрайта.

```
BlazeBolt.AnimatedSpriteGetSize(entity) → width, height
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `width` (number), `height` (number).

---

### AnimatedSpriteSetOrigin

Устанавливает точку origin.

```
BlazeBolt.AnimatedSpriteSetOrigin(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `x` | number | Origin X |
| `y` | number | Origin Y |

**Возвращает:** ничего.

---

### AnimatedSpriteGetOrigin

Возвращает точку origin.

```
BlazeBolt.AnimatedSpriteGetOrigin(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `x` (number), `y` (number).

---

### AnimatedSpriteSetRotation

Устанавливает угол поворота (градусы).

```
BlazeBolt.AnimatedSpriteSetRotation(entity, rotation)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |
| `rotation` | number | Угол поворота |

**Возвращает:** ничего.

---

### AnimatedSpriteGetRotation

Возвращает угол поворота.

```
BlazeBolt.AnimatedSpriteGetRotation(entity) → rotation
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `rotation` (number).

---

### AnimatedSpriteSetColor

Устанавливает цвет (тинт).

```
BlazeBolt.AnimatedSpriteSetColor(entity, r, g, b, [a])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта | — |
| `r` | number | Красный (0–1) | — |
| `g` | number | Зелёный (0–1) | — |
| `b` | number | Синий (0–1) | — |
| `a` | number | Альфа (0–1) | `1.0` |

**Возвращает:** ничего.

---

### AnimatedSpriteGetColor

Возвращает цвет анимированного спрайта.

```
BlazeBolt.AnimatedSpriteGetColor(entity) → r, g, b, a
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `r` (number), `g` (number), `b` (number), `a` (number).

---

### Пример: Анимированные спрайты

```lua
local anim

function Start()
    anim = BlazeBolt.CreateAnimatedSprite("assets/player.json", 400, 300)
    BlazeBolt.AnimatedSpriteSetLooping(anim, true)
    BlazeBolt.AnimatedSpriteSetPlaybackSpeed(anim, 1.0)
    BlazeBolt.AnimatedSpritePlay(anim)
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        if BlazeBolt.AnimatedSpriteIsPlaying(anim) then
            BlazeBolt.AnimatedSpritePause(anim)
        else
            BlazeBolt.AnimatedSpritePlay(anim)
        end
    end

    if BlazeBolt.IsKeyJustPressed(Keys.R) then
        BlazeBolt.AnimatedSpriteRestart(anim)
    end

    BlazeBolt.AnimatedSpriteSetPosition(anim,
        BlazeBolt.AnimatedSpriteGetPosition(anim) + 100 * dt, 300)
end
```

---

## Колесо состояний анимаций (AnimationWheel)

Колесо состояний (AnimationWheel) — это менеджер анимаций для одного персонажа или объекта. Вместо того чтобы вручную переключать текстуры и настройки, вы просто определяете состояния ("стоит", "бег", "прыжок", "атака"), а колесо само загружает нужные GIF-файлы и применяет параметры.

**Зачем нужно:** без колеса вам пришлось бы писать кучу if-else для каждой анимации. С колесом — просто вызываете `SetState("run")`, и всё работает.

**Пример:** персонаж может стоять (idle), бежать (run), прыгать (jump) и атаковать (attack). Каждое состояние — это GIF-файл со своими настройками скорости и зацикливания.

### CreateAnimationWheel

Создаёт колесо состояний, привязанное к анимированному спрайту.

```
BlazeBolt.CreateAnimationWheel(animatedSpriteEntity) → entity
```

| Аргумент | Тип | Описание |
|---|---|---|
| `animatedSpriteEntity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** `entity` (integer) — идентификатор колеса состояний.

---

### AnimationWheelAddState

Добавляет состояние в колесо. Каждое состояние — это набор: имя, путь к GIF, скорость воспроизведения и зацикливание.

```
BlazeBolt.AnimationWheelAddState(wheel, name, gifPath, [speed], [looping])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `wheel` | integer | Идентификатор колеса | — |
| `name` | string | Уникальное имя состояния | — |
| `gifPath` | string | Путь к GIF-файлу анимации | — |
| `speed` | number | Скорость воспроизведения | `1.0` |
| `looping` | boolean | Зацикливание | `true` |

**Возвращает:** ничего.

---

### AnimationWheelRemoveState

Удаляет состояние из колеса.

```
BlazeBolt.AnimationWheelRemoveState(wheel, name)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `name` | string | Имя состояния |

**Возвращает:** ничего.

---

### AnimationWheelHasState

Проверяет, существует ли состояние в колесе.

```
BlazeBolt.AnimationWheelHasState(wheel, name) → exists
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `name` | string | Имя состояния |

**Возвращает:** `exists` (boolean).

---

### AnimationWheelSetInitialState

Устанавливает начальное состояние (применяется при первом вызове `SetState` или автоматически).

```
BlazeBolt.AnimationWheelSetInitialState(wheel, name)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `name` | string | Имя начального состояния |

**Возвращает:** ничего.

---

### AnimationWheelGetInitialState

Возвращает имя начального состояния.

```
BlazeBolt.AnimationWheelGetInitialState(wheel) → name
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |

**Возвращает:** `name` (string).

---

### AnimationWheelSetState

Переключает текущее состояние. Автоматически загружает GIF, устанавливает скорость и зацикливание, запускает анимацию с начала.

```
BlazeBolt.AnimationWheelSetState(wheel, name)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `name` | string | Имя нового состояния |

**Возвращает:** ничего.

---

### AnimationWheelGetState

Возвращает имя текущего состояния.

```
BlazeBolt.AnimationWheelGetState(wheel) → name
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |

**Возвращает:** `name` (string).

---

### AnimationWheelSetPlaybackSpeed

Устанавливает скорость воспроизведения для указанного состояния.

```
BlazeBolt.AnimationWheelSetPlaybackSpeed(wheel, stateName, speed)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `stateName` | string | Имя состояния |
| `speed` | number | Скорость (1.0 = нормальная) |

**Возвращает:** ничего.

---

### AnimationWheelGetPlaybackSpeed

Возвращает скорость воспроизведения состояния.

```
BlazeBolt.AnimationWheelGetPlaybackSpeed(wheel, stateName) → speed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `stateName` | string | Имя состояния |

**Возвращает:** `speed` (number).

---

### AnimationWheelSetLooping

Устанавливает зацикливание для состояния.

```
BlazeBolt.AnimationWheelSetLooping(wheel, stateName, looping)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `stateName` | string | Имя состояния |
| `looping` | boolean | Зацикливание |

**Возвращает:** ничего.

---

### AnimationWheelIsLooping

Проверяет, зациклено ли состояние.

```
BlazeBolt.AnimationWheelIsLooping(wheel, stateName) → looping
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `stateName` | string | Имя состояния |

**Возвращает:** `looping` (boolean).

---

### AnimationWheelSetGifPath

Заменяет GIF-файл для состояния (позволяет менять анимацию на лету).

```
BlazeBolt.AnimationWheelSetGifPath(wheel, stateName, gifPath)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `stateName` | string | Имя состояния |
| `gifPath` | string | Новый путь к GIF-файлу |

**Возвращает:** ничего.

---

### AnimationWheelGetGifPath

Возвращает путь к GIF-файлу состояния.

```
BlazeBolt.AnimationWheelGetGifPath(wheel, stateName) → gifPath
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `stateName` | string | Имя состояния |

**Возвращает:** `gifPath` (string).

---

### AnimationWheelSetAutoAdvance

Включает/выключает автоматическое переключение состояний. При включении: когда анимация без зацикливания заканчивается, колесо автоматически переключается на `initialState`.

```
BlazeBolt.AnimationWheelSetAutoAdvance(wheel, autoAdvance)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |
| `autoAdvance` | boolean | Автопереход |

**Возвращает:** ничего.

---

### AnimationWheelGetAutoAdvance

Проверяет, включён ли автопереход.

```
BlazeBolt.AnimationWheelGetAutoAdvance(wheel) → autoAdvance
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |

**Возвращает:** `autoAdvance` (boolean).

---

### AnimationWheelGetStateNames

Возвращает список имён всех состояний в колесе.

```
BlazeBolt.AnimationWheelGetStateNames(wheel) → names
```

| Аргумент | Тип | Описание |
|---|---|---|
| `wheel` | integer | Идентификатор колеса |

**Возвращает:** `names` (table) — массив строк с именами состояний.

---

### Пример: Колесо состояний персонажа

```lua
local sprite
local wheel

function Start()
    -- Создаём анимированный спрайт
    sprite = BlazeBolt.CreateAnimatedSprite("idle.gif", 400, 300)

    -- Создаём колесо состояний
    wheel = BlazeBolt.CreateAnimationWheel(sprite)

    -- Добавляем состояния (имя, gif, скорость, зацикливание)
    BlazeBolt.AnimationWheelAddState(wheel, "idle",   "idle.gif",   1.0, true)
    BlazeBolt.AnimationWheelAddState(wheel, "run",    "run.gif",    1.5, true)
    BlazeBolt.AnimationWheelAddState(wheel, "jump",   "jump.gif",   1.0, false)
    BlazeBolt.AnimationWheelAddState(wheel, "attack", "attack.gif", 2.0, false)

    -- Начальное состояние
    BlazeBolt.AnimationWheelSetInitialState(wheel, "idle")
    BlazeBolt.AnimationWheelSetState(wheel, "idle")
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.ONE) then
        BlazeBolt.AnimationWheelSetState(wheel, "idle")
    elseif BlazeBolt.IsKeyJustPressed(Keys.TWO) then
        BlazeBolt.AnimationWheelSetState(wheel, "run")
    elseif BlazeBolt.IsKeyJustPressed(Keys.THREE) then
        BlazeBolt.AnimationWheelSetState(wheel, "jump")
    elseif BlazeBolt.IsKeyJustPressed(Keys.FOUR) then
        BlazeBolt.AnimationWheelSetState(wheel, "attack")
    end

    -- Замена GIF на лету
    if BlazeBolt.IsKeyJustPressed(Keys.R) then
        BlazeBolt.AnimationWheelSetGifPath(wheel, "idle", "new_idle.gif")
    end
end
```

---

## Текст (Text)

Текстовый объект позволяет отображать текст на экране с настраиваемым шрифтом, цветом, размером и выравниванием. Подходит для HUD (счёт, жизни, очки), диалогов, меню и любых текстовых элементов.

**Важно:** для отрисовки текста нужен файл шрифта `.ttf`. Шрифт загружается при создании объекта.

### CreateText

Создаёт текстовый объект с указанным шрифтом и начальным текстом. Текст можно изменить позже через `TextSetString`.

```
BlazeBolt.CreateText(fontPath, text, [x], [y]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `fontPath` | string | Путь к шрифту (.ttf) | — |
| `text` | string | Начальный текст | — |
| `x` | number | Позиция X | `0` |
| `y` | number | Позиция Y | `0` |

**Возвращает:** `entity` (integer).

---

### TextSetString

Устанавливает содержимое текста.

```
BlazeBolt.TextSetString(entity, text)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `text` | string | Новый текст |

**Возвращает:** ничего.

---

### TextGetString

Возвращает содержимое текста.

```
BlazeBolt.TextGetString(entity) → text
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `text` (string).

---

### TextSetPosition

Устанавливает позицию текста.

```
BlazeBolt.TextSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### TextGetPosition

Возвращает позицию текста.

```
BlazeBolt.TextGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `x` (number), `y` (number).

---

### TextSetColor

Устанавливает цвет текста.

```
BlazeBolt.TextSetColor(entity, r, g, b, [a])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `entity` | integer | Идентификатор текста | — |
| `r` | number | Красный (0–1) | — |
| `g` | number | Зелёный (0–1) | — |
| `b` | number | Синий (0–1) | — |
| `a` | number | Альфа (0–1) | `1.0` |

**Возвращает:** ничего.

---

### TextGetColor

Возвращает цвет текста.

```
BlazeBolt.TextGetColor(entity) → r, g, b, a
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `r` (number), `g` (number), `b` (number), `a` (number).

---

### TextSetScale

Устанавливает масштаб текста.

```
BlazeBolt.TextSetScale(entity, width, height)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `width` | number | Масштаб по X |
| `height` | number | Масштаб по Y |

**Возвращает:** ничего.

---

### TextGetScale

Возвращает масштаб текста.

```
BlazeBolt.TextGetScale(entity) → width, height
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `width` (number), `height` (number).

---

### TextSetOrigin

Устанавливает точку origin.

```
BlazeBolt.TextSetOrigin(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `x` | number | Origin X |
| `y` | number | Origin Y |

**Возвращает:** ничего.

---

### TextGetOrigin

Возвращает точку origin.

```
BlazeBolt.TextGetOrigin(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `x` (number), `y` (number).

---

### TextSetRotation

Устанавливает угол поворота (градусы).

```
BlazeBolt.TextSetRotation(entity, rotation)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `rotation` | number | Угол поворота |

**Возвращает:** ничего.

---

### TextGetRotation

Возвращает угол поворота.

```
BlazeBolt.TextGetRotation(entity) → rotation
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `rotation` (number).

---

### TextSetAlignment

Устанавливает выравнивание текста.

```
BlazeBolt.TextSetAlignment(entity, alignment)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `alignment` | integer | Значение из `TextAlignment` (LEFT=0, CENTER=1, RIGHT=2) |

**Возвращает:** ничего.

---

### TextGetAlignment

Возвращает выравнивание текста.

```
BlazeBolt.TextGetAlignment(entity) → alignment
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `alignment` (integer).

---

### TextSetVisible

Устанавливает видимость текста.

```
BlazeBolt.TextSetVisible(entity, visible)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |
| `visible` | boolean | Видимость |

**Возвращает:** ничего.

---

### TextIsVisible

Проверяет видимость текста.

```
BlazeBolt.TextIsVisible(entity) → visible
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор текста |

**Возвращает:** `visible` (boolean).

---

### Пример: Текст

```lua
local label
local score = 0

function Start()
    label = BlazeBolt.CreateText("assets/font.ttf", "Score: 0", 10, 10)
    BlazeBolt.TextSetColor(label, 1.0, 1.0, 0.0, 1.0)
    BlazeBolt.TextSetScale(label, 1.5, 1.5)
    BlazeBolt.TextSetAlignment(label, TextAlignment.LEFT)
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        score = score + 1
        BlazeBolt.TextSetString(label, "Score: " .. tostring(score))
    end
end
```

---

## Меши (Mesh)

Меш — это объект с произвольной геометрией (вершины + треугольники), который можно отрисовать с кастомным шейдером. Используется для:
- **Пост-эффектов** (размытие, виньетирование, изменение цвета всего экрана)
- **Пользовательских фигур** (которых нет среди стандартных спрайтов)
- **Визуальных шейдеров** (вода, огонь, генеративные эффекты)

**Как работает:** вы создаёте меш, задаёте ему вершины (точки формы) и индексы (как точки соединяются в треугольники), назначаете шейдер, и вызываете отрисовку в `Draw()`.

### CreateMesh

Создаёт пустой 2D меш. После создания нужно задать данные через `MeshSetData` и опционально назначить шейдер через `MeshSetShader`.

```
BlazeBolt.CreateMesh() → entity
```

**Возвращает:** `entity` (integer).

---

### MeshSetData

Устанавливает данные меша (вершины и индексы).

```
BlazeBolt.MeshSetData(entity, vertices, indices)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `vertices` | table | Массив вершин: `{ {x=0, y=0, u=0, v=0}, ... }` |
| `indices` | table | Массив индексов: `{ 0, 1, 2, ... }` |

Каждая вершина — таблица с полями `x`, `y` (обязательные) и `u`, `v` (необязательные, по умолчанию 0).

**Возвращает:** ничего.

---

### MeshSetShader

Назначает кастомный шейдер мешу. Позволяет использовать собственные вершинные и фрагментные шейдеры для рендеринга меша.

```
BlazeBolt.MeshSetShader(entity, vertexPath, fragmentPath)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `vertexPath` | string | Путь к вершинному шейдеру (.vert) |
| `fragmentPath` | string | Путь к фрагментному шейдеру (.frag) |

**Возвращает:** ничего.

---

### MeshSetUniformFloat

Устанавливает uniform float значение для шейдера меша.

```
BlazeBolt.MeshSetUniformFloat(entity, name, value)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `name` | string | Имя uniform-переменной |
| `value` | number | Значение |

**Возвращает:** ничего.

---

### MeshSetUniformInt

Устанавливает uniform int значение для шейдера меша.

```
BlazeBolt.MeshSetUniformInt(entity, name, value)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `name` | string | Имя uniform-переменной |
| `value` | integer | Значение |

**Возвращает:** ничего.

---

### MeshSetUniformVec2

Устанавливает uniform vec2 значение для шейдера меша.

```
BlazeBolt.MeshSetUniformVec2(entity, name, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `name` | string | Имя uniform-переменной |
| `x` | number | Компонента X |
| `y` | number | Компонента Y |

**Возвращает:** ничего.

---

### MeshSetUniformVec3

Устанавливает uniform vec3 значение для шейдера меша.

```
BlazeBolt.MeshSetUniformVec3(entity, name, x, y, z)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `name` | string | Имя uniform-переменной |
| `x` | number | Компонента X |
| `y` | number | Компонента Y |
| `z` | number | Компонента Z |

**Возвращает:** ничего.

---

### MeshSetUniformVec4

Устанавливает uniform vec4 значение для шейдера меша.

```
BlazeBolt.MeshSetUniformVec4(entity, name, x, y, z, w)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |
| `name` | string | Имя uniform-переменной |
| `x` | number | Компонента X |
| `y` | number | Компонента Y |
| `z` | number | Компонента Z |
| `w` | number | Компонента W |

**Возвращает:** ничего.

---

### MeshDraw

Отрисовывает меш (с кастомным шейдером, если назначен).

```
BlazeBolt.MeshDraw(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор меша |

**Возвращает:** ничего.

---

### Пример использования меша с шейдером

```lua
function Start()
    -- Создаём меш на весь экран (quad)
    local screenW = GetScreenWidth()
    local screenH = GetScreenHeight()

    local mesh = BlazeBolt.CreateMesh()

    local vertices = {
        { x = 0,        y = 0,        u = 0, v = 0 },
        { x = screenW,  y = 0,        u = 1, v = 0 },
        { x = screenW,  y = screenH,  u = 1, v = 1 },
        { x = 0,        y = screenH,  u = 0, v = 1 },
    }
    local indices = { 0, 1, 2, 0, 2, 3 }

    BlazeBolt.MeshSetData(mesh, vertices, indices)

    -- Назначаем кастомный шейдер
    BlazeBolt.MeshSetShader(mesh, "shaders/post.vert", "shaders/post.frag")

    -- Передаём uniform-переменные
    BlazeBolt.MeshSetUniformFloat(mesh, "u_time", 0.0)
    BlazeBolt.MeshSetUniformVec2(mesh, "u_resolution", screenW, screenH)
end

function Update(dt)
    time = time + dt
    BlazeBolt.MeshSetUniformFloat(mesh, "u_time", time)
end

function Draw()
    BlazeBolt.MeshDraw(mesh)
end
```

---

## Камера (Camera)

### CreateCamera

Создаёт камеру.

```
BlazeBolt.CreateCamera() → entity
```

**Возвращает:** `entity` (integer).

---

### CameraSetPosition

Устанавливает позицию камеры.

```
BlazeBolt.CameraSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор камеры |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### CameraGetPosition

Возвращает позицию камеры.

```
BlazeBolt.CameraGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор камеры |

**Возвращает:** `x` (number), `y` (number).

---

### CameraSetZoom

Устанавливает зум камеры.

```
BlazeBolt.CameraSetZoom(entity, zoom)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор камеры |
| `zoom` | number | Коэффициент зума (1.0 = нормальный) |

**Возвращает:** ничего.

---

### CameraGetZoom

Возвращает зум камеры.

```
BlazeBolt.CameraGetZoom(entity) → zoom
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор камеры |

**Возвращает:** `zoom` (number).

---

### CameraSetRotation

Устанавливает угол поворота камеры (градусы).

```
BlazeBolt.CameraSetRotation(entity, rotation)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор камеры |
| `rotation` | number | Угол поворота |

**Возвращает:** ничего.

---

### CameraGetRotation

Возвращает угол поворота камеры.

```
BlazeBolt.CameraGetRotation(entity) → rotation
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор камеры |

**Возвращает:** `rotation` (number).

---

### Пример: Камера

```lua
local camera
local playerSprite

function Start()
    camera = BlazeBolt.CreateCamera()
    playerSprite = BlazeBolt.CreateSprite("assets/player.png", 400, 300)
end

function Update(dt)
    local x, y = 400, 300
    if BlazeBolt.IsKeyPressed(Keys.W) then y = y - 200 * dt end
    if BlazeBolt.IsKeyPressed(Keys.S) then y = y + 200 * dt end
    if BlazeBolt.IsKeyPressed(Keys.A) then x = x - 200 * dt end
    if BlazeBolt.IsKeyPressed(Keys.D) then x = x + 200 * dt end
    BlazeBolt.CameraSetPosition(camera, x, y)

    local zoom = BlazeBolt.CameraGetZoom(camera)
    if BlazeBolt.IsKeyPressed(Keys.UP) then
        BlazeBolt.CameraSetZoom(camera, zoom + 0.5 * dt)
    end
    if BlazeBolt.IsKeyPressed(Keys.DOWN) then
        BlazeBolt.CameraSetZoom(camera, zoom - 0.5 * dt)
    end
end
```

---

## Системы частиц (ParticleSystem)

### CreateParticleSystem

Создаёт систему частиц.

```
BlazeBolt.CreateParticleSystem() → entity
```

**Возвращает:** `entity` (integer).

---

### ParticleSystemSetPosition

Устанавливает позицию генерации частиц.

```
BlazeBolt.ParticleSystemSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### ParticleSystemSetTexture

Устанавливает текстуру частиц.

```
BlazeBolt.ParticleSystemSetTexture(entity, path)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `path` | string | Путь к текстуре |

**Возвращает:** ничего.

---

### ParticleSystemSetEmissionRate

Устанавливает скорость генерации (частиц/сек).

```
BlazeBolt.ParticleSystemSetEmissionRate(entity, rate)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `rate` | number | Частиц в секунду |

**Возвращает:** ничего.

---

### ParticleSystemGetEmissionRate

Возвращает скорость генерации.

```
BlazeBolt.ParticleSystemGetEmissionRate(entity) → rate
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |

**Возвращает:** `rate` (number).

---

### ParticleSystemSetLifetime

Устанавливает время жизни частиц (от мин до макс).

```
BlazeBolt.ParticleSystemSetLifetime(entity, min, max)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `min` | number | Минимальное время жизни (сек) |
| `max` | number | Максимальное время жизни (сек) |

**Возвращает:** ничего.

---

### ParticleSystemSetSpeed

Устанавливает диапазон скорости частиц.

```
BlazeBolt.ParticleSystemSetSpeed(entity, min, max)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `min` | number | Минимальная скорость |
| `max` | number | Максимальная скорость |

**Возвращает:** ничего.

---

### ParticleSystemSetSize

Устанавливает диапазон начального размера частиц.

```
BlazeBolt.ParticleSystemSetSize(entity, min, max)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `min` | number | Минимальный размер |
| `max` | number | Максимальный размер |

**Возвращает:** ничего.

---

### ParticleSystemSetEndSize

Устанавливает диапазон конечного размера частиц.

```
BlazeBolt.ParticleSystemSetEndSize(entity, min, max)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `min` | number | Минимальный конечный размер |
| `max` | number | Максимальный конечный размер |

**Возвращает:** ничего.

---

### ParticleSystemSetColor

Устанавливает цвет частиц (от начального до конечного).

```
BlazeBolt.ParticleSystemSetColor(entity, r1, g1, b1, [a1], r2, g2, b2, [a2])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `entity` | integer | Идентификатор системы частиц | — |
| `r1` | number | Начальный красный | — |
| `g1` | number | Начальный зелёный | — |
| `b1` | number | Начальный синий | — |
| `a1` | number | Начальная альфа | `1.0` |
| `r2` | number | Конечный красный | — |
| `g2` | number | Конечный зелёный | — |
| `b2` | number | Конечный синий | — |
| `a2` | number | Конечная альфа | `0.0` |

**Возвращает:** ничего.

---

### ParticleSystemSetDirection

Устанавливает диапазон углов направления частиц.

```
BlazeBolt.ParticleSystemSetDirection(entity, minAngle, maxAngle)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `minAngle` | number | Минимальный угол (градусы) |
| `maxAngle` | number | Максимальный угол (градусы) |

**Возвращает:** ничего.

---

### ParticleSystemSetRotationSpeed

Устанавливает скорость вращения частиц.

```
BlazeBolt.ParticleSystemSetRotationSpeed(entity, speed)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `speed` | number | Скорость вращения |

**Возвращает:** ничего.

---

### ParticleSystemSetActive

Включает/выключает генерацию частиц.

```
BlazeBolt.ParticleSystemSetActive(entity, active)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `active` | boolean | Активность |

**Возвращает:** ничего.

---

### ParticleSystemIsActive

Проверяет, активна ли система частиц.

```
BlazeBolt.ParticleSystemIsActive(entity) → active
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |

**Возвращает:** `active` (boolean).

---

### ParticleSystemSetVisible

Устанавливает видимость системы частиц.

```
BlazeBolt.ParticleSystemSetVisible(entity, visible)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `visible` | boolean | Видимость |

**Возвращает:** ничего.

---

### ParticleSystemIsVisible

Проверяет видимость системы частиц.

```
BlazeBolt.ParticleSystemIsVisible(entity) → visible
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |

**Возвращает:** `visible` (boolean).

---

### ParticleSystemEmit

Генерирует определённое количество частиц вручную.

```
BlazeBolt.ParticleSystemEmit(entity, count)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |
| `count` | integer | Количество частиц |

**Возвращает:** ничего.

---

### ParticleSystemClear

Удаляет все активные частицы.

```
BlazeBolt.ParticleSystemClear(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |

**Возвращает:** ничего.

---

### ParticleSystemGetCount

Возвращает количество активных частиц.

```
BlazeBolt.ParticleSystemGetCount(entity) → count
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор системы частиц |

**Возвращает:** `count` (integer).

---

### Пример: Системы частиц

```lua
local fire

function Start()
    fire = BlazeBolt.CreateParticleSystem()
    BlazeBolt.ParticleSystemSetPosition(fire, 400, 300)
    BlazeBolt.ParticleSystemSetTexture(fire, "assets/spark.png")
    BlazeBolt.ParticleSystemSetEmissionRate(fire, 50)
    BlazeBolt.ParticleSystemSetLifetime(fire, 0.5, 1.5)
    BlazeBolt.ParticleSystemSetSpeed(fire, 50, 150)
    BlazeBolt.ParticleSystemSetSize(fire, 4, 12)
    BlazeBolt.ParticleSystemSetEndSize(fire, 0, 2)
    BlazeBolt.ParticleSystemSetColor(fire, 1.0, 0.5, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0)
    BlazeBolt.ParticleSystemSetDirection(fire, 250, 290)
    BlazeBolt.ParticleSystemSetRotationSpeed(fire, 180)
    BlazeBolt.ParticleSystemSetActive(fire, true)
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.ParticleSystemEmit(fire, 20)
    end
end
```

---

## Тайлсеты (Tileset)

### CreateTileset

Создаёт тайлсет из атласа текстур.

```
BlazeBolt.CreateTileset(texturePath, tileW, tileH, atlasCols, atlasRows) → entity
```

| Аргумент | Тип | Описание |
|---|---|---|
| `texturePath` | string | Путь к атласу текстур |
| `tileW` | integer | Ширина тайла (пиксели) |
| `tileH` | integer | Высота тайла (пиксели) |
| `atlasCols` | integer | Количество колонок в атласе |
| `atlasRows` | integer | Количество строк в атласе |

**Возвращает:** `entity` (integer).

---

### TilesetSetMap

Устанавливает карту тайлов (2D-массив индексов).

```
BlazeBolt.TilesetSetMap(entity, map)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |
| `map` | table | Двумерный массив: `{ {0,1,2}, {3,4,5}, ... }` |

Индекс `0` означает пустой тайл.

**Возвращает:** ничего.

---

### TilesetGetTile

Возвращает индекс тайла в указанной ячейке.

```
BlazeBolt.TilesetGetTile(entity, col, row) → tileIndex
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |
| `col` | integer | Номер колонки |
| `row` | integer | Номер строки |

**Возвращает:** `tileIndex` (integer).

---

### TilesetSetTile

Устанавливает индекс тайла в указанной ячейке.

```
BlazeBolt.TilesetSetTile(entity, col, row, tileIndex)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |
| `col` | integer | Номер колонки |
| `row` | integer | Номер строки |
| `tileIndex` | integer | Индекс тайла в атласе |

**Возвращает:** ничего.

---

### TilesetSetTileSize

Устанавливает размер тайлов.

```
BlazeBolt.TilesetSetTileSize(entity, w, h)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |
| `w` | integer | Ширина тайла |
| `h` | integer | Высота тайла |

**Возвращает:** ничего.

---

### TilesetGetTileSize

Возвращает размер тайлов.

```
BlazeBolt.TilesetGetTileSize(entity) → w, h
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** `w` (integer), `h` (integer).

---

### TilesetSetPosition

Устанавливает позицию тайлсета.

```
BlazeBolt.TilesetSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### TilesetGetPosition

Возвращает позицию тайлсета.

```
BlazeBolt.TilesetGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** `x` (number), `y` (number).

---

### TilesetGetMapWidth

Возвращает ширину карты (в тайлах).

```
BlazeBolt.TilesetGetMapWidth(entity) → width
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** `width` (integer).

---

### TilesetGetMapHeight

Возвращает высоту карты (в тайлах).

```
BlazeBolt.TilesetGetMapHeight(entity) → height
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** `height` (integer).

---

### TilesetGetTileCount

Возвращает общее количество тайлов в атласе.

```
BlazeBolt.TilesetGetTileCount(entity) → count
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** `count` (integer).

---

### TilesetDraw

Перестраивает и отрисовывает тайлсет.

```
BlazeBolt.TilesetDraw(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** ничего.

---

### DestroyTileset

Удаляет тайлсет.

```
BlazeBolt.DestroyTileset(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тайлсета |

**Возвращает:** ничего.

---

### Пример: Тайлсеты

```lua
local tileset

function Start()
    tileset = BlazeBolt.CreateTileset("assets/tileset.png", 32, 32, 8, 8)

    local map = {}
    for row = 0, 9 do
        map[row] = {}
        for col = 0, 14 do
            map[row][col] = math.random(0, 5)
        end
    end

    BlazeBolt.TilesetSetMap(tileset, map)
    BlazeBolt.TilesetSetPosition(tileset, 0, 0)
    BlazeBolt.TilesetDraw(tileset)
end
```

---

## Освещение (Light)

### CreatePointLight

Создаёт точечный источник света.

```
BlazeBolt.CreatePointLight([x], [y], [r], [g], [b], [intensity], [radius]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Позиция X | `0` |
| `y` | number | Позиция Y | `0` |
| `r` | number | Красный компонент цвета | `1` |
| `g` | number | Зелёный компонент цвета | `1` |
| `b` | number | Синий компонент цвета | `1` |
| `intensity` | number | Интенсивность | `1.0` |
| `radius` | number | Радиус действия | `200.0` |

**Возвращает:** `entity` (integer).

---

### CreateAmbientLight

Создаётambient-освещение (фоновый свет).

```
BlazeBolt.CreateAmbientLight([r], [g], [b], [intensity]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `r` | number | Красный компонент | `1` |
| `g` | number | Зелёный компонент | `1` |
| `b` | number | Синий компонент | `1` |
| `intensity` | number | Интенсивность | `0.3` |

**Возвращает:** `entity` (integer).

---

### LightSetPosition

Устанавливает позицию источника света.

```
BlazeBolt.LightSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### LightGetPosition

Возвращает позицию источника света.

```
BlazeBolt.LightGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |

**Возвращает:** `x` (number), `y` (number).

---

### LightSetColor

Устанавливает цвет света.

```
BlazeBolt.LightSetColor(entity, r, g, b)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |
| `r` | number | Красный (0–1) |
| `g` | number | Зелёный (0–1) |
| `b` | number | Синий (0–1) |

**Возвращает:** ничего.

---

### LightGetColor

Возвращает цвет света.

```
BlazeBolt.LightGetColor(entity) → r, g, b
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |

**Возвращает:** `r` (number), `g` (number), `b` (number).

---

### LightSetIntensity

Устанавливает интенсивность света.

```
BlazeBolt.LightSetIntensity(entity, intensity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |
| `intensity` | number | Интенсивность |

**Возвращает:** ничего.

---

### LightGetIntensity

Возвращает интенсивность света.

```
BlazeBolt.LightGetIntensity(entity) → intensity
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |

**Возвращает:** `intensity` (number).

---

### LightSetRadius

Устанавливает радиус действия точечного света.

```
BlazeBolt.LightSetRadius(entity, radius)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |
| `radius` | number | Радиус |

**Возвращает:** ничего.

---

### LightGetRadius

Возвращает радиус действия света.

```
BlazeBolt.LightGetRadius(entity) → radius
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |

**Возвращает:** `radius` (number).

---

### LightSetEnabled

Включает/выключает источник света.

```
BlazeBolt.LightSetEnabled(entity, enabled)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |
| `enabled` | boolean | Включён |

**Возвращает:** ничего.

---

### LightGetEnabled

Проверяет, включён ли источник света.

```
BlazeBolt.LightGetEnabled(entity) → enabled
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |

**Возвращает:** `enabled` (boolean).

---

### DestroyLight

Удаляет источник света.

```
BlazeBolt.DestroyLight(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор света |

**Возвращает:** ничего.

---

### Пример: Освещение

```lua
local playerLight
local ambientLight

function Start()
    ambientLight = BlazeBolt.CreateAmbientLight(0.2, 0.2, 0.3, 0.3)

    playerLight = BlazeBolt.CreatePointLight(400, 300, 1.0, 0.8, 0.6, 1.5, 250.0)
end

function Update(dt)
    local x, y = BlazeBolt.LightGetPosition(playerLight)
    if BlazeBolt.IsKeyPressed(Keys.RIGHT) then x = x + 200 * dt end
    if BlazeBolt.IsKeyPressed(Keys.LEFT) then x = x - 200 * dt end
    if BlazeBolt.IsKeyPressed(Keys.UP) then y = y - 200 * dt end
    if BlazeBolt.IsKeyPressed(Keys.DOWN) then y = y + 200 * dt end
    BlazeBolt.LightSetPosition(playerLight, x, y)

    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        local r = BlazeBolt.Random(0, 1)
        local g = BlazeBolt.Random(0, 1)
        local b = BlazeBolt.Random(0, 1)
        BlazeBolt.LightSetColor(playerLight, r, g, b)
    end
end
```

---

## Физика (Physics)

Физика основана на Box2D. Типы тел: `0` = Static, `1` = Kinematic, `2` = Dynamic.

### PhysicsInit

Инициализирует физический мир.

```
BlazeBolt.PhysicsInit([gx], [gy])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `gx` | number | Гравитация по X | `0` |
| `gy` | number | Гравитация по Y | `-9.81` |

**Возвращает:** ничего.

---

### PhysicsSetGravity

Устанавливает гравитацию.

```
BlazeBolt.PhysicsSetGravity(x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | number | Гравитация по X |
| `y` | number | Гравитация по Y |

**Возвращает:** ничего.

---

### PhysicsGetGravity

Возвращает текущую гравитацию.

```
BlazeBolt.PhysicsGetGravity() → x, y
```

**Возвращает:** `x` (number), `y` (number).

---

### PhysicsCreateBody

Создаёт физическое тело.

```
BlazeBolt.PhysicsCreateBody(bodyType, [x], [y], [mass], [friction], [restitution]) → entity
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `bodyType` | integer | Тип тела (0=Static, 1=Kinematic, 2=Dynamic) | — |
| `x` | number | Позиция X | `0` |
| `y` | number | Позиция Y | `0` |
| `mass` | number | Масса | `1` |
| `friction` | number | Трение | `0.3` |
| `restitution` | number | Упругость | `0.5` |

**Возвращает:** `entity` (integer) — идентификатор тела.

---

### PhysicsAddCircle

Добавляет круглую форму к телу.

```
BlazeBolt.PhysicsAddCircle(entity, radius, [ox], [oy])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `entity` | integer | Идентификатор тела | — |
| `radius` | number | Радиус | — |
| `ox` | number | Смещение по X от центра | `0` |
| `oy` | number | Смещение по Y от центра | `0` |

**Возвращает:** ничего.

---

### PhysicsAddRectangle

Добавляет прямоугольную форму к телу.

```
BlazeBolt.PhysicsAddRectangle(entity, halfWidth, halfHeight)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `halfWidth` | number | Половина ширины |
| `halfHeight` | number | Половина высоты |

**Возвращает:** ничего.

---

### PhysicsSetLinearVelocity

Устанавливает линейную скорость.

```
BlazeBolt.PhysicsSetLinearVelocity(entity, vx, vy)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `vx` | number | Скорость по X |
| `vy` | number | Скорость по Y |

**Возвращает:** ничего.

---

### PhysicsGetLinearVelocity

Возвращает линейную скорость.

```
BlazeBolt.PhysicsGetLinearVelocity(entity) → vx, vy
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `vx` (number), `vy` (number).

---

### PhysicsSetAngularVelocity

Устанавливает угловую скорость.

```
BlazeBolt.PhysicsSetAngularVelocity(entity, angularVelocity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `angularVelocity` | number | Угловая скорость |

**Возвращает:** ничего.

---

### PhysicsGetAngularVelocity

Возвращает угловую скорость.

```
BlazeBolt.PhysicsGetAngularVelocity(entity) → angularVelocity
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `angularVelocity` (number).

---

### PhysicsApplyForce

Применяет силу к центру масс тела.

```
BlazeBolt.PhysicsApplyForce(entity, fx, fy)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `fx` | number | Сила по X |
| `fy` | number | Сила по Y |

**Возвращает:** ничего.

---

### PhysicsApplyForceAtPoint

Применяет силу в указанной точке.

```
BlazeBolt.PhysicsApplyForceAtPoint(entity, fx, fy, px, py)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `fx` | number | Сила по X |
| `fy` | number | Сила по Y |
| `px` | number | Точка приложения по X |
| `py` | number | Точка приложения по Y |

**Возвращает:** ничего.

---

### PhysicsApplyImpulse

Применяет импульс к центру масс.

```
BlazeBolt.PhysicsApplyImpulse(entity, ix, iy)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `ix` | number | Импульс по X |
| `iy` | number | Импульс по Y |

**Возвращает:** ничего.

---

### PhysicsApplyImpulseAtPoint

Применяет импульс в указанной точке.

```
BlazeBolt.PhysicsApplyImpulseAtPoint(entity, ix, iy, px, py)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `ix` | number | Импульс по X |
| `iy` | number | Импульс по Y |
| `px` | number | Точка приложения по X |
| `py` | number | Точка приложения по Y |

**Возвращает:** ничего.

---

### PhysicsApplyTorque

Применяет крутящий момент.

```
BlazeBolt.PhysicsApplyTorque(entity, torque)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `torque` | number | Крутящий момент |

**Возвращает:** ничего.

---

### PhysicsSetPosition

Устанавливает позицию физического тела.

```
BlazeBolt.PhysicsSetPosition(entity, x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |

**Возвращает:** ничего.

---

### PhysicsGetPosition

Возвращает позицию физического тела.

```
BlazeBolt.PhysicsGetPosition(entity) → x, y
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `x` (number), `y` (number).

---

### PhysicsSetAngle

Устанавливает угол поворота тела (радианы).

```
BlazeBolt.PhysicsSetAngle(entity, angle)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `angle` | number | Угол в радианах |

**Возвращает:** ничего.

---

### PhysicsGetAngle

Возвращает угол поворота тела.

```
BlazeBolt.PhysicsGetAngle(entity) → angle
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `angle` (number, радианы).

---

### PhysicsSetGravityScale

Устанавливает множитель гравитации для тела.

```
BlazeBolt.PhysicsSetGravityScale(entity, scale)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `scale` | number | Множитель гравитации |

**Возвращает:** ничего.

---

### PhysicsGetGravityScale

Возвращает множитель гравитации.

```
BlazeBolt.PhysicsGetGravityScale(entity) → scale
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `scale` (number).

---

### PhysicsSetActive

Включает/выключает физическое тело.

```
BlazeBolt.PhysicsSetActive(entity, active)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `active` | boolean | Активность |

**Возвращает:** ничего.

---

### PhysicsIsActive

Проверяет, активно ли физическое тело.

```
BlazeBolt.PhysicsIsActive(entity) → active
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `active` (boolean).

---

### PhysicsSetFixedRotation

Включает/выключает фиксированный поворот тела.

```
BlazeBolt.PhysicsSetFixedRotation(entity, fixed)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `fixed` | boolean | Фиксированный поворот |

**Возвращает:** ничего.

---

### PhysicsIsFixedRotation

Проверяет, фиксирован ли поворот.

```
BlazeBolt.PhysicsIsFixedRotation(entity) → fixed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `fixed` (boolean).

---

### PhysicsSetBullet

Включает/выключает режим "пули" (точный расчёт столкновений).

```
BlazeBolt.PhysicsSetBullet(entity, bullet)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `bullet` | boolean | Режим пули |

**Возвращает:** ничего.

---

### PhysicsIsBullet

Проверяет, включён ли режим "пули".

```
BlazeBolt.PhysicsIsBullet(entity) → bullet
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `bullet` (boolean).

---

### PhysicsDestroyBody

Удаляет физическое тело.

```
BlazeBolt.PhysicsDestroyBody(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** ничего.

---

### PhysicsGetMass

Возвращает массу тела.

```
BlazeBolt.PhysicsGetMass(entity) → mass
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `mass` (number).

---

### PhysicsSetFriction

Устанавливает коэффициент трения.

```
BlazeBolt.PhysicsSetFriction(entity, friction)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `friction` | number | Коэффициент трения |

**Возвращает:** ничего.

---

### PhysicsGetFriction

Возвращает коэффициент трения.

```
BlazeBolt.PhysicsGetFriction(entity) → friction
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `friction` (number).

---

### PhysicsSetRestitution

Устанавливает коэффициент упругости.

```
BlazeBolt.PhysicsSetRestitution(entity, restitution)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |
| `restitution` | number | Коэффициент упругости |

**Возвращает:** ничего.

---

### PhysicsGetRestitution

Возвращает коэффициент упругости.

```
BlazeBolt.PhysicsGetRestitution(entity) → restitution
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор тела |

**Возвращает:** `restitution` (number).

---

### PhysicsStep

Выполняет一步 физической симуляции (обычно вызывается автоматически).

```
BlazeBolt.PhysicsStep()
```

**Возвращает:** ничего.

---

### PhysicsSyncSprite

Синхронизирует позицию и поворот физического тела со спрайтом.

```
BlazeBolt.PhysicsSyncSprite(bodyEntity, spriteEntity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `bodyEntity` | integer | Идентификатор физического тела |
| `spriteEntity` | integer | Идентификатор спрайта |

**Возвращает:** ничего.

---

### PhysicsSyncText

Синхронизирует физическое тело с текстовым объектом.

```
BlazeBolt.PhysicsSyncText(bodyEntity, textEntity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `bodyEntity` | integer | Идентификатор физического тела |
| `textEntity` | integer | Идентификатор текста |

**Возвращает:** ничего.

---

### PhysicsSyncAnimatedSprite

Синхронизирует физическое тело с анимированным спрайтом.

```
BlazeBolt.PhysicsSyncAnimatedSprite(bodyEntity, animEntity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `bodyEntity` | integer | Идентификатор физического тела |
| `animEntity` | integer | Идентификатор анимированного спрайта |

**Возвращает:** ничего.

---

### Пример: Физика

```lua
local ground
local ball
local ballSprite

function Start()
    BlazeBolt.PhysicsInit(0, -9.81)

    ground = BlazeBolt.PhysicsCreateBody(0, 400, 550, 0, 0, 0)
    BlazeBolt.PhysicsAddRectangle(ground, 400, 10)

    ball = BlazeBolt.PhysicsCreateBody(2, 400, 100, 1.0, 0.3, 0.7)
    BlazeBolt.PhysicsAddCircle(ball, 20)

    ballSprite = BlazeBolt.CreateSprite("assets/ball.png", 400, 100)
end

function Update(dt)
    BlazeBolt.PhysicsSyncSprite(ball, ballSprite)

    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.PhysicsApplyImpulse(ball, 0, 500)
    end

    if BlazeBolt.IsKeyJustPressed(Keys.RIGHT) then
        BlazeBolt.PhysicsApplyForce(ball, 300, 0)
    end

    local vx, vy = BlazeBolt.PhysicsGetLinearVelocity(ball)
    BlazeBolt.Print("Velocity: ", vx, ", ", vy)
end
```

---

## Аудио (Audio)

### LoadSound

Загружает звуковой файл.

```
BlazeBolt.LoadSound(filename, soundName, [loop]) → soundId
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `filename` | string | Путь к звуковому файлу | — |
| `soundName` | string | Имя для идентификации | — |
| `loop` | boolean | Зацикливание | `false` |

**Возвращает:** `soundId` (integer) — идентификатор звука.

---

### PlaySound

Воспроизводит звук по имени.

```
BlazeBolt.PlaySound(soundName)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundName` | string | Имя звука |

**Возвращает:** ничего.

---

### PlaySoundById

Воспроизводит звук по идентификатору.

```
BlazeBolt.PlaySoundById(soundId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundId` | integer | Идентификатор звука |

**Возвращает:** ничего.

---

### StopSound

Останавливает воспроизведение звука по имени.

```
BlazeBolt.StopSound(soundName)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundName` | string | Имя звука |

**Возвращает:** ничего.

---

### StopAllSounds

Останавливает все воспроизводимые звуки.

```
BlazeBolt.StopAllSounds()
```

**Возвращает:** ничего.

---

### SetSoundVolume

Устанавливает громкость звука.

```
BlazeBolt.SetSoundVolume(soundName, volume)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundName` | string | Имя звука |
| `volume` | number | Громкость (0.0 – 1.0) |

**Возвращает:** ничего.

---

### IsSoundPlaying

Проверяет, воспроизводится ли звук.

```
BlazeBolt.IsSoundPlaying(soundName) → playing
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundName` | string | Имя звука |

**Возвращает:** `playing` (boolean).

---

### Sound Pitch & Looping

#### SetSoundPitch

```
BlazeBolt.SetSoundPitch(soundId, pitch)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundId` | integer | Идентификатор звука |
| `pitch` | number | Высота тона (1.0 = нормальная) |

#### SetSoundLoopingById

```
BlazeBolt.SetSoundLoopingById(soundId, loop)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundId` | integer | Идентификатор звука |
| `loop` | boolean | Зацикливание |

---

### 3D позиционный звук

#### SetSoundPosition

```
BlazeBolt.SetSoundPosition(soundId, x, y, z)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundId` | integer | Идентификатор звука |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |
| `z` | number | Позиция Z |

#### SetSoundPositionByName

```
BlazeBolt.SetSoundPositionByName(soundName, x, y, z)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `soundName` | string | Имя звука |
| `x` | number | Позиция X |
| `y` | number | Позиция Y |
| `z` | number | Позиция Z |

#### GetSoundPosition

```
BlazeBolt.GetSoundPosition(soundId) → x, y, z
```

#### SetSoundVelocity

```
BlazeBolt.SetSoundVelocity(soundId, x, y, z)
```

#### SetSoundRolloff

```
BlazeBolt.SetSoundRolloff(soundId, rolloff)
```

#### SetSoundReferenceDistance

```
BlazeBolt.SetSoundReferenceDistance(soundId, distance)
```

#### SetSoundMaxDistance

```
BlazeBolt.SetSoundMaxDistance(soundId, distance)
```

#### SetSoundSpatial

```
BlazeBolt.SetSoundSpatial(soundId, spatial)
```

#### SetSoundCone

```
BlazeBolt.SetSoundCone(soundId, innerAngle, outerAngle, outerGain)
```

#### SetSoundDirection

```
BlazeBolt.SetSoundDirection(soundId, x, y, z)
```

---

### Позиционный слушатель (Listener)

#### SetListenerPosition

```
BlazeBolt.SetListenerPosition(x, y, z)
```

#### GetListenerPosition

```
BlazeBolt.GetListenerPosition() → x, y, z
```

#### SetListenerVelocity

```
BlazeBolt.SetListenerVelocity(x, y, z)
```

#### SetListenerOrientation

```
BlazeBolt.SetListenerOrientation(fx, fy, fz, ux, uy, uz)
```

#### SetListenerGain

```
BlazeBolt.SetListenerGain(gain)
```

---

### Аудио-эффекты (EFX)

#### CreateAudioEffect

```
BlazeBolt.CreateAudioEffect() → effectId
```

#### DestroyAudioEffect

```
BlazeBolt.DestroyAudioEffect(effectId)
```

#### SetAudioEffectType

```
BlazeBolt.SetAudioEffectType(effectId, type) → success
```

#### SetAudioEffectf

```
BlazeBolt.SetAudioEffectf(effectId, param, value) → success
```

#### SetAudioEffecti

```
BlazeBolt.SetAudioEffecti(effectId, param, value) → success
```

#### GetAudioEffectf

```
BlazeBolt.GetAudioEffectf(effectId, param) → value
```

#### GetAudioEffecti

```
BlazeBolt.GetAudioEffecti(effectId, param) → value
```

#### GetAudioEfxSupported

```
BlazeBolt.GetAudioEfxSupported() → supported
```

---

### Аудио-фильтры

#### CreateAudioFilter

```
BlazeBolt.CreateAudioFilter() → filterId
```

#### DestroyAudioFilter

```
BlazeBolt.DestroyAudioFilter(filterId)
```

#### SetAudioFilterType

```
BlazeBolt.SetAudioFilterType(filterId, type) → success
```

#### SetAudioFilterf

```
BlazeBolt.SetAudioFilterf(filterId, param, value) → success
```

---

### Слоты эффектов

#### CreateAudioEffectSlot

```
BlazeBolt.CreateAudioEffectSlot() → slotId
```

#### DestroyAudioEffectSlot

```
BlazeBolt.DestroyAudioEffectSlot(slotId)
```

#### SetAudioEffectSlotEffect

```
BlazeBolt.SetAudioEffectSlotEffect(slotId, effectId) → success
```

#### ClearAudioEffectSlotEffect

```
BlazeBolt.ClearAudioEffectSlotEffect(slotId) → success
```

#### SetAudioEffectSlotGain

```
BlazeBolt.SetAudioEffectSlotGain(slotId, gain) → success
```

---

### Привязка эффектов и фильтров

#### AttachAudioEffect

```
BlazeBolt.AttachAudioEffect(soundId, slotId) → success
```

#### DetachAudioEffect

```
BlazeBolt.DetachAudioEffect(soundId) → success
```

#### AttachAudioFilter

```
BlazeBolt.AttachAudioFilter(soundId, filterId) → success
```

---

### Пример: Аудио

```lua
function Start()
    BlazeBolt.LoadSound("assets/background.ogg", "bgm", true)
    BlazeBolt.LoadSound("assets/jump.wav", "jump", false)
    BlazeBolt.LoadSound("assets/shoot.wav", "shoot", false)

    BlazeBolt.SetSoundVolume("bgm", 0.3)
    BlazeBolt.PlaySound("bgm")
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.PlaySound("jump")
    end

    if BlazeBolt.IsKeyJustPressed(Keys.MOUSE_LEFT) then
        BlazeBolt.PlaySound("shoot")
    end

    if BlazeBolt.IsKeyJustPressed(Keys.M) then
        if BlazeBolt.IsSoundPlaying("bgm") then
            BlazeBolt.StopSound("bgm")
        else
            BlazeBolt.PlaySound("bgm")
        end
    end
end
```

---

## Окна (Window)

### CreateWindow

Создаёт дополнительное окно.

```
BlazeBolt.CreateWindow(width, height, title) → windowPtr
```

| Аргумент | Тип | Описание |
|---|---|---|
| `width` | integer | Ширина окна |
| `height` | integer | Высота окна |
| `title` | string | Заголовок окна |

**Возвращает:** `windowPtr` (light userdata) — указатель на окно.

---

### SetWindowTitle

Устанавливает заголовок окна.

```
BlazeBolt.SetWindowTitle(windowPtr, title)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `windowPtr` | light userdata | Указатель на окно |
| `title` | string | Новый заголовок |

**Возвращает:** ничего.

---

### SetWindowSize

Устанавливает размер окна.

```
BlazeBolt.SetWindowSize(windowPtr, width, height)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `windowPtr` | light userdata | Указатель на окно |
| `width` | integer | Новая ширина |
| `height` | integer | Новая высота |

**Возвращает:** ничего.

---

### SetWindowIcon

Устанавливает иконку окна.

```
BlazeBolt.SetWindowIcon(windowPtr, iconPath)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `windowPtr` | light userdata | Указатель на окно |
| `iconPath` | string | Путь к изображению иконки |

**Возвращает:** ничего.

---

### SetMainWindowTitle

Устанавливает заголовок главного окна.

```
BlazeBolt.SetMainWindowTitle(title)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `title` | string | Новый заголовок |

**Возвращает:** ничего.

---

### GetMainWindowTitle

Возвращает заголовок главного окна.

```
BlazeBolt.GetMainWindowTitle() → title
```

**Возвращает:** `title` (string).

---

### SetMainWindowSize

Устанавливает размер главного окна.

```
BlazeBolt.SetMainWindowSize(width, height)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `width` | integer | Новая ширина |
| `height` | integer | Новая высота |

**Возвращает:** ничего.

---

### GetMainWindowWidth

Возвращает ширину главного окна.

```
BlazeBolt.GetMainWindowWidth() → width
```

**Возвращает:** `width` (integer).

---

### GetMainWindowHeight

Возвращает высоту главного окна.

```
BlazeBolt.GetMainWindowHeight() → height
```

**Возвращает:** `height` (integer).

---

### SetMainWindowPosition

Устанавливает позицию главного окна.

```
BlazeBolt.SetMainWindowPosition(x, y)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | integer | Позиция X |
| `y` | integer | Позиция Y |

**Возвращает:** ничего.

---

### GetMainWindowPosition

Возвращает позицию главного окна.

```
BlazeBolt.GetMainWindowPosition() → x, y
```

**Возвращает:** `x` (integer), `y` (integer).

---

### SetMainWindowIcon

Устанавливает иконку главного окна.

```
BlazeBolt.SetMainWindowIcon(iconPath)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `iconPath` | string | Путь к изображению иконки |

**Возвращает:** ничего.

---

### SetMainWindowShouldClose

Устанавливает флаг закрытия главного окна.

```
BlazeBolt.SetMainWindowShouldClose(flag)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `flag` | boolean | Закрыть окно |

**Возвращает:** ничего.

---

### IsMainWindowShouldClose

Проверяет, запрошено ли закрытие главного окна.

```
BlazeBolt.IsMainWindowShouldClose() → shouldClose
```

**Возвращает:** `shouldClose` (boolean).

---

### SetMainWindowFullscreen

Включает или выключает полноэкранный режим главного окна.

```
BlazeBolt.SetMainWindowFullscreen(fullscreen)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `fullscreen` | boolean | `true` — включить полноэкранный режим, `false` — оконный |

**Возвращает:** ничего.

---

### IsMainWindowFullscreen

Проверяет, находится ли главное окно в полноэкранном режиме.

```
BlazeBolt.IsMainWindowFullscreen() → isFullscreen
```

**Возвращает:** `isFullscreen` (boolean).

---

### ToggleMainWindowFullscreen

Переключает полноэкранный режим главного окна.

```
BlazeBolt.ToggleMainWindowFullscreen()
```

**Возвращает:** ничего.

---

### SetMainWindowVSync

Включает или выключает вертикальную синхронизацию (VSync).

```
BlazeBolt.SetMainWindowVSync(enabled)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `enabled` | boolean | `true` — включить VSync, `false` — выключить |

**Возвращает:** ничего.

---

### IsMainWindowVSync

Проверяет, включена ли вертикальная синхронизация.

```
BlazeBolt.IsMainWindowVSync() → isVSync
```

**Возвращает:** `isVSync` (boolean).

---

### ToggleMainWindowVSync

Переключает вертикальную синхронизацию.

```
BlazeBolt.ToggleMainWindowVSync()
```

**Возвращает:** ничего.

---

### Пример: Окна

```lua
function Start()
    BlazeBolt.SetMainWindowTitle("My Game")
    BlazeBolt.SetMainWindowSize(1024, 768)
    BlazeBolt.SetMainWindowPosition(100, 100)
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.F11) then
        BlazeBolt.ToggleMainWindowFullscreen()
    end

    if BlazeBolt.IsKeyJustPressed(Keys.F) then
        BlazeBolt.ToggleMainWindowVSync()
    end

    if BlazeBolt.IsKeyJustPressed(Keys.ESCAPE) then
        BlazeBolt.SetMainWindowShouldClose(true)
    end
end
```

---

## Скрипты (Script Management)

### LoadScript

Загружает Lua-скрипт.

```
BlazeBolt.LoadScript(scriptPath) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `scriptPath` | string | Путь к скрипту |

**Возвращает:** `success` (boolean).

---

### LoadScriptsFromList

Загружает скрипты из файла-списка (.BlazeBoltProject).

```
BlazeBolt.LoadScriptsFromList(listPath) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `listPath` | string | Путь к файлу списка |

**Возвращает:** `success` (boolean).

---

### ReloadScript

Перезагружает скрипт.

```
BlazeBolt.ReloadScript(scriptPath) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `scriptPath` | string | Путь к скрипту |

**Возвращает:** `success` (boolean).

---

### ReloadAllScripts

Перезагружает все загруженные скрипты.

```
BlazeBolt.ReloadAllScripts() → success
```

**Возвращает:** `success` (boolean).

---

### EnableScript

Включает или выключает скрипт.

```
BlazeBolt.EnableScript(scriptName, enabled)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `scriptName` | string | Имя скрипта |
| `enabled` | boolean | Включён/выключен |

**Возвращает:** ничего.

---

### IsScriptLoaded

Проверяет, загружен ли скрипт.

```
BlazeBolt.IsScriptLoaded(scriptName) → loaded
```

| Аргумент | Тип | Описание |
|---|---|---|
| `scriptName` | string | Имя скрипта |

**Возвращает:** `loaded` (boolean).

---

### GetLoadedScripts

Возвращает список загруженных скриптов.

```
BlazeBolt.GetLoadedScripts() → scripts
```

**Возвращает:** `scripts` (table) — массив имён загруженных скриптов.

---

### Пример: Скрипты

```lua
function Start()
    local scripts = BlazeBolt.GetLoadedScripts()
    for _, name in ipairs(scripts) do
        BlazeBolt.Print("Loaded script: " .. name)
    end
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.F5) then
        BlazeBolt.ReloadAllScripts()
        BlazeBolt.Print("All scripts reloaded!")
    end

    if BlazeBolt.IsKeyJustPressed(Keys.L) then
        BlazeBolt.LoadScript("assets/scripts/new_module.lua")
    end
end
```

---

## Сцены (Scene Management)

Управление сценами — это переключение между разными состояниями игры (меню, уровни, пауза) и загрузка/сохранение уровня из файла.

> **Для начинающих:** Есть два разных понятия:
> - **`LoadScene`** — переключение между логическими сценами (например, из "menu" в "game").
> - **`LoadSceneFile`** — загрузка `.scene` файла с объектами (спрайты, текст, камеры) в текущую сцену.
>
> Обычно вы используете оба: `LoadScene("game")` переключает сцену, а внутри `Start()` вызываете `LoadSceneFile("level1.scene")` чтобы создать все игровые объекты.

| Функция | Назначение | Когда использовать |
|---|---|---|
| `LoadSceneFile(path)` | Загружает объекты из `.scene` файла | В начале уровня, при загрузке сохранения |
| `SaveSceneFile(path)` | Сохраняет все объекты в `.scene` файл | При сохранении игры, автосейв |
| `LoadScene(name)` | Переключает логическую сцену | Переход между меню, уровнями, паузой |
| `GetCurrentScene()` | Узнать текущую сцену | В условных переходах, HUD |

---

### LoadSceneFile

Загружает `.scene` файл и создаёт все объекты сцены: спрайты, текст, камеры, источники света, системы частиц, тайлсеты и физические тела.

Файл `.scene` создаётся в редакторе сцен BlazeBolt или вручную в JSON-формате (см. раздел "Формат .scene файлов").

```
BlazeBolt.LoadSceneFile(path) → table<string, Entity>
```

| Аргумент | Тип | Описание |
|---|---|---|
| `path` | string | Путь к файлу `.scene` (например `"levels/level1.scene"`) |

**Возвращает:** таблицу `{ [имя_объекта] = EntityID }` для доступа к созданным объектам по имени, указанному в файле.

**Для начинающих:** сохраните возвращённую таблицу в глобальную переменную. Имена объектов берутся из поля `name` в `.scene` файле:

```lua
function Start()
    local objects = BlazeBolt.LoadSceneFile("levels/level1.scene")

    -- Работа с объектами по их именам
    local player = objects["player"]        -- спрайт игрока
    local camera = objects["main_camera"]   -- камера
    local healthText = objects["hud_health"] -- текст HP
end
```

**Для профессионалов:** один вызов `LoadSceneFile` может загрузить тысячи объектов. Файл парсится как JSON, поэтому вы можете генерировать `.scene` файлы процедурно. Возвращаемая таблица позволяет быстро найти любой объект по имени без поиска по всем Entity.

---

### SaveSceneFile

Сохраняет **все** текущие объекты сцены в `.scene` файл. Сохраняются: спрайты, текст, анимированные спрайты, камеры, источники света, системы частиц, тайлсеты и физические тела со всеми их свойствами.

```
BlazeBolt.SaveSceneFile(path) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `path` | string | Путь для сохранения файла `.scene` |

**Возвращает:** `success` (boolean) — `true`, если сохранение прошло успешно.

**Для начинающих:** вызывайте `SaveSceneFile` при выходе с уровня или по кнопке "Сохранить":

```lua
function End()
    BlazeBolt.SaveSceneFile("savegame.scene")
end
```

**Для профессионалов:** `SaveSceneFile` сериализует все объекты в JSON. Вы можете подгружать сохранённый файл в редактор сцен для дальнейшего редактирования. `SaveSceneFile` **заменяет** файл, а не дополняет его.

---

### LoadScene

Загружает сцену по имени (переключение между логическими сценами внутри проекта).

Сцена в BlazeBolt — это набор Lua-скриптов, которые работают вместе. При переключении сцены движок вызывает `On<SceneName>Load()` для старой сцены, а затем `On<NewScene>Load()` для новой.

```
BlazeBolt.LoadScene(sceneName) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `sceneName` | string | Имя сцены (не файл, а имя, заданное в настройках проекта) |

**Возвращает:** `success` (boolean).

**Для начинающих:** если вам нужно просто загрузить уровень из `.scene` файла — используйте `LoadSceneFile`. `LoadScene` используется для переключения между разделами игры (меню, игра, пауза):

```lua
-- Внутри файла сцены "menu"
function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.ENTER) then
        BlazeBolt.LoadScene("game")  -- переключиться на сцену "game"
    end
end
```

**Для профессионалов:** при `LoadScene` не происходит автоматической очистки объектов. Используйте `DestroyAll()` в `On<Scene>Unload()` при необходимости. Скрипты старой сцены продолжают работать, если не вызван `DestroyAll()`.

---

### GetCurrentScene

Возвращает имя текущей активной сцены.

```
BlazeBolt.GetCurrentScene() → sceneName
```

**Возвращает:** `sceneName` (string) — имя сцены, установленное через `LoadScene`.

Полезно для условной логики в `Update(dt)`:

```lua
function Update(dt)
    local scene = BlazeBolt.GetCurrentScene()
    if scene == "menu" then
        -- логика меню
    elseif scene == "game" then
        -- игровая логика
    end
end
```

---

### Пример: Полный цикл работы со сценами

```lua
-- Глобальные ссылки на объекты уровня
local objects = {}

function Start()
    -- Загружаем уровень из файла
    objects = BlazeBolt.LoadSceneFile("levels/level1.scene")
end

function Update(dt)
    local currentScene = BlazeBolt.GetCurrentScene()

    if currentScene == "menu" then
        if BlazeBolt.IsKeyJustPressed(Keys.ENTER) then
            BlazeBolt.LoadScene("game")
        end
    elseif currentScene == "game" then
        -- Игровая логика
        local hero = objects["player"]
        local healthText = objects["hud_health"]

        if hero and BlazeBolt.IsKeyJustPressed(Keys.ESCAPE) then
            -- Автосохранение
            if BlazeBolt.SaveSceneFile("autosave.scene") then
                BlazeBolt.Print("Game saved!")
            end
            BlazeBolt.LoadScene("menu")
        end
    end
end

function OnGameLoad()
    -- Этот коллбэк вызывается автоматически при загрузке сцены "game"
    objects = BlazeBolt.LoadSceneFile("levels/level1.scene")
end

function OnGameUnload()
    -- Очистка при выходе из сцены "game"
    BlazeBolt.DestroyAll()
    objects = {}
end
```

> **Для профессионалов:** мерж `.scene` файлов — вызывайте `LoadSceneFile` несколько раз подряд. Все объекты добавятся в один мир:
> ```lua
> BlazeBolt.LoadSceneFile("common/lighting.scene")  -- общее освещение
> BlazeBolt.LoadSceneFile("levels/level1.scene")     -- объекты уровня
> BlazeBolt.LoadSceneFile("levels/level1_dynamic.scene") -- динамические объекты
> ```
> Затем один `SaveSceneFile("save.scene")` сохранит всё вместе.**

---

## Формат .scene файлов (Scene File Format)

Файлы `.scene` хранят сцены в формате JSON. Каждый файл содержит массив объектов `objects`, где каждый объект описывает один элемент сцены.

### Структура корневого документа

```json
{
    "name": "my_level",
    "version": 1,
    "objects": [ ... ]
}
```

### Общие поля

Каждый объект в массиве `objects` может содержать:

| Поле | Тип | Описание |
|---|---|---|
| `name` | string | Уникальное имя объекта |
| `type` | string | Тип объекта (см. ниже) |
| `pos_x`, `pos_y` | number | Позиция в мировых координатах (Y-up) |
| `rot` | number | Поворот в градусах |

### Типы объектов

#### `sprite`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `texture` | string | — | Путь к текстуре |
| `texture_rect` | array[4] | `[0,0,1,1]` | UV-прямоугольник `[u, v, w, h]` |
| `size_x`, `size_y` | number | `64` | Размер спрайта |
| `origin_x`, `origin_y` | number | `0.5` | Точка привязки (0–1) |
| `color_r`, `color_g`, `color_b`, `color_a` | number | `1` | Цвет |
| `visible` | bool | `true` | Видимость |

#### `animated_sprite`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `texture` | string | — | Путь к текстуре анимации |
| `size_x`, `size_y` | number | `64` | Размер |
| `origin_x`, `origin_y` | number | `0.5` | Точка привязки |
| `color_r`, `color_g`, `color_b`, `color_a` | number | `1` | Цвет |
| `visible` | bool | `true` | Видимость |
| `looping` | bool | `false` | Зацикливание анимации |
| `playback_speed` | number | `1` | Скорость воспроизведения |
| `rot` | number | `0` | Поворот |

#### `text`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `font` | string | — | Путь к шрифту |
| `text` | string | — | Текст |
| `scale_x`, `scale_y` | number | `1` | Масштаб |
| `color_r`, `color_g`, `color_b`, `color_a` | number | `1` | Цвет |
| `visible` | bool | `true` | Видимость |
| `alignment` | number | `0` | Выравнивание (`0`=Left, `1`=Center, `2`=Right) |
| `rot` | number | `0` | Поворот |

#### `camera`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `zoom` | number | `1` | Зум камеры |
| `rot` | number | `0` | Поворот |

#### `point_light`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `color_r`, `color_g`, `color_b` | number | `1` | Цвет |
| `intensity` | number | `1` | Интенсивность |
| `radius` | number | `200` | Радиус |
| `enabled` | bool | `true` | Включён |

#### `ambient_light`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `color_r`, `color_g`, `color_b` | number | `1` | Цвет |
| `intensity` | number | `0.3` | Интенсивность |

#### `particle_system`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `texture` | string | — | Путь к текстуре частиц |
| `emission_rate` | number | — | Скорость эмиссии |
| `active` | bool | `true` | Активность |
| `visible` | bool | `true` | Видимость |

#### `tileset`

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `texture` | string | — | Путь к текстуре атласа |
| `tile_width`, `tile_height` | number | `32` | Размер тайла |
| `atlas_cols`, `atlas_rows` | number | `8` | Количество тайлов в атласе |

#### `physics_body`

Объект-коллайдер без визуального представления.

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `rot` | number | `0` | Поворот в градусах |

### Поля физики (Physics Body)

Любой объект (sprite, text, tileset и т.д.) может содержать поля физики. Для этого достаточно добавить `body_type`.

| Поле | Тип | По умолчанию | Описание |
|---|---|---|---|
| `body_type` | number | — | Тип тела: `0`=Static, `1`=Dynamic, `2`=Kinematic. Если отсутствует — физика не создаётся |
| `mass` | number | `1.0` | Масса |
| `friction` | number | `0.3` | Трение |
| `restitution` | number | `0.5` | Упругость (отскок) |
| `collider_shape` | string | `"circle"` | Форма коллайдера: `"circle"` или `"rectangle"` |
| `circle_radius` | number | `32.0` | Радиус круга |
| `circle_offset_x`, `circle_offset_y` | number | `0` | Смещение круга относительно объекта |
| `rect_half_width`, `rect_half_height` | number | `32.0` | Половина ширины/высоты прямоугольника |
| `gravity_scale` | number | `1.0` | Масштаб гравитации |
| `fixed_rotation` | bool | `false` | Заблокировать вращение |
| `bullet` | bool | `false` | Режим пули (непрерывное обнаружение) |

### Пример: спрайт с физикой

```json
{
    "objects": [
        {
            "name": "player",
            "type": "sprite",
            "texture": "player.png",
            "pos_x": 100,
            "pos_y": 200,
            "size_x": 64,
            "size_y": 64,
            "body_type": 1,
            "mass": 1.0,
            "friction": 0.3,
            "collider_shape": "circle",
            "circle_radius": 32
        },
        {
            "name": "wall",
            "type": "physics_body",
            "pos_x": 300,
            "pos_y": 0,
            "body_type": 0,
            "collider_shape": "rectangle",
            "rect_half_width": 200,
            "rect_half_height": 20
        }
    ]
}
```

---

## Ввод (Input)

### IsKeyPressed

Проверяет, зажата ли клавиша (удерживается).

```
BlazeBolt.IsKeyPressed(key) → pressed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `key` | integer | Код клавиши (из таблицы `Keys`) |

**Возвращает:** `pressed` (boolean).

---

### IsKeyJustPressed

Проверяет, была ли клавиша нажата в этом кадре.

```
BlazeBolt.IsKeyJustPressed(key) → justPressed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `key` | integer | Код клавиши |

**Возвращает:** `justPressed` (boolean).

---

### GetMouseX

Возвращает позицию мыши по X.

```
BlazeBolt.GetMouseX() → x
```

**Возвращает:** `x` (number).

---

### GetMouseY

Возвращает позицию мыши по Y.

```
BlazeBolt.GetMouseY() → y
```

**Возвращает:** `y` (number).

---

### GetMouseDeltaX

Возвращает смещение мыши по X за кадр.

```
BlazeBolt.GetMouseDeltaX() → dx
```

**Возвращает:** `dx` (number).

---

### GetMouseDeltaY

Возвращает смещение мыши по Y за кадр.

```
BlazeBolt.GetMouseDeltaY() → dy
```

**Возвращает:** `dy` (number).

---

### IsMouseButtonPressed

Проверяет, нажата ли кнопка мыши.

```
BlazeBolt.IsMouseButtonPressed(button) → pressed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `button` | integer | Код кнопки (из `MouseButtons`) |

**Возвращает:** `pressed` (boolean).

---

### IsMouseButtonJustPressed

Проверяет, была ли кнопка мыши нажата в этом кадре.

```
BlazeBolt.IsMouseButtonJustPressed(button) → justPressed
```

| Аргумент | Тип | Описание |
|---|---|---|
| `button` | integer | Код кнопки |

**Возвращает:** `justPressed` (boolean).

---

### GetScrollY

Возвращает значение колёсика прокрутки.

```
BlazeBolt.GetScrollY() → scrollY
```

**Возвращает:** `scrollY` (number).

---

### Пример: Ввод

```lua
local player
local speed = 200

function Start()
    player = BlazeBolt.CreateSprite("assets/player.png", 400, 300)
end

function Update(dt)
    local x, y = BlazeBolt.SpriteGetPosition(player)

    if BlazeBolt.IsKeyPressed(Keys.W) then y = y - speed * dt end
    if BlazeBolt.IsKeyPressed(Keys.S) then y = y + speed * dt end
    if BlazeBolt.IsKeyPressed(Keys.A) then x = x - speed * dt end
    if BlazeBolt.IsKeyPressed(Keys.D) then x = x + speed * dt end
    BlazeBolt.SpriteSetPosition(player, x, y)

    local mouseX = BlazeBolt.GetMouseX()
    local mouseY = BlazeBolt.GetMouseY()
    if BlazeBolt.IsMouseButtonJustPressed(MouseButtons.LEFT) then
        BlazeBolt.Print("Clicked at: ", mouseX, ", ", mouseY)
    end

    local scroll = BlazeBolt.GetScrollY()
    if scroll ~= 0 then
        BlazeBolt.Print("Scroll: ", scroll)
    end
end
```

---

## Математические типы (Math Types)

### Vector2

Двумерный вектор. Глобальный конструктор `Vector2(x, y)`.

#### Конструктор

```lua
local v = Vector2(3.0, 4.0)    -- x=3, y=4
local v2 = Vector2()           -- x=0, y=0
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Компонента X | `0` |
| `y` | number | Компонента Y | `0` |

**Возвращает:** `Vector2` (userdata)

#### Свойства

| Свойство | Тип | Описание |
|---|---|---|
| `v.x` | number | Компонента X |
| `v.y` | number | Компонента Y |

#### Методы

| Метод | Описание |
|---|---|
| `v:length()` → number | Длина вектора |
| `v:lengthSquared()` → number | Длина вектора в квадрате |
| `v:normalized()` → Vector2 | Возвращает нормализованный копию |
| `v:normalize()` | Нормализует вектор на месте |
| `v:dot(other)` → number | Скалярное произведение |
| `v:cross(other)` → number | Векторное произведение (возврат Z-компоненты) |
| `v:clone()` → Vector2 | Создаёт копию |

#### Операторы

| Оператор | Описание |
|---|---|
| `a + b` | Сложение |
| `a - b` | Вычитание |
| `a * scalar` | Умножение на скаляр |
| `scalar * a` | Умножение на скаляр |
| `a / scalar` | Деление на скаляр |
| `-a` | Унарный минус |
| `a == b` | Сравнение |

---

### Vector3

Трёхмерный вектор. Глобальный конструктор `Vector3(x, y, z)`.

#### Конструктор

```lua
local v = Vector3(1.0, 2.0, 3.0)
local v2 = Vector3()           -- x=0, y=0, z=0
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Компонента X | `0` |
| `y` | number | Компонента Y | `0` |
| `z` | number | Компонента Z | `0` |

**Возвращает:** `Vector3` (userdata)

#### Свойства

| Свойство | Тип | Описание |
|---|---|---|
| `v.x` | number | Компонента X |
| `v.y` | number | Компонента Y |
| `v.z` | number | Компонента Z |

#### Методы

| Метод | Описание |
|---|---|
| `v:length()` → number | Длина вектора |
| `v:lengthSquared()` → number | Длина в квадрате |
| `v:normalized()` → Vector3 | Возвращает нормализованную копию |
| `v:normalize()` | Нормализует вектор на месте |
| `v:dot(other)` → number | Скалярное произведение |
| `v:cross(other)` → Vector3 | Векторное произведение |
| `v:clone()` → Vector3 | Создаёт копию |

#### Операторы

| Оператор | Описание |
|---|---|
| `a + b` | Сложение |
| `a - b` | Вычитание |
| `a * scalar` | Умножение на скаляр |
| `scalar * a` | Умножение на скаляр |
| `a / scalar` | Деление на скаляр |
| `-a` | Унарный минус |
| `a == b` | Сравнение |

---

### Vector4

Четырёхмерный вектор. Глобальный конструктор `Vector4(x, y, z, w)`.

#### Конструктор

```lua
local v = Vector4(1.0, 2.0, 3.0, 4.0)
local v2 = Vector4()           -- x=0, y=0, z=0, w=0
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Компонента X | `0` |
| `y` | number | Компонента Y | `0` |
| `z` | number | Компонента Z | `0` |
| `w` | number | Компонента W | `0` |

**Возвращает:** `Vector4` (userdata)

#### Свойства

| Свойство | Тип | Описание |
|---|---|---|
| `v.x` | number | Компонента X |
| `v.y` | number | Компонента Y |
| `v.z` | number | Компонента Z |
| `v.w` | number | Компонента W |

#### Методы

| Метод | Описание |
|---|---|
| `v:length()` → number | Длина вектора |
| `v:lengthSquared()` → number | Длина в квадрате |
| `v:normalized()` → Vector4 | Возвращает нормализованную копию |
| `v:normalize()` | Нормализует вектор на месте |
| `v:dot(other)` → number | Скалярное произведение |
| `v:toVector3()` → Vector3 | Конвертирует в Vector3 (отбрасывает w) |
| `v:clone()` → Vector4 | Создаёт копию |

#### Операторы

| Оператор | Описание |
|---|---|
| `a + b` | Сложение |
| `a - b` | Вычитание |
| `a * scalar` | Умножение на скаляр |
| `scalar * a` | Умножение на скаляр |
| `a / scalar` | Деление на скаляр |
| `-a` | Унарный минус |
| `a == b` | Сравнение |

---

### Matrix3x3

Матрица 3x3. Глобальный конструктор `Matrix3x3()`.

#### Конструктор

```lua
local m = Matrix3x3()          -- единичная матрица
```

**Возвращает:** `Matrix3x3` (userdata)

#### Доступ к элементам

```lua
m[1]  -- элемент [0][0] (первая колонка, первый ряд)
m[5]  -- элемент [1][1] (вторая колонка, второй ряд)
m[9]  -- элемент [2][2] (третья колонка, третий ряд)
```

Индексация от 1 до 9. Формула: `индекс = колонка * 3 + ряд + 1`.

#### Методы

| Метод | Описание |
|---|---|
| `m:get(col, row)` → number | Получить элемент (col и row от 1 до 3) |
| `m:set(col, row, val)` | Установить элемент |
| `m:toArray()` → table | Конвертировать в плоский массив из 9 элементов |
| `m:clone()` → Matrix3x3 | Создать копию |
| `m:identity()` → Matrix3x3 | Создать единичную матрицу |
| `m:translation(x, y)` → Matrix3x3 | Матрица переноса |
| `m:scale(x, y)` → Matrix3x3 | Матрица масштабирования |
| `m:rotation(degrees)` → Matrix3x3 | Матрица поворота (в градусах) |

#### Операторы

| Оператор | Описание |
|---|---|
| `a * b` | Умножение матриц |
| `a == b` | Сравнение |

---

## Шумы (Noise)

Генераторы процедурного шума. Все функции — глобальные (не в таблице `BlazeBolt`).

### SetNoiseSeed

Устанавливает зерно генератора шума.

```
SetNoiseSeed(seed)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `seed` | integer | Зерно (seed) |

**Возвращает:** ничего.

---

### PerlinNoise1D

Генерирует 1D шум Перлина.

```
PerlinNoise1D(x) → value
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | number | Координата |

**Возвращает:** `value` (number, от -1 до 1).

---

### PerlinNoise2D

Генерирует 2D шум Перлина.

```
PerlinNoise2D(x, y) → value
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | number | Координата X |
| `y` | number | Координата Y |

**Возвращает:** `value` (number, от -1 до 1).

---

### PerlinNoise3D

Генерирует 3D шум Перлина.

```
PerlinNoise3D(x, y, z) → value
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | number | Координата X |
| `y` | number | Координата Y |
| `z` | number | Координата Z |

**Возвращает:** `value` (number, от -1 до 1).

---

### SimplexNoise2D

Генерирует 2D Simplex шум.

```
SimplexNoise2D(x, y) → value
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | number | Координата X |
| `y` | number | Координата Y |

**Возвращает:** `value` (number, от -1 до 1).

---

### ValueNoise2D

Генерирует 2D Value шум.

```
ValueNoise2D(x, y) → value
```

| Аргумент | Тип | Описание |
|---|---|---|
| `x` | number | Координата X |
| `y` | number | Координата Y |

**Возвращает:** `value` (number, от -1 до 1).

---

### FbmNoise2D

Генерирует 2D FBM (Fractional Brownian Motion) шум на основе шума Перлина.

```
FbmNoise2D(x, y, [octaves], [lacunarity], [gain]) → value
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Координата X | — |
| `y` | number | Координата Y | — |
| `octaves` | integer | Количество октав | `6` |
| `lacunarity` | number | Лакунарность | `2.0` |
| `gain` | number | Коэффициент затухания | `0.5` |

**Возвращает:** `value` (number).

---

### FbmSimplexNoise2D

Генерирует 2D FBM шум на основе Simplex шума.

```
FbmSimplexNoise2D(x, y, [octaves], [lacunarity], [gain]) → value
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Координата X | — |
| `y` | number | Координата Y | — |
| `octaves` | integer | Количество октав | `6` |
| `lacunarity` | number | Лакунарность | `2.0` |
| `gain` | number | Коэффициент затухания | `0.5` |

**Возвращает:** `value` (number).

---

### DomainWarpNoise2D

Генерирует 2D шум с доменным искажением (domain warping).

```
DomainWarpNoise2D(x, y, [warpScale]) → value
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `x` | number | Координата X | — |
| `y` | number | Координата Y | — |
| `warpScale` | number | Масштаб искажения | `1.0` |

**Возвращает:** `value` (number).

---

### Пример: Шумы

```lua
local mesh

function Start()
    SetNoiseSeed(42)

    mesh = BlazeBolt.CreateMesh()

    local w = GetScreenWidth()
    local h = GetScreenHeight()

    local vertices = {
        { x = 0, y = 0, u = 0, v = 0 },
        { x = w, y = 0, u = 1, v = 0 },
        { x = w, y = h, u = 1, v = 1 },
        { x = 0, y = h, u = 0, v = 1 },
    }
    local indices = { 0, 1, 2, 0, 2, 3 }

    BlazeBolt.MeshSetData(mesh, vertices, indices)
    BlazeBolt.MeshSetShader(mesh, "shaders/terrain.vert", "shaders/terrain.frag")
end

function Update(dt)
    local time = BlazeBolt.GetTime()
    for i = 0, 100 do
        local x = i * 0.1
        local val = FbmNoise2D(x + time, 0, 6, 2.0, 0.5)
        BlazeBolt.MeshSetUniformFloat(mesh, "heights[" .. i .. "]", val)
    end
end

function Draw()
    BlazeBolt.MeshDraw(mesh)
end
```

---

## Утилиты (Utility)

### Print

Выводит значения в консоль (аналог print, но с поддержкой всех типов).

```
BlazeBolt.Print(...)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `...` | any | Произвольные аргументы (string, number, boolean, nil) |

**Возвращает:** ничего.

---

### GetDeltaTime

Возвращает время с прошлого кадра.

```
BlazeBolt.GetDeltaTime() → dt
```

**Возвращает:** `dt` (number, секунды).

---

### GetTime

Возвращает общее время с момента запуска.

```
BlazeBolt.GetTime() → time
```

**Возвращает:** `time` (number, секунды).

---

### Random

Генерирует случайное число с плавающей точкой.

```
BlazeBolt.Random([min], [max]) → value
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `min` | number | Минимальное значение | `0` |
| `max` | number | Максимальное значение | `1` |

**Возвращает:** `value` (number).

---

### RandomInt

Генерирует случайное целое число.

```
BlazeBolt.RandomInt(min, max) → value
```

| Аргумент | Тип | Описание |
|---|---|---|
| `min` | integer | Минимальное значение (включительно) |
| `max` | integer | Максимальное значение (включительно) |

**Возвращает:** `value` (integer).

---

### SetRandomSeed

Устанавливает зерно генератора случайных чисел.

```
BlazeBolt.SetRandomSeed(seed)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `seed` | integer | Зерно |

**Возвращает:** ничего.

---

### Quit

Завершает работу программы.

```
BlazeBolt.Quit()
```

**Возвращает:** ничего.

---

### AddConsoleMessage

Добавляет сообщение в консоль движка.

```
BlazeBolt.AddConsoleMessage(msg, [type])
```

| Аргумент | Тип | Описание | По умолчанию |
|---|---|---|---|
| `msg` | string | Текст сообщения | — |
| `type` | integer | Тип сообщения (0=обычное) | `0` |

**Возвращает:** ничего.

---

### Пример: Утилиты

```lua
local frameCount = 0
local fpsTimer = 0

function Update(dt)
    frameCount = frameCount + 1
    fpsTimer = fpsTimer + dt

    if fpsTimer >= 1.0 then
        BlazeBolt.Print("FPS: ", frameCount)
        BlazeBolt.AddConsoleMessage("FPS: " .. frameCount, 0)
        frameCount = 0
        fpsTimer = 0
    end

    local dt2 = BlazeBolt.GetDeltaTime()
    local time = BlazeBolt.GetTime()

    BlazeBolt.SetRandomSeed(12345)
    local r1 = BlazeBolt.Random(0, 100)
    local r2 = BlazeBolt.RandomInt(1, 10)
    BlazeBolt.Print("Random: ", r1, ", ", r2)
end
```

---

## Сетевые функции (Networking)

Сетевые функции для создания TCP/UDP серверов и клиентов.

### NetInit

Инициализирует сетевую подсистему.

```
BlazeBolt.NetInit()
```

**Возвращает:** ничего.

---

### NetShutdown

Завершает сетевую подсистему.

```
BlazeBolt.NetShutdown()
```

**Возвращает:** ничего.

---

#### TCP Server

### CreateTCPServer

Создаёт TCP-сервер, слушающий на указанном порту.

```
BlazeBolt.CreateTCPServer(port) → serverId
```

| Аргумент | Тип | Описание |
|---|---|---|
| `port` | integer | Порт для прослушивания |

**Возвращает:** `serverId` (integer).

---

### TCPServerStop

Останавливает TCP-сервер.

```
BlazeBolt.TCPServerStop(serverId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** ничего.

---

### TCPServerIsRunning

Проверяет, запущен ли TCP-сервер.

```
BlazeBolt.TCPServerIsRunning(serverId) → running
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** `running` (boolean).

---

### TCPServerPoll

Обрабатывает сетевые события сервера.

```
BlazeBolt.TCPServerPoll(serverId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** ничего.

---

### TCPServerAccept

Принимает входящее TCP-подключение.

```
BlazeBolt.TCPServerAccept(serverId) → clientId, addr, port
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** `clientId` (integer), `addr` (string), `port` (integer).

---

### TCPServerSend

Отправляет данные подключённому клиенту.

```
BlazeBolt.TCPServerSend(serverId, clientId, data)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `clientId` | integer | Идентификатор клиента |
| `data` | string | Данные для отправки |

**Возвращает:** ничего.

---

### TCPServerBroadcast

Отправляет данные всем подключённым клиентам.

```
BlazeBolt.TCPServerBroadcast(serverId, data)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `data` | string | Данные для отправки |

**Возвращает:** ничего.

---

### TCPServerReceive

Получает данные от клиента.

```
BlazeBolt.TCPServerReceive(serverId, clientId) → data
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** `data` (string).

---

### TCPServerDisconnect

Отключает клиента от сервера.

```
BlazeBolt.TCPServerDisconnect(serverId, clientId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** ничего.

---

### TCPServerGetClientCount

Возвращает количество подключённых клиентов.

```
BlazeBolt.TCPServerGetClientCount(serverId) → count
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** `count` (integer).

---

### TCPServerIsClientConnected

Проверяет, подключён ли указанный клиент.

```
BlazeBolt.TCPServerIsClientConnected(serverId, clientId) → connected
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** `connected` (boolean).

---

#### TCP Client

### CreateTCPClient

Создаёт TCP-клиент.

```
BlazeBolt.CreateTCPClient() → clientId
```

**Возвращает:** `clientId` (integer).

---

### TCPClientConnect

Подключается к TCP-серверу.

```
BlazeBolt.TCPClientConnect(clientId, host, port) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |
| `host` | string | Адрес сервера |
| `port` | integer | Порт сервера |

**Возвращает:** `success` (boolean).

---

### TCPClientSend

Отправляет данные серверу.

```
BlazeBolt.TCPClientSend(clientId, data)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |
| `data` | string | Данные |

**Возвращает:** ничего.

---

### TCPClientReceive

Получает данные от сервера.

```
BlazeBolt.TCPClientReceive(clientId) → data
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** `data` (string).

---

### TCPClientDisconnect

Отключается от сервера.

```
BlazeBolt.TCPClientDisconnect(clientId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** ничего.

---

### TCPClientIsConnected

Проверяет, подключён ли клиент.

```
BlazeBolt.TCPClientIsConnected(clientId) → connected
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** `connected` (boolean).

---

#### UDP Server

### CreateUDPServer

Создаёт UDP-сервер.

```
BlazeBolt.CreateUDPServer(port) → serverId
```

| Аргумент | Тип | Описание |
|---|---|---|
| `port` | integer | Порт для прослушивания |

**Возвращает:** `serverId` (integer).

---

### UDPServerStop

Останавливает UDP-сервер.

```
BlazeBolt.UDPServerStop(serverId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** ничего.

---

### UDPServerIsRunning

Проверяет, запущен ли UDP-сервер.

```
BlazeBolt.UDPServerIsRunning(serverId) → running
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** `running` (boolean).

---

### UDPServerPoll

Обрабатывает сетевые события.

```
BlazeBolt.UDPServerPoll(serverId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** ничего.

---

### UDPServerSend

Отправляет данные пиру.

```
BlazeBolt.UDPServerSend(serverId, peerId, data)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `peerId` | integer | Идентификатор пира |
| `data` | string | Данные |

**Возвращает:** ничего.

---

### UDPServerReceive

Получает данные от конкретного пира.

```
BlazeBolt.UDPServerReceive(serverId, peerId) → data
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `peerId` | integer | Идентификатор пира |

**Возвращает:** `data` (string).

---

### UDPServerReceiveAny

Получает данные от любого пира.

```
BlazeBolt.UDPServerReceiveAny(serverId) → peerId, data
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** `peerId` (integer), `data` (string).

---

### UDPServerRemovePeer

Удаляет пира из таблицы.

```
BlazeBolt.UDPServerRemovePeer(serverId, peerId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `peerId` | integer | Идентификатор пира |

**Возвращает:** ничего.

---

### UDPServerGetPeerCount

Возвращает количество известных пиров.

```
BlazeBolt.UDPServerGetPeerCount(serverId) → count
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |

**Возвращает:** `count` (integer).

---

### UDPServerIsPeerKnown

Проверяет, известен ли пир.

```
BlazeBolt.UDPServerIsPeerKnown(serverId, peerId) → known
```

| Аргумент | Тип | Описание |
|---|---|---|
| `serverId` | integer | Идентификатор сервера |
| `peerId` | integer | Идентификатор пира |

**Возвращает:** `known` (boolean).

---

#### UDP Client

### CreateUDPClient

Создаёт UDP-клиент.

```
BlazeBolt.CreateUDPClient() → clientId
```

**Возвращает:** `clientId` (integer).

---

### UDPClientConnect

Подключается к UDP-серверу.

```
BlazeBolt.UDPClientConnect(clientId, host, port) → success
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |
| `host` | string | Адрес сервера |
| `port` | integer | Порт сервера |

**Возвращает:** `success` (boolean).

---

### UDPClientSend

Отправляет данные серверу.

```
BlazeBolt.UDPClientSend(clientId, data)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |
| `data` | string | Данные |

**Возвращает:** ничего.

---

### UDPClientReceive

Получает данные от сервера.

```
BlazeBolt.UDPClientReceive(clientId) → data
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** `data` (string).

---

### UDPClientDisconnect

Отключается от сервера.

```
BlazeBolt.UDPClientDisconnect(clientId)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** ничего.

---

### UDPClientIsConnected

Проверяет, подключён ли клиент.

```
BlazeBolt.UDPClientIsConnected(clientId) → connected
```

| Аргумент | Тип | Описание |
|---|---|---|
| `clientId` | integer | Идентификатор клиента |

**Возвращает:** `connected` (boolean).

---

### Пример: Сетевые функции (TCP)

```lua
local server
local client

function Start()
    BlazeBolt.NetInit()

    server = BlazeBolt.CreateTCPServer(8080)
    client = BlazeBolt.CreateTCPClient()

    local success = BlazeBolt.TCPClientConnect(client, "127.0.0.1", 8080)
    BlazeBolt.Print("Connected: ", tostring(success))
end

function Update(dt)
    BlazeBolt.TCPServerPoll(server)

    local clientId, addr, port = BlazeBolt.TCPServerAccept(server)
    if clientId then
        BlazeBolt.Print("Client connected from " .. addr .. ":" .. tostring(port))
        BlazeBolt.TCPServerBroadcast(server, "Hello everyone!")
    end

    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.TCPClientSend(client, "Hello from client!")
    end

    local data = BlazeBolt.TCPClientReceive(client)
    if data then
        BlazeBolt.Print("Client received: " .. data)
    end

    local count = BlazeBolt.TCPServerGetClientCount(server)
    BlazeBolt.Print("Connected clients: ", count)
end

function End()
    BlazeBolt.TCPServerStop(server)
    BlazeBolt.TCPClientDisconnect(client)
    BlazeBolt.NetShutdown()
end
```

---

### Пример: Сетевые функции (UDP)

```lua
local server
local client

function Start()
    BlazeBolt.NetInit()

    server = BlazeBolt.CreateUDPServer(9090)
    client = BlazeBolt.CreateUDPClient()
    BlazeBolt.UDPClientConnect(client, "127.0.0.1", 9090)
end

function Update(dt)
    BlazeBolt.UDPServerPoll(server)

    if BlazeBolt.IsKeyJustPressed(Keys.SPACE) then
        BlazeBolt.UDPClientSend(client, "ping")
    end

    local peerId, data = BlazeBolt.UDPServerReceiveAny(server)
    if peerId and data then
        BlazeBolt.Print("Server received from peer " .. peerId .. ": " .. data)
        BlazeBolt.UDPServerSend(server, peerId, "pong")
    end

    local response = BlazeBolt.UDPClientReceive(client)
    if response then
        BlazeBolt.Print("Client received: " .. response)
    end
end

function End()
    BlazeBolt.UDPServerStop(server)
    BlazeBolt.UDPClientDisconnect(client)
    BlazeBolt.NetShutdown()
end
```

---

## Порядок рендера (Render Order)

### SetRenderOrder

Устанавливает порядок рендера подсистем.

```
BlazeBolt.SetRenderOrder(order)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `order` | table | Массив строк: `{"Sprites", "Text", "Particles", "Mesh"}` |

**Возвращает:** ничего.

---

### GetRenderOrder

Возвращает текущий порядок рендера.

```
BlazeBolt.GetRenderOrder() → order
```

**Возвращает:** `order` (table) — массив строк.

---

### Пример: Порядок рендера

```lua
function Start()
    local currentOrder = BlazeBolt.GetRenderOrder()
    BlazeBolt.Print("Current render order:")
    for i, layer in ipairs(currentOrder) do
        BlazeBolt.Print("  " .. i .. ": " .. layer)
    end

    BlazeBolt.SetRenderOrder({"Mesh", "Particles", "Sprites", "Text"})
    BlazeBolt.Print("Render order updated!")
end
```

---

## Удаление сущностей

### Destroy

Удаляет сущность по идентификатору.

```
BlazeBolt.Destroy(entity)
```

| Аргумент | Тип | Описание |
|---|---|---|
| `entity` | integer | Идентификатор сущности |

**Возвращает:** ничего.

---

### DestroyAll

Удаляет все сущности.

```
BlazeBolt.DestroyAll()
```

**Возвращает:** ничего.

---

### Пример: Удаление сущностей

```lua
local entities = {}

function Start()
    for i = 1, 10 do
        local e = BlazeBolt.CreateSprite("assets/star.png", i * 50, 300)
        table.insert(entities, e)
    end
end

function Update(dt)
    if BlazeBolt.IsKeyJustPressed(Keys.ONE) then
        if #entities > 0 then
            local e = table.remove(entities)
            BlazeBolt.Destroy(e)
            BlazeBolt.Print("Destroyed entity: " .. e)
        end
    end

    if BlazeBolt.IsKeyJustPressed(Keys.TWO) then
        BlazeBolt.DestroyAll()
        entities = {}
        BlazeBolt.Print("All entities destroyed!")
    end
end
```

---

## Глобальные функции

| Функция | Описание |
|---|---|
| `GetScreenWidth()` → integer | Возвращает ширину текущего окна |
| `GetScreenHeight()` → integer | Возвращает высоту текущего окна |

---

## Константы (Constants)

### TextAlignment

| Константа | Значение | Описание |
|---|---|---|
| `TextAlignment.LEFT` | 0 | Выравнивание по левому краю |
| `TextAlignment.CENTER` | 1 | Выравнивание по центру |
| `TextAlignment.RIGHT` | 2 | Выравнивание по правому краю |

### MouseButtons

| Константа | Значение | Описание |
|---|---|---|
| `MouseButtons.LEFT` | 0 | Левая кнопка мыши |
| `MouseButtons.RIGHT` | 1 | Правая кнопка мыши |
| `MouseButtons.MIDDLE` | 2 | Средняя кнопка мыши |

### Keys

Полная таблица клавиш (аналоги GLFW). Основные:

| Константа | Описание |
|---|---|
| `Keys.A` – `Keys.Z` | Буквы |
| `Keys.ZERO` – `Keys.NINE` | Цифры |
| `Keys.SPACE` | Пробел |
| `Keys.ENTER` | Enter |
| `Keys.ESCAPE` | Escape |
| `Keys.LEFT_SHIFT` | Левый Shift |
| `Keys.RIGHT_SHIFT` | Правый Shift |
| `Keys.LEFT_CONTROL` | Левый Ctrl |
| `Keys.RIGHT_CONTROL` | Правый Ctrl |
| `Keys.UP`, `Keys.DOWN`, `Keys.LEFT`, `Keys.RIGHT` | Стрелки |
| `Keys.F1` – `Keys.F25` | Функциональные клавиши |
| `Keys.BACKSPACE` | Backspace |
| `Keys.TAB` | Tab |
| `Keys.CAPS_LOCK` | Caps Lock |
| `Keys.PLUS` | Плюс |
| `Keys.MINUS` | Минус |

---

## Управление графическим API (Graphics API)

Начиная с версии 1.3, BlazeBolt поддерживает переключение между OpenGL и Vulkan во время выполнения.

### BlazeBolt.SetGraphicsAPI(api)

Устанавливает предпочитаемый графический API.

**Параметры:**
| Имя | Тип | Описание |
|---|---|---|
| `api` | `string` | `"opengl"` или `"vulkan"` |

**Пример:**
```lua
BlazeBolt.SetGraphicsAPI("vulkan")
```

### BlazeBolt.GetGraphicsAPI()

Возвращает текущий выбранный API.

**Возвращает:** `string` — `"opengl"` или `"vulkan"`

**Пример:**
```lua
local current = BlazeBolt.GetGraphicsAPI()
print("Current API: " .. current)
```

**Примечание:** выбор API сохраняется в файле `.BlazeBoltProject` и применяется при следующем запуске игры через редактор.

---

## OOP-стиль (Object-Oriented Wrappers)

Начиная с версии 1.3, все типы объектов имеют **OOP-обёртки** с методами. Вместо:

```lua
local id = BlazeBolt.CreateSprite("tex.png", 0, 0)
BlazeBolt.SpriteSetPosition(id, 100, 200)
BlazeBolt.SpriteSetSize(id, 64, 64)
```

Вы можете писать:

```lua
local spr = BlazeBolt.Sprite.new("tex.png", 0, 0)
spr:SetPosition(100, 200)
spr:SetSize(64, 64)
```

Все обёртки являются лёгкими таблицами с метатаблицами. Они хранят внутри Entity ID (поле `.id`) и передают его в соответствующие C-функции при вызове методов.

### BlazeBolt.Sprite

| Метод | Описание |
|---|---|
| `Sprite.new(texturePath, x, y)` | Создать спрайт. Возвращает объект Sprite |
| `:SetTexture(path)` | Установить текстуру |
| `:SetPosition(x, y)` | Установить позицию |
| `:GetPosition()` | Получить позицию (x, y) |
| `:SetSize(w, h)` | Установить размер |
| `:GetSize()` | Получить размер (w, h) |
| `:SetOrigin(x, y)` | Установить точку опоры |
| `:GetOrigin()` | Получить точку опоры (x, y) |
| `:SetRotation(rot)` | Установить поворот (градусы) |
| `:GetRotation()` | Получить поворот |
| `:SetColor(r, g, b, a)` | Установить цвет (0..1) |
| `:GetColor()` | Получить цвет (r, g, b, a) |
| `:SetTextureRect(x, y, w, h)` | Установить область текстуры |
| `:SetVisible(bool)` | Показать/скрыть |
| `:IsVisible()` | Проверить видимость |
| `:Destroy()` | Уничтожить объект |

### BlazeBolt.AnimatedSprite

| Метод | Описание |
|---|---|
| `AnimatedSprite.new(texturePath, x, y)` | Создать анимированный спрайт |
| `:Play()` | Запустить анимацию |
| `:IsPlaying()` | Проверить, проигрывается ли |
| `:Pause()` | Поставить на паузу |
| `:Stop()` | Остановить |
| `:Restart()` | Перезапустить |
| `:SetTexture(path)` | Установить текстуру |
| `:SetLooping(bool)` | Зациклить анимацию |
| `:IsLooping()` | Проверить зацикленность |
| `:SetPlaybackSpeed(speed)` | Скорость воспроизведения |
| `:GetPlaybackSpeed()` | Получить скорость |
| `:SetFrame(frame)` | Установить кадр |
| `:GetCurrentFrame()` | Текущий кадр |
| `:GetNumFrames()` | Количество кадров |
| `:SetPosition(x, y)` | Позиция |
| `:GetPosition()` | Получить позицию |
| `:SetSize(w, h)` | Размер |
| `:GetSize()` | Получить размер |
| `:SetOrigin(x, y)` | Точка опоры |
| `:GetOrigin()` | Получить точку опоры |
| `:SetRotation(rot)` | Поворот |
| `:GetRotation()` | Получить поворот |
| `:SetColor(r, g, b, a)` | Цвет |
| `:GetColor()` | Получить цвет |
| `:Destroy()` | Уничтожить |

### BlazeBolt.Text

| Метод | Описание |
|---|---|
| `Text.new(fontPath, text, x, y)` | Создать текст |
| `:SetString(text)` | Установить содержимое |
| `:GetString()` | Получить содержимое |
| `:SetPosition(x, y)` | Позиция |
| `:GetPosition()` | Получить позицию |
| `:SetColor(r, g, b, a)` | Цвет |
| `:GetColor()` | Получить цвет |
| `:SetScale(sx, sy)` | Масштаб |
| `:GetScale()` | Получить масштаб |
| `:SetOrigin(x, y)` | Точка опоры |
| `:GetOrigin()` | Получить точку опоры |
| `:SetRotation(rot)` | Поворот |
| `:GetRotation()` | Получить поворот |
| `:SetAlignment(align)` | Выравнивание (`TextAlignment.LEFT`, `CENTER`, `RIGHT`) |
| `:GetAlignment()` | Получить выравнивание |
| `:SetVisible(bool)` | Видимость |
| `:IsVisible()` | Проверить видимость |
| `:Destroy()` | Уничтожить |

### BlazeBolt.Camera

| Метод | Описание |
|---|---|
| `Camera.new()` | Создать камеру |
| `:SetPosition(x, y)` | Позиция |
| `:GetPosition()` | Получить позицию |
| `:SetZoom(zoom)` | Масштаб |
| `:GetZoom()` | Получить масштаб |
| `:SetRotation(rot)` | Поворот |
| `:GetRotation()` | Получить поворот |

### BlazeBolt.Tileset

| Метод | Описание |
|---|---|
| `Tileset.new(texturePath, tileW, tileH, atlasCols, atlasRows)` | Создать тайлсет |
| `:SetMap(map)` | Установить карту (таблица таблиц) |
| `:GetTile(col, row)` | Получить индекс тайла |
| `:SetTile(col, row, idx)` | Установить тайл |
| `:SetTileSize(w, h)` | Размер тайла |
| `:GetTileSize()` | Получить размер (w, h) |
| `:SetPosition(x, y)` | Позиция |
| `:GetPosition()` | Получить позицию |
| `:GetMapWidth()` | Ширина карты |
| `:GetMapHeight()` | Высота карты |
| `:GetTileCount()` | Количество тайлов |
| `:Draw()` | Принудительная отрисовка |
| `:Destroy()` | Уничтожить |

### BlazeBolt.ParticleSystem

| Метод | Описание |
|---|---|
| `ParticleSystem.new()` | Создать систему частиц |
| `:SetPosition(x, y)` | Позиция эмиттера |
| `:SetTexture(path)` | Текстура частицы |
| `:SetEmissionRate(rate)` | Скорость эмиссии |
| `:GetEmissionRate()` | Получить скорость |
| `:SetLifetime(min, max)` | Время жизни |
| `:SetSpeed(min, max)` | Скорость |
| `:SetSize(min, max)` | Размер |
| `:SetEndSize(min, max)` | Конечный размер |
| `:SetColor(r, g, b, a, [endR, endG, endB, endA])` | Цвет (стартовый и опционально конечный) |
| `:SetDirection(minAngle, maxAngle)` | Направление (градусы) |
| `:SetRotationSpeed(speed)` | Скорость вращения |
| `:SetActive(bool)` | Вкл/Выкл |
| `:IsActive()` | Проверить активность |
| `:SetVisible(bool)` | Видимость |
| `:IsVisible()` | Проверить видимость |
| `:Emit(count)` | Эмитировать N частиц |
| `:Clear()` | Очистить все частицы |
| `:GetCount()` | Количество частиц |

### BlazeBolt.Light

| Метод | Описание |
|---|---|
| `Light.newPoint(x, y, r, g, b, intensity, radius)` | Создать точечный источник |
| `Light.newAmbient(r, g, b, intensity)` | Создать фоновый свет |
| `:SetPosition(x, y)` | Позиция |
| `:GetPosition()` | Получить позицию |
| `:SetColor(r, g, b)` | Цвет |
| `:GetColor()` | Получить цвет |
| `:SetIntensity(intensity)` | Интенсивность |
| `:GetIntensity()` | Получить интенсивность |
| `:SetRadius(radius)` | Радиус |
| `:GetRadius()` | Получить радиус |
| `:SetEnabled(bool)` | Вкл/Выкл |
| `:GetEnabled()` | Получить состояние |
| `:Destroy()` | Уничтожить |

### BlazeBolt.Mesh

| Метод | Описание |
|---|---|
| `Mesh.new()` | Создать меш |
| `:SetData(vertices, indices)` | Установить вершины и индексы |
| `:SetShader(vertexPath, fragmentPath)` | Шейдер |
| `:SetUniformFloat(name, value)` | Uniform float |
| `:SetUniformInt(name, value)` | Uniform int |
| `:SetUniformVec2(name, x, y)` | Uniform vec2 |
| `:SetUniformVec3(name, x, y, z)` | Uniform vec3 |
| `:SetUniformVec4(name, x, y, z, w)` | Uniform vec4 |
| `:Draw()` | Нарисовать |

### BlazeBolt.SpriteBatch

| Метод | Описание |
|---|---|
| `SpriteBatch.new(maxSize)` | Создать батч (maxSize — макс. спрайтов) |
| `:SetTexture(path)` | Текстура батча |
| `:Add(sprite)` | Добавить спрайт (объект или ID) |
| `:Remove(sprite)` | Удалить спрайт |
| `:Clear()` | Очистить |
| `:SetMaxSize(n)` | Макс. размер |
| `:GetMaxSize()` | Получить макс. размер |
| `:GetCount()` | Количество спрайтов |
| `:Draw()` | Нарисовать |
| `:Destroy()` | Уничтожить |

### BlazeBolt.AnimationWheel

| Метод | Описание |
|---|---|
| `AnimationWheel.new(animatedSprite)` | Создать колесо состояний для анимированного спрайта |
| `:AddState(name, gifPath, speed, loop)` | Добавить состояние |
| `:RemoveState(name)` | Удалить состояние |
| `:HasState(name)` | Проверить существование |
| `:SetInitialState(name)` | Начальное состояние |
| `:GetInitialState()` | Получить начальное состояние |
| `:SetState(name)` | Переключить состояние |
| `:GetState()` | Текущее состояние |
| `:SetPlaybackSpeed(state, speed)` | Скорость для состояния |
| `:GetPlaybackSpeed(state)` | Получить скорость |
| `:SetLooping(state, loop)` | Зациклить состояние |
| `:IsLooping(state)` | Проверить зацикленность |
| `:Destroy()` | Уничтожить |

### BlazeBolt.Physics (пространство имён)

| Метод | Описание |
|---|---|
| `Physics.Init(gx, gy)` | Инициализировать физику (gravity) |
| `Physics.SetGravity(x, y)` | Установить гравитацию |
| `Physics.GetGravity()` | Получить гравитацию (x, y) |
| `Physics.CreateBody(type, x, y, mass, friction, restitution)` | Создать тело |
| `Physics.AddCircle(body, radius, ox, oy)` | Добавить круглый коллайдер |
| `Physics.AddRectangle(body, hw, hh)` | Добавить прямоугольный коллайдер |
| `Physics.SetLinearVelocity(body, vx, vy)` | Линейная скорость |
| `Physics.GetLinearVelocity(body)` | Получить скорость (vx, vy) |
| `Physics.Step()` | Шаг физики |
| `Physics.SyncSprite(body, sprite)` | Синхронизировать спрайт с телом |
| `Physics.SyncText(body, text)` | Синхронизировать текст с телом |

### BlazeBolt.Audio (пространство имён)

| Метод | Описание |
|---|---|
| `Audio.LoadSound(filename, name, loop)` | Загрузить звук |
| `Audio.Play(name)` | Воспроизвести |
| `Audio.PlayById(id)` | Воспроизвести по ID |
| `Audio.Stop(name)` | Остановить |
| `Audio.StopAll()` | Остановить все |
| `Audio.SetVolume(name, vol)` | Громкость (0..1) |
| `Audio.IsPlaying(name)` | Проверить воспроизведение |

### BlazeBolt.Input (пространство имён)

| Метод | Описание |
|---|---|
| `Input.IsKeyPressed(key)` | Клавиша зажата |
| `Input.IsKeyJustPressed(key)` | Клавиша нажата в этом кадре |
| `Input.GetMousePosition()` | Позиция мыши (x, y) |
| `Input.GetMouseDelta()` | Дельта мыши (dx, dy) |
| `Input.IsMouseButtonPressed(btn)` | Кнопка мыши зажата |
| `Input.IsMouseButtonJustPressed(btn)` | Кнопка мыши нажата в этом кадре |
| `Input.GetScroll()` | Прокрутка колеса |
