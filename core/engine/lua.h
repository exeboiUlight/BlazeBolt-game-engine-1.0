// lua.h - исправленная версия с функциями получения размера экрана
#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>
#include <world.h>
#include <physics/physics.h>
#include <subject/sprite2D.hpp>
#include <subject/animatad2D.h>
#include <subject/text2D.hpp>
#include <subject/audio.h>
#include <engine/managers.hpp>
#include <graphics/mesh.h>
#include <utils/input/input.h>
#include <graphics/window.h>
#include <graphics/shader.h>
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <gui/gui.h>

namespace LuaEngine {
    
    // Forward declaration
    class LuaEngine;
    
    // Helper structure to track registered objects
    struct RegisteredObject {
        enum Type { SPRITE, ANIMATION, TEXT, MESH, WINDOW, UNKNOWN } type;
        void* ptr;
        Entity entity;
        std::shared_ptr<Window> windowPtr;
        unsigned int shaderID;  // Custom shader ID (0 = use default)
        
        RegisteredObject() : type(UNKNOWN), ptr(nullptr), entity(0), shaderID(0) {}
        RegisteredObject(Type t, void* p, Entity e = 0) : type(t), ptr(p), entity(e), shaderID(0) {}
        RegisteredObject(std::shared_ptr<Window> w) : type(WINDOW), ptr(nullptr), entity(0), windowPtr(w), shaderID(0) {}
    };
    
    // Shader info structure
    struct ShaderInfo {
        std::string name;
        std::string vertexPath;
        std::string fragmentPath;
        Shader* shader;
        unsigned int id;
        
        ShaderInfo() : shader(nullptr), id(0) {}
        ShaderInfo(const std::string& n, const std::string& vp, const std::string& fp, unsigned int i)
            : name(n), vertexPath(vp), fragmentPath(fp), shader(nullptr), id(i) {}
    };
    
    // Script info structure with scene support
    struct ScriptInfo {
        std::string name;
        std::string path;
        bool enabled;
        bool loaded;
        bool isScene;
        std::vector<std::string> dependencies;
        
        ScriptInfo() : enabled(true), loaded(false), isScene(false) {}
        ScriptInfo(const std::string& n, const std::string& p) : name(n), path(p), enabled(true), loaded(false), isScene(false) {}
    };
    
    // Scene manager (forward declared, implemented after LuaEngine)
    class SceneManager {
    private:
        std::string currentScene;
        std::string nextScene;
        std::unordered_map<std::string, ScriptInfo> scenes;
        std::unordered_map<std::string, std::string> sceneJSONs;
        LuaEngine* engine;
        bool transitioning;
        float transitionTimer;
        
    public:
        SceneManager(LuaEngine* eng);
        
        void registerScene(const std::string& name, const std::string& path);
        void registerSceneJSON(const std::string& name, const std::string& jsonPath);
        bool loadScene(const std::string& name);
        void update(float dt);
        void performSceneSwitch();
        void callSceneLoad(const std::string& scene);
        void callSceneUnload(const std::string& scene);
        
        std::string getCurrentScene() const { return currentScene; }
        bool isTransitioning() const { return transitioning; }
        
        const std::string& getSceneJSON(const std::string& name) const {
            auto it = sceneJSONs.find(name);
            if (it != sceneJSONs.end()) return it->second;
            static std::string empty;
            return empty;
        }
    };
    
    // ==================== SCENE OBJECT ====================
    struct SceneObject {
        std::string name;
        std::string type;
        Vector2 position;
        Vector2 size;
        float rotation;
        Vector4 color;
        bool visible;
        std::string texturePath;
        std::string scriptPath;
        std::unordered_map<std::string, std::string> properties;
        
        SceneObject() : type("Node2D"), position(0, 0), size(0.1f, 0.1f), rotation(0),
                      color(1, 1, 1, 1), visible(true) {}
        
        std::string toJSON() const {
            std::string json = "{";
            json += "\"name\":\"" + name + "\",";
            json += "\"type\":\"" + type + "\",";
            json += "\"position\":[" + std::to_string(position.x) + "," + std::to_string(position.y) + "],";
            json += "\"size\":[" + std::to_string(size.x) + "," + std::to_string(size.y) + "],";
            json += "\"rotation\":" + std::to_string(rotation) + ",";
            json += "\"color\":[" + std::to_string(color.x) + "," + std::to_string(color.y) + "," 
                               + std::to_string(color.z) + "," + std::to_string(color.w) + "],";
            json += "\"visible\":" + std::string(visible ? "true" : "false") + ",";
            json += "\"texture\":\"" + texturePath + "\",";
            json += "\"script\":\"" + scriptPath + "\"";
            for (const auto& prop : properties) {
                json += ",\"" + prop.first + "\":\"" + prop.second + "\"";
            }
            json += "}";
            return json;
        }
        
        static SceneObject fromJSON(const std::string& json) {
            SceneObject obj;
            return obj;
        }
    };
    
    // ==================== LUA ENGINE CLASS ====================
    class LuaEngine {
    private:
        lua_State* state;
        World<BlazeBolt::Sprite2D> spriteWorld;
        World<Animation2D> animationWorld;
        World<BlazeBolt::Text2D> textWorld;
        World<Mesh2D> meshWorld;
        
        Audio audioEngine;
        
        PhysicsWorld physicsWorld;
        
        float deltaTime;
        bool initialized;
        bool audioInitialized;
        
        std::unordered_map<Entity, RegisteredObject> objectMap;
        std::unordered_map<std::string, int> soundNameToId;
        std::unordered_map<Entity, PhysicsBody*> physicsBodyMap;
        
        // Script management
        std::vector<ScriptInfo> scripts;
        std::vector<std::string> activeScripts;
        std::string scriptsListPath;
        std::string projectFileName; // .BlazeBoltProject
        
        // Scene management
        std::unique_ptr<SceneManager> sceneManager;
        
        // Additional windows
        std::vector<std::shared_ptr<Window>> additionalWindows;
        
        // Main window pointer
        Window* mainWindow;

    public:
        Gui::GuiSystem guiSystem;
        std::unordered_map<Entity, Gui::GuiWidget*> guiWidgetMap;
        Entity guiNextEntity;

    private:
        Matrix3x3 projectionViewMatrix2D;
        BlazeBolt::QuadVertexBufferObject2D quadVertexBufferObject;
        BlazeBolt::SpriteShader2D spriteShader2D;
        BlazeBolt::FontShader2D fontShader2D;
        BlazeBolt::SpriteMesh2D spriteMesh2D;
        BlazeBolt::TextureManager textureManager;
        BlazeBolt::FontManager fontManager;
        
        // Shader management
        std::unordered_map<unsigned int, ShaderInfo> shaders;
        std::unordered_map<Entity, unsigned int> entityShaderMap;
        unsigned int nextShaderId;
        
        void registerCFunctions();
        bool parseScriptsList(const std::string& listPath);
    public:
        LuaEngine(Window &window);
        ~LuaEngine();

        // Main window management
        Window* getMainWindow() const { return mainWindow; }
        int getScreenWidth() const { return mainWindow ? mainWindow->getWidth() : 0; }
        int getScreenHeight() const { return mainWindow ? mainWindow->getHeight() : 0; }
        
        // Script management
        bool loadScriptsFromList(const std::string& listPath);
        bool loadScript(const std::string& scriptPath);
        bool reloadScript(const std::string& scriptPath);
        bool reloadAllScripts();
        void enableScript(const std::string& scriptName, bool enabled);
        bool isScriptLoaded(const std::string& scriptName) const;
        std::vector<std::string> getLoadedScripts() const;
        void addActiveScript(const std::string& scriptName) { activeScripts.push_back(scriptName); }
        
        // Scene management
        SceneManager* getSceneManager() { return sceneManager.get(); }
        bool loadScene(const std::string& sceneName);
        
        // Lua callbacks
        bool callFunction(const std::string& funcName);
        bool callUpdate(float dt);
        bool callDraw();
        bool callEnd();
        
        // Sprite management
        // TODO: Replace this huge function definition list, all these implementations far below and the huge array of Lua function definitions with something that generates automatically, or at least has a structure.
        // e.g. When the types(subjects) will have a Node-based approach
        Entity createSprite(const std::string &texturePath, const Vector2 &position);
        void spriteSetTexture(Entity entity, const std::string &texturePath);
        void spriteSetPosition(Entity entity, const Vector2 &position);
        Vector2 spriteGetPosition(Entity entity);
        void spriteSetSize(Entity entity, const Vector2 &size);
        Vector2 spriteGetSize(Entity entity);
        void spriteSetOrigin(Entity entity, const Vector2 &origin);
        Vector2 spriteGetOrigin(Entity entity);
        void spriteSetRotation(Entity entity, float rotation);
        float spriteGetRotation(Entity entity);
        void spriteSetColor(Entity entity, const Vector4 &color);
        Vector4 spriteGetColor(Entity entity);
        void spriteSetVisible(Entity entity, bool visible);
        bool spriteIsVisible(Entity entity);
        void drawAllSprites() const;
        
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
        Entity createText(const std::string &fontPath, const std::string &text, const Vector2 &position);
        void textSetString(Entity entity, const std::string &text);
        std::string textGetString(Entity entity);
        void textSetPosition(Entity entity, const Vector2 &position);
        Vector2 textGetPosition(Entity entity);
        void textSetScale(Entity entity, const Vector2 &scale);
        Vector2 textGetScale(Entity entity);
        void textSetOrigin(Entity entity, const Vector2 &origin);
        Vector2 textGetOrigin(Entity entity);
        void textSetRotation(Entity entity, float rotation);
        float textGetRotation(Entity entity);
        void textSetColor(Entity entity, const Vector4 &color);
        Vector4 textGetColor(Entity entity);
        void textSetAlignment(Entity entity, BlazeBolt::Text2D::Alignment alignment);
        BlazeBolt::Text2D::Alignment textGetAlignment(Entity entity);
        void textSetVisible(Entity entity, bool visible);
        bool textIsVisible(Entity entity);
        void drawAllTexts();
        void setSpriteScreenSize(int width, int height);
        
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
        
        // Window management
        std::shared_ptr<Window> createWindow(int width, int height, const std::string& title);
        void destroyWindow(std::shared_ptr<Window> window);
        void setWindowTitle(std::shared_ptr<Window> window, const std::string& title);
        void setWindowSize(std::shared_ptr<Window> window, int width, int height);
        void setWindowPosition(std::shared_ptr<Window> window, int x, int y);
        void setWindowIcon(std::shared_ptr<Window> window, const std::string& iconPath);
        
        // Object deletion
        void destroyEntity(Entity entity);
        void destroyAllEntities();

        // Shader management
        unsigned int createShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
        void destroyShader(unsigned int shaderId);
        Shader* getShader(unsigned int shaderId) const;
        void setEntityShader(Entity entity, unsigned int shaderId);
        unsigned int getEntityShader(Entity entity);
        
        // Physics
        void physicsInit(float gravityX, float gravityY);
        void physicsSetGravity(float x, float y);
        void physicsGetGravity(float* x, float* y) const;
        Entity physicsCreateBody(int bodyType, float x, float y, float mass, float friction, float restitution);
        void physicsAddCircle(Entity bodyEntity, float radius, float offsetX, float offsetY);
        void physicsAddRectangle(Entity bodyEntity, float halfWidth, float halfHeight);
        void physicsSetLinearVelocity(Entity bodyEntity, float vx, float vy);
        void physicsGetLinearVelocity(Entity bodyEntity, float* vx, float* vy);
        void physicsSetAngularVelocity(Entity bodyEntity, float av);
        float physicsGetAngularVelocity(Entity bodyEntity);
        void physicsApplyForce(Entity bodyEntity, float fx, float fy);
        void physicsApplyForceAtPoint(Entity bodyEntity, float fx, float fy, float px, float py);
        void physicsApplyImpulse(Entity bodyEntity, float ix, float iy);
        void physicsApplyImpulseAtPoint(Entity bodyEntity, float ix, float iy, float px, float py);
        void physicsApplyTorque(Entity bodyEntity, float torque);
        void physicsSetPosition(Entity bodyEntity, float x, float y);
        void physicsGetPosition(Entity bodyEntity, float* x, float* y);
        void physicsSetAngle(Entity bodyEntity, float angle);
        float physicsGetAngle(Entity bodyEntity);
        void physicsSetGravityScale(Entity bodyEntity, float scale);
        float physicsGetGravityScale(Entity bodyEntity);
        void physicsSetActive(Entity bodyEntity, bool active);
        bool physicsIsActive(Entity bodyEntity);
        void physicsSetFixedRotation(Entity bodyEntity, bool fixed);
        bool physicsIsFixedRotation(Entity bodyEntity);
        void physicsSetBullet(Entity bodyEntity, bool bullet);
        bool physicsIsBullet(Entity bodyEntity);
        void physicsDestroyBody(Entity bodyEntity);
        float physicsGetMass(Entity bodyEntity);
        void physicsStep();
        void physicsSyncSprite(Entity bodyEntity, Entity spriteEntity);
        
        // General
        float getDeltaTime() const;
        void setDeltaTime(float dt);
        lua_State* getState();
        bool isInitialized() const;
        Audio& getAudio();
        
        void drawAll();
        void updateAll(float dt);
        
        void addConsoleMessage(const std::string& msg, int type = 0);

        Entity guiAddWidget(Gui::GuiWidget* widget);
        Gui::GuiWidget* guiGetWidget(Entity entity);
    };
    
    // ==================== SCENE MANAGER IMPLEMENTATION ====================
    inline SceneManager::SceneManager(LuaEngine* eng) : engine(eng), transitioning(false), transitionTimer(0) {}
    
    inline void SceneManager::registerScene(const std::string& name, const std::string& path) {
        ScriptInfo info(name, path);
        info.isScene = true;
        scenes[name] = info;
    }
    
    inline bool SceneManager::loadScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it == scenes.end()) return false;
        nextScene = name;
        transitioning = true;
        transitionTimer = 0;
        return true;
    }
    
    inline void SceneManager::update(float dt) {
        if (transitioning) {
            transitionTimer += dt;
            if (transitionTimer >= 0.5f) {
                performSceneSwitch();
                transitioning = false;
            }
        }
    }
    
    inline void SceneManager::performSceneSwitch() {
        if (!nextScene.empty()) {
            if (!currentScene.empty()) {
                callSceneUnload(currentScene);
            }
            
            auto it = scenes.find(nextScene);
            if (it != scenes.end()) {
                engine->loadScript(it->second.path);
                engine->addActiveScript(nextScene);
            }
            
            currentScene = nextScene;
            callSceneLoad(currentScene);
            nextScene.clear();
        }
    }
    
    inline void SceneManager::callSceneLoad(const std::string& scene) {
        if (!engine) return;
        lua_State* state = engine->getState();
        if (!state) return;
        
        std::string funcName = "On" + scene + "Load";
        lua_getglobal(state, funcName.c_str());
        if (lua_isfunction(state, -1)) {
            lua_pcall(state, 0, 0, 0);
        }
        lua_pop(state, 1);
    }
    
    inline void SceneManager::callSceneUnload(const std::string& scene) {
        if (!engine) return;
        lua_State* state = engine->getState();
        if (!state) return;
        
        std::string funcName = "On" + scene + "Unload";
        lua_getglobal(state, funcName.c_str());
        if (lua_isfunction(state, -1)) {
            lua_pcall(state, 0, 0, 0);
        }
        lua_pop(state, 1);
    }
    
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
        static int CreateAnimation(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
            const char* path = luaL_checkstring(state, 1);
            bool isGif = lua_toboolean(state, 2);
            float x = luaL_optnumber(state, 3, 0);
            float y = luaL_optnumber(state, 4, 0);
            Entity entity = engine->createAnimation(path, isGif, Vector2(x, y));
            lua_pushinteger(state, entity);
            return 1;
        }
        
        static int CreateAnimationFromSheet(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnil(state); return 1; }
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
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            lua_pushinteger(state, engine->animationGetFrameCount(entity));
            return 1;
        }
        
        static int AnimationIsPlaying(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushboolean(state, false); return 1; }
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
        
        static int IsKeyJustReleased(lua_State* state) {
            int key = luaL_checkinteger(state, 1);
            lua_pushboolean(state, Input::getInstance().isKeyJustReleased(key));
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

        // GUI functions
        static int GuiPanel(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            const char* id = luaL_checkstring(state, 1);
            float x = (float)luaL_optnumber(state, 2, 0);
            float y = (float)luaL_optnumber(state, 3, 0);
            float w = (float)luaL_optnumber(state, 4, 0);
            float h = (float)luaL_optnumber(state, 5, 0);
            if (w == 0) w = (float)engine->getScreenWidth();
            if (h == 0) h = (float)engine->getScreenHeight();
            auto widget = engine->guiSystem.addPanel(id, x, y, w, h);
            Entity entity = engine->guiAddWidget(widget);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int GuiButton(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            const char* id = luaL_checkstring(state, 1);
            float x = (float)luaL_checknumber(state, 2);
            float y = (float)luaL_checknumber(state, 3);
            float w = (float)luaL_checknumber(state, 4);
            float h = (float)luaL_checknumber(state, 5);
            const char* text = luaL_checkstring(state, 6);
            auto widget = engine->guiSystem.addButton(id, x, y, w, h, text);
            Entity entity = engine->guiAddWidget(widget);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int GuiLabel(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            const char* id = luaL_checkstring(state, 1);
            float x = (float)luaL_checknumber(state, 2);
            float y = (float)luaL_checknumber(state, 3);
            const char* text = luaL_checkstring(state, 4);
            auto widget = engine->guiSystem.addLabel(id, x, y, text);
            Entity entity = engine->guiAddWidget(widget);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int GuiSlider(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            const char* id = luaL_checkstring(state, 1);
            float x = (float)luaL_checknumber(state, 2);
            float y = (float)luaL_checknumber(state, 3);
            float w = (float)luaL_checknumber(state, 4);
            float minVal = (float)luaL_checknumber(state, 5);
            float maxVal = (float)luaL_checknumber(state, 6);
            float val = (float)luaL_optnumber(state, 7, minVal);
            auto widget = engine->guiSystem.addSlider(id, x, y, w, minVal, maxVal, val);
            Entity entity = engine->guiAddWidget(widget);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int GuiCheckbox(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushinteger(state, 0); return 1; }
            const char* id = luaL_checkstring(state, 1);
            float x = (float)luaL_checknumber(state, 2);
            float y = (float)luaL_checknumber(state, 3);
            const char* text = luaL_checkstring(state, 4);
            bool checked = lua_toboolean(state, 5);
            auto widget = engine->guiSystem.addCheckbox(id, x, y, text, checked);
            Entity entity = engine->guiAddWidget(widget);
            lua_pushinteger(state, entity);
            return 1;
        }

        static int GuiRemove(lua_State* state) {
            const char* id = luaL_checkstring(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->guiSystem.remove(id);
            return 0;
        }

        static int GuiRemoveByEntity(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            auto it = engine->guiWidgetMap.find(entity);
            if (it != engine->guiWidgetMap.end()) {
                engine->guiSystem.remove(it->second->id);
                engine->guiWidgetMap.erase(it);
            }
            return 0;
        }

        static int GuiClear(lua_State* state) {
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->guiSystem.clear();
            engine->guiWidgetMap.clear();
            return 0;
        }

        static int GuiSetPosition(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float x = (float)luaL_checknumber(state, 2);
            float y = (float)luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w) { w->x = x; w->y = y; w->updateModelMatrix(); }
            return 0;
        }

        static int GuiSetSize(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float w = (float)luaL_checknumber(state, 2);
            float h = (float)luaL_checknumber(state, 3);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* wid = engine->guiGetWidget(entity);
            if (wid) { wid->w = w; wid->h = h; wid->updateModelMatrix(); }
            return 0;
        }

        static int GuiSetColor(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float cr = (float)luaL_checknumber(state, 2);
            float cg = (float)luaL_checknumber(state, 3);
            float cb = (float)luaL_checknumber(state, 4);
            float ca = (float)luaL_optnumber(state, 5, 1.0f);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w) { w->r = cr; w->g = cg; w->b = cb; w->a = ca; }
            return 0;
        }

        static int GuiSetText(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            const char* text = luaL_checkstring(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (!w) return 0;
            if (w->getType() == Gui::GuiWidget::BUTTON)
                static_cast<Gui::GuiButton*>(w)->text = text;
            else if (w->getType() == Gui::GuiWidget::LABEL)
                static_cast<Gui::GuiLabel*>(w)->text = text;
            else if (w->getType() == Gui::GuiWidget::CHECKBOX)
                static_cast<Gui::GuiCheckbox*>(w)->text = text;
            return 0;
        }

        static int GuiSetVisible(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool vis = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w) w->visible = vis;
            return 0;
        }

        static int GuiSetEnabled(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            bool en = lua_toboolean(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w) w->enabled = en;
            return 0;
        }

        static int GuiSetValue(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float val = (float)luaL_checknumber(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w && w->getType() == Gui::GuiWidget::SLIDER)
                static_cast<Gui::GuiSlider*>(w)->value = val;
            return 0;
        }

        static int GuiGetValue(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) { lua_pushnumber(state, 0); return 1; }
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w && w->getType() == Gui::GuiWidget::SLIDER) {
                lua_pushnumber(state, static_cast<Gui::GuiSlider*>(w)->value);
                return 1;
            }
            if (w && w->getType() == Gui::GuiWidget::CHECKBOX) {
                lua_pushboolean(state, static_cast<Gui::GuiCheckbox*>(w)->checked);
                return 1;
            }
            lua_pushnumber(state, 0);
            return 1;
        }

        static int GuiSetOnClick(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (!w) return 0;
            if (w->luaOnClickRef >= 0)
                luaL_unref(state, LUA_REGISTRYINDEX, w->luaOnClickRef);
            if (lua_isfunction(state, 2)) {
                lua_pushvalue(state, 2);
                w->luaOnClickRef = luaL_ref(state, LUA_REGISTRYINDEX);
            } else {
                w->luaOnClickRef = -1;
            }
            return 0;
        }

        static int GuiSetOnChange(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (!w) return 0;
            if (w->luaOnChangeRef >= 0)
                luaL_unref(state, LUA_REGISTRYINDEX, w->luaOnChangeRef);
            if (lua_isfunction(state, 2)) {
                lua_pushvalue(state, 2);
                w->luaOnChangeRef = luaL_ref(state, LUA_REGISTRYINDEX);
            } else {
                w->luaOnChangeRef = -1;
            }
            return 0;
        }

        static int GuiSetZOrder(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            int z = luaL_checkinteger(state, 2);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w) w->zOrder = z;
            return 0;
        }

        static int GuiSetFont(lua_State* state) {
            const char* path = luaL_checkstring(state, 1);
            int size = luaL_optinteger(state, 2, 24);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            engine->guiSystem.setFont(path, size);
            return 0;
        }

        static int GuiSetBorder(lua_State* state) {
            Entity entity = luaL_checkinteger(state, 1);
            float br = (float)luaL_checknumber(state, 2);
            float bg = (float)luaL_checknumber(state, 3);
            float bb = (float)luaL_checknumber(state, 4);
            float ba = (float)luaL_optnumber(state, 5, 1.0f);
            float bw = (float)luaL_optnumber(state, 6, 1.0f);
            LuaEngine* engine = getEngine(state);
            if (!engine) return 0;
            Gui::GuiWidget* w = engine->guiGetWidget(entity);
            if (w && w->getType() == Gui::GuiWidget::PANEL) {
                auto* p = static_cast<Gui::GuiPanel*>(w);
                p->br = br; p->bg = bg; p->bb = bb; p->ba = ba; p->bw = bw;
            }
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
        
        // Scene management
        {"LoadScene", _functions::LoadScene},
        {"GetCurrentScene", _functions::GetCurrentScene},
        
        // Screen size functions (global, not in table)
        // These will be registered separately in registerCFunctions()
        
        // Sprite functions
        {"CreateSprite", _functions::CreateSprite},
        {"SpriteSetTexture", _functions::SpriteSetTexture},
        {"SpriteSetPosition", _functions::SpriteSetPosition},
        {"SpriteGetPosition", _functions::SpriteGetPosition},
        {"SpriteSetSize", _functions::SpriteSetSize},
        {"SpriteGetSize", _functions::SpriteGetSize},
        {"SpriteSetOrigin", _functions::SpriteSetOrigin},
        {"SpriteGetOrigin", _functions::SpriteGetOrigin},
        {"SpriteSetRotation", _functions::SpriteSetRotation},
        {"SpriteGetRotation", _functions::SpriteGetRotation},
        {"SpriteSetColor", _functions::SpriteSetColor},
        {"SpriteGetColor", _functions::SpriteGetColor},
        {"SpriteSetVisible", _functions::SpriteSetVisible},
        {"SpriteIsVisible", _functions::SpriteIsVisible},
        
        // Animation functions
        {"CreateAnimation", _functions::CreateAnimation},
        {"CreateAnimationFromSheet", _functions::CreateAnimationFromSheet},
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
        {"CreateText", _functions::CreateText},
        {"TextSetString", _functions::TextSetString},
        {"TextGetString", _functions::TextGetString},
        {"TextSetPosition", _functions::TextSetPosition},
        {"TextGetPosition", _functions::TextGetPosition},
        {"TextSetColor", _functions::TextSetColor},
        {"TextGetColor", _functions::TextGetColor},
        {"TextSetScale", _functions::TextSetScale},
        {"TextGetScale", _functions::TextGetScale},
        {"TextSetOrigin", _functions::TextSetOrigin},
        {"TextGetOrigin", _functions::TextGetOrigin},
        {"TextSetRotation", _functions::TextSetRotation},
        {"TextGetRotation", _functions::TextGetRotation},
        {"TextSetAlignment", _functions::TextSetAlignment},
        {"TextGetAlignment", _functions::TextGetAlignment},
        {"TextSetVisible", _functions::TextSetVisible},
        {"TextIsVisible", _functions::TextIsVisible},
        
        // Mesh functions
        {"CreateMesh", _functions::CreateMesh},
        {"MeshSetData", _functions::MeshSetData},
        {"MeshDraw", _functions::MeshDraw},
        
        // Object deletion
        {"Destroy", _functions::Destroy},
        {"DestroyAll", _functions::DestroyAll},
        
        // Physics functions
        {"PhysicsInit", _functions::PhysicsInit},
        {"PhysicsSetGravity", _functions::PhysicsSetGravity},
        {"PhysicsGetGravity", _functions::PhysicsGetGravity},
        {"PhysicsCreateBody", _functions::PhysicsCreateBody},
        {"PhysicsAddCircle", _functions::PhysicsAddCircle},
        {"PhysicsAddRectangle", _functions::PhysicsAddRectangle},
        {"PhysicsSetLinearVelocity", _functions::PhysicsSetLinearVelocity},
        {"PhysicsGetLinearVelocity", _functions::PhysicsGetLinearVelocity},
        {"PhysicsSetAngularVelocity", _functions::PhysicsSetAngularVelocity},
        {"PhysicsGetAngularVelocity", _functions::PhysicsGetAngularVelocity},
        {"PhysicsApplyForce", _functions::PhysicsApplyForce},
        {"PhysicsApplyForceAtPoint", _functions::PhysicsApplyForceAtPoint},
        {"PhysicsApplyImpulse", _functions::PhysicsApplyImpulse},
        {"PhysicsApplyImpulseAtPoint", _functions::PhysicsApplyImpulseAtPoint},
        {"PhysicsApplyTorque", _functions::PhysicsApplyTorque},
        {"PhysicsSetPosition", _functions::PhysicsSetPosition},
        {"PhysicsGetPosition", _functions::PhysicsGetPosition},
        {"PhysicsSetAngle", _functions::PhysicsSetAngle},
        {"PhysicsGetAngle", _functions::PhysicsGetAngle},
        {"PhysicsSetGravityScale", _functions::PhysicsSetGravityScale},
        {"PhysicsGetGravityScale", _functions::PhysicsGetGravityScale},
        {"PhysicsSetActive", _functions::PhysicsSetActive},
        {"PhysicsIsActive", _functions::PhysicsIsActive},
        {"PhysicsSetFixedRotation", _functions::PhysicsSetFixedRotation},
        {"PhysicsIsFixedRotation", _functions::PhysicsIsFixedRotation},
        {"PhysicsSetBullet", _functions::PhysicsSetBullet},
        {"PhysicsIsBullet", _functions::PhysicsIsBullet},
        {"PhysicsDestroyBody", _functions::PhysicsDestroyBody},
        {"PhysicsGetMass", _functions::PhysicsGetMass},
        {"PhysicsStep", _functions::PhysicsStep},
        {"PhysicsSyncSprite", _functions::PhysicsSyncSprite},
        
        // Audio functions
        {"LoadSound", _functions::LoadSound},
        {"PlaySound", _functions::PlaySound},
        {"PlaySoundById", _functions::PlaySoundById},
        {"StopSound", _functions::StopSound},
        {"StopAllSounds", _functions::StopAllSounds},
        {"SetSoundVolume", _functions::SetSoundVolume},
        {"IsSoundPlaying", _functions::IsSoundPlaying},
        
        // Window functions
        {"CreateWindow", _functions::CreateWindow},
        {"SetWindowTitle", _functions::SetWindowTitle},
        {"SetWindowSize", _functions::SetWindowSize},
        {"SetWindowIcon", _functions::SetWindowIcon},

        // Shader functions
        {"CreateShader", _functions::CreateShader},
        {"DestroyShader", _functions::DestroyShader},
        {"SetEntityShader", _functions::SetEntityShader},
        {"GetEntityShader", _functions::GetEntityShader},
        {"UseShader", _functions::UseShader},
        {"SetShaderFloat", _functions::SetShaderFloat},
        {"SetShaderInt", _functions::SetShaderInt},
        {"SetShaderVec2", _functions::SetShaderVec2},
        {"SetShaderVec3", _functions::SetShaderVec3},
        {"SetShaderVec4", _functions::SetShaderVec4},
        
        // Main window functions
        {"SetMainWindowTitle", _functions::SetMainWindowTitle},
        {"GetMainWindowTitle", _functions::GetMainWindowTitle},
        {"SetMainWindowSize", _functions::SetMainWindowSize},
        {"GetMainWindowWidth", _functions::GetMainWindowWidth},
        {"GetMainWindowHeight", _functions::GetMainWindowHeight},
        {"SetMainWindowPosition", _functions::SetMainWindowPosition},
        {"GetMainWindowPosition", _functions::GetMainWindowPosition},
        {"SetMainWindowIcon", _functions::SetMainWindowIcon},
        {"SetMainWindowShouldClose", _functions::SetMainWindowShouldClose},
        {"IsMainWindowShouldClose", _functions::IsMainWindowShouldClose},
        
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
        {"GetTime", _functions::GetTime},
        {"Random", _functions::Random},
        {"RandomInt", _functions::RandomInt},
        {"SetRandomSeed", _functions::SetRandomSeed},
        {"Quit", _functions::Quit},
        
        // Console
        {"AddConsoleMessage", _functions::AddConsoleMessage},
        
        // GUI functions
        {"GuiPanel", _functions::GuiPanel},
        {"GuiButton", _functions::GuiButton},
        {"GuiLabel", _functions::GuiLabel},
        {"GuiSlider", _functions::GuiSlider},
        {"GuiCheckbox", _functions::GuiCheckbox},
        {"GuiRemove", _functions::GuiRemove},
        {"GuiRemoveByEntity", _functions::GuiRemoveByEntity},
        {"GuiClear", _functions::GuiClear},
        {"GuiSetPosition", _functions::GuiSetPosition},
        {"GuiSetSize", _functions::GuiSetSize},
        {"GuiSetColor", _functions::GuiSetColor},
        {"GuiSetText", _functions::GuiSetText},
        {"GuiSetVisible", _functions::GuiSetVisible},
        {"GuiSetEnabled", _functions::GuiSetEnabled},
        {"GuiSetValue", _functions::GuiSetValue},
        {"GuiGetValue", _functions::GuiGetValue},
        {"GuiSetOnClick", _functions::GuiSetOnClick},
        {"GuiSetOnChange", _functions::GuiSetOnChange},
        {"GuiSetZOrder", _functions::GuiSetZOrder},
        {"GuiSetFont", _functions::GuiSetFont},
        {"GuiSetBorder", _functions::GuiSetBorder},
        
        {nullptr, nullptr}
    };
}

// ==================== IMPLEMENTATION ====================
namespace LuaEngine {
    LuaEngine::LuaEngine(Window &window) :
        state(nullptr),
        spriteWorld(), animationWorld(), textWorld(), meshWorld(),
        audioEngine(), physicsWorld(),
        deltaTime(0.0f),
        initialized(false), audioInitialized(false),
        objectMap(), soundNameToId(), physicsBodyMap(),
        scripts(), scriptsListPath(), projectFileName(),
        sceneManager(),
        additionalWindows(), mainWindow(&window),
        guiSystem(), guiWidgetMap(), guiNextEntity(1),
        projectionViewMatrix2D(), quadVertexBufferObject(),
        spriteShader2D(), fontShader2D(), spriteMesh2D(),
        textureManager(), fontManager(),
        shaders(), entityShaderMap(), nextShaderId(1)
    {
        this->spriteMesh2D.setVertexBuffer(this->quadVertexBufferObject);
        this->sceneManager = std::make_unique<SceneManager>(this);

        this->state = luaL_newstate();
        if (this->state == nullptr) {
            fprintf(stderr, "Failed to create Lua state\n");
            return;
        }
        
        luaL_openlibs(this->state);
        registerCFunctions();
        this->guiSystem.setLuaState(this->state);
        this->guiSystem.setProjectionView(this->projectionViewMatrix2D);
        
        this->audioInitialized = this->audioEngine.init();
        if (!this->audioInitialized) {
            fprintf(stderr, "Warning: Failed to initialize audio\n");
        }
        
        this->initialized = true;
        printf("LuaEngine initialized successfully\n");
    }
    
    LuaEngine::~LuaEngine() {
        if (!this->initialized) {
            return;
        }
        if (this->audioInitialized) {
            this->audioEngine.shutdown();
            this->audioInitialized = false;
        }

        destroyAllEntities();

        if (this->state != nullptr) {
            lua_close(this->state);
            this->state = nullptr;
        }

        this->initialized = false;
        printf("LuaEngine shutdown complete\n");
    }
    
    bool LuaEngine::parseScriptsList(const std::string& listPath) {
        std::string actualPath = listPath;
        if (listPath.find(".BlazeBoltProject") == std::string::npos) {
            std::ifstream test(listPath + ".BlazeBoltProject");
            if (test.is_open()) {
                actualPath = listPath + ".BlazeBoltProject";
            }
        }
        
        std::ifstream file(actualPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open project file: " << actualPath << std::endl;
            return false;
        }
        
        scripts.clear();
        std::string line;
        
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty() || line[0] == '#') continue;
            
            ScriptInfo info;
            info.isScene = false;
            
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string name = line.substr(0, eqPos);
                info.path = line.substr(eqPos + 1);
                if (name[0] == '@') {
                    name = name.substr(1);
                    info.isScene = true;
                }
                info.name = name;
            } else {
                info.name = std::filesystem::path(line).stem().string();
                info.path = line;
            }
            
            scripts.push_back(info);
        }
        
        file.close();
        std::cout << "Loaded " << scripts.size() << " scripts from " << actualPath << std::endl;
        return true;
    }
    
    void LuaEngine::registerCFunctions() {
        lua_newtable(state);
        for (const luaL_Reg* reg = BlazeBolt; reg->name != nullptr; ++reg) {
            lua_pushcfunction(state, reg->func);
            lua_setfield(state, -2, reg->name);
        }
        lua_setglobal(state, "BlazeBolt");
        
        // Register global functions for screen size
        lua_pushcfunction(state, _functions::GetScreenWidth);
        lua_setglobal(state, "GetScreenWidth");
        
        lua_pushcfunction(state, _functions::GetScreenHeight);
        lua_setglobal(state, "GetScreenHeight");
        
        lua_pushlightuserdata(state, this);
        lua_setglobal(state, "__lua_engine");
        
        // Text constants
        lua_newtable(state);
        lua_pushinteger(state, static_cast<int>(BlazeBolt::Text2D::Alignment::Left)); lua_setfield(state, -2, "LEFT");
        lua_pushinteger(state, static_cast<int>(BlazeBolt::Text2D::Alignment::Center)); lua_setfield(state, -2, "CENTER");
        lua_pushinteger(state, static_cast<int>(BlazeBolt::Text2D::Alignment::Right)); lua_setfield(state, -2, "RIGHT");
        lua_setglobal(state, "TextAlignment");
        
        // Keyboard constants
        lua_newtable(state);
        #define ADD_KEY(name) lua_pushinteger(state, GLFW_KEY_##name); lua_setfield(state, -2, #name)
        // Letters
        ADD_KEY(A); ADD_KEY(B); ADD_KEY(C); ADD_KEY(D); ADD_KEY(E); ADD_KEY(F); ADD_KEY(G);
        ADD_KEY(H); ADD_KEY(I); ADD_KEY(J); ADD_KEY(K); ADD_KEY(L); ADD_KEY(M); ADD_KEY(N);
        ADD_KEY(O); ADD_KEY(P); ADD_KEY(Q); ADD_KEY(R); ADD_KEY(S); ADD_KEY(T); ADD_KEY(U);
        ADD_KEY(V); ADD_KEY(W); ADD_KEY(X); ADD_KEY(Y); ADD_KEY(Z);
        // Numbers
        ADD_KEY(0); ADD_KEY(1); ADD_KEY(2); ADD_KEY(3); ADD_KEY(4); ADD_KEY(5); ADD_KEY(6); ADD_KEY(7); ADD_KEY(8); ADD_KEY(9);
        // Function keys
        ADD_KEY(F1); ADD_KEY(F2); ADD_KEY(F3); ADD_KEY(F4); ADD_KEY(F5); ADD_KEY(F6);
        ADD_KEY(F7); ADD_KEY(F8); ADD_KEY(F9); ADD_KEY(F10); ADD_KEY(F11); ADD_KEY(F12);
        ADD_KEY(F13); ADD_KEY(F14); ADD_KEY(F15); ADD_KEY(F16); ADD_KEY(F17); ADD_KEY(F18);
        ADD_KEY(F19); ADD_KEY(F20); ADD_KEY(F21); ADD_KEY(F22); ADD_KEY(F23); ADD_KEY(F24); ADD_KEY(F25);
        // Arrow keys
        ADD_KEY(UP); ADD_KEY(DOWN); ADD_KEY(LEFT); ADD_KEY(RIGHT);
        // Special keys
        ADD_KEY(SPACE); ADD_KEY(ENTER); ADD_KEY(ESCAPE); ADD_KEY(TAB); ADD_KEY(BACKSPACE); ADD_KEY(DELETE);
        ADD_KEY(INSERT); ADD_KEY(HOME); ADD_KEY(END); ADD_KEY(PAGE_UP); ADD_KEY(PAGE_DOWN);
        // Lock keys
        ADD_KEY(CAPS_LOCK); ADD_KEY(SCROLL_LOCK); ADD_KEY(NUM_LOCK); ADD_KEY(PRINT_SCREEN); ADD_KEY(PAUSE);
        // Modifier keys
        ADD_KEY(LEFT_SHIFT); ADD_KEY(RIGHT_SHIFT); ADD_KEY(LEFT_CONTROL); ADD_KEY(RIGHT_CONTROL);
        ADD_KEY(LEFT_ALT); ADD_KEY(RIGHT_ALT); ADD_KEY(LEFT_SUPER); ADD_KEY(RIGHT_SUPER);
        // Keypad
        ADD_KEY(KP_0); ADD_KEY(KP_1); ADD_KEY(KP_2); ADD_KEY(KP_3); ADD_KEY(KP_4); ADD_KEY(KP_5);
        ADD_KEY(KP_6); ADD_KEY(KP_7); ADD_KEY(KP_8); ADD_KEY(KP_9);
        ADD_KEY(KP_DECIMAL); ADD_KEY(KP_DIVIDE); ADD_KEY(KP_MULTIPLY);
        ADD_KEY(KP_SUBTRACT); ADD_KEY(KP_ADD); ADD_KEY(KP_ENTER); ADD_KEY(KP_EQUAL);
        // Menu key
        ADD_KEY(MENU);
        // Last key (for iteration bounds)
        ADD_KEY(LAST);
        #undef ADD_KEY
        lua_setglobal(state, "Keys");
        
        // Mouse constants
        lua_newtable(state);
        lua_pushinteger(state, GLFW_MOUSE_BUTTON_LEFT); lua_setfield(state, -2, "LEFT");
        lua_pushinteger(state, GLFW_MOUSE_BUTTON_RIGHT); lua_setfield(state, -2, "RIGHT");
        lua_pushinteger(state, GLFW_MOUSE_BUTTON_MIDDLE); lua_setfield(state, -2, "MIDDLE");
        lua_setglobal(state, "MouseButtons");
    }

    bool LuaEngine::loadScriptsFromList(const std::string& listPath) {
        if (!initialized) return false;
        
        scriptsListPath = listPath;
        if (!parseScriptsList(listPath)) return false;
        
        bool allSuccess = true;
        for (auto& script : scripts) {
            if (script.enabled) {
                if (loadScript(script.path)) {
                    script.loaded = true;
                    activeScripts.push_back(script.name);
                    std::cout << "Loaded script: " << script.name << std::endl;
                } else {
                    script.loaded = false;
                    allSuccess = false;
                }
            }
        }
        
        return allSuccess;
    }
    
    bool LuaEngine::loadScript(const std::string& scriptPath) {
        if (!initialized) return false;
        
        if (luaL_dofile(state, scriptPath.c_str()) != LUA_OK) {
            std::cerr << "Error loading script: " << lua_tostring(state, -1) << std::endl;
            lua_pop(state, 1);
            return false;
        }
        return true;
    }
    
    bool LuaEngine::reloadScript(const std::string& scriptPath) {
        for (auto& script : scripts) {
            if (script.path == scriptPath || script.name == scriptPath) {
                if (!script.enabled) return false;
                return loadScript(script.path);
            }
        }
        return loadScript(scriptPath);
    }
    
    bool LuaEngine::reloadAllScripts() {
        if (scripts.empty()) return false;
        
        bool allSuccess = true;
        for (auto& script : scripts) {
            if (script.enabled) {
                if (loadScript(script.path)) {
                    script.loaded = true;
                } else {
                    script.loaded = false;
                    allSuccess = false;
                }
            }
        }
        return allSuccess;
    }
    
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
    
    bool LuaEngine::isScriptLoaded(const std::string& scriptName) const {
        for (const auto& script : scripts) {
            if (script.name == scriptName) {
                return script.loaded && script.enabled;
            }
        }
        return false;
    }
    
    std::vector<std::string> LuaEngine::getLoadedScripts() const {
        std::vector<std::string> loaded;
        for (const auto& script : scripts) {
            if (script.loaded && script.enabled) {
                loaded.push_back(script.name);
            }
        }
        return loaded;
    }
    
    bool LuaEngine::loadScene(const std::string& sceneName) {
        if (sceneManager) {
            return sceneManager->loadScene(sceneName);
        }
        return false;
    }
    
    // Sprite implementations
    // FIXME: Maybe, we shouldn't create every entity (e.g. Sprite2D, Text2D, etc.) on heap?
    Entity LuaEngine::createSprite(const std::string &texturePath, const Vector2 &position) {
        BlazeBolt::Sprite2D *sprite = new BlazeBolt::Sprite2D();
        const GL::Texture2D *texture = this->textureManager.loadFromFile2D(texturePath);
        if (texture != nullptr) {
            sprite->setTexture(*texture);
        }
        sprite->setPosition(position);

        Entity entity = spriteWorld.spawn(sprite);
        objectMap[entity] = RegisteredObject(RegisteredObject::SPRITE, sprite, entity);
        return entity;
    }
    void LuaEngine::spriteSetTexture(Entity entity, const std::string &texturePath) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        const GL::Texture2D *texture = this->textureManager.loadFromFile2D(texturePath);
        if (texture != nullptr) {
            sprite->setTexture(*texture);
        }
    }
    void LuaEngine::spriteSetPosition(Entity entity, const Vector2 &position) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setPosition(position);
    }
    Vector2 LuaEngine::spriteGetPosition(Entity entity) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        return sprite == nullptr ? Vector2(0.0f, 0.0f) : sprite->getPosition();
    }
    // ! IMPORTANT: Only if allowed by the host.
    // TODO: Rename to spriteSetScale and do the same in the Lua implementation.
    void LuaEngine::spriteSetSize(Entity entity, const Vector2 &size) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setScale(size);
    }
    Vector2 LuaEngine::spriteGetSize(Entity entity) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        return sprite == nullptr ? Vector2(1.0f, 1.0f) : sprite->getScale();
    }
    void LuaEngine::spriteSetOrigin(Entity entity, const Vector2& origin) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setOrigin(origin);
    }
    Vector2 LuaEngine::spriteGetOrigin(Entity entity) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        return sprite == nullptr ? Vector2(0.0f, 0.0f) : sprite->getOrigin();
    }
    void LuaEngine::spriteSetRotation(Entity entity, float rotation) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setRotation(rotation);
    }
    float LuaEngine::spriteGetRotation(Entity entity) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        return sprite == nullptr ? 0.0f : sprite->getRotation();
    }
    void LuaEngine::spriteSetColor(Entity entity, const Vector4& color) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setColor(color);
    }
    Vector4 LuaEngine::spriteGetColor(Entity entity) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        return sprite == nullptr ? Vector4(0.0f, 0.0f, 0.0f, 0.0f) : sprite->getColor();
    }
    void LuaEngine::spriteSetVisible(Entity entity, bool visible) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setVisible(visible);
    }
    bool LuaEngine::spriteIsVisible(Entity entity) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        return sprite == nullptr ? false : sprite->isVisible();
    }
    void LuaEngine::drawAllSprites() const {
        this->spriteShader2D.bind();
        this->spriteShader2D.setAspectRatio(static_cast<float>(getScreenWidth()) / static_cast<float>(getScreenHeight()));
        for (const auto &pair : spriteWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                // Check if entity has a custom shader
                auto shaderIt = entityShaderMap.find(pair.second);
                if (shaderIt != entityShaderMap.end()) {
                    Shader *customShader = getShader(shaderIt->second);
                    if (customShader) {
                        customShader->use();
                    }
                }
                pair.first->draw(this->textureManager.getDefault2D(), this->spriteShader2D, this->spriteMesh2D, this->projectionViewMatrix2D);
            }
        }
    }
    
    // Animation implementations
    Entity LuaEngine::createAnimation(const std::string& path, bool isGif, const Vector2& position) {
        Animation2D* anim = new Animation2D();
        anim->setScreenSize(getScreenWidth(), getScreenHeight());
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
        anim->setScreenSize(getScreenWidth(), getScreenHeight());
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
        return anim ? anim->getFrameCount() : 0;
    }
    
    bool LuaEngine::animationIsPlaying(Entity entity) {
        Animation2D* anim = animationWorld.getEntity(entity);
        return anim ? anim->isPlaying() : false;
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
            if (pair.first && !pair.second) {
                pair.first->update(dt);
            }
        }
    }
    
    void LuaEngine::drawAllAnimations() {
        for (const auto& pair : animationWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                auto shaderIt = entityShaderMap.find(pair.second);
                if (shaderIt != entityShaderMap.end()) {
                    Shader* customShader = getShader(shaderIt->second);
                    if (customShader) {
                        customShader->use();
                    }
                }
                pair.first->draw();
            }
        }
    }
    
    // Text implementations
    // FIXME: Maybe, we shouldn't create every entity (e.g. Sprite2D, Text2D, etc.) on heap?
    Entity LuaEngine::createText(const std::string &fontPath, const std::string &text, const Vector2 &position) {
        BlazeBolt::Text2D *textObject = new BlazeBolt::Text2D(this->quadVertexBufferObject, this->fontManager.loadFromFile(fontPath));
        textObject->setText(text);
        textObject->setPosition(position);
        Entity entity = this->textWorld.spawn(textObject);
        this->objectMap[entity] = RegisteredObject(RegisteredObject::TEXT, textObject, entity);
        return entity;
    }
    void LuaEngine::textSetString(Entity entity, const std::string &text) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setText(text);
    }
    std::string LuaEngine::textGetString(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getText() : "";
    }
    void LuaEngine::textSetPosition(Entity entity, const Vector2 &position) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setPosition(position);
    }
    Vector2 LuaEngine::textGetPosition(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getPosition() : Vector2(0.0f, 0.0f);
    }
    void LuaEngine::textSetScale(Entity entity, const Vector2 &scale) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setScale(scale);
    }
    Vector2 LuaEngine::textGetScale(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getScale() : Vector2(0.0f, 0.0f);
    }
    void LuaEngine::textSetOrigin(Entity entity, const Vector2 &origin) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setOrigin(origin);
    }
    Vector2 LuaEngine::textGetOrigin(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getOrigin() : Vector2(0.0f, 0.0f);
    }
    void LuaEngine::textSetRotation(Entity entity, float rotation) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setRotation(rotation);
    }
    float LuaEngine::textGetRotation(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getRotation() : 0.0f;
    }
    void LuaEngine::textSetColor(Entity entity, const Vector4 &color) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setColor(color);
    }
    Vector4 LuaEngine::textGetColor(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getColor() : Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    void LuaEngine::textSetAlignment(Entity entity, BlazeBolt::Text2D::Alignment alignment) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setAlignment(alignment);
    }
    BlazeBolt::Text2D::Alignment LuaEngine::textGetAlignment(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->getAlignment() : BlazeBolt::Text2D::Alignment::Left;
    }
    void LuaEngine::textSetVisible(Entity entity, bool visible) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        if (textObject == nullptr) {
            return;
        }
        textObject->setVisible(visible);
    }
    bool LuaEngine::textIsVisible(Entity entity) {
        BlazeBolt::Text2D *textObject = this->textWorld.getEntity(entity);
        return textObject ? textObject->isVisible() : false;
    }
    
    void LuaEngine::drawAllTexts() {
        this->fontShader2D.bind();
        this->fontShader2D.setAspectRatio(static_cast<float>(this->getScreenWidth()) / static_cast<float>(this->getScreenHeight()));
        for (const auto& pair : textWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                auto shaderIt = entityShaderMap.find(pair.second);
                if (shaderIt != entityShaderMap.end()) {
                    Shader *customShader = getShader(shaderIt->second);
                    if (customShader) {
                        customShader->use();
                    }
                }
                pair.first->draw(this->fontShader2D, this->projectionViewMatrix2D);
            }
        }
    }

    // FIXME: Setting screen (window) size for each entity is the worst idea ever, we need to just pass it at the render stage and use it anywhere we want
    void LuaEngine::setSpriteScreenSize(int width, int height) {
        for (const auto& pair : animationWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
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
            if (pair.first && !pair.second) {
                auto shaderIt = entityShaderMap.find(pair.second);
                if (shaderIt != entityShaderMap.end()) {
                    Shader* customShader = getShader(shaderIt->second);
                    if (customShader) {
                        customShader->use();
                    }
                }
                pair.first->draw();
            }
        }
    }
    
    // Shader implementations
    unsigned int LuaEngine::createShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
        unsigned int id = nextShaderId++;
        Shader* shader = new Shader(vertexPath, fragmentPath);
        shaders[id] = ShaderInfo(name, vertexPath, fragmentPath, id);
        shaders[id].shader = shader;
        return id;
    }

    void LuaEngine::destroyShader(unsigned int shaderId) {
        auto it = shaders.find(shaderId);
        if (it != shaders.end()) {
            delete it->second.shader;
            shaders.erase(it);
        }
        for (auto it2 = entityShaderMap.begin(); it2 != entityShaderMap.end(); ) {
            if (it2->second == shaderId) {
                it2 = entityShaderMap.erase(it2);
            } else {
                ++it2;
            }
        }
    }

    Shader* LuaEngine::getShader(unsigned int shaderId) const {
        auto it = shaders.find(shaderId);
        if (it != shaders.end()) {
            return it->second.shader;
        }
        return nullptr;
    }

    void LuaEngine::setEntityShader(Entity entity, unsigned int shaderId) {
        if (shaderId == 0) {
            entityShaderMap.erase(entity);
        } else {
            entityShaderMap[entity] = shaderId;
        }
    }

    unsigned int LuaEngine::getEntityShader(Entity entity) {
        auto it = entityShaderMap.find(entity);
        if (it != entityShaderMap.end()) {
            return it->second;
        }
        return 0;
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
        if (id >= 0) return audioEngine.isPlaying(id);
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
    
    // Window management
    std::shared_ptr<Window> LuaEngine::createWindow(int width, int height, const std::string& title) {
        auto window = std::make_shared<Window>(width, height, title.c_str());
        additionalWindows.push_back(window);
        return window;
    }
    
    void LuaEngine::destroyWindow(std::shared_ptr<Window> window) {
        auto it = std::find(additionalWindows.begin(), additionalWindows.end(), window);
        if (it != additionalWindows.end()) {
            additionalWindows.erase(it);
        }
    }
    
    void LuaEngine::setWindowTitle(std::shared_ptr<Window> window, const std::string& title) {
        if (window) window->setTitle(title.c_str());
    }
    
    void LuaEngine::setWindowSize(std::shared_ptr<Window> window, int width, int height) {
        if (window) window->setSize(width, height);
    }
    
    void LuaEngine::setWindowPosition(std::shared_ptr<Window> window, int x, int y) {
        if (window) {
            glfwSetWindowPos(window->getGLFWwindow(), x, y);
        }
    }
    
    void LuaEngine::setWindowIcon(std::shared_ptr<Window> window, const std::string& iconPath) {
        if (window) window->setIcon(iconPath.c_str());
    }
    
    // Object deletion
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
                default: break;
            }
            objectMap.erase(it);
        }
        // Clean up shader mapping
        entityShaderMap.erase(entity);
    }
    
    void LuaEngine::destroyAllEntities() {
        spriteWorld.clear();
        animationWorld.clear();
        textWorld.clear();
        meshWorld.clear();
        physicsWorld.clear();
        physicsBodyMap.clear();
        objectMap.clear();
        entityShaderMap.clear();
        for (auto& pair : shaders) {
            delete pair.second.shader;
        }
        shaders.clear();
        guiSystem.clear();
        guiWidgetMap.clear();
    }
    
    // Physics implementations
    void LuaEngine::physicsInit(float gravityX, float gravityY) {
        physicsWorld.setGravity(gravityX, gravityY);
    }
    
    void LuaEngine::physicsSetGravity(float x, float y) {
        physicsWorld.setGravity(x, y);
    }
    
    void LuaEngine::physicsGetGravity(float* x, float* y) const {
        physicsWorld.getGravity(x, y);
    }
    
    Entity LuaEngine::physicsCreateBody(int bodyType, float x, float y, float mass, float friction, float restitution) {
        PhysicsBodyType type;
        switch (bodyType) {
            case 0: type = PhysicsBodyType::Static; break;
            case 1: type = PhysicsBodyType::Dynamic; break;
            case 2: type = PhysicsBodyType::Kinematic; break;
            default: type = PhysicsBodyType::Dynamic; break;
        }
        
        PhysicsBody* body = physicsWorld.createBody(type, x, y, mass, friction, restitution);
        if (!body) return 0;
        
        Entity entity = meshWorld.spawn(new Mesh2D());
        physicsBodyMap[entity] = body;
        return entity;
    }
    
    void LuaEngine::physicsAddCircle(Entity bodyEntity, float radius, float offsetX, float offsetY) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->addCircle(radius, offsetX, offsetY);
        }
    }
    
    void LuaEngine::physicsAddRectangle(Entity bodyEntity, float halfWidth, float halfHeight) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->addRectangle(halfWidth, halfHeight);
        }
    }
    
    void LuaEngine::physicsSetLinearVelocity(Entity bodyEntity, float vx, float vy) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setLinearVelocity(vx, vy);
        }
    }
    
    void LuaEngine::physicsGetLinearVelocity(Entity bodyEntity, float* vx, float* vy) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            Vector2 v = it->second->getLinearVelocity();
            *vx = v.x;
            *vy = v.y;
        } else {
            *vx = 0;
            *vy = 0;
        }
    }
    
    void LuaEngine::physicsSetAngularVelocity(Entity bodyEntity, float av) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setAngularVelocity(av);
        }
    }
    
    float LuaEngine::physicsGetAngularVelocity(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->getAngularVelocity() : 0;
    }
    
    void LuaEngine::physicsApplyForce(Entity bodyEntity, float fx, float fy) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->applyForce(fx, fy);
        }
    }
    
    void LuaEngine::physicsApplyForceAtPoint(Entity bodyEntity, float fx, float fy, float px, float py) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->applyForceAtPoint(fx, fy, px, py);
        }
    }
    
    void LuaEngine::physicsApplyImpulse(Entity bodyEntity, float ix, float iy) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->applyImpulse(ix, iy);
        }
    }
    
    void LuaEngine::physicsApplyImpulseAtPoint(Entity bodyEntity, float ix, float iy, float px, float py) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->applyImpulseAtPoint(ix, iy, px, py);
        }
    }
    
    void LuaEngine::physicsApplyTorque(Entity bodyEntity, float torque) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->applyTorque(torque);
        }
    }
    
    void LuaEngine::physicsSetPosition(Entity bodyEntity, float x, float y) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setPosition(x, y);
        }
    }
    
    void LuaEngine::physicsGetPosition(Entity bodyEntity, float* x, float* y) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            Vector2 p = it->second->getPosition();
            *x = p.x;
            *y = p.y;
        } else {
            *x = 0;
            *y = 0;
        }
    }
    
    void LuaEngine::physicsSetAngle(Entity bodyEntity, float angle) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setAngle(angle);
        }
    }
    
    float LuaEngine::physicsGetAngle(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->getAngle() : 0;
    }
    
    void LuaEngine::physicsSetGravityScale(Entity bodyEntity, float scale) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setGravityScale(scale);
        }
    }
    
    float LuaEngine::physicsGetGravityScale(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->getGravityScale() : 1;
    }
    
    void LuaEngine::physicsSetActive(Entity bodyEntity, bool active) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setActive(active);
        }
    }
    
    bool LuaEngine::physicsIsActive(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->isActive() : false;
    }
    
    void LuaEngine::physicsSetFixedRotation(Entity bodyEntity, bool fixed) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setFixedRotation(fixed);
        }
    }
    
    bool LuaEngine::physicsIsFixedRotation(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->isFixedRotation() : false;
    }
    
    void LuaEngine::physicsSetBullet(Entity bodyEntity, bool bullet) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setBullet(bullet);
        }
    }
    
    bool LuaEngine::physicsIsBullet(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->isBullet() : false;
    }
    
    void LuaEngine::physicsDestroyBody(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            physicsWorld.destroyBody(it->second);
            physicsBodyMap.erase(it);
        }
    }
    
    float LuaEngine::physicsGetMass(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->getMass() : 0;
    }
    
    void LuaEngine::physicsStep() {
        physicsWorld.step(deltaTime);
    }
    
    void LuaEngine::physicsSyncSprite(Entity bodyEntity, Entity spriteEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it == physicsBodyMap.end()) { return; }
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(spriteEntity);
        if (sprite == nullptr) { return; }
        
        PhysicsBody *body = it->second;
        Vector2 position = body->getPosition();
        float angle = body->getAngle();
        
        sprite->setPosition(position);
        sprite->setRotation(angle * (180.0f / 3.14159265358979323846f));
    }
    
    // General
    float LuaEngine::getDeltaTime() const { return deltaTime; }
    void LuaEngine::setDeltaTime(float dt) { deltaTime = dt; }
    lua_State* LuaEngine::getState() { return state; }
    bool LuaEngine::isInitialized() const { return initialized; }
    Audio& LuaEngine::getAudio() { return audioEngine; }
    
    void LuaEngine::addConsoleMessage(const std::string& msg, int type) {
        std::cout << "[Lua] " << msg << std::endl;
    }

    Entity LuaEngine::guiAddWidget(Gui::GuiWidget* widget) {
        Entity e = guiNextEntity++;
        guiWidgetMap[e] = widget;
        return e;
    }

    Gui::GuiWidget* LuaEngine::guiGetWidget(Entity entity) {
        auto it = guiWidgetMap.find(entity);
        return it != guiWidgetMap.end() ? it->second : nullptr;
    }
    
    bool LuaEngine::callFunction(const std::string& funcName) {
        if (!initialized) return false;
        lua_getglobal(state, funcName.c_str());
        if (lua_isfunction(state, -1)) {
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                std::cerr << "Error in " << funcName << ": " << lua_tostring(state, -1) << std::endl;
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
        bool anyCalled = false;
        for (const auto& scriptName : activeScripts) {
            lua_getglobal(state, scriptName.c_str());
            if (lua_istable(state, -1)) {
                lua_getfield(state, -1, "Update");
                if (lua_isfunction(state, -1)) {
                    lua_pushnumber(state, dt);
                    if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
                        std::cerr << "Error in " << scriptName << ".Update: " << lua_tostring(state, -1) << std::endl;
                        lua_pop(state, 1);
                    } else {
                        anyCalled = true;
                    }
                }
                lua_pop(state, 1);
            }
            lua_pop(state, 1);
        }
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
        return anyCalled;
    }
    
    bool LuaEngine::callDraw() {
        if (!initialized) return false;
        bool anyCalled = false;
        for (const auto& scriptName : activeScripts) {
            lua_getglobal(state, scriptName.c_str());
            if (lua_istable(state, -1)) {
                lua_getfield(state, -1, "Draw");
                if (lua_isfunction(state, -1)) {
                    if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                        std::cerr << "Error in " << scriptName << ".Draw: " << lua_tostring(state, -1) << std::endl;
                        lua_pop(state, 1);
                    } else {
                        anyCalled = true;
                    }
                }
                lua_pop(state, 1);
            }
            lua_pop(state, 1);
        }
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
        return anyCalled;
    }
    
    bool LuaEngine::callEnd() {
        if (!initialized) return false;
        bool anyCalled = false;
        for (const auto& scriptName : activeScripts) {
            lua_getglobal(state, scriptName.c_str());
            if (lua_istable(state, -1)) {
                lua_getfield(state, -1, "End");
                if (lua_isfunction(state, -1)) {
                    if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                        std::cerr << "Error in " << scriptName << ".End: " << lua_tostring(state, -1) << std::endl;
                        lua_pop(state, 1);
                    } else {
                        anyCalled = true;
                    }
                }
                lua_pop(state, 1);
            }
            lua_pop(state, 1);
        }
        lua_getglobal(state, "End");
        if (lua_isfunction(state, -1)) {
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                std::cerr << "Error in End: " << lua_tostring(state, -1) << std::endl;
                lua_pop(state, 1);
                return false;
            }
            return true;
        }
        lua_pop(state, 1);
        return anyCalled;
    }
    
    void LuaEngine::drawAll() {
        this->projectionViewMatrix2D = Matrix3x3::projection(this->getScreenWidth(), this->getScreenHeight());
        drawAllSprites();
        drawAllAnimations();
        drawAllTexts();
        drawAllMeshes();
        this->guiSystem.setProjectionView(this->projectionViewMatrix2D);
        this->guiSystem.draw();
    }
    
    void LuaEngine::updateAll(float dt) {
        deltaTime = dt;
        this->guiSystem.setProjectionView(this->projectionViewMatrix2D);
        this->guiSystem.update();
        updateAllAnimations(dt);
        updateAudio();
        if (sceneManager) {
            sceneManager->update(dt);
        }
    }
}