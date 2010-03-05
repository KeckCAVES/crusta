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
vec4 getControlPoint(in float coord)\n\
{\n\
    vec4 ret = texture1D(controlPointTex, coord);\n\
    return ret;\n\
}\n\
\n\
void main()\n\
{\n\
    vec3 pos = unproject(gl_FragCoord.xy);\n\
\n\
    //walk all the line segments and process their contribution\n\
    vec4 color      = vec4(0.0, 0.0, 0.0, 0.0);\n\
    float coord     = lineStartCoord;\n\
    vec4 startCP    = getControlPoint(coord);\n\
\n\
    vec4 endCP = vec4(0.0);\n\
    for (int i=0; i<numSegments; ++i, startCP=endCP, coord+=lineCoordStep)\n\
    {\n\
        vec3 toPos = pos - startCP.xyz;\n\
\n\
        endCP           = getControlPoint(coord+lineCoordStep);\n\
        vec3 startToEnd = endCP.xyz - startCP.xyz;\n\
\n\
        //compute the u coordinate along the segment\n\
        float sqrLen = dot(startToEnd, startToEnd);\n\
        float u      = dot(toPos, startToEnd) / sqrLen;\n\
\n\
        if (u<=1.0 && u>=0.0)\n\
        {\n\
//color += vec4(0.0, 0.3, 0.0, 1.0);\n\
\n\
             //fetch the tangent\n\
             vec3 tangent = texture1D(tangentTex, coord).rgb;\n\
\n\
             //compute the v coordinate along the tangent\n\
             toPos   = startCP.xyz + u*startToEnd;\n\
             toPos   = pos - toPos;\n\
             sqrLen  = dot(tangent, tangent);\n\
             float v = dot(toPos, tangent) / sqrLen;\n\
\n\
             if (v<=1.0 && v>=-1.0)\n\
             {\n\
                //accumulate the contribution\n\
//                v = v*0.5 + 0.5;\n\
vec4 fromc = vec4(0.0, 1.0, 0.0, 1.0);\n\
vec4 toc   = vec4(1.0, 0.0, 0.0, 1.0);\n\
float a    = numSegments<2 ? 1.0 : float(i) / float(numSegments-1);\n\
color      = a*fromc + (1.0-a)*toc;\n\
            }\n\
        }\n\
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

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_LINE_BIT | GL_POLYGON_BIT);

    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    int numSegments = prepareLineData(glData);

    if (numSegments > 0)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const Point3f& centroid = lines->front()->getCentroid();

        glPushMatrix();
        Vrui::Vector centroidTranslation(centroid[0], centroid[1], centroid[2]);
        Vrui::NavTransform nav =
            Vrui::getDisplayState(contextData).modelviewNavigational;
        nav *= Vrui::NavTransform::translate(centroidTranslation);
        glLoadMatrix(nav);

        glData->shader.useProgram();

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glUniform2f(glData->windowPosUniform, viewport[0], viewport[1]);
        glUniform2f(glData->rcpWindowSizeUniform,
                    1.0f/viewport[2], 1.0f/viewport[3]);
        glUniform1i(glData->numSegmentsUniform, numSegments);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, glData->controlPointTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D, glData->tangentTex);

        glBegin(GL_QUADS);
            glVertex3f(-1.0f,  1.0f, 0.0f);
            glVertex3f(-1.0f, -1.0f, 0.0f);
            glVertex3f( 1.0f, -1.0f, 0.0f);
            glVertex3f( 1.0f,  1.0f, 0.0f);
        glEnd();

        glData->shader.disablePrograms();

        glPopMatrix();
        CHECK_GLA
    }

    glActiveTexture(GL_TEXTURE0);
    
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

    if (tangents.size()>0 && lines->size()>0)
    {
        glLineWidth(1.0);
        const Point3s& cps = (*lines)[0]->getControlPoints();
        for (int i=0; i<(int)tangents.size(); ++i)
        {
            Point3 tip = cps[i] + 4.0*Vector3(tangents[i][0], tangents[i][1], tangents[i][2]);
            glBegin(GL_LINES);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3dv(cps[i].getComponents());
            glColor3f(1.0f, 1.0, 0.0f);
            glVertex3dv(tip.getComponents());
            glEnd();
        }
    }

    glPopAttrib();
    glActiveTexture(activeTexture);
    CHECK_GLA
}


PolylineRenderer::GlData::
GlData()
{
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
    glUniform1i(uniform, 2);
    uniform = shader.getUniformLocation("tangentTex");
    glUniform1i(uniform, 3);

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
    glDeleteTextures(1, &controlPointTex);
    glDeleteTextures(1, &tangentTex);
}


int PolylineRenderer::
prepareLineData(GlData* glData) const
{
    const Point3s&  lineCps    = lines->front()->getControlPoints();
    const Point3fs& lineRelCps = lines->front()->getRelativeControlPoints();

    int numCPs = static_cast<int>(lineCps.size());
    if (numCPs<2)
        return 0;

    float lineWidth = 1.0f / Vrui::getNavigationTransformation().getScaling();
    lineWidth *= 0.05f;

    typedef float CP[4];

    int numSegs = numCPs-1;
    CP* cps = new CP[numCPs];
    tangents.resize(numCPs);

    Vector3  curTan(0);
    float    length = 0.0f;
    int      next   = 1;
    CP*      cp     = cps;
    Tangent* tan    = &tangents.front();
    for (int cur=0; cur<numCPs-1; ++cur, ++next, ++cp, ++tan)
    {
        const Point3& curP  = lineCps[cur];
        const Point3& nextP = lineCps[next];

        //first point of the segment
        (*cp)[0] = lineRelCps[cur][0];
        (*cp)[1] = lineRelCps[cur][1];
        (*cp)[2] = lineRelCps[cur][2];
        (*cp)[3] = length;

        curTan = Geometry::cross(Vector3(curP), Vector3(nextP));
        curTan.normalize();
        curTan *= lineWidth;

        (*tan)[0] = curTan[0];
        (*tan)[1] = curTan[1];
        (*tan)[2] = curTan[2];

        length += Geometry::dist(curP, nextP);
    }

    //last control point
    (*cp)[0] = lineRelCps.back()[0];
    (*cp)[1] = lineRelCps.back()[1];
    (*cp)[2] = lineRelCps.back()[2];
    (*cp)[3] = length;

    (*tan)[0] = curTan[0];
    (*tan)[1] = curTan[1];
    (*tan)[2] = curTan[2];

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, glData->controlPointTex);
    glTexSubImage1D(GL_TEXTURE_1D, 0,0, numCPs, GL_RGBA, GL_FLOAT, cps);
    CHECK_GLA

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_1D, glData->tangentTex);
    glTexSubImage1D(GL_TEXTURE_1D, 0,0, numCPs, GL_RGB, GL_FLOAT, &tangents[0]);
    CHECK_GLA

    delete[] cps;

    return numSegs;
}


void PolylineRenderer::
initContext(GLContextData& contextData) const
{
///\todo implement VRUI level support for blit. For now assume it exists
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
