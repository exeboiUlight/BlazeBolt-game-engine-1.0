#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>
#include <world.h>
#include <physics/physics.h>
#include <graphics/window.h>
#include <subject/sprite/staticSprite2D.hpp>
#include <subject/sprite/animatedSprite2D.hpp>
#include <graphics/spriteBatch2D.hpp>
#include <subject/text2D.hpp>
#include <subject/audio.h>
#include <subject/camera2D.hpp>
#include <subject/particle2D.hpp>
#include <subject/tileset2D.hpp>
#include <subject/light2D.hpp>
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

#include <engine/luaTypes.hpp>
#include <engine/luaSceneManager.hpp>

namespace LuaEngine {

    class LuaEngine {
    private:
        lua_State* state;
        World<BlazeBolt::Sprite2D> spriteWorld;
        World<BlazeBolt::AnimatedSprite2D> animatedSpriteWorld;
        World<BlazeBolt::Text2D> textWorld;
        World<Mesh2D> meshWorld;
        World<Camera2D> cameraWorld;
        World<ParticleSystem2D> particleWorld;
        World<BlazeBolt::Tileset2D> tilesetWorld;
        World<BlazeBolt::Light2D> lightWorld;

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
        BlazeBolt::SpriteBatchShader2D spriteBatchShader2D;
        BlazeBolt::FontShader2D fontShader2D;
        BlazeBolt::SpriteMesh spriteMesh;
        std::vector<BlazeBolt::SpriteBatch2D> spriteBatches;
        BlazeBolt::TextureManager textureManager;
        BlazeBolt::FontManager fontManager;

        // Shader management
        std::unordered_map<unsigned int, ShaderInfo> shaders;
        std::unordered_map<Entity, unsigned int> entityShaderMap;
        unsigned int nextShaderId;

        // Render order
        std::vector<std::string> renderOrder;

        void registerCFunctions();
        bool parseScriptsList(const std::string& listPath);
    public:
        LuaEngine(Window &window);
        ~LuaEngine();

        // Main window management
        Window* getMainWindow() const { return mainWindow; }
        int getScreenWidth() const { return mainWindow ? mainWindow->getWidth() : 0; }
        int getScreenHeight() const { return mainWindow ? mainWindow->getHeight() : 0; }

        // Accessors for internal systems
        World<BlazeBolt::Sprite2D>& getSpriteWorld() { return spriteWorld; }
        World<BlazeBolt::Tileset2D>& getTilesetWorld() { return tilesetWorld; }
        BlazeBolt::TextureManager& getTextureManager() { return textureManager; }

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
        void spriteSetTextureRect(Entity entity, const Vector4 &rect);
        void spriteSetVisible(Entity entity, bool visible);
        bool spriteIsVisible(Entity entity);
        void drawAllSprites() const;

        // Sprite batch management
        Entity createSpriteBatch(uint32_t maxSize = 25);
        void spriteBatchSetTexture(Entity batchEntity, const std::string &texturePath);
        bool spriteBatchAdd(Entity batchEntity, Entity spriteEntity);
        bool spriteBatchRemove(Entity batchEntity, Entity spriteEntity);
        void spriteBatchClear(Entity batchEntity);
        void spriteBatchSetMaxSize(Entity batchEntity, uint32_t maxSize);
        uint32_t spriteBatchGetMaxSize(Entity batchEntity);
        uint32_t spriteBatchGetCount(Entity batchEntity);
        void drawAllSpriteBatches();
        void drawSpriteBatch(Entity batchEntity);
        void destroySpriteBatch(Entity batchEntity);

        // Animation management
        Entity createAnimatedSprite(const std::string &texturePath, const Vector2 &position);
        void animatedSpritePlay(Entity entity);
        bool animatedSpriteIsPlaying(Entity entity);
        void animatedSpritePause(Entity entity);
        void animatedSpriteStop(Entity entity);
        void animatedSpriteRestart(Entity entity);
        void animatedSpriteSetLooping(Entity entity, bool looping);
        bool animatedSpriteIsLooping(Entity entity);
        void animatedSpriteSetPlaybackSpeed(Entity entity, float playbackSpeed);
        float animatedSpriteGetPlaybackSpeed(Entity entity);
        void animatedSpriteSetFrame(Entity entity, int frame);
        uint32_t animatedSpriteGetCurrentFrame(Entity entity);
        uint32_t animatedSpriteGetNumFrames(Entity entity);
        void animatedSpriteSetPosition(Entity entity, const Vector2 &position);
        Vector2 animatedSpriteGetPosition(Entity entity);
        void animatedSpriteSetSize(Entity entity, const Vector2 &size);
        Vector2 animatedSpriteGetSize(Entity entity);
        void animatedSpriteSetOrigin(Entity entity, const Vector2 &origin);
        Vector2 animatedSpriteGetOrigin(Entity entity);
        void animatedSpriteSetRotation(Entity entity, float rotation);
        float animatedSpriteGetRotation(Entity entity);
        void animatedSpriteSetColor(Entity entity, const Vector4 &color);
        Vector4 animatedSpriteGetColor(Entity entity);
        void updateAllAnimatedSprites(float dt);
        void drawAllAnimatedSprites();

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
        void physicsSetFriction(Entity bodyEntity, float friction);
        float physicsGetFriction(Entity bodyEntity);
        void physicsSetRestitution(Entity bodyEntity, float restitution);
        float physicsGetRestitution(Entity bodyEntity);
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
        void physicsSyncAnimatedSprite(Entity bodyEntity, Entity animEntity);

        // Camera management
        Entity createCamera();
        void cameraSetPosition(Entity entity, const Vector2 &position);
        Vector2 cameraGetPosition(Entity entity);
        void cameraSetZoom(Entity entity, float zoom);
        float cameraGetZoom(Entity entity);
        void cameraSetRotation(Entity entity, float rotation);
        float cameraGetRotation(Entity entity);
        Camera2D* getActiveCamera();

        // Particle system management
        Entity createParticleSystem();
        void particleSystemSetPosition(Entity entity, const Vector2 &position);
        void particleSystemSetTexture(Entity entity, const std::string &texturePath);
        void particleSystemSetEmissionRate(Entity entity, float rate);
        float particleSystemGetEmissionRate(Entity entity);
        void particleSystemSetLifetime(Entity entity, float min, float max);
        void particleSystemSetSpeed(Entity entity, float min, float max);
        void particleSystemSetSize(Entity entity, float min, float max);
        void particleSystemSetEndSize(Entity entity, float min, float max);
        void particleSystemSetColor(Entity entity, const Vector4 &start, const Vector4 &end);
        void particleSystemSetDirection(Entity entity, float minAngle, float maxAngle);
        void particleSystemSetRotationSpeed(Entity entity, float speed);
        void particleSystemSetActive(Entity entity, bool active);
        bool particleSystemIsActive(Entity entity);
        void particleSystemSetVisible(Entity entity, bool visible);
        bool particleSystemIsVisible(Entity entity);
        void particleSystemEmit(Entity entity, int count);
        void particleSystemClear(Entity entity);
        int particleSystemGetCount(Entity entity);
        void updateAllParticleSystems(float dt);
        void drawAllParticleSystems();

        // Tileset management
        Entity createTileset(const std::string& texturePath, uint32_t tileW, uint32_t tileH, uint32_t atlasCols, uint32_t atlasRows);
        void tilesetSetMap(Entity entity, const std::vector<std::vector<int>>& map);
        int tilesetGetTile(Entity entity, uint32_t col, uint32_t row);
        void tilesetSetTile(Entity entity, uint32_t col, uint32_t row, int tileIndex);
        void tilesetSetTileSize(Entity entity, uint32_t w, uint32_t h);
        void tilesetGetTileSize(Entity entity, uint32_t* w, uint32_t* h);
        void tilesetSetPosition(Entity entity, const Vector2& pos);
        Vector2 tilesetGetPosition(Entity entity);
        uint32_t tilesetGetMapWidth(Entity entity);
        uint32_t tilesetGetMapHeight(Entity entity);
        uint32_t tilesetGetTileCount(Entity entity);
        void drawAllTilesets();
        void destroyTileset(Entity entity);

        // Light management
        Entity createPointLight(float x, float y, float r, float g, float b, float intensity, float radius);
        Entity createAmbientLight(float r, float g, float b, float intensity);
        void lightSetPosition(Entity entity, const Vector2& pos);
        Vector2 lightGetPosition(Entity entity);
        void lightSetColor(Entity entity, const Vector3& color);
        Vector3 lightGetColor(Entity entity);
        void lightSetIntensity(Entity entity, float intensity);
        float lightGetIntensity(Entity entity);
        void lightSetRadius(Entity entity, float radius);
        float lightGetRadius(Entity entity);
        void lightSetEnabled(Entity entity, bool enabled);
        bool lightGetEnabled(Entity entity);
        void destroyLight(Entity entity);
        void uploadLightDataToShader(const BlazeBolt::SpriteShader2D& shader) const;
        void uploadLightDataToShader(const BlazeBolt::SpriteBatchShader2D& shader) const;

        // General
        float getDeltaTime() const;
        void setDeltaTime(float dt);
        lua_State* getState();
        bool isInitialized() const;
        Audio& getAudio();

        void drawAll();
        void updateAll(float deltaTime);

        void addConsoleMessage(const std::string& msg, int type = 0);

        // Render order
        void setRenderOrder(const std::vector<std::string>& order);
        const std::vector<std::string>& getRenderOrder() const;
    };
}
