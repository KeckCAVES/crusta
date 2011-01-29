#ifndef _basics_H_
#define _basics_H_

#include <GL/VruiGlew.h> //must be included before gl.h

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

/* lvl
 40 -  49: debug coverage: 40 manip CP, 41-44 manip cov, 49 validate cov
 90 -  99: debug ray intersection
*/

#ifndef CRUSTA_DEBUG_LEVEL_MIN_VALUE
#define CRUSTA_DEBUG_LEVEL_MIN_VALUE 100000
#endif //CRUSTA_DEBUG_LEVEL_MIN_VALUE

#ifndef CRUSTA_DEBUG_LEVEL_MAX_VALUE
#define CRUSTA_DEBUG_LEVEL_MAX_VALUE 100000
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

///\todo turn this into a simple class with comparators and a stamp() method
typedef double                  FrameStamp;
typedef std::vector<FrameStamp> FrameStamps;

///\todo deprecate Scalar
typedef double Scalar;

typedef Geometry::Point<int, 2>     Point2i;
typedef Geometry::Point<double, 2>  Point2;
typedef Geometry::Point<double, 3>  Point3;
typedef Geometry::Point<float, 3>   Point3f;
typedef std::vector<Point3>         Point3s;
typedef std::vector<Point3f>        Point3fs;
typedef Geometry::Vector<float, 2>  Vector2f;
typedef Geometry::Vector<double, 2> Vector2;
typedef Geometry::Vector<double, 3> Vector3;
typedef Geometry::Vector<float, 3>  Vector3f;
typedef std::vector<Vector2f>       Vector2fs;
typedef std::vector<Vector3>        Vector3s;
typedef std::vector<Vector3f>       Vector3fs;

typedef GLColor<float, 4>           Color;
typedef std::vector<Color>          Colors;

typedef Geometry::HitResult<double> HitResult;
typedef Geometry::Ray<double, 3>    Ray;


static const int    TILE_RESOLUTION          = 65;
static const float  TILE_TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;

extern bool PROJECTION_FAILED;

/**\todo this framestamp is currently used as a hack around doing synchronized
post display processing, by handling it at the begining of a new frame and
explicitely setting the current frame stamp afterward */
extern FrameStamp CURRENT_FRAME;


END_CRUSTA


#endif //_basics_H_
