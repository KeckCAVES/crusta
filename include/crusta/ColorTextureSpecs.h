///\todo GPL

#ifndef _ColorTextureSpecs_H_
#define _ColorTextureSpecs_H_

#include <Geometry/Vector.h>

#include <crusta/PixelSpecs.h>
#include <crusta/QuadtreeFile.h>
#include <crusta/QuadtreeFileHeaders.h>

#if CONSTRUO_BUILD
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
        res[i] = (a[i] + b[i]) * 0.5;
    return res;
}
inline TextureColor
pixelAvg(const TextureColor& a, const TextureColor& b, const TextureColor& c,
         const TextureColor& d)
{
    TextureColor res(0,0,0);
    for (int i=0; i<3; ++i)
        res[i] = (a[i] + b[i] + c[i] + d[i]) * 0.25;
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
    double value[3];
    bool isNodata(const TextureColor& test) const
    {
        return double(test[0])==value[0] &&
               double(test[1])==value[1] &&
               double(test[2])==value[2];
    }
};

#if CONSTRUO_BUILD
template<>
class TreeState<TextureColor>
{
public:
    typedef QuadtreeFile< TextureColor, QuadtreeFileHeader<TextureColor>,
                          QuadtreeTileHeader<TextureColor>               > File;

    File* file;
};
#endif //CONSTRUO_BUILD

template <>
struct QuadtreeTileHeader<TextureColor> :
    public QuadtreeTileHeaderBase<TextureColor>
{
    static const TextureColor defaultPixelValue;
};


END_CRUSTA

#endif //_ColorTextureSpecs_H_
