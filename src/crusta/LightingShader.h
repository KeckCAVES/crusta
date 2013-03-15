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

#include <crusta/GL/VruiGlew.h>

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/checkGl.h>
#include <crusta/QuadNodeData.h>
#include <crusta/shader/ShaderDecoratedLineRenderer.h>
#include <crusta/CrustaSettings.h>


namespace crusta {


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
    GLhandleARB vertexShader; // Handle for the vertex shader
    GLhandleARB fragmentShader; // Handle for fragment shader
    GLhandleARB geometryShader; // Handle for the geometry shader
    GLhandleARB programObject; // Handle for the linked program object

    ShaderDecoratedLineRenderer decoratedLineRenderer;

    GLint textureStepUniform;
    GLint verticalScaleUniform;

    GLint colorNodataUniform;
    GLint layerfNodataUniform;
    GLint demDefaultUniform;

    // Only needed for SliceTool
    GLint numPlanesUniform;
    GLint slicePlanesUniform;
    GLint separatingPlanesUniform;
    GLint slopePlanesUniform;
    GLint strikeShiftAmountUniform;
    GLint dipShiftAmountUniform;
    GLint slopePlaneCentersUniform;
    GLint sliceFaultCenterUniform;
    GLint sliceFalloffUniform;
    GLint sliceColoringUniform;
    GLint strikeDirectionsUniform;
    GLint dipDirectionsUniform;

    FrameStamp colorMapperConfigurationStamp;

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
    ShaderDecoratedLineRenderer& getDecoratedLineRenderer()
    {
        return decoratedLineRenderer;
    }

    void clearUniforms()
    {
        textureStepUniform   = -2;
        verticalScaleUniform = -2;

        colorNodataUniform  = -2;
        layerfNodataUniform = -2;
        demDefaultUniform   = -2;

        if (SETTINGS->sliceToolEnable) {
            slicePlanesUniform = separatingPlanesUniform = strikeShiftAmountUniform = dipShiftAmountUniform = slopePlaneCentersUniform = -2;
            sliceFalloffUniform = -2;
            sliceColoringUniform = -2;
            strikeDirectionsUniform = dipDirectionsUniform = -2;
        }
    }

    void setTextureStep(float ts)
    {
        glUniform1f(textureStepUniform, ts);
        CHECK_GLA
    }
    void setVerticalScale(float vs)
    {
        glUniform1f(verticalScaleUniform, vs);
        CHECK_GLA
    }

    void setColorNodata(const TextureColor::Type& tc)
    {
        glUniform3f(colorNodataUniform, tc[0]/255.0f,tc[1]/255.0f,tc[2]/255.0f);
        CHECK_GLA
    }
    void setLayerfNodata(float ln)
    {
        glUniform1f(layerfNodataUniform, ln);
        CHECK_GLA
    }
    void setDemDefault(float dn)
    {
        glUniform1f(demDefaultUniform, dn);
        CHECK_GLA
    }

    // Planes are stored as contiguous 4-tuples of (nx,ny,ny,distance_to_origin)
    // they are assumed to be relative the centroid of this tile
    void setSlicePlanes(int numPlanes, float strikeDirections[3*63], float dipDirections[3*63],
                        float planes[4*63], float separatingPlanes[4*64], float slopePlanes[4*63],
                        double strikeShiftAmount, double dipShiftAmount, Geometry::Vector<double,3> planeCenters[63],
                        Geometry::Vector<double,3> faultCenter, double falloff, double coloring)
    {
        CHECK_GLA

        glUniform1i(numPlanesUniform, numPlanes);
        int numPoints = numPlanes ? (numPlanes+1) : 0;

        glUniform3fv(strikeDirectionsUniform, numPlanes, strikeDirections);
        glUniform3fv(dipDirectionsUniform, numPlanes, dipDirections);

        glUniform4fv(slicePlanesUniform, numPlanes, planes);
        glUniform4fv(separatingPlanesUniform, numPoints, separatingPlanes);
        glUniform4fv(slopePlanesUniform, numPlanes, slopePlanes);

        glUniform1f(strikeShiftAmountUniform, strikeShiftAmount);        
        glUniform1f(dipShiftAmountUniform, dipShiftAmount);

        float pc[3*63];
        for (int i=0; i < numPlanes; ++i) {
            pc[3*i+0] = planeCenters[i][0];
            pc[3*i+1] = planeCenters[i][1];
            pc[3*i+2] = planeCenters[i][2];
        }

        glUniform3fv(slopePlaneCentersUniform, numPlanes, pc);

        float fc[3] = { faultCenter[0], faultCenter[1], faultCenter[2] };
        glUniform3fv(sliceFaultCenterUniform, 1, fc);

        glUniform1f(sliceFalloffUniform, falloff);
        glUniform1f(sliceColoringUniform, coloring);
        CHECK_GLA
    }

};


} //namespace crusta


#endif //_LightingShader_H_
