#ifndef _PixelOps_HPP_
#define _PixelOps_HPP_


#include <crustacore/PixelOps.h>


namespace crusta {


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
struct PixelOps<Geometry::Vector<uint8_t,3> >
{
    static Geometry::Vector<uint8_t,3> average(const Geometry::Vector<uint8_t,3>& a, const Geometry::Vector<uint8_t,3>& b,
                              const Geometry::Vector<uint8_t,3>& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        Geometry::Vector<uint8_t,3> res(0,0,0);
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
            res[i] = (((float)a[i] + (float)b[i]) * 0.5f) + 0.5f;
        return res;
    }

    static Geometry::Vector<uint8_t,3> average(const Geometry::Vector<uint8_t,3>& a, const Geometry::Vector<uint8_t,3>& b,
                              const Geometry::Vector<uint8_t,3>& c, const Geometry::Vector<uint8_t,3>& d,
                              const Geometry::Vector<uint8_t,3>& nodata)
    {
        const Geometry::Vector<uint8_t,3> p[4] = {a,b,c,d};

        float res[3] = {0.0f, 0.0f, 0.0f};
        float sumWeights = 0.0f;
        for (int i=0; i<4; ++i)
        {
            if (p[i] != nodata)
            {
                for (int j=0; j<Geometry::Vector<uint8_t,3>::dimension; ++j)
                    res[j] += (float)p[i][j] * 0.25f;
                sumWeights += 0.25f;
            }
        }

        if (sumWeights == 0.0f)
            return nodata;

        Geometry::Vector<uint8_t,3> ret;
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
            ret[i] = (res[i] / sumWeights) + 0.5f;

        return ret;
    }

    static Geometry::Vector<uint8_t,3> minimum(const Geometry::Vector<uint8_t,3>& a, const Geometry::Vector<uint8_t,3>& b,
                              const Geometry::Vector<uint8_t,3>& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        Geometry::Vector<uint8_t,3> res(0,0,0);
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
            res[i] = std::min(a[i], b[i]);
        return res;
    }

    static Geometry::Vector<uint8_t,3> maximum(const Geometry::Vector<uint8_t,3>& a, const Geometry::Vector<uint8_t,3>& b,
                              const Geometry::Vector<uint8_t,3>& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        Geometry::Vector<uint8_t,3> res(0,0,0);
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
            res[i] = std::max(a[i], b[i]);
        return res;
    }
};


} //namespace crusta


#endif //_PixelOps_HPP_
