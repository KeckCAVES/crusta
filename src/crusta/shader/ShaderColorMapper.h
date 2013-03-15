#ifndef _ShaderColorMapper_H_
#define _ShaderColorMapper_H_


#include <vector>

#include <crusta/shader/ShaderAtlasDataSource.h>


namespace crusta {


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
    virtual void reset();
    virtual void initUniforms(GLuint programObj);
    virtual bool update();
    virtual std::string getCode();
};


} //namespace crusta


#endif //_ShaderColorMapper_H_
