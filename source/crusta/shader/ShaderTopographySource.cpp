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

std::string ShaderTopographySource::
getUniforms()
{
    if (uniformsEmitted)
        return "";

    std::ostringstream uniforms;

    uniforms << geometrySrc->getUniforms();
    uniforms << heightSrc->getUniforms();

    uniforms << "uniform vec3 " << centroidName << ";" << std::endl;

    uniformsEmitted = true;
    return uniforms.str();
}

std::string ShaderTopographySource::
getFunctions()
{
    if (functionsEmitted)
        return "";
    
    std::ostringstream functions;
    
    functions << geometrySrc->getFunctions();
    functions << heightSrc->getFunctions();
    
    functions << "vec3 " << sample("in vec2 coords") << " {" << std::endl;
    functions << "  vec3 res      = " << geometrySrc->sample("coords") << ".xyz;" << std::endl;
    functions << "  vec3 dir      = normalize(" << centroidName << " + res);" << std::endl;
    functions << "  float height  = " << heightSrc->sample("coords") << ".x;" << std::endl;
    functions << "  height        = height==layerfNodata ? demDefault : height;" << std::endl;
    functions << "  height       *= verticalScale;" << std::endl;
    functions << "  res          += height * dir;" << std::endl;
    functions << "  return res;" << std::endl;
    functions << "}" << std::endl;
    
    functionsEmitted = true;
    return functions.str();
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

void ShaderTopographySource::
reset()
{
    ShaderDataSource::reset();
    geometrySrc->reset();
    heightSrc->reset();
}


END_CRUSTA
