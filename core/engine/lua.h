#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>
#include <world.h>
#include <subject/sprite2D.h>
#include <subject/animatad2D.h>
#include <subject/text.h>
#include <subject/audio.h>
#include <graphics/mesh.h>
#include <utils/input/input.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace LuaEngine {
    
    // Forward declarations
    class LuaEngine;
    
    // Helper structure to track registered objects
    struct RegisteredObject {
        enum Type { SPRITE, ANIMATION, TEXT, MESH, UNKNOWN } type;
        void* ptr;
        Entity entity;
        
        RegisteredObject() : type(UNKNOWN), ptr(nullptr), entity(0) {}
        RegisteredObject(Type t, void* p, Entity e = 0) : type(t), ptr(p), entity(e) {}
    };
    
    // Script info structure
    struct ScriptInfo {
        std::string name;
        std::string path;
        bool enabled;
        bool loaded;
        
        ScriptInfo() : enabled(true), loaded(false) {}
        ScriptInfo(const std::string& n, const std::string& p) : name(n), path(p), enabled(true), loaded(false) {}
    };
    
    // ==================== LUA ENGINE CLASS ====================
    class LuaEngine {
    private:
        lua_State* state;
        World<Sprite2D> spriteWorld;
        World<Animation2D> animationWorld;
        World<Text> textWorld;
        World<Mesh2D> meshWorld;
        
        Audio audioEngine;
        
        float deltaTime;
        bool initialized;
        bool audioInitialized;
        
        std::unordered_map<Entity, RegisteredObject> objectMap;
        std::unordered_map<std::string, int> soundNameToId;
        
        // Script management
        std::vector<ScriptInfo> scripts;
        std::string scriptsListPath;
        
        void registerCFunctions();
        bool parseScriptsList(const std::string& listPath);
        
    public:
        LuaEngine();
        ~LuaEngine();
        
        int Init();
        void shutdown();
        
        // Script management
        bool loadScriptsFromList(const std::string& listPath);
        bool loadScript(const std::string& scriptPath);
        bool reloadScript(const std::string& scriptPath);
        bool reloadAllScripts();
        void enableScript(const std::string& scriptName, bool enabled);
        bool isScriptLoaded(const std::string& scriptName) const;
        std::vector<std::string> getLoadedScripts() const;
        
        // Lua callbacks
        bool callFunction(const std::string& funcName);
        bool callUpdate(float dt);
        bool callDraw();
        
        // Sprite management
        Entity createSprite(const std::string& texturePath, const Vector2& position);
        void spriteSetPosition(Entity entity, const Vector2& pos);
        Vector2 spriteGetPosition(Entity entity);
        void spriteSetSize(Entity entity, const Vector2& size);
        Vector2 spriteGetSize(Entity entity);
        void spriteSetOrigin(Entity entity, const Vector2& origin);
        void spriteSetRotation(Entity entity, float rotation);
        void spriteSetColor(Entity entity, const Vector4& color);
        void spriteSetVisible(Entity entity, bool visible);
        void drawAllSprites();
        
        // Animation management
        Entity createAnimation(const std::string& path, bool isGif, const Vector2& position);
        Entity createAnimationFromSheet(const std::string& texturePath, int frameWidth, int frameHeight,
                                         int totalFrames, int framesPerRow, int frameDelayMs,
                                         const Vector2& position);
        void animationPlay(Entity entity);
        void animationPause(Entity entity);
        void animationStop(Entity entity);
        void animationRestart(Entity entity);
        void animationSetLooping(Entity entity, bool loop);
        void animationSetSpeed(Entity entity, float speed);
        void animationSetFrame(Entity entity, int frame);
        int animationGetFrameCount(Entity entity);
        bool animationIsPlaying(Entity entity);
        void animationSetPosition(Entity entity, const Vector2& pos);
        void animationSetSize(Entity entity, const Vector2& size);
        void updateAllAnimations(float dt);
        void drawAllAnimations();
        
        // Text management
        Entity createText(const std::string& fontPath, const std::string& text, 
                          const Vector2& position, int fontSize);
        void textSetString(Entity entity, const std::string& text);
        std::string textGetString(Entity entity);
        void textSetPosition(Entity entity, const Vector2& pos);
        void textSetColor(Entity entity, const Vector4& color);
        void textSetScale(Entity entity, float scale);
        void textSetVisible(Entity entity, bool visible);
        void drawAllTexts();
        void setTextScreenSize(int width, int height);
        
        // Mesh management
        Entity createMesh();
        void meshSetData(Entity entity, const std::vector<Mesh2D::Vertex>& vertices, const std::vector<GLuint>& indices);
        void meshDraw(Entity entity);
        void drawAllMeshes();
        
        // Audio management
        int loadSound(const std::string& filename, const std::string& soundName, bool loop);
        void playSound(const std::string& soundName);
        void playSoundById(int soundId);
        void stopSound(const std::string& soundName);
        void stopSoundById(int soundId);
        void setSoundVolume(const std::string& soundName, float volume);
        void setSoundVolumeById(int soundId, float volume);
        bool isSoundPlaying(const std::string& soundName);
        void stopAllSounds();
        void updateAudio();
        
        // General
        void destroyEntity(Entity entity);
        float getDeltaTime() const;
        void setDeltaTime(float dt);
        lua_State* getState();
        bool isInitialized() const;
        Audio& getAudio();
        
        void drawAll();
        void updateAll(float dt);
    };
    
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
        // Script management functions
        static int LoadScript(lua_State* state) {
            const char* scriptPath = luaL_checkstring(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
            bool result = engine->loadScript(scriptPath);
            lua_pushboolean(state, result);
            return 1;
        }
        
        static int LoadScriptsFromList(lua_State* state) {
            const char* listPath = luaL_checkstring(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
            bool result = engine->loadScriptsFromList(listPath);
            lua_pushboolean(state, result);
            return 1;
        }
        
        static int ReloadScript(lua_State* state) {
            const char* scriptPath = luaL_checkstring(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
            bool result = engine->reloadScript(scriptPath);
            lua_pushboolean(state, result);
            return 1;
        }
        
        static int ReloadAllScripts(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
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
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
            lua_pushboolean(state, engine->isScriptLoaded(scriptName));
            return 1;
        }
        
        static int GetLoadedScripts(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                return 1;
            }
            
            std::vector<std::string> scripts = engine->getLoadedScripts();
            lua_newtable(state);
            for (size_t i = 0; i < scripts.size(); i++) {
                lua_pushstring(state, scripts[i].c_str());
                lua_rawseti(state, -2, i + 1);
            }
            return 1;
        }
        
        // Sprite functions
        static int NewSprite(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                return 1;
            }
            
            const char* texturePath = luaL_checkstring(state, 1);
            float x = luaL_optnumber(state, 2, 0);
            float y = luaL_optnumber(state, 3, 0);
            
            std::cout << "NewSprite: Creating sprite with texture: " << texturePath << " at (" << x << ", " << y << ")" << std::endl;
            
            Entity entity = engine->createSprite(texturePath, Vector2(x, y));
            
            std::cout << "NewSprite: Entity ID = " << entity << std::endl;
            
            lua_pushinteger(state, entity);
            return 1;
        }
        
        static int SpriteSetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->spriteSetPosition(entity, Vector2(x, y));
            return 0;
        }
        
        static int SpriteSetSize(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float w = luaL_checknumber(state, 2);
            float h = luaL_checknumber(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->spriteSetSize(entity, Vector2(w, h));
            return 0;
        }
        
        static int SpriteSetOrigin(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->spriteSetOrigin(entity, Vector2(x, y));
            return 0;
        }
        
        static int SpriteSetRotation(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float rotation = luaL_checknumber(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->spriteSetRotation(entity, rotation);
            return 0;
        }
        
        static int SpriteSetColor(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float r = luaL_checknumber(state, 2);
            float g = luaL_checknumber(state, 3);
            float b = luaL_checknumber(state, 4);
            float a = luaL_optnumber(state, 5, 1.0f);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->spriteSetColor(entity, Vector4(r, g, b, a));
            return 0;
        }
        
        static int SpriteSetVisible(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool visible = lua_toboolean(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->spriteSetVisible(entity, visible);
            return 0;
        }
        
        static int SpriteGetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            
            Vector2 pos = engine->spriteGetPosition(entity);
            lua_pushnumber(state, pos.x);
            lua_pushnumber(state, pos.y);
            return 2;
        }
        
        static int SpriteGetSize(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                lua_pushnil(state);
                return 2;
            }
            
            Vector2 size = engine->spriteGetSize(entity);
            lua_pushnumber(state, size.x);
            lua_pushnumber(state, size.y);
            return 2;
        }
        
        // Animation functions
        static int NewAnimation(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                return 1;
            }
            
            const char* path = luaL_checkstring(state, 1);
            bool isGif = lua_toboolean(state, 2);
            float x = luaL_optnumber(state, 3, 0);
            float y = luaL_optnumber(state, 4, 0);
            
            Entity entity = engine->createAnimation(path, isGif, Vector2(x, y));
            lua_pushinteger(state, entity);
            return 1;
        }
        
        static int NewAnimationFromSheet(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                return 1;
            }
            
            const char* texturePath = luaL_checkstring(state, 1);
            int frameWidth = luaL_checkinteger(state, 2);
            int frameHeight = luaL_checkinteger(state, 3);
            int totalFrames = luaL_checkinteger(state, 4);
            int framesPerRow = luaL_checkinteger(state, 5);
            int frameDelayMs = luaL_optinteger(state, 6, 100);
            float x = luaL_optnumber(state, 7, 0);
            float y = luaL_optnumber(state, 8, 0);
            
            Entity entity = engine->createAnimationFromSheet(texturePath, frameWidth, frameHeight, 
                                                              totalFrames, framesPerRow, frameDelayMs, 
                                                              Vector2(x, y));
            
            lua_pushinteger(state, entity);
            return 1;
        }
        
        static int AnimationPlay(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationPlay(entity);
            return 0;
        }
        
        static int AnimationPause(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationPause(entity);
            return 0;
        }
        
        static int AnimationStop(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationStop(entity);
            return 0;
        }
        
        static int AnimationRestart(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationRestart(entity);
            return 0;
        }
        
        static int AnimationSetLooping(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool loop = lua_toboolean(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationSetLooping(entity, loop);
            return 0;
        }
        
        static int AnimationSetSpeed(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float speed = luaL_checknumber(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationSetSpeed(entity, speed);
            return 0;
        }
        
        static int AnimationSetFrame(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            int frame = luaL_checkinteger(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationSetFrame(entity, frame);
            return 0;
        }
        
        static int AnimationGetFrameCount(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushinteger(state, 0);
                return 1;
            }
            
            lua_pushinteger(state, engine->animationGetFrameCount(entity));
            return 1;
        }
        
        static int AnimationIsPlaying(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
            lua_pushboolean(state, engine->animationIsPlaying(entity));
            return 1;
        }
        
        static int AnimationSetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationSetPosition(entity, Vector2(x, y));
            return 0;
        }
        
        static int AnimationSetSize(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float w = luaL_checknumber(state, 2);
            float h = luaL_checknumber(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->animationSetSize(entity, Vector2(w, h));
            return 0;
        }
        
        // Text functions
        static int NewText(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                return 1;
            }
            
            const char* fontPath = luaL_checkstring(state, 1);
            const char* text = luaL_checkstring(state, 2);
            float x = luaL_optnumber(state, 3, 0);
            float y = luaL_optnumber(state, 4, 0);
            int fontSize = luaL_optinteger(state, 5, 48);
            
            Entity entity = engine->createText(fontPath, text, Vector2(x, y), fontSize);
            lua_pushinteger(state, entity);
            return 1;
        }
        
        static int TextSetString(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            const char* text = luaL_checkstring(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->textSetString(entity, text);
            return 0;
        }
        
        static int TextGetString(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushstring(state, "");
                return 1;
            }
            
            lua_pushstring(state, engine->textGetString(entity).c_str());
            return 1;
        }
        
        static int TextSetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float x = luaL_checknumber(state, 2);
            float y = luaL_checknumber(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->textSetPosition(entity, Vector2(x, y));
            return 0;
        }
        
        static int TextSetColor(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float r = luaL_checknumber(state, 2);
            float g = luaL_checknumber(state, 3);
            float b = luaL_checknumber(state, 4);
            float a = luaL_optnumber(state, 5, 1.0f);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->textSetColor(entity, Vector4(r, g, b, a));
            return 0;
        }
        
        static int TextSetScale(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float scale = luaL_checknumber(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->textSetScale(entity, scale);
            return 0;
        }
        
        static int TextSetVisible(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool visible = lua_toboolean(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->textSetVisible(entity, visible);
            return 0;
        }
        
        // Mesh functions
        static int NewMesh(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushnil(state);
                return 1;
            }
            
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
        
        // Audio functions
        static int LoadSound(lua_State* state) {
            const char* filename = luaL_checkstring(state, 1);
            const char* soundName = luaL_checkstring(state, 2);
            bool loop = lua_toboolean(state, 3);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushinteger(state, -1);
                return 1;
            }
            
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
        
        static int StopSoundById(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->stopSoundById(soundId);
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
        
        static int SetSoundVolumeById(lua_State* state) {
            int soundId = luaL_checkinteger(state, 1);
            float volume = luaL_checknumber(state, 2);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->setSoundVolumeById(soundId, volume);
            return 0;
        }
        
        static int IsSoundPlaying(lua_State* state) {
            const char* soundName = luaL_checkstring(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) {
                lua_pushboolean(state, false);
                return 1;
            }
            
            lua_pushboolean(state, engine->isSoundPlaying(soundName));
            return 1;
        }
        
        static int StopAllSounds(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->stopAllSounds();
            return 0;
        }
        
        // Input functions
        static int IsKeyPressed(lua_State* state) {
            int key = luaL_checkinteger(state, 1);
            bool pressed = Input::getInstance().isKeyPressed(key);
            lua_pushboolean(state, pressed);
            return 1;
        }
        
        static int IsKeyJustPressed(lua_State* state) {
            int key = luaL_checkinteger(state, 1);
            bool pressed = Input::getInstance().isKeyJustPressed(key);
            lua_pushboolean(state, pressed);
            return 1;
        }
        
        static int IsKeyJustReleased(lua_State* state) {
            int key = luaL_checkinteger(state, 1);
            bool released = Input::getInstance().isKeyJustReleased(key);
            lua_pushboolean(state, released);
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
            bool pressed = Input::getInstance().isMouseButtonPressed(button);
            lua_pushboolean(state, pressed);
            return 1;
        }
        
        static int IsMouseButtonJustPressed(lua_State* state) {
            int button = luaL_checkinteger(state, 1);
            bool pressed = Input::getInstance().isMouseButtonJustPressed(button);
            lua_pushboolean(state, pressed);
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
                    const char* str = lua_tostring(state, i);
                    std::cout << str;
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
            if (!engine) {
                lua_pushnumber(state, 0);
                return 1;
            }
            lua_pushnumber(state, engine->getDeltaTime());
            return 1;
        }
        
        static int DestroyEntity(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            
            engine->destroyEntity(entity);
            return 0;
        }
        
        static int Quit(lua_State* state) {
            exit(0);
            return 0;
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
    };
    
    // ==================== LUA REGISTRATION TABLE ====================
    static const luaL_Reg BlazeBolt[] = {
        // Script management
        {"LoadScript", _functions::LoadScript},
        {"LoadScriptsFromList", _functions::LoadScriptsFromList},
        {"ReloadScript", _functions::ReloadScript},
        {"ReloadAllScripts", _functions::ReloadAllScripts},
        {"EnableScript", _functions::EnableScript},
        {"IsScriptLoaded", _functions::IsScriptLoaded},
        {"GetLoadedScripts", _functions::GetLoadedScripts},
        
        // Sprite functions
        {"NewSprite", _functions::NewSprite},
        {"SpriteSetPosition", _functions::SpriteSetPosition},
        {"SpriteSetSize", _functions::SpriteSetSize},
        {"SpriteSetOrigin", _functions::SpriteSetOrigin},
        {"SpriteSetRotation", _functions::SpriteSetRotation},
        {"SpriteSetColor", _functions::SpriteSetColor},
        {"SpriteSetVisible", _functions::SpriteSetVisible},
        {"SpriteGetPosition", _functions::SpriteGetPosition},
        {"SpriteGetSize", _functions::SpriteGetSize},
        
        // Animation functions
        {"NewAnimation", _functions::NewAnimation},
        {"NewAnimationFromSheet", _functions::NewAnimationFromSheet},
        {"AnimationPlay", _functions::AnimationPlay},
        {"AnimationPause", _functions::AnimationPause},
        {"AnimationStop", _functions::AnimationStop},
        {"AnimationRestart", _functions::AnimationRestart},
        {"AnimationSetLooping", _functions::AnimationSetLooping},
        {"AnimationSetSpeed", _functions::AnimationSetSpeed},
        {"AnimationSetFrame", _functions::AnimationSetFrame},
        {"AnimationGetFrameCount", _functions::AnimationGetFrameCount},
        {"AnimationIsPlaying", _functions::AnimationIsPlaying},
        {"AnimationSetPosition", _functions::AnimationSetPosition},
        {"AnimationSetSize", _functions::AnimationSetSize},
        
        // Text functions
        {"NewText", _functions::NewText},
        {"TextSetString", _functions::TextSetString},
        {"TextGetString", _functions::TextGetString},
        {"TextSetPosition", _functions::TextSetPosition},
        {"TextSetColor", _functions::TextSetColor},
        {"TextSetScale", _functions::TextSetScale},
        {"TextSetVisible", _functions::TextSetVisible},
        
        // Mesh functions
        {"NewMesh", _functions::NewMesh},
        {"MeshSetData", _functions::MeshSetData},
        {"MeshDraw", _functions::MeshDraw},
        
        // Audio functions
        {"LoadSound", _functions::LoadSound},
        {"PlaySound", _functions::PlaySound},
        {"PlaySoundById", _functions::PlaySoundById},
        {"StopSound", _functions::StopSound},
        {"StopSoundById", _functions::StopSoundById},
        {"SetSoundVolume", _functions::SetSoundVolume},
        {"SetSoundVolumeById", _functions::SetSoundVolumeById},
        {"IsSoundPlaying", _functions::IsSoundPlaying},
        {"StopAllSounds", _functions::StopAllSounds},
        
        // Input functions
        {"IsKeyPressed", _functions::IsKeyPressed},
        {"IsKeyJustPressed", _functions::IsKeyJustPressed},
        {"IsKeyJustReleased", _functions::IsKeyJustReleased},
        {"GetMouseX", _functions::GetMouseX},
        {"GetMouseY", _functions::GetMouseY},
        {"GetMouseDeltaX", _functions::GetMouseDeltaX},
        {"GetMouseDeltaY", _functions::GetMouseDeltaY},
        {"IsMouseButtonPressed", _functions::IsMouseButtonPressed},
        {"IsMouseButtonJustPressed", _functions::IsMouseButtonJustPressed},
        {"GetScrollY", _functions::GetScrollY},
        
        // Utility functions
        {"Print", _functions::Print},
        {"GetDeltaTime", _functions::GetDeltaTime},
        {"Destroy", _functions::DestroyEntity},
        {"Quit", _functions::Quit},
        {"GetTime", _functions::GetTime},
        {"Random", _functions::Random},
        {"RandomInt", _functions::RandomInt},
        {"SetRandomSeed", _functions::SetRandomSeed},
        
        {nullptr, nullptr}
    };
}

// ==================== IMPLEMENTATION ====================
namespace LuaEngine {
    
    // Constructor
    LuaEngine::LuaEngine() : state(nullptr), deltaTime(0), initialized(false), audioInitialized(false) {}
    
    // Destructor
    LuaEngine::~LuaEngine() {
        shutdown();
    }
    
    // Parse scripts list
    bool LuaEngine::parseScriptsList(const std::string& listPath) {
        std::ifstream file(listPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open scripts list: " << listPath << std::endl;
            return false;
        }
        
        scripts.clear();
        std::string line;
        
        while (std::getline(file, line)) {
            // Remove whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Parse format: "name=path" or just "path"
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string name = line.substr(0, eqPos);
                std::string path = line.substr(eqPos + 1);
                scripts.emplace_back(name, path);
            } else {
                std::string name = std::filesystem::path(line).stem().string();
                scripts.emplace_back(name, line);
            }
        }
        
        file.close();
        std::cout << "Loaded " << scripts.size() << " scripts from " << listPath << std::endl;
        return true;
    }
    
    // Register C functions
    void LuaEngine::registerCFunctions() {
        lua_newtable(state);
        
        for (const luaL_Reg* reg = BlazeBolt; reg->name != nullptr; ++reg) {
            lua_pushcfunction(state, reg->func);
            lua_setfield(state, -2, reg->name);
        }
        
        lua_setglobal(state, "BlazeBolt");
        
        // Store engine pointer
        lua_pushlightuserdata(state, this);
        lua_setglobal(state, "__lua_engine");
        
        // Key constants
        lua_newtable(state);
        
        // Буквы A-Z
        lua_pushinteger(state, GLFW_KEY_A); lua_setfield(state, -2, "A");
        lua_pushinteger(state, GLFW_KEY_B); lua_setfield(state, -2, "B");
        lua_pushinteger(state, GLFW_KEY_C); lua_setfield(state, -2, "C");
        lua_pushinteger(state, GLFW_KEY_D); lua_setfield(state, -2, "D");
        lua_pushinteger(state, GLFW_KEY_E); lua_setfield(state, -2, "E");
        lua_pushinteger(state, GLFW_KEY_F); lua_setfield(state, -2, "F");
        lua_pushinteger(state, GLFW_KEY_G); lua_setfield(state, -2, "G");
        lua_pushinteger(state, GLFW_KEY_H); lua_setfield(state, -2, "H");
        lua_pushinteger(state, GLFW_KEY_I); lua_setfield(state, -2, "I");
        lua_pushinteger(state, GLFW_KEY_J); lua_setfield(state, -2, "J");
        lua_pushinteger(state, GLFW_KEY_K); lua_setfield(state, -2, "K");
        lua_pushinteger(state, GLFW_KEY_L); lua_setfield(state, -2, "L");
        lua_pushinteger(state, GLFW_KEY_M); lua_setfield(state, -2, "M");
        lua_pushinteger(state, GLFW_KEY_N); lua_setfield(state, -2, "N");
        lua_pushinteger(state, GLFW_KEY_O); lua_setfield(state, -2, "O");
        lua_pushinteger(state, GLFW_KEY_P); lua_setfield(state, -2, "P");
        lua_pushinteger(state, GLFW_KEY_Q); lua_setfield(state, -2, "Q");
        lua_pushinteger(state, GLFW_KEY_R); lua_setfield(state, -2, "R");
        lua_pushinteger(state, GLFW_KEY_S); lua_setfield(state, -2, "S");
        lua_pushinteger(state, GLFW_KEY_T); lua_setfield(state, -2, "T");
        lua_pushinteger(state, GLFW_KEY_U); lua_setfield(state, -2, "U");
        lua_pushinteger(state, GLFW_KEY_V); lua_setfield(state, -2, "V");
        lua_pushinteger(state, GLFW_KEY_W); lua_setfield(state, -2, "W");
        lua_pushinteger(state, GLFW_KEY_X); lua_setfield(state, -2, "X");
        lua_pushinteger(state, GLFW_KEY_Y); lua_setfield(state, -2, "Y");
        lua_pushinteger(state, GLFW_KEY_Z); lua_setfield(state, -2, "Z");
        
        // Цифры 0-9
        lua_pushinteger(state, GLFW_KEY_0); lua_setfield(state, -2, "NUM0");
        lua_pushinteger(state, GLFW_KEY_1); lua_setfield(state, -2, "NUM1");
        lua_pushinteger(state, GLFW_KEY_2); lua_setfield(state, -2, "NUM2");
        lua_pushinteger(state, GLFW_KEY_3); lua_setfield(state, -2, "NUM3");
        lua_pushinteger(state, GLFW_KEY_4); lua_setfield(state, -2, "NUM4");
        lua_pushinteger(state, GLFW_KEY_5); lua_setfield(state, -2, "NUM5");
        lua_pushinteger(state, GLFW_KEY_6); lua_setfield(state, -2, "NUM6");
        lua_pushinteger(state, GLFW_KEY_7); lua_setfield(state, -2, "NUM7");
        lua_pushinteger(state, GLFW_KEY_8); lua_setfield(state, -2, "NUM8");
        lua_pushinteger(state, GLFW_KEY_9); lua_setfield(state, -2, "NUM9");
        
        // Функциональные клавиши F1-F25
        lua_pushinteger(state, GLFW_KEY_F1); lua_setfield(state, -2, "F1");
        lua_pushinteger(state, GLFW_KEY_F2); lua_setfield(state, -2, "F2");
        lua_pushinteger(state, GLFW_KEY_F3); lua_setfield(state, -2, "F3");
        lua_pushinteger(state, GLFW_KEY_F4); lua_setfield(state, -2, "F4");
        lua_pushinteger(state, GLFW_KEY_F5); lua_setfield(state, -2, "F5");
        lua_pushinteger(state, GLFW_KEY_F6); lua_setfield(state, -2, "F6");
        lua_pushinteger(state, GLFW_KEY_F7); lua_setfield(state, -2, "F7");
        lua_pushinteger(state, GLFW_KEY_F8); lua_setfield(state, -2, "F8");
        lua_pushinteger(state, GLFW_KEY_F9); lua_setfield(state, -2, "F9");
        lua_pushinteger(state, GLFW_KEY_F10); lua_setfield(state, -2, "F10");
        lua_pushinteger(state, GLFW_KEY_F11); lua_setfield(state, -2, "F11");
        lua_pushinteger(state, GLFW_KEY_F12); lua_setfield(state, -2, "F12");
        lua_pushinteger(state, GLFW_KEY_F13); lua_setfield(state, -2, "F13");
        lua_pushinteger(state, GLFW_KEY_F14); lua_setfield(state, -2, "F14");
        lua_pushinteger(state, GLFW_KEY_F15); lua_setfield(state, -2, "F15");
        lua_pushinteger(state, GLFW_KEY_F16); lua_setfield(state, -2, "F16");
        lua_pushinteger(state, GLFW_KEY_F17); lua_setfield(state, -2, "F17");
        lua_pushinteger(state, GLFW_KEY_F18); lua_setfield(state, -2, "F18");
        lua_pushinteger(state, GLFW_KEY_F19); lua_setfield(state, -2, "F19");
        lua_pushinteger(state, GLFW_KEY_F20); lua_setfield(state, -2, "F20");
        lua_pushinteger(state, GLFW_KEY_F21); lua_setfield(state, -2, "F21");
        lua_pushinteger(state, GLFW_KEY_F22); lua_setfield(state, -2, "F22");
        lua_pushinteger(state, GLFW_KEY_F23); lua_setfield(state, -2, "F23");
        lua_pushinteger(state, GLFW_KEY_F24); lua_setfield(state, -2, "F24");
        lua_pushinteger(state, GLFW_KEY_F25); lua_setfield(state, -2, "F25");
        
        // Клавиши со стрелками
        lua_pushinteger(state, GLFW_KEY_UP); lua_setfield(state, -2, "UP");
        lua_pushinteger(state, GLFW_KEY_DOWN); lua_setfield(state, -2, "DOWN");
        lua_pushinteger(state, GLFW_KEY_LEFT); lua_setfield(state, -2, "LEFT");
        lua_pushinteger(state, GLFW_KEY_RIGHT); lua_setfield(state, -2, "RIGHT");
        
        // Модификаторы
        lua_pushinteger(state, GLFW_KEY_LEFT_SHIFT); lua_setfield(state, -2, "LEFT_SHIFT");
        lua_pushinteger(state, GLFW_KEY_RIGHT_SHIFT); lua_setfield(state, -2, "RIGHT_SHIFT");
        lua_pushinteger(state, GLFW_KEY_LEFT_CONTROL); lua_setfield(state, -2, "LEFT_CONTROL");
        lua_pushinteger(state, GLFW_KEY_RIGHT_CONTROL); lua_setfield(state, -2, "RIGHT_CONTROL");
        lua_pushinteger(state, GLFW_KEY_LEFT_ALT); lua_setfield(state, -2, "LEFT_ALT");
        lua_pushinteger(state, GLFW_KEY_RIGHT_ALT); lua_setfield(state, -2, "RIGHT_ALT");
        lua_pushinteger(state, GLFW_KEY_LEFT_SUPER); lua_setfield(state, -2, "LEFT_SUPER");
        lua_pushinteger(state, GLFW_KEY_RIGHT_SUPER); lua_setfield(state, -2, "RIGHT_SUPER");
        
        // Клавиши цифровой клавиатуры
        lua_pushinteger(state, GLFW_KEY_KP_0); lua_setfield(state, -2, "KP_0");
        lua_pushinteger(state, GLFW_KEY_KP_1); lua_setfield(state, -2, "KP_1");
        lua_pushinteger(state, GLFW_KEY_KP_2); lua_setfield(state, -2, "KP_2");
        lua_pushinteger(state, GLFW_KEY_KP_3); lua_setfield(state, -2, "KP_3");
        lua_pushinteger(state, GLFW_KEY_KP_4); lua_setfield(state, -2, "KP_4");
        lua_pushinteger(state, GLFW_KEY_KP_5); lua_setfield(state, -2, "KP_5");
        lua_pushinteger(state, GLFW_KEY_KP_6); lua_setfield(state, -2, "KP_6");
        lua_pushinteger(state, GLFW_KEY_KP_7); lua_setfield(state, -2, "KP_7");
        lua_pushinteger(state, GLFW_KEY_KP_8); lua_setfield(state, -2, "KP_8");
        lua_pushinteger(state, GLFW_KEY_KP_9); lua_setfield(state, -2, "KP_9");
        lua_pushinteger(state, GLFW_KEY_KP_DECIMAL); lua_setfield(state, -2, "KP_DECIMAL");
        lua_pushinteger(state, GLFW_KEY_KP_DIVIDE); lua_setfield(state, -2, "KP_DIVIDE");
        lua_pushinteger(state, GLFW_KEY_KP_MULTIPLY); lua_setfield(state, -2, "KP_MULTIPLY");
        lua_pushinteger(state, GLFW_KEY_KP_SUBTRACT); lua_setfield(state, -2, "KP_SUBTRACT");
        lua_pushinteger(state, GLFW_KEY_KP_ADD); lua_setfield(state, -2, "KP_ADD");
        lua_pushinteger(state, GLFW_KEY_KP_ENTER); lua_setfield(state, -2, "KP_ENTER");
        lua_pushinteger(state, GLFW_KEY_KP_EQUAL); lua_setfield(state, -2, "KP_EQUAL");
        
        // Специальные клавиши
        lua_pushinteger(state, GLFW_KEY_ENTER); lua_setfield(state, -2, "ENTER");
        lua_pushinteger(state, GLFW_KEY_ESCAPE); lua_setfield(state, -2, "ESCAPE");
        lua_pushinteger(state, GLFW_KEY_SPACE); lua_setfield(state, -2, "SPACE");
        lua_pushinteger(state, GLFW_KEY_TAB); lua_setfield(state, -2, "TAB");
        lua_pushinteger(state, GLFW_KEY_BACKSPACE); lua_setfield(state, -2, "BACKSPACE");
        lua_pushinteger(state, GLFW_KEY_DELETE); lua_setfield(state, -2, "DELETE");
        lua_pushinteger(state, GLFW_KEY_INSERT); lua_setfield(state, -2, "INSERT");
        lua_pushinteger(state, GLFW_KEY_HOME); lua_setfield(state, -2, "HOME");
        lua_pushinteger(state, GLFW_KEY_END); lua_setfield(state, -2, "END");
        lua_pushinteger(state, GLFW_KEY_PAGE_UP); lua_setfield(state, -2, "PAGE_UP");
        lua_pushinteger(state, GLFW_KEY_PAGE_DOWN); lua_setfield(state, -2, "PAGE_DOWN");
        lua_pushinteger(state, GLFW_KEY_CAPS_LOCK); lua_setfield(state, -2, "CAPS_LOCK");
        lua_pushinteger(state, GLFW_KEY_NUM_LOCK); lua_setfield(state, -2, "NUM_LOCK");
        lua_pushinteger(state, GLFW_KEY_SCROLL_LOCK); lua_setfield(state, -2, "SCROLL_LOCK");
        lua_pushinteger(state, GLFW_KEY_PRINT_SCREEN); lua_setfield(state, -2, "PRINT_SCREEN");
        lua_pushinteger(state, GLFW_KEY_PAUSE); lua_setfield(state, -2, "PAUSE");
        lua_pushinteger(state, GLFW_KEY_MENU); lua_setfield(state, -2, "MENU");
        
        lua_setglobal(state, "Keys");
        
        // Mouse button constants
        lua_newtable(state);
        lua_pushinteger(state, GLFW_MOUSE_BUTTON_LEFT); lua_setfield(state, -2, "LEFT");
        lua_pushinteger(state, GLFW_MOUSE_BUTTON_RIGHT); lua_setfield(state, -2, "RIGHT");
        lua_pushinteger(state, GLFW_MOUSE_BUTTON_MIDDLE); lua_setfield(state, -2, "MIDDLE");
        lua_setglobal(state, "MouseButtons");
    }
    
    // Initialize
    int LuaEngine::Init() {
        if (initialized) return 0;
        
        state = luaL_newstate();
        if (!state) {
            std::cerr << "Failed to create Lua state" << std::endl;
            return -1;
        }
        
        luaL_openlibs(state);
        registerCFunctions();
        
        audioInitialized = audioEngine.init();
        if (!audioInitialized) {
            std::cerr << "Warning: Failed to initialize audio" << std::endl;
        }
        
        initialized = true;
        std::cout << "LuaEngine initialized successfully" << std::endl;
        return 0;
    }
    
    // Shutdown
    void LuaEngine::shutdown() {
        if (audioInitialized) {
            audioEngine.shutdown();
            audioInitialized = false;
        }
        
        if (state) {
            lua_close(state);
            state = nullptr;
        }
        initialized = false;
    }
    
    // Load scripts from list
    bool LuaEngine::loadScriptsFromList(const std::string& listPath) {
        if (!initialized) {
            std::cerr << "Engine not initialized" << std::endl;
            return false;
        }
        
        scriptsListPath = listPath;
        
        if (!parseScriptsList(listPath)) {
            return false;
        }
        
        bool allSuccess = true;
        for (auto& script : scripts) {
            if (script.enabled) {
                if (loadScript(script.path)) {
                    script.loaded = true;
                    std::cout << "Loaded script: " << script.name << " (" << script.path << ")" << std::endl;
                } else {
                    script.loaded = false;
                    allSuccess = false;
                    std::cerr << "Failed to load script: " << script.name << std::endl;
                }
            }
        }
        
        return allSuccess;
    }
    
    // Load single script
    bool LuaEngine::loadScript(const std::string& scriptPath) {
        if (!initialized) {
            std::cerr << "Engine not initialized" << std::endl;
            return false;
        }
        
        if (luaL_dofile(state, scriptPath.c_str()) != LUA_OK) {
            std::cerr << "Error loading script " << scriptPath << ": " << lua_tostring(state, -1) << std::endl;
            lua_pop(state, 1);
            return false;
        }
        
        return true;
    }
    
    // Reload script
    bool LuaEngine::reloadScript(const std::string& scriptPath) {
        for (auto& script : scripts) {
            if (script.path == scriptPath || script.name == scriptPath) {
                if (!script.enabled) {
                    std::cout << "Script " << script.name << " is disabled, skipping reload" << std::endl;
                    return false;
                }
                
                std::cout << "Reloading script: " << script.name << std::endl;
                return loadScript(script.path);
            }
        }
        
        std::cout << "Reloading script: " << scriptPath << std::endl;
        return loadScript(scriptPath);
    }
    
    // Reload all scripts
    bool LuaEngine::reloadAllScripts() {
        if (scripts.empty()) {
            std::cerr << "No scripts list loaded, call loadScriptsFromList first" << std::endl;
            return false;
        }
        
        std::cout << "Reloading all scripts..." << std::endl;
        
        bool allSuccess = true;
        for (auto& script : scripts) {
            if (script.enabled) {
                if (loadScript(script.path)) {
                    script.loaded = true;
                    std::cout << "Reloaded script: " << script.name << std::endl;
                } else {
                    script.loaded = false;
                    allSuccess = false;
                    std::cerr << "Failed to reload script: " << script.name << std::endl;
                }
            }
        }
        
        return allSuccess;
    }
    
    // Enable/disable script
    void LuaEngine::enableScript(const std::string& scriptName, bool enabled) {
        for (auto& script : scripts) {
            if (script.name == scriptName) {
                script.enabled = enabled;
                if (enabled && !script.loaded) {
                    loadScript(script.path);
                    script.loaded = true;
                }
                break;
            }
        }
    }
    
    // Check if script is loaded
    bool LuaEngine::isScriptLoaded(const std::string& scriptName) const {
        for (const auto& script : scripts) {
            if (script.name == scriptName) {
                return script.loaded && script.enabled;
            }
        }
        return false;
    }
    
    // Get loaded scripts
    std::vector<std::string> LuaEngine::getLoadedScripts() const {
        std::vector<std::string> loaded;
        for (const auto& script : scripts) {
            if (script.loaded && script.enabled) {
                loaded.push_back(script.name);
            }
        }
        return loaded;
    }
    
    // Sprite implementations
    Entity LuaEngine::createSprite(const std::string& texturePath, const Vector2& position) {
        Sprite2D* sprite = new Sprite2D();
        sprite->setTexture(texturePath);
        sprite->setPosition(position);
        
        Entity entity = spriteWorld.spawn(sprite);
        
        objectMap[entity] = RegisteredObject(RegisteredObject::SPRITE, sprite, entity);
        
        std::cout << "createSprite: Created entity " << entity << " with texture " << texturePath << std::endl;
        
        return entity;
    }
    
    void LuaEngine::spriteSetPosition(Entity entity, const Vector2& pos) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) {
            sprite->setPosition(pos);
        } else {
            std::cerr << "spriteSetPosition: Entity " << entity << " not found!" << std::endl;
        }
    }
    
    Vector2 LuaEngine::spriteGetPosition(Entity entity) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) return sprite->getPosition();
        return Vector2(0, 0);
    }
    
    void LuaEngine::spriteSetSize(Entity entity, const Vector2& size) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) sprite->setSize(size);
    }
    
    Vector2 LuaEngine::spriteGetSize(Entity entity) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) return sprite->getSize();
        return Vector2(1, 1);
    }
    
    void LuaEngine::spriteSetOrigin(Entity entity, const Vector2& origin) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) sprite->setOrigin(origin);
    }
    
    void LuaEngine::spriteSetRotation(Entity entity, float rotation) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) sprite->setRotation(rotation);
    }
    
    void LuaEngine::spriteSetColor(Entity entity, const Vector4& color) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) sprite->setColor(color);
    }
    
    void LuaEngine::spriteSetVisible(Entity entity, bool visible) {
        Sprite2D* sprite = spriteWorld.getEntity(entity);
        if (sprite) sprite->setVisible(visible);
    }
    
    void LuaEngine::drawAllSprites() {
        for (const auto& pair : spriteWorld.getAllEntities()) {
            if (!pair.second && pair.first) {
                pair.first->draw();
            }
        }
    }
    
    // Animation implementations
    Entity LuaEngine::createAnimation(const std::string& path, bool isGif, const Vector2& position) {
        Animation2D* anim = new Animation2D();
        bool loaded = false;
        if (isGif) {
            loaded = anim->loadFromGIF(path);
        }
        if (loaded) {
            anim->setPosition(position);
            anim->play();
        }
        Entity entity = animationWorld.spawn(anim);
        if (loaded) {
            objectMap[entity] = RegisteredObject(RegisteredObject::ANIMATION, anim, entity);
            return entity;
        }
        delete anim;
        return 0;
    }
    
    Entity LuaEngine::createAnimationFromSheet(const std::string& texturePath, int frameWidth, int frameHeight,
                                                 int totalFrames, int framesPerRow, int frameDelayMs,
                                                 const Vector2& position) {
        Animation2D* anim = new Animation2D();
        anim->loadFromSpriteSheet(texturePath, frameWidth, frameHeight, totalFrames, framesPerRow, frameDelayMs);
        anim->setPosition(position);
        anim->play();
        Entity entity = animationWorld.spawn(anim);
        objectMap[entity] = RegisteredObject(RegisteredObject::ANIMATION, anim, entity);
        return entity;
    }
    
    void LuaEngine::animationPlay(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->play();
    }
    
    void LuaEngine::animationPause(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->pause();
    }
    
    void LuaEngine::animationStop(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->stop();
    }
    
    void LuaEngine::animationRestart(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->restart();
    }
    
    void LuaEngine::animationSetLooping(Entity entity, bool loop) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->setLooping(loop);
    }
    
    void LuaEngine::animationSetSpeed(Entity entity, float speed) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->setSpeed(speed);
    }
    
    void LuaEngine::animationSetFrame(Entity entity, int frame) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->setFrame(frame);
    }
    
    int LuaEngine::animationGetFrameCount(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) return anim->getFrameCount();
        return 0;
    }
    
    bool LuaEngine::animationIsPlaying(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) return anim->isPlaying();
        return false;
    }
    
    void LuaEngine::animationSetPosition(Entity entity, const Vector2& pos) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->setPosition(pos);
    }
    
    void LuaEngine::animationSetSize(Entity entity, const Vector2& size) {
        Animation2D* anim = animationWorld.getEntity(entity);
        if (anim) anim->setSize(size);
    }
    
    void LuaEngine::updateAllAnimations(float dt) {
        for (const auto& pair : animationWorld.getAllEntities()) {
            if (!pair.second && pair.first) {
                pair.first->update(dt);
            }
        }
    }
    
    void LuaEngine::drawAllAnimations() {
        for (const auto& pair : animationWorld.getAllEntities()) {
            if (!pair.second && pair.first) {
                pair.first->draw();
            }
        }
    }
    
    // Text implementations
    Entity LuaEngine::createText(const std::string& fontPath, const std::string& text, 
                                  const Vector2& position, int fontSize) {
        Text* textObj = new Text(fontPath, fontSize);
        textObj->setText(text);
        textObj->setPosition(position);
        Entity entity = textWorld.spawn(textObj);
        objectMap[entity] = RegisteredObject(RegisteredObject::TEXT, textObj, entity);
        return entity;
    }
    
    void LuaEngine::textSetString(Entity entity, const std::string& text) {
        Text* txt = textWorld.getEntity(entity);
        if (txt) txt->setText(text);
    }
    
    std::string LuaEngine::textGetString(Entity entity) {
        Text* txt = textWorld.getEntity(entity);
        if (txt) return txt->getText();
        return "";
    }
    
    void LuaEngine::textSetPosition(Entity entity, const Vector2& pos) {
        Text* txt = textWorld.getEntity(entity);
        if (txt) txt->setPosition(pos);
    }
    
    void LuaEngine::textSetColor(Entity entity, const Vector4& color) {
        Text* txt = textWorld.getEntity(entity);
        if (txt) txt->setColor(color);
    }
    
    void LuaEngine::textSetScale(Entity entity, float scale) {
        Text* txt = textWorld.getEntity(entity);
        if (txt) txt->setScale(scale);
    }
    
    void LuaEngine::textSetVisible(Entity entity, bool visible) {
        Text* txt = textWorld.getEntity(entity);
        if (txt) txt->setVisible(visible);
    }
    
    void LuaEngine::drawAllTexts() {
        for (const auto& pair : textWorld.getAllEntities()) {
            if (!pair.second && pair.first) {
                pair.first->draw();
            }
        }
    }
    
    void LuaEngine::setTextScreenSize(int width, int height) {
        for (const auto& pair : textWorld.getAllEntities()) {
            if (!pair.second && pair.first) {
                pair.first->setScreenSize(width, height);
            }
        }
    }
    
    // Mesh implementations
    Entity LuaEngine::createMesh() {
        Mesh2D* mesh = new Mesh2D();
        Entity entity = meshWorld.spawn(mesh);
        objectMap[entity] = RegisteredObject(RegisteredObject::MESH, mesh, entity);
        return entity;
    }
    
    void LuaEngine::meshSetData(Entity entity, const std::vector<Mesh2D::Vertex>& vertices, const std::vector<GLuint>& indices) {
        Mesh2D* mesh = meshWorld.getEntity(entity);
        if (mesh) mesh->setData(vertices, indices);
    }
    
    void LuaEngine::meshDraw(Entity entity) {
        Mesh2D* mesh = meshWorld.getEntity(entity);
        if (mesh) mesh->draw();
    }
    
    void LuaEngine::drawAllMeshes() {
        for (const auto& pair : meshWorld.getAllEntities()) {
            if (!pair.second && pair.first) {
                pair.first->draw();
            }
        }
    }
    
    // Audio implementations
    int LuaEngine::loadSound(const std::string& filename, const std::string& soundName, bool loop) {
        if (!audioInitialized) return -1;
        int id = audioEngine.loadSound(filename, soundName, loop);
        if (id >= 0) {
            soundNameToId[soundName] = id;
        }
        return id;
    }
    
    void LuaEngine::playSound(const std::string& soundName) {
        if (!audioInitialized) return;
        audioEngine.play(soundName);
    }
    
    void LuaEngine::playSoundById(int soundId) {
        if (!audioInitialized) return;
        audioEngine.play(soundId);
    }
    
    void LuaEngine::stopSound(const std::string& soundName) {
        if (!audioInitialized) return;
        audioEngine.stop(soundName);
    }
    
    void LuaEngine::stopSoundById(int soundId) {
        if (!audioInitialized) return;
        audioEngine.stop(soundId);
    }
    
    void LuaEngine::setSoundVolume(const std::string& soundName, float volume) {
        if (!audioInitialized) return;
        audioEngine.setVolume(soundName, volume);
    }
    
    void LuaEngine::setSoundVolumeById(int soundId, float volume) {
        if (!audioInitialized) return;
        audioEngine.setVolume(soundId, volume);
    }
    
    bool LuaEngine::isSoundPlaying(const std::string& soundName) {
        if (!audioInitialized) return false;
        int id = audioEngine.getSourceIndex(soundName);
        if (id >= 0) {
            return audioEngine.isPlaying(id);
        }
        return false;
    }
    
    void LuaEngine::stopAllSounds() {
        if (!audioInitialized) return;
        audioEngine.stopAll();
    }
    
    void LuaEngine::updateAudio() {
        if (audioInitialized) {
            audioEngine.update();
        }
    }
    
    // General implementations
    bool LuaEngine::callFunction(const std::string& funcName) {
        if (!initialized) return false;
        
        lua_getglobal(state, funcName.c_str());
        if (lua_isfunction(state, -1)) {
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                std::cerr << "Error calling function " << funcName << ": " << lua_tostring(state, -1) << std::endl;
                lua_pop(state, 1);
                return false;
            }
            return true;
        }
        lua_pop(state, 1);
        return false;
    }
    
    bool LuaEngine::callUpdate(float dt) {
        deltaTime = dt;
        lua_getglobal(state, "Update");
        if (lua_isfunction(state, -1)) {
            lua_pushnumber(state, dt);
            if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
                std::cerr << "Error in Update: " << lua_tostring(state, -1) << std::endl;
                lua_pop(state, 1);
                return false;
            }
            return true;
        }
        lua_pop(state, 1);
        return false;
    }
    
    bool LuaEngine::callDraw() {
        if (!initialized) return false;
        
        lua_getglobal(state, "Draw");
        if (lua_isfunction(state, -1)) {
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                std::cerr << "Error in Draw: " << lua_tostring(state, -1) << std::endl;
                lua_pop(state, 1);
                return false;
            }
            return true;
        }
        lua_pop(state, 1);
        return false;
    }
    
    void LuaEngine::destroyEntity(Entity entity) {
        auto it = objectMap.find(entity);
        if (it != objectMap.end()) {
            switch (it->second.type) {
                case RegisteredObject::SPRITE:
                    spriteWorld.destroy(entity);
                    break;
                case RegisteredObject::ANIMATION:
                    animationWorld.destroy(entity);
                    break;
                case RegisteredObject::TEXT:
                    textWorld.destroy(entity);
                    break;
                case RegisteredObject::MESH:
                    meshWorld.destroy(entity);
                    break;
                default:
                    break;
            }
            objectMap.erase(it);
        }
    }
    
    float LuaEngine::getDeltaTime() const { return deltaTime; }
    void LuaEngine::setDeltaTime(float dt) { deltaTime = dt; }
    lua_State* LuaEngine::getState() { return state; }
    bool LuaEngine::isInitialized() const { return initialized; }
    Audio& LuaEngine::getAudio() { return audioEngine; }
    
    void LuaEngine::drawAll() {
        drawAllSprites();
        drawAllAnimations();
        drawAllTexts();
        drawAllMeshes();
    }
    
    void LuaEngine::updateAll(float dt) {
        deltaTime = dt;
        updateAllAnimations(dt);
        updateAudio();
    }
}