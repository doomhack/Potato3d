#ifndef DIVIDE_H
#define DIVIDE_H

#ifdef __arm__
    extern "C" unsigned int udiv64_arm (unsigned int a, unsigned int b, unsigned int c);
#endif

template <class T>
constexpr inline T FixedDiv(T a, T b)
{
    return (a * (1 << 16)) / b;
}

template<>
inline int FixedDiv(int a, int b)
{
#ifndef __arm__
    return (int)(((long long int)a << 16) / b);
#else

    int sign = (a^b) < 0; /* different signs */

    a = a<0 ? -a:a;
    b = b<0 ? -b:b;

    unsigned int l = (a << 16);
    unsigned int h = (a >> 16);

    int q = udiv64_arm (h,l,b);
    if (sign)
        q = -q;

    return q;
#endif

}

#endif // DIVIDE_H
