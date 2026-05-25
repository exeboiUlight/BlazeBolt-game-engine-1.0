#include "collision.h"
#include <algorithm>
#include <cmath>

static void getOBBCorners(const Vector2& center, float angle, float hw, float hh, Vector2 corners[4]) {
    float c = std::cos(angle);
    float s = std::sin(angle);

    Vector2 local[4] = {
        Vector2(-hw, -hh),
        Vector2( hw, -hh),
        Vector2( hw,  hh),
        Vector2(-hw,  hh)
    };

    for (int i = 0; i < 4; i++) {
        corners[i].x = center.x + local[i].x * c - local[i].y * s;
        corners[i].y = center.y + local[i].x * s + local[i].y * c;
    }
}

static void projectCorners(const Vector2* corners, int count, const Vector2& axis, float& min, float& max) {
    min = max = corners[0].dot(axis);
    for (int i = 1; i < count; i++) {
        float p = corners[i].dot(axis);
        if (p < min) min = p;
        if (p > max) max = p;
    }
}

static CollisionInfo obbOverlap(
    const Vector2& pos1, float angle1, float hw1, float hh1,
    const Vector2& pos2, float angle2, float hw2, float hh2)
{
    CollisionInfo info;
    info.collided = false;

    Vector2 corners1[4], corners2[4];
    getOBBCorners(pos1, angle1, hw1, hh1, corners1);
    getOBBCorners(pos2, angle2, hw2, hh2, corners2);

    Vector2 axes[4];
    for (int i = 0; i < 4; i++) {
        Vector2 edge = corners1[(i + 1) % 4] - corners1[i];
        axes[i] = Vector2(-edge.y, edge.x);
        float len = axes[i].length();
        if (len > 0.0001f) axes[i] /= len;
    }
    for (int i = 0; i < 4; i++) {
        Vector2 edge = corners2[(i + 1) % 4] - corners2[i];
        axes[4 + i] = Vector2(-edge.y, edge.x);
        float len = axes[4 + i].length();
        if (len > 0.0001f) axes[4 + i] /= len;
    }

    float minOverlap = 1e30f;
    Vector2 bestAxis;

    for (int a = 0; a < 8; a++) {
        const Vector2& axis = axes[a];
        if (axis.lengthSquared() < 0.5f) continue;

        float min1, max1, min2, max2;
        projectCorners(corners1, 4, axis, min1, max1);
        projectCorners(corners2, 4, axis, min2, max2);

        float overlap = std::min(max1, max2) - std::max(min1, min2);
        if (overlap <= 0) {
            info.collided = false;
            return info;
        }

        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestAxis = axis;
        }
    }

    Vector2 centerDiff = pos2 - pos1;
    if (bestAxis.dot(centerDiff) < 0) {
        bestAxis = -bestAxis;
    }

    float c1 = pos1.dot(bestAxis);
    float c2 = pos2.dot(bestAxis);
    float contactPos = (c1 + c2) * 0.5f;

    info.collided = true;
    info.normal = bestAxis;
    info.penetration = minOverlap;
    info.contactPoint = (pos1 + pos2) * 0.5f;

    return info;
}

static CollisionInfo circleOBBOverlap(
    const Vector2& circleCenter, float radius,
    const Vector2& rectPos, float rectAngle, float hw, float hh)
{
    CollisionInfo info;
    info.collided = false;

    float cosA = std::cos(-rectAngle);
    float sinA = std::sin(-rectAngle);
    Vector2 diff = circleCenter - rectPos;
    Vector2 localCenter;
    localCenter.x = diff.x * cosA - diff.y * sinA;
    localCenter.y = diff.x * sinA + diff.y * cosA;

    float closestX = std::clamp(localCenter.x, -hw, hw);
    float closestY = std::clamp(localCenter.y, -hh, hh);

    Vector2 closestLocal(closestX, closestY);
    Vector2 diffLocal = localCenter - closestLocal;
    float distSq = diffLocal.lengthSquared();

    if (distSq >= radius * radius) return info;

    float dist = std::sqrt(distSq);

    cosA = std::cos(rectAngle);
    sinA = std::sin(rectAngle);
    Vector2 contactWorld;
    contactWorld.x = rectPos.x + closestLocal.x * cosA - closestLocal.y * sinA;
    contactWorld.y = rectPos.y + closestLocal.x * sinA + closestLocal.y * cosA;

    Vector2 normal;
    if (dist < 0.0001f) {
        normal = Vector2(0, -1);
    } else {
        normal = diffLocal / dist;
        float nx = normal.x * cosA - normal.y * sinA;
        float ny = normal.x * sinA + normal.y * cosA;
        normal = Vector2(nx, ny);
        if (normal.lengthSquared() > 0) normal.normalize();
    }

    info.collided = true;
    info.normal = normal;
    info.penetration = radius - dist;
    info.contactPoint = contactWorld;

    return info;
}

static CollisionInfo circleCircleOverlap(
    const Vector2& p1, float r1,
    const Vector2& p2, float r2)
{
    CollisionInfo info;
    info.collided = false;

    Vector2 diff = p2 - p1;
    float dist = diff.length();

    if (dist >= r1 + r2) return info;

    if (dist < 0.0001f) {
        info.collided = true;
        info.normal = Vector2(0, -1);
        info.penetration = r1 + r2;
        info.contactPoint = p1;
        return info;
    }

    Vector2 normal = diff / dist;

    info.collided = true;
    info.normal = normal;
    info.penetration = r1 + r2 - dist;
    info.contactPoint = p1 + normal * (r1 - info.penetration * 0.5f);

    return info;
}

bool testCollision(PhysicsBody* bodyA, PhysicsBody* bodyB, CollisionInfo& info) {
    const auto& circlesA = bodyA->getCircles();
    const auto& rectsA = bodyA->getRectangles();
    const auto& circlesB = bodyB->getCircles();
    const auto& rectsB = bodyB->getRectangles();

    if (circlesA.empty() && rectsA.empty()) return false;
    if (circlesB.empty() && rectsB.empty()) return false;

    for (const auto& c1 : circlesA) {
        Vector2 p1(bodyA->getPosition().x + c1.offsetX, bodyA->getPosition().y + c1.offsetY);

        for (const auto& c2 : circlesB) {
            Vector2 p2(bodyB->getPosition().x + c2.offsetX, bodyB->getPosition().y + c2.offsetY);
            info = circleCircleOverlap(p1, c1.radius, p2, c2.radius);
            if (info.collided) return true;
        }

        for (const auto& r2 : rectsB) {
            info = circleOBBOverlap(p1, c1.radius, bodyB->getPosition(), bodyB->getAngle(), r2.halfWidth, r2.halfHeight);
            if (info.collided) return true;
        }
    }

    for (const auto& r1 : rectsA) {
        for (const auto& c2 : circlesB) {
            Vector2 p2(bodyB->getPosition().x + c2.offsetX, bodyB->getPosition().y + c2.offsetY);
            info = circleOBBOverlap(p2, c2.radius, bodyA->getPosition(), bodyA->getAngle(), r1.halfWidth, r1.halfHeight);
            if (info.collided) {
                info.normal = -info.normal;
                return true;
            }
        }

        for (const auto& r2 : rectsB) {
            info = obbOverlap(bodyA->getPosition(), bodyA->getAngle(), r1.halfWidth, r1.halfHeight,
                              bodyB->getPosition(), bodyB->getAngle(), r2.halfWidth, r2.halfHeight);
            if (info.collided) return true;
        }
    }

    return false;
}

void resolveCollision(PhysicsBody* bodyA, PhysicsBody* bodyB, const CollisionInfo& info) {
    if (bodyA->getType() == PhysicsBodyType::Static && bodyB->getType() == PhysicsBodyType::Static) return;

    float invMassA = bodyA->getInverseMass();
    float invMassB = bodyB->getInverseMass();
    float invInertiaA = bodyA->getInverseInertia();
    float invInertiaB = bodyB->getInverseInertia();

    Vector2 normal = info.normal;
    Vector2 contact = info.contactPoint;

    float totalInvMass = invMassA + invMassB;
    if (totalInvMass > 0) {
        float correction = info.penetration / totalInvMass * 0.8f;
        Vector2 correctionVec = normal * correction;
        if (invMassA > 0) bodyA->setPosition(bodyA->getPosition() + correctionVec * invMassA);
        if (invMassB > 0) bodyB->setPosition(bodyB->getPosition() - correctionVec * invMassB);
    }

    Vector2 velA = bodyA->getLinearVelocity();
    Vector2 velB = bodyB->getLinearVelocity();
    float angVelA = bodyA->getAngularVelocity();
    float angVelB = bodyB->getAngularVelocity();

    Vector2 rA = contact - bodyA->getPosition();
    Vector2 rB = contact - bodyB->getPosition();

    Vector2 velAPoint = velA + Vector2(-angVelA * rA.y, angVelA * rA.x);
    Vector2 velBPoint = velB + Vector2(-angVelB * rB.y, angVelB * rB.x);
    Vector2 relVel = velAPoint - velBPoint;

    float velAlongNormal = relVel.dot(normal);
    if (velAlongNormal > 0) return;

    float rACrossN = rA.cross(normal);
    float rBCrossN = rB.cross(normal);
    float effectiveMass = invMassA + invMassB
                        + rACrossN * rACrossN * invInertiaA
                        + rBCrossN * rBCrossN * invInertiaB;
    if (effectiveMass == 0) return;

    float e = std::min(bodyA->getRestitution(), bodyB->getRestitution());
    float j = -(1 + e) * velAlongNormal / effectiveMass;

    Vector2 impulse = normal * j;
    bodyA->setLinearVelocity(velA + impulse * invMassA);
    bodyB->setLinearVelocity(velB - impulse * invMassB);
    bodyA->setAngularVelocity(angVelA + rACrossN * j * invInertiaA);
    bodyB->setAngularVelocity(angVelB - rBCrossN * j * invInertiaB);

    Vector2 tangent = relVel - normal * velAlongNormal;
    float tanLen = tangent.length();
    if (tanLen > 0.0001f) {
        tangent /= tanLen;

        float rACrossT = rA.cross(tangent);
        float rBCrossT = rB.cross(tangent);
        float frictionMass = invMassA + invMassB
                           + rACrossT * rACrossT * invInertiaA
                           + rBCrossT * rBCrossT * invInertiaB;
        if (frictionMass > 0) {
            float jt = -(relVel.dot(tangent)) / frictionMass;
            float f = std::sqrt(bodyA->getFriction() * bodyB->getFriction());

            float jtApplied;
            float frictionLimit = j * f;
            if (std::abs(jt) < frictionLimit) {
                jtApplied = jt;
            } else {
                jtApplied = (jt > 0) ? -frictionLimit : frictionLimit;
            }

            bodyA->setLinearVelocity(bodyA->getLinearVelocity() + tangent * jtApplied * invMassA);
            bodyB->setLinearVelocity(bodyB->getLinearVelocity() - tangent * jtApplied * invMassB);
            bodyA->setAngularVelocity(bodyA->getAngularVelocity() + rACrossT * jtApplied * invInertiaA);
            bodyB->setAngularVelocity(bodyB->getAngularVelocity() - rBCrossT * jtApplied * invInertiaB);
        }
    }
}
