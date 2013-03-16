#include <crusta/GlProgram.h>

#include <fstream>
#include <stdexcept>

#include <crusta/vrui.h>


GlProgram::
GlProgram() :
    program(0), previousProgram(0)
{
    if (!glewIsSupported("GL_ARB_shader_objects"))
        Misc::throwStdErr("GLProgram: GL_ARB_shader_objects not supported");

	program = glCreateProgramObjectARB();
}

GlProgram::
~GlProgram()
{
    clear();
}

GlProgram::ShaderHandle GlProgram::
addFile(GLenum shaderType, const std::string& sourceFile)
{
	GLhandleARB shader = 0;
	try
    {
		//create a new vertex shader
		shader = glCreateShaderObjectARB(shaderType);
		
		//load and compile the shader source code
        std::string source = load(sourceFile);
		compile(shader, source);
		
		//store the shader for linking
		shaders.push_back(shader);
        return --shaders.end();
    }
	catch(std::runtime_error err)
    {
		//delete the shader
		if (shader != 0)
			glDeleteObjectARB(shader);
		
		//throw the error message
		Misc::throwStdErr("GLProgram::addFile: Error \"%s\" while compiling "
                          "shader %s", err.what(), sourceFile.c_str());
    }
    //we should never get here
    return shaders.end();
}

GlProgram::ShaderHandle GlProgram::
addString(GLenum shaderType, const std::string& sourceString)
{
	GLhandleARB shader = 0;
	try
    {
		//create a new vertex shader
		shader = glCreateShaderObjectARB(shaderType);
		
		//compile the shader source code
		compile(shader, sourceString);
		
		//store the shader for linking
		shaders.push_back(shader);
        return --shaders.end();
    }
	catch(std::runtime_error err)
    {
		//delete the shader
		if (shader != 0)
			glDeleteObjectARB(shader);
		
		//throw the error message
		Misc::throwStdErr("GLProgram::addFile: Error \"%s\" while compiling "
                          "shader %s", err.what());
    }
    //we should never get here
    return shaders.end();
}

void GlProgram::
removeShader(ShaderHandle& handle)
{
    //remove the shader from the program
    glDetachObjectARB(program, *handle);
    //delete the shader
    glDeleteObjectARB(*handle);
    shaders.erase(handle);
    //invalidate the handle
    handle = shaders.end();
}

void GlProgram::
clear()
{
    //detach all the shaders from the program
    for (ShaderHandle it=shaders.begin(); it!=shaders.end(); ++it)
        glDetachObjectARB(program, *it);
    //delete the program
    glDeleteObjectARB(program);
    
    //delete all the shaders
    for (ShaderHandle it=shaders.begin(); it!=shaders.end(); ++it)
        glDeleteObjectARB(*it);
    shaders.clear();
}


void GlProgram::
link()
{
	//attach all previously compiled shaders to the program object
	for(ShaderHandle it=shaders.begin(); it!=shaders.end(); ++it)
		glAttachObjectARB(program, *it);
	
	//link the program
	glLinkProgramARB(program);
	
	//check if the program linked successfully
	GLint linkStatus;
	glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linkStatus);
	if (!linkStatus)
    {
		//get some more detailed information
		GLcharARB linkLogBuffer[2048];
		GLsizei linkLogSize;
		glGetInfoLogARB(program, sizeof(linkLogBuffer), &linkLogSize,
                        linkLogBuffer);
		
		//signal an error
		Misc::throwStdErr("GLProgram::link: Error \"%s\" while linking shader "
                          "program", linkLogBuffer);
    }
}


int GlProgram::
getUniformLocation(const std::string& uniformName) const
{
	return glGetUniformLocationARB(program, uniformName.c_str());
}

int GlProgram::
getAttribLocation(const std::string& attribName) const
{
    return glGetAttribLocation(program, attribName.c_str());
}

void GlProgram::
push()
{
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glUseProgramObjectARB(program);
}

void GlProgram::
pop()
{
    glUseProgramObjectARB(previousProgram);
    previousProgram = 0;
}


std::string GlProgram::
load(const std::string& sourceFile) const
{
    std::ifstream file(sourceFile.c_str());
    return std::string((std::istreambuf_iterator<char>(file)),
                       (std::istreambuf_iterator<char>()));
}

void GlProgram::
compile(GLhandleARB shader, const std::string& sourceString) const
{
	//determine the length of the source code
	GLint length = static_cast<GLint>(sourceString.size());
	
	//upload the shader source into the shader object
	const GLcharARB* code =
        reinterpret_cast<const GLcharARB*>(sourceString.c_str());
	glShaderSourceARB(shader, 1, &code, &length);
	
	//compile the shader source
	glCompileShaderARB(shader);
	
	//check if the shader compiled successfully
	GLint compileStatus;
	glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB,
                              &compileStatus);
	if(!compileStatus)
    {
		//get some more detailed information
		GLcharARB compileLogBuffer[2048];
		GLsizei compileLogSize;
		glGetInfoLogARB(shader, sizeof(compileLogBuffer), &compileLogSize,
                        compileLogBuffer);
		
		//signal an error
		Misc::throwStdErr("%s", compileLogBuffer);
    }
}

GlProgram::
GlProgram(const GlProgram& source)
{
}

GlProgram& GlProgram::
operator=(const GlProgram& source)
{
    return *this;
}
