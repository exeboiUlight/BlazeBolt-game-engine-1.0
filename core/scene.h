#pragma once

#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace BlazeBolt {

enum class SceneObjectType : uint8_t {
    GameObject = 0,
    Transform = 1,
    SpriteRenderer = 2,
    Camera = 3,
    AudioSource = 4,
    Text = 5,
    RigidBody = 6,
    BoxCollider = 7,
    Light = 10
};

struct SceneObject {
    uint32_t id;
    SceneObjectType type;
    std::string name;
    uint32_t parentId;
    float position[3];
    float rotation[3];
    float scale[3];
    bool active;
    
    SceneObject() : id(0), type(SceneObjectType::GameObject), parentId(0), active(true) {
        position[0] = position[1] = position[2] = 0.0f;
        rotation[0] = rotation[1] = rotation[2] = 0.0f;
        scale[0] = scale[1] = scale[2] = 1.0f;
    }
};

class Scene {
public:
    static const uint32_t MAGIC = 0x424C415A;
    static const uint16_t VERSION = 1;
    
    Scene();
    explicit Scene(const std::string& path);
    ~Scene();
    
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    
    uint32_t createObject(const std::string& name, SceneObjectType type);
    void destroyObject(uint32_t id);
    SceneObject* getObject(uint32_t id);
    const std::vector<SceneObject>& getAllObjects() const { return objects; }
    const std::string& getName() const { return sceneName; }
    void setName(const std::string& name) { sceneName = name; }
    std::vector<SceneObject*> getRootObjects();
    
private:
    void writeBE32(FILE* file, uint32_t value) const;
    void writeBE32f(FILE* file, float value) const;
    void writeString(FILE* file, const std::string& str) const;
    uint32_t readBE32(FILE* file);
    float readBE32f(FILE* file);
    std::string readString(FILE* file);
    
    std::string sceneName;
    std::vector<SceneObject> objects;
    std::vector<uint32_t> freeIds;
    uint32_t nextId;
};

}