#include "../include/camera.h"

Camera::Camera()
{
    position = P3D::V3<P3D::fp>(-2511,1050,-3108);
    //position = P3D::V3<P3D::fp>(-230,39,6152);

    //position = P3D::V3<P3D::fp>(8508,13,8563);
    //angle = P3D::V3<P3D::fp>(0,-236,0);
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

void Camera::HandleInput(unsigned int keyState, P3D::fp gravity_velocity)
{
    if(keyState & KeyLeft)
        angle.y += 5;

    if(keyState & KeyRight)
        angle.y -= 5;

    if(keyState & KeyUp)
    {
        P3D::V3<P3D::fp> camAngle = angle;

        float angleYRad = P3D::pD2R(camAngle.y);

        P3D::V3<P3D::fp> d((float)-(std::sin(angleYRad) *20), 0, (float)-(std::cos(angleYRad) *20));

        position += d;
    }

    if(keyState & KeyDown)
    {
        P3D::V3<P3D::fp> camAngle = angle;

        float angleYRad = P3D::pD2R(camAngle.y);

        P3D::V3<P3D::fp> d((float)-(std::sin(angleYRad) *20), 0, (float)-(std::cos(angleYRad) *20));

        position -= d;
    }

    position.y -= gravity_velocity;

}

void Camera::MovePosition(const P3D::V3<P3D::fp>& delta)
{
    position += delta;
}

void Camera::SetPosition(const P3D::V3<P3D::fp>& pos)
{
    position = pos;
}
