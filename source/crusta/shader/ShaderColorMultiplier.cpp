#include <crusta/shader/ShaderColorMultiplier.h>


#include <sstream>


BEGIN_CRUSTA


std::string ShaderColorMultiplier::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;

    code << ShaderMultiDataSource::getCode();
    code << std::endl;

    code << "vec4 " << sample("in vec2 tc") << " {" << std::endl;
    code << "  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);" << std::endl;
    code << std::endl;

    for (size_t i=0; i < sources.size(); ++i)
    {
        std::ostringstream result("result");
        result << "result" << i;

        code << "  vec4 " << result.str() << " = "<< sources[i]->sample("tc") << ";" << std::endl;
        code << "  color *= " << result.str() << ";" << std::endl;
        code << std::endl;
    }

    code << "  return color;" << std::endl;
    code << "}" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
}


END_CRUSTA
