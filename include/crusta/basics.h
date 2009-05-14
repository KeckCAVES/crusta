#ifndef _basics_H_
#define _basics_H_

#include <stdlib.h>
#include <stdint.h>

#define BEGIN_CRUSTA namespace crusta {
#define END_CRUSTA   } //namespace crusta

BEGIN_CRUSTA

#ifndef CRUSTA_BEBUG_LEVEL
#define CRUSTA_BEBUG_LEVEL 0
#endif //CRUSTA_BEBUG_LEVEL
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


const uint TILE_RESOLUTION  = 33;
//const uint TILE_SUBDIVISION = 5;


/** keep track of the number of frames processed. Used, for example, by the
    cache to perform LRU that is aware of currently active nodes (the ones from
    the previous frame) */
extern uint crustaFrameNumber;

END_CRUSTA
    
#endif //_basics_H_
