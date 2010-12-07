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

std::string ShaderColorMapper::
getUniforms()
{
    if (uniformsEmitted)
        return "";

    std::ostringstream uniforms;
    uniforms << colorSrc.getUniforms();
    uniforms << scalarSrcUniforms->getUniforms();

    uniforms << "uniform vec2 " << scalarRangeName << ";" << std::endl;

    uniformsEmitted = true;
    return uniforms.str();
}

std::string ShaderColorMapper::
getFunctions()
{
    if (functionsEmitted)
        return "";

    std::ostringstream functions;
    functions << colorSrc.getUniforms();
    functions << scalarSrc->getUniforms();
    
    functions << "vec4 " << sample("in vec2 coords") <<  " {" << std::endl;
    functions << "  float scalar = " << scalarSrc->sample("coords") << ".x;" << std::endl;
    functions << "  if (scalar == layerfNodata)" << std::endl;
    functions << "    return vec4(0.0);" << std::endl;
    functions << "  scalar = (scalar-" << scalarRangeName << "[0]) * " << scalarRangeName << "[1];" << std::endl;
    if (clamp)
        functions << "  scalar = clamp(scalar, 0.0, 1.0);" << std::endl;
///\todo this could probably be folded into the subregion somehow
float texStep  = 1.0f / SETTINGS->colorMapTexSize;
float texRange = 1.0f - texStep;
texStep       *= 0.5f;
    functions << "  scalar = " << texStep << " + scalar * " << texRange << ";" << std::endl;
    functions << "  return " << colorSrc.sample("scalar") << ";" << std::endl;
    functions << "}" << std::endl;
    
    functionsEmitted = true;
    return functions.str();
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
reset()
{
    colorSrc.reset();
    scalarSrc->reset();
    ShaderDataSource::reset();
}


END_CRUSTA
