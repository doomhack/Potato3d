#ifndef MAINLOOP_H
#define MAINLOOP_H

#include "../include/common.h"
#include "../../Config.h"
#include "../../3dmaths/f3dmath.h"

#include "../../RenderDevice.h"
#include "../../PixelShaderGBA8.h"

#include "../include/videosystem.h"
#include "../include/camera.h"
#include "../include/worldmodel.h"
#include "../include/collision.h"
#include <unordered_set>

class MainLoop
{
public:
    MainLoop();

    void Run();

private:

    void UpdateFrustrumBB();
    void RenderModel();
    bool FrustrumTestTriangle(const P3D::BspModelTriangle* tri) const;

    void StorePVS();

    static constexpr P3D::fp zNear = 1;
    static constexpr P3D::fp zFar = 10000;
    static constexpr P3D::fp vFov = 90;
    static constexpr P3D::fp hFov = 90;


    P3D::V3<P3D::fp> frustrumPoints[4]; //Top left and bottom-right frustrum points.

    P3D::Plane<P3D::fp> frustrumPlanes[6];

    P3D::AABB<P3D::fp> viewFrustrumBB;

    P3D::RenderDevice renderDev;

    Camera camera;
    WorldModel model;
    VideoSystem vid;

    std::vector<const P3D::BspModelTriangle*> triBuffer;

    std::map<unsigned int, std::unordered_set<unsigned int>> visData;
};

#endif // MAINLOOP_H
