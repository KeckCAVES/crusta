/***********************************************************************
FTTextNode - Class for nodes to render 3D text using FreeType.
Copyright (c) 2013 Braden Pellett
Copyright (c) 2009 Oliver Kreylos

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
#ifdef ENABLE_SCENEGRAPH_FTFONT

#ifndef SCENEGRAPH_FTTEXTNODE_INCLUDED
#define SCENEGRAPH_FTTEXTNODE_INCLUDED

#include <vector>
#include <Geometry/Box.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>

#include <crustavrui/SceneGraph/FTFontStyleNode.h>

namespace SceneGraph {

class FTTextNode;

class FTTextGLObject: public GLObject
{
public:
  FTTextGLObject(FTTextNode* text);
  ~FTTextGLObject();
  void initContext(GLContextData& contextData) const;
  void glRenderAction(GLRenderState& renderState) const;
  FTTextNode* text;
};

struct FTTextGLObjectDataItem: public GLObject::DataItem
{
  float offset;
};

class FTTextNode: public GeometryNode
{
  public:
  FTTextNode();
  ~FTTextNode();
  
  // class Node
  static const char* getStaticClassName(void);
  const char* getClassName() const;
  void parseField(const char* fieldName, VRMLFile& vrmlFile);
  void update();
  
  // class GeometryNode
  Box calcBoundingBox() const;
  void glRenderAction(GLRenderState& renderState) const;

  typedef SF<FTFontStyleNodePointer> SFFTFontStyleNode;
  MFString string;
  SFFTFontStyleNode fontStyle;
  MFFloat length;
  SFFloat maxExtent;
  FTTextGLObject glObject;
  Box boundingBox;
};

typedef Misc::Autopointer<FTTextNode> FTTextNodePointer;

}

#endif
#endif
