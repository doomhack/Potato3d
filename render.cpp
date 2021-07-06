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

#ifdef FRONT_TO_BACK
        spanBuffer = new SpanBuffer[screenHeight];

        span_pool = new SpanNode[fbSize.y * SPAN_NODES_LINE];
        span_free_index = 0;  
#endif

        return true;
    }

    void Render::BeginFrame()
    {
        UpdateViewProjectionMatrix();

#ifdef FRONT_TO_BACK
        for(int i = 0; i < fbSize.y; i++)
        {
            spanBuffer[i].pixels_left = fbSize.x;
            spanBuffer[i].span_tree = nullptr;
        }

        span_free_index = 0;

        pixels_left = fbSize.x * fbSize.y;
#endif

#ifdef RENDER_STATS
        stats.triangles_drawn = 0;
        stats.triangles_submitted = 0;
        stats.vertex_transformed = 0;
        stats.scanlines_drawn = 0;
        stats.span_checks = 0;
        stats.span_count = 0;
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

        unsigned int c32 = color;

        FastFill32((unsigned int*)frameBuffer, c32 | c32 << 16, buffSize >> 2);
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

            case MatrixType::Transform:
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

#ifdef FRONT_TO_BACK
        if(pixels_left <= 0)
            return;
#endif

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

        if(w0 < zNear && w1 < zNear && w2 < zNear)
            return;

        if(w0 > zFar && w1 > zFar && w2 > zFar)
            return;

        fp min_0 = pMin(clipSpacePoints[0].pos.x, clipSpacePoints[0].pos.y);
        fp min_1 = pMin(clipSpacePoints[1].pos.x, clipSpacePoints[1].pos.y);
        fp min_2 = pMin(clipSpacePoints[2].pos.x, clipSpacePoints[2].pos.y);

        if(min_0 > w0 && min_1 > w1 && min_2 > w2)
            return;

        fp max_0 = pMax(clipSpacePoints[0].pos.x, clipSpacePoints[0].pos.y);
        fp max_1 = pMax(clipSpacePoints[1].pos.x, clipSpacePoints[1].pos.y);
        fp max_2 = pMax(clipSpacePoints[2].pos.x, clipSpacePoints[2].pos.y);

        if(-max_0 > w0 && -max_1 > w1 && -max_2 > w2)
            return;

        if (max_0 <= w0 && max_1 <= w1 && max_2 <= w2 && -min_0 <= w0 && -min_1 <= w1 && -min_2 <= w2)
        {
            DrawTriangleCull(clipSpacePoints, texture, color, flags);
        }
        else
        {
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

    fp Render::GetClipPointForVertex(const Vertex2d& vertex, ClipPlane clipPlane) const
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
        if((a1 <= b1 && a2 <= b2) || (a1 >= b1 && a2 >= b2))
            return 0;

        fp l1 = (a1 - b1);

        return l1 / (l1 - a2 + b2);

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

    void Render::DrawTriangleCull(const Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        Vertex2d screenSpacePoints[3];

#ifdef POLYGON_NOISE_SIZE
        fp xmin = fbSize.x, xmax = 0, ymin = fbSize.y, ymax = 0;
#endif

        for(int i = 0; i < 3; i++)
        {
            screenSpacePoints[i].pos = clipSpacePoints[i].pos.ToScreenSpace();

            screenSpacePoints[i].pos.x = fracToX(screenSpacePoints[i].pos.x);
            screenSpacePoints[i].pos.y = fracToY(screenSpacePoints[i].pos.y);

#ifdef POLYGON_NOISE_SIZE
            xmin = (screenSpacePoints[i].pos.x < xmin) ? screenSpacePoints[i].pos.x : xmin;
            xmax = screenSpacePoints[i].pos.x > xmax ? screenSpacePoints[i].pos.x : xmax;
#endif

            if(texture)
            {
                screenSpacePoints[i].uv.x = clipSpacePoints[i].uv.x * (int)texture->width;
                screenSpacePoints[i].uv.y = clipSpacePoints[i].uv.y * (int)texture->height;
            }
        }

        SortPointsByY(screenSpacePoints);

#ifdef POLYGON_NOISE_SIZE
        ymin = screenSpacePoints[0].pos.y;
        ymax = screenSpacePoints[2].pos.y;

        if(( pRound(xmax - xmin) * pRound(ymax - ymin)) < POLYGON_NOISE_SIZE)
        {
            if  (
                    (clipSpacePoints[0].pos.z > POLYGON_NOISE_Z_THREASHOLD) &&
                    (clipSpacePoints[1].pos.z > POLYGON_NOISE_Z_THREASHOLD) &&
                    (clipSpacePoints[2].pos.z > POLYGON_NOISE_Z_THREASHOLD)
                )
            {
                return;
            }
        }
#endif

#ifdef POLYGON_TEXTURE_Z_THREASHOLD
        if  (
                (clipSpacePoints[0].pos.z > POLYGON_TEXTURE_Z_THREASHOLD) &&
                (clipSpacePoints[1].pos.z > POLYGON_TEXTURE_Z_THREASHOLD) &&
                (clipSpacePoints[2].pos.z > POLYGON_TEXTURE_Z_THREASHOLD)
            )
        {
            texture = nullptr;
        }
#endif

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
        points[0].toPerspectiveCorrect();
        points[1].toPerspectiveCorrect();
        points[2].toPerspectiveCorrect();

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
            fp splitFrac = pApproxDiv((points[1].pos.y - points[0].pos.y), (points[2].pos.y - points[0].pos.y));

            //Interpolate new values for new vertex.
            Vertex2d triangle[4];

            triangle[0] = points[0];
            triangle[1] = points[1];

            //x pos
            triangle[2].pos.x = pLerp(points[0].pos.x, points[2].pos.x, splitFrac);
            triangle[2].pos.y = points[1].pos.y;

            //uv coords.
            triangle[2].uv.x = pLerp(points[0].uv.x, points[2].uv.x, splitFrac);
            triangle[2].uv.y = pLerp(points[0].uv.y, points[2].uv.y, splitFrac);

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
            fp splitFrac = pApproxDiv((points[1].pos.y - points[0].pos.y), (points[2].pos.y - points[0].pos.y));

            //Interpolate new values for new vertex.
            Vertex2d triangle[4];

            triangle[0] = points[0];
            triangle[1] = points[1];

            //x pos
            triangle[2].pos.x = pLerp(points[0].pos.x, points[2].pos.x, splitFrac);
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
        TriDrawXDeltaZWUV x_delta;


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

            ClipSpan(y, pos, x_delta, nullptr, color, NoFlags);

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
        TriDrawXDeltaZWUV x_delta;

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

            ClipSpan(y, pos, x_delta, nullptr, color, NoFlags);

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;
        }
    }

    void Render::ClipSpan(int y, TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color, const RenderFlags flags)
    {

#ifdef FRONT_TO_BACK
        SpanBuffer* s_buffer = &spanBuffer[y];

        if(s_buffer->pixels_left <= 0)
            return;
#endif

        const int fb_width = fbSize.x;

        int x_start = pos.x_left;
        int x_end = pos.x_right;

        if( (x_end < x_start) || (x_end < 0) || (x_start >= fb_width) )
            return;

        if(x_start < 0)
            x_start = 0;

        if(x_end >= fb_width)
            x_end = fb_width-1;

#ifdef FRONT_TO_BACK

        SpanNode* c_node = s_buffer->span_tree;
        SpanNode** p_node = &s_buffer->span_tree;

        while(true)
        {
            if(c_node == nullptr)
                break;

#ifdef RENDER_STATS
            stats.span_checks++;
#endif

            int x_start2 = c_node->x_start;
            int x_end2 = c_node->x_end;

            //Right of.
            if(x_end > x_end2)
            {
                if(x_start <= (x_end2 + 1)) //Overlap right
                {
                    if(x_start < x_start2) //Overlap both sides.
                    {
                        //Create new span for left edge.
                        TriEdgeTrace left_edge = pos;
                        left_edge.x_left = x_start;
                        left_edge.x_right = x_start2 - 1;

                        ClipSpan(y, left_edge, delta, texture, color, flags);
                    }

                    x_start = x_end2 + 1;

                    if(c_node->right == nullptr)
                    {
                        c_node->x_end = x_end;

                        p_node = nullptr;
                        break;
                    }
                }

                p_node = &c_node->right;
                c_node = c_node->right;
            }
            else //left of.
            {
                //Fully occluded.
                if(x_start >= x_start2)
                    return;

                if(x_end >= x_start2 - 1) //Overlap left
                {
                    x_end = x_start2 - 1;

                    if(c_node->left == nullptr)
                    {
                        c_node->x_start = x_start;

                        p_node = nullptr;
                        break;
                    }
                }

                p_node = &c_node->left;
                c_node = c_node->left;
            }
        }

        if(p_node)
        {
#ifdef RENDER_STATS
            stats.span_count++;
#endif
            SpanNode* new_node = GetFreeSpanNode();

            if(new_node == nullptr)
                return;

            new_node->x_start = x_start;
            new_node->x_end = x_end;
            new_node->left = nullptr;
            new_node->right = nullptr;

            *p_node = new_node;
        }

        unsigned int p_count = (x_end - x_start) + 1;

        s_buffer->pixels_left -= p_count;
        pixels_left -= p_count;

#endif

        if(texture && x_start > (int)pos.x_left)
        {
            int overlap = x_start - (int)pos.x_left;

            pos.w_left += pASR(delta.w * overlap, triFracShift);
            pos.u_left += pASR(delta.u * overlap, triFracShift);
            pos.v_left += pASR(delta.v * overlap, triFracShift);
        }

        pos.x_left = x_start;
        pos.x_right = x_end;

        DrawSpan(y, pos, delta, texture, color);
    }

    SpanNode* Render::GetFreeSpanNode()
    {
        if(span_free_index >= (SPAN_NODES_LINE * fbSize.y))
            return nullptr;

        return &span_pool[span_free_index++];
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

        const pixel* t_pxl = texture->pixels;

#ifdef RENDER_STATS
        stats.scanlines_drawn++;
#endif

        unsigned int l = count >> 4;
        unsigned int r = count & 15;

        while(l--)
        {
            fp invw_0 = pReciprocal(w);
            fp u0 = u * invw_0;
            fp v0 = v * invw_0;

            fp invw_15 = pReciprocal(w += dw);
            fp u15 = (u += du) * invw_15;
            fp v15 = (v += dv) * invw_15;

            fp du16 = pASR(u15-u0, 4);
            fp dv16 = pASR(v15-v0, 4);

            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
            DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++;
        }

        if(r)
        {
            fp invw_0 = pReciprocal(w);
            fp u0 = u * invw_0;
            fp v0 = v * invw_0;

            fp invw_15 = pReciprocal(w += dw);
            fp u15 = (u += du) * invw_15;
            fp v15 = (v += dv) * invw_15;

            fp du16 = pASR(u15-u0, 4);
            fp dv16 = pASR(v15-v0, 4);

            switch(r)
            {
                case 15:    DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 14:    DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 13:    DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 12:    DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 11:    DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 10:    DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 9:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 8:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 7:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 6:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 5:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 4:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 3:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 2:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0); fb++; u0 += du16; v0 += dv16;
                case 1:     DrawScanlinePixelLinear(fb, t_pxl, u0, v0);
            }
        }
    }

    inline void Render::DrawScanlinePixelLinear(pixel* fb, const pixel* texels, const fp u, const fp v)
    {
        unsigned int tx = (int)u;
        unsigned int ty = (int)v;

        tx = tx & TEX_MASK;
        ty = (ty & TEX_MASK) << TEX_SHIFT;
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

        FastFill16(fb, color, count);
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
            inv_y = pScaledReciprocal(triFracShift, (left.pos.y - other.pos.y));

        if(right.pos.x != left.pos.x)
            inv_x = pScaledReciprocal(triFracShift, (right.pos.x - left.pos.x));

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
            inv_y = pScaledReciprocal(triFracShift, (left.pos.y - other.pos.y));

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
