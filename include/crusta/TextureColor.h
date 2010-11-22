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
    /* uint8 is seen as a char not an integer; must, therefore, cast to int
       otherwise '\0' will be generated */
    for (int i=0; i<TextureColor::dimension-1; ++i)
        oss << (int)tc[i] << " ";
    oss << (int)tc[TextureColor::dimension-1];
    return oss;
}

inline
std::istream& operator>>(std::istream& iss, TextureColor& tc)
{
    /* uint8 is seen as a char not an integer; must, therefore, buffer to int
       otherwise 48 (ascii number for '0') will be generated */
    int temp;
    for (int i=0; i<TextureColor::dimension; ++i)
    {
        iss >> temp;
        tc[i] = temp;
    }
    return iss;
}


END_CRUSTA


#endif //_TextureColor_H_
