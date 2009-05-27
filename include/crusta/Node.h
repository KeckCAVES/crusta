#ifndef _Node_H_
#define _Node_H_

#include <vector>

#include <crusta/QuadtreeFileSpecs.h>
#include <crusta/QuadNodeData.h>
#include <crusta/TreeIndex.h>
#include <crusta/Scope.h>

BEGIN_CRUSTA

class QuadTerrain;

/** stores aspects of the terrain required for approximation management */
class Node
{
public:
    typedef std::vector<Node*> ChildBlocks;
    
    Node() :
        demTile(DemFile::INVALID_TILEINDEX),
        colorTile(ColorFile::INVALID_TILEINDEX), mainBuffer(NULL),
        videoBuffer(NULL), parent(NULL), children(NULL)
    {
        elevationRange[0] = elevationRange[1] = DemHeight(0);
        childDemTiles[0] = childDemTiles[1] = DemFile::INVALID_TILEINDEX;
        childDemTiles[2] = childDemTiles[4] = DemFile::INVALID_TILEINDEX;
        childColorTiles[0] = childColorTiles[1] = ColorFile::INVALID_TILEINDEX;
        childColorTiles[2] = childColorTiles[4] = ColorFile::INVALID_TILEINDEX;
    }


    /** pointer to the tree containing the node (to access shared data) */
    QuadTerrain* terrain;
    
    /** uniquely characterize this node's "position" in the tree. The tree
     index must correlate with the global hierarchy of the data
     sources */
    TreeIndex index;
    /** caches this node's scope for visibility and lod evaluation */
    Scope scope;

    /** centroid of the node geometry on the average elevation shell */
    DemHeight centroid[3];
    /** the range of the elevation values */
    DemHeight elevationRange[2];
    /** index of the DEM tile in the database */
    DemFile::TileIndex demTile;
    /** index of the DEM tiles of the children. This is to avoid having to
     read the indices from file everytime a child needs to be fetched */
    DemFile::TileIndex childDemTiles[4];
    /** index of the color texture tile in the database */
    ColorFile::TileIndex colorTile;
    /** index of the color tiles of the children. This is to avoid having to
        read the indices from file everytime a child needs to be fetched */
    ColorFile::TileIndex childColorTiles[4];
    
    /** cache buffer containing the data related to this node */
    MainCacheBuffer* mainBuffer;
    /** cache buffer containing the GL data related to this node (can be
     stored here, since an ActiveNode exists only in a GL context) */
    VideoCacheBuffer* videoBuffer;
    
    /** pointer to the parent node in the tree */
    Node* parent;
    /** pointer to the children nodes in the tree */
    Node* children;
};

END_CRUSTA

#endif //_Node_H_
