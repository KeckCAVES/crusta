#include <crusta/GlProgram.h>


#include <Misc/ThrowStdErr.h>


int GlProgram::
getAttribLocation(const char* attribName) const
{
    if (programObject == 0)
    {
        Misc::throwStdErr("GlProgram::getAttribLocation: Attempt to get "
                          "attribute location while program not in use");
    }

    return glGetAttribLocation(programObject, attribName);
}

