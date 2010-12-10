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
///\todo this is dangerous. Other shaders assume the existance of this uniform
    centroidName = "center";
}

void ShaderTopographySource::
setCentroid(const Point3f& c)
{
    assert(centroidUniform>=0);
    glUniform3fv(centroidUniform, 1, c.getComponents());
    CHECK_GLA
}

void ShaderTopographySource::
reset()
{
    geometrySrc->reset();
    heightSrc->reset();
    ShaderDataSource::reset();
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

bool ShaderTopographySource::
update()
{
    return false;
}

std::string ShaderTopographySource::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;

    code << geometrySrc->getCode();
    code << heightSrc->getCode();
    code << std::endl;

    code << "uniform vec3 " << centroidName << ";" << std::endl;
    code << std::endl;

    code << "vec3 " << sample("in vec2 coords") << " {" << std::endl;
    code << "  vec3 res      = " << geometrySrc->sample("coords") << ".xyz;" << std::endl;
    code << "  vec3 dir      = normalize(" << centroidName << " + res);" << std::endl;
    code << "  float height  = " << heightSrc->sample("coords") << ".x;" << std::endl;
    code << "  height        = height==layerfNodata ? demDefault : height;" << std::endl;
    code << "  height       *= verticalScale;" << std::endl;
    code << "  res          += height * dir;" << std::endl;
    code << "  return res;" << std::endl;
    code << "}" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
}


END_CRUSTA
