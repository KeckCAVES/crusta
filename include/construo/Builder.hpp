///\todo fix GPL


#include <construo/ImageFileLoader.h>
#include <construo/ImagePatch.h>

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
    subsamplingFilter = Filter::createPointFilter();

    scopeBuf          = new Scope::Scalar[tileSize[0]*tileSize[1]*3];
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
    for (/*typename Builder<PixelParam, PolyhedronParam>::*/
         PatchPtrs::iterator it=patches.begin(); it!=patches.end(); ++it)
    {
        delete *it;
    }
    delete[] scopeBuf;
    delete[] nodeDataBuf;
    delete[] nodeDataSampleBuf;
    delete[] domainBuf;
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
flagAncestorsForUpdate(Node* node)
{
    node = node->parent;
    while (node != NULL)
    {
        node->mustBeUpdated = true;
        node = node->parent;
    }
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
    for (uint i=0; i<2; i++)
    {
///\todo abstract the sampler
        //assume bilinear interpolation here
        rectOrigin[i] = static_cast<uint>(Math::floor(nodeImgBox.getMin(i)));
        rectSize[i] = static_cast<uint>(Math::ceil(nodeImgBox.getMax(i))) + 1 -
                                                   rectOrigin[i];
    }
    PixelParam* rectBuffer = new PixelParam[rectSize[0] * rectSize[1]];
    imgPatch->image->readRectangle(rectOrigin, rectSize, rectBuffer);

    //determine the sample positions for the scope
///\todo enhance get Refinement such that is can produce non-uniform samples
    node->scope.getRefinement<Scope::Scalar>(tileSize[0], scopeBuf);
    //prepare the node's data buffer
    node->data = nodeDataBuf;
    node->treeState->file->readTile(node->tileIndex, node->data);
    
    //resample pixel values for the node from the read rectangle
    Scope::Scalar* endPtr = scopeBuf + tileSize[0]*tileSize[1];
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
            for(uint i=0; i<2; i++)
            {
                double pFloor = Math::floor(p[i]);
                d[i]          = p[i] - pFloor;
                ip[i]         = static_cast<int>(pFloor) - rectOrigin[i];
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

    node->data = NULL;
    
    //make sure that the parent dependent on this new data is also updated
    flagAncestorsForUpdate(node);
    //we musn't forget to update all the adjacent non-sibling nodes either
    int dirs[2];
    dirs[0] = node->treeIndex.child&0x01 ? 1 : -1;
    dirs[1] = node->treeIndex.child&0x10 ? 1 : -1;

    int offsets[2];
    Node* kin;
    //flag the vertical edge with the corner and the horizontal without it
    for (int s= 0; s<=1; ++s)
    {
        for (int i=s-1; i<=1; ++i)
        {
            offsets[s]   = dirs[s];
            offsets[1-s] = i;
            node->getKin(kin, offsets)
            if (kin != NULL)
                flagAncestorsForUpdate(kin);
        }
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
prepareSubsamplingDomain(Node* node)
{
    Node* kin     = NULL;
    Node* blChild = &node->children[0];

    PixelParam* domain = domainBuf + (tileSize[1]-1)*domainSize[0] +
                         tileSize[0]-1;

    int domainOff[2];
    int nodeOff[2];
    for (domainOff[1]=-1; domainOff[1]<=2; ++domainOff[1])
    {
        for (domainOff[0]=-1; domainOff[0]<=2; ++domainOff[0])
        {
        //- retrieve the kin
            nodeOff[0] = domainOff[0];
            nodeOff[1] = domainOff[1];
            blChild->getKin(kin, nodeOff);

        //- retrieve data as appropriate
            //default to blank data
            const PixelParam* data = node->treeState->file->getBlank();
            
            //read the data from file
            if (kin!=NULL && kin->tileIndex!=Node::State::File::INVALID_INDEX)
            {
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
                    for (uint i=blChild.treeIndex.level; i<kin.treeIndex.level;
                         ++i)
                    {
                        scale *= 0.5;
                    }
                    double step[2] = {1.0/(tileSize[0]-1), 1.0/(tileSize[1]-1)};
                    step[0] *= scale;
                    step[1] *= scale;
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
                        if (iy == tileSize[1])
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
                            if (ix == tileSize[0])
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
            
        //- insert the data at the appropriate location
            PixelParam* at = domain +
                             domainOff[1] * (tileSize[1]-1) * domainSize[0] +
                             domainOff[0] * (tileSize[0]-1);
            for (uint y=0; y<tileSize[1]; ++y)
            {
                memcpy(at + y*domainSize[0], data+y*tileSize[0],
                       tileSize[0]*sizeof(PixelParam));
            }
        }
    }
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
    prepareSubsamplingDomain(node);

    /* walk the pixels of the node's data and performed filtered look-ups into
       the domain */
    PixelParam* data = nodeDataBuf;
    PixelParam* domain;
    for (domain = domainBuf + (tileSize[1]-1) * domainSize[0] + (tileSize[0]-1);
         domain < domainBuf + 2 * (tileSize[1]-1) * domainSize[0] + tileSize[0];
         domain+= domainSize[0] - tileSize[0])
    {
        for (uint i=0; i<tileSize[0]; ++i, ++domain, ++data)
            *data = subsampleFilter.lookup(domain, domainSize[0]);
    }

    //commit the data to file
    node->data = nodeDataBuf;

    QuadtreeTileHeader header;
    header.reset(node);
    node->treeState->file->writeTile(node->tileIndex, header, node->data);
    
    node->data = NULL;
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
