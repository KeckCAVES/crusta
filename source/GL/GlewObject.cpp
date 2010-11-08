#include <GL/GlewObject.h>


#include <GL/GLContextData.h>
#include <Misc/ThrowStdErr.h>


GL_THREAD_LOCAL(GlewObject) GlewObject::glewObjectInstance;

GlewObject::
GlewObject() :
    glewContext(NULL)
{
}

void GlewObject::
enableGlew(GLContextData& contextData)
{
    glewObjectInstance.glewContext =
        &contextData.retrieveDataItem<Item>(&glewObjectInstance)->glewContext;
}

GLEWContext* GlewObject::
glewGetContext()
{
    return glewObjectInstance.glewContext;
}

void GlewObject::
initContext(GLContextData& contextData) const
{
    Item* item = new Item;
    contextData.addDataItem(this, item);
    enableGlew(contextData);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        Misc::throwStdErr("GlewObject: Error %s, while initializing GLEW",
                          glewGetErrorString(err));
    }
}
