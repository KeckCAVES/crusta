#ifndef _ShaderTopographySource_H_
#define _ShaderTopographySource_H_


#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


class ShaderTopographySource : public ShaderDataSource
{
public:
    ShaderTopographySource(ShaderDataSource* _geometrySrc,
                           ShaderDataSource* _heightSrc);

    void setCentroid(const Point3f& c);

private:
    ShaderDataSource* geometrySrc;
    ShaderDataSource* heightSrc;

    GLint centroidUniform;
    std::string centroidName;

//- inherited from ShaderFragment
public:
    virtual std::string getUniforms();
    virtual std::string getFunctions();
    virtual void initUniforms(GLuint programObj);
    virtual void reset();
};


END_CRUSTA //_ShaderTopographySource_H_


#endif
