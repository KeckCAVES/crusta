#include <crusta/shader/ShaderFragment.h>


#include <sstream>


BEGIN_CRUSTA


int ShaderFragment::nextSequenceNumber = 0;

ShaderFragment::
ShaderFragment() :
    uniformsEmitted(false), functionsEmitted(false),
    sequenceNumber(nextSequenceNumber++)
{
}

ShaderFragment::
~ShaderFragment()
{
}

bool ShaderFragment::
update()
{
    return false;
}

std::string ShaderFragment::
getCode()
{
    return "";
}

std::string ShaderFragment::
getUniforms()
{
    return "";
}

std::string ShaderFragment::
getFunctions()
{
    return "";
}

void ShaderFragment::
reset()
{
    uniformsEmitted  = false;
    functionsEmitted = false;
}

std::string ShaderFragment::
makeUniqueName(const std::string& baseName) const
{
    std::ostringstream generatedName;
    generatedName << baseName << "_" << sequenceNumber;

    return generatedName.str();
}


END_CRUSTA
