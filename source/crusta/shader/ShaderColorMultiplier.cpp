#include <crusta/shader/ShaderColorMultiplier.h>


#include <sstream>

#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


void ShaderColorMultiplier::
clear()
{
    sources.clear();
}

void ShaderColorMultiplier::
addSource(ShaderDataSource *src)
{
    sources.push_back(src);
}

std::pair<std::string, std::string> ShaderColorMultiplier::
getUniformsAndFunctionsCode()
{
    if (definitionsAlreadyEmitted)
        return std::make_pair("", "");

    std::ostringstream uniforms;
    std::ostringstream code;

    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        std::pair<std::string, std::string> uniformAndFunctionsCode(
            (*it)->getUniformsAndFunctionsCode());

        uniforms << uniformAndFunctionsCode.first;
        code     << uniformAndFunctionsCode.second;
    }

    code << "vec4 " << getSamplingFunctionName() << "(in vec2 tc) {" << std::endl;
    code << "  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);" << std::endl;

    for (size_t i=0; i < sources.size(); ++i)
    {
        std::string funcName = sources[i]->getSamplingFunctionName();
        std::string resultName = funcName + "_result";

        code << "  vec4 " << resultName << " = "<< funcName << "(tc);" << std::endl;
        code << "  color *= " << resultName << ";" << std::endl;
    }

    code << "  return color;" << std::endl;
    code << "}" << std::endl;

    definitionsAlreadyEmitted = true;
    return std::make_pair(uniforms.str(), code.str());
}

void ShaderColorMultiplier::
initUniforms(GLuint programObj)
{
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        (*it)->initUniforms(programObj);
    }
}

void ShaderColorMultiplier::
clearDefinitionsEmittedFlag()
{
    ShaderFragment::clearDefinitionsEmittedFlag();

    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        (*it)->clearDefinitionsEmittedFlag();
    }
}


END_CRUSTA
