#include <crusta/shader/ShaderColorReader.h>

#include <cassert>
#include <sstream>

#include <crusta/checkGl.h>


BEGIN_CRUSTA


ShaderColorReader::
ShaderColorReader(ShaderDataSource* colorSource) :
    colorSrc(colorSource)
{
}

std::pair<std::string,std::string> ShaderColorReader::
getUniformsAndFunctionsCode()
{
    if (definitionsAlreadyEmitted)
        return std::make_pair("", "");
    
    //retrieve the uniforms and the code for the source
    std::pair<std::string, std::string> colorSrcUniforms(
        colorSrc->getUniformsAndFunctionsCode());
    
    std::ostringstream uniforms;
    uniforms << colorSrcUniforms.first;
    
    std::ostringstream code;
    code << colorSrcUniforms.second;

    code << "vec4 " << getSamplingFunctionName() << "(in vec2 coords) {" << std::endl;
    code << "  vec3 color = " << colorSrc->getSamplingFunctionName() << "(coords).rgb;" << std::endl;
    code << "  return color==colorNodata ? vec4(0.0) : vec4(color,1.0);" << std::endl;
    code << "}" << std::endl;
    
    definitionsAlreadyEmitted = true;
    return std::make_pair(uniforms.str(), code.str());
}

void ShaderColorReader::
initUniforms(GLuint programObj)
{
    colorSrc->initUniforms(programObj);
    CHECK_GLA
}

void ShaderColorReader::
clearDefinitionsEmittedFlag()
{
    colorSrc->clearDefinitionsEmittedFlag();
}


END_CRUSTA
