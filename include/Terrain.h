#ifndef _Terrain_H_
#define _Terrain_H_

#include <GridClient.h>

#include <GL/GLVertex.h>

#include <Cache.h>
#include <gridProcessing.h>

BEGIN_CRUSTA

class Terrain : public GridClient
{
public:
    typedef GLVertex<float, 2, void, 0, void, float, 3> Vertex;
    
    Terrain();
    ~Terrain();
    
protected:
    static void generateGeometryIndices(uint16* indices);
    static void generateGeometry(const Scope* scope, Vertex* vertices);

    uint16* geometryIndices;
    gridProcessing::Id geometrySlot;
    Cache::PoolId geometryPool;
    RamAllocator* geometryAllocator;

//- part of gridProcessing conventions
public:
    static void updateCallback(void* object,
                               const gridProcessing::ScopeData& scopeData);
    static void preDisplayCallback(void* object);
    static void displayCallback(void* object,
                                const gridProcessing::ScopeData& scopeData);
    static void postDisplayCallback(void* object);

//- inherited from GridClient
public:
    virtual void registerToGrid(GlobalGrid* grid);
};

END_CRUSTA

#endif //_Terrain_H_

