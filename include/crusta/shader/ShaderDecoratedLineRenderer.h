#ifndef _Crusta_ShaderDecoratedLineRenderer_H_
#define _Crusta_ShaderDecoratedLineRenderer_H_


#include <crusta/shader/ShaderDataSource.h>
#include <crusta/shader/ShaderFileFragment.h>


BEGIN_CRUSTA


class ShaderDecoratedLineRenderer : public ShaderFileFragment
{
public:
    ShaderDecoratedLineRenderer(const std::string& shaderFileName_);

    void setSources(ShaderDataSource* coverage, ShaderDataSource* lineData);

    /**\{ uniform setters */
    void setLineNumSegments(int numSegments);
    void setLineCoordScale(float coordScale);
    void setLineWidth(float width);
    /**\}*/

protected:
    ShaderDataSource* coverageSrc;
    ShaderDataSource* lineDataSrc;

///\todo comment the various uniforms
    GLint lineNumSegmentsUniform;
    GLint lineCoordScaleUniform;
    GLint lineWidthUniform;

//- inherited from ShaderFileFragment
protected:
    virtual std::string replaceToken(const std::string& token,
                                     const TokenArgs& args);

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual void initUniforms(GLuint programObj);
};


END_CRUSTA


#endif //_Crusta_ShaderDecoratedLineRenderer_H_
