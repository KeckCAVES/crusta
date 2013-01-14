///\todo fix GPL

//#include "omp.h"

#include <construo/ImageFileLoader.h>
#include <construo/ImagePatch.h>
#include <construo/SubsampleFilter.h>
#include <crustacore/GlobeData.h>
#include <crustacore/PixelOps.h>


#define DYNAMIC_FILTER_TYPE SUBSAMPLEFILTER_PYRAMID
#define STATIC_FILTER_TYPE  SUBSAMPLEFILTER_PYRAMID


///\todo remove
#if CRUSTA_ENABLE_DEBUG
#define USE_NEAREST_FILTERING 0
#define DEBUG_PREPARESUBSAMPLINGDOMAIN 0
#define DEBUG_SOURCEFINEST 0
#define DEBUG_SOURCEFINEST_SHOW_TEXELS 0
#define DEBUG_FLAGANCESTORSFORUPDATE 0
#if DEBUG_FLAGANCESTORSFORUPDATE
static float flagAncestorsForUpdateColor[3];
#endif //DEBUG_FLAGANCESTORSFORUPDATE
#include <construo/ConstruoVisualizer.h>
#endif //CRUSTA_ENABLE_DEBUG

BEGIN_CRUSTA

template <typename PixelParam>
Builder<PixelParam>::
Builder(const std::string& spheroidName, const uint size[2])
{
///\todo Frak this is retarded. Reason so far is the getRefinement from scope
assert(size[0]==size[1]);

    tileSize[0] = size[0];
    tileSize[1] = size[1];

    globe = new Globe(spheroidName, tileSize);

    scopeBuf          = new Scope::Scalar[tileSize[0]*tileSize[1]*3];
    sampleBuf         = new Point[tileSize[0]*tileSize[1]];
    nodeDataBuf       = new PixelType[tileSize[0]*tileSize[1]];
    nodeDataSampleBuf = new PixelType[tileSize[0]*tileSize[1]];
    //the -3 takes into account the shared edges of the tiles
    domainSize[0] = 4*tileSize[0] - 3;
    domainSize[1] = 4*tileSize[1] - 3;
    domainBuf     = new PixelType[domainSize[0]*domainSize[1]];
}

template <typename PixelParam>
Builder<PixelParam>::
~Builder()
{
    delete globe;
    delete[] scopeBuf;
    delete[] sampleBuf;
    delete[] nodeDataBuf;
    delete[] nodeDataSampleBuf;
    delete[] domainBuf;
}


template <typename PixelParam>
void Builder<PixelParam>::
subsampleChildren(Node* node)
{
    typedef GlobeData<PixelParam> gd;
    typedef PixelOps<PixelType>   po;

    assert(node->children != NULL);
    //read in the node's existing data from file
    typename gd::File* file =
        node->globeFile->getPatch(node->treeIndex.patch());
    file->readTile(node->tileIndex, nodeDataSampleBuf);

    const PixelType& nodata = node->globeFile->getNodata();

    const int offsets[4] = {
        0, (tileSize[0]-1)>>1, ((tileSize[1]-1)>>1)*tileSize[0],
        ((tileSize[1]-1)>>1)*tileSize[0] + ((tileSize[0]-1)>>1) };
    const int halfSize[2] = { (tileSize[0]+1)>>1, (tileSize[1]+1)>>1 };
    for (int i=0; i<4; ++i)
    {
//        #pragma omp parallel for
        for (int y=0; y<halfSize[1]; ++y)
        {
            PixelType* wbase = nodeDataBuf + y*2*tileSize[0];
            PixelType* rbase = nodeDataSampleBuf + y*tileSize[0] + offsets[i];
            for (int x=0; x<halfSize[0]; ++x, wbase+=2, ++rbase)
            {
                wbase[0] = rbase[0];
                if (x<halfSize[0]-1)
                {
                    wbase[1] = po::average(rbase[0], rbase[1], nodata);
                }
                if (y<halfSize[1]-1)
                {
                    wbase[tileSize[0]] = po::average(
                        rbase[0], rbase[tileSize[0]], nodata);
                }
                if (x<halfSize[0]-1 && y<halfSize[1]-1)
                {
                    wbase[tileSize[0]+1] = po::average(rbase[0], rbase[1],
                        rbase[tileSize[0]], rbase[tileSize[0]+1], nodata);
                }
            }
        }
        //write the subsampled data to the child
        Node& child = node->children[i];
        child.data = nodeDataBuf;
        typename gd::TileHeader header = child.getTileHeader();
        typename gd::File* childFile =
            child.globeFile->getPatch(child.treeIndex.patch());
        childFile->writeTile(child.tileIndex, header, child.data);
        child.data = NULL;
    }
}

template <typename PixelParam>
void Builder<PixelParam>::
refine(Node* node)
{
    typedef GlobeData<PixelParam> gd;
    typename gd::File* file =
        node->globeFile->getPatch(node->treeIndex.patch());

    //try to load the missing children from the quadtree file
    if (node->children == NULL)
        node->loadMissingChildren();

    //if loading failed then we just have to create new children
    if (node->children == NULL)
    {
        node->createChildren();
        TileIndex childIndices[4];
        for (uint i=0; i<4; ++i)
        {
            Node* child      = &node->children[i];
            child->tileIndex = file->appendTile(node->globeFile->getBlank());
            assert(child->tileIndex!=INVALID_TILEINDEX);
///\todo should I dump default-value-initialized to file here?
            childIndices[i]  = child->tileIndex;
        }
        subsampleChildren(node);
        //link the parent with the new children in the file
        file->writeTile(node->tileIndex, childIndices);
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
    }
}

template <typename PixelParam>
void Builder<PixelParam>::
flagAncestorsForUpdate(Node* node)
{
    while (node!=NULL && !node->mustBeUpdated)
    {
#if DEBUG_FLAGANCESTORSFORUPDATE
ConstruoVisualizer::addPrimitive(GL_LINES, node->coverage, flagAncestorsForUpdateColor);
ConstruoVisualizer::show();
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


///\todo overlap is apparently unused here
template <typename PixelParam>
void Builder<PixelParam>::
sourceFinest(Node* node, Patch* imgPatch, uint overlap)
{
    typedef GlobeData<PixelParam> gd;

    ImgBoxes imgBoxes;
    const int*        imgSize  = imgPatch->image->getSize();
    const PixelType& imgNodata = imgPatch->image->getNodata();
    const Point::Scalar allowedBoxSize[2]  = { imgSize[0]>>1, imgSize[1]>>1 };

    //transform all the sample points into the image space
    node->scope.getRefinement(tileSize[0], scopeBuf);

    for (int i=0; i<int(tileSize[0]*tileSize[1]); ++i)
    {
        Point& p = sampleBuf[i];

        //convert the scope sample to a spherical one
        const Scope::Scalar* curScope = &scopeBuf[i*3];
        p = Converter::cartesianToSpherical(
            Scope::Vertex(curScope[0], curScope[1], curScope[2]));
        //transform the sample into the image space
        p = imgPatch->transform->worldToImage(p);

        //make sure the sample is valid
        if (p[0]<0 || p[0]>imgSize[0]-1 || p[1]<0 || p[1]>imgSize[1]-1)
            continue;

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
    typename gd::File* file =
        node->globeFile->getPatch(node->treeIndex.patch());
    file->readTile(node->tileIndex, node->data);

    //go through all the image boxes and sample them
    const PixelType& globeNodata = node->globeFile->getNodata();
//    #pragma omp parallel for
    for (int box=0; box<static_cast<int>(imgBoxes.size()); ++box)
    {
        const ImgBox& ib = imgBoxes[box];
        //read the corresponding image piece
        int rectOrigin[2];
        int rectSize[2];
        for (int i=0; i<2; i++)
        {
///\todo abstract the sampler assume bilinear interpolation here
            rectOrigin[i] = static_cast<int>(Math::floor(ib.min[i]));
            rectSize[i]   = static_cast<int>(Math::ceil (ib.max[i])) -
                            rectOrigin[i] + 1;
        }
        PixelType* rectBuffer = new PixelType[rectSize[0] * rectSize[1]];
        imgPatch->image->readRectangle(rectOrigin, rectSize, rectBuffer);

        //sample the points
//        #pragma omp parallel for
        typedef SubsampleFilter<PixelType, DYNAMIC_FILTER_TYPE> Filter;
        for (int i=0; i<static_cast<int>(ib.indices.size()); ++i)
        {
            const int& idx          = ib.indices[i];
            double at[2]            = { sampleBuf[idx][0], sampleBuf[idx][1] };
            PixelType& defaultValue = node->data[idx];

            node->data[idx] = Filter::sample(rectBuffer, rectOrigin, at,
                rectSize, imgNodata, defaultValue, globeNodata);

#if DEBUG_SOURCEFINEST
ConstruoVisualizer::Floats scopeSample;
scopeSample.resize(3);
Point wp = imgPatch->transform->imageToWorld(Point(at[0], at[1]));
scopeSample[0] = wp[0];
scopeSample[1] =   0.0;
scopeSample[2] = wp[1];
//static const float scopeSampleColor[3] = { 0.2f, 1.0f, 0.1f };
float scopeSampleColor[3] = {
    (float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX };
ConstruoVisualizer::addPrimitive(GL_POINTS, scopeSample, scopeSampleColor);
ConstruoVisualizer::show();
#endif //DEBUG_SOURCEFINEST
        }
        //clean up the temporary image uffer
        delete[] rectBuffer;
    }

    //prepare the header
    typename gd::TileHeader header = node->getTileHeader();

    //commit the data to file
    file->writeTile(node->tileIndex, header, node->data);

#if 0
{
static const float color[3] = { 0.2f, 1.0f, 0.1f };
ConstruoVisualizer::addScopeRefinement(tileSize[0], scopeBuf, color);
ConstruoVisualizer::show();
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

///\TODO Proper filtering will need this, but it is quite broken right now
#if 0
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
#endif
    }
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
}

template <typename PixelParam>
int Builder<PixelParam>::
updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution)
{
///\todo remove
#if DEBUG_SOURCEFINEST || 0
static const Color covColor(0.1f, 0.4f, 0.6f, 1.0f);
ConstruoVisualizer::addSphereCoverage(node->coverage, -1, covColor);
ConstruoVisualizer::peek();
#endif

    //check for an overlap
    uint overlap = node->coverage.overlaps(*(imgPatch->sphereCoverage));
    ///\todo HACK: For some reason if we test coverage at the root nodes it can
    //  mess up when the patch only 'slightly' goes into a given root node, so
    //  here we make sure we always go down at least a level.
    //  Unfortunately this subdivides nodes more than needed... must find the
    //  root problem at some point...
    if (overlap == SphereCoverage::SEPARATE && node->parent)
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
//ConstruoVisualizer::show();
    sourceFinest(node, imgPatch, overlap);
    return node->treeIndex.level();
}

template <typename PixelParam>
int Builder<PixelParam>::
updateFinestLevels(const ImagePatchSource& patchSource)
{
    int depth = 0;

    //create a new image patch
    Patch patch(patchSource.path,
                patchSource.pixelOffset, patchSource.pixelScale,
                patchSource.nodata, patchSource.pointSampled);

///\todo remove
#if DEBUG_SOURCE_FINEST
ConstruoVisualizer::clear();
ConstruoVisualizer::addPrimitive(GL_LINES, patch.sphereCoverage));
ConstruoVisualizer::peek();
#endif

#if DEBUG_SOURCEFINEST_SHOW_TEXELS
const int* size = patch.image->getSize();
ConstruoVisualizer::Floats edges;
edges.resize(size[0]*size[1]*4*2*3);
float* e = &edges[0];
ConstruoVisualizer::Floats centers;
centers.resize(size[0]*size[1]*3);
float* c = &centers[0];

Point corners[4];
for (int y=0; y<size[1]; ++y)
{
    for (int x=0; x<size[0]; ++x, c+=3)
    {
        corners[0] = patch.transform->imageToWorld(Point(x-0.5, y-0.5));
        corners[1] = patch.transform->imageToWorld(Point(x+0.5, y-0.5));
        corners[2] = patch.transform->imageToWorld(Point(x+0.5, y+0.5));
        corners[3] = patch.transform->imageToWorld(Point(x-0.5, y+0.5));
        Point origin = patch.transform->imageToWorld(Point(x, y));

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
ConstruoVisualizer::addPrimitive(GL_LINES, edges, edgeColor);
static const float centerColor[3] = { 0.2f, 0.6f, 0.9f };
ConstruoVisualizer::addPrimitive(GL_POINTS, centers, centerColor);
ConstruoVisualizer::show();
#endif //show image pixels

    //grab the smallest resolution from the image
    Point::Scalar imgResolution=patch.transform->getFinestResolution(
        patch.image->getSize());
    /* exaggerate the image's resolution because our sampling is not aligned
       with the image axis */
    imgResolution *= Point::Scalar(Math::sqrt(2.0));

    //iterate over all the spheroid's base patches to determine overlap
    for (typename Globe::BaseNodes::iterator bIt=globe->baseNodes.begin();
         bIt!=globe->baseNodes.end(); ++bIt)
    {
        depth = std::max(updateFiner(&(*bIt), &patch, imgResolution), depth);

        std::cout << ".";
        std::cout.flush();
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(&(*bIt));
//ConstruoVisualizer::show();
    }
    std::cout << " done\n\n";
    std::cout.flush();

    return depth;
}

template <typename PixelParam>
void Builder<PixelParam>::
prepareSubsamplingDomain(Node* node)
{
    typedef GlobeData<PixelParam> gd;

#if DEBUG_PREPARESUBSAMPLINGDOMAIN
ConstruoVisualizer::addPrimitive(GL_LINES, node->coverage);
ConstruoVisualizer::show();
#endif //DEBUG_PREPARESUBSAMPLINGDOMAIN

    /* specify the order of lower-level nodes manually such that the inner nodes
       are added last and overwrite the edge value (e.g. if one of the
       neighboring nodes does not exist or has no valid values, we want to use
       the inner node ones instead */
    static const int offsets[16][2] = {
        {-1,-1}, { 0,-1}, { 1,-1}, { 2,-1}, {-1, 2}, { 0, 2}, { 1, 2}, { 2, 2},
        {-1, 1}, {-1, 0}, { 2, 1}, { 2, 0}, { 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}
    };
    for (int i=0; i<16; ++i)
    {
    //- retrieve the kin
        Node* kin         = NULL;
        PixelType* domain = domainBuf + (tileSize[1]-1)*domainSize[0] +
                             tileSize[0]-1;

        int domainOff[2] = {offsets[i][0], offsets[i][1]};
        int nodeOff[2]   = {offsets[i][0], offsets[i][1]};
        uint kinO;
        node->getKin(kin, nodeOff, true, 1, &kinO);
#if DEBUG_PREPARESUBSAMPLINGDOMAIN
const static float scopeRefColor[3] = { 0.3f, 0.8f, 0.3f };
kin->scope.getRefinement(tileSize[0], scopeBuf);
ConstruoVisualizer::addScopeRefinement(tileSize[0], scopeBuf, scopeRefColor);
ConstruoVisualizer::show();
#endif //DEBUG_PREPARESUBSAMPLINGDOMAIN

    //- retrieve data as appropriate
        //default to blank data
        const PixelType* data = node->globeFile->getBlank();

        //read the data from file
/**\todo disabled reading from neighbors that aren't from the same base patch.
reading of neighbor data with differring orientation is currently absolutely
broken, overwrites random memory regions and breaks fraking everything.
Note: getKin across patches seem to be broken: e.g. offset==3 returned. */
        if (kin != NULL && node->treeIndex.patch()==kin->treeIndex.patch())
        {
            /* sampling from the same patch will always have data (even if that
               is "nodata" */
            assert(kin->tileIndex!=INVALID_TILEINDEX);
            typename gd::File* file =
                kin->globeFile->getPatch(kin->treeIndex.patch());
            if (nodeOff[0]==0 && nodeOff[1]==0)
            {
                //we're grabbing data from a same leveled kin
                file->readTile(kin->tileIndex, nodeDataBuf);
                data = nodeDataBuf;
            }
            else
            {
                //need to sample coarser level node as the kin replacement
                file->readTile(kin->tileIndex, nodeDataSampleBuf);
                //determine the resample step size
                double scale = 1;
                for (uint i=kin->treeIndex.level();
                     i<node->treeIndex.level()+1U; ++i)
                {
                    scale *= 0.5;
                }
                double step[2] = {scale/(tileSize[0]-1), scale/(tileSize[1]-1)};

                double at[2];
                int rectOrigin[2] = {0,0};
                int rectSize[2]   = {tileSize[0], tileSize[1]};
                const PixelType& nodata = kin->globeFile->getNodata();
                PixelType* wbase = nodeDataBuf;

//                #pragma omp parallel for
                typedef SubsampleFilter<PixelType, DYNAMIC_FILTER_TYPE> Filter;
                for (uint ny=0; ny<tileSize[1]; ++ny)
                {
                    at[1] = nodeOff[1]*scale + ny*step[1];
                    at[0] = nodeOff[0]*scale;
                    for (uint nx=0; nx<tileSize[0]; ++nx,at[0]+=step[0],++wbase)
                    {
                        *wbase = Filter::sample(nodeDataSampleBuf, rectOrigin,
                                                at, rectSize, nodata, nodata,
                                                nodata);
                    }
                }
                data = nodeDataBuf;
            }
        }

/**\todo disabled reading from neighbors that don't have the same orientation
reading of neighbor data with differring orientation is currently absolutely
broken, overwrites random memory regions and breaks fraking everything */
kinO = 0;
    //- insert the data at the appropriate location
        PixelType* base = domain +
                          domainOff[1] * (tileSize[1]-1) * domainSize[0] +
                          domainOff[0] * (tileSize[0]-1);
        int startY[4] = { 0, tileSize[1]-1, tileSize[1]-1, 0 };
        int stepY[4]  = { tileSize[0], 1,  -tileSize[0], -1 };
        int startX[4] = { 0, 0, tileSize[0]-1, tileSize[0]-1 };
        int stepX[4]  = { 1,  -tileSize[1], -1, tileSize[0]};
//        #pragma omp parallel for
        for (uint y=0; y<tileSize[1]; ++y)
        {
            PixelType* to         = base + y*domainSize[0];
            const PixelType* from = data + startY[kinO] + y*stepY[kinO] +
                                     startX[kinO];
            for (uint x=0; x<tileSize[0]; ++x, ++to, from+=stepX[kinO])
                *to = *from;
        }
    }
}

template <typename PixelParam>
void Builder<PixelParam>::
updateCoarser(Node* node, int level)
{
#if DEBUG_PREPARESUBSAMPLINGDOMAIN
static const float covColor[3] = { 0.1f, 0.4f, 0.6f };
ConstruoVisualizer::addPrimitive(GL_LINES, node->coverage, covColor);
ConstruoVisualizer::peek();
#endif

    //the nodes on the way must be flagged for update
    if (!node->mustBeUpdated)
        return;

//- recurse until we've hit the requested level
    if (node->treeIndex.level()!=static_cast<uint8>(level) &&
        node->children!=NULL)
    {
        for (uint i=0; i<4; ++i)
            updateCoarser(&(node->children[i]), level);
        return;
    }

//- we've reached a node that must be updated
    prepareSubsamplingDomain(node);

    /* walk the pixels of the node's data and performed filtered look-ups into
       the domain */
    typedef SubsampleFilter<PixelType, SUBSAMPLEFILTER_PYRAMID> Filter;

    const PixelType& globeNodata = node->globeFile->getNodata();

    PixelType* data = nodeDataBuf;
    PixelType* domain;
    for (domain = domainBuf +   (tileSize[1]-1)*domainSize[0]+  (tileSize[0]-1);
         domain<= domainBuf + 3*(tileSize[1]-1)*domainSize[0]+3*(tileSize[0]-1);
         domain+= 2*domainSize[0] - 2*tileSize[0])
    {
        for (uint i=0; i<tileSize[0]; ++i, domain+=2, ++data)
            *data  = Filter::sample(domain, domainSize[0], globeNodata);
    }

    //commit the data to file
    node->data = nodeDataBuf;

    typedef GlobeData<PixelParam> gd;
    typename gd::TileHeader header = node->getTileHeader();
    typename gd::File* file =
        node->globeFile->getPatch(node->treeIndex.patch());

    file->writeTile(node->tileIndex, header, node->data);

    node->data = NULL;
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
}

template <typename PixelParam>
void Builder<PixelParam>::
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
//ConstruoVisualizer::clear();
//ConstruoVisualizer::addPrimitive(GL_LINES, it->coverage);
//ConstruoVisualizer::show();
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


template <typename PixelParam>
void Builder<PixelParam>::
update()
{
    int depth = 0;
    int numPatches = static_cast<int>(imagePatchSources.size());
    for (int i=0; i<numPatches; ++i)
    {
        try
        {
            std::cout << "*** Adding source image " << i+1 << " out of " <<
                         numPatches << "\n\n";
            std::cout.flush();

            int newDepth = updateFinestLevels(imagePatchSources[i]);
            depth = std::max(newDepth, depth);
        }
        catch(std::runtime_error err)
        {
            std::cerr << "Ignoring image patch " << imagePatchSources[i].path <<
                         " due to exception " << err.what() << std::endl;
        }
    }

    updateCoarserLevels(depth);
}

///\todo remove
#define VERIFYQUADTREEFILE 1
template <typename PixelParam>
void Builder<PixelParam>::
verifyQuadtreeNode(Node* node)
{
    TileIndex childIndices[4];
    assert(node->treeState->file->readTile(node->tileIndex, childIndices));

    if (node->children == NULL)
    {
        for (uint i=0; i<4; ++i)
            assert(childIndices[i] == INVALID_TILEINDEX);
        return;
    }
    for (uint i=0; i<4; ++i)
        assert(childIndices[i] == node->children[i].tileIndex);
    for (uint i=0; i<4; ++i)
        verifyQuadtreeNode(&node->children[i]);
}
template <typename PixelParam>
void Builder<PixelParam>::
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
