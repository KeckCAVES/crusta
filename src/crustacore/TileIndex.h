#ifndef _TileIndex_H_
#define _TileIndex_H_


#include <crustacore/basics.h>


namespace crusta {


/** type of an index to a tile within the data pyramid.
    \note the unsigned int type here restricts the trees to ~4 billion
          tiles. Since the indices to children are stored for each tile
          this has been done for storage space preservation purposes */
typedef unsigned int TileIndex;

static const TileIndex INVALID_TILEINDEX = ~0x0U;


} //namespace crusta


#endif //_TileIndex_H_
