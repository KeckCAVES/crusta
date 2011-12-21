#include <crusta/ResourceLocator.h>


#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


void ResourceLocator::
init(const std::string& exePath, const std::string& resourcePath)
{
    if (!resourcePath.empty())
        addPath(resourcePath);
    addPathFromApplication(exePath);
}

ResourceLocator RESOURCELOCATOR;
IO::DirectoryPtr CURRENTDIRECTORY;


END_CRUSTA
