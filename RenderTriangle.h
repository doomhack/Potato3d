#ifndef RENDERTRIANGLE_H
#define RENDERTRIANGLE_H

#include <algorithm>
#include "RenderCommon.h"
#include "TextureCache.h"

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


    class RenderTriangleBase
    {
    public:

        virtual ~RenderTriangleBase() {};
        virtual void DrawTriangle(TransformedTriangle& tri, const Material& material, TextureCacheBase& texture_cache, const RenderTargetViewport& viewport, const RenderDeviceNearFarPlanes& planes) = 0;
    };

    template<const unsigned int render_flags> class RenderTriangle : public RenderTriangleBase
    {
    public:
        void DrawTriangle(TransformedTriangle& tri, const Material& material, TextureCacheBase& texture_cache, const RenderTargetViewport& viewport, const RenderDeviceNearFarPlanes& planes) override
        {
            tex_cache = &texture_cache;
            current_material = &material;
            current_viewport = &viewport;
            z_planes = &planes;

            DrawTriangleClip(tri);
        }


        void DrawTriangleClip(TransformedTriangle& clipSpacePoints)
        {
            unsigned int clip = 0;

            fp zNear = z_planes->z_near;
            fp zFar = z_planes->z_far;

            fp w0 = clipSpacePoints.verts[0].pos.w;
            fp w1 = clipSpacePoints.verts[1].pos.w;
            fp w2 = clipSpacePoints.verts[2].pos.w;

            if(w0 < zNear && w1 < zNear && w2 < zNear)
                return;

            if(w0 > zFar && w1 > zFar && w2 > zFar)
                return;

            if(w0 < zNear || w1 < zNear || w2 < zNear)
                clip |= W_Near;

            fp x0 = clipSpacePoints.verts[0].pos.x;
            fp x1 = clipSpacePoints.verts[1].pos.x;
            fp x2 = clipSpacePoints.verts[2].pos.x;

            if(x0 > w0 && x1 > w1 && x2 > w2)
                return;

            if(-x0 > w0 && -x1 > w1 && -x2 > w2)
                return;

            if(x0 > w0 || x1 > w1 || x2 > w2)
                clip |= X_W_Right;

            if(-x0 > w0 || -x1 > w1 || -x2 > w2)
                clip |= X_W_Left;

            fp y0 = clipSpacePoints.verts[0].pos.y;
            fp y1 = clipSpacePoints.verts[1].pos.y;
            fp y2 = clipSpacePoints.verts[2].pos.y;

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
                TriangulatePolygon(clipSpacePoints.verts, 3);
            }
            else
            {
                Vertex4d outputVxB[8];
                unsigned int countA = 3;

                //As we clip against each frustrum plane, we swap the buffers
                //so the output of the last clip is used as input to the next.
                Vertex4d* inBuffer = clipSpacePoints.verts;
                Vertex4d* outBuffer = outputVxB;

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
                TriangulatePolygon(inBuffer, countA);
            }
        }

        unsigned int ClipPolygon(const Vertex4d clipSpacePointsIn[], const int vxCount, Vertex4d clipSpacePointsOut[], ClipPlane clipPlane)
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

        fp GetClipPointForVertex(const Vertex4d& vertex, ClipPlane clipPlane) const
        {
            switch(clipPlane)
            {
                case W_Near:
                    return z_planes->z_near;

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

        fp GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2)
        {
            if((a1 <= b1 && a2 <= b2) || (a1 >= b1 && a2 >= b2))
                return -1;

            fp l1 = (a1 - b1);

            return l1 / (l1 - a2 + b2);
        }

        void TriangulatePolygon(Vertex4d clipSpacePoints[], const int vxCount)
        {
            if(vxCount < 3)
                return;

            for(int i = 0; i < vxCount; i++)
            {
                clipSpacePoints[i].pos.ToScreenSpace();

                clipSpacePoints[i].pos.x = fracToX(clipSpacePoints[i].pos.x);
                clipSpacePoints[i].pos.y = fracToY(clipSpacePoints[i].pos.y);
            }

            DrawTriangleCull(clipSpacePoints);

            int rounds = vxCount - 3;

            for(int i = 0; i < rounds; i++)
            {
                clipSpacePoints[i+1] = clipSpacePoints[0];
                DrawTriangleCull(&clipSpacePoints[i+1]);
            }
        }

        void DrawTriangleCull(Vertex4d screenSpacePoints[])
        {
            if constexpr (render_flags & (BackFaceCulling | FrontFaceCulling))
            {
                bool is_front = IsTriangleFrontface(screenSpacePoints);

                if constexpr(render_flags & BackFaceCulling)
                {
                    if(!is_front)
                        return;
                }
                else if constexpr (render_flags & FrontFaceCulling)
                {
                    if(is_front)
                        return;
                }
            }

            DrawTriangleSplit(screenSpacePoints);
        }

        void DrawTriangleSplit(Vertex4d screenSpacePoints[])
        {
            Vertex4d points[3];

            SortPointsByY(screenSpacePoints, points);

            if constexpr (render_flags & PerspectiveMapping)
            {
                if(current_material->type == Material::Texture)
                {
                    points[0].toPerspectiveCorrect();
                    points[1].toPerspectiveCorrect();
                    points[2].toPerspectiveCorrect();
                }
            }

            if(points[1].pos.y == points[2].pos.y)
            {
                DrawTriangleTop(points);
            }
            else if(points[0].pos.y == points[1].pos.y)
            {
                DrawTriangleBottom(points);
            }
            else
            {
                //Now we split the polygon into two triangles.
                //A flat top and flat bottom triangle.

                //How far down between vx0 -> vx2 are we spliting?
                fp splitFrac = ((points[1].pos.y - points[0].pos.y) / (points[2].pos.y - points[0].pos.y));

                //Interpolate new values for new vertex.
                Vertex4d triangle[4];

                triangle[0] = points[0];
                triangle[1] = points[1];

                //x pos
                triangle[2].pos.x = pLerp(points[0].pos.x, points[2].pos.x, splitFrac);
                triangle[2].pos.y = points[1].pos.y;

                if(current_material->type == Material::Texture)
                {
                    //uv coords.
                    triangle[2].uv.x = pLerp(points[0].uv.x, points[2].uv.x, splitFrac);
                    triangle[2].uv.y = pLerp(points[0].uv.y, points[2].uv.y, splitFrac);

                    if constexpr(render_flags & PerspectiveMapping)
                    {
                        triangle[2].pos.w = pLerp(points[0].pos.w, points[2].pos.w, splitFrac);
                    }
                }

                triangle[3] = points[2];

                DrawTriangleTop(triangle);
                DrawTriangleBottom(&triangle[1]);
            }
        }

        void SortPointsByY(Vertex4d pointsIn[], Vertex4d pointsOut[])
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
                    }
                    else
                    {
                        pointsOut[0] = pointsIn[2];
                        pointsOut[1] = pointsIn[0];
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

        void DrawTriangleTop(const Vertex4d points[])
        {
            //Flat bottom triangle.

            TriEdgeTrace pos;
            TriDrawXDeltaZWUV x_delta;
            TriDrawYDeltaZWUV y_delta;

            const Vertex4d& top     = points[0];
            const Vertex4d& left    = (points[1].pos.x < points[2].pos.x) ? points[1] : points[2];
            const Vertex4d& right   = (points[1].pos.x < points[2].pos.x) ? points[2] : points[1];

            const int fb_x = current_viewport->width;
            const int fb_y = current_viewport->height;

            if((top.pos.x >= fb_x && left.pos.x >= fb_x) || (right.pos.x < 0 && top.pos.x < 0))
                return;

            int yStart = top.pos.y;
            int yEnd =   left.pos.y;

            if(yEnd < 0 || yStart >= fb_y)
                return;

            if(yEnd > fb_y)
                yEnd = fb_y;

            if(yStart < 0)
                yStart = 0;

            if(current_material->type == Material::Texture)
                GetTriangleLerpDeltasZWUV(left, right, top, x_delta, y_delta);
            else
                GetTriangleLerpDeltasZ(left, right, top, y_delta);

            pos.fb_ypos = &current_viewport->start[yStart * current_viewport->y_pitch];
            pos.x_left = pos.x_right = top.pos.x;
            pos.u_left = top.uv.x;
            pos.v_left = top.uv.y;

            if constexpr(render_flags & PerspectiveMapping)
            {
                pos.w_left = top.pos.w;
            }

            for(int y = yStart; y < yEnd; y++)
            {
                DrawSpan(pos, x_delta);

                pos.x_left += y_delta.x_left;
                pos.x_right += y_delta.x_right;
                pos.fb_ypos += current_viewport->y_pitch;

                if(current_material->type == Material::Texture)
                {
                    pos.u_left += y_delta.u;
                    pos.v_left += y_delta.v;

                    if constexpr(render_flags & PerspectiveMapping)
                    {
                        pos.w_left += y_delta.w;
                    }
                }
            }
        }

        void DrawTriangleBottom(const Vertex4d points[])
        {
            //Flat top triangle.

            TriEdgeTrace pos;
            TriDrawXDeltaZWUV x_delta;
            TriDrawYDeltaZWUV y_delta;

            const Vertex4d& bottom  = points[2];
            const Vertex4d& left    = (points[0].pos.x < points[1].pos.x) ? points[0] : points[1];
            const Vertex4d& right   = (points[0].pos.x < points[1].pos.x) ? points[1] : points[0];

            const int fb_x = current_viewport->width;
            const int fb_y = current_viewport->height;

            if((bottom.pos.x >= fb_x && left.pos.x >= fb_x) || (right.pos.x < 0 && bottom.pos.x < 0))
                return;

            int yStart = left.pos.y;
            int yEnd = bottom.pos.y;

            if(yEnd > fb_y)
                yEnd = fb_y;

            if(yStart < 0)
                yStart = 0;

            if(yEnd < 0 || yStart >= fb_y)
                return;

            if(current_material->type == Material::Texture)
                GetTriangleLerpDeltasZWUV(left, right, bottom, x_delta, y_delta);
            else
                GetTriangleLerpDeltasZ(left, right, bottom, y_delta);

            pos.fb_ypos = &current_viewport->start[yStart * current_viewport->y_pitch];

            pos.x_left = left.pos.x;
            pos.x_right = right.pos.x;

            pos.u_left = left.uv.x;
            pos.v_left = left.uv.y;

            if constexpr(render_flags & PerspectiveMapping)
            {
                pos.w_left = left.pos.w;
            }

            for (int y = yStart; y < yEnd; y++)
            {
                DrawSpan(pos, x_delta);

                pos.x_left += y_delta.x_left;
                pos.x_right += y_delta.x_right;
                pos.fb_ypos += current_viewport->y_pitch;

                if(current_material->type == Material::Texture)
                {
                    pos.u_left += y_delta.u;
                    pos.v_left += y_delta.v;

                    if constexpr(render_flags & PerspectiveMapping)
                    {
                        pos.w_left += y_delta.w;
                    }
                }
            }
        }

        void DrawSpan(TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta)
        {
            const int fb_width = current_viewport->width;

            int x_start = pRound(pos.x_left);
            int x_end = pRound(pos.x_right);

            if(x_start >= fb_width)
                return;

            if(x_start < 0)
                x_start = 0;

            if(x_end >= fb_width)
                x_end = fb_width-1;

            if(current_material->type == Material::Texture)
            {
                fp preStepX = (fp(x_start) - pos.x_left);

                pos.u_left += delta.u * preStepX;
                pos.v_left += delta.v * preStepX;

                if constexpr (render_flags & PerspectiveMapping)
                {
                    pos.w_left += delta.w * preStepX;
                }
            }

            pos.x_left = x_start;
            pos.x_right = x_end;

            if(current_material->type == Material::Color)
            {
                DrawTriangleScanlineFlat(pos, current_material->color);
            }
            else
            {
                const pixel* texture = tex_cache->GetTexture(current_material->pixels);

                if constexpr (render_flags & PerspectiveMapping)
                {
                    DrawTriangleScanlinePerspectiveCorrect(pos, delta, texture);
                }
                else
                {
                    DrawTriangleScanlineAffine(pos, delta, texture);
                }
            }
        }

        void DrawTriangleScanlinePerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
        {
            int x_start = (int)pos.x_left;
            int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start) + 1;

            fp u = pos.u_left, v = pos.v_left, w = pos.w_left;

            pixel* fb = pos.fb_ypos + x_start;

            fp invw_0 = pReciprocal(w);
            fp invw_15 = pReciprocal(w += delta.w);

            fp u0 = u * invw_0;
            fp u15 = (u += delta.u) * invw_15;

            fp v0 = v * invw_0;
            fp v15 = (v += delta.v) * invw_15;

            unsigned int uv = PackUV(u0, v0);
            unsigned int duv = PackUV(pASR(u15-u0, 4), pASR(v15-v0, 4));

            if((size_t)fb & 1)
            {
                DrawScanlinePixelLinearHighByte(fb, texture, uv); fb++; uv += duv; count--;
            }

            unsigned int l = count >> 4;

            while(l--)
            {
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2;

                invw_0 = pReciprocal(w);
                invw_15 = pReciprocal(w += delta.w);

                u0 = u * invw_0;
                u15 = (u += delta.u) * invw_15;

                v0 = v * invw_0;
                v15 = (v += delta.v) * invw_15;

                uv = PackUV(u0, v0);
                duv = PackUV(pASR(u15-u0, 4), pASR(v15-v0, 4));
            }

            unsigned int r = ((count & 15) >> 1);

            switch(r)
            {
                case 7: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 6: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 5: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 4: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 3: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 2: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 1: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
            }

            if(count & 1)
                DrawScanlinePixelLinearLowByte(fb, texture, uv);
        }

        void DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
        {
            int x_start = (int)pos.x_left;
            int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start) + 1;

            pixel* fb = pos.fb_ypos + x_start;

            unsigned int uv = PackUV(pos.u_left, pos.v_left);
            unsigned int duv = PackUV(pASR(delta.u, 4), pASR(delta.v, 4));

            if((size_t)fb & 1)
            {
                DrawScanlinePixelLinearHighByte(fb, texture, uv); fb++; uv += duv; count--;
            }

            unsigned int l = count >> 4;

            while(l--)
            {
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
            }

            unsigned int r = ((count & 15) >> 1);

            switch(r)
            {
                case 7: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 6: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 5: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 4: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 3: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 2: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
                case 1: DrawScanlinePixelLinearPair(fb, texture, uv, uv+duv); fb+=2; uv += (duv << 1);
            }

            if(count & 1)
                DrawScanlinePixelLinearLowByte(fb, texture, uv);
        }

        inline void DrawScanlinePixelLinearPair(pixel* fb, const pixel* texels, const unsigned int uv1, const unsigned int uv2)
        {
            unsigned int tx = (uv1 >> 26);
            unsigned int ty = ((uv1 >> 4) & (TEX_MASK << TEX_SHIFT));

            unsigned int tx2 = (uv2 >> 26);
            unsigned int ty2 = ((uv2 >> 4) & (TEX_MASK << TEX_SHIFT));

            *(unsigned short*)fb = ((texels[ty + tx]) | (texels[(ty2 + tx2)] << 8));
        }

        inline void DrawScanlinePixelLinearLowByte(pixel *fb, const pixel* texels, const unsigned int uv)
        {
            unsigned int tx = (uv >> 26);
            unsigned int ty = ((uv >> 4) & (TEX_MASK << TEX_SHIFT));

            unsigned short* p16 = (unsigned short*)(fb);
            pixel* p8 = (pixel*)p16;

            unsigned short texel = texels[(ty + tx)] | (p8[1] << 8);

            *p16 = texel;
        }

        inline void DrawScanlinePixelLinearHighByte(pixel *fb, const pixel* texels, const unsigned int uv)
        {
            unsigned int tx = (uv >> 26);
            unsigned int ty = ((uv >> 4) & (TEX_MASK << TEX_SHIFT));

            unsigned short* p16 = (unsigned short*)(fb-1);
            pixel* p8 = (pixel*)p16;

            unsigned short texel = (texels[(ty + tx)] << 8) | *p8;

            *p16 = texel;
        }

        void DrawTriangleScanlineFlat(const TriEdgeTrace& pos, const pixel color)
        {
            int x_start = (int)pos.x_left;
            int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start) + 1;

            pixel* fb = pos.fb_ypos + x_start;

            if((size_t)fb & 1)
            {
                DrawScanlinePixelLinearHighByte(fb, &color, 0); fb++; count--;
            }

            if(count >> 1)
            {
                FastFill16((unsigned short*)fb, color | color << 8, count >> 1);
            }

            if(count & 1)
            {
                DrawScanlinePixelLinearLowByte(&fb[count-1], &color, 0);
            }
        }


        unsigned int PackUV(fp u, fp v)
        {
    #ifndef USE_FLOAT
            unsigned int du = u.toFPInt();
            unsigned int dv = v.toFPInt();
    #else
            unsigned int du = (unsigned int)pASL(u, 16);
            unsigned int dv = (unsigned int)pASL(v, 16);
    #endif

            return ((du << 10) & 0xffff0000) | ((dv >> 6) & 0x0000ffff);
        }

        bool IsTriangleFrontface(const Vertex4d screenSpacePoints[])
        {
            fp x1 = (screenSpacePoints[0].pos.x - screenSpacePoints[1].pos.x);
            fp y1 = (screenSpacePoints[1].pos.y - screenSpacePoints[0].pos.y);

            fp x2 = (screenSpacePoints[1].pos.x - screenSpacePoints[2].pos.x);
            fp y2 = (screenSpacePoints[2].pos.y - screenSpacePoints[1].pos.y);

            return ((x1 * y2) - (y1 * x2)) > 0;
        }

        void GetTriangleLerpDeltasZWUV(const Vertex4d& left, const Vertex4d& right, const Vertex4d& other, TriDrawXDeltaZWUV& x_delta, TriDrawYDeltaZWUV &y_delta)
        {
            fp d_y = (left.pos.y - other.pos.y);
            fp d_x = ((right.pos.x - left.pos.x) + 1);

            x_delta.u = (right.uv.x - left.uv.x) / d_x;
            x_delta.v = (right.uv.y - left.uv.y) / d_x;

            if constexpr (render_flags & PerspectiveMapping)
            {
                x_delta.w = (right.pos.w - left.pos.w) / d_x;
                y_delta.w = (left.pos.w - other.pos.w) / d_y;
            }

            y_delta.x_left = (left.pos.x - other.pos.x) / d_y;
            y_delta.x_right = (right.pos.x - other.pos.x) / d_y;

            y_delta.u = (left.uv.x - other.uv.x) / d_y;
            y_delta.v = (left.uv.y - other.uv.y) / d_y;
        }

        void GetTriangleLerpDeltasZ(const Vertex4d& left, const Vertex4d& right, const Vertex4d& other, TriDrawYDeltaZWUV &y_delta)
        {
            fp d_y = (left.pos.y - other.pos.y);

            y_delta.x_left = (left.pos.x - other.pos.x) / d_y;
            y_delta.x_right = (right.pos.x - other.pos.x) / d_y;
        }

        void LerpVertexXYZWUV(Vertex4d& out, const Vertex4d& left, const Vertex4d& right, fp frac)
        {
            out.pos.x = pLerp(left.pos.x, right.pos.x, frac);
            out.pos.y = pLerp(left.pos.y, right.pos.y, frac);
            out.pos.z = pLerp(left.pos.z, right.pos.z, frac);
            out.pos.w = pLerp(left.pos.w, right.pos.w, frac);

            out.uv.x = pLerp(left.uv.x, right.uv.x, frac);
            out.uv.y = pLerp(left.uv.y, right.uv.y, frac);
        }

        fp fracToY(fp frac)
        {
            fp halfFbY = pASR(current_viewport->height, 1);

            return ((halfFbY * -frac) + halfFbY);
        }

        fp fracToX(fp frac)
        {
            fp halfFbX = pASR(current_viewport->width, 1);

            return ((halfFbX * frac) + halfFbX);
        }

    private:
        TextureCacheBase* tex_cache = nullptr;
        const Material* current_material = nullptr;
        const RenderTargetViewport* current_viewport = nullptr;
        const RenderDeviceNearFarPlanes* z_planes = nullptr;
    };


    class RenderFlagsBase
    {
    public:
        virtual ~RenderFlagsBase() = default;
        virtual RenderTriangleBase* GetRender() = 0;
    };

    template<const int render_flags> class TriangleRenderFlags : public RenderFlagsBase
    {
    public:
        TriangleRenderFlags() {};
        RenderTriangleBase* GetRender() override {return dynamic_cast<RenderTriangleBase*>(&render_triangle);};

    private:
        RenderTriangle<render_flags> render_triangle;
    };

};

#endif // RENDERTRIANGLE_H
