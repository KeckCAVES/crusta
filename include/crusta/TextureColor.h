#ifndef _TextureColor_H_
#define _TextureColor_H_


#include <sstream>
#include <Geometry/Vector.h>

#include <crusta/basics.h>


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

inline
std::ostream& operator<<(std::ostream& oss, TextureColor& tc)
{
    for (int i=0; i<TextureColor::dimension-1; ++i)
        oss << tc[i] << " ";
    oss << tc[TextureColor::dimension-1];
    return oss;
}

inline
std::istream& operator>>(std::istream& iss, TextureColor& tc)
{
    for (int i=0; i<TextureColor::dimension; ++i)
        iss >> tc[i];
    return iss;
}


END_CRUSTA


#endif //_TextureColor_H_
