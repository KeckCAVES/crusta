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
#include <GL/GLColor.h>


#define BEGIN_CRUSTA namespace crusta {
#define END_CRUSTA   } //namespace crusta

BEGIN_CRUSTA

#ifndef CRUSTA_ENABLE_DEBUG
#define CRUSTA_ENABLE_DEBUG !NDEBUG
#endif //CRUSTA_ENABLE_DEBUG

#if CRUSTA_ENABLE_DEBUG

extern int CRUSTA_DEBUG_LEVEL_MIN;
extern int CRUSTA_DEBUG_LEVEL_MAX;

#define CRUSTA_DEBUG_ONLY(x) {x;}
#define CRUSTA_DEBUG_ONLY_SINGLE(x) x
#define CRUSTA_DEBUG(l,x) if (l>=CRUSTA_DEBUG_LEVEL_MIN &&\
                              l<=CRUSTA_DEBUG_LEVEL_MAX){x;}
#ifndef CRUSTA_DEBUG_OUTPUT_DESTINATION
#define CRUSTA_DEBUG_OUTPUT_DESTINATION stderr
#endif //CRUSTA_DEBUG_OUTPUT_DESTINATION
#define CRUSTA_DEBUG_OUT(l, b, args...)\
if (l>=CRUSTA_DEBUG_LEVEL_MIN && l<=CRUSTA_DEBUG_LEVEL_MAX) {\
    fprintf(CRUSTA_DEBUG_OUTPUT_DESTINATION, b, ## args); }
#else

#define CRUSTA_DEBUG_ONLY(x)
#define CRUSTA_DEBUG_ONLY_SINGLE(x)
#define CRUSTA_DEBUG(l,x)
#define CRUSTA_DEBUG_OUT(a, b, args...)

#endif //CRUSTA_ENABLE_DEBUG

#define DEBUG_INTERSECT_CRAP 0
#if DEBUG_INTERSECT_CRAP
extern bool DEBUG_INTERSECT;
#endif //DEBUG_INTERSECT_CRAP

typedef size_t		uint;

typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t    uint64;
typedef int8_t		int8;
typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t     int64;

typedef uint        error;

typedef double                  FrameStamp;
typedef std::vector<FrameStamp> FrameStamps;

typedef double      Scalar;

typedef Geometry::Point<Scalar, 3>  Point3;
typedef Geometry::Point<float, 3>   Point3f;
typedef std::vector<Point3>         Point3s;
typedef std::vector<Point3f>        Point3fs;
typedef Geometry::Vector<float, 2>  Vector2f;
typedef Geometry::Vector<Scalar, 3> Vector3;
typedef Geometry::Vector<float, 3>  Vector3f;
typedef std::vector<Vector2f>       Vector2fs;
typedef std::vector<Vector3>        Vector3s;
typedef std::vector<Vector3f>       Vector3fs;

typedef GLColor<float, 4>           Color;
typedef std::vector<Color>          Colors;

typedef Geometry::HitResult<Scalar> HitResult;
typedef Geometry::Ray<Scalar, 3>    Ray;


static const uint   TILE_RESOLUTION          = 65;
static const float  TILE_TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;


/**\todo this framestamp is currently used as a hack around doing synchronized
post display processing, by handling it at the begining of a new frame and
explicitely setting the current frame stamp afterward */
extern FrameStamp CURRENT_FRAME;


END_CRUSTA


#endif //_basics_H_
