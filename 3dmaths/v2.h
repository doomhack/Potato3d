#ifndef V2_H
#define V2_H

#include "fp.h"

namespace P3D
{
    template <class T> class V2
    {
    public:
        T x,y;

        V2(T x, T y) : x(x), y(y) {}
        V2() {}
        ~V2() {}

        constexpr V2& operator=(const V2& r)
        {
            x = r.x;
            y = r.y;

            return *this;
        }

        constexpr V2 operator+(const V2& r) const
        {
            V2 v(x,y);
            return v+=r;
        }

        constexpr V2 operator-(const V2& r) const
        {
            V2 v(x,y);
            return v-=r;
        }

        const V2 operator*(const V2& r) const
        {
            V2 v(x,y);
            return v*=r;
        }

        constexpr V2 operator*(const T& r) const
        {
            V2 v(x,y);

            v.x *= r;
            v.y *= r;

            return v;
        }

        constexpr V2& operator+=(const V2& r)
        {
            x += r.x;
            y += r.y;
            return *this;
        }

        constexpr V2& operator-=(const V2& r)
        {
            x -= r.x;
            y -= r.y;
            return *this;
        }

        constexpr V2& operator*=(const V2& r)
        {
            x *= r.x;
            y *= r.y;
            return *this;
        }
    };

    typedef V2<float> V2F;
    typedef V2<double> V2D;
    typedef V2<FP16> V2FP;

}
#endif // V2_H

