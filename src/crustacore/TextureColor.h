#ifndef _TextureColor_H_
#define _TextureColor_H_


#include <sstream>

#include <crustacore/Vector3ui8.h>


BEGIN_CRUSTA


/// serves as a container for the semantics of a 3-channel byte image value
struct TextureColor
{
    /// data type for values of a color texture stored in the globe file
    typedef Geometry::Vector<uint8_t,3> Type;
};


END_CRUSTA


#endif //_TextureColor_H_
