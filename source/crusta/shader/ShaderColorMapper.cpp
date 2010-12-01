#include <crusta/shader/ShaderColorMapper.h>

#include <cassert>
#include <sstream>

#include <crusta/checkGl.h>
#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


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
    assert(scalarRangeUniform>=0);
    glUniform2f(scalarRangeUniform, scalarMin, 1.0f / (scalarMax-scalarMin));
    CHECK_GLA
}

std::pair<std::string,std::string> ShaderColorMapper::
getUniformsAndFunctionsCode()
{
    if (definitionsAlreadyEmitted)
        return std::make_pair("", "");

    //retrieve the uniforms and the code for the sources
    std::pair<std::string, std::string> colorSrcUniforms(
        colorSrc.getUniformsAndFunctionsCode());
    std::pair<std::string, std::string> scalarSrcUniforms(
        scalarSrc->getUniformsAndFunctionsCode());

    std::ostringstream uniforms;

    uniforms << colorSrcUniforms.first;
    uniforms << scalarSrcUniforms.first;

    uniforms << "uniform vec2 " << scalarRangeName << ";" << std::endl;

    std::ostringstream code;

    code << colorSrcUniforms.second;
    code << scalarSrcUniforms.second;

    code << "vec4 " << getSamplingFunctionName() << "(in vec2 coords) {" << std::endl;
    code << "  float scalar = " << scalarSrc->getSamplingFunctionName() << "(coords).x;" << std::endl;
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
    code << "  return " << colorSrc.getSamplingFunctionName() << "(scalar);" << std::endl;
    code << "}" << std::endl;

//std::cout << code.str() << std::endl;

    definitionsAlreadyEmitted = true;
    return std::make_pair(uniforms.str(), code.str());
}

void ShaderColorMapper::
initUniforms(GLuint programObj)
{
    colorSrc.initUniforms(programObj);
    scalarSrc->initUniforms(programObj);

    colorSrc.setSubRegion(mapRegion);

    scalarRangeUniform = glGetUniformLocation(programObj,
                                              scalarRangeName.c_str());
    assert (scalarRangeUniform>=0);
    CHECK_GLA
}

void ShaderColorMapper::
clearDefinitionsEmittedFlag()
{
    colorSrc.clearDefinitionsEmittedFlag();
    scalarSrc->clearDefinitionsEmittedFlag();
}


END_CRUSTA
