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


const int PolylineRenderer::lineTexSize = 1024;


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

        float lineCoordStep = 1.0f / PolylineRenderer::lineTexSize;
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
    CHECK_GLA

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, glData->lineDataTex);
    CHECK_GLA
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, glData->symbolTex);
    CHECK_GLA

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
    CHECK_GLA

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

    glGenTextures(1, &lineDataTex);
    glBindTexture(GL_TEXTURE_2D, lineDataTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, lineTexSize, lineTexSize, 0,
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
initContext(GLContextData& contextData) const
{
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
