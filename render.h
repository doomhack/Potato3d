#ifndef RENDER_H
#define RENDER_H

#include "common.h"
#include "rtypes.h"

namespace P3D
{

    typedef struct TriEdgeTrace
    {
        fp x_left, x_right;
        fp z_left, z_right;
        fp w_left, w_right;
        fp u_left, u_right;
        fp v_left, v_right;
    } TriDrawState;

    typedef struct TriDrawPos
    {
        fp z;
        fp w;
        fp u;
        fp v;
    } TriDrawPos;

    typedef enum MatrixType
    {
        Model,
        View,
        Projection
    } MatrixType;

    typedef enum RenderFlags
    {
        NoFlags = 0u,
        PerspectiveCorrect = 1u,
        Alpha = 2u,
        NoBackfaceCull = 4u,
    } RenderFlags;

    typedef enum ClipPlane
    {
        W_Near,
        X_W_Left,
        X_W_Right,
        Y_W_Top,
        Y_W_Bottom
    } ClipPlane;

    class Render
    {
    public:
        explicit Render();

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel* framebuffer = nullptr, fp* zBuffer = nullptr);

        void BeginFrame();
        void EndFrame();
        void BeginObject();
        void EndObject();

        void ClearFramebuffer(pixel color, bool zBuffer);

        void UpdateTransformMatrix();
        void UpdateViewProjectionMatrix();


        void DrawTriangle(const Triangle3d* tri, const Texture *texture, const pixel color, const RenderFlags flags);

        M4<fp>& GetMatrix(MatrixType matrix);

        void SetFramebuffer(pixel* frameBuffer);
        pixel* GetFramebuffer();

    private:
        void DrawTriangleClip(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags);

        void ClipPolygon(const Vertex2d clipSpacePointsIn[], const int vxCount, Vertex2d clipSpacePointsOut[], int& vxCountOut, ClipPlane clipPlane);
        void TriangulatePolygon(Vertex2d clipSpacePoints[], const int vxCount, const Texture *texture, const pixel color, const RenderFlags flags);

        fp GetClipPointForVertex(const Vertex2d& vertex, ClipPlane clipPlane);

        void DrawTriangleCull(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags);

        fp GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2);

        void DrawTriangleSplitPerspectiveCorrect(Vertex2d points[], const Texture *texture, const RenderFlags flags);
        void DrawTriangleTopPerspectiveCorrect(const Vertex2d points[], const Texture *texture, const RenderFlags flags);
        void DrawTriangleBottomPerspectiveCorrect(const Vertex2d points[3], const Texture* texture, const RenderFlags flags);
        void DrawTriangleScanlinePerspectiveCorrect(int y, const TriEdgeTrace &pos, const Texture *texture, const RenderFlags flags);

        void DrawTriangleSplitLinear(const Vertex2d points[], const Texture *texture, const RenderFlags flags);
        void DrawTriangleTopLinear(const Vertex2d points[], const Texture *texture, const RenderFlags flags);
        void DrawTriangleBottomLinear(const Vertex2d points[3], const Texture* texture, const RenderFlags flags);
        void DrawTriangleScanlineLinear(int y, const TriEdgeTrace &pos, const Texture *texture, const RenderFlags flags);

        void DrawTriangleSplitFlat(const Vertex2d points[], const pixel color, const RenderFlags flags);
        void DrawTriangleTopFlat(const Vertex2d points[], const pixel color, const RenderFlags flags);
        void DrawTriangleBottomFlat(const Vertex2d points[3], const pixel color, const RenderFlags flags);
        void DrawTriangleScanlineFlat(int y, const TriEdgeTrace& pos, const pixel color, const RenderFlags flags);

        void SortPointsByY(Vertex2d points[]);

        Vertex2d TransformVertex(const Vertex3d* vertex);
        bool IsTriangleFrontface(const Vertex2d screenSpacePoints[]);
        bool IsTriangleOnScreen(const Vertex2d screenSpacePoints[]);

        void LerpEdgePosZWUV(TriDrawPos& out, const TriEdgeTrace &edge, fp frac);
        void LerpEdgePosZUV(TriDrawPos& out, const TriEdgeTrace &edge, fp frac);

        void LerpEdgeXZWUV(TriEdgeTrace& out, const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, fp frac);
        void LerpEdgeXZUV(TriEdgeTrace& out, const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, fp frac);
        void LerpEdgeXZ(TriEdgeTrace& out, const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, fp frac);

        void LerpVertexXYZWUV(Vertex2d& out, const Vertex2d& left, const Vertex2d& right, fp frac);


        int fracToY(fp frac);
        int fracToX(fp frac);



        pixel* frameBuffer;
        fp* zBuffer;
        V2<int> fbSize;

        fp zNear;
        fp zFar;

        M4<fp> modelMatrix;
        M4<fp> viewMatrix;


        M4<fp> projectionMatrix;
        M4<fp> viewProjectionMatrix; //P*V

        M4<fp> transformMatrix; //P*V*M

        const unsigned int xFracShift = 1;
        const unsigned int yFracShift = 10;

    };

}

#endif // RENDER_H
