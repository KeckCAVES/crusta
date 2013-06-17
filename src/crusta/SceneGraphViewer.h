/***********************************************************************
SceneGraphViewer - Class to render a scene graph loaded from one or
more VRML 2.0 files, along with crusta-specific extensions.
Copyright (c) 2013 Braden Pellett
Copyright (c) 2009-2013 Oliver Kreylos

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

#ifndef _crusta_SceneGraphViewer_h
#define _crusta_SceneGraphViewer_h

#include <string>
#include <crusta/vrui.h>

namespace crusta {

class SceneGraphViewer
{
  public:
  SceneGraphViewer();
  ~SceneGraphViewer();
 
  void load(const std::string& name);
  void disable();
  void enable();
  void display(GLContextData& contextData) const;

  bool active;
  SceneGraph::NodeCreator nodeCreator;
  SceneGraph::GroupNodePointer root;
};

}

#endif
