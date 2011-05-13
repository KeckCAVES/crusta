#include <crusta/shader/ShaderColorMixer.h>


#include <sstream>


BEGIN_CRUSTA


std::string ShaderColorMixer::
getCode()
{
    if (codeEmitted)
        return "";

    std::ostringstream code;

    code << ShaderMultiDataSource::getCode();
    code << std::endl;

    code << "vec4 " << sample("in vec2 tc") << " {" << std::endl;
    code << "  vec4 color = vec4(0.0, 0.0, 0.0, 0.0);" << std::endl;
    code << std::endl;

    for (size_t i=0; i<sources.size(); ++i)
    {
        std::ostringstream result;
        result << "result" << i;

        code << "  vec4 " << result.str() << " = "<< sources[i]->sample("tc") << ";" << std::endl;
        code << "  color.rgb = mix(color.rgb, " << result.str() << ".rgb, " << result.str() << ".a);" << std::endl;
        code << std::endl;
    }

    code << "  return color;" << std::endl;
    code << "}" << std::endl;
    code << std::endl;

    codeEmitted = true;
    return code.str();
}


END_CRUSTA
