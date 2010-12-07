#include <crusta/shader/ShaderColorMixer.h>


#include <sstream>

#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


std::string ShaderColorMixer::
getFunctions()
{
    if (functionsEmitted)
        return "";

    std::ostringstream functions;

    functions << ShaderMultiDataSource::getFunctions();

    functions << "vec4 " << sample("in vec2 tc") << " {" << std::endl;
    const Color& defaultColor = SETTINGS->terrainDefaultColor;
    functions << "  vec4 color = vec4(" << std::scientific << defaultColor[0] << ", " << defaultColor[1] << ", " << defaultColor[2] << ", " << defaultColor[3] << ");" << std::endl;

    for (size_t i=0; i<sources.size(); ++i)
    {
        std::ostringstream result("result");
        result << i;

        functions << "  vec4 " << result.str() << " = "<< sources[i]->sample("tc") << ";" << std::endl;
        functions << "  color.rgb = mix(color.rgb, " << result.str() << ".rgb, " << result.str() << ".a);" << std::endl;
    }

    functions << "  return color;" << std::endl;
    functions << "}" << std::endl;

    functionsEmitted = true;
    return functions.str();
}


END_CRUSTA
