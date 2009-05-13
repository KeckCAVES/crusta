#include <construo/ImageFileLoader.h>
#include <construo/ImageTransformReader.h>

BEGIN_CRUSTA

template <typename PixelParam>
ImagePatch<PixelParam>::
ImagePatch() :
    image(NULL), transform(NULL), imageCoverage(NULL), sphereCoverage(NULL)
{}

template <typename PixelParam>
ImagePatch<PixelParam>::
ImagePatch(const std::string patchName) :
    image(NULL), transform(NULL), imageCoverage(NULL), sphereCoverage(NULL)
{
	//load the image file
	image = ImageFileLoader<PixelParam>::loadImageFile(patchName.c_str());
    const uint* imgSize = image->getSize();
	
	//remove the extension from the image file name
    size_t dotPos = patchName.rfind('.');
    std::string baseName(patchName, 0, dotPos);
	
	//read the image's projection file
	std::string projName(baseName);
	projName.append("proj");
	transform = ImageTransformReader::open(projName.c_str());
	
	try
    {
		//read the image's coverage file:
		std::string covName(baseName);
		covName.append("cov");
		imageCoverage = new ImageCoverage(covName.c_str());
    }
	catch(std::runtime_error err)
    {
		//some error occurred; create a default coverage region:
		imageCoverage = new ImageCoverage(4);
		imageCoverage->setVertex(0, Point(0,0));
		imageCoverage->setVertex(1, Point(imgSize[0]-1, 0));
		imageCoverage->setVertex(2, Point(imgSize[0]-1, imgSize[1]-1));
		imageCoverage->setVertex(3, Point(0, imgSize[1]-1));
		imageCoverage->finalizeVertices();
    }
	
	//flip the image transformation and coverage:
	transform->flip(imgSize[1]);
	imageCoverage->flip(imgSize[1]);
	
	//compute the image's coverage on the spheroid:
    sphereCoverage = new StaticSphereCoverage(8, imageCoverage, transform);
}

template <typename PixelParam>
ImagePatch<PixelParam>::
~ImagePatch()
{
    delete image;
    delete transform;
    delete imageCoverage;
    delete sphereCoverage;
}


END_CRUSTA
