#ifndef _QuadNodeData_H_
#define _QuadNodeData_H_

#include <map>

#include <GL/VruiGlew.h> //must be included before gl.h
#include <GL/GLVertex.h>

#include <crusta/map/Shape.h>
#include <crusta/QuadtreeFileSpecs.h>
#include <crusta/TreeIndex.h>
#include <crusta/Scope.h>


BEGIN_CRUSTA


///\todo change name to more appropriate NodeDynamicData
/** stores crusta internal transient data related to a node of the terrain tree
    hierarchy */
struct NodeData
{
    typedef std::map<const Shape*, Shape::ControlPointHandleList>
        ShapeCoverage;

    NodeData();

    /** compute the bounding sphere. It is dependent on the vertical scale,
        so this method is a convinient API for such updates */
    void computeBoundingSphere(Scalar radius, Scalar verticalScale);
    /** compute the various "cached values" (e.g. bounding sphere, centroid,
        etc.) */
    void init(Scalar radius, Scalar verticalScale);

///\todo integrate me properly into the caching scheme (VIS 2010)
bool          lineCoverageDirty;
FrameStamp    lineCoverageAge;
Vector2fs     lineCoverageOffsets;
ShapeCoverage lineCoverage;
int           lineNumSegments;
Colors        lineData;

    /** uniquely characterize this node's "position" in the tree. The tree
     index must correlate with the global hierarchy of the data
     sources */
    TreeIndex index;
    /** caches this node's scope for visibility and lod evaluation */
    Scope scope;

    FrameStamp boundingAge;
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


/** defines a sub-region through and offset and size */
struct SubRegion
{
    SubRegion();
    SubRegion(const Point3f& iOffset, const Vector2f& iSize);
    Point3f  offset;
    Vector2f size;
};

///\todo Vis2010 integrate this into the system better
/** stores the coverage map textures */
struct StampedSubRegion : SubRegion
{
    StampedSubRegion();
    StampedSubRegion(const Point3f& iOffset, const Vector2f& iSize);
    FrameStamp age;
};


///\todo debug, remove
std::ostream& operator<<(std::ostream& os, const NodeData::ShapeCoverage& cov);


END_CRUSTA


#endif //_QuadNodeData_H_
