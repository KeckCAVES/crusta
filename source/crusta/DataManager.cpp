#include <crusta/DataManager.h>

#include <sstream>

#include <Math/Constants.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/Polyhedron.h>
#include <crusta/QuadCache.h>

BEGIN_CRUSTA

DataManager::
DataManager(Polyhedron* polyhedron, const std::string& demBase,
            const std::string& colorBase, Crusta* iCrusta) :
    CrustaComponent(iCrusta)
{
    uint resolution[2] = { TILE_RESOLUTION, TILE_RESOLUTION };
    uint numPatches = polyhedron->getNumPatches();

    if (!demBase.empty())
    {
        demFiles.resize(numPatches);
        for (uint i=0; i<numPatches; ++i)
        {
            std::ostringstream demName;
            demName << demBase << "_" << i << ".qtf";
            demFiles[i] = new DemFile(demName.str().c_str(), resolution);
        }
        demNodata = demFiles[0]->getDefaultPixelValue();
    }
    else
        demNodata = DemHeight(0);

    if (!colorBase.empty())
    {
        colorFiles.resize(numPatches);
        for (uint i=0; i<numPatches; ++i)
        {
            std::ostringstream colorName;
            colorName << colorBase << "_" << i << ".qtf";
            colorFiles[i] = new ColorFile(colorName.str().c_str(), resolution);
        }
        colorNodata = colorFiles[0]->getDefaultPixelValue();
    }
    else
        colorNodata = TextureColor(128, 128, 128);

    geometryBuf = new double[TILE_RESOLUTION*TILE_RESOLUTION*3];
}

DataManager::
~DataManager()
{
    for (DemFiles::iterator it=demFiles.begin(); it!=demFiles.end(); ++it)
        delete *it;
    for (ColorFiles::iterator it=colorFiles.begin(); it!=colorFiles.end(); ++it)
        delete *it;

    delete[] geometryBuf;
}


bool DataManager::
hasDemData() const
{
    return !demFiles.empty();
}

bool DataManager::
hasColorData() const
{
    return !colorFiles.empty();
}


void DataManager::
loadRoot(TreeIndex rootIndex, const Scope& scope)
{
    bool existed = false;
    MainCacheBuffer* rootBuf = crusta->getCache()->getMainCache().getBuffer(
        rootIndex, &existed);
    if (existed)
        return;
    if (rootBuf == NULL)
    {
        Misc::throwStdErr("DataManager::loadRoot: failed to acquire cache "
                          "buffer for root node of patch %d", rootIndex.patch);
    }

    QuadNodeMainData& root = rootBuf->getData();
    root.index     = rootIndex;
    root.scope     = scope;
    root.demTile   = hasDemData()   ? 0 :   DemFile::INVALID_TILEINDEX;
    root.colorTile = hasColorData() ? 0 : ColorFile::INVALID_TILEINDEX;

    sourceDem(NULL,  root);
    sourceColor(NULL,root);
    generateGeometry(root);

    crusta->getCache()->getMainCache().pin(rootBuf);
}

void DataManager::
loadChild(MainCacheBuffer* parent, uint8 which, MainCacheBuffer* child)
{
    QuadNodeMainData& parentData = parent->getData();

    //compute the child scopes
    Scope childScopes[4];
    parentData.scope.split(childScopes);

    QuadNodeMainData& childData = child->getData();
    childData.index     = parentData.index.down(which);
    childData.scope     = childScopes[which];
    childData.demTile   = parentData.childDemTiles[which];
    childData.colorTile = parentData.childColorTiles[which];

    //clear the old quadtreefile tile indices
    for (int i=0; i<4; ++i)
    {
        childData.childDemTiles[i]   = DemFile::INVALID_TILEINDEX;
        childData.childColorTiles[i] = ColorFile::INVALID_TILEINDEX;
    }

    //clear the old line data
    childData.lineCoverage.clear();
    childData.lineNumSegments = 0;
    childData.lineData.clear();

    //grab the proper data for the child
    sourceDem(&parentData,   childData);
    sourceColor(&parentData, childData);
    generateGeometry(childData);

/**\todo Vis2010 This is where the coverage data should be propagated to the
child. But, here I only see individual children, thus I'd have to split the
dirty bit to reflect that individuality. For now, since even loaded nodes need
to be inserted proper after all checks pass in QuadTerrain, just leave it up to
that to pass along the proper data */
}


void DataManager::
generateGeometry(QuadNodeMainData& child)
{
///\todo use average height to offset from the spheroid
    double shellRadius = crusta->getSettings().globeRadius;
    child.scope.getRefinement(shellRadius, TILE_RESOLUTION, geometryBuf);

    /* compute and store the centroid here, since node-creation level generation
     of these values only happens after the data load step */
    Scope::Vertex scopeCentroid = child.scope.getCentroid(shellRadius);
    child.centroid[0] = scopeCentroid[0];
    child.centroid[1] = scopeCentroid[1];
    child.centroid[2] = scopeCentroid[2];

    QuadNodeMainData::Vertex* v = child.geometry;
    for (double* g=geometryBuf; g<geometryBuf+TILE_RESOLUTION*TILE_RESOLUTION*3;
         g+=3, ++v)
    {
        v->position[0] = DemHeight(g[0] - child.centroid[0]);
        v->position[1] = DemHeight(g[1] - child.centroid[1]);
        v->position[2] = DemHeight(g[2] - child.centroid[2]);
    }
}

template <typename PixelParam>
inline void
sampleParentBase(int child, PixelParam range[2], PixelParam* dst,
                 PixelParam* src)
{
    static const int offsets[4] = {
        0, (TILE_RESOLUTION-1)>>1, ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION,
        ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION + ((TILE_RESOLUTION-1)>>1) };
    int halfSize[2] = { (TILE_RESOLUTION+1)>>1, (TILE_RESOLUTION+1)>>1 };

    for (int y=0; y<halfSize[1]; ++y)
    {
        PixelParam* wbase = dst + y*2*TILE_RESOLUTION;
        PixelParam* rbase = src + y*TILE_RESOLUTION + offsets[child];

        for (int x=0; x<halfSize[0]; ++x, wbase+=2, ++rbase)
        {
            range[0] = pixelMin(range[0], rbase[0]);
            range[1] = pixelMax(range[1], rbase[0]);

            wbase[0] = rbase[0];
            if (x<halfSize[0]-1)
                wbase[1] = pixelAvg(rbase[0], rbase[1]);
            if (y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION] = pixelAvg(rbase[0],
                                                  rbase[TILE_RESOLUTION]);
            }
            if (x<halfSize[0]-1 && y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION+1] = pixelAvg(rbase[0], rbase[1],
                                                    rbase[TILE_RESOLUTION],
                                                    rbase[TILE_RESOLUTION+1]);
            }
        }
    }
}

inline void
sampleParent(int child, DemHeight range[2], DemHeight* dst, DemHeight* src)
{
    range[0] = Math::Constants<DemHeight>::max;
    range[1] = Math::Constants<DemHeight>::min;

    sampleParentBase(child, range, dst, src);
}

inline void
sampleParent(int child, TextureColor range[2], TextureColor* dst,
             TextureColor* src)
{
    range[0] = TextureColor(255,255,255);
    range[1] = TextureColor(0,0,0);

    sampleParentBase(child, range, dst, src);
}

void DataManager::
sourceDem(QuadNodeMainData* parent, QuadNodeMainData& child)
{
    DemHeight* heights =  child.height;
    DemHeight* range   = &child.elevationRange[0];

    if (child.demTile != DemFile::INVALID_TILEINDEX)
    {
        DemTileHeader header;
        DemFile* file = demFiles[child.index.patch];
        if (!file->readTile(child.demTile,child.childDemTiles,header,heights))
        {
            Misc::throwStdErr("DataManager::sourceDem: Invalid DEM file: "
                              "could not read node %s's data",
                              child.index.med_str().c_str());
        }
        range[0] = header.range[0];
        range[1] = header.range[1];
    }
    else
    {
        if (parent != NULL)
            sampleParent(child.index.child, range, heights, parent->height);
        else
        {
            range[0] = range[1] = demNodata;
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                heights[i] = demNodata;
        }
    }
    child.init(crusta->getSettings().globeRadius, crusta->getVerticalScale());
}

void DataManager::
sourceColor(QuadNodeMainData* parent, QuadNodeMainData& child)
{
    TextureColor* colors = child.color;

    if (child.colorTile != ColorFile::INVALID_TILEINDEX)
    {
        ColorFile* file = colorFiles[child.index.patch];
        if (!file->readTile(child.colorTile, child.childColorTiles, colors))
        {
            Misc::throwStdErr("DataManager::sourceColor: Invalid Color "
                              "file: could not read node %s's data",
                              child.index.med_str().c_str());
        }
    }
    else
    {
        TextureColor range[2];
        if (parent != NULL)
            sampleParent(child.index.child, range, colors, parent->color);
        else
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                colors[i] = colorNodata;
        }
    }
}


END_CRUSTA
