///\todo GPL

#ifndef _ColorTextureSpecs_H_
#define _ColorTextureSpecs_H_

#include <Geometry/Vector.h>
#include <Math/Constants.h>

#include <crusta/PixelSpecs.h>
#include <crusta/QuadtreeFile.h>
#include <crusta/QuadtreeFileHeaders.h>

#if CONSTRUO_BUILD
#include <construo/DynamicFilter.h>
#include <construo/Filter.h>
#include <construo/Tree.h>
#endif //CONSTRUO_BUILD

BEGIN_CRUSTA

/// data type for values of a color texture stored in the preproccessed database
typedef Geometry::Vector<uint8, 3> TextureColor;

inline TextureColor
pixelAvg(const TextureColor& a, const TextureColor& b)
{
    TextureColor res(0,0,0);
    for (int i=0; i<3; ++i)
        res[i] = ((float)a[i] + (float)b[i]) * 0.5;
    return res;
}
inline TextureColor
pixelAvg(const TextureColor& a, const TextureColor& b, const TextureColor& c,
         const TextureColor& d)
{
    TextureColor res(0,0,0);
    for (int i=0; i<3; ++i)
        res[i] = ((float)a[i] + (float)b[i] + (float)c[i] + (float)d[i]) * 0.25;
    return res;
}

inline TextureColor
pixelMin(const TextureColor& one, const TextureColor& two)
{
    TextureColor res(0,0,0);
    for (int i=0; i<3; ++i)
        res[i] = one[i]<two[i] ? one[i] : two[i];
    return res;
}
inline TextureColor
pixelMax(const TextureColor& one, const TextureColor& two)
{
    TextureColor res(0,0,0);
    for (int i=0; i<3; ++i)
        res[i] = one[i]>two[i] ? one[i] : two[i];
    return res;
}


inline
TextureColor operator*(const TextureColor& tc, double d) {
    return TextureColor(tc[0]*d, tc[1]*d, tc[2]*d);
}
inline
TextureColor operator*(double d, const TextureColor& tc) {
    return tc * d;
}


template <>
struct Nodata<TextureColor>
{
    Nodata()
    {
        value[0] = value[1] = value[2] = Math::Constants<double>::max;
    }

    double value[3];
    bool isNodata(const TextureColor& test) const
    {
        return double(test[0])==value[0] &&
               double(test[1])==value[1] &&
               double(test[2])==value[2];
    }
};

#if CONSTRUO_BUILD
namespace filter {

template <>
inline TextureColor
nearestLookup(const TextureColor* img, const int origin[2],
              const Scalar at[2], const int size[2],
              const Nodata<TextureColor>& nodata,
              const TextureColor& defaultValue)
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

template <>
inline TextureColor
linearLookup(const TextureColor* img, const int origin[2],
              const Scalar at[2], const int size[2],
              const Nodata<TextureColor>& nd,
              const TextureColor& dval)
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
    Geometry::Vector<Scalar, 3> res;
    TextureColor iv[4];
    iv[0] = nd.isNodata(base[0])         ? dval : base[0];
    iv[1] = nd.isNodata(base[1])         ? dval : base[1];
    iv[2] = nd.isNodata(base[size[0]])   ? dval : base[size[0]];
    iv[3] = nd.isNodata(base[size[0]+1]) ? dval : base[size[0]+1];
    for (int i=0; i<3; ++i)
    {
        Scalar one = (1.0-d[0])*iv[0][i] + d[0]*iv[1][i];
        Scalar two = (1.0-d[0])*iv[2][i] + d[0]*iv[3][i];

        res[i] = ((1.0-d[1])*one  + d[1]*two);
    }

    //snap to nearest integer and clamp
    TextureColor returnValue;
    for (int i=0; i<3; ++i)
    {
        res[i] = res[i]<Scalar(0)   ? Scalar(0)   : res[i];
        res[i] = res[i]>Scalar(255) ? Scalar(255) : res[i];

        returnValue[i] = TextureColor::Scalar(res[i]+Scalar(0.5));
    }

    return returnValue;
}

} //end namespace filter

template <>
inline
TextureColor Filter::
lookup(TextureColor* at, uint rowLen)
{
    Geometry::Vector<Scalar, 3> res(0);

    for (int y=-width; y<=static_cast<int>(width); ++y)
    {
        TextureColor* atY = at + y*rowLen;
        for (int x=-width; x<=static_cast<int>(width); ++x)
        {
            TextureColor* atYX = atY + x;
            for (int i=0; i<3; ++i)
                res[i] += Scalar((*atYX)[i]) * weights[y]*weights[x];
        }
    }

    //snap to nearest integer and clamp
    TextureColor returnValue;
    for (int i=0; i<3; ++i)
    {
        res[i] = res[i]<Scalar(0)   ? Scalar(0)   : res[i];
        res[i] = res[i]>Scalar(255) ? Scalar(255) : res[i];

        returnValue[i] = TextureColor::Scalar(res[i]+Scalar(0.5));
    }

    return returnValue;
}

template<>
class TreeState<TextureColor>
{
public:
    typedef QuadtreeFile< TextureColor, QuadtreeFileHeader<TextureColor>,
                          QuadtreeTileHeader<TextureColor>               > File;

    File* file;
};

template <>
const char*
globeDataType<TextureColor>()
{
    return "ImageRGB";
}
#endif //CONSTRUO_BUILD

template <>
struct QuadtreeTileHeader<TextureColor> :
    public QuadtreeTileHeaderBase<TextureColor>
{
    static const TextureColor defaultPixelValue;
};


END_CRUSTA

#endif //_ColorTextureSpecs_H_
