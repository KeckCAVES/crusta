#ifndef _ShaderColorMultiplier_H_
#define _ShaderColorMultiplier_H_


#include <vector>

#include <crusta/shader/ShaderAtlasDataSource.h>


BEGIN_CRUSTA

///\todo abstract out the multi-source
class ShaderColorMultiplier : public ShaderDataSource
{
public:
    void clear();
    void addSource(ShaderDataSource* src);

    virtual std::pair<std::string,std::string> getUniformsAndFunctionsCode();
    virtual void initUniforms(GLuint programObj);
    virtual void clearDefinitionsEmittedFlag();

private:
    std::vector<ShaderDataSource*> sources;
};


END_CRUSTA


#endif //_ShaderColorMultiplier_H_
