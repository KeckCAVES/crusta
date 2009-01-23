#ifndef _GridClient_H_
#define _GridClient_H_

#include <vector>
#include <basics.h>

BEGIN_CRUSTA

class GlobalGrid;

class GridClient
{
public:
    virtual ~GridClient() {}
    virtual void registerToGrid(GlobalGrid* grid) = 0;
};

typedef std::vector<GridClient*> GridClientPtrs;

END_CRUSTA

#endif //_GridClient_H_
