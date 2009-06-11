#ifndef _QuadNodeData_H_
#define _QuadNodeData_H_

#include <GL/GLVertex.h>

#include <crusta/QuadtreeFileSpecs.h>
#include <crusta/TreeIndex.h>
#include <crusta/Scope.h>

BEGIN_CRUSTA

/** stores the main RAM view-independent data of the terrain that can be shared
    between multiple view-dependent trees */
struct QuadNodeMainData
{
    typedef GLVertex<void, 0, void, 0, void, float, 3> Vertex;

    QuadNodeMainData(uint size);
    ~QuadNodeMainData();

    /** compute the bounding sphere. It is dependent on the vertical scale,
        so this method is a convinient API for such updates */
    void computeBoundingSphere();
    /** compute the various "cached values" (e.g. bounding sphere, centroid,
        etc.) */
    void init();

    /** 3D vertex data for the node's flat-sphere geometry */
    Vertex* geometry;
    /** height map defining elevations within the node */
    DemHeight* height;
    /** color texture */
    TextureColor* color;

    /** uniquely characterize this node's "position" in the tree. The tree
     index must correlate with the global hierarchy of the data
     sources */
    TreeIndex index;
    /** caches this node's scope for visibility and lod evaluation */
    Scope scope;

    /** center of the bounding sphere primitive */
    Scope::Vertex boundingCenter;
    /** radius of a sphere containing the node */
    Scope::Scalar boundingRadius;

    /** centroid of the node geometry on the average elevation shell */
    DemHeight centroid[3];
    /** the range of the elevation values */
    DemHeight elevationRange[2];

    /** index of the DEM tile in the database */
    DemFile::TileIndex demTile;
    /** indices of the children in the DEM file */
    DemFile::TileIndex childDemTiles[4];
    /** index of the color texture tile in the database */
    ColorFile::TileIndex colorTile;
    /** indices of the children in the texture file */
    ColorFile::TileIndex childColorTiles[4];
};

/** stores the video RAM view-independent data of the terrain that can be shared
 between multiple view-dependent trees */
struct QuadNodeVideoData
{
    QuadNodeVideoData(uint size);
    ~QuadNodeVideoData();

    void createTexture(GLuint& texture, GLint internalFormat, uint size);

    /** texture storing the node's flat-sphere geometry */
    GLuint geometry;
    /** texture storing the node's elevations */
    GLuint height;
    /** texture storing the node's color image */
    GLuint color;
};




template <typename NodeDataType>
class CacheBuffer
{
public:
    CacheBuffer(uint size);

    /** retrieve the main memory node data from the buffer */
    NodeDataType& getData();

    /** check to see if the semi-dynamic data of the buffer is current and can
        be used without an update */
    bool isCurrent();
    
    /** check to see if the buffer is valid */
    bool isValid();

    /** confirm use of the buffer for the current frame */
    void touch();
    /** invalidate a buffer */
    void invalidate();
    /** pin the element in the cache such that it cannot be swaped out */
    void pin(bool wantPinned=true, bool asUsable=true);
    /** query the frame number of the buffer */
    FrameNumber getFrameNumber() const;

protected:
    /** sequence number used to evaluate LRU prioritization */
    FrameNumber frameNumber;
    /** the actual node data */
    NodeDataType data;
};

END_CRUSTA

#include <crusta/QuadNodeData.hpp>

BEGIN_CRUSTA

typedef CacheBuffer<QuadNodeMainData>  MainCacheBuffer;
typedef CacheBuffer<QuadNodeVideoData> VideoCacheBuffer;

END_CRUSTA

#endif //_QuadNodeData_H_
