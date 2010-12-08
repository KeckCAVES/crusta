#ifndef _ShaderAtlasDataSource_H_
#define _ShaderAtlasDataSource_H_


#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


///\todo comment!!
class ShaderAtlasDataSourceBase : public ShaderDataSource
{
public:
    ShaderAtlasDataSourceBase(const std::string& samplerName);

protected:
    std::string texSamplerName;
    GLint texOffsetUniform;
    std::string texOffsetName;
    GLint texScaleUniform;
    std::string texScaleName;

//- inherited from ShaderFragment
public:
    virtual void initUniforms(GLuint programObj);
    virtual bool update();
};

class Shader1dAtlasDataSource : public ShaderAtlasDataSourceBase
{
public:
    Shader1dAtlasDataSource(const std::string& samplerName);

    void setSubRegion(const SubRegion& s);

protected:
    static bool samplingFunctionEmitted;

//- inherited from ShaderDataSource
public:
    virtual std::string sample(const std::string& params) const;

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual std::string getCode();
};

class Shader2dAtlasDataSource : public ShaderAtlasDataSourceBase
{
public:
    Shader2dAtlasDataSource(const std::string& samplerName);

    void setSubRegion(const SubRegion& s);

protected:
    static bool samplingFunctionEmitted;

//- inherited from ShaderDataSource
public:
    virtual std::string sample(const std::string& params) const;

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual std::string getCode();
};


END_CRUSTA


#endif //_ShaderAtlasDataSource_H_
