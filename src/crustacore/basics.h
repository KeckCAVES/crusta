#ifndef _basics_H_
#define _basics_H_

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <vector>

#include <Geometry/HitResult.h>
#include <Geometry/Ray.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>


#define BEGIN_CRUSTA namespace crusta {
#define END_CRUSTA   } //namespace crusta

BEGIN_CRUSTA

#ifndef CRUSTA_ENABLE_DEBUG
#define CRUSTA_ENABLE_DEBUG !NDEBUG
#endif //CRUSTA_ENABLE_DEBUG

#if CRUSTA_ENABLE_DEBUG

/* lvl
 10 - 20: cache
    10 print datamanager frame delimiter
    11 full: new request not possible
    12 full: could not grab new buffer
    15 grabbed buffer
    17 print lru content
    18 print cache content
    19 cache miss on find
    20 cache hit on find

 40 - 49: coverage
    40    manipulate control points
    41-44 manipulate coverage
    49    validate coverage

 90 -  99: ray intersection
*/

#ifndef CRUSTA_DEBUG_LEVEL_MIN_VALUE
#define CRUSTA_DEBUG_LEVEL_MIN_VALUE 10000
#endif //CRUSTA_DEBUG_LEVEL_MIN_VALUE

#ifndef CRUSTA_DEBUG_LEVEL_MAX_VALUE
#define CRUSTA_DEBUG_LEVEL_MAX_VALUE 10000
#endif //CRUSTA_DEBUG_LEVEL_MAX_VALUE

extern int CRUSTA_DEBUG_LEVEL_MIN;
extern int CRUSTA_DEBUG_LEVEL_MAX;

#define CRUSTA_DEBUG(l,x) if (l>=CRUSTA_DEBUG_LEVEL_MIN &&\
                              l<=CRUSTA_DEBUG_LEVEL_MAX){x}
#define CRUSTA_DEBUG_OUT std::cerr
#else

#define CRUSTA_DEBUG(l,x)

#endif //CRUSTA_ENABLE_DEBUG

#define DEBUG_INTERSECT_CRAP 0
#if DEBUG_INTERSECT_CRAP
extern bool DEBUG_INTERSECT;
#endif //DEBUG_INTERSECT_CRAP

///\todo turn this into a simple class with comparators and a stamp() method
typedef double                  FrameStamp;

///\todo deprecate Scalar
typedef double Scalar;

static const int    TILE_RESOLUTION          = 65;
static const float  TILE_TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;

extern bool PROJECTION_FAILED;

extern FrameStamp CURRENT_FRAME;
extern FrameStamp LAST_FRAME;


END_CRUSTA


#endif //_basics_H_
