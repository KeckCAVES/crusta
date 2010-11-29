#ifndef _ShaderAtlasDataSource_H_
#define _ShaderAtlasDataSource_H_


#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


///\todo comment!!
class ShaderAtlasDataSourceBase : public ShaderDataSource
{
public:
    ShaderAtlasDataSourceBase(const std::string& samplerName);

    virtual void setSubRegion(const SubRegion& s) = 0;

    virtual void initUniforms(GLuint programObj);

protected:
    std::string texSamplerName;
    GLint texOffsetUniform;
    std::string texOffsetName;
    GLint texScaleUniform;
    std::string texScaleName;
};

class Shader1dAtlasDataSource : public ShaderAtlasDataSourceBase
{
public:
    Shader1dAtlasDataSource(const std::string& samplerName);

    void setSubRegion(const SubRegion& s);

    virtual std::pair<std::string, std::string> getUniformsAndFunctionsCode();
};

class Shader2dAtlasDataSource : public ShaderAtlasDataSourceBase
{
public:
    Shader2dAtlasDataSource(const std::string& samplerName);

    void setSubRegion(const SubRegion& s);

    virtual std::pair<std::string, std::string> getUniformsAndFunctionsCode();
};


END_CRUSTA


#endif //_ShaderAtlasDataSource_H_
