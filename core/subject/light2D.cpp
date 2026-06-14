#include "light2D.hpp"

namespace BlazeBolt {
    Light2D::Light2D(Type t) :
        type(t),
        position(0, 0),
        color(1, 1, 1),
        intensity(1.0f),
        radius(200.0f),
        enabled(true)
    {
    }

    Light2D::Type Light2D::getType() const { return type; }

    void Light2D::setPosition(const Vector2& pos) { position = pos; }
    const Vector2& Light2D::getPosition() const { return position; }

    void Light2D::setColor(const Vector3& col) { color = col; }
    const Vector3& Light2D::getColor() const { return color; }

    void Light2D::setIntensity(float i) { intensity = i; }
    float Light2D::getIntensity() const { return intensity; }

    void Light2D::setRadius(float r) { radius = r; }
    float Light2D::getRadius() const { return radius; }

    void Light2D::setEnabled(bool e) { enabled = e; }
    bool Light2D::isEnabled() const { return enabled; }
}
