#ifndef _VruiGlew_H_
#define _VruiGlew_H_


#include <GL/glew.h>
#include <GL/GLObject.h>
#include <GL/TLSHelper.h>


class GLContextData;

/** convenience class to add GLEW support to a VRUI applications. Any glew
    dependent calls should be made after enable() has been called */
class VruiGlew : public GLObject
{
public:
    VruiGlew();

    /** sets up the GLEW context. Must be called before making any extension
        GL calls */
    static void enable(GLContextData& contextData);

    /** retrieve the GLEW context for the active thread */
    static GLEWContext* getContext();

private:
    /** stores the GLEW context in the VRUI GL context data */
    struct Item : public GLObject::DataItem
    {
        GLEWContext glewContext;
    };

    /** pointer to the glew per thread global context */
    static GL_THREAD_LOCAL(VruiGlew) singleton;

    /** the current glew context */
    GLEWContext* glewContext;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};

#define glewGetContext VruiGlew::getContext


#endif //_VruiGlew_H_
