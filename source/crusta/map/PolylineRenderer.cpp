#include <crusta/map/PolylineRenderer.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Math/Constants.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


static const char* polylineVP = "\
void main() {\n\
    gl_Position = gl_Vertex;\n\
}\
";

static const char* polylineFP = "\
uniform vec2      windowPos;\n\
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
vec3 unproject(in vec2 fragCoord)\n\
{\n\
    vec2 texCoord = (fragCoord - windowPos) * rcpWindowSize;\n\
    gl_FragDepth  = texture2D(depthTex, texCoord).r;\n\
//gl_FragColor.xyz = vec3(gl_FragDepth);\n\
//gl_FragColor.a = 1.0;\n\
//return vec3(1.0);\n\
\n\
    vec4 tmp;\n\
    tmp.xy = (texCoord * 2.0) - 1.0;\n\
    tmp.z  = gl_FragDepth*2.0 - 1.0;\n\
    tmp.w  = 1.0;\n\
\n\
    tmp   = gl_ModelViewProjectionMatrixInverse * tmp;\n\
    tmp.w = 1.0 / tmp.w;\n\
    return tmp.xyz * tmp.w;\n\
}\n\
\n\
void main()\n\
{\n\
//gl_FragColor.xyz = normalize(texture1D(controlPointTex, lineStartCoord).rgb);\n\
//gl_FragColor.a = 1.0;\n\
//return;\n\
    vec3 pos = unproject(gl_FragCoord.xy);\n\
//gl_FragColor.xyz = normalize(pos);\n\
//gl_FragColor.a = 1.0;\n\
//return;\n\
\n\
//gl_FragColor.xyz = vec3(length(pos) / 7371000.0);\n\
//gl_FragColor.xyz = normalize(pos);\n\
//gl_FragColor.a = 1.0;\n\
//return;\n\
\n\
    //walk all the line segments and process their contribution\n\
    vec4 color   = vec4(0.0, 0.0, 0.0, 0.0);\n\
    float coord  = lineStartCoord;\n\
    vec4 startCP = texture1D(controlPointTex, coord);\n\
\n\
//gl_FragColor.xyz = normalize(startCP.xyz);\n\
//gl_FragColor.a = 1.0;\n\
//return;\n\
\n\
\n\
    vec4 endCP = vec4(0.0);\n\
    for (int i=0; i<numSegments; ++i, startCP=endCP)\n\
    {\n\
        vec3 toPos = pos - startCP.xyz;\n\
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
color = vec4(u);\n\
continue;\n\
#if 0\n\
vec4 fromc1 = vec4(0.0, 1.0, 0.0, 1.0);\n\
vec4 toc1   = vec4(1.0, 0.0, 0.0, 1.0);\n\
float a1    = numSegments<2 ? 1.0 : float(i) / float(numSegments-1);\n\
color       = a1*fromc1 + (1.0-a1)*toc1;\n\
continue;\n\
#endif\n\
\n\
        //fetch the tangent\n\
        vec3 tangent = texture1D(tangentTex, coord -\n\
                                             (1.0-u)*lineCoordStep).rgb;\n\
\n\
        //compute the v coordinate along the tangent\n\
        toPos   = startCP.xyz + u*startToEnd;\n\
        toPos   = pos - toPos;\n\
        sqrLen  = dot(tangent, tangent);\n\
        float v = dot(toPos, tangent) / sqrLen;\n\
\n\
        if (v<-1.0 || v>1.0)\n\
            continue;\n\
\n\
        //acculumate the contribution\n\
//        v = v*0.5 + 0.5;\n\
vec4 fromc = vec4(0.0, 1.0, 0.0, 1.0);\n\
vec4 toc   = vec4(1.0, 0.0, 0.0, 1.0);\n\
float a    = numSegments<2 ? 1.0 : float(i) / float(numSegments-1);\n\
        color = a*fromc + (1.0-a)*toc;\n\
    }\n\
\n\
    gl_FragColor = color;\n\
}\
";

PolylineRenderer::
PolylineRenderer(Crusta* iCrusta) :
    CrustaComponent(iCrusta)
{
}

void PolylineRenderer::
display(GLContextData& contextData) const
{
    CHECK_GLA
    if (lines->size()<1)
        return;

#if 1
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
    glActiveTexture(GL_TEXTURE0);
    
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_LINE_BIT | GL_POLYGON_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_POLYGON_OFFSET_LINE);
    
    glPolygonOffset(1.0f, 50.0f);

    //compute the vertically scaled control points and centroids for display
    int numLines = static_cast<int>(lines->size());
    Point3s centroids;
    centroids.resize(numLines, Point3(0,0,0));
    std::vector<Point3s> controlPoints;
    controlPoints.resize(numLines);

    for (int i=0; i<numLines; ++i)
    {
        const Point3s& cps = (*lines)[i]->getControlPoints();
        int numPoints      = static_cast<int>(cps.size());
        controlPoints[i].resize(numPoints);
        for (int j=0; j<numPoints; ++j)
        {
            Point3 point = crusta->mapToScaledGlobe(cps[j]);
            for (int k=0; k<3; ++k)
            {
                controlPoints[i][j][k] = point[k];
                centroids[i][k]       += point[k];
            }
        }
        double norm = 1.0 / numPoints;
        for (int k=0; k<3; ++k)
            centroids[i][k] *= norm;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    for (int i=0; i<numLines; ++i)
    {
        const Point3s& cps = controlPoints[i];
        if (cps.size() < 1)
            continue;

        glPushMatrix();
        Vrui::Vector centroidTranslation(centroids[i][0], centroids[i][1],
                                         centroids[i][2]);
        Vrui::NavTransform nav =
            Vrui::getDisplayState(contextData).modelviewNavigational;
        nav *= Vrui::NavTransform::translate(centroidTranslation);
        glLoadMatrix(nav);
        
        //draw visible lines
        glDepthFunc(GL_LEQUAL);
        glLineWidth(2.0);
        Color symbolColor = (*lines)[i]->getSymbol().color;
        glColor4fv(symbolColor.getComponents());
        glBegin(GL_LINE_STRIP);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
        {
            glVertex3f((*it)[0] - centroids[i][0],
                       (*it)[1] - centroids[i][1],
                       (*it)[2] - centroids[i][2]);
        }
        glEnd();

        //display hidden lines
        glDepthFunc(GL_GREATER);
        glLineWidth(1.0);
        symbolColor[3] *= 0.33f;
        glColor4fv(symbolColor.getComponents());
        glBegin(GL_LINE_STRIP);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
        {
            glVertex3f((*it)[0] - centroids[i][0],
                       (*it)[1] - centroids[i][1],
                       (*it)[2] - centroids[i][2]);
        }
        glEnd();

        glPopMatrix();
        CHECK_GLA
    }

    glPopAttrib();
    glActiveTexture(activeTexture);
    CHECK_GLA
#else
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    readDepthBuffer(glData);
    int numSegments = prepareLineData(glData);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glData->shader.useProgram();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glUniform2f(glData->windowPosUniform,viewport[0],viewport[1]);
    glUniform2f(glData->rcpWindowSizeUniform,1.0f/viewport[2],1.0f/viewport[3]);
    glUniform1i(glData->numSegmentsUniform, numSegments);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glData->depthTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, glData->controlPointTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, glData->tangentTex);

    glBegin(GL_QUADS);
        glVertex3f(-1.0f,  1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f, -1.0f, 0.0f);
        glVertex3f( 1.0f,  1.0f, 0.0f);
    glEnd();

    glData->shader.disablePrograms();

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
#endif
}


PolylineRenderer::GlData::
GlData()
{
    depthTexSize[0] = depthTexSize[1] = Math::Constants<int>::max;

    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GLA

#ifndef __APPLE__
    glGenFramebuffersEXT(1, &blitFbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blitFbo);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                           GL_TEXTURE_2D, depthTex, 0);
    CHECK_GLA

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif //__APPLE__

    static const int lineTexSize = 512;

    glGenTextures(1, &controlPointTex);
    glBindTexture(GL_TEXTURE_1D, controlPointTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB, lineTexSize, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    CHECK_GLA

    glGenTextures(1, &tangentTex);
    glBindTexture(GL_TEXTURE_1D, tangentTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F_ARB, lineTexSize, 0,
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

    float lineCoordStep = 1.0f / lineTexSize;
    uniform = shader.getUniformLocation("lineCoordStep");
    glUniform1f(uniform, lineCoordStep);
    uniform = shader.getUniformLocation("lineStartCoord");
    glUniform1f(uniform, 0.5f * lineCoordStep);

    windowPosUniform      = shader.getUniformLocation("windowPos");
    rcpWindowSizeUniform  = shader.getUniformLocation("rcpWindowSize");
    numSegmentsUniform    = shader.getUniformLocation("numSegments");

    shader.disablePrograms();
}

PolylineRenderer::GlData::
~GlData()
{
#ifndef __APPLE__
    glDeleteFramebuffersEXT(1, &blitFbo);
#endif //__APPLE__
    glDeleteTextures(1, &depthTex);
}


void PolylineRenderer::
readDepthBuffer(GlData* glData) const
{
    CHECK_GLA
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
#ifdef __APPLE__
    GLfloat* depthBuf = new GLfloat[viewport[2]*viewport[3]];
    glReadPixels(0, 0, viewport[2], viewport[3], GL_DEPTH_COMPONENT,
                 GL_FLOAT, depthBuf);
    CHECK_GLA

    glBindTexture(GL_TEXTURE_2D, glData->depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                 viewport[2], viewport[3], 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, depthBuf);
    CHECK_GLA

    delete[] depthBuf;
#else
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
        assert(glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT) ==
               GL_FRAMEBUFFER_COMPLETE_EXT);
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
#endif //__APPLE__
}

int PolylineRenderer::
prepareLineData(GlData* glData) const
{
    const Point3s& lineCps = lines->front()->getControlPoints();

    int numCPs = static_cast<int>(lineCps.size());
    if (numCPs<2)
        return 0;

    float lineWidth = 1.0f / Vrui::getNavigationTransformation().getScaling();
    lineWidth *= 0.1f;

    typedef float CP[4];
    typedef float Tangent[3];

    CP* cps = new CP[numCPs];
    Tangent* tans = new Tangent[numCPs];

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
    for (int cur=1; cur<numCPs; ++cur, ++prev, ++cp, ++tan)
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
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, numCPs, GL_RGBA, GL_FLOAT, cps);
    CHECK_GLA

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, glData->tangentTex);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, numCPs, GL_RGB, GL_FLOAT, tans);
    CHECK_GLA

    delete[] cps;
    delete[] tans;

    return numCPs-1;
}


void PolylineRenderer::
initContext(GLContextData& contextData) const
{
///\todo implement VRUI level support for blit. For now assume it exists
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
