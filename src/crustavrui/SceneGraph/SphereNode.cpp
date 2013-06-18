/***********************************************************************
SphereNode - Node class for sphere shapes.
Copyright (c) 2008 Oliver Kreylos
Copyright (c) 2013 Braden Pellett

This file is part of the Crusta Virtual Globe.

The Crusta Virtual Globe is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

The Crusta Virtual Globe is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Crusta Virtual Globe; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <crustavrui/SceneGraph/SphereNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***************************
Methods of class SphereNode:
***************************/

void SphereNode::createList(GLContextData& renderState) const
	{
	glDrawSphereMercatorWithTexture(radius.getValue(),45,90);
	}

SphereNode::SphereNode(void)
	:radius(Scalar(1.0))
	{
	}

const char* SphereNode::getStaticClassName(void)
	{
	return "Sphere";
	}

const char* SphereNode::getClassName(void) const
	{
	return "Sphere";
	}

EventOut* SphereNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"radius")==0)
		return makeEventOut(this,radius);
	else
		return GeometryNode::getEventOut(fieldName);
	}

EventIn* SphereNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"radius")==0)
		return makeEventIn(this,radius);
	else
		return GeometryNode::getEventIn(fieldName);
	}

void SphereNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"radius")==0)
		{
		vrmlFile.parseField(radius);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void SphereNode::update(void)
	{
	/* Invalidate the display list: */
	DisplayList::update();
	}

Box SphereNode::calcBoundingBox(void) const
	{
  Scalar r=radius.getValue();
	return Box(Point(-r,-r,-r),Point(r,r,r));
	}

void SphereNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.enableCulling(GL_BACK);
	
	/* Render the display list: */
	DisplayList::glRenderAction(renderState.contextData);
	}

}
