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

#ifdef USE_VERTEX_CACHE
        transformedVertexCache = new V4<fp>[VERTEX_CACHE_SIZE];
        transformedVertexCacheIndexes = new unsigned char[VERTEX_CACHE_SIZE];
#endif

        return true;
    }

    void Render::BeginFrame()
    {
        UpdateViewProjectionMatrix();

#ifdef USE_VERTEX_CACHE
        FastFill32((unsigned int*)transformedVertexCacheIndexes, 0, VERTEX_CACHE_SIZE / 4);
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

        unsigned int c32 = (color | (color << 8));

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

    pixel *Render::GetFramebuffer()
    {
        return this->frameBuffer;
    }

    void Render::DrawTriangle(const Triangle3d* tri, const Texture* texture, const pixel color, const RenderFlags flags)
    {

#ifdef RENDER_STATS
        stats.triangles_submitted++;
#endif

        Vertex2d clipSpacePoints[8];

        for(int i = 0; i < 3; i++)
        {
            const Vertex3d* v = &tri->verts[i];

            clipSpacePoints[i] = this->TransformVertex(v);
        }

        DrawTriangleClip(clipSpacePoints, texture, color, flags);
    }

    Vertex2d Render::TransformVertex(const Vertex3d* vertex)
    {
        Vertex2d screenspace;

#ifdef USE_VERTEX_CACHE
        if( (vertex->vertex_id != vertex->no_vx_id) && transformedVertexCacheIndexes[vertex->vertex_id])
        {
            screenspace.pos = transformedVertexCache[vertex->vertex_id];
        }
        else
        {
            screenspace.pos = transformMatrix * vertex->pos;

            if(vertex->vertex_id != vertex->no_vx_id)
            {
                transformedVertexCache[vertex->vertex_id] = screenspace.pos;
                transformedVertexCacheIndexes[vertex->vertex_id] = 1;
            }

    #ifdef RENDER_STATS
            stats.vertex_transformed++;
    #endif
        }
#else

    #ifdef RENDER_STATS
            stats.vertex_transformed++;
    #endif

        screenspace.pos = transformMatrix * vertex->pos;
#endif

        screenspace.uv = vertex->uv;

        return screenspace;
    }

    void Render::DrawTriangleClip(Vertex2d clipSpacePoints[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        unsigned int clip = 0;

        fp w0 = clipSpacePoints[0].pos.w;
        fp w1 = clipSpacePoints[1].pos.w;
        fp w2 = clipSpacePoints[2].pos.w;

        if(w0 < zNear && w1 < zNear && w2 < zNear)
            return;

        if(w0 > zFar && w1 > zFar && w2 > zFar)
            return;

        if(w0 < zNear || w1 < zNear || w2 < zNear)
            clip |= W_Near;

        fp x0 = clipSpacePoints[0].pos.x;
        fp x1 = clipSpacePoints[1].pos.x;
        fp x2 = clipSpacePoints[2].pos.x;

        if(x0 > w0 && x1 > w1 && x2 > w2)
            return;

        if(-x0 > w0 && -x1 > w1 && -x2 > w2)
            return;

        if(x0 > w0 || x1 > w1 || x2 > w2)
            clip |= X_W_Right;

        if(-x0 > w0 || -x1 > w1 || -x2 > w2)
            clip |= X_W_Left;

        fp y0 = clipSpacePoints[0].pos.y;
        fp y1 = clipSpacePoints[1].pos.y;
        fp y2 = clipSpacePoints[2].pos.y;

        if(y0 > w0 && y1 > w1 && y2 > w2)
            return;

        if(-y0 > w0 && -y1 > w1 && -y2 > w2)
            return;

        if(y0 > w0 || y1 > w1 || y2 > w2)
            clip |= Y_W_Top;

        if(-y0 > w0 || -y1 > w1 || -y2 > w2)
            clip |= Y_W_Bottom;


        if (clip == NoClip)
        {
            TriangulatePolygon(clipSpacePoints, 3, texture, color, flags);
        }
        else
        {
            Vertex2d outputVxB[8];
            unsigned int countA = 3;

            //As we clip against each frustrum plane, we swap the buffers
            //so the output of the last clip is used as input to the next.
            Vertex2d* inBuffer = clipSpacePoints;
            Vertex2d* outBuffer = outputVxB;

            if(clip & W_Near)
            {
                countA = ClipPolygon(inBuffer, countA, outBuffer, W_Near);
                std::swap(inBuffer, outBuffer);
            }

            if(clip & X_W_Left)
            {
                countA = ClipPolygon(inBuffer, countA, outBuffer, X_W_Left);
                std::swap(inBuffer, outBuffer);
            }

            if(clip & X_W_Right)
            {
                countA = ClipPolygon(inBuffer, countA, outBuffer, X_W_Right);
                std::swap(inBuffer, outBuffer);
            }

            if(clip & Y_W_Top)
            {
                countA = ClipPolygon(inBuffer, countA, outBuffer, Y_W_Top);
                std::swap(inBuffer, outBuffer);
            }

            if(clip & Y_W_Bottom)
            {
                countA = ClipPolygon(inBuffer, countA, outBuffer, Y_W_Bottom);
                std::swap(inBuffer, outBuffer);
            }

            //Now outBuffer and outCount contain the final result.
            TriangulatePolygon(inBuffer, countA, texture, color, flags);
        }
    }

    unsigned int Render::ClipPolygon(const Vertex2d clipSpacePointsIn[], const int vxCount, Vertex2d clipSpacePointsOut[], ClipPlane clipPlane)
    {
        if(vxCount < 3)
            return 0;

        unsigned int vxCountOut = 0;

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
                LerpVertexXYZWUV(clipSpacePointsOut[vxCountOut], clipSpacePointsIn[i], clipSpacePointsIn[i2], frac);

                vxCountOut++;
            }
        }

        return vxCountOut;
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

            default:
                return 0;
        }
    }

    fp Render::GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2)
    {
        if((a1 <= b1 && a2 <= b2) || (a1 >= b1 && a2 >= b2))
            return -1;

        fp l1 = (a1 - b1);

        return l1 / (l1 - a2 + b2);
    }

    void Render::TriangulatePolygon(Vertex2d clipSpacePoints[], const int vxCount, const Texture *texture, const pixel color, const RenderFlags flags)
    {
        if(vxCount < 3)
            return;

        for(int i = 0; i < vxCount; i++)
        {
            clipSpacePoints[i].pos.ToScreenSpace();

            clipSpacePoints[i].pos.x = fracToX(clipSpacePoints[i].pos.x);
            clipSpacePoints[i].pos.y = fracToY(clipSpacePoints[i].pos.y);
        }

        DrawTriangleSplit(clipSpacePoints, texture, color, flags);

        int rounds = vxCount - 3;

        for(int i = 0; i < rounds; i++)
        {
            clipSpacePoints[i+1] = clipSpacePoints[0];
            DrawTriangleSplit(&clipSpacePoints[i+1], texture, color, flags);
        }
    }

    void Render::DrawTriangleSplit(Vertex2d screenSpacePoints[], const Texture *texture, const pixel color, RenderFlags flags)
    {
        Vertex2d points[3];

        SortPointsByY(screenSpacePoints, points);

        if(points[1].pos.y == points[2].pos.y)
        {
            DrawTriangleTop(points, texture, color, flags);
        }
        else if(points[0].pos.y == points[1].pos.y)
        {
            DrawTriangleBottom(points, texture, color, flags);
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
            triangle[2].pos.x = pRound(pLerp(points[0].pos.x, points[2].pos.x, splitFrac));
            triangle[2].pos.y = points[1].pos.y;

            if(texture)
            {
                //uv coords.
                triangle[2].uv.x = pLerp(points[0].uv.x, points[2].uv.x, splitFrac);
                triangle[2].uv.y = pLerp(points[0].uv.y, points[2].uv.y, splitFrac);

                triangle[2].pos.w = pLerp(points[0].pos.w, points[2].pos.w, splitFrac);

            }

            triangle[3] = points[2];

            DrawTriangleTop(triangle, texture, color, flags);
            DrawTriangleBottom(&triangle[1], texture, color, flags);
        }
    }

    void Render::SortPointsByY(Vertex2d pointsIn[], Vertex2d pointsOut[])
    {
        if(pointsIn[0].pos.y < pointsIn[1].pos.y)
        {
            if(pointsIn[1].pos.y < pointsIn[2].pos.y)
            {
                pointsOut[0] = pointsIn[0];
                pointsOut[1] = pointsIn[1];
                pointsOut[2] = pointsIn[2];
            }
            else
            {
                pointsOut[2] = pointsIn[1];

                if(pointsIn[0].pos.y < pointsIn[2].pos.y)
                {
                    pointsOut[0] = pointsIn[0];
                    pointsOut[1] = pointsIn[2];
                    pointsOut[2] = pointsIn[1];
                }
                else
                {
                    pointsOut[0] = pointsIn[2];
                    pointsOut[1] = pointsIn[0];
                    pointsOut[2] = pointsIn[1];
                }
            }
        }
        else
        {
            if(pointsIn[1].pos.y < pointsIn[2].pos.y)
            {
                pointsOut[0] = pointsIn[1];

                if(pointsIn[0].pos.y < pointsIn[2].pos.y)
                {
                    pointsOut[1] = pointsIn[0];
                    pointsOut[2] = pointsIn[2];
                }
                else
                {
                    pointsOut[0] = pointsIn[1];
                    pointsOut[1] = pointsIn[2];
                    pointsOut[2] = pointsIn[0];
                }
            }
            else
            {
                pointsOut[0] = pointsIn[2];
                pointsOut[1] = pointsIn[1];
                pointsOut[2] = pointsIn[0];
            }
        }
    }

    void Render::DrawTriangleTop(const Vertex2d points[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        //Flat bottom triangle.

        TriEdgeTrace pos;
        TriDrawXDeltaZWUV x_delta;
        TriDrawYDeltaZWUV y_delta, y_delta_sum = {0,0,0,0};

        const Vertex2d& top     = points[0];
        const Vertex2d& left    = (points[1].pos.x < points[2].pos.x) ? points[1] : points[2];
        const Vertex2d& right   = (points[1].pos.x < points[2].pos.x) ? points[2] : points[1];

        if((top.pos.x >= fbSize.x && left.pos.x >= fbSize.x) || (right.pos.x < 0 && top.pos.x < 0))
            return;

        int yStart = (int)(top.pos.y);
        int yEnd =   (int)(left.pos.y);

        if(yEnd < 0 || yStart > fbSize.y)
            return;

        if(yEnd > fbSize.y)
            yEnd = fbSize.y;

        if(yStart < 0)
            yStart = 0;

        if(texture)
            GetTriangleLerpDeltasZWUV(left, right, top, x_delta, y_delta);
        else
            GetTriangleLerpDeltasZ(left, right, top, y_delta);

        for(int y = yStart; y < yEnd; y++)
        {
            pos.x_left = top.pos.x + y_delta_sum.x_left;
            pos.x_right = top.pos.x + y_delta_sum.x_right;

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;

            if(texture)
            {
                pos.u_left = top.uv.x + y_delta_sum.u;
                pos.v_left = top.uv.y + y_delta_sum.v;

                y_delta_sum.u += y_delta.u;
                y_delta_sum.v += y_delta.v;
            }

            ClipSpan(y, pos, x_delta, texture, color, flags);
        }

#ifdef RENDER_STATS
        stats.triangles_drawn++;
#endif

    }

    void Render::DrawTriangleBottom(const Vertex2d points[], const Texture *texture, const pixel color, const RenderFlags flags)
    {
        //Flat top triangle.

        TriEdgeTrace pos;
        TriDrawXDeltaZWUV x_delta;
        TriDrawYDeltaZWUV y_delta, y_delta_sum = {0,0,0,0};

        const Vertex2d& bottom  = points[2];
        const Vertex2d& left    = (points[0].pos.x < points[1].pos.x) ? points[0] : points[1];
        const Vertex2d& right   = (points[0].pos.x < points[1].pos.x) ? points[1] : points[0];

        if((bottom.pos.x >= fbSize.x && left.pos.x >= fbSize.x) || (right.pos.x < 0 && bottom.pos.x < 0))
            return;

        int yStart = (int)(left.pos.y);
        int yEnd = (int)(bottom.pos.y);

        if(yEnd > fbSize.y)
            yEnd = fbSize.y;

        if(yStart < 0)
            yStart = 0;

        if(yEnd < 0 || yStart > fbSize.y)
            return;

        if(texture)
            GetTriangleLerpDeltasZWUV(left, right, bottom, x_delta, y_delta);
        else
            GetTriangleLerpDeltasZ(left, right, bottom, y_delta);


        for (int y = yStart; y < yEnd; y++)
        {
            pos.x_left = left.pos.x + y_delta_sum.x_left;
            pos.x_right = right.pos.x + y_delta_sum.x_right;

            y_delta_sum.x_left += y_delta.x_left;
            y_delta_sum.x_right += y_delta.x_right;

            if(texture)
            {
                pos.u_left = left.uv.x + y_delta_sum.u;
                pos.v_left = left.uv.y + y_delta_sum.v;

                y_delta_sum.u += y_delta.u;
                y_delta_sum.v += y_delta.v;
            }

            ClipSpan(y, pos, x_delta, texture, color, flags);
        }

#ifdef RENDER_STATS
        stats.triangles_drawn++;
#endif

    }

    void Render::ClipSpan(int y, TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color, const RenderFlags flags)
    {
        const int fb_width = fbSize.x;

        int x_start = pRound(pos.x_left);
        int x_end = pRound(pos.x_right);

        if(x_start >= fb_width)
            return;

        if((x_end < x_start) || (x_end < 0))
            return;

        if(x_start < 0)
            x_start = 0;

        if(x_end >= fb_width)
            x_end = fb_width-1;

        if(texture)
        {
            fp preStepX = (fp(x_start) - pos.x_left);

            pos.u_left += (delta.u * preStepX);
            pos.v_left += (delta.v * preStepX);
        }

        pos.x_left = x_start;
        pos.x_right = x_end;

        DrawSpan(y, pos, delta, texture, color, flags);
    }

    void Render::DrawSpan(int y, TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture, const pixel color, const RenderFlags flags)
    {
        if(!texture)
        {
            DrawTriangleScanlineFlat(y, pos, color);
        }
        else
        {
            DrawTriangleScanlineAffine(y, pos, delta, texture);
        }
    }

    void Render:: DrawTriangleScanlineAffine(int y, const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture)
    {        
#ifndef USE_FLOAT
        unsigned int u = pos.u_left.toFPInt();
        unsigned int v = pos.v_left.toFPInt();
#else
        unsigned int u = (unsigned int)pASL(pos.u_left, 16);
        unsigned int v = (unsigned int)pASL(pos.v_left, 16);
#endif

        unsigned int uv = ((u << 10) & 0xffff0000) | ((v >> 6) & 0x0000ffff);

#ifndef USE_FLOAT
        unsigned int du = delta.u.toFPInt();
        unsigned int dv = delta.v.toFPInt();
#else
        unsigned int du = (unsigned int)pASL(delta.u, 16);
        unsigned int dv = (unsigned int)pASL(delta.v, 16);
#endif

        unsigned int duv = ((du << 10) & 0xffff0000) | ((dv >> 6) & 0x0000ffff);

        int x_start = (int)pos.x_left;
        int x_end = (int)pos.x_right;

        unsigned int count = (x_end - x_start) + 1;

        int buffOffset = ((y << 8) - (y << 4)) + x_start;
        //int buffOffset = ((y * fbSize.x) + x_start);
        pixel* fb = &frameBuffer[buffOffset];

        const pixel* t_pxl = texture->pixels;

#ifdef RENDER_STATS
        stats.scanlines_drawn++;
#endif

        if((unsigned int)fb & 1)
        {
            DrawScanlinePixelLinearHighByte(fb, t_pxl, uv); fb++; uv += duv; count--;
        }

        unsigned int l = count >> 4;

        while(l--)
        {
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
        }

        unsigned int r = ((count & 15) >> 1);

        switch(r)
        {
            case 7: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 6: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 5: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 4: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 3: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 2: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 1: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
        }

        if(count & 1)
            DrawScanlinePixelLinearLowByte(fb, t_pxl, uv);
    }

    inline void Render::DrawScanlinePixelLinearPair(pixel* fb, const pixel* texels, const unsigned int uv1, const unsigned int uv2)
    {
        unsigned int tx = (uv1 >> 26);
        unsigned int ty = ((uv1 >> 4) & (TEX_MASK << TEX_SHIFT));

        unsigned int tx2 = (uv2 >> 26);
        unsigned int ty2 = ((uv2 >> 4) & (TEX_MASK << TEX_SHIFT));

        *(unsigned short*)fb = ((texels[ty + tx]) | (texels[(ty2 + tx2)] << 8));
    }

    inline void Render::DrawScanlinePixelLinearLowByte(pixel *fb, const pixel* texels, const unsigned int uv)
    {
        unsigned int tx = (uv >> 26);
        unsigned int ty = ((uv >> 4) & (TEX_MASK << TEX_SHIFT));

        unsigned short texel = texels[(ty + tx)];

        volatile unsigned short* p16 = (volatile unsigned short*)(fb);
        *p16 = (texel | (*p16 & 0xff00));
    }

    inline void Render::DrawScanlinePixelLinearHighByte(pixel *fb, const pixel* texels, const unsigned int uv)
    {
        unsigned int tx = (uv >> 26);
        unsigned int ty = ((uv >> 4) & (TEX_MASK << TEX_SHIFT));

        unsigned short texel = texels[(ty + tx)];

        volatile unsigned short* p16 = (volatile unsigned short*)(fb-1);
        *p16 = (*p16 & 0xff) | (texel << 8);
    }

    void Render::DrawTriangleScanlineFlat(int y, const TriEdgeTrace& pos, const pixel color)
    {
        int x_start = (int)pos.x_left;
        int x_end = (int)pos.x_right;

        unsigned int count = (x_end - x_start) + 1;

        int buffOffset = ((y * fbSize.x) + x_start);
        pixel* fb = &frameBuffer[buffOffset];

        if(buffOffset & 1)
        {
            DrawScanlinePixelLinearHighByte(fb, &color, 0); fb++; count--;
        }

        FastFill16((unsigned short*)fb, color | color << 8, count >> 1);

        if(count & 1)
            DrawScanlinePixelLinearLowByte(fb, &color, 0);


#ifdef RENDER_STATS
        stats.scanlines_drawn++;
#endif
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
            inv_y = pReciprocal(left.pos.y - other.pos.y);

        if(right.pos.x != left.pos.x)
            inv_x = pReciprocal((right.pos.x - left.pos.x) + 1);

        x_delta.u = (right.uv.x - left.uv.x) * inv_x;
        x_delta.v = (right.uv.y - left.uv.y) * inv_x;

        y_delta.x_left = (left.pos.x - other.pos.x) * inv_y;
        y_delta.x_right = (right.pos.x - other.pos.x) * inv_y;

        y_delta.u = (left.uv.x - other.uv.x) * inv_y;
        y_delta.v = (left.uv.y - other.uv.y) * inv_y;
    }

    void Render::GetTriangleLerpDeltasZ(const Vertex2d& left, const Vertex2d& right, const Vertex2d& other, TriDrawYDeltaZWUV &y_delta)
    {
        //Use reciprocal table for these.
        fp inv_y = 0;

        if(left.pos.y != other.pos.y)
            inv_y = pReciprocal(left.pos.y - other.pos.y);

        y_delta.x_left = (left.pos.x - other.pos.x) * inv_y;
        y_delta.x_right = (right.pos.x - other.pos.x) * inv_y;
    }

    fp Render::fracToY(fp frac)
    {
        fp halfFbY = fbSize.y >> 1;

        return pRound((halfFbY * -frac) + halfFbY);
    }

    fp Render::fracToX(fp frac)
    {
        fp halfFbX = fbSize.x >> 1;

        return pRound((halfFbX * frac) + halfFbX);
    }

    RenderStats Render::GetRenderStats()
    {
        return stats;
    }
}
