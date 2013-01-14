#ifndef _glbasics_H_
#define _glbasics_H_

#include <crustacore/basics.h>
#include <crusta/GL/VruiGlew.h> //must be included before gl.h
#include <GL/GLColor.h>

BEGIN_CRUSTA

typedef GLColor<float, 4>           Color;
typedef std::vector<Color>          Colors;

END_CRUSTA

#endif //_glbasics_H_
