#ifndef _SubsamplingFilter_HPP_
#define _SubsamplingFilter_HPP_


#include <construo/SubsampleFilter.h>

#include <Math/Math.h>

#include <crustacore/Vector3ui8.h>


BEGIN_CRUSTA


//- single channel float -------------------------------------------------------

template <>
struct SubsampleFilter<float, SUBSAMPLEFILTER_POINT>
{
    static int width()
    {
        return 0;
    }

    static float sample(const float* img, const int origin[2],
                        const double at[2], const int size[2],
                        const float& imgNodata, const float& defaultValue,
                        const float&)
    {
        int ip[2];
        for (size_t i=0; i<2; ++i)
        {
            double pFloor = Math::floor(at[i] + double(0.5));
            ip[i]         = static_cast<int>(pFloor) - origin[i];
        }
        float imgValue = img[ip[1]*size[0] + ip[0]];
        return imgValue==imgNodata ? defaultValue : imgValue;
    }

    static float sample(float* at, int rowLen, const float&)
    {
        return *at;
    }
};

template <>
struct SubsampleFilter<float, SUBSAMPLEFILTER_PYRAMID>
{
    static int width()
    {
        return 1;
    }

    static float sample(const float* img, const int origin[2],
                        const double at[2], const int size[2],
                        const float& imgNodata, const float& defaultValue,
                        const float& globeNodata)
    {
        int ip[2];
        double d[2];
        for(size_t i=0; i<2; ++i)
        {
            double pFloor = Math::floor(at[i]);
            d[i]          = at[i] - pFloor;
            ip[i]         = static_cast<int>(pFloor) - origin[i];
            if (ip[i] == size[i]-1)
            {
                --ip[i];
                d[i] = 1.0;
            }
        }
        const float* base = img + (ip[1]*size[0] + ip[0]);
        float v[4];
        v[0] = base[0]         == imgNodata ? defaultValue : base[0];
        v[1] = base[1]         == imgNodata ? defaultValue : base[1];
        v[2] = base[size[0]]   == imgNodata ? defaultValue : base[size[0]];
        v[3] = base[size[0]+1] == imgNodata ? defaultValue : base[size[0]+1];

        double weights[4] = { (1-d[1])*(1-d[0]), (1-d[1])*d[0],
                              d[1]*(1-d[0]), d[1]*d[0] };

        double sum        = 0.0;
        double sumWeights = 0.0;
        for (int i=0; i<4; ++i)
        {
            if (v[i] != globeNodata)
            {
                sum        += v[i] * weights[i];
                sumWeights += weights[i];
            }
        }

        if (sumWeights == 0.0)
            return globeNodata;

        return float(sum / sumWeights);
    }

    static float sample(float* at, int rowLen,
                        const float& globeNodata)
    {
        static double weightStorage[3] = {0.25, 0.50, 0.25};
        double* weights = &weightStorage[1];

        double sum        = 0.0;
        double sumWeights = 0.0;

        for (int y=-1; y<=1; ++y)
        {
            float* atY = at + y*rowLen;
            for (int x=-1; x<=1; ++x)
            {
                float* atYX = atY + x;
                if (*atYX != globeNodata)
                {
                    double weight = weights[y]*weights[x];
                    sum          += double(*atYX) * weight;
                    sumWeights   += weight;
                }
            }
        }

        if (sumWeights == 0.0)
            return globeNodata;

        return float(sum / sumWeights);
    }
};


template <>
struct SubsampleFilter<float, SUBSAMPLEFILTER_LANCZOS5>
{
    static int width()
    {
        return 10;
    }

    //no dynamic sample for lanczos5

    static float sample(float* at, int rowLen,
                        const float& globeNodata)
    {
        static const double weightStorage[21] = {
             7.60213661720011e-34,  0.00386785330198227,  -4.5610817871754e-18,
              -0.0167391813072476, 9.83998047615722e-18,    0.0405539013275657,
            -1.47599707142358e-17,  -0.0911355426727928,  1.82443271487016e-17,
                0.313296117460564,    0.500313703779858,     0.313296117460564,
             1.82443271487016e-17,  -0.0911355426727928, -1.47599707142358e-17,
               0.0405539013275657, 9.83998047615722e-18,   -0.0167391813072476,
             -4.5610817871754e-18,  0.00386785330198227,  7.60213661720011e-34};
        const double* weights = &weightStorage[10];

        double sum        = 0.0;
        double sumWeights = 0.0;

        for (int y=-10; y<=10; ++y)
        {
            float* atY = at + y*rowLen;
            for (int x=-10; x<=10; ++x)
            {
                float* atYX = atY + x;
                if (*atYX != globeNodata)
                {
                    double weight = weights[y]*weights[x];
                    sum          += double(*atYX) * weight;
                    sumWeights   += weight;
                }
            }
        }

        if (sumWeights == 0.0)
            return globeNodata;

        return float(sum / sumWeights);
    }
};


//- 3-channel unit8 ------------------------------------------------------------

template <>
struct SubsampleFilter<Geometry::Vector<uint8_t,3>, SUBSAMPLEFILTER_POINT>
{
    static int width()
    {
        return 0;
    }

    static Geometry::Vector<uint8_t,3> sample(const Geometry::Vector<uint8_t,3>* img, const int origin[2],
                             const double at[2], const int size[2],
                             const Geometry::Vector<uint8_t,3>& imgNodata,
                             const Geometry::Vector<uint8_t,3>& defaultValue, const Geometry::Vector<uint8_t,3>&)
    {
        int ip[2];
        for (size_t i=0; i<2; ++i)
        {
            double pFloor = Math::floor(at[i] + double(0.5));
            ip[i]         = static_cast<int>(pFloor) - origin[i];
        }
        Geometry::Vector<uint8_t,3> imgValue = img[ip[1]*size[0] + ip[0]];
        return imgValue==imgNodata ? defaultValue : imgValue;
    }

    static Geometry::Vector<uint8_t,3> sample(Geometry::Vector<uint8_t,3>* at, int rowLen,
                             const Geometry::Vector<uint8_t,3>&)
    {
        return *at;
    }
};

template <>
struct SubsampleFilter<Geometry::Vector<uint8_t,3>, SUBSAMPLEFILTER_PYRAMID>
{
    static int width()
    {
        return 1;
    }

    static Geometry::Vector<uint8_t,3> sample(const Geometry::Vector<uint8_t,3>* img, const int origin[2],
                             const double at[2], const int size[2],
                             const Geometry::Vector<uint8_t,3>& imgNodata,
                             const Geometry::Vector<uint8_t,3>& defaultValue,
                             const Geometry::Vector<uint8_t,3>& globeNodata)
    {
        int ip[2];
        double d[2];
        for(size_t i=0; i<2; ++i)
        {
            double pFloor = Math::floor(at[i]);
            d[i]          = at[i] - pFloor;
            ip[i]         = static_cast<int>(pFloor) - origin[i];
            if (ip[i] == size[i]-1)
            {
                --ip[i];
                d[i] = 1.0;
            }
        }
        const Geometry::Vector<uint8_t,3>* base = img + (ip[1]*size[0] + ip[0]);
        Geometry::Vector<uint8_t,3> v[4];
        v[0] = base[0]         == imgNodata ? defaultValue : base[0];
        v[1] = base[1]         == imgNodata ? defaultValue : base[1];
        v[2] = base[size[0]]   == imgNodata ? defaultValue : base[size[0]];
        v[3] = base[size[0]+1] == imgNodata ? defaultValue : base[size[0]+1];

        double weights[4] = { (1-d[1])*(1-d[0]), (1-d[1])*d[0],
                              d[1]*(1-d[0]), d[1]*d[0] };

        Geometry::Vector<double, Geometry::Vector<uint8_t,3>::dimension> sum(0);
        double sumWeights(0.0);
        for (int i=0; i<4; ++i)
        {
            if (v[i] != globeNodata)
            {
                for (int j=0; j<Geometry::Vector<uint8_t,3>::dimension; ++j)
                    sum[j] += v[i][j] * weights[i];
                sumWeights += weights[i];
            }
        }

        if (sumWeights == 0.0)
            return globeNodata;

        //snap to nearest integer and clamp
        Geometry::Vector<uint8_t,3> returnValue;
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
        {
            sum[i] /= sumWeights;
            sum[i]  = std::max(sum[i], 0.0);
            sum[i]  = std::min(sum[i], 255.0);

            returnValue[i] = Geometry::Vector<uint8_t,3>::Scalar(sum[i]+0.5);
        }

        return returnValue;
    }

    static Geometry::Vector<uint8_t,3> sample(Geometry::Vector<uint8_t,3>* at, int rowLen,
                             const Geometry::Vector<uint8_t,3>& globeNodata)
    {
        static double weightStorage[3] = {0.25, 0.50, 0.25};
        double* weights = &weightStorage[1];

        Geometry::Vector<double, Geometry::Vector<uint8_t,3>::dimension> sum(0);
        double sumWeights = 0.0;

        for (int y=-1; y<=1; ++y)
        {
            Geometry::Vector<uint8_t,3>* atY = at + y*rowLen;
            for (int x=-1; x<=1; ++x)
            {
                Geometry::Vector<uint8_t,3>* atYX = atY + x;
                if (*atYX != globeNodata)
                {
                    double weight = weights[y]*weights[x];
                    for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
                        sum[i] += double((*atYX)[i]) * weight;
                    sumWeights += weight;
                }
            }
        }

        if (sumWeights == 0.0)
            return globeNodata;

        //snap to nearest integer and clamp
        Geometry::Vector<uint8_t,3> returnValue;
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
        {
            sum[i] /= sumWeights;
            sum[i]  = std::max(sum[i], 0.0);
            sum[i]  = std::min(sum[i], 255.0);

            returnValue[i] = Geometry::Vector<uint8_t,3>::Scalar(sum[i]+0.5);
        }

        return returnValue;
    }
};

template <>
struct SubsampleFilter<Geometry::Vector<uint8_t,3>, SUBSAMPLEFILTER_LANCZOS5>
{
    static int width()
    {
        return 10;
    }

    //no dynamic sampling

    static Geometry::Vector<uint8_t,3> sample(Geometry::Vector<uint8_t,3>* at, int rowLen,
                             const Geometry::Vector<uint8_t,3>& globeNodata)
    {
        static const double weightStorage[21] = {
             7.60213661720011e-34,  0.00386785330198227,  -4.5610817871754e-18,
              -0.0167391813072476, 9.83998047615722e-18,    0.0405539013275657,
            -1.47599707142358e-17,  -0.0911355426727928,  1.82443271487016e-17,
                0.313296117460564,    0.500313703779858,     0.313296117460564,
             1.82443271487016e-17,  -0.0911355426727928, -1.47599707142358e-17,
               0.0405539013275657, 9.83998047615722e-18,   -0.0167391813072476,
             -4.5610817871754e-18,  0.00386785330198227,  7.60213661720011e-34};
        const double* weights = &weightStorage[10];

        Geometry::Vector<double, Geometry::Vector<uint8_t,3>::dimension> sum(0);
        double sumWeights = 0.0;

        for (int y=-10; y<=10; ++y)
        {
            Geometry::Vector<uint8_t,3>* atY = at + y*rowLen;
            for (int x=-10; x<=10; ++x)
            {
                Geometry::Vector<uint8_t,3>* atYX = atY + x;
                if (*atYX != globeNodata)
                {
                    double weight = weights[y]*weights[x];
                    for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
                        sum[i] += double((*atYX)[i]) * weight;
                    sumWeights += weight;
                }
            }
        }

        if (sumWeights == 0.0)
            return globeNodata;

        //snap to nearest integer and clamp
        Geometry::Vector<uint8_t,3> returnValue;
        for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
        {
            sum[i] /= sumWeights;
            sum[i]  = std::max(sum[i], 0.0);
            sum[i]  = std::min(sum[i], 255.0);

            returnValue[i] = Geometry::Vector<uint8_t,3>::Scalar(sum[i]+0.5);
        }

        return returnValue;
    }
};


END_CRUSTA


#endif //_SubsamplingFilter_HPP_
