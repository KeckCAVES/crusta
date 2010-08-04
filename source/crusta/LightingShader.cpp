/***********************************************************************
LightingShader - Class to maintain a GLSL point-based lighting
shader that tracks the current OpenGL lighting state.
Copyright (c) 2008 Oliver Kreylos

This file is part of the LiDAR processing and analysis package.

The LiDAR processing and analysis package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The LiDAR processing and analysis package is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the LiDAR processing and analysis package; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/
#include <crusta/LightingShader.h>

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <GL/gl.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBFragmentShader.h>

///\todo integrate properly (VIS 2010)
#include <crusta/Crusta.h>

///\todo remove debug
#include <sys/stat.h>
#include <fstream>


/*************************************************
Static elements of class LightingShader:
*************************************************/

const char* LightingShader::applyLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float nl;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Compute the diffuse lighting angle: */\n\
        nl=dot(normalEc,lightDirEc);\n\
        if(nl>0.0)\n\
            {\n\
            vec3 eyeDirEc;\n\
            float nhv;\n\
            \n\
            /* Calculate per-source diffuse light term: */\n\
            color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
            \n\
            /* Compute the eye direction: */\n\
            eyeDirEc=normalize(-vertexEc.xyz);\n\
            \n\
            /* Compute the specular lighting angle: */\n\
            nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
            \n\
            /* Calculate per-source specular lighting term: */\n\
            color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
            }\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::applyAttenuatedLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float lightDist,nl,att;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDist=length(lightDirEc);\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Compute the diffuse lighting angle: */\n\
        nl=dot(normalEc,lightDirEc);\n\
        if(nl>0.0)\n\
            {\n\
            vec3 eyeDirEc;\n\
            float nhv;\n\
            \n\
            /* Calculate per-source diffuse light term: */\n\
            color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
            \n\
            /* Compute the eye direction: */\n\
            eyeDirEc=normalize(-vertexEc.xyz);\n\
            \n\
            /* Compute the specular lighting angle: */\n\
            nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
            \n\
            /* Calculate per-source specular lighting term: */\n\
            color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
            }\n\
        \n\
        /* Attenuate the per-source light terms: */\n\
        att=(gl_LightSource[<lightIndex>].quadraticAttenuation*lightDist+gl_LightSource[<lightIndex>].linearAttenuation)*lightDist+gl_LightSource[<lightIndex>].constantAttenuation;\n\
        color*=1.0/att;\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::applySpotLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float sl,nl,att;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Calculate the spot light angle: */\n\
        sl=-dot(lightDirEc,normalize(gl_LightSource[<lightIndex>].spotDirection));\n\
        \n\
        /* Check if the point is inside the spot light's cone: */\n\
        if(sl>=gl_LightSource[<lightIndex>].spotCosCutoff)\n\
            {\n\
            /* Compute the diffuse lighting angle: */\n\
            nl=dot(normalEc,lightDirEc);\n\
            if(nl>0.0)\n\
                {\n\
                vec3 eyeDirEc;\n\
                float nhv;\n\
                \n\
                /* Calculate per-source diffuse light term: */\n\
                color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
                \n\
                /* Compute the eye direction: */\n\
                eyeDirEc=normalize(-vertexEc.xyz);\n\
                \n\
                /* Compute the specular lighting angle: */\n\
                nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
                \n\
                /* Calculate per-source specular lighting term: */\n\
                color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
                }\n\
            \n\
            /* Calculate the spot light attenuation: */\n\
            att=pow(sl,gl_LightSource[<lightIndex>].spotExponent);\n\
            color*=att;\n\
            }\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::applyAttenuatedSpotLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float sl,nl,att;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Calculate the spot light angle: */\n\
        sl=-dot(lightDirEc,normalize(gl_LightSource[<lightIndex>].spotDirection))\n\
        \n\
        /* Check if the point is inside the spot light's cone: */\n\
        if(sl>=gl_LightSource[<lightIndex>].spotCosCutoff)\n\
            {\n\
            /* Compute the diffuse lighting angle: */\n\
            nl=dot(normalEc,lightDirEc);\n\
            if(nl>0.0)\n\
                {\n\
                vec3 eyeDirEc;\n\
                float nhv;\n\
                \n\
                /* Calculate per-source diffuse light term: */\n\
                color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
                \n\
                /* Compute the eye direction: */\n\
                eyeDirEc=normalize(-vertexEc.xyz);\n\
                \n\
                /* Compute the specular lighting angle: */\n\
                nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
                \n\
                /* Calculate per-source specular lighting term: */\n\
                color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
                }\n\
            \n\
            /* Calculate the spot light attenuation: */\n\
            att=pow(sl,gl_LightSource[<lightIndex>].spotExponent);\n\
            color*=att;\n\
            }\n\
        \n\
        /* Attenuate the per-source light terms: */\n\
        att=(gl_LightSource[<lightIndex>].quadraticAttenuation*lightDist+gl_LightSource[<lightIndex>].linearAttenuation)*lightDist+gl_LightSource[<lightIndex>].constantAttenuation;\n\
        color*=1.0/att;\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::fetchTerrainColorAsConstant =
"\
    vec4 terrainColor = vec4(1.0); //white\n\
";

const char* LightingShader::fetchTerrainColorFromColorMap =
"\
    float e = texture2D(heightTex, coord).r;\n\
          e = (e-minColorMapElevation) * colorMapElevationInvRange;\n\
    vec4 mapColor     = texture1D(colorMap, e);\n\
    vec4 terrainColor = texture2D(colorTex, coord);\n\
    terrainColor      = mix(terrainColor, mapColor, mapColor.w);\
";

const char* LightingShader::fetchTerrainColorFromTexture =
"\
    vec4 terrainColor = texture2D(colorTex, coord);\
";

/*****************************************
Methods of class LightingShader:
*****************************************/

std::string LightingShader::createApplyLightFunction(const char* functionTemplate,int lightIndex) const
    {
    std::string result;

    /* Create the light index string: */
    char index[10];
    snprintf(index,sizeof(index),"%d",lightIndex);

    /* Replace all occurrences of <lightIndex> in the template string with the light index string: */
    const char* match="<lightIndex>";
    const char* matchStart = NULL;
    int matchLen=0;
    for(const char* tPtr=functionTemplate;*tPtr!='\0';++tPtr)
        {
        if(matchLen==0)
            {
            if(*tPtr=='<')
                {
                matchStart=tPtr;
                matchLen=1;
                }
            else
                result.push_back(*tPtr);
            }
        else if(matchLen<12)
            {
            if(*tPtr==match[matchLen])
                {
                ++matchLen;
                if(matchLen==static_cast<int>(strlen(match)))
                    {
                    result.append(index);
                    matchLen=0;
                    }
                }
            else
                {
                for(const char* cPtr=matchStart;cPtr!=tPtr;++cPtr)
                    result.push_back(*cPtr);
                matchLen=0;
                --tPtr;
                }
            }
        }

    return result;
    }

LightingShader::LightingShader() :
    mustRecompile(true),
    colorMaterial(false),
    linesDecorated(false),
    texturingMode(2),
    lightStates(0),
    vertexShader(0),fragmentShader(0),
    programObject(0)
{
    /* Determine the maximum number of light sources supported by the local OpenGL: */
    glGetIntegerv(GL_MAX_LIGHTS,&maxNumLights);

    /* Initialize the light state array: */
    lightStates=new LightState[maxNumLights];
    updateLightingState();

    /* Check for the required OpenGL extensions: */
    if(!GLARBShaderObjects::isSupported())
        Misc::throwStdErr("GLShader::GLShader: GL_ARB_shader_objects not supported");
    if(!GLARBVertexShader::isSupported())
        Misc::throwStdErr("GLShader::GLShader: GL_ARB_vertex_shader not supported");
    if(!GLARBFragmentShader::isSupported())
        Misc::throwStdErr("GLShader::GLShader: GL_ARB_fragment_shader not supported");

    /* Initialize the required extensions: */
    GLARBShaderObjects::initExtension();
    GLARBVertexShader::initExtension();
    GLARBFragmentShader::initExtension();

    /* Create the vertex and fragment shaders: */
    vertexShader=glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    fragmentShader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    /* Create the program object: */
    programObject=glCreateProgramObjectARB();
    glAttachObjectARB(programObject,vertexShader);
    glAttachObjectARB(programObject,fragmentShader);
}

LightingShader::~LightingShader()
{
    glDeleteObjectARB(programObject);
    glDeleteObjectARB(vertexShader);
    glDeleteObjectARB(fragmentShader);
    delete[] lightStates;
}


///\todo remove once code is baked in
static time_t lastFPTime;
static bool
checkFileForChanges(const char* fileName)
{
    struct stat statRes;
    if (stat(fileName, &statRes) == 0)
    {
        if (statRes.st_mtime != lastFPTime)
        {
            lastFPTime      = statRes.st_mtime;
            return true;
        }
        else
            return false;
    }
    else
    {
        Misc::throwStdErr("LightingShader: can't find FP to check");
        return false;
    }
}

void LightingShader::
update()
{
    /* Update light states and recompile the shader if necessary: */
    updateLightingState();

///\todo bake this into the code
std::string progFile(CRUSTA_SHARE_PATH);
if (linesDecorated)
    progFile += "/decoratedRenderer.fp";
else
    progFile += "/plainRenderer.fp";
mustRecompile |= checkFileForChanges(progFile.c_str());

    if (mustRecompile)
    {
        compileShader();
        mustRecompile = false;
    }
}

void LightingShader::
updateLightingState()
{
    /* Check the color material flag: */
    bool newColorMaterial=glIsEnabled(GL_COLOR_MATERIAL);
    if(newColorMaterial!=colorMaterial)
        mustRecompile=true;
    colorMaterial=newColorMaterial;

    for(int lightIndex=0;lightIndex<maxNumLights;++lightIndex)
    {
        GLenum light=GL_LIGHT0+lightIndex;

        /* Get the light's enabled flag: */
        bool enabled=glIsEnabled(light);
        if(enabled!=lightStates[lightIndex].enabled)
            mustRecompile=true;
        lightStates[lightIndex].enabled=enabled;

        if(enabled)
        {
            /* Determine the light's attenuation and spot light state: */
            bool attenuated=false;
            bool spotLight=false;

            /* Get the light's position: */
            GLfloat pos[4];
            glGetLightfv(light,GL_POSITION,pos);
            if(pos[3]!=0.0f)
            {
                /* Get the light's attenuation coefficients: */
                GLfloat att[3];
                glGetLightfv(light,GL_CONSTANT_ATTENUATION,&att[0]);
                glGetLightfv(light,GL_LINEAR_ATTENUATION,&att[1]);
                glGetLightfv(light,GL_QUADRATIC_ATTENUATION,&att[2]);

                /* Determine whether the light is attenuated: */
                if(att[0]!=1.0f||att[1]!=0.0f||att[2]!=0.0f)
                    attenuated=true;

                /* Get the light's spot light cutoff angle: */
                GLfloat spotLightCutoff;
                glGetLightfv(light,GL_SPOT_CUTOFF,&spotLightCutoff);
                spotLight=spotLightCutoff<=90.0f;
            }

            if(attenuated!=lightStates[lightIndex].attenuated)
                mustRecompile=true;
            lightStates[lightIndex].attenuated=attenuated;

            if(spotLight!=lightStates[lightIndex].spotLight)
                mustRecompile=true;
            lightStates[lightIndex].spotLight=spotLight;
        }
    }
}

///\todo remove once code is baked in
void readFileToString(const char* fileName, std::string& content)
{
    std::ifstream ifs(fileName);
    content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

void LightingShader::compileShader()
    {
    std::string vertexShaderUniforms;
    std::string vertexShaderFunctions;
    std::string vertexShaderMain;

    vertexShaderUniforms +=
    "\
        uniform sampler2D geometryTex;\n\
        uniform sampler2D heightTex;\n\
        uniform sampler2D colorTex;\n\
        uniform sampler1D colorMap;\n\
        \n\
        uniform float colorMapElevationInvRange;\n\
        uniform float minColorMapElevation;\n\
        uniform float texStep;\n\
        uniform float verticalScale;\n\
        uniform vec3  center;\n\
        \n\
        varying vec3 position;\n\
        varying vec3 normal;\n\
        varying vec2 texCoord;\n\
    ";

    vertexShaderFunctions +=
    "\
        vec3 surfacePoint(in vec2 coords)\n\
        {\n\
            vec3 res      = texture2D(geometryTex, coords).rgb;\n\
            vec3 dir      = normalize(center + res);\n\
            float height  = texture2D(heightTex, coords).r;\n\
            height       *= verticalScale;\n\
            res          += height * dir;\n\
            return res;\n\
        }\n\
        \n\
    ";

    /* Create the main vertex shader starting boilerplate: */
    vertexShaderMain+=
        "\
        void main()\n\
        {\n\
            vec4 vertexEc;\n\
            vec3 normalEc;\n\
            vec4 ambient,diffuse,specular;\n\
            vec4 color;\n\
            vec2 coord = gl_Vertex.xy;\n\
            position   = surfacePoint(coord);\n\
            \n\
            vec3 up    = surfacePoint(vec2(coord.x, coord.y + texStep));\n\
            vec3 down  = surfacePoint(vec2(coord.x, coord.y - texStep));\n\
            vec3 left  = surfacePoint(vec2(coord.x - texStep, coord.y));\n\
            vec3 right = surfacePoint(vec2(coord.x + texStep, coord.y));\n\
            \n\
            normal = cross((right - left), (up - down));\n\
            normal = normalize(normal);\n\
            \n\
            /* Compute the vertex position in eye coordinates: */\n\
            vertexEc = gl_ModelViewMatrix * vec4(position, 1.0);\n\
            \n\
            /* Compute the normal vector in eye coordinates: */\n\
            normalEc=normalize(gl_NormalMatrix*normal);\n\
            \n\
            \n";

    /* Get the material components: */
    if(colorMaterial)
        {
        vertexShaderMain+=
            "\
            /* Get the material properties from the current color: */\n\
            ambient=gl_Color;\n\
            diffuse=gl_Color;\n\
            specular=gl_FrontMaterial.specular;\n\
            \n";
        }
    else
        {
        vertexShaderMain+=
            "\
            /* Get the material properties from the material state: */\n\
            ambient=gl_FrontMaterial.ambient;\n\
            diffuse=gl_FrontMaterial.diffuse;\n\
            specular=gl_FrontMaterial.specular;\n\
            \n";
        }

    vertexShaderMain+=
        "\
        /* Modulate with the texture color: */\n\
        \n";

    switch (texturingMode)
    {
        case 0:
            vertexShaderMain+=fetchTerrainColorAsConstant;
            break;
        case 1:
            vertexShaderMain+=fetchTerrainColorFromColorMap;
            break;
        case 2:
            vertexShaderMain+=fetchTerrainColorFromTexture;
            break;
        default:
            vertexShaderMain+="vec4 terrainColor(1.0, 0.0, 0.0, 1.0);";
    }

    vertexShaderMain+=
        "\
        ambient *= terrainColor;\n\
        diffuse *= terrainColor;\n";

    /* Continue the main vertex shader: */
    vertexShaderMain+=
        "\
        /* Calculate global ambient light term: */\n\
        color  = gl_LightModel.ambient*ambient;\n\
        \n\
        /* Apply all enabled light sources: */\n";

    /* Create light application functions for all enabled light sources: */
    for(int lightIndex=0;lightIndex<maxNumLights;++lightIndex)
        if(lightStates[lightIndex].enabled)
            {
            /* Create the appropriate light application function: */
            if(lightStates[lightIndex].spotLight)
                {
                if(lightStates[lightIndex].attenuated)
                    vertexShaderFunctions+=createApplyLightFunction(applyAttenuatedSpotLightTemplate,lightIndex);
                else
                    vertexShaderFunctions+=createApplyLightFunction(applySpotLightTemplate,lightIndex);
                }
            else
                {
                if(lightStates[lightIndex].attenuated)
                    vertexShaderFunctions+=createApplyLightFunction(applyAttenuatedLightTemplate,lightIndex);
                else
                    vertexShaderFunctions+=createApplyLightFunction(applyLightTemplate,lightIndex);
                }

            /* Call the light application function from the shader's main function: */
            char call[256];
            snprintf(call,sizeof(call),"\t\t\tcolor+=applyLight%d(vertexEc,normalEc,ambient,diffuse,specular);\n",lightIndex);
            vertexShaderMain+=call;
            }

    /* Finish the main function: */
    vertexShaderMain+=
        "\
            \n\
            /* Compute final vertex color: */\n\
            gl_FrontColor = color;\n\
            \n\
            /* Use finalize vertex transformation: */\n\
            gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);\n\
            \n\
            /* Pass the texture coordinates to the fragment program: */\n\
            texCoord = gl_Vertex.xy;\n\
            }\n";

    /* Compile the vertex shader: */
    std::string vertexShaderSource = vertexShaderUniforms  +
                                     vertexShaderFunctions +
                                     vertexShaderMain;
    glCompileShaderFromString(vertexShader,vertexShaderSource.c_str());

    /* Compile the standard fragment shader: */
    std::string fragmentShaderSource;
    std::string progFile(CRUSTA_SHARE_PATH);
    if (linesDecorated)
        progFile += "/decoratedRenderer.fp";
    else
        progFile += "/plainRenderer.fp";
    readFileToString(progFile.c_str(), fragmentShaderSource);

try{
    glCompileShaderFromString(fragmentShader,fragmentShaderSource.c_str());
}
catch (std::exception& e){
    std::cerr << e.what() << std::endl;
    glCompileShaderFromString(fragmentShader, "void main(){gl_FragColor=vec4(1.0);}");
}

    /* Link the program object: */
    glLinkProgramARB(programObject);

    /* Check if the program linked successfully: */
    GLint linkStatus;
    glGetObjectParameterivARB(programObject,GL_OBJECT_LINK_STATUS_ARB,&linkStatus);
    if(!linkStatus)
    {
        /* Get some more detailed information: */
        GLcharARB linkLogBuffer[2048];
        GLsizei linkLogSize;
        glGetInfoLogARB(programObject,sizeof(linkLogBuffer),&linkLogSize,linkLogBuffer);

#if 1
        std::cerr << linkLogBuffer << std::endl;
        glCompileShaderFromString(fragmentShader,
                                  "void main(){gl_FragColor=vec4(1.0);}");
        glLinkProgramARB(programObject);
#else
        /* Signal an error: */
        Misc::throwStdErr("Error \"%s\" while linking shader program",linkLogBuffer);
#endif
    }

    glUseProgramObjectARB(programObject);
    //setup "constant uniforms"
    GLint uniform;
    uniform = glGetUniformLocationARB(programObject, "geometryTex");
    glUniform1i(uniform, 0);
    uniform = glGetUniformLocationARB(programObject, "heightTex");
    glUniform1i(uniform, 1);
    uniform = glGetUniformLocationARB(programObject, "colorTex");
    glUniform1i(uniform, 2);

    uniform = glGetUniformLocationARB(programObject, "colorMap");
    glUniform1i(uniform, 6);

    colorMapElevationInvRangeUniform =
        glGetUniformLocationARB(programObject, "colorMapElevationInvRange");
    minColorMapElevationUniform =
        glGetUniformLocationARB(programObject, "minColorMapElevation");
    textureStepUniform  =glGetUniformLocationARB(programObject,"texStep");
    verticalScaleUniform=glGetUniformLocationARB(programObject,"verticalScale");
    centroidUniform     =glGetUniformLocationARB(programObject,"center");


    if (linesDecorated)
    {
        uniform = glGetUniformLocationARB(programObject, "lineDataTex");
        glUniform1i(uniform, 3);
        uniform = glGetUniformLocationARB(programObject, "lineCoverageTex");
        glUniform1i(uniform, 4);
        uniform = glGetUniformLocationARB(programObject, "symbolTex");
        glUniform1i(uniform, 5);

        uniform = glGetUniformLocationARB(programObject, "lineStartCoord");
        glUniform1f(uniform, crusta::Crusta::lineDataStartCoord);
        uniform = glGetUniformLocationARB(programObject, "lineCoordStep");
        glUniform1f(uniform, crusta::Crusta::lineDataCoordStep);

        lineNumSegmentsUniform =
            glGetUniformLocationARB(programObject, "lineNumSegments");
        lineCoordScaleUniform =
            glGetUniformLocationARB(programObject, "lineCoordScale");
        lineWidthUniform = glGetUniformLocationARB(programObject, "lineWidth");
    }

    glUseProgramObjectARB(0);
}

void LightingShader::enable()
    {
    /* Enable the shader: */
    glUseProgramObjectARB(programObject);
    }

void LightingShader::disable()
    {
    /* Disable the shader: */
    glUseProgramObjectARB(0);
    }
