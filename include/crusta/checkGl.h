#ifndef _checkGl_H_
#define _checkGl_H_

#include <assert.h>
#include <stdio.h>

#include <GL/VruiGlew.h> //must be included instead of gl.h
#include <Misc/ThrowStdErr.h>

#include <crusta/basics.h>

BEGIN_CRUSTA

class CheckGlBase
{
public:
    static bool print(GLenum r, GLenum cond, int line, const char* file)
    {
        if (r != cond)
        {
            fprintf(stderr, "GLError: %s at %s:%d\n", gluErrorString(r), file,
                    line);
            fflush(stderr);
            return false;
        }
        return true;
    }
    static void except(GLenum r, GLenum cond, int line, const char* file)
    {
        if (r != cond)
        {
            Misc::throwStdErr("GLError: %s at %s:%d\n", gluErrorString(r), file,
                              line);
        }
    }
};

class CheckGl : public CheckGlBase
{
public:
    static bool print(int line, const char* file)
    {
        GLenum r = glGetError();
        return CheckGlBase::print(r, GL_NO_ERROR, line, file);
    }
    static void except(int line, const char* file)
    {
        GLenum r = glGetError();
        CheckGlBase::except(r, GL_NO_ERROR, line, file);
    }
};

class CheckGlFramebuffer : public CheckGlBase
{
public:
    static bool print(int line, const char* file)
    {
        GLenum r = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        return CheckGlBase::print(r, GL_FRAMEBUFFER_COMPLETE, line, file);
    }
    static void except(int line, const char* file)
    {
        GLenum r = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        CheckGlBase::except(r, GL_FRAMEBUFFER_COMPLETE, line, file);
    }
};

END_CRUSTA

#ifndef NDEBUG
#define CHECK_GL crusta::CheckGl::print(__LINE__, __FILE__)
#define CHECK_GLE crusta::CheckGl::except(__LINE__, __FILE__)
#define CHECK_GLA {bool r=crusta::CheckGl::print(__LINE__, __FILE__);assert(r);}
#define CHECK_GLFB crusta::CheckGlFramebuffer::print(__LINE__, __FILE__)
#define CHECK_GLFBE crusta::CheckGlFramebuffer::except(__LINE__, __FILE__)
#define CHECK_GLFBA {bool r=crusta::CheckGlFramebuffer::print(\
                     __LINE__, __FILE__); assert(r);}
#else
#define CHECK_GL
#define CHECK_GLE
#define CHECK_GLA
#define CHECK_GLFB
#define CHECK_GLFBE
#define CHECK_GLFBA
#endif //NDEBUG

#define CHECK_GL_CLEAR_ERROR glGetError()
#define CHECK_GL_THROW_ERROR crusta::CheckGl::except(__LINE__, __FILE__)
#define CHECK_GLFB_THROW_INCOMPLETE crusta::CheckGlFramebuffer::except(\
                                        __LINE__, __FILE__)


#endif //_checkGl_H_
