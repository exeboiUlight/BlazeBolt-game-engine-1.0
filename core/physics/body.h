#pragma once
#include "types.h"
#include <utils/math/vector.h>
#include <vector>
#include <cmath>
#include <algorithm>

class PhysicsBody {
private:
    PhysicsBodyType type;
    Vector2 position;
    float angle;
    Vector2 linearVelocity;
    float angularVelocity;
    float mass;
    float inverseMass;
    float inverseInertia;
    float friction;
    float restitution;
    float gravityScale;
    bool fixedRotation;
    bool bullet;
    bool active;
    bool sleeping;
    float sleepTimer;

    std::vector<ColliderCircle> circles;
    std::vector<ColliderRectangle> rectangles;

    void recomputeInertia();

public:
    PhysicsBody(PhysicsBodyType type, float x, float y, float mass, float friction, float restitution);

    void addCircle(float radius, float offsetX, float offsetY);
    void addRectangle(float halfWidth, float halfHeight);

    void applyForce(float fx, float fy);
    void applyForceAtPoint(float fx, float fy, float px, float py);
    void applyImpulse(float ix, float iy);
    void applyImpulseAtPoint(float ix, float iy, float px, float py);
    void applyTorque(float torque);

    void step(float dt, float gravityX, float gravityY);

    PhysicsBodyType getType() const { return type; }
    void setType(PhysicsBodyType t);

    Vector2 getPosition() const { return position; }
    void setPosition(float x, float y) { position = Vector2(x, y); }
    void setPosition(const Vector2& p) { position = p; }

    float getAngle() const { return angle; }
    void setAngle(float a) { angle = a; }

    Vector2 getLinearVelocity() const { return linearVelocity; }
    void setLinearVelocity(float vx, float vy) { linearVelocity = Vector2(vx, vy); }
    void setLinearVelocity(const Vector2& v) { linearVelocity = v; }

    float getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(float av) { angularVelocity = av; }

    float getMass() const { return mass; }
    void setMass(float m);

    float getInverseMass() const { return inverseMass; }
    float getInverseInertia() const { return inverseInertia; }

    float getFriction() const { return friction; }
    void setFriction(float f) { friction = f; }

    float getRestitution() const { return restitution; }
    void setRestitution(float r) { restitution = r; }

    float getGravityScale() const { return gravityScale; }
    void setGravityScale(float s) { gravityScale = s; }

    bool isFixedRotation() const { return fixedRotation; }
    void setFixedRotation(bool f) { fixedRotation = f; }

    bool isBullet() const { return bullet; }
    void setBullet(bool b) { bullet = b; }

    bool isActive() const { return active; }
    void setActive(bool a) { active = a; sleeping = false; sleepTimer = 0; }

    bool isSleeping() const { return sleeping; }
    void wakeUp() { sleeping = false; sleepTimer = 0; }

    const std::vector<ColliderCircle>& getCircles() const { return circles; }
    const std::vector<ColliderRectangle>& getRectangles() const { return rectangles; }
};
