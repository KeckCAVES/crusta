#include <crusta/shader/ShaderFragment.h>


#include <sstream>


namespace crusta {


int ShaderFragment::nextSequenceNumber = 0;

ShaderFragment::
ShaderFragment() :
    codeEmitted(false), sequenceNumber(nextSequenceNumber++)
{
}

ShaderFragment::
~ShaderFragment()
{
}

void ShaderFragment::
reset()
{
    codeEmitted  = false;
}

std::string ShaderFragment::
makeUniqueName(const std::string& baseName) const
{
    std::ostringstream generatedName;
    generatedName << baseName << "_" << sequenceNumber;

    return generatedName.str();
}


} //namespace crusta
