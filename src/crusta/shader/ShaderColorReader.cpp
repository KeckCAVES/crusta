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

void ShaderColorReader::
reset()
{
    ShaderDataSource::reset();
    colorSrc->reset();
}

void ShaderColorReader::
initUniforms(GLuint programObj)
{
    colorSrc->initUniforms(programObj);
    CHECK_GLA
}

bool ShaderColorReader::
update()
{
    return false;
}

std::string ShaderColorReader::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;
    code << colorSrc->getCode();
    code << std::endl;

    code << "vec4 " << sample("in vec2 coords") << " {" << std::endl;
    code << "  vec3 color = " << colorSrc->sample("coords") << ".rgb;" << std::endl;
    code << "  return color==colorNodata ? vec4(0.0) : vec4(color,1.0);" << std::endl;
    code << "}" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
}


END_CRUSTA
