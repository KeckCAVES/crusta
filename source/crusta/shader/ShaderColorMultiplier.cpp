#include <crusta/shader/ShaderColorMultiplier.h>


#include <sstream>


BEGIN_CRUSTA


std::string ShaderColorMultiplier::
getFunctions()
{
    if (functionsEmitted)
        return "";

    std::ostringstream functions;
    
    functions << ShaderMultiDataSource::getFunctions();

    code << "vec4 " << sample("in vec2 tc") << " {" << std::endl;
    code << "  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);" << std::endl;

    for (size_t i=0; i < sources.size(); ++i)
    {
        std::ostringstream result("result");
        result << i;

        code << "  vec4 " << result.str() << " = "<< sources[i]->sample("tc") << ";" << std::endl;
        code << "  color *= " << result.str() << ";" << std::endl;
    }

    code << "  return color;" << std::endl;
    code << "}" << std::endl;

    functionsEmitted = true;
    return functions.str();
}


END_CRUSTA
