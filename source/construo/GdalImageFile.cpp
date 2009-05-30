#include <construo/GdalImageFile.h>

BEGIN_CRUSTA

bool GdalAllRegisteredCalled = false;

GdalDemImageFile::
GdalDemImageFile(const char* imageFileName) :
    Base(imageFileName)
{
}

void GdalDemImageFile::
readRectangle(const int rectOrigin[2], const int rectSize[2],
              Pixel* rectBuffer) const
{
    //retrieve raster bands from the data set
    int numBands = dataset->GetRasterCount();
    if (numBands < 1)
        Misc::throwStdErr("GdalDemImageFile: no raster bands in the file");\

    GDALRasterBand* band = dataset->GetRasterBand(1);

    //clip the rectangle against the image's valid region
    int min[2], max[2];
    for (int i=0; i<2; ++i)
    {
///\todo remove
assert(rectOrigin[i]>=0);
        min[i] = std::max(0,       rectOrigin[i]);
///\todo remove, oh but also fix the max[i]
assert(rectOrigin[i]+rectSize[i]-1 < size[i]);
        max[i] = std::min(size[i], rectOrigin[i]+rectSize[i]);
    }

    int readSize[2] = { max[0] - min[0], max[1] - min[1] };
    int rowWidth  = rectSize[0] * sizeof(Pixel);
    Pixel* dst = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                (min[0]-rectOrigin[0]) );

    band->RasterIO(GF_Read, min[0], min[1], readSize[0], readSize[1],
                   dst, readSize[0], readSize[1], GDT_Float32,
                   sizeof(Pixel), rowWidth);

    //scale the pixel values
    if (pixelScale != 1.0)
    {
        for (Pixel* p=rectBuffer; p<rectBuffer + rectSize[0]*rectSize[1]; ++p)
            *p = pixelScale * (*p);
    }
}



GdalColorImageFile::
GdalColorImageFile(const char* imageFileName) :
    Base(imageFileName)
{
}

void GdalColorImageFile::
readRectangle(const int rectOrigin[2], const int rectSize[2],
              Pixel* rectBuffer) const
{
    //retrieve raster bands from the data set
    int numBands = dataset->GetRasterCount();
    if (numBands < 1)
        Misc::throwStdErr("GdalColorImageFile: no raster bands in the file");\

    GDALRasterBand* bands[3];
    bands[0] = dataset->GetRasterBand(1);
    bands[1] = numBands<2 ?bands[0] :dataset->GetRasterBand(2);
    bands[2] = numBands<3 ?bands[1] :dataset->GetRasterBand(3);

    //clip the rectangle against the image's valid region
    int min[2], max[2];
    for (int i=0; i<2; ++i)
    {
///\todo remove
assert(rectOrigin[i]>=0);
        min[i] = std::max(0,       rectOrigin[i]);
///\todo remove, oh but also fix the max[i]
assert(rectOrigin[i]+rectSize[i]-1 < size[i]);
        max[i] = std::min(size[i], rectOrigin[i]+rectSize[i]);
    }

    int readSize[2] = { max[0] - min[0], max[1] - min[1] };
    int rowWidth  = rectSize[0] * sizeof(Pixel);
    Pixel* dst = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                (min[0]-rectOrigin[0]) );

    for (int i=0; i<3; ++i)
    {
        bands[i]->RasterIO(GF_Read, min[0], min[1], readSize[0], readSize[1],
                           &dst[0][i], readSize[0], readSize[1], GDT_Byte,
                           sizeof(Pixel), rowWidth);
    }

    //scale the pixel values
    if (pixelScale != 1.0)
    {
        for (Pixel* p=rectBuffer; p<rectBuffer + rectSize[0]*rectSize[1]; ++p)
            *p = pixelScale * (*p);
    }
}


END_CRUSTA
