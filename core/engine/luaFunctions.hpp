#pragma once
#include <engine/luaEngine.hpp>
#include <utils/math/noise.h>

namespace LuaEngine {

    // ==================== FUNCTIONS CLASS ====================
    class _functions {
    private:
        static LuaEngine* getEngine(lua_State* state) {
            lua_getglobal(state, "__lua_engine");
            LuaEngine* engine = static_cast<LuaEngine*>(lua_touserdata(state, -1));
            lua_pop(state, 1);
            return engine;
        }

    public:
        // Screen size functions
        static int GetScreenWidth(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) {
                lua_pushinteger(state, 0);
                return 1;
            }
            lua_pushinteger(state, engine->getMainWindow()->getWidth());
            return 1;
        }

        static int GetScreenHeight(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) {
                lua_pushinteger(state, 0);
                return 1;
            }
            lua_pushinteger(state, engine->getMainWindow()->getHeight());
            return 1;
        }

        // Script management functions
        static int LoadScript(lua_State* state) {
            const char* scriptPath = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            bool result = engine->loadScript(scriptPath);
            lua_pushboolean(state, result);
            return 1;
        }

        static int LoadScriptsFromList(lua_State* state) {
            const char* listPath = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            bool result = engine->loadScriptsFromList(listPath);
            lua_pushboolean(state, result);
            return 1;
        }

        static int ReloadScript(lua_State* state) {
            const char* scriptPath = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            bool result = engine->reloadScript(scriptPath);
            lua_pushboolean(state, result);
            return 1;
        }

        static int ReloadAllScripts(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            bool result = engine->reloadAllScripts();
            lua_pushboolean(state, result);
            return 1;
        }

        static int EnableScript(lua_State* state) {
            const char* scriptName = luaL_checkstring(state, 1);
            bool enabled = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->enableScript(scriptName, enabled);
            return 0;
        }

        static int IsScriptLoaded(lua_State* state) {
            const char* scriptName = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->isScriptLoaded(scriptName));
            return 1;
        }

        static int GetLoadedScripts(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            std::vector<std::string> scripts = engine->getLoadedScripts();
            lua_newtable(state);
            for (size_t i = 0; i < scripts.size(); i++) {
                lua_pushstring(state, scripts[i].c_str());
                lua_rawseti(state, -2, i + 1);
            }
            return 1;
        }

        // Scene management
        static int LoadScene(lua_State* state) {
            const char* sceneName = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            bool result = engine->loadScene(sceneName);
            lua_pushboolean(state, result);
            return 1;
        }

        static int GetCurrentScene(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getSceneManager()) {
                lua_pushstring(state, "");
                return 1;
            }
            lua_pushstring(state, engine->getSceneManager()->getCurrentScene().c_str());
            return 1;
        }

        // Scene file I/O
        static int LoadSceneFile(lua_State* state) {
            const char* path = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            auto nameToId = engine->loadSceneFile(path);
            lua_newtable(state);
            for (const auto& [name, id] : nameToId) {
                lua_pushstring(state, name.c_str());
                lua_pushinteger(state, (lua_Integer)id);
                lua_rawset(state, -3);
            }
            return 1;
        }

        static int SaveSceneFile(lua_State* state) {
            const char* path = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            bool result = engine->saveSceneFile(path);
            lua_pushboolean(state, result);
            return 1;
        }

        // Sprite functions
        static int CreateSprite(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            const char *texturePath = luaL_checkstring(state, 1);
            float x = luaL_optnumber(state, 2, 0);
            float y = luaL_optnumber(state, 3, 0);
            Entity entity = engine->createSprite(texturePath, Vector2(x, y));
            lua_pushinteger(state, entity);
            return 1;
        }
        static int SpriteSetTexture(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            const char* texturePath = luaL_checkstring(state, 2);
            engine->spriteSetTexture(entity, texturePath);
            return 0;
        }
        static int SpriteSetPosition(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->spriteSetPosition(entity, Vector2(x, y));
            return 0;
        }
        static int SpriteGetPosition(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 position = engine->spriteGetPosition(entity);
            lua_pushnumber(state, position.x);
            lua_pushnumber(state, position.y);
            return 2;
        }
        static int SpriteSetTextureRect(lua_State *state) {
            LuaEngine* engine = _functions::getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float u = luaL_checknumber(state, 2);
            float v = luaL_checknumber(state, 3);
            float w = luaL_checknumber(state, 4);
            float h = luaL_checknumber(state, 5);
            engine->spriteSetTextureRect(entity, Vector4(u, v, w, h));
            return 0;
        }

        static int SpriteSetSize(lua_State *state) {
            LuaEngine* engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float width = luaL_checknumber(state, 2);
            float height = luaL_checknumber(state, 3);
            engine->spriteSetSize(entity, Vector2(width, height));
            return 0;
        }
        static int SpriteGetSize(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 size = engine->spriteGetSize(entity);
            lua_pushnumber(state, size.x);
            lua_pushnumber(state, size.y);
            return 2;
        }
        static int SpriteSetOrigin(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->spriteSetOrigin(entity, Vector2(x, y));
            return 0;
        }
        static int SpriteGetOrigin(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            Vector2 origin = engine->spriteGetOrigin(entity);
            lua_pushnumber(state, origin.x);
            lua_pushnumber(state, origin.y);
            return 2;
        }
        static int SpriteSetRotation(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float rotation = luaL_checknumber(state, 2);
            engine->spriteSetRotation(entity, rotation);
            return 0;
        }
        static int SpriteGetRotation(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float rotation = engine->spriteGetRotation(entity);
            lua_pushnumber(state, rotation);
            return 1;
        }
        static int SpriteSetColor(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float r = luaL_checknumber(state, 2);
            float g = luaL_checknumber(state, 3);
            float b = luaL_checknumber(state, 4);
            float a = luaL_optnumber(state, 5, 1.0f);
            engine->spriteSetColor(entity, Vector4(r, g, b, a));
            return 0;
        }
        static int SpriteGetColor(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                lua_pushnil(state);
                lua_pushnil(state);
                return 4;
            }

            Entity entity = luaL_checkinteger(state, 1);
            Vector4 color = engine->spriteGetColor(entity);
            lua_pushnumber(state, color.x);
            lua_pushnumber(state, color.y);
            lua_pushnumber(state, color.z);
            lua_pushnumber(state, color.w);
            return 4;
        }
        static int SpriteSetVisible(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            bool visible = lua_toboolean(state, 2);
            engine->spriteSetVisible(entity, visible);
            return 0;
        }
        static int SpriteIsVisible(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            bool visible = engine->spriteIsVisible(entity);
            lua_pushboolean(state, visible);
            return 1;
        }

        // Sprite batch functions
        static int CreateSpriteBatch(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            uint32_t maxSize = luaL_optinteger(state, 1, 25);
            Entity entity = engine->createSpriteBatch(maxSize);
            lua_pushinteger(state, entity);
            return 1;
        }
        static int SpriteBatchSetTexture(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) return 0;
            Entity batchEntity = luaL_checkinteger(state, 1);
            const char *texturePath = luaL_checkstring(state, 2);
            engine->spriteBatchSetTexture(batchEntity, texturePath);
            return 0;
        }
        static int SpriteBatchAdd(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) { lua_pushboolean(state, false); return 1; }
            Entity batchEntity = luaL_checkinteger(state, 1);
            Entity spriteEntity = luaL_checkinteger(state, 2);
            bool result = engine->spriteBatchAdd(batchEntity, spriteEntity);
            lua_pushboolean(state, result);
            return 1;
        }
        static int SpriteBatchRemove(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) { lua_pushboolean(state, false); return 1; }
            Entity batchEntity = luaL_checkinteger(state, 1);
            Entity spriteEntity = luaL_checkinteger(state, 2);
            bool result = engine->spriteBatchRemove(batchEntity, spriteEntity);
            lua_pushboolean(state, result);
            return 1;
        }
        static int SpriteBatchClear(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) return 0;
            Entity batchEntity = luaL_checkinteger(state, 1);
            engine->spriteBatchClear(batchEntity);
            return 0;
        }
        static int SpriteBatchSetMaxSize(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) return 0;
            Entity batchEntity = luaL_checkinteger(state, 1);
            uint32_t maxSize = luaL_checkinteger(state, 2);
            engine->spriteBatchSetMaxSize(batchEntity, maxSize);
            return 0;
        }
        static int SpriteBatchGetMaxSize(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) { lua_pushinteger(state, 0); return 1; }
            Entity batchEntity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->spriteBatchGetMaxSize(batchEntity));
            return 1;
        }
        static int SpriteBatchGetCount(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) { lua_pushinteger(state, 0); return 1; }
            Entity batchEntity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->spriteBatchGetCount(batchEntity));
            return 1;
        }
        static int SpriteBatchDraw(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) return 0;
            Entity batchEntity = luaL_checkinteger(state, 1);
            engine->drawSpriteBatch(batchEntity);
            return 0;
        }
        static int DestroySpriteBatch(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) return 0;
            Entity batchEntity = luaL_checkinteger(state, 1);
            engine->destroySpriteBatch(batchEntity);
            return 0;
        }

        // Text functions
        static int CreateText(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            const char *fontPath = luaL_checkstring(state, 1);
            const char *text = luaL_checkstring(state, 2);
            float x = luaL_optnumber(state, 3, 0.0f);
            float y = luaL_optnumber(state, 4, 0.0f);
            Entity entity = engine->createText(fontPath, text, Vector2(x, y));
            lua_pushinteger(state, entity);
            return 1;
        }
        static int TextSetString(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }
            Entity entity = luaL_checkinteger(state, 1);
            const char *text = luaL_checkstring(state, 2);
            engine->textSetString(entity, text);
            return 0;
        }
        static int TextGetString(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            lua_pushstring(state, engine->textGetString(entity).c_str());
            return 1;
        }
        static int TextSetPosition(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->textSetPosition(entity, Vector2(x, y));
            return 0;
        }
        static int TextGetPosition(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }

            Entity entity = luaL_checkinteger(state, 1);
            Vector2 position = engine->textGetPosition(entity);
            lua_pushnumber(state, position.x);
            lua_pushnumber(state, position.y);
            return 2;
        }
        static int TextSetColor(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float r = luaL_checknumber(state, 2);
            float g = luaL_checknumber(state, 3);
            float b = luaL_checknumber(state, 4);
            float a = luaL_optnumber(state, 5, 1.0f);
            engine->textSetColor(entity, Vector4(r, g, b, a));
            return 0;
        }
        static int TextGetColor(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }

            Entity entity = luaL_checkinteger(state, 1);
            Vector4 color = engine->textGetColor(entity);
            lua_pushnumber(state, color.x);
            lua_pushnumber(state, color.y);
            lua_pushnumber(state, color.z);
            lua_pushnumber(state, color.w);
            return 4;
        }
        static int TextSetScale(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }
            Entity entity = luaL_checkinteger(state, 1);
            float width = luaL_checknumber(state, 2);
            float height = luaL_checknumber(state, 3);
            engine->textSetScale(entity, Vector2(width, height));
            return 0;
        }
        static int TextGetScale(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }

            Entity entity = luaL_checkinteger(state, 1);
            Vector2 scale = engine->textGetScale(entity);
            lua_pushnumber(state, scale.x);
            lua_pushnumber(state, scale.y);
            return 2;
        }
        static int TextSetOrigin(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->textSetOrigin(entity, Vector2(x, y));
            return 0;
        }
        static int TextGetOrigin(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }

            Entity entity = luaL_checkinteger(state, 1);
            Vector2 origin = engine->textGetOrigin(entity);
            lua_pushnumber(state, origin.x);
            lua_pushnumber(state, origin.y);
            return 2;
        }
        static int TextSetRotation(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }
            Entity entity = luaL_checkinteger(state, 1);
            float rotation = luaL_checknumber(state, 2);
            engine->textSetRotation(entity, rotation);
            return 0;
        }
        static int TextGetRotation(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            float rotation = engine->textGetRotation(entity);
            lua_pushnumber(state, rotation);
            return 1;
        }
        static int TextSetAlignment(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }
            Entity entity = luaL_checkinteger(state, 1);
            int alignment = luaL_checkinteger(state, 2);
            engine->textSetAlignment(entity, static_cast<BlazeBolt::Text2D::Alignment>(alignment));
            return 0;
        }
        static int TextGetAlignment(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            BlazeBolt::Text2D::Alignment alignment = engine->textGetAlignment(entity);
            lua_pushinteger(state, static_cast<int>(alignment));
            return 1;
        }
        static int TextSetVisible(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                return 0;
            }
            Entity entity = luaL_checkinteger(state, 1);
            bool visible = lua_toboolean(state, 2);
            engine->textSetVisible(entity, visible);
            return 0;
        }
        static int TextIsVisible(lua_State *state) {
            LuaEngine *engine = _functions::getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            Entity entity = luaL_checkinteger(state, 1);
            bool visible = engine->textIsVisible(entity);
            lua_pushboolean(state, visible);
            return 1;
        }

        // Animation functions
        static int CreateAnimatedSprite(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }

            const char *path = luaL_checkstring(state, 1);
            float x = luaL_optnumber(state, 2, 0);
            float y = luaL_optnumber(state, 3, 0);
            Entity entity = engine->createAnimatedSprite(path, Vector2(x, y));
            lua_pushinteger(state, entity);
            return 1;
        }
        static int AnimatedSpritePlay(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            engine->animatedSpritePlay(entity);
            return 0;
        }
        static int AnimatedSpriteIsPlaying(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushboolean(state, engine->animatedSpriteIsPlaying(entity));
            return 1;
        }
        static int AnimatedSpritePause(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            engine->animatedSpritePause(entity);
            return 0;
        }
        static int AnimatedSpriteStop(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            engine->animatedSpriteStop(entity);
            return 0;
        }
        static int AnimatedSpriteRestart(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            engine->animatedSpriteRestart(entity);
            return 0;
        }
        static int AnimatedSpriteSetTexture(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            const char *path = luaL_checkstring(state, 2);
            engine->animatedSpriteSetTexture(entity, path);
            return 0;
        }
        static int AnimatedSpriteSetLooping(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            bool looping = lua_toboolean(state, 2);
            engine->animatedSpriteSetLooping(entity, looping);
            return 0;
        }
        static int AnimatedSpriteIsLooping(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushboolean(state, engine->animatedSpriteIsLooping(entity));
            return 1;
        }
        static int AnimatedSpriteSetPlaybackSpeed(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float speed = luaL_checknumber(state, 2);
            engine->animatedSpriteSetPlaybackSpeed(entity, speed);
            return 0;
        }
        static int AnimatedSpriteGetPlaybackSpeed(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushnumber(state, engine->animatedSpriteGetPlaybackSpeed(entity));
            return 1;
        }
        static int AnimatedSpriteSetFrame(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            int frame = luaL_checkinteger(state, 2);
            engine->animatedSpriteSetFrame(entity, frame);
            return 0;
        }
        static int AnimatedSpriteGetCurrentFrame(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->animatedSpriteGetCurrentFrame(entity));
            return 1;
        }
        static int AnimatedSpriteGetNumFrames(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->animatedSpriteGetNumFrames(entity));
            return 1;
        }
        static int AnimatedSpriteSetPosition(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->animatedSpriteSetPosition(entity, Vector2(x, y));
            return 0;
        }
        static int AnimatedSpriteGetPosition(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 position = engine->animatedSpriteGetPosition(entity);
            lua_pushnumber(state, position.x);
            lua_pushnumber(state, position.y);
            return 2;
        }
        static int AnimatedSpriteSetSize(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float width = luaL_checknumber(state, 2);
            float height = luaL_checknumber(state, 3);
            engine->animatedSpriteSetSize(entity, Vector2(width, height));
            return 0;
        }
        static int AnimatedSpriteGetSize(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 size = engine->animatedSpriteGetSize(entity);
            lua_pushnumber(state, size.x);
            lua_pushnumber(state, size.y);
            return 2;
        }
        static int AnimatedSpriteSetOrigin(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->animatedSpriteSetOrigin(entity, Vector2(x, y));
            return 0;
        }
        static int AnimatedSpriteGetOrigin(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 origin = engine->animatedSpriteGetOrigin(entity);
            lua_pushnumber(state, origin.x);
            lua_pushnumber(state, origin.y);
            return 2;
        }
        static int AnimatedSpriteSetRotation(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float rotation = luaL_checknumber(state, 2);
            engine->animatedSpriteSetRotation(entity, rotation);
            return 0;
        }
        static int AnimatedSpriteGetRotation(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                return 1;
            }
            Entity entity = luaL_checkinteger(state, 1);
            float rotation = engine->animatedSpriteGetRotation(entity);
            lua_pushnumber(state, rotation);
            return 1;
        }
        static int AnimatedSpriteSetColor(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity entity = luaL_checkinteger(state, 1);
            float r = luaL_checknumber(state, 2);
            float g = luaL_checknumber(state, 3);
            float b = luaL_checknumber(state, 4);
            float a = luaL_optnumber(state, 5, 1.0f);
            engine->animatedSpriteSetColor(entity, Vector4(r, g, b, a));
            return 0;
        }
        static int AnimatedSpriteGetColor(lua_State *state) {
            LuaEngine* engine = getEngine(state);
            if (engine == nullptr) {
                lua_pushnil(state);
                lua_pushnil(state);
                lua_pushnil(state);
                lua_pushnil(state);
                return 4;
            }
            Entity entity = luaL_checkinteger(state, 1);
            Vector4 color = engine->animatedSpriteGetColor(entity);
            lua_pushnumber(state, color.x);
            lua_pushnumber(state, color.y);
            lua_pushnumber(state, color.z);
            lua_pushnumber(state, color.w);
            return 4;
        }

        // Animation wheel functions
        static int CreateAnimationWheel(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushnil(state); return 1; }
            Entity spriteEntity = luaL_checkinteger(state, 1);
            Entity entity = engine->createAnimationWheel(spriteEntity);
            lua_pushinteger(state, entity);
            return 1;
        }
        static int AnimationWheelAddState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *name = luaL_checkstring(state, 2);
            const char *gifPath = luaL_checkstring(state, 3);
            float playbackSpeed = static_cast<float>(luaL_optnumber(state, 4, 1.0));
            bool looping = lua_toboolean(state, 5);
            if (lua_gettop(state) < 5) looping = true;
            engine->animationWheelAddState(wheelEntity, name, gifPath, playbackSpeed, looping);
            return 0;
        }
        static int AnimationWheelRemoveState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *name = luaL_checkstring(state, 2);
            engine->animationWheelRemoveState(wheelEntity, name);
            return 0;
        }
        static int AnimationWheelHasState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushboolean(state, false); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *name = luaL_checkstring(state, 2);
            lua_pushboolean(state, engine->animationWheelHasState(wheelEntity, name));
            return 1;
        }
        static int AnimationWheelSetInitialState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *name = luaL_checkstring(state, 2);
            engine->animationWheelSetInitialState(wheelEntity, name);
            return 0;
        }
        static int AnimationWheelGetInitialState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushstring(state, ""); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            lua_pushstring(state, engine->animationWheelGetInitialState(wheelEntity).c_str());
            return 1;
        }
        static int AnimationWheelSetState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *name = luaL_checkstring(state, 2);
            engine->animationWheelSetState(wheelEntity, name);
            return 0;
        }
        static int AnimationWheelGetState(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushstring(state, ""); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            lua_pushstring(state, engine->animationWheelGetState(wheelEntity).c_str());
            return 1;
        }
        static int AnimationWheelSetPlaybackSpeed(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *stateName = luaL_checkstring(state, 2);
            float speed = static_cast<float>(luaL_checknumber(state, 3));
            engine->animationWheelSetPlaybackSpeed(wheelEntity, stateName, speed);
            return 0;
        }
        static int AnimationWheelGetPlaybackSpeed(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushnumber(state, 1.0); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *stateName = luaL_checkstring(state, 2);
            lua_pushnumber(state, engine->animationWheelGetPlaybackSpeed(wheelEntity, stateName));
            return 1;
        }
        static int AnimationWheelSetLooping(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *stateName = luaL_checkstring(state, 2);
            bool looping = lua_toboolean(state, 3);
            engine->animationWheelSetLooping(wheelEntity, stateName, looping);
            return 0;
        }
        static int AnimationWheelIsLooping(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushboolean(state, true); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *stateName = luaL_checkstring(state, 2);
            lua_pushboolean(state, engine->animationWheelIsLooping(wheelEntity, stateName));
            return 1;
        }
        static int AnimationWheelSetGifPath(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *stateName = luaL_checkstring(state, 2);
            const char *gifPath = luaL_checkstring(state, 3);
            engine->animationWheelSetGifPath(wheelEntity, stateName, gifPath);
            return 0;
        }
        static int AnimationWheelGetGifPath(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushstring(state, ""); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            const char *stateName = luaL_checkstring(state, 2);
            lua_pushstring(state, engine->animationWheelGetGifPath(wheelEntity, stateName).c_str());
            return 1;
        }
        static int AnimationWheelSetAutoAdvance(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { return 0; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            bool autoAdvance = lua_toboolean(state, 2);
            engine->animationWheelSetAutoAdvance(wheelEntity, autoAdvance);
            return 0;
        }
        static int AnimationWheelGetAutoAdvance(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_pushboolean(state, false); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            lua_pushboolean(state, engine->animationWheelGetAutoAdvance(wheelEntity));
            return 1;
        }
        static int AnimationWheelGetStateNames(lua_State *state) {
            LuaEngine *engine = getEngine(state);
            if (engine == nullptr) { lua_newtable(state); return 1; }
            Entity wheelEntity = luaL_checkinteger(state, 1);
            std::vector<std::string> names = engine->animationWheelGetStateNames(wheelEntity);
            lua_newtable(state);
            for (size_t i = 0; i < names.size(); i++) {
                lua_pushstring(state, names[i].c_str());
                lua_rawseti(state, -2, static_cast<lua_Integer>(i + 1));
            }
            return 1;
        }

        // Mesh functions
        static int CreateMesh(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            Entity entity = engine->createMesh();
            lua_pushinteger(state, entity);
            return 1;
        }

        static int MeshSetData(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            luaL_checktype(state, 2, LUA_TTABLE);
            std::vector<Mesh2D::Vertex> vertices;
            int vertexCount = lua_rawlen(state, 2);
            for (int i = 1; i <= vertexCount; i++) {
                lua_rawgeti(state, 2, i);
                if (lua_istable(state, -1)) {
                    Mesh2D::Vertex vert;
                    lua_getfield(state, -1, "x"); vert.x = luaL_checknumber(state, -1); lua_pop(state, 1);
                    lua_getfield(state, -1, "y"); vert.y = luaL_checknumber(state, -1); lua_pop(state, 1);
                    lua_getfield(state, -1, "u"); vert.u = luaL_optnumber(state, -1, 0); lua_pop(state, 1);
                    lua_getfield(state, -1, "v"); vert.v = luaL_optnumber(state, -1, 0); lua_pop(state, 1);
                    vertices.push_back(vert);
                }
                lua_pop(state, 1);
            }
            luaL_checktype(state, 3, LUA_TTABLE);
            std::vector<GLuint> indices;
            int indexCount = lua_rawlen(state, 3);
            for (int i = 1; i <= indexCount; i++) {
                lua_rawgeti(state, 3, i);
                indices.push_back(luaL_checkinteger(state, -1));
                lua_pop(state, 1);
            }
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->meshSetData(entity, vertices, indices);
            return 0;
        }

        static int MeshSetShader(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* vertexPath = luaL_checkstring(state, 2);
            const char* fragmentPath = luaL_checkstring(state, 3);
            engine->meshSetShader(entity, vertexPath, fragmentPath);
            return 0;
        }

        static int MeshSetUniformFloat(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float value = luaL_checknumber(state, 3);
            engine->meshSetUniformFloat(entity, name, value);
            return 0;
        }

        static int MeshSetUniformInt(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            int value = luaL_checkinteger(state, 3);
            engine->meshSetUniformInt(entity, name, value);
            return 0;
        }

        static int MeshSetUniformVec2(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float x = luaL_checknumber(state, 3);
            float y = luaL_checknumber(state, 4);
            engine->meshSetUniformVec2(entity, name, x, y);
            return 0;
        }

        static int MeshSetUniformVec3(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float x = luaL_checknumber(state, 3);
            float y = luaL_checknumber(state, 4);
            float z = luaL_checknumber(state, 5);
            engine->meshSetUniformVec3(entity, name, x, y, z);
            return 0;
        }

        static int MeshSetUniformVec4(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float x = luaL_checknumber(state, 3);
            float y = luaL_checknumber(state, 4);
            float z = luaL_checknumber(state, 5);
            float w = luaL_checknumber(state, 6);
            engine->meshSetUniformVec4(entity, name, x, y, z, w);
            return 0;
        }

        static int MeshDraw(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->meshDraw(entity);
            return 0;
        }

        // Camera functions
        static int CreateCamera(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            Entity entity = engine->createCamera();
            lua_pushinteger(state, entity);
            return 1;
        }

        static int CameraSetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->cameraSetPosition(entity, Vector2(x, y));
            return 0;
        }

        static int CameraGetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); lua_pushnumber(state, 0); return 2; }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 pos = engine->cameraGetPosition(entity);
            lua_pushnumber(state, pos.x);
            lua_pushnumber(state, pos.y);
            return 2;
        }

        static int CameraSetZoom(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float zoom = luaL_checknumber(state, 2);
            engine->cameraSetZoom(entity, zoom);
            return 0;
        }

        static int CameraGetZoom(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 1); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushnumber(state, engine->cameraGetZoom(entity));
            return 1;
        }

        static int CameraSetRotation(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float rotation = luaL_checknumber(state, 2);
            engine->cameraSetRotation(entity, rotation);
            return 0;
        }

        static int CameraGetRotation(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushnumber(state, engine->cameraGetRotation(entity));
            return 1;
        }

        // Particle system functions
        static int CreateParticleSystem(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            Entity entity = engine->createParticleSystem();
            lua_pushinteger(state, entity);
            return 1;
        }

        static int ParticleSystemSetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->particleSystemSetPosition(entity, Vector2(x, y));
            return 0;
        }

        static int ParticleSystemSetTexture(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            const char* path = luaL_checkstring(state, 2);
            engine->particleSystemSetTexture(entity, path);
            return 0;
        }

        static int ParticleSystemSetEmissionRate(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float rate = luaL_checknumber(state, 2);
            engine->particleSystemSetEmissionRate(entity, rate);
            return 0;
        }

        static int ParticleSystemGetEmissionRate(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushnumber(state, engine->particleSystemGetEmissionRate(entity));
            return 1;
        }

        static int ParticleSystemSetLifetime(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float min = luaL_checknumber(state, 2);
            float max = luaL_checknumber(state, 3);
            engine->particleSystemSetLifetime(entity, min, max);
            return 0;
        }

        static int ParticleSystemSetSpeed(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float min = luaL_checknumber(state, 2);
            float max = luaL_checknumber(state, 3);
            engine->particleSystemSetSpeed(entity, min, max);
            return 0;
        }

        static int ParticleSystemSetSize(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float min = luaL_checknumber(state, 2);
            float max = luaL_checknumber(state, 3);
            engine->particleSystemSetSize(entity, min, max);
            return 0;
        }

        static int ParticleSystemSetEndSize(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float min = luaL_checknumber(state, 2);
            float max = luaL_checknumber(state, 3);
            engine->particleSystemSetEndSize(entity, min, max);
            return 0;
        }

        static int ParticleSystemSetColor(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float r1 = luaL_checknumber(state, 2);
            float g1 = luaL_checknumber(state, 3);
            float b1 = luaL_checknumber(state, 4);
            float a1 = luaL_optnumber(state, 5, 1.0f);
            float r2 = luaL_checknumber(state, 6);
            float g2 = luaL_checknumber(state, 7);
            float b2 = luaL_checknumber(state, 8);
            float a2 = luaL_optnumber(state, 9, 0.0f);
            engine->particleSystemSetColor(entity, Vector4(r1, g1, b1, a1), Vector4(r2, g2, b2, a2));
            return 0;
        }

        static int ParticleSystemSetDirection(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float minAngle = luaL_checknumber(state, 2);
            float maxAngle = luaL_checknumber(state, 3);
            engine->particleSystemSetDirection(entity, minAngle, maxAngle);
            return 0;
        }

        static int ParticleSystemSetRotationSpeed(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float speed = luaL_checknumber(state, 2);
            engine->particleSystemSetRotationSpeed(entity, speed);
            return 0;
        }

        static int ParticleSystemSetActive(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            bool active = lua_toboolean(state, 2);
            engine->particleSystemSetActive(entity, active);
            return 0;
        }

        static int ParticleSystemIsActive(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushboolean(state, engine->particleSystemIsActive(entity));
            return 1;
        }

        static int ParticleSystemSetVisible(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            bool visible = lua_toboolean(state, 2);
            engine->particleSystemSetVisible(entity, visible);
            return 0;
        }

        static int ParticleSystemIsVisible(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushboolean(state, engine->particleSystemIsVisible(entity));
            return 1;
        }

        static int ParticleSystemEmit(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            int count = luaL_checkinteger(state, 2);
            engine->particleSystemEmit(entity, count);
            return 0;
        }

        static int ParticleSystemClear(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            engine->particleSystemClear(entity);
            return 0;
        }

        static int ParticleSystemGetCount(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->particleSystemGetCount(entity));
            return 1;
        }

        // Tileset functions
        static int CreateTileset(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            const char* texturePath = luaL_checkstring(state, 1);
            uint32_t tileW = luaL_checkinteger(state, 2);
            uint32_t tileH = luaL_checkinteger(state, 3);
            uint32_t atlasCols = luaL_checkinteger(state, 4);
            uint32_t atlasRows = luaL_checkinteger(state, 5);
            Entity entity = engine->createTileset(texturePath, tileW, tileH, atlasCols, atlasRows);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int TilesetSetMap(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            luaL_checktype(state, 2, LUA_TTABLE);
            std::vector<std::vector<int>> map;
            int rows = lua_rawlen(state, 2);
            for (int r = 1; r <= rows; r++) {
                lua_rawgeti(state, 2, r);
                luaL_checktype(state, -1, LUA_TTABLE);
                int cols = lua_rawlen(state, -1);
                std::vector<int> row;
                for (int c = 1; c <= cols; c++) {
                    lua_rawgeti(state, -1, c);
                    row.push_back(luaL_checkinteger(state, -1));
                    lua_pop(state, 1);
                }
                map.push_back(row);
                lua_pop(state, 1);
            }
            engine->tilesetSetMap(entity, map);
            return 0;
        }

        static int TilesetGetTile(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, -1); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            uint32_t col = luaL_checkinteger(state, 2);
            uint32_t row = luaL_checkinteger(state, 3);
            lua_pushinteger(state, engine->tilesetGetTile(entity, col, row));
            return 1;
        }

        static int TilesetSetTile(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            uint32_t col = luaL_checkinteger(state, 2);
            uint32_t row = luaL_checkinteger(state, 3);
            int tileIndex = luaL_checkinteger(state, 4);
            engine->tilesetSetTile(entity, col, row, tileIndex);
            return 0;
        }

        static int TilesetSetTileSize(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            uint32_t w = luaL_checkinteger(state, 2);
            uint32_t h = luaL_checkinteger(state, 3);
            engine->tilesetSetTileSize(entity, w, h);
            return 0;
        }

        static int TilesetGetTileSize(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); lua_pushinteger(state, 0); return 2; }
            Entity entity = luaL_checkinteger(state, 1);
            uint32_t w, h;
            engine->tilesetGetTileSize(entity, &w, &h);
            lua_pushinteger(state, w);
            lua_pushinteger(state, h);
            return 2;
        }

        static int TilesetSetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->tilesetSetPosition(entity, Vector2(x, y));
            return 0;
        }

        static int TilesetGetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); lua_pushnumber(state, 0); return 2; }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 pos = engine->tilesetGetPosition(entity);
            lua_pushnumber(state, pos.x);
            lua_pushnumber(state, pos.y);
            return 2;
        }

        static int TilesetGetMapWidth(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->tilesetGetMapWidth(entity));
            return 1;
        }

        static int TilesetGetMapHeight(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->tilesetGetMapHeight(entity));
            return 1;
        }

        static int TilesetGetTileCount(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushinteger(state, engine->tilesetGetTileCount(entity));
            return 1;
        }

        static int TilesetDraw(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            BlazeBolt::Tileset2D* tileset = engine->getTilesetWorld().getEntity(entity);
            if (tileset) {
                tileset->rebuild(engine->getSpriteWorld());
                tileset->draw(engine->getTextureManager().getDefault2D());
            }
            return 0;
        }

        static int DestroyTileset(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            engine->destroyTileset(entity);
            return 0;
        }

        // Light functions
        static int CreatePointLight(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            float x = luaL_optnumber(state, 1, 0);
            float y = luaL_optnumber(state, 2, 0);
            float r = luaL_optnumber(state, 3, 1);
            float g = luaL_optnumber(state, 4, 1);
            float b = luaL_optnumber(state, 5, 1);
            float intensity = luaL_optnumber(state, 6, 1.0f);
            float radius = luaL_optnumber(state, 7, 200.0f);
            Entity entity = engine->createPointLight(x, y, r, g, b, intensity, radius);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int CreateAmbientLight(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            float r = luaL_optnumber(state, 1, 1);
            float g = luaL_optnumber(state, 2, 1);
            float b = luaL_optnumber(state, 3, 1);
            float intensity = luaL_optnumber(state, 4, 0.3f);
            Entity entity = engine->createAmbientLight(r, g, b, intensity);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int LightSetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            engine->lightSetPosition(entity, Vector2(x, y));
            return 0;
        }

        static int LightGetPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); lua_pushnumber(state, 0); return 2; }
            Entity entity = luaL_checkinteger(state, 1);
            Vector2 pos = engine->lightGetPosition(entity);
            lua_pushnumber(state, pos.x);
            lua_pushnumber(state, pos.y);
            return 2;
        }

        static int LightSetColor(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float r = luaL_checknumber(state, 2);
            float g = luaL_checknumber(state, 3);
            float b = luaL_checknumber(state, 4);
            engine->lightSetColor(entity, Vector3(r, g, b));
            return 0;
        }

        static int LightGetColor(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 1); lua_pushnumber(state, 1); lua_pushnumber(state, 1); return 3; }
            Entity entity = luaL_checkinteger(state, 1);
            Vector3 color = engine->lightGetColor(entity);
            lua_pushnumber(state, color.x);
            lua_pushnumber(state, color.y);
            lua_pushnumber(state, color.z);
            return 3;
        }

        static int LightSetIntensity(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float intensity = luaL_checknumber(state, 2);
            engine->lightSetIntensity(entity, intensity);
            return 0;
        }

        static int LightGetIntensity(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushnumber(state, engine->lightGetIntensity(entity));
            return 1;
        }

        static int LightSetRadius(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            float radius = luaL_checknumber(state, 2);
            engine->lightSetRadius(entity, radius);
            return 0;
        }

        static int LightGetRadius(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushnumber(state, engine->lightGetRadius(entity));
            return 1;
        }

        static int LightSetEnabled(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            bool enabled = lua_toboolean(state, 2);
            engine->lightSetEnabled(entity, enabled);
            return 0;
        }

        static int LightGetEnabled(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            Entity entity = luaL_checkinteger(state, 1);
            lua_pushboolean(state, engine->lightGetEnabled(entity));
            return 1;
        }

        static int DestroyLight(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Entity entity = luaL_checkinteger(state, 1);
            engine->destroyLight(entity);
            return 0;
        }

        // Object deletion
        static int Destroy(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->destroyEntity(entity);
            return 0;
        }

        static int DestroyAll(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->destroyAllEntities();
            return 0;
        }

        // Physics functions
        static int PhysicsInit(lua_State* state) {
            float gx = luaL_optnumber(state, 1, 0);
            float gy = luaL_optnumber(state, 2, -9.81f);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsInit(gx, gy);
            return 0;
        }

        static int PhysicsSetGravity(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetGravity(x, y);
            return 0;
        }

        static int PhysicsGetGravity(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); lua_pushnumber(state, 0); return 2; }
            float x, y;
            engine->physicsGetGravity(&x, &y);
            lua_pushnumber(state, x);
            lua_pushnumber(state, y);
            return 2;
        }

        static int PhysicsCreateBody(lua_State* state) {
            int bodyType = luaL_checkinteger(state, 1);
            float x = luaL_optnumber(state, 2, 0);
            float y = luaL_optnumber(state, 3, 0);
            float mass = luaL_optnumber(state, 4, 1);
            float friction = luaL_optnumber(state, 5, 0.3f);
            float restitution = luaL_optnumber(state, 6, 0.5f);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            Entity entity = engine->physicsCreateBody(bodyType, x, y, mass, friction, restitution);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int PhysicsAddCircle(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float radius = luaL_checknumber(state, 2);
            float ox = luaL_optnumber(state, 3, 0);
            float oy = luaL_optnumber(state, 4, 0);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsAddCircle(entity, radius, ox, oy);
            return 0;
        }

        static int PhysicsAddRectangle(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float hw = luaL_checknumber(state, 2);
            float hh = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsAddRectangle(entity, hw, hh);
            return 0;
        }

        static int PhysicsSetLinearVelocity(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float vx = luaL_checknumber(state, 2);
            float vy = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetLinearVelocity(entity, vx, vy);
            return 0;
        }

        static int PhysicsGetLinearVelocity(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); lua_pushnumber(state, 0); return 2; }
            float vx, vy;
            engine->physicsGetLinearVelocity(entity, &vx, &vy);
            lua_pushnumber(state, vx);
            lua_pushnumber(state, vy);
            return 2;
        }

        static int PhysicsSetAngularVelocity(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float av = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetAngularVelocity(entity, av);
            return 0;
        }

        static int PhysicsGetAngularVelocity(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            lua_pushnumber(state, engine->physicsGetAngularVelocity(entity));
            return 1;
        }

        static int PhysicsApplyForce(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float fx = luaL_checknumber(state, 2);
            float fy = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsApplyForce(entity, fx, fy);
            return 0;
        }

        static int PhysicsApplyForceAtPoint(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float fx = luaL_checknumber(state, 2);
            float fy = luaL_checknumber(state, 3);
            float px = luaL_checknumber(state, 4);
            float py = luaL_checknumber(state, 5);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsApplyForceAtPoint(entity, fx, fy, px, py);
            return 0;
        }

        static int PhysicsApplyImpulse(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float ix = luaL_checknumber(state, 2);
            float iy = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsApplyImpulse(entity, ix, iy);
            return 0;
        }

        static int PhysicsApplyImpulseAtPoint(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float ix = luaL_checknumber(state, 2);
            float iy = luaL_checknumber(state, 3);
            float px = luaL_checknumber(state, 4);
            float py = luaL_checknumber(state, 5);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsApplyImpulseAtPoint(entity, ix, iy, px, py);
            return 0;
        }

        static int PhysicsApplyTorque(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float torque = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsApplyTorque(entity, torque);
            return 0;
        }

        static int PhysicsSetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetPosition(entity, x, y);
            return 0;
        }

        static int PhysicsGetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); lua_pushnumber(state, 0); return 2; }
            float x, y;
            engine->physicsGetPosition(entity, &x, &y);
            lua_pushnumber(state, x);
            lua_pushnumber(state, y);
            return 2;
        }

        static int PhysicsSetAngle(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float angle = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetAngle(entity, angle);
            return 0;
        }

        static int PhysicsGetAngle(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            lua_pushnumber(state, engine->physicsGetAngle(entity));
            return 1;
        }

        static int PhysicsSetGravityScale(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float scale = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetGravityScale(entity, scale);
            return 0;
        }

        static int PhysicsGetGravityScale(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 1); return 1; }
            lua_pushnumber(state, engine->physicsGetGravityScale(entity));
            return 1;
        }

        static int PhysicsSetActive(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool active = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetActive(entity, active);
            return 0;
        }

        static int PhysicsIsActive(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->physicsIsActive(entity));
            return 1;
        }

        static int PhysicsSetFixedRotation(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool fixed = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetFixedRotation(entity, fixed);
            return 0;
        }

        static int PhysicsIsFixedRotation(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->physicsIsFixedRotation(entity));
            return 1;
        }

        static int PhysicsSetBullet(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool bullet = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetBullet(entity, bullet);
            return 0;
        }

        static int PhysicsIsBullet(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->physicsIsBullet(entity));
            return 1;
        }

        static int PhysicsDestroyBody(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsDestroyBody(entity);
            return 0;
        }

        static int PhysicsGetMass(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            lua_pushnumber(state, engine->physicsGetMass(entity));
            return 1;
        }

        static int PhysicsSetFriction(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float friction = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetFriction(entity, friction);
            return 0;
        }

        static int PhysicsGetFriction(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            lua_pushnumber(state, engine->physicsGetFriction(entity));
            return 1;
        }

        static int PhysicsSetRestitution(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float restitution = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSetRestitution(entity, restitution);
            return 0;
        }

        static int PhysicsGetRestitution(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            lua_pushnumber(state, engine->physicsGetRestitution(entity));
            return 1;
        }

        static int PhysicsStep(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsStep();
            return 0;
        }

        static int PhysicsSyncSprite(lua_State* state) {
            Entity bodyEntity = luaL_checkinteger(state, 1);
            Entity spriteEntity = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSyncSprite(bodyEntity, spriteEntity);
            return 0;
        }

        static int PhysicsSyncText(lua_State* state) {
            Entity bodyEntity = luaL_checkinteger(state, 1);
            Entity textEntity = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSyncText(bodyEntity, textEntity);
            return 0;
        }

        static int PhysicsSyncAnimatedSprite(lua_State* state) {
            Entity bodyEntity = luaL_checkinteger(state, 1);
            Entity animEntity = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->physicsSyncAnimatedSprite(bodyEntity, animEntity);
            return 0;
        }

        // Audio functions
        static int LoadSound(lua_State* state) {
            const char* filename = luaL_checkstring(state, 1);
            const char* soundName = luaL_checkstring(state, 2);
            bool loop = lua_toboolean(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, -1); return 1; }
            int soundId = engine->loadSound(filename, soundName, loop);
            lua_pushinteger(state, soundId);
            return 1;
        }

        static int PlaySound(lua_State* state) {
            const char* soundName = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->playSound(soundName);
            return 0;
        }

        static int PlaySoundById(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->playSoundById(soundId);
            return 0;
        }

        static int StopSound(lua_State* state) {
            const char* soundName = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->stopSound(soundName);
            return 0;
        }

        static int StopAllSounds(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->stopAllSounds();
            return 0;
        }

        static int SetSoundVolume(lua_State* state) {
            const char* soundName = luaL_checkstring(state, 1);
            float volume = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundVolume(soundName, volume);
            return 0;
        }

        static int IsSoundPlaying(lua_State* state) {
            const char* soundName = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->isSoundPlaying(soundName));
            return 1;
        }

        // Audio: pitch & looping by ID
        static int SetSoundPitch(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float pitch = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundPitch(soundId, pitch);
            return 0;
        }

        static int SetSoundLoopingById(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            bool loop = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundLooping(soundId, loop);
            return 0;
        }

        // Audio: 3D positional
        static int SetSoundPosition(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            float z = luaL_checknumber(state, 4);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundPosition(soundId, x, y, z);
            return 0;
        }

        static int SetSoundPositionByName(lua_State* state) {
            const char* soundName = luaL_checkstring(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            float z = luaL_checknumber(state, 4);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundPositionByName(soundName, x, y, z);
            return 0;
        }

        static int GetSoundPosition(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            float x = 0, y = 0, z = 0;
            if (engine) engine->getSoundPosition(soundId, x, y, z);
            lua_pushnumber(state, x);
            lua_pushnumber(state, y);
            lua_pushnumber(state, z);
            return 3;
        }

        static int SetSoundVelocity(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            float z = luaL_checknumber(state, 4);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundVelocity(soundId, x, y, z);
            return 0;
        }

        static int SetSoundRolloff(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float rolloff = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundRolloff(soundId, rolloff);
            return 0;
        }

        static int SetSoundReferenceDistance(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float dist = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundReferenceDistance(soundId, dist);
            return 0;
        }

        static int SetSoundMaxDistance(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float dist = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundMaxDistance(soundId, dist);
            return 0;
        }

        static int SetSoundSpatial(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            bool spatial = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundSpatial(soundId, spatial);
            return 0;
        }

        static int SetSoundCone(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float inner = luaL_checknumber(state, 2);
            float outer = luaL_checknumber(state, 3);
            float gain = luaL_checknumber(state, 4);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundCone(soundId, inner, outer, gain);
            return 0;
        }

        static int SetSoundDirection(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            float z = luaL_checknumber(state, 4);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setSoundDirection(soundId, x, y, z);
            return 0;
        }

        // Listener
        static int SetListenerPosition(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            float z = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setListenerPosition(x, y, z);
            return 0;
        }

        static int GetListenerPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            float x = 0, y = 0, z = 0;
            if (engine) engine->getListenerPosition(x, y, z);
            lua_pushnumber(state, x);
            lua_pushnumber(state, y);
            lua_pushnumber(state, z);
            return 3;
        }

        static int SetListenerVelocity(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            float z = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setListenerVelocity(x, y, z);
            return 0;
        }

        static int SetListenerOrientation(lua_State* state) {
            float fx = luaL_checknumber(state, 1);
            float fy = luaL_checknumber(state, 2);
            float fz = luaL_checknumber(state, 3);
            float ux = luaL_checknumber(state, 4);
            float uy = luaL_checknumber(state, 5);
            float uz = luaL_checknumber(state, 6);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setListenerOrientation(fx, fy, fz, ux, uy, uz);
            return 0;
        }

        static int SetListenerGain(lua_State* state) {
            float gain = luaL_checknumber(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setListenerGain(gain);
            return 0;
        }

        // EFX Effects
        static int CreateAudioEffect(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            int id = engine ? engine->createAudioEffect() : -1;
            lua_pushinteger(state, id);
            return 1;
        }

        static int DestroyAudioEffect(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (engine) engine->destroyAudioEffect(idx);
            return 0;
        }

        static int SetAudioEffectType(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int type = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioEffectType(idx, type) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int SetAudioEffectf(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int param = luaL_checkinteger(state, 2);
            float value = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioEffectf(idx, param, value) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int SetAudioEffecti(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int param = luaL_checkinteger(state, 2);
            int value = luaL_checkinteger(state, 3);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioEffecti(idx, param, value) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int GetAudioEffectf(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int param = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            float val = engine ? engine->getAudioEffectf(idx, param) : 0;
            lua_pushnumber(state, val);
            return 1;
        }

        static int GetAudioEffecti(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int param = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            int val = engine ? engine->getAudioEffecti(idx, param) : 0;
            lua_pushinteger(state, val);
            return 1;
        }

        static int GetAudioEfxSupported(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->getAudioEfxSupported() : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        // EFX Filters
        static int CreateAudioFilter(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            int id = engine ? engine->createAudioFilter() : -1;
            lua_pushinteger(state, id);
            return 1;
        }

        static int DestroyAudioFilter(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (engine) engine->destroyAudioFilter(idx);
            return 0;
        }

        static int SetAudioFilterType(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int type = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioFilterType(idx, type) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int SetAudioFilterf(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            int param = luaL_checkinteger(state, 2);
            float value = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioFilterf(idx, param, value) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        // EFX Effect Slots
        static int CreateAudioEffectSlot(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            int id = engine ? engine->createAudioEffectSlot() : -1;
            lua_pushinteger(state, id);
            return 1;
        }

        static int DestroyAudioEffectSlot(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (engine) engine->destroyAudioEffectSlot(idx);
            return 0;
        }

        static int SetAudioEffectSlotEffect(lua_State* state) {
            int slotIdx = luaL_checkinteger(state, 1);
            int effectIdx = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioEffectSlotEffect(slotIdx, effectIdx) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int ClearAudioEffectSlotEffect(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->clearAudioEffectSlotEffect(idx) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int SetAudioEffectSlotGain(lua_State* state) {
            int idx = luaL_checkinteger(state, 1);
            float gain = luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->setAudioEffectSlotGain(idx, gain) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        // Linking
        static int AttachAudioEffect(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            int slotIdx = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->attachAudioEffect(soundId, slotIdx) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int DetachAudioEffect(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->detachAudioEffect(soundId) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        static int AttachAudioFilter(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            int filterIdx = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            bool ok = engine ? engine->attachAudioFilter(soundId, filterIdx) : false;
            lua_pushboolean(state, ok);
            return 1;
        }

        // Window management
        static int CreateWindow(lua_State* state) {
            int width = luaL_checkinteger(state, 1);
            int height = luaL_checkinteger(state, 2);
            const char* title = luaL_checkstring(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            auto window = engine->createWindow(width, height, title);
            lua_pushlightuserdata(state, window.get());
            return 1;
        }

        static int SetWindowTitle(lua_State* state) {
            Window* window = static_cast<Window*>(lua_touserdata(state, 1));
            const char* title = luaL_checkstring(state, 2);
            if (window) window->setTitle(title);
            return 0;
        }

        static int SetWindowSize(lua_State* state) {
            Window* window = static_cast<Window*>(lua_touserdata(state, 1));
            int width = luaL_checkinteger(state, 2);
            int height = luaL_checkinteger(state, 3);
            if (window) window->setSize(width, height);
            return 0;
        }

        static int SetWindowIcon(lua_State* state) {
            Window* window = static_cast<Window*>(lua_touserdata(state, 1));
            const char* iconPath = luaL_checkstring(state, 2);
            if (window) window->setIcon(iconPath);
            return 0;
        }

        // Main window functions (no pointer needed)
        static int SetMainWindowTitle(lua_State* state) {
            const char* title = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->setTitle(title);
            return 0;
        }

        static int GetMainWindowTitle(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushstring(state, ""); return 1; }
            lua_pushstring(state, engine->getMainWindow()->getTitle());
            return 1;
        }

        static int SetMainWindowSize(lua_State* state) {
            int width = luaL_checkinteger(state, 1);
            int height = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->setSize(width, height);
            return 0;
        }

        static int GetMainWindowWidth(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushinteger(state, 0); return 1; }
            lua_pushinteger(state, engine->getMainWindow()->getWidth());
            return 1;
        }

        static int GetMainWindowHeight(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushinteger(state, 0); return 1; }
            lua_pushinteger(state, engine->getMainWindow()->getHeight());
            return 1;
        }

        static int SetMainWindowPosition(lua_State* state) {
            int x = luaL_checkinteger(state, 1);
            int y = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            glfwSetWindowPos(engine->getMainWindow()->getGLFWwindow(), x, y);
            return 0;
        }

        static int GetMainWindowPosition(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushinteger(state, 0); lua_pushinteger(state, 0); return 2; }
            int x, y;
            engine->getMainWindow()->getPosition(&x, &y);
            lua_pushinteger(state, x);
            lua_pushinteger(state, y);
            return 2;
        }

        static int SetMainWindowIcon(lua_State* state) {
            const char* iconPath = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->setIcon(iconPath);
            return 0;
        }

        static int SetMainWindowShouldClose(lua_State* state) {
            bool flag = lua_toboolean(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->setShouldClose(flag);
            return 0;
        }

        static int IsMainWindowShouldClose(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushboolean(state, true); return 1; }
            lua_pushboolean(state, engine->getMainWindow()->shouldClose());
            return 1;
        }

        // Fullscreen functions
        static int SetMainWindowFullscreen(lua_State* state) {
            bool fullscreen = lua_toboolean(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->setFullscreen(fullscreen);
            return 0;
        }

        static int IsMainWindowFullscreen(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->getMainWindow()->isFullscreen());
            return 1;
        }

        static int ToggleMainWindowFullscreen(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->toggleFullscreen();
            return 0;
        }

        // VSync functions
        static int SetMainWindowVSync(lua_State* state) {
            bool enabled = lua_toboolean(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->setVSync(enabled);
            return 0;
        }

        static int IsMainWindowVSync(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) { lua_pushboolean(state, false); return 1; }
            lua_pushboolean(state, engine->getMainWindow()->isVSync());
            return 1;
        }

        static int ToggleMainWindowVSync(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine || !engine->getMainWindow()) return 0;
            engine->getMainWindow()->toggleVSync();
            return 0;
        }

        // Input functions
        static int IsKeyPressed(lua_State* state) {
            int key = luaL_checkinteger(state, 1);
            lua_pushboolean(state, Input::getInstance().isKeyPressed(key));
            return 1;
        }
        static int IsKeyJustPressed(lua_State* state) {
            int key = luaL_checkinteger(state, 1);
            lua_pushboolean(state, Input::getInstance().isKeyJustPressed(key));
            return 1;
        }
        static int GetMouseX(lua_State* state) {
            lua_pushnumber(state, Input::getInstance().getMouseX());
            return 1;
        }
        static int GetMouseY(lua_State* state) {
            lua_pushnumber(state, Input::getInstance().getMouseY());
            return 1;
        }
        static int GetMouseDeltaX(lua_State* state) {
            lua_pushnumber(state, Input::getInstance().getMouseDeltaX());
            return 1;
        }
        static int GetMouseDeltaY(lua_State* state) {
            lua_pushnumber(state, Input::getInstance().getMouseDeltaY());
            return 1;
        }
        static int IsMouseButtonPressed(lua_State* state) {
            int button = luaL_checkinteger(state, 1);
            lua_pushboolean(state, Input::getInstance().isMouseButtonPressed(button));
            return 1;
        }
        static int IsMouseButtonJustPressed(lua_State* state) {
            int button = luaL_checkinteger(state, 1);
            lua_pushboolean(state, Input::getInstance().isMouseButtonJustPressed(button));
            return 1;
        }
        static int GetScrollY(lua_State* state) {
            lua_pushnumber(state, Input::getInstance().getScrollY());
            return 1;
        }

        // Utility functions
        static int Print(lua_State* state) {
            int n = lua_gettop(state);
            std::cout << "[Lua] ";
            for (int i = 1; i <= n; i++) {
                if (lua_isstring(state, i)) {
                    std::cout << lua_tostring(state, i);
                } else if (lua_isnumber(state, i)) {
                    std::cout << lua_tonumber(state, i);
                } else if (lua_isboolean(state, i)) {
                    std::cout << (lua_toboolean(state, i) ? "true" : "false");
                } else if (lua_isnil(state, i)) {
                    std::cout << "nil";
                } else {
                    const char* str = luaL_tolstring(state, i, nullptr);
                    std::cout << str;
                    lua_pop(state, 1);
                }
                if (i < n) std::cout << "\t";
            }
            std::cout << std::endl;
            return 0;
        }

        static int GetDeltaTime(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            lua_pushnumber(state, engine->getDeltaTime());
            return 1;
        }

        static int GetTime(lua_State* state) {
            lua_pushnumber(state, glfwGetTime());
            return 1;
        }

        static int Random(lua_State* state) {
            float min = luaL_optnumber(state, 1, 0);
            float max = luaL_optnumber(state, 2, 1);
            float random = min + (float)rand() / RAND_MAX * (max - min);
            lua_pushnumber(state, random);
            return 1;
        }

        static int RandomInt(lua_State* state) {
            int min = luaL_checkinteger(state, 1);
            int max = luaL_checkinteger(state, 2);
            int random = min + rand() % (max - min + 1);
            lua_pushinteger(state, random);
            return 1;
        }

        static int SetRandomSeed(lua_State* state) {
            unsigned int seed = luaL_checkinteger(state, 1);
            srand(seed);
            return 0;
        }

        static int Quit(lua_State* state) {
            exit(0);
            return 0;
        }

        static int AddConsoleMessage(lua_State* state) {
            const char* msg = luaL_checkstring(state, 1);
            int type = luaL_optinteger(state, 2, 0);
            LuaEngine* engine = getEngine(state);
            if (engine) {
                engine->addConsoleMessage(msg, type);
            }
            return 0;
        }

        // Noise functions
        static Noise& getNoise() {
            static Noise noise(42);
            return noise;
        }

        static int SetNoiseSeed(lua_State* state) {
            uint32_t seed = static_cast<uint32_t>(luaL_checkinteger(state, 1));
            getNoise().setSeed(seed);
            return 0;
        }

        static int PerlinNoise1D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            lua_pushnumber(state, getNoise().perlin1D(x));
            return 1;
        }

        static int PerlinNoise2D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            lua_pushnumber(state, getNoise().perlin2D(x, y));
            return 1;
        }

        static int PerlinNoise3D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            float z = luaL_checknumber(state, 3);
            lua_pushnumber(state, getNoise().perlin3D(x, y, z));
            return 1;
        }

        static int SimplexNoise2D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            lua_pushnumber(state, getNoise().simplex2D(x, y));
            return 1;
        }

        static int ValueNoise2D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            lua_pushnumber(state, getNoise().value2D(x, y));
            return 1;
        }

        static int FbmNoise2D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            int octaves = static_cast<int>(luaL_optinteger(state, 3, 6));
            float lacunarity = static_cast<float>(luaL_optnumber(state, 4, 2.0));
            float gain = static_cast<float>(luaL_optnumber(state, 5, 0.5));
            lua_pushnumber(state, getNoise().fbm2D(x, y, octaves, lacunarity, gain));
            return 1;
        }

        static int FbmSimplexNoise2D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            int octaves = static_cast<int>(luaL_optinteger(state, 3, 6));
            float lacunarity = static_cast<float>(luaL_optnumber(state, 4, 2.0));
            float gain = static_cast<float>(luaL_optnumber(state, 5, 0.5));
            lua_pushnumber(state, getNoise().fbmSimplex2D(x, y, octaves, lacunarity, gain));
            return 1;
        }

        static int DomainWarpNoise2D(lua_State* state) {
            float x = luaL_checknumber(state, 1);
            float y = luaL_checknumber(state, 2);
            float warpScale = static_cast<float>(luaL_optnumber(state, 3, 1.0));
            lua_pushnumber(state, getNoise().domainWarp2D(x, y, warpScale));
            return 1;
        }

        // Render order functions
        static int SetRenderOrder(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            std::vector<std::string> order;
            if (lua_istable(state, 1)) {
                int len = static_cast<int>(lua_rawlen(state, 1));
                for (int i = 1; i <= len; i++) {
                    lua_rawgeti(state, 1, i);
                    const char* name = lua_tostring(state, -1);
                    if (name) order.push_back(name);
                    lua_pop(state, 1);
                }
            }
            engine->setRenderOrder(order);
            return 0;
        }

        static int GetRenderOrder(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_newtable(state); return 1; }
            const auto& order = engine->getRenderOrder();
            lua_newtable(state);
            for (size_t i = 0; i < order.size(); i++) {
                lua_pushstring(state, order[i].c_str());
                lua_rawseti(state, -2, static_cast<lua_Integer>(i + 1));
            }
            return 1;
        }
    };
}
