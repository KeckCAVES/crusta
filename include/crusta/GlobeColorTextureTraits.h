#ifndef _GlobeColorTextureTraits_H_
#define _GlobeColorTextureTraits_H_


#include <crusta/GlobeDataTraits.h>

#include <Geometry/Vector.h>
#include <Math/Constants.h>

#if CONSTRUO_BUILD
#include <construo/Tree.h>
#endif //CONSTRUO_BUILD


BEGIN_CRUSTA


///data type for values of a color texture stored in the preproccessed database
typedef Geometry::Vector<uint8, 3> TextureColor;


inline
TextureColor operator*(const TextureColor& tc, double d)
{
    return TextureColor(tc[0]*d, tc[1]*d, tc[2]*d);
}
inline
TextureColor operator*(double d, const TextureColor& tc)
{
    return tc * d;
}


template <>
struct GlobeDataTraits<TextureColor>
{
    typedef double Scalar;

/*- fixed filtered lookups into two dimensional pixel domains. The lookups must
be aligned with the centers of the pixels in the domain. They are used for
subsampling. */
    enum SubsampleFilter
    {
        SUBSAMPLE_POINT,
        SUBSAMPLE_PYRAMID,
        SUBSAMPLE_LANCZOS5
    };

    struct Nodata
    {
        Nodata()
        {
            for (int i=0; i<TextureColor::dimension; ++i)
                value[i] = Constants<double>::max;
        }

        bool isNodata(const TextureColor& test) const
        {
            bool result = true;
            for (int i=0; i<TextureColor::dimension; ++i)
                result = result && double(test[i])==value[i];
        }

        static Nodata parse(const std::string& nodataString)
        {
            Nodata nodata;
            std::istringstream iss(nodataString);
            for (int i=0; i<TextureColor::dimension; ++i)
                iss >> nodata.value[i];
            return nodata;
        }

        double value[TextureColor::dimension];
    };


    static TextureColor average(const TextureColor& a, const TextureColor& b)
    {
        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
            res[i] = ((float)a[i] + (float)b[i]) * 0.5;
        return res;
    }
    static TextureColor average(const TextureColor& a, const TextureColor& b,
                                const TextureColor& c, const TextureColor& d)
    {
        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
        {
            float color = (float)a[i] + (float)b[i] + (float)c[i] + (float)d[i];
            color       = color*0.25f + 0.5f;
            res[i]      = color;
        }
        return res;
    }
    static TextureColor minimum(const TextureColor& a, const TextureColor& b)
    {
        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
            res[i] = std::min(a[i], b[i]);
        return res;
    }
    static TextureColor maximum(const TextureColor& a, const TextureColor& b)
    {
        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
            res[i] = std::max(a[i], b[i]);
        return res;
    }


    static TextureColor nearestLookup(
        const TextureColor* img, const int origin[2],
        const Scalar at[2], const int size[2],
        const Nodata& nodata, const TextureColor& defaultValue)
    {
        int ip[2];
        for (uint i=0; i<2; ++i)
        {
            Scalar pFloor = Math::floor(at[i] + Scalar(0.5));
            ip[i]         = static_cast<int>(pFloor) - origin[i];
        }
        TextureColor imgValue = img[ip[1]*size[0] + ip[0]];
        return nodata.isNodata(imgValue) ? defaultValue : imgValue;
    }

    static TextureColor linearLookup(
        const TextureColor* img, const int origin[2],
        const Scalar at[2], const int size[2],
        const Nodata& nodata, const TextureColor& dValue)
    {
        int ip[2];
        Scalar d[2];
        for(uint i=0; i<2; ++i)
        {
            Scalar pFloor = Math::floor(at[i]);
            d[i]          = at[i] - pFloor;
            ip[i]         = static_cast<int>(pFloor) - origin[i];
            if (ip[i] == size[i]-1)
            {
                --ip[i];
                d[i] = 1.0;
            }
        }
        const TextureColor* base = img + (ip[1]*size[0] + ip[0]);
        Geometry::Vector<Scalar, TextureColor::dimesion> res;
        TextureColor iv[4];
        iv[0] = nodata.isNodata(base[0])         ? dValue : base[0];
        iv[1] = nodata.isNodata(base[1])         ? dValue : base[1];
        iv[2] = nodata.isNodata(base[size[0]])   ? dValue : base[size[0]];
        iv[3] = nodata.isNodata(base[size[0]+1]) ? dValue : base[size[0]+1];
        for (int i=0; i<TextureColor::dimesion; ++i)
        {
            Scalar one = (1.0-d[0])*iv[0][i] + d[0]*iv[1][i];
            Scalar two = (1.0-d[0])*iv[2][i] + d[0]*iv[3][i];

            res[i] = ((1.0-d[1])*one  + d[1]*two);
        }

        //snap to nearest integer and clamp
        TextureColor returnValue;
        for (int i=0; i<TextureColor::dimesion; ++i)
        {
            res[i] = res[i]<Scalar(0)   ? Scalar(0)   : res[i];
            res[i] = res[i]>Scalar(255) ? Scalar(255) : res[i];

            returnValue[i] = TextureColor::Scalar(res[i]+Scalar(0.5));
        }

        return returnValue;
    }


    template <SubsampleFilter filter>
    static int subsampleWidth();

    template <>
    static int subsampleWidth<SUBSAMPLE_POINT>()
    {
        return 0;
    }
    template <>
    static int subsampleWidth<SUBSAMPLE_PYRAMID>()
    {
        return 1;
    }
    template <>
    static int subsampleWidth<SUBSAMPLE_LANCZOS5>()
    {
        return 10;
    }

    template <SubsampleFilter filter>
    static void subsample(PixelParam* at, int rowLen);

    template <>
    static TextureColor subsample<SUBSAMPLE_POINT>(PixelParam* at, int rowLen)
    {
        return *at;
    }
    template <>
    static TextureColor subsample<SUBSAMPLE_PYRAMID>(PixelParam* at, int rowLen)
    {
        static double weightStorage[3] = {0.25, 0.50, 0.25};
        double* weights = &weightStorage[1];

        Geometry::Vector<Scalar, TextureColor::dimesion> res(0);

        for (int y=-1; y<=1; ++y)
        {
            TextureColor* atY = at + y*rowLen;
            for (int x=-1; x<=1; ++x)
            {
                TextureColor* atYX = atY + x;
                for (int i=0; i<TextureColor::dimension; ++i)
                    res[i] += Scalar((*atYX)[i]) * weights[y]*weights[x];
            }
        }

        //snap to nearest integer and clamp
        TextureColor returnValue;
        for (int i=0; i<TextureColor::dimesion; ++i)
        {
            res[i] = res[i]<Scalar(0)   ? Scalar(0)   : res[i];
            res[i] = res[i]>Scalar(255) ? Scalar(255) : res[i];

            returnValue[i] = TextureColor::Scalar(res[i]+Scalar(0.5));
        }

        return returnValue;
    }
    template <>
    static TextureColor subsample<SUBSAMPLE_LANCZOS5>(PixelParam* at,
                                                      int rowLen)
    {
        static const double weightStorage[21] = {
             7.60213661720011e-34,  0.00386785330198227,  -4.5610817871754e-18,
              -0.0167391813072476, 9.83998047615722e-18,    0.0405539013275657,
            -1.47599707142358e-17,  -0.0911355426727928,  1.82443271487016e-17,
                0.313296117460564,    0.500313703779858,     0.313296117460564,
             1.82443271487016e-17,  -0.0911355426727928, -1.47599707142358e-17,
               0.0405539013275657, 9.83998047615722e-18,   -0.0167391813072476,
             -4.5610817871754e-18,  0.00386785330198227,  7.60213661720011e-34};
        double* weights = &weightStorage[10];

        Geometry::Vector<Scalar, TextureColor::dimesion> res(0);

        for (int y=-10; y<=10; ++y)
        {
            TextureColor* atY = at + y*rowLen;
            for (int x=-10; x<=10; ++x)
            {
                TextureColor* atYX = atY + x;
                for (int i=0; i<TextureColor::dimension; ++i)
                    res[i] += Scalar((*atYX)[i]) * weights[y]*weights[x];
            }
        }

        //snap to nearest integer and clamp
        TextureColor returnValue;
        for (int i=0; i<TextureColor::dimesion; ++i)
        {
            res[i] = res[i]<Scalar(0)   ? Scalar(0)   : res[i];
            res[i] = res[i]>Scalar(255) ? Scalar(255) : res[i];

            returnValue[i] = TextureColor::Scalar(res[i]+Scalar(0.5));
        }

        return returnValue;
    }


    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<PixelParam, FileHeader, TileHeader> File;

    struct FileHeader
    {
    public:
        void read(Misc::LargeFile*)        {}
        static size_t getSize()            {return 0;}

#if CONSTRUO_BUILD
        void write(Misc::LargeFile*) const {}
#endif //CONSTRUO_BUILD
    };

    struct TileHeader
    {
        void read(Misc::LargeFile*)       {}
        static size_t getSize()           {return 0;}

#if CONSTRUO_BUILD
        void write(Misc::LargeFile) const {}
#endif //CONSTRUO_BUILD
    };

    static const char* typeName()
    {
        return "ImageRGB";
    }

    static const int numChannels()
    {
        return 3;
    }
};


END_CRUSTA


#endif //_GlobeColorTextureTraits_H_
