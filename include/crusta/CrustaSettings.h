#ifndef _CrustaSettings_H_
#define _CrustaSettings_H_


#include <string>

#include <crusta/basics.h>


BEGIN_CRUSTA


class CrustaSettings
{
public:
    CrustaSettings();

    void loadFromFile(std::string configurationFileName="", bool merge=true);

    /** name of the sphere onto which data is mapped */
    std::string globeName;
    /** radius of the sphere onto which data is mapped */
    double globeRadius;

    /** flags if line mapping should be rendered decorated or not */
    bool decorateVectorArt;

    ///\{ material properties of the terrain surface
    Color terrainAmbientColor;
    Color terrainDiffuseColor;
    Color terrainEmissiveColor;
    Color terrainSpecularColor;
    float terrainShininess;
    ///\}

    ///\{ cache settings
    int cacheMainNodeSize;
    int cacheMainGeometrySize;
    int cacheMainHeightSize;
    int cacheMainImagerySize;
    int cacheGpuGeometrySize;
    int cacheGpuHeightSize;
    int cacheGpuImagerySize;
    int cacheGpuCoverageSize;
    int cacheGpuLineDataSize;
    ///\}

    ///\{ data manager settings
    /** impose a limit on the number of outstanding fetch requests. This
        minimizes processing outdated requests */
    int dataManMaxFetchRequests;
    ///\}

    ///\{ decorated line settings
///\todo these depend on GL capabilities and should be wrapped in GLObjects
    int   lineDataTexSize;
    float lineDataCoordStep;
    float lineDataStartCoord;
    int   lineCoverageTexSize;
    ///\}
};


extern CrustaSettings* SETTINGS;


END_CRUSTA


#endif //_CrustaSettings_H_
