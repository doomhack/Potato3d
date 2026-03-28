#include "../include/camera.h"

Camera::Camera()
{
    //position = P3D::V3<P3D::fp>(-1331,1053,-1074); //Villa
    //position = P3D::V3<P3D::fp>(-6085,-78,1109); //Facility

    //position = P3D::V3<P3D::fp>(-230,39,6152); //DDHQ2

    //position = P3D::V3<P3D::fp>(8508,13,8563);
    //angle = P3D::V3<P3D::fp>(0,-236,0);

    //position = P3D::V3<P3D::fp>(-350,50,-350); //PVS Test
    position = P3D::V3<P3D::fp>(-50,100,-250); //PVS Test
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
