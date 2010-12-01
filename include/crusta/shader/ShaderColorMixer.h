#ifndef _ShaderColorMixer_H_
#define _ShaderColorMixer_H_


#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


///\todo abstract out the multi-source
class ShaderColorMixer : public ShaderDataSource
{
public:
    void clear();
    void addSource(ShaderDataSource* src);

    virtual std::pair<std::string, std::string> getUniformsAndFunctionsCode();
    virtual void initUniforms(GLuint programObj);
    virtual void clearDefinitionsEmittedFlag();

private:
    std::vector<ShaderDataSource*> sources;
};


END_CRUSTA


#endif //_ShaderColorMixer_H_
