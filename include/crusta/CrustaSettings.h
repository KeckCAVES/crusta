#ifndef _CrustaSettings_H_


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
    bool decoratedVectorArt;

    ///\{ material properties of the terrain surface
    Color terrainAmbientColor;
    Color terrainDiffuseColor;
    Color terrainEmissiveColor;
    Color terrainSpecularColor;
    float terrainShininess;
    ///\}
};


END_CRUSTA


#endif //_CrustaSettings_H_
