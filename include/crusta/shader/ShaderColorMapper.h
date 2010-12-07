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
                      ShaderDataSource* scalarSource,
                      bool clampMap=false);

    const SubRegion& getColorMapRegion() const;
    void setScalarRange(float scalarMin, float scalarMax);

private:
    bool                    clamp;
    SubRegion               mapRegion;
    Shader1dAtlasDataSource colorSrc;
    ShaderDataSource*       scalarSrc;

    std::string scalarRangeName;
    GLint       scalarRangeUniform;

//- inherited from ShaderFragment
public:
    virtual std::string getUniforms();
    virtual std::string getFunctions();
    virtual void initUniforms(GLuint programObj);
    virtual void reset();
};


END_CRUSTA


#endif //_ShaderColorMapper_H_
