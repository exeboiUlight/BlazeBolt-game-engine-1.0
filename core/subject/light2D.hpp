#pragma once
#include <utils/math/vector.h>
#include <cstdint>

namespace BlazeBolt {
    class Light2D {
    public:
        enum Type { POINT, AMBIENT };

    private:
        Type type;
        Vector2 position;
        Vector3 color;
        float intensity;
        float radius;
        bool enabled;

    public:
        Light2D(Type type);
        ~Light2D() = default;

        Type getType() const;

        void setPosition(const Vector2& pos);
        const Vector2& getPosition() const;

        void setColor(const Vector3& col);
        const Vector3& getColor() const;

        void setIntensity(float i);
        float getIntensity() const;

        void setRadius(float r);
        float getRadius() const;

        void setEnabled(bool e);
        bool isEnabled() const;
    };
}
