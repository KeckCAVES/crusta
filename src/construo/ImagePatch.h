///\todo fix GPL

#ifndef _ImagePatch_H_
#define _ImagePatch_H_

#include <construo/ImageCoverage.h>
#include <construo/ImageFile.h>
#include <construo/ImageTransform.h>
#include <construo/SphereCoverage.h>


namespace crusta {


/**
    Structure describing a source image patch
*/
template <typename PixelType>
class ImagePatch
{
public:
    /** pointer to object representing the image's pixel data, transformation,
        and coverage */
    ImageFile<PixelType>* image;
    /** transformation from the image's image coordinate system to the world
        coordinate system */
    ImageTransform* transform;
    ///image's coverage region in image coordinates
    ImageCoverage* imageCoverage;
    ///image's coverage region in world coordinates (destination)
    SphereCoverage* sphereCoverage;

    ImagePatch();
    ImagePatch(const std::string patchName,
               double pixelOffset, double pixelScale,
               const std::string& nodata, bool pointSampled);
    ~ImagePatch();
};


} //namespace crusta


#include <construo/ImagePatch.hpp>

#endif //_ImagePatch_H_
