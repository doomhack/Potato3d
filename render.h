#ifndef RENDER_H
#define RENDER_H

#include "common.h"
#include "rtypes.h"

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


    void DrawTriangle(const Triangle3d* tri, Texture *texture, pixel color);

    P3D::M4<fp>& GetMatrix(MatrixType matrix);

    void SetFramebuffer(pixel* frameBuffer);

private:
    void DrawTriangleClip(Vertex2d clipSpacePoints[], Texture *texture, pixel color);
    void DrawTriangleCull(Vertex2d clipSpacePoints[], Texture *texture, pixel color);

    fp GetLineIntersection(fp v1, fp v2, const fp pos);


    void DrawTriangleSplit(Vertex2d points[], Texture* texture);
    void DrawTriangleTop(Vertex2d points[3], Texture* texture);
    void DrawTriangleBottom(Vertex2d points[3], Texture* texture);

    void DrawTriangleSplit(Vertex2d points[], pixel color);
    void DrawTriangleTop(Vertex2d points[3], pixel color);
    void DrawTriangleBottom(Vertex2d points[3], pixel color);

    void DrawTriangleScanline(int y, TriEdgeTrace& pos, Texture* texture);
    void DrawTriangleScanline(int y, TriEdgeTrace& pos, pixel color);


    void SortPointsByY(Vertex2d points[3]);

    Vertex2d TransformVertex(const Vertex3d* vertex);
    bool IsTriangleFrontface(Vertex2d screenSpacePoints[3]);
    bool IsTriangleOnScreen(Vertex2d screenSpacePoints[3]);

    int fracToY(fp frac);
    int fracToX(fp frac);



    pixel* frameBuffer;
    fp* zBuffer;
    P3D::V2<int> fbSize;

    fp zNear;
    fp zFar;

    P3D::M4<fp> modelMatrix;
    P3D::M4<fp> viewMatrix;


    P3D::M4<fp> projectionMatrix;
    P3D::M4<fp> viewProjectionMatrix; //P*V

    P3D::M4<fp> transformMatrix; //P*V*M
};

#endif // RENDER_H
