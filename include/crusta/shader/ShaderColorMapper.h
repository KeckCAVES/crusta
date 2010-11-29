#ifndef _ShaderColorMapper_H_
#define _ShaderColorMapper_H_


#include <vector>

#include <crusta/shader/ShaderAtlasDataSource.h>


BEGIN_CRUSTA


class ShaderColorMapper : public ShaderDataSource
{
public:
    ShaderColorMapper(const std::string& samplerName,
                      const SubRegion& colorMapRegion,
                      ShaderDataSource* scalarSource);

    const SubRegion& getColorMapRegion() const;
    void setScalarRange(float scalarMin, float scalarMax);

    virtual std::pair<std::string,std::string> getUniformsAndFunctionsCode();
    virtual void initUniforms(GLuint programObj);
    virtual void clearDefinitionsEmittedFlag();

private:
    SubRegion               mapRegion;
    Shader1dAtlasDataSource colorSrc;
    ShaderDataSource*       scalarSrc;

    std::string scalarRangeName;
    GLint       scalarRangeUniform;
};


END_CRUSTA


#endif //_ShaderColorMapper_H_
