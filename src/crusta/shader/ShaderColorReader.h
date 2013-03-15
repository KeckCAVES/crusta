#ifndef _ShaderColorReader_H_
#define _ShaderColorReader_H_


#include <vector>

#include <crusta/shader/ShaderAtlasDataSource.h>


namespace crusta {


class ShaderColorReader : public ShaderDataSource
{
public:
    ShaderColorReader(ShaderDataSource* colorSource);

private:
    ShaderDataSource* colorSrc;

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual void initUniforms(GLuint programObj);
    virtual bool update();
    virtual std::string getCode();
};


} //namespace crusta


#endif //_ShaderColorReader_H_
