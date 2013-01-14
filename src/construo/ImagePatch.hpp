#include <construo/ImageFileLoader.h>
#include <construo/GdalTransform.h>

#include <crustacore/Vector3ui8.h>


BEGIN_CRUSTA


template <typename PixelType>
ImagePatch<PixelType>::
ImagePatch() :
    image(NULL), transform(NULL), imageCoverage(NULL), sphereCoverage(NULL)
{}

template <typename PixelType>
ImagePatch<PixelType>::
ImagePatch(const std::string patchName,
           double pixelOffset, double pixelScale,
           const std::string& nodataString, bool pointSampled) :
    image(NULL), transform(NULL), imageCoverage(NULL), sphereCoverage(NULL)
{
    //load the image file
    image = ImageFileLoader<PixelType>::loadImageFile(patchName.c_str());
    image->setPixelOffset(pixelOffset);
    image->setPixelScale(pixelScale);
    if (!nodataString.empty())
    {
        std::istringstream iss(nodataString);
        PixelType nodata;
        iss >> nodata;
        image->setNodata(nodata);
        std::cout << "Forced nodata value:\n" << nodataString << "\n\n";
    }
    const int* imgSize = image->getSize();

    //remove the extension from the image file name
    size_t dotPos = patchName.rfind('.');
    std::string baseName(patchName, 0, dotPos);

    //read the image's projection file
    std::string projName(baseName);
    projName.append(".proj");
    transform = new GdalTransform(projName.c_str());
    transform->setPointSampled(pointSampled, imgSize);

    try
    {
        //read the image's coverage file:
        std::string covName(baseName);
        covName.append(".cov");
        imageCoverage = new ImageCoverage(covName.c_str());

sphereCoverage = new StaticSphereCoverage(3, imageCoverage, transform);
    }
    catch(std::runtime_error err)
    {
///\todo fix this hack
if (!pointSampled)
{
    //some error occurred; create a default coverage region:
    ImageCoverage sphereImageCoverage(4);
    sphereImageCoverage.setVertex(0, Point(-0.5,-0.5));
    sphereImageCoverage.setVertex(1, Point(imgSize[0]-0.5, -0.5));
    sphereImageCoverage.setVertex(2, Point(imgSize[0]-0.5, imgSize[1]-0.5));
    sphereImageCoverage.setVertex(3, Point(-0.5, imgSize[1]-0.5));
    sphereImageCoverage.finalizeVertices();
sphereCoverage = new StaticSphereCoverage(3, &sphereImageCoverage, transform);
        imageCoverage = new ImageCoverage(4);
        imageCoverage->setVertex(0, Point(-0.1,-0.1));
        imageCoverage->setVertex(1, Point(imgSize[0]-1+0.1, -0.1));
        imageCoverage->setVertex(2, Point(imgSize[0]-1+0.1, imgSize[1]-1+0.1));
        imageCoverage->setVertex(3, Point(-0.1, imgSize[1]-1+0.1));
        imageCoverage->finalizeVertices();
}
else
{
        //some error occurred; create a default coverage region:
        imageCoverage = new ImageCoverage(4);
        imageCoverage->setVertex(0, Point(0,0));
        imageCoverage->setVertex(1, Point(imgSize[0]-1, 0));
        imageCoverage->setVertex(2, Point(imgSize[0]-1, imgSize[1]-1));
        imageCoverage->setVertex(3, Point(0, imgSize[1]-1));
        imageCoverage->finalizeVertices();
sphereCoverage = new StaticSphereCoverage(3, imageCoverage, transform);
}
    }

    //compute the image's coverage on the spheroid:
//    sphereCoverage = new StaticSphereCoverage(3, imageCoverage, transform);
}

template <typename PixelType>
ImagePatch<PixelType>::
~ImagePatch()
{
    delete image;
    delete transform;
    delete imageCoverage;
    delete sphereCoverage;
}


END_CRUSTA
