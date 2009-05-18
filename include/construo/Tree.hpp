#include <sstream>

#include <crusta/QuadtreeFileHeaders.h>

///\todo remove
#include <construo/Visualizer.h>
#include <iostream>

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
getKin(TreeNode*& kin, int offsets[2], bool loadMissing, int down)
{
    kin              = this;
    int nodeSize     = pow(2, down);
    int nodeCoord[2] = {0, 0};
    int kinCoord[2]  = {offsets[0], offsets[1]};

if (debugGetKin) {
Visualizer::show();
Visualizer::clear();
Visualizer::Floats target;
target.resize(3);
Scope::Vertex v0(kin->scope.corners[1][0] - kin->scope.corners[0][0],
                 kin->scope.corners[1][1] - kin->scope.corners[0][1],
                 kin->scope.corners[1][2] - kin->scope.corners[0][2]);
Scope::Vertex v1(kin->scope.corners[2][0] - kin->scope.corners[0][0],
                 kin->scope.corners[2][1] - kin->scope.corners[0][1],
                 kin->scope.corners[2][2] - kin->scope.corners[0][2]);
double s = 1.0 / nodeSize;
Scope::Vertex t(
kin->scope.corners[0][0] + s*(0.5+offsets[0])*v0[0] + s*(0.5+offsets[1])*v1[0],
kin->scope.corners[0][1] + s*(0.5+offsets[0])*v0[1] + s*(0.5+offsets[1])*v1[1],
kin->scope.corners[0][2] + s*(0.5+offsets[0])*v0[2] + s*(0.5+offsets[1])*v1[2]);
double norm = 1.0 / sqrt(t[0]*t[0] + t[1]*t[1] + t[2]*t[2]);
t[0] *= norm; t[1] *= norm; t[2] *= norm;
Point tt(atan2(t[1], t[0]), acos(t[2]));
target[0] = tt[0];
target[1] = 0.0;
target[2] = tt[1];
Visualizer::addPrimitive(GL_POINTS, target);
} //DEBUG_GETKIN

//- walk up the tree until a parent node containing the requested location
    /* the kinCoord are updated to always be relative to the lower-left corner
       of the current node */
    while (kinCoord[0]<0 || kinCoord[0]>=nodeSize ||
           kinCoord[1]<0 || kinCoord[1]>=nodeSize)
    {
if (debugGetKin) {
Visualizer::addPrimitive(GL_LINES, kin->coverage);
Visualizer::show();
} //DEBUG_GETKIN
        //in case we have explicit neighgors we can move sideways directly
        if (kin->isExplicitNeighborNode)
        {
            ExplicitNeighborNode<PixelParam>* self =
                reinterpret_cast<ExplicitNeighborNode<PixelParam>*>(kin);
            //neighbor id depending on horizontal/vertical and neg/pos direction
            static const uint neighborIds[2][2] = { {1, 3}, {2, 0} };

            //determine the most important direction and corresponding neighbor
            uint horizontal = abs(kinCoord[0])>abs(kinCoord[1]) ? 0 : 1;
            if (kinCoord[horizontal]>=0 && kinCoord[horizontal]<nodeSize)
                horizontal = 1 - horizontal;
            uint direction  = kinCoord[horizontal]<0 ? 0 : 1;
            uint neighbor   = neighborIds[horizontal][direction];

            kin = self->neighbors[neighbor];
            assert(kin != NULL);
/**\todo I could update the code here to account for a situation that won't
         occur where the neighbor doesn't exist and potentially the other way
         (e.g. top then left instead of left then top) might succeed */
        //- transform the kinCoords into the new neighbor's frame
            //orientation specific rotation matrices
            static const int rotations[4][2][2] = {
                {{ 1, 0}, {0, 1}},   {{0, 1}, {-1, 0}},
                {{-1, 0}, {0,-1}},   {{0,-1}, { 1, 0}},
            };

            uint orientation = self->orientations[neighbor];
            //undo the rotation of the kinCoord
            rotate(kinCoord, orientation, rotations);

            //determine the shift of the new origin and apply it
            int originShifts[4][2] = {
                {0,0}, {nodeSize-1,0}, {nodeSize-1,nodeSize-1}, {0,nodeSize-1}
            };
            int shift[2] = {-originShifts[orientation][0],
                            -originShifts[orientation][1] };

            //determine the shift for moving into the neighbor
            int neighborShifts[4][2] = {
                {0,nodeSize}, {-nodeSize,0}, {0,-nodeSize}, {nodeSize,0}
            };
            //moving to the neighbor is shifting in dir for 2 cells
            int dir[2] = {-neighborShifts[neighbor][0],
                          -neighborShifts[neighbor][1] };
            /* composite the shift to the neighbor with the one due to origin
               rotation */
            translate(shift, dir);
            //unrotate the shift to the orientated neighbor
            rotate(shift, orientation, rotations);

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
            nodeCoord[0] += kin->treeIndex.child&0x1 ? nodeSize : 0;
            nodeCoord[1] += kin->treeIndex.child&0x2 ? nodeSize : 0;
            kinCoord[0]   = nodeCoord[0] + offsets[0];
            kinCoord[1]   = nodeCoord[1] + offsets[1];

            //move to the parent (space)
            kin        = kin->parent;
            nodeSize <<= 1;
        }
    }

//- walk down the tree honing in on the requested kin
if (debugGetKin) {
Visualizer::addPrimitive(GL_LINES, kin->coverage);
Visualizer::show();
} //DEBUG_GETKIN
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
            if (offsets[i] >= nodeSize)
            {
                childIndex |= (1<<i);
                offsets[i] -= nodeSize;
            }
        }
        //move on to the child node
        kin = &kin->children[childIndex];
if (debugGetKin) {
Visualizer::addPrimitive(GL_LINES, kin->coverage);
Visualizer::show();
} //DEBUG_GETKIN
    }

    return true;
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
///\todo parametrize the TreeNode by the coverage type?
        child.coverage  = StaticSphereCoverage(2, child.scope);

        child.computeResolution();
    }
}

template <typename PixelParam>
void TreeNode<PixelParam>::
loadMissingChildren()
{
    /* read the children's tile indices from the file. Only allocate the
       children if valid counterparts exist in the file */
    assert(treeState!=NULL && treeState->file!=NULL && "uninitialized state");
    typename State::File::TileIndex childIndices[4];
    if (!treeState->file->readTile(tileIndex, childIndices))
        return;
    if (childIndices[0] == State::File::INVALID_TILEINDEX)
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
    TreeNode<PixelParam>::isExplicitNeighborNode = true;
    for (uint i=0; i<4; ++i)
    {
        neighbors[i]    = NULL;
        orientations[i] = 0; //corresponds to facing up
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

template <typename PixelParam, typename PolyhedronParam>
Spheroid<PixelParam, PolyhedronParam>::
Spheroid(const std::string& baseName, const uint tileResolution[2])
{
    PolyhedronParam polyhedron(SPHEROID_RADIUS);
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
///\todo parametrize the Spheroid by the sphere coverage. use Static for now.
        node->coverage = StaticSphereCoverage(2, node->scope);
        node->computeResolution();
        
        polyhedron.getConnectivity(i, connectivity);
        for (uint n=0; n<4; ++n)
        {
            neighbors[n]    = &baseNodes[connectivity[n][0]];
            orientations[n] = connectivity[n][1];
            node->setNeighbors(neighbors, orientations);
        }
    }

///\todo remove
#if 0
int numVerts = numPatches*16*2*3;
float* verts = new float[numVerts];
float* curVert = verts;
#if 1
    for (uint i=0; i<numPatches; ++i)
    {
        const SphereCoverage::Points& scv = baseNodes[i].coverage.getVertices();
        uint num = (uint)scv.size();
        for (uint j=0; j<num; ++j)
        {
            for (uint k=0; k<2; ++k)
            {
                *curVert = scv[(j+k)%num][0]; ++curVert;
                *curVert =               0.0; ++curVert;
                *curVert = scv[(j+k)%num][1]; ++curVert;
            }
        }
        Visualizer::show(GL_POINTS, (i+1)*16*2*3, verts);
    }
#else
for (uint i=0; i<numPatches; ++i)
{
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[0][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[1][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[1][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[3][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[3][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[2][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[2][k];
    for (uint k=0; k<3; ++k, ++curVert)
        *curVert = baseNodes[i].scope.corners[0][k];
}
#endif
Visualizer::show(GL_LINES, numVerts, verts);
#endif
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
