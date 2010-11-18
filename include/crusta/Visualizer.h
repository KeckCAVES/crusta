/***********************************************************************
 Visualizer - "Empty" VR application that displays a simple OpenGL scene in
 a virtual reality environment, showing off some Vrui toolkit
 functionality.
 Copyright (c) 2003-2006 Oliver Kreylos

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 ***********************************************************************/

#ifndef _Visualizer_H_
#define _Visualizer_H_

#include <vector>

#include <GL/VruiGlew.h> //must be included before gl.h

#include <GL/GLContextData.h>
#include <GLMotif/Button.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <Threads/Thread.h>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>
#include <Vrui/Application.h>
#include <Vrui/Vrui.h>

#include <crusta/basics.h>


BEGIN_CRUSTA


class Scope;
class SphereCoverage;


class Visualizer : public Vrui::Application
{
public:
    class Primitive
    {
    public:
        Primitive();
        Primitive(const Primitive& other);
        void clear();
        void setVertices(const Point3s& verts);

        GLenum  mode;
        Color   color;
        Point3  centroid;
        Point3s vertices;
    };
    typedef std::vector<Primitive> Primitives;

    static Color defaultColor;

    Visualizer(int& argc,char**& argv,char**& appDefaults);
    ~Visualizer();

    static void init();

    static void start(int& argc,char**& argv,char**& appDefaults);
    static void stop();

    static void clear(int temp=-1);
    static void clearAll();

    static void addPrimitive(GLenum drawMode, const Point3s& verts,
                             int temp=-1, const Color& color=defaultColor);

    static void peek();
    static void show(const char* message=NULL);

    virtual void display(GLContextData& contextData) const;
    virtual void frame();

    virtual void resetNavigationCallback(Misc::CallbackData* cbData);
    void continueCallback(Misc::CallbackData* cbData);

protected:
    static Visualizer*     vis;
    Threads::Thread        appThread;
    mutable Threads::Mutex showMut;
    Threads::Cond          showCond;

    Primitives primitives[2];
    Primitive  tempPrimitives[10][2];
    int shownSlot;

    GLMotif::PopupMenu*   popMenu;
    GLMotif::PopupWindow* continueWindow;
    GLMotif::Button*      continueButton;

    void produceMainMenu();
    void produceContinueWindow();

    static Primitive& getNewPrimitive(int temp);

    void* runWrapper();
};


END_CRUSTA


#endif //_Visualizer_H_
