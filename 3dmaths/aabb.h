#ifndef AABB_H
#define AABB_H

#include "v3.h"
#include "utils.h"

namespace P3D
{

    template <class T> class AABB
    {
    public:
        explicit constexpr AABB()
        {
            x1 = y1 = z1 = std::numeric_limits<T>::max();
            x2 = y2 = z2 = std::numeric_limits<T>::min();
        }

        explicit constexpr AABB(const V3<T>& point)
        {
            AddPoint(point);
        }

        explicit constexpr AABB(const V3<T>& point, T size)
        {
            x1 = point.x - pASR(size,1);
            x2 = point.x + pASR(size,1);

            y1 = point.y - pASR(size,1);
            y2 = point.y + pASR(size,1);

            z1 = point.z - pASR(size,1);
            z2 = point.z + pASR(size,1);
        }

        explicit constexpr AABB(T xmin, T xmax, T ymin, T ymax, T zmin, T zmax)
        {
            x1 = xmin, x2 = xmax, y1 = ymin, y2 = ymax, z1 = zmin, z2 = zmax;
        }

        constexpr void AddPoint(const V3<T>& point)
        {
            x1 = pMin(x1, point.x);
            x2 = pMax(x2, point.x);

            y1 = pMin(y1, point.y);
            y2 = pMax(y2, point.y);

            z1 = pMin(z1, point.z);
            z2 = pMax(z2, point.z);
        }

        constexpr void AddTriangle(const V3<T>& v1, const V3<T>& v2, const V3<T>& v3)
        {
            AddPoint(v1);
            AddPoint(v2);
            AddPoint(v3);
        }

        constexpr void AddAABB(const AABB& other)
        {
            x1 = pMin(x1, other.x1);
            x2 = pMax(x2, other.x2);

            y1 = pMin(y1, other.y1);
            y2 = pMax(y2, other.y2);

            z1 = pMin(z1, other.z1);
            z2 = pMax(z2, other.z2);
        }

        constexpr bool Intersect(const V3<T>& point) const
        {
            if(x1 > point.x)
                return false;

            if(x2 < point.x)
                return false;

            if(y1 > point.y)
                return false;

            if(y2 < point.y)
                return false;

            if(z1 > point.z)
                return false;

            if(z2 < point.z)
                return false;

            return true;
        }

        constexpr bool Intersect(const AABB& other) const
        {
            if(x1 > other.x2)
                return false;

            if(x2 < other.x1)
                return false;

            if(y1 > other.y2)
                return false;

            if(y2 < other.y1)
                return false;

            if(z1 > other.z2)
                return false;

            if(z2 < other.z1)
                return false;

            return true;
        }

    private:

        T x1, x2;
        T y1, y2;
        T z1, z2;
    };

}

#endif // AABB_H
