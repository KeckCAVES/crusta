#include <crusta/ResourceLocator.h>


#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


void ResourceLocator::
init(const std::string& exePath, const std::string& resourcePath)
{
    if (!resourcePath.empty())
        addPath(resourcePath);
    addPathFromApplication(exePath);
    addPath(CRUSTA_ETC_PATH);
    addPath(CRUSTA_SHARE_PATH);
}

ResourceLocator RESOURCELOCATOR;
IO::DirectoryPtr CURRENTDIRECTORY;


END_CRUSTA
