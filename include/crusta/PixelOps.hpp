#ifndef _PixelOps_HPP_
#define _PixelOps_HPP_


#include <crusta/PixelOps.h>

#include <Geometry/Vector.h>


BEGIN_CRUSTA


//- single channel float -------------------------------------------------------

template <>
struct PixelOps<float>
{
    static float average(const float& a, const float& b,
                             const float& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        else
            return (a+b) * float(0.5);
    }

    static float average(const float& a, const float& b,
                             const float& c, const float& d,
                             const float& nodata)
    {
        const float p[4] = {a,b,c,d};

        float sum = 0;
        float sumWeights = 0;
        for (int i=0; i<4; ++i)
        {
            if (p[i] != nodata)
            {
                sum        += p[i] * float(0.25);
                sumWeights += float(0.25);
            }
        }

        if (sumWeights == 0)
            return nodata;
        else
            return sum / sumWeights;
    }

    static float minimum(const float& a, const float& b,
                             const float& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        else
            return std::min(a, b);
    }

    static float maximum(const float& a, const float& b,
                             const float& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        else
            return std::max(a, b);
    }
};


//- 3-channel unit8 ------------------------------------------------------------

template <>
struct PixelOps<Vector3ui8>
{
    static Vector3ui8 average(const Vector3ui8& a, const Vector3ui8& b,
                              const Vector3ui8& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        Vector3ui8 res(0,0,0);
        for (int i=0; i<Vector3ui8::dimension; ++i)
            res[i] = (((float)a[i] + (float)b[i]) * 0.5f) + 0.5f;
        return res;
    }

    static Vector3ui8 average(const Vector3ui8& a, const Vector3ui8& b,
                              const Vector3ui8& c, const Vector3ui8& d,
                              const Vector3ui8& nodata)
    {
        const Vector3ui8 p[4] = {a,b,c,d};

        float res[3] = {0.0f, 0.0f, 0.0f};
        float sumWeights = 0.0f;
        for (int i=0; i<4; ++i)
        {
            if (p[i] != nodata)
            {
                for (int j=0; j<Vector3ui8::dimension; ++j)
                    res[j] += (float)p[i][j] * 0.25f;
                sumWeights += 0.25f;
            }
        }

        if (sumWeights == 0.0f)
            return nodata;

        Vector3ui8 ret;
        for (int i=0; i<Vector3ui8::dimension; ++i)
            ret[i] = (res[i] / sumWeights) + 0.5f;

        return ret;
    }

    static Vector3ui8 minimum(const Vector3ui8& a, const Vector3ui8& b,
                              const Vector3ui8& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        Vector3ui8 res(0,0,0);
        for (int i=0; i<Vector3ui8::dimension; ++i)
            res[i] = std::min(a[i], b[i]);
        return res;
    }

    static Vector3ui8 maximum(const Vector3ui8& a, const Vector3ui8& b,
                              const Vector3ui8& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        Vector3ui8 res(0,0,0);
        for (int i=0; i<Vector3ui8::dimension; ++i)
            res[i] = std::max(a[i], b[i]);
        return res;
    }
};


END_CRUSTA


#endif //_PixelOps_HPP_
