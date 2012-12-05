#ifndef _ShaderColorReader_H_
#define _ShaderColorReader_H_


#include <vector>

#include <crusta/shader/ShaderAtlasDataSource.h>


BEGIN_CRUSTA


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


END_CRUSTA


#endif //_ShaderColorReader_H_
