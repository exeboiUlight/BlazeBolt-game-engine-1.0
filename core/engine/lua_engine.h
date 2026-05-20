#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>
#include <world.h>
#include <physics/physics.h>
#include <graphics/window.h>
#include <subject/sprite2D.hpp>
#include <subject/animatad2D.h>
#include <subject/text2D.hpp>
#include <subject/audio.h>
#include <engine/managers.hpp>
#include <graphics/mesh.h>
#include <utils/input.hpp>
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

#include <engine/lua_types.h>
#include <engine/lua_scene_manager.h>

namespace LuaEngine {

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
        void physicsSyncText(Entity bodyEntity, Entity textEntity);
        void physicsSyncAnimation(Entity bodyEntity, Entity animEntity);

        // General
        float getDeltaTime() const;
        void setDeltaTime(float dt);
        lua_State* getState();
        bool isInitialized() const;
        Audio& getAudio();

        void drawAll();
        void updateAll(float deltaTime);

        void addConsoleMessage(const std::string& msg, int type = 0);
    };
}
