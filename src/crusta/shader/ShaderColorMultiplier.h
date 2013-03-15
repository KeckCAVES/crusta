#ifndef _Crusta_ShaderColorMultiplier_H_
#define _Crusta_ShaderColorMultiplier_H_


#include <vector>

#include <crusta/shader/ShaderMultiDataSource.h>


namespace crusta {

///\todo abstract out the multi-source
class ShaderColorMultiplier : public ShaderMultiDataSource
{
//- inherited from ShaderFragment
public:
    virtual std::string getCode();
};


} //namespace crusta


#endif //_Crusta_ShaderColorMultiplier_H_
