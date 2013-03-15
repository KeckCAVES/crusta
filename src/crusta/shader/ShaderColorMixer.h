#ifndef _ShaderColorMixer_H_
#define _ShaderColorMixer_H_


#include <crusta/shader/ShaderMultiDataSource.h>


namespace crusta {


///\todo abstract out the multi-source
class ShaderColorMixer : public ShaderMultiDataSource
{
//- inherited from ShaderFragment
public:
    virtual std::string getCode();
};


} //namespace crusta


#endif //_ShaderColorMixer_H_
