#pragma once
#include <utils/math/vector.h>

class Camera2D {
private:
    Vector2 position;
    float zoom;
    float rotation;

    Matrix3x3 viewMatrix;
    Matrix3x3 projMatrix;
    Matrix3x3 viewProjMatrix;
    bool dirty;
    float cachedAspectRatio;

    void updateMatrices();

public:
    Camera2D();

    void setPosition(const Vector2& pos);
    Vector2 getPosition() const;
    void setZoom(float z);
    float getZoom() const;
    void setRotation(float deg);
    float getRotation() const;

    Matrix3x3 getViewMatrix() const;
    Matrix3x3 getProjectionMatrix(float aspectRatio);
    Matrix3x3 getViewProjectionMatrix(float aspectRatio);
};
