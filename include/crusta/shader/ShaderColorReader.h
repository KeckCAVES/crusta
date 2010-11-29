#ifndef _ShaderColorReader_H_
#define _ShaderColorReader_H_


#include <vector>

#include <crusta/shader/ShaderAtlasDataSource.h>


BEGIN_CRUSTA


class ShaderColorReader : public ShaderDataSource
{
public:
    ShaderColorReader(ShaderDataSource* colorSource);
    
    virtual std::pair<std::string,std::string> getUniformsAndFunctionsCode();
    virtual void initUniforms(GLuint programObj);
    virtual void clearDefinitionsEmittedFlag();
    
private:
    ShaderDataSource* colorSrc;
};


END_CRUSTA


#endif //_ShaderColorReader_H_
