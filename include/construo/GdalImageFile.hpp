#include <Misc/File.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

BEGIN_CRUSTA

template <typename PixelParam>
GdalImageFileBase<PixelParam>::
GdalImageFileBase(const char* imageFileName)
{
    if (!GdalAllRegisteredCalled)
    {
        GDALAllRegister();
        GdalAllRegisteredCalled = true;
    }

    dataset = (GDALDataset*)GDALOpen(imageFileName, GA_ReadOnly);
    if (dataset == NULL)
        Misc::throwStdErr("GdalImageFileBase: could not open %s",imageFileName);

    ImageBase::size[0] = dataset->GetRasterXSize();
    ImageBase::size[1] = dataset->GetRasterYSize();

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
              << std::endl << "Size: " << ImageBase::size[0] << "x"
              << ImageBase::size[1] << "  Bands: " << dataset->GetRasterCount()
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

template <typename PixelParam>
GdalImageFileBase<PixelParam>::
~GdalImageFileBase()
{
    if (dataset != NULL)
        GDALClose((GDALDatasetH)dataset);
}

END_CRUSTA
