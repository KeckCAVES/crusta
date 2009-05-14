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
    virtual ~BuilderBase(){}

    ///add a source image patch to be integrated into the spheroid
    virtual void addImagePatch(const std::string& patchName) = 0;
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

    ///refines a node by adding the children to the build tree
    void refine(Node* node);

    ///flags all the ancestors for an update starting at the given parent
    void flagAncestorsForUpdate(Node* parent);
    ///sources the data for a node from an image patch and commits it to file
    void sourceFinest(Node* node, Patch* imgPatch, uint overlap);
    ///traverse the tree to update it for given patch
    uint updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution);
    /** sources new patches to create new finer levels or update existing ones.
        Returns the depth of the update-tree for use during updating of the
        coarse levels. */
    uint updateFinestLevels();

    ///read the data of a node into the corresponding domain region
    void addNodeToDomain(Node* node, PixelParam* at, uint tileSize[2],
                         uint rowLen);
    ///read all the finer data required for subsampling into a continuous region
    PixelParam* prepareSubsamplingDomain(Node* node);
    /** traverse the tree to resample the next level of nodes that have had
        finer data modified */
    void updateCoarser(Node* node, uint level);
    ///regenerate interior hierarchy nodes that have had finer levels updated
    void updateCoarserLevels(uint depth);

    ///new or existing database containing the hierarchy to be updated
    Globe* globe;
    ///vector of patches to be added to the hierarchy
    PatchPtrs patches;

    ///temporary buffer to hold scope refinements
    Scope::Scalar* scopeBuf;
    ///temporary buffer to hold node data
    PixelParam* nodeDataBuf;

//- Inherited from BuilderBase
public:
    virtual void addImagePatch(const std::string& patchName);
    virtual void update();    
};

END_CRUSTA

#include <construo/Builder.hpp>

#endif //_Builder_H_
