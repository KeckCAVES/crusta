#ifndef _ResourceLocator_H_
#define _ResourceLocator_H_


#include <Misc/FileLocator.h>
#include <IO/Directory.h>

#include <crustacore/basics.h>


BEGIN_CRUSTA


class ResourceLocator : public Misc::FileLocator
{
public:
    void init(const std::string& exePath, const std::string& resourcePath);
};

extern ResourceLocator RESOURCELOCATOR;
extern IO::DirectoryPtr CURRENTDIRECTORY;

END_CRUSTA


#endif //_ResourceLocator_H_
