#ifndef _ConstruoSettings_H_


#include <string>

#include <crusta/basics.h>


BEGIN_CRUSTA


class ConstruoSettings
{
public:
    ConstruoSettings();

    void loadFromFile(std::string configurationFileName="", bool merge=true);

    /** name of the sphere onto which data is mapped */
    std::string globeName;
    /** radius of the sphere onto which data is mapped */
    double globeRadius;
};


END_CRUSTA


#endif //_ConstruoSettings_H_
