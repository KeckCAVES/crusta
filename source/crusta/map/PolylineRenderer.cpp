#include <crusta/map/PolylineRenderer.h>

#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Math/Constants.h>
#include <Misc/ThrowStdErr.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/map/Polyline.h>


///\todo remove debug
#include <sys/stat.h>


BEGIN_CRUSTA


 static const int lineTexSize = 1024;


static const char* polylineVP = "\
void main() {\n\
    gl_Position = gl_Vertex;\n\
}\
";

static const char* polylineFP = "\
uniform vec2      windowPos;\n\
uniform vec2      rcpWindowSize;\n\
uniform sampler2D depthTex;\n\
uniform sampler2D terrainTex;\n\
\n\
uniform int numSegments;\n\
\n\
uniform float lineStartCoord;\n\
uniform float lineCoordStep;\n\
uniform sampler1D lineDataTex;\n\
uniform sampler2D symbolTex;\n\
\n\
vec2 computeFragCoord(in vec2 fragCoord)\n\
{\n\
    return (fragCoord - windowPos) * rcpWindowSize;\n\
}\n\
\n\
vec3 unproject(in mat4 inverseMVP, in vec2 fragCoord)\n\
{\n\
    gl_FragDepth  = texture2D(depthTex, fragCoord).r;\n\
\n\
    vec4 tmp;\n\
    tmp.xy = (fragCoord * 2.0) - 1.0;\n\
    tmp.z  = gl_FragDepth*2.0 - 1.0;\n\
    tmp.w  = 1.0;\n\
\n\
    tmp = inverseMVP * tmp;\n\
    tmp.w = 1.0 / tmp.w;\n\
    return tmp.xyz * tmp.w;\n\
}\n\
\n\
void main()\n\
{\n\
    vec2 fragCoord        = computeFragCoord(gl_FragCoord.xy);\n\
    vec4 terrainAttribute = texture2D(terrainTex, fragCoord);\n\
    if (all(equal(terrainAttribute, vec4(1.0))))\n\
        discard;\n\
\n\
    float coord = lineStartCoord + terrainAttribute.r*lineCoordStep +\n\
                  terrainAttribute.g*255*lineCoordStep;\n\
    //read in the inverse model view projection matrix\n\
    mat4 inverseMVP;\n\
    inverseMVP[0] = texture1D(lineDataTex, coord); coord+=lineCoordStep;\n\
    inverseMVP[1] = texture1D(lineDataTex, coord); coord+=lineCoordStep;\n\
    inverseMVP[2] = texture1D(lineDataTex, coord); coord+=lineCoordStep;\n\
    inverseMVP[3] = texture1D(lineDataTex, coord); coord+=lineCoordStep;\n\
\n\
    //compute the position of the fragment\n\
    vec3 pos = unproject(inverseMVP, fragCoord);\n\
\n\
    //walk all the line bits for the node of the fragmentsegments and process their contribution\n\
    vec4 color      = vec4(0.0, 0.0, 0.0, 0.0);\n\
    vec4 startCP    = getControlPoint(coord);\n\
\n\
    vec4 endCP = vec4(0.0);\n\
    for (int i=0; i<numSegments; ++i, startCP=endCP, coord+=lineCoordStep)\n\
    {\n\
        vec3 toPos = pos - startCP.xyz;\n\
\n\
        endCP           = getControlPoint(coord+lineCoordStep);\n\
        vec4 startToEnd = endCP - startCP;\n\
\n\
        //compute the u coordinate along the segment\n\
        float sqrLen = dot(startToEnd.xyz, startToEnd.xyz);\n\
        float u      = dot(toPos, startToEnd.xyz) / sqrLen;\n\
\n\
        if (u<=1.0 && u>=0.0)\n\
        {\n\
\n\
             //fetch the tangent\n\
             vec3 tangent = texture1D(tangentTex, coord).rgb;\n\
\n\
             //compute the v coordinate along the tangent\n\
             toPos   = startCP.xyz + u*startToEnd.xyz;\n\
             toPos   = pos - toPos;\n\
             sqrLen  = dot(tangent, tangent);\n\
             float v = dot(toPos, tangent) / sqrLen;\n\
\n\
             if (v<=1.0 && v>=-1.0)\n\
             {\n\
                //accumulate the contribution\n\
                v = v*0.5 + 0.5;\n\
vec4 fromc = vec4(0.0, 1.0, 0.0, 1.0);\n\
vec4 toc   = vec4(1.0, 0.0, 0.0, 1.0);\n\
float a    = numSegments<2 ? 1.0 : float(i) / float(numSegments-1);\n\
vec4 col   = a*fromc + (1.0-a)*toc;\n\
u          = startCP.w + u*startToEnd.w;\n\
col       *= texture2D(symbolTex, vec2(u,v));\n\
color      = mix(color, col, col.w);\n\
            }\n\
        }\n\
    }\n\
\n\
    gl_FragColor = mix(gl_FragColor, color, color.w);\n\
}\
";

PolylineRenderer::
PolylineRenderer(Crusta* iCrusta) :
    CrustaComponent(iCrusta)
{
}


GLuint PolylineRenderer::
getLineDataTexture(GLContextData& contextData)
{
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    return glData->lineDataTex;
}

static time_t lastVPTime;
static time_t lastFPTime;
static void
compileShaderFromFile(PolylineRenderer::GlData* glData,
                      const char* vp, const char* fp)
{
    bool needToRecompile = false;
    struct stat statRes;
    if (stat(vp, &statRes) == 0)
    {
        if (statRes.st_mtime != lastVPTime)
        {
            needToRecompile = true;
            lastVPTime      = statRes.st_mtime;
        }
    }
    else
        Misc::throwStdErr("PolylineRenderer: can't find VP");

    if (stat(fp, &statRes) == 0)
    {
        if (statRes.st_mtime != lastFPTime)
        {
            needToRecompile = true;
            lastFPTime      = statRes.st_mtime;
        }
    }
    else
        Misc::throwStdErr("PolylineRenderer: can't find FP");

    if (needToRecompile)
    {
        GLShader& shader = glData->shader;

        try
        {
            shader.~GLShader();
            new(&shader)GLShader();

            shader.compileVertexShader(vp);
            shader.compileFragmentShader(fp);
            shader.linkShader();
        }
        catch (std::exception& e)
        {
            fprintf(stderr, "%s",  e.what());

            static const char* dvp = "void main(){gl_Position=gl_Vertex;}";
            static const char* dfp = "void main(){gl_FragColor=vec4(1.0);}";

            shader.~GLShader();
            new(&shader)GLShader();

            shader.compileVertexShaderFromString(dvp);
            shader.compileFragmentShaderFromString(dfp);
            shader.linkShader();
        }

        shader.useProgram();

        GLint uniform;
        uniform = shader.getUniformLocation("depthTex");
        glUniform1i(uniform, 0);
        uniform = shader.getUniformLocation("terrainTex");
        glUniform1i(uniform, 1);
        uniform = shader.getUniformLocation("lineDataTex");
        glUniform1i(uniform, 2);
        uniform = shader.getUniformLocation("symbolTex");
        glUniform1i(uniform, 3);

        float lineCoordStep = 1.0f / lineTexSize;
        uniform = shader.getUniformLocation("lineCoordStep");
        glUniform1f(uniform, lineCoordStep);
        uniform = shader.getUniformLocation("lineStartCoord");
        glUniform1f(uniform, 0.5f * lineCoordStep);
        uniform = shader.getUniformLocation("lineLastSample");
        glUniform1f(uniform, lineCoordStep);
        uniform = shader.getUniformLocation("lineSkipSamples");
        glUniform1f(uniform, 2.0f * lineCoordStep);

        shader.disablePrograms();
    }
}

void PolylineRenderer::
display(GLContextData& contextData) const
{
    CHECK_GLA

    GlData* glData = contextData.retrieveDataItem<GlData>(this);

    compileShaderFromFile(glData, "polylineRenderer.vp", "polylineRenderer.fp");

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_LINE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glData->shader.useProgram();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, glData->lineDataTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, glData->symbolTex);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f( 1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f( 1.0f,  1.0f, 0.0f);
    glEnd();

    glData->shader.disablePrograms();

    CHECK_GLA

#if 0
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
#endif

    glPopAttrib();
    glActiveTexture(activeTexture);
    CHECK_GLA
}


class TargaImage
{
public:
    TargaImage() : byteCount(0), pixels(NULL)
    {
        size[0] = size[1] = 0;
    }
    ~TargaImage()
    {
        destroy();
    }

    bool load(const char* filename)
    {
        FILE* file;
        uint8 type[4];
        uint8 info[6];

        file = fopen(filename, "rb");
        if (file == NULL)
            return false;

        fread (&type, sizeof(char), 3, file);
        fseek (file, 12, SEEK_SET);
        fread (&info, sizeof(char), 6, file);

        //image type either 2 (color) or 3 (greyscale)
        if (type[1]!=0 || (type[2]!=2 && type[2]!=3))
        {
            fclose(file);
            return false;
        }

        size[0]   = info[0] + info[1]*256;
        size[1]   = info[2] + info[3]*256;
        byteCount = info[4] / 8;

        if (byteCount!=3 && byteCount!=4)
        {
            fclose(file);
            return false;
        }

        int imageSize = size[0]*size[1] * byteCount;

        //allocate memory for image data
        pixels = new uint8[imageSize];

        //read in image data
        fread(pixels, sizeof(uint8), imageSize, file);

        //close file
        fclose(file);

        return true;
    }

    void destroy()
    {
        size[0] = size[1] = 0;
        byteCount = 0;
        delete[] pixels;
        pixels = NULL;
    }

    int    size[2];
    int    byteCount;
    uint8* pixels;
};

PolylineRenderer::GlData::
GlData()
{
    glPushAttrib(GL_TEXTURE_BIT);

    static const int lineTexSize = 1024;

    glGenTextures(1, &lineDataTex);
    glBindTexture(GL_TEXTURE_1D, lineDataTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB, lineTexSize, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    CHECK_GLA

    glGenTextures(1, &symbolTex);
    glBindTexture(GL_TEXTURE_2D, symbolTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    TargaImage atlas;
    if (atlas.load("Crusta_MapSymbolAtlas.tga"))
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas.size[0], atlas.size[1], 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, atlas.pixels);

        atlas.destroy();
    }
    else
    {
        float defaultTexel[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_FLOAT, defaultTexel);
    }
    CHECK_GLA

#if 0
    shader.compileVertexShaderFromString(polylineVP);
    shader.compileFragmentShaderFromString(polylineFP);
    shader.linkShader();

    shader.useProgram();

    GLint uniform;
    uniform = shader.getUniformLocation("depthTex");
    glUniform1i(uniform, 0);
    uniform = shader.getUniformLocation("terrainTex");
    glUniform1i(uniform, 1);
    uniform = shader.getUniformLocation("lineDataTex");
    glUniform1i(uniform, 2);
    uniform = shader.getUniformLocation("symbolTex");
    glUniform1i(uniform, 3);

    float lineCoordStep = 1.0f / lineTexSize;
    uniform = shader.getUniformLocation("lineCoordStep");
    glUniform1f(uniform, lineCoordStep);
    uniform = shader.getUniformLocation("lineStartCoord");
    glUniform1f(uniform, 0.5f * lineCoordStep);
    uniform = shader.getUniformLocation("lineLastSample");
    glUniform1f(uniform, lineCoordStep);
    uniform = shader.getUniformLocation("lineSkipSamples");
    glUniform1f(uniform, 2.0f * lineCoordStep);

    shader.disablePrograms();
#endif

    glPopAttrib();
}

PolylineRenderer::GlData::
~GlData()
{
    glDeleteTextures(1, &lineDataTex);
    glDeleteTextures(1, &symbolTex);
}


void PolylineRenderer::
prepareLineData(GlData* glData) const
{
#if 0
    //create temporary for the line data
    Colors lineData;

    float scaleFac  = Vrui::getNavigationTransformation().getScaling();
    float lineWidth = 0.1f / scaleFac;

    Color symbolOriginSize[2] = { {0.0f, 0.0f, 1.0f, 0.5f},
                                  {0.0f, 0.5f, 1.0f, 0.5f} };
    int curSymbol = 0;

    for (LinePtrs::iterator lit=lines->begin(); lit!=lines->end(); ++lit)
    {
        const Point3s&  lineCps    = (*lit)->getControlPoints();
        const Point3fs& lineRelCps = (*lit)->getRelativeControlPoints();

        int numCPs = static_cast<int>(lineCps.size());
        if (numCPs<2)
            continue;

///\todo right now just alternating line features, need to sort by atlas
        lineData.push_back(symbolOriginSize[curSymbol++]);
        lineData.push_back(Color(1.0f, 0.0f, 0.0f, 0.0f)); //only 1 of a kind

        //dump the number of segments
        int numSegs = numCPs-1;
        lineData.push_back(Color(numSegs, 0.0f, 0.0f, 0.0f));

        Vector3f curTan(0);
        float    length = 0.0f;
        int      next   = 1;
        for (int cur=0; cur<numCPs-1; ++cur, ++next)
        {
            const Point3& curP  = lineCps[cur];
            const Point3& nextP = lineCps[next];

            //first point of the segment
            lineData.push_back(
                lineRelCps[cur][0], lineRelCps[cur][1], lineRelCps[cur][2]);
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

            length += scaleFac*Geometry::dist(curP, nextP);
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
#endif
}


void PolylineRenderer::
initContext(GLContextData& contextData) const
{
///\todo implement VRUI level support for blit. For now assume it exists
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
