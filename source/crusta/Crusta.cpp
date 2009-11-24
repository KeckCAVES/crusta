#include <crusta/Crusta.h>

#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>

#include <crusta/DataManager.h>
#include <crusta/MapManager.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/Triacontahedron.h>

BEGIN_CRUSTA

/* start the frame counting at 2 because initialization code uses unsigneds that
   are initialized with 0. Thus if crustaFrameNumber starts at 0, the init code
   wouldn't be able to retrieve any cache buffers since all the buffers of the
   current and previous frame are locked */
FrameNumber Crusta::currentFrame   = 2;
FrameNumber Crusta::lastScaleFrame = 2;

double Crusta::verticalScale = 1.0;

DataManager* Crusta::dataMan = NULL;
MapManager*  Crusta::mapMan  = NULL;

Crusta::RenderPatches Crusta::renderPatches;

Crusta::Actives Crusta::actives;
Threads::Mutex Crusta::activesMutex;

void Crusta::
init(const std::string& demFileBase, const std::string& colorFileBase)
{
    Triacontahedron polyhedron(SPHEROID_RADIUS);

    dataMan  = new DataManager(&polyhedron, demFileBase, colorFileBase);
    mapMan   = new MapManager();

    uint numPatches = polyhedron.getNumPatches();
    renderPatches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
        renderPatches[i] = new QuadTerrain(i, polyhedron.getScope(i));
}

void Crusta::
shutdown()
{
    for (RenderPatches::iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        delete *it;
    }
}

double Crusta::
getHeight(double x, double y, double z)
{
    Scope::Vertex p(x,y,z);

//- find the base patch
    MainCacheBuffer*  nodeBuf = NULL;
    QuadNodeMainData* node    = NULL;
    for (int patch=0; patch<static_cast<int>(renderPatches.size()); ++patch)
    {
        //grab specification of the patch
        nodeBuf = crustaQuadCache.getMainCache().findCached(TreeIndex(patch));
        assert(nodeBuf != NULL);
        node = &nodeBuf->getData();

        //check containment
        if (node->scope.contains(p))
            break;
    }

    assert(node->index.patch < static_cast<uint>(renderPatches.size()));

//- grab the finest level data possible
    while (true)
    {
        //is it even possible to retrieve higher res data?
        bool higherResDataFetchable = false;
        for (int i=0; i<4; ++i)
        {
            if (node->childDemTiles[i]  !=  DemFile::INVALID_TILEINDEX ||
                node->childColorTiles[i]!=ColorFile::INVALID_TILEINDEX)
            {
                higherResDataFetchable = true;
                break;
            }
        }
        if (!higherResDataFetchable)
            break;

        //try to grab the children for evaluation
        CacheRequests missingChildren;
        for (int i=0; i<4; ++i)
        {
            //find child
            TreeIndex childIndex      = node->index.down(i);
            MainCacheBuffer* childBuf =
                crustaQuadCache.getMainCache().findCached(childIndex);
            if (childBuf == NULL)
                missingChildren.push_back(CacheRequest(0, nodeBuf, i));
            else if (childBuf->getData().scope.contains(p))
            {
                //switch to the child for traversal continuation
                nodeBuf = childBuf;
                node    = &childBuf->getData();
                missingChildren.clear();
                break;
            }
        }
        if (!missingChildren.empty())
        {
            //request the missing and stop traversal
            crustaQuadCache.getMainCache().request(missingChildren);
            break;
        }
    }

//- locate the cell of the refinement containing the point
    int numLevels = 1;
    int res = TILE_RESOLUTION-1;
    while (res > 1)
    {
        ++numLevels;
        res >>= 1;
    }

    Scope scope = node->scope;
    int offset[2] = {0,0};
    int shift = (TILE_RESOLUTION-1)>>1;
    for (int level=1; level<numLevels; ++level)
    {
        //compute the coverage for the children
        Scope childScopes[4];
        scope.split(childScopes);

        //find the child containing the point
        for (int i=0; i<4; ++i)
        {
            if (childScopes[i].contains(p))
            {
                //adjust the current bottom-left offset and switch to that scope
                offset[0] += i&0x1 ? shift : 0;
                offset[1] += i&0x2 ? shift : 0;
                shift    >>= 1;
                scope      = childScopes[i];
                break;
            }
        }
    }

//- sample the cell
///\todo sample properly. For now just return the height of the corner
    double height = node->height[offset[1]*TILE_RESOLUTION + offset[0]];
    height       *= Crusta::getVerticalScale();
    return height;
}

const FrameNumber& Crusta::
getCurrentFrame()
{
    return currentFrame;
}

const FrameNumber& Crusta::
getLastScaleFrame()
{
    return lastScaleFrame;
}

void Crusta::
setVerticalScale(double newVerticalScale)
{
    verticalScale  = newVerticalScale;
    lastScaleFrame = currentFrame;
}

double Crusta::
getVerticalScale()
{
    return verticalScale;
}

DataManager* Crusta::
getDataManager()
{
    return dataMan;
}

MapManager* Crusta::
getMapManager()
{
    return mapMan;
}


void Crusta::
submitActives(const Actives& touched)
{
    Threads::Mutex::Lock lock(activesMutex);
    actives.insert(actives.end(), touched.begin(), touched.end());
}


void Crusta::
frame()
{
    ++currentFrame;

    DEBUG_OUT(8, "\n\n\n--------------------------------------\n%u\n\n\n",
              static_cast<unsigned int>(currentFrame));

    //make sure all the active nodes are current
    confirmActives();

    //process the requests from the last frame
    crustaQuadCache.getMainCache().frame();
}

void Crusta::
display(GLContextData& contextData)
{
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->display(contextData);
    }
}

void Crusta::
confirmActives()
{
    for (Actives::iterator it=actives.begin(); it!=actives.end(); ++it)
    {
        if (!(*it)->isCurrent())
            (*it)->getData().computeBoundingSphere();
        (*it)->touch();
    }
    actives.clear();
}


END_CRUSTA
