#ifndef _GlProgram_H_
#define _GlProgram_H_


#include <list>
#include <string>

#include <GL/VruiGlew.h>


class GlProgram
{
public:
    /** type for list of shader object handles */
    typedef std::list<GLhandleARB> ShaderList;
    /** type for a handle to an added shader */
    typedef ShaderList::iterator ShaderHandle;

    /** creates an "empty" shader */
    GlProgram();
    /** destroys a shader */
    ~GlProgram();

    /** loads the shader source from the specified file and compiles it into the
        specified shader component (OpenGL shader type enum) */
    ShaderHandle addFile(GLenum shaderType, const std::string& sourceFile);
    /** compiles the specified shader source into the specified shader component
        (OpenGL shader type enum) */
    ShaderHandle addString(GLenum shaderType, const std::string& sourceString);
    /** remove a previously added shader component */
    void removeShader(ShaderHandle& handle);
    /** reset the shader to the initial empty/invalid state */
    void clear();

    /** links all previously loaded shaders into a shader program */
    void link();

    /** returns the index of a uniform variable defined in the shader program */
    int getUniformLocation(const std::string& uniformName) const;
    /** returns the index of an attribute defined in the shader program */
    int getAttribLocation(const std::string& attribName) const;

    /** installs the shader program in the current OpenGL context and records
        the previously active program.
        \note: only supports a single push/pop. Stack depth = 1 */
    void push();
    /** restores the program that was active before the push */
    void pop();

protected:
     /** loads a shader's source code from a file and returns it as a string */
    std::string load(const std::string& sourceFile) const;
    /** compiles a shader's source code into an existing shader object */
    void compile(GLhandleARB shader, const std::string& sourceString) const;

    /** list of handles of compiled shader objects */
    ShaderList shaders;
    /** handle for the shader program */
    GLhandleARB program;
    /** index of the previous program recorded at push */
    GLint previousProgram;

private:
    /** prohibit copy constructor */
    GlProgram(const GlProgram& source);
    /** prohibit assignment operator */
    GlProgram& operator=(const GlProgram& source);
};

#endif //_GlProgram_H_
