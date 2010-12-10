#ifndef _Builder_H_
#define _Builder_H_

#include <string>
#include <vector>

#include <construo/ImagePatch.h>
#include <construo/Tree.h>


BEGIN_CRUSTA


class BuilderBase
{
public:
    struct ImagePatchSource
    {
        ImagePatchSource(const std::string& name, double offset, double scale,
                         const std::string& data, bool sample) :
            path(name), pixelScale(scale), nodata(data), pointSampled(sample)
        {
        }

        std::string path;
        double      pixelOffset;
        double      pixelScale;
        std::string nodata;
        bool        pointSampled;
    };
    typedef std::vector<ImagePatchSource> ImagePatchSources;

    virtual ~BuilderBase(){}

    ///add a source image patch to be integrated into the spheroid
    void addImagePatches(const ImagePatchSources& sources)
    {
        imagePatchSources = sources;
    }

    ///update the spheroid with the new patches
    virtual void update() = 0;

protected:
    ImagePatchSources imagePatchSources;
};

template <typename PixelParam>
class Builder : public BuilderBase
{
public:
    ///opens a spheroid file to which new additions are to be made
    Builder(const std::string& spheroidName, const uint tileSize[2]);
    ///closes the spheroid file and cleans up scratch data
    ~Builder();

protected:
    typedef typename PixelParam::Type PixelType;

    typedef Spheroid<PixelParam>  Globe;
    typedef TreeNode<PixelParam>  Node;
    typedef ImagePatch<PixelType> Patch;

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
    int updateFinestLevels(const ImagePatchSource& patchSource);

    ///read all the finer data required for subsampling into a continuous region
    void prepareSubsamplingDomain(Node* node);
    /** traverse the tree to resample the next level of nodes that have had
        finer data modified */
    void updateCoarser(Node* node, int level);
    ///regenerate interior hierarchy nodes that have had finer levels updated
    void updateCoarserLevels(int depth);

    ///new or existing database containing the hierarchy to be updated
    Globe* globe;

    ///temporary buffer to hold scope refinements
    Scope::Scalar* scopeBuf;
    ///temporary buffer to hold sample positions for sourcing data
    Point* sampleBuf;
    ///temporary buffer to hold node data
    PixelType* nodeDataBuf;
    ///temporary buffer to hold node data that needs to be sampled before use
    PixelType* nodeDataSampleBuf;
    ///size of the tiles to be generated/updated
    uint tileSize[2];
    ///temporary buffer to hold the subsampling domain
    PixelType* domainBuf;
    ///size of the temporary subsampling domain
    uint domainSize[2];

//- Inherited from BuilderBase
public:
    virtual void update();

///\todo remove
void verifyQuadtreeNode(Node* node);
void verifyQuadtreeFile(Node* node);
};

END_CRUSTA

#include <construo/Builder.hpp>

#endif //_Builder_H_
