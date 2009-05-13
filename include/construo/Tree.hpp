#include <sstream>

#include <construo/QuadtreeFileHeaders.h>

BEGIN_CRUSTA

template <typename PixelParam>
TreeNode<PixelParam>::
TreeNode() :
    parent(NULL), children(NULL), treeState(NULL),
    tileIndex(State::File::INVALID_TILEINDEX),
    data(NULL), isNew(false), mustBeUpdated(false)
{}

template <typename PixelParam>
TreeNode<PixelParam>::
~TreeNode()
{
    delete[] children;
    delete[] data;
}

template <typename PixelParam>
bool TreeNode<PixelParam>::
getNeighbor(uint neighborId, TreeNode*& neighbor, uint& orientation,
            bool loadMissing)
{
    /* initialize the orientation to TOP since we might not call getNeighbor
       recursively. This is ok because the orientation can only be changed if
       we move between the base patches, which is possible once. No further
       recursive calls are made at that point, such that we've already passed
       this assignment in all recursive calls */
    orientation = TOP;

    /* if we are root and we're not an ExplicitNeighborNode then we can't have
       a neighbor */
    if (parent == NULL)
    {
        neighbor = NULL;
        return false;
    }

//- siblings are the easy case as we can get them directly from the parent
    static const uint INVALID = ~0x0;
    /* which sibling to pick is dependent on which child this node is and the
       requested neighbor */
    static const uint sibling[4][4] = {
        {      2, INVALID, INVALID,       1}, //child 0 (lower-left)
        {      3,       0, INVALID, INVALID}, //child 1 (lower-right)
        {INVALID, INVALID,       0,       3}, //child 2 (top-left)
        {INVALID,       2,       1, INVALID}  //child 3 (top-right)
    };

    uint siblingIndex = sibling[treeIndex.child][neighborId];
    if (siblingIndex != INVALID)
    {
        assert(parent->children!=NULL && "parent node not linked to child");
        neighbor = parent->children[siblingIndex];
        return neighbor!=NULL ? true : false;
    }

//- recurse to a child of the parent's neighbor to determine the neighbor
    //get the parent's neighbor on the same side as ours
    TreeNode* parentNeighbor = NULL;
    if (!parent->getNeighbor(neighborId, parentNeighbor, orientation,
                             loadMissing))
    {
        neighbor = parentNeighbor;
        return false;
    }
    //make sure the children we want to access have been loaded from file
    if (parentNeighbor->children==NULL && loadMissing)
        parentNeighbor->loadMissingChildren();
    //if children still aren't available then we are done here
    if (parentNeighbor->children == NULL)
    {
        neighbor = parentNeighbor;
        return false;
    }

    /* which child to pick is dependent on which child this node is, the
       requested neighbor and the orientation of the parent's neighbor.
       INVALID denotes cases that shouldn't occur as they would be sibling
       nodes and processed already above */
    static const uint neighborChild[4][4][4] = {
        //child 0 (lower-left)
        {
            //TOP      LEFT    BOTTOM    RIGHT    //orientation
            {INVALID, INVALID, INVALID, INVALID}, //request TOP
            {      1,       0,       2,       3}, //request LEFT
            {      2,       3,       1,       0}, //request BOTTOM
            {INVALID, INVALID, INVALID, INVALID}  //request RIGHT
        },
        //child 1 (lower-right)
        {
            //TOP      LEFT    BOTTOM    RIGHT    //orientation
            {INVALID, INVALID, INVALID, INVALID}, //request TOP
            {INVALID, INVALID, INVALID, INVALID}, //request LEFT
            {      3,       1,       0,       2}, //request BOTTOM
            {      0,       2,       3,       1}  //request RIGHT
        },
        //child 2 (uppper-left)
        {
            //TOP      LEFT    BOTTOM    RIGHT    //orientation
            {      0,       2,       3,       1}, //request TOP
            {      3,       1,       0,       2}, //request LEFT
            {INVALID, INVALID, INVALID, INVALID}, //request BOTTOM
            {INVALID, INVALID, INVALID, INVALID}  //request RIGHT
        },
        //child 3 (upper-right)
        {
            //TOP      LEFT    BOTTOM    RIGHT    //orientation
            {      1,       0,       2,       3}, //request TOP
            {INVALID, INVALID, INVALID, INVALID}, //request LEFT
            {INVALID, INVALID, INVALID, INVALID}, //request BOTTOM
            {      2,       3,       1,       0}  //request RIGHT
        }
    };

    //determine the proper child that is the requested neighbor and return
    uint childIndex = neighborChild[treeIndex.child][neighborId][orientation];
    assert(childIndex != INVALID && "sibling request not caught");
    neighbor = parentNeighbor->children[childIndex];
    return neighbor!=NULL ? true : false;
}

template <typename PixelParam>
void TreeNode<PixelParam>::
createChildren()
{
    //compute the child scopes
    Scope childScopes[4];
    scope.split(childScopes);

    //allocate and initialize the children
    children = new TreeNode[4];
    for (uint i=0; i<4; ++i)
    {
        TreeNode& child = children[i];
        child.parent    = this;
        child.treeState = treeState;
        child.treeIndex = treeIndex.down(i);
        child.scope     = childScopes[i];
        coverage        = SphereCoverage(child.scope);

        child.computeResolution();
    }
}

template <typename PixelParam>
void TreeNode<PixelParam>::
loadMissingChildren()
{
    //read the children's tile indices from the file
    assert(treeState!=NULL && treeState->file!=NULL && "uninitialized state");
    State::File::TileIndex childIndices[4];
    if (!readTile(tileIndex, childIndices))
        return;

    //create the new in-memory representations
    createChildren();
    //assign the child tile indices
    for (uint i=0; i<4; ++i)
        children[i].tileIndex = childIndices[i];
}

static Scope::Vertex
mid(const Scope::Vertex& one, const Scope::Vertex& two,
    const Scope::Scalar& radius)
{
    Scope::Vertex mid;
    Scope::Scalar len = Scope::Scalar(0);
    for (uint i=0; i<3; ++i)
    {
        mid[i] = (one[i] + two[i]) * Scope::Scalar(0.5);
        len   += mid[i] * mid[i];
    }
    len = radius / sqrt(len);
    for (uint i=0; i<3; ++i)
        mid[i] *= len;

    return mid;
}

template <typename PixelParam>
void TreeNode<PixelParam>::
computeResolution()
{
    Scope::Scalar radius = scope.getRadius();
    Scope::Vertex one = scope.corners[0];
    Scope::Vertex two = mid(one, scope.corners[1], radius);
    for (uint i=2; i+1<TILE_RESOLUTION; i<<=1)
        one = mid(one, two, radius);

    Scope::Scalar len = Scope::Scalar(0);
    for (uint i=0; i<3; ++i)
    {
        Scope::Scalar diff = two[i] - one[i];
        len += diff * diff;
    }
    resolution = sqrt(len);
}

template <typename PixelParam>
ExplicitNeighborNode<PixelParam>::
ExplicitNeighborNode()
{
    for (uint i=0; i<4; ++i)
    {
        neighbors[i]    = NULL;
        orientations[i] = TreeNode<PixelParam>::TOP;
    }
}

template <typename PixelParam>
void ExplicitNeighborNode<PixelParam>::
setNeighbors(ExplicitNeighborNode* nodes[4], const uint orients[4])
{
    for (uint i=0; i<4; ++i)
    {
        neighbors[i]    = nodes[i];
        orientations[i] = orients[i];
    }
}

template <typename PixelParam>
bool ExplicitNeighborNode<PixelParam>::
getNeighbor(uint neighborId, TreeNode<PixelParam>*& neighbor, uint& orientation,
            bool loadMissing)
{
    assert(neighborId>=0 && neighborId<=4 && "neighbor out of bounds");
    neighbor    = neighbors[neighborId];
    orientation = orientations[neighborId];

    return neighbors[neighborId]!=NULL ? true : false;
}


template <typename PixelParam, typename PolyhedronParam>
Spheroid<PixelParam, PolyhedronParam>::
Spheroid(const std::string& baseName, const uint tileResolution[2])
{
    PolyhedronParam polyhedron;
    uint numPatches = polyhedron.getNumPatches();

    baseNodes.resize(numPatches);
    baseStates.resize(numPatches);

    //create the base nodes and open the corresponding quadtree file
    for (uint i=0; i<numPatches; ++i)
    {
        //create the base node
        BaseNode* node  = &baseNodes[i];
        node->treeState = &baseStates[i];
        //the root must have index 0
        node->treeIndex = TreeIndex(i);
        node->tileIndex = 0;

        //open/create the quadtreefile
        std::ostringstream oss;
        oss << baseName << "_" << i << ".qtf";
        TreeFile*& file = baseStates[i].file;
        file = new TreeFile(oss.str().c_str(), tileResolution);

        //make sure the quadtree file has at least a root
        if (file->getNumTiles() == 0)
        {
            //prepare a "blank region"
            uint numPixels = tileResolution[0]*tileResolution[1];
            node->data = new PixelParam[numPixels];
            for (PixelParam* cur=node->data; cur<(node->data+numPixels); ++cur)
                *cur = QuadtreeTileHeader<PixelParam>::defaultPixelValue;
            //prepare corresponding header
            QuadtreeTileHeader<PixelParam> tileHeader;
            tileHeader.reset(node);

            //the new root must have index 0
            node->tileIndex = file->appendTile();
            assert(node->tileIndex==0 && file->getNumTiles()==1);

            file->writeTile(node->tileIndex, tileHeader, node->data);
            
            //get rid of the temporary data
            delete[] node->data;
            node->data = NULL;
        }
    }
    
    //initialize the geometry of the base nodes and link them
    uint orientations[4];
    BaseNode* neighbors[4];
    typename PolyhedronParam::Connectivity connectivity[4];
    for (uint i=0; i<numPatches; ++i)
    {
        BaseNode* node = &baseNodes[i];
        node->scope    = polyhedron.getScope(i);
        node->coverage = SphereCoverage(node->scope);
        node->computeResolution();
        
        polyhedron.getConnectivity(i, connectivity);
        for (uint n=0; n<4; ++n)
        {
            neighbors[n]    = &baseNodes[connectivity[n][0]];
            orientations[n] = connectivity[n][1];
            node->setNeighbors(neighbors, orientations);
        }
    }
}

template <typename PixelParam, typename PolyhedronParam>
Spheroid<PixelParam, PolyhedronParam>::
~Spheroid()
{
    for (typename BaseStates::iterator it=baseStates.begin();
         it!=baseStates.end(); ++it)
    {
        delete it->file;
    }
}

END_CRUSTA
