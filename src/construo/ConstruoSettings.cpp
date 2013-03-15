#include <construo/ConstruoSettings.h>


#include <iostream>

#include <Misc/ConfigurationFile.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>


namespace crusta {


ConstruoSettings::
ConstruoSettings() :
    globeName("Sphere_Earth"), globeRadius(6371000.0)
{
}

void ConstruoSettings::
loadFromFile(std::string configurationFileName, bool merge)
{
    Misc::ConfigurationFile cfgFile;

    //try to load the specified configuration file
    if (!configurationFileName.empty())
    {
        try {
            cfgFile.load(configurationFileName.c_str());
        }
        catch (std::runtime_error e) {
            std::cout << "Caught exception " << e.what() << " while reading " <<
                         "the following configuration file: " <<
                         configurationFileName << std::endl;
        }
    }

    //try to merge the local edits if requested
    if (merge)
    {
        try {
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
    }

    //try to extract the pertinent information from the configuration file
    cfgFile.setCurrentSection("/Crusta/Globe");
    globeName   = cfgFile.retrieveString("./name", globeName);
    globeRadius = cfgFile.retrieveValue<double>("./radius", globeRadius);
}

} //namespace crusta
