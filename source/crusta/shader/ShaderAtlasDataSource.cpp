#include <crusta/shader/ShaderAtlasDataSource.h>


#include <cassert>
#include <sstream>

#include <Misc/ThrowStdErr.h>

#include <crusta/checkGl.h>


BEGIN_CRUSTA


ShaderAtlasDataSourceBase::
ShaderAtlasDataSourceBase(const std::string& samplerName) :
    texSamplerName(samplerName), texOffsetUniform(-2), texScaleUniform(-2)
{
    texOffsetName = makeUniqueName("texOffset");
    texScaleName  = makeUniqueName("texScale");
}

void ShaderAtlasDataSourceBase::
initUniforms(GLuint programObj)
{
    texOffsetUniform = glGetUniformLocation(programObj, texOffsetName.c_str());
    texScaleUniform  = glGetUniformLocation(programObj,  texScaleName.c_str());
    assert(texOffsetUniform>=0 && texScaleUniform>=0);
    CHECK_GLA
}




Shader1dAtlasDataSource::
Shader1dAtlasDataSource(const std::string& samplerName) :
    ShaderAtlasDataSourceBase(samplerName)
{
}

void Shader1dAtlasDataSource::
setSubRegion(const SubRegion& s)
{
    assert(texOffsetUniform>=0 && texScaleUniform>=0);
    glUniform2fv(texOffsetUniform, 1, s.offset.getComponents());
    glUniform2fv( texScaleUniform, 1,   s.size.getComponents());
    CHECK_GLA
}

std::pair<std::string, std::string> Shader1dAtlasDataSource::
getUniformsAndFunctionsCode()
{
    if (definitionsAlreadyEmitted)
        return std::make_pair("", "");

    std::ostringstream uniforms;
    uniforms << "uniform vec2 " << texOffsetName << ";" << std::endl;
    uniforms << "uniform vec2 " <<  texScaleName << ";" << std::endl;


    std::ostringstream functions;

    functions << "vec4 " << getSamplingFunctionName() << "(in float tc) {" << std::endl;
    functions << "   vec2 tc2 = " << texOffsetName << " + vec2(tc, 0.5) * " << texScaleName << ";" << std::endl;
    functions << "   return texture2D(" << texSamplerName << ", tc2);" << std::endl;
    functions << "}" << std::endl;

    definitionsAlreadyEmitted = true;
    return std::make_pair(uniforms.str(), functions.str());
}




Shader2dAtlasDataSource::
Shader2dAtlasDataSource(const std::string& samplerName) :
    ShaderAtlasDataSourceBase(samplerName)
{
}


void Shader2dAtlasDataSource::
setSubRegion(const SubRegion& s)
{
//std::cout << "offset & scale uniform = " << texOffsetsAndScalesUniform << std::endl;
    assert(texOffsetUniform>=0 && texScaleUniform>=0);
    glUniform3fv(texOffsetUniform, 1, s.offset.getComponents());
    glUniform2fv( texScaleUniform, 1,   s.size.getComponents());
    CHECK_GLA
}

std::pair<std::string, std::string> Shader2dAtlasDataSource::
getUniformsAndFunctionsCode()
{
    if (definitionsAlreadyEmitted)
        return std::make_pair("", "");

    std::ostringstream uniforms;
    uniforms << "uniform vec3 " << texOffsetName << ";" << std::endl;
    uniforms << "uniform vec2 " <<  texScaleName << ";" << std::endl;


    std::ostringstream functions;

    functions << "vec4 " << getSamplingFunctionName() << "(in vec2 tc) {" << std::endl;
    functions << "   vec3 tc2 = " << texOffsetName << " + vec3(tc * " << texScaleName << ", 0.0);" << std::endl;
    functions << "   return texture2DArray(" << texSamplerName << ", tc2);" << std::endl;
    functions << "}" << std::endl;

    definitionsAlreadyEmitted = true;
    return std::make_pair(uniforms.str(), functions.str());
}


END_CRUSTA
