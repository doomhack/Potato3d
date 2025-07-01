#ifndef V4_H
#define V4_H

#include "fp.h"
#include "v3.h"
#include "utils.h"

namespace P3D
{

    template <class T> class V4
    {
    public:
        T x;
        T y;
        T z;
        T w;

        V4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
        V4() {}
        //~V4() {}

        constexpr V4 operator+(const V4& r) const
        {
            V4 v(x,y,z,w);
            return v+=r;
        }

        constexpr V4 operator-(const V4& r) const
        {
            V4 v(x,y,z,w);
            return v-=r;
        }

        constexpr V4 operator*(const V4& r) const
        {
            V4 v(x,y,z,w);
            return v*=r;
        }

        constexpr V4 operator*(const T& r) const
        {
            V4 v(x,y,z,w);

            v.x *= r;
            v.y *= r;
            v.z *= r;
            v.w *= r;

            return v;
        }

        constexpr V4& operator+=(const V4& r)
        {
            x += r.x;
            y += r.y;
            z += r.z;
            w += r.w;

            return *this;
        }

        constexpr V4& operator-=(const V4& r)
        {
            x -= r.x;
            y -= r.y;
            z -= r.z;
            w -= r.w;

            return *this;
        }

        constexpr V4& operator*=(const V4& r)
        {
            x *= r.x;
            y *= r.y;
            z *= r.z;
            w *= r.w;

            return *this;
        }

        constexpr V3<T> CrossProduct(const V4& r) const
        {
            V3<T> v;

            v.x = ((y * r.z) - (z * r.y));
            v.y = ((z * r.x) - (x * r.z));
            v.z = ((x * r.y) - (y * r.x));

            return v;
        }

        constexpr void ToScreenSpace()
        {
            if (w != T(1))
            {
                x = x / w;
                y = y / w;
                z = z / w;
            }
        }
    };

    typedef V4<float> V4F;
    typedef V4<double> V4D;
    typedef V4<FP16> V4FP;

}

#endif // V4_H
