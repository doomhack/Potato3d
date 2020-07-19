#include <algorithm>
#include "render.h"

namespace P3D
{
    Render::Render()
    {
        frameBuffer = nullptr;
        fbSize = {0,0};

        modelMatrix.setToIdentity();
        viewMatrix.setToIdentity();


        projectionMatrix.setToIdentity();
        viewProjectionMatrix.setToIdentity();

        transformMatrix.setToIdentity();
    }

    bool Render::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel* frameBuffer)
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

        fp aspectRatio = fp((int)screenWidth) / fp((int)screenHeight);

        projectionMatrix.perspective(hFov, aspectRatio, zNear, zFar);

        spanBuffer = new SpanBuffer[screenHeight];

        return true;
    }

    void Render::BeginFrame()
    {
        UpdateViewProjectionMatrix();

        for(int i = 0; i < fbSize.y; i++)
        {
            spanBuffer[i].min_opening = 0;
            spanBuffer[i].span_list.clear();
        }

#ifdef RENDER_STATS
        stats.triangles_drawn = 0;
        stats.triangles_submitted = 0;
        stats.vertex_transformed = 0;
        stats.scanlines_drawn = 0;
#endif

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

    void Render::ClearFramebuffer(pixel color)
    {
        const unsigned int buffSize = fbSize.x * fbSize.y * sizeof(pixel);
#ifdef FB_32
        FastFill32((unsigned int*)frameBuffer, color, buffSize >> 2);
#else
        unsigned int c32 = color;

        FastFill32((unsigned int*)frameBuffer, c32 | c32 << 16, buffSize >> 2);
#endif
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

            default: //Shouldn't get here. Shut up compiler.
                return this->viewMatrix;
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
#ifdef RENDER_STATS
        stats.triangles_submitted++;
#endif

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
#ifdef RENDER_STATS
        stats.vertex_transformed++;
#endif

        Vertex2d screenspace;

        screenspace.pos = transformMatrix * vertex->pos;
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

            if(frac >= 0)
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

        fp dt = (len_1 - len_2);

        return ((len_1) / dt);
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
                screenSpacePoints[i].uv.y = clipSpacePoints[i].uv.y * (int)texture->height;
            }
        }

        for(int i = 0; i < 3; i++)
        {
            screenSpacePoints[i].pos.x = fracToX(screenSpacePoints[i].pos.x);
            screenSpacePoints[i].pos.y = fracToY(screenSpacePoints[i].pos.y);
        }

        SortPointsByY(screenSpacePoints);

#ifdef RENDER_STATS
        stats.triangles_drawn++;
#endif

        if(texture)
            DrawTriangleSplit(screenSpacePoints, texture, flags);
        else
            DrawTriangleSplitFlat(screenSpacePoints, color);
    }

    void Render::DrawTriangleSplit(Vertex2d *points, const Texture *texture, RenderFlags flags)
    {
        if(flags & PerspectiveCorrect)
        {
            points[0].toPerspectiveCorrect();
            points[1].toPerspectiveCorrect();
            points[2].toPerspectiveCorrect();
        }

        if(texture->alpha)
            flags = (RenderFlags)(flags | RenderFlags::Alpha);

        if(points[1].pos.y == points[2].pos.y)
        {
            DrawTriangleTop(points, texture, flags);
        }
        else if(points[0].pos.y == points[1].pos.y)
        {
            DrawTriangleBottom(points, texture, flags);
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
            triangle[2].pos.x = pRound(pLerp(points[0].pos.x, points[2].pos.x, splitFrac));
            triangle[2].pos.y = points[1].pos.y;

            //uv coords.
            triangle[2].uv.x = pLerp(points[0].uv.x, points[2].uv.x, splitFrac);
            triangle[2].uv.y = pLerp(points[0].uv.y, points[2].uv.y, splitFrac);

            if(flags & PerspectiveCorrect)
                triangle[2].pos.w = pLerp(points[0].pos.w, points[2].pos.w, splitFrac);

            triangle[3] = points[2];

            DrawTriangleTop(triangle, texture, flags);
            DrawTriangleBottom(&triangle[1], texture, flags);
        }
    }

    void Render::DrawTriangleSplitFlat(const Vertex2d points[], const pixel color)
    {
        if(points[1].pos.y == points[2].pos.y)
        {
            DrawTriangleTopFlat(points, color);
        }
        else if(points[0].pos.y == points[1].pos.y)
        {
            DrawTriangleBottomFlat(points, color);
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
            triangle[2].pos.x = pRound(pLerp(points[0].pos.x, points[2].pos.x, splitFrac));
            triangle[2].pos.y = points[1].pos.y;

            triangle[3] = points[2];

            DrawTriangleTopFlat(triangle, color);

            DrawTriangleBottomFlat(&triangle[1], color);
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

    void Render::DrawTriangleTop(const Vertex2d *points, const Texture *texture, const RenderFlags flags)
    {
        TriEdgeTrace pos;
        TriDrawXDeltaZWUV x_delta;
        TriDrawYDeltaZWUV y_delta, y_delta_sum;

        const Vertex2d& top     = points[0];
        const Vertex2d& left    = (points[1].pos.x < points[2].pos.x) ? points[1] : points[2];
        const Vertex2d& right   = (points[1].pos.x < points[2].pos.x) ? points[2] : points[1];

        if((top.pos.x >= fbSize.x && left.pos.x >= fbSize.x) || (right.pos.x < 0 && top.pos.x < 0))
            return;

        int yStart = (top.pos.y);
        int yEnd = (left.pos.y);

        if(yEnd < 0 || yStart > fbSize.y)
            return;


        GetTriangleLerpDeltasZWUV(left, right, top, x_delta, y_delta);

        if(yStart < 0)
        {
            y_delta_sum.x_left = (y_delta.x_left * -yStart);
            y_delta_sum.x_right = (y_delta.x_right * -yStart);

            y_delta_sum.u = (y_delta.u * -yStart);
            y_delta_sum.v = (y_delta.v * -yStart);
            y_delta_sum.w = (y_delta.w * -yStart);

            yStart = 0;
        }
        else
        {
            y_delta_sum.x_left = 0;
            y_delta_sum.x_right = 0;

            y_delta_sum.w = 0;
            y_delta_sum.u = 0;
            y_delta_sum.v = 0;

        }

        if(yEnd >= fbSize.y)
            yEnd = fbSize.y-1;

        for (int y = yStart; y <= yEnd; y++)
        {
            pos.x_left = top.pos.x + pASR(y_delta_sum.x_left, triFracShift);
            pos.x_right = top.pos.x + pASR(y_delta_sum.x_right, triFracShift);

            pos.u_left = top.uv.x + pASR(y_delta_sum.u, triFracShift);
            pos.v_left = top.uv.y + pASR(y_delta_sum.v, triFracShift);
            pos.w_left = top.pos.w + pASR(y_delta_sum.w, triFracShift);

            ClipSpan(y, pos, x_delta, texture, 0, flags);

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;
            y_delta_sum.w += y_delta.w;

            y_delta_sum.u += y_delta.u;
            y_delta_sum.v += y_delta.v;
        }
    }

    void Render::DrawTriangleTopFlat(const Vertex2d points[], const pixel color)
    {
        TriEdgeTrace pos;
        TriDrawYDeltaZ y_delta, y_delta_sum;

        const Vertex2d& top     = points[0];
        const Vertex2d& left    = (points[1].pos.x < points[2].pos.x) ? points[1] : points[2];
        const Vertex2d& right   = (points[1].pos.x < points[2].pos.x) ? points[2] : points[1];

        if((top.pos.x >= fbSize.x && left.pos.x >= fbSize.x) || (right.pos.x < 0 && top.pos.x < 0))
            return;

        int yStart = top.pos.y;
        int yEnd = left.pos.y;

        if(yEnd < 0 || yStart > fbSize.y)
            return;

        GetTriangleLerpDeltasZ(left, right, top, y_delta);

        if(yStart < 0)
        {
            y_delta_sum.x_left = (y_delta.x_left * -yStart);
            y_delta_sum.x_right = (y_delta.x_right * -yStart);

            yStart = 0;
        }
        else
        {
            y_delta_sum.x_left = 0;
            y_delta_sum.x_right = 0;
        }

        if(yEnd >= fbSize.y)
            yEnd = fbSize.y-1;

        for (int y = yStart; y <= yEnd; y++)
        {
            pos.x_left = top.pos.x + pASR(y_delta_sum.x_left, triFracShift);
            pos.x_right = top.pos.x + pASR(y_delta_sum.x_right, triFracShift);

            DrawTriangleScanlineFlat(y, pos, color);

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;
        }
    }

    void Render::DrawTriangleBottom(const Vertex2d *points, const Texture *texture, const RenderFlags flags)
    {
        TriEdgeTrace pos;
        TriDrawXDeltaZWUV x_delta;
        TriDrawYDeltaZWUV y_delta, y_delta_sum;

        const Vertex2d& bottom  = points[2];
        const Vertex2d& left    = (points[0].pos.x < points[1].pos.x) ? points[0] : points[1];
        const Vertex2d& right   = (points[0].pos.x < points[1].pos.x) ? points[1] : points[0];

        if((bottom.pos.x >= fbSize.x && left.pos.x >= fbSize.x) || (right.pos.x < 0 && bottom.pos.x < 0))
            return;


        int yStart = (bottom.pos.y);
        int yEnd = (left.pos.y);

        if(yStart < 0 || yEnd >= fbSize.y)
            return;

        GetTriangleLerpDeltasZWUV(left, right, bottom, x_delta, y_delta);

        if(yStart >= fbSize.y)
        {
            int overflow = yStart - (fbSize.y-1);

            y_delta_sum.x_left = (y_delta.x_left * overflow);
            y_delta_sum.x_right = (y_delta.x_right * overflow);

            y_delta_sum.w = (y_delta.w * overflow);
            y_delta_sum.u = (y_delta.u * overflow);
            y_delta_sum.v = (y_delta.v * overflow);

            yStart = fbSize.y-1;
        }
        else
        {
            y_delta_sum.x_left = 0;
            y_delta_sum.x_right = 0;

            y_delta_sum.w = 0;
            y_delta_sum.u = 0;
            y_delta_sum.v = 0;
        }

        if(yEnd < 0)
            yEnd = 0;

        for (int y = yStart; y >= yEnd; y--)
        {
            pos.x_left = bottom.pos.x - pASR(y_delta_sum.x_left, triFracShift);
            pos.x_right = bottom.pos.x - pASR(y_delta_sum.x_right, triFracShift);

            pos.w_left = bottom.pos.w - pASR(y_delta_sum.w, triFracShift);
            pos.u_left = bottom.uv.x - pASR(y_delta_sum.u, triFracShift);
            pos.v_left = bottom.uv.y - pASR(y_delta_sum.v, triFracShift);

            ClipSpan(y, pos, x_delta, texture, 0, flags);

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;

            y_delta_sum.w += y_delta.w;
            y_delta_sum.u += y_delta.u;
            y_delta_sum.v += y_delta.v;
        }
    }

    void Render::DrawTriangleBottomFlat(const Vertex2d points[], const pixel color)
    {
        TriEdgeTrace pos;
        TriDrawYDeltaZ y_delta, y_delta_sum;

        const Vertex2d& bottom  = points[2];
        const Vertex2d& left    = (points[0].pos.x < points[1].pos.x) ? points[0] : points[1];
        const Vertex2d& right   = (points[0].pos.x < points[1].pos.x) ? points[1] : points[0];

        if((bottom.pos.x >= fbSize.x && left.pos.x >= fbSize.x) || (right.pos.x < 0 && bottom.pos.x < 0))
            return;

        int yStart = bottom.pos.y;
        int yEnd = left.pos.y;

        if(yStart < 0 || yEnd >= fbSize.y)
            return;

        GetTriangleLerpDeltasZ(left, right, bottom, y_delta);

        if(yStart >= fbSize.y)
        {
            int overflow = yStart - (fbSize.y-1);

            y_delta_sum.x_left = (y_delta.x_left * overflow);
            y_delta_sum.x_right = (y_delta.x_right * overflow);

            yStart = fbSize.y-1;
        }
        else
        {
            y_delta_sum.x_left = 0;
            y_delta_sum.x_right = 0;
        }

        if(yEnd < 0)
            yEnd = 0;

        for (int y = yStart; y >= yEnd; y--)
        {
            pos.x_left = bottom.pos.x - pASR(y_delta_sum.x_left, triFracShift);
            pos.x_right = bottom.pos.x - pASR(y_delta_sum.x_right, triFracShift);

            DrawTriangleScanlineFlat(y, pos, color);

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;
        }
    }

    void Render::ClipSpan(int y, TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color, const RenderFlags flags)
    {
        SpanBuffer* s_buffer = &spanBuffer[y];

        if(s_buffer->min_opening >= fbSize.x)
            return;

        int x_start = pos.x_left;
        int x_end = pos.x_right;

        if(s_buffer->min_opening > x_end)
            return;

        if( (x_end < x_start) || (x_end < 0) || (x_start >= fbSize.x) )
            return;

        if(x_start < 0)
            x_start = 0;

        if(x_end >= fbSize.x)
            x_end = fbSize.x-1;

        //Clip against spans already in buffer.
        unsigned int span_count = s_buffer->span_list.size();

        Span* l_abutt = nullptr;
        Span* r_abutt = nullptr;

        int first_opening = s_buffer->min_opening;

        if(x_start <= first_opening && x_end > first_opening)
            first_opening = x_end + 1;

        for(unsigned int i = 0; i < span_count; i++)
        {
            Span& other = s_buffer->span_list[i];

            int x_start2 = other.x_start;
            int x_end2 = other.x_end;

            //Fully occluded.
            if(x_start >= x_start2 && x_end <= x_end2)
                return;


            if(x_start2 <= first_opening)
            {
                if(first_opening < x_end2)
                    first_opening = x_end2 + 1;
            }

            //Fully to the right of.
            if(x_start > x_end2)
            {
                if(x_start == x_end2 + 1)
                    l_abutt = &other;

                continue;
            }

            //Fully to the left of.
            if(x_end < x_start2)
            {
                if(x_end == x_start2 - 1)
                    r_abutt = &other;

                continue;
            }

            //Overlaps both sides.
            if(x_start < x_start2 && x_end > x_end2)
            {
                //Create new span for left edge.
                TriEdgeTrace left_edge = pos;
                left_edge.x_left = x_start;
                left_edge.x_right = x_start2 - 1;

                ClipSpan(y, left_edge, delta, texture, color, flags);

                //Adjust start point for right edge.
                x_start = x_end2 + 1;
                l_abutt = &other;
                continue;
            }

            //Left overlap.
            if(x_start < x_start2)
            {
                x_end = x_start2-1;
                r_abutt = &other;
            }
            //Right overlap.
            else
            {
                x_start = x_end2 + 1;
                l_abutt = &other;
            }
        }

        if(x_start > (int)pos.x_left)
        {
            int overlap = x_start - (int)pos.x_left;

            pos.w_left += (delta.w * overlap);
            pos.u_left += (delta.u * overlap);
            pos.v_left += (delta.v * overlap);

            pos.x_left = x_start;
        }

        pos.x_right = x_end;

        if(l_abutt && r_abutt)
        {
            l_abutt->x_end = r_abutt->x_end;
            r_abutt->x_start = l_abutt->x_start;
        }
        else if(r_abutt)
        {
            r_abutt->x_start = x_start;
        }
        else if(l_abutt)
        {
            l_abutt->x_end = x_end;
        }
        else
        {
            Span span = {x_start, x_end};
            s_buffer->span_list.push_back(span);
        }

        s_buffer->min_opening = first_opening;

        DrawSpan(y, pos, delta, texture, color);
    }

    void Render::DrawSpan(int y, TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color)
    {
        if(!texture)
        {
            DrawTriangleScanlineFlat(y, pos, color);
        }
        else
        {
            DrawTriangleScanlinePerspectiveCorrect(y, pos, delta, texture);
        }
    }

    void Render:: DrawTriangleScanlinePerspectiveCorrect(int y, const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture)
    {
        int x_start = pos.x_left;
        int x_end = pos.x_right;

        fp u = pos.u_left, v = pos.v_left, w = pos.w_left;
        fp du = delta.u, dv = delta.v, dw = delta.w;

        unsigned int count = (x_end - x_start) + 1;

        int buffOffset = ((y * fbSize.x) + x_start);
        pixel* fb = &frameBuffer[buffOffset];

        const unsigned int umask = texture->u_mask;
        const unsigned int vmask = texture->v_mask;
        const unsigned int vshift = texture->v_shift;
        const pixel* t_pxl = texture->pixels;

#ifdef RENDER_STATS
        stats.scanlines_drawn++;
#endif

        fp invw_0 = pReciprocal(w);
        fp invw_15 = pReciprocal(w + (pASL(dw, 4)));

        fp u0 = u * invw_0;
        fp u15 = (u + (pASL(du, 4))) * invw_15;

        fp v0 = v * invw_0;
        fp v15 = (v + (pASL(dv, 4))) * invw_15;

        fp du8 = pASR(u15-u0, 4);
        fp dv8 = pASR(v15-v0, 4);

        fp uu = u0;
        fp vv = v0;

        unsigned int l = count >> 4;
        unsigned int r = count & 15;

        while(l--)
        {
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;

            w += (pASL(dw, 4));
            u += (pASL(du, 4));
            v += (pASL(dv, 4));

            invw_0 = pReciprocal(w);
            invw_15 = pReciprocal(w + (pASL(dw, 4)));

            u0 = u * invw_0;
            u15 = (u + (pASL(du, 4))) * invw_15;

            v0 = v * invw_0;
            v15 = (v + (pASL(dv, 4))) * invw_15;

            du8 = pASR(u15-u0, 4);
            dv8 = pASR(v15-v0, 4);

            uu = u0;
            vv = v0;
        }

        while(r--)
        {
            DrawScanlinePixelLinear(fb, t_pxl, uu, vv, umask, vmask, vshift); fb++; uu += du8; vv += dv8;
        }
    }

    inline void Render::DrawScanlinePixelLinear(pixel* fb, const pixel* texels, const fp u, const fp v, const unsigned int umask, const unsigned int vmask, const unsigned int vshift)
    {
        unsigned int tx = (int)u;
        unsigned int ty = (int)v;

        tx = tx & umask;
        ty = (ty << vshift) & vmask;
        *fb = texels[ty | tx];
    }

    void Render::DrawTriangleScanlineFlat(int y, const TriEdgeTrace& pos, const pixel color)
    {
        int x_start = pos.x_left;
        int x_end = pos.x_right;

        unsigned int count = (x_end - x_start) + 1;

        int buffOffset = ((y * fbSize.x) + x_start);
        pixel* fb = &frameBuffer[buffOffset];

#ifdef RENDER_STATS
        stats.scanlines_drawn++;
#endif

        do
        {
            *fb++ = color;
        } while(--count);
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

    void Render::GetTriangleLerpDeltasZWUV(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawXDeltaZWUV& x_delta, TriDrawYDeltaZWUV &y_delta)
    {
        //Use reciprocal table for these.
        fp inv_y = 0;
        fp inv_x = 0;

        if(left.pos.y != other.pos.y)
            inv_y = fp(1 << triFracShift) / (left.pos.y - other.pos.y);


        if(right.pos.x != left.pos.x)
            inv_x = pReciprocal(right.pos.x - left.pos.x);

        x_delta.w = (right.pos.w - left.pos.w) * inv_x;
        x_delta.u = (right.uv.x - left.uv.x) * inv_x;
        x_delta.v = (right.uv.y - left.uv.y) * inv_x;

        y_delta.x_left = (left.pos.x - other.pos.x) * inv_y;
        y_delta.x_right = (right.pos.x - other.pos.x) * inv_y;
        y_delta.w = (left.pos.w - other.pos.w) * inv_y;
        y_delta.u = (left.uv.x - other.uv.x) * inv_y;
        y_delta.v = (left.uv.y - other.uv.y) * inv_y;
    }

    void Render::GetTriangleLerpDeltasZ(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawYDeltaZ &y_delta)
    {
        //Use reciprocal table for these.
        fp inv_y = 0;

        if(left.pos.y != other.pos.y)
            inv_y = fp(1 << triFracShift) / (left.pos.y - other.pos.y);

        y_delta.x_left = (left.pos.x - other.pos.x) * inv_y;
        y_delta.x_right = (right.pos.x - other.pos.x) * inv_y;
    }

    int Render::fracToY(fp frac)
    {
        fp y = fp(2)-(frac + fp(1));

        fp sy = pASR(y * fbSize.y, 1) + fp(0.5f);

        return sy;
    }

    int Render::fracToX(fp frac)
    {
        fp x = frac + fp(1);

        fp sx = pASR(x * fbSize.x, 1) + fp(0.5f);

        return sx;
    }

    RenderStats Render::GetRenderStats()
    {
        return stats;
    }
}
