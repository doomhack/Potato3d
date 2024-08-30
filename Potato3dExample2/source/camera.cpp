#include "../include/camera.h"

Camera::Camera()
{
    position = P3D::V3<P3D::fp>(8508,13,8563);
    angle = P3D::V3<P3D::fp>(0,-236,0);
}

const P3D::V3<P3D::fp>& Camera::GetPosition() const
{
    return position;
}

const P3D::V3<P3D::fp> Camera::GetEyePosition() const
{
    return (position + eyeOffset);
}

const P3D::V3<P3D::fp>& Camera::GetAngle() const
{
    return angle;
}

void Camera::HandleInput(unsigned int keyState)
{
    if(keyState & KeyLeft)
        angle.y += 2;

    if(keyState & KeyRight)
        angle.y -= 2;

    if(keyState & KeyUp)
    {
        P3D::V3<P3D::fp> camAngle = angle;

        float angleYRad = P3D::pD2R(camAngle.y);

        P3D::V3<P3D::fp> d((float)-(std::sin(angleYRad) *10), 0, (float)-(std::cos(angleYRad) *10));

        position += d;
    }

    if(keyState & KeyDown)
    {
        P3D::V3<P3D::fp> camAngle = angle;

        float angleYRad = P3D::pD2R(camAngle.y);

        P3D::V3<P3D::fp> d((float)-(std::sin(angleYRad) *10), 0, (float)-(std::cos(angleYRad) *10));

        position -= d;
    }

    position.y -= 5;

}

void Camera::MovePosition(const P3D::V3<P3D::fp>& delta)
{
    position += delta;
}

void Camera::SetPosition(const P3D::V3<P3D::fp>& pos)
{
    position = pos;
}
