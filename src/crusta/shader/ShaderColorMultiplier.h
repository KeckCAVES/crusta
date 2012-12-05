#ifndef _Crusta_ShaderColorMultiplier_H_
#define _Crusta_ShaderColorMultiplier_H_


#include <vector>

#include <crusta/shader/ShaderMultiDataSource.h>


BEGIN_CRUSTA

///\todo abstract out the multi-source
class ShaderColorMultiplier : public ShaderMultiDataSource
{
//- inherited from ShaderFragment
public:
    virtual std::string getCode();
};


END_CRUSTA


#endif //_Crusta_ShaderColorMultiplier_H_
