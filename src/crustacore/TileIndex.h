#ifndef _TileIndex_H_
#define _TileIndex_H_


#include <crustacore/basics.h>


BEGIN_CRUSTA


/** type of an index to a tile within the data pyramid.
    \note the unsigned int type here restricts the trees to ~4 billion
          tiles. Since the indices to children are stored for each tile
          this has been done for storage space preservation purposes */
typedef unsigned int TileIndex;

static const TileIndex INVALID_TILEINDEX = ~0x0U;


END_CRUSTA


#endif //_TileIndex_H_
