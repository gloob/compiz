/*
 * Copyright (c) 2006 Darryll Truchan <moppsy@comcast.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * TODO:
 * Region -> CompRegion
 * XRectangle -> CompRect
 */


#include <core/core.h>
#include <core/privatehandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include "put_options.h"

enum PutType
{
    PutUnknown = 0,
    PutBottomLeft = 1,
    PutBottom = 2,
    PutBottomRight = 3,
    PutLeft = 4,
    PutCenter = 5,
    PutRight = 6,
    PutTopLeft = 7,
    PutTop = 8,
    PutTopRight = 9,
    PutRestore = 10,
    PutViewport = 11,
    PutViewportLeft = 12,
    PutViewportRight = 13,
    PutAbsolute = 14,
    PutPointer = 15,
    PutViewportUp = 16,
    PutViewportDown = 17,
    PutRelative = 18,
    PutNextOutput = 19,
    PutEmptyBottomLeft = 20,
    PutEmptyBottom = 21,
    PutEmptyBottomRight = 22,
    PutEmptyLeft = 23,
    PutEmptyCenter = 24,
    PutEmptyRight = 25,
    PutEmptyTopLeft = 26,
    PutEmptyTop = 27,
    PutEmptyTopRight,
};

class PutScreen :
    public PrivateHandler <PutScreen, CompScreen>,
    public PutOptions,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface
{

    public:

	PutScreen (CompScreen *s);
	~PutScreen ();


	void
	preparePaint (int);

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix &, const CompRegion &,
		       CompOutput *, unsigned int);

	void
	donePaint ();

	void
	handleEvent (XEvent *);

	bool
	initiateCommon (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options,
			PutType            type);

	bool
	initiate (CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector &option);
    private:

	Region
	emptyRegion (CompWindow *window,
		     Region     region);
	bool	    
	boxCompare (BOX a,
		    BOX b);

	BOX
	extendBox (CompWindow *w,		     
		   BOX        tmp,
		   Region     r,	   
		   bool       xFirst,
		   bool       left,
		   bool       right,
		   bool       up,
		   bool       down);
	BOX
	findRect (CompWindow *w,
		  Region     r,
		  bool       left,
		  bool       right,
		  bool       up,
		  bool       down);

	unsigned int
	computeResize(CompWindow     *w,
		      XWindowChanges *xwc,
		      bool           left,
		      bool           right,
		      bool           up,
		      bool           down);

	int
	adjustVelocity (CompWindow *w);

	void
	finishWindowMovement (CompWindow *w);

	bool
	getDistance (CompWindow         *w,
		     PutType            type,
		     CompOption::Vector &option,
		     int                *distX,
		     int                *distY);

	unsigned int
	getOutputForWindow (CompWindow *w);

	PutType
	typeFromString (const CompString &);

	/* Data */

	CompScreen      *screen;
	CompositeScreen *cScreen;
        GLScreen        *gScreen;

	Atom compizPutWindowAtom;
        Window       lastWindow;
	PutType      lastType;
	int          moreAdjust;
	CompScreen::GrabHandle   grabIndex;

	friend class PutWindow;
};
class PutWindow :
    public PrivateHandler <PutWindow, CompWindow>,
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{

    public:

	PutWindow (CompWindow *window);
	~PutWindow ();

	CompWindow      *window;
	CompositeWindow *cWindow;
        GLWindow        *gWindow;

	bool
	glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		 const CompRegion &, unsigned int);

    private:
	GLfloat xVelocity, yVelocity;	/* animation velocity       */
	GLfloat tx, ty;			/* animation translation    */

	int lastX, lastY;			/* starting position        */
	int targetX, targetY;               /* target of the animation  */

	Bool adjust;			/* animation flag           */
	friend class PutScreen;

};

#define PUT_SCREEN(s) \
PutScreen *ps = PutScreen::get (s);

#define PUT_WINDOW(w) \
PutWindow *pw = PutWindow::get (w);

class PutPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<PutScreen, PutWindow>
{
    public:

	bool init ();

	/* This implements ::getOption and ::setOption in your screen class for you. Yay */
			
	PLUGIN_OPTION_HELPER (PutScreen);
};


COMPIZ_PLUGIN_20081216 (put, PutPluginVTable);

