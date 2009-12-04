#include <crusta/map/PolylineRenderer.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <Math/Constants.h>
#include <Vrui/Vrui.h>

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
uniform int numSegments;\n\
\n\
uniform float lineStartCoord;\n\
uniform float lineCoordStep;\n\
uniform sampler1D controlPointTex;\n\
uniform sampler1D tangentTex;\n\
\n\
void main()\n\
{\n\
    vec2 coords  = gl_FragCoord.xy * rcpWindowSize;\n\
\n\
    //regenerate the 3D position of the fragment\n\
    float depth = texture2D(depthTex, coords).r;\n\
    vec4 pos    = vec4(coords.x, coords.y, depth, 1.0);\n\
    pos         = gl_ModelViewProjectionMatrixInverse * pos;\n\
\n\
    //walk all the line segments and process their contribution\n\
    vec4 color   = vec4(0.0, 0.0, 0.0, 0.0);\n\
    float coord  = lineStartCoord;\n\
    vec4 startCP = texture1D(controlPointTex, coord);\n\
\n\
    vec4 endCP = vec4(0.0);\n\
    for (int i=0; i<numSegments; ++i, startCP=endCP)\n\
    {\n\
        vec3 toPos = pos.xyz - startCP.xyz;\n\
\n\
        coord           = coord + lineCoordStep;\n\
        endCP           = texture1D(controlPointTex, coord);\n\
        vec3 startToEnd = endCP.xyz - startCP.xyz;\n\
\n\
        //compute the u coordinate along the segment\n\
        float sqrLen = dot(startToEnd, startToEnd);\n\
        float u      = dot(toPos, startToEnd) / sqrLen;\n\
\n\
        if (u<0.0 || u>1.0)\n\
            continue;\n\
\n\
        //fetch the tangent\n\
        vec3 tangent = texture1D(tangentTex, coord -\n\
                                             (1.0-u)*lineCoordStep).rgb;\n\
\n\
        //compute the v coordinate along the tangent\n\
        toPos   = startCP.xyz + u*startToEnd;\n\
        toPos   = pos.xyz - toPos;\n\
        sqrLen  = dot(tangent, tangent);\n\
        float v = dot(toPos, tangent) / sqrLen;\n\
\n\
        if (v<-1.0 || v>1.0)\n\
            continue;\n\
\n\
        //acculumate the contribution\n\
//        v = v*0.5 + 0.5;\n\
        color = vec4(0.0, 1.0, 0.0, 1.0);\n\
    }\n\
\n\
    gl_FragColor = color;\n\
    gl_FragDepth = depth;\n\
}\
";

void PolylineRenderer::
display(GLContextData& contextData) const
{
    if (lines->size()<1)
        return;

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    readDepthBuffer(glData);
    
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
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GLA

    glGenFramebuffersEXT(1, &blitFbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blitFbo);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                           GL_TEXTURE_2D, depthTex, 0);
    CHECK_GLA

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    static const int lineTexSize = 512;
    
    glGenTextures(1, &controlPointTex);
    glBindTexture(GL_TEXTURE_1D, controlPointTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, lineTexSize, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    CHECK_GLA

    glGenTextures(1, &tangentTex);
    glBindTexture(GL_TEXTURE_1D, tangentTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, lineTexSize, 0,
                 GL_RGB, GL_FLOAT, NULL);
    CHECK_GLA

///\todo bind whatever was bound before
    glBindTexture(GL_TEXTURE_1D, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    shader.compileVertexShaderFromString(polylineVP);
    shader.compileFragmentShaderFromString(polylineFP);
    shader.linkShader();

    shader.useProgram();

    GLint uniform;
    uniform = shader.getUniformLocation("depthTex");
    glUniform1i(uniform, 0);
    uniform = shader.getUniformLocation("controlPointTex");
    glUniform1i(uniform, 1);
    uniform = shader.getUniformLocation("tangentTex");
    glUniform1i(uniform, 2);

    uniform = shader.getUniformLocation("lineStartCoord");
    float lineCoordStep = 1.0f / lineTexSize;
    glUniform1f(uniform, lineCoordStep);
    uniform = shader.getUniformLocation("lineCoordStep");
    glUniform1f(uniform, 0.5f * lineCoordStep);

    rcpWindowSizeUniform  = shader.getUniformLocation("rcpWindowSize");
    numSegmentsUniform    = shader.getUniformLocation("numSegments");

    shader.disablePrograms();
}

PolylineRenderer::GlData::
~GlData()
{
    glDeleteTextures(1, &depthTex);
    glDeleteFramebuffersEXT(1, &blitFbo);
}


void PolylineRenderer::
readDepthBuffer(GlData* glData) const
{
    CHECK_GLA

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLint drawFrame;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &drawFrame);
    GLint drawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
    GLint readFrame;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &readFrame);
    GLint readBuffer;
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);

    if (viewport[2] != glData->depthTexSize[0] ||
        viewport[3] != glData->depthTexSize[1])
    {
        glData->depthTexSize[0] = viewport[2];
        glData->depthTexSize[1] = viewport[3];

        glBindTexture(GL_TEXTURE_2D, glData->depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                     glData->depthTexSize[0], glData->depthTexSize[1], 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        CHECK_GLA

        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, glData->blitFbo);
        CHECK_GLA
        glDrawBuffer(GL_NONE);
        GLenum status = glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
        assert(status == GL_FRAMEBUFFER_COMPLETE_EXT);
    }

    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, drawFrame);
    CHECK_GLA

    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, glData->blitFbo);
    glDrawBuffer(GL_NONE);
    CHECK_GLA

    glBlitFramebufferEXT(0, 0, glData->depthTexSize[0], glData->depthTexSize[1],
                         0, 0, glData->depthTexSize[0], glData->depthTexSize[1],
                         GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    CHECK_GLA

    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, drawFrame);
    CHECK_GLA
    glDrawBuffer(drawBuffer);
    CHECK_GLA
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, readFrame);
    CHECK_GLA
    glReadBuffer(readBuffer);
    CHECK_GLA
}

void PolylineRenderer::
prepareLineData(GlData* glData) const
{
    const Point3s& lineCps = lines->front()->getControlPoints();

    int numSegments = static_cast<int>(lineCps.size()) - 1;
    if (numSegments<=0)
        return;

    float lineWidth = 1.0f / Vrui::getNavigationTransformation().getScaling();
    lineWidth *= 0.001f;
    
    typedef float CP[4];
    typedef float Tangent[3];

    CP* cps = new CP[numSegments+1];
    Tangent* tans = new Tangent[numSegments+1];

    float prevLen = 0.0f;
    CP* cp = cps;
    (*cp)[0] = lineCps[0][0];
    (*cp)[1] = lineCps[0][1];
    (*cp)[2] = lineCps[0][2];
    (*cp)[3] = prevLen;
    ++cp;

    Vector3 tanVec = Geometry::cross(Vector3(lineCps[0]), Vector3(lineCps[1]));
    tanVec.normalize();
    tanVec *= lineWidth;

    Tangent* tan = tans;
    (*tan)[0] = tanVec[0];
    (*tan)[1] = tanVec[1];
    (*tan)[2] = tanVec[2];
    ++tan;

    int prev = 0;
    int cur  = 1;
    for (int i=0; i<numSegments; ++i, ++prev, ++cur, ++cp, ++tan)
    {
        const Point3& prevP = lineCps[prev];
        const Point3& curP  = lineCps[cur];

        (*cp)[0] = curP[0];
        (*cp)[1] = curP[1];
        (*cp)[2] = curP[2];
        prevLen  = prevLen + Geometry::dist(curP, prevP);
        (*cp)[3] = prevLen;
        
        tanVec = Geometry::cross(Vector3(prevP), Vector3(curP));
        tanVec.normalize();
        tanVec *= lineWidth;
        
        (*tan)[0] = tanVec[0];
        (*tan)[1] = tanVec[1];
        (*tan)[2] = tanVec[2];
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, glData->controlPointTex);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, numSegments,
                    GL_RGBA, GL_FLOAT, cps);
    CHECK_GLA

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, glData->tangentTex);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, numSegments,
                    GL_RGB, GL_FLOAT, tans);
    CHECK_GLA

    delete[] cps;
    delete[] tans;
}


void PolylineRenderer::
initContext(GLContextData& contextData) const
{
///\todo implement VRUI level support for blit. For now assume it exists
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
