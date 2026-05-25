#include <engine/luaFunctions.hpp>
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

        // Animation functions
        {"CreateAnimatedSprite", _functions::CreateAnimatedSprite},
        {"AnimatedSpritePlay", _functions::AnimatedSpritePlay},
        {"AnimatedSpriteIsPlaying", _functions::AnimatedSpriteIsPlaying},
        {"AnimatedSpritePause", _functions::AnimatedSpritePause},
        {"AnimatedSpriteStop", _functions::AnimatedSpriteStop},
        {"AnimatedSpriteRestart", _functions::AnimatedSpriteRestart},
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

        {nullptr, nullptr}
    };

    // ==================== IMPLEMENTATION ====================
    LuaEngine::LuaEngine(Window &window) :
        state(nullptr),
        spriteWorld(), animatedSpriteWorld(), textWorld(), meshWorld(), cameraWorld(), particleWorld(),
        audioEngine(), physicsWorld(),
        deltaTime(0.0f),
        initialized(false), audioInitialized(false),
        objectMap(), soundNameToId(), physicsBodyMap(),
        scripts(), scriptsListPath(), projectFileName(),
        sceneManager(),
        additionalWindows(), mainWindow(&window),
        projectionViewMatrix2D(), quadVertexBufferObject(),
        spriteShader2D(), fontShader2D(), spriteMesh(),
        textureManager(), fontManager(),
        shaders(), entityShaderMap(), nextShaderId(1)
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
                pair.first->draw(this->textureManager.getDefault2D(), this->spriteShader2D, this->spriteMesh, this->projectionViewMatrix2D);
            }
        }
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
                auto shaderIt = entityShaderMap.find(pair.second);
                if (shaderIt != entityShaderMap.end()) {
                    Shader* customShader = getShader(shaderIt->second);
                    if (customShader) {
                        customShader->use();
                    }
                }
                pair.first->draw(this->spriteShader2D, this->spriteMesh, this->projectionViewMatrix2D);
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
                default: break;
            }
            objectMap.erase(it);
        }
        // Clean up shader mapping
        entityShaderMap.erase(entity);
    }

    void LuaEngine::destroyAllEntities() {
        spriteWorld.clear();
        animatedSpriteWorld.clear();
        textWorld.clear();
        meshWorld.clear();
        cameraWorld.clear();
        particleWorld.clear();
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
        const GL::Texture2D* texture = this->textureManager.loadFromFile2D(texturePath);
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
                pair.first->draw(this->textureManager.getDefault2D(), this->spriteShader2D, this->spriteMesh, vp);
            }
        }
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
        this->drawAllSprites();
        this->drawAllAnimatedSprites();
        this->drawAllTexts();
        this->drawAllMeshes();
        this->drawAllParticleSystems();
    }

    void LuaEngine::updateAll(float deltaTime) {
        this->deltaTime = deltaTime;
        this->updateAllAnimatedSprites(deltaTime);
        this->updateAllParticleSystems(deltaTime);
        this->updateAudio();
        if (sceneManager != nullptr) { sceneManager->update(deltaTime); }
    }
}
