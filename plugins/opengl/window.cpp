#include "privates.h"

GLWindow::GLWindow (CompWindow *w) :
    OpenGLPrivateHandler<GLWindow, CompWindow, COMPIZ_OPENGL_ABI> (w),
    priv (new PrivateGLWindow (w, this))
{
    WRAPABLE_INIT_HND(glPaint);
    WRAPABLE_INIT_HND(glDraw);
    WRAPABLE_INIT_HND(glAddGeometry);
    WRAPABLE_INIT_HND(glDrawTexture);
    WRAPABLE_INIT_HND(glDrawGeometry);

    priv->clip = XCreateRegion ();
    assert (priv->clip);

    CompositeWindow *cw = CompositeWindow::get (w);
	
    priv->paint.opacity    = cw->opacity ();
    priv->paint.brightness = cw->brightness ();
    priv->paint.saturation = cw->saturation ();

    priv->lastPaint = priv->paint;

}

GLWindow::~GLWindow ()
{
    delete priv;
}

PrivateGLWindow::PrivateGLWindow (CompWindow *w,
				  GLWindow   *gw) :
    window (w),
    gWindow (gw),
    cWindow (CompositeWindow::get (w)),
    screen (w->screen ()),
    gScreen (GLScreen::get (screen)),
    texture (screen),
    clip (0),
    bindFailed (false),
    vertices (0),
    vertexSize (0),
    vertexStride (0),
    indices (0),
    indexSize (0),
    vCount (0),
    texUnits (0),
    texCoordSize (2),
    indexCount (0)
{
    paint.xScale	= 1.0f;
    paint.yScale	= 1.0f;
    paint.xTranslate	= 0.0f;
    paint.yTranslate	= 0.0f;

    w->add (this);
    WindowInterface::setHandler (w);

    cWindow->add (this);
    CompositeWindowInterface::setHandler (cWindow);
}

PrivateGLWindow::~PrivateGLWindow ()
{
	
    if (clip)
	XDestroyRegion (clip);
		if (vertices)
	free (vertices);

    if (indices)
	free (indices);
}

void
PrivateGLWindow::setWindowMatrix ()
{
    matrix = texture.matrix ();
    matrix.x0 -= (window->attrib ().x * matrix.xx);
    matrix.y0 -= (window->attrib ().y * matrix.yy);
}

bool
GLWindow::bind ()
{

    if (!priv->cWindow->pixmap () && !priv->cWindow->bind ())
	return false;

    if (!priv->texture.bindPixmap (priv->cWindow->pixmap (),
				   priv->window->width (),
				   priv->window->height (),
				   priv->window->attrib ().depth))
    {
	compLogMessage (priv->screen->display (), "opengl", CompLogLevelInfo,
			"Couldn't bind redirected window 0x%x to "
			"texture\n", (int) priv->window->id ());
    }

    priv->setWindowMatrix ();

    return true;
}

void
GLWindow::release ()
{
    if (priv->cWindow->pixmap ())
    {
	priv->texture = GLTexture (priv->screen);
	priv->cWindow->release ();
    }
}

GLWindowInterface::GLWindowInterface ()
{
    WRAPABLE_INIT_FUNC(glPaint);
    WRAPABLE_INIT_FUNC(glDraw);
    WRAPABLE_INIT_FUNC(glAddGeometry);
    WRAPABLE_INIT_FUNC(glDrawTexture);
    WRAPABLE_INIT_FUNC(glDrawGeometry);

}


bool
GLWindowInterface::glPaint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix            &transform,
			    Region                    region,
			    unsigned int              mask)
    WRAPABLE_DEF_FUNC_RETURN(glPaint, attrib, transform, region, mask)

bool
GLWindowInterface::glDraw (const GLMatrix     &transform,
			   GLFragment::Attrib &fragment,
			   Region             region,
			   unsigned int       mask)
    WRAPABLE_DEF_FUNC_RETURN(glDraw, transform, fragment, region, mask)

void
GLWindowInterface::glAddGeometry (GLTexture::Matrix *matrix,
				  int	            nMatrix,
				  Region	    region,
				  Region	    clip)
    WRAPABLE_DEF_FUNC(glAddGeometry, matrix, nMatrix, region, clip)

void
GLWindowInterface::glDrawTexture (GLTexture          *texture,
				  GLFragment::Attrib &fragment,
				  unsigned int       mask)
    WRAPABLE_DEF_FUNC(glDrawTexture, texture, fragment, mask)

void
GLWindowInterface::glDrawGeometry ()
    WRAPABLE_DEF_FUNC(glDrawGeometry)

Region
GLWindow::clip ()
{
    return priv->clip;
}

GLWindowPaintAttrib &
GLWindow::paintAttrib ()
{
    return priv->paint;
}

void
PrivateGLWindow::resizeNotify (int dx, int dy, int dwidth, int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);
    setWindowMatrix ();
    gWindow->release ();
}

void
PrivateGLWindow::moveNotify (int dx, int dy, bool now)
{
    window->moveNotify (dx, dy, now);
    setWindowMatrix ();
}

void
PrivateGLWindow::windowNotify (CompWindowNotify n)
{
    switch (n)
    {
	case CompWindowNotifyUnmap:
	    gWindow->release ();
	    break;
	case CompWindowNotifyAliveChanged:
	    gWindow->updatePaintAttribs ();
	    break;
	default:
	    break;
	
    }

    window->windowNotify (n);
}

void
GLWindow::updatePaintAttribs ()
{
    CompositeWindow *cw = CompositeWindow::get (priv->window);

    if (priv->window->alive ())
    {
	priv->paint.opacity    = cw->opacity ();
	priv->paint.brightness = cw->brightness ();
	priv->paint.saturation = cw->saturation ();
    }
    else
    {
	priv->paint.opacity    = cw->opacity ();
	priv->paint.brightness = 0xa8a8;
	priv->paint.saturation = 0;
    }
}

bool
PrivateGLWindow::damageRect (bool initial, BoxPtr box)
{
    texture.damage ();
    return cWindow->damageRect (initial, box);
}
