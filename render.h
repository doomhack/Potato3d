#ifndef RENDER_H
#define RENDER_H

#include "common.h"
#include "rtypes.h"

namespace P3D
{
    typedef struct TriEdgeTrace
    {
        fp x_left, x_right, w_left;
        fp u_left, v_left;
        pixel* fb_ypos;
    } TriEdgeTrace;

    typedef struct TriDrawXDeltaZWUV
    {
        fp u;
        fp v;
        fp w;
    } TriDrawXDeltaZWUV;

    typedef struct TriDrawYDeltaZWUV
    {
        fp x_left, x_right;
        fp u, v, w;
    } TriDrawYDeltaZWUV;

    typedef enum MatrixType
    {
        Model,
        View,
        Projection,
        Transform
    } MatrixType;

    typedef enum ClipPlane
    {
        NoClip = 0u,
        W_Near = 1u,
        X_W_Left = 2u,
        X_W_Right = 4u,
        Y_W_Top = 8u,
        Y_W_Bottom = 16u
    } ClipPlane;

    class Render
    {
    public:
        explicit Render();

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel *framebuffer = nullptr);

        void BeginFrame();
        void EndFrame();
        void BeginObject();
        void EndObject();

        void ClearFramebuffer(pixel color);

        void UpdateTransformMatrix();
        void UpdateViewProjectionMatrix();


        void DrawTriangle(const Triangle3d* tri, const Texture *texture, const pixel color, const RenderFlags flags);

        M4<fp>& GetMatrix(MatrixType matrix);

        void SetFramebuffer(pixel* frameBuffer);
        pixel* GetFramebuffer();

        RenderStats GetRenderStats();


    private:
        void DrawTriangleClip(Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags);

        unsigned int ClipPolygon(const Vertex2d clipSpacePointsIn[], const int vxCount, Vertex2d clipSpacePointsOut[], ClipPlane clipPlane);
        void TriangulatePolygon(Vertex2d clipSpacePoints[], const int vxCount, const Texture *texture, const pixel color, const RenderFlags flags);

        fp GetClipPointForVertex(const Vertex2d& vertex, ClipPlane clipPlane) const;

        fp GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2);

        void DrawTriangleSplit(Vertex2d screenSpacePoints[], const Texture *texture, const pixel color, RenderFlags flags);
        void DrawTriangleTop(const Vertex2d points[], const Texture *texture, const pixel color, const RenderFlags flags);
        void DrawTriangleBottom(const Vertex2d points[], const Texture *texture, const pixel color, const RenderFlags flags);

        void DrawSpan(TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color, const RenderFlags flags);

        void DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture);
        void DrawTriangleScanlinePerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture);


        inline void DrawScanlinePixelLinearPair(pixel *fb, const pixel* texels, const unsigned int uv1, const unsigned int uv2);
        inline void DrawScanlinePixelLinearLowByte(pixel* fb, const pixel* texels, const unsigned int uv);
        inline void DrawScanlinePixelLinearHighByte(pixel* fb, const pixel* texels, const unsigned int uv);


        void DrawTriangleSplitFlat(const Vertex2d points[], const pixel color);
        void DrawTriangleTopFlat(const Vertex2d points[], const pixel color);
        void DrawTriangleBottomFlat(const Vertex2d points[3], const pixel color);
        void DrawTriangleScanlineFlat(const TriEdgeTrace& pos, const pixel color);



        void SortPointsByY(Vertex2d pointsIn[], Vertex2d pointsOut[]);

        Vertex2d TransformVertex(const Vertex3d* vertex);

        void LerpVertexXYZWUV(Vertex2d& out, const Vertex2d& left, const Vertex2d& right, fp frac);

        void GetTriangleLerpDeltasZWUV(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawXDeltaZWUV& x_delta, TriDrawYDeltaZWUV &y_delta);
        void GetTriangleLerpDeltasZ(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawYDeltaZWUV &y_delta);

        unsigned int PackUV(fp u, fp v);

        fp fracToY(fp frac);
        fp fracToX(fp frac);



        pixel* frameBuffer;
        V2<int> fbSize;

        fp zNear;
        fp zFar;

        M4<fp> modelMatrix;
        M4<fp> viewMatrix;
        M4<fp> projectionMatrix;
        M4<fp> viewProjectionMatrix; //P*V

        M4<fp> transformMatrix; //P*V*M

        RenderStats stats;

        V4<fp>* transformedVertexCache = nullptr;
        unsigned char* transformedVertexCacheIndexes = nullptr;
    };

}

#endif // RENDER_H
