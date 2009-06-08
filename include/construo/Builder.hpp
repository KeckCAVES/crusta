///\todo fix GPL


#include <construo/ImageFileLoader.h>
#include <construo/ImagePatch.h>

#include <construo/PixelHelpers.h>

///\todo remove
#define DEBUG_PREPARESUBSAMPLINGDOMAIN 0
#define DEBUG_SOURCEFINEST 0
#define DEBUG_FLAGANCESTORSFORUPDATE 0
#if DEBUG_FLAGANCESTORSFORUPDATE
static float flagAncestorsForUpdateColor[3];
#endif //DEBUG_FLAGANCESTORSFORUPDATE
#include <construo/Visualizer.h>

BEGIN_CRUSTA

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
Builder(const std::string& spheroidName, const uint size[2])
{
///\todo Frak this is retarded. Reason so far is the getRefinement from scope
assert(size[0]==size[1]);

    tileSize[0] = size[0];
    tileSize[1] = size[1];

    globe = new Globe(spheroidName, tileSize);

///\todo for now hardcode the subsampling filter
//    Filter::makePointFilter(subsamplingFilter);
    Filter::makeTriangleFilter(subsamplingFilter);
//    Filter::makeFiveLobeLanczosFilter(subsamplingFilter);

    scopeBuf          = new Scope::Scalar[tileSize[0]*tileSize[1]*3];
    sampleBuf         = new Point[tileSize[0]*tileSize[1]];
    nodeDataBuf       = new PixelParam[tileSize[0]*tileSize[1]];
    nodeDataSampleBuf = new PixelParam[tileSize[0]*tileSize[1]];
    //the -3 takes into account the shared edges of the tiles
    domainSize[0] = 4*tileSize[0] - 3;
    domainSize[1] = 4*tileSize[1] - 3;
    domainBuf     = new PixelParam[domainSize[0]*domainSize[1]];
}

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
~Builder()
{
    delete globe;
    for (typename PatchPtrs::iterator it=patches.begin(); it!=patches.end();
         ++it)
    {
        delete *it;
    }
    delete[] scopeBuf;
    delete[] sampleBuf;
    delete[] nodeDataBuf;
    delete[] nodeDataSampleBuf;
    delete[] domainBuf;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
subsampleChildren(Node* node)
{
    assert(node->children != NULL);
    //read in the node's existing data from file
    node->treeState->file->readTile(node->tileIndex, nodeDataSampleBuf);

    static const int offsets[4] = {
        0, (tileSize[0]-1)>>1, ((tileSize[1]-1)>>1)*tileSize[0],
        ((tileSize[1]-1)>>1)*tileSize[0] + ((tileSize[0]-1)>>1) };
    int halfSize[2] = { (tileSize[0]+1)>>1, (tileSize[1]+1)>>1 };
    for (int i=0; i<4; ++i)
    {
        for (int y=0; y<halfSize[1]; ++y)
        {
            PixelParam* wbase = nodeDataBuf + y*2*tileSize[0];
            PixelParam* rbase = nodeDataSampleBuf + y*tileSize[0] + offsets[i];
            for (int x=0; x<halfSize[0]; ++x, wbase+=2, ++rbase)
            {
                wbase[0] = rbase[0];
                if (x<halfSize[0]-1)
                    wbase[1] = pixelAvg(rbase[0], rbase[1]);
                if (y<halfSize[1]-1)
                    wbase[tileSize[0]] = pixelAvg(rbase[0], rbase[tileSize[0]]);
                if (x<halfSize[0]-1 && y<halfSize[1]-1)
                {
                    wbase[tileSize[0]+1] = pixelAvg(rbase[0], rbase[1],
                        rbase[tileSize[0]], rbase[tileSize[0]+1]);
                }
            }
        }
        //write the subsampled data to the child
        Node& child = node->children[i];
        child.treeState->file->writeTile(child.tileIndex, nodeDataBuf);
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
refine(Node* node)
{
    //try to load the missing children from the quadtree file
    if (node->children == NULL)
        node->loadMissingChildren();

    //if loading failed then we just have to create new children
    if (node->children == NULL)
    {
        node->createChildren();
        typename Node::State::File::TileIndex childIndices[4];
        for (uint i=0; i<4; ++i)
        {
            Node* child      = &node->children[i];
            child->tileIndex = child->treeState->file->appendTile();
            assert(child->tileIndex!=Node::State::File::INVALID_TILEINDEX);
///\todo should I dump default-value-initialized to file here?
            childIndices[i]  = child->tileIndex;
        }
        subsampleChildren(node);
        //link the parent with the new children in the file
        node->treeState->file->writeTile(node->tileIndex, childIndices);
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
flagAncestorsForUpdate(Node* node)
{
    while (node!=NULL && !node->mustBeUpdated)
    {
#if DEBUG_FLAGANCESTORSFORUPDATE
Visualizer::addPrimitive(GL_LINES, node->coverage, flagAncestorsForUpdateColor);
Visualizer::show();
#endif //DEBUG_FLAGANCESTORSFORUPDATE
        node->mustBeUpdated = true;
        node = node->parent;
    }
}

struct ImgBox
{
    typedef std::vector<int> Indices;

    ImgBox()
    {
        min[0] = min[1] =  HUGE_VAL;
        max[0] = max[1] = -HUGE_VAL;
    }
    bool add(int index, const Point& p, const Point::Scalar allowed[2])
    {
        //tentatively expand the box to accommodate the new point
        Point newMin, newMax;
        newMin[0] = std::min(min[0], p[0]);
        newMin[1] = std::min(min[1], p[1]);
        newMax[0] = std::max(max[0], p[0]);
        newMax[1] = std::max(max[1], p[1]);

        //check that the box doesn't get too bloated
        if ((newMax[0] - newMin[0]) > allowed[0] ||
            (newMax[1] - newMin[1]) > allowed[1])
        {
            return false;
        }

        //add the new point if all is okay
        indices.push_back(index);
        min = newMin;
        max = newMax;
        return true;
    }

    Indices indices;
    Point min;
    Point max;
};
typedef std::vector<ImgBox> ImgBoxes;

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
sourceFinest(Node* node, Patch* imgPatch, uint overlap)
{
#if 1
    ImgBoxes imgBoxes;
    const int* imgSize = imgPatch->image->getSize();
    Point::Scalar allowedBoxSize[2] = { imgSize[0]>>1, imgSize[1]>>1 };

    //transform all the sample points into the image space
    node->scope.getRefinement(tileSize[0], scopeBuf);
    Scope::Scalar* curScope = scopeBuf;

    for (int i=0; i<int(tileSize[0]*tileSize[1]); ++i, curScope+=3)
    {
        Point& p = sampleBuf[i];

        //convert the scope sample to a spherical one
        p = Converter::cartesianToSpherical(
            Scope::Vertex(curScope[0], curScope[1], curScope[2]));
        //transform the sample into the image space
        p = imgPatch->transform->worldToImage(p);

        //make sure the sample is valid
        if (overlap!=SphereCoverage::ISCONTAINED &&
            !imgPatch->imageCoverage->contains(p))
        {
            continue;
        }
assert(p[0]>=0 && p[0]<=imgSize[0]-1 && p[1]>=0 && p[1]<=imgSize[1]-1);

        //insert the sample into an image box
        bool wasInserted = false;
        for (ImgBoxes::iterator it=imgBoxes.begin(); it!=imgBoxes.end(); ++it)
        {
            if (it->add(i, p, allowedBoxSize))
            {
                wasInserted = true;
                break;
            }
        }
        //add the sample to a new box if not already inserted
        if (!wasInserted)
        {
            imgBoxes.push_back(ImgBox());
            imgBoxes.back().add(i, p, allowedBoxSize);
        }
    }

    //prepare the node's data buffer
    node->data = nodeDataBuf;
    node->treeState->file->readTile(node->tileIndex, node->data);

#if 1
    //go through all the image boxes and sample them
    for (ImgBoxes::iterator bIt=imgBoxes.begin(); bIt!=imgBoxes.end(); ++bIt)
    {
        //read the corresponding image piece
        int rectOrigin[2];
        int rectSize[2];
        for (int i=0; i<2; i++)
        {
///\todo abstract the sampler assume bilinear interpolation here
            rectOrigin[i] = static_cast<int>(Math::floor(bIt->min[i]));
            rectSize[i]   = static_cast<int>(Math::ceil (bIt->max[i])) -
                            rectOrigin[i] + 1;
        }
        PixelParam* rectBuffer = new PixelParam[rectSize[0] * rectSize[1]];
        imgPatch->image->readRectangle(rectOrigin, rectSize, rectBuffer);

        //sample the points
        for (ImgBox::Indices::iterator sIt=bIt->indices.begin();
             sIt!=bIt->indices.end(); ++sIt)
        {
            Point& p = sampleBuf[*sIt];
#if 0
            /* nearest neighbor sampling */
            int ip[2];
            for (uint i=0; i<2; ++i)
            {
                double pFloor = Math::floor(p[i] + 0.5f);
                ip[i]         = static_cast<int>(pFloor) - rectOrigin[i];
            }
if (!(ip[0]>=0 && ip[0]<imgSize[0] && ip[0]<rectSize[0] &&
      ip[1]>=0 && ip[1]<imgSize[1] && ip[1]<rectSize[1]))
    std::cout << "frak";
            node->data[*sIt]  = rectBuffer[ip[1]*rectSize[0] + ip[0]];
if (!(*((float*)&node->data[*sIt])>=0.0 &&
      *((float*)&node->data[*sIt])<=9000.0))
    std::cout << "frakawe";
#else
            int ip[2];
            double d[2];
            for(uint i=0; i<2; i++)
            {
                double pFloor = Math::floor(p[i]);
                d[i]          = p[i] - pFloor;
                ip[i]         = static_cast<int>(pFloor) - rectOrigin[i];
                if (ip[i] == rectSize[i]-1)
                {
                    --ip[i];
                    d[i] = 1.0;
                }
            }
            PixelParam* base = rectBuffer + (ip[1]*rectSize[0] + ip[0]);
            PixelParam  one  = (1.0-d[0])*base[0] + d[0]*(base[1]);
            PixelParam  two  = (1.0-d[0])*base[rectSize[0]] +
                               d[0]*base[rectSize[0]+1];
            node->data[*sIt] = (1.0-d[1])*one + d[1]*two;
#endif
#if DEBUG_SOURCEFINEST
Visualizer::Floats scopeSample;
scopeSample.resize(3);
Point wp = imgPatch->transform->imageToWorld(p);
scopeSample[0] = wp[0];
scopeSample[1] =   0.0;
scopeSample[2] = wp[1];
//static const float scopeSampleColor[3] = { 0.2f, 1.0f, 0.1f };
float scopeSampleColor[3] = {
    (float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX };
Visualizer::addPrimitive(GL_POINTS, scopeSample, scopeSampleColor);
Visualizer::show();
#endif //DEBUG_SOURCEFINEST
        }
        //clean up the temporary image uffer
        delete[] rectBuffer;
    }
#endif


#else


    //convert the node's bounding box to the image space
    Box nodeImgBox = imgPatch->transform->worldToImage(
        node->coverage.getBoundingBox());
    //use the box to load an appropriate rectangle from the image file
    int rectOrigin[2];
    int rectSize[2];
    for (int i=0; i<2; i++)
    {
/**\todo find a more robust solution to using the bounding box to grab an image
region for the sampling. For now just arbitrarily widen the region */
        static const int border = 5;
///\todo abstract the sampler
        //assume bilinear interpolation here
        rectOrigin[i] = static_cast<int>(Math::floor(nodeImgBox.min[i]))-border;
        rectSize[i]   = static_cast<int>(Math::ceil(nodeImgBox.max[i])) + 1 -
                                                    rectOrigin[i] + border;
    }
    PixelParam* rectBuffer = new PixelParam[rectSize[0] * rectSize[1]];
    imgPatch->image->readRectangle(rectOrigin, rectSize, rectBuffer);

    //determine the sample positions for the scope
///\todo enhance get Refinement such that is can produce non-uniform samples
    node->scope.getRefinement(tileSize[0], scopeBuf);
///\todo remove
    //prepare the node's data buffer
    node->data = nodeDataBuf;
    node->treeState->file->readTile(node->tileIndex, node->data);

    //resample pixel values for the node from the read rectangle
    Scope::Scalar* endPtr = scopeBuf + tileSize[0]*tileSize[1]*3;
    PixelParam* dataPtr   = node->data;
    for (Scope::Scalar* curPtr=scopeBuf; curPtr!=endPtr; curPtr+=3, ++dataPtr)
    {
        /* transform the sample position into spherical space and then to the
           image patch's image coordinates: */
        Point p = Converter::cartesianToSpherical(
            Scope::Vertex(curPtr[0], curPtr[1], curPtr[2]));
#if DEBUG_SOURCEFINEST
Visualizer::Floats scopeSample;
scopeSample.resize(3);
scopeSample[0] = p[0];
scopeSample[1] =  0.0;
scopeSample[2] = p[1];
static const float scopeSampleColor[3] = { 0.2f, 1.0f, 0.1f };
Visualizer::addPrimitive(GL_POINTS, scopeSample, scopeSampleColor);
Visualizer::show();
#endif //DEBUG_SOURCEFINEST
        p = imgPatch->transform->worldToImage(p);

        /* Check if the point is inside the image patch's coverage region: */
        if(overlap==SphereCoverage::ISCONTAINED ||
           imgPatch->imageCoverage->contains(p))
        {
#if 1
            /* nearest neighbor sampling */
            int ip[2];
            for (uint i=0; i<2; ++i)
            {
                double pFloor = Math::floor(p[i] + 0.5f);
                ip[i]         = static_cast<int>(pFloor) - rectOrigin[i];
            }
            *dataPtr  = rectBuffer[ip[1]*rectSize[0] + ip[0]];
#else
            /* Sample the point: */
            int ip[2];
            double d[2];
            for(uint i=0; i<2; i++)
            {
                double pFloor = Math::floor(p[i]);
                d[i]          = p[i] - pFloor;
                ip[i]         = static_cast<int>(pFloor) - rectOrigin[i];
            }
            PixelParam* basePtr = rectBuffer + (ip[1]*rectSize[0] + ip[0]);
            PixelParam one = (1.0-d[0])*basePtr[0] + d[0]*(basePtr[1]);
            PixelParam two = (1.0-d[0])*basePtr[rectSize[0]] +
                             d[0]*basePtr[rectSize[0]+1];
            *dataPtr  = (1.0-d[1])*one + d[1]*two;
#endif
#if DEBUG_SOURCEFINEST
Point pp = imgPatch->transform->imageToWorld(
    ip[0]+rectOrigin[0], (ip[1]+rectOrigin[1]));
Visualizer::Floats imgSample;
imgSample.resize(3);
imgSample[0] = pp[0];
imgSample[1] =   0.0;
imgSample[2] = pp[1];
float imgSampleColor[3] = {
    (float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX };
Visualizer::addPrimitive(GL_POINTS, imgSample, imgSampleColor);
Visualizer::show();
#endif //DEBUG_SOURCEFINEST
        }
    }

    //clean up the temporary image uffer
    delete[] rectBuffer;
#endif

    //prepare the header
    QuadtreeTileHeader<PixelParam> header;
    header.reset(node);

    //we store elevations as deviations from the average
//    pixel::relativeToAverageElevation(node, header);

    //commit the data to file
    node->treeState->file->writeTile(node->tileIndex, header, node->data);

#if 0
{
static const float color[3] = { 0.2f, 1.0f, 0.1f };
Visualizer::addScopeRefinement(tileSize[0], scopeBuf, color);
Visualizer::show();
}
#endif
    node->data = NULL;

    if (node->parent != NULL)
    {
#if DEBUG_FLAGANCESTORSFORUPDATE
flagAncestorsForUpdateColor[0] = (float)rand() / RAND_MAX;
flagAncestorsForUpdateColor[1] = (float)rand() / RAND_MAX;
flagAncestorsForUpdateColor[2] = (float)rand() / RAND_MAX;
#endif //DEBUG_FLAGANCESTORSFORUPDATE
        //make sure that the parent dependent on this new data is also updated
        flagAncestorsForUpdate(node->parent);

        //we musn't forget to update all the adjacent non-sibling nodes either
/**\todo DANGER: there exist valence 5 corners. Need to account for reaching 2
different nodes on corners depending on which neighbor we traverse first. Need
to adjust getKin and its API to allow this */
        static const int offsets[4][3][2] = {
            { {-1, 0}, {-1,-1}, { 0,-1} }, { { 0,-1}, { 1,-1}, { 1, 0} },
            { {-1, 0}, {-1, 1}, { 0, 1} }, { { 0, 1}, { 1, 1}, { 1, 0} }
        };
        int off[2];
        Node* kin;
        for (uint i=0; i<3; ++i)
        {
            off[0] = offsets[node->treeIndex.child][i][0];
            off[1] = offsets[node->treeIndex.child][i][1];
            node->parent->getKin(kin, off);
            if (kin != NULL)
                flagAncestorsForUpdate(kin);
        }
    }
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
}

template <typename PixelParam, typename PolyhedronParam>
int Builder<PixelParam, PolyhedronParam>::
updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution)
{
///\todo remove
#if DEBUG_SOURCEFINEST || 1
if (node->treeIndex.level<2)
{
static const float covColor[3] = { 0.1f, 0.4f, 0.6f };
Visualizer::addPrimitive(GL_LINES, node->coverage, covColor);
Visualizer::peek();
}
#endif

    //check for an overlap
    uint overlap = node->coverage.overlaps(*(imgPatch->sphereCoverage));
    if (overlap == SphereCoverage::SEPARATE)
        return 0;

    //recurse to children if the resolution of the node is too coarse
    if (node->resolution > imgResolution)
    {
        int depth = 0;
        refine(node);
        for (uint i=0; i<4; ++i)
        {
            depth = std::max(updateFiner(&node->children[i], imgPatch,
                                         imgResolution), depth);
        }
        return depth;
    }

    //this node has the appropriate resolution
//Visualizer::show();
    sourceFinest(node, imgPatch, overlap);
    return node->treeIndex.level;
}

template <typename PixelParam, typename PolyhedronParam>
int Builder<PixelParam, PolyhedronParam>::
updateFinestLevels()
{
    int depth = 0;
    //iterate over all the image patches to source
    int numPatches = static_cast<int>(patches.size());
    for (int i=0; i<numPatches; ++i)
    {
///\todo remove
#if 1
Visualizer::clear();
Visualizer::addPrimitive(GL_LINES, *(patches[i]->sphereCoverage));
Visualizer::peek();
#endif

#if DEBUG_SOURCEFINEST
const int* size = patches[i]->image->getSize();
Visualizer::Floats edges;
edges.resize(size[0]*size[1]*4*2*3);
float* e = &edges[0];
Visualizer::Floats centers;
centers.resize(size[0]*size[1]*3);
float* c = &centers[0];

Point corners[4];
for (int y=0; y<size[1]; ++y)
{
    for (int x=0; x<size[0]; ++x, c+=3)
    {
        corners[0] = patches[i]->transform->imageToWorld(Point(x-0.5, y-0.5));
        corners[1] = patches[i]->transform->imageToWorld(Point(x+0.5, y-0.5));
        corners[2] = patches[i]->transform->imageToWorld(Point(x+0.5, y+0.5));
        corners[3] = patches[i]->transform->imageToWorld(Point(x-0.5, y+0.5));
        Point origin = patches[i]->transform->imageToWorld(Point(x, y));

        for (uint i=0; i<4; ++i, e+=6)
        {
            int end = (i+1)%4;
            e[0] = corners[i][0];   e[1] = 0.0; e[2] = corners[i][1];
            e[3] = corners[end][0]; e[4] = 0.0; e[5] = corners[end][1];
        }

        c[0] = origin[0];
        c[1] = 0.0;
        c[2] = origin[1];
    }
}

static const float edgeColor[3] = { 0.9f, 0.6f, 0.2f };
Visualizer::addPrimitive(GL_LINES, edges, edgeColor);
static const float centerColor[3] = { 0.2f, 0.6f, 0.9f };
Visualizer::addPrimitive(GL_POINTS, centers, centerColor);
Visualizer::show();
#endif //show image pixels

        std::cout << "Adding source image " << i << " out of " << numPatches;
        std::cout.flush();

        //grab the smallest resolution from the image
        Point::Scalar imgResolution=patches[i]->transform->getFinestResolution(
            patches[i]->image->getSize());
        /* exaggerate the image's resolution because our sampling is not aligned
           with the image axis */
        imgResolution *= Point::Scalar(0.5);

        //iterate over all the spheroid's base patches to determine overlap
        for (typename Globe::BaseNodes::iterator bIt=globe->baseNodes.begin();
             bIt!=globe->baseNodes.end(); ++bIt)
        {
            depth = std::max(updateFiner(&(*bIt), patches[i], imgResolution),
                             depth);

            std::cout << ".";
            std::cout.flush();
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(&(*bIt));
//Visualizer::show();
        }
        std::cout << " done" << std::endl;
    }
    std::cout << std::endl;

    return depth;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
prepareSubsamplingDomain(Node* node)
{
#if DEBUG_PREPARESUBSAMPLINGDOMAIN
Visualizer::addPrimitive(GL_LINES, node->coverage);
Visualizer::show();
#endif //DEBUG_PREPARESUBSAMPLINGDOMAIN
    Node* kin     = NULL;

    PixelParam* domain = domainBuf + (tileSize[1]-1)*domainSize[0] +
                         tileSize[0]-1;

    /* specify the order of lower-level nodes manually such that the inner nodes
       are added last and overwrite the edge value (e.g. if one of the
       neighboring nodes does not exist or has no valid values, we want to use
       the inner node ones instead */
    static const int offsets[16][2] = {
        {-1,-1}, { 0,-1}, { 1,-1}, { 2,-1}, {-1, 2}, { 0, 2}, { 1, 2}, { 2, 2},
        {-1, 1}, {-1, 0}, { 2, 1}, { 2, 0}, { 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}
    };
    bool dataIsAvailable[16];
    int domainOff[2];
    int nodeOff[2];
    for (int i=0; i<16; ++i)
    {
    //- retrieve the kin
        nodeOff[0] = domainOff[0] = offsets[i][0];
        nodeOff[1] = domainOff[1] = offsets[i][1];
        uint kinO;
        node->getKin(kin, nodeOff, true, 1, &kinO);
#if DEBUG_PREPARESUBSAMPLINGDOMAIN
const static float scopeRefColor[3] = { 0.3f, 0.8f, 0.3f };
kin->scope.getRefinement(tileSize[0], scopeBuf);
Visualizer::addScopeRefinement(tileSize[0], scopeBuf, scopeRefColor);
Visualizer::show();
#endif //DEBUG_PREPARESUBSAMPLINGDOMAIN

    //- retrieve data as appropriate
        //default to blank data
        const PixelParam* data = node->treeState->file->getBlank();

        //read the data from file
/**\todo disabled reading from neighbors that aren't from the same base patch.
reading of neighbor data with differring orientation is currently absolutely
broken, overwrites random memory regions and breaks fraking everything.
Note: getKin across patches seem to be broken: e.g. offset==3 returned. */
        if (kin != NULL && node->treeIndex.patch==kin->treeIndex.patch)
        {
            /* sampling from the same patch will always have data (even if that
               is "nodata" */
            dataIsAvailable[i] = true;

            assert(kin->tileIndex!=Node::State::File::INVALID_TILEINDEX);
            if (nodeOff[0]==0 && nodeOff[1]==0)
            {
                //we're grabbing data from a same leveled kin
                kin->treeState->file->readTile(kin->tileIndex, nodeDataBuf);
                data = nodeDataBuf;
            }
            else
            {
                //need to sample coarser level node as the kin replacement
                kin->treeState->file->readTile(kin->tileIndex,
                                               nodeDataSampleBuf);
                //determine the resample step size
                double scale = 1;
                for (int i=kin->treeIndex.level;
                     i<node->treeIndex.level+1; ++i)
                {
                    scale *= 0.5;
                }
                double step[2] = {scale/(tileSize[0]-1), scale/(tileSize[1]-1)};
///\todo abstract out the filtering. Currently using bilinear filtering
                double d[2];
                uint ny=0;
                PixelParam* wbase = nodeDataBuf;
                for (double y=nodeOff[1]*scale; ny<tileSize[1];
                     ++ny, y+=step[1])
                {
                    double py = y * (tileSize[1]-1);
                    double fy = floor(py);
                    d[1]      = py - fy;
                    uint iy   = static_cast<uint>(fy);
                    if (iy == tileSize[1]-1)
                    {
                        --iy;
                        d[1] = 1.0;
                    }
                    uint oy = iy * tileSize[0];

                    uint nx=0;
                    for (double x=nodeOff[0]*scale; nx<tileSize[0];
                         ++nx, x+=step[0], ++wbase)
                    {
                        double px = x * (tileSize[0]-1);
                        double fx = floor(px);
                        d[0]      = px - fx;
                        uint ix   = static_cast<uint>(fx);
                        if (ix == tileSize[0]-1)
                        {
                            --ix;
                            d[0] = 1.0;
                        }

                        PixelParam* rbase = nodeDataSampleBuf + oy + ix;

                        PixelParam one = (1.0-d[0]) * rbase[0] +
                                               d[0] * rbase[1];
                        PixelParam two = (1.0-d[0]) * rbase[tileSize[0]] +
                                               d[0] * rbase[tileSize[0]+1];

                        *wbase = (1.0-d[1])*one + d[1]*two;
                    }
                }
                data = nodeDataBuf;
            }
        }
        else
        {
            /* flag that we didn't retrieve any data for this patch because of
               broken sampling across patches */
            dataIsAvailable[i] = kin==NULL;
        }

/**\todo disabled reading from neighbors that don't have the same orientation
reading of neighbor data with differring orientation is currently absolutely
broken, overwrites random memory regions and breaks fraking everything */
kinO = 0;
//no need to copy blank anymore... will instead generate a clamp_to_edge
if (dataIsAvailable[i])
{
    //- insert the data at the appropriate location
        PixelParam* base = domain +
                           domainOff[1] * (tileSize[1]-1) * domainSize[0] +
                           domainOff[0] * (tileSize[0]-1);
        int startY[4] = { 0, tileSize[1]-1, tileSize[1]-1, 0 };
        int stepY[4]  = { tileSize[0], 1,  -tileSize[0], -1 };
        int startX[4] = { 0, 0, tileSize[0]-1, tileSize[0]-1 };
        int stepX[4]  = { 1,  -tileSize[1], -1, tileSize[0]};
        for (uint y=0; y<tileSize[1]; ++y)
        {
            PixelParam* to         = base + y*domainSize[0];
            const PixelParam* from = data + startY[kinO] + y*stepY[kinO] +
                                     startX[kinO];
            for (uint x=0; x<tileSize[0]; ++x, ++to, from+=stepX[kinO])
                *to = *from;
        }
}
    }

/**\todo this is a temporary fix for the really bad seams at patch boundaries.
I'm artificially extending the border data into the neighboring tiles that are
missing data because it's on another patch (clamp_to_edge like) */
domain = domainBuf;
static const int horizontalIndices[4] = {1,2,5,6};
PixelParam* horizontalSources[4] = {
    domain +   (tileSize[1]-1)*domainSize[0] +   (tileSize[0]-1),
    domain +   (tileSize[1]-1)*domainSize[0] + 2*(tileSize[0]-1),
    domain + 3*(tileSize[1]-1)*domainSize[0] +   (tileSize[0]-1),
    domain + 3*(tileSize[1]-1)*domainSize[0] + 2*(tileSize[0]-1),
};
PixelParam* horizontalDestinations[4] = {
    domain +   (tileSize[0]-1),
    domain + 2*(tileSize[0]-1),
    domain + (3*(tileSize[1]-1) + 1)*domainSize[0] +   (tileSize[0]-1),
    domain + (3*(tileSize[1]-1) + 1)*domainSize[0] + 2*(tileSize[0]-1),
};
for (int i=0; i<4; ++i)
{
    if (!dataIsAvailable[horizontalIndices[i]])
    {
        PixelParam* src = horizontalSources[i];
        PixelParam* dst = horizontalDestinations[i];
        for (uint y=0; y<tileSize[1]-1; ++y, dst+=domainSize[0])
            memcpy(dst, src, tileSize[0]*sizeof(PixelParam));
    }
}

static const int verticalIndices[4] = {8,9,10,11};
PixelParam* verticalSources[4] = {
    domain + 2*(tileSize[1]-1)*domainSize[0] +   (tileSize[0]-1),
    domain +   (tileSize[1]-1)*domainSize[0] +   (tileSize[0]-1),
    domain + 2*(tileSize[1]-1)*domainSize[0] + 3*(tileSize[0]-1),
    domain +   (tileSize[1]-1)*domainSize[0] + 3*(tileSize[0]-1),
};
PixelParam* verticalDestinations[4] = {
    domain + 2*(tileSize[1]-1)*domainSize[0],
    domain +   (tileSize[1]-1)*domainSize[0],
    domain + 2*(tileSize[1]-1)*domainSize[0] + 3*(tileSize[0]-1) + 1,
    domain +   (tileSize[1]-1)*domainSize[0] + 3*(tileSize[0]-1) + 1,
};
for (int i=0; i<4; ++i)
{
    if (!dataIsAvailable[verticalIndices[i]])
    {
        PixelParam* src = verticalSources[i];
        for (uint y=0; y<tileSize[1]; ++y, src+=domainSize[0])
        {
            PixelParam* dst = verticalDestinations[i] + y*domainSize[0];
            for (uint x=0; x<tileSize[0]-1; ++x, ++dst)
                *dst = *src;
        }
    }
}

//bottom-left corner
if (!dataIsAvailable[0])
{
    PixelParam* hsrc = domain + (tileSize[1]-1)*domainSize[0];
    PixelParam* dst  = domain;
    for (uint y=0; y<tileSize[1]-1; ++y, dst+=domainSize[0])
        memcpy(dst, hsrc, (y+1)*sizeof(PixelParam));

    PixelParam* vsrc = domain + tileSize[0]-1;
    for (uint y=0; y<tileSize[1]-1; ++y, vsrc+=domainSize[0])
    {
        dst = domain + y*domainSize[0];
        for (uint x=y+1; x<tileSize[0]; ++x)
            dst[x] = *vsrc;
    }
}

//bottom-right corner
if (!dataIsAvailable[3])
{
    PixelParam* hsrc = domain+ (tileSize[1]-1)*domainSize[0]+ 3*(tileSize[0]-1);
    for (uint y=0; y<tileSize[1]-1; ++y)
    {
        PixelParam* dst  = domain + y*domainSize[0] + 3*(tileSize[0]-1) +
                           tileSize[0]-1-y;
        memcpy(dst, &hsrc[tileSize[0]-1-y], (y+1)*sizeof(PixelParam));
    }

    PixelParam* vsrc = domain + 3*(tileSize[0]-1);
    for (uint y=0; y<tileSize[1]-1; ++y, vsrc+=domainSize[0])
    {
        PixelParam* dst = domain + y*domainSize[0] + 3*(tileSize[0]-1);
        for (uint x=1; x<tileSize[0]-1-y; ++x)
            dst[x] = *vsrc;
    }
}

//top-left corner
if (!dataIsAvailable[4])
{
    PixelParam* hsrc = domain + 3*(tileSize[1]-1)*domainSize[0];
    for (uint y=1; y<tileSize[1]-1; ++y)
    {
        PixelParam* dst  = domain + (3*(tileSize[1]-1) + y)*domainSize[0];
        memcpy(dst, hsrc, (tileSize[0]-1-y)*sizeof(PixelParam));
    }

    PixelParam* vsrc = domain + 3*(tileSize[1]-1) + (tileSize[0]-1);
    for (uint y=1; y<tileSize[1]; ++y, vsrc+=domainSize[0])
    {
        PixelParam* dst = domain + (3*(tileSize[1]-1)+y)*domainSize[0];
        for (uint x=tileSize[0]-1-y; x<tileSize[0]-1; ++x)
            dst[x] = *vsrc;
    }
}

//top-right corner
if (!dataIsAvailable[7])
{
    PixelParam* hsrc = domain+3*(tileSize[1]-1)*domainSize[0]+3*(tileSize[0]-1);
    for (uint y=1; y<tileSize[1]-1; ++y)
    {
        PixelParam* dst = domain + (3*(tileSize[1]-1) + y)*domainSize[0] +
                          3*(tileSize[0]-1) + y + 1;
        memcpy(dst, &hsrc[y+1], (tileSize[0]-1-y)*sizeof(PixelParam));
    }

    PixelParam* vsrc = domain+3*(tileSize[1]-1)*domainSize[0]+3*(tileSize[0]-1);
    for (uint y=1; y<tileSize[1]; ++y, vsrc+=domainSize[0])
    {
        PixelParam* dst = domain + (3*(tileSize[1]-1) + y)*domainSize[0] +
                          3*(tileSize[0]-1);;
        for (uint x=0; x<y+1; ++x)
            dst[x] = *vsrc;
    }
}

}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarser(Node* node, int level)
{
#if DEBUG_PREPARESUBSAMPLINGDOMAIN
static const float covColor[3] = { 0.1f, 0.4f, 0.6f };
Visualizer::addPrimitive(GL_LINES, node->coverage, covColor);
Visualizer::peek();
#endif

    //the nodes on the way must be flagged for update
    if (!node->mustBeUpdated)
        return;

//- recurse until we've hit the requested level
    if (node->treeIndex.level!=static_cast<uint>(level) && node->children!=NULL)
    {
        for (uint i=0; i<4; ++i)
            updateCoarser(&(node->children[i]), level);
        return;
    }

//- we've reached a node that must be updated
    prepareSubsamplingDomain(node);

    /* walk the pixels of the node's data and performed filtered look-ups into
       the domain */
    PixelParam* data = nodeDataBuf;
    PixelParam* domain;
    for (domain = domainBuf +   (tileSize[1]-1)*domainSize[0]+  (tileSize[0]-1);
         domain<= domainBuf + 3*(tileSize[1]-1)*domainSize[0]+3*(tileSize[0]-1);
         domain+= 2*domainSize[0] - 2*tileSize[0])
    {
        for (uint i=0; i<tileSize[0]; ++i, domain+=2, ++data)
            *data  = subsamplingFilter.lookup(domain, domainSize[0]);
    }

    //commit the data to file
    node->data = nodeDataBuf;

    QuadtreeTileHeader<PixelParam> header;
    header.reset(node);
    node->treeState->file->writeTile(node->tileIndex, header, node->data);

    node->data = NULL;
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarserLevels(int depth)
{
    if (depth==0)
        return;

    for (int level=depth-1; level>=0; --level)
    {
        std::cout << "Upsampling level " << level;
        std::cout.flush();
        for (typename Globe::BaseNodes::iterator it=globe->baseNodes.begin();
             it!=globe->baseNodes.end(); ++it)
        {
//Visualizer::clear();
//Visualizer::addPrimitive(GL_LINES, it->coverage);
//Visualizer::show();
            //traverse the tree and update the next level
            updateCoarser(&(*it), level);
//verifyQuadtreeFile(&(*it));
            std::cout << ".";
            std::cout.flush();
        }
        std::cout << " done" << std::endl;
    }
    std::cout << std::endl;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
addImagePatch(const std::string& patchName, double pixelScale)
{
    //create a new image patch
    ImagePatch<PixelParam>* newIp = new ImagePatch<PixelParam>(patchName,
                                                               pixelScale);
    //store the new image patch:
    patches.push_back(newIp);
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
update()
{
    int depth = updateFinestLevels();
    updateCoarserLevels(depth);
}

///\todo remove
#define VERIFYQUADTREEFILE 1
template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
verifyQuadtreeNode(Node* node)
{
    typename Node::State::File::TileIndex childIndices[4];
    assert(node->treeState->file->readTile(node->tileIndex, childIndices));

    if (node->children == NULL)
    {
        for (uint i=0; i<4; ++i)
            assert(childIndices[i] == Node::State::File::INVALID_TILEINDEX);
        return;
    }
    for (uint i=0; i<4; ++i)
        assert(childIndices[i] == node->children[i].tileIndex);
    for (uint i=0; i<4; ++i)
        verifyQuadtreeNode(&node->children[i]);
}
template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
verifyQuadtreeFile(Node* node)
{
#if VERIFYQUADTREEFILE
    //go up to parent
    while (node->parent != NULL)
        node = node->parent;

    return verifyQuadtreeNode(node);
#endif //VERIFYQUADTREEFILE
}

END_CRUSTA
