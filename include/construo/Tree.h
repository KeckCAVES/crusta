#ifndef _ConstruoTree_H_
#define _ConstruoTree_H_

#include <construo/SphereCoverage.h>

#include <crusta/GlobeFile.h>
#include <crusta/Scope.h>
#include <crusta/TileIndex.h>
#include <crusta/TreeIndex.h>

BEGIN_CRUSTA

/** Data elements providing the basis for a quadtree structure where elements
    are dynamically allocated and interconnected through pointers */
template <typename PixelParam>
class TreeNode
{
public:
    typedef GlobeData<PixelParam> gd;
    typedef typename gd::File     File;

    TreeNode();
    virtual ~TreeNode();

///\todo generalize this so that requests can be made for arbitrary levels
    /** query a kin of the node. Since the trees are created on demand,
        valid parts of the tree may not be represented in memory during neighbor
        traversal. By default such parts will be loaded from file if possible,
        but the behaviour can be altered through 'loadMissing'. */
    bool getKin(TreeNode*& kin, int offsets[2], bool loadMissing=true,
                int down=0, uint* orientation=NULL);

    /** create in-memory storage for the children nodes with the most basic
        properties (i.e. parent link-up, treeState, treeIndex, scope and
        sphereCoverage) */
    void createChildren();
    /** create in-memory representations for the children nodes if they are
        reflected in the quadtree file */
    void loadMissingChildren();
    /** determine the approximate resolution of the node. This is only computed
        for the step off the middle of an edge as we assume the sphere to be
        worst approximated away from the corners of the scope */
    void computeResolution();

    ///pointer up to the parent node in the tree
    TreeNode* parent;
    /** pointer to the first child in the set of four children. Children must be
        allocated as a continuous range in memory */
    TreeNode* children;

//- Construo node data
    static GlobeFile<PixelParam>* globeFile;

    TreeIndex treeIndex;
    TileIndex tileIndex;

    Scope scope;
    SphereCoverage coverage;
    Scope::Scalar resolution;

    PixelParam* data;

    bool mustBeUpdated;

protected:
    bool isExplicitNeighborNode;

///\todo remove
public:
static bool debugGetKin;
};

template <typename PixelParam>
class ExplicitNeighborNode : public TreeNode<PixelParam>
{
public:
    ExplicitNeighborNode();
    void setNeighbors(ExplicitNeighborNode* nodes[4],
                      const uint orientations[4]);

    ///pointers to the neighboring nodes
    ExplicitNeighborNode* neighbors[4];
    ///orientations of the neighboring nodes
    uint orientations[4];
};

template <typename PixelParam>
class Spheroid
{
public:
    typedef ExplicitNeighborNode<PixelParam> BaseNode;
    typedef std::vector<BaseNode>            BaseNodes;

    Spheroid(const std::string& baseName, const uint tileResolution[2]);

    BaseNodes             baseNodes;
    GlobeFile<PixelParam> globeFile;
};

END_CRUSTA

#include <construo/Tree.hpp>

#endif //_ConstruoTree_H_
