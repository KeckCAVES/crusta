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

#include <crustavrui/SceneGraph/FTTextNode.h>

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h> //DEBUG

#define FACE_SIZE_FACTOR 100.0

namespace SceneGraph {

FTTextNode::FTTextNode():
  maxExtent(Scalar(0)),
  glObject(this)
{
}

FTTextNode::~FTTextNode()
{
}

const char* FTTextNode::getStaticClassName()
{
  return "FTText";
}

const char* FTTextNode::getClassName() const
{
  return "FTText";
}

void FTTextNode::parseField(const char* fieldName, VRMLFile& vrmlFile)
{
  std::string f(fieldName);
  if(f == "string") {vrmlFile.parseField(string);}
  else if(f == "fontStyle") {vrmlFile.parseSFNode(fontStyle);}
  else if(f == "length") {vrmlFile.parseField(length);}
  else if(f == "maxExtent") {vrmlFile.parseField(maxExtent);}
  else {GeometryNode::parseField(fieldName,vrmlFile);}
}

void FTTextNode::update()
{
  if (fontStyle.getValue() == 0) {
    fontStyle.setValue(new FTFontStyleNode);
    fontStyle.getValue()->update();
  }
  FTFontStyleNode* fs = fontStyle.getValue().getPointer();
  if (!fs->ftfont->FaceSize(fs->size.getValue()/FACE_SIZE_FACTOR, 1))
    Misc::throwStdErr("FTGL: Could not set font size");
}

Box FTTextNode::calcBoundingBox() const
{
  return boundingBox;
}

void FTTextNode::glRenderAction(GLRenderState& renderState) const
{
  glObject.glRenderAction(renderState);
}

FTTextGLObject::FTTextGLObject(FTTextNode* text):
  text(text)
{
}

FTTextGLObject::~FTTextGLObject()
{
}

void FTTextGLObject::initContext(GLContextData& contextData) const
{
  FTTextGLObjectDataItem* data = new FTTextGLObjectDataItem();
  FTFontStyleNode* fs = text->fontStyle.getValue().getPointer();
  contextData.addDataItem(this, data);
  FTBBox bbox = fs->ftfont->BBox(text->string.getValue(0).c_str());
  text->boundingBox = Box(Point(bbox.Lower().X(), bbox.Lower().Y(), bbox.Lower().Z()),
    Point(bbox.Upper().X(), bbox.Upper().Y(), bbox.Upper().Z())); // Dodgy
  float width = (bbox.Upper().X() - bbox.Lower().X());
  switch (fs->justify_major) {
    case FTFontStyle::FIRST:
    case FTFontStyle::BEGIN: data->offset = 0; break;
    case FTFontStyle::MIDDLE: data->offset = -width/2.0; break;
    case FTFontStyle::END: data->offset = -width; break;
    default: data->offset = 0; break;
  }
}

void FTTextGLObject::glRenderAction(GLRenderState& renderState) const
{
  if (text->string.getNumValues() == 0) return;

  FTTextGLObjectDataItem* data = renderState.contextData.retrieveDataItem<FTTextGLObjectDataItem>(this);
  FTFontStyleNode* fs = text->fontStyle.getValue().getPointer();

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glEnable(GL_ALPHA_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPushMatrix();

  glScalef(FACE_SIZE_FACTOR, FACE_SIZE_FACTOR, FACE_SIZE_FACTOR);
  //TODO allow multiple lines 
  fs->ftfont->Render(text->string.getValue(0).c_str(), -1, FTPoint(data->offset, 0, 0));

  glPopMatrix();
  glDisable(GL_BLEND);
  glPopAttrib();
}

}
#endif
