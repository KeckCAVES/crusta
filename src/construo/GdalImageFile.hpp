#include <Misc/File.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <crustacore/Vector3ui8.h>


BEGIN_CRUSTA


template <typename PixelType>
GdalImageFileBase<PixelType>::
GdalImageFileBase(const std::string& imageFileName)
{
    if (!GdalAllRegisteredCalled)
    {
        GDALAllRegister();
        GdalAllRegisteredCalled = true;
    }

    dataset = (GDALDataset*)GDALOpen(imageFileName.c_str(), GA_ReadOnly);
    if (dataset == NULL)
    {
        Misc::throwStdErr("GdalImageFileBase: could not open %s",
                          imageFileName.c_str());
    }

    this->size[0] = dataset->GetRasterXSize();
    this->size[1] = dataset->GetRasterYSize();

    //dump the projection if there isn't one already
    double geoXform[6];
    char projection[1024];
    bool hasGeoXform = false;

    //read the image's projection file
    std::string fileName(imageFileName);
    size_t dotPos = fileName.rfind('.');
    std::string baseName(fileName, 0, dotPos);
    std::string projectionFileName(baseName);
    projectionFileName.append(".proj");

    try
    {
        Misc::File projectionFile(projectionFileName.c_str(), "r");

        char line[1024];
        while (projectionFile.gets(line, sizeof(line)))
        {
            if (strcasecmp(line, "Geotransform:\n")==0)
            {
                /* Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
                   Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
                   (see GDAL documentation) */
                projectionFile.gets(line, sizeof(line));
                std::istringstream iss(line);
                for (int i=0; i<6; ++i)
                    iss >> geoXform[i];
                hasGeoXform = true;
            }
            else if (strcasecmp(line, "Projection:\n")==0)
            {
                //read in the projection
                projectionFile.gets(projection, sizeof(projection));
            }
        }
    }
    catch (Misc::File::OpenError openError)
    {
        Misc::File projectionFile(projectionFileName.c_str(),"wt");
        hasGeoXform = dataset->GetGeoTransform(geoXform) == CE_None;
        if (hasGeoXform)
        {
            projectionFile.puts("Geotransform:\n");
            std::ostringstream oss;
            for (int i=0; i<6; ++i)
            {
                oss << std::setprecision(std::numeric_limits<double>::digits10)
                    << geoXform[i] << " ";
            }
            oss << std::endl;
            projectionFile.puts(oss.str().c_str());
        }
        else
            projectionFile.puts("NO_GEOTRANSFORM\n");

        strcpy(projection, dataset->GetProjectionRef());
        projectionFile.puts("\nProjection:\n");
        projectionFile.puts(projection);
    }

    std::cout << "Loading Source " << imageFileName << "..." << std::endl
              << "Driver: " << dataset->GetDriver()->GetDescription() << " - "
              << dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME )
              << std::endl << "Size: " << this->size[0] << "x"
              << this->size[1] << "  Bands: " << dataset->GetRasterCount()
              << std::endl << "Coordinate System:" << std::endl
              << projection << std::endl;
    if (hasGeoXform)
    {
        std::cout << "Origin = (" << geoXform[0] << ", " << geoXform[3] << ")"
                  << std::endl << "Pixel Size = (" << geoXform[1] << ", "
                  << geoXform[5] << ")" << std::endl;
    }
    else
    {
        std::cout << "Source has no geo-transform. Perhaps ground control "
                  << "points?" << std::endl;
    }
}

template <typename PixelType>
GdalImageFileBase<PixelType>::
~GdalImageFileBase()
{
    if (dataset != NULL)
        GDALClose((GDALDatasetH)dataset);
}


//- single channel float -------------------------------------------------------

template <>
class GdalImageFile<float> : public GdalImageFileBase<float>
{
public:
    typedef GdalImageFileBase<float> Base;

    GdalImageFile(const std::string& imageFileName) :
        Base(imageFileName)
    {
        //retrieve raster bands from the data set
        int numBands = dataset->GetRasterCount();
        if (numBands < 1)
            Misc::throwStdErr("GdalImageFile:DEM: no raster bands in the file");

        GDALRasterBand* band = dataset->GetRasterBand(1);

        //try to retrieve the nodata value from the band
        nodata = float(band->GetNoDataValue());

        //output the no-data value
        std::cout << "Internal nodata value:\n" << nodata << "\n";
    }


public:
    virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               float* rectBuffer) const
    {
        //retrieve raster bands from the data set
        int numBands = dataset->GetRasterCount();
        if (numBands < 1)
            Misc::throwStdErr("GdalImageFile:DEM: no raster bands in the file");

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
        int rowWidth  = rectSize[0] * sizeof(float);
        float* dst = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                        (min[0]-rectOrigin[0]) );

        band->RasterIO(GF_Read, min[0], min[1], readSize[0], readSize[1],
                       dst, readSize[0], readSize[1], GDT_Float32,
                       sizeof(float), rowWidth);

        //scale the pixel values
        if (pixelOffset!=0.0 || pixelScale!=1.0)
        {
            for (float* p=rectBuffer; p < rectBuffer + rectSize[0]*rectSize[1];
                 ++p)
            {
                if (*p != nodata)
                    *p = pixelOffset + pixelScale * (*p);
            }
        }
    }

};


//- 3-channel unit8 ------------------------------------------------------------

template <>
class GdalImageFile<Vector3ui8> : public GdalImageFileBase<Vector3ui8>
{
public:
    typedef GdalImageFileBase<Vector3ui8> Base;

    GdalImageFile(const std::string& imageFileName) :
        Base(imageFileName)
    {
        //retrieve raster bands from the data set
        int numBands = dataset->GetRasterCount();
        if (numBands < 1)
        {
            Misc::throwStdErr("GdalImageFile:Color: no raster bands in "
                              "the file");
        }

        std::vector<GDALRasterBand*> bands;
        bands.push_back(dataset->GetRasterBand(1));

        for (int i=1; i<Vector3ui8::dimension; ++i)
        {
            bands.push_back(numBands<i+1 ? bands[i-1] :
                                           dataset->GetRasterBand(i+1));
        }

        for (int i=0; i<Vector3ui8::dimension; ++i)
            nodata[i] = Vector3ui8::Scalar(bands[i]->GetNoDataValue());

        //output the no-data value
        std::cout << "Internal nodata value:\n(" << nodata << ")\n";
    }


public:
    virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Vector3ui8* rectBuffer) const
    {
        //retrieve raster bands from the data set
        int numBands = dataset->GetRasterCount();
        if (numBands < 1)
        {
            Misc::throwStdErr("GdalImageFile:Color: no raster bands in "
                              "the file");
        }

        std::vector<GDALRasterBand*> bands;
        bands.push_back(dataset->GetRasterBand(1));

        for (int i=1; i<Vector3ui8::dimension; ++i)
        {
            bands.push_back(numBands<i+1 ? bands[i-1] :
                                           dataset->GetRasterBand(i+1));
        }

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
        int rowWidth  = rectSize[0] * sizeof(Vector3ui8);
        Vector3ui8* dst = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                         (min[0]-rectOrigin[0]) );

        for (int i=0; i<Vector3ui8::dimension; ++i)
        {
            bands[i]->RasterIO(GF_Read, min[0], min[1], readSize[0],readSize[1],
                               &dst[0][i], readSize[0], readSize[1], GDT_Byte,
                               sizeof(Vector3ui8), rowWidth);
        }

        //scale the pixel values
        if (pixelOffset!=0.0 || pixelScale!=1.0)
        {
            for (Vector3ui8* p=rectBuffer;
                 p < (rectBuffer + rectSize[0]*rectSize[1]); ++p)
            {
                if (*p != nodata)
                    *p = pixelOffset + pixelScale * (*p);
            }
        }
    }

};


END_CRUSTA
