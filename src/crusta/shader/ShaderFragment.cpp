#include <crusta/shader/ShaderFragment.h>


#include <sstream>


BEGIN_CRUSTA


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


END_CRUSTA
