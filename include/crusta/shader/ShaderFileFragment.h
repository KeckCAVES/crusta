#ifndef _Crusta_ShaderFileFragment_H_
#define _Crusta_ShaderFileFragment_H_


#include <crusta/shader/ShaderFragment.h>


BEGIN_CRUSTA


class ShaderFileFragment : public ShaderFragment
{
public:
    
//- inherited from ShaderFragment
public:
    virtual std::string getUniforms();
    virtual std::string getFunctions();
    virtual void initUniforms(GLuint programObj);
    virtual void clearDefinitionsEmittedFlag();
};


END_CRUSTA


#endif //_Crusta_ShaderFileFragment_H_
