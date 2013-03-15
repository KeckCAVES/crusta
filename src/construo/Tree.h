#ifndef _ConstruoTree_H_
#define _ConstruoTree_H_

#include <construo/SphereCoverage.h>

#include <crustacore/GlobeFile.h>
#include <crustacore/Scope.h>
#include <crustacore/TileIndex.h>
#include <crustacore/TreeIndex.h>


namespace crusta {


template <typename PixelParam>
class TreeNode;

/**\todo having to specialize this helper breaks the whole separation into
traits that deal with the specifics of the PixelParam. Fix this eventually.
This had to be done to avoid a cyclic include where Tree includes GlobeData that
defines Tileheader::reset(TreeNode) */
template <typename PixelParam>
typename GlobeData<PixelParam>::TileHeader
TreeNodeCreateTileHeader(const TreeNode<PixelParam>& node);

/** Data elements providing the basis for a quadtree structure where elements
    are dynamically allocated and interconnected through pointers */
template <typename PixelParam>
class TreeNode
{
friend typename GlobeData<PixelParam>::TileHeader
TreeNodeCreateTileHeader<>(const TreeNode<PixelParam>& node);

public:
    typedef GlobeData<PixelParam>   gd;
    typedef typename gd::File       File;
    typedef typename gd::TileHeader TileHeader;

    TreeNode();
    virtual ~TreeNode();

    /** get the tile header appropriate for this node */
    TileHeader getTileHeader();

///\todo generalize this so that requests can be made for arbitrary levels
    /** query a kin of the node. Since the trees are created on demand,
        valid parts of the tree may not be represented in memory during neighbor
        traversal. By default such parts will be loaded from file if possible,
        but the behaviour can be altered through 'loadMissing'. */
    bool getKin(TreeNode<PixelParam>*& kin, int offsets[2],
                bool loadMissing=true, int down=0, size_t* orientation=NULL);

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
    TreeNode<PixelParam>* parent;
    /** pointer to the first child in the set of four children. Children must be
        allocated as a continuous range in memory */
    TreeNode<PixelParam>* children;

//- Construo node data
    static GlobeFile<PixelParam>* globeFile;

    TreeIndex treeIndex;
    TileIndex tileIndex;

    Scope scope;
    SphereCoverage coverage;
    Scope::Scalar resolution;

    typename PixelParam::Type* data;

    bool mustBeUpdated;

protected:
    bool isExplicitNeighborNode;

///\todo remove
#if CRUSTA_ENABLE_DEBUG
public:
static bool debugGetKin;
#endif //CRUSTA_ENABLE_DEBUG
};



template <typename PixelParam>
class ExplicitNeighborNode : public TreeNode<PixelParam>
{
public:
    ExplicitNeighborNode();
    void setNeighbors(ExplicitNeighborNode* nodes[4],
                      const size_t orientations[4]);

    ///pointers to the neighboring nodes
    ExplicitNeighborNode* neighbors[4];
    ///orientations of the neighboring nodes
    size_t orientations[4];
};

template <typename PixelParam>
class Spheroid
{
public:
    typedef ExplicitNeighborNode<PixelParam> BaseNode;
    typedef std::vector<BaseNode>            BaseNodes;

    Spheroid(const std::string& baseName, const size_t tileResolution[2]);

    BaseNodes             baseNodes;
    GlobeFile<PixelParam> globeFile;
};

} //namespace crusta

#ifndef _ConstruoTree_nohpp_
#include <construo/Tree.hpp>
#endif

#endif //_ConstruoTree_H_
