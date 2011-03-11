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

#include <crusta/ColorMapper.h>
#include <crusta/DataManager.h>
///\todo integrate properly (VIS 2010)
#include <crusta/Crusta.h>

///\todo remove debug
#include <sys/stat.h>
#include <fstream>


BEGIN_CRUSTA


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
    lightStates(0),
    vertexShader(0),fragmentShader(0), geometryShader(0),
    programObject(0),
    decoratedLineRenderer(std::string(CRUSTA_SHARE_PATH) +
                          "/decoratedRenderer.fp"),
    colorMapperConfigurationStamp(0)
{
    /* Determine the maximum number of light sources supported by the local OpenGL: */
    glGetIntegerv(GL_MAX_LIGHTS,&maxNumLights);

    /* Initialize the light state array: */
    lightStates=new LightState[maxNumLights];
    updateLightingState();

    /* Create the vertex and fragment shaders: */
    vertexShader   = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    geometryShader = glCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB);

    /* Create the program object: */
    programObject=glCreateProgramObjectARB();
    glAttachObjectARB(programObject,vertexShader);
    glAttachObjectARB(programObject,fragmentShader);
    glAttachObjectARB(programObject,geometryShader);
    glProgramParameteriEXT(programObject, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
    glProgramParameteriEXT(programObject, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
    glProgramParameteriEXT(programObject, GL_GEOMETRY_VERTICES_OUT_EXT, 32); // FIXME

    clearUniforms();
}

LightingShader::~LightingShader()
{
    glDeleteObjectARB(programObject);
    glDeleteObjectARB(vertexShader);
    glDeleteObjectARB(fragmentShader);
    glDeleteObjectARB(geometryShader);

    delete[] lightStates;
}


void LightingShader::
update(GLContextData& contextData)
{
    /* Update light states and recompile the shader if necessary: */
    updateLightingState();

    if (linesDecorated)
        mustRecompile |= decoratedLineRenderer.update();

    //check that the color mapper hasn't changed
    if (colorMapperConfigurationStamp <
        COLORMAPPER->getMapperConfigurationStamp())
    {
        mustRecompile = true;
        colorMapperConfigurationStamp =
            COLORMAPPER->getMapperConfigurationStamp();
    }

    if (mustRecompile)
    {
        compileShader(contextData);

        glUseProgramObjectARB(programObject);
        initUniforms(contextData);
        glUseProgramObjectARB(0);

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

void LightingShader::
initUniforms(GLContextData& contextData)
{
    clearUniforms();

    //setup "constant uniforms"
    GLint uniform;
    uniform = glGetUniformLocation(programObject, "geometryTex");
    glUniform1i(uniform, 0);
    uniform = glGetUniformLocation(programObject, "colorTex");
    glUniform1i(uniform, 1);
    uniform = glGetUniformLocation(programObject, "layerfTex");
    glUniform1i(uniform, 2);

    uniform = glGetUniformLocation(programObject, "colorMapTex");
    glUniform1i(uniform, 6);

    textureStepUniform  =glGetUniformLocation(programObject,"texStep");
    verticalScaleUniform=glGetUniformLocation(programObject,"verticalScale");

    colorNodataUniform  = glGetUniformLocation(programObject, "colorNodata");
    layerfNodataUniform = glGetUniformLocation(programObject, "layerfNodata");
    demDefaultUniform   = glGetUniformLocation(programObject, "demDefault");

    slicePlaneMatrixUniform = glGetUniformLocation(programObject, "slicePlaneMatrix");
    sliceShiftVecUniform    = glGetUniformLocation(programObject, "sliceShiftVec");
    slicePlaneCenterUniform = glGetUniformLocation(programObject, "slicePlaneCenter");
    sliceFaultCenterUniform = glGetUniformLocation(programObject, "sliceFaultCenter");
    sliceFalloffUniform     = glGetUniformLocation(programObject, "sliceFalloff");

    DataManager::SourceShaders& dataSources =
        DATAMANAGER->getSourceShaders(contextData);
    dataSources.topography.initUniforms(programObject);
    typedef DataManager::Shader2dAtlasDataSources::iterator Iterator;
    for (Iterator it=dataSources.layers.begin(); it!=dataSources.layers.end();
         ++it)
    {
        it->initUniforms(programObject);
    }
    COLORMAPPER->getColorSource(contextData)->initUniforms(programObject);;

    if (linesDecorated)
    {
        decoratedLineRenderer.setSources(&dataSources.coverage,
                                         &dataSources.lineData);
        decoratedLineRenderer.initUniforms(programObject);
    }
}

void LightingShader::
compileShader(GLContextData& contextData)
{
    std::string vertexShaderUniforms;
    std::string vertexShaderFunctions;
    std::string vertexShaderMain;

    //import the shaders from the data manager and color mapper
    DataManager::SourceShaders& dataSources =
        DATAMANAGER->getSourceShaders(contextData);
    dataSources.reset();
    ShaderDataSource& colorSource = *COLORMAPPER->getColorSource(contextData);
    colorSource.reset();

    vertexShaderUniforms +=
    "\
        #extension GL_EXT_gpu_shader4   : enable\n\
        #extension GL_EXT_texture_array : enable\n\
        \n\
        uniform sampler2DArray geometryTex;\n\
        uniform sampler2DArray colorTex;\n\
        uniform sampler2DArray layerfTex;\n\
        uniform sampler2D colorMapTex;\n\
        \n\
        uniform float texStep;\n\
        uniform float verticalScale;\n\
        \n\
        uniform vec3  colorNodata;\n\
        uniform float layerfNodata;\n\
        uniform float demDefault;\n\
        \n\
        varying vec3 position;\n\
        varying vec3 normal;\n\
        varying vec2 texCoord;\n\
        \n\
    ";

    vertexShaderFunctions += dataSources.topography.getCode();
    vertexShaderFunctions += colorSource.getCode();

    vertexShaderFunctions +=
    "\
        vec3 surfacePoint(in vec2 coords)\n\
        {\n\
            return " + dataSources.topography.sample("coords") + ";\n\
        }\n\
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
            float minTexCoord = texStep * 0.5;\n\
            float maxTexCoord = 1.0 - (texStep * 0.5);\n\
            \n\
            vec2 ncoord = vec2(coord.x, coord.y + texStep);\n\
            ncoord.y    = clamp(ncoord.y, minTexCoord, maxTexCoord);\n\
            vec3 up     = surfacePoint(ncoord);\n\
            \n\
            ncoord      = vec2(coord.x, coord.y - texStep);\n\
            ncoord.y    = clamp(ncoord.y, minTexCoord, maxTexCoord);\n\
            vec3 down   = surfacePoint(ncoord);\n\
            \n\
            ncoord      = vec2(coord.x - texStep, coord.y);\n\
            ncoord.x    = clamp(ncoord.x, minTexCoord, maxTexCoord);\n\
            vec3 left   = surfacePoint(ncoord);\n\
            \n\
            ncoord      = vec2(coord.x + texStep, coord.y);\n\
            ncoord.x    = clamp(ncoord.x, minTexCoord, maxTexCoord);\n\
            vec3 right  = surfacePoint(ncoord);\n\
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
        vec4 terrainColor = " + colorSource.sample("coord") + ";\n\
        ";

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
            /* now happens in the geometry shader */\n\
            //gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);\n\
            gl_Position = vec4(position, 1.0);\n\
            \n\
            /* Pass the texture coordinates to the fragment program: */\n\
            texCoord = gl_Vertex.xy;\n\
            }\n";

    /* Compile the vertex shader: */
    std::string vertexShaderSource = vertexShaderUniforms  +
                                     vertexShaderFunctions +
                                     vertexShaderMain;

CRUSTA_DEBUG(80, CRUSTA_DEBUG_OUT << vertexShaderSource << std::endl;)

    compileShaderFromString(vertexShader,vertexShaderSource.c_str());


//------------------------------------------------------------------------------


    /* Compile the standard fragment shader: */
    std::string fragmentShaderSource;

    if (linesDecorated)
    {
        decoratedLineRenderer.setSources(&dataSources.coverage,
                                         &dataSources.lineData);
        decoratedLineRenderer.reset();
        fragmentShaderSource += decoratedLineRenderer.getCode() + "\n\n";
    }
    else
    {
        fragmentShaderSource += "void render(inout vec4 fragColor) {}\n\n";
    }

    fragmentShaderSource += "\
void main()\n\
{\n\
  /* setup the default color to whatever the vertex program computed */\n\
  gl_FragColor = gl_Color;\n\
  /** render the fragment */\n\
  render(gl_FragColor);\n\
}\n\
";

CRUSTA_DEBUG(80, CRUSTA_DEBUG_OUT << fragmentShaderSource << std::endl;)

    try
    {
        compileShaderFromString(fragmentShader,fragmentShaderSource.c_str());
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        compileShaderFromString(fragmentShader,
                                "void main() { gl_FragColor=vec4(1.0); }");
    }

    /* Compile geometry shader (for slicing planes) */
    std::string geometryShaderSource;

    geometryShaderSource += "\
        #version 120\n\
        #extension GL_EXT_geometry_shader4 : enable\n\
        \n\
        uniform mat4 slicePlaneMatrix;\n\
        uniform vec3 sliceShiftVec;\n\
        uniform vec3 slicePlaneCenter;\n\
        uniform vec3 sliceFaultCenter;\n\
        uniform float sliceFalloff;\n\
        uniform vec3 center;\n\
        \n\
        vec4 isect(vec4 a, vec4 b) {\n\
            float lambda = -(slicePlaneMatrix * vec4(a.xyz,1)).x / (slicePlaneMatrix * vec4((b-a).xyz, 0)).x;\n\
            return a + lambda * (b-a);\n\
        }\n\
        void tri(vec4 a, vec4 b, vec4 c, vec4 col) {\n\
            gl_Position = gl_ModelViewProjectionMatrix * a;\n\
            EmitVertex();\n\
            gl_Position = gl_ModelViewProjectionMatrix * b;\n\
            EmitVertex();\n\
            gl_Position = gl_ModelViewProjectionMatrix * c;\n\
            EmitVertex();\n\
            EndPrimitive();\n\
        }\n\
        void quad(vec4 a, vec4 b, vec4 col) {\n\
            tri(b,a, vec4(slicePlaneCenter, 1), col);\n\
        }\n\
        vec4 shift(vec4 p) {\n\
            float d = length(p.xyz - sliceFaultCenter);\n\
            float lambda = clamp(2 * (sliceFalloff - d) / sliceFalloff, 0.0, 1.0);\n\
            return p + lambda * vec4(sliceShiftVec, 0.0);\n\
        }\n\
        void main(void) {\n\
            int i;\n\
            int leftBitmap = 0;\n\
            for (i=0; i < gl_VerticesIn; i++) {\n\
                vec4 p = gl_PositionIn[i];\n\
                float slicePlaneDst = (slicePlaneMatrix * p).x;\n\
                if (slicePlaneDst < 0.0)\n\
                    leftBitmap |= 1 << i;\n\
            }\n\
            if (length(sliceShiftVec) == 0)\n\
                leftBitmap = 7;\n\
            if (leftBitmap == 0 || leftBitmap == 7) {\n\
                for (i=0; i < gl_VerticesIn; i++) {\n\
                    gl_FrontColor = gl_FrontColorIn[i];\n\
                    vec4 p = gl_PositionIn[i];\n\
                    if (leftBitmap == 0)\n\
                        p = shift(p);\n\
                    gl_Position = gl_ModelViewProjectionMatrix * p;\n\
                    EmitVertex();\n\
                }\n\
                EndPrimitive();\n\
                return;\n\
            }\n\
            vec4 a = gl_PositionIn[0];\n\
            vec4 b = gl_PositionIn[1];\n\
            vec4 c = gl_PositionIn[2];\n\
            \n\
            vec4 ab = isect(a,b);\n\
            vec4 bc = isect(b,c);\n\
            vec4 ca = isect(c,a);\n\
            \n\
            vec4 red = 0*vec4(1,0,0,0);\n\
            vec4 blue = 0*vec4(0,0,1,0);\n\
            vec4 green = 0*vec4(0,1,0,0);\n\
            vec4 yellow = vec4(1,1,0,0);\n\
            vec4 planeColor = vec4(0.3,0.3,0.4,0);\n\
            gl_FrontColor = gl_FrontColorIn[0];\n\
            switch (leftBitmap) {\n\
                case 1:\n\
                    tri(a,ab,ca, red);\n\
                    tri(shift(ab),shift(b),shift(c), blue);\n\
                    tri(shift(ab),shift(c),shift(ca), green);\n\
                    gl_FrontColor = planeColor;\n\
                    quad(ab,ca, yellow);\n\
                    quad(shift(ca),shift(ab), yellow);\n\
                    break;\n\
                case 2:\n\
                    tri(b,bc,ab, red);\n\
                    tri(shift(bc),shift(c),shift(a), blue);\n\
                    tri(shift(bc),shift(a),shift(ab), green);\n\
                    gl_FrontColor = planeColor;\n\
                    quad(bc,ab, yellow);\n\
                    quad(shift(ab),shift(bc), yellow);\n\
                    break;\n\
                case 4:\n\
                    tri(c,ca,bc, red);\n\
                    tri(shift(ca),shift(a),shift(b), blue);\n\
                    tri(shift(ca),shift(b),shift(bc), green);\n\
                    gl_FrontColor = planeColor;\n\
                    quad(ca,bc, yellow);\n\
                    quad(shift(bc),shift(ca), yellow);\n\
                    break;\n\
                case 3:\n\
                    tri(b,bc,ca,blue);\n\
                    tri(b,ca,a, green);\n\
                    tri(shift(bc),shift(c),shift(ca), red);\n\
                    gl_FrontColor = planeColor;\n\
                    quad(bc,ca, yellow);\n\
                    quad(shift(ca),shift(bc), yellow);\n\
                    break;\n\
                case 5:\n\
                    tri(a,ab,bc, blue);\n\
                    tri(a,bc,c, green);\n\
                    tri(shift(ab),shift(b),shift(bc), red);\n\
                    gl_FrontColor = planeColor;\n\
                    quad(ab,bc, yellow);\n\
                    quad(shift(bc),shift(ab), yellow);\n\
                    break;\n\
                case 6:\n\
                    tri(c,ca,ab, blue);\n\
                    tri(c,ab,b, green);\n\
                    tri(shift(ca),shift(a),shift(ab), red);\n\
                    gl_FrontColor = planeColor;\n\
                    quad(ca,ab, yellow);\n\
                    quad(shift(ab),shift(ca), yellow);\n\
                    break;\n\
                }\n\
        }\n\
    ";
    // + (b-a) * (abs((slicePlaneMatrix * a) / (slicePlaneMatrix * (b-a))));\n\
     //a = a + (b-a) * (abs((slicePlaneMatrix * a) / (slicePlaneMatrix * (b-a))));
/*

                if (slicePlaneDst > 0.0)\n\

                    */
/*
    float lambda = -(slicePlaneMatrix * vec4(a.xyz,1)).x / (slicePlaneMatrix * vec4((b-a).xyz, 0)).x;\n\
    vec4 ab = a + lambda * (b-a);\n\
*/
    //code << "  float slicePlaneDst = (" << slicePlaneMatrixName << "* vec4(res + " << centroidName << ", 1.0)).x;" << std::endl;
    //code << "  if (slicePlaneDst > 0.0)" << std::endl;
    //code << "    res += " << sliceShiftVecName << ";" << std::endl;

    CRUSTA_DEBUG(80, CRUSTA_DEBUG_OUT << geometryShaderSource << std::endl;)

    compileShaderFromString(geometryShader, geometryShaderSource.c_str());

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

        std::cerr << linkLogBuffer << std::endl;
        compileShaderFromString(fragmentShader,
                                  "void main(){gl_FragColor=vec4(1.0);}");
        glLinkProgram(programObject);
    }
}

void LightingShader::
compileShaderFromString(GLhandleARB shaderObject,const char* shaderSource)
{
    /* Determine the length of the source string: */
    GLint shaderSourceLength=GLint(strlen(shaderSource));

    /* Upload the shader source into the shader object: */
    const GLcharARB* ss=reinterpret_cast<const GLcharARB*>(shaderSource);
    glShaderSource(shaderObject,1,&ss,&shaderSourceLength);

    /* Compile the shader source: */
    glCompileShader(shaderObject);

    /* Check if the shader compiled successfully: */
    GLint compileStatus;
    glGetObjectParameterivARB(shaderObject,GL_OBJECT_COMPILE_STATUS_ARB,
                              &compileStatus);
    if(!compileStatus)
    {
        /* Get some more detailed information: */
        GLcharARB compileLogBuffer[2048];
        GLsizei compileLogSize;
        glGetInfoLogARB(shaderObject, sizeof(compileLogBuffer),
                        &compileLogSize, compileLogBuffer);

        /* Signal an error: */
        Misc::throwStdErr("glCompileShaderFromString: Error \"%s\" while compiling shader",compileLogBuffer);
    }
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


END_CRUSTA
