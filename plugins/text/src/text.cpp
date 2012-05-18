/*
 * Compiz text plugin
 * Description: Adds text to pixmap support to Compiz.
 *
 * text.c
 *
 * Copyright: (C) 2006-2007 Patrick Niklaus, Danny Baumann, Dennis Kasprzyk
 * Authors: Patrick Niklaus <marex@opencompsiting.org>
 *	    Danny Baumann   <maniac@opencompositing.org>
 *	    Dennis Kasprzyk <onestone@opencompositing.org>
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
 */

#include "private.h"

#define PI 3.14159265359f

COMPIZ_PLUGIN_20090315 (text, TextPluginVTable);

CompString
PrivateTextScreen::getUtf8Property (Window id,
				    Atom   atom)
{
    Atom          type;
    int           result, format;
    unsigned long nItems, bytesAfter;
    char          *val;
    CompString    retval;

    result = XGetWindowProperty (screen->dpy (), id, atom, 0L, 65536, False,
				 utf8StringAtom, &type, &format, &nItems,
				 &bytesAfter, (unsigned char **) &val);

    if (result != Success)
	return retval;

    if (type == utf8StringAtom && format == 8 && val && nItems > 0)
    {
	char valueString[nItems + 1];

	strncpy (valueString, val, nItems);
	valueString[nItems] = 0;

	retval = valueString;
    }

    if (val)
	XFree (val);

    return retval;
}

CompString
PrivateTextScreen::getTextProperty (Window id,
				    Atom   atom)
{
    XTextProperty text;
    CompString    retval;

    text.nitems = 0;
    if (XGetTextProperty (screen->dpy (), id, &text, atom))
    {
        if (text.value)
	{
	    char valueString[text.nitems + 1];

	    strncpy (valueString, (char *) text.value, text.nitems);
	    valueString[text.nitems] = 0;

	    retval = valueString;

	    XFree (text.value);
	}
    }

    return retval;
}

CompString
PrivateTextScreen::getWindowName (Window id)
{
    CompString name;

    name = getUtf8Property (id, visibleNameAtom);

    if (name.empty ())
	name = getUtf8Property (id, wmNameAtom);

    if (name.empty ())
	name = getTextProperty (id, XA_WM_NAME);

    return name;
}

/* Actual text rendering functions */

/*
 * Draw a rounded rectangle path
 */
void
TextSurface::drawBackground (int     x,
			     int     y,
			     int     width,
			     int     height,
			     int     radius)
{
    int x0, y0, x1, y1;

    x0 = x;
    y0 = y;
    x1 = x + width;
    y1 = y + height;

    cairo_new_path (cr);
    cairo_arc (cr, x0 + radius, y1 - radius, radius, PI / 2, PI);
    cairo_line_to (cr, x0, y0 + radius);
    cairo_arc (cr, x0 + radius, y0 + radius, radius, PI, 3 * PI / 2);
    cairo_line_to (cr, x1 - radius, y0);
    cairo_arc (cr, x1 - radius, y0 + radius, radius, 3 * PI / 2, 2 * PI);
    cairo_line_to (cr, x1, y1 - radius);
    cairo_arc (cr, x1 - radius, y1 - radius, radius, 0, PI / 2);
    cairo_close_path (cr);
}

bool
TextSurface::initCairo (int width,
			int height)
{
    Display *dpy = screen->dpy ();

    mPixmap = None;
    if (width > 0 && height > 0)
	mPixmap = XCreatePixmap (dpy, screen->root (), width, height, 32);

    mWidth  = width;
    mHeight = height;

    if (!mPixmap)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create %d x %d pixmap.", width, height);
	return false;
    }

    surface = cairo_xlib_surface_create_with_xrender_format (dpy,
							     mPixmap,
							     scrn,
							     format,
							     width,
							     height);

    if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("text", CompLogLevelError, "Couldn't create surface.");
	return false;
    }

    cr = cairo_create (surface);
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create cairo context.");
	return false;
    }

    return true;
}

bool
TextSurface::update (int width,
		     int height)
{
    Display *dpy = screen->dpy ();

    cairo_surface_destroy (surface);
    surface = NULL;

    cairo_destroy (cr);
    cr = NULL;

    XFreePixmap (dpy, mPixmap);
    mPixmap = None;

    return initCairo (width, height);
}

bool
TextSurface::render (const CompText::Attrib &attrib,
		     const CompString       &text)
{
    int width, height, layoutWidth;

    if (!valid ())
	return false;

    pango_font_description_set_family (font, attrib.family);
    pango_font_description_set_absolute_size (font,
					      attrib.size * PANGO_SCALE);
    pango_font_description_set_style (font, PANGO_STYLE_NORMAL);

    if (attrib.flags & CompText::StyleBold)
	pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);

    if (attrib.flags & CompText::StyleItalic)
	pango_font_description_set_style (font, PANGO_STYLE_ITALIC);

    pango_layout_set_font_description (layout, font);

    if (attrib.flags & CompText::Ellipsized)
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

    pango_layout_set_auto_dir (layout, false);
    pango_layout_set_text (layout, text.c_str (), -1);

    pango_layout_get_pixel_size (layout, &width, &height);

    if (attrib.flags & CompText::WithBackground)
    {
	width  += 2 * attrib.bgHMargin;
	height += 2 * attrib.bgVMargin;
    }

    width  = MIN (attrib.maxWidth, width);
    height = MIN (attrib.maxHeight, height);

    /* update the size of the pango layout */
    layoutWidth = attrib.maxWidth;
    if (attrib.flags & CompText::WithBackground)
	layoutWidth -= 2 * attrib.bgHMargin;

    pango_layout_set_width (layout, layoutWidth * PANGO_SCALE);

    if (!update (width, height))
	return false;

    pango_cairo_update_layout (cr, layout);

    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    if (attrib.flags & CompText::WithBackground)
    {
	drawBackground (0, 0, width, height,
			MIN (attrib.bgHMargin, attrib.bgVMargin));
	cairo_set_source_rgba (cr,
			       attrib.bgColor[0] / 65535.0,
			       attrib.bgColor[1] / 65535.0,
			       attrib.bgColor[2] / 65535.0,
			       attrib.bgColor[3] / 65535.0);
	cairo_fill (cr);
	cairo_move_to (cr, attrib.bgHMargin, attrib.bgVMargin);
    }

    cairo_set_source_rgba (cr,
			   attrib.color[0] / 65535.0,
			   attrib.color[1] / 65535.0,
			   attrib.color[2] / 65535.0,
			   attrib.color[3] / 65535.0);

    pango_cairo_show_layout (cr, layout);

    return true;
}

bool
TextSurface::valid () const
{
    return scrn && format && layout && font &&
	   cr && cairo_status (cr) == CAIRO_STATUS_SUCCESS &&
	   surface && cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS;
}

TextSurface::TextSurface () :
    mWidth  (0),
    mHeight (0),
    mPixmap (None),
    cr (NULL),
    surface (NULL),
    layout (NULL),
    format (NULL),
    font (NULL),
    scrn (NULL)
{
    Display *dpy = screen->dpy ();

    scrn = ScreenOfDisplay (dpy, screen->screenNum ());

    if (!scrn)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't get screen for %d.", screen->screenNum ());
	return;
    }

    format = XRenderFindStandardFormat (dpy, PictStandardARGB32);
    if (!format)
    {
	compLogMessage ("text", CompLogLevelError, "Couldn't get format.");
	return;
    }

    if (!initCairo (1, 1))
	return;

    /* init pango */
    layout = pango_cairo_create_layout (cr);
    if (!layout)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create pango layout.");
	return;
    }

    font = pango_font_description_new ();
    if (!font)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create font description.");
	return;
    }
}

TextSurface::~TextSurface ()
{
    if (layout)
	g_object_unref (layout);
    if (surface)
	cairo_surface_destroy (surface);
    if (cr)
	cairo_destroy (cr);
    if (font)
	pango_font_description_free (font);
}

void
CompText::clear ()
{
    if (pixmap)
	XFreePixmap (screen->dpy (), pixmap);

    width  = 0;
    height = 0;
}

bool
CompText::renderText (CompString   text,
		      const Attrib &attrib)
{
    TextSurface surface;
    bool        retval = false;

    TEXT_SCREEN (screen);

    if (!ts)
	return false;

    if (!surface.valid ())
	return false;

    if (!(attrib.flags & NoAutoBinding) && !ts->gScreen)
	return false;

    if (surface.render (attrib, text))
    {
	if (!(attrib.flags & NoAutoBinding))
	{
	    texture = GLTexture::bindPixmapToTexture (surface.mPixmap,
						      surface.mWidth,
						      surface.mHeight,
						      32);
	    retval  = !texture.empty ();
	}
	else
	{
	    retval = true;
	}
    }

    if (!retval && surface.mPixmap)
    {
	XFreePixmap (screen->dpy (), surface.mPixmap);
	return retval;
    }

    clear ();

    pixmap = surface.mPixmap;
    width  = surface.mWidth;
    height = surface.mHeight;

    return retval;
}

bool
CompText::renderWindowTitle (Window               window,
		             bool                 withViewportNumber,
		             const CompText::Attrib &attrib)
{
    CompString text;

    TEXT_SCREEN (screen);

    if (!ts)
	return false;

    if (withViewportNumber)
    {
	CompString title;
    	CompPoint  winViewport;
	CompSize   viewportSize;

	title = ts->getWindowName (window);
	if (!title.empty ())
	{
	    CompWindow *w;

	    w = screen->findWindow (window);
	    if (w)
	    {
		int viewport;

		winViewport  = w->defaultViewport ();
		viewportSize = screen->vpSize ();
		viewport = winViewport.y () * viewportSize.width () +
		           winViewport.x () + 1;
		text = compPrintf ("%s -[%d]-", title.c_str (), viewport);
	    }
	    else
	    {
		text = title;
	    }
	}
    }
    else
    {
	text = ts->getWindowName (window);
    }

    if (text.empty ())
	return false;

    return renderText (text, attrib);
}

Pixmap
CompText::getPixmap ()
{
    Pixmap retval = None;

    if (texture.empty ())
    {
	retval = pixmap;
	pixmap = None;
    }

    return retval;
}

int
CompText::getWidth () const
{
    return width;
}

int
CompText::getHeight () const
{
    return height;
}

void
CompText::draw (float x,
	        float y,
	        float alpha) const
{
    GLboolean  wasBlend;
    GLint      oldBlendSrc, oldBlendDst;

    if (texture.empty ())
	return;

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    wasBlend = glIsEnabled (GL_BLEND);
    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (alpha, alpha, alpha, alpha);

    for (unsigned int i = 0; i < texture.size (); i++)
    {
	GLTexture         *tex = texture[i];
	GLTexture::Matrix m = tex->matrix ();

	tex->enable (GLTexture::Good);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (x, y - height);
	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x + width, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (x + width, y - height);

	glEnd ();

	tex->disable ();
    }

    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

CompText::CompText () :
    width (0),
    height (0),
    pixmap (None)
{
}

CompText::~CompText ()
{
    if (pixmap)
	XFreePixmap (screen->dpy (), pixmap);
}

PrivateTextScreen::PrivateTextScreen (CompScreen *screen) :
    PluginClassHandler <PrivateTextScreen, CompScreen, COMPIZ_TEXT_ABI> (screen),
    gScreen (GLScreen::get (screen))
{
    visibleNameAtom = XInternAtom (screen->dpy (), "_NET_WM_VISIBLE_NAME", 0);
    utf8StringAtom = XInternAtom (screen->dpy (), "UTF8_STRING", 0);
    wmNameAtom = XInternAtom (screen->dpy (), "_NET_WM_NAME", 0);
}

PrivateTextScreen::~PrivateTextScreen ()
{
}

bool
TextPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    CompPrivate p;
    p.uval = COMPIZ_TEXT_ABI;
    screen->storeValue ("text_ABI", p);

    return true;
}

void
TextPluginVTable::fini ()
{
    screen->eraseValue ("text_ABI");
}