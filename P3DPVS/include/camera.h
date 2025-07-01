#ifndef CAMERA_H
#define CAMERA_H

#include "../include/common.h"
#include "../../Config.h"
#include "../../3dmaths/f3dmath.h"

class Camera
{
    public:
        Camera();

        const P3D::V3<P3D::fp>& GetPosition() const;
        const P3D::V3<P3D::fp> GetEyePosition() const;
        const P3D::V3<P3D::fp>& GetAngle() const;
        void MovePosition(const P3D::V3<P3D::fp>& delta);
        void SetPosition(const P3D::V3<P3D::fp>& pos);

        void HandleInput(unsigned int keyState, P3D::fp gravity_velocity);

    private:
        P3D::V3<P3D::fp> position = P3D::V3<P3D::fp>(0,50,0);
        P3D::V3<P3D::fp> angle = P3D::V3<P3D::fp>(0,0,0);
        const P3D::V3<P3D::fp> eyeOffset = P3D::V3<P3D::fp>(0,0,0);
};

#endif // CAMERA_H
