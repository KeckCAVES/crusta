#include <Terrain.h>

#include <GL/GLGeometryWrappers.h>

#include <GlobalGrid.h>

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    TILE_RESOLUTION*((TILE_RESOLUTION+1)*2+2)-2;

static void
mid(uint oneIndex, uint twoIndex, Terrain::Vertex* vertices)
{
    Terrain::Vertex& one = vertices[oneIndex];
    Terrain::Vertex& two = vertices[twoIndex];
    Terrain::Vertex& res = vertices[(oneIndex + twoIndex) >> 1];

    res.texCoord[0] = (one.texCoord[0] + two.texCoord[0]) * 0.5f;
    res.texCoord[1] = (one.texCoord[1] + two.texCoord[1]) * 0.5f;
    
    res.position[0] = (one.position[0]  + two.position[0] ) * 0.5f;
    res.position[1] = (one.position[1]  + two.position[1] ) * 0.5f;
    res.position[2] = (one.position[2]  + two.position[2] ) * 0.5f;

    double invLen   = 1.0 / sqrt(res.position[0]*res.position[0] +
                                 res.position[1]*res.position[1] +
                                 res.position[2]*res.position[2]);
    res.position[0] *= invLen;
    res.position[1] *= invLen;
    res.position[2] *= invLen;
}
static void
centroid(uint oneIndex, uint twoIndex, uint threeIndex, uint fourIndex,
         Terrain::Vertex* vertices)
{
    Terrain::Vertex& one   = vertices[oneIndex];
    Terrain::Vertex& two   = vertices[twoIndex];
    Terrain::Vertex& three = vertices[threeIndex];
    Terrain::Vertex& four  = vertices[fourIndex];
    Terrain::Vertex& res   = vertices[(oneIndex + twoIndex +
                                       threeIndex+fourIndex) >> 2];
    
    res.texCoord[0] = (one.texCoord[0]   + two.texCoord[0] +
                       three.texCoord[0] + four.texCoord[0]) * 0.25f;
    res.texCoord[1] = (one.texCoord[1]   + two.texCoord[1] +
                       three.texCoord[1] + four.texCoord[1]) * 0.25f;
    
    res.position[0] = (one.position[0]   + two.position[0] +
                       three.position[0] + four.position[0]) * 0.25f;
    res.position[1] = (one.position[1]   + two.position[1] +
                       three.position[1] + four.position[1]) * 0.25f;
    res.position[2] = (one.position[2]   + two.position[2] +
                       three.position[2] + four.position[2]) * 0.25f;

    double invLen   = 1.0 / sqrt(res.position[0]*res.position[0] +
                                 res.position[1]*res.position[1] +
                                 res.position[2]*res.position[2]);
    res.position[0] *= invLen;
    res.position[1] *= invLen;
    res.position[2] *= invLen;
}

Terrain::
Terrain()
{
    uint verticesPerSide = TILE_RESOLUTION+1;
    uint vertexBufferSize = verticesPerSide*verticesPerSide * sizeof(Vertex);
    geometryAllocator = new RamAllocator(vertexBufferSize);
    geometryPool = Cache::registerMainCachePool(geometryAllocator);
    geometryIndices = new uint16[NUM_GEOMETRY_INDICES];
    generateGeometryIndices(geometryIndices);
/*
       7
    /  |
  8  -  456
  0  - 3
  |  / |
  1  - 2
*/
}
                                                
Terrain::
~Terrain()
{
    delete geometryAllocator;
    delete[] geometryIndices;
}

void Terrain::
generateGeometryIndices(uint16* indices)
{
    int  inc      = 1;
    uint alt      = 1;
    uint index[2] = {0, TILE_RESOLUTION+1};
    for (uint b=0; b<TILE_RESOLUTION; ++b, inc=-inc, alt=1-alt,
         index[0]+=TILE_RESOLUTION+1, index[1]+=TILE_RESOLUTION+1)
    {
        for (uint i=0; i<(TILE_RESOLUTION+1)*2;
             ++i, index[alt]+=inc, alt=1-alt, ++indices)
        {
            *indices = index[alt];
        }
        index[0]-=inc; index[1]-=inc;
        if (b != TILE_RESOLUTION-1)
        {
            for (uint i=0; i<2; ++i, ++indices)
                *indices = index[1];
        }
    }
}

void Terrain::
generateGeometry(const Scope* scope, Vertex* vertices)
{
    //set the initial four corners from the scope
    static const uint cornerIndices[] = {
        TILE_RESOLUTION*(TILE_RESOLUTION+1), 0,
        TILE_RESOLUTION, TILE_RESOLUTION*(TILE_RESOLUTION+1) + TILE_RESOLUTION
    };
    static const Vertex::TexCoord cornerTexCoords[] = {
        Vertex::TexCoord(0,1), Vertex::TexCoord(0,0),
        Vertex::TexCoord(1,0), Vertex::TexCoord(1,1)
    };
    for (uint i=0; i<4; ++i)
    {
        vertices[cornerIndices[i]] = Vertex(
            cornerTexCoords[i], Vertex::Position(scope->corners[i][0],
                                                 scope->corners[i][1],
                                                 scope->corners[i][2]));
    }
    
    //refine the rest
    static const uint endIndex  = (TILE_RESOLUTION+1) * (TILE_RESOLUTION+1);
    for (uint blockSize=TILE_RESOLUTION; blockSize>1; blockSize>>=1)
    {
        uint blockSizeY = blockSize * (TILE_RESOLUTION+1);
        for (uint startY=0, endY=blockSizeY; endY<endIndex;
             startY+=blockSizeY, endY+=blockSizeY)
        {
            for (uint startX=0, endX=blockSize; endX<TILE_RESOLUTION+1;
                 startX+=blockSize, endX+=blockSize)
            {
                //left mid point (only if on the edge where it has not already
                //been computed by the neighbor)
                if (startX == 0)
                    mid(startY, endY, vertices);                    
                //bottom mid point
                mid(endY+startX, endY+endX, vertices);
                //right mid point
                mid(endY+endX, startY+endX, vertices);
                //top mid point (only if on the edge where it has not already
                //been computed by the neighbor)
                if (startY == 0)
                    mid(endX, startX, vertices);
                //centroid
                centroid(startY+startX, endY+startX, endY+endX, startY+endX,
                         vertices);
            }
        }
        
    }
}



void Terrain::
updateCallback(void* object, const gridProcessing::ScopeData& scopeData)
{
    Terrain* self = static_cast<Terrain*>(object);

///\todo remove
assert(scopeData.data->size() > self->geometrySlot);
    //geometry is generated on the spot if it isn't available yet
    Cache::Buffer*& geometryBuffer = (*scopeData.data)[self->geometrySlot];
    if (geometryBuffer == NULL)
    {
        Cache* mainCache = Cache::getMainCache();
        mainCache->grabNew(self->geometryPool, geometryBuffer);
assert(geometryBuffer->getMemory() != NULL);
        generateGeometry(scopeData.scope,
                         static_cast<Vertex*>(geometryBuffer->getMemory()));
    }
}


void Terrain::
preDisplayCallback(void* object)
{
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glLineWidth(1.0f);
}

void Terrain::
displayCallback(void* object, const gridProcessing::ScopeData& scopeData)
{
    Terrain* self = static_cast<Terrain*>(object);
    
///\todo remove
assert(scopeData.data->size() > self->geometrySlot);
    
assert(((*scopeData.data)[self->geometrySlot])->getMemory() != NULL);
    Vertex* vertices = static_cast<Vertex*>(
        ((*scopeData.data)[self->geometrySlot])->getMemory());
    glColor3f(0.5f,0.5f,0.5f);
#if 1
    glVertexPointer(vertices);
    glDrawElements(GL_TRIANGLE_STRIP, NUM_GEOMETRY_INDICES, GL_UNSIGNED_SHORT,
                   self->geometryIndices);
#else
    for (uint y=TILE_RESOLUTION+1; y<=(TILE_RESOLUTION-1)*(TILE_RESOLUTION+1);
         y+=TILE_RESOLUTION+1)
    {
        glBegin(GL_LINE_STRIP);
        for (uint x=y; x<y+TILE_RESOLUTION+1; ++x)
            glVertex(vertices[x]);
        glEnd();
    }
    for (uint x=1; x<=TILE_RESOLUTION-1; ++x)
    {
        glBegin(GL_LINE_STRIP);
        for (uint y=x; y<=x+TILE_RESOLUTION*(TILE_RESOLUTION+1);
             y+=(TILE_RESOLUTION+1))
            glVertex(vertices[y]);
        glEnd();
    }
#endif

	glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    for (int i=0; i<4; ++i)
    {
        glNormal(scopeData.scope->corners[i] - Point::origin);
        glVertex(scopeData.scope->corners[i]);
    }
    glEnd();
}

void Terrain::
postDisplayCallback(void* object)
{
    glPopAttrib();
}

void Terrain::
registerToGrid(GlobalGrid* grid)
{
    geometrySlot = grid->registerDataSlot();

    gridProcessing::ScopeCallback updating(this, &Terrain::updateCallback);
    grid->registerScopeCallback(gridProcessing::UPDATE_PHASE, updating);

    gridProcessing::ScopeCallback displaying(this, &Terrain::displayCallback,
                                             &Terrain::preDisplayCallback,
                                             &Terrain::postDisplayCallback);
    grid->registerScopeCallback(gridProcessing::DISPLAY_PHASE, displaying);
}

END_CRUSTA
