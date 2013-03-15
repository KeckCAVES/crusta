#include <crusta/shader/ShaderDataSource.h>


namespace crusta {


std::string ShaderDataSource::
sample(const std::string& params) const
{
    return makeUniqueName("sample") + "(" + params + ")";
}


} //namespace crusta
