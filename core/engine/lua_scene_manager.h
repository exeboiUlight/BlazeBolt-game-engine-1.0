#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include <engine/lua_types.h>

namespace LuaEngine {

    class LuaEngine;

    // Scene manager
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
}
