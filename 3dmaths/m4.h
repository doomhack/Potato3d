#ifndef M4_H
#define M4_H


#include "v3.h"
#include "v4.h"

namespace P3D
{

    typedef enum MatrixFlags
    {
        Updated = 1u
    } MatrixFlags;

    template <class T> class M4
    {
    public:
        explicit M4()   {flags = Updated;}

        bool GetFlag(MatrixFlags flag) const
        {
            return (flags & flag) > 0;
        }

        void SetFlag(MatrixFlags flag)
        {
            flags |= flag;
        }

        void UnSetFlag(MatrixFlags flag)
        {
            flags &= ~flag;
        }

        bool ResetFlag(MatrixFlags flag)
        {
            bool isSet = GetFlag(flag);
            UnSetFlag(flag);

            return isSet;
        }

        constexpr void setToIdentity()
        {
            SetFlag(Updated);

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
            SetFlag(Updated);

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
            SetFlag(Updated);

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
            SetFlag(Updated);

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
            T x, y, z, w;

#if 0
            x = T(vector.x) * m[0][0] +
                //T(vector.y) * m[1][0] +
                T(vector.z) * m[2][0] +
                m[3][0];

            y = //T(vector.x) * m[0][1] +
                T(vector.y) * m[1][1] +
                //T(vector.z) * m[2][1] +
                m[3][1];

            z = T(vector.x) * m[0][2] +
                //T(vector.y) * m[1][2] +
                T(vector.z) * m[2][2] +
                m[3][2];

            w = T(vector.x) * m[0][3] +
                //T(vector.y) * m[1][3] +
                T(vector.z) * m[2][3] +
                m[3][3];
#else
                x = T(vector.x) * m[0][0] +
                    T(vector.y) * m[1][0] +
                    T(vector.z) * m[2][0] +
                    m[3][0];

                y = T(vector.x) * m[0][1] +
                    T(vector.y) * m[1][1] +
                    T(vector.z) * m[2][1] +
                    m[3][1];

                z = T(vector.x) * m[0][2] +
                    T(vector.y) * m[1][2] +
                    T(vector.z) * m[2][2] +
                    m[3][2];

                w = T(vector.x) * m[0][3] +
                    T(vector.y) * m[1][3] +
                    T(vector.z) * m[2][3] +
                    m[3][3];
#endif

                return V4<T>(x, y, z, w);
        }

        constexpr void translate(const V3<T>& vector)
        {
            SetFlag(Updated);

            T vx = vector.x;
            T vy = vector.y;
            T vz = vector.z;

            m[3][0] += m[0][0] * vx + m[1][0] * vy + m[2][0] * vz;
            m[3][1] += m[0][1] * vx + m[1][1] * vy + m[2][1] * vz;
            m[3][2] += m[0][2] * vx + m[1][2] * vy + m[2][2] * vz;
            m[3][3] += m[0][3] * vx + m[1][3] * vy + m[2][3] * vz;
        }

        constexpr void rotateY(T angle)
        {
            if (angle == 0)
                return;

            SetFlag(Updated);

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
                T a = pD2R(angle);
                c = std::cos((float)a);
                s = std::sin((float)a);

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

            return;
        }

        constexpr void rotateX(T angle)
        {
            if (angle == 0)
                return;

            SetFlag(Updated);

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
                T a = pD2R(angle);
                c = std::cos((float)a);
                s = std::sin((float)a);

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
            return;
        }

        constexpr void rotateZ(T angle)
        {
            if (angle == 0)
                return;

            SetFlag(Updated);

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
                T a = pD2R(angle);
                c = std::cos((float)a);
                s = std::sin((float)a);
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
            return;
        }

        constexpr void perspective(T verticalAngle, T aspectRatio, T nearPlane, T farPlane)
        {
            // Bail out if the projection volume is zero-sized.
            if (nearPlane == farPlane || aspectRatio == 0)
                return;

            SetFlag(Updated);

            // Construct the projection.
            M4 m;
            float radians = pD2R(verticalAngle / 2);
            float sine = std::sin(radians);

            if (sine == 0)
                return;

            float cosRads = std::cos(radians);

            float cotan = (cosRads / sine);
            float clip = farPlane - nearPlane;
            float clip2 = -(nearPlane + farPlane) / clip;
            float clip3 = -(2.0f * (float)nearPlane * (float)farPlane) / clip;

            m.m[0][0] = (cotan / (float)aspectRatio);
            m.m[1][0] = 0;
            m.m[2][0] = 0;
            m.m[3][0] = 0;
            m.m[0][1] = 0;
            m.m[1][1] = cotan;
            m.m[2][1] = 0;
            m.m[3][1] = 0;
            m.m[0][2] = 0;
            m.m[1][2] = 0;
            m.m[2][2] = clip2;
            m.m[3][2] = clip3;
            m.m[0][3] = 0;
            m.m[1][3] = 0;
            m.m[2][3] = -1;
            m.m[3][3] = 0;

            // Apply the projection.
            *this *= m;
        }

        constexpr V4<T> inverted() const
        {
            V4<T> inv;
            inv.m = {0};

            T det = matrixDet4(m);
            if (det == 0)
            {
                return M4<T>();
            }

            det = pReciprocal(det);

            inv.m[0][0] =  matrixDet3(m, 1, 2, 3, 1, 2, 3) * det;
            inv.m[0][1] = -matrixDet3(m, 0, 2, 3, 1, 2, 3) * det;
            inv.m[0][2] =  matrixDet3(m, 0, 1, 3, 1, 2, 3) * det;
            inv.m[0][3] = -matrixDet3(m, 0, 1, 2, 1, 2, 3) * det;
            inv.m[1][0] = -matrixDet3(m, 1, 2, 3, 0, 2, 3) * det;
            inv.m[1][1] =  matrixDet3(m, 0, 2, 3, 0, 2, 3) * det;
            inv.m[1][2] = -matrixDet3(m, 0, 1, 3, 0, 2, 3) * det;
            inv.m[1][3] =  matrixDet3(m, 0, 1, 2, 0, 2, 3) * det;
            inv.m[2][0] =  matrixDet3(m, 1, 2, 3, 0, 1, 3) * det;
            inv.m[2][1] = -matrixDet3(m, 0, 2, 3, 0, 1, 3) * det;
            inv.m[2][2] =  matrixDet3(m, 0, 1, 3, 0, 1, 3) * det;
            inv.m[2][3] = -matrixDet3(m, 0, 1, 2, 0, 1, 3) * det;
            inv.m[3][0] = -matrixDet3(m, 1, 2, 3, 0, 1, 2) * det;
            inv.m[3][1] =  matrixDet3(m, 0, 2, 3, 0, 1, 2) * det;
            inv.m[3][2] = -matrixDet3(m, 0, 1, 3, 0, 1, 2) * det;
            inv.m[3][3] =  matrixDet3(m, 0, 1, 2, 0, 1, 2) * det;

            return inv;
        }

        constexpr inline T matrixDet4(const T m[4][4]) const
        {
            T det;
            det  = m[0][0] * matrixDet3(m, 1, 2, 3, 1, 2, 3);
            det -= m[1][0] * matrixDet3(m, 0, 2, 3, 1, 2, 3);
            det += m[2][0] * matrixDet3(m, 0, 1, 3, 1, 2, 3);
            det -= m[3][0] * matrixDet3(m, 0, 1, 2, 1, 2, 3);
            return det;
        }

        constexpr inline T matrixDet3(const T m[4][4], int col0, int col1, int col2, int row0, int row1, int row2)
        {
            return m[col0][row0] *
                        (m[col1][row1] * m[col2][row2] -
                         m[col1][row2] * m[col2][row1]) -
                   m[col1][row0] *
                        (m[col0][row1] * m[col2][row2] -
                         m[col0][row2] * m[col2][row1]) +
                   m[col2][row0] *
                        (m[col0][row1] * m[col1][row2] -
                         m[col0][row2] * m[col1][row1]);
        }

    private:
        T m[4][4];



        unsigned int flags;
    };

    typedef M4<float> M4F;
    typedef M4<double> M4D;
    typedef M4<FP> M4FP;

}

#endif // M4_H
