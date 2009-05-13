///\todo fix GPL


#include <construo/ImageFileLoader.h>
#include <construo/ImagePatch.h>

BEGIN_CRUSTA

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
Builder(const std::string& spheroidName, const uint tileSize[2])
{
    globe = new Globe(spheroidName, tileSize);
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
        for (uint i=0; i<4; ++i)
        {
            Node* child      = &node->children[i];
            child->isNew     = true;
            child->tileIndex = child->treeState->file->appendTile();
            assert(child->tileIndex!=Node::State::File::INVALID_TILEINDEX);
///\todo should I dump default-value-initialized to file here?
        }
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
sourceFinest(Node* node, Patch* imgPatch)
{
    
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution)
{
    //check for an overlap
    if (node->coverage.overlaps(imgPatch->sphereCoverage) ==
        SphereCoverage::SEPARATE)
    {
        return;
    }

    //recurse to children if the resolution of the node is too coarse
    if (node->resolution > imgResolution)
    {
        refine(node);
        for (uint i=0; i<4; ++i)
            updateFiner(&node->children[i], imgPatch, imgResolution);
        return;
    }

    //this node has the appropriate resolution
    sourceFinest(node, imgPatch);
}
    
template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateFinestLevels()
{
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
            updateFiner(*bIt, *pIt, imgResolution);
        }
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarserLevels()
{
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
    
}

END_CRUSTA
