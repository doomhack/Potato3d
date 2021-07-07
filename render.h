#ifndef RENDER_H
#define RENDER_H

#include "common.h"
#include "rtypes.h"

namespace P3D
{

    typedef struct TriEdgeTrace
    {
        fp x_left, x_right;
        fp w_left;
        fp u_left;
        fp v_left;
    } TriEdgeTrace;

    typedef struct TriDrawPos
    {
        fp x;
        fp w;
        fp u;
        fp v;
    } TriDrawPos;

    typedef struct TriDrawXDeltaZWUV
    {
        fp w;
        fp u;
        fp v;
    } TriDrawXDeltaZWUV;

    typedef struct TriDrawYDeltaZWUV
    {
        fp x_left, x_right;
        fp w;
        fp u;
        fp v;
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
        W_Near,
        X_W_Left,
        X_W_Right,
        Y_W_Top,
        Y_W_Bottom
    } ClipPlane;

    typedef struct SpanNode
    {
        short x_start;
        short x_end;
        SpanNode* left = nullptr;
        SpanNode* right = nullptr;
    } SpanNode;

    typedef struct SpanBuffer
    {
        SpanNode* span_tree = nullptr;
        int pixels_left = 0;
    } SpanBuffer;

    class Render
    {
    public:
        explicit Render();

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel* framebuffer = nullptr);

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
        void DrawTriangleClip(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags);

        void ClipPolygon(const Vertex2d clipSpacePointsIn[], const int vxCount, Vertex2d clipSpacePointsOut[], int& vxCountOut, ClipPlane clipPlane);
        void TriangulatePolygon(Vertex2d clipSpacePoints[], const int vxCount, const Texture *texture, const pixel color, const RenderFlags flags);

        fp GetClipPointForVertex(const Vertex2d& vertex, ClipPlane clipPlane) const;

        void DrawTriangleCull(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags);

        fp GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2);

        void DrawTriangleSplit(Vertex2d *points, const Texture *texture, const pixel color, RenderFlags flags);
        void DrawTriangleTop(const Vertex2d *points, const Texture *texture, const pixel color, const RenderFlags flags);
        void DrawTriangleBottom(const Vertex2d *points, const Texture *texture, const pixel color, const RenderFlags flags);

        void ClipSpan(int y, TriEdgeTrace &pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color, const RenderFlags flags);
        SpanNode* GetFreeSpanNode();

        void DrawSpan(int y, TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color);

        void DrawTriangleScanlinePerspectiveCorrect(int y, const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture);

        inline void DrawScanlinePixelLinear(pixel* fb, const pixel* texels, const fp u, const fp v);

        void DrawTriangleSplitFlat(const Vertex2d points[], const pixel color);
        void DrawTriangleTopFlat(const Vertex2d points[], const pixel color);
        void DrawTriangleBottomFlat(const Vertex2d points[3], const pixel color);
        void DrawTriangleScanlineFlat(int y, const TriEdgeTrace& pos, const pixel color);



        void SortPointsByY(Vertex2d points[]);

        Vertex2d TransformVertex(const Vertex3d* vertex);

        void LerpVertexXYZWUV(Vertex2d& out, const Vertex2d& left, const Vertex2d& right, fp frac);

        void GetTriangleLerpDeltasZWUV(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawXDeltaZWUV& x_delta, TriDrawYDeltaZWUV &y_delta);
        void GetTriangleLerpDeltasZ(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawYDeltaZWUV &y_delta);


        int fracToY(fp frac);
        int fracToX(fp frac);



        pixel* frameBuffer;
        V2<int> fbSize;

        fp zNear;
        fp zFar;

        M4<fp> modelMatrix;
        M4<fp> viewMatrix;


        M4<fp> projectionMatrix;
        M4<fp> viewProjectionMatrix; //P*V

        M4<fp> transformMatrix; //P*V*M

        const unsigned int triFracShift = 4;

        RenderStats stats;

        SpanBuffer* spanBuffer = nullptr;

        int pixels_left;

        SpanNode* span_pool;
        unsigned int span_free_index;
    };

}

#endif // RENDER_H
