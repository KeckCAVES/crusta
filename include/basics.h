#ifndef _basics_H_
#define _basics_H_

#include <stdlib.h>
#include <stdint.h>

#define BEGIN_CRUSTA namespace crusta {
#define END_CRUSTA   } //namespace crusta

BEGIN_CRUSTA

typedef size_t		uint;
    
typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef int8_t		int8;
typedef int16_t		int16;
typedef int32_t		int32;

typedef uint        error;

END_CRUSTA
    
#endif //_basics_H_
