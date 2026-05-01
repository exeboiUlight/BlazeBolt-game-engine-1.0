#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <utils/math/vector.h>

enum class PhysicsBodyType {
    Static = 0,
    Dynamic = 1,
    Kinematic = 2
};

struct ColliderCircle {
    float radius;
    float offsetX, offsetY;
};

struct ColliderRectangle {
    float halfWidth, halfHeight;
};

class PhysicsBody {
private:
    PhysicsBodyType type;
    Vector2 position;
    float angle;
    Vector2 linearVelocity;
    float angularVelocity;
    float mass;
    float inverseMass;
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
    
public:
    PhysicsBody(PhysicsBodyType type, float x, float y, float mass, float friction, float restitution)
        : type(type), position(x, y), angle(0), linearVelocity(0, 0), angularVelocity(0),
          mass(mass), friction(friction), restitution(restitution), gravityScale(1.0f),
          fixedRotation(false), bullet(false), active(true), sleeping(false), sleepTimer(0) {
        inverseMass = (type == PhysicsBodyType::Static) ? 0 : (mass > 0 ? 1.0f / mass : 0);
    }
    
    void addCircle(float radius, float offsetX, float offsetY) {
        circles.push_back({radius, offsetX, offsetY});
    }
    
    void addRectangle(float halfWidth, float halfHeight) {
        rectangles.push_back({halfWidth, halfHeight});
    }
    
    void applyForce(float fx, float fy) {
        if (type != PhysicsBodyType::Dynamic || inverseMass == 0) return;
        linearVelocity.x += fx * inverseMass;
        linearVelocity.y += fy * inverseMass;
    }
    
    void applyForceAtPoint(float fx, float fy, float px, float py) {
        applyForce(fx, fy);
        if (!fixedRotation && type == PhysicsBodyType::Dynamic) {
            float torque = (px - position.x) * fy - (py - position.y) * fx;
            angularVelocity += torque * inverseMass;
        }
    }
    
    void applyImpulse(float ix, float iy) {
        if (type != PhysicsBodyType::Dynamic || inverseMass == 0) return;
        linearVelocity.x += ix * inverseMass;
        linearVelocity.y += iy * inverseMass;
    }
    
    void applyImpulseAtPoint(float ix, float iy, float px, float py) {
        applyImpulse(ix, iy);
        if (!fixedRotation && type == PhysicsBodyType::Dynamic) {
            float torque = (px - position.x) * iy - (py - position.y) * ix;
            angularVelocity += torque * inverseMass;
        }
    }
    
    void applyTorque(float torque) {
        if (fixedRotation || type != PhysicsBodyType::Dynamic || inverseMass == 0) return;
        angularVelocity += torque * inverseMass;
    }
    
    void step(float dt, float gravityX, float gravityY) {
        if (!active || type == PhysicsBodyType::Static) return;
        
        if (type == PhysicsBodyType::Dynamic) {
            linearVelocity.x += gravityX * gravityScale * dt;
            linearVelocity.y += gravityY * gravityScale * dt;
        }
        
        float maxVel = 100.0f;
        linearVelocity.x = std::clamp(linearVelocity.x, -maxVel, maxVel);
        linearVelocity.y = std::clamp(linearVelocity.y, -maxVel, maxVel);
        
        position.x += linearVelocity.x * dt;
        position.y += linearVelocity.y * dt;
        angle += angularVelocity * dt;
        
        float damping = 0.999f;
        linearVelocity *= damping;
        angularVelocity *= 0.998f;
        
        float velThreshold = 0.01f;
        if (std::abs(linearVelocity.x) < velThreshold && std::abs(linearVelocity.y) < velThreshold) {
            sleepTimer += dt;
            if (sleepTimer > 1.0f) sleeping = true;
        } else {
            sleepTimer = 0;
            sleeping = false;
        }
    }
    
    bool overlaps(const PhysicsBody* other) const {
        for (const auto& c1 : circles) {
            Vector2 p1(position.x + c1.offsetX, position.y + c1.offsetY);
            for (const auto& c2 : other->circles) {
                Vector2 p2(other->position.x + c2.offsetX, other->position.y + c2.offsetY);
                float dist = (p1 - p2).length();
                if (dist < c1.radius + c2.radius) return true;
            }
            for (const auto& r2 : other->rectangles) {
                if (circleRectOverlap(p1, c1.radius, other->position, r2)) return true;
            }
        }
        for (const auto& r1 : rectangles) {
            Vector2 p1 = position;
            for (const auto& c2 : other->circles) {
                Vector2 p2(other->position.x + c2.offsetX, other->position.y + c2.offsetY);
                if (circleRectOverlap(p2, c2.radius, p1, r1)) return true;
            }
            for (const auto& r2 : other->rectangles) {
                if (rectRectOverlap(position, r1, other->position, r2)) return true;
            }
        }
        return false;
    }
    
    bool circleRectOverlap(const Vector2& circlePos, float radius, const Vector2& rectPos, const ColliderRectangle& rect) const {
        float closestX = std::clamp(circlePos.x, rectPos.x - rect.halfWidth, rectPos.x + rect.halfWidth);
        float closestY = std::clamp(circlePos.y, rectPos.y - rect.halfHeight, rectPos.y + rect.halfHeight);
        float dx = circlePos.x - closestX;
        float dy = circlePos.y - closestY;
        return (dx * dx + dy * dy) < (radius * radius);
    }
    
    bool rectRectOverlap(const Vector2& p1, const ColliderRectangle& r1, const Vector2& p2, const ColliderRectangle& r2) const {
        return std::abs(p1.x - p2.x) < (r1.halfWidth + r2.halfWidth) &&
               std::abs(p1.y - p2.y) < (r1.halfHeight + r2.halfHeight);
    }
    
    void resolveCollision(PhysicsBody* other) {
        if (type == PhysicsBodyType::Static && other->type == PhysicsBodyType::Static) return;
        
        Vector2 diff = position - other->position;
        float dist = diff.length();
        if (dist < 0.0001f) { diff = Vector2(0.001f, 0); dist = 0.001f; }
        Vector2 normal = diff / dist;
        
        float overlap = 0.1f;
        float totalInverseMass = inverseMass + other->inverseMass;
        if (totalInverseMass == 0) return;
        
        float correctionMag = overlap / totalInverseMass;
        Vector2 correction = normal * correctionMag * 0.8f;
        if (inverseMass > 0) position += correction * inverseMass;
        if (other->inverseMass > 0) other->position -= correction * other->inverseMass;
        
        Vector2 relVel = linearVelocity - other->linearVelocity;
        float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;
        if (velAlongNormal > 0) return;
        
        float e = std::min(restitution, other->restitution);
        float j = -(1 + e) * velAlongNormal;
        j /= totalInverseMass;
        
        Vector2 impulse = normal * j;
        if (type == PhysicsBodyType::Dynamic) linearVelocity += impulse * inverseMass;
        if (other->type == PhysicsBodyType::Dynamic) other->linearVelocity -= impulse * other->inverseMass;
        
        float fric = std::sqrt(friction * other->friction);
        Vector2 tangent = relVel - normal * velAlongNormal;
        float tanLen = tangent.length();
        if (tanLen > 0.0001f) tangent /= tanLen;
        
        float jt = -(relVel.x * tangent.x + relVel.y * tangent.y);
        jt /= totalInverseMass;
        
        Vector2 frictionImpulse;
        if (std::abs(jt) < j * fric) {
            frictionImpulse = tangent * jt;
        } else {
            frictionImpulse = tangent * (-j * fric);
        }
        
        if (type == PhysicsBodyType::Dynamic) linearVelocity += frictionImpulse * inverseMass;
        if (other->type == PhysicsBodyType::Dynamic) other->linearVelocity -= frictionImpulse * other->inverseMass;
    }
    
    PhysicsBodyType getType() const { return type; }
    void setType(PhysicsBodyType t) { type = t; inverseMass = (t == PhysicsBodyType::Static) ? 0 : (mass > 0 ? 1.0f / mass : 0); }
    Vector2 getPosition() const { return position; }
    void setPosition(float x, float y) { position = Vector2(x, y); }
    float getAngle() const { return angle; }
    void setAngle(float a) { angle = a; }
    Vector2 getLinearVelocity() const { return linearVelocity; }
    void setLinearVelocity(float vx, float vy) { linearVelocity = Vector2(vx, vy); }
    float getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(float av) { angularVelocity = av; }
    float getMass() const { return mass; }
    void setMass(float m) { mass = m; inverseMass = (type == PhysicsBodyType::Static) ? 0 : (m > 0 ? 1.0f / m : 0); }
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

class PhysicsWorld {
private:
    Vector2 gravity;
    std::vector<PhysicsBody*> bodies;
    
public:
    PhysicsWorld() : gravity(0, -9.8f) {}
    
    void setGravity(float x, float y) { gravity = Vector2(x, y); }
    void getGravity(float* x, float* y) const { *x = gravity.x; *y = gravity.y; }
    
    PhysicsBody* createBody(PhysicsBodyType type, float x, float y, float mass, float friction, float restitution) {
        PhysicsBody* body = new PhysicsBody(type, x, y, mass, friction, restitution);
        bodies.push_back(body);
        return body;
    }
    
    void destroyBody(PhysicsBody* body) {
        bodies.erase(std::remove(bodies.begin(), bodies.end(), body), bodies.end());
        delete body;
    }
    
    void step(float dt) {
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
                if (bodies[i]->overlaps(bodies[j])) {
                    bodies[i]->resolveCollision(bodies[j]);
                }
            }
        }
    }
    
    void clear() {
        for (auto& body : bodies) delete body;
        bodies.clear();
    }
    
    const std::vector<PhysicsBody*>& getBodies() const { return bodies; }
};
