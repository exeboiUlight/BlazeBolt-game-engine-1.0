#pragma once
#include <cstdio>
#include <vector>
#include <utility>
#include <cstdint>

typedef uint32_t Entity;

template<typename T>
class World {
private:
    std::vector<std::pair<T*, bool>> entities;
    std::vector<Entity> freeEntities;
    
public:
    World() {}
    
    ~World() {
        clear();
    }
    
    // Запрещаем копирование
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    
    // Spawn принимает указатель
    Entity spawn(T* object) {
        if (!object) return 0;
        
        if (!freeEntities.empty()) {
            Entity entity = freeEntities.back();
            entities[entity - 1].first = object;
            entities[entity - 1].second = false;
            freeEntities.pop_back();
            return entity;
        }
        
        entities.emplace_back(object, false);
        return entities.size();
    }
    
    void destroy(Entity entity) {
        if (entity == 0 || entity > entities.size()) return;
        if (entities[entity - 1].first) {
            delete entities[entity - 1].first;
            entities[entity - 1].first = nullptr;
        }
        entities[entity - 1].second = true;
        freeEntities.push_back(entity);
    }
    
    T* getEntity(Entity entity) {
        if (entity == 0 || entity > entities.size()) return nullptr;
        if (entities[entity - 1].second) return nullptr;
        return entities[entity - 1].first;
    }
    
    const std::vector<std::pair<T*, bool>>& getAllEntities() const {
        return entities;
    }
    
    void clear() {
        for (auto& pair : entities) {
            if (pair.first) {
                delete pair.first;
                pair.first = nullptr;
            }
        }
        entities.clear();
        freeEntities.clear();
    }
};