#ifndef V2_H
#define V2_H

#include "fp.h"

template <class T> class V2
{
public:
    T x,y;

    V2(T x, T y) : x(x), y(y) {}
    V2() {}
    ~V2() {}

    V2& operator=(const V2& r)
    {
        x = r.x;
        y = r.y;

        return *this;
    }

    V2 operator+(const V2& r)
    {
        V2 v(x,y);
        return v+=r;
    }

    V2 operator-(const V2& r)
    {
        V2 v(x,y);
        return v-=r;
    }

    V2 operator*(const V2& r)
    {
        V2 v(x,y);
        return v*=r;
    }

    V2 operator*(const T& r)
    {
        V2 v(x,y);

        v.x *= r;
        v.y *= r;

        return v;
    }

    V2& operator+=(const V2& r)
    {
        x += r.x;
        y += r.y;
        return *this;
    }

    V2& operator-=(const V2& r)
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }

    V2& operator*=(const V2& r)
    {
        x *= r.x;
        y *= r.y;
        return *this;
    }
};

typedef V2<float> V2F;
typedef V2<double> V2D;
typedef V2<FP> V2FP;


#endif // V2_H

