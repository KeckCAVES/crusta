#include <crusta/shader/ShaderAtlasDataSource.h>


#include <cassert>
#include <sstream>

#include <Misc/ThrowStdErr.h>

#include <crusta/checkGl.h>


namespace crusta {


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
CRUSTA_DEBUG(80, assert(texOffsetUniform>=0 && texScaleUniform>=0);)
    CHECK_GLA
}

bool ShaderAtlasDataSourceBase::
update()
{
    return false;
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
CRUSTA_DEBUG(80, assert(texOffsetUniform>=0 && texScaleUniform>=0);)
    if (texOffsetUniform >= 0)
        glUniform2fv(texOffsetUniform, 1, s.offset.getComponents());
    if (texScaleUniform >= 0)
        glUniform2fv(texScaleUniform, 1, s.size.getComponents());
    CHECK_GLA
}

std::string Shader1dAtlasDataSource::
sample(const std::string& params) const
{
    std::ostringstream oss;
    oss << "sample1dAtlas(" << texSamplerName << ", " << params << ", " <<
           texOffsetName << ", " << texScaleName << ")";
    return oss.str();
}

void Shader1dAtlasDataSource::
reset()
{
    ShaderDataSource::reset();
    texOffsetUniform = -2;
    texScaleUniform  = -2;
    samplingFunctionEmitted = false;
}

std::string Shader1dAtlasDataSource::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;

    if (!samplingFunctionEmitted)
    {
        code << "vec4 sample1dAtlas(in sampler2D sampler, in float tc, in vec2 offset, in vec2 scale) {" << std::endl;
        code << "   vec2 tc2 = offset + vec2(tc, 0.5) * scale;" << std::endl;
        code << "   return texture2D(sampler, tc2);" << std::endl;
        code << "}" << std::endl;
        code << std::endl;

        samplingFunctionEmitted = true;
    }

    code << "uniform vec2 " << texOffsetName << ";" << std::endl;
    code << "uniform vec2 " <<  texScaleName << ";" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
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
CHECK_GLA
CRUSTA_DEBUG(80, assert(texOffsetUniform>=0 && texScaleUniform>=0);)
    if (texOffsetUniform >= 0)
        glUniform3fv(texOffsetUniform, 1, s.offset.getComponents());
CHECK_GLA
    if (texScaleUniform >= 0)
        glUniform2fv(texScaleUniform, 1, s.size.getComponents());
    CHECK_GLA
}

std::string Shader2dAtlasDataSource::
sample(const std::string& params) const
{
    std::ostringstream oss;
    oss << "sample2dAtlas(" << texSamplerName << ", " << params << ", " <<
           texOffsetName << ", " << texScaleName << ")";
    return oss.str();
}

void Shader2dAtlasDataSource::
reset()
{
    ShaderDataSource::reset();
    texOffsetUniform = -2;
    texScaleUniform  = -2;
    samplingFunctionEmitted = false;
}

std::string Shader2dAtlasDataSource::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;
    if (!samplingFunctionEmitted)
    {
        code << "vec4 sample2dAtlas(in sampler2DArray sampler, in vec2 tc, in vec3 offset, in vec2 scale) {" << std::endl;
        code << "   vec3 tc2 = offset + vec3(tc * scale, 0.0);" << std::endl;
        code << "   return texture2DArray(sampler, tc2);" << std::endl;
        code << "}" << std::endl;
        code << std::endl;

        samplingFunctionEmitted = true;
    }

    code << "uniform vec3 " << texOffsetName << ";" << std::endl;
    code << "uniform vec2 " <<  texScaleName << ";" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
}


} //namespace crusta
