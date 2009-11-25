#ifndef _basics_H_
#define _basics_H_

#include <stdlib.h>
#include <stdint.h>
#include <vector>

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

#define BEGIN_CRUSTA namespace crusta {
#define END_CRUSTA   } //namespace crusta

BEGIN_CRUSTA

#ifndef CRUSTA_BEBUG_LEVEL
#define CRUSTA_BEBUG_LEVEL 0
#endif //CRUSTA_BEBUG_LEVEL
#define DEBUG_BEGIN(l) if (CRUSTA_BEBUG_LEVEL>=l) {
#define DEBUG_END }
#ifndef CRUSTA_DEBUG_OUTPUT_DESTINATION
#define CRUSTA_DEBUG_OUTPUT_DESTINATION stderr
#endif //CRUSTA_DEBUG_OUTPUT_DESTINATION
#if CRUSTA_BEBUG_LEVEL>0
#define DEBUG_OUT(a, b, args...)\
if ((a)<=CRUSTA_BEBUG_LEVEL) fprintf(CRUSTA_DEBUG_OUTPUT_DESTINATION, b, ## args)
#else
#define DEBUG_OUT(a, b, args...)
#endif

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
    
typedef uint64      FrameNumber;

typedef Geometry::Point<double, 3> Point3;
typedef std::vector<Point3> Point3s;
typedef Geometry::Vector<double, 3> Vector3;
    

const uint   TILE_RESOLUTION     = 65;
const double SPHEROID_RADIUS     = 6371000.0;
const double INV_SPHEROID_RADIUS = 1.0 / 6371000.0;

END_CRUSTA

#endif //_basics_H_
