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



bool Shader1dAtlasDataSource::samplingFunctionEmitted = false;

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

std::string Shader1dAtlasDataSource::
sample(const std::string& params)
{
    std::ostringstream oss;
    oss << "sample1dAtlas(" << params << ", " <<
           texOffsetName << ", " << texScaleName << ")";
    return oss.str();
}

std::string Shader1dAtlasDataSource::
getUniforms()
{
    if (uniformsEmitted)
        return "";

    std::ostringstream uniforms;
    uniforms << "uniform vec2 " << texOffsetName << ";" << std::endl;
    uniforms << "uniform vec2 " <<  texScaleName << ";" << std::endl;

    uniformsEmitted = true;
    return uniforms.str();
}

std::string Shader1dAtlasDataSource::
getFunctions()
{
    if (samplingFunctionEmitted)
        return "";

    std::ostringstream functions;
    functions << "vec4 sample1dAtlas(in float tc, in vec2 offset, in vec2 scale) {" << std::endl;
    functions << "   vec2 tc2 = offset + vec2(tc, 0.5) * scale;" << std::endl;
    functions << "   return texture2D(" << texSamplerName << ", tc2);" << std::endl;
    functions << "}" << std::endl;
    
    samplingFunctionEmitted = true;
    return functions.str();
}

void Shader1dAtlasDataSource::
reset()
{
    ShaderDataSource::reset();
    samplingFunctionEmitted = false;
}


bool Shader2dAtlasDataSource::samplingFunctionEmitted = false;

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

std::string Shader2dAtlasDataSource::
sample(const std::string& params)
{
    std::ostringstream oss;
    oss << "sample2dAtlas(" << params << ", " <<
           texOffsetName << ", " << texScaleName << ")";
    return oss.str();
}

std::string Shader2dAtlasDataSource::
getUniforms()
{
    if (uniformsEmitted)
        return "";

    std::ostringstream uniforms;
    uniforms << "uniform vec3 " << texOffsetName << ";" << std::endl;
    uniforms << "uniform vec2 " <<  texScaleName << ";" << std::endl;

    uniformsEmitted = true;
    return uniforms.str();
}

std::string Shader2dAtlasDataSource::
getFunctions()
{
    if (samplingFunctionEmitted)
        return "";

    std::ostringstream functions;
    functions << "vec4 sample2dAtlas(in vec2 tc, in vec3 offset, in vec2 scale) {" << std::endl;
    functions << "   vec3 tc2 = offset + vec3(tc * scale, 0.0);" << std::endl;
    functions << "   return texture2DArray(" << texSamplerName << ", tc2);" << std::endl;
    functions << "}" << std::endl;

    samplingFunctionEmitted = true;
    return functions.str();
}

void Shader2dAtlasDataSource::
reset()
{
    ShaderDataSource::reset();
    samplingFunctionEmitted = false;
}


END_CRUSTA
