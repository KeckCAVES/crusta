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

#include <crustavrui/SceneGraph/FTFontStyleNode.h>

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <SceneGraph/VRMLFile.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h> //DEBUG

namespace SceneGraph {

FTFontStyleNode::FTFontStyleNode():
  family("Sans"),
  style("Plain"),
  language(""),
  size(Scalar(1)),
  spacing(Scalar(1)),
  horizontal(true),
  leftToRight(true),
  topToBottom(true)
{
}

FTFontStyleNode::~FTFontStyleNode()
{
  delete ftfont;
}

const char* FTFontStyleNode::getStaticClassName()
{
  return "FTFontStyle";
}

const char* FTFontStyleNode::getClassName() const
{
  return "FTFontStyle";
}

void FTFontStyleNode::parseField(const char* fieldName, VRMLFile& vrmlFile)
{
  std::string f(fieldName);
  if (f == "family") {vrmlFile.parseField(family);}
  else if (f == "style") {vrmlFile.parseField(style);}
  else if (f == "language") {vrmlFile.parseField(language);}
  else if (f == "size") {vrmlFile.parseField(size);}
  else if (f == "spacing") {vrmlFile.parseField(spacing);}
  else if (f == "justify") {vrmlFile.parseField(justify);}
  else if (f == "horizontal") {vrmlFile.parseField(horizontal);}
  else if (f == "leftToRight") {vrmlFile.parseField(leftToRight);}
  else if (f == "topToBottom") {vrmlFile.parseField(topToBottom);}
  else {Node::parseField(fieldName,vrmlFile);}
}

void FTFontStyleNode::update()
{
  char* path;
  {
    if (!FcInit()) Misc::throwStdErr("Fontconfig: Could not init");
    FcConfig* config = FcConfigGetCurrent();
    FcPattern* pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FONTFORMAT, (FcChar8*)"TrueType");
    FcObjectSet* os = FcObjectSetBuild(FC_FILE, FC_FAMILY, FC_STYLE, NULL);
    FcFontSet* ttfonts = FcFontList(config, pattern, os);
    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8*)family.getValue(0).c_str());
    FcPatternAddString(pattern, FC_STYLE, (const FcChar8*)style.getValue().c_str());
    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult resultMatch = FcResultMatch;
    FcPattern* match = FcFontSetMatch(config, &ttfonts, 1, pattern, &resultMatch);
    if (FcPatternGetString(match, FC_FILE, 0, (FcChar8**)&path) == FcResultMatch) {
      fprintf(stderr, "Fontconfig: using font %s\n", path);
    } else {
      Misc::throwStdErr("Fontconfig: Could not find matching font");
    }
    FcFini();
  }

  ftfont = new FTGLTextureFont(path);
  if (ftfont->Error()) Misc::throwStdErr("FTGLTextureFont: Could not create font");

  justify_major = FTFontStyle::BEGIN;
  justify_minor = FTFontStyle::FIRST;
  FTFontStyle::Justification* j[] = {&justify_major, &justify_minor};
  for (unsigned int i=0; i<2; ++i) {
    if (justify.getNumValues() >= i+1) {
      if (justify.getValue(i) == "BEGIN")
        *j[i] = FTFontStyle::BEGIN;
      else if (justify.getValue(i) == "FIRST")
        *j[i] = FTFontStyle::FIRST;
      else if (justify.getValue(i) == "MIDDLE")
        *j[i] = FTFontStyle::MIDDLE;
      else if (justify.getValue(i) == "END")
        *j[i] = FTFontStyle::END;
    }
  }
}

}
#endif
