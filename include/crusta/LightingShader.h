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

#ifndef _LightingShader_H_
#define _LightingShader_H_

#include <cassert>
#include <iostream>
#include <string>

#include <GL/VruiGlew.h>

#include <crusta/basics.h>
#include <crusta/QuadNodeData.h>


BEGIN_CRUSTA


class LightingShader
{
    /* Embedded classes: */
    private:
    struct LightState // Structure tracking salient state of OpenGL light sources for just-in-time shader compilation
    {
        /* Elements: */
        public:
        bool enabled; // Flag whether the light source is enabled
        bool attenuated; // Flag whether the light source uses non-constant attenuation
        bool spotLight; // Flag whether the light source is a spot light

        /* Constructors and destructors: */
        LightState(void)
            :enabled(false),attenuated(false),spotLight(false)
            {
            }
    };

    /* Elements: */
    private:
    static const char* applyLightTemplate;
    static const char* applyAttenuatedLightTemplate;
    static const char* applySpotLightTemplate;
    static const char* applyAttenuatedSpotLightTemplate;

    bool mustRecompile;
    bool colorMaterial; // Flag whether material color tracking is enabled
    bool linesDecorated; // Flag whether decorated lines should be processed

    int maxNumLights; // Maximum number of lights supported by local OpenGL
    LightState* lightStates; // Array of tracking states for each OpenGL light source
    GLhandleARB vertexShader,fragmentShader; // Handle for the vertex and fragment shaders
    GLhandleARB programObject; // Handle for the linked program object

    GLint textureStepUniform;
    GLint verticalScaleUniform;
    GLint centroidUniform;

    GLint lineNumSegmentsUniform;
    GLint lineCoordScaleUniform;
    GLint lineWidthUniform;

    GLint colorNodataUniform;
    GLint layerfNodataUniform;
    GLint demDefaultUniform;

    /* Private methods: */
    std::string createApplyLightFunction(const char* functionTemplate,int lightIndex) const;

    /* Constructors and destructors: */
    public:
    LightingShader(void);
    ~LightingShader(void);

    /* Methods: */
    void update(GLContextData& contextData);
    void updateLightingState(); // Updates tracked lighting state; returns true if shader needs to be recompiled
    void initUniforms(GLContextData& contextData);
    void compileShader(GLContextData& contextData); // Recompiles the point-based lighting shader based on the current states of all OpenGL light sources
    void compileShaderFromString(GLhandleARB shaderObject,const char* shaderSource);
    void enable(void); // Enables point-based lighting in the current OpenGL context
    void disable(void); // Disables point-based lighting in the current OpenGL context

    void setLinesDecorated(bool flag)
    {
        if (linesDecorated != flag)
        {
            linesDecorated = flag;
            mustRecompile  = true;
        }
    }

    void setTextureStep(float ts)
    {
        glUniform1f(textureStepUniform, ts);
    }
    void setVerticalScale(float vs)
    {
        glUniform1f(verticalScaleUniform, vs);
    }
    void setCentroid(const Point3f& c)
    {
        glUniform3f(centroidUniform, c[0], c[1], c[2]);
    }

    void setLineNumSegments(int ns)
    {
        glUniform1i(lineNumSegmentsUniform, ns);
    }
    void setLineCoordScale(float lcs)
    {
        glUniform1f(lineCoordScaleUniform, lcs);
    }
    void setLineWidth(float lw)
    {
        glUniform1f(lineWidthUniform, lw);
    }

    void setColorNodata(const TextureColor::Type& tc)
    {
        glUniform3f(colorNodataUniform, tc[0]/255.0f,tc[1]/255.0f,tc[2]/255.0f);
    }
    void setLayerfNodata(float ln)
    {
        glUniform1f(layerfNodataUniform, ln);
    }
    void setDemDefault(float dn)
    {
        glUniform1f(demDefaultUniform, dn);
    }
};


END_CRUSTA


#endif //_LightingShader_H_
