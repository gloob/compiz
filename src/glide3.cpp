#if 0
/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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
 */

#include "private.h"

// =====================  Effect: Slab  =========================

bool
Glide3Anim::Glide3Anim (CompWindow *w,
                          WindowEvent curWindowEvent,
                          float duration,
                          const AnimEffect info,
                          const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, kDurationFactor * duration, info,
			  icon),
    PolygonAnim::PolygonAnim (w, curWindowEvent, kDurationFactor * duration,
                              info, icon)
{
    //if (!polygonsAnimInit (w))
	//return false;

    CompScreen *s = w->screen;
    ANIMADDON_WINDOW (w);

    float finalDistFac = animGetF (w, ANIMADDON_SCREEN_OPTION_GLIDE3_AWAY_POS);
    float finalRotAng = animGetF (w, ANIMADDON_SCREEN_OPTION_GLIDE3_AWAY_ANGLE);
    float thickness = animGetF (w, ANIMADDON_SCREEN_OPTION_GLIDE3_THICKNESS);

    PolygonSet *pset = aw->eng.polygonSet;

    mIncludeShadows = (thickness < 1e-5);

    if (!tessellateIntoRectangles (w, 1, 1, thickness))
	return false;

    PolygonObject *p = mPolygons;

    int i;

    for (i = 0; i < mNPolygons; i++, p++)
    {
	p->rotAxis.x = 1;
	p->rotAxis.y = 0;
	p->rotAxis.z = 0;

	p->finalRelPos.x = 0;
	p->finalRelPos.y = 0;
	p->finalRelPos.z = finalDistFac * 0.8 * DEFAULT_Z_CAMERA * s->width;

	p->finalRotAng = finalRotAng;
    }
    mAllFadeDuration = 1.0f;
    mBackAndSidesFadeDur = 0.2f;
    mDoLighting = true;
    mCorrectPerspective = CorrectPerspectivePolygon;

    return true;
}
#endif
