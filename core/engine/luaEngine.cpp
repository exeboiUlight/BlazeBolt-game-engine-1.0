#include <engine/luaFunctions.hpp>
#include <engine/luaMathBindings.hpp>
#include <engine/luaNetBindings.hpp>
#include <sceneFormat.hpp>
#include <engine/lua_api_init.hpp>
#include <cmath>

#ifndef M_PIf
    #define M_PIf 3.14159265358979323846f
#endif

namespace LuaEngine {

    // ==================== SCENE MANAGER IMPLEMENTATION ====================
    SceneManager::SceneManager(LuaEngine* eng) : engine(eng), transitioning(false), transitionTimer(0) {}

    void SceneManager::registerScene(const std::string& name, const std::string& path) {
        ScriptInfo info(name, path);
        info.isScene = true;
        scenes[name] = info;
    }

    bool SceneManager::loadScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it == scenes.end()) return false;
        nextScene = name;
        transitioning = true;
        transitionTimer = 0;
        return true;
    }

    void SceneManager::update(float dt) {
        if (transitioning) {
            transitionTimer += dt;
            if (transitionTimer >= 0.5f) {
                performSceneSwitch();
                transitioning = false;
            }
        }
    }

    void SceneManager::performSceneSwitch() {
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

    void SceneManager::callSceneLoad(const std::string& scene) {
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

    void SceneManager::callSceneUnload(const std::string& scene) {
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

        // Scene file I/O
        {"LoadSceneFile", _functions::LoadSceneFile},
        {"SaveSceneFile", _functions::SaveSceneFile},

        // Screen size functions (global, not in table)
        // These will be registered separately in registerCFunctions()

        // Sprite functions
        {"CreateSprite", _functions::CreateSprite},
        {"SpriteSetTexture", _functions::SpriteSetTexture},
        {"SpriteSetPosition", _functions::SpriteSetPosition},
        {"SpriteGetPosition", _functions::SpriteGetPosition},
        {"SpriteSetTextureRect", _functions::SpriteSetTextureRect},
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

        // Sprite batch functions
        {"CreateSpriteBatch", _functions::CreateSpriteBatch},
        {"SpriteBatchSetTexture", _functions::SpriteBatchSetTexture},
        {"SpriteBatchAdd", _functions::SpriteBatchAdd},
        {"SpriteBatchRemove", _functions::SpriteBatchRemove},
        {"SpriteBatchClear", _functions::SpriteBatchClear},
        {"SpriteBatchSetMaxSize", _functions::SpriteBatchSetMaxSize},
        {"SpriteBatchGetMaxSize", _functions::SpriteBatchGetMaxSize},
        {"SpriteBatchGetCount", _functions::SpriteBatchGetCount},
        {"SpriteBatchDraw", _functions::SpriteBatchDraw},
        {"DestroySpriteBatch", _functions::DestroySpriteBatch},

        // Animation functions
        {"CreateAnimatedSprite", _functions::CreateAnimatedSprite},
        {"AnimatedSpritePlay", _functions::AnimatedSpritePlay},
        {"AnimatedSpriteIsPlaying", _functions::AnimatedSpriteIsPlaying},
        {"AnimatedSpritePause", _functions::AnimatedSpritePause},
        {"AnimatedSpriteStop", _functions::AnimatedSpriteStop},
        {"AnimatedSpriteRestart", _functions::AnimatedSpriteRestart},
        {"AnimatedSpriteSetTexture", _functions::AnimatedSpriteSetTexture},
        {"AnimatedSpriteSetLooping", _functions::AnimatedSpriteSetLooping},
        {"AnimatedSpriteIsLooping", _functions::AnimatedSpriteIsLooping},
        {"AnimatedSpriteSetPlaybackSpeed", _functions::AnimatedSpriteSetPlaybackSpeed},
        {"AnimatedSpriteGetPlaybackSpeed", _functions::AnimatedSpriteGetPlaybackSpeed},
        {"AnimatedSpriteSetFrame", _functions::AnimatedSpriteSetFrame},
        {"AnimatedSpriteGetCurrentFrame", _functions::AnimatedSpriteGetCurrentFrame},
        {"AnimatedSpriteGetNumFrames", _functions::AnimatedSpriteGetNumFrames},
        {"AnimatedSpriteSetPosition", _functions::AnimatedSpriteSetPosition},
        {"AnimatedSpriteGetPosition", _functions::AnimatedSpriteGetPosition},
        {"AnimatedSpriteSetSize", _functions::AnimatedSpriteSetSize},
        {"AnimatedSpriteGetSize", _functions::AnimatedSpriteGetSize},
        {"AnimatedSpriteSetOrigin", _functions::AnimatedSpriteSetOrigin},
        {"AnimatedSpriteGetOrigin", _functions::AnimatedSpriteGetOrigin},
        {"AnimatedSpriteSetRotation", _functions::AnimatedSpriteSetRotation},
        {"AnimatedSpriteGetRotation", _functions::AnimatedSpriteGetRotation},
        {"AnimatedSpriteSetColor", _functions::AnimatedSpriteSetColor},
        {"AnimatedSpriteGetColor", _functions::AnimatedSpriteGetColor},

        // Animation wheel functions
        {"CreateAnimationWheel", _functions::CreateAnimationWheel},
        {"AnimationWheelAddState", _functions::AnimationWheelAddState},
        {"AnimationWheelRemoveState", _functions::AnimationWheelRemoveState},
        {"AnimationWheelHasState", _functions::AnimationWheelHasState},
        {"AnimationWheelSetInitialState", _functions::AnimationWheelSetInitialState},
        {"AnimationWheelGetInitialState", _functions::AnimationWheelGetInitialState},
        {"AnimationWheelSetState", _functions::AnimationWheelSetState},
        {"AnimationWheelGetState", _functions::AnimationWheelGetState},
        {"AnimationWheelSetPlaybackSpeed", _functions::AnimationWheelSetPlaybackSpeed},
        {"AnimationWheelGetPlaybackSpeed", _functions::AnimationWheelGetPlaybackSpeed},
        {"AnimationWheelSetLooping", _functions::AnimationWheelSetLooping},
        {"AnimationWheelIsLooping", _functions::AnimationWheelIsLooping},
        {"AnimationWheelSetGifPath", _functions::AnimationWheelSetGifPath},
        {"AnimationWheelGetGifPath", _functions::AnimationWheelGetGifPath},
        {"AnimationWheelSetAutoAdvance", _functions::AnimationWheelSetAutoAdvance},
        {"AnimationWheelGetAutoAdvance", _functions::AnimationWheelGetAutoAdvance},
        {"AnimationWheelGetStateNames", _functions::AnimationWheelGetStateNames},

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
        {"MeshSetShader", _functions::MeshSetShader},
        {"MeshSetUniformFloat", _functions::MeshSetUniformFloat},
        {"MeshSetUniformInt", _functions::MeshSetUniformInt},
        {"MeshSetUniformVec2", _functions::MeshSetUniformVec2},
        {"MeshSetUniformVec3", _functions::MeshSetUniformVec3},
        {"MeshSetUniformVec4", _functions::MeshSetUniformVec4},
        {"MeshDraw", _functions::MeshDraw},

        // Camera functions
        {"CreateCamera", _functions::CreateCamera},
        {"CameraSetPosition", _functions::CameraSetPosition},
        {"CameraGetPosition", _functions::CameraGetPosition},
        {"CameraSetZoom", _functions::CameraSetZoom},
        {"CameraGetZoom", _functions::CameraGetZoom},
        {"CameraSetRotation", _functions::CameraSetRotation},
        {"CameraGetRotation", _functions::CameraGetRotation},

        // Particle system functions
        {"CreateParticleSystem", _functions::CreateParticleSystem},
        {"ParticleSystemSetPosition", _functions::ParticleSystemSetPosition},
        {"ParticleSystemSetTexture", _functions::ParticleSystemSetTexture},
        {"ParticleSystemSetEmissionRate", _functions::ParticleSystemSetEmissionRate},
        {"ParticleSystemGetEmissionRate", _functions::ParticleSystemGetEmissionRate},
        {"ParticleSystemSetLifetime", _functions::ParticleSystemSetLifetime},
        {"ParticleSystemSetSpeed", _functions::ParticleSystemSetSpeed},
        {"ParticleSystemSetSize", _functions::ParticleSystemSetSize},
        {"ParticleSystemSetEndSize", _functions::ParticleSystemSetEndSize},
        {"ParticleSystemSetColor", _functions::ParticleSystemSetColor},
        {"ParticleSystemSetDirection", _functions::ParticleSystemSetDirection},
        {"ParticleSystemSetRotationSpeed", _functions::ParticleSystemSetRotationSpeed},
        {"ParticleSystemSetActive", _functions::ParticleSystemSetActive},
        {"ParticleSystemIsActive", _functions::ParticleSystemIsActive},
        {"ParticleSystemSetVisible", _functions::ParticleSystemSetVisible},
        {"ParticleSystemIsVisible", _functions::ParticleSystemIsVisible},
        {"ParticleSystemEmit", _functions::ParticleSystemEmit},
        {"ParticleSystemClear", _functions::ParticleSystemClear},
        {"ParticleSystemGetCount", _functions::ParticleSystemGetCount},

        // Tileset functions
        {"CreateTileset", _functions::CreateTileset},
        {"TilesetSetMap", _functions::TilesetSetMap},
        {"TilesetGetTile", _functions::TilesetGetTile},
        {"TilesetSetTile", _functions::TilesetSetTile},
        {"TilesetSetTileSize", _functions::TilesetSetTileSize},
        {"TilesetGetTileSize", _functions::TilesetGetTileSize},
        {"TilesetSetPosition", _functions::TilesetSetPosition},
        {"TilesetGetPosition", _functions::TilesetGetPosition},
        {"TilesetGetMapWidth", _functions::TilesetGetMapWidth},
        {"TilesetGetMapHeight", _functions::TilesetGetMapHeight},
        {"TilesetGetTileCount", _functions::TilesetGetTileCount},
        {"TilesetDraw", _functions::TilesetDraw},
        {"DestroyTileset", _functions::DestroyTileset},

        // Light functions
        {"CreatePointLight", _functions::CreatePointLight},
        {"CreateAmbientLight", _functions::CreateAmbientLight},
        {"LightSetPosition", _functions::LightSetPosition},
        {"LightGetPosition", _functions::LightGetPosition},
        {"LightSetColor", _functions::LightSetColor},
        {"LightGetColor", _functions::LightGetColor},
        {"LightSetIntensity", _functions::LightSetIntensity},
        {"LightGetIntensity", _functions::LightGetIntensity},
        {"LightSetRadius", _functions::LightSetRadius},
        {"LightGetRadius", _functions::LightGetRadius},
        {"LightSetEnabled", _functions::LightSetEnabled},
        {"LightGetEnabled", _functions::LightGetEnabled},
        {"DestroyLight", _functions::DestroyLight},

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
        {"PhysicsSetFriction", _functions::PhysicsSetFriction},
        {"PhysicsGetFriction", _functions::PhysicsGetFriction},
        {"PhysicsSetRestitution", _functions::PhysicsSetRestitution},
        {"PhysicsGetRestitution", _functions::PhysicsGetRestitution},
        {"PhysicsStep", _functions::PhysicsStep},
        {"PhysicsSyncSprite", _functions::PhysicsSyncSprite},
        {"PhysicsSyncText", _functions::PhysicsSyncText},
        {"PhysicsSyncAnimatedSprite", _functions::PhysicsSyncAnimatedSprite},

        // Audio functions
        {"LoadSound", _functions::LoadSound},
        {"PlaySound", _functions::PlaySound},
        {"PlaySoundById", _functions::PlaySoundById},
        {"StopSound", _functions::StopSound},
        {"StopAllSounds", _functions::StopAllSounds},
        {"SetSoundVolume", _functions::SetSoundVolume},
        {"IsSoundPlaying", _functions::IsSoundPlaying},
        {"SetSoundPitch", _functions::SetSoundPitch},
        {"SetSoundLoopingById", _functions::SetSoundLoopingById},
        {"SetSoundPosition", _functions::SetSoundPosition},
        {"SetSoundPositionByName", _functions::SetSoundPositionByName},
        {"GetSoundPosition", _functions::GetSoundPosition},
        {"SetSoundVelocity", _functions::SetSoundVelocity},
        {"SetSoundRolloff", _functions::SetSoundRolloff},
        {"SetSoundReferenceDistance", _functions::SetSoundReferenceDistance},
        {"SetSoundMaxDistance", _functions::SetSoundMaxDistance},
        {"SetSoundSpatial", _functions::SetSoundSpatial},
        {"SetSoundCone", _functions::SetSoundCone},
        {"SetSoundDirection", _functions::SetSoundDirection},
        {"SetListenerPosition", _functions::SetListenerPosition},
        {"GetListenerPosition", _functions::GetListenerPosition},
        {"SetListenerVelocity", _functions::SetListenerVelocity},
        {"SetListenerOrientation", _functions::SetListenerOrientation},
        {"SetListenerGain", _functions::SetListenerGain},
        {"CreateAudioEffect", _functions::CreateAudioEffect},
        {"DestroyAudioEffect", _functions::DestroyAudioEffect},
        {"SetAudioEffectType", _functions::SetAudioEffectType},
        {"SetAudioEffectf", _functions::SetAudioEffectf},
        {"SetAudioEffecti", _functions::SetAudioEffecti},
        {"GetAudioEffectf", _functions::GetAudioEffectf},
        {"GetAudioEffecti", _functions::GetAudioEffecti},
        {"GetAudioEfxSupported", _functions::GetAudioEfxSupported},
        {"CreateAudioFilter", _functions::CreateAudioFilter},
        {"DestroyAudioFilter", _functions::DestroyAudioFilter},
        {"SetAudioFilterType", _functions::SetAudioFilterType},
        {"SetAudioFilterf", _functions::SetAudioFilterf},
        {"CreateAudioEffectSlot", _functions::CreateAudioEffectSlot},
        {"DestroyAudioEffectSlot", _functions::DestroyAudioEffectSlot},
        {"SetAudioEffectSlotEffect", _functions::SetAudioEffectSlotEffect},
        {"ClearAudioEffectSlotEffect", _functions::ClearAudioEffectSlotEffect},
        {"SetAudioEffectSlotGain", _functions::SetAudioEffectSlotGain},
        {"AttachAudioEffect", _functions::AttachAudioEffect},
        {"DetachAudioEffect", _functions::DetachAudioEffect},
        {"AttachAudioFilter", _functions::AttachAudioFilter},

        // Window functions
        {"CreateWindow", _functions::CreateWindow},
        {"SetWindowTitle", _functions::SetWindowTitle},
        {"SetWindowSize", _functions::SetWindowSize},
        {"SetWindowIcon", _functions::SetWindowIcon},

        // Render order
        {"SetRenderOrder", _functions::SetRenderOrder},
        {"GetRenderOrder", _functions::GetRenderOrder},

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

        // Fullscreen functions
        {"SetMainWindowFullscreen", _functions::SetMainWindowFullscreen},
        {"IsMainWindowFullscreen", _functions::IsMainWindowFullscreen},
        {"ToggleMainWindowFullscreen", _functions::ToggleMainWindowFullscreen},

        // VSync functions
        {"SetMainWindowVSync", _functions::SetMainWindowVSync},
        {"IsMainWindowVSync", _functions::IsMainWindowVSync},
        {"ToggleMainWindowVSync", _functions::ToggleMainWindowVSync},

        // Input functions
        {"IsKeyPressed", _functions::IsKeyPressed},
        {"IsKeyJustPressed", _functions::IsKeyJustPressed},
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

        // Network - Init
        {"NetInit", _netFunctions::NetInit},
        {"NetShutdown", _netFunctions::NetShutdown},

        // Network - TCP Server
        {"CreateTCPServer", _netFunctions::CreateTCPServer},
        {"TCPServerStop", _netFunctions::TCPServerStop},
        {"TCPServerIsRunning", _netFunctions::TCPServerIsRunning},
        {"TCPServerPoll", _netFunctions::TCPServerPoll},
        {"TCPServerAccept", _netFunctions::TCPServerAccept},
        {"TCPServerSend", _netFunctions::TCPServerSend},
        {"TCPServerBroadcast", _netFunctions::TCPServerBroadcast},
        {"TCPServerReceive", _netFunctions::TCPServerReceive},
        {"TCPServerDisconnect", _netFunctions::TCPServerDisconnect},
        {"TCPServerGetClientCount", _netFunctions::TCPServerGetClientCount},
        {"TCPServerIsClientConnected", _netFunctions::TCPServerIsClientConnected},

        // Network - TCP Client
        {"CreateTCPClient", _netFunctions::CreateTCPClient},
        {"TCPClientConnect", _netFunctions::TCPClientConnect},
        {"TCPClientSend", _netFunctions::TCPClientSend},
        {"TCPClientReceive", _netFunctions::TCPClientReceive},
        {"TCPClientDisconnect", _netFunctions::TCPClientDisconnect},
        {"TCPClientIsConnected", _netFunctions::TCPClientIsConnected},

        // Network - UDP Server
        {"CreateUDPServer", _netFunctions::CreateUDPServer},
        {"UDPServerStop", _netFunctions::UDPServerStop},
        {"UDPServerIsRunning", _netFunctions::UDPServerIsRunning},
        {"UDPServerPoll", _netFunctions::UDPServerPoll},
        {"UDPServerSend", _netFunctions::UDPServerSend},
        {"UDPServerReceive", _netFunctions::UDPServerReceive},
        {"UDPServerReceiveAny", _netFunctions::UDPServerReceiveAny},
        {"UDPServerRemovePeer", _netFunctions::UDPServerRemovePeer},
        {"UDPServerGetPeerCount", _netFunctions::UDPServerGetPeerCount},
        {"UDPServerIsPeerKnown", _netFunctions::UDPServerIsPeerKnown},

        // Network - UDP Client
        {"CreateUDPClient", _netFunctions::CreateUDPClient},
        {"UDPClientConnect", _netFunctions::UDPClientConnect},
        {"UDPClientSend", _netFunctions::UDPClientSend},
        {"UDPClientReceive", _netFunctions::UDPClientReceive},
        {"UDPClientDisconnect", _netFunctions::UDPClientDisconnect},
        {"UDPClientIsConnected", _netFunctions::UDPClientIsConnected},

        // Graphics API
        {"SetGraphicsAPI", _functions::SetGraphicsAPI},
        {"GetGraphicsAPI", _functions::GetGraphicsAPI},

        {nullptr, nullptr}
    };

    // ==================== IMPLEMENTATION ====================
    LuaEngine::LuaEngine(Window &window, IRenderDevice* device) :
        state(nullptr),
        spriteWorld(), animatedSpriteWorld(), textWorld(), meshWorld(), cameraWorld(), particleWorld(),
        audioEngine(), physicsWorld(),
        deltaTime(0.0f),
        initialized(false), audioInitialized(false),
        objectMap(), soundNameToId(), physicsBodyMap(),
        scripts(), scriptsListPath(), projectFileName(),
        sceneManager(),
        additionalWindows(), mainWindow(&window),
        renderDevice(device),
        renderContext(device ? device->getContext() : nullptr),
        projectionViewMatrix2D(), quadVertexBufferObject(),
        spriteShader2D(), spriteBatchShader2D(), fontShader2D(), spriteMesh(),
        textureManager(device), fontManager(),
        shaders(), entityShaderMap(), nextShaderId(1),
        renderOrder({"Tilesets", "Sprites", "AnimatedSprites", "Texts", "Meshes", "Particles"})
    {
        this->spriteMesh.setVertexBuffer(this->quadVertexBufferObject);
        this->sceneManager = std::make_unique<SceneManager>(this);

        this->state = luaL_newstate();
        if (this->state == nullptr) {
            fprintf(stderr, "Failed to create Lua state\n");
            return;
        }

        luaL_openlibs(this->state);
        registerCFunctions();

        // Load OOP wrappers
        if (luaL_dostring(this->state, OOP_INIT_SCRIPT.c_str()) != LUA_OK) {
            fprintf(stderr, "Warning: Failed to load OOP wrappers: %s\n", lua_tostring(this->state, -1));
            lua_pop(this->state, 1);
        }

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

        registerMathTypes(state);

        // Noise functions
        lua_pushcfunction(state, _functions::SetNoiseSeed);
        lua_setglobal(state, "SetNoiseSeed");
        lua_pushcfunction(state, _functions::PerlinNoise1D);
        lua_setglobal(state, "PerlinNoise1D");
        lua_pushcfunction(state, _functions::PerlinNoise2D);
        lua_setglobal(state, "PerlinNoise2D");
        lua_pushcfunction(state, _functions::PerlinNoise3D);
        lua_setglobal(state, "PerlinNoise3D");
        lua_pushcfunction(state, _functions::SimplexNoise2D);
        lua_setglobal(state, "SimplexNoise2D");
        lua_pushcfunction(state, _functions::ValueNoise2D);
        lua_setglobal(state, "ValueNoise2D");
        lua_pushcfunction(state, _functions::FbmNoise2D);
        lua_setglobal(state, "FbmNoise2D");
        lua_pushcfunction(state, _functions::FbmSimplexNoise2D);
        lua_setglobal(state, "FbmSimplexNoise2D");
        lua_pushcfunction(state, _functions::DomainWarpNoise2D);
        lua_setglobal(state, "DomainWarpNoise2D");
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

    std::unordered_map<std::string, Entity> LuaEngine::loadSceneFile(const std::string& path) {
        std::unordered_map<std::string, Entity> nameToId;
        std::ifstream f(path);
        if (!f.is_open()) return nameToId;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        BlazeBolt::SceneDocument doc = BlazeBolt::SceneDocument::fromJSON(content);

        auto createPhysicsFromObj = [&](Entity visualId, const BlazeBolt::SceneObjectData& obj) -> Entity {
            if (!obj.numbers.count("body_type")) return 0;
            int bodyType = (int)obj.getNumber("body_type");
            float x = (float)obj.getNumber("pos_x", 0);
            float y = (float)obj.getNumber("pos_y", 0);
            float mass = (float)obj.getNumber("mass", 1);
            float friction = (float)obj.getNumber("friction", 0.3f);
            float restitution = (float)obj.getNumber("restitution", 0.5f);
            Entity bodyEntity = physicsCreateBody(bodyType, x, y, mass, friction, restitution);
            if (!bodyEntity) return 0;
            std::string colliderShape = obj.getString("collider_shape", "circle");
            if (colliderShape == "circle") {
                float radius = (float)obj.getNumber("circle_radius", 32);
                float ox = (float)obj.getNumber("circle_offset_x", 0);
                float oy = (float)obj.getNumber("circle_offset_y", 0);
                physicsAddCircle(bodyEntity, radius, ox, oy);
            } else {
                float hw = (float)obj.getNumber("rect_half_width", 32);
                float hh = (float)obj.getNumber("rect_half_height", 32);
                physicsAddRectangle(bodyEntity, hw, hh);
            }
            if (obj.numbers.count("gravity_scale"))
                physicsSetGravityScale(bodyEntity, (float)obj.getNumber("gravity_scale"));
            if (obj.booleans.count("fixed_rotation"))
                physicsSetFixedRotation(bodyEntity, obj.getBool("fixed_rotation"));
            if (obj.booleans.count("bullet"))
                physicsSetBullet(bodyEntity, obj.getBool("bullet"));
            if (visualId)
                visualToPhysicsMap[visualId] = bodyEntity;
            return bodyEntity;
        };

        for (const auto& obj : doc.objects) {
            if (obj.name.empty()) continue;
            Entity id = 0;

            if (obj.type == "sprite") {
                std::string tex = obj.getString("texture");
                double x = obj.getNumber("pos_x");
                double y = obj.getNumber("pos_y");
                id = createSprite(tex, Vector2((float)x, (float)y));
                if (id) {
                    if (obj.numbers.count("size_x") || obj.numbers.count("size_y"))
                        spriteSetSize(id, Vector2((float)obj.getNumber("size_x", 64), (float)obj.getNumber("size_y", 64)));
                    if (obj.numbers.count("rot"))
                        spriteSetRotation(id, (float)obj.getNumber("rot"));
                    if (obj.numbers.count("color_r") || obj.numbers.count("color_g") || obj.numbers.count("color_b") || obj.numbers.count("color_a"))
                        spriteSetColor(id, Vector4((float)obj.getNumber("color_r", 1), (float)obj.getNumber("color_g", 1), (float)obj.getNumber("color_b", 1), (float)obj.getNumber("color_a", 1)));
                    if (obj.booleans.count("visible"))
                        spriteSetVisible(id, obj.getBool("visible"));
                    if (obj.numbers.count("origin_x") || obj.numbers.count("origin_y"))
                        spriteSetOrigin(id, Vector2((float)obj.getNumber("origin_x", 0.5), (float)obj.getNumber("origin_y", 0.5)));
                    entityTexturePaths[id] = tex;
                    createPhysicsFromObj(id, obj);
                }
            } else if (obj.type == "animated_sprite") {
                std::string tex = obj.getString("texture");
                double x = obj.getNumber("pos_x");
                double y = obj.getNumber("pos_y");
                id = createAnimatedSprite(tex, Vector2((float)x, (float)y));
                if (id) {
                    if (obj.numbers.count("size_x") || obj.numbers.count("size_y"))
                        animatedSpriteSetSize(id, Vector2((float)obj.getNumber("size_x", 64), (float)obj.getNumber("size_y", 64)));
                    if (obj.numbers.count("rot"))
                        animatedSpriteSetRotation(id, (float)obj.getNumber("rot"));
                    if (obj.numbers.count("color_r") || obj.numbers.count("color_g") || obj.numbers.count("color_b") || obj.numbers.count("color_a"))
                        animatedSpriteSetColor(id, Vector4((float)obj.getNumber("color_r", 1), (float)obj.getNumber("color_g", 1), (float)obj.getNumber("color_b", 1), (float)obj.getNumber("color_a", 1)));
                    if (obj.booleans.count("visible"))
                        obj.booleans.at("visible") ? animatedSpritePlay(id) : (void)0;
                    if (obj.booleans.count("looping"))
                        animatedSpriteSetLooping(id, obj.getBool("looping"));
                    if (obj.numbers.count("playback_speed"))
                        animatedSpriteSetPlaybackSpeed(id, (float)obj.getNumber("playback_speed"));
                    entityTexturePaths[id] = tex;
                    createPhysicsFromObj(id, obj);
                }
            } else if (obj.type == "text") {
                std::string font = obj.getString("font");
                std::string text = obj.getString("text");
                double x = obj.getNumber("pos_x");
                double y = obj.getNumber("pos_y");
                id = createText(font, text, Vector2((float)x, (float)y));
                if (id) {
                    if (obj.numbers.count("scale_x") || obj.numbers.count("scale_y"))
                        textSetScale(id, Vector2((float)obj.getNumber("scale_x", 1), (float)obj.getNumber("scale_y", 1)));
                    if (obj.numbers.count("rot"))
                        textSetRotation(id, (float)obj.getNumber("rot"));
                    if (obj.numbers.count("color_r") || obj.numbers.count("color_g") || obj.numbers.count("color_b") || obj.numbers.count("color_a"))
                        textSetColor(id, Vector4((float)obj.getNumber("color_r", 1), (float)obj.getNumber("color_g", 1), (float)obj.getNumber("color_b", 1), (float)obj.getNumber("color_a", 1)));
                    if (obj.booleans.count("visible"))
                        textSetVisible(id, obj.getBool("visible"));
                    if (obj.numbers.count("alignment"))
                        textSetAlignment(id, static_cast<BlazeBolt::Text2D::Alignment>((int)obj.getNumber("alignment")));
                    entityFontPaths[id] = font;
                    createPhysicsFromObj(id, obj);
                }
            } else if (obj.type == "camera") {
                id = createCamera();
                if (id) {
                    if (obj.numbers.count("pos_x") || obj.numbers.count("pos_y"))
                        cameraSetPosition(id, Vector2((float)obj.getNumber("pos_x"), (float)obj.getNumber("pos_y")));
                    if (obj.numbers.count("zoom"))
                        cameraSetZoom(id, (float)obj.getNumber("zoom"));
                    if (obj.numbers.count("rot"))
                        cameraSetRotation(id, (float)obj.getNumber("rot"));
                    createPhysicsFromObj(id, obj);
                }
            } else if (obj.type == "point_light") {
                id = createPointLight(
                    (float)obj.getNumber("pos_x"),
                    (float)obj.getNumber("pos_y"),
                    (float)obj.getNumber("color_r", 1),
                    (float)obj.getNumber("color_g", 1),
                    (float)obj.getNumber("color_b", 1),
                    (float)obj.getNumber("intensity", 1),
                    (float)obj.getNumber("radius", 200));
                if (id && obj.booleans.count("enabled"))
                    lightSetEnabled(id, obj.getBool("enabled"));
                createPhysicsFromObj(id, obj);
            } else if (obj.type == "ambient_light") {
                id = createAmbientLight(
                    (float)obj.getNumber("color_r", 1),
                    (float)obj.getNumber("color_g", 1),
                    (float)obj.getNumber("color_b", 1),
                    (float)obj.getNumber("intensity", 0.3));
                createPhysicsFromObj(id, obj);
            } else if (obj.type == "particle_system") {
                id = createParticleSystem();
                if (id) {
                    if (obj.numbers.count("pos_x") || obj.numbers.count("pos_y"))
                        particleSystemSetPosition(id, Vector2((float)obj.getNumber("pos_x"), (float)obj.getNumber("pos_y")));
                    std::string ptex = obj.getString("texture");
                    if (!ptex.empty()) particleSystemSetTexture(id, ptex);
                    if (obj.numbers.count("emission_rate"))
                        particleSystemSetEmissionRate(id, (float)obj.getNumber("emission_rate"));
                    if (obj.booleans.count("active"))
                        particleSystemSetActive(id, obj.getBool("active"));
                    if (obj.booleans.count("visible"))
                        particleSystemSetVisible(id, obj.getBool("visible"));
                    createPhysicsFromObj(id, obj);
                }
            } else if (obj.type == "tileset") {
                std::string ttex = obj.getString("texture");
                auto tw = (uint32_t)obj.getNumber("tile_width", 32);
                auto th = (uint32_t)obj.getNumber("tile_height", 32);
                auto ac = (uint32_t)obj.getNumber("atlas_cols", 8);
                auto ar = (uint32_t)obj.getNumber("atlas_rows", 8);
                id = createTileset(ttex, tw, th, ac, ar);
                if (id && (obj.numbers.count("pos_x") || obj.numbers.count("pos_y")))
                    tilesetSetPosition(id, Vector2((float)obj.getNumber("pos_x"), (float)obj.getNumber("pos_y")));
                createPhysicsFromObj(id, obj);
            } else if (obj.type == "physics_body") {
                float x = (float)obj.getNumber("pos_x", 0);
                float y = (float)obj.getNumber("pos_y", 0);
                id = createPhysicsFromObj(0, obj);
                if (id && obj.numbers.count("rot")) {
                    auto it = physicsBodyMap.find(id);
                    if (it != physicsBodyMap.end())
                        it->second->setAngle((float)obj.getNumber("rot") * 3.14159265f / 180.0f);
                }
            }

            if (id && !obj.name.empty()) {
                nameToId[obj.name] = id;
                entityNames[id] = obj.name;
            }
        }
        return nameToId;
    }

    bool LuaEngine::saveSceneFile(const std::string& path) {
        BlazeBolt::SceneDocument doc;
        doc.name = std::filesystem::path(path).stem().string();

        auto saveVec2 = [](const Vector2& v) -> std::vector<double> { return {v.x, v.y}; };
        auto saveVec3 = [](const Vector3& v) -> std::vector<double> { return {v.x, v.y, v.z}; };
        auto saveVec4 = [](const Vector4& v) -> std::vector<double> { return {v.x, v.y, v.z, v.w}; };

        auto savePhysicsProps = [&](BlazeBolt::SceneObjectData& obj, Entity visualId) {
            auto it = visualToPhysicsMap.find(visualId);
            if (it == visualToPhysicsMap.end()) return;
            auto bit = physicsBodyMap.find(it->second);
            if (bit == physicsBodyMap.end()) return;
            PhysicsBody* body = bit->second;
            obj.numbers["body_type"] = (double)(int)body->getType();
            obj.numbers["mass"] = body->getMass();
            obj.numbers["friction"] = body->getFriction();
            obj.numbers["restitution"] = body->getRestitution();
            obj.numbers["gravity_scale"] = body->getGravityScale();
            obj.booleans["fixed_rotation"] = body->isFixedRotation();
            obj.booleans["bullet"] = body->isBullet();
            const auto& circles = body->getCircles();
            const auto& rects = body->getRectangles();
            if (!circles.empty()) {
                obj.strings["collider_shape"] = "circle";
                obj.numbers["circle_radius"] = circles[0].radius;
                obj.numbers["circle_offset_x"] = circles[0].offsetX;
                obj.numbers["circle_offset_y"] = circles[0].offsetY;
            } else if (!rects.empty()) {
                obj.strings["collider_shape"] = "rectangle";
                obj.numbers["rect_half_width"] = rects[0].halfWidth;
                obj.numbers["rect_half_height"] = rects[0].halfHeight;
            }
        };

        for (const auto& pair : spriteWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::SceneObjectData obj;
            obj.type = "sprite";
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            obj.numbers["pos_x"] = pair.first->getPosition().x;
            obj.numbers["pos_y"] = pair.first->getPosition().y;
            obj.numbers["size_x"] = pair.first->getScale().x;
            obj.numbers["size_y"] = pair.first->getScale().y;
            obj.numbers["rot"] = pair.first->getRotation();
            obj.numbers["origin_x"] = pair.first->getOrigin().x;
            obj.numbers["origin_y"] = pair.first->getOrigin().y;
            auto c = pair.first->getColor();
            obj.numbers["color_r"] = c.x; obj.numbers["color_g"] = c.y;
            obj.numbers["color_b"] = c.z; obj.numbers["color_a"] = c.w;
            obj.booleans["visible"] = pair.first->isVisible();
            auto tit = entityTexturePaths.find(pair.second);
            if (tit != entityTexturePaths.end()) obj.strings["texture"] = tit->second;
            savePhysicsProps(obj, pair.second);
            doc.objects.push_back(obj);
        }

        for (const auto& pair : animatedSpriteWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::SceneObjectData obj;
            obj.type = "animated_sprite";
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            obj.numbers["pos_x"] = pair.first->getPosition().x;
            obj.numbers["pos_y"] = pair.first->getPosition().y;
            obj.numbers["size_x"] = pair.first->getScale().x;
            obj.numbers["size_y"] = pair.first->getScale().y;
            obj.numbers["rot"] = pair.first->getRotation();
            obj.numbers["origin_x"] = pair.first->getOrigin().x;
            obj.numbers["origin_y"] = pair.first->getOrigin().y;
            auto c = pair.first->getColor();
            obj.numbers["color_r"] = c.x; obj.numbers["color_g"] = c.y;
            obj.numbers["color_b"] = c.z; obj.numbers["color_a"] = c.w;
            obj.booleans["visible"] = pair.first->isVisible();
            obj.booleans["looping"] = pair.first->isLooping();
            obj.numbers["playback_speed"] = pair.first->getPlaybackSpeed();
            auto tit = entityTexturePaths.find(pair.second);
            if (tit != entityTexturePaths.end()) obj.strings["texture"] = tit->second;
            savePhysicsProps(obj, pair.second);
            doc.objects.push_back(obj);
        }

        for (const auto& pair : textWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::SceneObjectData obj;
            obj.type = "text";
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            obj.numbers["pos_x"] = pair.first->getPosition().x;
            obj.numbers["pos_y"] = pair.first->getPosition().y;
            obj.numbers["scale_x"] = pair.first->getScale().x;
            obj.numbers["scale_y"] = pair.first->getScale().y;
            obj.numbers["rot"] = pair.first->getRotation();
            auto c = pair.first->getColor();
            obj.numbers["color_r"] = c.x; obj.numbers["color_g"] = c.y;
            obj.numbers["color_b"] = c.z; obj.numbers["color_a"] = c.w;
            obj.booleans["visible"] = pair.first->isVisible();
            obj.numbers["alignment"] = (double)pair.first->getAlignment();
            obj.strings["text"] = pair.first->getText();
            auto fit = entityFontPaths.find(pair.second);
            if (fit != entityFontPaths.end()) obj.strings["font"] = fit->second;
            savePhysicsProps(obj, pair.second);
            doc.objects.push_back(obj);
        }

        for (const auto& pair : cameraWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::SceneObjectData obj;
            obj.type = "camera";
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            obj.numbers["pos_x"] = pair.first->getPosition().x;
            obj.numbers["pos_y"] = pair.first->getPosition().y;
            obj.numbers["zoom"] = pair.first->getZoom();
            obj.numbers["rot"] = pair.first->getRotation();
            savePhysicsProps(obj, pair.second);
            doc.objects.push_back(obj);
        }

        for (const auto& pair : lightWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::SceneObjectData obj;
            if (pair.first->getType() == BlazeBolt::Light2D::POINT) {
                obj.type = "point_light";
                obj.numbers["pos_x"] = pair.first->getPosition().x;
                obj.numbers["pos_y"] = pair.first->getPosition().y;
                obj.numbers["radius"] = pair.first->getRadius();
            } else {
                obj.type = "ambient_light";
            }
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            auto c = pair.first->getColor();
            obj.numbers["color_r"] = c.x; obj.numbers["color_g"] = c.y;
            obj.numbers["color_b"] = c.z;
            obj.numbers["intensity"] = pair.first->getIntensity();
            obj.booleans["enabled"] = pair.first->isEnabled();
            savePhysicsProps(obj, pair.second);
            doc.objects.push_back(obj);
        }

        for (const auto& pair : particleWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::SceneObjectData obj;
            obj.type = "particle_system";
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            obj.numbers["pos_x"] = pair.first->getPosition().x;
            obj.numbers["pos_y"] = pair.first->getPosition().y;
            obj.booleans["active"] = pair.first->isActive();
            obj.booleans["visible"] = pair.first->isVisible();
            savePhysicsProps(obj, pair.second);
            doc.objects.push_back(obj);
        }

        // Save standalone physics bodies (physics_body type)
        for (const auto& pair : meshWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            auto bit = physicsBodyMap.find(pair.second);
            if (bit == physicsBodyMap.end()) continue;
            BlazeBolt::SceneObjectData obj;
            obj.type = "physics_body";
            { auto it = entityNames.find(pair.second); if (it != entityNames.end()) obj.name = it->second; }
            PhysicsBody* body = bit->second;
            obj.numbers["pos_x"] = body->getPosition().x;
            obj.numbers["pos_y"] = body->getPosition().y;
            obj.numbers["rot"] = body->getAngle() * 180.0 / 3.14159265;
            obj.numbers["body_type"] = (double)(int)body->getType();
            obj.numbers["mass"] = body->getMass();
            obj.numbers["friction"] = body->getFriction();
            obj.numbers["restitution"] = body->getRestitution();
            obj.numbers["gravity_scale"] = body->getGravityScale();
            obj.booleans["fixed_rotation"] = body->isFixedRotation();
            obj.booleans["bullet"] = body->isBullet();
            const auto& circles = body->getCircles();
            const auto& rects = body->getRectangles();
            if (!circles.empty()) {
                obj.strings["collider_shape"] = "circle";
                obj.numbers["circle_radius"] = circles[0].radius;
                obj.numbers["circle_offset_x"] = circles[0].offsetX;
                obj.numbers["circle_offset_y"] = circles[0].offsetY;
            } else if (!rects.empty()) {
                obj.strings["collider_shape"] = "rectangle";
                obj.numbers["rect_half_width"] = rects[0].halfWidth;
                obj.numbers["rect_half_height"] = rects[0].halfHeight;
            }
            doc.objects.push_back(obj);
        }

        std::string json = doc.toJSON();
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << json;
        f.close();
        return true;
    }

    // Sprite implementations
    // FIXME: Maybe, we shouldn't create every entity (e.g. Sprite2D, Text2D, etc.) on heap?
    Entity LuaEngine::createSprite(const std::string &texturePath, const Vector2 &position) {
        BlazeBolt::Sprite2D *sprite = new BlazeBolt::Sprite2D();
        const GL::Texture2D *texture = this->textureManager.loadGLTexture(texturePath);
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
        const GL::Texture2D *texture = this->textureManager.loadGLTexture(texturePath);
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
    void LuaEngine::spriteSetTextureRect(Entity entity, const Vector4& rect) {
        BlazeBolt::Sprite2D *sprite = spriteWorld.getEntity(entity);
        if (sprite == nullptr) {
            return;
        }
        sprite->setTextureRect(rect);
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
        this->uploadLightDataToShader(this->spriteShader2D);
        for (const auto &pair : spriteWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                pair.first->draw(this->textureManager.getDefault2D(), this->spriteShader2D, this->spriteMesh, this->projectionViewMatrix2D);
            }
        }
    }

    // Sprite batch implementations
    Entity LuaEngine::createSpriteBatch(uint32_t maxSize) {
        Entity entity = spriteBatches.size() + 1;
        spriteBatches.emplace_back(maxSize);
        return entity;
    }
    void LuaEngine::spriteBatchSetTexture(Entity batchEntity, const std::string &texturePath) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return;
        const GL::Texture2D *texture = this->textureManager.loadGLTexture(texturePath);
        if (texture != nullptr) {
            spriteBatches[batchEntity - 1].setTexture(*texture);
        }
    }
    bool LuaEngine::spriteBatchAdd(Entity batchEntity, Entity spriteEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return false;
        return spriteBatches[batchEntity - 1].add(spriteEntity);
    }
    bool LuaEngine::spriteBatchRemove(Entity batchEntity, Entity spriteEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return false;
        return spriteBatches[batchEntity - 1].remove(spriteEntity);
    }
    void LuaEngine::spriteBatchClear(Entity batchEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return;
        spriteBatches[batchEntity - 1].clear();
    }
    void LuaEngine::spriteBatchSetMaxSize(Entity batchEntity, uint32_t maxSize) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return;
        spriteBatches[batchEntity - 1].setMaxSize(maxSize);
    }
    uint32_t LuaEngine::spriteBatchGetMaxSize(Entity batchEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return 0;
        return spriteBatches[batchEntity - 1].getMaxSize();
    }
    uint32_t LuaEngine::spriteBatchGetCount(Entity batchEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return 0;
        return spriteBatches[batchEntity - 1].count();
    }
    void LuaEngine::drawAllSpriteBatches() {
        this->spriteBatchShader2D.bind();
        this->spriteBatchShader2D.setAspectRatio(static_cast<float>(getScreenWidth()) / static_cast<float>(getScreenHeight()));
        this->spriteBatchShader2D.setMVPMatrix(this->projectionViewMatrix2D);
        this->uploadLightDataToShader(this->spriteBatchShader2D);
        for (auto &batch : spriteBatches) {
            if (batch.count() == 0) continue;
            batch.rebuild(this->spriteWorld);
            batch.draw(this->textureManager.getDefault2D());
        }
    }
    void LuaEngine::drawSpriteBatch(Entity batchEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return;
        auto &batch = spriteBatches[batchEntity - 1];
        if (batch.count() == 0) return;
        this->spriteBatchShader2D.bind();
        this->spriteBatchShader2D.setAspectRatio(static_cast<float>(getScreenWidth()) / static_cast<float>(getScreenHeight()));
        this->spriteBatchShader2D.setMVPMatrix(this->projectionViewMatrix2D);
        batch.rebuild(this->spriteWorld);
        batch.draw(this->textureManager.getDefault2D());
    }
    void LuaEngine::destroySpriteBatch(Entity batchEntity) {
        if (batchEntity < 1 || batchEntity > spriteBatches.size()) return;
        spriteBatches.erase(spriteBatches.begin() + batchEntity - 1);
    }

    // Animation implementations
    Entity LuaEngine::createAnimatedSprite(const std::string &texturePath, const Vector2 &position) {
        BlazeBolt::AnimatedSprite2D *sprite = new BlazeBolt::AnimatedSprite2D();
        const BlazeBolt::AnimatedTexture2D *texture = this->textureManager.loadFromFileAnimated2D(texturePath);
        if (texture != nullptr) { sprite->setTexture(*texture); }
        sprite->setPosition(position);

        Entity entity = animatedSpriteWorld.spawn(sprite);
        objectMap[entity] = RegisteredObject(RegisteredObject::ANIMATED_SPRITE, sprite, entity);
        return entity;
    }
    void LuaEngine::animatedSpritePlay(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->play(); }
    }
    bool LuaEngine::animatedSpriteIsPlaying(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite != nullptr && sprite->isPlaying();
    }
    void LuaEngine::animatedSpritePause(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->pause(); }
    }
    void LuaEngine::animatedSpriteStop(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->stop(); }
    }
    void LuaEngine::animatedSpriteRestart(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->restart(); }
    }
    void LuaEngine::animatedSpriteSetTexture(Entity entity, const std::string &texturePath) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite == nullptr) { return; }
        const BlazeBolt::AnimatedTexture2D *texture = this->textureManager.loadFromFileAnimated2D(texturePath);
        if (texture != nullptr) { sprite->setTexture(*texture, true); }
    }
    void LuaEngine::animatedSpriteSetLooping(Entity entity, bool looping) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setLooping(looping); }
    }
    bool LuaEngine::animatedSpriteIsLooping(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite != nullptr && sprite->isLooping();
    }
    void LuaEngine::animatedSpriteSetPlaybackSpeed(Entity entity, float playbackSpeed) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setPlaybackSpeed(playbackSpeed); }
    }
    float LuaEngine::animatedSpriteGetPlaybackSpeed(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite != nullptr ? sprite->getPlaybackSpeed() : 0.0f;
    }
    void LuaEngine::animatedSpriteSetFrame(Entity entity, int frame) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setFrame(frame); }
    }
    uint32_t LuaEngine::animatedSpriteGetCurrentFrame(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getCurrentFrame() : 0;
    }
    uint32_t LuaEngine::animatedSpriteGetNumFrames(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getNumFrames() : 0;
    }
    void LuaEngine::animatedSpriteSetPosition(Entity entity, const Vector2 &position) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setPosition(position); }
    }
    Vector2 LuaEngine::animatedSpriteGetPosition(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getPosition() : Vector2(0.0f, 0.0f);
    }
    void LuaEngine::animatedSpriteSetSize(Entity entity, const Vector2 &size) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setScale(size); }
    }
    Vector2 LuaEngine::animatedSpriteGetSize(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getScale() : Vector2(1.0f, 1.0f);
    }
    void LuaEngine::animatedSpriteSetOrigin(Entity entity, const Vector2 &origin) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setOrigin(origin); }
    }
    Vector2 LuaEngine::animatedSpriteGetOrigin(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getOrigin() : Vector2(0.0f, 0.0f);
    }
    void LuaEngine::animatedSpriteSetRotation(Entity entity, float rotation) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setRotation(rotation); }
    }
    float LuaEngine::animatedSpriteGetRotation(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getRotation() : 0.0f;
    }
    void LuaEngine::animatedSpriteSetColor(Entity entity, const Vector4 &color) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        if (sprite != nullptr) { sprite->setColor(color); }
    }
    Vector4 LuaEngine::animatedSpriteGetColor(Entity entity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(entity);
        return sprite ? sprite->getColor() : Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    void LuaEngine::updateAllAnimatedSprites(float deltaTime) {
        for (const std::pair<BlazeBolt::AnimatedSprite2D*, bool> &pair : animatedSpriteWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                pair.first->update(deltaTime);
            }
        }
    }
    void LuaEngine::drawAllAnimatedSprites() {
        for (const std::pair<BlazeBolt::AnimatedSprite2D*, bool> &pair : animatedSpriteWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                pair.first->draw(this->spriteShader2D, this->spriteMesh, this->projectionViewMatrix2D);
            }
        }
    }

    // Animation wheel implementations
    Entity LuaEngine::createAnimationWheel(Entity animatedSpriteEntity) {
        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(animatedSpriteEntity);
        if (sprite == nullptr) { return 0; }

        BlazeBolt::AnimationWheel *wheel = new BlazeBolt::AnimationWheel();
        wheel->setSpriteEntity(animatedSpriteEntity);
        Entity entity = animationWheelWorld.spawn(wheel);
        objectMap[entity] = RegisteredObject(RegisteredObject::ANIMATION_WHEEL, wheel, entity);
        return entity;
    }

    void LuaEngine::animationWheelAddState(Entity wheelEntity, const std::string &name, const std::string &gifPath, float playbackSpeed, bool looping) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->addState(name, gifPath, playbackSpeed, looping);
        }
    }

    void LuaEngine::animationWheelRemoveState(Entity wheelEntity, const std::string &name) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->removeState(name);
        }
    }

    bool LuaEngine::animationWheelHasState(Entity wheelEntity, const std::string &name) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel != nullptr && wheel->hasState(name);
    }

    void LuaEngine::animationWheelSetInitialState(Entity wheelEntity, const std::string &name) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->setInitialState(name);
        }
    }

    std::string LuaEngine::animationWheelGetInitialState(Entity wheelEntity) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->getInitialState() : "";
    }

    void LuaEngine::animationWheelSetState(Entity wheelEntity, const std::string &name) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel == nullptr) { return; }

        const BlazeBolt::AnimationState *animState = wheel->getState(name);
        if (animState == nullptr) { return; }

        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(wheel->getSpriteEntity());
        if (sprite == nullptr) { return; }

        wheel->setCurrentState(name);

        // Apply the state to the animated sprite
        const BlazeBolt::AnimatedTexture2D *texture = this->textureManager.loadFromFileAnimated2D(animState->gifPath);
        if (texture != nullptr) {
            sprite->setTexture(*texture, true);
        }
        sprite->setPlaybackSpeed(animState->playbackSpeed);
        sprite->setLooping(animState->looping);
        sprite->restart();
    }

    std::string LuaEngine::animationWheelGetState(Entity wheelEntity) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->getCurrentState() : "";
    }

    void LuaEngine::animationWheelSetPlaybackSpeed(Entity wheelEntity, const std::string &stateName, float speed) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->setPlaybackSpeed(stateName, speed);
        }
    }

    float LuaEngine::animationWheelGetPlaybackSpeed(Entity wheelEntity, const std::string &stateName) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->getPlaybackSpeed(stateName) : 1.0f;
    }

    void LuaEngine::animationWheelSetLooping(Entity wheelEntity, const std::string &stateName, bool looping) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->setLooping(stateName, looping);
        }
    }

    bool LuaEngine::animationWheelIsLooping(Entity wheelEntity, const std::string &stateName) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->isLooping(stateName) : true;
    }

    void LuaEngine::animationWheelSetGifPath(Entity wheelEntity, const std::string &stateName, const std::string &gifPath) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->setGifPath(stateName, gifPath);
        }
    }

    std::string LuaEngine::animationWheelGetGifPath(Entity wheelEntity, const std::string &stateName) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->getGifPath(stateName) : "";
    }

    void LuaEngine::animationWheelSetAutoAdvance(Entity wheelEntity, bool autoAdvance) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            wheel->setAutoAdvance(autoAdvance);
        }
    }

    bool LuaEngine::animationWheelGetAutoAdvance(Entity wheelEntity) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->getAutoAdvance() : false;
    }

    std::vector<std::string> LuaEngine::animationWheelGetStateNames(Entity wheelEntity) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        return wheel ? wheel->getStateNames() : std::vector<std::string>();
    }

    void LuaEngine::updateAnimationWheels(float dt) {
        // Auto-advance: when a non-looping animation finishes, transition to the next state
        // For now this is a placeholder - auto-advance logic will be handled in Lua via the API
    }

    void LuaEngine::destroyAnimationWheel(Entity wheelEntity) {
        BlazeBolt::AnimationWheel *wheel = animationWheelWorld.getEntity(wheelEntity);
        if (wheel != nullptr) {
            objectMap.erase(wheelEntity);
            animationWheelWorld.destroy(wheelEntity);
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
                pair.first->draw(this->fontShader2D, this->projectionViewMatrix2D);
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
                    Shader* customShader = getShaderInternal(shaderIt->second);
                    if (customShader) {
                        customShader->use();
                    }
                }
                pair.first->draw();
            }
        }
    }

    // Shader implementations
    unsigned int LuaEngine::createShaderInternal(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
        unsigned int id = nextShaderId++;
        Shader* shader = new Shader(vertexPath, fragmentPath);
        shaders[id] = ShaderInfo(name, vertexPath, fragmentPath, id);
        shaders[id].shader = shader;
        return id;
    }

    void LuaEngine::destroyShaderInternal(unsigned int shaderId) {
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

    Shader* LuaEngine::getShaderInternal(unsigned int shaderId) const {
        auto it = shaders.find(shaderId);
        if (it != shaders.end()) {
            return it->second.shader;
        }
        return nullptr;
    }

    // Mesh shader implementations
    void LuaEngine::meshSetShader(Entity entity, const std::string& vertexPath, const std::string& fragmentPath) {
        Mesh2D* mesh = meshWorld.getEntity(entity);
        if (!mesh) return;

        // Find existing shader for this mesh or create new one
        auto it = entityShaderMap.find(entity);
        unsigned int shaderId = 0;

        if (it != entityShaderMap.end()) {
            // Reuse existing shader slot
            shaderId = it->second;
            Shader* oldShader = getShaderInternal(shaderId);
            if (oldShader) {
                delete oldShader;
            }
            Shader* shader = new Shader(vertexPath, fragmentPath);
            shaders[shaderId].shader = shader;
            shaders[shaderId].vertexPath = vertexPath;
            shaders[shaderId].fragmentPath = fragmentPath;
        } else {
            // Create new shader
            shaderId = nextShaderId++;
            Shader* shader = new Shader(vertexPath, fragmentPath);
            shaders[shaderId] = ShaderInfo("mesh_shader", vertexPath, fragmentPath, shaderId);
            shaders[shaderId].shader = shader;
            entityShaderMap[entity] = shaderId;
        }
    }

    void LuaEngine::meshSetUniformFloat(Entity entity, const std::string& name, float value) {
        auto it = entityShaderMap.find(entity);
        if (it == entityShaderMap.end()) return;
        Shader* shader = getShaderInternal(it->second);
        if (shader) {
            shader->use();
            shader->setFloat(name, value);
        }
    }

    void LuaEngine::meshSetUniformInt(Entity entity, const std::string& name, int value) {
        auto it = entityShaderMap.find(entity);
        if (it == entityShaderMap.end()) return;
        Shader* shader = getShaderInternal(it->second);
        if (shader) {
            shader->use();
            shader->setInt(name, value);
        }
    }

    void LuaEngine::meshSetUniformVec2(Entity entity, const std::string& name, float x, float y) {
        auto it = entityShaderMap.find(entity);
        if (it == entityShaderMap.end()) return;
        Shader* shader = getShaderInternal(it->second);
        if (shader) {
            shader->use();
            shader->setVec2(name, x, y);
        }
    }

    void LuaEngine::meshSetUniformVec3(Entity entity, const std::string& name, float x, float y, float z) {
        auto it = entityShaderMap.find(entity);
        if (it == entityShaderMap.end()) return;
        Shader* shader = getShaderInternal(it->second);
        if (shader) {
            shader->use();
            shader->setVec3(name, x, y, z);
        }
    }

    void LuaEngine::meshSetUniformVec4(Entity entity, const std::string& name, float x, float y, float z, float w) {
        auto it = entityShaderMap.find(entity);
        if (it == entityShaderMap.end()) return;
        Shader* shader = getShaderInternal(it->second);
        if (shader) {
            shader->use();
            shader->setVec4(name, x, y, z, w);
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

    void LuaEngine::setSoundPitch(int soundId, float pitch) {
        if (!audioInitialized) return;
        audioEngine.setPitch(soundId, pitch);
    }

    void LuaEngine::setSoundLooping(int soundId, bool loop) {
        if (!audioInitialized) return;
        audioEngine.setLooping(soundId, loop);
    }

    // 3D Positional Audio
    void LuaEngine::setSoundPosition(int soundId, float x, float y, float z) {
        if (!audioInitialized) return;
        audioEngine.setSourcePosition(soundId, x, y, z);
    }

    void LuaEngine::setSoundPositionByName(const std::string& soundName, float x, float y, float z) {
        if (!audioInitialized) return;
        audioEngine.setSourcePosition(soundName, x, y, z);
    }

    void LuaEngine::getSoundPosition(int soundId, float& x, float& y, float& z) {
        if (!audioInitialized) { x = y = z = 0; return; }
        audioEngine.getSourcePosition(soundId, x, y, z);
    }

    void LuaEngine::setSoundVelocity(int soundId, float x, float y, float z) {
        if (!audioInitialized) return;
        audioEngine.setSourceVelocity(soundId, x, y, z);
    }

    void LuaEngine::setSoundRolloff(int soundId, float rolloff) {
        if (!audioInitialized) return;
        audioEngine.setSourceRolloff(soundId, rolloff);
    }

    void LuaEngine::setSoundReferenceDistance(int soundId, float dist) {
        if (!audioInitialized) return;
        audioEngine.setSourceReferenceDistance(soundId, dist);
    }

    void LuaEngine::setSoundMaxDistance(int soundId, float dist) {
        if (!audioInitialized) return;
        audioEngine.setSourceMaxDistance(soundId, dist);
    }

    void LuaEngine::setSoundSpatial(int soundId, bool spatial) {
        if (!audioInitialized) return;
        audioEngine.setSourceSpatial(soundId, spatial);
    }

    void LuaEngine::setSoundCone(int soundId, float innerAngle, float outerAngle, float outerGain) {
        if (!audioInitialized) return;
        audioEngine.setSourceCone(soundId, innerAngle, outerAngle, outerGain);
    }

    void LuaEngine::setSoundDirection(int soundId, float x, float y, float z) {
        if (!audioInitialized) return;
        audioEngine.setSourceDirection(soundId, x, y, z);
    }

    // Listener
    void LuaEngine::setListenerPosition(float x, float y, float z) {
        if (!audioInitialized) return;
        audioEngine.setListenerPosition(x, y, z);
    }

    void LuaEngine::getListenerPosition(float& x, float& y, float& z) {
        if (!audioInitialized) { x = y = z = 0; return; }
        audioEngine.getListenerPosition(x, y, z);
    }

    void LuaEngine::setListenerVelocity(float x, float y, float z) {
        if (!audioInitialized) return;
        audioEngine.setListenerVelocity(x, y, z);
    }

    void LuaEngine::setListenerOrientation(float fx, float fy, float fz, float ux, float uy, float uz) {
        if (!audioInitialized) return;
        audioEngine.setListenerOrientation(fx, fy, fz, ux, uy, uz);
    }

    void LuaEngine::setListenerGain(float gain) {
        if (!audioInitialized) return;
        audioEngine.setListenerGain(gain);
    }

    // EFX Effects
    int LuaEngine::createAudioEffect() {
        if (!audioInitialized) return -1;
        return audioEngine.createEffect();
    }

    void LuaEngine::destroyAudioEffect(int effectIndex) {
        if (!audioInitialized) return;
        audioEngine.destroyEffect(effectIndex);
    }

    bool LuaEngine::setAudioEffectType(int effectIndex, int type) {
        if (!audioInitialized) return false;
        return audioEngine.setEffectType(effectIndex, type);
    }

    bool LuaEngine::setAudioEffectf(int effectIndex, int param, float value) {
        if (!audioInitialized) return false;
        return audioEngine.setEffectf(effectIndex, param, value);
    }

    bool LuaEngine::setAudioEffecti(int effectIndex, int param, int value) {
        if (!audioInitialized) return false;
        return audioEngine.setEffecti(effectIndex, param, value);
    }

    float LuaEngine::getAudioEffectf(int effectIndex, int param) {
        if (!audioInitialized) return 0;
        return audioEngine.getEffectf(effectIndex, param);
    }

    int LuaEngine::getAudioEffecti(int effectIndex, int param) {
        if (!audioInitialized) return 0;
        return audioEngine.getEffecti(effectIndex, param);
    }

    bool LuaEngine::getAudioEfxSupported() {
        if (!audioInitialized) return false;
        return audioEngine.getEfxSupported();
    }

    // EFX Filters
    int LuaEngine::createAudioFilter() {
        if (!audioInitialized) return -1;
        return audioEngine.createFilter();
    }

    void LuaEngine::destroyAudioFilter(int filterIndex) {
        if (!audioInitialized) return;
        audioEngine.destroyFilter(filterIndex);
    }

    bool LuaEngine::setAudioFilterType(int filterIndex, int type) {
        if (!audioInitialized) return false;
        return audioEngine.setFilterType(filterIndex, type);
    }

    bool LuaEngine::setAudioFilterf(int filterIndex, int param, float value) {
        if (!audioInitialized) return false;
        return audioEngine.setFilterf(filterIndex, param, value);
    }

    // EFX Effect Slots
    int LuaEngine::createAudioEffectSlot() {
        if (!audioInitialized) return -1;
        return audioEngine.createEffectSlot();
    }

    void LuaEngine::destroyAudioEffectSlot(int slotIndex) {
        if (!audioInitialized) return;
        audioEngine.destroyEffectSlot(slotIndex);
    }

    bool LuaEngine::setAudioEffectSlotEffect(int slotIndex, int effectIndex) {
        if (!audioInitialized) return false;
        return audioEngine.setEffectSlotEffect(slotIndex, effectIndex);
    }

    bool LuaEngine::clearAudioEffectSlotEffect(int slotIndex) {
        if (!audioInitialized) return false;
        return audioEngine.clearEffectSlotEffect(slotIndex);
    }

    bool LuaEngine::setAudioEffectSlotGain(int slotIndex, float gain) {
        if (!audioInitialized) return false;
        return audioEngine.setEffectSlotGain(slotIndex, gain);
    }

    // Linking
    bool LuaEngine::attachAudioEffect(int soundId, int slotIndex) {
        if (!audioInitialized) return false;
        return audioEngine.attachEffectToSource(soundId, slotIndex);
    }

    bool LuaEngine::detachAudioEffect(int soundId) {
        if (!audioInitialized) return false;
        return audioEngine.detachEffectFromSource(soundId);
    }

    bool LuaEngine::attachAudioFilter(int soundId, int filterIndex) {
        if (!audioInitialized) return false;
        return audioEngine.attachFilterToSource(soundId, filterIndex);
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
                case RegisteredObject::ANIMATED_SPRITE:
                    animatedSpriteWorld.destroy(entity);
                    break;
                case RegisteredObject::TEXT:
                    textWorld.destroy(entity);
                    break;
                case RegisteredObject::MESH:
                    meshWorld.destroy(entity);
                    break;
                case RegisteredObject::CAMERA:
                    cameraWorld.destroy(entity);
                    break;
                case RegisteredObject::PARTICLE:
                    particleWorld.destroy(entity);
                    break;
                case RegisteredObject::TILESET:
                    tilesetWorld.destroy(entity);
                    break;
                case RegisteredObject::LIGHT:
                    lightWorld.destroy(entity);
                    break;
                case RegisteredObject::ANIMATION_WHEEL:
                    animationWheelWorld.destroy(entity);
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
        spriteBatches.clear();
        animatedSpriteWorld.clear();
        textWorld.clear();
        meshWorld.clear();
        cameraWorld.clear();
        particleWorld.clear();
        tilesetWorld.clear();
        lightWorld.clear();
        animationWheelWorld.clear();
        physicsWorld.clear();
        physicsBodyMap.clear();
        objectMap.clear();
        entityShaderMap.clear();
        for (auto &[_, value] : shaders) {
            delete value.shader;
        }
        shaders.clear();
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

    void LuaEngine::physicsSetFriction(Entity bodyEntity, float friction) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setFriction(friction);
        }
    }

    float LuaEngine::physicsGetFriction(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->getFriction() : 0;
    }

    void LuaEngine::physicsSetRestitution(Entity bodyEntity, float restitution) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it != physicsBodyMap.end()) {
            it->second->setRestitution(restitution);
        }
    }

    float LuaEngine::physicsGetRestitution(Entity bodyEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        return it != physicsBodyMap.end() ? it->second->getRestitution() : 0;
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

    void LuaEngine::physicsSyncText(Entity bodyEntity, Entity textEntity) {
        auto it = physicsBodyMap.find(bodyEntity);
        if (it == physicsBodyMap.end()) { return; }
        BlazeBolt::Text2D *text = textWorld.getEntity(textEntity);
        if (text == nullptr) { return; }

        PhysicsBody *body = it->second;
        Vector2 position = body->getPosition();
        float angle = body->getAngle();

        text->setPosition(position);
        text->setRotation(angle * (180.0f / 3.14159265358979323846f));
    }

    void LuaEngine::physicsSyncAnimatedSprite(Entity bodyEntity, Entity spriteEntity) {
        std::unordered_map<Entity, PhysicsBody*>::iterator it = physicsBodyMap.find(bodyEntity);
        if (it == physicsBodyMap.end()) { return; }

        BlazeBolt::AnimatedSprite2D *sprite = animatedSpriteWorld.getEntity(spriteEntity);
        if (sprite == nullptr) { return; }

        PhysicsBody *body = it->second;
        Vector2 position = body->getPosition();
        float angle = body->getAngle();

        sprite->setPosition(position);
        sprite->setRotation(angle * (180.0f / M_PIf));
    }

    // Camera implementations
    Entity LuaEngine::createCamera() {
        Camera2D* camera = new Camera2D();
        Entity entity = cameraWorld.spawn(camera);
        objectMap[entity] = RegisteredObject(RegisteredObject::CAMERA, camera, entity);
        return entity;
    }

    void LuaEngine::cameraSetPosition(Entity entity, const Vector2 &position) {
        Camera2D* camera = cameraWorld.getEntity(entity);
        if (camera) camera->setPosition(position);
    }

    Vector2 LuaEngine::cameraGetPosition(Entity entity) {
        Camera2D* camera = cameraWorld.getEntity(entity);
        return camera ? camera->getPosition() : Vector2(0.0f, 0.0f);
    }

    void LuaEngine::cameraSetZoom(Entity entity, float zoom) {
        Camera2D* camera = cameraWorld.getEntity(entity);
        if (camera) camera->setZoom(zoom);
    }

    float LuaEngine::cameraGetZoom(Entity entity) {
        Camera2D* camera = cameraWorld.getEntity(entity);
        return camera ? camera->getZoom() : 1.0f;
    }

    void LuaEngine::cameraSetRotation(Entity entity, float rotation) {
        Camera2D* camera = cameraWorld.getEntity(entity);
        if (camera) camera->setRotation(rotation);
    }

    float LuaEngine::cameraGetRotation(Entity entity) {
        Camera2D* camera = cameraWorld.getEntity(entity);
        return camera ? camera->getRotation() : 0.0f;
    }

    Camera2D* LuaEngine::getActiveCamera() {
        for (const auto& pair : cameraWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                return pair.first;
            }
        }
        return nullptr;
    }

    // Particle system implementations
    Entity LuaEngine::createParticleSystem() {
        ParticleSystem2D* ps = new ParticleSystem2D();
        Entity entity = particleWorld.spawn(ps);
        objectMap[entity] = RegisteredObject(RegisteredObject::PARTICLE, ps, entity);
        return entity;
    }

    void LuaEngine::particleSystemSetPosition(Entity entity, const Vector2 &position) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setPosition(position);
    }

    void LuaEngine::particleSystemSetTexture(Entity entity, const std::string &texturePath) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (!ps) return;
        const GL::Texture2D* texture = this->textureManager.loadGLTexture(texturePath);
        if (texture) ps->setTexture(*texture);
    }

    void LuaEngine::particleSystemSetEmissionRate(Entity entity, float rate) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setEmissionRate(rate);
    }

    float LuaEngine::particleSystemGetEmissionRate(Entity entity) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        return ps ? ps->getEmissionRate() : 0.0f;
    }

    void LuaEngine::particleSystemSetLifetime(Entity entity, float min, float max) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setLifetime(min, max);
    }

    void LuaEngine::particleSystemSetSpeed(Entity entity, float min, float max) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setSpeed(min, max);
    }

    void LuaEngine::particleSystemSetSize(Entity entity, float min, float max) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setSize(min, max);
    }

    void LuaEngine::particleSystemSetEndSize(Entity entity, float min, float max) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setEndSize(min, max);
    }

    void LuaEngine::particleSystemSetColor(Entity entity, const Vector4 &start, const Vector4 &end) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setColor(start, end);
    }

    void LuaEngine::particleSystemSetDirection(Entity entity, float minAngle, float maxAngle) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setDirection(minAngle, maxAngle);
    }

    void LuaEngine::particleSystemSetRotationSpeed(Entity entity, float speed) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setRotationSpeed(speed);
    }

    void LuaEngine::particleSystemSetActive(Entity entity, bool active) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setActive(active);
    }

    bool LuaEngine::particleSystemIsActive(Entity entity) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        return ps ? ps->isActive() : false;
    }

    void LuaEngine::particleSystemSetVisible(Entity entity, bool visible) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->setVisible(visible);
    }

    bool LuaEngine::particleSystemIsVisible(Entity entity) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        return ps ? ps->isVisible() : false;
    }

    void LuaEngine::particleSystemEmit(Entity entity, int count) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->emit(count);
    }

    void LuaEngine::particleSystemClear(Entity entity) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        if (ps) ps->clear();
    }

    int LuaEngine::particleSystemGetCount(Entity entity) {
        ParticleSystem2D* ps = particleWorld.getEntity(entity);
        return ps ? ps->getParticleCount() : 0;
    }

    void LuaEngine::updateAllParticleSystems(float dt) {
        for (const auto& pair : particleWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                pair.first->update(dt);
            }
        }
    }

    void LuaEngine::drawAllParticleSystems() {
        float aspect = static_cast<float>(getScreenWidth()) / static_cast<float>(getScreenHeight());
        Camera2D* camera = getActiveCamera();
        Matrix3x3 vp = camera ? camera->getViewProjectionMatrix(aspect) : Matrix3x3::identity();

        for (const auto& pair : particleWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                pair.first->draw(this->textureManager.getDefault2D(), this->quadVertexBufferObject, aspect, vp);
            }
        }
    }

    // Tileset implementations
    Entity LuaEngine::createTileset(const std::string& texturePath, uint32_t tileW, uint32_t tileH, uint32_t atlasCols, uint32_t atlasRows) {
        BlazeBolt::Tileset2D *tileset = new BlazeBolt::Tileset2D(tileW, tileH, atlasCols, atlasRows);
        const GL::Texture2D *texture = this->textureManager.loadGLTexture(texturePath);
        Entity entity = tilesetWorld.spawn(tileset);
        objectMap[entity] = RegisteredObject(RegisteredObject::TILESET, tileset, entity);
        if (texture != nullptr) {
            tileset->setMap({}, this->spriteWorld, *texture);
        }
        return entity;
    }

    void LuaEngine::tilesetSetMap(Entity entity, const std::vector<std::vector<int>>& map) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (!tileset) return;
        const GL::Texture2D *texture = tileset->getBatch().getTexture();
        if (!texture) {
            const GL::Texture2D *defaultTex = &this->textureManager.getDefault2D();
            tileset->setMap(map, this->spriteWorld, *defaultTex);
        } else {
            tileset->setMap(map, this->spriteWorld, *texture);
        }
    }

    int LuaEngine::tilesetGetTile(Entity entity, uint32_t col, uint32_t row) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (!tileset) return -1;
        return tileset->getTile(col, row);
    }

    void LuaEngine::tilesetSetTile(Entity entity, uint32_t col, uint32_t row, int tileIndex) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (!tileset) return;
        const GL::Texture2D *texture = tileset->getBatch().getTexture();
        if (!texture) {
            const GL::Texture2D *defaultTex = &this->textureManager.getDefault2D();
            tileset->setTile(col, row, tileIndex, this->spriteWorld, *defaultTex);
        } else {
            tileset->setTile(col, row, tileIndex, this->spriteWorld, *texture);
        }
    }

    void LuaEngine::tilesetSetTileSize(Entity entity, uint32_t w, uint32_t h) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (tileset) tileset->setTileSize(w, h);
    }

    void LuaEngine::tilesetGetTileSize(Entity entity, uint32_t* w, uint32_t* h) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (tileset) tileset->getTileSize(*w, *h);
        else { *w = 0; *h = 0; }
    }

    void LuaEngine::tilesetSetPosition(Entity entity, const Vector2& pos) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (tileset) tileset->setPosition(pos);
    }

    Vector2 LuaEngine::tilesetGetPosition(Entity entity) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        if (tileset) return tileset->getPosition();
        return Vector2(0, 0);
    }

    uint32_t LuaEngine::tilesetGetMapWidth(Entity entity) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        return tileset ? tileset->getMapWidth() : 0;
    }

    uint32_t LuaEngine::tilesetGetMapHeight(Entity entity) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        return tileset ? tileset->getMapHeight() : 0;
    }

    uint32_t LuaEngine::tilesetGetTileCount(Entity entity) {
        BlazeBolt::Tileset2D *tileset = tilesetWorld.getEntity(entity);
        return tileset ? tileset->getTileCount() : 0;
    }

    void LuaEngine::drawAllTilesets() {
        this->spriteBatchShader2D.bind();
        this->spriteBatchShader2D.setAspectRatio(static_cast<float>(getScreenWidth()) / static_cast<float>(getScreenHeight()));
        this->spriteBatchShader2D.setMVPMatrix(this->projectionViewMatrix2D);
        this->uploadLightDataToShader(this->spriteBatchShader2D);
        for (auto& pair : tilesetWorld.getAllEntities()) {
            if (pair.first && !pair.second) {
                pair.first->rebuild(this->spriteWorld);
                pair.first->draw(this->textureManager.getDefault2D());
            }
        }
    }

    void LuaEngine::destroyTileset(Entity entity) {
        auto it = objectMap.find(entity);
        if (it != objectMap.end()) {
            tilesetWorld.destroy(entity);
            objectMap.erase(it);
        }
    }

    // Light implementations
    Entity LuaEngine::createPointLight(float x, float y, float r, float g, float b, float intensity, float radius) {
        BlazeBolt::Light2D *light = new BlazeBolt::Light2D(BlazeBolt::Light2D::POINT);
        light->setPosition(Vector2(x, y));
        light->setColor(Vector3(r, g, b));
        light->setIntensity(intensity);
        light->setRadius(radius);
        Entity entity = lightWorld.spawn(light);
        objectMap[entity] = RegisteredObject(RegisteredObject::LIGHT, light, entity);
        return entity;
    }

    Entity LuaEngine::createAmbientLight(float r, float g, float b, float intensity) {
        BlazeBolt::Light2D *light = new BlazeBolt::Light2D(BlazeBolt::Light2D::AMBIENT);
        light->setColor(Vector3(r, g, b));
        light->setIntensity(intensity);
        Entity entity = lightWorld.spawn(light);
        objectMap[entity] = RegisteredObject(RegisteredObject::LIGHT, light, entity);
        return entity;
    }

    void LuaEngine::lightSetPosition(Entity entity, const Vector2& pos) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        if (light) light->setPosition(pos);
    }

    Vector2 LuaEngine::lightGetPosition(Entity entity) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        return light ? light->getPosition() : Vector2(0, 0);
    }

    void LuaEngine::lightSetColor(Entity entity, const Vector3& color) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        if (light) light->setColor(color);
    }

    Vector3 LuaEngine::lightGetColor(Entity entity) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        return light ? light->getColor() : Vector3(1, 1, 1);
    }

    void LuaEngine::lightSetIntensity(Entity entity, float intensity) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        if (light) light->setIntensity(intensity);
    }

    float LuaEngine::lightGetIntensity(Entity entity) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        return light ? light->getIntensity() : 0;
    }

    void LuaEngine::lightSetRadius(Entity entity, float radius) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        if (light) light->setRadius(radius);
    }

    float LuaEngine::lightGetRadius(Entity entity) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        return light ? light->getRadius() : 0;
    }

    void LuaEngine::lightSetEnabled(Entity entity, bool enabled) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        if (light) light->setEnabled(enabled);
    }

    bool LuaEngine::lightGetEnabled(Entity entity) {
        BlazeBolt::Light2D *light = lightWorld.getEntity(entity);
        return light ? light->isEnabled() : false;
    }

    void LuaEngine::destroyLight(Entity entity) {
        auto it = objectMap.find(entity);
        if (it != objectMap.end()) {
            lightWorld.destroy(entity);
            objectMap.erase(it);
        }
    }

    void LuaEngine::uploadLightDataToShader(const BlazeBolt::SpriteShader2D& shader) const {
        float positions[16];
        float colors[24];
        float intensities[8];
        float radii[8];
        int pointCount = 0;
        float ambientR = 1, ambientG = 1, ambientB = 1, ambientI = 0.3f;

        for (const auto& pair : lightWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::Light2D *light = pair.first;
            if (!light->isEnabled()) continue;

            if (light->getType() == BlazeBolt::Light2D::AMBIENT) {
                ambientR = light->getColor().x;
                ambientG = light->getColor().y;
                ambientB = light->getColor().z;
                ambientI = light->getIntensity();
            } else if (light->getType() == BlazeBolt::Light2D::POINT && pointCount < 8) {
                positions[pointCount * 2 + 0] = light->getPosition().x;
                positions[pointCount * 2 + 1] = light->getPosition().y;
                colors[pointCount * 3 + 0] = light->getColor().x;
                colors[pointCount * 3 + 1] = light->getColor().y;
                colors[pointCount * 3 + 2] = light->getColor().z;
                intensities[pointCount] = light->getIntensity();
                radii[pointCount] = light->getRadius();
                pointCount++;
            }
        }

        shader.setLightData(pointCount, positions, colors, intensities, radii, Vector3(ambientR, ambientG, ambientB), ambientI);
    }

    void LuaEngine::uploadLightDataToShader(const BlazeBolt::SpriteBatchShader2D& shader) const {
        float positions[16];
        float colors[24];
        float intensities[8];
        float radii[8];
        int pointCount = 0;
        float ambientR = 1, ambientG = 1, ambientB = 1, ambientI = 0.3f;

        for (const auto& pair : lightWorld.getAllEntities()) {
            if (!pair.first || pair.second) continue;
            BlazeBolt::Light2D *light = pair.first;
            if (!light->isEnabled()) continue;

            if (light->getType() == BlazeBolt::Light2D::AMBIENT) {
                ambientR = light->getColor().x;
                ambientG = light->getColor().y;
                ambientB = light->getColor().z;
                ambientI = light->getIntensity();
            } else if (light->getType() == BlazeBolt::Light2D::POINT && pointCount < 8) {
                positions[pointCount * 2 + 0] = light->getPosition().x;
                positions[pointCount * 2 + 1] = light->getPosition().y;
                colors[pointCount * 3 + 0] = light->getColor().x;
                colors[pointCount * 3 + 1] = light->getColor().y;
                colors[pointCount * 3 + 2] = light->getColor().z;
                intensities[pointCount] = light->getIntensity();
                radii[pointCount] = light->getRadius();
                pointCount++;
            }
        }

        shader.setLightData(pointCount, positions, colors, intensities, radii, Vector3(ambientR, ambientG, ambientB), ambientI);
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
        Camera2D* camera = getActiveCamera();
        float aspect = static_cast<float>(getScreenWidth()) / static_cast<float>(getScreenHeight());
        if (camera) {
            this->projectionViewMatrix2D = camera->getViewProjectionMatrix(aspect);
        } else {
            this->projectionViewMatrix2D = Matrix3x3::identity();
        }
        if (renderContext) renderContext->beginFrame();
        for (const auto& layer : renderOrder) {
            if (layer == "Tilesets") this->drawAllTilesets();
            else if (layer == "Sprites") this->drawAllSprites();
            else if (layer == "AnimatedSprites") this->drawAllAnimatedSprites();
            else if (layer == "Texts") this->drawAllTexts();
            else if (layer == "Meshes") this->drawAllMeshes();
            else if (layer == "Particles") this->drawAllParticleSystems();
        }
        if (renderContext) renderContext->endFrame();
    }

    void LuaEngine::updateAll(float deltaTime) {
        this->deltaTime = deltaTime;
        this->updateAllAnimatedSprites(deltaTime);
        this->updateAnimationWheels(deltaTime);
        this->updateAllParticleSystems(deltaTime);
        this->updateAudio();
        if (sceneManager != nullptr) { sceneManager->update(deltaTime); }
    }

    void LuaEngine::setRenderOrder(const std::vector<std::string>& order) {
        renderOrder = order;
    }

    const std::vector<std::string>& LuaEngine::getRenderOrder() const {
        return renderOrder;
    }
}
