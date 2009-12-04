#include <crusta/map/PolylineRenderer.h>

///\todo remove
#include <iostream>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <Math/Constants.h>

#include <crusta/checkGl.h>
#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


static const char* polylineVP = "\
void main() {\n\
    gl_Position = gl_Vertex;\n\
}\
";

static const char* polylineFP = "\
uniform vec2      rcpWindowSize;\n\
uniform sampler2D depthTex;\n\
\n\
void main()\n\
{\n\
    vec2 coords  = gl_FragCoord.xy * rcpWindowSize;\n\
    gl_FragColor = texture2D(depthTex, coords);\n\
//    gl_FragColor = vec4(coords.x, coords.y, 0.0, 1.0);\n\
}\
";

void PolylineRenderer::
display(GLContextData& contextData) const
{
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    readDepthBuffer(glData);

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

#if 1
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
    glBindTexture(GL_TEXTURE_2D, glData->depthTex);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
    glEnd();
#else
    glData->shader.useProgram();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float rcpSize[2] = { 1.0f/viewport[2], 1.0f/viewport[3] };
    glUniform2f(glData->rcpWindowSizeUniform, rcpSize[0], rcpSize[1]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glData->depthTex);

    glBegin(GL_QUADS);
        glVertex3f(-1.0f,  1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f,  1.0f, 0.0f);
    glEnd();

    glData->shader.disablePrograms();
#endif

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(matrixMode);
#endif

    glPushAttrib(GL_ENABLE_BIT);

    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    for (Ptrs::const_iterator lIt=lines->begin(); lIt!=lines->end(); ++lIt)
    {
        const Point3s& cps = (*lIt)->getControlPoints();
        if (cps.size() < 2)
            continue;
        glBegin(GL_LINE_STRIP);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
            glVertex3f((*it)[0], (*it)[1], (*it)[2]);
        glEnd();
    }

///\todo remove
    for (Ptrs::const_iterator lIt=lines->begin(); lIt!=lines->end(); ++lIt)
    {
        const Point3s& cps = (*lIt)->getControlPoints();
        glBegin(GL_POINTS);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
            glVertex3f((*it)[0], (*it)[1], (*it)[2]);
        glEnd();
    }

    glPopAttrib();
    glActiveTexture(activeTexture);
}


PolylineRenderer::GlData::
GlData()
{
    depthTexSize[0] = depthTexSize[1] = Math::Constants<int>::max;

    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GLA

    glGenFramebuffers(1, &blitFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, blitFbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depthTex, 0);
    CHECK_GLA

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shader.compileVertexShaderFromString(polylineVP);
    shader.compileFragmentShaderFromString(polylineFP);
    shader.linkShader();

    shader.useProgram();

    GLint uniform;
    uniform = shader.getUniformLocation("depthTex");
    glUniform1i(uniform, 0);

    rcpWindowSizeUniform = shader.getUniformLocation("rcpWindowSize");

    shader.disablePrograms();
}

PolylineRenderer::GlData::
~GlData()
{
    glDeleteTextures(1, &depthTex);
    glDeleteFramebuffers(1, &blitFbo);
}


void PolylineRenderer::
readDepthBuffer(GlData* glData) const
{
    CHECK_GLA

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLint drawFrame;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFrame);
    GLint drawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
    GLint readFrame;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFrame);
    GLint readBuffer;
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);

std::cerr << "read1" << std::endl;

    if (viewport[2] != glData->depthTexSize[0] ||
        viewport[3] != glData->depthTexSize[1])
    {
std::cerr << "read2" << std::endl;

        glData->depthTexSize[0] = viewport[2];
        glData->depthTexSize[1] = viewport[3];

        glBindTexture(GL_TEXTURE_2D, glData->depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                     glData->depthTexSize[0], glData->depthTexSize[1], 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        CHECK_GLA

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glData->blitFbo);
        CHECK_GLA
        glDrawBuffer(GL_NONE);
        GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        assert(status == GL_FRAMEBUFFER_COMPLETE);
    }

std::cerr << "read3" << std::endl;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFrame);
    CHECK_GLA

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glData->blitFbo);
    glDrawBuffer(GL_NONE);
    CHECK_GLA

    glBlitFramebuffer(0, 0, glData->depthTexSize[0], glData->depthTexSize[1],
                      0, 0, glData->depthTexSize[0], glData->depthTexSize[1],
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    CHECK_GLA

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFrame);
    CHECK_GLA
    glDrawBuffer(drawBuffer);
    CHECK_GLA
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFrame);
    CHECK_GLA
    glReadBuffer(readBuffer);
    CHECK_GLA
}

void PolylineRenderer::
initContext(GLContextData& contextData) const
{
///\todo implement VRUI level support for blit. For now assume it exists
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
