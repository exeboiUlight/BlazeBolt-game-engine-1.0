#pragma once
#include <cstdio>
#include <vector>
#include <utility>
#include <cstdint>

typedef uint32_t Entity;

template<typename T>
class World {
private:
    std::vector<std::pair<T, bool>> entities = std::vector<std::pair<T, bool>>();
    std::vector<Entity> freeEntities = std::vector<Entity>();
public:
    World() {}
    ~World() {}

    // template<typename T>
    static World<T> &getInstance() {
        static World<T> world;
        return world;
    }
    
    Entity spawn(const T &object) {
        if (!this->freeEntities.empty()) {
            Entity entity = this->freeEntities[this->freeEntities.size() - 1];
            this->entities[entity - 1].first = object;
            this->entities[entity - 1].second = false;
            this->freeEntities.pop_back();
            
            return entity;
        }
        
        this->entities.emplace_back(object, false);
        return this->entities.size();
    }
    void destroy(Entity &entity) {
        if (entity == 0 || this->entities.size() + 1 <= entity) return;
        this->entities[entity - 1].second = true;
        this->freeEntities.push_back(entity);
        entity = 0;
    }
    
    T *getEntity(Entity entity) {
        if (entity == 0 || this->entities.size() + 1 <= entity) return nullptr;
        std::pair<T, bool> &sprite = this->entities[entity - 1];
        if (sprite.second) return nullptr;
        
        return &sprite.first;
    }

    const std::vector<std::pair<T, bool>> &getAllEntities() const {
        return this->entities;
    }
};