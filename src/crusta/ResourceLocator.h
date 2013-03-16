#ifndef _ResourceLocator_H_
#define _ResourceLocator_H_


#include <crustacore/basics.h>

#include <crusta/vrui.h>


namespace crusta {


class ResourceLocator : public Misc::FileLocator
{
public:
    void init(const std::string& exePath, const std::string& resourcePath);
};

extern ResourceLocator RESOURCELOCATOR;
extern IO::DirectoryPtr CURRENTDIRECTORY;

} //namespace crusta


#endif //_ResourceLocator_H_
