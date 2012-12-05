#include <GL/VruiGlew.h>


#include <GL/GLContextData.h>
#include <Misc/ThrowStdErr.h>


GL_THREAD_LOCAL(VruiGlew) VruiGlew::singleton;

VruiGlew::
VruiGlew() :
    glewContext(NULL)
{
}

void VruiGlew::
enable(GLContextData& contextData)
{
    singleton.glewContext =
        &contextData.retrieveDataItem<Item>(&singleton)->glewContext;
}

GLEWContext* VruiGlew::
getContext()
{
    return singleton.glewContext;
}

void VruiGlew::
initContext(GLContextData& contextData) const
{
    Item* item = new Item;
    contextData.addDataItem(this, item);
    enable(contextData);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        Misc::throwStdErr("VruiGlew: Error %s, while initializing GLEW",
                          glewGetErrorString(err));
    }
}
