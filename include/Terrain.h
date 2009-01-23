#ifndef _Terrain_H_
#define _Terrain_H_

#include <GridClient.h>
#include <gridProcessing.h>

BEGIN_CRUSTA

class Terrain : public GridClient
{
public:
    static void displayCallback(void* object,
                                const gridProcessing::ScopeData& scopeData);

//- inherited from GridClient
public:
    virtual void registerToGrid(GlobalGrid* grid);
};

END_CRUSTA

#endif //_Terrain_H_

