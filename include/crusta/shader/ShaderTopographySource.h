#ifndef _ShaderTopographySource_H_
#define _ShaderTopographySource_H_


#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


class ShaderTopographySource : public ShaderDataSource
{
public:
    ShaderTopographySource(ShaderDataSource* _geometrySrc,
                           ShaderDataSource* _heightSrc);

    virtual std::pair<std::string, std::string> getUniformsAndFunctionsCode();
    virtual void initUniforms(GLuint programObj);

    void setCentroid(const Point3f& c);

    void clearDefinitionsEmittedFlag();

private:
    ShaderDataSource* geometrySrc;
    ShaderDataSource* heightSrc;

    GLint centroidUniform;
    std::string centroidName;
};


END_CRUSTA //_ShaderTopographySource_H_


#endif
