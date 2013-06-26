/***********************************************************************
FTFontStyleNode - Class for nodes defining the appearance and layout of
3D text using FreeType.
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

#ifndef SCENEGRAPH_FONTSTYLENODE_INCLUDED
#define SCENEGRAPH_FONTSTYLENODE_INCLUDED

#include <Misc/Autopointer.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>
#include <ftgl.h>

namespace SceneGraph {
class TextNode;
class LabelSetNode;
}

namespace SceneGraph {

namespace FTFontStyle {
  enum Justification {
    FIRST, BEGIN, MIDDLE, END
  };
}

class FTFontStyleNode: public Node
{
  public:
  MFString family;
  SFString style;
  SFString language;
  SFFloat size;
  SFFloat spacing;
  MFString justify;
  SFBool horizontal;
  SFBool leftToRight;
  SFBool topToBottom;
  
  FTFont* ftfont;
  FTFontStyle::Justification justify_major;
  FTFontStyle::Justification justify_minor;
  
  FTFontStyleNode();
  virtual ~FTFontStyleNode();
  
  // class Node
  static const char* getStaticClassName(void);
  virtual const char* getClassName(void) const;
  virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
  virtual void update(void);
  
  const FTFont* getFont() const { return ftfont; }
};

typedef Misc::Autopointer<FTFontStyleNode> FTFontStyleNodePointer;
}

#endif
#endif
