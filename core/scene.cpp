#include "scene.h"
#include <algorithm>

namespace BlazeBolt {

Scene::Scene() : nextId(1) {
    sceneName = "Untitled";
}

Scene::Scene(const std::string& path) : nextId(1) {
    load(path);
}

Scene::~Scene() {
    objects.clear();
    freeIds.clear();
}

void Scene::writeBE32(FILE* file, uint32_t value) const {
    uint8_t bytes[4];
    bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[3] = static_cast<uint8_t>(value & 0xFF);
    fwrite(bytes, 1, 4, file);
}

void Scene::writeBE32f(FILE* file, float value) const {
    union { float f; uint32_t u; } conv;
    conv.f = value;
    writeBE32(file, conv.u);
}

void Scene::writeString(FILE* file, const std::string& str) const {
    uint32_t len = static_cast<uint32_t>(str.size());
    writeBE32(file, len);
    if (len > 0) {
        fwrite(str.c_str(), 1, len, file);
    }
}

uint32_t Scene::readBE32(FILE* file) {
    uint8_t bytes[4];
    fread(bytes, 1, 4, file);
    return static_cast<uint32_t>((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}

float Scene::readBE32f(FILE* file) {
    uint8_t bytes[4];
    fread(bytes, 1, 4, file);
    uint32_t v = static_cast<uint32_t>((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
    union { uint32_t u; float f; } conv;
    conv.u = v;
    return conv.f;
}

std::string Scene::readString(FILE* file) {
    uint32_t len = readBE32(file);
    if (len > 0) {
        std::vector<char> buf(len);
        fread(buf.data(), 1, len, file);
        return std::string(buf.data(), len);
    }
    return "";
}

bool Scene::load(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return false;
    
    uint32_t magic = readBE32(file);
    if (magic != MAGIC) { fclose(file); return false; }
    
    readBE32(file);
    
    sceneName = readString(file);
    uint32_t count = readBE32(file);
    
    objects.clear();
    for (uint32_t i = 0; i < count; ++i) {
        SceneObject obj;
        obj.id = readBE32(file);
        obj.type = static_cast<SceneObjectType>(fgetc(file));
        obj.name = readString(file);
        obj.parentId = readBE32(file);
        
        obj.position[0] = readBE32f(file);
        obj.position[1] = readBE32f(file);
        obj.position[2] = readBE32f(file);
        
        obj.rotation[0] = readBE32f(file);
        obj.rotation[1] = readBE32f(file);
        obj.rotation[2] = readBE32f(file);
        
        obj.scale[0] = readBE32f(file);
        obj.scale[1] = readBE32f(file);
        obj.scale[2] = readBE32f(file);
        
        obj.active = (fgetc(file) == 1);
        
        objects.push_back(obj);
        nextId = std::max(nextId, obj.id + 1);
    }
    
    fclose(file);
    return true;
}

bool Scene::save(const std::string& path) const {
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) return false;
    
    writeBE32(file, MAGIC);
    writeBE32(file, VERSION);
    writeString(file, sceneName);
    writeBE32(file, static_cast<uint32_t>(objects.size()));
    
    for (const auto& obj : objects) {
        writeBE32(file, obj.id);
        fputc(static_cast<uint8_t>(obj.type), file);
        writeString(file, obj.name);
        writeBE32(file, obj.parentId);
        
        writeBE32f(file, obj.position[0]);
        writeBE32f(file, obj.position[1]);
        writeBE32f(file, obj.position[2]);
        
        writeBE32f(file, obj.rotation[0]);
        writeBE32f(file, obj.rotation[1]);
        writeBE32f(file, obj.rotation[2]);
        
        writeBE32f(file, obj.scale[0]);
        writeBE32f(file, obj.scale[1]);
        writeBE32f(file, obj.scale[2]);
        
        fputc(obj.active ? 1 : 0, file);
    }
    
    fclose(file);
    return true;
}

uint32_t Scene::createObject(const std::string& name, SceneObjectType type) {
    uint32_t id = freeIds.empty() ? nextId++ : freeIds.back();
    if (!freeIds.empty()) freeIds.pop_back();
    
    SceneObject obj;
    obj.id = id;
    obj.name = name;
    obj.type = type;
    objects.push_back(obj);
    return id;
}

void Scene::destroyObject(uint32_t id) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->id == id) {
            objects.erase(it);
            freeIds.push_back(id);
            break;
        }
    }
}

SceneObject* Scene::getObject(uint32_t id) {
    for (auto& obj : objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

std::vector<SceneObject*> Scene::getRootObjects() {
    std::vector<SceneObject*> roots;
    for (auto& obj : objects) {
        if (obj.parentId == 0) {
            roots.push_back(&obj);
        }
    }
    return roots;
}

}