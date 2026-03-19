#ifndef MAINLOOP_H
#define MAINLOOP_H

#include "../include/common.h"
#include "../../Config.h"
#include "../../3dmaths/f3dmath.h"

#include "../../RenderDevice.h"
#include "../../PixelShaderGBA8.h"

#include "../include/videosystem.h"
#include "../include/worldmodel.h"
#include "collision.h"
#include <unordered_set>

class MainLoop
{
public:
    MainLoop();

    void Run(bool withGui = true, unsigned int slice = 0, unsigned int totalSlices = 1);

    const std::map<unsigned int, std::unordered_set<unsigned int>>& GetPVSData() const;
    unsigned int GetNodeCount() const { return model.GetModel()->header.node_count; }


private:

    void UpdateFrustrumBB();
    void RenderModel();
    bool FrustrumTestTriangle(const P3D::BspModelTriangle* tri) const;

    bool CheckCollisions(P3D::V3<P3D::fp> point);

    static constexpr P3D::fp zNear = 10;
    static constexpr P3D::fp zFar = 10000;
    static constexpr P3D::fp vFov = 90;
    static constexpr P3D::fp hFov = 90;

    P3D::V3<P3D::fp> position = P3D::V3<P3D::fp>(-2511,1050,-3108);

    P3D::V3<P3D::fp> frustrumPoints[4]; //Top left and bottom-right frustrum points.

    P3D::Plane<P3D::fp> frustrumPlanes[6];

    P3D::AABB<short> viewFrustrumBB;

    P3D::RenderDevice renderDev;

    WorldModel model;
    VideoSystem vid;
    Collision collision;

    P3D::List<const P3D::BspModelTriangle*> triBuffer;

    std::map<unsigned int, std::unordered_set<unsigned int>> visData;
};

#endif // MAINLOOP_H
