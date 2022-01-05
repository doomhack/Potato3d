#ifndef CORES_H
#define CORES_H

#include "../common.h"
#include "../rtypes.h"

namespace P3D
{
    void DrawTriangleScanlineFlat(const TriEdgeTrace& pos, const pixel color);
    void DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture);
    void DrawTriangleScanlinePerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture);
};
#endif // CORES_H
