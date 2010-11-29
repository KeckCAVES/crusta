#ifndef _ShaderDataSource_H_
#define _ShaderDataSource_H_


#include <vector>

#include <crusta/shader/ShaderFragment.h>


BEGIN_CRUSTA


///\todo comment!!
class ShaderDataSource : public ShaderFragment
{
public:
    std::string getSamplingFunctionName() const;
};


END_CRUSTA


#endif //_ShaderDataSource_H_
