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

void ShaderMultiDataSource::
initUniforms(GLuint programObj)
{
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        (*it)->initUniforms(programObj);
    }
}

bool ShaderMultiDataSource::
update()
{
    bool ret = false;
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        ret |= (*it)->update();
    }
    return ret;
}

std::string ShaderMultiDataSource::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        code << (*it)->getCode();
        code << std::endl;
    }

    codeEmitted = true;
    return code.str();
}


END_CRUSTA
