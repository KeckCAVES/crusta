#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


CrustaSettings::CrustaSettings() :
    decoratedVectorArt(false),
    terrainAmbientColor(0.4f, 0.4f, 0.4f, 1.0f),
    terrainDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
    terrainEmissiveColor(0.0f, 0.0f, 0.0f, 1.0f),
    terrainSpecularColor(0.3f, 0.3f, 0.3f, 1.0f),
    terrainShininess(55.0f)
{
}

void CrustaSettings::loadFromFile(std::string configurationFileName)
{
}

END_CRUSTA
