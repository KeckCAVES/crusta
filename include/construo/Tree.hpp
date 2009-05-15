#include <sstream>

#include <construo/QuadtreeFileHeaders.h>

BEGIN_CRUSTA

template <typename PixelParam>
TreeNode<PixelParam>::
TreeNode() :
    parent(NULL), children(NULL), treeState(NULL),
    tileIndex(State::File::INVALID_TILEINDEX),
    data(NULL), mustBeUpdated(false), isExplicitNeighborNode(false)
{}

template <typename PixelParam>
TreeNode<PixelParam>::
~TreeNode()
{
    delete[] children;
    delete[] data;
}

inline void
mult(int p[2], int m)
{
    p[0] *= m;
    p[1] *= m;
}
inline void
translate(int p[2], int tx, int ty)
{
    p[0] += tx;
    p[1] += ty;
}
inline void
translate(int p[2], int t[2])
{
    p[0] += t[0];
    p[1] += t[1];
}
inline void
rotate(int p[2], uint o, const int rotations[4][2][2])
{
    int r[2];
    r[0] = p[0]*rotations[o][0][0] + p[1]*rotations[o][0][1];
    r[1] = p[0]*rotations[o][1][0] + p[1]*rotations[o][1][1];
    p[0] = r[0];
    p[1] = r[1];
}

template <typename PixelParam>
bool TreeNode<PixelParam>::
getKin(TreeNode*& kin, int offsets[2], bool loadMissing)
{
    kin              = this;
    int nodeSize     = 1;
    int nodeCoord[2] = {0, 0};
    int kinCoord[2]  = {offsets[0], offsets[1]};

//- walk up the tree until a parent node containing the requested location
    /* the kinCoord are updated to always be relative to the lower-left corner
       of the current node */
    while (kinCoord[0]<0 || kinCoord[0]>=nodeSize ||
           kinCoord[1]<0 || kinCoord[1]>=nodeSize)
    {
        //in case we have explicit neighgors we can move sideways directly
        if (kin->isExplicitNeighborNode)
        {
            //neighbor id depending on horizontal/vertical and neg/pos direction
            static const uint neighborIds[2][2] = { {1, 3}, {2, 0} };

            //determine the most important direction and corresponding neighbor
            uint horizontal = abs(kinCoord[0])>abs(kinCoord[1]) ? 0 : 1;
            uint direction  = kinCoord[horizontal]<0 ? 0 : 1;
            uint neighbor   = neighborIds[horizontal][direction];

            kin = neighbors[neighbor];
/**\todo I could update the code here to account for a situation that won't
         occur where the neighbor doesn't exist and potentially the other way
         (e.g. top then left instead of left then top) might succeed */
        //- transform the kinCoords into the new neighbor's frame
            //orientation specific rotation matrices
            static const int rotations[4][2][2] = {
                {{ 1, 0}, {0, 1}},   {{0, 1}, {-1, 0}},
                {{-1, 0}, {0,-1}},   {{0,-1}, { 1, 0}},
            }
            //undo the rotation of the kinCoord
            rotate(kinCoord, orientations[neighbor], rotations);

            //determine the shift of the new origin and apply it
            //move to node center
            int shift[2] = {-1, -1};
            //undo the rotation on the origin
            rotate(shift, orientations[neighbor], rotations);
            /* move back to "old" origin (lower-left). Shift is the unrotated
               origin for the given orientation. */
            translate(shift, 1, 1);
            /* we still need to move that origin to the neighbor. Start with
               a shift upwards */
            int dir[2] = {0, 1};
            //unrotate it to the proper direction
            rotate(dir, orientations[neighbor], rotations);
            //moving to the neighbor is shifting in dir for 2 cells
            mult(dir, 2);
            /* composite the shift to the neighbor with the one due to origin
               rotation */
            translate(shift, dir);
            //finally apply the shift to the kinCoords
            translate(kinCoord, shift);
        }
        //traverse to parent updating the requested location to the parent space
        else
        {
            //if we have reached to root, there's nowhere to go. Fail.
            if (kin->parent == NULL)
            {
                kin = NULL;
                return false;
            }

            //adjust the coordinates given the child index inside its parent
            nodeCoord[0] += kin->treeIndex.child&0x01 ? nodeSize : 0;
            nodeCoord[1] += kin->treeIndex.child&0x10 ? nodeSize : 0;
            kinCoord[0]   = nodeCoord[0] + offsets[0];
            kinCoord[1]   = nodeCoord[1] + offsets[1];

            //move to the parent (space)
            kin        = kin->parent;
            nodeSize <<= 1;
        }
    }

//- walk down the tree honing in on the requested kin
    //use offsets to store the kinCoords now so that these can be returned
    offsets[0] = kinCoord[0];
    offsets[1] = kinCoord[1];
    while (nodeSize > 1)
    {
        //in case we reach a leaf either load missing nodes or end
        if (kin->children==NULL && loadMissing)
            kin->loadMissingChildren();
        if (kin->children == NULL)
            return false;

        //switch to the child space
        nodeSize >>= 1;
        /* determine the child node to proceed with by comparing the offsets
           to the node center (nodeSize, nodeSize) */
        uint childIndex = 0x0;
        for (uint i=0; i<2; ++i)
        {
            if (offsets[1] >= nodeSize)
            {
                childIndex |= (1<<i);
                offsets[i] -= nodeSize;
            }
        }
        //move on to the child node
        kin = &kin->children[childIndex];
    }

    return true;
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
    static const uint sibling[4][8] = {
        {      2,INVALID,INVALID,      1,INVALID,INVALID,INVALID,      3},//BL
        {      3,      0,INVALID,INVALID,      2,INVALID,INVALID,INVALID},//BR
        {INVALID,INVALID,      0,      3,INVALID,INVALID,      1,INVALID},//TL
        {INVALID,      2,      1,INVALID,INVALID,      0,INVALID,INVALID} //TR
    };

    uint siblingIndex = sibling[treeIndex.child][neighborId];
    if (siblingIndex != INVALID)
    {
        assert(parent->children!=NULL && "parent node not linked to child");
        neighbor = parent->children[siblingIndex];
        return neighbor!=NULL ? true : false;
    }

//- recurse to a child of the parent's neighbor to determine the neighbor
    //get the parent's neighbor on the same side as our request
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
    isExplicitNeighborNode = true;
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
    assert(neighborId>=TOP && neighborId<=TOP_RIGHT &&"neighbor out of bounds");

    if (neighborId <= RIGHT)
    {
        neighbor    = neighbors[neighborId];
        orientation = orientations[neighborId];
        return neighbor!=NULL ? true : false;
    }
    else
    {
        neighborId -= TOP_LEFT;
        static const uint diagonals[4][2] = {
            {TOP, LEFT}, {BOTTOM, LEFT}, {BOTTOM, RIGHT}, {TOP, RIGHT}
        };
        for (uint i=0; i<2; ++i)
        {
            uint n = diagonals[neighborId][i];
            if (neighbors[n] != NULL)
            {
                uint an = diagonals[neighborId][1-i]  + 4 - orientations[n];
                if (neighbors[n]->getNeighbor(an, neighbor, orientation,
                                               loadMissing))
                {
                    orientation += 4 - orientations[n];
                    return true;
                }
                else
                    return false;
            }
        }
        neighbor = NULL;
        return false;
    }
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
            //the new root must have index 0
            node->tileIndex = file->appendTile();
            assert(node->tileIndex==0 && file->getNumTiles()==1);
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
