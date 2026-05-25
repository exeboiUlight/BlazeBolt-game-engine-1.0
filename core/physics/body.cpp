#include "body.h"

PhysicsBody::PhysicsBody(PhysicsBodyType type, float x, float y, float mass, float friction, float restitution)
    : type(type), position(x, y), angle(0), linearVelocity(0, 0), angularVelocity(0),
      mass(mass), friction(friction), restitution(restitution), gravityScale(1.0f),
      fixedRotation(false), bullet(false), active(true), sleeping(false), sleepTimer(0),
      inverseMass(0), inverseInertia(0) {
    if (type != PhysicsBodyType::Static && mass > 0) {
        inverseMass = 1.0f / mass;
    }
    recomputeInertia();
}

void PhysicsBody::recomputeInertia() {
    if (type == PhysicsBodyType::Static || mass <= 0 || fixedRotation) {
        inverseInertia = 0;
        return;
    }

    float shapeCount = (float)(circles.size() + rectangles.size());
    if (shapeCount == 0) {
        inverseInertia = 1.0f / (mass * 100.0f);
        return;
    }

    float totalInertia = 0;
    float massPerShape = mass / shapeCount;

    for (const auto& c : circles) {
        float d2 = c.offsetX * c.offsetX + c.offsetY * c.offsetY;
        totalInertia += massPerShape * (0.5f * c.radius * c.radius + d2);
    }

    for (const auto& r : rectangles) {
        float w = r.halfWidth * 2.0f;
        float h = r.halfHeight * 2.0f;
        totalInertia += massPerShape * (w * w + h * h) / 12.0f;
    }

    inverseInertia = totalInertia > 0.0001f ? 1.0f / totalInertia : 1.0f / (mass * 100.0f);
}

void PhysicsBody::addCircle(float radius, float offsetX, float offsetY) {
    circles.push_back({radius, offsetX, offsetY});
    recomputeInertia();
}

void PhysicsBody::addRectangle(float halfWidth, float halfHeight) {
    rectangles.push_back({halfWidth, halfHeight});
    recomputeInertia();
}

void PhysicsBody::applyForce(float fx, float fy) {
    if (type != PhysicsBodyType::Dynamic || inverseMass == 0) return;
    linearVelocity.x += fx * inverseMass;
    linearVelocity.y += fy * inverseMass;
}

void PhysicsBody::applyForceAtPoint(float fx, float fy, float px, float py) {
    applyForce(fx, fy);
    if (!fixedRotation && type == PhysicsBodyType::Dynamic && inverseInertia > 0) {
        float torque = (px - position.x) * fy - (py - position.y) * fx;
        angularVelocity += torque * inverseInertia;
    }
}

void PhysicsBody::applyImpulse(float ix, float iy) {
    if (type != PhysicsBodyType::Dynamic || inverseMass == 0) return;
    linearVelocity.x += ix * inverseMass;
    linearVelocity.y += iy * inverseMass;
}

void PhysicsBody::applyImpulseAtPoint(float ix, float iy, float px, float py) {
    applyImpulse(ix, iy);
    if (!fixedRotation && type == PhysicsBodyType::Dynamic && inverseInertia > 0) {
        float torque = (px - position.x) * iy - (py - position.y) * ix;
        angularVelocity += torque * inverseInertia;
    }
}

void PhysicsBody::applyTorque(float torque) {
    if (fixedRotation || type != PhysicsBodyType::Dynamic || inverseInertia == 0) return;
    angularVelocity += torque * inverseInertia;
}

void PhysicsBody::step(float dt, float gravityX, float gravityY) {
    if (!active || type == PhysicsBodyType::Static) return;

    if (type == PhysicsBodyType::Dynamic) {
        linearVelocity.x += gravityX * gravityScale * dt;
        linearVelocity.y += gravityY * gravityScale * dt;
    }

    float maxVel = 100.0f;
    linearVelocity.x = std::clamp(linearVelocity.x, -maxVel, maxVel);
    linearVelocity.y = std::clamp(linearVelocity.y, -maxVel, maxVel);

    float maxAngVel = 20.0f;
    angularVelocity = std::clamp(angularVelocity, -maxAngVel, maxAngVel);

    position.x += linearVelocity.x * dt;
    position.y += linearVelocity.y * dt;

    if (!fixedRotation) {
        angle += angularVelocity * dt;
    }

    float damping = 0.999f;
    linearVelocity *= damping;
    if (!fixedRotation) {
        angularVelocity *= 0.998f;
    }

    float velThreshold = 0.01f;
    float angVelThreshold = 0.001f;
    if (std::abs(linearVelocity.x) < velThreshold &&
        std::abs(linearVelocity.y) < velThreshold &&
        std::abs(angularVelocity) < angVelThreshold) {
        sleepTimer += dt;
        if (sleepTimer > 1.0f) sleeping = true;
    } else {
        sleepTimer = 0;
        sleeping = false;
    }
}

void PhysicsBody::setType(PhysicsBodyType t) {
    type = t;
    inverseMass = (t == PhysicsBodyType::Static) ? 0 : (mass > 0 ? 1.0f / mass : 0);
    recomputeInertia();
}

void PhysicsBody::setMass(float m) {
    mass = m;
    inverseMass = (type == PhysicsBodyType::Static) ? 0 : (mass > 0 ? 1.0f / mass : 0);
    recomputeInertia();
}
