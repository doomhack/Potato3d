#ifndef M4_H
#define M4_H

#include "v3.h"
#include "v4.h"
#include "plane.h"

namespace P3D
{

    typedef enum MatrixFlags : unsigned int
    {
        Updated = 1u
    } MatrixFlags;

    template <class T> class M4
    {
    public:
        explicit M4(){}



        constexpr void setToIdentity()
        {
            m[0][0] = 1;
            m[0][1] = 0;
            m[0][2] = 0;
            m[0][3] = 0;
            m[1][0] = 0;
            m[1][1] = 1;
            m[1][2] = 0;
            m[1][3] = 0;
            m[2][0] = 0;
            m[2][1] = 0;
            m[2][2] = 1;
            m[2][3] = 0;
            m[3][0] = 0;
            m[3][1] = 0;
            m[3][2] = 0;
            m[3][3] = 1;
        }

        constexpr M4& operator+=(const M4& other)
        {
            m[0][0] += other.m[0][0];
            m[0][1] += other.m[0][1];
            m[0][2] += other.m[0][2];
            m[0][3] += other.m[0][3];
            m[1][0] += other.m[1][0];
            m[1][1] += other.m[1][1];
            m[1][2] += other.m[1][2];
            m[1][3] += other.m[1][3];
            m[2][0] += other.m[2][0];
            m[2][1] += other.m[2][1];
            m[2][2] += other.m[2][2];
            m[2][3] += other.m[2][3];
            m[3][0] += other.m[3][0];
            m[3][1] += other.m[3][1];
            m[3][2] += other.m[3][2];
            m[3][3] += other.m[3][3];
            return *this;
        }

        constexpr M4& operator-=(const M4& other)
        {
            m[0][0] -= other.m[0][0];
            m[0][1] -= other.m[0][1];
            m[0][2] -= other.m[0][2];
            m[0][3] -= other.m[0][3];
            m[1][0] -= other.m[1][0];
            m[1][1] -= other.m[1][1];
            m[1][2] -= other.m[1][2];
            m[1][3] -= other.m[1][3];
            m[2][0] -= other.m[2][0];
            m[2][1] -= other.m[2][1];
            m[2][2] -= other.m[2][2];
            m[2][3] -= other.m[2][3];
            m[3][0] -= other.m[3][0];
            m[3][1] -= other.m[3][1];
            m[3][2] -= other.m[3][2];
            m[3][3] -= other.m[3][3];
            return *this;
        }

        constexpr M4& operator*=(const M4& other)
        {
            T m0, m1, m2;
            m0 = m[0][0] * other.m[0][0]
                    + m[1][0] * other.m[0][1]
                    + m[2][0] * other.m[0][2]
                    + m[3][0] * other.m[0][3];
            m1 = m[0][0] * other.m[1][0]
                    + m[1][0] * other.m[1][1]
                    + m[2][0] * other.m[1][2]
                    + m[3][0] * other.m[1][3];
            m2 = m[0][0] * other.m[2][0]
                    + m[1][0] * other.m[2][1]
                    + m[2][0] * other.m[2][2]
                    + m[3][0] * other.m[2][3];
            m[3][0] = m[0][0] * other.m[3][0]
                    + m[1][0] * other.m[3][1]
                    + m[2][0] * other.m[3][2]
                    + m[3][0] * other.m[3][3];
            m[0][0] = m0;
            m[1][0] = m1;
            m[2][0] = m2;

            m0 = m[0][1] * other.m[0][0]
                    + m[1][1] * other.m[0][1]
                    + m[2][1] * other.m[0][2]
                    + m[3][1] * other.m[0][3];
            m1 = m[0][1] * other.m[1][0]
                    + m[1][1] * other.m[1][1]
                    + m[2][1] * other.m[1][2]
                    + m[3][1] * other.m[1][3];
            m2 = m[0][1] * other.m[2][0]
                    + m[1][1] * other.m[2][1]
                    + m[2][1] * other.m[2][2]
                    + m[3][1] * other.m[2][3];
            m[3][1] = m[0][1] * other.m[3][0]
                    + m[1][1] * other.m[3][1]
                    + m[2][1] * other.m[3][2]
                    + m[3][1] * other.m[3][3];
            m[0][1] = m0;
            m[1][1] = m1;
            m[2][1] = m2;

            m0 = m[0][2] * other.m[0][0]
                    + m[1][2] * other.m[0][1]
                    + m[2][2] * other.m[0][2]
                    + m[3][2] * other.m[0][3];
            m1 = m[0][2] * other.m[1][0]
                    + m[1][2] * other.m[1][1]
                    + m[2][2] * other.m[1][2]
                    + m[3][2] * other.m[1][3];
            m2 = m[0][2] * other.m[2][0]
                    + m[1][2] * other.m[2][1]
                    + m[2][2] * other.m[2][2]
                    + m[3][2] * other.m[2][3];
            m[3][2] = m[0][2] * other.m[3][0]
                    + m[1][2] * other.m[3][1]
                    + m[2][2] * other.m[3][2]
                    + m[3][2] * other.m[3][3];
            m[0][2] = m0;
            m[1][2] = m1;
            m[2][2] = m2;

            m0 = m[0][3] * other.m[0][0]
                    + m[1][3] * other.m[0][1]
                    + m[2][3] * other.m[0][2]
                    + m[3][3] * other.m[0][3];
            m1 = m[0][3] * other.m[1][0]
                    + m[1][3] * other.m[1][1]
                    + m[2][3] * other.m[1][2]
                    + m[3][3] * other.m[1][3];
            m2 = m[0][3] * other.m[2][0]
                    + m[1][3] * other.m[2][1]
                    + m[2][3] * other.m[2][2]
                    + m[3][3] * other.m[2][3];
            m[3][3] = m[0][3] * other.m[3][0]
                    + m[1][3] * other.m[3][1]
                    + m[2][3] * other.m[3][2]
                    + m[3][3] * other.m[3][3];
            m[0][3] = m0;
            m[1][3] = m1;
            m[2][3] = m2;
            return *this;
        }

        constexpr M4 operator*(const M4& m2) const
        {
            M4 m3;
            m3.m[0][0] = m[0][0] * m2.m[0][0]
                      + m[1][0] * m2.m[0][1]
                      + m[2][0] * m2.m[0][2]
                      + m[3][0] * m2.m[0][3];
            m3.m[0][1] = m[0][1] * m2.m[0][0]
                      + m[1][1] * m2.m[0][1]
                      + m[2][1] * m2.m[0][2]
                      + m[3][1] * m2.m[0][3];
            m3.m[0][2] = m[0][2] * m2.m[0][0]
                      + m[1][2] * m2.m[0][1]
                      + m[2][2] * m2.m[0][2]
                      + m[3][2] * m2.m[0][3];
            m3.m[0][3] = m[0][3] * m2.m[0][0]
                      + m[1][3] * m2.m[0][1]
                      + m[2][3] * m2.m[0][2]
                      + m[3][3] * m2.m[0][3];

            m3.m[1][0] = m[0][0] * m2.m[1][0]
                      + m[1][0] * m2.m[1][1]
                      + m[2][0] * m2.m[1][2]
                      + m[3][0] * m2.m[1][3];
            m3.m[1][1] = m[0][1] * m2.m[1][0]
                      + m[1][1] * m2.m[1][1]
                      + m[2][1] * m2.m[1][2]
                      + m[3][1] * m2.m[1][3];
            m3.m[1][2] = m[0][2] * m2.m[1][0]
                      + m[1][2] * m2.m[1][1]
                      + m[2][2] * m2.m[1][2]
                      + m[3][2] * m2.m[1][3];
            m3.m[1][3] = m[0][3] * m2.m[1][0]
                      + m[1][3] * m2.m[1][1]
                      + m[2][3] * m2.m[1][2]
                      + m[3][3] * m2.m[1][3];

            m3.m[2][0] = m[0][0] * m2.m[2][0]
                      + m[1][0] * m2.m[2][1]
                      + m[2][0] * m2.m[2][2]
                      + m[3][0] * m2.m[2][3];
            m3.m[2][1] = m[0][1] * m2.m[2][0]
                      + m[1][1] * m2.m[2][1]
                      + m[2][1] * m2.m[2][2]
                      + m[3][1] * m2.m[2][3];
            m3.m[2][2] = m[0][2] * m2.m[2][0]
                      + m[1][2] * m2.m[2][1]
                      + m[2][2] * m2.m[2][2]
                      + m[3][2] * m2.m[2][3];
            m3.m[2][3] = m[0][3] * m2.m[2][0]
                      + m[1][3] * m2.m[2][1]
                      + m[2][3] * m2.m[2][2]
                      + m[3][3] * m2.m[2][3];

            m3.m[3][0] = m[0][0] * m2.m[3][0]
                      + m[1][0] * m2.m[3][1]
                      + m[2][0] * m2.m[3][2]
                      + m[3][0] * m2.m[3][3];
            m3.m[3][1] = m[0][1] * m2.m[3][0]
                      + m[1][1] * m2.m[3][1]
                      + m[2][1] * m2.m[3][2]
                      + m[3][1] * m2.m[3][3];
            m3.m[3][2] = m[0][2] * m2.m[3][0]
                      + m[1][2] * m2.m[3][1]
                      + m[2][2] * m2.m[3][2]
                      + m[3][2] * m2.m[3][3];
            m3.m[3][3] = m[0][3] * m2.m[3][0]
                      + m[1][3] * m2.m[3][1]
                      + m[2][3] * m2.m[3][2]
                      + m[3][3] * m2.m[3][3];
            return m3;
        }

        constexpr V4<T> operator*(const V3<T>& vector) const
        {
            const T x = T(vector.x) * m[0][0] +
                        T(vector.y) * m[1][0] +
                        T(vector.z) * m[2][0] +
                        m[3][0];

            const T y = T(vector.x) * m[0][1] +
                        T(vector.y) * m[1][1] +
                        T(vector.z) * m[2][1] +
                        m[3][1];

            const T z = T(vector.x) * m[0][2] +
                        T(vector.y) * m[1][2] +
                        T(vector.z) * m[2][2] +
                        m[3][2];

            const T w = T(vector.x) * m[0][3] +
                        T(vector.y) * m[1][3] +
                        T(vector.z) * m[2][3] +
                        m[3][3];


            return V4<T>(x, y, z, w);
        }

        constexpr void translate(const V3<T>& vector)
        {
            const T vx = vector.x;
            const T vy = vector.y;
            const T vz = vector.z;

            m[3][0] += m[0][0] * vx + m[1][0] * vy + m[2][0] * vz;
            m[3][1] += m[0][1] * vx + m[1][1] * vy + m[2][1] * vz;
            m[3][2] += m[0][2] * vx + m[1][2] * vy + m[2][2] * vz;
            m[3][3] += m[0][3] * vx + m[1][3] * vy + m[2][3] * vz;
        }

        constexpr void rotateY(const T angle)
        {
            if (angle == 0)
                return;

            T c, s;

            if (angle == 90 || angle == -270)
            {
                s = 1;
                c = 0;
            }
            else if (angle == -90 || angle == 270)
            {
                s = -1;
                c = 0;
            }
            else if (angle == 180 || angle == -180)
            {
                s = 0;
                c = -1;
            }
            else
            {
                const T a = pD2R(angle);
                c = T(std::cos((float)a));
                s = T(std::sin((float)a));
            }


            T tmp;
            m[2][0] = (tmp = m[2][0]) * c + m[0][0] * s;
            m[0][0] = m[0][0] * c - tmp * s;
            m[2][1] = (tmp = m[2][1]) * c + m[0][1] * s;
            m[0][1] = m[0][1] * c - tmp * s;
            m[2][2] = (tmp = m[2][2]) * c + m[0][2] * s;
            m[0][2] = m[0][2] * c - tmp * s;
            m[2][3] = (tmp = m[2][3]) * c + m[0][3] * s;
            m[0][3] = m[0][3] * c - tmp * s;
        }

        constexpr void rotateX(const T angle)
        {
            if (angle == 0)
                return;

            T c, s;

            if (angle == 90 || angle == -270)
            {
                s = 1;
                c = 0;
            }
            else if (angle == -90 || angle == 270)
            {
                s = -1;
                c = 0;
            }
            else if (angle == 180 || angle == -180)
            {
                s = 0;
                c = -1;
            }
            else
            {
                const T a = pD2R(angle);
                c = T(std::cos((float)a));
                s = T(std::sin((float)a));
            }

            T tmp;
            m[1][0] = (tmp = m[1][0]) * c + m[2][0] * s;
            m[2][0] = m[2][0] * c - tmp * s;
            m[1][1] = (tmp = m[1][1]) * c + m[2][1] * s;
            m[2][1] = m[2][1] * c - tmp * s;
            m[1][2] = (tmp = m[1][2]) * c + m[2][2] * s;
            m[2][2] = m[2][2] * c - tmp * s;
            m[1][3] = (tmp = m[1][3]) * c + m[2][3] * s;
            m[2][3] = m[2][3] * c - tmp * s;
        }

        constexpr void rotateZ(const T angle)
        {
            if (angle == 0)
                return;

            T c, s;

            if (angle == 90 || angle == -270)
            {
                s = 1;
                c = 0;
            }
            else if (angle == -90 || angle == 270)
            {
                s = -1;
                c = 0;
            }
            else if (angle == 180 || angle == -180)
            {
                s = 0;
                c = -1;
            }
            else
            {
                const T a = pD2R(angle);
                c = T(std::cos((float)a));
                s = T(std::sin((float)a));
            }

            T tmp;
            m[0][0] = (tmp = m[0][0]) * c + m[1][0] * s;
            m[1][0] = m[1][0] * c - tmp * s;
            m[0][1] = (tmp = m[0][1]) * c + m[1][1] * s;
            m[1][1] = m[1][1] * c - tmp * s;
            m[0][2] = (tmp = m[0][2]) * c + m[1][2] * s;
            m[1][2] = m[1][2] * c - tmp * s;
            m[0][3] = (tmp = m[0][3]) * c + m[1][3] * s;
            m[1][3] = m[1][3] * c - tmp * s;
        }

        constexpr void lookAt(const V3<T>& eye, const V3<T>& center, const V3<T>& up)
        {
            const V3<T>& forward = center - eye;

            forward = forward.Normalised();

            const V3<T> side = forward.CrossProductNormalised(up);
            const V3<T> upVector = side.CrossProduct(forward);

            m[0][0] = side.x();
            m[1][0] = side.y();
            m[2][0] = side.z();
            m[3][0] = T(0);
            m[0][1] = upVector.x();
            m[1][1] = upVector.y();
            m[2][1] = upVector.z();
            m[3][1] = T(0);
            m[0][2] = -forward.x();
            m[1][2] = -forward.y();
            m[2][2] = -forward.z();
            m[3][2] = T(0);
            m[0][3] = T(0);
            m[1][3] = T(0);
            m[2][3] = T(0);
            m[3][3] = T(0);

            translate(-eye);
        }

        constexpr void perspective(const T verticalAngle, const T aspectRatio, const T nearPlane, const T farPlane)
        {
            if (nearPlane == farPlane || aspectRatio == 0)
                return;

            const T va = verticalAngle;
            const T ar = aspectRatio;
            const T scale = T(1.0) / std::tan((float)(va * T(0.5) * T(M_PI) / T(180.0)));

            const T fp = farPlane;
            const T np = nearPlane;
            const T fd = (fp - np);

            m[0][0] = T(scale / ar);
            m[1][0] = 0;
            m[2][0] = 0;
            m[3][0] = 0;

            m[0][1] = 0;
            m[1][1] = T(scale);
            m[2][1] = 0;
            m[3][1] = 0;

            m[0][2] = 0;
            m[1][2] = 0;
            m[2][2] = T(-(fp / fd));
            m[3][2] = T(-fp * (np / fd));

            m[0][3] = 0;
            m[1][3] = 0;
            m[2][3] = -1;
            m[3][3] = 0;
        }

        constexpr void orthographic(const T left, const T right, const T bottom, const T top, const T nearPlane, const T farPlane)
        {
            if (left == right || bottom == top || nearPlane == farPlane)
                return;

            const T fp = farPlane;
            const T np = nearPlane;
            const T l = left;
            const T r = right;
            const T t = top;
            const T b = bottom;

            const T width = (r - l);
            const T height = (t - b);
            const T depth = (fp - np);

            m[0][0] = T(2) / width;
            m[1][0] = 0;
            m[2][0] = 0;
            m[3][0] = -(l + r) / width;

            m[0][1] = 0;
            m[1][1] = T(2) / height;
            m[2][1] = 0;
            m[3][1] = -(t + b) / height;

            m[0][2] = 0;
            m[1][2] = 0;
            m[2][2] = T(-2) / depth;
            m[3][2] = -(np + fp) / depth;

            m[0][3] = 0;
            m[1][3] = 0;
            m[2][3] = 0;
            m[3][3] = 1;
        }

        constexpr M4 Inverted() const
        {
            M4 r;
            r.setToIdentity();

            r.m[0][0] = m[0][0];
            r.m[1][0] = m[0][1];
            r.m[2][0] = m[0][2];

            r.m[0][1] = m[1][0];
            r.m[1][1] = m[1][1];
            r.m[2][1] = m[1][2];

            r.m[0][2] = m[2][0];
            r.m[1][2] = m[2][1];
            r.m[2][2] = m[2][2];

            r.m[0][3] = 0;
            r.m[1][3] = 0;
            r.m[2][3] = 0;

            r.m[3][0] = -(r.m[0][0] * m[3][0] + r.m[1][0] * m[3][1] + r.m[2][0] * m[3][2]);
            r.m[3][1] = -(r.m[0][1] * m[3][0] + r.m[1][1] * m[3][1] + r.m[2][1] * m[3][2]);
            r.m[3][2] = -(r.m[0][2] * m[3][0] + r.m[1][2] * m[3][1] + r.m[2][2] * m[3][2]);
            r.m[3][3] = 1;

            return r;
        }

        constexpr void ExtractFrustrumPlanes(Plane<T> planes[6]) const
        {
            planes[Near]    = Plane<T>(V3<T>(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2]), m[3][3] + m[3][2]);
            planes[Far]     = Plane<T>(V3<T>(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2]), m[3][3] - m[3][2]);
            planes[Left]    = Plane<T>(V3<T>(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0]), m[3][3] + m[3][0]);
            planes[Right]   = Plane<T>(V3<T>(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0]), m[3][3] - m[3][0]);
            planes[Top]     = Plane<T>(V3<T>(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1]), m[3][3] - m[3][1]);
            planes[Bottom]  = Plane<T>(V3<T>(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1]), m[3][3] + m[3][1]);
        }

    private:
        T m[4][4];
    };

    typedef M4<float> M4F;
    typedef M4<double> M4D;
    typedef M4<FP16> M4FP;

}

#endif // M4_H
