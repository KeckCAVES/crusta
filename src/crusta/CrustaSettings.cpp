#include <crusta/CrustaSettings.h>


#include <iostream>

#include <crusta/ResourceLocator.h>

#include <crusta/vrui.h>


namespace crusta {


CrustaSettings::CrustaSettings() :
    // /Crusta/Globe
    globeName("Sphere_Earth"),
    globeRadius(6371000.0),

    // /Crusta/Terrain
    terrainDefaultHeight(0.0f),
    terrainDefaultColor(0.5f, 0.5f, 0.5f, 1.0f),
    terrainDefaultLayerfData(0.0f),
    terrainAmbientColor(1.0f, 1.0f, 1.0f, 1.0f),
    terrainDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
    terrainEmissiveColor(0.0f, 0.0f, 0.0f, 1.0f),
    terrainSpecularColor(0.0f, 0.0f, 0.0f, 1.0f),
    terrainShininess(55.0f),

    // /Crusta/Cache
    cacheMainNodeSize(4096),
    cacheMainGeometrySize(4096),
    cacheMainColorSize(4096),
    cacheMainLayerfSize(16384),
    cacheGpuGeometrySize(1024),
    cacheGpuColorSize(1024),
    cacheGpuLayerfSize(4096),
    cacheGpuCoverageSize(1024),
    cacheGpuLineDataSize(1024),

    // /Crusta/DataManager
    dataManMaxDataLayers(32),
    dataManMaxFetchRequests(8),

    // /Crusta/ColorMapper
    colorMapTexSize(1024),

    // /Crusta/DecoratedArt
    lineDecorated(true),
    lineSymbolWidth(0.3f),
    lineSymbolLength(0.15f),
    lineDataTexSize(8192),
    lineDataCoordStep(1.0f / lineDataTexSize),
    lineDataStartCoord(0.5f * lineDataCoordStep),
    lineCoverageTexSize(TILE_RESOLUTION>>1),

    // /Crusta/SurfaceProjector
    surfaceProjectorRayIntersect(true),

    // /Crusta/SliceTool
    sliceToolEnable(false)
{
}

void CrustaSettings::
loadFromFiles(const Strings& cfgNames)
{
    Misc::ConfigurationFile cfgFile;

    //try to load the default system-wide configuration
    try
    {
        std::string cfgFileName = RESOURCELOCATOR.locateFile(
            "crusta.cfg");
        cfgFile.merge(cfgFileName.c_str());
    }
    catch (Misc::File::OpenError e) {
        //ignore this exception
    }
    catch (std::runtime_error e) {
        if (std::string("FileLocator::locateFile:").compare(0, 24, e.what()))
        {
            //ignore FileLocator failure
        }
        else
        {
            std::cout << "Caught exception " << e.what() << " while " <<
                         "reading the system-wide crustaSettings.cfg " <<
                         "configuration file." << std::endl;
        }
    }

    //try to load the default local configuration file
    try
    {
        cfgFile.merge("crustaSettings.cfg");
    }
    catch (Misc::File::OpenError e) {
        //ignore this exception
    }
    catch (std::runtime_error e) {
        std::cout << "Caught exception " << e.what() << " while reading " <<
                     "the local crustaSettings.cfg configuration file." <<
                     std::endl;
    }

    //try to load the specified configuration files
    for (Strings::const_iterator it=cfgNames.begin(); it!=cfgNames.end(); ++it)
    {
        try {
            cfgFile.merge(it->c_str());
        }
        catch (std::runtime_error e) {
            std::cout << "Caught exception " << e.what() << " while reading " <<
                         "the following configuration file: " << *it <<
                         std::endl;
        }
    }

    //try to extract the globe specifications
    cfgFile.setCurrentSection("/Crusta/Globe");
    globeName   = cfgFile.retrieveValue<std::string>("name", globeName);
    globeRadius = cfgFile.retrieveValue<double>("radius", globeRadius);

    //try to extract the terrain properties
    cfgFile.setCurrentSection("/Crusta/Terrain");
    terrainDefaultHeight = cfgFile.retrieveValue<float>(
        "defaultHeight", terrainDefaultHeight);
    terrainDefaultColor = cfgFile.retrieveValue<Color>(
        "defaultColor", terrainDefaultColor);
    terrainDefaultLayerfData = cfgFile.retrieveValue<float>(
        "defaultLayerfData", terrainDefaultLayerfData);
    terrainAmbientColor = cfgFile.retrieveValue<Color>(
        "ambientColor", terrainAmbientColor);
    terrainDiffuseColor = cfgFile.retrieveValue<Color>(
        "diffuseColor", terrainDiffuseColor);
    terrainEmissiveColor = cfgFile.retrieveValue<Color>(
        "emissiveColor", terrainEmissiveColor);
    terrainSpecularColor = cfgFile.retrieveValue<Color>(
        "specularColor", terrainSpecularColor);
    terrainShininess = cfgFile.retrieveValue<double>(
        "shininess", terrainShininess);

    //try to extract the cache settings
    cfgFile.setCurrentSection("/Crusta/Cache");
    cacheMainNodeSize = cfgFile.retrieveValue<int>(
        "mainNodeSize", cacheMainNodeSize);
    cacheMainGeometrySize = cfgFile.retrieveValue<int>(
        "mainGeometrySize", cacheMainGeometrySize);
    cacheMainColorSize = cfgFile.retrieveValue<int>(
        "mainColorSize", cacheMainColorSize);
    cacheMainLayerfSize = cfgFile.retrieveValue<int>(
        "mainLayerfSize", cacheMainLayerfSize);
    cacheGpuGeometrySize = cfgFile.retrieveValue<int>(
        "gpuGeometrySize", cacheGpuGeometrySize);
    cacheGpuColorSize = cfgFile.retrieveValue<int>(
        "gpuColorSize", cacheGpuColorSize);
    cacheGpuLayerfSize = cfgFile.retrieveValue<int>(
        "gpuLayerfSize", cacheGpuLayerfSize);
    cacheGpuCoverageSize = cfgFile.retrieveValue<int>(
        "gpuCoverageSize", cacheGpuCoverageSize);
    cacheGpuLineDataSize = cfgFile.retrieveValue<int>(
        "gpuLineDataSize", cacheGpuLineDataSize);

    //try to extract the data manager settings
    cfgFile.setCurrentSection("/Crusta/DataManager");
    dataManMaxDataLayers = cfgFile.retrieveValue<int>(
        "maxDataLayers", dataManMaxDataLayers);
    dataManMaxFetchRequests = cfgFile.retrieveValue<int>(
        "maxFetchRequests", dataManMaxFetchRequests);

    //try to extract the color mapper settings
    cfgFile.setCurrentSection("/Crusta/ColorMapper");
    colorMapTexSize = cfgFile.retrieveValue<int>("texSize", colorMapTexSize);

    //try to extract the line decoration settings
    cfgFile.setCurrentSection("/Crusta/DecoratedArt");
    lineDecorated = cfgFile.retrieveValue<bool>(
        "decorated", lineDecorated);
    lineSymbolWidth = cfgFile.retrieveValue<float>(
        "symbolWidth", lineSymbolWidth);
    lineSymbolLength = cfgFile.retrieveValue<float>(
        "symbolLength", lineSymbolLength);
    lineDataTexSize = cfgFile.retrieveValue<int>(
        "dataTexSize", lineDataTexSize);
    lineDataCoordStep  = 1.0f / lineDataTexSize;
    lineDataStartCoord = 0.5f * lineDataCoordStep;

    //try to extract the surface projector settings
    cfgFile.setCurrentSection("/Crusta/SurfaceProjector");
    surfaceProjectorRayIntersect = cfgFile.retrieveValue<bool>(
        "rayIntersect", surfaceProjectorRayIntersect);

    //try to extract the slice tool settings
    cfgFile.setCurrentSection("/Crusta/SliceTool");
    sliceToolEnable = cfgFile.retrieveValue<bool>("enable", sliceToolEnable);
}


CrustaSettings* SETTINGS;


} //namespace crusta
