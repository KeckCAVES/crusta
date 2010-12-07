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
    virtual std::string getUniforms();
    virtual std::string getFunctions();
    virtual void initUniforms(GLuint programObj);
    virtual void clearDefinitionsEmittedFlag();
    virtual void reset();
};


END_CRUSTA


#endif //_ShaderColorReader_H_
