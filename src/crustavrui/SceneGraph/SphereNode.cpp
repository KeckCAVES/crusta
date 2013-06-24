/***********************************************************************
SphereNode - Node class for sphere shapes.
Copyright (c) 2013 Braden Pellett
Copyright (c) 2008 Oliver Kreylos

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

// Tuned manually
#define COARSE 0
#define FINE 6
#define FINEST 12
#define COARSE_TO_FINE 10000000000
#define FINE_TO_FINEST 100000000

namespace SceneGraph {

SphereNodeDisplayList::SphereNodeDisplayList(Scalar radius, int fineness):
  radius(radius),
  fineness(fineness)
{
  if (fineness < 0) fineness = 0;
}

void SphereNodeDisplayList::createList(GLContextData& renderState) const
{
  if (fineness < 3)
    { glDrawSphereMercatorWithTexture(radius, 3, 3+fineness); }
  else
    { glDrawSphereMercatorWithTexture(radius, fineness, fineness*2); }
}

SphereNode::SphereNode():
  radius(Scalar(1.0)),
  coarse(1, COARSE),
  fine(1, FINE),
  finest(1, FINEST)
{
}

const char* SphereNode::getStaticClassName()
{
  return "Sphere";
}

const char* SphereNode::getClassName() const
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

void SphereNode::update()
{
  coarse.radius = radius.getValue();
  fine.radius = radius.getValue();
  finest.radius = radius.getValue();
  coarse.update();
  fine.update();
  finest.update();
}

Box SphereNode::calcBoundingBox() const
{
  Scalar r=radius.getValue();
  return Box(Point(-r,-r,-r),Point(r,r,r));
}

void SphereNode::glRenderAction(GLRenderState& renderState) const
{
  Scalar ratio = Geometry::sqrDist(renderState.getViewerPos(), Point::origin) / radius.getValue();
  renderState.enableCulling(GL_BACK);
  if (ratio < FINE_TO_FINEST)
    { finest.glRenderAction(renderState.contextData); }
  if (ratio < COARSE_TO_FINE)
    { fine.glRenderAction(renderState.contextData); }
  else
    { coarse.glRenderAction(renderState.contextData); }
}

}
