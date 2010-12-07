#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


std::string ShaderDataSource::
sample(const std::string& params) const
{
    return makeUniqueName("sample") + "(" + params + ")";
}


END_CRUSTA
