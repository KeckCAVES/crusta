#ifndef _CompositeShader_H_
#define _CompositeShader_H_


#include <GL/GLShader.h>

#include <crusta/basics.h>


BEGIN_CRUSTA


class CompositeShader : public GLShader
{
public:
    CompositeShader();
    ~CompositeShader();

    void apply();

protected:
    GLint rcpWindowSizeUniform;
};


END_CRUSTA


#endif //_CompositeShader_H_
