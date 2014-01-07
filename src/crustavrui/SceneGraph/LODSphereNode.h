/***********************************************************************
LODSphereNode - Node class for sphere shapes.
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

#ifndef SCENEGRAPH_LODSPHERENODE_INCLUDED
#define SCENEGRAPH_LODSPHERENODE_INCLUDED

#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/DisplayList.h>
#include <SceneGraph/GeometryNode.h>

namespace SceneGraph {

class LODSphereNodeDisplayList: public DisplayList
{
  public:
  LODSphereNodeDisplayList(Scalar radius, int detail);
  void createList(GLContextData& contextData) const;
  Scalar radius;
  int detail;
};

class LODSphereNode: public GeometryNode
{
  public:
  LODSphereNode();
  ~LODSphereNode();
  
  // class Node
  static const char* getStaticClassName();
  const char* getClassName() const;
  EventOut* getEventOut(const char* fieldName) const;
  EventIn* getEventIn(const char* fieldName);
  void parseField(const char* fieldName,VRMLFile& vrmlFile);
  void update();
  
  // class GeometryNode
  Box calcBoundingBox() const;
  void glRenderAction(GLRenderState& renderState) const;

  SFFloat radius;
  MFInt detail;
  MFFloat lodRatio;
  std::vector<LODSphereNodeDisplayList*> spheres;
};

}

#endif
