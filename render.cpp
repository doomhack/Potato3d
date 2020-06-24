#include <algorithm>
#include "render.h"

namespace P3D
{
    Render::Render()
    {
        frameBuffer = nullptr;
        zBuffer = nullptr;
        fbSize = {0,0};

        modelMatrix.setToIdentity();
        viewMatrix.setToIdentity();


        projectionMatrix.setToIdentity();
        viewProjectionMatrix.setToIdentity();

        transformMatrix.setToIdentity();
    }

    bool Render::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel* frameBuffer, fp* zBuffer)
    {
        if(screenWidth == 0 || screenHeight == 0)
            return false;

        if(hFov <= 0 || hFov >= 180)
            return false;

        if(zNear <= 0 || zNear >= zFar)
            return false;

        fbSize.x = screenWidth;
        fbSize.y = screenHeight;

        this->zNear = zNear;
        this->zFar = zFar;

        if(frameBuffer)
            this->frameBuffer = frameBuffer;
        else
            this->frameBuffer = new pixel[screenWidth * screenHeight];

        if(zBuffer)
            this->zBuffer = zBuffer;
        else
            this->zBuffer = new fp[screenWidth * screenHeight];

        fp aspectRatio = fp((int)screenWidth) / fp((int)screenHeight);

        projectionMatrix.perspective(hFov, aspectRatio, zNear, zFar);

        return true;
    }

    void Render::BeginFrame()
    {
        UpdateViewProjectionMatrix();
    }

    void Render::EndFrame()
    {

    }

    void Render::BeginObject()
    {
        UpdateTransformMatrix();
    }

    void Render::EndObject()
    {

    }

    void Render::ClearFramebuffer(pixel color, bool clearZ)
    {
        const unsigned int buffSize = fbSize.x*fbSize.y;

        for(unsigned int i = 0; i < buffSize; i++)
            frameBuffer[i] = color;

        if(clearZ)
        {
            for(unsigned int i = 0; i < buffSize; i++)
                zBuffer[i] = 1;
        }
    }

    void Render::UpdateTransformMatrix()
    {
        if(viewProjectionMatrix.ResetFlag(MatrixFlags::Updated) | modelMatrix.ResetFlag(MatrixFlags::Updated))
            transformMatrix = viewProjectionMatrix * modelMatrix;
    }

    void Render::UpdateViewProjectionMatrix()
    {
        if(projectionMatrix.ResetFlag(MatrixFlags::Updated) | viewMatrix.ResetFlag(MatrixFlags::Updated))
            viewProjectionMatrix = projectionMatrix * viewMatrix;
    }

    M4<fp>& Render::GetMatrix(MatrixType matrix)
    {
        switch(matrix)
        {
            case MatrixType::View:
                return this->viewMatrix;

            case MatrixType::Model:
                return this->modelMatrix;

            case MatrixType::Projection:
                return this->projectionMatrix;
        }
    }

    void Render::SetFramebuffer(pixel* frameBuffer)
    {
        this->frameBuffer = frameBuffer;
    }

    pixel* Render::GetFramebuffer()
    {
        return this->frameBuffer;
    }

    void Render::DrawTriangle(const Triangle3d* tri, const Texture* texture, const pixel color, const RenderFlags flags)
    {
        Vertex2d clipSpacePoints[3];

        for(int i = 0; i < 3; i++)
        {
            const Vertex3d* v = &tri->verts[i];

            clipSpacePoints[i] = this->TransformVertex(v);
        }

        DrawTriangleClip(clipSpacePoints, texture, color, flags);
    }

    Vertex2d Render::TransformVertex(const Vertex3d* vertex)
    {
        V4<fp> p = transformMatrix * vertex->pos;

        Vertex2d screenspace;

        screenspace.pos = V4<fp>
        (
            p.x,
            p.y,
            p.z,
            p.w
        );

        screenspace.uv = vertex->uv;

        return screenspace;
    }

    void Render::DrawTriangleClip(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        fp w0 = clipSpacePoints[0].pos.w;
        fp w1 = clipSpacePoints[1].pos.w;
        fp w2 = clipSpacePoints[2].pos.w;

        fp x0 = clipSpacePoints[0].pos.x;
        fp x1 = clipSpacePoints[1].pos.x;
        fp x2 = clipSpacePoints[2].pos.x;

        fp y0 = clipSpacePoints[0].pos.y;
        fp y1 = clipSpacePoints[1].pos.y;
        fp y2 = clipSpacePoints[2].pos.y;

        fp z0 = clipSpacePoints[0].pos.z;
        fp z1 = clipSpacePoints[1].pos.z;
        fp z2 = clipSpacePoints[2].pos.z;

        if(x0 > w0 && x1 > w1 && x2 > w2)
            return;

        if(-x0 > w0 && -x1 > w1 && -x2 > w2)
            return;

        if(y0 > w0 && y1 > w1 && y2 > w2)
            return;

        if(-y0 > w0 && -y1 > w1 && -y2 > w2)
            return;

        if(z0 > w0 && z1 > w1 && z2 > w2)
            return;

        if(-z0 > w0 && -z1 > w1 && -z2 > w2)
            return;

        //All points behind clipping plane.
        if(w0 < zNear && w1 < zNear && w2 < zNear)
            return;

        Vertex2d outputVxA[8];
        Vertex2d outputVxB[8];
        int countA = 0;
        int countB = 0;

        //As we clip against each frustrum plane, we swap the buffers
        //so the output of the last clip is used as input to the next.
        ClipPolygon(clipSpacePoints, 3, outputVxA, countA, W_Near);
        ClipPolygon(outputVxA, countA, outputVxB, countB, X_W_Left);
        ClipPolygon(outputVxB, countB, outputVxA, countA, X_W_Right);
        ClipPolygon(outputVxA, countA, outputVxB, countB, Y_W_Top);
        ClipPolygon(outputVxB, countB, outputVxA, countA, Y_W_Bottom);

        //Now outputVxA and CountA contain the final result.
        TriangulatePolygon(outputVxA, countA, texture, color, flags);
    }

    void Render::ClipPolygon(const Vertex2d clipSpacePointsIn[], const int vxCount, Vertex2d clipSpacePointsOut[], int& vxCountOut, ClipPlane clipPlane)
    {
        vxCountOut = 0;

        for(int i = 0; i < vxCount; i++)
        {
            int i2 = (i < (vxCount-1)) ? i+1 : 0;

            const fp b1 = GetClipPointForVertex(clipSpacePointsIn[i], clipPlane);
            const fp b2 = GetClipPointForVertex(clipSpacePointsIn[i2], clipPlane);

            if(clipSpacePointsIn[i].pos.w >= b1)
            {
                clipSpacePointsOut[vxCountOut] = clipSpacePointsIn[i];
                vxCountOut++;
            }

            fp frac = GetLineIntersectionFrac(clipSpacePointsIn[i].pos.w, clipSpacePointsIn[i2].pos.w, b1, b2);

            if(frac > 0)
            {
                Vertex2d newVx;

                LerpVertexXYZWUV(newVx, clipSpacePointsIn[i], clipSpacePointsIn[i2], frac);

                clipSpacePointsOut[vxCountOut] = newVx;
                vxCountOut++;
            }
        }
    }

    void Render::TriangulatePolygon(Vertex2d clipSpacePoints[], const int vxCount, const Texture *texture, const pixel color, const RenderFlags flags)
    {
        if(vxCount < 3)
            return;

        DrawTriangleCull(clipSpacePoints, texture, color, flags);

        int rounds = vxCount - 3;

        for(int i = 0; i < rounds; i++)
        {
            clipSpacePoints[i+1] = clipSpacePoints[0];
            DrawTriangleCull(&clipSpacePoints[i+1], texture, color, flags);
        }
    }

    fp Render::GetClipPointForVertex(const Vertex2d& vertex, ClipPlane clipPlane)
    {
        switch(clipPlane)
        {
            case W_Near:
                return zNear;

            case X_W_Left:
                return -vertex.pos.x;

            case X_W_Right:
                return vertex.pos.x;

            case Y_W_Top:
                return vertex.pos.y;

            case Y_W_Bottom:
                return -vertex.pos.y;
        }

        return 0;
    }

    fp Render::GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2)
    {
        if(a1 <= b1 && a2 <= b2)
            return -1;
        else if(a1 >= b1 && a2 >= b2)
            return -2;

        fp len_1 = (a1 - b1);
        fp len_2 = (a2 - b2);

        return ((len_1) / (len_1 - len_2));
    }

    void Render::DrawTriangleCull(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        Vertex2d screenSpacePoints[3];

        for(int i = 0; i < 3; i++)
        {
            screenSpacePoints[i].pos = clipSpacePoints[i].pos.ToScreenSpace();

            if(texture)
            {
                screenSpacePoints[i].uv.x = clipSpacePoints[i].uv.x * (int)texture->width;
                screenSpacePoints[i].uv.y = fp((int)texture->height) - (clipSpacePoints[i].uv.y * (int)texture->height);
            }
        }

        //Backface cull here.
        if(!(flags & RenderFlags::NoBackfaceCull))
        {
            if(!IsTriangleFrontface(screenSpacePoints))
                return;
        }

        //Reject offscreen polys
        if(!IsTriangleOnScreen(screenSpacePoints))
            return;

        for(int i = 0; i < 3; i++)
        {
            screenSpacePoints[i].pos.x = fracToX(screenSpacePoints[i].pos.x);
            screenSpacePoints[i].pos.y = fracToY(screenSpacePoints[i].pos.y);
        }

        SortPointsByY(screenSpacePoints);

        if(texture)
        {            
            if(flags & PerspectiveCorrect)
                DrawTriangleSplitPerspectiveCorrect(screenSpacePoints, texture, flags);
            else
                DrawTriangleSplitLinear(screenSpacePoints, texture, flags);
        }
        else
            DrawTriangleSplitFlat(screenSpacePoints, color, flags);
    }

    bool Render::IsTriangleFrontface(const Vertex2d screenSpacePoints[])
    {
        fp x1 = (screenSpacePoints[0].pos.x - screenSpacePoints[1].pos.x);
        fp y1 = (screenSpacePoints[1].pos.y - screenSpacePoints[0].pos.y);

        fp x2 = (screenSpacePoints[1].pos.x - screenSpacePoints[2].pos.x);
        fp y2 = (screenSpacePoints[2].pos.y - screenSpacePoints[1].pos.y);

        return ((x1 * y2) - (y1 * x2)) > 0;
    }

    bool Render::IsTriangleOnScreen(const Vertex2d screenSpacePoints[])
    {
        fp lowx = 1, highx = -1, lowy = 1, highy = -1;

        for(int i = 0; i < 3; i++)
        {
            fp x = screenSpacePoints[i].pos.x;
            fp y = screenSpacePoints[i].pos.y;

            if(x < lowx)
                lowx = x;

            if(x > highx)
                highx = x;

            if(y < lowy)
                lowy = y;

            if(y > highy)
                highy = y;
        }

        if((lowx >= 1) || (highx <= -1) || (lowy >= 1) || (highy < -1))
            return false;

        if(lowx >= highx || lowy >= highy)
            return false;

        return true;
    }

    void Render::DrawTriangleSplitPerspectiveCorrect(Vertex2d *points, const Texture *texture, const RenderFlags flags)
    {

        points[0].toPerspectiveCorrect();
        points[1].toPerspectiveCorrect();
        points[2].toPerspectiveCorrect();

        if(points[1].pos.y == points[2].pos.y)
        {
            DrawTriangleTopPerspectiveCorrect(points, texture, flags);
        }
        else if(points[0].pos.y == points[1].pos.y)
        {
            DrawTriangleBottomPerspectiveCorrect(points, texture, flags);
        }
        else
        {
            //Now we split the polygon into two triangles.
            //A flat top and flat bottom triangle.

            //How far down between vx0 -> vx2 are we spliting?
            fp splitFrac = (points[1].pos.y - points[0].pos.y) / (points[2].pos.y - points[0].pos.y);

            //Interpolate new values for new vertex.
            Vertex2d triangle[4];

            triangle[0] = points[0];
            triangle[1] = points[1];

            //x pos
            triangle[2].pos.x = pLerp(points[0].pos.x, points[2].pos.x, splitFrac);
            triangle[2].pos.y = points[1].pos.y;
            triangle[2].pos.z = pLerp(points[0].pos.z, points[2].pos.z, splitFrac);
            triangle[2].pos.w = pLerp(points[0].pos.w, points[2].pos.w, splitFrac);

            //uv coords.
            triangle[2].uv.x = pLerp(points[0].uv.x, points[2].uv.x, splitFrac);
            triangle[2].uv.y = pLerp(points[0].uv.y, points[2].uv.y, splitFrac);

            triangle[3] = points[2];

            DrawTriangleTopPerspectiveCorrect(triangle, texture, flags);

            DrawTriangleBottomPerspectiveCorrect(&triangle[1], texture, flags);
        }
    }

    void Render::DrawTriangleSplitLinear(const Vertex2d points[], const Texture* texture, const RenderFlags flags)
    {
        if(points[1].pos.y == points[2].pos.y)
        {
            DrawTriangleTopLinear(points, texture, flags);
        }
        else if(points[0].pos.y == points[1].pos.y)
        {
            DrawTriangleBottomLinear(points, texture, flags);
        }
        else
        {
            //Now we split the polygon into two triangles.
            //A flat top and flat bottom triangle.

            //How far down between vx0 -> vx2 are we spliting?
            fp splitFrac = (points[1].pos.y - points[0].pos.y) / (points[2].pos.y - points[0].pos.y);

            //Interpolate new values for new vertex.
            Vertex2d triangle[4];

            triangle[0] = points[0];
            triangle[1] = points[1];

            //x pos
            triangle[2].pos.x = pLerp(points[0].pos.x, points[2].pos.x, splitFrac);
            triangle[2].pos.y = points[1].pos.y;
            triangle[2].pos.z = pLerp(points[0].pos.z, points[2].pos.z, splitFrac);

            //uv coords.
            triangle[2].uv.x = pLerp(points[0].uv.x, points[2].uv.x, splitFrac);
            triangle[2].uv.y = pLerp(points[0].uv.y, points[2].uv.y, splitFrac);

            triangle[3] = points[2];

            DrawTriangleTopLinear(triangle, texture, flags);

            DrawTriangleBottomLinear(&triangle[1], texture, flags);
        }
    }

    void Render::DrawTriangleSplitFlat(const Vertex2d points[], const pixel color, const RenderFlags flags)
    {
        if(points[1].pos.y == points[2].pos.y)
        {
            DrawTriangleTopFlat(points, color, flags);
        }
        else if(points[0].pos.y == points[1].pos.y)
        {
            DrawTriangleBottomFlat(points, color, flags);
        }
        else
        {
            //Now we split the polygon into two triangles.
            //A flat top and flat bottom triangle.

            //How far down between vx0 -> vx2 are we spliting?
            fp splitFrac = (points[1].pos.y - points[0].pos.y) / (points[2].pos.y - points[0].pos.y);

            //Interpolate new values for new vertex.
            Vertex2d triangle[4];

            triangle[0] = points[0];
            triangle[1] = points[1];

            //x pos
            triangle[2].pos.x = pLerp(points[0].pos.x, points[2].pos.x, splitFrac);
            triangle[2].pos.y = points[1].pos.y;
            triangle[2].pos.z = pLerp(points[0].pos.z, points[2].pos.z, splitFrac);

            triangle[3] = points[2];

            DrawTriangleTopFlat(triangle, color, flags);

            DrawTriangleBottomFlat(&triangle[1], color, flags);
        }
    }

    void Render::SortPointsByY(Vertex2d points[])
    {
        if(points[0].pos.y > points[1].pos.y)
            std::swap(points[0], points[1]);

        if(points[0].pos.y > points[2].pos.y)
            std::swap(points[0], points[2]);

        if(points[1].pos.y > points[2].pos.y)
            std::swap(points[1], points[2]);
    }

    void Render::DrawTriangleTopPerspectiveCorrect(const Vertex2d *points, const Texture *texture, const RenderFlags flags)
    {
        TriEdgeTrace pos;

        const Vertex2d *top, *left, *right;
        top = &points[0];

        if(points[1].pos.x < points[2].pos.x)
        {
            left = &points[1];
            right = &points[2];
        }
        else
        {
            left = &points[2];
            right = &points[1];
        }

        if((top->pos.x >= fbSize.x && left->pos.x >= fbSize.x) || (right->pos.x < 0 && top->pos.x < 0))
            return;

        int yStart = top->pos.y;
        int yEnd = left->pos.y;

        if(yEnd < 0 || yStart > fbSize.y)
            return;

        fp inv_height = 0;

        if(yEnd > yStart)
            inv_height = (fp(1 << yFracShift)/(yEnd - yStart));

        fp yFracScaled;

        if(yStart < 0)
        {
            yFracScaled = (fp(-yStart) * inv_height);
            yStart = 0;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif

            LerpEdgeXZWUV(pos, *left, *right, *top, yFrac);
        }
        else
        {
            pos.x_left = pos.x_right = top->pos.x;
            pos.z_left = pos.z_right = top->pos.z;
            pos.w_left = pos.w_right = top->pos.w;

            pos.u_left = pos.u_right = top->uv.x;
            pos.v_left = pos.v_right = top->uv.y;

            yFracScaled = 0;
        }

        if(yEnd >= fbSize.y)
            yEnd = fbSize.y-1;

        for (int y = yStart; y <= yEnd; y++)
        {
            DrawTriangleScanlinePerspectiveCorrect(y, pos, texture, flags);

            yFracScaled += inv_height;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif
            LerpEdgeXZWUV(pos, *left, *right, *top, yFrac);
        }
    }

    void Render::DrawTriangleTopLinear(const Vertex2d points[], const Texture* texture, const RenderFlags flags)
    {
        TriEdgeTrace pos;

        const Vertex2d *top, *left, *right;
        top = &points[0];

        if(points[1].pos.x < points[2].pos.x)
        {
            left = &points[1];
            right = &points[2];
        }
        else
        {
            left = &points[2];
            right = &points[1];
        }

        if((top->pos.x >= fbSize.x && left->pos.x >= fbSize.x) || (right->pos.x < 0 && top->pos.x < 0))
            return;

        int yStart = top->pos.y;
        int yEnd = left->pos.y;

        if(yEnd < 0 || yStart > fbSize.y)
            return;

        fp inv_height = 0;

        if(yEnd > yStart)
            inv_height = (fp(1 << yFracShift)/(yEnd - yStart));

        fp yFracScaled;

        if(yStart < 0)
        {
            yFracScaled = (fp(-yStart) * inv_height);
            yStart = 0;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif

            LerpEdgeXZUV(pos, *left, *right, *top, yFrac);
        }
        else
        {
            pos.x_left = pos.x_right = top->pos.x;
            pos.z_left = pos.z_right = top->pos.z;

            pos.u_left = pos.u_right = top->uv.x;
            pos.v_left = pos.v_right = top->uv.y;

            yFracScaled = 0;
        }

        if(yEnd >= fbSize.y)
            yEnd = fbSize.y-1;

        for (int y = yStart; y <= yEnd; y++)
        {
            DrawTriangleScanlineLinear(y, pos, texture, flags);

            yFracScaled += inv_height;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif
            LerpEdgeXZUV(pos, *left, *right, *top, yFrac);
        }
    }

    void Render::DrawTriangleTopFlat(const Vertex2d points[], const pixel color, const RenderFlags flags)
    {
        TriEdgeTrace pos;

        const Vertex2d *top, *left, *right;
        top = &points[0];

        if(points[1].pos.x < points[2].pos.x)
        {
            left = &points[1];
            right = &points[2];
        }
        else
        {
            left = &points[2];
            right = &points[1];
        }

        if((top->pos.x >= fbSize.x && left->pos.x >= fbSize.x) || (right->pos.x < 0 && top->pos.x < 0))
            return;

        int yStart = top->pos.y;
        int yEnd = left->pos.y;

        if(yEnd < 0 || yStart > fbSize.y)
            return;

        fp inv_height = 0;

        if(yEnd > yStart)
            inv_height = (fp(1 << yFracShift)/(yEnd - yStart));

        fp yFracScaled;

        if(yStart < 0)
        {
            yFracScaled = (fp(-yStart) * inv_height);
            yStart = 0;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif

            LerpEdgeXZ(pos, *left, *right, *top, yFrac);
        }
        else
        {
            pos.x_left = pos.x_right = top->pos.x;
            pos.z_left = pos.z_right = top->pos.z;

            yFracScaled = 0;
        }

        if(yEnd >= fbSize.y)
            yEnd = fbSize.y-1;

        for (int y = yStart; y <= yEnd; y++)
        {
            DrawTriangleScanlineFlat(y, pos, color, flags);

            yFracScaled += inv_height;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif
            LerpEdgeXZ(pos, *left, *right, *top, yFrac);
        }
    }

    void Render::DrawTriangleBottomPerspectiveCorrect(const Vertex2d *points, const Texture *texture, const RenderFlags flags)
    {
        TriEdgeTrace pos;

        const Vertex2d *bottom, *left, *right;
        bottom = &points[2];

        if(points[0].pos.x < points[1].pos.x)
        {
            left = &points[0];
            right = &points[1];
        }
        else
        {
            left = &points[1];
            right = &points[0];
        }

        if((bottom->pos.x >= fbSize.x && left->pos.x >= fbSize.x) || (right->pos.x < 0 && bottom->pos.x < 0))
            return;

        int yStart = bottom->pos.y;
        int yEnd = left->pos.y;

        if(yStart < 0 || yEnd >= fbSize.y)
            return;

        fp inv_height = (fp(1 << yFracShift)/(bottom->pos.y - left->pos.y));

        fp yFracScaled;

        if(yStart >= fbSize.y)
        {
            yFracScaled = (fp(yStart-(fbSize.y-1)) * inv_height);
            yStart = fbSize.y-1;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif

            LerpEdgeXZWUV(pos, *left, *right, *bottom, yFrac);
        }
        else
        {
            pos.x_left = pos.x_right = bottom->pos.x;
            pos.z_left = pos.z_right = bottom->pos.z;
            pos.w_left = pos.w_right = bottom->pos.w;

            pos.u_left = pos.u_right = bottom->uv.x;
            pos.v_left = pos.v_right = bottom->uv.y;

            yFracScaled = 0;
        }

        if(yEnd < 0)
            yEnd = 0;


        for (int y = yStart; y >= yEnd; y--)
        {
            DrawTriangleScanlinePerspectiveCorrect(y, pos, texture, flags);

            yFracScaled += inv_height;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif
            LerpEdgeXZWUV(pos, *left, *right, *bottom, yFrac);
        }
    }

    void Render::DrawTriangleBottomLinear(const Vertex2d points[], const Texture* texture, const RenderFlags flags)
    {
        TriEdgeTrace pos;

        const Vertex2d *bottom, *left, *right;
        bottom = &points[2];

        if(points[0].pos.x < points[1].pos.x)
        {
            left = &points[0];
            right = &points[1];
        }
        else
        {
            left = &points[1];
            right = &points[0];
        }

        if((bottom->pos.x >= fbSize.x && left->pos.x >= fbSize.x) || (right->pos.x < 0 && bottom->pos.x < 0))
            return;

        int yStart = bottom->pos.y;
        int yEnd = left->pos.y;

        if(yStart < 0 || yEnd >= fbSize.y)
            return;

        fp inv_height = (fp(1 << yFracShift)/(bottom->pos.y - left->pos.y));

        fp yFracScaled;

        if(yStart >= fbSize.y)
        {
            yFracScaled = (fp(yStart-(fbSize.y-1)) * inv_height);
            yStart = fbSize.y-1;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif

            LerpEdgeXZUV(pos, *left, *right, *bottom, yFrac);
        }
        else
        {
            pos.x_left = pos.x_right = bottom->pos.x;
            pos.z_left = pos.z_right = bottom->pos.z;

            pos.u_left = pos.u_right = bottom->uv.x;
            pos.v_left = pos.v_right = bottom->uv.y;

            yFracScaled = 0;
        }

        if(yEnd < 0)
            yEnd = 0;


        for (int y = yStart; y >= yEnd; y--)
        {
            DrawTriangleScanlineLinear(y, pos, texture, flags);

            yFracScaled += inv_height;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif
            LerpEdgeXZUV(pos, *left, *right, *bottom, yFrac);
        }
    }

    void Render::DrawTriangleBottomFlat(const Vertex2d points[], const pixel color, const RenderFlags flags)
    {
        TriEdgeTrace pos;

        const Vertex2d *bottom, *left, *right;
        bottom = &points[2];

        if(points[0].pos.x < points[1].pos.x)
        {
            left = &points[0];
            right = &points[1];
        }
        else
        {
            left = &points[1];
            right = &points[0];
        }

        if((bottom->pos.x >= fbSize.x && left->pos.x >= fbSize.x) || (right->pos.x < 0 && bottom->pos.x < 0))
            return;

        int yStart = bottom->pos.y;
        int yEnd = left->pos.y;

        if(yStart < 0 || yEnd >= fbSize.y)
            return;

        fp inv_height = (fp(1 << yFracShift)/(bottom->pos.y - left->pos.y));

        fp yFracScaled;

        if(yStart >= fbSize.y)
        {
            yFracScaled = (fp(yStart-(fbSize.y-1)) * inv_height);
            yStart = fbSize.y-1;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif

            LerpEdgeXZ(pos, *left, *right, *bottom, yFrac);
        }
        else
        {
            pos.x_left = pos.x_right = bottom->pos.x;
            pos.z_left = pos.z_right = bottom->pos.z;

            yFracScaled = 0;
        }

        if(yEnd < 0)
            yEnd = 0;


        for (int y = yStart; y >= yEnd; y--)
        {
            DrawTriangleScanlineFlat(y, pos, color, flags);

            yFracScaled += inv_height;

#ifndef USE_FLOAT
            fp yFrac = yFracScaled >> yFracShift;
#else
            fp yFrac = yFracScaled / (1 << yFracShift);
#endif
            LerpEdgeXZ(pos, *left, *right, *bottom, yFrac);
        }
    }

    void Render::DrawTriangleScanlinePerspectiveCorrect(int y, const TriEdgeTrace &pos, const Texture *texture, const RenderFlags flags)
    {
        TriDrawPos sl_pos, sl_delta;

        int x_start = pos.x_left;
        int x_end = pos.x_right+1;

        if( (x_end <= x_start) || (x_end <= 0) || (x_start >= fbSize.x) )
            return;

        sl_pos.z =  pos.z_left;
        sl_pos.w =  pos.w_left;
        sl_pos.u =  pos.u_left;
        sl_pos.v =  pos.v_left;

        fp inv_width = fp(1)/(x_end - x_start);

        sl_delta.z = ((pos.z_right - pos.z_left) * inv_width);
        sl_delta.w = ((pos.w_right - pos.w_left) * inv_width);
        sl_delta.u = ((pos.u_right - pos.u_left) * inv_width);
        sl_delta.v = ((pos.v_right - pos.v_left) * inv_width);

        while(x_start < 0)
        {
            sl_pos.z += sl_delta.z;
            sl_pos.w += sl_delta.w;
            sl_pos.u += sl_delta.u;
            sl_pos.v += sl_delta.v;

            x_start++;
        }

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        int count = (x_end - x_start);

        int buffOffset = ((y * fbSize.x) + x_start);
        fp* zb = &zBuffer[buffOffset];
        pixel* fb = &frameBuffer[buffOffset];

        while(count-- > 0)
        {
            if(sl_pos.z < *zb)
            {
                fp invw = fp(1) / sl_pos.w;

                int tx = sl_pos.u * invw;
                int ty = sl_pos.v * invw;

                tx = tx & (texture->width - 1);
                ty = ty & (texture->height - 1);

                *fb = texture->pixels[ty * texture->width + tx];
                *zb = sl_pos.z;
            }

            zb++;
            fb++;

            sl_pos.z += sl_delta.z;
            sl_pos.w += sl_delta.w;
            sl_pos.u += sl_delta.u;
            sl_pos.v += sl_delta.v;
        }
    }

    void Render::DrawTriangleScanlineLinear(int y, const TriEdgeTrace& pos, const Texture* texture, const RenderFlags flags)
    {
        TriDrawPos sl_pos, sl_delta;

        int x_start = pos.x_left;
        int x_end = pos.x_right+1;

        if( (x_end <= x_start) || (x_end <= 0) || (x_start >= fbSize.x) )
            return;

        sl_pos.z =  pos.z_left;
        sl_pos.u =  pos.u_left;
        sl_pos.v =  pos.v_left;

        fp inv_width = fp(1)/(x_end - x_start);

        sl_delta.z = ((pos.z_right - pos.z_left) * inv_width);
        sl_delta.u = ((pos.u_right - pos.u_left) * inv_width);
        sl_delta.v = ((pos.v_right - pos.v_left) * inv_width);

        while(x_start < 0)
        {
            sl_pos.z += sl_delta.z;
            sl_pos.u += sl_delta.u;
            sl_pos.v += sl_delta.v;

            x_start++;
        }

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        int count = (x_end - x_start);

        int buffOffset = ((y * fbSize.x) + x_start);
        fp* zb = &zBuffer[buffOffset];
        pixel* fb = &frameBuffer[buffOffset];

        while(count-- > 0)
        {
            if(sl_pos.z < *zb)
            {
                int tx = sl_pos.u;
                int ty = sl_pos.v;

                tx = tx & (texture->u_mask);
                ty = ty & (texture->v_mask);

                *fb = texture->pixels[(ty << texture->v_shift) | tx];

                *zb = sl_pos.z;
            }

            zb++;
            fb++;

            sl_pos.z += sl_delta.z;
            sl_pos.u += sl_delta.u;
            sl_pos.v += sl_delta.v;
        }
    }

    void Render::DrawTriangleScanlineFlat(int y, const TriEdgeTrace& pos, const pixel color, const RenderFlags flags)
    {
        TriDrawPos sl_pos, sl_delta;

        int x_start = pos.x_left;
        int x_end = pos.x_right+1;

        if( (x_end <= x_start) || (x_end <= 0) || (x_start >= fbSize.x) )
            return;

        sl_pos.z =  pLSL(pos.z_left, xFracShift);

        fp inv_width = fp(1 << xFracShift)/(x_end - x_start);

        sl_delta.z = ((pos.z_right - pos.z_left) * inv_width);

        while(x_start < 0)
        {
            sl_pos.z += sl_delta.z;

            x_start++;
        }

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        int count = (x_end - x_start);

        int buffOffset = ((y * fbSize.x) + x_start);
        fp* zb = &zBuffer[buffOffset];
        pixel* fb = &frameBuffer[buffOffset];

        while(count-- > 0)
        {
            fp z_pos = pLSR(sl_pos.z, xFracShift);


            if(z_pos < *zb)
            {
                *fb = color;
                *zb = z_pos;
            }

            zb++;
            fb++;

            sl_pos.z += sl_delta.z;
        }
    }

    void Render::LerpEdgePosZWUV(TriDrawPos& out, const TriEdgeTrace& edge, fp frac)
    {
        out.z = pLerp(edge.z_left, edge.z_right, frac);
        out.w = pLerp(edge.w_left, edge.w_right, frac);
        out.u = pLerp(edge.u_left, edge.u_right, frac);
        out.v = pLerp(edge.v_left, edge.v_right, frac);
    }

    void Render::LerpEdgePosZUV(TriDrawPos& out, const TriEdgeTrace &edge, fp frac)
    {
        out.z = pLerp(edge.z_left, edge.z_right, frac);
        out.u = pLerp(edge.u_left, edge.u_right, frac);
        out.v = pLerp(edge.v_left, edge.v_right, frac);
    }

    void Render::LerpEdgeXZWUV(TriEdgeTrace& out, const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, fp frac)
    {
        out.x_left = pLerp(other.pos.x, left.pos.x, frac);
        out.x_right = pLerp(other.pos.x, right.pos.x, frac);

        out.z_left = pLerp(other.pos.z, left.pos.z, frac);
        out.z_right = pLerp(other.pos.z, right.pos.z, frac);

        out.w_left = pLerp(other.pos.w, left.pos.w, frac);
        out.w_right = pLerp(other.pos.w, right.pos.w, frac);

        out.u_left = pLerp(other.uv.x, left.uv.x, frac);
        out.u_right = pLerp(other.uv.x, right.uv.x, frac);

        out.v_left = pLerp(other.uv.y, left.uv.y, frac);
        out.v_right = pLerp(other.uv.y, right.uv.y, frac);
    }

    void Render::LerpEdgeXZUV(TriEdgeTrace& out, const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, fp frac)
    {
        out.x_left = pLerp(other.pos.x, left.pos.x, frac);
        out.x_right = pLerp(other.pos.x, right.pos.x, frac);

        out.z_left = pLerp(other.pos.z, left.pos.z, frac);
        out.z_right = pLerp(other.pos.z, right.pos.z, frac);

        out.u_left = pLerp(other.uv.x, left.uv.x, frac);
        out.u_right = pLerp(other.uv.x, right.uv.x, frac);

        out.v_left = pLerp(other.uv.y, left.uv.y, frac);
        out.v_right = pLerp(other.uv.y, right.uv.y, frac);
    }

    void Render::LerpEdgeXZ(TriEdgeTrace& out, const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, fp frac)
    {
        out.x_left = pLerp(other.pos.x, left.pos.x, frac);
        out.x_right = pLerp(other.pos.x, right.pos.x, frac);

        out.z_left = pLerp(other.pos.z, left.pos.z, frac);
        out.z_right = pLerp(other.pos.z, right.pos.z, frac);
    }

    void Render::LerpVertexXYZWUV(Vertex2d& out, const Vertex2d& left, const Vertex2d& right, fp frac)
    {
        out.pos.x = pLerp(left.pos.x, right.pos.x, frac);
        out.pos.y = pLerp(left.pos.y, right.pos.y, frac);
        out.pos.z = pLerp(left.pos.z, right.pos.z, frac);
        out.pos.w = pLerp(left.pos.w, right.pos.w, frac);

        out.uv.x = pLerp(left.uv.x, right.uv.x, frac);
        out.uv.y = pLerp(left.uv.y, right.uv.y, frac);
    }

    int Render::fracToY(fp frac)
    {
        fp y = fp(2)-(frac + fp(1));

#ifdef USE_FLOAT
        int sy = (y * fbSize.y) / 2;
#else
        int sy = y.intMul(fbSize.y) >> 1;

        if(sy < FP::min())
            return FP::min();
        else if(sy > FP::max())
            return FP::max();
#endif

        return sy;
    }

    int Render::fracToX(fp frac)
    {
        fp x = frac + fp(1);

#ifdef USE_FLOAT
        int sx = (x * fbSize.x) / 2;
#else
        int sx = x.intMul(fbSize.x) >> 1;

        if(sx < FP::min())
            return FP::min();
        else if(sx > FP::max())
            return FP::max();
#endif

        return sx;
    }
}
