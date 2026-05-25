#include "world.h"
#include "collision.h"

PhysicsWorld::PhysicsWorld() : gravity(0, -9.8f), accumulator(0) {}

PhysicsBody* PhysicsWorld::createBody(PhysicsBodyType type, float x, float y, float mass, float friction, float restitution) {
    PhysicsBody* body = new PhysicsBody(type, x, y, mass, friction, restitution);
    bodies.push_back(body);
    return body;
}

void PhysicsWorld::destroyBody(PhysicsBody* body) {
    bodies.erase(std::remove(bodies.begin(), bodies.end(), body), bodies.end());
    delete body;
}

void PhysicsWorld::step(float dt) {
    accumulator += dt;
    if (accumulator > MAX_FRAME_TIME) accumulator = MAX_FRAME_TIME;

    while (accumulator >= FIXED_DT) {
        fixedStep(FIXED_DT);
        accumulator -= FIXED_DT;
    }
}

void PhysicsWorld::fixedStep(float dt) {
    for (auto& body : bodies) {
        if (body->isActive() && !body->isSleeping()) {
            body->step(dt, gravity.x, gravity.y);
        }
    }

    for (size_t i = 0; i < bodies.size(); i++) {
        if (!bodies[i]->isActive() || bodies[i]->getType() == PhysicsBodyType::Static) continue;
        for (size_t j = i + 1; j < bodies.size(); j++) {
            if (!bodies[j]->isActive()) continue;
            if (bodies[i]->getType() == PhysicsBodyType::Static && bodies[j]->getType() == PhysicsBodyType::Static) continue;

            CollisionInfo info;
            if (testCollision(bodies[i], bodies[j], info)) {
                resolveCollision(bodies[i], bodies[j], info);
            }
        }
    }
}

void PhysicsWorld::clear() {
    for (auto& body : bodies) delete body;
    bodies.clear();
    accumulator = 0;
}
