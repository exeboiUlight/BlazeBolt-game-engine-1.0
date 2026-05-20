#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <world.h>

namespace LuaEngine {

    // Forward declaration
    class LuaEngine;

    // Helper structure to track registered objects
    struct RegisteredObject {
        enum Type { SPRITE, ANIMATION, TEXT, MESH, WINDOW, UNKNOWN } type;
        void* ptr;
        Entity entity;
        std::shared_ptr<Window> windowPtr;
        unsigned int shaderID;

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
}
