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
#include <Misc/ThrowStdErr.h>
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

SphereNodeDisplayList::SphereNodeDisplayList(Scalar radius, int detail):
  radius(radius),
  detail(detail)
{
}

void SphereNodeDisplayList::createList(GLContextData& renderState) const
{
  if (detail < 3)
    { glDrawSphereMercatorWithTexture(radius, 3, 3+detail); }
  else
    { glDrawSphereMercatorWithTexture(radius, detail, detail*2); }
}

SphereNode::SphereNode():
  radius(Scalar(1.0))
{
  detail.appendValue(COARSE);
  lodRatio.appendValue(COARSE_TO_FINE);
  detail.appendValue(FINE);
  lodRatio.appendValue(FINE_TO_FINEST);
  detail.appendValue(FINEST);
}

SphereNode::~SphereNode()
{
  while (!spheres.empty()) {
    delete spheres.back();
    spheres.pop_back();
  }
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
  std::string f = fieldName;
  if (f == "radius")
    return makeEventOut(this,radius);
  else if (f == "detail")
    return makeEventOut(this,detail);
  else if (f == "lodRatio")
    return makeEventOut(this,lodRatio);
  else
    return GeometryNode::getEventOut(fieldName);
}

EventIn* SphereNode::getEventIn(const char* fieldName)
{
  std::string f = fieldName;
  if(f == "radius")
    return makeEventIn(this,radius);
  else if (f == "detail")
    return makeEventIn(this,detail);
  else if (f == "lodRatio")
    return makeEventIn(this,lodRatio);
  else
    return GeometryNode::getEventIn(fieldName);
}

void SphereNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
{
  std::string f = fieldName;
  if(f == "radius")
    return vrmlFile.parseField(radius);
  else if (f == "detail")
    return vrmlFile.parseField(detail);
  else if (f == "lodRatio")
    return vrmlFile.parseField(lodRatio);
  else
    GeometryNode::parseField(fieldName,vrmlFile);
}

void SphereNode::update()
{
  if (lodRatio.getNumValues()+1 != detail.getNumValues() && detail.getNumValues() != 1)
    Misc::throwStdErr("SphereNode: there must be one lodRatio per detail transition (|lodRatio|+1 != |detail|)");
  for (MFInt::ValueList::const_iterator i = detail.getValues().begin(); i != detail.getValues().end(); ++i) {
    spheres.push_back(new SphereNodeDisplayList(radius.getValue(), *i));
    spheres.back()->update();
  }
}

Box SphereNode::calcBoundingBox() const
{
  Scalar r=radius.getValue();
  return Box(Point(-r,-r,-r),Point(r,r,r));
}

void SphereNode::glRenderAction(GLRenderState& renderState) const
{
  Scalar current_ratio = Geometry::sqrDist(renderState.getViewerPos(), Point::origin) / radius.getValue();
  renderState.enableCulling(GL_BACK);
  if (detail.getNumValues() == 1) {
    spheres.front()->glRenderAction(renderState.contextData);
  } else {
    std::vector<SphereNodeDisplayList*>::const_reverse_iterator sphere = spheres.rbegin();
    MFFloat::ValueList::const_reverse_iterator ratio = lodRatio.getValues().rbegin();
    while (sphere != spheres.rend() && ratio != lodRatio.getValues().rend()) {
      if (current_ratio < *ratio) break;
      ++sphere; ++ratio;
    }
    (*sphere)->glRenderAction(renderState.contextData);
  }
}

}
