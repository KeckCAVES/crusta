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

#include <crusta/SceneGraphViewer.h>

namespace crusta {

SceneGraphViewer::SceneGraphViewer():
  active(true)
{
  // Load crusta-specific node types
  nodeCreator.registerNodeType(new SceneGraph::GenericNodeFactory<SceneGraph::SphereNode>());
  root = new SceneGraph::GroupNode;
}

SceneGraphViewer::~SceneGraphViewer()
{
}

void SceneGraphViewer::load(const std::string& name)
{
  SceneGraph::VRMLFile vrmlFile(name.c_str(), Vrui::openFile(name.c_str()), nodeCreator, Vrui::getClusterMultiplexer());
  vrmlFile.parse(root);
}

void SceneGraphViewer::disable()
{
  active = false;
}

void SceneGraphViewer::enable()
{
  active = true;
}

void SceneGraphViewer::display(GLContextData& contextData) const
{
  if(active){
    glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);
    Vrui::renderSceneGraph(root.getPointer(), true, contextData);
    glPopAttrib();
  }
}

}
