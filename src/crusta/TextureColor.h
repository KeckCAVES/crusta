#ifndef _TextureColor_H_
#define _TextureColor_H_


#include <sstream>

#include <crusta/Vector3ui8.h>


BEGIN_CRUSTA


/// serves as a container for the semantics of a 3-channel byte image value
struct TextureColor
{
    /// data type for values of a color texture stored in the globe file
    typedef Vector3ui8 Type;
};


END_CRUSTA


#endif //_TextureColor_H_
