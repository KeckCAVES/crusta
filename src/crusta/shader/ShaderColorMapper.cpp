#include <crusta/shader/ShaderColorMapper.h>

#include <cassert>
#include <sstream>

#include <crusta/checkGl.h>
#include <crusta/CrustaSettings.h>


namespace crusta {


ShaderColorMapper::
ShaderColorMapper(const std::string& samplerName,
                  const SubRegion& colorMapRegion,
                  ShaderDataSource* scalarSource,
                  bool clampMap) :
    clamp(clampMap), mapRegion(colorMapRegion), colorSrc(samplerName),
    scalarSrc(scalarSource), scalarRangeUniform(-2)
{
    scalarRangeName = makeUniqueName("scalarRange");
}

const SubRegion& ShaderColorMapper::
getColorMapRegion() const
{
    return mapRegion;
}

void ShaderColorMapper::
setScalarRange(float scalarMin, float scalarMax)
{
CRUSTA_DEBUG(80, assert(scalarRangeUniform >= 0);)
    if (scalarRangeUniform >= 0)
        glUniform2f(scalarRangeUniform, scalarMin, 1.0f/(scalarMax-scalarMin));
    CHECK_GLA
}


void ShaderColorMapper::
reset()
{
    colorSrc.reset();
    scalarSrc->reset();
    ShaderDataSource::reset();
    scalarRangeUniform = -2;
}

void ShaderColorMapper::
initUniforms(GLuint programObj)
{
    colorSrc.initUniforms(programObj);
    scalarSrc->initUniforms(programObj);

    colorSrc.setSubRegion(mapRegion);

    scalarRangeUniform = glGetUniformLocation(programObj,
                                              scalarRangeName.c_str());
CRUSTA_DEBUG(80, assert(scalarRangeUniform>=0);)
    CHECK_GLA
}

bool ShaderColorMapper::
update()
{
    return false;
}

std::string ShaderColorMapper::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;
    code << colorSrc.getCode();
    code << scalarSrc->getCode();
    code << std::endl;

    code << "uniform vec2 " << scalarRangeName << ";" << std::endl;
    code << std::endl;

    code << "vec4 " << sample("in vec2 coords") <<  " {" << std::endl;
    code << "  float scalar = " << scalarSrc->sample("coords") << ".x;" << std::endl;
    code << "  if (scalar == layerfNodata)" << std::endl;
    code << "    return vec4(0.0);" << std::endl;
    code << "  scalar = (scalar-" << scalarRangeName << "[0]) * " << scalarRangeName << "[1];" << std::endl;
    if (clamp)
        code << "  scalar = clamp(scalar, 0.0, 1.0);" << std::endl;
///\todo this could probably be folded into the subregion somehow
float texStep  = 1.0f / SETTINGS->colorMapTexSize;
float texRange = 1.0f - texStep;
texStep       *= 0.5f;
    code << "  scalar = " << texStep << " + scalar * " << texRange << ";" << std::endl;
    code << "  return " << colorSrc.sample("scalar") << ";" << std::endl;
    code << "}" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
}


} //namespace crusta
