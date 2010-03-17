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
    void computeBoundingSphere(Scalar verticalScale);
    /** compute the various "cached values" (e.g. bounding sphere, centroid,
        etc.) */
    void init(Scalar verticalScale);

    /** 3D vertex data for the node's flat-sphere geometry */
    Vertex* geometry;
    /** height map defining elevations within the node */
    DemHeight* height;
    /** color texture */
    TextureColor* color;

///\todo integrate me properly into the caching scheme (VIS 2010)
Colors lineData;

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



END_CRUSTA


#endif //_QuadNodeData_H_
