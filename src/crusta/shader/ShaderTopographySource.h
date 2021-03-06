#ifndef _ShaderTopographySource_H_
#define _ShaderTopographySource_H_


#include <crusta/shader/ShaderDataSource.h>


namespace crusta {


class ShaderTopographySource : public ShaderDataSource
{
public:
    ShaderTopographySource(ShaderDataSource* _geometrySrc,
                           ShaderDataSource* _heightSrc);

    void setCentroid(const Geometry::Point<float,3>& c);

private:
    ShaderDataSource* geometrySrc;
    ShaderDataSource* heightSrc;

    GLint centroidUniform;
    std::string centroidName;

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual void initUniforms(GLuint programObj);
    virtual bool update();
    virtual std::string getCode();
};


} //namespace crusta //_ShaderTopographySource_H_


#endif
