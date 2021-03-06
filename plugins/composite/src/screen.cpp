/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <boost/make_shared.hpp>

#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

#include "privates.h"

#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include <core/timer.h>

template class WrapableInterface<CompositeScreen, CompositeScreenInterface>;

namespace bt = compiz::composite::buffertracking;

static const int FALLBACK_REFRESH_RATE = 60;   /* if all else fails */

CompWindow *lastDamagedWindow = 0;

void
PrivateCompositeScreen::handleEvent (XEvent *event)
{
    CompWindow      *w;

    switch (event->type)
    {
	case CreateNotify:
	    if (screen->root () == event->xcreatewindow.parent)
	    {
		/* The first time some client asks for the composite
		 * overlay window, the X server creates it, which causes
		 * an errorneous CreateNotify event.  We catch it and
		 * ignore it. */
		if (overlay == event->xcreatewindow.window)
		    return;
	    }
	    break;

	case PropertyNotify:
	    if (event->xproperty.atom == Atoms::winOpacity)
	    {
		w = screen->findWindow (event->xproperty.window);
		if (w)
		    CompositeWindow::get (w)->updateOpacity ();
	    }
	    else if (event->xproperty.atom == Atoms::winBrightness)
	    {
		w = screen->findWindow (event->xproperty.window);
		if (w)
		    CompositeWindow::get (w)->updateBrightness ();
	    }
	    else if (event->xproperty.atom == Atoms::winSaturation)
	    {
		w = screen->findWindow (event->xproperty.window);
		if (w)
		    CompositeWindow::get (w)->updateSaturation ();
	    }
	    break;

	default:
	    if (shapeExtension &&
		event->type == shapeEvent + ShapeNotify)
	    {
		w = screen->findWindow (((XShapeEvent *) event)->window);
		if (w &&
		    w->mapNum ())
		    CompositeWindow::get (w)->addDamage ();
	    }
	    else if (event->type == damageEvent + XDamageNotify)
	    {
		XDamageNotifyEvent *de = (XDamageNotifyEvent*)event;
		damages[de->damage] = de->area;
	    }
	    break;
    }

    screen->handleEvent (event);

    switch (event->type)
    {
	case Expose:
	    handleExposeEvent (&event->xexpose);
	break;

	case ClientMessage:
	    if (event->xclient.message_type == Atoms::winOpacity)
	    {
		w = screen->findWindow (event->xclient.window);
		if (w && (w->type () & CompWindowTypeDesktopMask) == 0)
		{
		    unsigned short opacity = event->xclient.data.l[0] >> 16;

		    screen->setWindowProp32 (w->id (),
			Atoms::winOpacity, opacity);
		}
	    }
	    else if (event->xclient.message_type == Atoms::winBrightness)
	    {
		w = screen->findWindow (event->xclient.window);
		if (w)
		{
		    unsigned short brightness = event->xclient.data.l[0] >> 16;

		    screen->setWindowProp32 (w->id (),
			Atoms::winBrightness, brightness);
		}
	    }
	    else if (event->xclient.message_type == Atoms::winSaturation)
	    {
		w = screen->findWindow (event->xclient.window);
		if (w)
		{
		    unsigned short saturation = event->xclient.data.l[0] >> 16;

		    screen->setWindowProp32 (w->id (),
			Atoms::winSaturation, saturation);
		}
	    }
	    break;

	default:
	    if (event->type == damageEvent + XDamageNotify)
	    {
		XDamageNotifyEvent *de = (XDamageNotifyEvent *) event;

		if (lastDamagedWindow && de->drawable == lastDamagedWindow->id ())
		    w = lastDamagedWindow;
		else
		{
		    w = screen->findWindow (de->drawable);
		    if (w)
			lastDamagedWindow = w;
		}

		if (w)
		    CompositeWindow::get (w)->processDamage (de);
	    }
	    else if (shapeExtension &&
		     event->type == shapeEvent + ShapeNotify)
	    {
		w = screen->findWindow (((XShapeEvent *) event)->window);

		if (w && w->mapNum ())
		    CompositeWindow::get (w)->addDamage ();
	    }
	    else if (randrExtension &&
		     event->type == randrEvent + RRScreenChangeNotify)
	    {
		XRRScreenChangeNotifyEvent *rre;

		rre = (XRRScreenChangeNotifyEvent *) event;

		if (screen->root () == rre->root)
		    detectRefreshRate ();
	    }
	    break;
    }
}

int
CompositeScreen::damageEvent ()
{
    return priv->damageEvent;
}

template class PluginClassHandler<CompositeScreen, CompScreen, COMPIZ_COMPOSITE_ABI>;

CompositeScreen::CompositeScreen (CompScreen *s) :
    PluginClassHandler<CompositeScreen, CompScreen, COMPIZ_COMPOSITE_ABI> (s),
    priv (new PrivateCompositeScreen (this))
{
    if (!XQueryExtension (s->dpy (), COMPOSITE_NAME,
			  &priv->compositeOpcode,
			  &priv->compositeEvent,
			  &priv->compositeError))
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "No composite extension");
	setFailed ();
	return;
    }

    int	compositeMajor, compositeMinor;

    XCompositeQueryVersion (s->dpy (), &compositeMajor, &compositeMinor);
    if (compositeMajor == 0 && compositeMinor < 2)
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "Old composite extension");
	setFailed ();
	return;
    }

    if (!XDamageQueryExtension (s->dpy (), &priv->damageEvent,
				&priv->damageError))
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "No damage extension");
	setFailed ();
	return;
    }

    if (!XFixesQueryExtension (s->dpy (), &priv->fixesEvent, &priv->fixesError))
    {
	compLogMessage ("core", CompLogLevelFatal,
		        "No fixes extension");
	setFailed ();
	return;
    }

    priv->shapeExtension = XShapeQueryExtension (s->dpy (), &priv->shapeEvent,
						 &priv->shapeError);
    priv->randrExtension = XRRQueryExtension (s->dpy (), &priv->randrEvent,
					      &priv->randrError);

    priv->makeOutputWindow ();

    priv->detectRefreshRate ();

    priv->slowAnimations = false;

    if (!priv->init ())
	setFailed ();
}

CompositeScreen::~CompositeScreen ()
{
    priv->paintTimer.stop ();
    XCompositeReleaseOverlayWindow (screen->dpy (),
				    screen->root ());
    delete priv;
}

namespace
{
bool alwaysMarkDirty ()
{
    return true;
}
}


PrivateCompositeScreen::PrivateCompositeScreen (CompositeScreen *cs) :
    cScreen (cs),
    compositeEvent (0),
    compositeError (0),
    compositeOpcode (0),
    damageEvent (0),
    damageError (0),
    fixesEvent (0),
    fixesError (0),
    fixesVersion (0),
    shapeExtension (false),
    shapeEvent (0),
    shapeError (0),
    randrExtension (false),
    randrEvent (0),
    randrError (0),
    damageMask (COMPOSITE_SCREEN_DAMAGE_ALL_MASK),
    currentlyTrackingDamage (DamageForCurrentFrame),
    overlay (None),
    output (None),
    exposeRects (),
    windowPaintOffset (0, 0),
    overlayWindowCount (0),
    outputShapeChanged (false),
    redrawTime (1000 / FALLBACK_REFRESH_RATE),
    optimalRedrawTime (1000 / FALLBACK_REFRESH_RATE),
    scheduled (false),
    painting (false),
    reschedule (false),
    damageRequiresRepaintReschedule (true),
    slowAnimations (false),
    pHnd (NULL),
    FPSLimiterMode (CompositeFPSLimiterModeDefault),
    withDestroyedWindows (),
    cmSnAtom (0),
    newCmSnOwner (None),
    roster (*screen,
	    ageingBuffers,
	    boost::bind (alwaysMarkDirty))
{
    gettimeofday (&lastRedraw, 0);
    // wrap outputChangeNotify
    ScreenInterface::setHandler (screen);

    optionSetSlowAnimationsKeyInitiate (CompositeScreen::toggleSlowAnimations);
}

PrivateCompositeScreen::~PrivateCompositeScreen ()
{
    Display *dpy = screen->dpy ();

    if (cmSnAtom)
	XSetSelectionOwner (dpy, cmSnAtom, None, CurrentTime);

    if (newCmSnOwner != None)
	XDestroyWindow (dpy, newCmSnOwner);
}

bool
PrivateCompositeScreen::init ()
{
    Display              *dpy = screen->dpy ();
    Time                 cmSnTimestamp = 0;
    XEvent               event;
    XSetWindowAttributes attr;
    char                 buf[128];

    snprintf (buf, 128, "_NET_WM_CM_S%d", screen->screenNum ());
    cmSnAtom = XInternAtom (dpy, buf, 0);

    Window currentCmSnOwner = XGetSelectionOwner (dpy, cmSnAtom);

    if (currentCmSnOwner != None &&
	!replaceCurrentWm)
    {
	compLogMessage (
		    "composite", CompLogLevelError,
		    "Screen %d on display \"%s\" already has a compositing "
		    "manager (%x); try using the --replace option to replace "
		    "the current compositing manager.",
		    screen->screenNum (), DisplayString (dpy), currentCmSnOwner);

	return false;
    }

    attr.override_redirect = true;
    attr.event_mask        = PropertyChangeMask;

    newCmSnOwner = XCreateWindow (dpy, screen->root (),
				  -100, -100, 1, 1, 0,
				  CopyFromParent, CopyFromParent,
				  CopyFromParent,
				  CWOverrideRedirect | CWEventMask,
				  &attr);

    XChangeProperty (dpy, newCmSnOwner, Atoms::wmName, Atoms::utf8String, 8,
		     PropModeReplace, (unsigned char *) PACKAGE,
		     strlen (PACKAGE));

    XWindowEvent (dpy, newCmSnOwner, PropertyChangeMask, &event);

    cmSnTimestamp = event.xproperty.time;

    XSetSelectionOwner (dpy, cmSnAtom, newCmSnOwner, cmSnTimestamp);

    if (XGetSelectionOwner (dpy, cmSnAtom) != newCmSnOwner)
    {
	compLogMessage ("core", CompLogLevelError,
			"Could not acquire compositing manager "
			"selection on screen %d display \"%s\"",
			screen->screenNum (), DisplayString (dpy));

	return false;
    }

    /* Send client message indicating that we are now the compositing manager */
    event.xclient.type         = ClientMessage;
    event.xclient.window       = screen->root ();
    event.xclient.message_type = Atoms::manager;
    event.xclient.format       = 32;
    event.xclient.data.l[0]    = cmSnTimestamp;
    event.xclient.data.l[1]    = cmSnAtom;
    event.xclient.data.l[2]    = 0;
    event.xclient.data.l[3]    = 0;
    event.xclient.data.l[4]    = 0;

    XSendEvent (dpy, screen->root (), FALSE, StructureNotifyMask, &event);

    return true;
}

bool
CompositeScreen::registerPaintHandler (compiz::composite::PaintHandler *pHnd)
{
    Display *dpy;

    WRAPABLE_HND_FUNCTN_RETURN (bool, registerPaintHandler, pHnd);

    dpy =  screen->dpy ();

    if (priv->pHnd)
	return false;

    CompScreen::checkForError (dpy);

    XCompositeRedirectSubwindows (dpy, screen->root (),
				  CompositeRedirectManual);

    priv->overlayWindowCount = 0;

    if (CompScreen::checkForError (dpy))
    {
	compLogMessage ("composite", CompLogLevelError,
			"Another composite manager is already "
			"running on screen: %d", screen->screenNum ());

	return false;
    }

    foreach (CompWindow *w, screen->windows ())
    {
	CompositeWindow *cw = CompositeWindow::get (w);
	cw->priv->overlayWindow = false;
	cw->priv->redirected = true;
    }

    priv->pHnd = pHnd;

    priv->detectRefreshRate ();

    showOutputWindow ();

    return true;
}

void
CompositeScreen::unregisterPaintHandler ()
{
    Display *dpy;

    WRAPABLE_HND_FUNCTN (unregisterPaintHandler)

    dpy = screen->dpy ();

    foreach (CompWindow *w, screen->windows ())
    {
	CompositeWindow *cw = CompositeWindow::get (w);
	cw->priv->overlayWindow = false;
	cw->priv->redirected = false;
	cw->release ();
    }

    priv->overlayWindowCount = 0;

    XCompositeUnredirectSubwindows (dpy, screen->root (),
				    CompositeRedirectManual);

    priv->pHnd = NULL;
    priv->paintTimer.stop ();

    priv->detectRefreshRate ();

    hideOutputWindow ();
}

bool
CompositeScreen::compositingActive ()
{
    if (priv->pHnd)
	return priv->pHnd->compositingActive ();

    return false;
}

const CompRegion *
PrivateCompositeScreen::damageTrackedBuffer (const CompRegion &region)
{
    const CompRegion *currentDamage = NULL;

    switch (currentlyTrackingDamage)
    {
	case DamageForCurrentFrame:
	    currentDamage = &(roster.currentFrameDamage ());
	    ageingBuffers.markAreaDirty (region);
	    break;
	case DamageForLastFrame:
	    currentDamage = &(lastFrameDamage);
	    lastFrameDamage += region;
	    break;
	case DamageFinalPaintRegion:
	    currentDamage = &(tmpRegion);
	    tmpRegion += region;
	    break;
	default:
	    compLogMessage ("composite", CompLogLevelFatal, "unreachable section");
	    assert (false);
	    abort ();
    }

    assert (currentDamage);
    return currentDamage;
}

void
CompositeScreen::damageScreen ()
{
    /* Don't tell plugins about damage events when the damage buffer is already full */
    bool alreadyDamaged = priv->damageMask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK;
    alreadyDamaged |= ((currentDamage () & screen->region ()) == screen->region ());

    priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_ALL_MASK;
    priv->damageMask &= ~COMPOSITE_SCREEN_DAMAGE_REGION_MASK;

    if (priv->damageRequiresRepaintReschedule)
	priv->scheduleRepaint ();

    /*
     * Call through damageRegion since plugins listening for incoming damage
     * may need to know that the whole screen was redrawn
     */
    if (!alreadyDamaged)
    {
	damageRegion (CompRegion (0, 0, screen->width (), screen->height ()));

	/* Set the damage region as the fullscreen region, because if
	 * windows are unredirected we need to correctly subtract from
	 * it later
	 */
	priv->damageTrackedBuffer (screen->region ());
    }
}

void
CompositeScreen::damageRegion (const CompRegion &region)
{
    WRAPABLE_HND_FUNCTN (damageRegion, region);

    if (priv->damageMask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	return;

    /* Don't cause repaints to be scheduled for empty damage
     * regions */
    if (region.isEmpty ())
        return;

    const CompRegion *currentDamage = priv->damageTrackedBuffer (region);
    priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_REGION_MASK;

    /* If the number of damage rectangles grows two much between repaints,
     * we have a lot of overhead just for doing the damage tracking -
     * in order to make sure we're not having too much overhead, damage
     * the whole screen if we have a lot of damage rects
     */

    if (currentDamage->numRects () > 100)
	damageScreen ();

    if (priv->damageRequiresRepaintReschedule)
	priv->scheduleRepaint ();
}

void
CompositeScreen::damageCutoff ()
{
    WRAPABLE_HND_FUNCTN (damageCutoff);
}

void
CompositeScreen::damagePending ()
{
    priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_PENDING_MASK;

    if (priv->damageRequiresRepaintReschedule)
	priv->scheduleRepaint ();
}

void
CompositeScreen::applyDamageForFrameAge (unsigned int age)
{
    /* Track into "last frame damage" */
    priv->currentlyTrackingDamage = DamageForLastFrame;
    damageRegion (priv->roster.damageForFrameAge (age));
    priv->currentlyTrackingDamage = DamageForCurrentFrame;
}

unsigned int
CompositeScreen::getFrameAge ()
{
    if (priv->pHnd)
	return priv->pHnd->getFrameAge ();

    return 1;
}

void
CompositeScreen::recordDamageOnCurrentFrame (const CompRegion &r)
{
    priv->ageingBuffers.markAreaDirtyOnLastFrame (r);
}

typedef CompositeScreen::AreaShouldBeMarkedDirty ShouldMarkDirty;

namespace
{
    bool alwaysDirty ()
    {
	return true;
    }
}

CompositeScreen::DamageQuery::Ptr
CompositeScreen::getDamageQuery (ShouldMarkDirty callback)
{
    /* No initial damage */
    bt::AgeingDamageBufferObserver &observer (priv->ageingBuffers);
    return boost::make_shared <bt::FrameRoster> (*screen,
						 boost::ref (observer),
						 !callback.empty () ?
						     callback :
						     boost::bind (alwaysDirty));
}

unsigned int
CompositeScreen::damageMask ()
{
    return priv->damageMask;
}

void
CompositeScreen::showOutputWindow ()
{
    if (priv->pHnd)
    {
	Display       *dpy = screen->dpy ();
	XserverRegion region;

	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeBounding,
				    0, 0, 0);
	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeInput,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);

	damageScreen ();

	priv->outputShapeChanged = true;
    }
}

void
CompositeScreen::hideOutputWindow ()
{
    Display       *dpy = screen->dpy ();
    XserverRegion region = XFixesCreateRegion (dpy, NULL, 0);

    XFixesSetWindowShapeRegion (dpy,
				priv->output,
				ShapeBounding,
				0, 0, region);

    XFixesDestroyRegion (dpy, region);
}

void
CompositeScreen::updateOutputWindow ()
{
    if (priv->pHnd)
    {
	Display       *dpy = screen->dpy ();
	XserverRegion region;
	CompRegion    tmpRegion (screen->region ());

	for (CompWindowList::reverse_iterator rit =
	     screen->windows ().rbegin ();
	     rit != screen->windows ().rend (); ++rit)
	    if (CompositeWindow::get (*rit)->overlayWindow ())
		tmpRegion -= (*rit)->region ();

	XShapeCombineRegion (dpy, priv->output, ShapeBounding,
			     0, 0, tmpRegion.handle (), ShapeSet);


	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeInput,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);

	priv->outputShapeChanged = true;
    }
}

bool
CompositeScreen::outputWindowChanged () const
{
    return priv->outputShapeChanged;
}

void
PrivateCompositeScreen::makeOutputWindow ()
{
    overlay = XCompositeGetOverlayWindow (screen->dpy (), screen->root ());
    output  = overlay;

    XSelectInput (screen->dpy (), output, ExposureMask);

    cScreen->hideOutputWindow ();
}

Window
CompositeScreen::output ()
{
    return priv->output;
}

Window
CompositeScreen::overlay ()
{
    return priv->overlay;
}

int &
CompositeScreen::overlayWindowCount ()
{
    return priv->overlayWindowCount;
}

void
CompositeScreen::setWindowPaintOffset (int x, int y)
{
    priv->windowPaintOffset = CompPoint (x, y);
}

CompPoint
CompositeScreen::windowPaintOffset ()
{
    return priv->windowPaintOffset;
}

void
PrivateCompositeScreen::detectRefreshRate ()
{
    const bool forceRefreshRate = (pHnd ? pHnd->requiredForcedRefreshRate () : false);
    const bool detect = optionGetDetectRefreshRate () && !forceRefreshRate;

    if (detect)
    {
	CompString        name;
	CompOption::Value value;

	value.set ((int) 0);

	if (randrExtension)
	{
	    XRRScreenConfiguration *config;

	    config  = XRRGetScreenInfo (screen->dpy (),
					screen->root ());
	    value.set ((int) XRRConfigCurrentRate (config));

	    XRRFreeScreenConfigInfo (config);
	}

	if (value.i () == 0)
	    value.set ((int) FALLBACK_REFRESH_RATE);

	mOptions[CompositeOptions::DetectRefreshRate].value ().set (false);
	screen->setOptionForPlugin ("composite", "refresh_rate", value);
	mOptions[CompositeOptions::DetectRefreshRate].value ().set (true);
	optimalRedrawTime = redrawTime = 1000 / value.i ();
    }
    else
    {
	if (forceRefreshRate && (optionGetRefreshRate () < FALLBACK_REFRESH_RATE))
	{
	    CompOption::Value value;

	    value.set ((int) FALLBACK_REFRESH_RATE);

	    screen->setOptionForPlugin ("composite", "refresh_rate", value);
	}

	redrawTime = 1000 / optionGetRefreshRate ();
	optimalRedrawTime = redrawTime;
    }
}

CompositeFPSLimiterMode
CompositeScreen::FPSLimiterMode ()
{
    return priv->FPSLimiterMode;
}

void
CompositeScreen::setFPSLimiterMode (CompositeFPSLimiterMode newMode)
{
    priv->FPSLimiterMode = newMode;
}

void
PrivateCompositeScreen::scheduleRepaint ()
{
    if (painting)
    {
	reschedule = true;
	return;
    }

    if (scheduled)
	return;

    scheduled = true;

    int delay;

    if (FPSLimiterMode == CompositeFPSLimiterModeVSyncLike ||
	(pHnd && pHnd->hasVSync ()))
	delay = 1;
    else
    {
	struct timeval now;
	gettimeofday (&now, 0);
	int elapsed = compiz::core::timer::timeval_diff (&now, &lastRedraw);

	if (elapsed < 0)
	    elapsed = 0;

 	delay = elapsed < optimalRedrawTime ? optimalRedrawTime - elapsed : 1;
    }

    paintTimer.start
	(boost::bind (&CompositeScreen::handlePaintTimeout, cScreen),
	delay);
}

int
CompositeScreen::redrawTime ()
{
    return priv->redrawTime;
}

int
CompositeScreen::optimalRedrawTime ()
{
    return priv->optimalRedrawTime;
}

bool
CompositeScreen::handlePaintTimeout ()
{
    struct      timeval tv;

    priv->painting = true;
    priv->reschedule = false;
    gettimeofday (&tv, 0);

    if (priv->damageMask)
    {
	/* Damage that accumulates here does not require a repaint reschedule
	 * as it will end up on this frame */
	priv->damageRequiresRepaintReschedule = false;

	if (priv->pHnd)
	    priv->pHnd->prepareDrawing ();

	int timeDiff = compiz::core::timer::timeval_diff (&tv, &priv->lastRedraw);

	/* handle clock rollback */
	if (timeDiff < 0)
	    timeDiff = 0;
	/*
	 * Now that we use a "tickless" timing algorithm, timeDiff could be
	 * very large if the screen is truely idle.
	 * However plugins expect the old behaviour where timeDiff is rarely
	 * larger than the frame rate (optimalRedrawTime).
	 * So enforce this to keep animations timed correctly and smooth...
	 */
	if (timeDiff > 100)
	    timeDiff = priv->optimalRedrawTime;

	priv->redrawTime = timeDiff;
	preparePaint (priv->slowAnimations ? 1 : timeDiff);

	/* substract top most overlay window region */
	if (priv->overlayWindowCount)
	{
	    for (CompWindowList::reverse_iterator rit =
		 screen->windows ().rbegin ();
		 rit != screen->windows ().rend (); ++rit)
	    {
		CompWindow *w = (*rit);

		if (w->destroyed () || w->invisible ())
		    continue;

		if (!CompositeWindow::get (w)->redirected ())
		    priv->ageingBuffers.subtractObscuredArea (w->region ());

		break;
	    }

	    if (priv->damageMask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	    {
		priv->damageMask &= ~COMPOSITE_SCREEN_DAMAGE_ALL_MASK;
		priv->damageMask |= COMPOSITE_SCREEN_DAMAGE_REGION_MASK;
	    }
	}

	/* All further damage is for the next frame now, as
	 * priv->tmpRegion will be assigned. Notify plugins that do
	 * damage tracking of this */
	damageCutoff ();

	priv->tmpRegion = (priv->roster.currentFrameDamage () + priv->lastFrameDamage) & screen->region ();
	priv->currentlyTrackingDamage = DamageFinalPaintRegion;

	if (priv->damageMask & COMPOSITE_SCREEN_DAMAGE_REGION_MASK &&
	    priv->tmpRegion == screen->region ())
		damageScreen ();

	Display *dpy = screen->dpy ();
	std::map<Damage, XRectangle>::iterator d = priv->damages.begin ();

	for (; d != priv->damages.end (); ++d)
	{
	    XserverRegion sub = XFixesCreateRegion (dpy, &d->second, 1);
	    if (sub != None)
	    {
		XDamageSubtract (dpy, d->first, sub, None);
		XFixesDestroyRegion (dpy, sub);
	    }
	}

	XSync (dpy, False);
	priv->damages.clear ();

	/* Any more damage requires a repaint reschedule */
	priv->damageRequiresRepaintReschedule = true;
	priv->lastFrameDamage = CompRegion ();

	int mask = priv->damageMask;
	priv->damageMask = 0;

	CompOutput::ptrList outputs (0);

	if (priv->optionGetForceIndependentOutputPainting () ||
	    !screen->hasOverlappingOutputs ())
	{
	    foreach (CompOutput &o, screen->outputDevs ())
	    {
		outputs.push_back (&o);
	    }
	}
	else
	    outputs.push_back (&screen->fullscreenOutput ());

	priv->currentlyTrackingDamage = DamageForCurrentFrame;

	/* All new damage goes on the next frame */
	priv->ageingBuffers.incrementAges ();

	paint (outputs, mask);

	donePaint ();

	priv->outputShapeChanged = false;

	foreach (CompWindow *w, screen->windows ())
	{
	    if (w->destroyed ())
	    {
		CompositeWindow::get (w)->addDamage ();
		break;
	    }
	}
    }

    priv->lastRedraw = tv;
    priv->painting = false;
    priv->scheduled = false;
    if (priv->reschedule)
	priv->scheduleRepaint ();

    return false;
}

void
CompositeScreen::preparePaint (int msSinceLastPaint)
    WRAPABLE_HND_FUNCTN (preparePaint, msSinceLastPaint)

void
CompositeScreen::donePaint ()
    WRAPABLE_HND_FUNCTN (donePaint)

void
CompositeScreen::paint (CompOutput::ptrList &outputs,
		        unsigned int        mask)
{
    WRAPABLE_HND_FUNCTN (paint, outputs, mask)

    if (priv->pHnd)
	priv->pHnd->paintOutputs (outputs, mask, priv->tmpRegion);
}

const CompWindowList &
CompositeScreen::getWindowPaintList ()
{
    WRAPABLE_HND_FUNCTN_RETURN (const CompWindowList &, getWindowPaintList)

    /* Include destroyed windows */
    if (screen->destroyedWindows ().empty ())
	return screen->windows ();
    else
    {
	CompWindowList destroyedWindows = screen->destroyedWindows ();

	priv->withDestroyedWindows.resize (0);

	foreach (CompWindow *w, screen->windows ())
	{
	    foreach (CompWindow *dw, screen->destroyedWindows ())
	    {
		if (dw->next == w)
		{
		    priv->withDestroyedWindows.push_back (dw);
		    destroyedWindows.remove (dw);
		    break;
		}
	    }

	    priv->withDestroyedWindows.push_back (w);
	}

	/* We need to put all the destroyed windows which didn't get
	 * inserted in the paint list at the top of the stack since
	 * w->next was probably either invalid or NULL */

	foreach (CompWindow *dw, destroyedWindows)
	    priv->withDestroyedWindows.push_back (dw);

	return priv->withDestroyedWindows;
    }
}

void
PrivateCompositeScreen::handleExposeEvent (XExposeEvent *event)
{
    if (output == event->window)
	return;

    exposeRects.push_back (CompRect (event->x,
				     event->y,
				     event->width,
				     event->height));

    if (event->count == 0)
    {
	CompRect rect;

	foreach (CompRect rect, exposeRects)
	{
	    cScreen->damageRegion (CompRegion (rect));
	}

	exposeRects.clear ();
    }
}

void
PrivateCompositeScreen::outputChangeNotify ()
{
    screen->outputChangeNotify ();
    XMoveResizeWindow (screen->dpy (), overlay, 0, 0,
		       screen->width (), screen->height ());
    cScreen->damageScreen ();
}

bool
CompositeScreen::toggleSlowAnimations (CompAction         *action,
				       CompAction::State  state,
				       CompOption::Vector &options)
{
    CompositeScreen *cs = CompositeScreen::get (screen);

    if (cs)
	cs->priv->slowAnimations = !cs->priv->slowAnimations;

    return true;
}

void
CompositeScreenInterface::preparePaint (int msSinceLastPaint)
    WRAPABLE_DEF (preparePaint, msSinceLastPaint)

void
CompositeScreenInterface::donePaint ()
    WRAPABLE_DEF (donePaint)

void
CompositeScreenInterface::paint (CompOutput::ptrList &outputs,
				 unsigned int        mask)
    WRAPABLE_DEF (paint, outputs, mask)

const CompWindowList &
CompositeScreenInterface::getWindowPaintList ()
    WRAPABLE_DEF (getWindowPaintList)

bool
CompositeScreenInterface::registerPaintHandler (compiz::composite::PaintHandler *pHnd)
    WRAPABLE_DEF (registerPaintHandler, pHnd);

void
CompositeScreenInterface::unregisterPaintHandler ()
    WRAPABLE_DEF (unregisterPaintHandler);

void
CompositeScreenInterface::damageRegion (const CompRegion &r)
    WRAPABLE_DEF (damageRegion, r);

void
CompositeScreenInterface::damageCutoff ()
    WRAPABLE_DEF (damageCutoff);

const CompRegion &
CompositeScreen::currentDamage () const
{
    return priv->roster.currentFrameDamage ();
}
