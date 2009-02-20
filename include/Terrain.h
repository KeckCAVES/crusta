#ifndef _Terrain_H_
#define _Terrain_H_

#include <GridClient.h>

#include <GL/GLObject.h>
#include <GL/GLShader.h>
#include <GL/GLVertex.h>

#include <Cache.h>
#include <gridProcessing.h>

BEGIN_CRUSTA

class TextureAllocator;

class Terrain : public GridClient, public GLObject
{
public:
    typedef GLVertex<float, 2, void, 0, void, float, 3> Vertex;
    
    Terrain();
    ~Terrain();
    
protected:
    struct GlData : public GLObject::DataItem
    {
        GlData();
        GLShader elevationShader;        
    };
    
    static void generateGeometryIndices(uint16* indices);
    static void generateGeometry(const Scope* scope, Vertex* vertices);
static void generateElevation(Vertex* vertices, uint textureId);
    
    uint16* geometryIndices;
    gridProcessing::Id geometrySlot;
    Cache::PoolId geometryPool;
    RamAllocator* geometryAllocator;
    
gridProcessing::Id elevationSlot;
Cache::PoolId elevationPool;
TextureAllocator* elevationAllocator;

//- part of gridProcessing conventions
public:
    static void updateCallback(void* object,
                               const gridProcessing::ScopeData& scopeData,
                               GLContextData& contextData);
    static void preDisplayCallback(void* object, GLContextData& contextData);
    static void displayCallback(void* object,
                                const gridProcessing::ScopeData& scopeData,
                                GLContextData& contextData);
    static void postDisplayCallback(void* object, GLContextData& contextData);

//- inherited from GridClient
public:
    virtual void registerToGrid(GlobalGrid* grid);

//- inherited from GLObject
   	virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Terrain_H_

