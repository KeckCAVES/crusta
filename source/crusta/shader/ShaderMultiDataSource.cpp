#include <crusta/shader/ShaderMultiDataSource.h>


#include <sstream>


BEGIN_CRUSTA


void ShaderMultiDataSource::
clear()
{
    sources.clear();
}

void ShaderMultiDataSource::
addSource(ShaderDataSource *src)
{
    sources.push_back(src);
}

std::string ShaderMultiDataSource::
getUniforms()
{
    if (uniformsEmitted)
        return "";
    
    std::ostringstream uniforms;
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        uniforms << (*it)->getUniforms();
    }
        
    uniformsEmitted = true;
    return uniforms.str();
}

std::string ShaderMultiDataSource::
getFunctions()
{
    if (functionsEmitted)
        return "";
    
    std::ostringstream functions;
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        functions << (*it)->getFunctions();
    }
    
    functionsEmitted = true;
    return functions.str();
}

void ShaderMultiDataSource::
initUniforms(GLuint programObj)
{
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        (*it)->initUniforms(programObj);
    }
}

void ShaderMultiDataSource::
reset()
{
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        (*it)->reset();
    }

    ShaderDataSource::reset();
}


END_CRUSTA
