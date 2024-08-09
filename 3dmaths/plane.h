#ifndef PLANE_H
#define PLANE_H

#include "v3.h"

namespace P3D
{
typedef enum FrustrumPlanes : unsigned int
{
    Near = 0,
    Far = 1,
    Left = 2,
    Right = 3,
    Top = 4,
    Bottom = 5
} FrustrumPlanes;

template <class T> class Plane
{
public:
    constexpr Plane() {}
    constexpr Plane(V3<T> normal, T distance) : n(normal), d(distance) {}

    constexpr void Normalise()
    {
        T length = n.Length();
        n = n.Normalised();
        d /= length;
    }

    constexpr T DistanceToPoint(const V3<T>& v)
    {
        return n.DotProduct(v) + d;
    }

    constexpr bool TriangleIsFrontside(const V3<T>& v1, const V3<T>& v2, const V3<T>& v3)
    {
        if ((DistanceToPoint(v1) < 0) && (DistanceToPoint(v2) < 0) && (DistanceToPoint(v3) < 0))
            return false;

        return true;
    }

    constexpr V3<T> Normal() const  {return n;}
    constexpr T Distance() const    {return d;}

private:
    V3<T> n;
    T d;
};
}

#endif // PLANE_H

