#ifndef _GlProgram_H_
#define _GlProgram_H_


#include <GL/GLShader.h>


class GlProgram : public GLShader
{
public:
    int getAttribLocation(const char* attribName) const;
};


#endif //_GlProgram_H_
