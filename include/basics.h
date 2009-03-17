#ifndef _basics_H_
#define _basics_H_

#include <stdlib.h>
#include <stdint.h>

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

#define BEGIN_CRUSTA namespace crusta {
#define END_CRUSTA   } //namespace crusta

BEGIN_CRUSTA

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

typedef Geometry::Point<float,3> Point;
typedef Geometry::Vector<float,3> Vector;


const uint TILE_RESOLUTION = 3;

END_CRUSTA
    
#endif //_basics_H_
