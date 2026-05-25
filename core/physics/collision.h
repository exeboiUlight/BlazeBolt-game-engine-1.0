#pragma once
#include "types.h"
#include "body.h"
#include <utils/math/vector.h>

struct CollisionInfo {
    bool collided;
    Vector2 normal;
    float penetration;
    Vector2 contactPoint;
};

bool testCollision(PhysicsBody* bodyA, PhysicsBody* bodyB, CollisionInfo& info);
void resolveCollision(PhysicsBody* bodyA, PhysicsBody* bodyB, const CollisionInfo& info);
