#include <crusta/CompositeShader.h>


BEGIN_CRUSTA

static const char* compositeVP = "\
void main() {\n\
    gl_Position = gl_Vertex;\n\
}\
";

static const char* compositeFP = "\
uniform vec2      rcpWindowSize;\n\
uniform sampler2D colorTex;\n\
uniform sampler2D depthTex;\n\
\n\
void main()\n\
{\n\
    vec2 coords  = gl_FragCoord.xy * rcpWindowSize;\n\
    gl_FragColor = texture2D(colorTex, coords);\n\
    gl_FragDepth = texture2D(depthTex, coords).r;\n\
}\
";

CompositeShader::
CompositeShader()
{
    compileVertexShaderFromString(compositeVP);
    compileFragmentShaderFromString(compositeFP);
    linkShader();

    useProgram();

    GLint uniform;
    uniform = getUniformLocation("colorTex");
    glUniform1i(uniform, 0);
    uniform = getUniformLocation("depthTex");
    glUniform1i(uniform, 1);

    rcpWindowSizeUniform = getUniformLocation("rcpWindowSize");

    disablePrograms();
}

CompositeShader::
~CompositeShader()
{
}


void CompositeShader::
apply()
{
    GLint matrixMode;
    glGetIntegerv(GL_MATRIX_MODE, &matrixMode);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

#if 0
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
    glEnd();
#else
    useProgram();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float rcpSize[2] = { 1.0f/viewport[2], 1.0f/viewport[3] };
    glUniform2f(rcpWindowSizeUniform, rcpSize[0], rcpSize[1]);

    glBegin(GL_QUADS);
        glVertex3f(-1.0f,  1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f,  1.0f, 0.0f);
    glEnd();

    disablePrograms();
#endif

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(matrixMode);
}


END_CRUSTA
