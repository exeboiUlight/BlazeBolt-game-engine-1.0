#pragma once
#include <engine/luaEngine.hpp>

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

        // Shader functions
        static int CreateShader(lua_State* state) {
            const char* name = luaL_checkstring(state, 1);
            const char* vertexPath = luaL_checkstring(state, 2);
            const char* fragmentPath = luaL_checkstring(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            unsigned int shaderId = engine->createShader(name, vertexPath, fragmentPath);
            lua_pushinteger(state, shaderId);
            return 1;
        }

        static int DestroyShader(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->destroyShader(shaderId);
            return 0;
        }

        static int SetEntityShader(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            unsigned int shaderId = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->setEntityShader(entity, shaderId);
            return 0;
        }

        static int GetEntityShader(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            unsigned int shaderId = engine->getEntityShader(entity);
            lua_pushinteger(state, shaderId);
            return 1;
        }

        static int UseShader(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Shader* shader = engine->getShader(shaderId);
            if (shader) {
                shader->use();
            }
            return 0;
        }

        static int SetShaderFloat(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float value = luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Shader* shader = engine->getShader(shaderId);
            if (shader) {
                shader->setFloat(name, value);
            }
            return 0;
        }

        static int SetShaderInt(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            int value = luaL_checkinteger(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Shader* shader = engine->getShader(shaderId);
            if (shader) {
                shader->setInt(name, value);
            }
            return 0;
        }

        static int SetShaderVec2(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float x = luaL_checknumber(state, 3);
            float y = luaL_checknumber(state, 4);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Shader* shader = engine->getShader(shaderId);
            if (shader) {
                shader->setVec2(name, x, y);
            }
            return 0;
        }

        static int SetShaderVec3(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float x = luaL_checknumber(state, 3);
            float y = luaL_checknumber(state, 4);
            float z = luaL_checknumber(state, 5);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Shader* shader = engine->getShader(shaderId);
            if (shader) {
                shader->setVec3(name, x, y, z);
            }
            return 0;
        }

        static int SetShaderVec4(lua_State* state) {
            unsigned int shaderId = luaL_checkinteger(state, 1);
            const char* name = luaL_checkstring(state, 2);
            float x = luaL_checknumber(state, 3);
            float y = luaL_checknumber(state, 4);
            float z = luaL_checknumber(state, 5);
            float w = luaL_checknumber(state, 6);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Shader* shader = engine->getShader(shaderId);
            if (shader) {
                shader->setVec4(name, x, y, z, w);
            }
            return 0;
        }

        static int SetWindowTitle(lua_State* state) {
            void* windowPtr = lua_touserdata(state, 1);
            const char* title = luaL_checkstring(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            auto window = std::shared_ptr<Window>(static_cast<Window*>(windowPtr), [](Window*){});
            engine->setWindowTitle(window, title);
            return 0;
        }

        static int SetWindowSize(lua_State* state) {
            void* windowPtr = lua_touserdata(state, 1);
            int width = luaL_checkinteger(state, 2);
            int height = luaL_checkinteger(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            auto window = std::shared_ptr<Window>(static_cast<Window*>(windowPtr), [](Window*){});
            engine->setWindowSize(window, width, height);
            return 0;
        }

        static int SetWindowIcon(lua_State* state) {
            void* windowPtr = lua_touserdata(state, 1);
            const char* iconPath = luaL_checkstring(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            auto window = std::shared_ptr<Window>(static_cast<Window*>(windowPtr), [](Window*){});
            engine->setWindowIcon(window, iconPath);
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
    };
}
