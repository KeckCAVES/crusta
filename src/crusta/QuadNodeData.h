#ifndef _QuadNodeData_H_
#define _QuadNodeData_H_

#include <map>

#include <crustacore/DemHeight.h>
#include <crustacore/LayerData.h>
#include <crusta/map/Shape.h>
#include <crustacore/TextureColor.h>
#include <crustacore/TileIndex.h>
#include <crustacore/TreeIndex.h>
#include <crustacore/Scope.h>

#include <crusta/glbasics.h>
#include <GL/GLVertex.h>


BEGIN_CRUSTA


///\todo change name to more appropriate NodeDynamicData
/** stores crusta internal transient data related to a node of the terrain tree
    hierarchy */
struct NodeData
{
    /** encapsulates the tile indices of the node and the children for a data
        source */
    struct Tile
    {
        Tile();
        uint8     dataId;
        TileIndex node;
        TileIndex children[4];
    };
    typedef std::vector<Tile> Tiles;

    typedef std::map<const Shape*, Shape::ControlPointHandleList>
        ShapeCoverage;

    NodeData();

    /** compute the bounding sphere. It is dependent on the vertical scale,
        so this method is a convinient API for such updates */
    void computeBoundingSphere(Scalar radius, Scalar verticalScale);

    /** get effective bounding radius considering translations by the slicing tool **/
    Point3 getEffectiveBoundingCenter() const;

    /** compute the various "cached values" (e.g. bounding sphere, centroid,
        etc.) */
    void init(Scalar radius, Scalar verticalScale);

    /** get the elevation range of the node or the default range */
    void getElevationRange(DemHeight::Type range[2]) const;
    /** get the height value if it is valid or the default */
    DemHeight::Type getHeight(const DemHeight::Type& test) const;
    /** get the color value if it is valid or the default */
    TextureColor::Type getColor(const TextureColor::Type& test) const;
    /** get the layer data value if it is valid or the default */
    LayerDataf::Type getLayerData(const LayerDataf::Type& test) const;

///\todo integrate me properly into the caching scheme (VIS 2010)
std::vector<int> lineCoverageOffsets;
ShapeCoverage    lineCoverage;
bool             lineInheritCoverage;
int              lineNumSegments;
FrameStamp       lineDataStamp;
Colors           lineData;

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
    Point3f centroid;
    /** the range of the elevation values */
    DemHeight::Type elevationRange[2];

    /** indices for the DEM tiles in the database */
    Tile demTile;
    /** indices for the Color tiles in the database */
    Tiles colorTiles;
    /** indices for the Layer tiles in the databases */
    Tiles layerTiles;
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
std::ostream& operator<<(std::ostream&os, const SubRegion& sub);
std::ostream& operator<<(std::ostream& os, const NodeData::ShapeCoverage& cov);


END_CRUSTA


#endif //_QuadNodeData_H_
