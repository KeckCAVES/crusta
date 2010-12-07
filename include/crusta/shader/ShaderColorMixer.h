#ifndef _ShaderColorMixer_H_
#define _ShaderColorMixer_H_


#include <crusta/shader/ShaderMultiDataSource.h>


BEGIN_CRUSTA


///\todo abstract out the multi-source
class ShaderColorMixer : public ShaderMultiDataSource
{
//- inherited from ShaderFragment
public:
    virtual std::string getFunctions();
};


END_CRUSTA


#endif //_ShaderColorMixer_H_
