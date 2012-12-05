#include <crusta/shader/ShaderDecoratedLineRenderer.h>


#include <cassert>

#include <crusta/checkGl.h>
#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


ShaderDecoratedLineRenderer::
ShaderDecoratedLineRenderer(const std::string& shaderFileName_) :
    ShaderFileFragment(shaderFileName_), coverageSrc(NULL), lineDataSrc(NULL),
    numSegmentsUniform(-2), symbolLengthUniform(-2), symbolWidthUniform(-2)
{
}


void ShaderDecoratedLineRenderer::
setSources(ShaderDataSource* coverage, ShaderDataSource* lineData)
{
    coverageSrc = coverage;
    lineDataSrc = lineData;
}


void ShaderDecoratedLineRenderer::
setNumSegments(int numSegments)
{
CRUSTA_DEBUG(80, assert(numSegmentsUniform>=0);)
    if (numSegmentsUniform >= 0)
        glUniform1i(numSegmentsUniform, numSegments);
    CHECK_GLA
}

void ShaderDecoratedLineRenderer::
setSymbolLength(float length)
{
CRUSTA_DEBUG(80, assert(symbolLengthUniform>=0);)
    if (symbolLengthUniform >= 0)
        glUniform1f(symbolLengthUniform, length);
    CHECK_GLA
}

void ShaderDecoratedLineRenderer::
setSymbolWidth(float width)
{
CRUSTA_DEBUG(80, assert(symbolWidthUniform>=0);)
    if (symbolWidthUniform >= 0)
        glUniform1f(symbolWidthUniform, width);
    CHECK_GLA
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

    numSegmentsUniform  = -2;
    symbolLengthUniform = -2;
    symbolWidthUniform  = -2;
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

    numSegmentsUniform  = glGetUniformLocation(programObj,  "lineNumSegments");
    symbolLengthUniform = glGetUniformLocation(programObj, "lineSymbolLength");
    symbolWidthUniform  = glGetUniformLocation(programObj,  "lineSymbolWidth");
}

END_CRUSTA
