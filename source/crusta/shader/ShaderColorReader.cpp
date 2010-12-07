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

std::string ShaderColorReader::
getUniforms()
{
    if (uniformsEmitted)
        return "";
    
    std::ostringstream uniforms;
    uniforms << colorSrc->getUniforms();
    
    uniformsEmitted = true;
    return uniforms.str();
}

std::string ShaderColorReader::
getFunctions()
{
    if (functionsEmitted)
        return "";
    
    std::ostringstream functions;
    functions << colorSrc->getFunctions();
    
    functions << "vec4 " << sample("in vec2 coords") << " {" << std::endl;
    functions << "  vec3 color = " << colorSrc->sample("coords") << ".rgb;" << std::endl;
    functions << "  return color==colorNodata ? vec4(0.0) : vec4(color,1.0);" << std::endl;
    functions << "}" << std::endl;
    
    functionsEmitted = true;
    return functions.str();
}    

void ShaderColorReader::
initUniforms(GLuint programObj)
{
    colorSrc->initUniforms(programObj);
    CHECK_GLA
}

void ShaderColorReader::
reset()
{
    ShaderDataSource::reset();
    colorSrc->reset();
}


END_CRUSTA
