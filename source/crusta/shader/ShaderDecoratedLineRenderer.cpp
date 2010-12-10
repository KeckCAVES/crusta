#include <crusta/shader/ShaderDecoratedLineRenderer.h>


#include <cassert>

#include <crusta/checkGl.h>
#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


ShaderDecoratedLineRenderer::
ShaderDecoratedLineRenderer(const std::string& shaderFileName_) :
    ShaderFileFragment(shaderFileName_), coverageSrc(NULL), lineDataSrc(NULL),
    lineNumSegmentsUniform(-2), lineCoordScaleUniform(-2),
    lineWidthUniform(-2)
{
}


void ShaderDecoratedLineRenderer::
setSources(ShaderDataSource* coverage, ShaderDataSource* lineData)
{
    coverageSrc = coverage;
    lineDataSrc = lineData;
}


void ShaderDecoratedLineRenderer::
setLineNumSegments(int numSegments)
{
    glUniform1i(lineNumSegmentsUniform, numSegments);
CRUSTA_DEBUG(70, assert(lineNumSegmentsUniform>=0); CHECK_GLA)
}

void ShaderDecoratedLineRenderer::
setLineCoordScale(float coordScale)
{
    glUniform1f(lineCoordScaleUniform, coordScale);
CRUSTA_DEBUG(70, assert(lineCoordScaleUniform>=0); CHECK_GLA)
}

void ShaderDecoratedLineRenderer::
setLineWidth(float width)
{
    glUniform1f(lineWidthUniform, width);
CRUSTA_DEBUG(70, assert(lineWidthUniform>=0); CHECK_GLA)
}


std::string ShaderDecoratedLineRenderer::
replaceToken(const std::string& token, const TokenArgs& args)
{
    std::string code;
    if (token == "DataSourceCode")
    {
        assert(coverageSrc!=NULL && lineDataSrc!=NULL && args.size()==0);

        code += coverageSrc->getCode();
        code += lineDataSrc->getCode();
    }
    else if (token == "sampleLineData")
    {
        assert(args.size() == 1);
        code += lineDataSrc->sample(args[0]);
    }
    else if (token == "sampleCoverage")
    {
        assert(args.size() == 1);
        code += coverageSrc->sample(args[0]);
    }

    return code;
}


void ShaderDecoratedLineRenderer::
reset()
{
    assert(coverageSrc!=NULL && lineDataSrc!=NULL);
    coverageSrc->reset();
    lineDataSrc->reset();
    
    ShaderFileFragment::reset();
}

void ShaderDecoratedLineRenderer::
initUniforms(GLuint programObj)
{
    assert(coverageSrc!=NULL && lineDataSrc!=NULL);
    coverageSrc->initUniforms(programObj);
    lineDataSrc->initUniforms(programObj);

///\todo who's responsible for texture unit use and binding?
    GLint uniform;
    uniform = glGetUniformLocation(programObj, "symbolTex");
    glUniform1i(uniform, 5);
    uniform = glGetUniformLocation(programObj, "lineCoverageTex");
    glUniform1i(uniform, 4);
    uniform = glGetUniformLocation(programObj, "lineDataTex");
    glUniform1i(uniform, 3);

    uniform = glGetUniformLocation(programObj, "lineStartCoord");
    glUniform1f(uniform, crusta::SETTINGS->lineDataStartCoord);
    uniform = glGetUniformLocation(programObj, "lineCoordStep");
    glUniform1f(uniform, crusta::SETTINGS->lineDataCoordStep);

    lineNumSegmentsUniform = glGetUniformLocation(programObj,"lineNumSegments");
    lineCoordScaleUniform  = glGetUniformLocation(programObj, "lineCoordScale");
    lineWidthUniform       = glGetUniformLocation(programObj,      "lineWidth");
}

END_CRUSTA
