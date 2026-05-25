#pragma once

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
