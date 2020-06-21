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
        const fp wClip = zNear;

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
        if(w0 < wClip && w1 < wClip && w2 < wClip)
            return;

        //All points in valid space.
        if(w0 >= wClip && w1 >= wClip && w2 >= wClip)
        {
            DrawTriangleCull(clipSpacePoints, texture, color, flags);
            return;
        }

        Vertex2d outputVx[4];
        int vp = 0;

        //qDebug() << w0 << w1 << w2;

        for(int i = 0; i < 3; i++)
        {
            if(clipSpacePoints[i].pos.w >= wClip)
            {
                outputVx[vp] = clipSpacePoints[i];
                vp++;
            }

            int i2 = i < 2 ? i+1 : 0;

            fp frac = GetLineIntersection(clipSpacePoints[i].pos.w, clipSpacePoints[i2].pos.w, wClip);

            if(frac > 0)
            {
                //qDebug() << "Clipfrac = " << frac;

                Vertex2d newVx;

                newVx.pos.x = pLerp(clipSpacePoints[i].pos.x, clipSpacePoints[i2].pos.x, frac);
                newVx.pos.y = pLerp(clipSpacePoints[i].pos.y, clipSpacePoints[i2].pos.y, frac);
                newVx.pos.z = pLerp(clipSpacePoints[i].pos.z, clipSpacePoints[i2].pos.z, frac);
                newVx.pos.w = wClip;

                newVx.uv.x = pLerp(clipSpacePoints[i].uv.x, clipSpacePoints[i2].uv.x, frac);
                newVx.uv.y = pLerp(clipSpacePoints[i].uv.y, clipSpacePoints[i2].uv.y, frac);

                outputVx[vp] = newVx;
                vp++;
            }
        }

        if(vp == 3)
        {
            DrawTriangleCull(outputVx, texture, color, flags);
        }
        else if(vp == 4)
        {
            DrawTriangleCull(outputVx, texture, color, flags);
            outputVx[1] = outputVx[0];
            DrawTriangleCull(&outputVx[1], texture, color, flags);
        }
    }


    //Return -1 == both <= pos.
    //Return -2 == both >= pos.
    fp Render::GetLineIntersection(fp v1, fp v2, const fp pos)
    {
        if(v1 >= pos && v2 >= pos)
            return -2;
        else if(v1 <= pos && v2 <= pos)
            return -1;
        else if(v1 == v2)
        {
            if(v1 >= pos)
                return -2;

            return -1;
        }
        else if(v1 > v2)
        {
            fp len = (v1 - v2);

            fp splitFrac = (v1 - pos) / len;

            return splitFrac;
        }

        fp len = (v2 - v1);

        fp splitFrac = (v2 - pos) / len;

        return fp(1) - splitFrac;
    }


    void Render::DrawTriangleCull(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        Vertex2d screenSpacePoints[3];

        for(int i = 0; i < 3; i++)
        {
            screenSpacePoints[i].pos = clipSpacePoints[i].pos.ToScreenSpace();

            screenSpacePoints[i].pos.x = fracToX(screenSpacePoints[i].pos.x);
            screenSpacePoints[i].pos.y = fracToY(screenSpacePoints[i].pos.y);

            screenSpacePoints[i].uv = clipSpacePoints[i].uv;
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
        int x1 = (screenSpacePoints[0].pos.x - screenSpacePoints[1].pos.x);
        int y1 = (screenSpacePoints[0].pos.y - screenSpacePoints[1].pos.y);

        int x2 = (screenSpacePoints[1].pos.x - screenSpacePoints[2].pos.x);
        int y2 = (screenSpacePoints[1].pos.y - screenSpacePoints[2].pos.y);

        return ((x1 * y2) - (y1 * x2)) > 0;
    }

    bool Render::IsTriangleOnScreen(const Vertex2d screenSpacePoints[])
    {
        int lowx = fbSize.x, highx = -1, lowy = fbSize.y, highy = -1;

        for(int i = 0; i < 3; i++)
        {
            int x = screenSpacePoints[i].pos.x;
            int y = screenSpacePoints[i].pos.y;

            if(x < lowx)
                lowx = x;

            if(x > highx)
                highx = x;

            if(y < lowy)
                lowy = y;

            if(y > highy)
                highy = y;
        }

        if(lowx == highx || lowy == highy)
            return false;

        if((lowx >= fbSize.x) || (highx < 0) || (lowy >= fbSize.y) || (highy < 0))
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

        const fp yFracScale = 1024;
        fp inv_height = (fp(yFracScale)/(points[1].pos.y - points[0].pos.y));
        fp yFracScaled = inv_height;

        int yStart = top->pos.y;
        int yEnd = left->pos.y;

        if(yStart < 0)
        {
            yFracScaled = (fp(-yStart) * inv_height);
            yStart = 0;

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(top->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(top->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(top->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(top->pos.z, right->pos.z, yFrac);

            pos.w_left = pLerp(top->pos.w, left->pos.w, yFrac);
            pos.w_right = pLerp(top->pos.w, right->pos.w, yFrac);

            pos.u_left = pLerp(top->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(top->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(top->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(top->uv.y, right->uv.y, yFrac);
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

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(top->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(top->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(top->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(top->pos.z, right->pos.z, yFrac);

            pos.w_left = pLerp(top->pos.w, left->pos.w, yFrac);
            pos.w_right = pLerp(top->pos.w, right->pos.w, yFrac);

            pos.u_left = pLerp(top->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(top->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(top->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(top->uv.y, right->uv.y, yFrac);
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

        const fp yFracScale = 1024;
        fp inv_height = (fp(yFracScale)/(points[1].pos.y - points[0].pos.y));
        fp yFracScaled = inv_height;

        int yStart = top->pos.y;
        int yEnd = left->pos.y;

        if(yStart < 0)
        {
            yFracScaled = (fp(-yStart) * inv_height);
            yStart = 0;

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(top->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(top->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(top->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(top->pos.z, right->pos.z, yFrac);

            pos.u_left = pLerp(top->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(top->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(top->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(top->uv.y, right->uv.y, yFrac);
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

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(top->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(top->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(top->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(top->pos.z, right->pos.z, yFrac);

            pos.u_left = pLerp(top->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(top->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(top->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(top->uv.y, right->uv.y, yFrac);
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

        const fp yFracScale = 1024;
        fp inv_height = (fp(yFracScale)/(points[1].pos.y - points[0].pos.y));
        fp yFracScaled = inv_height;

        int yStart = top->pos.y;
        int yEnd = left->pos.y;

        if(yStart < 0)
        {
            yFracScaled = (fp(-yStart) * inv_height);
            yStart = 0;

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(top->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(top->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(top->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(top->pos.z, right->pos.z, yFrac);
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

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(top->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(top->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(top->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(top->pos.z, right->pos.z, yFrac);
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

        const fp yFracScale = 1024;
        fp inv_height = (fp(yFracScale)/(bottom->pos.y - left->pos.y));

        fp yFracScaled;

        int yStart = bottom->pos.y;
        int yEnd = left->pos.y;

        if(yStart >= fbSize.y)
        {
            yFracScaled = (fp(yStart-(fbSize.y-1)) * inv_height);
            yStart = fbSize.y-1;

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(bottom->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(bottom->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(bottom->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(bottom->pos.z, right->pos.z, yFrac);

            pos.w_left = pLerp(bottom->pos.w, left->pos.w, yFrac);
            pos.w_right = pLerp(bottom->pos.w, right->pos.w, yFrac);

            pos.u_left = pLerp(bottom->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(bottom->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(bottom->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(bottom->uv.y, right->uv.y, yFrac);
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

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(bottom->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(bottom->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(bottom->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(bottom->pos.z, right->pos.z, yFrac);

            pos.w_left = pLerp(bottom->pos.w, left->pos.w, yFrac);
            pos.w_right = pLerp(bottom->pos.w, right->pos.w, yFrac);

            pos.u_left = pLerp(bottom->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(bottom->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(bottom->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(bottom->uv.y, right->uv.y, yFrac);
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

        const fp yFracScale = 1024;
        fp inv_height = (fp(yFracScale)/(bottom->pos.y - left->pos.y));

        fp yFracScaled;

        int yStart = bottom->pos.y;
        int yEnd = left->pos.y;

        if(yStart >= fbSize.y)
        {
            yFracScaled = (fp(yStart-(fbSize.y-1)) * inv_height);
            yStart = fbSize.y-1;

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(bottom->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(bottom->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(bottom->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(bottom->pos.z, right->pos.z, yFrac);

            pos.u_left = pLerp(bottom->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(bottom->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(bottom->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(bottom->uv.y, right->uv.y, yFrac);
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

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(bottom->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(bottom->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(bottom->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(bottom->pos.z, right->pos.z, yFrac);

            pos.u_left = pLerp(bottom->uv.x, left->uv.x, yFrac);
            pos.u_right = pLerp(bottom->uv.x, right->uv.x, yFrac);

            pos.v_left = pLerp(bottom->uv.y, left->uv.y, yFrac);
            pos.v_right = pLerp(bottom->uv.y, right->uv.y, yFrac);
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

        const fp yFracScale = 1024;
        fp inv_height = (fp(yFracScale)/(bottom->pos.y - left->pos.y));

        fp yFracScaled;

        int yStart = bottom->pos.y;
        int yEnd = left->pos.y;

        if(yStart >= fbSize.y)
        {
            yFracScaled = (fp(yStart-(fbSize.y-1)) * inv_height);
            yStart = fbSize.y-1;

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(bottom->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(bottom->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(bottom->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(bottom->pos.z, right->pos.z, yFrac);
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

            fp yFrac = yFracScaled / yFracScale;

            pos.x_left = pLerp(bottom->pos.x, left->pos.x, yFrac);
            pos.x_right = pLerp(bottom->pos.x, right->pos.x, yFrac);

            pos.z_left = pLerp(bottom->pos.z, left->pos.z, yFrac);
            pos.z_right = pLerp(bottom->pos.z, right->pos.z, yFrac);
        }
    }

    void Render::DrawTriangleScanlinePerspectiveCorrect(int y, const TriEdgeTrace &pos, const Texture *texture, const RenderFlags flags)
    {
        TriDrawPos sl_pos;

        int x_start = pos.x_left;
        int x_end = pos.x_right;

        fp inv_width = 0;
        const fp xFracScale = 1024;
        fp xFracScaled;

        if(x_start < x_end)
        {
            inv_width = fp(xFracScale)/(x_end - x_start);
        }

        if(x_start < 0)
        {
            xFracScaled = (fp(-x_start) * inv_width);
            x_start = 0;

            fp xFrac = xFracScaled / xFracScale;

            sl_pos.z = pLerp(pos.z_left, pos.z_right, xFrac);
            sl_pos.w = pLerp(pos.w_left, pos.w_right, xFrac);
            sl_pos.u = pLerp(pos.u_left, pos.u_right, xFrac);
            sl_pos.v = pLerp(pos.v_left, pos.v_right, xFrac);
        }
        else
        {
            sl_pos.w = pos.w_left;
            sl_pos.z = pos.z_left;
            sl_pos.u = pos.u_left;
            sl_pos.v = pos.v_left;

            xFracScaled = 0;
        }

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        int buffOffset = ((y * fbSize.x) + x_start);
        fp* zb = &zBuffer[buffOffset];
        pixel* fb = &frameBuffer[buffOffset];

        for(int x = x_start; x <= x_end; x++)
        {
            if(sl_pos.z < *zb)
            {
                fp invw = fp(1) / sl_pos.w;

                int tx = ((sl_pos.u * fp((int)texture->width)) * invw);
                int ty = fp((int)texture->height) - ((sl_pos.v * fp((int)texture->height)) * invw);


                tx = tx & (texture->width - 1);
                ty = ty & (texture->height - 1);

                *fb = texture->pixels[ty * texture->width + tx];
                *zb = sl_pos.z;
            }

            xFracScaled += inv_width;
            zb++;
            fb++;

            fp xFrac = xFracScaled / xFracScale;

            sl_pos.z = pLerp(pos.z_left, pos.z_right, xFrac);
            sl_pos.w = pLerp(pos.w_left, pos.w_right, xFrac);
            sl_pos.u = pLerp(pos.u_left, pos.u_right, xFrac);
            sl_pos.v = pLerp(pos.v_left, pos.v_right, xFrac);
        }
    }

    void Render::DrawTriangleScanlineLinear(int y, const TriEdgeTrace& pos, const Texture* texture, const RenderFlags flags)
    {
        TriDrawPos sl_pos;

        int x_start = pos.x_left;
        int x_end = pos.x_right;

        fp inv_width = 0;
        const fp xFracScale = 1024;
        fp xFracScaled;

        if(x_start < x_end)
        {
            inv_width = fp(xFracScale)/(x_end - x_start);
        }

        if(x_start < 0)
        {
            xFracScaled = (fp(-x_start) * inv_width);
            x_start = 0;

            fp xFrac = xFracScaled / xFracScale;

            sl_pos.z = pLerp(pos.z_left, pos.z_right, xFrac);
            sl_pos.u = pLerp(pos.u_left, pos.u_right, xFrac);
            sl_pos.v = pLerp(pos.v_left, pos.v_right, xFrac);
        }
        else
        {
            sl_pos.z = pos.z_left;
            sl_pos.u = pos.u_left;
            sl_pos.v = pos.v_left;

            xFracScaled = 0;
        }

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        int buffOffset = ((y * fbSize.x) + x_start);
        fp* zb = &zBuffer[buffOffset];
        pixel* fb = &frameBuffer[buffOffset];

        for(int x = x_start; x <= x_end; x++)
        {
            if(sl_pos.z < *zb)
            {
                int tx = sl_pos.u * (int)texture->width;
                int ty = (fp(1)-sl_pos.v) * (int)texture->height;

                tx = tx & (texture->width - 1);
                ty = ty & (texture->height - 1);

                *fb = texture->pixels[ty * texture->width + tx];
                *zb = sl_pos.z;
            }

            xFracScaled += inv_width;
            zb++;
            fb++;

            fp xFrac = xFracScaled / xFracScale;

            sl_pos.z = pLerp(pos.z_left, pos.z_right, xFrac);
            sl_pos.u = pLerp(pos.u_left, pos.u_right, xFrac);
            sl_pos.v = pLerp(pos.v_left, pos.v_right, xFrac);
        }
    }

    void Render::DrawTriangleScanlineFlat(int y, const TriEdgeTrace& pos, const pixel color, const RenderFlags flags)
    {
        fp zPos;

        int x_start = pos.x_left;
        int x_end = pos.x_right;

        fp inv_width = 0;
        const fp xFracScale = 1024;
        fp xFracScaled;

        if(x_start < x_end)
        {
            inv_width = fp(xFracScale)/(x_end - x_start);
        }
        else if(x_start > x_end)
            return;

        if(x_start < 0)
        {
            xFracScaled = (fp(-x_start) * inv_width);
            x_start = 0;

            fp xFrac = xFracScaled / xFracScale;

            zPos = pLerp(pos.z_left, pos.z_right, xFrac);
        }
        else
        {
            zPos = pos.z_left;
            xFracScaled = 0;
        }

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        int buffOffset = ((y * fbSize.x) + x_start);
        fp* zb = &zBuffer[buffOffset];
        pixel* fb = &frameBuffer[buffOffset];


        for(int i = x_start; i <= x_end; i++)
        {
            if(zPos < *zb)
            {
                *fb = color;
                *zb = zPos;
            }

            xFracScaled += inv_width;
            zb++;
            fb++;

            fp xFrac = xFracScaled / xFracScale;
            zPos = pLerp(pos.z_left, pos.z_right, xFrac);

        }
    }


    int Render::fracToY(fp frac)
    {
        fp y = fp(1)-((frac + fp(1)) / fp(2));

    #ifdef USE_FLOAT
        int sy = y * fbSize.y;
    #else
        int sy = y.intMul(fbSize.y);

        if(sy < FP::min())
            return FP::min();
        else if(sy > FP::max())
            return FP::max();
    #endif

        return sy;
    }

    int Render::fracToX(fp frac)
    {
        fp x = (frac + fp(1)) / fp(2);

    #ifdef USE_FLOAT
        int sx = x * fbSize.x;
    #else
        int sx = x.intMul(fbSize.x);

        if(sx < FP::min())
            return FP::min();
        else if(sx > FP::max())
            return FP::max();
    #endif

        return sx;
    }
}
