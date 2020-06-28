#ifndef V3_H
#define V3_H

#include <cmath>
#include "fp.h"

namespace P3D
{

    template <class T> class V3
    {
    public:
        T x,y,z;

        V3(T x, T y, T z) : x(x), y(y), z(z) {}
        V3() {}
        ~V3() {}

        constexpr V3& operator=(const V3& r)
        {
            x = r.x;
            y = r.y;
            z = r.z;

            return *this;
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

        constexpr V3 CrossProduct(const V3& r) const
        {
            V3 v;

            v.x = ((y * r.z) - (z * r.y));
            v.y = ((z * r.x) - (x * r.z));
            v.z = ((x * r.y) - (y * r.x));

            return v;
        }

        constexpr T CrossProductZ(const V3& r) const
        {
            return ((x * r.y) - (y * r.x));
        }


        constexpr void Normalise()
        {
            // Need some extra precision if the length is very small.
            float len = float(x) * float(x) +
                        float(y) * float(y) +
                        float(z) * float(z);

            len = std::sqrt(len);

            x = (x / len);
            y = (y / len);
            z = (z / len);
        }

        T DotProduct(const V3<T>& v2) const
        {
            return x * v2.x + y * v2.y + z * v2.z;
        }
    };

    typedef V3<float> V3F;
    typedef V3<double> V3D;
    typedef V3<FP> V3FP;

}

#endif // V3_H
