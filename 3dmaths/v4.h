#ifndef V4_H
#define V4_H

#include "fp.h"

#include "v3.h"

template <class T> class V4
{
public:
    T x,y,z,w;

    V4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    V4() {}
    ~V4() {}

    V4& operator=(const V4& r)
    {
        x = r.x;
        y = r.y;
        z = r.z;
        w = r.w;

        return *this;
    }

    V4 operator+(const V4& r)
    {
        V4 v(x,y,z,w);
        return v+=r;
    }

    V4 operator-(const V4& r)
    {
        V4 v(x,y,z,w);
        return v-=r;
    }

    V4 operator*(const V4& r)
    {
        V4 v(x,y,z,w);
        return v*=r;
    }

    V4 operator*(const T& r)
    {
        V4 v(x,y,z,w);

        v.x *= r;
        v.y *= r;
        v.z *= r;
        v.w *= r;

        return v;
    }

    V4& operator+=(const V4& r)
    {
        x += r.x;
        y += r.y;
        z += r.z;
        w += r.w;

        return *this;
    }

    V4& operator-=(const V4& r)
    {
        x -= r.x;
        y -= r.y;
        z -= r.z;
        w -= r.w;

        return *this;
    }

    V4& operator*=(const V4& r)
    {
        x *= r.x;
        y *= r.y;
        z *= r.z;
        w *= r.w;

        return *this;
    }

    V3<T> CrossProduct(const V4& r)
    {
        V3<T> v;

        v.x = ((y * r.z) - (z * r.y));
        v.y = ((z * r.x) - (x * r.z));
        v.z = ((x * r.y) - (y * r.x));

        return v;
    }

    T CrossProductZ(const V4& r)
    {
        return ((x * r.y) - (y * r.x));
    }

    V4<T> ToScreenSpace()
    {
        if (w == T(1))
            return V4<T>(x, y, z, w);
        else
            return V4<T>(x / w, y / w, z / w, w);
    }
};

typedef V4<float> V4F;
typedef V4<double> V4D;
typedef V4<FP> V4FP;


#endif // V4_H
