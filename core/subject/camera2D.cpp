#include "camera2D.hpp"

Camera2D::Camera2D()
    : position(0, 0), zoom(1.0f), rotation(0.0f)
    , dirty(true), cachedAspectRatio(1.0f)
{
}

void Camera2D::setPosition(const Vector2& pos) {
    position = pos;
    dirty = true;
}

Vector2 Camera2D::getPosition() const {
    return position;
}

void Camera2D::setZoom(float z) {
    zoom = (z > 0.001f) ? z : 0.001f;
    dirty = true;
}

float Camera2D::getZoom() const {
    return zoom;
}

void Camera2D::setRotation(float deg) {
    rotation = deg;
    dirty = true;
}

float Camera2D::getRotation() const {
    return rotation;
}

void Camera2D::updateMatrices() {
    // View matrix: world -> camera space
    // Inverse of camera transform: translate by -position, rotate by -rotation
    viewMatrix = Matrix3x3::translation(-position.x, -position.y) * Matrix3x3::rotation(-rotation);

    // Projection matrix: camera space -> clip space
    // The shader divides x by aspect ratio, so projection only handles zoom
    // scale(zoom, zoom) makes objects appear larger when zoom > 1
    projMatrix = Matrix3x3::scale(zoom, zoom);

    // Combined view-projection
    viewProjMatrix = projMatrix * viewMatrix;

    dirty = false;
}

Matrix3x3 Camera2D::getViewMatrix() const {
    if (dirty) const_cast<Camera2D*>(this)->updateMatrices();
    return viewMatrix;
}

Matrix3x3 Camera2D::getProjectionMatrix(float aspectRatio) {
    if (dirty || aspectRatio != cachedAspectRatio) {
        cachedAspectRatio = aspectRatio;
        dirty = true;
    }
    if (dirty) updateMatrices();
    return projMatrix;
}

Matrix3x3 Camera2D::getViewProjectionMatrix(float aspectRatio) {
    if (dirty || aspectRatio != cachedAspectRatio) {
        cachedAspectRatio = aspectRatio;
        dirty = true;
    }
    if (dirty) updateMatrices();
    return viewProjMatrix;
}
