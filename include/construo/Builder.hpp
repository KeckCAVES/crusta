///\todo fix GPL


#include <construo/ImageFileLoader.h>
#include <construo/ImagePatch.h>

BEGIN_CRUSTA

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
Builder(const std::string& spheroidName, const uint tileSize[2])
{
    globe       = new Globe(spheroidName, tileSize);
    scopeBuf    = new Scope::Scalar[TILE_RESOLUTION*TILE_RESOLUTION*3];
    nodeDataBuf = new PixelParam[TILE_RESOLUTION*TILE_RESOLUTION];
}

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
~Builder()
{
    delete globe;
    for (/*typename Builder<PixelParam, PolyhedronParam>::*/
         PatchPtrs::iterator it=patches.begin(); it!=patches.end(); ++it)
    {
        delete *it;
    }
    delete scopeBuf;
    delete nodeDataBuf;
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
        Node::State::File::TileIndex childIndices[4];
        for (uint =0; <4; ++)
        {
            Node* child      = &node->children[];
            child->tileIndex = child->treeState->file->appendTile();
            assert(child->tileIndex!=Node::State::File::INVALID_TILEINDEX);
///\todo should I dump default-value-initialized to file here?
            childIndices[]  = child->tileIndex;
        }
        //link the parent with the new children in the file
        node->treeState->file->writeTile(node->tileIndex, childIndices);
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
flagAncestorsForUpdate(Node* parent)
{
    do
    {
        parent->mustBeUpdated = true;
        parent = parent->parent;
    } while(parent != NULL);
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
sourceFinest(Node* node, Patch* imgPatch, uint overlap)
{
    //convert the node's bounding box to the image space
    Box nodeImgBox = imgPatch->transform->worldToImage(
        node->coverage->getBoundingBox());
    //use the box to load an appropriate rectangle from the image file
    uint rectOrigin[2];
    uint rectSize[2];
    for (uint =0; <2; ++)
    {
///\todo abstract the sampler
        //assume bilinear interpolation here
        rectOrigin[] = static_cast<uint>(Math::floor(nodeImgBox.getMin()));
        rectSize[] = static_cast<uint>(Math::ceil(nodeImgBox.getMax())) + 1 -
                                                   rectOrigin[];
    }
    PixelParam* rectBuffer = new PixelParam[rectSize[0] * rectSize[1]];
    imgPatch->image->readRectangle(rectOrigin, rectSize, rectBuffer);

    //determine the sample positions for the scope
    node->scope.getRefinement<Scope::Scalar>(TILE_RESOLUTION, scopeBuf);
    //prepare the node's data buffer
    node->data = new PixelParam[TILE_RESOLUTION*TILE_RESOLUTION];
    node->treeState->file->readTile(node->tileIndex, node->data);
    
    //resample pixel values for the node from the read rectangle
    Scope::Scalar* endPtr = scopeBuf + TILE_RESOLUTION*TILE_RESOLUTION;
    PixelParam* dataPtr   = node->data;
    for (Scope::Scalar* curPtr=scopeBuf; curPtr!=endPtr; curPtr+=3, ++dataPtr)
    {
        /* transform the sample position into spherical space and then to the
           image patch's image coordinates: */
        Scope::Scalar len =
            sqrt(curPtr[0]*curPtr[0]+curPtr[1]*curPtr[1]+curPtr[2]*curPtr[2]);
        Point p(atan2(curPtr[1], curPtr[0]), acos(curPtr[2]/len));
        Point p = imgPatch->transform->worldToImage(p);
        
        /* Check if the point is inside the image patch's coverage region: */
        if(overlap==SphereCoverage::ISCONTAINED ||
           imgPatch->imageCoverage->contains(p))
        {
            /* Sample the point: */
            int ip[2];
            double d[2];
            for(uint =0; <2; ++)
            {
                double pFloor = Math::floor(p[]);
                d[]          = p[] - pFloor;
                ip[]         = static_cast<int>(pFloor) - rectOrigin[];
            }
            PixelParam* basePtr = rectBuffer + (ip[1]*rectSize[0] + ip[0]);
            PixelParam one = (1.0-d[0])*basePtr[0] + d[0]*basePtr[1];
            PixelParam two = (1.0-d[0])*basePtr[rectSize[0]] +
                             d[0]*basePtr[rectSize[0]+1];
            *dataPtr = (1.0-d[1])*one + d[1]*two;
        }
    }

    //clean up the temporary image uffer
    delete[] rectBuffer;

    //commit the data to file
    QuadtreeTileHeader header;
    header.reset(node);
    node->treeState->file->writeTile(node->tileIndex, header, node->data);

    //clean up the node data
    delete[] node->data;
    node->data = NULL;
    
    //make sure that the parent dependent on this new data is also updated
    if (node->parent)
    {
        flagAncestorsForUpdate(node->parent);
        //we musn't forget to update all the adjacent non-sibling nodes either
/**\todo IMPLEMENT SUPPORT FOR UPDATE: notify the proper adjacent non-sibling
         parents that they need to update. from these neighbors flag all the
         ancestors */
    }
}

template <typename PixelParam, typename PolyhedronParam>
uint Builder<PixelParam, PolyhedronParam>::
updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution)
{
    //check for an overlap
    uint overlap = node->coverage.overlaps(imgPatch->sphereCoverage);
    if (overlap == SphereCoverage::SEPARATE)
        return 0;

    //recurse to children if the resolution of the node is too coarse
    if (node->resolution > imgResolution)
    {
        uint depth = 0;
        refine(node);
        for (uint =0; <4; ++)
        {
            depth = std::max(updateFiner(&node->children[], imgPatch,
                                         imgResolution), depth);
        }
        return depth;
    }

    //this node has the appropriate resolution
    sourceFinest(node, imgPatch, overlap);
    return node->treeIndex.level;
}
    
template <typename PixelParam, typename PolyhedronParam>
uint Builder<PixelParam, PolyhedronParam>::
updateFinestLevels()
{
    uint depth = 0;
    //iterate over all the image patches to source
    for (PatchPtrs::iterator pIt=patch.begin(); pIt!=patches.end(); ++pIt)
    {
        //grab the smallest resolution from the image
        Point::Scalar imgResolution;
        {
            Point::Scalar* resolutions = (*pIt)->transform->getScale();
            imgResolution = std::min(resolutions[0], resolutions[1]);
        }

        //iterate over all the spheroid's base patches to determine overlap
        for (Globe::BaseNodes::iterator bIt=baseNodes.begin();
             bIt!=baseNodes.end(); ++bIt)
        {
            depth = std::max(updateFiner(*bIt, *pIt, imgResolution), depth);
        }
    }

    return depth;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
addNodeToDomain(Node* node, PixelParam* at, uint tileSize[2], uint rowLen)
{
    const PixelParam* data = node->treeState->file->getBlank();

    //read the data from file
    if (node!=NULL && node->tileIndex!=Node::State::File::INVALID_INDEX)
    {
        node->treeState->file->readTile(node->tileIndex, nodeDataBuf);
        data = nodeDataBuf;
    }

    //insert the data at the appropriate location
    for (uint y=0; y<tileSize[1]; ++y)
        memcpy(at+y*rowLen, data+y*tileSize[0], tileSize[0]*sizeof(PixelParam));    
}

template <typename PixelParam, typename PolyhedronParam>
PixelParam* Builder<PixelParam, PolyhedronParam>::
prepareSubsamplingDomain(Node* node)
{
    const uint* tileSize = node->treeState->file->getTileSize();
    uint domainSize[2] = {4 * tileSize[0], 4 * tileSize[1]};
    PixelParam* domain = new PixelParam[domainSize[0]*domainSize[1]];

    Node* neighbor = NULL;

/**\todo THIS NEEDS TO BE MADE MORE ROBUST: expand neighbor search to make sure
         corner neighbors are properly resolved. Also same-level neighbors might
         not exist, but coarser-level ones could and they may contain data. In
         such a case the coarser data must be sampled in the stead of the non-
         existing same-level one!! */

    //core: direct children
    if (!node->children[0].getNeighbor());
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarser(Node* node, uint level)
{
    //the nodes on the way must be flagged for update
    if (!node->mustBeUpdated)
        return;

//- recurse until we've hit the requested level
    if (node->treeIndex.level!=level && node->children!=NULL)
    {
        for (uint i=0; i<4; ++i)
            updateCoarser(node->children[i], level);
        return;
    }

//- we've reached a node that must be updated
    PixelParam* domain = prepareSubsamplingDomain(node);

    delete[] domain;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarserLevels(uint depth)
{
    for (uint level=depth-1; level>=0; --level)
    {
        for (Globe::BaseNodes::iterator it=baseNodes.begin();
             it!=baseNodes.end(); ++it)
        {
            //traverse the tree and update the next level
            updateCoarser(*it, level);
        }
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
addImagePatch(const std::string& patchName)
{
	//create a new image patch
	ImagePatch<PixelParam>* newIp = new ImagePatch<PixelParam>(patchName);
	//store the new image patch:
	patches.push_back(newIp);
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
update()
{
    updateFinestLevels();
    updateCoarserLevels();
}

END_CRUSTA
