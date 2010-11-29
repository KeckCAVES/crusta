#include <crusta/shader/ShaderTopographySource.h>

#include <cassert>
#include <sstream>

#include <crusta/checkGl.h>


BEGIN_CRUSTA


ShaderTopographySource::
ShaderTopographySource(ShaderDataSource* _geometrySrc,
                       ShaderDataSource* _heightSrc) :
    geometrySrc(_geometrySrc), heightSrc(_heightSrc), centroidUniform(-2)
{
    centroidName = makeUniqueName("center");
}

void ShaderTopographySource::
setCentroid(const Point3f& c)
{
    assert(centroidUniform>=0);
    glUniform3fv(centroidUniform, 1, c.getComponents());
    CHECK_GLA
}

void ShaderTopographySource::
initUniforms(GLuint programObj)
{
    geometrySrc->initUniforms(programObj);
    heightSrc->initUniforms(programObj);

    centroidUniform = glGetUniformLocation(programObj, centroidName.c_str());
    assert(centroidUniform>=0);
    CHECK_GLA
}

std::pair<std::string, std::string> ShaderTopographySource::
getUniformsAndFunctionsCode()
{
    if (definitionsAlreadyEmitted)
        return std::make_pair("", "");

    std::pair<std::string, std::string> geomUF(
        geometrySrc->getUniformsAndFunctionsCode());
    std::pair<std::string, std::string> heightUF(
        heightSrc->getUniformsAndFunctionsCode());

    std::ostringstream uu;

    uu << geomUF.first;
    uu << heightUF.first;

    uu << "uniform vec3 " << centroidName << ";" << std::endl;

    std::ostringstream ss;

    ss << geomUF.second;
    ss << heightUF.second;

    ss << "vec3 " << getSamplingFunctionName() << "(in vec2 coords) {" << std::endl;
    ss << "  vec3 res      = " << geometrySrc->getSamplingFunctionName() << "(coords).xyz;" << std::endl;
    ss << "  vec3 dir      = normalize(" << centroidName << " + res);" << std::endl;
    ss << "  float height  = " << heightSrc->getSamplingFunctionName() << "(coords).x;" << std::endl;
    ss << "  height        = height==layerfNodata ? demDefault : height;" << std::endl;
    ss << "  height       *= verticalScale;" << std::endl;
    ss << "  res          += height * dir;" << std::endl;
    ss << "  return res;" << std::endl;
    ss << "}" << std::endl;

    definitionsAlreadyEmitted = true;

    return std::make_pair(uu.str(), ss.str());
}

void ShaderTopographySource::
clearDefinitionsEmittedFlag()
{
    geometrySrc->clearDefinitionsEmittedFlag();
    heightSrc->clearDefinitionsEmittedFlag();
    definitionsAlreadyEmitted = false;
}


END_CRUSTA
