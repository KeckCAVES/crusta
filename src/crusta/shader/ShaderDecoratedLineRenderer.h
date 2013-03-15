#ifndef _Crusta_ShaderDecoratedLineRenderer_H_
#define _Crusta_ShaderDecoratedLineRenderer_H_


#include <crusta/shader/ShaderDataSource.h>
#include <crusta/shader/ShaderFileFragment.h>


namespace crusta {


class ShaderDecoratedLineRenderer : public ShaderFileFragment
{
public:
    ShaderDecoratedLineRenderer(const std::string& shaderFileName_);

    void setSources(ShaderDataSource* coverage, ShaderDataSource* lineData);

    /**\{ uniform setters */
    void setNumSegments(int numSegments);
    void setSymbolLength(float length);
    void setSymbolWidth(float width);
    /**\}*/

protected:
    ShaderDataSource* coverageSrc;
    ShaderDataSource* lineDataSrc;

///\todo comment the various uniforms
    GLint numSegmentsUniform;
    GLint symbolLengthUniform;
    GLint symbolWidthUniform;

//- inherited from ShaderFileFragment
protected:
    virtual std::string replaceToken(const std::string& token,
                                     const TokenArgs& args);

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual void initUniforms(GLuint programObj);
};


} //namespace crusta


#endif //_Crusta_ShaderDecoratedLineRenderer_H_
