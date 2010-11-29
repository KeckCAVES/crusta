#include <crusta/shader/ShaderColorMixer.h>


#include <sstream>

#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


void ShaderColorMixer::
clear()
{
    sources.clear();
}

void ShaderColorMixer::
addSource(ShaderDataSource *src)
{
    sources.push_back(src);
}

std::pair<std::string, std::string> ShaderColorMixer::
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
    const Color& defaultColor = SETTINGS->terrainDefaultColor;
    code << "  vec4 color = vec4(" << std::scientific << defaultColor[0] << ", " << defaultColor[1] << ", " << defaultColor[2] << ", " << defaultColor[3] << ");" << std::endl;

    for (size_t i=0; i < sources.size(); ++i)
    {
        std::string funcName = sources[i]->getSamplingFunctionName();
        std::string resultName = funcName + "_result";

        code << "  vec4 " << resultName << " = "<< funcName << "(tc);" << std::endl;
        code << "  color.rgb = mix(color.rgb, " << resultName << ".rgb, " << resultName << ".a);" << std::endl;
//        code << "  color.rgb += " << resultName << ".rgb * " << resultName << ".a;" << std::endl;
    }

    code << "  return color;" << std::endl;
    code << "}" << std::endl;

    definitionsAlreadyEmitted = true;
    return std::make_pair(uniforms.str(), code.str());
}

void ShaderColorMixer::
initUniforms(GLuint programObj)
{
    for (std::vector<ShaderDataSource*>::iterator it=sources.begin();
         it!=sources.end(); ++it)
    {
        (*it)->initUniforms(programObj);
    }
}

void ShaderColorMixer::
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
