#include <crusta/shader/ShaderFragment.h>


#include <sstream>


BEGIN_CRUSTA


int ShaderFragment::nextSequenceNumber = 0;

ShaderFragment::
ShaderFragment() :
    definitionsAlreadyEmitted(false), sequenceNumber(nextSequenceNumber++)
{
}

ShaderFragment::
~ShaderFragment()
{
}

std::string ShaderFragment::
makeUniqueName(const std::string& baseName) const
{
    std::ostringstream generatedName;
    generatedName << baseName << "_" << sequenceNumber;

    return generatedName.str();
}

void ShaderFragment::
clearDefinitionsEmittedFlag()
{
    definitionsAlreadyEmitted = false;
}


END_CRUSTA
