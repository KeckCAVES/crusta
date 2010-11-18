#include <crusta/DataManager.h>

#include <sstream>

#include <Math/Constants.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/PixelOps.h>
#include <crusta/PolyhedronLoader.h>
#include <crusta/QuadCache.h>
#include <crusta/Triacontahedron.h>


BEGIN_CRUSTA


DataManager::
DataManager(Crusta* iCrusta) :
    CrustaComponent(iCrusta), demFile(NULL), colorFile(NULL), polyhedron(NULL),
    demNodata(0), colorNodata(128, 128, 128)
{
    geometryBuf = new double[TILE_RESOLUTION*TILE_RESOLUTION*3];
}

DataManager::
~DataManager()
{
    unload();

    delete[] geometryBuf;
}


void DataManager::
load(const std::string& demPath, const std::string& colorPath)
{
    //detach from existing databases
    unload();

    if (!demPath.empty())
    {
        demFile = new DemFile;
        try
        {
            demFile->open(demPath);
            polyhedron = PolyhedronLoader::load(demFile->getPolyhedronType(),
                                                SPHEROID_RADIUS);
            demNodata  = demFile->getNodata();
        }
        catch (std::runtime_error e)
        {
            delete demFile;
            demFile = NULL;
            demNodata = GlobeData<DemHeight>::defaultNodata();

            std::cerr << e.what();
        }
    }
    else
        demNodata = GlobeData<DemHeight>::defaultNodata();

    if (!colorPath.empty())
    {
        colorFile = new ColorFile;
        try
        {
            colorFile->open(colorPath);
            colorNodata = colorFile->getNodata();

            if (polyhedron != NULL)
            {
                if (polyhedron->getType() != colorFile->getPolyhedronType())
                {
                    std::cerr << "Mismatching polyhedron for " <<
                                 demPath.c_str() << " and " <<
                                 colorPath.c_str() << ".\n";

                    delete colorFile;
                    colorFile = NULL;
                    colorNodata = GlobeData<TextureColor>::defaultNodata();
                }
            }
            else
            {
                polyhedron =
                    PolyhedronLoader::load(colorFile->getPolyhedronType(),
                                           SPHEROID_RADIUS);
            }

        }
        catch (std::runtime_error e)
        {
            delete colorFile;
            colorFile = NULL;
            colorNodata = GlobeData<TextureColor>::defaultNodata();

            std::cerr << e.what();
        }
    }
    else
        colorNodata = GlobeData<TextureColor>::defaultNodata();

    if (polyhedron == NULL)
        polyhedron = new Triacontahedron(SPHEROID_RADIUS);
}

void DataManager::
unload()
{
    if (demFile)
    {
        delete demFile;
        demFile = NULL;
    }
    if (colorFile)
    {
        delete colorFile;
        colorFile = NULL;
    }
    if (polyhedron)
    {
        delete polyhedron;
        polyhedron = NULL;
    }
}


bool DataManager::
hasDemData() const
{
    return demFile != NULL;
}

bool DataManager::
hasColorData() const
{
    return colorFile != NULL;
}


const Polyhedron* const DataManager::
getPolyhedron() const
{
    return polyhedron;
}

const DemHeight& DataManager::
getDemNodata()
{
    return demNodata;
}

const TextureColor& DataManager::
getColorNodata()
{
    return colorNodata;
}


void DataManager::
loadRoot(TreeIndex rootIndex, const Scope& scope)
{
    MainCache& mainCache = crusta->getCache()->getMainCache();
    MainCacheBuffer* rootBuf = mainCache.getBuffer(rootIndex);
    if (rootBuf == NULL)
    {
        Misc::throwStdErr("DataManager::loadRoot: failed to acquire cache "
                          "buffer for root node of patch %d", rootIndex.patch);
    }

    QuadNodeMainData& root = rootBuf->getData();
    root.index     = rootIndex;
    root.scope     = scope;
    root.demTile   = hasDemData()   ? 0 : INVALID_TILEINDEX;
    root.colorTile = hasColorData() ? 0 : INVALID_TILEINDEX;

    //clear the old quadtreefile tile indices
    for (int i=0; i<4; ++i)
    {
        root.childDemTiles[i]   = INVALID_TILEINDEX;
        root.childColorTiles[i] = INVALID_TILEINDEX;
    }

    //clear the old line data
    root.lineCoverage.clear();
    root.lineData.clear();

    //grab the proper data for the root
    sourceDem(NULL,  root);
    sourceColor(NULL,root);
    generateGeometry(root);

    mainCache.pin(rootBuf);
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
        childData.childDemTiles[i]   = INVALID_TILEINDEX;
        childData.childColorTiles[i] = INVALID_TILEINDEX;
    }

    //clear the old line data
    childData.lineCoverage.clear();
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
    double shellRadius = SPHEROID_RADIUS;
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
                 PixelParam* src, const PixelParam& nodata)
{
    typedef PixelOps<PixelParam> po;

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
            range[0] = po::minimum(range[0], rbase[0], nodata);
            range[1] = po::maximum(range[1], rbase[0], nodata);

            wbase[0] = rbase[0];
            if (x<halfSize[0]-1)
                wbase[1] = po::average(rbase[0], rbase[1], nodata);
            if (y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION] =
                    po::average(rbase[0], rbase[TILE_RESOLUTION], nodata);
            }
            if (x<halfSize[0]-1 && y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION+1] = po::average(rbase[0], rbase[1],
                    rbase[TILE_RESOLUTION], rbase[TILE_RESOLUTION+1], nodata);
            }
        }
    }
}

inline void
sampleParent(int child, DemHeight range[2], DemHeight* dst, DemHeight* src,
             const DemHeight& nodata)
{
    range[0] = Math::Constants<DemHeight>::max;
    range[1] = Math::Constants<DemHeight>::min;

    sampleParentBase(child, range, dst, src, nodata);
}

inline void
sampleParent(int child, TextureColor range[2], TextureColor* dst,
             TextureColor* src, const TextureColor& nodata)
{
    range[0] = TextureColor(255,255,255);
    range[1] = TextureColor(0,0,0);

    sampleParentBase(child, range, dst, src, nodata);
}

void DataManager::
sourceDem(QuadNodeMainData* parent, QuadNodeMainData& child)
{
    typedef DemFile::File    File;
    typedef File::TileHeader TileHeader;

    DemHeight* heights =  child.height;
    DemHeight* range   = &child.elevationRange[0];

    if (child.demTile != INVALID_TILEINDEX)
    {
        TileHeader header;
        File* file = demFile->getPatch(child.index.patch);
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
        {
            sampleParent(child.index.child, range, heights, parent->height,
                         demNodata);
        }
        else
        {
            range[0] =  Math::Constants<DemHeight>::max;
            range[1] = -Math::Constants<DemHeight>::max;
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                heights[i] = demNodata;
        }
    }
    child.init(crusta->getVerticalScale());
}

void DataManager::
sourceColor(QuadNodeMainData* parent, QuadNodeMainData& child)
{
    typedef ColorFile::File File;

    TextureColor* colors = child.color;

    if (child.colorTile != INVALID_TILEINDEX)
    {
        File* file = colorFile->getPatch(child.index.patch);
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
        {
            sampleParent(child.index.child, range, colors, parent->color,
                         colorNodata);
        }
        else
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                colors[i] = colorNodata;
        }
    }
}


END_CRUSTA
