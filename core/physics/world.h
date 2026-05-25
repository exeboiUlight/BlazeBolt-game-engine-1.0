#pragma once
#include "body.h"
#include <vector>
#include <algorithm>

class PhysicsWorld {
private:
    Vector2 gravity;
    std::vector<PhysicsBody*> bodies;

    static constexpr float FIXED_DT = 1.0f / 120.0f;
    static constexpr float MAX_FRAME_TIME = 0.05f;
    float accumulator;

    void fixedStep(float dt);

public:
    PhysicsWorld();

    void setGravity(float x, float y) { gravity = Vector2(x, y); }
    void getGravity(float* x, float* y) const { *x = gravity.x; *y = gravity.y; }

    PhysicsBody* createBody(PhysicsBodyType type, float x, float y, float mass, float friction, float restitution);
    void destroyBody(PhysicsBody* body);

    void step(float dt);
    void clear();

    const std::vector<PhysicsBody*>& getBodies() const { return bodies; }
};
