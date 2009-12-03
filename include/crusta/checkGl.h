#ifndef _checkGl_H_
#define _checkGl_H_

#include <assert.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <Misc/ThrowStdErr.h>

#include <crusta/basics.h>

BEGIN_CRUSTA

class CheckGl
{
public:
	static bool print(int line, const char* file)
	{
#ifndef NDEBUG
		GLenum r = glGetError();
		if (r != GL_NO_ERROR)
		{
			fprintf(stderr, "GLError: %s at %s:%d\n", gluErrorString(r), file,
                    line);
			fflush(stderr);
			return false;
		}
#endif //NDEBUG
		return true;
	}
	static void except(int line, const char* file)
	{
#ifndef NDEBUG
		GLenum r = glGetError();
		if (r != GL_NO_ERROR)
		{
			Misc::throwStdErr("GLError: %s at %s:%d\n", gluErrorString(r), file,
                              line);
		}
	}
#endif //NDEBUG
};

END_CRUSTA

#define CHECK_GL crusta::CheckGl::print(__LINE__, __FILE__);
#define CHECK_GLE crusta::CheckGl::except(__LINE__, __FILE__);
#define CHECK_GLA {bool r=crusta::CheckGl::print(__LINE__, __FILE__);assert(r);}

#endif //_checkGl_H_
