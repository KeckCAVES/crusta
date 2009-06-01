#ifndef _Builder_H_
#define _Builder_H_

#include <string>
#include <vector>

#include <construo/Filter.h>
#include <construo/ImagePatch.h>
#include <construo/Tree.h>

BEGIN_CRUSTA

class BuilderBase
{
public:
    virtual ~BuilderBase(){}

    ///add a source image patch to be integrated into the spheroid
    virtual void addImagePatch(const std::string& patchName,
                               double pixelScale) = 0;
    ///update the spheroid with the new patches
    virtual void update() = 0;
};

template <typename PixelParam, typename PolyhedronParam>
class Builder : public BuilderBase
{
public:
    ///opens a spheroid file to which new additions are to be made
    Builder(const std::string& spheroidName, const uint tileSize[2]);
    ///closes the spheroid file and cleans up scratch data
    ~Builder();

protected:
    typedef Spheroid<PixelParam, PolyhedronParam>   Globe;
    typedef TreeNode<PixelParam>                    Node;
    typedef ImagePatch<PixelParam>                  Patch;
    typedef std::vector<Patch*>                     PatchPtrs;

    ///subsamples data from the given node into that node's children
    void subsampleChildren(Node* node);
    ///refines a node by adding the children to the build tree
    void refine(Node* node);

    ///flags all the ancestors for an update
    void flagAncestorsForUpdate(Node* node);
    ///sources the data for a node from an image patch and commits it to file
    void sourceFinest(Node* node, Patch* imgPatch, uint overlap);
    ///traverse the tree to update it for given patch
    int updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution);
    /** sources new patches to create new finer levels or update existing ones.
        Returns the depth of the update-tree for use during updating of the
        coarse levels. */
    int updateFinestLevels();

    ///read all the finer data required for subsampling into a continuous region
    void prepareSubsamplingDomain(Node* node);
    /** traverse the tree to resample the next level of nodes that have had
        finer data modified */
    void updateCoarser(Node* node, int level);
    ///regenerate interior hierarchy nodes that have had finer levels updated
    void updateCoarserLevels(int depth);

    ///new or existing database containing the hierarchy to be updated
    Globe* globe;
    ///vector of patches to be added to the hierarchy
    PatchPtrs patches;

    ///filter used for subsampling the coarser levels from finer ones
    Filter subsamplingFilter;

    ///temporary buffer to hold scope refinements
    Scope::Scalar* scopeBuf;
    ///temporary buffer to hold sample positions for sourcing data
    Point* sampleBuf;
    ///temporary buffer to hold node data
    PixelParam* nodeDataBuf;
    ///temporary buffer to hold node data that needs to be sampled before use
    PixelParam* nodeDataSampleBuf;
    ///size of the tiles to be generated/updated
    uint tileSize[2];
    ///temporary buffer to hold the subsampling domain
    PixelParam* domainBuf;
    ///size of the temporary subsampling domain
    uint domainSize[2];

//- Inherited from BuilderBase
public:
    virtual void addImagePatch(const std::string& patchName, double pixelScale);
    virtual void update();

///\todo remove
void verifyQuadtreeNode(Node* node);
void verifyQuadtreeFile(Node* node);
};

END_CRUSTA

#include <construo/Builder.hpp>

#endif //_Builder_H_
