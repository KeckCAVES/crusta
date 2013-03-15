#ifndef _ShaderDataSource_H_
#define _ShaderDataSource_H_


#include <vector>

#include <crusta/shader/ShaderFragment.h>


namespace crusta {


///\todo comment!!
class ShaderDataSource : public ShaderFragment
{
public:
    virtual std::string sample(const std::string& params) const;
};


} //namespace crusta


#endif //_ShaderDataSource_H_
