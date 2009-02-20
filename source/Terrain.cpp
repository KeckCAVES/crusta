#include <Terrain.h>

#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>

#include <GlobalGrid.h>
#include <TextureAllocator.h>

///\todo remove
#include <simplexnoise1234.h>


BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    TILE_RESOLUTION*((TILE_RESOLUTION+1)*2+2)-2;
static const uint ELEVATION_RESOLUTION = TILE_RESOLUTION+1;
static const float ELEVATION_COORD_START = (1.0/(ELEVATION_RESOLUTION-1)) * 0.5;
static const float ELEVATION_COORD_END = 1.0 - ELEVATION_COORD_START;
    

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
      12
       |
10 11 13 - 14 
       9 -  7
       | /  |
       8  -  456
       0  - 3
       |  / |
       1  - 2
*/
    
elevationAllocator = new TextureAllocator(
    32, GL_INTENSITY32F_ARB, ELEVATION_RESOLUTION, ELEVATION_RESOLUTION,
    GL_LINEAR, GL_LINEAR);
elevationPool = Cache::registerVideoCachePool(elevationAllocator);
}

Terrain::
~Terrain()
{
    delete geometryAllocator;
delete elevationAllocator;
    delete[] geometryIndices;
}


Terrain::GlData::
GlData()
{
    elevationShader.compileVertexShader("elevation.vs");
    elevationShader.compileFragmentShader("elevation.fs");
    elevationShader.linkShader();
    
    elevationShader.useProgram();
    GLint heightTexUniform = elevationShader.getUniformLocation("heights");
    glUniform1i(heightTexUniform, 1);
    elevationShader.disablePrograms();
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
        Vertex::TexCoord(ELEVATION_COORD_START,ELEVATION_COORD_END),
        Vertex::TexCoord(ELEVATION_COORD_START,ELEVATION_COORD_START),
        Vertex::TexCoord(ELEVATION_COORD_END,ELEVATION_COORD_START),
        Vertex::TexCoord(ELEVATION_COORD_END,ELEVATION_COORD_END)
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
generateElevation(Vertex* vertices, uint textureId)
{
///\todo need to make the cache main to video transparent
    uint numVertices = ELEVATION_RESOLUTION*ELEVATION_RESOLUTION;
    float* tex = new float[numVertices];
    for (float* cur=tex; cur<tex+numVertices; ++cur, ++vertices)
    {
        float theta = acos(vertices->position[2]);
        float phi = atan2(vertices->position[1], vertices->position[0]);
        float elevation = SimplexNoise1234::noise(theta, phi);
        elevation += 1.0f;
        elevation /= 8.0f;
        elevation += 1.0f;
        *cur = elevation;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    ELEVATION_RESOLUTION, ELEVATION_RESOLUTION,
                    GL_RED, GL_FLOAT, tex);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] tex;
}



void Terrain::
updateCallback(void* object, const gridProcessing::ScopeData& scopeData,
               GLContextData& contextData)
{
    Terrain* self = static_cast<Terrain*>(object);

///\todo remove
assert(scopeData.data->size() > self->elevationSlot);
    //geometry is generated on the spot if it isn't available yet
    Cache::Buffer*& geometryBuffer = (*scopeData.data)[self->geometrySlot];
    if (geometryBuffer == NULL)
    {
        Cache* mainCache = Cache::getMainCache();
        mainCache->grabNew(self->geometryPool, geometryBuffer);
assert(geometryBuffer->getMemory() != NULL);
        generateGeometry(
            scopeData.scope,
            reinterpret_cast<Vertex*>(geometryBuffer->getMemory()));
    }
    else
        Cache::getMainCache()->touch(geometryBuffer);
    
    Cache::Buffer*& elevationBuffer = (*scopeData.data)[self->elevationSlot];
    if (elevationBuffer == NULL)
    {
        Cache::getVideoCache()->grabNew(self->elevationPool, elevationBuffer);
assert(elevationBuffer->getId() != 0);
        generateElevation(
            reinterpret_cast<Vertex*>(geometryBuffer->getMemory()),
            elevationBuffer->getId());
    }
    else
        Cache::getVideoCache()->touch(elevationBuffer);
}


void Terrain::
preDisplayCallback(void* object, GLContextData& contextData)
{
    Terrain* self = reinterpret_cast<Terrain*>(object);

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
//	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glLineWidth(1.0f);
    
    glActiveTexture(GL_TEXTURE1);

    GlData* glData = contextData.retrieveDataItem<GlData>(self);
    glData->elevationShader.useProgram();
}

void Terrain::
displayCallback(void* object, const gridProcessing::ScopeData& scopeData,
                GLContextData& contextData)
{
    Terrain* self = reinterpret_cast<Terrain*>(object);

///\todo remove
assert(scopeData.data->size() > self->elevationSlot);
    
assert(((*scopeData.data)[self->geometrySlot])->getMemory() != NULL);
assert(((*scopeData.data)[self->elevationSlot])->getId() != 0);
    Vertex* vertices = reinterpret_cast<Vertex*>(
        ((*scopeData.data)[self->geometrySlot])->getMemory());
    GLuint textureId = ((*scopeData.data)[self->elevationSlot])->getId();
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    
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

    glBindTexture(GL_TEXTURE_2D, 0);

#if 0
    glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    for (int i=0; i<4; ++i)
    {
        glNormal(scopeData.scope->corners[i] - Point::origin);
        glVertex(scopeData.scope->corners[i]);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
#endif
}

void Terrain::
postDisplayCallback(void* object, GLContextData& contextData)
{
    Terrain* self = reinterpret_cast<Terrain*>(object);

    GlData* glData = contextData.retrieveDataItem<GlData>(self);
    glData->elevationShader.disablePrograms();

    glActiveTexture(GL_TEXTURE0);

    glPopAttrib();
}

void Terrain::
registerToGrid(GlobalGrid* grid)
{
    geometrySlot = grid->registerDataSlot();
elevationSlot = grid->registerDataSlot();

    gridProcessing::ScopeCallback updating(this, &Terrain::updateCallback);
    grid->registerScopeCallback(gridProcessing::UPDATE_PHASE, updating);

    gridProcessing::ScopeCallback displaying(this, &Terrain::displayCallback,
                                             &Terrain::preDisplayCallback,
                                             &Terrain::postDisplayCallback);
    grid->registerScopeCallback(gridProcessing::DISPLAY_PHASE, displaying);
}

void Terrain::
initContext(GLContextData& contextData) const
{
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
