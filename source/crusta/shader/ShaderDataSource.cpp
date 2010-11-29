#include <crusta/shader/ShaderDataSource.h>


BEGIN_CRUSTA


std::string ShaderDataSource::
getSamplingFunctionName() const
{
    return makeUniqueName("sampleFunc");
}


END_CRUSTA
