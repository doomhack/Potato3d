#ifndef V3_H
#define V3_H

#include <cmath>
#include "fp.h"

namespace P3D
{

    template <class T> class V3
    {
    public:
        T x;
        T y;
        T z;

        V3(T x, T y, T z) : x(x), y(y), z(z) {}
        V3() {}
        //~V3() {}

        constexpr bool operator==(const V3& r)
        {
            return ((x == r.x) && (y == r.y) && (z == r.z));
        }

        constexpr V3 operator+(const V3& r) const
        {
            V3 v(x,y,z);
            return v+=r;
        }

        constexpr V3 operator-(const V3& r) const
        {
            V3 v(x,y,z);
            return v-=r;
        }

        constexpr V3 operator*(const V3& r) const
        {
            V3 v(x,y,z);
            return v*=r;
        }

        constexpr V3 operator*(const T& r) const
        {
            V3 v(x,y,z);
            return v*=r;
        }

        V3& operator+=(const V3& r)
        {
            x += r.x;
            y += r.y;
            z += r.z;

            return *this;
        }

        V3& operator-=(const V3& r)
        {
            x -= r.x;
            y -= r.y;
            z -= r.z;

            return *this;
        }

        V3& operator*=(const V3& r)
        {
            x *= r.x;
            y *= r.y;
            z *= r.z;

            return *this;
        }

        V3& operator*=(const T& r)
        {
            x *= r;
            y *= r;
            z *= r;

            return *this;
        }

        V3 CrossProduct(const V3& r) const
        {
            V3 v;

            v.x = (float)(((double)y * (double)r.z) - ((double)z * (double)r.y));
            v.y = (float)(((double)z * (double)r.x) - ((double)x * (double)r.z));
            v.z = (float)(((double)x * (double)r.y) - ((double)y * (double)r.x));

            return v;
        }

        V3 CrossProductNormalised(const V3& r) const
        {
            V3 v;

            double xd = ((double)y * (double)r.z) - ((double)z * (double)r.y);
            double yd = ((double)z * (double)r.x) - ((double)x * (double)r.z);
            double zd = ((double)x * (double)r.y) - ((double)y * (double)r.x);

            double len =    xd * xd +
                            yd * yd +
                            zd * zd;

            len = std::sqrt(len);

            v.x = (float)(xd / len);
            v.y = (float)(yd / len);
            v.z = (float)(zd / len);

            return v;
        }

        constexpr V3 Normalised() const
        {
            V3 v;

            T len = Length();

            v.x = (float)((float)x / len);
            v.y = (float)((float)y / len);
            v.z = (float)((float)z / len);

            return v;
        }

        constexpr T CrossProductZ(const V3& r) const
        {
            return ((x * r.y) - (y * r.x));
        }

        constexpr T DotProduct(const V3<T>& v2) const
        {
            return x * v2.x + y * v2.y + z * v2.z;
        }

        constexpr T Length() const
        {
            return  std::sqrt((x * x) + (y * y) + (z * z));
        }
    };

    typedef V3<float> V3F;
    typedef V3<double> V3D;
    typedef V3<FP16> V3FP;

}

#endif // V3_H
