#ifndef _glbasics_H_
#define _glbasics_H_

#include <crustacore/basics.h>
#include <crustavrui/GL/VruiGlew.h> //must be included before gl.h

#include <crusta/vrui.h>

namespace crusta {

typedef GLColor<float, 4>           Color;
typedef std::vector<Color>          Colors;

} //namespace crusta

#endif //_glbasics_H_
