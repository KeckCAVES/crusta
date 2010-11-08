#ifndef _GlewObject_H_
#define _GlewObject_H_


#include <GL/glew.h>
#include <GL/GLObject.h>
#include <GL/TLSHelper.h>


class GLContextData;

/** convenience class to add GLEW support to a VRUI application. You application
    should inherit from GlewObject and make sure to call the enableGlew() during
    the display call */
class GlewObject : public GLObject
{
public:
    GlewObject();

    /** sets up the GLEW context. Must be called before making any extension
        GL calls */
    static void enableGlew(GLContextData& contextData);

    /** retrieve the GLEW context for the active thread */
    static GLEWContext* glewGetContext();
    
private:
    /** stores the GLEW context in the VRUI GL context data */
    struct Item : public GLObject::DataItem
    {
        GLEWContext glewContext;
    };

    /** pointer to the glew per thread global context */
    static GL_THREAD_LOCAL(GlewObject) glewObjectInstance;

    GLEWContext* glewContext;
//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};

#define glewGetContext GlewObject::glewGetContext


#endif //_GlewObject_H_
